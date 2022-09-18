#ifndef lint
static char *sccsid = "@(#)vm_page.c	4.7	ULTRIX	4/4/91";
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
 *
 *   Modification history:
 *
 *  11-feb-91	jaw
 *	fix to mp locking in dpageout.
 *
 *  16-Oct-90	sekhar
 *	fixed kluster() to pass correct va to memall for correct
 *	page coloring.
 *	
 *  4-Sep-90	dlh
 *	added vector processor support code
 *
 *  17-Jul-90 -- jaw
 *	fix for slave hold hang.
 *
 *  3-Jul-90 -- jmartin
 *	Hold lk_cmap during the call to memall in pagein; amend previous
 *	fix.
 *
 * 20-Jun-90 -- jmartin
 *	Fix unitialized klback in fodkluster.
 *
 * 19-Jun-90 -- jmartin
 *	Fixes for "panic: vrelvm rssize"
 *
 *  30 Dec 89 bp
 *	Changed restart logic in pagein to lower spl before going to restart
 *	point.  Added km_reserve check in pagein when checking for memall page
 *	allocation.
 *
 *  28 Dec 89 gmm
 *	Change declartion of cpudata pointer to volatile to work in MIPS/MP
 *	systems - in pageout()
 *
 *  11 Dec 89 jaa
 *	change dynamic swap to account for swap up front (ala v3.1) 
 * 	but actually do the allocation only when pushing the page/process
 *
 * 04 Dec 89 	sekhar
 *	minor change to checkpage to track swap useage.
 *
 * 30-Nov-89 -- jmartin
 *	Support for coprocessor access to process virtual memory.
 *
 * 11 Sep 89 	bp,jmartin
 *	fixed swap hashing bug introduced by dynamic swap
 *
 * 24-jun-89    gg
 *	Fixed Missing smp_unlock in error return case.
 *
 * 12-Jun-89	bp
 *	Added kernel memory allocator high water mark trimming to
 *	the routine pageout.  This event occurs when freemem is 
 *	less than lotsfree.
 *
 * 12 Jun 89 -- gg
 *	Dynamic swap changes:
 *	---------------------
 *	In checkpage() and dpageout() are modified to allocate swap space
 *	(if not allocated already) before pushing the page to swap disk.
 *	Added a few checks in pagein() and fodkluster() before 
 *	mhash/munhash.
 *
 * 26 May 89 -- jmartin
 *	Restore call to blkflush() from pagein().  Have pageout() wait
 *	for other processors to acknowledge that they are stopped.  Fix
 *	lint-detected bug in dpageout().
 *
 * 02-May-89 -- jaw, jmartin
 *	fix forkutl to work on mips.
 *
 *  15 Apr 89 -- jaw
 *	missing smp_lock in cleanup().
 *
 *  6 Mar 89 -- jmartin
 *	Set c_intrans inside memall().  Preserve pg_prot field of pte
 *	through the call to memall.
 *
 *  26-Jan-89	jaw
 *	SMP clean up 
 *
 * 15 Dec 88 -- jmartin
 *	SMP locking for shared memory data structures (cf. h/shm.h)
 *	Integration of v3.0 shared memory changes, i.e. KM_ALLOC of
 *	per-process shared memory structures.
 *
 * 31 Aug 88 -- jmartin
 *	Protect searches of text chains and changes to x_rssize with
 *	SMP lock lk_text.
 *
 * 19 Aug 88 -- miche
 *	Don't count on slavehold go clear to release processors:
 *	do a round of inter-processor interrupts, which will be
 *	translated by the P's into clears of the cpu_state|STOP bit.
 *
 *  8 Aug 88 -- jmartin
 *	Fix problem with ZFOD shared memory.  Reorganize fodkluster.
 *
 * 25 Jul 88 -- jmartin
 *	Lock pte changes with lk_cmap; pageout buffers with lk_cmap_bio.
 *	Rewrite klustering to be smp-safe.
 *
 * 18-Jun-88 -- jaw  change to new cpu data format.
 *
 * 02 Feb 88 -- jaa
 *	Fixed checkpage so that if a proc is locked or exiting, it 
 *	is not eligible for paging
 *
 * 27 Jan 88 -- gmm
 *	Changed intrcpu() to intrpt_cpu() to conform to the new IPI interface
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
 * 04 Sep 87 -- depp
 *      Due to the demise of xflush_free_text(), checkpage no longer has
 *      to flush remote hashed pages on pageout.
 *
 * 14 Jul 87 -- cb
 *	Added in rr's changes.
 *
 * 20 Feb 87 -- depp
 *	Added check for GTRC flag in gnode.  If set, the pagein() routine
 * 	will not hash the page or look for the page in the hash lists.
 *
 * 21 Jan 87 -- jaw
 *	performance fixes to syscall.
 *
 * 15 Jan 87 -- depp
 *	Fixed SM bug in pagein().
 *
 * 15 Dec 86 -- depp
 *	Fixed problem with PG_M not properly propagating to all attached
 *	text PTEs (pagein()).
 *
 * 09 Oct 86 -- depp
 *	Changed checkpage() to remove from the hash lists any pages that
 *	are from a remote file on pageout.  Also, fixed problem in shared
 *	memory - on pagein, if a SM page is intrans, it was sleeping on an
 *	address in the proc structure, rather than a global address.
 *
 * 11 Sep 86 -- koehler
 *	a few mount ops became macros, gnode name change
 *
 * 18 Jun 86 -- depp
 *      Added shared memory ZFOD support.
 *
 * 29 Apr 86 -- depp
 *	converted to lock macros from calls routines.
 *
 * 02-Apr-86 -- jrs
 *	Add set of runrun so that halt of slaves will really take
 *	effect when we want it to
 *
 * 02 Apr 86 -- depp
 *	Added in performance enhancements from 4.3UCB.  The first is the
 *	2 hand clock algorithm for large memory configurations.  The second
 *	is klustering of reads from a gnode.
 *
 * 18-Mar-86 -- jrs
 *	Clean up cpu premption to use new intrcpu instead of intrslave
 *
 * 11 Mar 86 -- depp
 *	Added conditional call to "vcleanu" (vm_mem.c) to "pagein" routine
 *	to remove any "u" areas found on the "u" list to the freelist.
 *
 * 24 Feb 86 -- depp
 *	Moved the setting of the PG_M bit in the routine "pagein" until
 *	just after the page has be read in.  This move was necessary because
 *	the SRM considers the PG_M bit set and the PG_V reset as an invalid
 *	combination.
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
 * 30 Sep 85 -- depp
 *	Added checks for page locking into "pageout"
 *
 * 19 Jul 85 -- depp
 *	Added the setting of pg_cm (for shared memory only) if the 
 *	page is to be pushed, so that in distsmpte, the attached process
 *	pte's will have their pg_m field properly cleared.
 *
 * 001 - March 11 1985 - Larry Cohen
 *     disable mapped in files so NOFILE can be larger than 32
 *
 *
 * 11 Mar 85 -- depp
 *	Added in System V shared memory.
 *
 */

#include "../machine/reg.h"
#include "../machine/pte.h"

#include "../h/param.h"
#include "../h/smp_lock.h"
#include "../h/systm.h"
#include "../h/mount.h"
#include "../h/gnode.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/proc.h"
#include "../h/buf.h"
#include "../h/map.h"
#include "../h/text.h"
#include "../h/cmap.h"
#include "../h/vm.h"
#include "../h/file.h"
#include "../h/trace.h"
#include "../h/ipc.h"
#include "../h/shm.h"
#include "../h/cpudata.h"
#include "../h/kmalloc.h"

#ifdef mips
#include "../machine/cpu.h"
#endif mips
#ifdef vax
#include "../machine/mtpr.h"
#endif
#ifdef vax
#include "../machine/cpu.h"
#include "../../machine/common/cpuconf.h"
#include "../machine/vectors.h"
#endif

extern struct smem smem[];
extern struct sminfo sminfo;

struct lock_t *lock_by_type[] = {
	NULL,		/* CSYS */
	&lk_text,	/* CTEXT */
	&lk_p_vm,	/* CDATA */
	&lk_p_vm,	/* CSTACK */
	&lk_smem,	/* CSMEM */
	NULL,		/* illegal */
	NULL,		/* illegal */
	NULL,		/* illegal */
};

