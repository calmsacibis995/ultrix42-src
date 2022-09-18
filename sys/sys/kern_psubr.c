#ifndef lint
static char *sccsid = "@(#)kern_psubr.c	4.1    ULTRIX  7/2/90";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1988 by				*
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
 * Modification History: /sys/sys/kern_psubr.c
 *
 * 20-Jul-89 jaw
 *	clean-up and eliminate alot of debug code.  
 *
 * 12 Sep 88 -- miche
 *	ref'ing extended to differentiate between ALIVE: when
 * 	anyone can ref you, DYING, when you cannot be reffed
 *	AND your parent cannot free your process slot, DEAD,
 *	when you are well and truly gone.  Coodination betwee
 *	exit and wait.
 *
 * 23 Aug 88 -- miche
 *	file created: routines for ref'ing, get'ing, del'ing and
 *	rele'sing a process structure: guarantees it won't exist,
 *	which also means the slot can't be used by someone else.
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
#include "../h/smp_lock.h"
#include "../h/cpudata.h"


int
proc_ref(p)
	register struct proc *p;
{

	if (p->p_exist == P_ALIVE) {
		p->p_ref++;
		return(1);
	} else {
		return(0);
	}
}

int
parent_ref()
{
	int rv, s = spl5();
	smp_lock(&lk_procqs, LK_RETRY);
	rv = proc_ref(u.u_procp->p_pptr);
	smp_unlock(&lk_procqs);
	splx(s);
	return(rv);
}

struct proc *
proc_get(pid)
	int pid;
{
	register struct proc *p, *rv;
	register int s;

	rv = (struct proc *)NULL;

	if (p = pfind(pid)) {
		s = spl5();
		smp_lock(&lk_procqs, LK_RETRY);
		if ((p->p_pid == pid) && proc_ref(p)) {
			rv = p;
		}
		smp_unlock(&lk_procqs);
		splx(s);
	}

	return(rv);
}

void
proc_del()
{
	register struct proc *p = u.u_procp;
	register int s;

	s = spl5();
	smp_lock(&lk_procqs, LK_RETRY);

	if (p->p_exist != P_ALIVE)
		panic("proc_del: not alive state");
	p->p_exist = P_DYING;
	if (p->p_ref != 0) {
		sleep_unlock(&p->p_ref, PZERO-1, &lk_procqs);
		if (p->p_ref != 0) {
			panic("proc_del: bad ref");
		}
		splx(s);
		return;
	}
	smp_unlock(&lk_procqs);
	splx(s);
}

void
proc_rele(p)
	register struct proc *p;
{
	register int s;
	int need_wakeup=0;

	s = spl5();
	smp_lock(&lk_procqs, LK_RETRY);
	if (--p->p_ref < 0) {
		panic("proc_rele: bad ref");
	}

	if (p->p_ref == 0 && p->p_exist == P_DYING) {
		need_wakeup = 1;
	}
	smp_unlock(&lk_procqs);
	splx(s);
	if (need_wakeup) {
		wakeup(&p->p_ref);
	}
}

/*
 * called as the VERY LAST thing a process does - tells
 * the parent it is ok to trounce its resources.
 */
void
proc_exit(p)
	register struct proc *p;	/* p is ME!!! */
{
	int s;


	s = spl5();
	smp_lock(&lk_procqs, LK_RETRY);

	if (p->p_exist != P_DYING)
		panic("proc_exit: not dying state");

	p->p_exist = P_DEAD;
	smp_unlock(&lk_procqs);
	splx(s);
	if (CURRENT_CPUDATA->cpu_hlock) {
		panic("proc_exit: holding a lock");
	}
	wakeup((caddr_t)p);
}

/*
 * This is called in wait1() by the parent when it has found a process
 * in the zombie state.  This coordinates between exit and wait, from
 * the time the child has marked itself as ZOMBie, and when it has
 * actually freed its virtual memory.
 * This sleep should be very short: child is exiting at this moment.
 */
proc_wait(p)
	register struct proc *p;	/* p is the ZOMBie child */
{

	while (p->p_exist != P_DEAD) {
		if (p->p_exist == P_ALIVE)
			panic("proc_wait: waiting on live process");
		sleep_unlock((caddr_t)p, PZERO-1, &lk_procqs);
		smp_lock(&lk_procqs, LK_RETRY);
	}
	return(1);
}
