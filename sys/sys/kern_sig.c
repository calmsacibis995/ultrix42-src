#ifndef lint
static	char	*sccsid = "@(#)kern_sig.c	4.5	(ULTRIX)	4/11/91";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1985,86,87,88 by			*
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
 * Modification History: /sys/sys/kern_sig.c
 *
 * 11-Apr-91	dlh
 * 	core():
 * 		Allow core dump of vector context on VAX 9000.
 *
 * 09-Oct-90	jaw
 *	merge in mm changes
 *
 * 4-Sep-90	dlh
 *	added vector processor support code - output vpcontext area during
 *	core()
 *
 * 10-May-90 jmartin
 *	If multiple signals are pending, take SIGKILL first.  Thus, if
 *	the SIGKILL came from exec or swap, we avoid dispatching to a
 *	handler in nonexistent user address space.
 *
 * 03-Mar-90 jaw
 *	fix missed wakeup if stop signal is recieved as a process
 *	is being put to sleep.
 *
 * 05 Feb 90 -- dws
 *	Various POSIX fixes including changes to the sigperms macro,
 *	SIGSTOP|SIGKILL handling in sigvec(), SNOCLDSTP handling in
 *	setsigvec(), SIGCONT handling in kill(), killable processes
 *	in killpg1(), and stopping processes in orphaned process groups
 *	in psignal().
 *
 * 11 dec 89 -- robin
 *	Fixed a problem that stops core files from being created
 *	if a user level program seg faults.  The u.u_error must be
 *	set to zero before calling access in the core routine.  Access
 *	depends on the error field being zero.
 *
 * 08-Dec-89 gmm
 *      Unlock all smp_locks in psig() before exiting 
 *
 * 09-Nov-89
 *	allow spin locks to be compiled out
 *
 * 24-Aug-89 jaw
 *	missing splx in issig();
 *
 * 20-Jul-89 jaw
 *	fix hang when vfork gets stop signal.
 *
 *  19-Jul-89   gg
 *	fix SIGCONT to perform the default action if the user specified
 *	handler is SIG_IGN (POSIX requirement).
 *
 *  12-May-89	jaw
 *	fix to killpg for SYSV and POSIX environments.
 *
 *  28-Feb-89	jaw
 *	fix hole in psignal where process state was not be held still while
 *	delivering the signal.
 *
 *  26-Jan-89	jaw
 *	SMP clean up 
 *
 * 22 Nov 88 -- jaw 
 *	fix case where we were sending signal to our parent before we
 *	were REALLY stopped....  more magic.
 *
 * 22 Sep 88 -- jaw 
 *	Remove primary bop 
 *
 * 23 Aug 88 -- miche
 *	Add support for ref'ing a process.
 *
 * 18-Jun-88 -- jaw  change to new cpu data format.
 *
 * 07 Jun 88 - miche
 *	SMP procqs.
 *
 * 22 Apr 88 - miche
 *	Added mp locking for the signal subsystem.  The fields p_sig
 *	and p_cursig require locking, unless we are sure no one else
 *	can be playing with these (e.g., cursig when we are running,
 *	exit, fork, wait)  sigmask, sigignore and sigcatch are private:
 *	they are unprotected because the worse that can happen is that
 *	someone reads a value that is about to change.
 *
 * 10 Feb 88 - chet
 *	Added gnode lock around access() call in core for synchronization.
 *
 * 27 Jan 88 -  gmm
 *	Changed intrcpu() to intrpt_cpu() to conform to the new IPI interface
 *************SMP changes above *********
 *
 * 23 Nov 88 -- prs
 *	Fixed gfs_namei interface in core() to use a MAXPATHLEN buffer.
 *	All calls to gfs_namei() need a buffer of MAXPATHLEN bytes
 *	in length when symbolic links are to be followed.
 * 
 * 01-Sep-88 -- jaw
 * 	Move SMASTER flag from p_flag to p_master field.  This change is
 *	needed so that both the master and slave don't write the p_flag
 * 	field at the same time.
 *
 * 01-Sep-88 -- map
 *	Allow SIGCONT regardless of permissions if in POSIX mode and 
 *	target process is in same session.
 *
 * 11-Jul-88 -- map
 *	Only check for OLDSIG flag in System V mode as other modes allow
 *	"reliable" signal() function.
 *
 * 07-Jun-88 -- map
 *	If sigvec() called with nsv = NULL then must succeed even if
 *	signal cannot be blocked or ignored (SIGKILL or SIGSTOP).
 *
 * 02-Jun-88 -- map
 *	Changed behavior of ignored, blocked signals back to original
 *	BSD behavior (they are discarded) now that POSIX allows it.
 *	Allow SIGCONT to be blocked/ignored and to continue process
 *	even if blocked/ignored.  This is a POSIX requirement.
 *
 * 08-Mar-88 -- map
 *	Fixed return statement in killpg1(). It was returning 0 instead
 *	of ESRCH in case where there were no processes to kill.
 *
 * 05-Feb-88 -- map
 *	Fixed problem with kill(-1) and kill(0).  BSD4.3 changed semantics
 *	which broke Ultrix compatibility.
 *
 * 04-Oct-87 -- map
 *	Changed kill/killpg to 4.3 version and added POSIX behavior
 *
 * 28-Sep-87 -- map
 *	Added sigpending() system call.
 *
 * 27 Mar-86 -- jaw 
 *	bug in ASMP code.  Jeff allowed the slave to execute "issig" which
 *	causes signals to be dropped.  This shows up in run adb on a
 *	program.  The fix is to switch to the master on entry into issig.
 *
 * 11 Sep 86 -- koehler 
 *	changed the namei interface
 *
 * 18 Mar 86 -- jrs
 *	Clean up cpu determination and preemption
 *
 * 16 Jul 85 -- jrs
 *	Add run queue locking and multicpu sched code
 *
 * 09 Sep 85 -- Reilly
 *	Modified to handle the new 4.3BSD namei code.
 *
 * 25 Oct 84 -- jrs
 *	Add changes for proc queues and
 *	security fix for non-read text segments
 *	Derived from 4.2BSD, labeled:
 *		kern_sig.c 6.3	84/05/22
 *
 * 10/1/85 -- Larry Cohen
 *	Add SIGWINCH signal to list of signals that dont go propogated.
 * -----------------------------------------------------------------------
 */

