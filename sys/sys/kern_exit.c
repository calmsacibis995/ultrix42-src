#ifndef lint
static char *sccsid = "@(#)kern_exit.c	4.3	ULTRIX	2/28/91";
#endif

/************************************************************************
 *									*
 *			Copyright (c) 1987, 1988 by    			*
 *		Digital Equipment Corporation, Maynard, MA		*
 *			All rights reserved.				*
 *									*
 *   This software is furnished under a license and may be used and	*
 *   copied  only  in accordance with the terms of such license and	*
 *   with the  inclusion  of  the  above  copyright  notice.   This	*
 *   software  or  any	other copies thereof may not be provided or	*
 *   otherwise made available to any other person.  No title to and	*
 *   ownership of the software is hereby transferred.			*
 *									*
 *   This software is  derived	from  software	received  from	the	*
 *   University    of	California,   Berkeley,   and	from   Bell	*
 *   Laboratories.  Use, duplication, or disclosure is	subject  to	*
 *   restrictions  under  license  agreements  with  University  of	*
 *   California and with AT&T.						*
 *									*
 *   The information in this software is subject to change  without	*
 *   notice  and should not be construed as a commitment by Digital	*
 *   Equipment Corporation.						*
 *									*
 *   Digital assumes no responsibility for the use  or	reliability	*
 *   of its software on equipment which is not supplied by Digital.	*
 *									*
 ************************************************************************/

/* ------------------------------------------------------------------------
 * Modification History: /sys/sys/kern_exit.c
 *
 * 28-Feb-91	prs
 *	Added support for a configurable number of
 *	open file descriptors.
 *
 * 4-Sep-90	dlh
 *	added vector processor support code
 *
 * 05-Feb-90 dws
 *	Added code to exit for handling the creation of an orphaned process 
 *	group for processes in POSIX mode.
 *
 * 07-Dec-89 scott
 *	set first param of AUDIT_CALL to 1 (for SYS_exit)
 *
 * 22-Sep-89 gg
 *	change waitpid to copyout status into user space in vax case too
 *
 * 21-Sep-89 jaw 
 * 	move force close outside of locking procqs.
 *
 * 20-Jul-89 jaw
 *	remove some debug code.
 *
 * 12-Jul-89 -- dws
 *	added cleanup code for trusted path
 *
 * 09-Jun-89 -- Larry Scott
 *	added audit support
 *
 * 07-Jun-89 -- Giles Atkinson
 *	Remove pre-LMF code for login limits
 *
 * 07-Jun-89 -- map (Mark Parenti)
 *	Removed #ifdef vax around options check in wait1().  This check
 *	should be done for all architectures.
 *
 * 26-May-89 -- map (Mark Parenti)
 *	Fix the following bug in waitpid():
 *		1) Add copyout of status in MIPS case.  Previously
 *			no status was ever returned.
 *
 *	Fix the following bugs in wait() (only used in waitpid() case):
 *		1) Check for proper pid before incrementing "found" counter.
 *			Previously this was not properly done causing
 *			an improper pid to be returned in the WUNTRACED
 *			case.
 *		2) The above change also fixes problem when no matching pid
 *			is found.  We now return ECHILD as required. 
 *
 *  8-May-89 -- Giles Atkinson
 *    Add exit-actions mechanism: processing of linked list of
 *    structures depending from user struct.
 *
 * 24-Apr-89 -- jaw 
 *	fix race condition in process tracing between "ptrace" and child
 *	exiting.
 *
 * 07-March 89 -- gmm
 *	release_uarea_noreturn() now called without an argument
 *
 * 23 Aug 88 -- miche
 *	Add support for ref'ing a process.
 *
 * 25 Jul 88 -- jmartin
 *	Replace "&= ~" with CLEAR_P_VM.
 *
 * 07 Jun 88 -- miche
 *	SMP procqs:  knows about the bitmap
 *
 * 28 Jan 88 -- us
 *	Added kernel memory allocator.
 ******* SMP CHANGES above ********
 *
 *
 * 29 Jan 89 -- map (Mark Parenti)
 *	Fixed #ifdef in waitpid. MIPS version was not inside #ifdef mips.
 *
 * 8 Nov 88 -- map - Mark A. Parenti
 *	1) Force closing of controlling terminal if exiting process is
 *	a controlling process in POSIX mode.  This will also cause
 *	the process group to be cleared preventing signals from going to
 *	any other process in the process group.
 *
 *	2) Add sending of SIGHUP in POSIX/System V mode if process is
 *		a session leader.
 *	3) Add waitpid() system call from POSIX.
 *	4) Clear session id field in proc structure upon exit.
 *
 * 02 Sep 88 -- map
 *	Force closing of controlling terminal if exiting process is
 *	a controlling process in POSIX mode.  This will also cause
 *	the process group to be cleared preventing signals from going to
 *	any other process in the process group.
 *
 * 11 Jul 88 -- map
 *	1) Add sending of SIGHUP in POSIX/System V mode if process is
 *		a session leader.
 *	2) Add waitpid() system call from POSIX.
 *	3) Clear session id field in proc structure upon exit.
 *
 * 9  Jun 88 -- amato
 *	Allocate the usage structure in the exit system before 
 *	changing the state of the process in case the allocate 
 *	has to sleep.  
 *
 * 12 Jan 88 -- fglover
 *	Release any outstanding Sys-V locks in exit routine
 *
 * 11 Sep 86 -- koehler 
 *	exit no longer uses a mbuf
 *
 * 17 Nov 86 -- depp
 *	Fixed "stale u-area" problem in vm_pt.c and vm_mem.c.  So, the routine
 *	"exit" no longer needs raised IPL when a process' "u" area is 
 *	deallocated.
 *
 * 11 Mar 86 -- robin
 *	Added code for user login limits that at&t require.
 *
 * 11 Mar 86 -- lp
 *	Changed order in which things get done on exit. Close down
 *	files before releasing vm (used by n-buffering).
 *
 * 11 Nov 85 -- depp
 *	Removed all conditional compiles for System V IPC.
 *
 * 24 Oct 84 -- jrs
 *	Update for linked proc lists
 *	Derived from 4.2BSD, labeled:
 *		kern_exit.c 6.3 84/06/10
 *
 * 4 April 85 -- Larry Cohen
 *	Changes to support open block in use capability  - 001
 *
 * 11 Mar 85 -- depp
 *	Added System V semaphore and shared memory support
 *
 *  4/13/85 - Larry Cohen
 *	call ioctl FIOCINUSE when closing inuse desriptor
 *
 *  05-May-85 - Larry Cohen
 *	loop up to u_omax instead of NOFILE
 *
 *  19 Jul 85 -- depp
 *	Removed calls to smexit, as this call is handled by vrelvm.
 *
 * 18 Sep 85 -- depp
 *	Added punlock call to unlock memory segments
 *
 * 23-Jul-85 -- jrs
 *	Add multicpu sched code
 *
 * -----------------------------------------------------------------------
 */

