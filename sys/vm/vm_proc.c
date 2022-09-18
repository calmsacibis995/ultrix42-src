#ifndef lint
static	char	*sccsid = "@(#)vm_proc.c	4.4	(ULTRIX)	2/28/91";
#endif lint

/***********************************************************************
 *									*
 *			Copyright (c) 1986, 1988, 1989 by		*
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
 * 28-Feb-91 -- prs
 *	Added support for a configurable number of
 *	open file descriptors.
 *
 * 14-Nov-90 -- jaa
 *	fix for poplog cld - if user protects some page(s) noaccess
 *	with mprotect, and then forks, we must handle the noaccess
 *	pages without a tlbmiss in vmdup()
 *
 * 4-Sep-90	dlh
 *	added include vectors.h
 *
 * 19-Jun-90 -- jmartin
 *	Fixes for "panic: vrelvm rssize"
 *
 * 03-Mar-90 jaw
 *	ipl fix for mips.
 *
 *  16 Feb 90 sekhar
 * 	Modified vmdup so that pages which are not fill on demand are 
 *	copied by allocating FORKPAGES at a time. 
 *
 *  14 Feb 90 sekhar
 * 	Turn off SPHYSIO bit in vrelvm if  this process was created with
 * 	a vfork. This bit was propagated from its parent on a vfork.
 *
 *  11 Dec 89 jaa
 *	change dynamic swap to account for swap up front (ala v3.1) 
 * 	but actually do the allocation only when pushing the page/process
 *
 * 08-Dec-89 -- gmm
 *	Remove call to flush_tlb() after setjmp() for mips. Now this is done
 *	on a per process basis before the process resumed.
 *
 * 30-Nov-89 -- jmartin
 *	Support for coprocessor access to process virtual memory.
 *
 * 14-Nov-89 -- gmm
 *	Release lk_rq in the context of the child process (in setjmp). Also
 *	flush tlbs in the child process. This has to be done better. All these
 *	only for mips.
 *
 * 1 Sep 89 -- jaa
 *	fixed old bug - when shrinking in expand(), 
 *	v must be decremented to properly invalidate 
 *	the addr from the tb
 *
 * 31 Aug 89 -- gg
 *	Added missing initialisation to expand() .
 *
 * 4 Aug 89 -- jaa
 *	bug fix to expand
 *
 * 24 Jul 89 -- jmartin
 *	Change interface to ptexpand.
 *
 * 24 Jul 89 -- jaa
 *	Fix a race condition in vpassvm with hardclock
 *
 * 12 Jun 89 -- gg
 *	Dynamic swap changes.
 *	The dmap structure is moved to proc from user,so the 
 *	corresponding changes.
 *
 *  8 May 89 -- Giles Atkinson
 *    Zero u area list header for exit_actn structs in procdup().
 *
 * 02-May-89 -- jaw, jmartin
 *	fix forkutl to work on mips.
 *
 *  6 Mar 89 -- jmartin
 *	Clear c_intrans (now set by memall()) in vmdup().
 *
 * 15 Dec 88 -- jmartin
 *	SMP locking for shared memory data structures (cf. h/shm.h)
 *	Integration of v3.0 shared memory changes, i.e. KM_ALLOC of
 *	per-process shared memory structures.
 *
 * 11 Oct 88 -- jmartin
 *	Make the highest page (cluster) of forkutl a guard page during
 *	vmdup.
 *
 * 31 Aug 88 -- jmartin
 *	Use lk_text to protect exchange between vfork parent and child
 *	on the text chain.
 *
 * 25 Jul 88 -- jmartin
 *	Use the macro SET_P_VM and the lock lk_cmap in vmdup.
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
 * 14 Jul 87 -- depp
 *      insured that vfork() shared memory semantics were handled properly
 *      in vpassvm().
 *
 * 15 Dec 86 -- depp
 *	Added fix to shared memory handling, in regards to a process' SM 
 *	page table swapping space.  
 *
 * 11 Sep 86 -- koehler
 *	moved a few things into registers, added more informative print
 *
 * 29 Apr 86 -- depp
 *	converted to locking macros from calls routines
 *
 * 11 Nov 85 -- depp
 *	Removed all conditional compiles for System V IPC.
 *
 *  19 Jul 85 -- depp
 *	fixed bug in SMEXPAND routine, if change < 0, then base += change,
 *	NOT base -= change
 *
 *  05-May-85 - Larry Cohen
 *	loop up to u_omax instead of NOFILE
 *
 * 001 - March 11 1985 - Larry Cohen
 *     disable mapped in files so NOFILE can be larger than 32
 *
 * 11 Mar 85 -- depp
 *	Added System V shared memory support
 *
 * 19 Dec 84 -- jrs
 *	Changed setjmp's to savectx for swap recovery fixes
 *	Derived from 4.2 BSD, labeled:
 *		vm_proc.c	6.1	83/07/29
 *
 *
 *----------------------------------------------------------------------
 */
