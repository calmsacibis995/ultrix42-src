#ifndef lint
static	char	*sccsid = "@(#)vm_proc.c	4.1	(ULTRIX)	7/2/90";
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

/*----------------------------------------------------------------------
 * Modification History
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
#include "../vax/mtpr.h"
#endif vax
#include "../machine/cpu.h"

#include "../h/kmalloc.h"
#include "../h/ipc.h"
#include "../h/shm.h"
extern struct sminfo sminfo;

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

#ifdef mips
	XPRINTF(XPR_VM,"enter vgetvm",0,0,0,0);
#endif mips
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
	int prss_orig;
	
#ifdef mips
	XPRINTF(XPR_VM,"enter vrelvm",0,0,0,0);
#endif mips
	/*
	 * Release memory; text first, shared segments then data/stack pages.
	 */
	xfree();
	if (p->p_smbeg) {		/* SHMEM */
		if(p->p_sm == (struct p_sm *) NULL){
			panic("vrelvm: p_sm");
		}
		smclean();
	}
	prss_orig = p->p_rssize; /* save it */
	p->p_rssize -= vmemfree(dptopte(p, 0), p->p_dsize);
	p->p_rssize -= vmemfree(sptopte(p, p->p_ssize - 1), p->p_ssize);
	if (p->p_rssize != 0) {
		printf("p = %x, p_rssize = %d, p_textp = %x, prss_orig = %d\n",
					p,p->p_rssize,p->p_textp,prss_orig);
		panic("vrelvm rss");
	}
	/*
	 * Wait for all page outs to complete, then
	 * release swap space.
	 */
	p->p_swrss = 0;
	while (p->p_poip)
		sleep((caddr_t)&p->p_poip, PSWP+1);
	(void) vsexpand((size_t)0, &u.u_dmap, 1);
	(void) vsexpand((size_t)0, &u.u_smap, 1);
	p->p_tsize = 0;
	p->p_dsize = 0;
	p->p_ssize = 0;
	u.u_tsize = 0;
	u.u_dsize = 0;
	u.u_ssize = 0;
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
	register struct p_sm *p_smp, *q_smp;  /* SHMEM */

#ifdef mips
	XPRINTF(XPR_VM,"enter vpassvm",0,0,0,0);
#endif mips
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
	 * Relink text segment.
	 */
	q->p_textp = p->p_textp;
	xrepl(p, q);
	p->p_textp = 0;

	/* relink all shared memory segments	SHMEM */
	uq->u_smsize = q->p_smsize = p->p_smsize; 
	up->u_smsize = p->p_smsize = 0;
	q->p_smend = p->p_smend; p->p_smend = 0;
#ifdef mips
	q->p_smcount = p->p_smcount; p->p_smcount = 0;
