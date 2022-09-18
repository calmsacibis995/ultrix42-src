#ifndef lint
static char *sccsid = "@(#)kern_fork.c	4.6    ULTRIX  3/6/91";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1984, 1986, 1988 by		*
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
 * Modification History: /sys/sys/kern_fork.c
 *
 * 06 Mar 91 	jaw
 *	fix to pid allocation when pids wrap.
 *
 * 28 Feb 91	prs
 *	Added support for a configurable number of
 *	open file descriptors.
 *
 * 4 Sep 90	dlh
 *	added vector support
 *
 * 10 Aug 90 -- jaa
 *	Child inherits p_mips_flag (contains SFIXADE)
 *
 *  13 Feb 90 sekhar
 *	Propagate SPHYSIO bit (if set) to the child process on a vfork.
 *	This is to prevent the child process from being swapped.
 *
 *  11 Dec 89 jaa
 *	change dynamic swap to account for swap up front (ala v3.1) 
 * 	but actually do the allocation only when pushing the page/process
 *
 * 30-Nov-89 -- jmartin
 *	Support for coprocessor access to process virtual memory.
 *
 * 20-Jul-89 jaw
 *	change "bbssi/bbcci" subroutines to set_bit_atomic/clear_bit_atomic.
 *
 * 12 Jun 89 -- gg
 *	Dynamic swap changes in routines fork() and fork1().
 *
 * 06 Apr 89 -- prs
 *	SMP quota locking added.
 *
 * 15 Dec 88 -- jmartin
 *	SMP locking for shared memory data structures (cf. h/shm.h)
 *	Integration of v3.0 shared memory changes, i.e. KM_ALLOC of
 *	per-process shared memory structures.
 *
 * 13 Dec 88 -- jaw 
 *	use define in limits to do MAXPID check.
 *
 * 31 Aug 88 -- jmartin
 *	Move some of vfork finish to vpassvm.  Fix potential race with
 *	child at vfork finish.
 *
 * 28 Jul 88 -- miche
 *	protect remaining scheduling fields in the process structure
 *
 * 25 Jul 88 -- jmartin
 *	Use the macros SET_P_VM and CLEAR_P_VM to replace "|=" and "&=~".
 *
 * 07 Jun 88 -- miche
 *	SMP procqs:  knows about the bitmap.  The new routines nextpid()
 *	returns the next process id;  the routines get_proc_slot() and
 *	clear_proc_slot() get and return proces slots, respectively.
 *	In conjunction, newproc() expects a pointer to the to-be-used
 *	process slot as an argument.
 *
 * 21 Jan 88 -- jmartin
 *	Replace calls to the (inline) functions clearseg and copyseg
 *	respectively with blkclr (or bzero) and blkcpy (or bcopy).
 *	Establish a window in process memory through which a parent
 *	can write to (and read from) the memory of the child.  This
 *	window is UPAGES*NBPG bytes located between the u-area and the
 *	user stack.  Remove the following entities: CMAP1, CADDR1,
 *	CMAP2, CADDR2, Vfmap, vfutl, clearseg, copyseg.  Redefine
 *	Forkmap and forkutl.  Change the computation for the location
 *	of USRSTACK and the size of the process page table.
 *
 ************** SMP changes above ****************
 * 30-Aug-88	jaw
 *	add proc field p_master to fix ASMP bug.
 *
 * 11 Jul 88 -- Mark Parenti
 *	Add support for session ID from POSIX.
 *
 * 12 Jan 88 -- Fred Glover
 *	Prevent inheritance of sys-V file locks (fork1)
 *
 * 22 Dec 87 -- Tim Burke
 *	Copy controling terminal of parent to the new proc.
 *
 * 22 Oct 87 -- map
 *	Initialize prog_env proc structure field on fork
 *	Initialize suid (saved set uid) proc structure field on fork
 *	Initialize guid (saved set gid) proc structure field on fork
 *
 * 16 July 85 -- jrs
 *	Changes to support run queue locking and force master on proc create
 *
 * 11 Nov 85 -- depp
 *	Removed all conditional compiles for System V IPC.
 *
 * 23 Sep 85 -- depp
 *	Added plock handling to newproc
 *
 * 19 Jul 85 -- depp
 *	Added clearing of smend.
 *
 * 4 April 85 -- Larry Cohen   -   001
 *	Changes to support open block if in use.
 *
 * 11 Mar 85 -- depp
 *	Added System V shared memory support.
 *
 * 25 Oct 84 -- jrs
 *	Add changes for proc queue lists
 *	Derived from 4.2BSD, labeled:
 *		kern_fork.c 6.3	84/06/06
 *
 * -----------------------------------------------------------------------
 */

