#ifndef lint
static	char	*sccsid = "@(#)vm_mem.c	4.4	(ULTRIX)	11/9/90";
#endif lint

/************************************************************************
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
 ************************************************************************/
/*
 *   Modification history:
 *
 *   4-Sep-90	dlh
 *	added include of vectors.h
 *
 *   6-Jul-90 -- jmartin
 *	Rewrite vmemall so that it always sleeps when it isn't making
 *	progress.
 *
 *   3-Jul-90 -- jmartin
 *	Change vmemall to request only non-negative numbers of pages
 *	from memall.  Allow memall to be called with lk_cmap already
 *	held.  Save a fragment of the PC to which memall will return in
 *	the otherwise unused c_refcnt field.
 *
 *  19-Jun-90 -- jmartin
 *	Fixes for "panic: vrelvm rssize"
 *
 *  17 Apr 90 jaa
 *	moved clean_icache() from the loop in memall and 
 *	cleaned up if statement
 *
 *  14 Mar 90 bp
 *	repaired mhremove to be machine independent
 *
 *  23 Feb 90 	sekhar
 *	Merged Joe Martin's fix for 3.1 cld. When locking/unlocking PTEs,
 *	check for page crossing and reevaluate vtopte.
 *
 *  30 Dec 89	bp
 *	Added kernel memory allocator reserved memory logic.  The reserved
 *	memory quantity is stored in km_reserve.  It is patchable, however
 *	you should be very careful in the value you select.
 *
 *  11 Dec 89 	sekhar
 * 	Changed icache flushing for MIPS in memall. When the page is 
 *	allocated, the icache is flushed according to the following rules:
 *	    1. for a text segment always flush the icache 
 *	    2. for data and stack pages, icache is flushed  only if SXCTDAT
 *	       bit is set in proc structure.
 *	    3. for a shared memory segment, icache is flushed only if
 *	       the bit SMXCTDAT is set in the shared memory structure.
 *
 *  11 Dec 89  	jaw
 *	add icache flush for mips SMP.
 *
 *  11 Sep 89 	bp,jmartin
 *	fixed swap hashing bug introduced by dynamic swap
 *
 *  14 Jun 89 -- jaa
 *	made all interfaces consistent across architectures
 *
 *  12-Jun-89    bp
 *	Moved kernel memory allocator to vm_kmalloc.c
 *
 *  6 Mar 89 -- jmartin
 *	Set c_intrans inside memall().  Preserve pg_prot field of pte.
 *
 * 09-Feb-89 -- jaw
 *	Move CPU_TBI state flag to seperate longword.  This was done
 *	because the state flag field can only be written by the cpu
 *	who owns it.
 *
 *  6 Dec 88 -- jmartin
 *	When accessing a text PTE of another process, synchronize with
 *	the function ptexpand(), which moves process page tables.
 *
 * 31 Aug 88 -- jmartin
 *	Use the SMP lock lk_text to protect text chain (x_link) searches
 *	calls to distpte.  Don't miss wakeups on freemem.
 *
 * 25 Jul 88 -- jmartin
 *	Use the SMP locks lk_cmap and lk_p_vm.
 *
 * 7-Jun-88  -- jaa
 *	Fixed get_sys_ptes() to round up to page boundary.
 *
 * 27-Apr-88 -- jaa
 *	Linted file, removed km_debug printf's in km_alloc
 *	added range checking in km_alloc/km_free
 *	corrected error leg in km_alloc that didn't release map resources
 *	km_alloc now sleeps on kmemmap if no map resources available
 *	and km_free wakes up anybody sleeping on kmemmap 
 *
 * 03 Feb 88 -- jmartin
 *	Introduce the SMP lock lk_buckets, which controls the free
 *	memory chains of KMALLOC/kmalloc, the memory allocator usage
 *	structure and the related statistics.
 *
 * 02 Feb 88 -- jaa
 *	Moved M_requests[] into KMEMSTATS and made it a circular list
 *	panic string corrections
 *
 * 14 Dec 87 -- jaa
 *	Integrated new km_alloc/km_free code
 *
 * 04 Sep 87 -- depp
 *      A number of changes, all involved with removing the xflush_free_text()
 *      algorithm, and replacing it with an array (x_hcmap) to hold the
 *      indexes of remote cmap entries that are hashed.  Maunhash() was
 *      added to be rid of those silly "psuedo-munhash" code fragments.
 *      Also, mhash(), munhash(), maunhash(), and mpurge() now call macros
 *      to manipulate the remote hash array  (x_hcmap) in the text struct.
 *
 * 09 Jul 87 -- depp
 *	Removed conditionals around the collection of kernel memory stats.
 *	They will now be collected, and may be reported via vmstat -K.
 *
 * 12 Jan 86 -- depp
 *	Added changes to 2 routines, memfree() and vcleanu().  Memfree()
 *	will now check to see if the "u" pages list (see 11 Mar 86 comment
 *	below) is getting too long, if so, the list is flushed before new
 *	pages are added to it.	Vclearu() now does a wakeup() if memory 
 *	has been low. 
 *
 * 15 Dec 86 -- depp
 *	Changed kmemall() so that if the resource map is exhaused and
 *	KM_SLEEP set, then the process will sleep (and cycle) on lbolt.
 *	This means that kmemall() is guaranteed to return successfully
 *	if KM_SLEEP is set.
 *
 * 11 Sep 86 -- koehler
 *	gnode name change and more informative printf
 *
 * 27 Aug 86 -- depp
 *	Moved common code in kmemall and memall into a new routine pfclear
 *
 * 29 Apr 86 -- depp
 *	converted to locking macros from calls routines.  Plus added 
 *	KM_CONTIG option to kmemall() {see that routine for more information}.
 *
 *	mlock/munlock/mwait have been converted to macros MLOCK/MUNLOCK/MWAIT
 *	and are now defined in /sys/h/vmmac.h.
 *
 *
 * 11 Mar 86 -- depp
 *	Fixed stale kernel stack problem by having "vrelu" and "vrelpt"
 *	indicate to [v]memfree to place "u" pages on a temporary list, 
 *	to be cleared by a new routine "vcleanu" (called by pagein).
 *
 * 24 Feb 86 -- depp
 *	Added 6 new routines to this file:
 *		pfalloc/pffree		physical page allocator/deallocator
 *		kmemall/kmemfree	System virtual cluster alloc/dealloc
 *		km_alloc/km_free	System virtual block alloc/dealloc
 *	Also, to insure proper sequencing of memory requests, "vmemall" now
 *	raises the IPL whenever "freemem" is referenced.
 *
 * 13 Nov 85 -- depp
 *	Added "cm" parameter to distsmpte call.  This parameter indicates that
 *	the "pg_m" bit is to be cleared in the processes PTEs that are sharing
 *	a data segment.  This replaces the "pg_cm" definition of "pg_alloc"
 *	which could cause a conflict.
 *
 * 11 Nov 85 -- depp
 *	Removed all conditional compiles for System V IPC.
 *
 * 001 - March 11 1985 - Larry Cohen
 *     disable mapped in files so NOFILE can be larger than 32
 *
 *
 * 11 Mar 85 -- depp
 *	Added in System V shared memory support.
 *
 */