#include "../machine/reg.h"
#include "../machine/psl.h"

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/map.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/exec.h"
#include "../h/exit_actn.h"
#include "../h/tty.h"
#include "../h/kernel.h"
#include "../h/proc.h"
#include "../h/buf.h"
#include "../h/wait.h"
#include "../h/vm.h"
#include "../h/text.h"
#include "../h/file.h"
#include "../h/gnode.h"
#include "../h/ioctl.h"
#include "../h/cpudata.h"
#include "../h/kmalloc.h"

#ifdef GFSDEBUG
extern short GFS[];
#endif GFSDEBUG

/*
 * Exit system call: pass back caller's arg
 */
rexit()
{
	register struct a {
		int	rval;
	} *uap;

	uap = (struct a *)u.u_ap;
	exit((uap->rval & 0377) << 8);
}

/*
 * Release resources.
 * Save u. area for parent to look at.
 * Enter zombie state.
 * Wake up parent and init processes,
 * and dispose of children.
 */
extern	int	vpmask;
exit(rv)
	int rv;
{
	register int i;
	register struct proc *p, *q, *nq;
	register int x;
	register struct gnode *gp;
	struct file *fp;
	caddr_t value;
	char *temp;

	AUDIT_CALL ( 1, 0, (rv>>8), AUD_HDR|AUD_RES, (int *)0, 0 );

#ifdef PGINPROF
	vmsizmon();
#endif
	(void) punlock();	/* clean up any locked memory segments */
	p = u.u_procp;
	KM_ALLOC(p->p_ru, struct rusage *, sizeof(struct rusage), 
		 KM_ZOMBIE, KM_CLEAR);

#ifdef mips
	checkfp(p, 1);
#endif mips

	/* Scan the list of structures hanging from the user struct
	 * calling the function whose address is in each.
	 */
	 {
		register struct exit_actn *xp, *nxp;

		for (xp = u.u_exitp; xp; xp = nxp) {
			nxp = xp->xa_next;
			if ((*xp->xa_func)(xp))
				continue;	/* Don't deallocate */
			KM_FREE((caddr_t)xp, KM_EXIT_ACTN);
		}
	}

	/*
	 * if this process was a vector process, then clean up
	 */
	if (p->p_vpcontext) {
		vp_cleanup(p);
	}


	/* mark self as leaving: no one should now touch */
	proc_del();
	CLEAR_P_VM(p, SULOCK);
	p->p_type |= SWEXIT;
	p->p_sigignore = ~0;
	p->p_cpticks = 0;
	p->p_pctcpu = 0;
	for (i = 0; i < NSIG; i++)
		u.u_signal[i] = SIG_IGN;
	untimeout(realitexpire, (caddr_t)p);

#ifdef GFSDEBUG
	if(GFS[7])
		cprintf("exit: pid %d gp 0x%x (%d) closef\n", p->p_pid,
		p->p_textp->x_gptr, p->p_textp->x_gptr->g_number);
#endif GFSDEBUG
	/* Close down any active files */
	for (i = 0; i <= u.u_lastfile; i++) {
		if (!U_OFILE(i)) {
			U_POFILE_SET(i,0);
			continue;
		}

		fp = U_OFILE(i);

		/* Release all Sys-V style locks */
		(void) gno_lockrelease (fp);

		if (U_POFILE(i) & UF_INUSE) {
			gp = (struct gnode *)fp->f_data;
			if (gp && (gp->g_flag & GINUSE)) {
				gp->g_flag &= ~(GINUSE);
				(*fp->f_ops->fo_ioctl)(fp, FIOCINUSE, value);
				wakeup((caddr_t)&gp->g_flag);
			}
		}
		U_OFILE_SET(i,NULL);
		U_POFILE_SET(i,0);
		closef(fp);
	}
	if (u.u_ofile_of) {
                KM_FREE(u.u_ofile_of, KM_NOFILE);
                KM_FREE(u.u_pofile_of, KM_NOFILE);
		u.u_ofile_of = NULL;
		u.u_pofile_of = NULL;
        }
#ifdef GFSDEBUG
	if(GFS[7])
		cprintf("exit: get rid of rdir and cdir\n");
#endif GFSDEBUG
	gfs_lock(u.u_cdir);
	gput(u.u_cdir);
	if (u.u_rdir) {
		gfs_lock(u.u_rdir);
		gput(u.u_rdir);
	}
	/*
	 * Release virtual memory.  If we resulted from
	 * a vfork(), instead give the resources back to
	 * the parent.
	 */

#ifdef GFSDEBUG
	if(GFS[7])
		cprintf("exit: release vm\n");
#endif GFSDEBUG
	vrelvm();

	/* clean up a debugged process */
	if (p->p_debug) {
		/* if this process was traced,
		 * free the debug structure
		 * need to lock to syncronize with parent
		 * who might be exiting ptrace 
		 */
		temp = p->p_debug;
		smp_lock(&lk_debug,LK_RETRY);
		p->p_trace &= ~STRC;
		p->p_debug = NULL;
		smp_unlock(&lk_debug);
		wakeup(temp);
		KM_FREE(temp, KM_DEBUG);
	}


	u.u_rlimit[RLIMIT_FSIZE].rlim_cur = RLIM_INFINITY;

	semexit();	/* clean up any outstanding semaphores */

	acct();
#ifdef QUOTA
	qclean();
#endif
	crfree(u.u_cred);

	/*
	 * must do this off the kernel stack: can't do this
	 * here since we are still using the kernel stack 
	 * vrelpt(u.u_procp);
	 * vrelu(u.u_procp, 0);
	 */

	(void) splclock();
	smp_lock(&lk_pid, LK_RETRY);
	multprog--;
	i = PIDHASH(p->p_pid);
	x = p - proc;
	if (pidhash[i] == x)
		pidhash[i] = p->p_idhash;
	else {
		for (i = pidhash[i]; i != 0; i = proc[i].p_idhash)
			if (proc[i].p_idhash == x) {
				proc[i].p_idhash = p->p_idhash;
				goto done;
			}
		panic("exit");
	}
	if (p->p_pid == 1)
		panic("init died");
done:
	smp_unlock(&lk_pid);
	(void) spl0();

	p->p_xstat = rv;
	*p->p_ru = u.u_ru;
	ruadd(p->p_ru, &u.u_cru);
	if (p->p_cptr)		/* only need this if any child is S_ZOMB */
		wakeup((caddr_t)&proc[1]);
	
	/*
	 * In POSIX, if exiting process creates an orphaned process group, 
	 * and any member of the pgrp is stopped, SIGHUP/SIGCONT the
	 * process group.
	 */
	if (p->p_progenv == A_POSIX) {
		/* Check if process group leader */
		if (p->p_pid == p->p_pgrp && pg_stopped(p->p_pgrp)) {
			gsignal(p->p_pgrp, SIGHUP);
			gsignal(p->p_pgrp, SIGCONT);
		} 
		/* Check if parent of process group leader */
		FORALLPROC(
			if (pp->p_ppid == p->p_pid && proc_get(pp->p_pid) == pp) {
				if (pp->p_ppid == p->p_pid && pg_stopped(pp->p_pgrp)) {
					gsignal(pp->p_pgrp, SIGHUP);
					gsignal(pp->p_pgrp, SIGCONT);
				}
			proc_rele(pp);
			}
		)
	}
	/*
	 * Protect the childen of this process from future
	 * tty signals, clearing TSTP/TTIN/TTOU if pending.
	 */
	(void) spgrp(p);
	(void) splclock();
	smp_lock(&lk_procqs, LK_RETRY);
	while (q = p->p_cptr) {
		nq = q->p_osptr;
		if (nq != NULL)
			nq->p_ysptr = NULL;
		if (proc[1].p_cptr)
			proc[1].p_cptr->p_ysptr = q;
		q->p_osptr = proc[1].p_cptr;
		q->p_ysptr = NULL;
		proc[1].p_cptr = q;

		q->p_pptr = &proc[1];
		q->p_ppid = 1;

		p->p_cptr = nq;
		/*
		 * Traced processes are killed
		 * since their existence means someone is screwing up.
		 * Stopped processes are sent a hangup and a continue.
		 * This is designed to be ``safe'' for setuid
		 * processes since they must be willing to tolerate
		 * hangups anyways.
		 */
		if ((q->p_trace & STRC || q->p_stat == SSTOP)
		    && proc_ref(q)) {
			smp_unlock(&lk_procqs);
			spl0();
			/*
			 * technically, we should get a lock here, but
			 * we don't care about trace since we are going
			 * to kill the process, and checking stat doesn't
			 * matter, since the HUP/CONT is harmless.
			 * If we SSTOP after the check, a lock wasn't
			 * going to save us anyway.
			 */
			if (q->p_trace&STRC) {
				q->p_trace &= ~STRC;
				psignal(q, SIGKILL);
			} else if (q->p_stat == SSTOP) {
				psignal(q, SIGHUP);
				psignal(q, SIGCONT);
			}
			proc_rele(q);
			(void) splclock();
			smp_lock(&lk_procqs, LK_RETRY);
		}
	}
	/*
	 * Trusted path clean up
	 */
	if ((p->p_type & SLOGIN) != 0) {
		p->p_type &= ~SLOGIN;	
		if (p->p_ttyp != NULL)
			p->p_ttyp->t_sid = 0;		
	}
	p->p_cptr = NULL;		/* fix from 4.3bsd */
	/*
	 * Save off parent pointer, as it is one of the things
	 * that can be cleaned out by the parent in wait()
	 * Ref the proc of parent to ensure they see us.
	 */
	if (q = p->p_pptr)
		if (!proc_ref(q)) {
			q = NULL;
		}
	smp_unlock(&lk_procqs);

	if ((p->p_progenv == A_POSIX) || (p->p_progenv == A_SYSV)) {
		if (p->p_ttyp != NULL) {
		    if (p->p_sid == p->p_pid) { /* Session leader */
			gsignal(p->p_ttyp->t_pgrp, SIGHUP);
			forceclose(u.u_ttyd); /* Release controlling terminal */
			p->p_ttyp = NULL;
		    }
		}
			
	}
	/*
	 * Change our status very last so that our parent
	 * won't erase our fields before we are done with them -
	 * but before we wake up our parent or this could be missed
	 */
	smp_lock(&lk_rq, LK_RETRY);
	CURRENT_CPUDATA->cpu_noproc = 1;
	p->p_stat = SZOMB;
	smp_unlock(&lk_rq);
	if (q) {
		smp_lock(&lk_waitchk, LK_RETRY);
		q->p_waitchk++;
		smp_unlock(&lk_waitchk);
		psignal(q, SIGCHLD);
		proc_rele(q);
		wakeup((caddr_t)q);
	}

#ifdef mips
	release_tlbpid(p);
#endif mips

	/* this is a jsb to a locore routine that switches to
	 * interrupt stack and then releases the uarea for the process
	 * that is being exited.  Then it drops into the idle loop
	 */
	release_uarea_noreturn();
	/* NOTREACHED */	
}