#include "../machine/reg.h"
#include "../machine/pte.h"
#include "../machine/psl.h"

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/gnode.h"
#include "../h/proc.h"
#include "../h/exec.h"
#include "../h/timeb.h"
#include "../h/times.h"
#include "../h/conf.h"
#include "../h/buf.h"
#include "../h/mount.h"
#include "../h/text.h"
#include "../h/seg.h"
#include "../h/vm.h"
#include "../h/acct.h"
#include "../h/uio.h"
#include "../h/kernel.h"
#include "../h/cpudata.h"
#include "../h/kmalloc.h"
#ifdef vax
#include "../machine/mtpr.h"
#include "../machine/vectors.h"
#endif vax
#include "../../machine/common/cpuconf.h"
extern int smp;
extern int sigpause_priority_mod;
extern int sigpause_priority_limit;
extern int psignal_priority_mod;

#define	cantmask	(sigmask(SIGKILL)|sigmask(SIGSTOP))
#define	stopsigmask	(sigmask(SIGSTOP)|sigmask(SIGTSTP)| \
			 sigmask(SIGTTIN)|sigmask(SIGTTOU))
#define	dflignmask	(sigmask(SIGCHLD)|sigmask(SIGIO)|sigmask(SIGURG)|\
			 sigmask(SIGWINCH))
/*
 * POSIX wants:
 *	sender_euid == 0 || sender_euid = rec_ruid || sender_ruid = rec_ruid ||
 *      sender_euid == rec_suid || sender_ruid == rec_suid
 */
#define sigperms(ptr) (u.u_uid == 0 ||		\
		 u.u_uid == (ptr)->p_uid ||	\
		 u.u_ruid == (ptr)->p_uid ||	\
		 u.u_uid == (ptr)->p_suid  ||	\
	 	 u.u_ruid == (ptr)->p_suid	\
)


/*
 * Generalized interface signal handler.
 */
sigvec()
{
	/*
	 * This routine is only ever called on behalf of oneself
	 */
	register struct a {
		int	signo;
		struct	sigvec *nsv;
		struct	sigvec *osv;
#ifdef mips
		int	(*sigtramp)();
#endif mips
	} *uap = (struct a  *)u.u_ap;
	struct sigvec vec;
	register struct sigvec *sv;
	register int sig;
	register int bit;
	register struct proc *p;

	p = u.u_procp;
	sig = uap->signo;
	if (sig <= 0 || sig >= NSIG) {
		u.u_error = EINVAL;
		return;
	}

	if ((sig == SIGKILL || sig == SIGSTOP) && (p->p_progenv != A_POSIX)) {
		u.u_error = EINVAL;
		return;
	}

	sv = &vec;
	if (uap->osv) {
		sv->sv_handler = u.u_signal[sig];
		sv->sv_mask = u.u_sigmask[sig];
		bit = sigmask(sig);
		sv->sv_flags = 0;
		if ((u.u_sigonstack & bit) != 0)
			sv->sv_flags |= SV_ONSTACK;
		if ((u.u_sigintr & bit) != 0)
			sv->sv_flags |= SV_INTERRUPT;
		if ((u.u_oldsig & bit) != 0)
			sv->sv_flags |= SV_OLDSIG;
		if (p->p_sigflag & SNOCLDSTP)
			sv->sv_flags |= SA_NOCLDSTOP;
		u.u_error =
		    copyout((caddr_t)sv, (caddr_t)uap->osv, sizeof (vec));
		if (u.u_error)
			return;
	}

	if (uap->nsv) {
		u.u_error =
		    copyin((caddr_t)uap->nsv, (caddr_t)sv, sizeof (vec));
		if (u.u_error)
			return;
		if ( (sig == SIGKILL || sig == SIGSTOP) && (sv->sv_handler != SIG_DFL) ) {
			u.u_error = EINVAL;
			return;
		}
#ifdef mips
		/*
		 * check for unaligned pc on sighandler
		 */
		if (sv->sv_handler != SIG_IGN
		    && ((int)sv->sv_handler & (sizeof(int)-1))) {
			u.u_error = EINVAL;
			return;
		}
#endif mips

		setsigvec(sig, sv);
	}
#ifdef mips
	u.u_sigtramp = uap->sigtramp;
#endif mips

}

setsigvec(sig, sv)
	register int sig;
	register struct sigvec *sv;
{
	register struct proc *p;
	register int bit;

	bit = sigmask(sig);
	p = u.u_procp;
	/*
	 * Change setting atomically.
	 */
	(void) splhigh();
	u.u_signal[sig] = sv->sv_handler;
	u.u_sigmask[sig] = sv->sv_mask &~ cantmask;
	if (p->p_progenv == A_SYSV) {
		if (sv->sv_flags & SV_OLDSIG) {
			sv->sv_flags |= SV_INTERRUPT;
			u.u_oldsig |= bit; /* record as using old signal() */
		}
		else {
			u.u_oldsig &= ~bit;
		}
	}
	if (sv->sv_flags & SV_INTERRUPT)
                u.u_sigintr |= bit;
        else
                u.u_sigintr &= ~bit;
	if (sv->sv_flags & SV_ONSTACK)
		u.u_sigonstack |= bit;
	else
		u.u_sigonstack &= ~bit;
	if (p->p_progenv == A_SYSV) {
		if (sv->sv_flags & SV_OLDSIG)
			u.u_oldsig |= bit; /* record as using old signal() */
		else
			u.u_oldsig &= ~bit;
	}
	if (p->p_progenv == A_POSIX && sig == SIGCHLD) {
		if (sv->sv_flags & SA_NOCLDSTOP)
			p->p_sigflag |= SNOCLDSTP;
		else
			p->p_sigflag &= ~SNOCLDSTP;
	}
	if (sv->sv_handler == SIG_IGN) {
		smp_lock(&lk_signal, LK_RETRY);
		p->p_sig &= ~bit;		/* never to be seen again */
		p->p_sigignore |= bit;
		p->p_sigcatch &= ~bit;
		smp_unlock(&lk_signal);
	} else {
		p->p_sigignore &= ~bit;
		if (sv->sv_handler == SIG_DFL) {
			p->p_sigcatch &= ~bit;
			/*
			 * If setting a signal to SIG_DFL 
			 * whose default action is to ignore
			 * then discard signal now.
			 */
			if( bit & dflignmask ) {
				smp_lock(&lk_signal, LK_RETRY);
				p->p_sig &= ~bit;
				smp_unlock(&lk_signal);
			}
		}
		else {
			p->p_sigcatch |= bit;
		}
	}
	(void) spl0();
}