#endif mips
	q->p_smbeg = 0;

	if(p->p_sm == (struct p_sm *) NULL) {
		q->p_sm == (struct p_sm *) NULL;
	} else {  
		if(sminfo.smseg == 0) {
			panic("vpassvm: parent has smem, smseg == 0");
		}
		KM_ALLOC(q->p_sm, struct p_sm *, SIZEOF_PSM, KM_SHMSEG, KM_CLEAR);
		if(q->p_sm == (struct p_sm *) NULL) {
			panic("vpassvm: alloc q->p_sm");
		}

		/* relink all shared memory segments	SHMEM */
		p_smp = p->p_sm;
		q_smp = q->p_sm;
		for(i = 0; i < sminfo.smseg; i++, p_smp++, q_smp++){
			q_smp->sm_p = p_smp->sm_p;
			smrepl(p, q, i);
			p_smp->sm_p = NULL;
#ifdef vax
			q_smp->sm_spte = p_smp->sm_spte; p_smp->sm_spte = 0;
#endif vax
#ifdef mips
			q_smp->sm_saddr = p_smp->sm_saddr; p_smp->sm_saddr = 0;
			q_smp->sm_eaddr = p_smp->sm_eaddr; p_smp->sm_eaddr = 0;
#endif mips
			q_smp->sm_pflag = p_smp->sm_pflag; p_smp->sm_pflag = 0;
			q_smp->sm_lock = p_smp->sm_lock; p_smp->sm_lock = 0;
		}
		q->p_smbeg = p->p_smbeg; p->p_smbeg = 0;
		KM_FREE(p->p_sm, KM_SHMSEG);
		p->p_sm = (struct p_sm *) NULL;
	}

	/*
	 * Pass swap space maps.
	 */
	uq->u_dmap = up->u_dmap; up->u_dmap = zdmap;
	uq->u_smap = up->u_smap; up->u_smap = zdmap;

	/*
	 * Pass u. paging statistics.
	 */
	uq->u_outime = up->u_outime; up->u_outime = 0;
	uq->u_ru = up->u_ru;
	bzero((caddr_t)&up->u_ru, sizeof (struct rusage));
	uq->u_cru = up->u_cru;
	bzero((caddr_t)&up->u_cru, sizeof (struct rusage));

	/*
	 * And finally, pass the page tables themselves.
	 * On return we are running on the other set of
	 * page tables, but still with the same u. area.
	 */
	vpasspt(p, q, up, uq, umap);
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
	size_t ods, oss;
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
	ods = u.u_dsize;
	oss = u.u_ssize;
	if (region == 0) {
		v = dptov(p, p->p_dsize);
		p->p_dsize += change;
		u.u_dsize += change;
	} else {
		p->p_ssize += change;
		v = sptov(p, p->p_ssize-1);
		u.u_ssize += change;
	}

	/*
	 * Compute the end of the text+data regions and the beginning
	 * of the stack region in the page tables,
	 * and expand the page tables if necessary.
	 */
	p0 = u.u_pcb.pcb_p0br + (u.u_pcb.pcb_p0lr&~AST_CLR);
	p1 = u.u_pcb.pcb_p1br + (u.u_pcb.pcb_p1lr&~PME_CLR);
	if (change > p1 - p0)
		ptexpand(clrnd(ctopt(change - (p1 - p0))), ods, oss, 
					p->p_smsize);	/* SHMEM */
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
	if (change < 0)
		p->p_rssize -= vmemfree(base, -change);

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
	size_t ods;
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
	ods = u.u_dsize;
	v = dptov(p, p->p_dsize);
	p->p_dsize += change;
	u.u_dsize += change;

		/* Compute the base of the allocated/freed region. */
	base = u.u_pcb.pcb_p0br + u.u_tsize + ods;
	if (change < 0){
		base += change;
		/* If we shrunk, give back the virtual memory.	*/
		p->p_rssize -= vmemfree(base, -change);
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
	size_t osize;
	int size;
	u_int v;
	int avail;

XPRINTF(XPR_VM,"enter expand",0,0,0,0);
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
	if (region == 0) {
		osize = u.u_dsize;
		v = dptov(p, p->p_dsize);
		avail = (p->p_datapt * NPTEPG) - p->p_dsize;
		p->p_dsize += change;
		u.u_dsize += change;
		if (change > avail) {
			ptexpand(clrnd(ctopt(change - avail)), osize, 
			    u.u_ssize, region);
		}
		base = p->p_databr + osize + (change > 0 ? 0 : change);
	} else {
		osize = u.u_ssize;
		avail = (p->p_stakpt * NPTEPG)
			- REDZONEPAGES - p->p_ssize;
		p->p_ssize += change;
		v = sptov(p, p->p_ssize - 1);
		u.u_ssize += change;

		if (change > avail) {
			ptexpand(clrnd(ctopt(change - avail)), u.u_dsize, 
			    osize, region);
		}
		base = (p->p_stakbr + p->p_stakpt * NPTEPG) 
			- (p->p_ssize + REDZONEPAGES) + (change > 0 ? 0 : change);
	}

	/*
	 * If we shrunk, give back the virtual memory.
	 */
	if (change < 0) {
		p->p_rssize -= vmemfree(base, -change);
		XPRINTF(XPR_VM, "expand p_rssize %d change %d", p->p_rssize,
		    change, 0, 0);
	}
	
	/*
	 * If shrinking, clear pte's, otherwise
	 * initialize zero fill on demand pte's.
	 */
	*(int *)&proto = PG_UW;
	if (change < 0)
		change = -change;
	else {
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
XPRINTF(XPR_VM,"enter smexpand",0,0,0,0);
	/* wow! - rr */
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
{
	register struct file *fp;
	register int n;

	/*
	 * Allocate page tables for new process, waiting
	 * for memory to be free.
	 */
#ifdef mips
	XPRINTF(XPR_VM, "enter procdup pid = %d", p->p_pid, 0, 0, 0);
#endif mips
#ifdef vax
	while (vgetpt(p, vmemall) == 0) {
#endif vax
#ifdef mips
        if (isvfork == 0) {
		while (vgetpt(p, vmemall, vmemfree) == 0) {
#endif mips
		kmapwnt++;
#ifdef mips
		XPRINTF(XPR_VM, "procdup sleeping vgetpt failed pid = %d",
			p->p_pid, 0, 0, 0);
#endif mips
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
	XPRINTF(XPR_VM, "procdup calling vgetu pid = %d", p->p_pid, 0, 0, 0);
        save();
#endif mips
	(void) vgetu(p, vmemall, Forkmap, &forkutl, &u);

	/*
	 * Arrange for a non-local goto when the new process
	 * is started, to resume here, returning nonzero from savectx.
	 */
	forkutl.u_pcb.pcb_sswap = (int *)&u.u_ssave;
#ifdef vax
	if (savectx(&forkutl.u_ssave))
#endif vax
#ifdef mips
	XPRINTF(XPR_VM, "procdup calling setjmp pid = %d", p->p_pid, 0, 0, 0);
		/*
         * TODO: can this setjmp() be avoided? perhaps only update PC.
         */
        if (setjmp(&forkutl.u_ssave))
#endif mips
		/*
		 * Return 1 in child.
		 */
		return (1);

	/* 001
	 * Increase reference counts on shared objects
	 * and remove INUSE reference to inode.
	 */
	for (n = 0; n <= u.u_omax; n++) {
		fp = u.u_ofile[n];
		if (fp == NULL)
			continue;
		fp->f_count++;
		forkutl.u_pofile[n] &= ~(UF_INUSE);
	}

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
	forkutl.u_dmap = u.u_cdmap;
	forkutl.u_smap = u.u_csmap;
	forkutl.u_outime = 0;

	/*
	 * Attach to the text segment.
	 */
	if (p->p_textp) {
		p->p_textp->x_count++;
#ifdef GFSDEBUG
		if(p->p_textp->x_freef || p->p_textp->x_freeb) {
			cprintf("procdup: pid %d xp 0x%x on free list\n",
			p->p_pid, p->p_textp);
			panic("procdup");
		}
#endif
#ifdef mips
                while (xlink(p) == 0) {
                        kmapwnt++;
#ifdef mips
		XPRINTF(XPR_VM, "procdup sleeping xlink failed pid = %d",
			p->p_pid, 0, 0, 0);
#endif mips
                        sleep((caddr_t)kernelmap, PSWP+4);
                }
#endif mips
#ifdef vax
		xlink(p);
#endif vax
	}

#ifdef mips
	XPRINTF(XPR_VM, "procdup calling clear_tlbmappings pid = %d", p->p_pid, 0, 0, 0);
        clear_tlbmappings(p);
#endif mips

	/* attach all shared memory segments	SHMEM	*/
	if(p->p_smbeg != 0) {
		smfork(u.u_procp, p);
	}

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

/*
 *	this is where all the work for fork is done.
 *	we take out the loops for i=0,1 and put the code inline
 *	we also use only registers in the while loop
 *	As far as this goes, we only run with CLSIZE==2 anyways!!!!
 */
vmdup(p, pte, v, count, type)
#if CLSIZE==2
	register struct proc *p;
#else
	struct proc *p;
#endif
	register struct pte *pte;
	register unsigned v;
	register size_t count;
	int type;
{
	register struct pte *opte = vtopte(u.u_procp, v);
	register struct cmap *c;
#if CLSIZE!=2
	register int i;
#endif

#ifdef mips
	XPRINTF(XPR_VM,"enter vmdup",0,0,0,0);
#endif mips
	while (count != 0) {
		count -= CLSIZE;
		if (opte->pg_fod) {
			v += CLSIZE;
#if CLSIZE==2
			*(int *)pte++ = *(int *)opte++;
			*(int *)pte++ = *(int *)opte++;
#else
			for (i = 0; i < CLSIZE; i++)
				*(int *)pte++ = *(int *)opte++;
#endif
			continue;
		}
#ifdef vax
		opte += CLSIZE;
#endif vax
		(void) vmemall(pte, CLSIZE, p, type);
		p->p_rssize += CLSIZE;
#if CLSIZE==2
		copyseg((caddr_t)ctob(v), pte->pg_pfnum);
		*(int *)(pte) |= (PG_V|PG_M) + PG_UW;
		c = &cmap[pgtocm(pte->pg_pfnum)]; /* grab before bumping pte */
		v++;pte++;
		copyseg((caddr_t)ctob(v), pte->pg_pfnum);
		*(int *)(pte) |= (PG_V|PG_M) + PG_UW;
		v++;pte++;
		MUNLOCK(c); /* unlock page */
#else
		for (i = 0; i < CLSIZE; i++) {
			copyseg((caddr_t)ctob(v+i), (pte+i)->pg_pfnum);
			*(int *)(pte+i) |= (PG_V|PG_M) + PG_UW;
		}
		v += CLSIZE;
		c = &cmap[pgtocm(pte->pg_pfnum)];
		MUNLOCK(c);
		pte += CLSIZE;
#endif
#ifdef mips
		opte += CLSIZE;
#endif mips
	}
	p->p_flag |= SPTECHG;
}