#include "../machine/pte.h"

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/proc.h"
#include "../h/map.h"
#include "../h/cmap.h"
#include "../h/text.h"
#include "../h/vm.h"
#include "../h/file.h"

#ifdef vax
#include "../machine/mtpr.h"
#endif vax
#include "../machine/cpu.h"
#ifdef vax
#include "../../machine/common/cpuconf.h"
#include "../machine/vectors.h"
#endif vax

#include "../h/kmalloc.h"
#include "../h/ipc.h"
#include "../h/shm.h"
#include "../h/smp_lock.h"
extern struct sminfo sminfo;
int foodebug=0;

/*
 * Get virtual memory resources for a new process.
 * Called after page tables are allocated, but before they
 * are initialized, we initialize the memory management registers,
 * and then expand the page tables for the data and stack segments
 * creating zero fill pte's there.  Text pte's are set up elsewhere.
 *
 * SHOULD FREE EXTRA PAGE TABLE PAGES HERE OR SOMEWHERE.
 */
vgetvm(ts, ds, ss)
	size_t ts, ds, ss;
{

#ifdef vax
	u.u_pcb.pcb_p0lr = AST_NONE;
	setp0lr(ts);
	setp1lr(P1PAGES - HIGHPAGES);
#endif vax
#ifdef mips
        u.u_procp->p_dsize = 0;
        u.u_procp->p_ssize = 0;
#endif mips
	u.u_procp->p_tsize = ts;
	u.u_tsize = ts;
	expand((int)ss, 1);
	expand((int)ds, 0);
}

/*

 * Release the virtual memory resources (memory
 * pages, and swap area) associated with the current process.
 * Caller must not be swappable.  Used at exit or execl.
 */
vrelvm()
{
	register struct proc *p = u.u_procp;
	int prss_orig, free_count;
	register int s;
	
	if (p->p_vm & SVFORK) {
		s = splimp();
		smp_lock(&lk_p_vm, LK_RETRY);
		p->p_vm &= ~(SVFORK | SPHYSIO);
		p->p_vm |= SKEEP;
		wakeup((caddr_t)p);
		while ((p->p_vm & SVFDONE) == 0) {
			sleep_unlock((caddr_t)p, PZERO - 1, &lk_p_vm);
			smp_lock(&lk_p_vm, LK_RETRY);
		}
		p->p_vm &= ~(SVFDONE|SKEEP);
		smp_unlock(&lk_p_vm);
		(void)splx(s);

		return;
	}
	/*
	 * Release memory; text first, shared segments then data/stack pages.
	 */
	xfree();
	if (p->p_smcount) {		/* SHMEM */
		if(p->p_sm == (struct p_sm *) NULL)
			panic("vrelvm: p_sm");
		smclean();
	}

	free_count = vmemfree(dptopte(p, 0), p->p_dsize) +
			vmemfree(sptopte(p, p->p_ssize - 1), p->p_ssize);
	s = splimp();
	smp_lock(&lk_p_vm, LK_RETRY);
	prss_orig = p->p_rssize; /* save it */
	if ((p->p_rssize -= free_count) != 0) {
		printf("p = %x, p_rssize = %d, p_textp = %x, prss_orig = %d\n",
					p,p->p_rssize,p->p_textp,prss_orig);
		panic("vrelvm rss");
	}
	/*
	 * Wait for all page outs to complete, then
	 * release swap space.
	 */
	p->p_swrss = 0;

	while (p->p_poip) {
		sleep_unlock((caddr_t)&p->p_poip, PSWP+1, &lk_p_vm);
		smp_lock(&lk_p_vm, LK_RETRY);
	}		
	smp_unlock(&lk_p_vm);
	(void)splx(s);

	(void)vsfree(p->p_dmap, CDATA);
	(void)dmfree(p->p_dmap, p->p_dsize, CDATA);
	p->p_dmap = (struct dmap *)NULL;

	(void)vsfree(p->p_smap, CSTACK);
	(void)dmfree(p->p_smap, p->p_ssize, CSTACK);
	p->p_smap = (struct dmap *)NULL;

	p->p_tsize = p->p_dsize = p->p_ssize = 0;
	u.u_tsize = u.u_dsize = u.u_ssize = 0;
	release_dev_VM_maint(p);
}