sigblock()
{
	struct a {
		int	mask;
	} *uap = (struct a *)u.u_ap;
	register struct proc *p = u.u_procp;

	(void) splhigh();
	u.u_r.r_val1 = p->p_sigmask;
	p->p_sigmask |= uap->mask &~ cantmask;
	(void) spl0();
}

sigsetmask()
{
	struct a {
		int	mask;
	} *uap = (struct a *)u.u_ap;
	register struct proc *p = u.u_procp;

	(void) splhigh();
	u.u_r.r_val1 = p->p_sigmask;
	p->p_sigmask = uap->mask &~ cantmask;
	(void) spl0();
}

sigpending()
{
	struct a {
		sigset_t	*set;
	} *uap = (struct a*)u.u_ap;
	register struct proc *p = u.u_procp;

	(void) splhigh();
	u.u_error = copyout((caddr_t)&p->p_sig, (caddr_t)uap->set, 
	    sizeof (sigset_t));
	(void) spl0();
		
}

sigpause()
{
	struct a {
		int	mask;
	} *uap = (struct a *)u.u_ap;
	register struct proc *p = u.u_procp;

	/*
	 * When returning from sigpause, we want
	 * the old mask to be restored after the
	 * signal handler has finished.  Thus, we
	 * save it here and mark the proc structure
	 * to indicate this (should be in u.).
	 */
	u.u_oldmask = p->p_sigmask;
	u.u_sigflag |= SOMASK;
	p->p_sigmask = uap->mask &~ cantmask;
	for (;;) {
		if (sigpause_priority_mod) {
			if (p->p_pri < sigpause_priority_limit)
				sleep((caddr_t)&u, sigpause_priority_limit);
			else
				sleep((caddr_t)&u, p->p_pri);
		} else
			sleep((caddr_t)&u, PSLEP);
	}
	/*NOTREACHED*/
}
#undef cantmask

sigstack()
{
	register struct a {
		struct	sigstack *nss;
		struct	sigstack *oss;
	} *uap = (struct a *)u.u_ap;
	struct sigstack ss;

	if (uap->oss) {
		u.u_error = copyout((caddr_t)&u.u_sigstack, (caddr_t)uap->oss, 
		    sizeof (struct sigstack));
		if (u.u_error)
			return;
	}
	if (uap->nss) {
		u.u_error =
		    copyin((caddr_t)uap->nss, (caddr_t)&ss, sizeof (ss));
		if (u.u_error == 0)
			u.u_sigstack = ss;
	}
}


kill()
{
	register struct a {
		int	pid;
		int	signo;
	} *uap = (struct a *)u.u_ap;
	register struct proc *p, *pp;
	register int f = 0;

	if (uap->signo < 0 || uap->signo > NSIG) {
		u.u_error = EINVAL;
		return;
	}
	if (uap->pid > 0) {
		/* kill single process */
		pp = u.u_procp;
		/* ref the proc so it does not exit on us */
		if ((p=proc_get(uap->pid))==0) {
			u.u_error = ESRCH;
			return;
		}
		if (u.u_uid == 0)
			f++;
		else 
		     if( u.u_uid == p->p_suid)
			f++;		/* Eff = Eff always o.k. */
		     else {
			/* If sending process is POSIX or SYS V then check */
			/* other possible matches.			   */
			if ((pp->p_progenv == A_POSIX) || 
				(pp->p_progenv == A_SYSV)) { 
				if ( (pp->p_progenv == A_POSIX && uap->signo == SIGCONT) ? 
				     (pp->p_sid == p->p_sid) : (sigperms(p)) ) {
				     /* Found a match. If receiving process is */
				     /* POSIX or SysV then all set.	       */
				     if ((p->p_progenv == A_POSIX) || 
					(p->p_progenv == A_SYSV)) { 
					 f++;
				     }
				     else 
					u.u_error = EPERM; /* No match */
				} 
				else 
					u.u_error = EPERM;

			}
			else
				u.u_error = EPERM;  /* No match */
		     }

		 if (uap->signo && f)
			psignal(p, uap->signo);
		proc_rele(p);
		return;
	}
	switch (uap->pid) {
	case -1:		/* broadcast signal */
		u.u_error = killpg1(uap->signo, 0, 1);
		break;
	case 0:			/* signal own process group */
		u.u_error = killpg1(uap->signo, 0, 0);
		break;
	default:		/* negative explicit process group */
		u.u_error = killpg1(uap->signo, -uap->pid, 0);
		break;
	}
	return;
}

killpg()
{
	register struct a {
		int	pgrp;
		int	signo;
	} *uap = (struct a *)u.u_ap;

	if (uap->signo < 0 || uap->signo > NSIG) {
		u.u_error = EINVAL;
		return;
	}
	u.u_error = killpg1(uap->signo, uap->pgrp, 0);
}

/* KILL CODE SHOULDNT KNOW ABOUT PROCESS INTERNALS !?! */

