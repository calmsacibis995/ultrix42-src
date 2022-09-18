#ifndef lint
static	char	*sccsid = "@(#)pt_machdep.c	4.3	(ULTRIX)	9/6/90";
#endif lint

/***********************************************************************
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
 **********************************************************************/

/*----------------------------------------------------------------------
 * Modification History
 *
 * 4-Sep-90	dlh
 *	added vector processor support code
 *
 * 03-Aug-90	rafiey (Ali Rafieymehr)
 *	Set the pages (u.pages) to modify.
 *
 * 19-Jun-90 -- jmartin
 *	Fixes for "panic: vrelvm rssize"
 *
 * 05 Mar 90 jaw
 *	IPL level lowered when text lock is released in vinitpt.
 *
 * 24 Jul 89 -- jmartin
 *	Don't call swapout; just ask the swapper and sleep.
 *
 * 06-Jun-89  -- jaa
 *    Creation date
 *    split machdep routines from ../vm_pt.c 
 */

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/proc.h"
#include "../h/map.h"
#include "../h/cmap.h"
#include "../h/vm.h"
#include "../h/buf.h"
#include "../h/text.h"
#include "../h/mount.h"
#include "../h/gnode.h"
#include "../h/kernel.h"
#include "../machine/pte.h"
#include "../machine/mtpr.h"
#include "../machine/cpu.h"
#include "../h/ipc.h"
#include "../h/shm.h"
#include "../h/cpudata.h"
#ifdef vax
#include "../machine/cpu.h"
#include "../../machine/common/cpuconf.h"
#include "../machine/vectors.h"
#endif vax

extern struct sminfo sminfo;
extern int swapfrag;

/*
 * Get page tables for process p.  Allocator
 * for memory is argument; process must be locked
 * from swapping if vmemall is used; if memall is
 * used, call will return w/o waiting for memory.
 * In any case an error return results if no user
 * page table space is available.
 */
vgetpt(p, pmemall, pmemfree)
	register struct proc *p;
	int (*pmemall)();
	int (*pmemfree)();       /* here for consistency with mips */
{
	register long a;
	register int i;

	if (p->p_szpt == 0)
		panic("vgetpt");
	/*
	 * Allocate space in the kernel map for this process.
	 * Then allocate page table pages, and initialize the
	 * process' p0br and addr pointer to be the kernel
	 * virtual addresses of the base of the page tables and
	 * the pte for the process pcb (at the base of the u.).
	 */
	if ((a = rmalloc(kernelmap, (long)p->p_szpt)) == 0)
		return (0);
	if ((*pmemall)(&Usrptmap[a], p->p_szpt, p, CSYS, NULL, V_NOOP) == 0) {
		rmfree(kernelmap, (long)p->p_szpt, a);
		return (0);
	}
	p->p_p0br = kmxtob(a);
	p->p_addr = uaddr(p);
	/*
	 * Now validate the system page table entries for the
	 * user page table pages, flushing old translations
	 * for these kernel virtual addresses.  Clear the new
	 * page table pages for clean post-mortems.
	 */
	vmaccess(&Usrptmap[a], (caddr_t)p->p_p0br, p->p_szpt, 0);
	blkclr((caddr_t)p->p_p0br, p->p_szpt*NBPG);
	return (1);
}

/*
 * Initialize text portion of page table.
 */