#include "../machine/pte.h"
#include "../h/types.h"
#include "../machine/cpu.h"

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/cmap.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/proc.h"
#include "../h/text.h"
#include "../h/vm.h"
#include "../h/file.h"
#include "../h/gnode.h"
#include "../h/buf.h"
#include "../h/mount.h"
#include "../h/trace.h"
#include "../h/map.h"
#include "../h/kernel.h"
#include "../h/ipc.h"
#include "../h/shm.h"
#include "../h/kmalloc.h"
#ifdef vax
#include "../machine/mtpr.h"
#endif vax
#ifdef vax
#include "../machine/cpu.h"
#include "../../machine/common/cpuconf.h"
#include "../machine/vectors.h"
#endif
#include "../machine/psl.h"
#ifdef SMP_DEBUG
#include "../h/cpudata.h"
#endif SMP_DEBUG
extern struct smem smem[];

/*
 * Memory which is softly reserved for the kernel memory allocator.
 */

int km_reserve = 32*1024/NBPG;

/*
 * Allocate memory, and always succeed
 * by jolting page-out daemon
 * so as to obtain page frames.
 * To be used in conjunction with vmemfree().
 */
vmemall(pte, size, p, type, nop, nop1)
	register struct pte *pte;
	register int size;
	struct proc *p;
	int nop, nop1;       /* dummy args to be consistent with memall */
{
	register int alloc, avail;
	int s;

	XPRINTF(XPR_VM,"enter vmemall",0,0,0,0);
	if (size <= 0 || size > maxmem)
		panic("vmemall size");

	do {
		s = splimp();
		smp_lock(&lk_cmap, LK_RETRY);
		avail = (freemem < km_reserve) ?
			0 : (freemem - km_reserve)/CLSIZE*CLSIZE;
		if (avail && (alloc = memall(pte, imin(avail, size), p, type,
					     NULL, V_NOOP))) {
			smp_unlock(&lk_cmap);
			size -= alloc;
			pte += alloc;
		} else {
			outofmem();
			sleep_unlock((caddr_t)&freemem, PSWP+2, &lk_cmap);
		}
		(void) splx(s);
	} while (size > 0);
	
	if (avail-alloc < desfree)
		outofmem();
	/*
	 * Always succeeds, but return success for
	 * vgetu and vgetpt (e.g.) which call either
	 * memall or vmemall depending on context.
	 */
	return (1);
}

/*
 * Free valid and reclaimable page frames belonging to the
 * count pages starting at pte.  If a page is valid
 * or reclaimable and locked (but not a system page), then
 * we simply mark the page as c_gone and let the pageout
 * daemon free the page when it is through with it.
 * If a page is reclaimable, and already in the free list, then
 * we mark the page as c_gone, and (of course) don't free it.
 *
 * Determines the largest contiguous cluster of
 * valid pages and frees them in one call to memfree.
 */
vmemfree(pte, count)
	register struct pte *pte;
	register int count;
{
	register struct cmap *c;
	register struct pte *spte;
	register int j;
	int size, pcnt;
	register int flg = KMF_DETACH;
	int s;

	XPRINTF(XPR_VM,"enter vmemfree",0,0,0,0);

	/* Are we deallocating "u" pages or it's PTEs ? */
	if (count < 0) {
		flg = KMF_UAREA;
		count = -count;
	}

	if (count % CLSIZE)
		panic("vmemfree");

	s = splimp();
	for (pcnt = 0; count > 0; pte += CLSIZE, count -= CLSIZE) {
		smp_lock(&lk_cmap, LK_RETRY);
		for (size = 0; count > 0; pte += CLSIZE, count -= CLSIZE)
			if (pte->pg_fod == 0 && pte->pg_pfnum) {
				c = &cmap[pgtocm(pte->pg_pfnum)];
				if (c->c_lock && c->c_type != CSYS) {
					if (c->c_intrans) 
						panic("vmemfree: intrans");
					for (j = 0; j < CLSIZE; j++)
						*(int *)(pte+j) &= PG_PROT;
					c->c_gone = 1;
					pcnt += CLSIZE;			
					break;
				}
				if (c->c_free) {
					for (j = 0; j < CLSIZE; j++)
						*(int *)(pte+j) &= PG_PROT;
					if (c->c_type == CTEXT) {
						smp_lock(&lk_text, LK_RETRY);
						distpte(&text[c->c_ndx],
							(int)c->c_page, pte);
						smp_unlock(&lk_text);
					} else if (c->c_type == CSMEM) {
						/* SHMEM */
						smp_lock(&lk_smem, LK_RETRY);
						distsmpte(&smem[c->c_ndx],
							  (int)c->c_page, pte,
							  PG_NOCLRM);
						smp_unlock(&lk_smem);
					}
					c->c_gone = 1;
					break;
				}
				pcnt += CLSIZE;			
				if (size == 0)
					spte = pte;
				size += CLSIZE;
			} else if (pte->pg_fod) {
				for (j = 0; j < CLSIZE; j++)
					*(int *)(pte+j) &= PG_PROT;
				break;
			} else
				break;
		if (size)
			memfree(spte, size, flg);
		smp_unlock(&lk_cmap);
	}
	(void) splx(s);
	return (pcnt);
}