killpg1(signo, pgrp, all)
	int signo, pgrp, all;
{
	register struct proc *p, *pp;
	int p_ret;
	int f, error = 0;
	register int s;

	if (!all && pgrp == 0) {
		/*
		 * Zero process id means send to my process group.
		 */
		pgrp = u.u_procp->p_pgrp;
		if (pgrp == 0)
			return (ESRCH);
	}
	p = u.u_procp;
	f = 0;
	FORALLPROC (
	   if ((pp->p_pgrp != pgrp && !all) || pp->p_ppid == 0 ||
		(pp->p_type&SSYS) || (all && (pp == p && p->p_progenv != A_POSIX)))
			NEXTPROC;
	   s=splhigh();
	   smp_lock(&lk_procqs,LK_RETRY);
	   p_ret = proc_ref(pp);
	   smp_unlock(&lk_procqs);
	   splx(s);
	   if (p_ret) {
		/* repeat the tests which can change from above */
		if ((pp->p_pgrp != pgrp && !all) || pp->p_ppid == 0 ) {
	 			proc_rele(pp);
				NEXTPROC;
		}
		if (u.u_uid != 0 &&  (signo != SIGCONT || !inferior(pp))) {

		/* If the signal is SIGCONT and we are in POSIX mode then
		 * can send to any process in our session.
		 */
		   if ((pp->p_progenv == A_POSIX) && (signo == SIGCONT) &&
			 (p->p_sid == pp->p_sid))
				goto sendit;

		/* If effective uids do not match or we are attempting to */
		/* send to all processes then must check the mode we are  */
		/* in and base the send/nosend decision on that.	  */
		   if((u.u_uid != pp->p_suid) || all) {

		     switch(p->p_progenv) {

		     case A_POSIX:
		     case A_SYSV:
			/* If sending process is POSIX or SYS V then check */
			/* other possible matches.			   */
			if( !(sigperms(pp)) ) {
				error = EPERM;
	 			proc_rele(pp);
				NEXTPROC;
			}
			else {
			/* Found a match. If receiving process is */
			/* not POSIX or SysV then continue */
			     if ((pp->p_progenv != A_POSIX) && 
				(pp->p_progenv != A_SYSV)) { 
				   if(!all)
					error = EPERM;
	 			   proc_rele(pp);
	 			   NEXTPROC;
			     }
			}
			break;
		     case A_BSD:
		     default:
	 		proc_rele(pp);
			/* Only allowed to send to all processes if suser */
			/* which is filtered above. If not sending to all */
			/* and this process is not a match, then set error */
			/* for possible later use.			  */
			if (all)
				return(EPERM);
			if(!all) 
				error = EPERM;
			NEXTPROC;
		     }
		   }
		}
sendit:
		f++;
		if (signo)
			psignal(pp, signo);
	 	proc_rele(pp);
  	   }
	)
	return (( f == 0 ? (error ? error : ESRCH ) : 0));
}

/*
 * Send the specified signal to
 * all processes with 'pgrp' as
 * process group.
 */
gsignal(pgrp, sig)
	register int pgrp;
	register int sig;
{
	register struct proc *p;
	int s;

	if (pgrp == 0)
		return;
	FORALLPROC(
		if (pp->p_pgrp == pgrp) {
			s = splhigh();
			smp_lock(&lk_procqs, LK_RETRY);
			if ((pp->p_pgrp == pgrp) && proc_ref(pp)) {
				smp_unlock(&lk_procqs);
				splx(s);
				psignal(pp, sig);
				proc_rele(pp);
			} else {
				smp_unlock(&lk_procqs);
				splx(s);
			}
		}
	)
}

/*
 * Send the specified signal to
 * the specified process.
 * The caller of this routine is responsible for ensuring
 * that the specified process does not exit while psignal
 * is trying to send it a signal.
 * Use proc_ref(struct proc * p)  or proc_get(pid) as needed
 */