vinitpt(p,s)
	struct proc *p;
	int s;
{
	register struct text *xp;
	register struct proc *q;
	register struct pte *pte;
	register int i;
	struct pte proto;
	unsigned unlock_flag = NULL;

	XPRINTF(XPR_VM,"enter vinitpt",0,0,0,0);
	xp = p->p_textp;
	if (xp == 0)
                return (1);
	pte = tptopte(p, 0);
	if (q = xp->x_caddr) {
	/*
	 * If there is another instance of same text in core
	 * then just copy page tables from other process.
	 *
	 * This operation can be thought of as a large "distpte", so
	 * we lock it under the same protocol as we lock "distpte".
	 * We don't give up the lock until the process is linked to
	 * the text chain (see xlink(p) in sys/vm_text.c), so that
	 * real "distpte"s will see this process.
	 */
		struct pte *qpte = tptopte(q, 0);
		int s1;

		/* synchronize with ptexpand() */
		s1 = splclock();
		smp_lock(&lk_rq,LK_RETRY);
		if (CURRENT_CPUDATA->cpu_tbi_flag) {
			mtpr(TBIA,0);
			CURRENT_CPUDATA->cpu_tbi_flag = 0;
		}
		smp_unlock(&lk_rq);
		splx(s1);
		blkcpy((caddr_t)qpte, (caddr_t)pte,
		    (unsigned) (sizeof(struct pte) * xp->x_size));
		goto done;
	}
	/*
	 * OK to give up the spin lock, as we are the one and only
	 * process using this text and have excluded others by
	 * maintaining ((xp->x_flag & X_LOCK) == 1).
	 */
	unlock_flag = !NULL;
	smp_unlock(&lk_text);
	splx(s);
	/*
	 * Initialize text page tables, zfod if we are loading
	 * the text now; unless the process is demand loaded,
	 * this will suffice as the text will henceforth either be
	 * read from a file or demand paged in.
	 */
	*(int *)&proto = PG_URKR;
	if (xp->x_flag & XLOAD || xp->x_flag & XNOSPCE) {
		proto.pg_fod = 1;
		((struct fpte *)&proto)->pg_fileno = PG_FZERO;
	}
	for (i = 0; i < xp->x_size; i++)
		*pte++ = proto;
	if ((xp->x_flag & XPAGI) == 0)
		goto done;
	/*
	 * Text is demand loaded.  If process is not loaded (i.e. being
	 * swapped in) then retrieve page tables from swap area.  Otherwise
	 * this is the first time and we must initialize the page tables
	 * from the blocks in the file system.
	 */
	if (xp->x_flag & XLOAD || xp->x_flag & XNOSPCE){
		vinifod((struct fpte *)tptopte(p, 0), PG_FTEXT, xp->x_gptr,
		    (daddr_t)1, xp->x_size);

		xp->x_flag &= ~XNOSPCE; /* clear the XNOSPCE flag setin xccdec*/
	} else {
		register int *dp, poff, ptsize,nfrag;
		dp = xp->x_dmap->dm_ptdaddr;
		nfrag = dtob(swapfrag);
		poff = 0;
		ptsize = xp->x_size * sizeof (struct pte);
		while(ptsize > nfrag){
			if(*dp == 0)
				panic("vinitpt: text pt swap addr 0");
			swap(p, *dp++, (caddr_t)tptopte(p, poff),
			nfrag, B_READ, B_PAGET, swapdev,0);

			ptsize -= nfrag;
			poff += nfrag/sizeof(struct pte);
		}
		if(*dp == 0)
			panic("vinitpt: text pt swap addr 0");
		swap(p, *dp, (caddr_t)tptopte(p, poff),
		    ptsize, B_READ, B_PAGET, swapdev, 0);
	}
done:
	/*
	 * In the case where we are overlaying ourself with new page
	 * table entries, old user-space translations should be flushed.
	 */
	if (p == u.u_procp)
		newptes(p, tptov(p, 0), xp->x_size);
	else
		SET_P_VM(p, SPTECHG);
	if (unlock_flag) {
		(void)splimp();
		smp_lock(&lk_text, LK_RETRY);
	}
	return(1);
}

