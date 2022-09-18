#ifndef lint
static	char	*sccsid = "@(#)vm_smem.c	4.1	(ULTRIX)	7/2/90";
#endif lint

/***********************************************************************
 *									*
 *			Copyright (c) 1986 by				*
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
#include "../vax/mtpr.h"
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
    register struct smem *sp;	/* shared memory header ptr	*/
{
	register struct proc *p;
	register int smsize;		/* in clicks		*/
	register long a, npg;

#ifdef mips
	XPRINTF(XPR_SM,"enter smfree, sp = 0x%x",sp,0,0,0);
#endif mips
	if(sp == NULL)
		return;
	p = u.u_procp;
	SM_LOCK(sp);
	if(--sp->sm_count == 0  &&  (sp->sm_perm.mode & SM_DEST)){
		smsize = clrnd((int)btoc(sp->sm_size));

		/* free the memory for this SMS, if normal	*/
		if ((sp->sm_perm.mode & IPC_SYSTEM) == 0) {
			sp->sm_rssize -= vmemfree(sp->sm_ptaddr, smsize);
			if(sp->sm_rssize != 0)
				panic("smfree rssize");

			while (sp->sm_poip)
				sleep((caddr_t)&sp->sm_poip, PSWP+1);
		}

		smunlink(p, sp);

		/* free the page table for this SMS	*/
		a = btokmx((struct pte *) sp->sm_ptaddr);
		npg = clrnd(btoc(sizeof(struct pte) * smsize));
		(void) memfree(&Usrptmap[a], npg, KMF_NODETACH);
		rmfree(kernelmap, (long)npg, (long)a);

		/* free the swap space for this SMS, if normal	*/
		if ((sp->sm_perm.mode & IPC_SYSTEM) == 0)
			vssmfree(sp, (long)smsize);
		smtot -= smsize;

		sp->sm_perm.mode = 0;
		sp->sm_perm.seq++;
		if(((int)(sp->sm_perm.seq * sminfo.smmni + (sp - smem)))
								< 0)
			sp->sm_perm.seq = 0;

		sp->sm_flag &= ~SMLOCK;
	} else {
		sp->sm_flag &= ~SMLOCK;
		smccdec(sp, p);
	}
}


/* SMCCDEC - shared memory core-count decrement. Decrement	*/
/*	the in-core usage count of a shared data segment. When	*/
/*	it drops to zero, free the core space.			*/
smccdec(sp, p)
    register struct smem *sp;
    register struct proc *p;
{
	register int i, smsize;
	register struct pte *pte;

#ifdef mips
	XPRINTF(XPR_SM,"enter smccdec: sp 0x%x p 0x%x",sp,p,0,0);
#endif mips
	if (sp == NULL  ||  sp->sm_ccount == 0)
		return;
	SM_LOCK(sp);
	sp->sm_ccount--;

	/*
	 * this process is detaching from this SMS (due to exit, swapout, 
	 * or detach) so if the modify bit is on for any of the SMS PTEs 
	 * we must be sure to propogate it back to the primary PTEs before 
	 * the copy-PTEs are zeroed.  Note: Since the primary PTEs are only
	 * table entries and are not used by the hardware, an TBIS is not
	 * required.
	 *
	 * NOTE: This is not done for system/user sharing PTEs
	 */
	if ((sp->sm_perm.mode & IPC_SYSTEM) == 0) {
#ifdef vax
		for(i=0; i < sminfo.smseg; i++) {
			if(p->p_sm[i].sm_p == sp){
				break;
			}
		}
		if (i == sminfo.smseg)
			panic("smccdec: smseg");
		smsize = clrnd(btoc(sp->sm_size));
		pte = p->p_p0br + p->p_sm[i].sm_spte;
		for(i=0; i < smsize; i+=CLSIZE,pte+=CLSIZE){
			register int j;
			if(dirtycl(pte)){
				*(int *)(sp->sm_ptaddr + i) |= PG_M;
				distcl(sp->sm_ptaddr + i);
			}
			for(j=0; j < CLSIZE; j++)
				*(int *)(pte+j) = 0;
		}
#endif vax
#ifdef mips
		smsize = clrnd(btoc(sp->sm_size));
#endif mips
		if (sp->sm_ccount == 0 && !(sp->sm_flag & SMNOSW)) {
#ifdef DEPPDEBUG
			printf("SMS out; sp = 0x%x\n",sp);
#endif DEPPDEBUG
			vsswap(sp, sp->sm_ptaddr, CSMEM, 0, smsize,
			       (struct dmap *)0);
			if(sp->sm_rssize != 0)
				panic("smccdec: rssize");
		}
	}
	smunlink(p, sp);
	SM_UNLOCK(sp);
}


