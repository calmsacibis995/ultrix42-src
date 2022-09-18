#ifndef lint
static	char	*sccsid = "@(#)vm_smem.c	4.4	(ULTRIX)	11/9/90";
#endif lint

/***********************************************************************
 *									*
 *			Copyright (c) 1986, 1988 by			*
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
 *  09 Aug 90 sekhar
 *	defined sm_killp and sm_get_smlist for both vax and mips.
 *
 *  03 Aug 90 sekhar
 *	changes for memory mapped devices.
 *	added a new routine sm_killp to deal with timouts from 
 *	writes to non existant memory.
 *
 *  20 Jun 90 sekhar
 * 	Following changes for memory mapped devices:
 * 	Added a new routine sm_get_smlist. 
 *  	In smfree if IPC_MMAP is set then SM_DEST need not be set
 *	to free up the shared memory segment.
 *	
 *
 * 19-Jun-90 -- jmartin
 *	Fixes for "panic: vrelvm rssize"
 *
 *  11 Dec 89 jaa
 *	change dynamic swap to account for swap up front (ala v3.1) 
 * 	but actually do the allocation only when pushing the page/process
 *
 * 04 Dec 89 -- sekhar
 *	minor changes to smccdec for tracking swap useage.
 *
 * 24 Jul 89 -- jaa
 *	Fix a race condition in sm_ins_psm with hardclock
 *
 * 12 Jun 89 -- gg
 *	Dynamic swap changes
 *	---------------------
 *	Modified smccdec() routine to allocate swap space before doing
 *	swapping.
 *	Also, modified smccdec() routine to return success or failure. 
 *
 * 15 Dec 88 -- jmartin
 *	SMP locking for shared memory data structures (cf. h/shm.h)
 *	Integration of v3.0 shared memory changes, i.e. KM_ALLOC of
 *	per-process shared memory structures.
 *
 * 14 Dec 87 -- jaa
 *	changed to use new kernel memory allocator
 *	wmemfree is not used anymore
 *
 * 29 Apr 86 -- depp
 *	converted to locking macros from calls routines
 *
 *	smlock/smunlock/smwait have are now macros SM_LOCK/SM_UNLOCK/SM_WAIT
 *	and are defined in /sys/h/vmmac.h
 *
 * 03 Jan 86 -- depp
 *	In "smfree" moved the call to smunlink, so that the memory is
 *	properly deallocated before the SMS is unlinked.
 *
 * 11 Nov 85 -- depp
 *	Removed all conditional compiles for System V IPC.
 *
 * 30 Sep 85 -- depp
 *	Added shared memory segment locking
 *
 * 19 Jul 85 -- depp
 *	Minor cleanup
 *
 * 11 Mar 85 -- depp		SHMEM
 *	1) - this file provides the virtual memory support for	
 *		the shared memory IPC (uipc_smem.c). It is a	
 *		direct analog of vm_text.c since the Ultrix	
 *		design for shared memory segments is to handle	
 *		them the same as shared text segments.		
 *		LeRoy Fundingsland    1/16/85    DEC		
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
#include "../h/user.h"		/* includes errno.h & smem.smpt.h */
#include "../h/proc.h"
#include "../h/seg.h"
#include "../h/cmap.h"
#include "../h/vm.h"
#include "../h/map.h"
#include "../h/kmalloc.h"

#include "../h/ipc.h"
#include "../h/shm.h"

extern struct smem smem[];
extern struct sminfo sminfo;
extern smtot;

