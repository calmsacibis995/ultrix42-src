#ifndef lint
static char *sccsid = "@(#)uipc_smem.c	4.4	ULTRIX	11/9/90";
#endif lint

/***********************************************************************
 *									*
 *			Copyright (c) 1985,89 by			*
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
 **********************************************************************/
/*
 *
 *   Modification history:
 *
 * 4 Sep 90	dlh
 *	added vector processor support code
 *
 * 13-Aug-90 -- sekhar
 *	removed all mips conditionals from code changes for 
 *	memory mapped devices ; code is now common to both vax and mips.
 *
 * 13-Jul-90 -- jmartin
 *	Apply "clrnd" to sm_size as well as "btoc".
 *
 *  23 Apr 90 sekhar
 *	Modifications to handle memory mapped devices:
 *  	changes to smctl(), smsys() , smat() for mmap.
 *
 *  11 Dec 89 jaa
 *	change dynamic swap to account for swap up front (ala v3.1) 
 * 	but actually do the allocation only when pushing the page/process
 *
 * 30-Nov-89 -- jmartin
 *	Support for coprocessor access to process virtual memory.
 *
 * 09-Nov-89 - jaw
 *	remove asymmetric system call support.  
 *
 * 17 Oct 89 - jaa
 *	fixed smdt() on mips to invalidate tlb 
 *
 * 24 Jul 89 - jaa
 *	fixed smget() to correct "panic: smccdec smseg" on err leg
 *
 * 14 Jun 89 -- jaa
 *	This is only for system call interfaces.  The rest of the
 *	shared memory code has been moved to ../vm/.
 *	Also broke out machine specif routines
 *
 * 12 Jun 89 -- gg
 *	dynamic swap changes - changed call  vssmalloc to dmalloc().
 *
 * 09 Jun 89 -- scott
 *	Added audit support
 *
 * 15 Dec 88 -- jmartin
 *	SMP locking for shared memory data structures (cf. h/shm.h)
 *	Integration of v3.0 shared memory changes, i.e. KM_ALLOC of
 *	per-process shared memory structures.
 *
 * 09 Dec 88 -- depp
 *	Forced 4 Mb alignment for mips
 *
 * 17 Nov 88 -- depp
 *	Numberous mips shared memory fixes.
 *
 * 25 Jul 88 -- jmartin
 *	Use the SMP lock lk_p_vm to protect per-process VM data.  Replace
 *	"|=" and "&= ~" with macros SET_P_VM and CLEAR_P_VM.
 *
 * 9 Jun 88 -- jaa
 *	Added fixes to correct sm_daddr panic in vssmfree().
 *
 * 3 Mar 88 -- jaa
 *	Allowed highest attachable shared memory addr 
 *	to be configurable (sminfo.smsmat)
 *	Dynamically allocate shared mem segments. These 
 *	are also configurable (sminfo.smseg)
 *
 * 09 Feb 88 -- depp
 *      Changed semantic if user attempts to attach to the same smid more than
 *      once.  It used to silently return (causing SVVS to hang), now it will 
 *      return an error.  It seems that a SYSV side effect is to permit a 
 *      single process to share a SMS with itself.  We DON'T!
 *
 * 15 Dec 86 -- depp
 *	Fixed process SM page table swap error
 *
 * 18 Aug 86 -- depp
 *	Fixed a couple of range checking bugs in "smat()"
 *
 * 29 Apr 86 -- depp
 *	converted to locking macros from calls routines
 *
 * 12-Feb-86 -- jrs
 *	Added calls to tbsync() for mp translation buffer control
 *
 * 03 Jan 86 -- depp
 *	Modified "smat" to insure that multiple attaches on the same 
 *	shmid can not be done.  If an attach is attempted on an attached SMS,
 *	smat silently returns the address of the SMS.
 *
 *	Also, in "smdt", smfree was moved before the clearing of the detached
 *	PTEs, so that the modification bit (PG_M) could be properly
 *	propagated to the global PTEs.
 *
 * 11 Nov 85 -- depp
 *	Removed all conditional compiles for System V IPC.
 *
 * 30 Sep 85 -- depp
 *	Added shared memory segment locking to a number of routines
 *
 * 26 Jul 85 -- depp
 *	Insured that an error will be returned in smat if the process
 *	has advised that pageouts should be sequential (SSEQL)
 *
 * 19 Jul 85 -- depp
 *	Cleaned up some shared memory code.  Renamed smexec to smclean.
 *	Removed some commented out code.
 *
 * 29 Apr 85 -- depp
 *	Removed the test for maximum shared memory to be allocated for
 *	system, as virtual memory eliminates the need.
 *
 * 24 Apr 85 -- depp
 *	Fixed bug in smat, the beginning of SM segments was incorrectly 
 *	calculated.
 *
 * 11 Mar 85 -- depp
 *	1) - this file ported from System V and modified to	 
 *		handle the very different Ultrix shared memory	 
 *		approach.					 
 *		LeRoy Fundingsland    1 10 85    DEC		 
 *
 */