/* VINITSMPT - Initialize shared memory portion of page table	*/
/*	for the given shared memory segment.			*/
/*	As a short cut, to get this done in a hurry, I have	*/
/*	made shared memory segment page tables wired-down.	*/
/* LeRoy Fundingsland    1/18/85    DEC		SHMEM		*/
vinitsmpt(p, sp)
	register struct proc *p;
	register struct smem *sp;
{
	register struct pte *pte, *pte1;
	register int i, smindex;
	register int smsize;		/* SMS size in clicks	*/
	int s;

	XPRINTF(XPR_VM,"enter vinitsmpt",0,0,0,0);
	if(sp == NULL)
		return;

	if(p->p_sm == (struct p_sm *) NULL) {
		panic("vinitsmpt: p_sm");
	}

	for(smindex=0; smindex < sminfo.smseg; smindex++)
		if(p->p_sm[smindex].sm_p == sp)
			break;
	if(smindex >= sminfo.smseg)
		panic("vinitsmpt");
	pte = p->p_p0br + p->p_sm[smindex].sm_saddr;
	smsize = clrnd(btoc(sp->sm_size));

	/* set up process' ptes */
	pte1 = sp->sm_ptaddr;
	for(i=0; i < smsize; i++)
			*(int *)pte++ = *(int *)pte1++ |
				(int)p->p_sm[smindex].sm_pflag;
	/*
	 * In the case where we are overlaying ourself with new page
	 * table entries, old user-space translations must be flushed.
	 */
	if (p == u.u_procp)
		newptes(p, tptov(p,p->p_sm[smindex].sm_saddr),
							smsize);
	else
		SET_P_VM(p, SPTECHG);
}

#ifndef distpte
/*
 * Update the page tables of all processes linked
 * to a particular text segment, by distributing
 * dpte to the the text page at virtual frame v.
 *
 * Note that invalidation in the translation buffer for
 * the current process is the responsibility of the caller.
 */
distpte(xp, tp, dpte)
	struct text *xp;
	register size_t tp;
	register struct pte *dpte;
{
	register struct proc *p;
	register struct pte *pte;
	register int i;
	int s;

#ifdef SMP_DEBUG
	if (smp_debug)
		lsert(&lk_text);
#endif SMP_DEBUG
	s = splclock();
	smp_lock(&lk_rq,LK_RETRY);
	if (CURRENT_CPUDATA->cpu_tbi_flag) {
		mtpr(TBIA,0);
		CURRENT_CPUDATA->cpu_tbi_flag = 0;
	}
	smp_unlock(&lk_rq);
	splx(s);
	for (p = xp->x_caddr; p; p = p->p_xlink) {
		pte = tptopte(p, tp);
		SET_P_VM(p, SPTECHG);
      		if (pte != dpte) {
			for (i = 0; i < CLSIZE; i++)
				pte[i] = dpte[i];
		}
	}
}
#endif distpte

