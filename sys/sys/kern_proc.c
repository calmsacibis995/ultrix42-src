#ifndef lint
static	char	*sccsid = "@(#)kern_proc.c	4.1	(ULTRIX)	7/2/90";
#endif

/************************************************************************
 *									*
 *			Copyright (c) 1986, 1987 by			*
 *		Digital Equipment Corporation, Maynard, MA		*
 *			All rights reserved.				*
 *									*
 *   This software is furnished under a license and may be used and	*
 *   copied  only  in accordance with the terms of such license and	*
 *   with the  inclusion  of  the  above  copyright  notice.   This	*
 *   software  or  any  other copies thereof may not be provided or	*
 *   otherwise made available to any other person.  No title to and	*
 *   ownership of the software is hereby transferred.			*
 *									*
 *   This software is  derived  from  software  received  from  the	*
 *   University    of   California,   Berkeley,   and   from   Bell	*
 *   Laboratories.  Use, duplication, or disclosure is  subject  to	*
 *   restrictions  under  license  agreements  with  University  of	*
 *   California and with AT&T.						*
 *									*
 *   The information in this software is subject to change  without	*
 *   notice  and should not be construed as a commitment by Digital	*
 *   Equipment Corporation.						*
 *									*
 *   Digital assumes no responsibility for the use  or  reliability	*
 *   of its software on equipment which is not supplied by Digital.	*
 *									*
 ************************************************************************/

/* ------------------------------------------------------------------------
 * Modification History: /sys/sys/kern_proc.c
 *
 * 05 Feb 90 -- dws
 *	Added pg_orphan() and pg_stopped().
 *
 * 08 Aug 88 -- Tim Burke
 *	Modified proc_search to return an error indication for POSIX programs
 *	when trying to set their process group to a non-existant group.
 *
 * 07 Jun 88 -- miche
 *	SMP procqs:  lk_pid is for process id's and their hashing.
 *	NOTE:  in SMP pfind() is ONLY A HINT
 *
 * 22 Apr 88 -- miche
 *	protect p_sig reference in spgrp for mp.
 *
 * 22 Dec 87 -- Tim Burke
 *	Added proc_search.  This is a POSIX routine which will search the
 * 	proc table to see if any other processes exist in the same process
 *	group with the same controling tty.
 *
 * 09 Sep 85 -- Reilly
 *	Modified to handle the new 4.3BSD namei code.
 *
 * 25 Oct 84 -- jrs
 *	Add support for linked proc queues
 *	Derived from 4.2BSD, labeled:
 *		kern_proc.c 6.2	84/05/22
 *
 * -----------------------------------------------------------------------
 */

#include "../machine/reg.h"
#include "../machine/pte.h"
#include "../machine/psl.h"

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/map.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/kernel.h"
#include "../h/proc.h"
#include "../h/buf.h"
#include "../h/gnode.h"
#include "../h/seg.h"
#include "../h/acct.h"
#include "../h/wait.h"
#include "../h/vm.h"
#include "../h/text.h"
#include "../h/file.h"
#include "../h/quota.h"
#include "../h/uio.h"
#include "../h/mbuf.h"
#include "../h/exec.h"


/*
 * Since all children, however indirect, of this process no longer
 * have a terminal it makes not sense for them to have outstanding
 * tty signals.  They can't get any more signals in the future because
 * close(tty) was already called in exit, and the ttyclose routine
 * clears out the pgrp field in the terminal structure.

 * I have taken real liberties with this:  the 4.3 version walks
 * through child and sibling chains, and the 4.2 version is just a
 * broken mess.  We don't like the chains because they're slow.  The
 * purpose is just to ensure that all our descendents no longer expect
 * to use this pgrp:  4.3 clears your pgrp even if you have changed it,
 * which makes no sense.  So, we just walk through looking for all who
 * are using this pgrp, (and hence have this controlling terminal), and
 * clear them out.
 */
spgrp(pgrp)
	int pgrp;
{
	int s;
	FORALLPROC(
		if ((pp->p_pgrp == pgrp) &&
		    (pp->p_sig & (SIGTSTP|SIGTTIN|SIGTTOU))) {
			s = spl5();
			smp_lock(&lk_signal, LK_RETRY);
			pp->p_sig &=
			    ~(sigmask(SIGTSTP)|sigmask(SIGTTIN)|sigmask(SIGTTOU));
			smp_unlock(&lk_signal);
			splx(s);
		}	
	)
}

/*
 * Is p an inferior of the current process?
 * SMP: make sure p_pptr hasn't been cleared.
 */
inferior(p)
	register struct proc *p;
{

	for (; p != u.u_procp; p = p->p_pptr)
		if ((p == NULL) || (p->p_ppid == 0))
			return (0);
	return (1);
}

/*
 * This routine returns only a HINT - the caller must
 * ensure that the process returned is still the one
 * desired at the point it is used
 * If you want the process to hold still, use proc_get(pid),
 * which returns a "ref'ed" process.  See kern_psubr.c
 */