#ifdef vax
#include "../machine/mtpr.h"
#endif vax
#ifdef mips
#include "../machine/cpu.h"
#endif mips
#include "../machine/pte.h"

#include "../h/param.h"		/* includes types.h & vax/param.h */
#include "../h/dir.h"
#include "../h/user.h"		/* includes errno.h		*/
#include "../h/proc.h"
#include "../h/seg.h"
#include "../h/cmap.h"
#include "../h/vm.h"

#include "../h/kmalloc.h"
#include "../h/ipc.h"
#include "../h/shm.h"
#include "../h/cpudata.h"
#ifdef vax
#include "../machine/cpu.h"
#include "../../machine/common/cpuconf.h"
#include "../machine/vectors.h"
#endif

int	smtot;		/* total shared memory currently used (clicks)*/

extern struct timeval	time;		/* system idea of date */
extern struct smem smem[];
extern struct sminfo sminfo;

struct smem	*ipcget(),
		*smconv();

/* SMAT - shared memory attach system call. Attach the		*/
/*	specified shared memory segment to the current process.	*/
smat(smid, addr, flag)
	int	smid;
	char	*addr;
	int	flag;
{
	register struct smem *sp;	/* shared memory header ptr */
	register struct proc *p;
	int i, smindex;
	int s;

	if((sp = smconv(smid, SM_DEST)) == NULL)
		return(-1);
	if(ipcaccess(&sp->sm_perm, SM_R)) 
		return(-1);
	if((flag & SM_RDONLY) == 0)
		if(ipcaccess(&sp->sm_perm, SM_W)) 
			return(-1);
	p = u.u_procp;

	/* Shared memory is currently not supported with SSEQL advisory */
	s = splimp();
	smp_lock(&lk_p_vm, LK_RETRY);
	if (p->p_vm & SSEQL) {
		smp_unlock(&lk_p_vm);
		(void)splx(s);
		u.u_error = EINVAL;
		return(-1);
	}
	smp_unlock(&lk_p_vm);
	(void)splx(s);

	if(sminfo.smseg <= 0){
		u.u_error = EINVAL;
		return(-1);
	}

	/* allocate on first attach */
	if(p->p_sm == (struct p_sm *)NULL){
		if(p->p_smcount) 
			panic("smat: smbeg");
		smindex = i = 0;
		KM_ALLOC(p->p_sm, struct p_sm *, SIZEOF_PSM, KM_SHMSEG, KM_CLEAR);
	} else  {
		i = -1;
		for(smindex = 0; smindex < sminfo.smseg; smindex++) {
			if (i == -1 && p->p_sm[smindex].sm_p == 0) {
				i = smindex;
				continue;
			}
		
			/* 
			 * if already attached, then return an error
			 * (see history note above, dated: 9-Feb-88
			 */
			if (p->p_sm[smindex].sm_p == sp) {
				u.u_error = EMFILE;
				return(-1);
			}
		}

		/* No open slots?? */
		if ((smindex = i) == -1) {
			u.u_error = EMFILE;
			return(-1);
		}
	}

	if (flag & SM_RND)
		addr = (char *)((u_int)addr & ~(SMLBA - 1));

	if (SM_CHKALIGN(addr, sp->sm_size)) {
		u.u_error = EINVAL;
		return(-1);
	}

	/* Is the requested address too high? */
	if (sminfo.smsmat && (((int)addr + sp->sm_size) >= sminfo.smsmat)) {
		u.u_error = EINVAL;
		return(-1);
	}

	/*
	 * lock segment. 
	 * note: if sm_attach can sleep (VAX does, MIPS doesn't), 
	 * must unlock, then relock around the sleep condition
	 */
	SM_LOCK(sp);

	/* find and attach to the users VAS */
	if (sm_attach(p, sp, addr, flag)) {
		SM_UNLOCK(sp);
		return(-1);
	}

	/* finish up */
	sp->sm_atime = (time_t) time.tv_sec;
	sp->sm_lpid = p->p_pid;
	SM_UNLOCK(sp);
	return(u.u_r.r_val1);
}