int	nohash = 0;		/* turn on/off hashing */
int	nobufcache = 1;		/* turn on/off buf cache for data */

extern int swapfrag;
/*
 * Handle a page fault.
 *
 * Basic outline
 *	If page is allocated, but just not valid:
 *		Wait if intransit, else just revalidate
 *		Done
 *	Compute <dev,bn> from which page operation would take place
 *	If page is text page, and filling from file system or swap space:
 *		If in free list cache, reattach it and then done
 *	Allocate memory for page in
 *		If block here, restart because we could have swapped, etc.
 *	Lock process from swapping for duration
 *	Update pte's to reflect that page is intransit.
 *	If page is zero fill on demand:
 *		Clear pages and flush free list cache of stale cacheing
 *		for this swap page (e.g. before initializing again due
 *		to 407/410 exec).
 *	If page is fill from file and in buffer cache:
 *		Copy the page from the buffer cache.
 *	If not a fill on demand:
 *		Determine swap address and cluster to page in
 *	Do the swap to bring the page in
 *	Instrument the pagein
 *	After swap validate the required new page
 *	Leave prepaged pages reclaimable (not valid)
 *	Update shared copies of text page tables
 *	Complete bookkeeping on pages brought in:
 *		No longer intransit
 *		Hash text pages into core hash structure
 *		Unlock pages (modulo raw i/o requirements)
 *		Flush translation buffer
 *	Process pagein is done
 */
#ifdef TRACE
#define	pgtrace(e)	trace(e,v,u.u_procp->p_pid)
#else
#define	pgtrace(e)
#endif

extern int km_reserve;
int	preptofree = 1;		/* send pre-paged pages to free list */
int	buf_pagein_cnt = 0;
int	buf_pagein_bytes = 0;