/*
 * Unlink a page frame from the free list -
 *
 * Performed if the page being reclaimed
 * is in the free list.
 */

/* Caller takes lk_cmap lock. */

munlink(pf)
	unsigned pf;
{
	register int next, prev;

	XPRINTF(XPR_VM,"enter munlink",0,0,0,0);
	next = cmap[pgtocm(pf)].c_next;
	prev = cmap[pgtocm(pf)].c_prev;
	cmap[prev].c_next = next;
	cmap[next].c_prev = prev;
	cmap[pgtocm(pf)].c_free = 0;
	if ((freemem - km_reserve) < minfree)
		outofmem();
	freemem -= CLSIZE;
}

/*
 *****************************************************************************
 *****************************************************************************
 *
 * Function:
 *
 *	pfclear -- clears CMAP entry (on the free list) of encumberances
 *		   so that it may be reallocated.
 *
 * Function description:
 *
 *	This function is used by memall() to provide a common mechanism 
 *	to clear CMAP entries prior to reallocation. 
 *
 * Interface:
 *
 *	PFCLEAR(c);
 *	  struct cmap *c;	 CMAP entry to be cleared
 *
 * Return Value:
 *
 *	None
 *
 * Error Handling:
 *
 *	Panics only
 *
 * Panics:
 *
 *
 *	"pfclear: ecmap"
 *		The cmap entry should be on a hash list, but isn't.
 *
 *	"pfclear: mfind"
 *		Mfind indicates (by non-0 return) that this cmap entry has
 *		not been unhashed.
 *	
 *****************************************************************************
 *****************************************************************************
 */

pfclear(c) 
register struct cmap *c;
{
	register int index = c->c_ndx;
	register struct proc *rp;
	struct pte *rpte;
#ifdef vax
	int s;
#endif vax

	XPRINTF(XPR_VM,"enter pfclear",0,0,0,0);
#ifdef SMP_DEBUG
	if (smp_debug)
		lsert(&lk_cmap, "pfclear");
#endif SMP_DEBUG
	/* If reclaimable, then clear associated PTEs */
	if (c->c_gone == 0 && c->c_type != CSYS) {
		if (c->c_type == CSMEM) /* SHMEM */
			smp_lock(&lk_smem, LK_RETRY);
		smp_lock(&lk_text, LK_RETRY);
		rp = (c->c_type == CTEXT) ? text[index].x_caddr : &proc[index];
		if(c->c_type != CSMEM) {
			smp_lock(&lk_p_vm, LK_RETRY);
			while (rp->p_vm & SNOVM)
				rp = rp->p_xlink;
		}
		switch (c->c_type) {
		case CTEXT:
			rpte = tptopte(rp, c->c_page);
			break;
		case CDATA:
			rpte = dptopte(rp, c->c_page);
			break;
		case CSMEM: /* SHMEM */
			rpte = smem[index].sm_ptaddr +
						c->c_page;
			break;
		case CSTACK:
			rpte = sptopte(rp, c->c_page);
			break;
		}
#ifdef vax
		s = splclock();
		smp_lock(&lk_rq,LK_RETRY);
		/* synchronize with ptexpand() */
		if (CURRENT_CPUDATA->cpu_tbi_flag){
			mtpr(TBIA,0);
			CURRENT_CPUDATA->cpu_tbi_flag=0;

		}
		smp_unlock(&lk_rq);
		splx(s);
#endif vax
		zapcl(rpte, pg_pfnum) = 0;
		if (c->c_type != CSMEM)
			smp_unlock(&lk_p_vm);
		if (c->c_type == CTEXT)
			distpte(&text[index], (int)c->c_page, rpte);
		else if (c->c_type == CSMEM) /* SHMEM */
			distsmpte(&smem[index], (int)c->c_page, rpte, 
				  PG_NOCLRM);
		smp_unlock(&lk_text);
		if (c->c_type == CSMEM) /* SHMEM */
			smp_unlock(&lk_smem);
	}

	/* If on CMAP hash lists; then remove */
	if (c->c_blkno)
	        maunhash(c);
}

#ifdef mips
int class_hits = 0, class_misses = 0, class_ends = 0, class_tries = 0;
#endif mips


/*
 * Allocate memory -
 *
 * The free list appears as a doubly linked list
 * in the core map with cmap[0] serving as a header.
 */