/* SMCTL - shared memory control operations system call.	*/
/*	Provides three functions to the user: "stat" of the	*/
/*	"smem" struct, set the user-group-mode for the segment,	*/
/*	and allow the user to remove the "smem" and destroy the	*/
/*	shared memory segment.					*/
smctl(smid, cmd, arg)
	int	smid,
		cmd;
	struct smem *arg;
{
	register struct smem *sp;	/* shared memory header ptr */
	register struct p_sm *psmp;	/* see proc.h		*/
	register struct proc *p;	/* proc pointer */
	register int i;
	struct smem ds;			/* hold area for IPC_SET */
	int s;				/* saved IPL */
	int nbytes;

	/* we don't need a sp for these since we're just returning info */
	if((sp = smconv(smid, (cmd == IPC_STAT)?0:SM_DEST)) == NULL)
		return(-1);
	switch(cmd){

		/* Remove shared memory identifier. */
	case IPC_RMID:
		/*
		 *	shared memory segments created by mmap system
		 *	call are removed when the reference count 
		 *	drops to zero. 
		 */
		if (sp->sm_perm.mode & IPC_MMAP) {
			u.u_error = EPERM;
			return(-1);
		}
		if(u.u_uid != sp->sm_perm.uid  &&  
				u.u_uid != sp->sm_perm.cuid  &&
				!suser())
			return(-1);
		sp->sm_ctime = (time_t) time.tv_sec;
		sp->sm_perm.mode |= SM_DEST;

		/* Change key to "private" so old key can be	*/
		/* reused without waiting for last detach. Only	*/
		/* allowed accesses to this segment now are	*/
		/* smdt() and smctl(IPC_STAT). All others will	*/
		/* give "bad smem".				*/
		sp->sm_perm.key = IPC_PRIVATE;

			/* If there are no processes attached	*/
			/* to this SMS then delete it.		*/
			/* If there are attached processes then	*/
			/* the SM_DEST flag will cause the SMS	*/
			/* to be deleted on the last detach.	*/
		s = splimp();
		smp_lock(&lk_smem, LK_RETRY);
		if(sp->sm_count == 0) {
			sp->sm_count = 1;
			smp_unlock(&lk_smem);
			(void)splx(s);
			smfree(sp);
		} else {
			smp_unlock(&lk_smem);
			(void)splx(s);
		}
		return(0);

		/* Set ownership and permissions. */
	case IPC_SET:
		if (sp->sm_perm.mode & IPC_SYSTEM) {
			u.u_error = EPERM;
			return(-1);
		}

		if(u.u_uid != sp->sm_perm.uid  &&  
				u.u_uid != sp->sm_perm.cuid  &&
				!suser())
			return(-1);
		if(copyin(arg, &ds, sizeof(ds))){
			u.u_error = EFAULT;
			return(-1);
		}
		sp->sm_perm.uid = ds.sm_perm.uid;
		sp->sm_perm.gid = ds.sm_perm.gid;
		sp->sm_perm.mode = (ds.sm_perm.mode & 0777) |
					(sp->sm_perm.mode & ~0777);
		sp->sm_ctime = (time_t) time.tv_sec;
		return(0);

		/* Get shared memory data structure. */
	case IPC_STAT:
		if(ipcaccess(&sp->sm_perm, SM_R) != 0)
			return(-1);
		if(copyout(sp, arg, sizeof(*sp))) {
			u.u_error = EFAULT;
			return(-1);
		}
		return(0);

	case SHM_LOCK:
		if (sp->sm_perm.mode & IPC_SYSTEM) {
			u.u_error = EPERM;
			return(-1);
		}

		if (!(suser()))
			return(-1);

		/* find segment	in proc table	*/
		p = u.u_procp;
		if((psmp = p->p_sm) == (struct p_sm *) NULL){
			u.u_error = EINVAL;
			return(-1);
		}
		for (i=0; i < sminfo.smseg; i++, psmp++)
			if (psmp->sm_p == sp)
				break;
		if (i >= sminfo.smseg || psmp->sm_lock) {
			u.u_error = EINVAL;
			return(-1);
		}

		/*
		 * silently return on double lock by same proc or 
		 * vfork child/parent 
		 */
		if (psmp->sm_lock)
		        return(0);

		psmp->sm_lock++;
		s = splimp();
		smp_lock(&lk_smem, LK_RETRY);
		sp->sm_flag |= SMNOSW;
		sp->sm_lcount++;
		smp_unlock(&lk_smem);
		(void)splx(s);
		return(0);

	case SHM_UNLOCK:
		if (sp->sm_perm.mode & IPC_SYSTEM) {
			u.u_error = EPERM;
			return(-1);
		}

		if (!(suser()))
			return(-1);

		/* find segment in proc table */
		p = u.u_procp;
		if((psmp = p->p_sm) == (struct p_sm *) NULL) {
			u.u_error = EINVAL;
			return(-1);
		}
		for (i=0; i < sminfo.smseg; i++, psmp++)
			if (psmp->sm_p == sp)
				break;
		if (i >= sminfo.smseg) {
			u.u_error = EINVAL;
			return(-1);
		}

		/* If not locked or this proc didn't lock it; error */
		if (!(psmp->sm_lock)) {
			u.u_error = EINVAL;
			return(-1);
		}
		s = splimp();
		smp_lock(&lk_smem, LK_RETRY);
		if (!(sp->sm_flag & SMNOSW)) {
			u.u_error = EINVAL;
			smp_unlock(&lk_smem);
			(void)splx(s);
			return(-1);
		}

		if (--(sp->sm_lcount) == 0)
			sp->sm_flag &= ~SMNOSW;
		smp_unlock(&lk_smem);
		(void)splx(s);
		/* unlock SMS (at least from this proc's point of view */
		psmp->sm_lock = 0;
		return(0);

	default:
		u.u_error = EINVAL;
		return(-1);
	}
}