struct proc *
pfind(pid)
	int pid;
{
	register struct proc *p;
	int s;

	s = spl5();
	smp_lock(&lk_pid, LK_RETRY);
	for (p = &proc[pidhash[PIDHASH(pid)]]; p != &proc[0]; p = &proc[p->p_idhash])
		if (p->p_pid == pid) 
			break;
	smp_unlock(&lk_pid);
	(void) splx(s);
	if (p->p_pid == pid)
		return (p);
	else	return ((struct proc *)0);
}

/*
 * initialize the process table.
 * There is no queue, but all the status bits need to be
 * marked SIDL so they will be correct when first picked up.
 * proc[0] (the swapper) already has had its state to SRUN
 * No SMP locking is needed here because this is only run during boot.
 */
pqinit()
{
	register struct proc *p;
	for (p = &proc[1] ; p < procNPROC ; p++)
		p->p_stat = SIDL;
}


/*
 *	proc_search
 *
 *	Function: Search proc table to see if another process exists that has
 *		  the same process group and controling tty.
 *
 *	Inputs:	  new_pgrp -  This integer is the process group that will be
 *			      searched for in the proc table.
 *
 *	Returns: 0 - If another process exists in the same process group with
 *		     the same controling tty.  Search has suceeded.
 *		 1 - If no other process has both the same controling tty and
 *		     the same process group.  Search has failed.
 */

proc_search(new_pgrp)
	int new_pgrp;
{
	register struct proc *pp;
	register int other_pgrp_members = 0;
	/*
	 * we do no smp locking here because
	 *	- if we ever find it safe, then in a single
	 *	  processor, there was a point at which it was safe
	 *	- if we find it not safe, there was a point in
	 *	  a single processor in which it was not safe.
	 */

	/*
	 * As a guess of where to begin the linear search, see if the original
	 * process that grabbed the tty still exists.  If so see if its
	 * controlling terminal is the same as ours.  If not then search
	 * the whole table.
	 */

	pp = pfind(new_pgrp);
	if (pp && pp->p_pgrp == new_pgrp)
		if (pp->p_ttyp == u.u_procp->p_ttyp) 
			return(0);

	FORALLPROC (	/* pp is reset in this loop */
		if (pp->p_pgrp == new_pgrp) {
			other_pgrp_members = 1;
			if (pp->p_ttyp == u.u_procp->p_ttyp) 
				return(0);
		}
	) /* end FORALLPROC */

	/*
	 * At this point it is evident that no process exists in the same
	 * process group with the same controlling terminal.  If there are
	 * other processes in this group then don't allow the change of 
	 * process group by returning 1.  A POSIX program is only allowed to
	 * change process groups to an already existing group.  The final
	 * return of 0 is for non-POSIX programs which are trying to set their
	 * process group to a new group.
	 */
	if ((other_pgrp_members) || (u.u_procp->p_progenv == A_POSIX))
		return(1); 	/* Search Failed */
	return(0);	/* Success, nobody else in pgrp */
	
}

/*
 * Is this an orphan process group.
 */
pg_orphan(pgrp, psid)
int pgrp;
int psid;
{
	register struct proc *p;
	register int s;

	/* invalid process group */	
	if (pgrp == 0) 
		return(0);

	/* process group leader has terminated */
	if ((p = proc_get(pgrp)) == (struct proc *)0)
		return(1);

	/* parent of process group leader has terminated */
	if (p->p_pptr == &proc[1] || pfind(p->p_ppid) == (struct proc *)0) {
		proc_rele(p);
		return(1);
	}
	proc_rele(p);

	FORALLPROC(
		if (pp->p_pgrp == pgrp) {
			s = splclock();
			smp_lock(&lk_procqs, LK_RETRY);
  	  		if (pp->p_pgrp == pgrp)  {
				if (pp->p_pptr == (struct proc *)0) {
					smp_unlock(&lk_procqs);
					splx(s);
					NEXTPROC;
				}
				if (!(pp->p_pptr->p_pgrp == pgrp || 
				      pp->p_pptr->p_sid != psid)) {
					smp_unlock(&lk_procqs);
					splx(s);
					return(0);
				}
			}
			smp_unlock(&lk_procqs);
			splx(s);
		}
	)
        return(1);
}

/*
 * Are any members of the process group stopped 
 */
pg_stopped(pgrp)
pid_t pgrp;
{
	register int s;

	FORALLPROC(
		if (pp->p_pgrp == pgrp) {
			s = splclock();
			smp_lock(&lk_procqs, LK_RETRY);
			if (pp->p_pgrp == pgrp && pp->p_stat == SSTOP) {
				smp_unlock(&lk_procqs);
				splx(s);
				return(1);
			}
			smp_unlock(&lk_procqs);
			splx(s);
		}
	)
	return(0);
}