memall(pte, size, p, type, v, flag)
	register struct pte *pte;
	int size;
	struct proc *p;
	unsigned int v;        /* used on mips only */
	int flag;              /* used on mips only */
{
	register struct cmap *c;
	register int i, j;
	register unsigned pf;
	register int next, curpos, owner;
	int s, perturb;
#ifdef mips
	unsigned mask;
        int class_match;
#endif mips
	
	if (size % CLSIZE)
		panic("memall");
	/*
	 * Perturb the freemem value when the caller is not
	 * the kernel memory allocator.  The kernel memory allocator
	 * is identified by the fact that type == -CSYS.
	 */

	if (type == -CSYS) {
		type = CSYS;
		perturb = 0;
	} else perturb = km_reserve;

	s = splimp();
	owner = smp_owner(&lk_cmap);
	if (owner == LK_FALSE)
		smp_lock(&lk_cmap, LK_RETRY);

	/* Insure that enough free memory exists to make allocation */
	if (size > (freemem - perturb)) {
		if (owner == LK_FALSE)
			smp_unlock(&lk_cmap);
		(void) splx(s);
		return (0);
	}
	trace(TR_MALL, size, u.u_procp->p_pid);

	/* Allocation loop: by page cluster */
	for (i = size; i > 0; i -= CLSIZE, pte+=CLSIZE) {

		/* Has another pagein already made this allocation? */
		if ((type==CTEXT || type==CSMEM) &&
		    pte->pg_fod==0 && pte->pg_pfnum) {
			if (owner == LK_FALSE)
				smp_unlock(&lk_cmap);
			(void) splx(s);
			return size-i;
		}
#ifdef mips
		if(flag == V_CACHE) {
			if (type == CTEXT) {
			    class_match = 
				(v + (u.u_procp->p_textp - text)) & icachemask;
			    mask = icachemask;
			} else {
			    class_match = (v + u.u_procp->p_pid) & dcachemask;
			    mask = dcachemask;
			}
			curpos = cmap[CMHEAD].c_next;
			/* nasty constant! */
			for (j = 0 ; j < 64 ; j++) {
				/* walk down list looking for good page */
				if (curpos == CMHEAD) { /* end of list */
					curpos = cmap[CMHEAD].c_next; /*fail*/
					class_ends++;
					goto out;
				}
				if ((cmtopg(curpos) & mask) == class_match){
					class_hits++;
					goto out;
				}
				curpos = cmap[curpos].c_next;
			}
			/* nasty constant! */
			if (j == 64) {
				curpos = cmap[CMHEAD].c_next; /* fail */
				class_misses++;
			}
		} else
#endif mips
		/* Retrieve next free entry from TOP of list */
		curpos = cmap[CMHEAD].c_next;
#ifdef mips
out:
#endif mips
		if ((pf=cmtopg(curpos)) > maxfree)
			panic("bad mem alloc");
		c = &cmap[curpos];
		if (c->c_free == 0)
			panic("dup mem alloc");
		if (c->c_intrans || c->c_want)
			panic("memall intrans|want");
		freemem -= CLSIZE;
		next = c->c_next;
#ifdef mips
		 if (flag == V_CACHE) { 
		 /* may have taken it not from the head */
                        cmap[cmap[curpos].c_prev].c_next = next;
                        cmap[next].c_prev = cmap[curpos].c_prev;
                } else {
#endif mips
		cmap[CMHEAD].c_next = next;
		cmap[next].c_prev = CMHEAD;
#ifdef mips
		}
#endif mips

		/*
		 * If reclaimable, then clear encumberances
		 */
		pfclear(c);

#ifdef mips
		/* flush text pages
		 * or if the proc executes data (407...)
		 * from the icache
		 */
		if (type == CTEXT || p->p_vm & SXCTDAT ||
		   (type == CSMEM && ((struct smem *)p)->sm_flag & SMXCTDAT) ){
			clean_icache(PHYS_TO_K0(ptob(pf)), CLBYTES);
			if (smp) 
				produce_icache_clears(pf);
		}
#endif mips
		for (j = 0; j < CLSIZE; j++) {
		    *(int *)(pte+j) &= PG_PROT;
		    *(int *)(pte+j) |= pf++ << PTE_PFNSHIFT;
		}

		/* 
		 * Initialize CMAP entry
		 */
		switch (type) {

		case CSYS:
			c->c_ndx = p->p_ndx;
			break;

		case CTEXT:
			c->c_ndx = p->p_textp - &text[0];
			/*
			 * Make sure that potential sharers of this page
			 * see in a timely fashion that it has been
			 * allocated.  Likewise for CSMEM below.
			 */
			smp_lock(&lk_text, LK_RETRY);
			distpte(p->p_textp,
				(c->c_page = vtotp(p, ptetov(p, pte))),
				pte);
			smp_unlock(&lk_text);
			break;

		case CDATA:
			c->c_page = vtodp(p, ptetov(p, pte));
			c->c_ndx = p->p_ndx;
			break;

		case CSMEM: /* SHMEM */
			c->c_ndx = (struct smem *)p - &smem[0];
#ifdef vax
			pte->pg_alloc = 1;
#endif vax
			smp_lock(&lk_smem, LK_RETRY);
			distsmpte((struct smem *)p, 
				  c->c_page=pte-((struct smem *)p)->sm_ptaddr,
				  pte, PG_NOCLRM);
			smp_unlock(&lk_smem);
			break;

		case CSTACK:
			c->c_page = vtosp(p, ptetov(p, pte));
			c->c_ndx = p->p_ndx;
			break;
		}

#ifdef vax
		/* c->c_refcnt = (return PC)>>4 & 0xff; */
		asm("ashl $-4,0x10(fp),-(sp); cvtlb (sp)+,0x13(r10)");
#endif vax
		c->c_free = 0;
		c->c_gone = 0;
		if (type != CSYS)
			c->c_intrans = 1;
		c->c_lock = 1;
		c->c_type = type;
	}
	if (owner == LK_FALSE)
		smp_unlock(&lk_cmap);
	(void) splx(s);
	return (size);
}

/*
 * Free memory -
 *
 * The page frames being returned are inserted
 * to the head/tail of the free list depending
 * on whether there is any possible future use of them,
 * unless "flg" indicates that the page frames should be 
 * temporily stored on the "u" list until after the
 * context switch occurs.  In this case, the cmap entries
 * are deallocated as free, but place on the list {ucmap,eucmap}
 * until "vcleanu" is called to push them onto the free list.
 *
 * If the freemem count had been zero,
 * the processes sleeping for memory
 * are awakened.
 */

memfree(pte, size, flg)
	register struct pte *pte;
	register int size;
{
	register int i, j, prev, next;
	register struct cmap *c;
#ifdef mips
	struct text *xp;
#endif mips
	void vcleanu();

	XPRINTF(XPR_VM,"enter memfree",0,0,0,0);
#ifdef SMP_DEBUG
	if (smp_debug)
		lsert(&lk_cmap, "memfree");
#endif SMP_DEBUG
	if (size % CLSIZE)
		panic("memfree");
	while (size > 0) {
		size -= CLSIZE;
		i = pte->pg_pfnum;
		if (i < firstfree || i > maxfree)
			panic("bad mem free");
		i = pgtocm(i);
		c = &cmap[i];
#ifdef mips
		if (c->c_type == CTEXT) {
			xp = &text[c->c_ndx];
			if (xp->x_flag & XTRC) {
				int dev = c->c_mdev==MSWAPX?swapdev:mount[c->c_mdev].m_dev;
				if (c->c_blkno && mfind(dev,c->c_blkno,xp->x_gptr)) {
					munhash(dev,(daddr_t)(u_long)c->c_blkno,xp->x_gptr);
				}
			}
		}
#endif mips
		if (c->c_free)
			panic("dup mem free");
		if (c->c_intrans || c->c_want)
			panic("memfree: freeing intrans|want page");
		if (flg && c->c_type != CSYS) {
			for (j = 0; j < CLSIZE; j++)
				*(int *)(pte+j) &= PG_PROT;
			c->c_gone = 1;
		}

		/*
		 * If	deallocating "u" pages, place on temp "u" list 
		 *	to be cleared by "vcleanu" routine
		 * else place either at the head or tail of freelist
		 *	depending on whether it may be reclaimed
		 */
		if (flg == KMF_UAREA) {
			/* insure that list doesn't get too long */
			if (nucmap >= UCLEAR)	/* machine/vmparam.h */
				vcleanu();
			if (nucmap == 0)
				ucmap = eucmap = i;
			else {
				cmap[eucmap].c_next = i;
				c->c_prev = eucmap;
				eucmap = i;
			}
			nucmap++;
		} else {
			if (flg == KMF_DETACH && c->c_blkno == 0) {
			next = cmap[CMHEAD].c_next;
			cmap[next].c_prev = i;
			c->c_prev = CMHEAD;
			c->c_next = next;
			cmap[CMHEAD].c_next = i;
		} else {
			prev = cmap[CMHEAD].c_prev;
			cmap[prev].c_next = i;
			c->c_next = CMHEAD;
			c->c_prev = prev;
			cmap[CMHEAD].c_prev = i;
			}
			if ((freemem - km_reserve) < CLSIZE * KLMAX)
				wakeup((caddr_t)&freemem);
			freemem += CLSIZE;
		}
		c->c_free = 1;
		pte += CLSIZE;
	}
}