/* SMDT - shared memory detach system call. Detach the		*/
/*	specified shared memory segment from the current	*/
/*	process.						*/
smdt(addr, type)
	char *addr;
	int  type;	/* SHMT_MMAP => segment created by mmap  */
			/* SHMT_SHM  => segment created by smget */
{
	register struct p_sm *psmp;	/* see proc.h		*/
	register struct smem *sp;
	register struct proc *p;
	register int *seg;
	register int i, j;
	struct p_sm *osmp;
	int index;
	int segbeg;

	/* Check for page alignment		*/
	if ((int)addr & (ctob(1) - 1)  ||
			(segbeg = btop(addr)) == 0) {
		u.u_error = EINVAL;
		return(-1);
	}

	/* find segment				*/
	p = u.u_procp;
	if((psmp = p->p_sm) == (struct p_sm *) NULL) {
		u.u_error = EINVAL;
		return(-1);
	}
	for (i = 0; i < sminfo.smseg; i++, psmp++)
		if (psmp->sm_p != NULL && psmp->sm_saddr == segbeg)
			break;
	if (i >= sminfo.smseg) {
		u.u_error = EINVAL;
		return(-1);
	}
	sp = psmp->sm_p;
	/* 
	 * 	shared memory segments created because of mmap
	 *	system call can only be detached by munmap 
	 *	and shm segments created by shmget system call
	 *	can only be detached using the shmdt call
	 */

	if (    (sp->sm_perm.mode & IPC_MMAP && (type != SHMT_MMAP))
	     || (!(sp->sm_perm.mode & IPC_MMAP) && (type != SHMT_SHM))) {
		u.u_error = EINVAL;
		return(-1);
	}

	/* if this process has SMS locked, then unlock */
	if (psmp->sm_lock) {
		int s;

		psmp->sm_lock = 0;
		s = splimp();
		smp_lock(&lk_smem, LK_RETRY);
		if (--(sp->sm_lcount) == 0)
			sp->sm_flag &= ~SMNOSW;
		smp_unlock(&lk_smem);
		(void)splx(s);
	}
	smfree(sp);

#ifdef vax
	/* clear the PTEs for this SMS		*/
	i = clrnd(btoc(sp->sm_size));
	seg = (int *)mfpr(P0BR) + segbeg;
	while (i--)
		*seg++ = 0;

	/* Quiesce the vector processor if necessary */
	VPSYNC ();

	mtpr(TBIA, 0);
	tbsync();
#endif vax
#ifdef mips
	release_tlbpid(p);
	get_tlbpid(p);
	clear_tlbmappings(p);
	set_tlbwired(p);
	unmaptlb(p->p_tlbpid, btop((caddr_t)sp->sm_ptaddr));
	newptes(p, segbeg, clrnd(btoc(sp->sm_size)));
#endif mips
	(void) clear_foreign_tlbs(p, segbeg, clrnd(btoc(sp->sm_size)), CSYS);

	sm_del_psm(p, psmp - p->p_sm);
        if (p->p_smcount) {
		SM_PSM_UPDATE(p);
	} else  {
		SM_PSM_CLEAR(p);
	}

        u.u_smsize = p->p_smsize;

	sp->sm_dtime = (time_t) time.tv_sec;
	sp->sm_lpid = p->p_pid;
	return(0);
}