/*
 * Pass virtual memory resources from p to q.
 * P's u. area is up, q's is uq.  Used internally
 * when starting/ending a vfork().
 */
vpassvm(p, q, up, uq, umap)
	register struct proc *p, *q;
	register struct user *up, *uq;
	struct pte *umap;
{
	int i;
	int s;



	s = splclock();

	smp_lock(&lk_smem, LK_RETRY);
	smp_lock(&lk_text, LK_RETRY);
	smp_lock(&lk_p_vm, LK_RETRY);

	uq->u_smsize = q->p_smsize = p->p_smsize; 
	up->u_smsize = p->p_smsize = 0;
	q->p_smend = p->p_smend; p->p_smend = 0;

  	q->p_smcount = 0;
  	q->p_smbeg = 0;
  
	if (q->p_sm = p->p_sm) {
		if (sminfo.smseg == 0)
  			panic("vpassvm: parent has smem, smseg == 0");
		for(i = 0; i < sminfo.smseg; i++)
  			smrepl(p, q, i);
		q->p_smcount = p->p_smcount; p->p_smcount = 0;
		q->p_smbeg = p->p_smbeg; p->p_smbeg = 0;
  		p->p_sm = (struct p_sm *) NULL;

  	}
  
	/*
	 * Pass u. paging statistics.
	 */
	uq->u_outime = up->u_outime; up->u_outime = 0;
	uq->u_ru = up->u_ru;
	bzero((caddr_t)&up->u_ru, sizeof (struct rusage));
	uq->u_cru = up->u_cru;
	bzero((caddr_t)&up->u_cru, sizeof (struct rusage));




	/*
	 * Pass fields related to vm sizes.
	 */
	uq->u_tsize = q->p_tsize = p->p_tsize; up->u_tsize = p->p_tsize = 0;
	uq->u_dsize = q->p_dsize = p->p_dsize; up->u_dsize = p->p_dsize = 0;
	uq->u_ssize = q->p_ssize = p->p_ssize; up->u_ssize = p->p_ssize = 0;

	/*
	 * Pass proc table paging statistics.
	 */
	q->p_swrss = p->p_swrss; p->p_swrss = 0;
	q->p_rssize = p->p_rssize; p->p_rssize = 0;
	q->p_poip = p->p_poip; p->p_poip = 0;


	/*
	 * Pass swap space maps.
	 */
	q->p_dmap = p->p_dmap; p->p_dmap = (struct dmap *)NULL;
	q->p_smap = p->p_smap; p->p_smap = (struct dmap *)NULL;


	/*
	 * Relink text segment.
	 */
	q->p_textp = p->p_textp;
	xrepl(p, q);
	p->p_textp = 0;
	u.u_procp->p_vm ^= SNOVM;
	if (up == &u)
		/*
		 * The parent is giving up resources to the child.  It
		 * keeps a pointer to the child because cmap table
		 * entries still point to the parent even while the
		 * child has the page tables.  (See pfclear or checkpage.)
		 */
  		p->p_xlink = q;	