/*
 *****************************************************************************
 *****************************************************************************
 *
 * Function:
 *
 *	vcleanu -- kernel routine
 *
 * Function description:
 *
 *	This function will remove all of the page frames on the "u" list
 *	to the free list.  Since the cmap entries have been properly 
 *	initialized and linked in memall, all that must be done is to
 *	move the list intact onto the top of the free list.
 *
 *	NOTE:  This routine is currently only being called in "pagein".  
 *	If additional calls become necessary, this routine must only be
 *	called if "nucmap != 0" (see "panic").
 *
 * Return Value:
 *
 *	None
 *
 * Interface:
 *
 *	void vcleanu()
 *
 * Errors:
 *
 *	None
 *
 * Panics:
 *
 *	"vcleanu"
 *		This list is empty.  This routine should not have been called,
 *		which is one indication that the list may be corrupted.
 *	
 *****************************************************************************
 *****************************************************************************
 */

void
vcleanu()
{
	register int next;

	XPRINTF(XPR_VM,"enter vcleanu",0,0,0,0);
#ifdef SMP_DEBUG
	if (smp_debug)
		lsert(&lk_cmap, "vcleanu");
#endif SMP_DEBUG
	if (ucmap == -1)
		panic("vcleanu");
	next = cmap[CMHEAD].c_next;
	cmap[next].c_prev = eucmap;
	cmap[ucmap].c_prev = CMHEAD;
	cmap[eucmap].c_next = next;
	cmap[CMHEAD].c_next = ucmap;
	if ((freemem - km_reserve) < CLSIZE * KLMAX)
		wakeup((caddr_t)&freemem);
	freemem += nucmap * CLSIZE;
	ucmap = eucmap = -1;
	nucmap = 0;
}

/*
 *****************************************************************************
 *****************************************************************************
 *
 * Function:
 *
 *	pfalloc -- kernel routine
 *
 * Function description:
 *
 *	This function will allocate page frame clusters.  These
 *	physical pages are not coupled in any manner to system page table
 *	space.  It's up to the requesting routine to allocate and map the
 *	returned PFNs into the system page table.
 *
 *	As a rule, this routine should only be used to allocate single
 *	page frame clusters (npfc == 1).  It should only be used for 
 *	multiple clusters at startup/configuration time, as the free
 *	list will get out of order, and the overhead will become unbearable.
 *
 * Interface:
 *
 *	unsigned int pfalloc (type, npfc)
 *	  int type;		type must = CSYS ( ../h/cmap.h)
 *	  int npfc;		number of page frame clusters requested
 *				(normally, should == 1)
 *
 * Return Value:
 *
 *	= 0	Error -- no memory to allocate
 *	> 0	Normal -- PFN of first physical page allocated in
 *
 *	In a normal return, the first PFN allocated is returned, the balance
 *	of the PFNs in the block are easily determined since they are 
 *	contiguous.
 *
 * Error Handling:
 *
 *	If return value = 0, then either:
 *		1. the "cmap" free list is empty.
 *		2. a block of size "npgf" contiguous page frames could not
 *		   be located.
 *
 * Panics:
 *
 *	"pfalloc: type"
 *		Type must equal CSYS.  User types are not permitted.  In the
 *		future, other system types may exist and be permitted to use
 *		this routine.
 *
 *	"pfalloc: dup mem alloc"
 *		The cmap entry was on the free list, but the c_free flag
 *		indicates that the entry is NOT free.  This indicates 
 *		corruption of the free list.
 *
 *	"pfalloc: bad mem alloc"
 *		The index into the cmap structures is too high for this 
 *		memory configuration.  This indicates that the index is
 *		corrupted, and memory can not be reliably allocated.
 *
 *	"pfalloc: intrans|want"
 *		At this point, the physical cluster should be free of 
 *		encumbrances, but the cmap entry indicates that the
 *		physical cluster is intransient or wanted by an user proc.
 *
 *	
 *****************************************************************************
 *****************************************************************************
 */