/* SMLINK - Add a process to those sharing a shared memory	*/
/*	segment by getting the page tables and then linking to	*/
/*	sm_caddr.						*/
smlink(p, sp)
    register struct proc *p;
    register struct smem *sp;
{
	register int smindex;

#ifdef mips
	XPRINTF(XPR_SM,"enter smlink: sp 0x%x p 0x%x",sp,p,0,0);
#endif mips
	if(p->p_sm == (struct p_sm *) NULL) {
		panic("smlink: p_sm");
	}
	for(smindex=0; smindex < sminfo.smseg; smindex++)
		if(p->p_sm[smindex].sm_p == sp)
			break;
	if(smindex >= sminfo.smseg)
		panic("smlink");	/* cannot happen	*/

			/* vinitsmpt() expects that the		*/
			/* sm_spte and sm_pflag fields have	*/
			/* already been set within the proper	*/
			/* "p_sm" struct for this proc.		*/
#ifdef vax
	vinitsmpt(p, sp);
#endif vax
	p->p_sm[smindex].sm_link = sp->sm_caddr;
	sp->sm_caddr = p;
#ifdef DEPPDEBUG
	if (sp->sm_ccount == 0)
		printf("SMS in; sp = 0x%x\n ",sp);
#endif DEPPDEBUG
	sp->sm_ccount++;
}


/* SMUNLINK - unlink the given process from the linked list of	*/
/*	processes sharing the given shared memory segment.	*/
smunlink(p, sp)
    register struct proc *p;
    register struct smem *sp;
{
	register struct proc *q;
	register int p_smindex, q_smindex;

#ifdef mips
	XPRINTF(XPR_SM,"enter smunlink: sp 0x%x p 0x%x",sp,p,0,0);
#endif mips
	if (sp == NULL  ||  sp->sm_caddr == NULL)
		return;

	if(p->p_sm == (struct p_sm *) NULL) {
		panic("smlink: p->p_sm");
	}
			/* find index of this shared memory	*/
			/* seg for this process.		*/
	for(p_smindex=0; p_smindex < sminfo.smseg; p_smindex++)
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
	if(q->p_sm == (struct p_sm *) NULL) {
		panic("smlink: q->p_sm");
	}
	for(q_smindex=0; q_smindex < sminfo.smseg; q_smindex++)
		if(q->p_sm[q_smindex].sm_p == sp)
			break;
	if(q_smindex >= sminfo.smseg)
		panic("smunlink #1");

	while(q->p_sm[q_smindex].sm_link){
		if (q->p_sm[q_smindex].sm_link == p) {
			q->p_sm[q_smindex].sm_link =
					p->p_sm[p_smindex].sm_link;
			p->p_sm[p_smindex].sm_link = 0;
			return;
		}
			/* check next entry in linked list	*/
		q = q->p_sm[q_smindex].sm_link;

			/* find index of shared memory seg	*/
			/* for this process.			*/
		for(q_smindex=0; q_smindex < sminfo.smseg; q_smindex++)
			if(q->p_sm[q_smindex].sm_p == sp)
				break;
		if(q_smindex >= sminfo.smseg)
			panic("smunlink #2");
	}
	panic("lost shared memory");
}


/* SMREPL - Replace p by q in a shared memory incore linked	*/
/*	list. Used by vfork() (vpassvm()) internally.		*/
smrepl(p, q, smindex)
    register struct proc *p, *q;
{
	register struct smem *sp;

#ifdef mips
	XPRINTF(XPR_SM,"enter smlink: p 0x%x q 0x%x smindex %d",p,q,smindex,0);
#endif mips
	if((sp = q->p_sm[smindex].sm_p) == NULL)
		return;

	smunlink(p, sp);
	q->p_sm[smindex].sm_link = sp->sm_caddr;
	sp->sm_caddr = q;
}

/* 
 * VM_SYSTEM_SMGET() -- kernel level smget() routine for kernel/user mapping
 *
 *   This routine MUST only be used to map static kernel page aligned data
 *   structures into an application's address space.  This routine will
 *   perform a special version of the shmget() system call.  The assumptions
 *   are as follows
 *       1. The system virtual address space, and the underlaying PTEs are
 *          fully static (i.e., DO NOT use km_alloc'd sva)
 *       2. The caller validates that the application is permitted to 
 *          create this Shared Memory Segment (SMS)
 *       3. Once the application has requested the driver to "get" the 
 *          SMS (via an IOCTL I assume), full SMS semantics are in effect.
 *          This means that the application must now call the shmat() system 
 *          call to actually attach to this segment.  Also, the SMS can
 *          be detached, reattached, and (upon the last detach) removed.
 *       4. The beginning system virtual address and size are page cluster
 *          aligned.  It is up to the calling routine to insure that the 
 *          protocol for finding a non-page aligned structure is in place for
 *          the application.
 *
 *   INPUT PARAMETERS:
 *          sva        Beginning system virtual address to be double mapped
 *          size       Size of the segment in bytes
 *
 *               Note: Both must be page aligned
 *
 *          mode       This is the protection field.  It uses file type
 *                     protection (user, group, others) for read/write.
 *                     Note that execute permission is not used.
 *
 *   RETURN PARAMETER:
 *          -1         Error setting up SMS, u.u_error has been set to
 *                     indicate error condition
 *          smid       Shared memory identifier, to be passed back to the
 *                     application.  This is the normal return value.
 */