#ifndef distsmpte
/* DISTSMPTE - Update the page tables of all processes linked	*/
/*	to a particular shared memory segment, by distributing	*/
/*	dpte to the the shared memory page at virtual frame smp.*/
/*								*/
/*	Note that invalidation in the translation buffer for	*/
/*	the current process is the responsibility of the caller.*/
/* 			 SHMEM					*/
distsmpte(sp, smp, dpte, cm)
	register struct smem *sp;
	size_t smp;
	register struct pte *dpte;	/* Global PTE */
	int cm;				/* clear pg_m flag */
{
	register struct pte *pte;
	register int i, j;
	register struct proc *p;
	int s;

	/* if the SMS is currently not attached	*/
	/* to any process then return.		*/
	if(sp->sm_ccount == 0 || sp->sm_caddr == NULL)
		return;

	p = sp->sm_caddr;
	if(p->p_sm == (struct p_sm *) NULL) {
		panic("distmpte: p_sm1");
	}

	for(i=0; i < sminfo.smseg; i++)
		if(p->p_sm[i].sm_p == sp)
			break;
	if(i >= sminfo.smseg)
		panic("distsmpte");

	/* if requested, clear pg_m bit in global PTE */
	if (cm == PG_CLEARM) {
		dpte->pg_m = 0;
		distcl(dpte);
	}

	while(p){
		int unlock_text;

		unlock_text = 0;
		if (smp_owner(&lk_text) == LK_FALSE) {
			smp_lock(&lk_text, LK_RETRY);
			++unlock_text;
		}
		pte = p->p_p0br + p->p_sm[i].sm_saddr+smp;
		/* synchronize with ptexpand() */

		s = splclock();
		smp_lock(&lk_rq,LK_RETRY);
		if (CURRENT_CPUDATA->cpu_tbi_flag) {
			mtpr(TBIA,0);
			CURRENT_CPUDATA->cpu_tbi_flag = 0;
		}
		smp_unlock(&lk_rq);
		splx(s);
		/* this panic should eventually go away */
		if (pte->pg_v && dpte->pg_fod) {
			panic("distsmpte: PG_V && PG_FOD");
		}

		SET_P_VM(p, SPTECHG);

		/* CAREFUL: I'm incrementing 'pte' */
		for (j=0; j < CLSIZE; j++, pte++) {
			*(int *) pte &= PG_PROT|PG_M;
			*(int *) pte |= (*(int *)(dpte+j)) & ~(PG_PROT|PG_M);
			if (cm == PG_CLEARM)
				pte->pg_m = 0;
		}
		if (unlock_text)	/* Did this function take lk_text? */
			smp_unlock(&lk_text);

		/* now for next proc in linked list	*/
		if((p = p->p_sm[i].sm_link) == NULL)
			break;

		if(p->p_sm == (struct p_sm *) NULL) {
			panic("distmpte: p_sm2");
		}

		for(i=0; i < sminfo.smseg; i++)
			if(p->p_sm[i].sm_p == sp)
				break;
		if(i >= sminfo.smseg)
			panic("distsmpte #2");
	}
}
#endif distsmpte

#ifndef dirtysm
/* 
 *  DIRTYSM -- checks for dirty ptes in process space
 */

dirtysm(sp, smp)
	register struct smem *sp;	/* pointer to smem structure */
	register size_t smp;		/* offset into SMS */
{
	register struct proc *p;	/* Proc pointer */
	register struct pte *pte;	/* pointer to Proc's pte */
	register int i;		

	XPRINTF(XPR_VM,"enter dirtysm",0,0,0,0);
	/* SMS offset must be on cluster boundary */
	if (smp % CLSIZE)
		panic("dirtysm: smp");

	/* if the SMS is currently not attached to any process then return */
	if(sp->sm_ccount == 0)
		return (0);

	/* find SMS of first attached proc */
        p = sp->sm_caddr;

	if(p->p_sm == (struct p_sm *) NULL) {
		panic("dirtysm: p_sm1");
	}

	for(i = 0; i < sminfo.smseg; i++)
		if(p->p_sm[i].sm_p == sp)
			break;
	if(i >= sminfo.smseg)
		panic("dirtysm: no SMS");

	/*
	 * follow attached proc list, looking for associated dirty
	 * PTEs.  If one is found, then return (1), else return (0)
	 */
	while(p){
		pte = p->p_p0br + p->p_sm[i].sm_saddr+smp;
		if (dirtycl(pte))
			return(1);

		/* now for next proc in linked list	*/
		if((p = p->p_sm[i].sm_link) == NULL)
			break;

		if(p->p_sm == (struct p_sm *) NULL) {
			panic("dirtysm: p_sm2");
		}

		/* find next proc's SMS */
		for(i=0; i < sminfo.smseg; i++)
			if(p->p_sm[i].sm_p == sp)
				break;
		if(i >= sminfo.smseg)
			panic("dirtysm: no SMS #2");
	}
	return (0);
}
#endif dirtysm

/*
 * Release page tables of process p.  If the process is in context, then
 * "vmemfree" is notified that the released page frames should be placed
 * on the "ucmap" free list until after context switch.
 */
vrelpt(p)
	register struct proc *p;
{
	register int a;

	if (p->p_szpt == 0)
		return;
	a = btokmx(p->p_p0br);
	if (u.u_procp == p)
		(void) vmemfree(&Usrptmap[a],-(p->p_szpt));
	else
		(void) vmemfree(&Usrptmap[a], p->p_szpt);
	rmfree(kernelmap, (long)p->p_szpt, (long)a);
}