#include "../machine/reg.h"
#include "../machine/pte.h"
#include "../machine/psl.h"
#ifdef mips
#include "../machine/cpu.h"
#endif mips

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/map.h"
#include "../h/cmap.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/kernel.h"
#include "../h/proc.h"
#include "../h/gnode.h"
#include "../h/seg.h"
#include "../h/vm.h"
#include "../h/text.h"
#include "../h/file.h"
#include "../h/acct.h"
#include "../h/quota.h"
#include "../h/lock.h"
#include "../h/smp_lock.h"
#include "../h/cpudata.h"

#include "../h/kmalloc.h"
#include "../h/ipc.h"
#include "../h/shm.h"
#include "../h/limits.h"

#ifdef vax
#include "../machine/vectors.h"
#endif vax

extern struct sminfo sminfo;

/*
 * fork system call.
 */
fork()
{
	if(u.u_procp->p_cdmap = dmalloc(u.u_dsize, CDATA)) {
		if(u.u_procp->p_csmap = dmalloc(u.u_ssize, CSTACK)) {
			fork1(0);
			return;
		}
		dmfree(u.u_procp->p_cdmap, u.u_dsize, CDATA);
		u.u_procp->p_cdmap = (struct dmap *)NULL;
	}
	u.u_r.r_val2 = 0;
	swfail_stat.fork_fail++;
	return;
}

vfork()
{

	fork1(1);
}

fork1(isvfork)
	int isvfork;
{
	register struct proc *p1, *p2;
	register int a;
	extern int maxuprc;

	a = 0;
	if (u.u_uid != 0) FORALLPROC(
		if (pp->p_uid == u.u_uid)
				a++;
	)

	/*
	 * Disallow if
	 *  not su and too many procs owned; or
	 *  too many processes.
	 */
	 if (!(u.u_uid !=0 && a>maxuprc)) {
	 	p2 = (struct proc *)get_proc_slot();
	if (p2==NULL)
		tablefull("proc");
		else p2 = &proc[(int)p2];
#ifdef SMP_DEBUG
		if (smp_debug) {
			if (p2->p_exist != P_ALIVE) {
				cprintf("p2 0x%x\n", p2);
				panic("fork: bad proc slot");
			}
		}
#endif SMP_DEBUG
	}
	if (p2==NULL || (u.u_uid!=0 && a>maxuprc)) {
		u.u_error = EAGAIN;
		if (!isvfork) {
			(void) dmfree(u.u_procp->p_cdmap, u.u_dsize, CDATA);
			(void) dmfree(u.u_procp->p_csmap, u.u_ssize, CSTACK);
		}
		goto out;
	}
	p1 = u.u_procp;
#ifdef vax
	if (p1->p_vpcontext) {
		if (vp_allocate(p2) == VP_FAILURE) {
		    if (!isvfork) {
			(void) dmfree(u.u_procp->p_cdmap, u.u_dsize, CDATA);
			(void) dmfree(u.u_procp->p_csmap, u.u_ssize, CSTACK);
		    }
                    goto out;
		}
		if (p1->p_vpcontext->vpc_state != VPC_SAVED) {
			vp_contextsave (p1);
		}
		bcopy (p1->p_vpcontext, p2->p_vpcontext, 
		    ((sizeof (struct vpcontext)) - (sizeof (char *))) );
		bcopy (p1->p_vpcontext->vpc_vregs, p2->p_vpcontext->vpc_vregs,
			VPREGSIZE);
	}
#endif vax
#ifdef mips
	/* fp bug from dana */
	if (!isvfork)
		checkfp(p1,0);

#endif mips
	if (newproc(p2, isvfork)) {
		u.u_r.r_val1 = p1->p_pid;
		u.u_r.r_val2 = 1;  /* child */
		u.u_start = time;
		u.u_acflag = AFORK;
#ifdef mips
#ifdef notdef
		if (u.u_procp->p_flag & STRC) {
			if (u.u_pcb.pcb_ssi.ssi_cnt)
				remove_bp();
			u.u_trapcause = CAUSEFORK;
			u.u_trapinfo = u.u_procp->p_ppid;
			psignal(u.u_procp, SIGTRAP);
		}
#else isdef
		if (u.u_pcb.pcb_ssi.ssi_cnt)
			remove_bp();
#endif notdef
#endif mips
		/*
		 *	Child must not inherit Sys-V file locks
		 */
		for (a = 0; a <= u.u_omax; a++) {
			U_POFILE_SET(a, U_POFILE(a) & ~UF_FDLOCK);
		}
#ifdef vax
		if (u.u_procp->p_vpcontext) {
			if ((u.u_acflag & AVP) != AVP) {
				uprintf ("setting AVP flag for proc #%d\n",
					u.u_procp->p_pid);
				u.u_acflag |= AVP;
			}
		}
#endif
		return;
	}
#ifdef vax
	if (u.u_procp->p_vpcontext) {
		if ((u.u_acflag & AVP) != AVP) {
			uprintf ("setting AVP flag for proc #%d\n",
				u.u_procp->p_pid);
			u.u_acflag |= AVP;
		}
	}
#endif
	u.u_r.r_val1 = p2->p_pid;

out:
	u.u_r.r_val2 = 0;
	u.u_procp->p_cdmap = (struct dmap *)NULL;
	u.u_procp->p_csmap = (struct dmap *)NULL;
}