waitpid()
{
	register struct a {
		int	pid;
		int	*stat_loc;
		int	options;
	} *uap;
	int error;

	uap = (struct a *)u.u_ap;
#ifdef vax
	u.u_error = wait1(uap->options, (struct rusage *)0, uap->pid);
#endif vax
#ifdef mips
	error = wait1(uap->options, (struct rusage *)0, 0, 0, uap->pid);
	/* wait1 can return an error or set u.u_error */
	if (error && !u.u_error)
		u.u_error = error;	/* gak! */
#endif mips
	if (u.u_error)
		return;
	if (uap->stat_loc != (int *)0)
		u.u_error = copyout((caddr_t)&u.u_r.r_val2,
			(caddr_t)uap->stat_loc, sizeof (int));

}


wait()
{
#ifdef vax
	struct rusage ru;
	register struct rusage *rup, *rup2;

	if ((u.u_ar0[PS] & PSL_ALLCC) != PSL_ALLCC) {
		u.u_error = wait1(0, (struct rusage *)0, -1);
		return;
	}
	rup2 = &ru;
	u.u_error = wait1(u.u_ar0[R0], rup2, -1);
	if (u.u_error)
		return;
	rup  = (struct rusage *)u.u_ar0[R1];
	if (rup != (struct rusage *)0)
		u.u_error = copyout((caddr_t)rup2, (caddr_t)rup,
		    sizeof (struct rusage));
#endif vax
#ifdef mips
	struct wait3 {
		union wait *status;	/* user supplied pointer */
		int options;		/* user supplied options */
		struct rusage *ru;	/* user supplied pointer */
	} *uap = (struct wait3 *)u.u_ap;
	int error;

	error = wait1(uap->options, uap->ru, 1, sizeof(struct rusage), -1);
	/* wait1 can return an error or set u.u_error */
	if (error && !u.u_error)
		u.u_error = error;	/* gak! */
	if (u.u_error)
		return;
	if (uap->status != (union wait *)0)
		u.u_error = copyout((caddr_t)&u.u_r.r_val2,
			(caddr_t)uap->status, sizeof (union wait));
#endif mips
}
#ifdef mips
/*
 * mips_wait() provides the same functionality as wait() above but allows
 * the rusage structure to grow so that mips specific stats can be returned
 * in the future.
 */