#define	Xu(a)  {register int t; \
		t = up->u_pcb.a; up->u_pcb.a = uq->u_pcb.a; uq->u_pcb.a = t;}
#define	Xup(a) {register struct pte *tp; \
	        tp = up->u_pcb.a; up->u_pcb.a = uq->u_pcb.a; uq->u_pcb.a = tp;}
#define	Xp(a)  {register int t; t = p->a; p->a = q->a; q->a = t;}
#define	Xpp(a) {register struct pte *tp; tp = p->a; p->a = q->a; q->a = tp;}

/*
 * Pass the page tables of process p to process q.
 * Used during vfork().  P and q are not symmetric;
 * p is the giver and q the receiver; after calling vpasspt
 * p will be ``cleaned out''.  Thus before vfork() we call vpasspt
 * with the child as q and give it our resources; after vfork() we
 * call vpasspt with the child as p to steal our resources back.
 * We are cognizant of whether we are p or q because we have to
 * be careful to keep our u. area and restore the other u. area from
 * umap after we temporarily put our u. area in both p and q's page tables.
 */
vpasspt(p, q, up, uq, umap)
	register struct proc *p, *q;
	register struct user *up, *uq;
	struct pte *umap;
{
#define P0PAGES P1PAGES
	int old_p0length, old_p1length;
	unsigned old_p1start;
	XPRINTF(XPR_VM,"enter vpasspt",0,0,0,0);
	if (up != &u) {		/* Force p==parent and q==child. */
		register caddr_t x;
		x =  (caddr_t)p;   p =  q;  q = (struct proc *)x;
		x = (caddr_t)up;  up = uq; uq = (struct user *)x;
	}
	/*
	 * Double map the parent's u. area and fork window into the
	 * child's u. area and fork window.  A copy of the child's
	 * u. area map remains in umap.  This code is executed by
	 * the parent while the child is not running.
	 */
	bcopy(Forkmap, q->p_addr - FORKPAGES, HIGHPAGES*sizeof(struct pte));
	/* Switch the page tables. */
	old_p0length = u.u_pcb.pcb_p0lr;
	old_p1start = P0PAGES + u.u_pcb.pcb_p1lr;
	old_p1length = P1PAGES - u.u_pcb.pcb_p1lr;
	Xu(pcb_szpt); Xu(pcb_p0lr); Xu(pcb_p1lr); Xup(pcb_p0br); Xup(pcb_p1br);
	Xpp(p_p0br); Xp(p_szpt); Xpp(p_addr);

	mtpr(P0BR, u.u_pcb.pcb_p0br);
	mtpr(P1BR, u.u_pcb.pcb_p1br);
	mtpr(P0LR, u.u_pcb.pcb_p0lr &~ AST_CLR);
	mtpr(P1LR, u.u_pcb.pcb_p1lr);
	/* Invalidate address translations to our former self. */
	newptes(u.u_procp, LOWPAGES, old_p0length);
	newptes(u.u_procp, old_p1start, old_p1length);

	/*
	 * Restore the child's u. area.  Use only page frame numbers
	 * from umap, as protection values in umap are not the
	 * "natural" protections for u. area pages.
	 */
	{
		register int i;
		for (i = 0; i < UPAGES; i++)
			q->p_addr[i].pg_pfnum = umap[i].pg_pfnum;
	}
}

/*
 * Get u area for process p.  If a old u area is given,
 * then copy the new area from the old, else
 * swap in as specified in the proc structure.
 *
 * Since argument map/newu is potentially shared
 * when an old u. is provided we have to be careful not
 * to block after beginning to use them in this case.
 * (This is not true when called from swapin() with no old u.)
 */
vgetu(p, palloc, map, newu, oldu)
	register struct proc *p;
	int (*palloc)();
	register struct pte *map;
	register struct user *newu;
	struct user *oldu;
{
	register int i;
	int ret, memall();