/* SMGET - get shared memory segment system call.		*/
smget(key,size,smflag)
	int key;
	int size;
	int smflag;
{
	register struct smem *sp;	/* shared memory header ptr */
	register int tmp_size, i;
	register struct pte *pte;
	int s;				/* ipcget status	*/
	struct fpte proto;
	int a;				/* saved IPL		*/

	u.u_error = 0; 	/* assume no error until we get one */
	a = splimp();
	for (;;) {
		smp_lock(&lk_smem, LK_RETRY);
		if ((sp = ipcget(key, smflag, &smem[0], sminfo.smmni,
					sizeof(*sp), &s)) == NULL) {
			smp_unlock(&lk_smem);
			(void)splx(a);
			return(-1);
		}
		if (sp->sm_flag & SMLOCK) {
			sp->sm_flag |= SMWANT;
			sleep_unlock((caddr_t)sp, PSWP, &lk_smem);
			if (s)
				ipcfree(sp->sm_perm);
		} else
			break;
	}
	sp->sm_flag |= SMLOCK;
	smp_unlock(&lk_smem);
	(void)splx(a);
	SET_P_VM(u.u_procp, SKEEP);

	if(s){ 
		/* This is a new shared memory segment, set it up */
		/* If user/kernel sharing, limits have been checked */

		tmp_size = clrnd((int)btoc(size));
		sp->sm_size = size;
		if ((smflag & IPC_SYSTEM) == 0) {
			if(size < sminfo.smmin  || 
			   size > sminfo.smmax ||
			   size < 1) {
				u.u_error = EINVAL;
				ipcfree(sp->sm_perm);
				goto fail;
			}
			if (vssmalloc(sp) == 0) {
				swfail_stat.shm_dmap_fail++;
				u.u_error = ENOMEM;
				ipcfree(sp->sm_perm);
				goto fail;
			}
		}

		/* allocate the page table for this SMS */
		if(vgetsmpt(sp) == 0){
			u.u_error = ENOMEM;
			vssmfree(sp);
			ipcfree(sp->sm_perm);
			goto fail;
		}

		/* if normal, then create ZFOD PTES, else ignore 
		 * Note: protection is Read only and tlbmod() will
		 * adjust if it should be read/write
		 */
		if ((smflag & IPC_SYSTEM) == 0) {
			pte = sp->sm_ptaddr;
#ifndef mips
			*(u_int*) &proto = (u_int) 0;
#else mips
			*(u_int*) &proto = (u_int) PG_UW;
#endif mips
			proto.pg_fod = 1;
			proto.pg_fileno = PG_FZERO;
			for ( i = 0; i < tmp_size; i++, pte++) 
				*(int *)pte = *(int*) &proto;
		}

		smtot += tmp_size;
		sp->sm_count = sp->sm_ccount = 0;
		sp->sm_atime = sp->sm_dtime = 0;
		sp->sm_ctime = (time_t) time.tv_sec;
		sp->sm_lpid = 0;
		sp->sm_cpid = u.u_procp->p_pid;

	} else  { /* not new, so simply insure requested size within SMS */
		if(size  &&  size > sp->sm_size) 
			u.u_error = EINVAL;
	}

fail:
	CLEAR_P_VM(u.u_procp, SKEEP);
	SM_UNLOCK(sp);
	return((u.u_error ? -1 : sp->sm_perm.seq * sminfo.smmni + (sp - smem)));

}