mips_wait(status, options, ru, rusage_size)
union wait *status;	/* user supplied pointer */
int options;		/* user supplied options */
struct rusage *ru;	/* user supplied pointer */
int rusage_size;	/* user supplied size    */
{
	int error;

	if(rusage_size < 0 || rusage_size > sizeof(struct rusage)){
		return(EINVAL);
	}
	error = wait1(options, ru, 1, rusage_size);
	if (error)
		return(error);
	if (status != (union wait *)0) {
		error = copyout((caddr_t)&u.u_r.r_val2,
			(caddr_t)status, sizeof (union wait));
		return(error);
	}
}
#endif mips

/*
 * Wait system call.
 * Search for a terminated (zombie) child,
 * finally lay it to rest, and collect its status.
 * Look also for stopped (traced) children,
 * and pass back status from them.
 * State must be IDLE while bit is clear in bitmap, so that
 * when process slot is picked up, it will have the right state.
 *
 * We need lk_procqs to walk though the child and sibling pointers,
 * but p_stat and p_trace are under lk_rq.  We don't want to hold
 * both, so we do a single search without lk_rq, and if it fails,
 * we do a second search with it.  Otherwise, we will get missed
 * wakeups, since the stat values can change behind us.
 */
#ifdef vax
wait1(options, ru, pid)
	register int options;
	register struct rusage *ru;
	register int pid;