	XPRINTF(XPR_VM,"enter vgetu",0,0,0,0);

	if((*palloc)(p->p_addr, clrnd(UPAGES), p, CSYS, NULL,V_NOOP) == 0)			return(0);

	/*
	 * New u. pages are to be accessible in map/newu as well
	 * as in process p's virtual memory.
	 */
	for (i = 0; i < UPAGES; i++) {
		*(int *)(map+i) = *(int *)(p->p_addr+i)
			& PG_PFNUM | PG_V | PG_M | PG_KW;
		*(int *)(p->p_addr + i) |= PG_URKW | PG_V | PG_M;
	}
	Sysmap[btop((long)(p->p_pcb)&0x7fffffff)] = *p->p_addr;
	setredzone(p->p_addr, (caddr_t)0);
	vmaccess(map, (caddr_t)newu, UPAGES);
	/*
	 * New u.'s come from forking or inswap.
	 */
	if (oldu) {
		bcopy((caddr_t)oldu, (caddr_t)newu, UPAGES * NBPG);
		newu->u_procp = p;
	} else {
		swap(p, *(p->p_smap->dm_ptdaddr), (caddr_t)0, ctob(UPAGES),
			B_READ, B_UAREA, swapdev, 0);
		if (
		    newu->u_pcb.pcb_ssp != -1 ||
		    newu->u_tsize != p->p_tsize || newu->u_dsize != p->p_dsize ||
		    newu->u_ssize != p->p_ssize || newu->u_procp != p)
			panic("vgetu");
	}
	/*
	 * Initialize the pcb copies of the p0 and p1 region bases and
	 * software page table size from the information in the proc structure.
	 */
	newu->u_pcb.pcb_p0br = p->p_p0br;
	newu->u_pcb.pcb_p1br = initp1br(p->p_p0br + p->p_szpt * NPTEPG);
	newu->u_pcb.pcb_szpt = p->p_szpt;
	return (1);
}

/*
 * Release u. area, swapping it out if desired.
 *
 * To fix the "stale u-area" problem, this routine will indicate to 
 * [v]memall that the u-area must be put in a temp "u" list until
 * after the context switch.  This will only happen if the u-area in
 * question is currently in context.
 */

vrelu(p, swapu)
	register struct proc *p;
{
	register int i;
	struct pte uu[UPAGES];

	XPRINTF(XPR_VM,"enter vrelu",0,0,0,0);
	if (swapu)
		swap(p, *(p->p_smap->dm_ptdaddr), (caddr_t)0, ctob(UPAGES),
		    B_WRITE, B_UAREA, swapdev, 0);
	for (i = 0; i < UPAGES; i++)
		uu[i] = p->p_addr[i];

	if (u.u_procp == p)
		(void) vmemfree(uu, -(clrnd(UPAGES)));
	else
		(void) vmemfree(uu, clrnd(UPAGES));
}

#ifdef unneeded
int	ptforceswap;
#endif
/*
 * Expand a page table, assigning new kernel virtual
 * space and copying the page table entries over both
 * in the system map and as necessary in the user page table space.
 */