psignal(p, sig)
	register struct proc *p;
	register int sig;
{
	register int s;
	register int mask;
	register int cpindex, cpident;
	struct cpudata *pcpu;
	short savepid;
	struct proc *parent_p;
	int orphan;
	void (*action)();
	
	if ((unsigned)sig >= NSIG)
		return;
	mask = sigmask(sig);

	/*
	 * If proc is traced, always give parent a chance.
	 */
	if (p->p_trace & STRC)
		action = SIG_DFL;
	else {
		/*
		 * If the signal is being ignored,
		 * then we forget about it immediately.
		 */
 		if ((p->p_sigignore & mask) && (sig != SIGCONT))
			return;
		if (p->p_sigmask & mask)
			action = SIG_HOLD;
		else if (p->p_sigcatch & mask)
			action = SIG_CATCH;
		else
			action = SIG_DFL;
	}

	if ((p->p_progenv == A_POSIX) && 
		((sig ==SIGSTOP) ||
		(sig ==SIGTSTP) ||
		(sig ==SIGTTIN) ||
		(sig ==SIGTTOU)))
		orphan = pg_orphan(p->p_pgrp, p->p_sid);

	s = splhigh();
	smp_lock(&lk_signal, LK_RETRY);
	if (sig) {
		p->p_sig |= mask;
		switch (sig) {

		case SIGTERM:
			if ((p->p_trace&STRC) || action != SIG_DFL)
				break;
			/* fall into ... */

		case SIGKILL:
			/*
			 * Don't bother locking: could change
			 * immediately on releasing lock anyway
			 */
			if (p->p_nice > NZERO)
				p->p_nice = NZERO;
			break;

		case SIGCONT:
			p->p_sig &= ~stopsigmask;
			break;

		case SIGSTOP:
		case SIGTSTP:
		case SIGTTIN:
		case SIGTTOU:
			p->p_sig &= ~sigmask(SIGCONT);
			/* 
			 * A process associated with an orphaned process group shall not be 
			 * allowed to stop in response to the SIGTSTP, SIGTTIN, or SIGTTOU signals.
			 */
			if (p->p_progenv == A_POSIX && sig != SIGSTOP && action == SIG_DFL && 
				orphan) {
				p->p_sig &= ~stopsigmask;       /* Throw away */
				smp_unlock(&lk_signal);
				splx(s);
				return;
			}
			break;
		}
	}
	/*
	 * Defer further processing for signals which are held.
	 */
	if ((action == SIG_HOLD) && (sig != SIGCONT)) {
		smp_unlock(&lk_signal);
		splx(s);
		return;
	}
	/* 
	 * get runqueue lock to insure that the processes doesn't change
	 * state while we are sending the signal.
	 */
	smp_lock(&lk_rq,LK_RETRY);
	switch (p->p_stat) {

	case SSLEEP:
		/*
		 * If process is sleeping at negative priority we can't
		 * interrupt the sleep... the signal will be noticed
		 * when the process returns through trap() or syscall()
		 * And, if the pri changes while we are reading it,
		 * the process is on its way to seeing the signal.
		 */
		if (p->p_pri <= PZERO) {
			smp_unlock(&lk_rq);
			smp_unlock(&lk_signal);
			splx(s);
			return;
		}
		/*
		 * Process is sleeping and traced... make it runnable
		 * so it can discover the signal in issig() and stop
		 * for the parent.
		 */
		if (p->p_trace&STRC) {
			goto run;
		}

		switch (sig) {

		case SIGSTOP:
		case SIGTSTP:
		case SIGTTIN:
		case SIGTTOU:
			/*
			 * These are the signals which by default
			 * stop a process.
			 */
			if (action != SIG_DFL) {
				goto run;
			}
			/*
			 * Don't clog system with children of init
			 * stopped from the keyboard.
			 * 'p' protected from above here
			 */
			if (sig != SIGSTOP && (p->p_pptr == &proc[1])) {
				smp_unlock(&lk_rq);
				p->p_sig &= ~mask;
				smp_unlock(&lk_signal);
				psignal(p, SIGKILL);
				splx(s);
				return;
			}
			/*
			 * If a child in vfork(), stopping could
			 * cause deadlock.
			 */
			if (p->p_vm&SVFORK) {
				smp_unlock(&lk_rq);
				smp_unlock(&lk_signal);
				splx(s);
				return;
			}

			p->p_sig &= ~mask;
			p->p_cursig = sig;
			smp_unlock(&lk_signal);

			/* inline version of stop(p) because of lock
			   hierachy.  The process "p" is in sleep state
			   at this point and we have run queue lock so
			   it can't move.  Put process in stopped state.
			*/
			p->p_stat = SSTOP;
			p->p_trace &= ~SWTED;
			savepid = p->p_pid;
			smp_unlock(&lk_rq);

			/* now process "p" is stopped.  Get procqs lock
			   so p_pptr field doesn't change and ref the 
			   processes. */ 
			
			smp_lock(&lk_procqs, LK_RETRY);
			/* make sure proc is still there. Note that no locks
			   are held above so process could go away! */
			parent_p = p->p_pptr;
			if ((savepid == p->p_pid) && proc_ref(parent_p)) {
				smp_unlock(&lk_procqs);
				
				/*
				 * now synchronize with wait syscall
				 */
				smp_lock(&lk_waitchk,LK_RETRY);
				parent_p->p_waitchk++;
				smp_unlock(&lk_waitchk);
				
				/* send SIGCHLD if needed */
				if (parent_p != &proc[1]) {
				    if ((parent_p->p_sigflag&SNOCLDSTP) == 0) 
					psignal(parent_p, SIGCHLD);
				}
				proc_rele(parent_p);
				wakeup((caddr_t)parent_p);
			} else {
				smp_unlock(&lk_procqs);
			}

			splx(s);
			return;

		case SIGIO:
		case SIGURG:
		case SIGCHLD:
		case SIGWINCH:
			/*
			 * These signals are special in that they
			 * don't get propogated... if the process
			 * isn't interested, forget it.
			 */
			if (action != SIG_DFL) {
				goto run;
			}
			smp_unlock(&lk_rq);
			p->p_sig &= ~mask;		/* take it away */
			smp_unlock(&lk_signal);
			splx(s);
			return;

		default:
			/*
			 * All other signals cause the process to run
			 */
			goto run;
		}
		/*NOTREACHED*/

	case SSTOP:
		/*
		 * If traced process is already stopped,
		 * then no further action is necessary.
		 */
		if (p->p_trace&STRC) {
			smp_unlock(&lk_rq);
			smp_unlock(&lk_signal);
			splx(s);
			return;
		}
		switch (sig) {

		case SIGKILL:
			/*
			 * Kill signal always sets processes running.
			 */
			goto run;

		case SIGCONT:
			/*
			 * If the process catches SIGCONT, let it handle
			 * the signal itself.  If it isn't waiting on
			 * an event, then it goes back to run state.
			 * Otherwise, process goes back to sleep state.
			 * If have gone back to sleep or run already,
			 * there is nothing to do here
			 */
			smp_unlock(&lk_signal);
			if (action != SIG_DFL || p->p_wchan == 0) {
				if (p->p_pri > PUSER)
					p->p_pri = PUSER;
				setrun(p);
			}
			else	p->p_stat = SSLEEP;
			smp_unlock(&lk_rq);
			splx(s);
			return;

		case SIGSTOP:
		case SIGTSTP:
		case SIGTTIN:
		case SIGTTOU:
			/*
			 * Already stopped, don't need to stop again.
			 * (If we did the shell could get confused.)
			 */
			smp_unlock(&lk_rq);
			p->p_sig &= ~mask;		/* take it away */
			smp_unlock(&lk_signal);
			splx(s);
			return;

		default:
			/*
			 * If process is sleeping interruptibly, then
			 * unstick it so that when it is continued
			 * it can look at the signal.
			 * But don't setrun the process as its not to
			 * be unstopped by the signal alone.
			 */
			smp_unlock(&lk_signal);
			if (p->p_wchan && p->p_pri > PZERO)
				unsleep(p);
			smp_unlock(&lk_rq);
			splx(s);
			return;
		}
		/*NOTREACHED*/

	default:
		smp_unlock(&lk_rq);
		smp_unlock(&lk_signal);
		/*
		 * SRUN, SIDL, SZOMB do nothing with the signal,
		 * other than kicking ourselves if we are running.
		 * It will either never be noticed, or noticed very soon.
		 * Don't lock the run queue:  if we miss someone,
		 * they changed states, and will pick up the signal
		 * at the next opportunity.
		 */
		cpident = CURRENT_CPUDATA->cpu_num;
		for (cpindex = lowcpu; cpindex <= highcpu ; cpindex++) {
			if ((pcpu=CPUDATA(cpindex)) &&
			    p == pcpu->cpu_proc &&
			    !pcpu->cpu_noproc) {
				if (cpident == cpindex) {
					aston();
				} else {
					intrpt_cpu(cpindex,IPI_SCHED);
				}
				break;
			}
		}
		splx(s);
		return;
	}
	/*NOTREACHED*/
run:
	smp_unlock(&lk_signal);
	/*
   	 * Processes is either in SSTOP or SSLEEP state.  Set it
	 * runable with priority at least as high as PUSER
 	 * so this job that got a signal starts running...
	 */
	if ((psignal_priority_mod) && (p->p_pri > PUSER))
		p->p_pri = PUSER;
	setrun(p);
	smp_unlock(&lk_rq);
	splx(s);
	return;
}

/*
 * Returns true if the current
 * process has a signal to process.
 * The signal to process is put in p_cursig.
 * This is asked at least once each time a process enters the
 * system (though this can usually be done without actually
 * calling issig by checking the pending signal masks.)
 * A signal does not do anything
 * directly to a process; it sets
 * a flag that asks the process to
 * do something to itself.
 */