unsigned
pfalloc (type, npfc)
int type;	/* cluster type */
int npfc;	/* number of page frame clusters */
{
	register struct cmap *c;
	register int next, curpos, count;
	int end, head;
	unsigned pf;
	int s;

	XPRINTF(XPR_VM,"enter pfalloc",0,0,0,0);
	if (type != CSYS)
		panic("pfalloc: type");
	s = splimp();
	smp_lock(&lk_cmap, LK_RETRY);
	if (freemem < npfc * CLSIZE) {
		smp_unlock(&lk_cmap);
		(void)splx(s);
		return(0);
	}

	/*
	 * if only one cluster requested, bypass contiguous lookup
	 * else look for contiguous page frames to pass back
	 */
	if (npfc == 1) {
		head = CMHEAD;
		pf = cmtopg(cmap[CMHEAD].c_next);
	}
	else {
		next = CMHEAD;
		end = cmap[CMHEAD].c_prev;
		count = npfc;

		while ((curpos = next = cmap[next].c_next) != end) {
			while (--count && (cmap[next].c_next == next + 1))
				next++;
			if (count == 0) {
				head = cmap[curpos].c_prev;
				pf = cmtopg(curpos);
				goto allocate;
			}
			count = npfc;
		}
		smp_unlock(&lk_cmap);
		(void)splx(s);
		return(0);	/* sorry, not a large enough block */

	}
allocate:	
	while (npfc--) {
		curpos = cmap[head].c_next;
		c = &cmap[curpos];
		freemem -= CLSIZE;
		next = c->c_next;
		cmap[head].c_next = next;
		cmap[next].c_prev = head;
		if (c->c_free == 0)
			panic("pfalloc: dup mem alloc");
		if (cmtopg(curpos) > maxfree)
			panic("pfalloc: bad mem alloc");
		
		/* clear CMAP entry */
		pfclear(c);

		/* intialize CMAP entry */
		c->c_ndx = 0;
		c->c_free = 0;
		c->c_gone = 0;
		if (c->c_intrans || c->c_want)
			panic("pfalloc: intrans|want");
		c->c_type = type;
	}
#ifdef KM_STATS
	km_stats.tot_pfalloc++;
#endif KM_STATS
	smp_unlock(&lk_cmap);
	(void)splx(s);
	return (pf);
}


/*
 *****************************************************************************
 *****************************************************************************
 *
 * Function:
 *
 *	pffree -- kernel routine
 *
 * Function description:
 *
 *	This routine will free physical page clusters by placing them on the
 *	"cmap" free list.
 *
 * Interface:
 *
 *	void pffree (pfn, npfc)
 *	  unsigned int pfn		First PFN in cluster to be deallocated
 *	  int npfc			must equal 1 (for now)
 *
 * Return Value:
 *
 *	None
 *
 * Error Handling:
 *
 *	None
 *
 * Panics:
 *
 *	"pffree: bad mem free"
 *		The PFN is outside the valid range for this memory 
 *		configuration or npfc not = 1
 *
 *	"pffree: dup mem free"
 *		The cmap entry is marked as being free.  This indicates that
 *		either the free list is corrupted or that the input PFN has
 *		been corrupted.
 *	
 *****************************************************************************
 *****************************************************************************
 */

void
pffree (pfn, npfc)
unsigned pfn;
int npfc;
{
	register int next;
	register struct cmap *c;
	register int s, i;
	
	XPRINTF(XPR_VM,"enter pffree",0,0,0,0);
	if (pfn < firstfree || pfn > maxfree || npfc != 1)
		panic("pffree: bad mem free");
	i = pgtocm(pfn);
	c = &cmap[i];
	if (c->c_free)
		panic("pffree: dup mem free");
	s = splimp();
	smp_lock(&lk_cmap, LK_RETRY);
	next = cmap[CMHEAD].c_next;
	cmap[next].c_prev = i;
	c->c_prev = CMHEAD;
	c->c_next = next;
	cmap[CMHEAD].c_next = i;
	c->c_free = 1;
	if ((freemem - km_reserve) < CLSIZE * KLMAX)
		wakeup((caddr_t)&freemem);
	freemem += CLSIZE;
#ifdef KM_STATS
	km_stats.tot_pffree++;
#endif KM_STATS
	smp_unlock(&lk_cmap);
	(void) splx(s);
}


/*
 * Enter clist block c on the hash chains.
 * It contains file system block bn from device dev.
 * Dev must either be a mounted file system or the swap device
 */

/* Called by pagein.  lk_cmap is locked there. */

mhash(c, dev, bn)
	register struct cmap *c;
	dev_t dev;
	daddr_t bn;
{
	register int i = CMHASH(bn);
	register struct mount *mp;
	register struct text *xp;
	

	XPRINTF(XPR_VM,"enter mhash",0,0,0,0);
	c->c_hlink = cmhash[i];
	cmhash[i] = c - cmap;
	c->c_blkno = bn;
	xp = &text[c->c_ndx];
	GETMP(mp, dev);	
	if(mp == NULL)
		panic("mhash: no mp");
	c->c_mdev = (mp == (struct mount *) MSWAPX) ? MSWAPX : mp - mount;
	if (X_DO_RHASH(xp))
	        G_SET_HCMAP(xp, xp->x_gptr, c, c - cmap);
}

/*
 * Pull the clist entry of <dev,bn> off the hash chains.
 * We have checked before calling (using mfind) that the
 * entry really needs to be unhashed, so panic if we can't
 * find it (can't happen).
 *
 * N.B. if dev == swapdev, gp is NULL since the block on the swap
 * device may not be associated with any active text or data segment
 */
munhash(dev, bn, gp)
	register dev_t dev;
	register daddr_t bn;
	register struct gnode *gp;
{
	register struct cmap *c1, *c2;
	register int index;
	register struct gnode *gpproto = NULL;
	struct text *xp;
	int i = CMHASH(bn);
	struct mount *mp;
	int needgp = 1;
	int si = splimp();
	
	XPRINTF(XPR_VM,"enter munhash",0,0,0,0);
	c1 = &cmap[cmhash[i]];
	if (c1 == ecmap)
		panic("munhash");
	
	if (gp && gp->g_dev == dev) {	/* handle simple case to save */
		index = gp->g_mp - mount;
		mp = gp->g_mp;
	} else {
		GETMP(mp, dev)
		index = (mp == (struct mount *) MSWAPX) ? MSWAPX : mp - mount;
	}
	
	/*
	 * it is sufficient for the local case to use just dev and bn
	 * since locally we use lbn's rather than vbn's
	 */
	
	if((index == MSWAPX) || (mp->m_flags & M_LOCAL)) {
		gpproto = gp;	/* so matches work out */
		needgp = 0;	/* local doesn't need gp */
	} else  {
	        if (c1->c_type != CTEXT)
		        panic("munhash: unhashing non text page");
		gpproto = text[c1->c_ndx].x_gptr;
	}

	if ((c1->c_blkno) == bn && (index == c1->c_mdev) && (gp == gpproto)) {
		cmhash[i] = c1->c_hlink;
	} else {
		for (;;) {
			c2 = c1;
			c1 = &cmap[c2->c_hlink];
			if (c1 == ecmap)
				panic("munhash: ecmap");
			if(needgp) {
			        if (c1->c_type != CTEXT)
				        panic("muhash: unhashing non text page 2");
				gpproto = text[c1->c_ndx].x_gptr;
			}
				
			if ((c1->c_blkno == bn) && (index == c1->c_mdev) &&
				(gp == gpproto)) {
			  break;
			}
		}
		c2->c_hlink = c1->c_hlink;
	}
	if (mfind(dev, bn, gp)) 
		panic("munhash mfind");
        xp = &text[c1->c_ndx];
	if (X_DO_RHASH(xp))
                G_RST_HCMAP(xp,gp,c1);
	c1->c_mdev = (u_char) NODEV;
	c1->c_blkno = 0;
	c1->c_hlink = 0;
	splx(si);
}