	/*
	 * And finally, pass the page tables themselves.
	 * On return we are running on the other set of
	 * page tables, but still with the same u. area.
	 */
	vpasspt(p, q, up, uq, umap);
	smp_unlock(&lk_p_vm);
	smp_unlock(&lk_text);
	smp_unlock(&lk_smem);
	(void)splx(s);
}

#ifdef vax
/*
 * Change the size of the data+stack regions of the process.
 * If the size is shrinking, it's easy-- just release virtual memory.
 * If it's growing, initalize new page table entries as 
 * 'zero fill on demand' pages.
 */
expand(change, region)
	register int change;
	int region;
{
	register struct proc *p;
	register struct pte *base, *p0, *p1;
	struct pte proto;
	int p0lr, p1lr;
	struct pte *base0;
	int size;
	u_int v;

	p = u.u_procp;
	if (change == 0)
		return;
	if (change % CLSIZE)
		panic("expand");

#ifdef PGINPROF
	vmsizmon();
#endif

	/*
	 * Update the sizes to reflect the change.  Note that we may
	 * swap as a result of a ptexpand, but this will work, because
	 * the routines which swap out will get the current text and data
	 * sizes from the arguments they are passed, and when the process
	 * resumes the lengths in the proc structure are used to 
	 * build the new page tables.
	 */
	u.u_odsize = u.u_dsize;
	u.u_ossize = u.u_ssize;
	u.u_osmsize = p->p_smsize;
	if (region == 0) {
		v = dptov(p, p->p_dsize);
		u.u_dsize = p->p_dsize += change;
	} else {
		u.u_ssize = p->p_ssize += change;
		v = sptov(p, p->p_ssize-1);
	}

	/*
	 * Compute the end of the text+data regions and the beginning
	 * of the stack region in the page tables,
	 * and expand the page tables if necessary.
	 */
	p0 = u.u_pcb.pcb_p0br + (u.u_pcb.pcb_p0lr&~AST_CLR);
	p1 = u.u_pcb.pcb_p1br + (u.u_pcb.pcb_p1lr&~PME_CLR);
	if (change > p1 - p0)
		ptexpand(clrnd(ctopt(change - (p1 - p0))), region);
	/* PTEXPAND SHOULD GIVE BACK EXCESS PAGE TABLE PAGES */

	/*
	 * Compute the base of the allocated/freed region.
	 */
	p0lr = u.u_pcb.pcb_p0lr&~AST_CLR;
	p1lr = u.u_pcb.pcb_p1lr&~PME_CLR;
	if (region == 0)
		base = u.u_pcb.pcb_p0br + p0lr + (change > 0 ? 0 : change);
	else
		base = u.u_pcb.pcb_p1br + p1lr - (change > 0 ? change : 0);

	/*
	 * If we shrunk, give back the virtual memory.
	 */
	if (change < 0) {
		int free_count, s;

		free_count = vmemfree(base, -change);
		s = splimp();
		smp_lock(&lk_p_vm, LK_RETRY);
		p->p_rssize -= free_count;
		smp_unlock(&lk_p_vm);
		(void) splx(s);
	}

	/*
	 * Update the processor length registers and copies in the pcb.
	 */
	if (region == 0)
		setp0lr(p0lr + change);
	else
		setp1lr(p1lr - change);

	/*
	 * If shrinking, clear pte's, otherwise
	 * initialize zero fill on demand pte's.
	 */
	*(int *)&proto = PG_UW;
	if (change < 0) {
		change = -change;
		v -= change;
	} else {
		proto.pg_fod = 1;
		((struct fpte *)&proto)->pg_fileno = PG_FZERO;
		cnt.v_nzfod += change;
	}
	base0 = base;
	size = change;
	while (--change >= 0)
		*base++ = proto;

	/*
	 * We changed mapping for the current process,
	 * so must update the hardware translation
	 */
	newptes(base0, v, size);
}