pagein(virtaddr, dlyu)
	unsigned virtaddr;
	int dlyu;
{
	register struct proc *p;
	register struct pte *pte;
	register u_int v;
	register int i, j;
	register struct cmap *c;
	unsigned pf;
	int type, fileno;
	struct pte opte;
	dev_t dev;
	int klsize;
	unsigned vsave;
	int smindex;		/* SHMEM */
	struct smem *sp;	/* SHMEM */
	daddr_t bn, bncache, bnswap;
	int s;
	int seql;
	int use_buffer_cache = 0;
	int klmax = KLMAX;	/* maybe less if paging in thru buffer cache */
	unsigned *counter;
	struct lock_t *seg_lock;
	int intrans_set=0;
#ifdef PGINPROF
	int otime, olbolt, oicr, a;

#ifdef vax
	s = spl6();
#endif vax
#ifdef mips
	s = splclock();
#endif mips
	otime = time, olbolt = lbolt, oicr = mfpr(ICR);
#endif PGINPROF
	if (CURRENT_CPUDATA->cpu_noproc	/* && IS_PROC_ADDR(virtaddr) */ )
					/* when we page system pages */
		panic("pagein: no process in context");
	cnt.v_faults++;
	/*
	 * Classify faulted page into a segment and get a pte
	 * for the faulted page.
	 */
	vsave = v = clbase(btop(virtaddr));
	p = u.u_procp;
	if (isatsv(p, v))
		type = CTEXT;
	else if (isadsv(p, v)) {
		type = CDATA;

#ifdef vax
		/* begin SHMEM */
		if(vtodp(p, v) >= p->p_dsize) {
			register int xp;

			type = CSMEM;

			if(p->p_sm == (struct p_sm *) NULL) {
				panic("pagin: p_sm");
			}

			/* translate the process data-space PTE	*/
			/* to the non-swapped shared memory PTE	*/

			xp = vtotp(p, v);
			if(p->p_sm != (struct p_sm *) NULL) {
				for(i = 0; i < sminfo.smseg; i++){
					if(p->p_sm[i].sm_p == NULL)
						continue;
					if(xp >= p->p_sm[i].sm_saddr  &&
						xp < p->p_sm[i].sm_saddr +
				/* should be "clrnd(btoc(...))" */
						btoc(p->p_sm[i].sm_p->sm_size))

						break;
				}
				if(i >= sminfo.smseg)
					panic("pagein SMEM");
				sp = p->p_sm[i].sm_p;
				pte = sp->sm_ptaddr +
					(xp - p->p_sm[i].sm_saddr);
				smindex = i;
				if (sp->sm_perm.mode & IPC_SYSTEM)
					panic("pagein: Attempt to pagein kernel/user shared memory page");
			}
		}
		/* end SHMEM */
#endif vax
#ifdef mips
        } else if (isasmsv(p, v, &smindex)) {
		struct p_sm *psm = &p->p_sm[smindex];
		type = CSMEM;
		sp = psm->sm_p;
		if (sp->sm_perm.mode & IPC_SYSTEM)
			panic("pagein: Attempt to pagein kernel/user shared memory page");
		pte = sp->sm_ptaddr + vtosmp(psm,v);
#endif mips
	} else
		type = CSTACK;

	/*
	 * If page is reclaimable, reclaim it.
	 * If page is text and intransit, sleep while it is intransit,
	 * If it is valid after the sleep, we are done.
	 * Otherwise we have to start checking again, since page could
	 * even be reclaimable now (we may have swapped for a long time).
	 */
	counter = &(cnt.v_mprace);
restart:
	s = splimp();

	/*
	 * Recalculating the PTE for type==CSMEM is currently not
	 * necessary because the SMEM page tables are "wired-down".
	 * When (if) the SMEM is generalized to allow the page table
	 * to be swapped then recalculation will be necessary.
	 */
	if (type != CSMEM)
		pte = vtopte(p, v);
	smp_lock(&lk_cmap, LK_RETRY);
	c = &cmap[pgtocm(pte->pg_pfnum)];
	if (pte->pg_v) {	/* The page was validated by another user. */
page_reclaimed:
		if (dlyu) {	/* We want to lock the page. */
			if (c->c_lock) {
				c->c_want = 1;
				sleep_unlock((caddr_t)c, PSWP+1, &lk_cmap);
				(void) splx(s);
				goto restart;
			}
			c->c_lock = 1;
		}
		++*counter;
#ifdef PGINPROF
		if (counter == &(cnt.v_pgrec)) {
			a = vmtime(otime, olbolt, oicr);
			rectime += a;
			if (a >= 0)
				vmfltmon(rmon, a, rmonmin, rres, NRMON);
		}
#endif
		smp_unlock(&lk_cmap);
		(void)splx(s);
		return;
	}

	/* if any free "u" pages to be placed on free list, do it now */
	if (nucmap)
		vcleanu();

	if (pte->pg_fod == 0 && pte->pg_pfnum) {
		if (((type == CTEXT) || (type == CSMEM)) && c->c_intrans) {
			counter = &(cnt.v_intrans);
			pgtrace(TR_INTRANS);
			sleep_unlock(((type==CTEXT) ? (caddr_t)p->p_textp
						    : (caddr_t)&sp->sm_flag),
				      PSWP+1, &lk_cmap);
			pgtrace(TR_EINTRANS);
			(void) splx(s);
			goto restart;
		}
		/*
		 * If page is in the free list, then take
		 * it back into the resident set, updating
		 * the size recorded for the resident set.
		 */
		counter = &(cnt.v_pgrec);
		smp_lock(seg_lock=lock_by_type[type], LK_RETRY);
		if (c->c_free) {
			pgtrace(TR_FRECLAIM);
			munlink(pte->pg_pfnum);
			cnt.v_pgfrec++;
			*((c->c_type == CTEXT) ? &p->p_textp->x_rssize :
				((c->c_type == CSMEM) ? &sp->sm_rssize :
					&p->p_rssize)) += CLSIZE;
		} else
			pgtrace(TR_RECLAIM);
		pte->pg_v = 1;
		if (anycl(pte, pg_m))
			pte->pg_m = 1;
		distcl(pte);
		if (type == CTEXT)
			distpte(p->p_textp, vtotp(p, v), pte);
#ifdef vax
		else if (type == CSMEM)		/* SHMEM */
			distsmpte(sp,
					vtotp(p, v) -
					p->p_sm[smindex].sm_saddr,
					pte, PG_NOCLRM);
#endif vax
		smp_unlock(seg_lock);
		u.u_ru.ru_minflt++;
		goto page_reclaimed;
	}

	/*
	 * <dev,bn> is where data comes from/goes to.
	 * <dev,bncache> is where data is cached from/to.
	 * <swapdev,bnswap> is where data will eventually go.
	 */
	if (pte->pg_fod == 0) {
		fileno = -1;
		bnswap = bncache = bn = vtod(p, v, p->p_dmap, p->p_smap);
		if(bnswap == -1)
			panic("pagein: vtod");
		dev = swapdev;
	} else {
		fileno = ((struct fpte *)pte)->pg_fileno;
                bn = ((struct fpte *)pte)->pg_blkno;
		bnswap = vtod(p, v, p->p_dmap, p->p_smap);
		if (fileno > PG_FMAX)
			panic("pagein pg_fileno");
		if (fileno == PG_FTEXT) {
			if (p->p_textp == 0)
				panic("pagein PG_FTEXT");
			dev = p->p_textp->x_gptr->g_dev;
			bncache = bn;
			
		} else if (fileno == PG_FZERO) {
			dev = swapdev;
			bncache = bnswap;
			
		}
	}
	klsize = 1;
	opte = *pte;

	/*
	 * Check for text detached but in free list.
	 * This can happen only if the page is filling
	 * from a gnode or from the swap device, (e.g. not when reading
	 * in 407/410 execs to a zero fill page.)
	 */
	if (type == CTEXT && fileno != PG_FZERO && !nohash &&
				((p->p_textp->x_gptr->g_flag & GTRC) == 0)) {
 		if ((c = mfind(dev, bncache, p->p_textp->x_gptr)) != 0) {
			if (c->c_lock) {
				c->c_want = 1;
				sleep_unlock((caddr_t)c, PSWP+1, &lk_cmap);
				(void) splx(s);
				goto restart;
			}
			if (c->c_type != CTEXT || c->c_gone == 0 ||
			    c->c_free == 0) 
				panic("pagein mfind");
			if (c->c_intrans || c->c_want)
				panic("pagein intrans|want");
			if (c->c_page != vtotp(p, v))
				panic("pagein c_page chgd");

			/*
			 * Following code mimics memall().
			 */
 			pf = cmtopg(c - cmap);	/* moved by rr from while loop*/
			munlink(pf);
			for (j = 0; j < CLSIZE; j++) {
				*(int *)(pte+j) &= PG_PROT;
				*(int *)(pte+j) |= pf++ << PTE_PFNSHIFT;
			}
			/*
			 * Make sure that potential sharers of this page
			 * see in a timely fashion that it has been
			 * recovered.
			 */
			smp_lock(&lk_text, LK_RETRY);
			p->p_textp->x_rssize += CLSIZE;
			distpte(p->p_textp, c->c_page, pte);
			smp_unlock(&lk_text);
	
			c->c_free = 0;
			c->c_gone = 0;
			c->c_lock = 1;
			c->c_ndx = p->p_textp - &text[0];
			if (dev == swapdev) {
				cnt.v_xsfrec++;
				pgtrace(TR_XSFREC);
			} else {
				cnt.v_xifrec++;
				pgtrace(TR_XIFREC);
			}
			cnt.v_pgrec++;
			u.u_ru.ru_minflt++;
			if (dev != swapdev && bnswap != -1) {
				c = mfind(swapdev, bnswap, p->p_textp->x_gptr);
				if(c)
				    munhash(swapdev,bnswap,p->p_textp->x_gptr);
                                SET_SWDIRTY(pte);

			}
			goto skipswap;
		}
	}

	/*
	 * Wasn't reclaimable or reattachable.  Have to prepare to
	 * bring the page in.  Lock the process against swapping and
	 * attempt to allocate the required memory.  If we fail, mark
	 * the process swappable, go to sleep and start over, since
	 * anything could have happened.
	 */
	if ((freemem - km_reserve) < CLSIZE * KLMAX ||
	    (memall(pte, CLSIZE,
		    ((type==CSMEM) ? (struct proc *)sp : p), 
		    type, v, V_CACHE) == 0)) {
		pgtrace(TR_WAITMEM);
		while ((freemem - km_reserve) < CLSIZE * KLMAX) {
			sleep_unlock((caddr_t)&freemem, PSWP+2, &lk_cmap);
			smp_lock(&lk_cmap, LK_RETRY);
		}	
		pgtrace(TR_EWAITMEM);
		smp_unlock(&lk_cmap);
		(void) splx(s);
		goto restart;
	}
	intrans_set=1;
	SET_P_VM(p, SPAGE);

	/*
	 * Memory is committed and the process is locked against swapping.
	 * Construct the new pte, and increment
	 * the (process or text) resident set size.
	 */
	smp_lock(seg_lock=lock_by_type[type], LK_RETRY);
	if (type == CTEXT)
		p->p_textp->x_rssize += CLSIZE;
	else if (type == CSMEM)		/* SHMEM */
		sp->sm_rssize += CLSIZE;
        else
		p->p_rssize += CLSIZE;
	smp_unlock(seg_lock);

	/*
	 * Two cases: either fill on demand (zero, or from file or text)
	 * or from swap space.
	 */
	if (opte.pg_fod) {
		if (fileno == PG_FZERO || fileno == PG_FTEXT) {
			/*
			 * Flush any previous text page use of this
			 * swap device block.
			 */
			if (type == CTEXT && bnswap != -1) {
				c = mfind(swapdev, bnswap, p->p_textp->x_gptr);
				if(c)munhash(swapdev,bnswap,p->p_textp->x_gptr); 
			}
			/*
			 * If zero fill, short-circuit hard work
			 * by just clearing pages.
			 */
			if (fileno == PG_FZERO) {
				pgtrace(TR_ZFOD);
				/*
				 * Here we map the physical page to forkutl
				 * and zero it.  Because this is a ZFOD page,
				 * this page fault did not originate in vmdup,
				 * so we are not oversubscribing the forkutl
				 * resource.
				 */
				for (i=0; i<CLSIZE; i++)
				  *(int *)(Forkmap+i) = *(int *)(pte+i)
				    & PG_PFNUM | PG_V | PG_M | PG_KW;
				newptes(p, btop(&forkutl), CLSIZE);
				bzero(&forkutl, CLBYTES);
				if (type != CTEXT)
					cnt.v_zfod += CLSIZE;
				goto skipswap;
			}
			pgtrace(TR_EXFOD);
			cnt.v_exfod += CLSIZE;
		} else
			panic("pagein vread");
		smp_unlock(&lk_cmap);
		(void) splx(s);

		/*
		 * Fill from gnode.  Try to find adjacent
		 * pages to bring in also.
		 */

		/* only use if data paged in from file system */
		if (type == CDATA && !nobufcache) {
			use_buffer_cache = 1;
			klmax = KLMAX/2;	/* 8 clicks == 8192 bytes */
		}

		if (type == CSMEM)
			panic("pagein: SHMEM fodkluster");
		v = fodkluster(p, v, pte, &klsize, dev, &bn, klmax);
		bncache = bn;
		
		/*
		 * Blocks of an executable may still be in the buffer
		 * cache, so we explicitly flush them out to disk
		 * so that the proper data will be paged in.  While
		 * flushblocks() gets the operation started (cf.
		 * ../sys/kern_exec.c), it is asynchronous and we cannot
		 * count on its being finished at this point.
		 */
		blkflush(dev, bn, (long)klsize*CLSIZE*NBPG, p->p_textp->x_gptr);
#ifdef TRACE
		if (type != CTEXT)
			trace(TR_XFODMISS, dev, bn);
#endif
	} else {
		int in_klsize;
		smp_unlock(&lk_cmap);
		if (opte.pg_pfnum)
			panic("pagein pfnum");
		pgtrace(TR_SWAPIN);
		/*
		 * Fill from swap area.  Try to find adjacent
		 * pages to bring in also.
		 */
		smp_lock(&lk_p_vm, LK_RETRY);
		in_klsize = ((type == CTEXT) ?
			     kltxt :
			     ((p->p_vm & SSEQL) ? klseql : klin));
		smp_unlock(&lk_p_vm);
		(void) splx(s);
		if(type != CSMEM)	/* SHMEM */
			v = kluster(p, v, pte, B_READ, &klsize, in_klsize, bn);
		/* THIS COULD BE COMPUTED INCREMENTALLY... */
		bncache = bn = vtod(p, v, p->p_dmap, p->p_smap);
		if(bncache == -1)
			panic("pagein: bncache=-1");
	}

	distcl(pte);

	if(bn == -1)
		panic("pagein: bn = -1");

	if(type == CSMEM)	/* SHMEM */
#ifdef vax
		swap(sp, bn, ptob(vtotp(p, v) - p->p_sm[smindex].sm_saddr),
#endif vax
#ifdef mips
		swap(sp, bn, ptob(v - p->p_sm[smindex].sm_saddr),
#endif mips
				ctob(CLSIZE), B_READ, B_PGIN|B_SMEM, dev, 0); 
	else {
		/* make pagein use the BUFFER CACHE!!! - rr*/
		/* for data only, text is reclaimed out of vm (I hope!) */
		/* this is checked above */
		if (use_buffer_cache) {
			struct gnode *gp = p->p_textp->x_gptr;
			struct buf *bp = NULL;
			int size = klsize*ctob(CLSIZE);
			if (ISLOCAL(gp->g_mp))
				bp = bread(dev, bn, size, 0);
			else
				bp = bread(dev, bn, size, gp);
			if (bp->b_flags & B_ERROR) {
				printf("buf_pagein got B_ERROR dev %x bn %d size %d gp %x\n",
					dev,bn,size,gp);
				brelse(bp);
				goto swapit;
			}
			buf_pagein_cnt++;
			buf_pagein_bytes += size;
			bcopy(bp->b_un.b_addr,ptob(v),size);
			brelse(bp);
		}
		else {
swapit:
			swap(p, bn, ptob(v), klsize * ctob(CLSIZE),
	    			B_READ, B_PGIN, dev, 0); 
		}
	}
#ifdef TRACE
	trace(TR_PGINDONE, vsave, u.u_procp->p_pid);
#endif
	/*
	 * Instrumentation.
	 */
	s = splimp();
	smp_lock(&lk_cmap, LK_RETRY);
	u.u_ru.ru_majflt++;
	cnt.v_pgin++;
	cnt.v_pgpgin += klsize * CLSIZE;
#ifdef PGINPROF
	a = vmtime(otime, olbolt, oicr) / 100;
	pgintime += a;
	if (a >= 0)
		vmfltmon(pmon, a, pmonmin, pres, NPMON);
#endif

skipswap:
	/*
	 * Fix page table entries.
	 *
	 * Only page requested in is validated, and rest of pages
	 * can be ``reclaimed''.  This allows system to reclaim prepaged pages
	 * quickly if they are not used and memory is tight.
	 */
	pte = vtopte(p, vsave);
	if (opte.pg_fod)
		SET_SWDIRTY(pte);
	if (pte->pg_pfnum == 0)
		panic("pagein: pfnum = 0");
	pte->pg_v = 1;
	distcl(pte);
	if (type == CTEXT) {
		smp_lock(&lk_text, LK_RETRY);
		distpte(p->p_textp, vtotp(p, vsave), pte);
		if (opte.pg_fod)
			p->p_textp->x_flag |= XWRIT;
		smp_unlock(&lk_text);
	} else if (type == CSMEM) {	/* SHMEM */
#ifdef vax
		smp_lock(&lk_smem, LK_RETRY);
		distsmpte(sp, vtotp(p, vsave) - p->p_sm[smindex].sm_saddr,
			  pte, PG_NOCLRM);
		smp_unlock(&lk_smem);
#endif vax
	}
	smp_unlock(&lk_cmap);
	if (type == CSMEM) 	/* SHMEM */
		XPRINTF(XPR_SM,"pagein: fill pte 0x%x\n",*(int *)pte, 0,0,0);
	/*
	 * Memall returned page(s) locked.  Unlock all
	 * pages in kluster.  If locking pages for raw i/o
	 * leave the page which was required to be paged in locked,
	 * but still unlock others.
	 * If text pages, hash into the cmap situation table.
	 */
	pte = vtopte(p, v);
	for (i = 0; i < klsize; i++) {
		smp_lock(&lk_cmap, LK_RETRY);
		c = &cmap[pgtocm(pte->pg_pfnum)];
		if (v != vsave && fileno == PG_FTEXT) {
			SET_SWDIRTY(pte);
			distcl(pte);
			if (type == CTEXT) {
				smp_lock(&lk_text, LK_RETRY);
				distpte(p->p_textp, vtotp(p, v), pte);
				smp_unlock(&lk_text);
			}
		}
		if (c->c_intrans ^= intrans_set)
			panic("pagein: intrans");
		if (type == CTEXT && c->c_blkno == 0 && bncache && !nohash &&
				((p->p_textp->x_gptr->g_flag & GTRC) == 0)) {
			if(bncache != -1){
				mhash(c, dev, bncache);
				bncache += CLBYTES / DEV_BSIZE;
			}	
		}
		if (v != vsave || !dlyu)
			MUNLOCK(c);
		if (v != vsave && type != CTEXT && 
				type != CSMEM &&	/* SHMEM */
				preptofree &&
				opte.pg_fod == 0) {
			/*
			 * Throw pre-paged data/stack pages at the
			 * bottom of the free list.
			 */
			smp_lock(&lk_p_vm, LK_RETRY);
			p->p_rssize -= CLSIZE;
			smp_unlock(&lk_p_vm);
			memfree(pte, CLSIZE, 0);
		}
		smp_unlock(&lk_cmap);
		newptes(p, v, CLSIZE);
		v += CLSIZE;
		pte += CLSIZE;
	}
	(void) splx(s);
	if (type == CTEXT)
		wakeup((caddr_t)p->p_textp);
	else if (type == CSMEM)		/* SHMEM */
		wakeup((caddr_t)&sp->sm_flag);

	/*
	 * All done.
	 */
	CLEAR_P_VM(p, SPAGE);
	seql = p->p_vm & SSEQL;  /* read advisory without lock */

	/*
	 * If process is declared fifo, memory is tight,
	 * and this was a data page-in, free memory
	 * klsdist pagein clusters away from the current fault.
	 */
	if (seql && freemem < lotsfree && type == CDATA) {
		int k = (vtodp(p, vsave) / CLSIZE) / klseql;
		dpageout(p, (k - klsdist) * klseql * CLSIZE, klout*CLSIZE);
		dpageout(p, (k + klsdist) * klseql * CLSIZE, klout*CLSIZE);
	}
}

/*
 * Take away n pages of data space
 * starting at data page dp.
 * Used to take pages away from sequential processes.
 * Mimics pieces of code in pageout() below.
 */
dpageout(p, dp, n)
	struct proc *p;
	int dp, n;
{
	register struct cmap *c;
	int i, s, klsize;
	register struct pte *pte;
	unsigned v;
	daddr_t daddr;

	if (dp < 0) {
		n += dp;
		dp = 0;
	}
	if (dp + n > p->p_dsize)
		n = p->p_dsize - dp;
	for (i = 0; i < n; i += CLSIZE, dp += CLSIZE) {
		pte = dptopte(p, dp);
		v = dptov(p, dp);
		if (pte->pg_fod || pte->pg_pfnum == 0)
			continue;
		s = splimp();
		smp_lock(&lk_cmap, LK_RETRY);
		c = &cmap[pgtocm(pte->pg_pfnum)];
		if (c->c_lock || c->c_free) {
			smp_unlock(&lk_cmap);
			(void) splx(s);
			continue;
		      }
		if (pte->pg_v) {
			pte->pg_v = 0;
#ifdef mips
                        flushpte(p, v, CLSIZE, c->c_type);
#endif mips
			(void)clear_foreign_tlbs(p, c->c_page, CLSIZE, CDATA);
			if (anycl(pte, pg_m))
				pte->pg_m = 1;
			distcl(pte);
		}
		if (dirtycl(pte)) {
			if (bswlist.av_forw == NULL) {
				smp_unlock(&lk_cmap);
				(void)splx(s);
				continue;
			}
			smp_unlock(&lk_cmap);
			(void)splx(s);
			v = kluster(p, v, pte, B_WRITE,
				&klsize, klout, (daddr_t)0);
			/* THIS ASSUMES THAT p == u.u_procp */
			daddr = vtod(p, v, p->p_dmap, p->p_smap);
			if(daddr == -1) {
			     if(swalloc_vtod(p,v) == 0)
					continue;
			     else {
				daddr = vtod(p,v,p->p_dmap, p->p_smap);
			     }
			}

			s = splimp();
			smp_lock(&lk_cmap, LK_RETRY);
			MLOCK(c);
#ifdef mips
                        flushpte(p, dptov(p,dp), CLSIZE, c->c_type);
#endif mips
			CLEAR_DIRTY(pte);
			distcl(pte);
			smp_lock(&lk_p_vm, LK_RETRY);
			p->p_poip++;
			smp_unlock(&lk_p_vm);
			smp_unlock(&lk_cmap);
			(void)splx(s);
			swap(p, daddr, ptob(v), klsize * ctob(CLSIZE),
			    B_WRITE, B_DIRTY, swapdev, pte->pg_pfnum);
		} else {
			if (c->c_gone == 0) {
				smp_lock(&lk_p_vm, LK_RETRY);
				p->p_rssize -= CLSIZE;
				smp_unlock(&lk_p_vm);
			}
			memfree(pte, CLSIZE, 0);
			smp_unlock(&lk_cmap);
			(void) splx(s);
			cnt.v_seqfree += CLSIZE;
		}
	}
}
		    

int	pushes;

#define	FRONT	1
#define	BACK	2

/*
 * The page out daemon, which runs as process 2.
 *
 * As long as there are at least lotsfree pages,
 * this process is not run.  When the number of free
 * pages stays in the range desfree to lotsfree,
 * this daemon runs through the pages in the loop
 * at a rate determined in vmsched().  Pageout manages
 * two hands on the clock.  The front hand moves through
 * memory, clearing the valid bit (simulating a reference bit),
 * and stealing pages from procs that are over maxrss.
 * The back hand travels a distance behind the front hand,
 * freeing the pages that have not been referenced in the time
 * since the front hand passed.  If modified, they are pushed to
 * swap before being freed.
 */
pageout()
{
	register int count;
	register int maxhand = pgtocm(maxfree);
	register int fronthand, backhand;
	register int cpindex, cpident;
	struct cpudata *pcpu;
	volatile struct cpudata *volpcpu; /* has to be volatile for primary
					     to see other cpus' cpu_state */
	int kmhwmhit;

	/*
	 * Set the two clock hands to be separated by a reasonable amount,
	 * but no more than 360 degrees apart.
	 */
	backhand = 0 / CLBYTES;
	fronthand = HANDSPREAD / CLBYTES;
	if (fronthand >= maxhand)
		fronthand = maxhand - 1;

loop:
	/*
	 * Before sleeping, look to see if there are any swap I/O headers
	 * in the ``cleaned'' list that correspond to dirty
	 * pages that have been pushed asynchronously. If so,
	 * empty the list by calling cleanup().
	 *
	 * N.B.: We guarantee never to block while the cleaned list is nonempty.
	 */
	(void) splhigh();
  	if (bclnlist != NULL) {
 		(void) spl0();
  		cleanup();
 		goto loop;
  	}

	/*
	 * Sync up kernel memory allocator pte(s) if necessary.
	 */

	if (kmhwmhit) km_intr_cpus();

	/*
	 * the slavehold is guaranteed visible to the other processors
	 * because we do an interlocked instruction in intrpt_cpu.
	 */

	smp_lock(&lk_rq,LK_RETRY);
	slavehold = 0;
	cpident = CURRENT_CPUDATA->cpu_num;
	for (cpindex = lowcpu; cpindex <= highcpu; cpindex++) {
		if (cpindex != cpident && (pcpu=CPUDATA(cpindex))) {
			pcpu->cpu_runrun++;
			intrpt_cpu(cpindex, IPI_CPUHOLD);
		}
	}
	smp_unlock(&lk_rq);

  	sleep((caddr_t)&proc[2], PSWP+1);
	ASSERT_PRIMARY_CPU(pageout);
	/*
	 * this will stop the secondary processors:
	 * see machdep.c, intrcpu.
	 */
	smp_lock(&lk_rq,LK_RETRY);

	slavehold = 1;
	cpident = CURRENT_CPUDATA->cpu_num;
	for (cpindex = lowcpu; cpindex <= highcpu; cpindex++) {
		if (cpindex != cpident && (pcpu=CPUDATA(cpindex))) {
			pcpu->cpu_runrun++;
			intrpt_cpu(cpindex, IPI_CPUHOLD);
		}
	}
	smp_unlock(&lk_rq);

	(void) spl0();
	/* Wait for the other processors to stop. */ 
	for (cpindex = lowcpu; cpindex <= highcpu; cpindex++)
		if (cpindex != cpident && (volpcpu=CPUDATA(cpindex)))
			while ((volpcpu->cpu_state & (CPU_STOP|CPU_RUN))
			       != CPU_STOP) {
			}
#ifdef vax
	/* Quiesce our vector processor (if any) */
	VPSYNC();
	mtpr(TBIA, 0);
#else mips
	flush_tlb();
#endif vax ^ mips

	count = 0;
	pushes = 0;
	kmhwmhit = 0;

	while (nscan < desscan && freemem < lotsfree) {
		/*
		 * If checkpage manages to add a page to the free list,
		 * we give ourselves another couple of trips around the loop.
		 */
		if (checkpage(fronthand, FRONT))
			count = 0;
		if (checkpage(backhand, BACK))
			count = 0;
		else {
			(void) spl6();
			smp_lock(&lk_cmap_bio, LK_RETRY);
			if (bswlist.av_forw == NULL) {
				bswlist.b_flags |= B_WANTED;
				smp_unlock(&lk_cmap_bio);
				(void) spl0();
				goto loop;
			}
			smp_unlock(&lk_cmap_bio);
			(void) spl0();
		}
		cnt.v_scan++;
		nscan++;
		if (++fronthand >= maxhand) {
			fronthand = 0;
			cnt.v_rev++;
			if (count > 2) {
				/*
				 * Extremely unlikely, but we went around
				 * the loop twice and didn't get anywhere.
				 * Don't cycle, stop till the next clock tick.
				 */
				goto loop;
			}
			count++;
		}
		if (++backhand >= maxhand)
			backhand = 0;
	}
	if (freemem < lotsfree) kmhwmhit = km_hwmscan(KM_HWMSOFT);
	goto loop;
}

/*
 * An iteration of the clock pointer (hand) around the loop.
 * Look at the page at hand.  If it is a
 * locked (for physical i/o e.g.), system (u., page table)
 * or free, then leave it alone.
 * Otherwise, if we are running the front hand,
 * invalidate the page for simulation of the reference bit.
 * If the proc is over maxrss, we take it.
 * If running the back hand, check whether the page
 * has been reclaimed.  If not, free the page,
 * pushing it to disk first if necessary.
 */
checkpage(hand, whichhand)
	int hand, whichhand;
{
	register struct proc *rp;
	register struct cmap *c;
	register struct cmap *c1, *c2;
	register struct pte *pte;
	register struct dmap *dmp;
	struct text *xp;
	struct smem *sp;		/* SHMEM */
	swblk_t daddr;
	unsigned v;			/* Virt page within proc addr space*/
	int klsize;
	int lock;			/* Is the segment locked ? */
	int j;
	int s;
	int indx;
	struct gnode *gp;
	struct lock_t *seg_lock;
        int smpage;
	int segsize;
#ifdef mips
        struct user *ppushutl;
#define pushutl (*ppushutl)
#endif mips
	union {
		struct text	*text;
		struct proc	*data_or_stack;
		struct smem	*shared_mem;
		caddr_t		ptr;
	} seg;

	/*
	 * Find a process and text pointer for the
	 * page, and a virtual page number in either the
	 * process or the text image.
	 */
	s = splimp();
	smp_lock(&lk_cmap, LK_RETRY);
	c = &cmap[hand];
	if (c->c_lock || c->c_free || c->c_type == CSYS ||
	    (smp_lock(seg_lock=lock_by_type[c->c_type], LK_ONCE) == LK_LOST)){
		smp_unlock(&lk_cmap);
		(void) splx(s);
		return (0);
	      }
	switch (c->c_type) {

	case CTEXT:
		seg.text = xp = &text[c->c_ndx];
		if ((lock = xp->x_flag & (XNOSW|XLOCK)) == 0) {
			rp = xp->x_caddr;
			gp = xp->x_gptr;
			v = tptov(rp, c->c_page);
			pte = tptopte(rp, c->c_page);
			dmp = xp->x_dmap;
			segsize = xp->x_size;
		}
		break;

	case CDATA:
	case CSTACK:
		seg.data_or_stack = rp = &proc[c->c_ndx];
		while (rp->p_vm & SNOVM)
			rp = rp->p_xlink;
		/*
		 * If the process is being swapped out
		 * or about to exit, do not bother with its
		 * pages
		 */
		if ((lock = ((rp->p_vm & (SULOCK | SLOCK)) ||
			     (rp->p_type & SWEXIT))) == 0)
			if (c->c_type == CDATA) {
				v = dptov(rp, c->c_page);
				pte = dptopte(rp, c->c_page);
				dmp = rp->p_dmap;
				segsize = rp->p_dsize;
			} else {
			      	v = sptov(rp, c->c_page);
				pte = sptopte(rp, c->c_page);
				dmp = rp->p_smap;
				segsize = rp->p_ssize;
			}
		break;

	case CSMEM:	/* SHMEM */
		seg.shared_mem = sp = &smem[c->c_ndx];
		if ((lock = ((sp->sm_flag & (SMNOSW | SMLOCK)) ||
			     (((struct proc *)sp->sm_caddr)->p_type & SWEXIT))) == 0) {
#ifdef vax
			v = c->c_page;
#endif vax
#ifdef mips
			rp = sp->sm_caddr;
			v = smptov(rp, sp, c->c_page);
#endif mips
			pte = sp->sm_ptaddr + c->c_page;
			dmp = sp->sm_dmap;
		/* shared memory size is in bytes not clicks ; ALLOC_WASTED */
		/* macro invoked later expects segsize to be in core clicks */
			segsize = clrnd(btoc(sp->sm_size));
		}
		break;
	}
	/* if segment locked, then not eligible for paging */
	if (lock) {
		smp_unlock(seg_lock);
		smp_unlock(&lk_cmap);
		(void) splx(s);
		return(0);
	}

	if (pte->pg_pfnum != cmtopg(hand)) 
		panic("bad c_page");

	/*
	 * If page is valid; make invalid but reclaimable.
	 * If this pte is not valid, then it must be reclaimable
	 * and we can add it to the free list.
	 */
	if (pte->pg_v) {
		if (whichhand == BACK) {
			smp_unlock(seg_lock);
			smp_unlock(&lk_cmap);
			(void) splx(s);
			return(0);
		}
		pte->pg_v = 0;
#ifdef mips
                flushpte(rp, v, CLSIZE, c->c_type);
#endif mips
		(void)clear_foreign_tlbs(seg.ptr, c->c_page, CLSIZE, c->c_type);
		if (pte->pg_pfnum == 0)
			panic("pageout: checkpage");
		if (anycl(pte, pg_m))
			pte->pg_m = 1;
		distcl(pte);
		if (c->c_type == CTEXT)
			distpte(xp, vtotp(rp, v), pte);
		if (c->c_type == CSMEM) {	/* SHMEM */
			distsmpte(sp, v, pte, PG_NOCLRM);
			smp_unlock(&lk_smem);
			smp_unlock(&lk_cmap);
			(void) splx(s);
			return(0);
		}
		if ((rp->p_vm & (SSEQL|SUANOM)) == 0 &&
		    rp->p_rssize <= rp->p_maxrss) {
			smp_unlock(seg_lock);
			smp_unlock(&lk_cmap);
			(void) splx(s);
			return (0);
		      }
	}
	if (c->c_type != CTEXT
			&&  c->c_type != CSMEM) { /* SHMEM */
		/*
		 * Guarantee a minimal investment in data
		 * space for jobs in balance set.
		 */
		if ((rp->p_rssize + rp->p_slptime) < saferss) {
			smp_unlock(&lk_p_vm);
			smp_unlock(&lk_cmap);
			(void) splx(s);
			return (0);
		      }
	}

	/*
	 * If the page is currently dirty, we
	 * have to arrange to have it cleaned before it
	 * can be freed.  We mark it clean immediately.
	 * If it is reclaimed while being pushed, then modified
	 * again, we are assured of the correct order of 
	 * writes because we lock the page during the write.  
	 * This guarantees that a swap() of this process (and
	 * thus this page), initiated in parallel, will,
	 * in fact, push the page after us.
	 *
	 * The most general worst case here would be for
	 * a reclaim, a modify and a swapout to occur
	 * all before the single page transfer completes.
	 */
	if (dirtycl(pte)
		|| (c->c_type == CSMEM && dirtysm(sp,v))) { /* SHMEM */
		/*
		 * If the process is being swapped out
		 * or about to exit, do not bother with its
		 * dirty pages
		 */
		if (seg_lock != &lk_p_vm)
			smp_lock(&lk_p_vm, LK_RETRY);
		if ((c->c_type != CSMEM &&	/* SHMEM */
				((rp->p_vm & SLOCK) || (rp->p_type & SWEXIT)))
		    ||
		/*
		 * Limit pushes to avoid saturating
		 * pageout device.
		 */
		    (pushes > maxpgio / 4)) {
			if (seg_lock != &lk_p_vm)
				smp_unlock(&lk_p_vm);
			smp_unlock(seg_lock);
			smp_unlock(&lk_cmap);
			(void) splx(s);
			return (0);
		}
		if (seg_lock != &lk_p_vm)
			smp_unlock(&lk_p_vm);
		pushes++;

		/*
		 * Now carefully make sure that there will
		 * be a header available for the push so that
		 * we will not block waiting for a header in
		 * swap().  The reason this is important is
		 * that we (proc[2]) are the one who cleans
		 * dirty swap headers and we could otherwise
		 * deadlock waiting for ourselves to clean
		 * swap headers.  The sleep here on &proc[2]
		 * is actually (effectively) a sleep on both
		 * ourselves and &bswlist, and this is known
		 * to swdone and swap in vm_swp.c.  That is,
		 * &proc[2] will be awakened both when dirty
		 * headers show up and also to get the pageout
		 * daemon moving.
		 */
		(void) spl6();
		smp_lock(&lk_cmap_bio, LK_RETRY);
		if (bswlist.av_forw == NULL) {
			bswlist.b_flags |= B_WANTED;
			smp_unlock(&lk_cmap_bio);
			smp_unlock(seg_lock);
			smp_unlock(&lk_cmap);
			(void) splx(s);
			return (0);
		}
		smp_unlock(&lk_cmap_bio);
		(void) splimp();

		if(dmp == NULL)
			panic("checkpage: NULL dmap");
		if((indx = ((ctod(c->c_page))/swapfrag)) > dmp->dm_last)
			panic("checkpage: invalid swap index"); 
		if(dmp->dm_map[indx] == 0){
			if((dmp->dm_map[indx]=rmalloc(swapmap,swapfrag))== 0){
				smp_unlock(seg_lock);
				smp_unlock(&lk_cmap);
				(void) splx(s);
				return(0);
			}
			ALLOC_SWAP(swapfrag, c->c_type);
			/* if we just allocated a swap fragment for the last entry in
			   dmap then we have to adjust the swap wastage */
			if (indx == dmp->dm_last -1 ) 
				ALLOC_WASTED(segsize);
			dmp->dm_cnt ++;
		}

		/*
		 * Now committed to pushing the page...
		 */
		if ((c->c_lock ^= 1) == 0)
			panic("checkpage: cmap entry already locked");
#ifdef vax
		if (c->c_type != CSMEM)	/* SHMEM */
			uaccess(rp, Pushmap, &pushutl);
#endif vax
#ifdef mips
                ppushutl = (struct user *)PHYS_TO_K0(
                                ptob(rp->p_addr[0].pg_pfnum));
                flushpte(rp, v, CLSIZE, c->c_type);
#endif mips
		CLEAR_DIRTY(pte);
		distcl(pte);
		if (c->c_type == CTEXT)  {
			xp->x_poip++;
			distpte(xp, vtotp(rp, v), pte);
		} else if (c->c_type == CSMEM) { /* SHMEM */
			sp->sm_poip++;
			distsmpte(sp, v, pte, PG_CLEARM);
		} else
			rp->p_poip++;
		smp_unlock(seg_lock);
		smp_unlock(&lk_cmap);
		(void) splx(s);
		if(c->c_type == CSMEM){		/* SHMEM */
			smpage = ctod(c->c_page);
			daddr = sp->sm_dmap->dm_map[smpage/swapfrag] +
					smpage%swapfrag;
			swap(sp, daddr, ptob(c->c_page), ctob(CLSIZE), B_WRITE,
				B_DIRTY|B_SMEM, swapdev, pte->pg_pfnum);
		} else {
			v = kluster(rp, v, pte, B_WRITE,
				    &klsize, klout, (daddr_t)0);
			if (klsize == 0)
				panic("pageout klsize");
			daddr = vtod(rp, v, rp->p_dmap, rp->p_smap);
			swap(rp, daddr, ptob(v), klsize*ctob(CLSIZE), B_WRITE,
			        B_DIRTY, swapdev, pte->pg_pfnum);
		}
		/*
		 * The cleaning of this page will be
		 * completed later, in cleanup() called
		 * (synchronously) by us (proc[2]).  In
		 * the meantime, the page frame is locked
		 * so no havoc can result.
		 */
		return (1);	/* well, it'll be free soon */

	}
	/*
	 * Decrement the resident set size of the current
	 * text object/process, and put the page in the
	 * free list. Note that we don't give memfree the
	 * pte as its argument, since we don't want to destroy
	 * the pte.  If it hasn't already been discarded
	 * it may yet have a chance to be reclaimed from
	 * the free list.
	 */
	if (c->c_gone == 0)
		*((c->c_type == CTEXT) ? &xp->x_rssize :
			((c->c_type == CSMEM) ? &sp->sm_rssize :
				&rp->p_rssize)) -= CLSIZE;
	smp_unlock(seg_lock);
 	memfree(pte, CLSIZE, 0);
	smp_unlock(&lk_cmap);
	(void) splx(s);
	cnt.v_dfree += CLSIZE;
	return (1);		/* freed a page! */
#ifdef mips
#undef pushutl
#endif mips
}

/*
 * Process the ``cleaned'' list.
 *
 * Scan through the linked list of swap I/O headers
 * and free the corresponding pages that have been
 * cleaned by being written back to the paging area.
 * If the page has been reclaimed during this time,
 * we do not free the page.  As they are processed,
 * the swap I/O headers are removed from the cleaned
 * list and inserted into the free list.
 */
cleanup()
{
	register struct buf *bp;
	struct proc *rp;
	struct text *xp;
	struct smem *sp;	/* SHMEM */
	register struct cmap *c;
	register struct pte *pte;
	unsigned pf;
	register int i;
	int s, center;

	for (;;) {
		s = spl6();
		smp_lock(&lk_cmap_bio,LK_RETRY);
		if ((bp = bclnlist) == 0)
			break;
		bclnlist = bp->av_forw;
		smp_unlock(&lk_cmap_bio);
		splx(s);
		pte = vtopte(&proc[2], btop(bp->b_un.b_addr));
		center = 0;
		for (i = 0; i < bp->b_bcount; i += CLSIZE * NBPG) {
			register short *seg_poip = NULL;
			struct lock_t *seg_lock;
			pf = pte->pg_pfnum;
			s = splimp();
			smp_lock(&lk_cmap, LK_RETRY);
			c = &cmap[pgtocm(pf)];
			MUNLOCK(c);
			if (pf != bp->b_pfcent) {
				if (c->c_gone) {
					memfree(pte, CLSIZE, 0);
					cnt.v_dfree += CLSIZE;
				}
				goto skip;
			}
			center++;
			if (c->c_type==CSYS)
				panic("cleanup CSYS");
			smp_lock(seg_lock=lock_by_type[c->c_type], LK_RETRY);
			switch (c->c_type) {

			case CTEXT:
				xp = &text[c->c_ndx];
				seg_poip = &xp->x_poip;
				break;

			case CDATA:
			case CSTACK:
				rp = &proc[c->c_ndx];
				while (rp->p_vm & SNOVM)
					rp = rp->p_xlink;
				seg_poip = &rp->p_poip;
				break;

			case CSMEM:	/* SHMEM */
				sp = &smem[c->c_ndx];
				seg_poip = &sp->sm_poip;
				break;
			}
			--*seg_poip;
			if (c->c_gone == 0) {
				register struct pte *upte;
				switch (c->c_type) {

				case CTEXT:
					upte = tptopte(xp->x_caddr, c->c_page);
					break;

				case CDATA:
					upte = dptopte(rp, c->c_page);
					break;

				case CSTACK:
					upte = sptopte(rp, c->c_page);
					break;

				case CSMEM:	/* SHMEM */
					upte = sp->sm_ptaddr + c->c_page;
					break;
				}
				if (upte->pg_v) {
					smp_unlock(seg_lock);
					goto skip;
				}
				*((c->c_type == CTEXT) ? &xp->x_rssize :
				  ((c->c_type == CSMEM) ? &sp->sm_rssize :
				  &rp->p_rssize)) -= CLSIZE;
			}
			smp_unlock(seg_lock);
			memfree(pte, CLSIZE, 0);
skip:
			smp_unlock(&lk_cmap);
			(void) splx(s);
			cnt.v_dfree += CLSIZE;
			if (seg_poip != NULL && *seg_poip == 0)
				wakeup((caddr_t)seg_poip);
			pte += CLSIZE;
		}
		if (center != 1)
			panic("cleanup center");
		bp->b_flags = 0;
		s = spl6();
		smp_lock(&lk_cmap_bio, LK_RETRY);
		bp->av_forw = bswlist.av_forw;
		bswlist.av_forw = bp;
		if (bswlist.b_flags & B_WANTED) {
			bswlist.b_flags &= ~B_WANTED;
			wakeup((caddr_t)&bswlist);
		}
		smp_unlock(&lk_cmap_bio);
		(void) splx(s);
	}
	smp_unlock(&lk_cmap_bio);
	(void) splx(s);
}

/*
 * Kluster locates pages adjacent to the argument pages
 * that are immediately available to include in the pagein/pageout,
 * and given the availability of memory includes them.
 * It knows that the process image is contiguous in chunks;
 * an assumption here is that CLSIZE * KLMAX is a divisor of swapfrag,
 * so that by looking at KLMAX chunks of pages, all such will
 * necessarily be mapped swap contiguous.
 */
int	noklust;
int	klicnt[KLMAX];
int	klocnt[KLMAX];

kluster(p, v, pte0, rw, pkl, klsize, bn0)
	register struct proc *p;
	register unsigned v;
	struct pte *pte0;
	int rw, *pkl, klsize;
	daddr_t bn0;
{
	int type, cl, clmax;
	int kloff, k, klmax;
	register struct pte *pte;
	int klback, klforw;
	register int i;
	unsigned v0;
	daddr_t bn;
	register struct cmap *c;
	int s;

	if (rw == B_READ)
		klicnt[0]++;
	else
		klocnt[0]++;
	*pkl = 1;
	if (noklust || klsize <= 1 || klsize > KLMAX || (klsize & (klsize - 1)))
		return (v);
	if (rw == B_READ && freemem < CLSIZE * KLMAX)
		return (v);
	if (isassv(p, v)) {
		type = CSTACK;
		cl = vtosp(p, v) / CLSIZE;
		clmax = p->p_ssize / CLSIZE;
	} else if (isadsv(p, v)) {
		type = CDATA;
		cl = vtodp(p, v) / CLSIZE;
		clmax = p->p_dsize / CLSIZE;
	} else {
		type = CTEXT;
		if (rw == B_WRITE) {
			bn0 = vtod(p, v, p->p_dmap, p->p_smap);
			mhremove(pte0,bn0,1,LK_FALSE);
		}
		cl = vtotp(p, v) / CLSIZE;
		clmax = p->p_textp->x_size / CLSIZE;
	}
	kloff = cl & (klsize - 1);
	pte = pte0;
	bn = bn0;
	v0 = v;
	s = splimp();
	smp_lock(&lk_cmap, LK_RETRY);
	for (k = kloff; --k >= 0;) {
		i = ((type == CSTACK) ? CLSIZE : (-CLSIZE));
		pte += i;
		v0 +=i;
		if (type == CTEXT && rw == B_READ && bn) {
			bn -= CLBYTES / DEV_BSIZE;
			if (mfind(swapdev, bn, p->p_textp->x_gptr))
				break;
		}
		if (!klok(pte, rw))
			break;
		if (rw==B_READ) {
			i = memall(pte, CLSIZE, p, type, v0, V_CACHE);
			if (i == 0)
				break;
			if (type == CTEXT) {
				smp_lock(&lk_text, LK_RETRY);
				p->p_textp->x_rssize += CLSIZE;
				smp_unlock(&lk_text);
			} else {
				smp_lock(&lk_p_vm, LK_RETRY);
				p->p_rssize += CLSIZE;
				smp_unlock(&lk_p_vm);
			}
		}
		else if (type == CTEXT) {
			bn -= CLBYTES / DEV_BSIZE;
			mhremove(pte,bn,1,LK_TRUE);
		}
	}
	klback = (kloff - k) - 1;
	pte = pte0;
	if ((cl - kloff) + klsize > clmax)
		klmax = clmax - (cl - kloff);
	else
		klmax = klsize;
	bn = bn0;
	v0 = v;
	for (k = kloff; ++k < klmax;) {
		i = ((type == CSTACK) ? (-CLSIZE) : CLSIZE);
		pte += i;
		v0 +=i;
		if (type == CTEXT && rw == B_READ && bn) {
			bn += (CLBYTES / DEV_BSIZE);
			if (mfind(swapdev, bn, p->p_textp->x_gptr))
				break;
		}
		if (!klok(pte, rw))
			break;
		if (rw==B_READ) {
			i = memall(pte, CLSIZE, p, type, v, V_CACHE);
			if (i == 0)
				break;
			if (type == CTEXT) {
				smp_lock(&lk_text, LK_RETRY);
				p->p_textp->x_rssize += CLSIZE;
				smp_unlock(&lk_text);
			} else {
				smp_lock(&lk_p_vm, LK_RETRY);
				p->p_rssize += CLSIZE;
				smp_unlock(&lk_p_vm);
			}
		}
		else if (type == CTEXT) {
			bn += (CLBYTES / DEV_BSIZE);
			mhremove(pte,bn,1,LK_TRUE);
		}
	}
	klforw = (k - kloff) - 1;
	if (klforw + klback == 0) {
		smp_unlock(&lk_cmap);
		(void)splx(s);
		return (v);
	}
	i = ((type==CSTACK) ? klforw : klback) * CLSIZE;
	pte = pte0 - i;
	v -= i;
	*pkl = klforw + klback + 1;
	if (rw == B_READ)
		klicnt[0]--, klicnt[*pkl - 1]++;
	else
		klocnt[0]--, klocnt[*pkl - 1]++;
	v0 = v;
	if (rw == B_WRITE)
		for (i = 0; i < *pkl; i++,pte+=CLSIZE,v+=CLSIZE) {
			if (pte == pte0)
				continue;
			c = &cmap[pgtocm(pte->pg_pfnum)];
			MLOCK(c);
#ifdef mips
                        newptes(p, v, CLSIZE);
#endif mips
			CLEAR_DIRTY(pte);
			distcl(pte);
			if (type == CTEXT) {
				smp_lock(&lk_text, LK_RETRY);
				distpte(p->p_textp, vtotp(p, v), pte);
				smp_unlock(&lk_text);
			}
		}
	smp_unlock(&lk_cmap);
	(void) splx(s);
	return (v0);
}

klok(pte, rw)
	register struct pte *pte;
	int rw;
{
	register struct cmap *c;

	if (rw == B_WRITE) {
		if (pte->pg_fod)
			return (0);
		if (pte->pg_pfnum == 0)
			return (0);
		c = &cmap[pgtocm(pte->pg_pfnum)];
		if (c->c_lock || c->c_intrans)
			return (0);
		if (!dirtycl(pte))
			return (0);
		return (1);
	} else {
		if (pte->pg_fod)
			return (0);
		if (pte->pg_pfnum)
			return (0);
		return (1);
	}
}

/*
 * Fodkluster locates pages adjacent to the argument pages
 * that are immediately available to include in the pagein,
 * and given the availability of memory includes them.
 * It wants to page in a file system block if it can.
 */
int nofodklust = 0;
int fodklcnt[KLMAX];

fodkluster(p, v0, pte0, pkl, dev, pbn, klmax)
	register struct proc *p;
	unsigned v0;
	struct pte *pte0;
	int *pkl;
	dev_t dev;
	daddr_t *pbn;
	int klmax;
{
	register struct fpte *fpte;
	register daddr_t bn;
	register int klsize;
	register unsigned v;
	register int delta;
	daddr_t bnswap;
	int klback, type;
	unsigned vmin, vmax;
	int s;

	if (nofodklust)
		return (v0);
	fodklcnt[0]++;
	*pkl = 1;
	if (freemem < klmax)
		return (v0);
	if (isatsv(p, v0)) {
		type = CTEXT;
		vmin = tptov(p, 0);
		vmax = tptov(p, clrnd(p->p_tsize) - CLSIZE);
	} else {
		type = CDATA;
		vmin = dptov(p, 0);
		vmax = dptov(p, clrnd(p->p_dsize) - CLSIZE);
	}
	fpte = (struct fpte *)pte0;
	bn = *pbn;
	v = v0;
	s = splimp();
	smp_lock(&lk_cmap, LK_RETRY);
	for (klback = 0, klsize = 1, delta = -CLSIZE; klsize<klmax; klsize++) {
		v += delta;
		if (v<vmin || v>vmax		/* page not klusterable? */
		    || ((fpte+=delta)->pg_fod == 0)
		    || (fpte->pg_blkno != (bn+=btodb(delta*NBPG)))
		    || (type == CTEXT && mfind(dev, bn, p->p_textp->x_gptr)))
			if (delta < 0) {
				delta = -delta;	/* try the other direction */
				fpte = (struct fpte *)pte0;
				bn = *pbn;
				v = v0;
				klsize--;
				continue;
			} else
				break;		/* no other direction left */
		if (type == CTEXT) {
			/*
			 * Flush any previous text page use of this
			 * swap device block.
			 */
			bnswap = vtod(p, v, p->p_dmap, p->p_smap);
			if(bnswap != -1)
				if (mfind(swapdev, bnswap, p->p_textp->x_gptr))
				   munhash(swapdev, bnswap, p->p_textp->x_gptr);
		}
		if (memall((struct pte *)fpte, CLSIZE, p, type, v, V_CACHE)==0)
			break;		/* heavy contention: give up */
		if (type == CTEXT) {
			smp_lock(&lk_text, LK_RETRY);
			p->p_textp->x_rssize += CLSIZE;
			distpte(p->p_textp, vtotp(p, v), (struct pte *)fpte);
			smp_unlock(&lk_text);
		} else {
			smp_lock(&lk_p_vm, LK_RETRY);
			p->p_rssize += CLSIZE;
			smp_unlock(&lk_p_vm);
		}
		if (delta < 0)
			klback = klsize;
	}
	smp_unlock(&lk_cmap);
	(void) splx(s);
	*pbn -= klback * btodb(CLBYTES);
	*pkl = klsize;
	fodklcnt[0]--; fodklcnt[klsize - 1]++;
	return (v0 - klback * CLSIZE);
}