/*
 * maunhash -- This routine is the same as munhash(), except that the
 *             match is on the CMAP address not the mdev/gnode
 *  
 *             This gets rid of those silly "pseudo-munhashes" that have
 *             plagued us.
 */
maunhash(c)
     register struct cmap *c;
{
	register struct cmap *c1, *c2;
	register int j;
	register struct gnode *gp;
	register struct text *xp;

	XPRINTF(XPR_VM,"enter maunhash",0,0,0,0);
        j = CMHASH(c->c_blkno);
	c1 = &cmap[cmhash[j]];
	if (c1 == c)
		cmhash[j] = c1->c_hlink;
	else {
		for (;;) {
			if (c1 == ecmap)
				panic("maunhash ecmap");
			c2 = c1;
			c1 = &cmap[c2->c_hlink];
			if (c1 == c)
				break;
		}
		c2->c_hlink = c1->c_hlink;
	}

	if (c->c_type != CTEXT)
	        panic("maunhash: unhashing non text page");

	/*
	 * xp and gp below are used only for remote text.  As this
	 * function is called only when (c->c_blkno != 0), X_SET(xp) has
	 * been executed, but X_CLEAR(xp) has not.  In the worst case,
	 * this function (which is executed holding lk_cmap) is stalling
	 * an X_FLUSH(xp) operation which precedes X_CLEAR(xp).
	 * Therefore, c->c_ndx points to the correct remote text
	 * segment.
	 *
	 * In the case of memory hashed from local text, the value of gp
	 * is not used by mfind. (See ../sys/vm_text.c, ../h/text.h,
	 * ../h/gnode.h)
	 */
	xp = &text[c->c_ndx];
	gp = xp->x_gptr;

	/* do an mfind() to insure that duplicates aren't found */
	if (mfind(c->c_mdev==MSWAPX?swapdev:mount[c->c_mdev].m_dev,
	      (daddr_t)c->c_blkno, gp)) {
		printf(" maunhash: mdev 0x%x blkno %d x_gptr 0x%x\n",
		  c->c_mdev== MSWAPX?swapdev:mount[c->c_mdev].m_dev,
		  c->c_blkno, gp);
		panic("maunhash: mfind");
	}

	/* if remote and large, remove from hash array */
	if (X_DO_RHASH(xp))
                G_RST_HCMAP(xp,gp,c);

	/* finish unhashing the given CMAP entry */
	c->c_mdev = (u_char) NODEV;
	c->c_blkno = 0;
	c->c_hlink = 0;
}

/*
 * Look for block bn of device dev in the free pool.
 * Currently it should not be possible to find it unless it is
 * c_free and c_gone, although this may later not be true.
 * (This is because active texts are locked against file system
 * writes by the system.)
 *
 * N.B. if dev == swapdev, gp is NULL since the block on the swap
 * device may not be associated with any active text or data segment
 */

/* Assumes that lk_cmap is taken/released by the caller */

struct cmap *
mfind(dev, bn, gp)
	register dev_t dev;
	register daddr_t bn;
	register struct gnode *gp;
{
	register struct cmap *c1 = &cmap[cmhash[CMHASH(bn)]];
	register int index;
	register struct gnode *gpproto = NULL;
	struct mount *mp;
	int needgp = 1;
	
	XPRINTF(XPR_VM,"enter mfind",0,0,0,0);
	if (gp && gp->g_dev == dev) {	/* handle simple case to save */
		if ((mp = gp->g_mp) == NULL)
			return((struct cmap *) 0);
		index = gp->g_mp - mount;
	}
	else {
		GETMP(mp, dev)
		if (mp == NULL)
			return((struct cmap *) 0);
		index = (mp == (struct mount *) MSWAPX) ? MSWAPX : mp - mount;
	}
	
	/*
	 * it is sufficient for the local case to use just dev and bn
	 * since locally we use lbn's rather than vbn's
	 * this includes the swap device
	 */
	if((index == MSWAPX) || (mp->m_flags & M_LOCAL)) {
		gpproto = gp;	/* so matches work out */
		needgp = 0;	/* local doesn't need the gp */
	}
	/* now search the core map */
	while (c1 != ecmap) {
		if(needgp) {
		        if (c1->c_type != CTEXT)
			        panic("mfind: trying to find non text on hash");
		        gpproto = text[c1->c_ndx].x_gptr;
		}

		if((c1->c_blkno == bn) && (c1->c_mdev == index) &&
		(gp == gpproto)) {
			return (c1);
		}
		c1 = &cmap[c1->c_hlink];
	}
	return ((struct cmap *)0);
}

/*
 * Remove hashed swapped blocks 
 */

mhremove(pte,bn,n,lock)
	register struct pte *pte;
	register daddr_t bn;
	register int n;
	int lock;
{
	register struct cmap *c, *c1;
	register int s;

	if (lock == LK_FALSE) {
		s = splimp();
		smp_lock(&lk_cmap, LK_RETRY);
	}

	for (;n--; pte += CLSIZE, bn += CLBYTES/DEV_BSIZE) {
		c = &cmap[pgtocm(pte->pg_pfnum)];
		if (c->c_mdev != MSWAPX &&
			(c1 = mfind(swapdev,bn,(struct gnode *) 0)))
				maunhash(c1);
	}

	if (lock == LK_FALSE) {
		smp_unlock(&lk_cmap);
		(void) splx(s);
	}
}