/* SMEXPAND - shared-memory specific expand routine. Change the	*/
/*	size of the data region of the current process. If the	*/
/*	size is shrinking, it's easy -- just release virtual	*/
/*	memory. If it's growing, initialize new page table	*/
/*	entries as 'zero fill on demand' pages.			*/
/*								*/
/*	This routine is only called from kern_mman.c/obreak.	*/
/*	It provides the same function as EXPAND but:		*/
/*		- it only expands data region			*/
/*		- it does not grow the page table		*/
/*		- it does not reset P0LR			*/
/*	LeRoy Fundingsland    3/21/85    DEC	SHMEM		*/

smexpand(change)
    register int change;
{
	register struct proc *p;
	register struct pte *base, *base0;
	struct pte proto;
	register int size;
	register u_int v;

	if (change == 0)
		return;
	if (change % CLSIZE)
		panic("expand");
	p = u.u_procp;

#ifdef PGINPROF
	vmsizmon();
#endif

		/* Update the sizes to reflect the change.	*/
	u.u_odsize = u.u_dsize;
	v = dptov(p, p->p_dsize);
	u.u_dsize = p->p_dsize += change;

		/* Compute the base of the allocated/freed region. */
	base = u.u_pcb.pcb_p0br + u.u_tsize + u.u_odsize;
	if (change < 0) {
		int free_count, s;

		base += change;
		/* If we shrunk, give back the virtual memory.	*/
		free_count = vmemfree(base, -change);
		s = splimp();
		smp_lock(&lk_p_vm, LK_RETRY);
		p->p_rssize -= free_count;
		smp_unlock(&lk_p_vm);
		(void) splx(s);
	}

		/* If shrinking, clear pte's, otherwise		*/
		/* initialize zero fill on demand pte's.	*/
	*(int *)&proto = PG_UW;
	if (change < 0)
		change = -change;
	else {
		proto.pg_fod = 1;
		((struct fpte *)&proto)->pg_fileno = PG_FZERO;
		cnt.v_nzfod += change;
	}
	base0 = base;
	size = change;
	while (--change >= 0)
		*base++ = proto;

		/* We changed mapping for the current process,	*/
		/* so must update the hardware translation	*/
	newptes(base0, v, size);
}
#endif vax

#ifdef mips
/*
 * Change the size of the data+stack regions of the process.
 * If the size is shrinking, it's easy-- just release virtual memory.
 * If it's growing, initalize new page table entries as 
 * 'zero fill on demand' pages.
 */
expand(change, region)
	int change, region;
{
	register struct proc *p;
	register struct pte *base;
	struct pte proto;
	int size;
	u_int v;
	int avail;

	p = u.u_procp;
	if (change == 0)
		return;
	if (change % CLSIZE)
		panic("expand");

#ifdef PGINPROF
	vmsizmon();
#endif

	/*
	 * Update the sizes to reflect the change.  Note that we may
	 * swap as a result of a ptexpand, but this will work, because
	 * the routines which swap out will get the current text and data
	 * sizes from the arguments they are passed, and when the process
	 * resumes the lengths in the proc structure are used to 
	 * build the new page tables.
	 */
	u.u_odsize = u.u_dsize;
	u.u_ossize = u.u_ssize;
	if (region == 0) {
		avail = (p->p_datapt * NPTEPG) - p->p_dsize;
		v = dptov(p, p->p_dsize);
		u.u_dsize= p->p_dsize += change;
		if (change > avail) 
			ptexpand(clrnd(ctopt(change - avail)), region);
		base = p->p_databr + u.u_odsize + (change > 0 ? 0 : change);
	} else {
		avail = (p->p_stakpt * NPTEPG)
			- HIGHPAGES - p->p_ssize;
		u.u_ssize = p->p_ssize += change;
		v = sptov(p, p->p_ssize - 1);

		if (change > avail) 
			ptexpand(clrnd(ctopt(change - avail)), region);
		base = (p->p_stakbr + p->p_stakpt * NPTEPG) 
			- (p->p_ssize + HIGHPAGES) + (change > 0 ? 0 : change);
	}

	/*
	 * If we shrunk, give back the virtual memory.
	 */

	if (change < 0) {
		int free_count, s;

		/* If we shrunk, give back the virtual memory.	*/
		free_count = vmemfree(base, -change);
		s = splimp();
		smp_lock(&lk_p_vm, LK_RETRY);
		p->p_rssize -= free_count;
		smp_unlock(&lk_p_vm);
		(void) splx(s);
	}
	
	/*
	 * If shrinking, clear pte's, otherwise
	 * initialize zero fill on demand pte's.
	 */
	*(int *)&proto = PG_UW;
	if (change < 0) {
		change = -change;
		v -= change;
	} else {
		proto.pg_fod = 1;
		((struct fpte *)&proto)->pg_fileno = PG_FZERO;
			cnt.v_nzfod += change;
	}
	size = change;
	while (--change >= 0)
		*base++ = proto;

	/*
	 * We changed mapping for the current process,
	 * so must update the hardware translation
	 */
	newptes(p, v, size);
}