#endif vax
#ifdef mips
wait1(options, ru, copyflag, rusage_size, pid)
	register int options;
	struct rusage *ru;
	int copyflag;
	int rusage_size;
	int pid;
#endif mips
{
	register int f;
	register struct proc *p, *q;

	if (options & ~(WNOHANG | WUNTRACED)) 
		return(EINVAL);

	if (u.u_procp->p_cptr == NULL)
		return(ECHILD);

	(void) splclock();
	smp_lock(&lk_waitchk, LK_RETRY);
	f = 0;
loop:
	u.u_procp->p_waitchk = 0;
	smp_unlock(&lk_waitchk);
	(void) splclock();
	q=u.u_procp;
	smp_lock(&lk_procqs, LK_RETRY);
	for (p = u.u_procp->p_cptr; p; p = p->p_osptr) {
		/* if stat is ZOMB, it cannot change,
		 * and we can do anything we want to the p
		 * the proc_wait() ensures that the process
		 * is really done under the lk_procqs lock.
		 */
		/* If we want a special pid then check now. */
		if((pid != -1)) {
			if( ((pid > 0) && (pid != p->p_pid)) ||
			   ((pid == 0) && (q->p_pgrp != p->p_pgrp)) ||
			   ((pid < 0) && (-pid != p->p_pgrp)) ) {
				continue;
			}
		}
		f++;
		if ((p->p_stat == SZOMB) && proc_wait(p)) {
			u.u_r.r_val1 = p->p_pid;
			u.u_r.r_val2 = p->p_xstat;
			p->p_xstat = p->p_pid = p->p_ppid = 0;
			if (q = p->p_ysptr)
				q->p_osptr = p->p_osptr;
			if (q = p->p_osptr)
				q->p_ysptr = p->p_ysptr;
			if ((q = p->p_pptr)->p_cptr == p)
				q->p_cptr = p->p_osptr;
			p->p_pptr = p->p_ysptr = p->p_osptr = p->p_cptr = 0;
			smp_unlock(&lk_procqs);
			(void) spl0();
			if (ru && p->p_ru)
#ifdef vax
				*ru = *p->p_ru;
#endif vax
#ifdef mips
				if (copyflag)
					u.u_error = copyout((caddr_t)p->p_ru,
						(caddr_t)ru, rusage_size);
				else
					*ru = *p->p_ru;
#endif mips
			if (p->p_ru) {
				ruadd(&u.u_cru, p->p_ru);
				KM_FREE(p->p_ru, KM_ZOMBIE);
				p->p_ru = 0;
			}
			p->p_sig = p->p_sigcatch = p->p_sigignore = 0;
			p->p_sigmask = p->p_pgrp = 0;
			p->p_file = p->p_sched = p->p_trace = p->p_type = p->p_vm = 0;
			p->p_sid = 0;
			p->p_wchan =0;
			p->p_cursig = 0;
			p->p_ref = 0;
			p->p_stat = SIDL;
			p->p_exist = P_ALIVE;
			clear_slot(p);
			return (0);
		}
		if (p->p_stat == SSTOP && (p->p_trace&SWTED)==0 &&
		    (p->p_trace&STRC || options&WUNTRACED)) {
			smp_unlock(&lk_procqs);
			(void) splclock();
			smp_lock(&lk_rq, LK_RETRY);
			p->p_trace |= SWTED;
			smp_unlock(&lk_rq);
			(void) spl0();
			u.u_r.r_val1 = p->p_pid;
			u.u_r.r_val2 = (p->p_cursig<<8) | WSTOPPED;
			return (0);
		}
	}
	smp_unlock(&lk_procqs);
	(void) spl0();
	if (f == 0)
		return(ECHILD);

	if (options&WNOHANG)
		return(u.u_r.r_val1 = 0);

	/* emulate 4.1 signals if sigintr set */
	if (setjmp(&u.u_qsave)) {
                p = u.u_procp;
                if ((u.u_sigintr & sigmask(p->p_cursig)) != 0)
                        return(EINTR);
                u.u_eosys = RESTARTSYS;
                return (0);
        }

	(void) splclock();
	smp_lock(&lk_waitchk, LK_RETRY);
	if (u.u_procp->p_waitchk == 0) {
		sleep_unlock((caddr_t)u.u_procp, PWAIT, &lk_waitchk);
		smp_lock(&lk_waitchk, LK_RETRY);
	}
	goto loop;
}