/*
 * Purge blocks from device dev from incore cache
 * before umount().
 */
mpurge(mdev)
	int mdev;
{
	register struct cmap *c1, *c2;
	register int i;
	register struct text *xp;
	int si = splimp();
	smp_lock(&lk_cmap, LK_RETRY);

	XPRINTF(XPR_VM,"enter mpurge",0,0,0,0);
	for (i = 0; i < CMHSIZ; i++) {
more:
		c1 = &cmap[cmhash[i]];
		if (c1 == ecmap)
			continue;
		if (c1->c_mdev == mdev)
			cmhash[i] = c1->c_hlink;
		else {
			for (;;) {
				c2 = c1;
				c1 = &cmap[c1->c_hlink];
				if (c1 == ecmap)
					goto cont;
				if (c1->c_mdev == mdev)
					break;
			}
			c2->c_hlink = c1->c_hlink;
		}
		xp = &text[c1->c_ndx];
		if (X_DO_RHASH(xp))
		        G_RST_HCMAP(xp,xp->x_gptr,c1);
		c1->c_mdev = (u_char) NODEV;
		c1->c_blkno = 0;
		c1->c_hlink = 0;
		goto more;
cont:
		;
	}
	smp_unlock(&lk_cmap);
	(void) splx(si);
}

/*
 * Initialize core map
 */
meminit(first, last)
	int first, last;
{
	register int i;
	register struct cmap *c;

	XPRINTF(XPR_VM,"enter meminit",0,0,0,0);
	firstfree = clrnd(first);
	maxfree = clrnd(last - (CLSIZE - 1));
	freemem = maxfree - firstfree;
	ecmx = ecmap - cmap;
#ifdef mips
	if (ecmx < freemem / CLSIZE + 1)
		freemem = (ecmx-1) * CLSIZE;
#endif mips
#ifdef vax
	if (ecmx < freemem / CLSIZE)
		freemem = ecmx * CLSIZE;
#endif vax
	for (i = 1; i <= freemem / CLSIZE; i++) {
		cmap[i-1].c_next = i;
		c = &cmap[i];
		c->c_prev = i-1;
		c->c_free = 1;
		c->c_gone = 1;
		c->c_type = CSYS;
		c->c_mdev = (u_char) NODEV;
		c->c_blkno = 0;
	}
	cmap[freemem / CLSIZE].c_next = CMHEAD;
	for (i = 0; i < CMHSIZ; i++)
		cmhash[i] = ecmx;
	cmap[CMHEAD].c_prev = freemem / CLSIZE;
	cmap[CMHEAD].c_type = CSYS;
	avefree = freemem;
	hand = 0;
	ucmap = eucmap = -1;
	nucmap = 0;
#ifdef mips
	flush_cache();
#endif mips
	lockinit(&lk_cmap_bio, &lock_cmap_bio_d);
	lockinit(&lk_p_vm, &lock_p_vm_d);
	lockinit(&lk_smem, &lock_smem_d);
	lockinit(&lk_cmap, &lock_cmap_d);
}

/* 
 * Lock a virtual segment.
 *
 * For each cluster of pages, if the cluster is not valid,
 * touch it to fault it in, otherwise just lock page frame.
 * Called from physio to ensure that the pages 
 * participating in raw i/o are valid and locked.
 */
vslock(base, count)
	caddr_t base;
{
	register unsigned v;
	register int npf;
	register struct pte *pte = 0;
	register struct cmap *c;
	register struct proc *p = u.u_procp;
	int s;

	XPRINTF(XPR_VM,"enter vslock",0,0,0,0);
	v = btop(base);
	npf = btoc(count + ((int)base & CLOFSET));
	s = splimp();
	smp_lock(&lk_cmap, LK_RETRY);
	while (npf > 0) {
		if (((int)pte & PGOFSET) < CLSIZE*sizeof(struct pte)
		    || (pte->pg_pfnum==0 && pte->pg_v==0))
			pte = vtopte(p, v); /* handles crossing segments */
		if (pte->pg_v) {
			c = &cmap[pgtocm(pte->pg_pfnum)];
				MLOCK(c);
		} else {
			smp_unlock(&lk_cmap);
			(void) splx(s);
			pagein(ctob(v), 1);	/* return it locked */
			s = splimp();
			smp_lock(&lk_cmap, LK_RETRY);
		}
#ifdef mips
#ifdef CACHETRICKS
		if (pte->pg_n == 0) {
			register unsigned pf;

			pf = pte->pg_pfnum;
			c = &cmap[pgtocm(pf)];
			c->c_icachecnt = icachecnt[pf & icachemask];
			c->c_dcachecnt = dcachecnt[pf & dcachemask];
		}
#endif CACHETRICKS
#endif mips
		pte += CLSIZE;
		v += CLSIZE;
		npf -= CLSIZE;
	}
	smp_unlock(&lk_cmap);
	(void) splx(s);
}

/* 
 * Unlock a virtual segment.
 */
vsunlock(base, count, rw)
	caddr_t base;
{
	register struct pte *pte = 0;
	register int npf;
	register struct cmap *c;
	int s;
	register struct proc *p = u.u_procp;
	register unsigned v = btop(base);

	XPRINTF(XPR_VM,"enter vsunlock",0,0,0,0);
	npf = btoc(count + ((int)base & CLOFSET));
	s = splimp();
	smp_lock(&lk_cmap, LK_RETRY);
	while (npf > 0) {
		if (((int)pte & PGOFSET) < CLSIZE*sizeof(struct pte)
		    || (pte->pg_pfnum==0 && pte->pg_v==0))
			pte = vtopte(p, v); /* handles crossing segments */
		if (pte->pg_v == 0)
			panic("vsunlock: invalid PTE");
		c = &cmap[pgtocm(pte->pg_pfnum)];
		MUNLOCK(c);
		if (rw == B_READ)	/* Reading from device writes memory */
			*(int *)pte |= PG_M; /* Longword write on a VAX. */
		pte += CLSIZE;
		v += CLSIZE;
		npf -= CLSIZE;
	}
	smp_unlock(&lk_cmap);
	(void) splx(s);
}