smexpand()
{
}

#endif mips

/*
 * Create a duplicate copy of the current process
 * in process slot p, which has been partially initialized
 * by newproc().
 *
 * Could deadlock here if two large proc's get page tables
 * and then each gets part of his UPAGES if they then have
 * consumed all the available memory.  This can only happen when
 *	USRPTSIZE + UPAGES * NPROC > maxmem
 * which is impossible except on systems with tiny real memories,
 * when large procs stupidly fork() instead of vfork().
 */
procdup(p, isvfork)
	register struct proc *p;
	int isvfork;
{
	register struct text *xp;
#ifdef mips
	register int s;
#endif mips
	/*
	 * Allocate page tables for new process, waiting
	 * for memory to be free.
	 */
#ifdef mips
        if (isvfork == 0) {
#endif mips
		while (vgetpt(p, vmemall, vmemfree) == 0) {
		kmapwnt++;
		sleep((caddr_t)kernelmap, PSWP+4);
	}
#ifdef mips
	}
#endif mips
	/*
	 * Snapshot the current u. area pcb and get a u.
	 * for the new process, a copy of our u.
	 */
#ifdef vax
	resume(pcbb(u.u_procp));
#endif vax
#ifdef mips
        save();
#endif mips
	(void) vgetu(p, vmemall, Forkmap, &forkutl, &u);
	/*
	 * Arrange for a non-local goto when the new process
	 * is started, to resume here, returning nonzero from savectx.
	 */
	forkutl.u_pcb.pcb_sswap = (int *)&u.u_ssave;

#ifdef mips
	/*
         * TODO: can this setjmp() be avoided? perhaps only update PC.
         */
	s = splhigh(); /* child returns with lk_rq held and its ipl should 
			  remain high */
        if (setjmp(&forkutl.u_ssave)) {
		smp_unlock(&lk_rq); 
		splx(s); /* restore the ipl before save() above in the parent*/
		if (CURRENT_CPUDATA->cpu_exitproc) {
			vrelu(CURRENT_CPUDATA->cpu_exitproc,0);
			vrelpt(CURRENT_CPUDATA->cpu_exitproc);
			proc_exit(CURRENT_CPUDATA->cpu_exitproc);
			CURRENT_CPUDATA->cpu_exitproc = 0; 
		}
#endif mips
#ifdef vax
	if (savectx(&forkutl.u_ssave))
#endif vax
		/*
		 * Return 1 in child.
		 */
		return (1);
#ifdef mips
	}
	splx(s);
#endif mips

	dup_ofiles(&forkutl);

	/* Do not pass exit actions list to child. */

	forkutl.u_exitp = 0;

	/*
	 * If the new process is being created in vfork(), then
	 * exchange vm resources with it.  We want to end up with
	 * just a u. area and an empty p0 region, so initialize the
	 * prototypes in the other area before the exchange.
	 */
	if (isvfork) {
#ifdef vax
		forkutl.u_pcb.pcb_p0lr = u.u_pcb.pcb_p0lr & AST_CLR;
		forkutl.u_pcb.pcb_p1lr = P1PAGES - HIGHPAGES;
#endif
#ifdef mips
		/*
		 * I think this is a bug on the VAX verion anyway.
		 * The child is started with an empty page table and
		 * then the parent and child swap page tables. The above
		 * statement has no effect because the parent will not
		 * run again in user mode until after it gets its page
		 * table back.
		 * Copying resched isn't really necessary, I'm not sure
		 * why they copy the AST bits on the VAX.
		 */
		forkutl.u_pcb.pcb_resched = u.u_pcb.pcb_resched;
#endif
		vpassvm(u.u_procp, p, &u, &forkutl, Forkmap);
#ifdef mips
		clear_tlbmappings(p);
#endif mips
		/*
		 * Return 0 in parent.
		 */
		return (0);
	}
	/*
	 * A real fork; clear vm statistics of new process
	 * and link into the new text segment.
	 * Equivalent things happen during vfork() in vpassvm().
	 */
	bzero((caddr_t)&forkutl.u_ru, sizeof (struct rusage));
	bzero((caddr_t)&forkutl.u_cru, sizeof (struct rusage));
	forkutl.u_procp->p_dmap = u.u_procp->p_cdmap;
	forkutl.u_procp->p_smap = u.u_procp->p_csmap;
	forkutl.u_outime = 0;

	/*
	 * Attach to the text segment.
	 */
	if (xp = p->p_textp) {
		X_LOCK(xp);
		xp->x_count++;
#ifdef mips
                while (xlink(p) == 0) {
                        kmapwnt++;
                        sleep((caddr_t)kernelmap, PSWP+4);
                }
#endif mips
#ifdef vax
		xlink(p);
#endif vax
		X_UNLOCK(xp);
	}

#ifdef mips
        clear_tlbmappings(p);
#endif mips

	/* attach all shared memory segments	SHMEM	*/
	if(p->p_smcount)
		smfork(u.u_procp, p);

	/*
	 * Duplicate data and stack space of current process
	 * in the new process.
	 */
	vmdup(p, dptopte(p, 0), dptov(p, 0), p->p_dsize, CDATA);
	vmdup(p, sptopte(p, p->p_ssize - 1), sptov(p, p->p_ssize - 1), p->p_ssize, CSTACK);

	/*
	 * Return 0 in parent.
	 */
	return (0);
}