/* SMFREE - relinquish use of the shared memory segment of a	*/
/*	process.						*/
smfree(sp)
	register struct smem *sp; /* shared memory header ptr	*/
{
	register struct proc *p;
	register int smsize;		/* in clicks		*/
	register long a, npg;
	int s;

	if(sp == NULL)
		return;
	p = u.u_procp;
	SM_LOCK(sp);
#ifdef mips
	/* If IPC_MMAP is set then SM_DEST is not to be checked 
	 * before freeing up the shared memory segment. 
	 */
	if(--sp->sm_count == 0  &&  
	  ((sp->sm_perm.mode & IPC_MMAP) || (sp->sm_perm.mode & SM_DEST))){
#else
	if(--sp->sm_count == 0  &&  (sp->sm_perm.mode & SM_DEST)){
#endif
		int free_count;

		smsize = clrnd((int)btoc(sp->sm_size));

			/* free the memory for this SMS		*/
		if((sp->sm_perm.mode & IPC_SYSTEM) == 0) {
			free_count = vmemfree(sp->sm_ptaddr, smsize);

			s = splimp();
			smp_lock(&lk_smem, LK_RETRY);
			if((sp->sm_rssize -= free_count) != 0)
				panic("smfree: rssize");
			while (sp->sm_poip) {
				sleep_unlock((caddr_t)&sp->sm_poip,
					     PSWP+1, &lk_smem);
				smp_lock(&lk_smem, LK_RETRY);
			}
		} else {
			s = splimp();
			smp_lock(&lk_smem, LK_RETRY);
		}

		smunlink(p, sp);
		smp_unlock(&lk_smem);
		(void)splx(s);

		/* free the page table for this SMS	*/
		a = btokmx((struct pte *) sp->sm_ptaddr);
		npg = clrnd(btoc(sizeof(struct pte) * smsize));
		s = splimp();
		smp_lock(&lk_cmap, LK_RETRY);
		(void) memfree(&Usrptmap[a], npg, KMF_NODETACH);
		smp_unlock(&lk_cmap);
		(void)splx(s);
		rmfree(kernelmap, (long)npg, (long)a);

		/* free the swap space for this SMS, if normal	*/
		if ((sp->sm_perm.mode & IPC_SYSTEM) == 0)
			vssmfree(sp);
		smtot -= smsize;

		sp->sm_perm.mode = 0;
		sp->sm_perm.seq++;
		if(((int)(sp->sm_perm.seq * sminfo.smmni + (sp - smem)))
								< 0)
			sp->sm_perm.seq = 0;
		ipcfree(sp->sm_perm);
	} else
		smccdec(sp, p);
	SM_UNLOCK(sp);
}


/* SMCCDEC - shared memory core-count decrement. Decrement	*/
/*	the in-core usage count of a shared data segment. When	*/
/*	it drops to zero, free the core space.			*/
smccdec(sp, p)
	register struct smem *sp;
	register struct proc *p;
{
	int s;

	s = splimp();
	smp_lock(&lk_smem, LK_RETRY);
	if (sp == NULL  ||  sp->sm_ccount == 0){
		smp_unlock(&lk_smem);
		(void)splx(s);
		return(1);
	}
	sp->sm_ccount--;
	if((sp->sm_perm.mode & IPC_SYSTEM) == 0) {
		sm_cpdirty(p, sp);	/* note: could be macro */
		if(sp->sm_ccount == 0 && !(sp->sm_flag & SMNOSW)) {
			smp_unlock(&lk_smem);
			/* Allocate swap space for shared segment */
			if(vsalloc(sp->sm_dmap, CSMEM) == 0){
				swfail_stat.shm_swap_fail++;
				smp_lock(&lk_smem, LK_RETRY);
				smunlink(p, sp);
				smp_unlock(&lk_smem);
				(void)splx(s);
				return(0);
			}
			vsswap(sp, sp->sm_ptaddr, CSMEM, 0, 
			       clrnd(btoc(sp->sm_size)), (struct dmap *)0);
			smp_lock(&lk_smem, LK_RETRY);
			if(sp->sm_rssize != 0)
				panic("smccdec: rssize");
		}
	}
	smunlink(p, sp);
	smp_unlock(&lk_smem);
	(void)splx(s);
	return(1);
}


/* SMLINK - Add a process to those sharing a shared memory	*/
/*	segment by getting the page tables and then linking to	*/
/*	sm_caddr.						*/
smlink(p, sp)
    register struct proc *p;
    register struct smem *sp;
{
	register int smindex;
	int s;

	if(p->p_sm == (struct p_sm *) NULL) 
		panic("smlink: p_sm");
	for(smindex=0; smindex < sminfo.smseg; smindex++)
		if(p->p_sm[smindex].sm_p == sp)
			break;
	if(smindex >= sminfo.smseg)
		panic("smlink");	/* cannot happen	*/

			/* vinitsmpt() expects that the		*/
			/* sm_saddr and sm_pflag fields have	*/
			/* already been set within the proper	*/
			/* "p_sm" struct for this proc.		*/
	s = splimp();
	smp_lock(&lk_smem, LK_RETRY);
	vinitsmpt(p, sp);
	p->p_sm[smindex].sm_link = sp->sm_caddr;
	sp->sm_caddr = p;
	sp->sm_ccount++;
	smp_unlock(&lk_smem);
	(void)splx(s);
}


/* SMUNLINK - unlink the given process from the linked list of	*/
/*	processes sharing the given shared memory segment.	*/
smunlink(p, sp)
    register struct proc *p;
    register struct smem *sp;
{
	register struct proc *q;
	register int p_smindex, q_smindex;

	if(sp == NULL  ||  sp->sm_caddr == NULL)
		return;

	if(p->p_sm == (struct p_sm *) NULL) 
		panic("smunlink: p->p_sm");

	/* find index of this shared memory	*/
	/* seg for this process.		*/
	for(p_smindex = 0; p_smindex < sminfo.smseg; p_smindex++)
		if(p->p_sm[p_smindex].sm_p == sp)
			break;
	/* return gracefully if not found	*/
	/* because sp may not be attached when	*/
	/* IPC_RMID is asserted.		*/
	if(p_smindex >= sminfo.smseg)
		return;
	
	/* handle the special case where "p" is	*/
	/* at the beginning of the linked list.	*/
	if (sp->sm_caddr == p) {
		sp->sm_caddr = p->p_sm[p_smindex].sm_link;
		p->p_sm[p_smindex].sm_link = 0;
		return;
	}
	
	/* now handle the general case		*/
	/* find index of shared memory seg	*/
	/* for first process in list.		*/
	q = sp->sm_caddr;
	if(q->p_sm == (struct p_sm *) NULL) 
		panic("smunlink: q->p_sm");

	for(q_smindex = 0; q_smindex < sminfo.smseg; q_smindex++)
		if(q->p_sm[q_smindex].sm_p == sp)
			break;
	if(q_smindex >= sminfo.smseg)
		panic("smunlink #1");

	while(q->p_sm[q_smindex].sm_link){
		if(q->p_sm[q_smindex].sm_link == p) {
			q->p_sm[q_smindex].sm_link =
				p->p_sm[p_smindex].sm_link;
			p->p_sm[p_smindex].sm_link = 0;
			return;
		}
		/* check next entry in linked list	*/
		q = q->p_sm[q_smindex].sm_link;
		
		/* find index of shared memory seg	*/
		/* for this process.			*/
		for(q_smindex = 0; q_smindex < sminfo.smseg; q_smindex++)
			if(q->p_sm[q_smindex].sm_p == sp)
				break;
		if(q_smindex >= sminfo.smseg)
			panic("smunlink #2");
	}
	panic("smunlink: lost shared memory");
}


/* SMREPL - Replace p by q in a shared memory incore linked	*/
/*	list. Used by vfork() (vpassvm()) internally.		*/
smrepl(p, q, smindex)
    register struct proc *p, *q;
{
	register struct smem *sp;

	if((sp = q->p_sm[smindex].sm_p) == NULL)
		return;
	smunlink(p, sp);
	q->p_sm[smindex].sm_link = sp->sm_caddr;
	sp->sm_caddr = q;
}


/* SMCONV - Convert user supplied smid into a ptr to the	*/
/*	associated shared memory header.			*/
struct smem *
smconv(s, flg)
	register int s;		/* smid */
	int flg;		/* error if matching bits are set in mode */
{
	register struct smem *sp;	/* ptr to associated header */

	if (s < 0) {
		u.u_error = EINVAL;
		return(NULL);
	}

	sp = &smem[s % sminfo.smmni];
	if((sp->sm_perm.mode & IPC_ALLOC) == 0  ||
			sp->sm_perm.mode & flg  ||
			s / sminfo.smmni != sp->sm_perm.seq) {
		u.u_error = EINVAL;
		return(NULL);
	}
	return(sp);
}


/* SMCLEAN - Called by vrelvm to handle shared			*/
/*	memory cleanup processing.				*/
smclean()
{
	register struct p_sm *psmp;	/* see proc.h		*/
	register struct proc *p;
	register int *seg, *segend;	/* ptr's to pte		*/
	register int i;
	struct p_sm *osmp;
	int cnt;

			/* Detach all attached segments		*/
	p = u.u_procp;
	if((psmp = p->p_sm) == (struct p_sm *) NULL ) 
		panic("smclean: p_sm");
	
	cnt = p->p_smcount;
	p->p_smcount = 0;
	p->p_smbeg = 0;
	for(i = 0; i < cnt; i++, psmp++){
		if(psmp->sm_p == NULL)
			continue;
		/* if this process has SMS locked, then unlock */
		if(psmp->sm_lock) {
			int s;

			psmp->sm_lock = 0;
			s = splimp();
			smp_lock(&lk_smem, LK_RETRY);
			if(--((psmp->sm_p)->sm_lcount) == 0)
				(psmp->sm_p)->sm_flag &= ~SMNOSW;
			smp_unlock(&lk_smem);
			(void)splx(s);
		}
		smfree(psmp->sm_p);
#ifdef vax
		seg = (int *)mfpr(P0BR) + psmp->sm_saddr;
		segend = seg + btoc((psmp->sm_p)->sm_size);
		while(seg < segend)
			*seg++ = 0;
		psmp->sm_saddr = 0;
		psmp->sm_p = NULL;
#endif vax
	}
	SM_PSM_CLEAR(p);
#ifdef vax
       setp0lr(u.u_tsize + u.u_dsize);
#endif vax
	u.u_smsize = 0;
}


/* SMFORK - Called by newproc(sys/kern_fork.c) to handle shared	*/
/*	memory FORK processing.					*/
smfork(pp, cp)
    struct proc *pp,	/* ptr to parent proc table entry */
		*cp;	/* ptr to child proc table entry */
{
	register struct p_sm	*cpsmp,	/* ptr to child p_sm	*/
				*ppsmp;	/* ptr to parent p_sm	*/
	register int i;

	/* Copy ptrs and update counts on any attached segments. */
	if((cpsmp = cp->p_sm) == (struct p_sm *) NULL )
		panic("smfork: cpsmp");

	if((ppsmp = pp->p_sm) == (struct p_sm *) NULL )
		panic("smfork: ppsmp");

	SM_PSM_COPY(pp,cp);

	cp->p_smcount = pp->p_smcount;

	for(i = 0; i < sminfo.smseg; i++, cpsmp++, ppsmp++){
		if(cpsmp->sm_p = ppsmp->sm_p){
			int s = splimp();
			smp_lock(&lk_smem, LK_RETRY);
			cpsmp->sm_p->sm_count++;
			smp_unlock(&lk_smem);
			(void)splx(s);
			cpsmp->sm_saddr = ppsmp->sm_saddr;
			cpsmp->sm_eaddr = ppsmp->sm_eaddr;
			cpsmp->sm_pflag = ppsmp->sm_pflag;
			cpsmp->sm_lock = 0;
			smlink(cp, cpsmp->sm_p);
		}
	}
}


/*
 * This routine will create a "hole" in the struct p_sm array.
 * The hole will be zero filled. The calling routine will fill the "hole".
 * This array is ordered by address (low to high).
 * If at end of list, then we simply return (it's already zero filled)
 * else we need to create a "hole" in the array, so we
 *      1. malloc a new p_sm array (zero filled)
 *      2. copy upto "hole" without offsetting index
 *      3. copy from "hole" in orig, offsetting index in new by one
 *      4. Leave "hole" in place (it's already zero filled)
 *      5. To cleanup, simply replace new array in proc and free old one
 * We are guaranteed that smindex is a valid index
 *
 * In the future, this should be done with lists ...
 *
 */
sm_ins_psm(p, smindex)
	register struct proc *p;
	register int smindex;
{
	register struct p_sm *fpsm, *tpsm, *tp;
	int next = smindex + 1;
	int cnt;

	if(++(p->p_smcount) > sminfo.smseg) {
		cprintf("p_smcount = %d\n",p->p_smcount);
		panic("sm_ins_psm: p_smcount >= sminfo.smseg");
	}

	/* if at end of list ... */
	if(p->p_sm[smindex].sm_p == NULL)
		return;

	/* if not, then shuffle ... */
	if(next  == sminfo.smseg)
		panic("sm_ins_psm: too many segments per process");
	KM_ALLOC(tp, struct p_sm *, SIZEOF_PSM, KM_SHMSEG, KM_CLEAR);
	if(smindex)
		bcopy(p->p_sm, tp, sizeof(struct p_sm) * smindex);
	bcopy(&p->p_sm[smindex], &tp[next], 
	      sizeof(struct p_sm)*(sminfo.smseg - next));

	cnt = p->p_smcount;
	p->p_smcount = 0;
        KM_FREE(p->p_sm, KM_SHMSEG);
	p->p_sm = tp;
	p->p_smcount = cnt;
	return;
}

/*
 * This routine will remove the entry in the p_sm array, compress it, and 
 * zero fill the last entry
 */
sm_del_psm(p, smindex)
	register struct proc *p;
	register smindex;
{
	/* move backward into the "hole" */
	if(++smindex < sminfo.smseg)
		bcopy(&p->p_sm[smindex], &p->p_sm[smindex-1],
		      sizeof(struct p_sm) * (sminfo.smseg - smindex));
	
	/* zero last element */
	bzero(&p->p_sm[sminfo.smseg-1], sizeof(struct p_sm));
	if(--(p->p_smcount) < 0)
		panic("sm_del_psm: smcount");
}

/*
 * This routine will indicate whether the address is a shared memory segment
 * and return the segment index via the smindex variable.
 */
sm_retrieve_sms(p, pn, smindex)
	struct proc *p;
	register int pn;
	int *smindex;
{
	register int i;
	register struct p_sm *psm = p->p_sm; /* pointer to proc's SMS struct */

	if(psm != (struct p_sm *) NULL) {
		for(i = 0; i < p->p_smcount; i++, psm++) {
			if(psm->sm_p == NULL)
				break;
			if((pn >= psm->sm_saddr) && (pn < psm->sm_eaddr)) {
				*smindex = i;
				return(1);
			}
		}
	}
	return (0);
}

/*
 * This routine will retrieve starting page number of the shared segment within
 *  the process' address space, given the proc pointer and the smem pointer.
 *
 * Since this routine is not called, unless the segment is attached to the 
 * process, success is guaranteed (or panic will result).
 */
sm_retrieve_sa(p, sp)
	struct proc *p;
	struct smem *sp;
{
	register int i;
	register struct p_sm *psm = p->p_sm;

	if(psm == (struct p_sm *) NULL)
		panic ("sm_retrieve_sa: p_sm == (struct p_sm *) NULL");

	for(i = 0; i < p->p_smcount; i++, psm++) {
		if(sp == psm->sm_p)
			return(psm->sm_saddr);
	}
	panic ("sm_retrieve_sa: Could not find SMS in proc");
}

/*
 * Purpose:
 *     This function returns the list of shared memory segments
 *     in the address range [saddr, eaddr]. Note the following:
 *     1. All the shared memory segments must be wholly contained 
 *	  in the address range. 
 *     2. saddr must be the starting address of some shm segment.
 *     3. eaddr must be the ending address of some shm segment.
 * 
 * Input:
 *     p    	- proc structure.
 *     saddr 	- starting address.
 *     eaddr	- ending address. 
 * 
 * Output:
 *     fpsm	- pointer to the first shared memory segment in the list.
 *     lpsm	- pointer to the last  shared memory segment in the list.
 *
 * Returns
 *     -1 on an error ; 0 on success.
 */

sm_get_smlist(p, saddr, eaddr, fpsm, lpsm)
        struct proc *p;
	int saddr;
	int eaddr;
        struct p_sm **fpsm;	/* pointer to the first shm */
	struct p_sm **lpsm;	/* pointer to the last shm */
{
	register int shmcnt = p->p_smcount; 	
	register struct p_sm *psm = p->p_sm;	

	/*	
	 *	This routine not yet tested on the vax but exists
	 *	only as latent support for memory mapped devices on vax.
	 *	However, it has been tested on the mips.
	 */

	if(psm == NULL) 
		return(-1);
	*fpsm = *lpsm = NULL;	
	/*
	 *	look for a segment with starting addr saddr.
	 */
	for(; shmcnt > 0; shmcnt--, psm++) 
	    if (psm->sm_saddr == saddr) 
		break;

	if (!shmcnt)
	    return(-1);
	*fpsm = psm;

	/*	
	 *	look for a segment with ending addr eaddr.
	 */
	for(; shmcnt > 0; shmcnt--, psm++)
	    if (psm->sm_eaddr == eaddr) { 
		*lpsm = psm;
		return(0);
	    }
	return(-1);
}

/* 
 * 
 */

sm_killp(pfn)
unsigned pfn;
{
	extern struct smem smem[];
	extern struct sminfo sminfo;

	register int i;
	register struct smem *psm;
	register struct proc *pproc;
	
	int foundshm = -1;
	int s,rv;
	
	/*
	 *	This routine not yet tested on the vax but exists
	 *	only as latent support for memory mapped devices on vax.
	 *	However, it has been tested on the mips.
	 */

#ifdef vax
	return(-1);
#endif
	/*
	 *	Locking semantics:
	 *	
	 */
	
	s = splimp();
	smp_lock(&lk_smem, LK_RETRY);
	for (i = 0, psm = smem; i < sminfo.smmni ; i++, psm++) {

	    /* 
	     *	Search all the shared memory segments. For each segment
	     *	check if pfn is mapped.
	     */

	    /*	
	     *  Skip following entries:
	     *	    IPC_ALLOC not set => unused entry. 
	     *	    IPC_MMAP  not set => not an entry created by mmap
	     *				 system call. 
	     *
	     *	Protection of various fields:
	     *	    sm_perm.mode - lk_smem
	     *	    sm_caddr     - lk_smem
	     *      sm_link	 - lk_smem
	     *
	     *	sm_size consistency:
	     *	    sm_size is written only once (see routine smget)
	     *	    when the segment is created.
	     */
	    
	    if (    (!(psm->sm_perm.mode & IPC_ALLOC)) 
		 || (!(psm->sm_perm.mode & IPC_MMAP))) 
		continue;

	    if (   (btop(psm->sm_perm.key) <= pfn) 
		&& (btop(psm->sm_perm.key) + clrnd(btoc(psm->sm_size))) > pfn) {

		/* 
		 *	Have to kill  all the processes which have mapped
	 	 *	this segment. So for each process post a kill
		 *	signal.
		 */

		foundshm = 0;
		for (pproc = psm->sm_caddr ;
		     pproc ; pproc = pproc->p_sm->sm_link) {
		    /*
		     *	process may be exiting. so before posting
		     *	a signal gain a reference.
		     */

		    smp_lock(&lk_procqs, LK_RETRY);
		    rv = proc_ref(pproc);
		    smp_unlock(&lk_procqs);
		    if (rv) {	/* process is ALIVE */
			psignal(pproc, SIGKILL);

		         /* decrement the ref count */
		        proc_rele(pproc);
		    	uprintf("Process %d killed - CPU write timeout\n", pproc->p_pid);
		    	mprintf("Process %d killed - CPU write timeout\n", pproc->p_pid);
		    }
		} /* end for */
	    } /* end if */
	} /* end for */
	smp_unlock(&lk_smem);
	(void)splx(s);
	return(foundshm);
}