sig_send_kill(pid) 
int pid;
{
	struct proc *p;

	if (p = proc_get(pid)) {
		psignal(p,SIGKILL);
		proc_rele(p);
	}

}
issig(have_rq)
	int have_rq;
{

	register int s,ret_val;


	s = splhigh();
	smp_lock(&lk_signal,LK_RETRY);
	ret_val = issig_subr(have_rq);
	smp_unlock(&lk_signal);
	(void) splx(s);

	return(ret_val);
}
issig_subr(have_rq)
	int have_rq;
{
	register struct proc *p;
	register int sig;
	register int sigbits, mask;
	register struct cpudata *pcpu;

	pcpu = CURRENT_CPUDATA;
	
	p = u.u_procp;
	
	for (;;) {
		/* Don't need lock for simple longword read
		 * If another bit is set, we will catch it
		 * the next time around.
		 */
		sigbits = p->p_sig;
		sigbits &= ~p->p_sigmask;
		if ((p->p_trace&STRC) == 0)
			sigbits &= ~p->p_sigignore;
		if (p->p_vm&SVFORK)
			sigbits &= ~stopsigmask;
		if (sigbits == 0)
			break;
		sig = (sigbits & sigmask(SIGKILL)) ? SIGKILL : ffs(sigbits);
		mask = sigmask(sig);

		p->p_sig &= ~mask;		/* take the signal! */
		p->p_cursig = sig;

		if (p->p_trace&STRC && ((p->p_vm&SVFORK) == 0) &&
			 (p->p_cursig != SIGKILL) ) {
			int first_time=1;
			/*
			 * If traced, always stop, and stay
			 * stopped until released by the parent.
			 */
			do {
				/*
				 * reset pcpu here because procxmt can
				 * return on a different processor
				 */
				pcpu = CURRENT_CPUDATA;
				(void) splhigh();
				if (first_time) {
					if (!have_rq) 
						smp_lock(&lk_rq, LK_RETRY);
					smp_unlock(&lk_signal);
				}else 
					smp_lock(&lk_rq, LK_RETRY);
				p->p_stat = SSTOP;
				p->p_trace &= ~SWTED;
				/*
				 * Synchronously get into the stop state,
				 * then never care about the lock again here.
				 * Don't forget to save off state and return it.
				 */
				swtch_check();
				if (smp) {
					pcpu->cpu_proc->p_hlock = 
						pcpu->cpu_hlock->l_plock;
					pcpu->cpu_hlock->l_plock=0;
				} else {
					pcpu->cpu_proc->p_hlock = 
							pcpu->cpu_hlock;
					pcpu->cpu_hlock=0;	
				}	

				/* first time we send a signal to our parent 
				   to let it know we are stoppend...after that
				   procxmt takes care of synch */
				if (first_time){
					first_time=0;
					
					/* swtch then send SIGCHLD to parent*/
					sig_parent_swtch();
				}
				else {
					/* we are stopped so wake up parent
					   doing the tracing...  Note the 
					   parent won't run until lk_rq 
					   is released in swtch routine. */
					wakeup((caddr_t)u.u_procp->p_debug);
					swtch();
				}
				pcpu = CURRENT_CPUDATA;
				pcpu->cpu_hlock = pcpu->cpu_proc->p_hlock;
				pcpu->cpu_proc->p_hlock=0;
			} while (!procxmt() && p->p_trace&STRC);

			(void) splhigh();
			smp_lock(&lk_signal, LK_RETRY);
			if (have_rq) smp_lock(&lk_rq, LK_RETRY);

			/*
			 * If the traced bit got turned off,
			 * then put the signal taken above back into p_sig
			 * and go back up to the top to rescan signals.
			 * This ensures that p_sig* and u_signal are consistent.
			 */
			if ((p->p_trace&STRC) == 0) {
				p->p_sig |= mask;
				continue;
			}

			/*
			 * If parent wants us to take the signal,
			 * then it will leave it in p->p_cursig;
			 * otherwise we just look for signals again.
			 */
			sig = p->p_cursig;
			if (sig == 0)
				continue;

			/*
			 * If signal is being masked put it back
			 * into p_sig and look for other signals.
			 */
			mask = sigmask(sig);
			if (p->p_sigmask & mask) {
				p->p_sig |= mask;
				continue;
			}
		}
		switch ((int)u.u_signal[sig]) {

		case SIG_DFL:
			/*
			 * Don't take default actions on system processes.
			 */
			if (p->p_ppid == 0)
				break;
			switch (sig) {

			case SIGTSTP:
			case SIGTTIN:
			case SIGTTOU:
				/*
				 * Children of init aren't allowed to stop
				 * on signals from the keyboard.
				 */
				if (p->p_pptr == &proc[1]) {
					timeout(sig_send_kill,p->p_pid,1);
					continue;
				}
				/* fall into ... */

			case SIGSTOP:
				if (p->p_trace&STRC)
					continue;
				/*sending of SIGCHLD moved after switch
				  so we are REALLY stopped before telling
				  parent. */
				(void) splhigh();
				if (!have_rq) smp_lock(&lk_rq, LK_RETRY);
				smp_unlock(&lk_signal);
				p->p_stat = SSTOP;
				p->p_trace &= ~SWTED;
				/*
				 * Synchronously get into the stop state,
				 * then never care about the lock again here.
				 * Don't forget to save off state and return it.
				 */
				if (smp) {
					pcpu->cpu_proc->p_hlock = 
						pcpu->cpu_hlock->l_plock;
					pcpu->cpu_hlock->l_plock=0;
				} else {
					pcpu->cpu_proc->p_hlock = 
						pcpu->cpu_hlock;
					pcpu->cpu_hlock=0;
				}	
				/* We can now inform our parent we are
				   stopped.  "sig_parent_swtch" does a
				   context switch THEN sends a SIGCHLD
				   to the parent */
				sig_parent_swtch();
				pcpu = CURRENT_CPUDATA;
				pcpu->cpu_hlock = pcpu->cpu_proc->p_hlock;
				pcpu->cpu_proc->p_hlock=0;

				smp_lock(&lk_signal,LK_RETRY);
				if (have_rq) smp_lock(&lk_rq,LK_RETRY);
				continue;

			case SIGCONT:
			case SIGCHLD:
			case SIGURG:
			case SIGIO:
			case SIGWINCH:
				/*
				 * These signals are normally not
				 * sent if the action is the default.
				 */
				continue;		/* == ignore */

			default:
				goto send;
			}
			/*NOTREACHED*/

		case SIG_HOLD:
		case SIG_IGN:
			/*
			 * Masking above should prevent us
			 * ever trying to take action on a held
			 * or ignored signal, unless process is traced.
			 */

			if (((p->p_trace&STRC) == 0) && !(sig == SIGCONT &&
				u.u_signal[sig] == SIG_IGN))
				printf("issig\n");
			continue;

		default:
			/*
			 * This signal has an action, let
			 * psig process it.
			 */
			goto send;
		}
		/*NOTREACHED*/
	}
	/*
	 * Didn't find a signal to send.
	 */
	p->p_cursig = 0;
	return (0);

send:
	/*
	 * Let psig process the signal.
	 */
	return (sig);
}
/* 
 *
 * Called from locore.  We send a signal to our child AFTER..!!! we are
 * 			stopped all the way.
 *
 */