/*
 * Create a new process-- the internal version of
 * sys fork.
 * It returns 1 in the new process, 0 in the old.
 */
newproc(rpp, isvfork)
	register struct proc *rpp;
	register int isvfork;
{
	register struct proc *rip;
	register int n;
	register int plock;
	struct p_sm *cp_sm, *pp_sm;

	/*
	 * Make a proc table entry for the new process.
	 */
	rip = u.u_procp;
#ifdef QUOTA
	rpp->p_quota = rip->p_quota;
	quota_lock(rpp->p_quota);
	rpp->p_quota->q_cnt++;
	quota_unlock(rpp->p_quota);
#endif
	timerclear(&rpp->p_realtimer.it_value);
	/*
	 * we are trying moving this down to avoid getting the
	 * lock twice, but things may die if this isn't set.
	 *
	 *	rpp->p_sched = SLOAD;
	 */
	rpp->p_vm = rip->p_vm & SPAGI;
	rpp->p_smbeg = 0;
	rpp->p_smend = 0;
	rpp->p_smsize = 0;	
#ifdef mips
	rpp->p_puac = rip->p_puac;
	rpp->p_mips_flag = rip->p_mips_flag;
#endif mips
	rpp->p_smcount = 0;
	rpp->p_sm = (struct p_sm *) NULL;
	if (isvfork) {
		rpp->p_vm |= (SVFORK | rip->p_vm & SPHYSIO);
		rpp->p_ndx = rip->p_ndx;
		rpp->p_textp = 0;
		rpp->p_tsize = rpp->p_dsize = rpp->p_ssize = 0;
#ifndef mips
		rpp->p_szpt = clrnd(ctopt(HIGHPAGES));
#else mips
		rpp->p_textpt = 0;
		rpp->p_datapt = 0;
		rpp->p_stakpt = 0;
#endif mips
	} else {
		rpp->p_ndx = rpp - proc;
		rpp->p_textp = rip->p_textp;
		rpp->p_tsize = rip->p_tsize;
		rpp->p_dsize = rip->p_dsize;
		rpp->p_ssize = rip->p_ssize;
#ifndef mips
		rpp->p_szpt = rip->p_szpt;
#else mips
		rpp->p_textpt = 0;
		rpp->p_datapt = rip->p_datapt;
		rpp->p_stakpt = rip->p_stakpt;
#endif mips
		if(rip->p_sm != (struct p_sm *) NULL) {
			if(sminfo.smseg == 0) {
				panic("newproc: parent has smem, smseg == 0");
			}
			KM_ALLOC(rpp->p_sm, struct p_sm *, SIZEOF_PSM, KM_SHMSEG, KM_CLEAR);
			if(rpp->p_sm == (struct p_sm *) NULL) {
				panic("newproc: alloc p_sm");
			}
			rpp->p_smbeg = rip->p_smbeg;
			rpp->p_smend = rip->p_smend;
			rpp->p_smsize = rip->p_smsize;	
			rpp->p_smcount = rip->p_smcount;
		}
	}
	rpp->p_uid = rip->p_uid;
	rpp->p_suid = rip->p_suid;
	rpp->p_sgid = rip->p_sgid;
	rpp->p_pgrp = rip->p_pgrp;
	rpp->p_sid = rip->p_sid;
	rpp->p_ttyp = rip->p_ttyp;
	rpp->p_nice = rip->p_nice;
#ifdef mips
	rpp->p_usrpri = rip->p_usrpri;
#endif mips
	rpp->p_pid = nextpid();
	rpp->p_ppid = rip->p_pid;
	rpp->p_pptr = rip;

	if (isvfork) {  /* don't bother lock - eventually put in percpu */
		forkstat.cntvfork++;
		forkstat.sizvfork += rip->p_dsize + rip->p_ssize;
	} else {
		forkstat.cntfork++;
		forkstat.sizfork += rip->p_dsize + rip->p_ssize;
	}
	(void) spl5();
	smp_lock(&lk_procqs, LK_RETRY);
	rpp->p_osptr = rip->p_cptr;
	if (rip->p_cptr)
		rip->p_cptr->p_ysptr = rpp;
	rpp->p_ysptr = NULL;
	rpp->p_cptr = NULL;
	rip->p_cptr = rpp;
	smp_unlock(&lk_procqs);
	(void) spl0();
	rpp->p_time = 0;
	rpp->p_cpu = 0;
	rpp->p_nice = rip->p_nice;
	rpp->p_progenv = rip->p_progenv;
	rpp->p_sigmask = rip->p_sigmask;
	rpp->p_sigcatch = rip->p_sigcatch;
	rpp->p_sigignore = rip->p_sigignore;
	rpp->p_sig = 0;	/* clear pending signals */
	rpp->p_sigflag = 0; /* clear signal flags */
#ifdef mips
	rpp->p_tlbpid = -1;
	rpp->p_datastart = rip->p_datastart;
	rpp->p_textstart = rip->p_textstart;
	rpp->p_fp = 0;
	rpp->p_dev_VM_maint = rip->p_dev_VM_maint;
#endif mips
	rpp->p_rssize = 0;
	rpp->p_maxrss = rip->p_maxrss;
	rpp->p_wchan = 0;
	rpp->p_slptime = 0;
	rpp->p_pctcpu = 0;
	rpp->p_cpticks = 0;

	(void) spl5();
	smp_lock(&lk_pid, LK_RETRY);
	n = PIDHASH(rpp->p_pid);
	rpp->p_idhash = pidhash[n];
	pidhash[n] = rpp - proc;
	multprog++;
	smp_unlock(&lk_pid);
	(void) spl0();

	gref(u.u_cdir);
	if (u.u_rdir)
		gref(u.u_rdir);
	crhold(u.u_cred);
	
	/*
	 * This begins the section where we must prevent the parent
	 * from being swapped.
	 */
	SET_P_VM(rip, SKEEP);

	/* temporarily unlock parent's segments so that both fork and vfork
	 * will work without children receiving locked segments.  Swapping
	 * will not occur due to SKEEP (p_vm).  However, there is a small
	 * window for paging to occur.
	 */
	 if ((plock = (u.u_lock & (TXTLOCK|DATLOCK)))) 
	 	punlock();
	if (procdup(rpp, isvfork)) 
		return (1);

	/*
	 * As soon as we sleep, we can be swapped.
	 */
	CLEAR_P_VM(rip, SKEEP);

	/*
	 * Make child runnable and add to run queue.
	 */
#ifdef vax
	(void) spl6();
#endif vax
#ifdef mips
	(void) splclock();
#endif mips
	smp_lock(&lk_rq,LK_RETRY);
	rpp->p_sched = SLOAD;
	rpp->p_affinity=ALLCPU;
	rpp->p_stat = SRUN;
	setrq(rpp);
	smp_unlock(&lk_rq);
	(void) spl0();

	/*
	 * Cause child to take a non-local goto as soon as it runs.
	 * On older systems this was done with SSWAP (p_vm) bit in proc
	 * table; on VAX we use u.u_pcb.pcb_sswap so don't need
	 * to do rpp->p_vm |= SSWAP.  Actually do nothing here.
	 */
	/* rpp->p_vm |= SSWAP; */

	/*
	 * If vfork make chain from parent process to child
	 * (where virtal memory is temporarily).  Wait for
	 * child to finish, steal virtual memory back,
	 * and wakeup child to let it die.
	 */
	if (isvfork) {
		(void)splimp();
		for (;;) {
			smp_lock(&lk_p_vm, LK_RETRY);
			if (rpp->p_vm & SVFORK)
				sleep_unlock((caddr_t)rpp, PZERO-1, &lk_p_vm);
			else {
				smp_unlock(&lk_p_vm);
				break;
			}
		}
		/*
		 * Here we insure that the child is not running before
		 * exchanging virtual memory resources with it.  At
		 * this point, the child is known to have given up
		 * lk_p_vm, as we have just succeeded in taking it.
		 * However, the child is still running until it gives
		 * up lk_rq in swtch(), having taken it in
		 * sleep_unlock (cf. call to sleep_unlock from vrelvm
		 * in sys/vm_proc.c).  The following seemingly useless
		 * code makes sure that the child has stopped running
		 * on the stack which we are about to remap.
		 */
		(void)spl6();
		smp_lock(&lk_rq, LK_RETRY);
		smp_unlock(&lk_rq);
		(void)spl0();

		if ((rpp->p_sched & SLOAD) == 0)
			panic("newproc vfork");
		uaccess(rpp, Forkmap, &forkutl);
		vpassvm(rpp, u.u_procp, &forkutl, &u, Forkmap);
		rpp->p_ndx = rpp - proc;
		SET_P_VM(rpp, SVFDONE);
#ifdef mips
		/*
		 * do not know what the child did with my address space.
		 */
		release_tlbpid(u.u_procp);
		get_tlbpid(u.u_procp);
		clear_tlbmappings(u.u_procp);
		set_tlbwired(u.u_procp);
#endif mips
		wakeup((caddr_t)rpp);
	}
	/* Now relock parent's segments if required */
	if (plock&TXTLOCK)
		textlock();
	if (plock&DATLOCK)
		datalock();

	/*
	 * 0 return means parent.
	 */
	return (0);
}