dup_ofiles(child_u)
register struct user *child_u;
{
	register int n;
	register struct file *fp;

	/*
	 * If parent u_area has an overflow array for open file
	 * descriptors, make another km_alloc'ed area for child
	 * u_area to hold extra file descriptors.
	 */
	if (u.u_of_count != 0) {
		KM_ALLOC(child_u->u_ofile_of, struct file **, 
			 u.u_of_count * sizeof (struct file *),
			 KM_NOFILE, KM_CLEAR);
		KM_ALLOC(child_u->u_pofile_of, char *, u.u_of_count,
			 KM_NOFILE, KM_CLEAR);
	}

#define CHILDU_OFILE_SET(fd, value) { \
	            if ((unsigned)(fd) < NOFILE_IN_U ) \
		        child_u->u_ofile[(fd)] = (value); \
                    else \
	                child_u->u_ofile_of[(fd) - NOFILE_IN_U] = (value); \
}
#define CHILDU_POFILE_SET(fd, value) { \
	            if ((unsigned)(fd) < NOFILE_IN_U ) \
		        child_u->u_pofile[(fd)] = (value); \
                    else \
	                child_u->u_pofile_of[(fd) - NOFILE_IN_U] = (value); \
}
	/* 001
	 * Increase reference counts on shared objects
	 * and remove INUSE reference to inode.
	 */
	smp_lock(&lk_file, LK_RETRY);
	for (n = 0; n <= u.u_omax; n++) {
		fp = U_OFILE(n);
		if (fp == NULL)
			continue;
		fp->f_count++;
		CHILDU_OFILE_SET(n, fp);
		CHILDU_POFILE_SET(n, U_POFILE(n) & ~(UF_INUSE));
	}
	smp_unlock(&lk_file);
}

	/* Copy the parent's memory to the child */