sig_parent(p) 
	register struct proc *p;
{
	register struct proc *parent_p;
	short savepid;

	savepid = p->p_pid;
	smp_unlock(&lk_rq);

	/* process "p" is free to go.  Now we get proc queue
	   lock and reference "p" agin.  "p" could be gone because
	   of release of rq lock above.  If p is gone then do
	   nothing, if not go refernce parent and send
	   proper signals. */ 
	smp_lock(&lk_procqs, LK_RETRY);
	parent_p = p->p_pptr;
	if ((p->p_exist == P_ALIVE) &&(savepid == p->p_pid) && 
						proc_ref(parent_p)) {
		smp_unlock(&lk_procqs);
		smp_lock(&lk_waitchk, LK_RETRY);
		parent_p->p_waitchk++;
		smp_unlock(&lk_waitchk);
		if ( (parent_p->p_sigflag & SNOCLDSTP) == 0) {
			psignal(parent_p, SIGCHLD);
		}
 		proc_rele(parent_p);
		wakeup((caddr_t)parent_p);
	} else {
		smp_unlock(&lk_procqs);
	}

}

/*
 * Perform the action specified by
 * the current signal.
 * The usual sequence is:
 *	if (issig())
 *		psig();
 * The signal bit has already been cleared by issig,
 * and the current signal number stored in p->p_cursig.
 */
psig()
{
	register struct proc *p = u.u_procp;
	register int sig = p->p_cursig;
	register int mask = sigmask(sig), returnmask;
	register void (*action)();

	if (sig == 0)
		panic("psig");
	action = u.u_signal[sig];
	if (action != SIG_DFL) {

		if (action == SIG_IGN || (p->p_sigmask & mask))
			panic("psig action");
		u.u_error = 0;
		/*
		 * Set the new mask value and also defer further
		 * occurences of this signal (unless we're simulating
		 * the old signal facilities). 
		 *
		 * Special case: user has done a sigpause.  Here the
		 * current mask is not of interest, but rather the
		 * mask from before the sigpause is what we want restored
		 * after the signal processing is completed.
		 */
		(void) splhigh();
		if  (u.u_oldsig & sigmask(sig)) {
			if (sig != SIGILL && sig != SIGTRAP) {
				u.u_signal[sig] = SIG_DFL;
				p->p_sigcatch &= ~mask;
				u.u_oldsig &= ~sigmask(sig);
			}
			mask = 0;
		}
		if (u.u_sigflag & SOMASK) {
			returnmask = u.u_oldmask;
			u.u_sigflag &= ~SOMASK;
		} else
			returnmask = p->p_sigmask;
		p->p_sigmask |= u.u_sigmask[sig] | mask;
		(void) spl0();
		u.u_ru.ru_nsignals++;

		sendsig(action, sig, returnmask);
		/* Don't lock: only we can be setting
		 * cursig right now, since we are running
		 */
		p->p_cursig = 0;
		return;
	}

	/* remove all the locks since the process is going to exit */
	while(CURRENT_CPUDATA->cpu_hlock) 
		smp_unlock(CURRENT_CPUDATA->cpu_hlock);

	u.u_acflag |= AXSIG;
	switch (sig) {

	case SIGILL:
	case SIGIOT:
	case SIGBUS:
	case SIGQUIT:
	case SIGTRAP:
	case SIGEMT:
	case SIGFPE:
	case SIGSEGV:
	case SIGSYS:
		u.u_arg[0] = sig;
		if (core())
			sig += 0200;
	}
	exit(sig);
}

/*
 * Create a core image on the file "core"
 * If you are looking for protection glitches,
 * there are probably a wealth of them here
 * when this occurs to a suid command.
 *
 * It writes UPAGES block of the
 * user.h area followed by the entire
 * data+stack segments.
 */