/*
 * Routine: get_proc_slot
 * 	arguments:	none
 *	returns:	index into process table
 *
 * This routine searches the bitmap looking for a free
 * process slot.  When it finds one, it marks it as
 * being busy and returns.
 *
 * We count on the wait routine to have set the state of
 * the unused process slot to PIDLE.
 */
get_proc_slot()
{
	register int bit;
	register unsigned long *pb = proc_bitmap;
	register unsigned long *lastpb = &proc_bitmap[max_proc_index];

	for ( ; pb < lastpb; pb++) {	/* for all indices */
		while ( ~(*pb) ) {	/* if any bits clear */
			bit = ffs(~(*pb)) - 1; /* 0 based */
			if (set_bit_atomic(bit, pb)) {
				/* We got it:  multiply the index
				 * by 32 and add the bit value
				 */
				bit += (long)(pb - proc_bitmap) << 5;
				return(bit);
			}
			/* someone else got it */
		}
	}
	/*
	 * There are no process slots available
	 * The 0 return is kludge assuming that the swapper
	 * is process 0 and never gives up its slot, and
	 * that we don't use this subroutine to get it.
	 */
	u.u_error = EAGAIN;
	return(0);
}

clear_slot(p)
	struct proc *p;
{
	register unsigned index;
	register int bit;
	index = p - proc;
	bit = index % 32;	/* zero based */
	index = index >> 5;	/* divide by 32 */
	clear_bit_atomic(bit, &proc_bitmap[index]);
}