vmdup(p, pte, v, count, type)
	struct proc *p;
	register struct pte *pte;
	unsigned v;
	register size_t count;
	int type;
{
	register int copy_count, i;
	register struct pte *opte = vtopte(u.u_procp, v);
	register char *a = (char *)NULL;
	int s; 
	unsigned fod, maxcopy;

	if (count == 0)
		return;
	if (opte == (struct pte *)NULL && count != 0)
		panic("vmdup:  parent pte not found");
	/*
	 * The outer loop executes until all the pages are copied. 
	 * The number of pages copied in one iteration of this loop is 
	 * determined by the inner for loop (see note below ).
	 */
	for (; count>0; count-=copy_count) {
	       /*
		* Fill on demand pages:
		* 
		*     For these pages only the ptes need to copied.  Hence
	 	*     count the number of pages which are consecutive and 
		*     which are also fill on demand. These can then be copied
	 	*     with a single bcopy.
		* 
		* Pages NOT fill on demand:
		*       ===
	 	*     Count the number of pages which are consecutive and 
		*     which are not fill on demand. However no more than 
		*     FORKPAGES will be counted. The reason is that both
		*     ptes as well data pages have to be copied. Limiting the
		*     count to FORKPAGES limits the amount of memory 
		*     which has to be allocated for data pages using vmemall 
		*     (which also locks the pages). This is therefore specially 
		*     desirable for very large processes. 
		*     
	 	*/
		if (fod=opte->pg_fod)
			maxcopy = count;
		else
			maxcopy = (count > FORKPAGES) ? FORKPAGES : count;
		for (copy_count=CLSIZE;
		     copy_count < maxcopy && ((opte+copy_count)->pg_fod == fod);
		     copy_count+=CLSIZE)
		  	;
#ifdef mips
		/*
		 * Have to examine each page to see if any have
		 * been protected no access.  bcopy would cause
		 * a tlbmiss, since we're in kernel mode it winds
		 * up as a panic: trap.  Fault the page in and lock 
		 * it so the pageout daemon doesn't take it and 
		 * adjust the count.
		 */
		if(!fod) {
			for (i = 0; i < copy_count; ++i) {
				if ((opte+i)->pg_prot < PROT_URKR) {
					vslock(a = ptob(v+i), NBPG);
					copy_count = i + 1;
					break;
				}
			}
		}
#endif mips
		bcopy(opte, pte, copy_count * sizeof(struct pte));
		if (!fod) {		/* pages with actual contents */
			for (i = 0; i < copy_count; i++)
				*(int *)(pte+i) &= PG_PROT;
			(void) vmemall(pte, copy_count, p, type, NULL, NULL);
			for (i=0; i<copy_count; i++) 
				*(int *)(pte+i) |= (PG_V | PG_M);
			s = splimp();
			smp_lock(&lk_p_vm, LK_RETRY);
			p->p_rssize += copy_count;
			smp_unlock(&lk_p_vm);
			(void) splx(s);
#ifdef mips
        		clear_tlbmappings(u.u_procp);
#endif mips
			for (i = 0; i < copy_count; i++) 
				*(int *)(Forkmap+i) 
					= *(int *)(pte+i) & ~PG_PROT | PG_KW;
			newptes(u.u_procp, btop(&forkutl), copy_count);
			bcopy(ptob(v), &forkutl, copy_count * NBPG);
#ifdef mips
			/* unlock the page, but don't set the modify bit */
			if(a != (char *)NULL)
				vsunlock(a, NBPG, 0);
#endif mips
			s = splimp(); 
			smp_lock(&lk_cmap, LK_RETRY); 
			for (i=0; i<copy_count; i+=CLSIZE) {
				register struct cmap *c;

#ifdef mips
				if((pte+i)->pg_prot < PROT_URKR) {
					(pte+i)->pg_v = 0;
					newptes(p, v+i, 1);
				}
#endif mips
				c = &cmap[pgtocm((pte+i)->pg_pfnum)];
				c->c_intrans = 0;
				MUNLOCK(c);
			}
			smp_unlock(&lk_cmap);
			(void) splx(s);
		}
		v += copy_count;
		pte += copy_count;
		opte += copy_count;
	}
	SET_P_VM(p, SPTECHG);
}