vm_system_smget(sva, size, mode) 
int sva;
int size;
int mode;
{
	register int num_ptes = btop(size);
#ifdef vax
	register struct pte *spte;
#endif vax
#ifdef mips
	register int spfn;
#endif mips
	register struct pte *upte;
	register struct smem *sp;
	register int smid;
	register int nocache;
	extern struct smem *smconv();

#ifdef mips
	XPRINTF(XPR_SM,"enter vm_system_smget: sva 0x%x size %d mode %o",
		sva, size, mode, 0);
#endif mips
#ifdef vax
	if (((sva & VA_SPACE) != VA_SYS) || 
#endif vax
#ifdef mips
	if (IS_KUSEG(sva) ||
#endif mips
	    !isclbnd(sva) || !isclbnd(size) || (size < CLBYTES)) {
		u.u_error = EINVAL;
		return (-1);
	}

#ifdef vax
	spte = svtopte(sva); 
	while (num_ptes--) {
		if ((spte++)->pg_v == 0) {
			u.u_error = EINVAL;
			return(-1);
		}
	}
#endif vax

	mode &= 0777;
	if ((smid = smget(sva, size, IPC_CREAT | IPC_SYSTEM | mode)) == -1) {
		return(-1);
	}

	/* 
	 * get the smem pointer, as it has just been allocated or 
	 * retrieved, it must exist.
	 */
	if ((sp = smconv(smid, 0)) == NULL)
		panic("vm_system_smget: invalid SMS");

	/* if first time, then fill */
	upte = sp->sm_ptaddr;
	if (upte->pg_v == 0) {
		num_ptes = btop(size);
#ifdef vax
		spte = svtopte(sva);
		while (num_ptes--) 
			*(int *) (upte++) = 
				PG_V | (*(int *) (spte++) & PG_PFNUM);
#endif vax
#ifdef mips
		if (IS_KSEG0(sva)) {
			spfn = K0_TO_PHYS(sva) >> PGSHIFT;
			nocache = 0;
		}
		else if (IS_KSEG1(sva)) {
			spfn = K1_TO_PHYS(sva) >> PGSHIFT;
			nocache = 1;
		}
		else if (IS_KSEG2(sva)) {
			u.u_error = EINVAL;
			return(-1);
		} else {
			u.u_error = EINVAL;
			return(-1);
		}
		while (num_ptes--) {
			upte->pg_v = 1;
			upte->pg_m = 1;
#ifdef NOMEMCACHE
			upte->pg_n = 1;
#else  NOMEMCACHE
			upte->pg_n = nocache;
#endif NOMEMCACHE
			upte->pg_prot = PG_UW;
			(upte++)->pg_pfnum = spfn++;
		}
#endif mips
		XPRINTF(XPR_SM,"vm_system_smget: first pte 0x%x *pte 0x%x",
			sp->sm_ptaddr, *(int *) sp->sm_ptaddr,0,0);
	}
	return (smid);
}

#ifdef mips
/*
 * This routine will indicate whether the address is a shared memory segment
 * and return the segment index via the smindex variable.
 */
sm_retrieve_sms(p, pn, smindex)
struct proc *p;
register int pn;
int *smindex;
{
	register i;
	register struct p_sm *psm = p->p_sm; /* pointer to proc's SMS struct */

#ifdef notdef
	XPRINTF(XPR_SM,"enter sm_retrieve_sms p 0x%x pn %d",
		p,pn,0,0);
#endif notdef
	if (psm == (struct p_sm *) NULL) 
		return(0);

	for (i = 0; i < sminfo.smseg; i++, psm++) {
		if (psm->sm_p == NULL)
			continue;
		if ((pn >= psm->sm_saddr) && (pn < psm->sm_eaddr)) {
			*smindex = i;
			return(1);
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
	register i;
	register struct p_sm *psm = p->p_sm;

	if (psm == (struct p_sm *) NULL)
		panic ("sm_retrieve_sa: p_sm == (struct p_sm *) NULL");

	for (i = 0; i < p->p_smcount; i++, psm++) {
		if (sp == psm->sm_p)
			return(psm->sm_saddr);
	}
	panic ("sm_retrieve_sa: Could not find SMS in proc");
}
#endif mips