/*
 * avoid searching for our pid until
 * we've at least wrapped once
 */
int pidchecked = PID_MAX;
extern mpid;

/*
 * Get the next PID under lock, and then ensure that
 * no one else already has it.
 */
nextpid()
{
	register int pid;
	(void) spl5();
	smp_lock(&lk_pid, LK_RETRY);

	pid = mpid++;
	if (pid >= PID_MAX) {
		mpid = 100;
		pidchecked = 0;
	}
retry:
	/*
	 * make sure pid is not in use.
	 * while we are at it, remember the lowest pid that we see
	 * so we won't have to check the intervening numbers again.
	 * Note that the pgrp check here is rather stupid, since anyone
	 * can set any process to use any pgrp at any time....
	 */
	if (mpid > pidchecked) {
		pidchecked = PID_MAX;

  	     FORALLPROC (
		if (pid == pp->p_pid || pid == pp->p_pgrp ||
		   pid == pp->p_sid) {
			pid = mpid++;
			if (pid > pidchecked)
				goto retry;
		}
		if (pp->p_pid > mpid && pidchecked > pp->p_pid)
			pidchecked = pp->p_pid;
		if (pp->p_pgrp > mpid && pidchecked > pp->p_pgrp)
			pidchecked = pp->p_pgrp;
		)
	}
	smp_unlock(&lk_pid);
	(void) spl0();
	return(pid);
}