core()
{
	register struct gnode *gp;
	register struct nameidata *ndp = &u.u_nd;

        unsigned int temp_u_error;

	int	vp_size;

	/* ctob() : macro to convert core clicks to bytes */

	if (u.u_uid != u.u_ruid || u.u_gid != u.u_rgid)
		return (0);
	/* try not to dump core for files user does not have
	   read access to the executable 
           The access() call expects u.u_error to be zero so
           save the real value and force it to zero befor access().
           When access returns we will restore the real value.
	*/
        temp_u_error = u.u_error;
	u.u_error = 0;

	if (u.u_procp->p_textp) {
		if (gp = u.u_procp->p_textp->x_gptr) {
			/* Lock gnode before doing access check because */
			/* all calls to access() must be on a locked gnode. */
			/* Not doing so can cause race conditions when */
			/* dealing with remote filesystems. */
			gfs_lock(gp);
			if (access(gp, GREAD)) {
				gfs_unlock(gp);
				return(0);
			}
			gfs_unlock(gp);
		}
	}
        u.u_error = temp_u_error;

#ifdef vax
	if (u.u_acflag & AVP) {
		/*
		 * the vpcontext is written out in two pieces.  The first 
		 * piece is the struct vpcontext, but the last entry in this 
		 * structure is not written.  This last entry is a pointer to 
		 * the vector registers.  These registers (8K worth of data!) 
		 * are written out separately.
		 */
		vp_size = sizeof(struct vpcontext) - sizeof (char *) 
			+ VPREGSIZE;
	} else {
		vp_size = 0;
	}
#else
	vp_size = 0;
#endif vax
	if ((ctob(UPAGES+u.u_dsize+u.u_ssize) + vp_size) >=
	    u.u_rlimit[RLIMIT_CORE].rlim_cur)
		return (0);
	u.u_error = 0;
	ndp->ni_nameiop = CREATE | FOLLOW;
	/*
	 * All calls to gfs_namei need a MAXPATHLEN buffer if FOLLOW is
	 * specified in ni_nameiop.
	 */
	KM_ALLOC(ndp->ni_dirp, char *, MAXPATHLEN, KM_NAMEI, KM_NOARG);
	bcopy("core", ndp->ni_dirp, 5);
	if ((gp = gfs_namei(ndp)) == NULL) {
		if (u.u_error) {
			KM_FREE(ndp->ni_dirp, KM_NAMEI);
			return (0);
		}
		gp = GMAKNODE(0644 | GFREG, (dev_t) 0, ndp);
		if (gp==NULL) {
			KM_FREE(ndp->ni_dirp, KM_NAMEI);
			return (0);
		}
	}
	KM_FREE(ndp->ni_dirp, KM_NAMEI);
	if (access(gp, GWRITE) ||
	   (gp->g_mode & GFMT) != GFREG ||
	   gp->g_nlink != 1) {
		u.u_error = EFAULT;
		goto out;
	}
	(void)GTRUNC(gp, (u_long)0, u.u_cred);
	u.u_acflag |= ACORE;
	/* copy u area */
	u.u_error = rdwri(UIO_WRITE, gp, (caddr_t)&u, ctob(UPAGES), 0, 1,
	(int *)0);
	if (u.u_error == 0) {
		/* copy data area */
		u.u_error = rdwri(UIO_WRITE, gp,
		(caddr_t)ctob(dptov(u.u_procp, 0)), ctob(u.u_dsize),
		ctob(UPAGES), 0, (int *)0);
	} 
	if (u.u_error == 0) {
		/* copy stack */
		u.u_error = rdwri(UIO_WRITE, gp,
		(caddr_t)ctob(sptov(u.u_procp, u.u_ssize - 1)),
		    ctob(u.u_ssize),
		    ctob(UPAGES)+ctob(u.u_dsize), 0, (int *)0);
	} 
#ifdef vax
	/* first, a sanity check:  if the AVP flag is set, then this 
	 * should be a vector capable machine
	 */
	if (u.u_acflag & AVP) {
		if ( ! ( (cpu == VAX_6400) || (cpu == VAX_9000) ) ) {
		    goto skip_vpcontext_save;
		}
	}
	/* if vector process, then copy vector context.  
	 * just the v regs, or status and error values too ??
	 * test vpc_state; may need to call vp_contextsave()
	 * test for VP_FAILURE or VP_SUCCESS
	 */
	if ((u.u_acflag & AVP) && (u.u_error == 0)) {
		/* debby: Since it isn't immediately obvious whether I can 
		 *        get at the cpudata struct, or even if that is 
		 *        relavent, the following is coded with the 
		 *        assumption that it is irrelevant.  Also, there may 
		 *        be circumstances where a vp_contextsave called 
		 *        from core() may cause major trouble.  
		 */
		/* if the vpc_state is not VPC_SAVED, then do the 
		 * vp_contextsave.  But remember that if the state is 
		 * VPC_WAIT then no vector unit has been assigned yet.
		 */
		if (u.u_procp->p_vpcontext->vpc_state != VPC_SAVED) {
			/* need save the vector processor's context into the 
			 * vpcontext area so that it can then be copied into 
			 * the gnode
			 */
			if (vp_contextsave (u.u_procp) == VP_FAILURE) {
				/* set u_error to value not defined in errno.h 
				 * at some point someone will need to tell 
				 * me what this should really be set to!
				 */
				u.u_error = 80;
			}
			/* note: do not set the vpc_state to VPC_SAVED.  
			 *       knowing what the vpc_state was may be 
			 *       useful when debugging the core dump.
			 */
		}
		if (u.u_error == 0) {
		    u.u_error =
			rdwri		/* read or write data on a gnode */
			    (UIO_WRITE,	/* write the data */
			    gp,		/* pointer to the gnode */
			    u.u_procp->p_vpcontext,
					/* pointer to the data to be writen */
			    sizeof (struct vpcontext) - sizeof (char *),
					/* len (in bytes) of data */
		    	    ctob(UPAGES)+ctob(u.u_dsize)+ctob(u.u_ssize),
			    		/* offset into write buffer */
			    UIO_SYSSPACE,
					/* space to write from ??? one of: 
					 *    UIO_USERSPACE  (0) or 
					 *    UIO_SYSSPACE   (1) or 
					 *    UIO_USERISPACE (2)
					 */
			    (int *) 0);	/* pointer to ??? */
		}
		if (u.u_error == 0) {
		    u.u_error =
			rdwri		/* read or write data on a gnode */
			    (UIO_WRITE,	/* write the data */
			    gp,		/* pointer to the gnode */
			    u.u_procp->p_vpcontext->vpc_vregs,
					/* pointer to the data to be writen */
			    VPREGSIZE,	/* len (in bytes) of data */
		    	    ctob(UPAGES)+ctob(u.u_dsize)+ctob(u.u_ssize)+
				sizeof(struct vpcontext) - sizeof (char *),
			    		/* offset into write buffer */
			    UIO_SYSSPACE,
					/* space to write from ??? one of: 
					 *    UIO_USERSPACE  (0) or 
					 *    UIO_SYSSPACE   (1) or 
					 *    UIO_USERISPACE (2)
					 */
			    (int *) 0);	/* pointer to ??? */
		}
after_dump_vpcontext:
		/* release memory which was KM_ALLOC'd for the vector process */
		vp_cleanup ( u.u_procp );
	}
skip_vpcontext_save:
#endif vax
	gp->g_flag |= GUPD|GCHG;
	(void) GUPDATE(gp, &time, &time, 0, u.u_cred);
out:
	gput(gp);
	return (u.u_error == 0);
}