/* SMSYS - System entry point for SMAT, SMCTL, SMDT, and SMGET	*/
/*	system calls.						*/
smsys()
{
	register struct a {
		u_int	id;
		int     parm1,
		        parm2,
		        parm3;
	} *uap = (struct a *)u.u_ap;
	register int retarg;
	int saveaffinity;
	int	smat(),
		smctl(),
		smdt(),
		smget();
	static int (*calls[])() = {smat, smctl, smdt, smget};

	/*  NOT MP SAFE YET....TODO */ 
	saveaffinity = switch_affinity(boot_cpu_mask);

	switch (uap->id) {
	      case 0:
	      case 1:
		u.u_r.r_val1 = 
			(*calls[uap->id])(uap->parm1, uap->parm2, uap->parm3);
		break;

	      case 2:
		u.u_r.r_val1 = (*calls[uap->id])(uap->parm1, SHMT_SHM);
		break;

	      case 3:
		/*
		 * 	do not permit users to pass IPC_SYSTEM flag 
		 *	or the IPC_MMAP flag which is used to implement
	    	 *	memory mapped devices.
		 *
		 */
		uap->parm3 = uap->parm3 & ~(IPC_SYSTEM | IPC_MMAP);
		u.u_r.r_val1 = 
			(*calls[uap->id])(uap->parm1, uap->parm2, uap->parm3);
		break;

	      default:
		if(uap->id > 3){
			u.u_error = EINVAL;
			switch_affinity(saveaffinity);
			return;
		}
		break;
	}
	if(u.u_error) 
		u.u_r.r_val1 = 0;
	AUDIT_CALL_SHM ( u.u_error, u.u_r.r_val1, AUD_HDR|AUD_PRM|AUD_RES, (int *)0, uap->id );
	switch_affinity(saveaffinity);
}