/*ARGSUSED*/
ptexpand(change, region)
	register int change;
	int region;		/* this arg only used in MIPS version */
{
	register int kold = btokmx(u.u_pcb.pcb_p0br);
	register int knew;	/* Must leave a register for p1br below */
	int spages, tdpages;
	int ss = P1PAGES - u.u_pcb.pcb_p1lr;
	int szpt = u.u_pcb.pcb_szpt;
	int s;
	struct cpudata *pcpu;

	if (change <= 0 || change % CLSIZE)
		panic("ptexpand");
	/*
	 * Change is the number of new page table pages needed.
	 * Kold is the old index in the kernelmap of the page tables.
	 * Allocate a new kernel map segment of size szpt+change for
	 * the page tables, and the new page table pages in the
	 * middle of this new region.
	 */
top:
#ifdef unneeded
	if (ptforceswap)
		goto bad;
#endif
	if ((knew=rmalloc(kernelmap, (long)(szpt+change))) == 0)
		goto bad;
	spages = ss/NPTEPG;
	tdpages = szpt - spages;
	if (memall(&Usrptmap[knew+tdpages], change, u.u_procp, CSYS,
	NULL, V_NOOP) == 0) {
		rmfree(kernelmap, (long)(szpt+change), (long)knew);
		goto bad;
	}

	/* Quiesce the vector processor if necessary */
	VPSYNC ();

	/*
	 * Spages pages of u.+stack page tables go over unchanged.
	 * Tdpages of text+data page table may contain a few stack
	 * pages which need to go in one of the newly allocated pages;
	 * this is a rough cut.
	 */
	kmcopy(knew, kold, tdpages);
	kmcopy(knew+tdpages+change, kold+tdpages, spages);
	{	/* This block localizes register usage. */
		register struct pte *p1, *p2;
		register int i;

	/*
	 * Validate and clear the newly allocated page table pages in the
	 * center of the new region of the kernelmap.
	 */
		for (i = knew + tdpages, p1 = &Usrptmap[i], p2 = p1 + change;
		     p1 < p2;
		     p1++, i++) {
			Hard(p1) &= ~PG_PROT;
			Hard(p1) |= (int)(PG_V | PG_KW | PG_M);
			mtpr(TBIS, kmxtob(i));
			bzero(kmxtob(i), NBPG);
		}
	/*
	 * Move the stack and u. pte's which are before the newly
	 * allocated pages into the last of the newly allocated pages.
	 * They are taken from the end of the current p1 region,
	 * and moved to the end of the new p1 region.
	 */
		p1 = u.u_pcb.pcb_p1br + u.u_pcb.pcb_p1lr;
		p2 = initp1br(kmxtob(knew+szpt+change)) + u.u_pcb.pcb_p1lr;

		s = splimp();
		smp_lock(&lk_text, LK_RETRY);
		for (i = kmxtob(kold+szpt) - p1; i != 0; i--)
			*p2++ = *p1++;
	}
	{	/* This block localizes register usage. */
		register struct pte *p1br; /* MANDATORY REGISTER (see below) */
		register struct proc *p = u.u_procp;
	/*
	 * Now switch to the new page tables.  This change must be
	 * synchronized with other processes sharing the text, with any
	 * process removing one of this process' reclaimable pages from
	 * the free list, and perhaps someday with pageout.
	 */
		p->p_p0br = kmxtob(knew);
		p->p_szpt += change;
		p->p_addr = uaddr(p);
		tbsync();
		smp_unlock(&lk_text);
		(void) splx(s);
		setp0br(p->p_p0br);
	/*
	 * Here the process remaps the stack on which it is running.
	 * Use a register to avoid confusion.
	 */
		p1br = initp1br(kmxtob(knew+szpt+change));
		setp1br(p1br);
		mtpr(TBIA, 0);
	}

	u.u_pcb.pcb_szpt += change;
	/*
	 * Finally, free old kernelmap.
	 */
	if (szpt)
		rmfree(kernelmap, (long)szpt, (long)kold);
	return;

bad:
	/*
	 * Swap out the process so that the unavailable 
	 * resource will be allocated upon swapin.
	 */
	SET_P_VM(u.u_procp, SSWAP);
	do {
		swapself++;
		sleep((caddr_t)&swapself, PSWP);
	} while (u.u_procp->p_vm & SSWAP);
}

/*
* This is a vax only routine. Will not exist in MIPS
*/
vax_noaccess(p)
register struct proc *p;
{

	register struct pte *t_begin, *t_end;

	t_end = p->p_p0br+(p->p_szpt*NPTEPG) - (p->p_ssize+HIGHPAGES);
	t_begin = p->p_p0br+p->p_tsize+p->p_dsize;

	blkclr((caddr_t)t_begin, (t_end-t_begin)*sizeof(struct pte));
}
