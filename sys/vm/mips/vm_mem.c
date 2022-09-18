#ifndef lint
static	char	*sccsid = "@(#)vm_mem.c	4.1	(ULTRIX)	7/2/90";
#endif lint

/************************************************************************
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
 ************************************************************************/
/*
 *
 *   Modification history:
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
#include "../h/types.h"
#include "../h/kmalloc.h"
#ifdef vax
#include "../machine/mtpr.h"
#endif vax
#include "../machine/psl.h"
extern struct smem smem[];

#ifdef KMEMSTATS
int km_debug = 0;
#endif KMEMSTATS

/*
 * Allocate memory, and always succeed
 * by jolting page-out daemon
 * so as to obtain page frames.
 * To be used in conjunction with vmemfree().
 */
vmemall(pte, size, p, type)
	register struct pte *pte;
	register int size;
	register struct proc *p;
{
	register int m;
	register int s;

#ifdef mips
	XPRINTF(XPR_VM,"enter vmemall",0,0,0,0);
#endif mips
	if (size <= 0 || size > maxmem)
		panic("vmemall size");

	s = splimp();
	while (size > 0) {
		if (freemem < desfree)
			outofmem();
		while (freemem == 0) {
			sleep((caddr_t)&freemem, PSWP+2);
		}
		m = freemem;
		if (m > size) m = size;	/* m = min of freemem and size */
#ifdef mips
		(void) memall(pte, m, p, type, NULL, V_NOOP);
#endif mips
#ifdef vax
		(void) memall(pte, m, p, type);
#endif vax
		size -= m;
		pte += m;
	}
	if (freemem < desfree)
		outofmem();

	splx(s);
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
#ifdef mips
	XPRINTF(XPR_VM,"enter vmemfree",0,0,0,0);
#endif mips

	/* Are we deallocating "u" pages or it's PTEs ? */
	if (count < 0) {
		flg = KMF_UAREA;
		count = -count;
	}

	if (count % CLSIZE)
		panic("vmemfree");

	for (size = 0, pcnt = 0; count > 0; pte += CLSIZE, count -= CLSIZE) {
		if (pte->pg_fod == 0 && pte->pg_pfnum) {
			c = &cmap[pgtocm(pte->pg_pfnum)];
			if (c->c_lock && c->c_type != CSYS) {
				for (j = 0; j < CLSIZE; j++)
					*(int *)(pte+j) &= PG_PROT;
				c->c_gone = 1;
				pcnt += CLSIZE;			
				goto free;
			}
			if (c->c_free) {
				for (j = 0; j < CLSIZE; j++)
					*(int *)(pte+j) &= PG_PROT;
#ifdef vax
				if (c->c_type == CTEXT)
					distpte(&text[c->c_ndx],
						(int)c->c_page, pte);
				/* SHMEM */
				else if (c->c_type == CSMEM)
					distsmpte(&smem[c->c_ndx],
						(int)c->c_page, pte,
						PG_NOCLRM);
#endif vax
				c->c_gone = 1;
				goto free;
			}
			pcnt += CLSIZE;			
			if (size == 0)
				spte = pte;
			size += CLSIZE;
			continue;
		}
#ifdef notdef /* 001 */
		/* Don't do anything with mapped ptes */
		if (pte->pg_fod && pte->pg_v)
			goto free;
#endif
		if (pte->pg_fod) {
#ifdef notdef /* 001 */
			fileno = ((struct fpte *)pte)->pg_fileno;
			if (fileno < NOFILE)
				panic("vmemfree vread");
#endif notdef
			for (j = 0; j < CLSIZE; j++)
				*(int *)(pte+j) &= PG_PROT;
		}
free:
		if (size) {
			memfree(spte, size, flg);
			size = 0;
		}
	}
	if (size)
		memfree(spte, size, flg);
	return (pcnt);
}

/*
 * Unlink a page frame from the free list -
 *
 * Performed if the page being reclaimed
 * is in the free list.
 */
munlink(pf)
	unsigned pf;
{
	register int next, prev;

#ifdef mips
	XPRINTF(XPR_VM,"enter munlink",0,0,0,0);
#endif mips
	next = cmap[pgtocm(pf)].c_next;
	prev = cmap[pgtocm(pf)].c_prev;
	cmap[prev].c_next = next;
	cmap[next].c_prev = prev;
	cmap[pgtocm(pf)].c_free = 0;
	if (freemem < minfree)
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

#ifdef mips
	XPRINTF(XPR_VM,"enter pfclear",0,0,0,0);
#endif mips
	/* If reclaimable, then clear associated PTEs */
	if (c->c_gone == 0 && c->c_type != CSYS) {
		if (c->c_type == CTEXT)
			rp = text[index].x_caddr;
		else
			rp = &proc[index];
		if(c->c_type != CSMEM)
			while (rp->p_flag & SNOVM)
				rp = rp->p_xlink;
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
		zapcl(rpte, pg_pfnum) = 0;
#ifdef vax
		if (c->c_type == CTEXT)
			distpte(&text[index], (int)c->c_page,
							rpte);
		else if (c->c_type == CSMEM)
			distsmpte(&smem[index],
					(int)c->c_page, rpte,
					PG_NOCLRM);
#endif vax
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
#ifdef	mips 
memall(pte, size, p, type, v, flag)
#endif	mips
#ifdef	vax
memall(pte, size, p, type)
#endif	vax
	register struct pte *pte;
	int size;
	struct proc *p;
#ifdef mips
	unsigned int v;
	int flag;
#endif mips
{
	register struct cmap *c;
	register int i, j;
	register unsigned pf;
	register int next, curpos;
	int s;
#ifdef mips
	unsigned mask;
        int class_match;
#endif mips
	
#ifdef mips
	XPRINTF(XPR_VM,"enter memall",0,0,0,0);
#endif mips
	if (size % CLSIZE)
		panic("memall");
	s = splimp();

	/* Insure that enough free memory exists to make allocation */
	if (size > freemem) {
		splx(s);
		return (0);
	}
	trace(TR_MALL, size, u.u_procp->p_pid);

	/* Allocation loop: by page cluster */
	for (i = size; i > 0; i -= CLSIZE) {
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
		c = &cmap[curpos];
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
		if(c->c_free == 0)
			panic("dup mem alloc");

		if (cmtopg(curpos) > maxfree)
			panic("bad mem alloc");

		/*
		 * If reclaimable, then clear encumberances
		 */
		pfclear(c);

		/* 
		 * Initialize CMAP entry
		 */
		switch (type) {

		case CSYS:
			c->c_ndx = p->p_ndx;
			break;

		case CTEXT:
			c->c_page = vtotp(p, ptetov(p, pte));
			c->c_ndx = p->p_textp - &text[0];
			break;

		case CDATA:
			c->c_page = vtodp(p, ptetov(p, pte));
			c->c_ndx = p->p_ndx;
			break;

		case CSMEM: /* SHMEM */
			c->c_page = pte - ((struct smem *)p)->sm_ptaddr;
			c->c_ndx = (struct smem *)p - &smem[0];
			break;

		case CSTACK:
			c->c_page = vtosp(p, ptetov(p, pte));
			c->c_ndx = p->p_ndx;
			break;
		}
		
		pf = cmtopg(curpos);
		for (j = 0; j < CLSIZE; j++) {
#ifdef mips
			if (type==CTEXT)
				clean_icache(PHYS_TO_K0(ptob(pf)), NBPG);
			*(int *)pte++ = pf++ << PTE_PFNSHIFT;
#endif mips
#ifdef vax
			*(int *)pte++ = pf++;
#endif vax
		}
		c->c_free = 0;
		c->c_gone = 0;
		if (c->c_intrans || c->c_want)
			panic("memall intrans|want");
		c->c_lock = 1;
		c->c_type = type;
	}
	splx(s);
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
	int s;
	void vcleanu();

#ifdef mips
	XPRINTF(XPR_VM,"enter memfree",0,0,0,0);
#endif mips
	if (size % CLSIZE)
		panic("memfree");
	if (freemem < CLSIZE * KLMAX)
		wakeup((caddr_t)&freemem);
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
		if (flg && c->c_type != CSYS) {
			for (j = 0; j < CLSIZE; j++)
				*(int *)(pte+j) &= PG_PROT;
			c->c_gone = 1;
		}
		s = splimp();

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
		} else if (flg == KMF_DETACH && c->c_blkno == 0) {
			next = cmap[CMHEAD].c_next;
			cmap[next].c_prev = i;
			c->c_prev = CMHEAD;
			c->c_next = next;
			cmap[CMHEAD].c_next = i;
			freemem += CLSIZE;
		} else {
			prev = cmap[CMHEAD].c_prev;
			cmap[prev].c_next = i;
			c->c_next = CMHEAD;
			c->c_prev = prev;
			cmap[CMHEAD].c_prev = i;
			freemem += CLSIZE;
		}
		c->c_free = 1;
		splx(s);
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
register s, next;

#ifdef mips
	XPRINTF(XPR_VM,"enter vcleanu",0,0,0,0);
#endif mips
	s = splimp();
	if (ucmap == -1)
		panic("vcleanu");
	if (freemem < CLSIZE * KLMAX)
		wakeup((caddr_t)&freemem);
	next = cmap[CMHEAD].c_next;
	cmap[next].c_prev = eucmap;
	cmap[ucmap].c_prev = CMHEAD;
	cmap[eucmap].c_next = next;
	cmap[CMHEAD].c_next = ucmap;
	freemem += nucmap * CLSIZE;
	ucmap = eucmap = -1;
	nucmap = 0;
	splx(s);
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
	register next, curpos, count;
	int end, head;
	unsigned pf;
	int s;

#ifdef mips
	XPRINTF(XPR_VM,"enter pfalloc",0,0,0,0);
#endif mips
	if (type != CSYS)
		panic("pfalloc: type");
	s = splimp();
	if (freemem < npfc * CLSIZE) {
		splx(s);
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
		splx(s);
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
	splx(s);
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
 *		been currupted.
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
	
#ifdef mips
	XPRINTF(XPR_VM,"enter pffree",0,0,0,0);
#endif mips
	if (pfn < firstfree || pfn > maxfree || npfc != 1)
		panic("pffree: bad mem free");
	i = pgtocm(pfn);
	c = &cmap[i];
	if (c->c_free)
		panic("pffree: dup mem free");
	if (freemem < CLSIZE * KLMAX)
		wakeup((caddr_t)&freemem);
	s = splimp();
	next = cmap[CMHEAD].c_next;
	cmap[next].c_prev = i;
	c->c_prev = CMHEAD;
	c->c_next = next;
	cmap[CMHEAD].c_next = i;
	c->c_free = 1;
	freemem += CLSIZE;
#ifdef KM_STATS
	km_stats.tot_pffree++;
#endif KM_STATS
	splx(s);
}


/*
 * Enter clist block c on the hash chains.
 * It contains file system block bn from device dev.
 * Dev must either be a mounted file system or the swap device
 */
mhash(c, dev, bn)
	register struct cmap *c;
	dev_t dev;
	daddr_t bn;
{
	register int i = CMHASH(bn);
	register struct mount *mp;
	register struct text *xp;
	

#ifdef mips
	XPRINTF(XPR_VM,"enter mhash",0,0,0,0);
#endif mips
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
	
#ifdef mips
	XPRINTF(XPR_VM,"enter munhash",0,0,0,0);
#endif mips
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

#ifdef mips
	XPRINTF(XPR_VM,"enter maunhash",0,0,0,0);
#endif mips
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
	int si = splimp();
	
#ifdef mips
	XPRINTF(XPR_VM,"enter mfind",0,0,0,0);
#endif mips
	if (gp && gp->g_dev == dev) {	/* handle simple case to save */
		if ((mp = gp->g_mp) == NULL) {
			splx(si);
			return((struct cmap *) 0);
		}
		index = gp->g_mp - mount;
	}
	else {
		GETMP(mp, dev)
		if (mp == NULL) {
			splx(si);
			return((struct cmap *) 0);
		}
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
			splx(si);
			return (c1);
		}
		c1 = &cmap[c1->c_hlink];
	}
	splx(si);
	return ((struct cmap *)0);
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

#ifdef mips
	XPRINTF(XPR_VM,"enter mpurge",0,0,0,0);
#endif mips
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
	splx(si);
}

/*
 * Initialize core map
 */
meminit(first, last)
	int first, last;
{
	register int i;
	register struct cmap *c;

#ifdef mips
	XPRINTF(XPR_VM,"enter meminit",0,0,0,0);
#endif mips
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
	register struct pte *pte;
	register struct cmap *c;
#ifdef mips
	register unsigned pf;
#endif mips

#ifdef mips
	XPRINTF(XPR_VM,"enter vslock",0,0,0,0);
#endif mips
	v = btop(base);
	pte = vtopte(u.u_procp, v);
	npf = btoc(count + ((int)base & CLOFSET));
	while (npf > 0) {
		if (pte->pg_v) {
			c = &cmap[pgtocm(pte->pg_pfnum)];
#ifdef mips
			if (c->c_lock) {
#endif mips
			MLOCK(c);
#ifdef mips
				MUNLOCK(c);
				continue;
#endif mips
		} 
#ifdef mips
			MLOCK(c);
		} 
#endif mips
		else
			pagein(ctob(v), 1);	/* return it locked */
#ifdef mips
#ifdef CACHETRICKS
		if (pte->pg_n == 0) {
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
}

/* 
 * Unlock a virtual segment.
 */
vsunlock(base, count, rw)
	caddr_t base;
{
	register struct pte *pte;
	register int npf;
	register struct cmap *c;

#ifdef mips
	XPRINTF(XPR_VM,"enter vsunlock",0,0,0,0);
#endif mips
	pte = vtopte(u.u_procp, btop(base));
	npf = btoc(count + ((int)base & CLOFSET));
	while (npf > 0) {
		c = &cmap[pgtocm(pte->pg_pfnum)];
		MUNLOCK(c);
		if (rw == B_READ)	/* Reading from device writes memory */
			pte->pg_m = 1;
		pte += CLSIZE;
		npf -= CLSIZE;
	}
}

struct kmembuckets bucket[NBUCKET];
struct kmemusage *kmemusage;
int kmemu[KM_LAST];

#ifdef KMEMSTATS
struct kmemstats kmemstats[KM_LAST];

struct M_request {
	int size;
	int type;
	int flags;
} M_requests[KM_REQSIZ];	/* burn 12 KB */

int M_request_debug = 1;
int M_request_location = 0;

#endif KMEMSTATS

/* allocate system virtual and pte space */
caddr_t 
get_sys_ptes(npg, pte)
int npg;
struct pte **pte;
{
	register long alloc = 0;
	register int fraction;
	register int s = splimp();

#ifdef mips
	XPRINTF(XPR_VM,"enter get_sys_ptes",0,0,0,0);
#endif mips

	npg += (fraction = npg % CLSIZE) ? (CLSIZE - fraction) : 0 ;
	alloc = rmalloc(kmemmap, npg);
	splx(s);
	if (alloc <= 0) {
#ifdef KMEMSTATS
		if(km_debug) 
			cprintf("get_sys_ptes: rmalloc: npg (%d)\n", npg);
#endif KMEMSTATS
		return((caddr_t)NULL);
	}

	*pte  = &kmempt[alloc];
	return((caddr_t)kmemxtob(alloc));
}

/*
 * Duplicate a malloced area
 */
km_memdup(va)
register caddr_t va;
{
	register struct kmemusage *kup = (struct kmemusage *)NULL;
	register int s = splimp();

/* SMP: just have to lock usage struct (kup) */
#ifdef mips
	XPRINTF(XPR_VM,"enter km_memdup",0,0,0,0);
#endif mips
	kup = btokup(va);
	if (kup->ku_indx >= KMBUCKET) {
		kup->ku_refcnt++;
	} else {
		cprintf("km_memdup: va = 0x%x index = %d\n",
		va, kup->ku_indx);
		panic("km_memdup not a cluster");
	}
	splx(s);
}

/*
 * Allocate a block of memory
 */
caddr_t
km_alloc(size, type, flags)
	unsigned long size;
	long type, flags;
{
	register caddr_t va = (caddr_t)NULL;
	register long alloc = 0;
	register struct kmembuckets *kbp = (struct kmembuckets *)NULL;
	register struct kmemusage *kup = (struct kmemusage *)NULL;
#ifdef KMEMSTATS
	register struct kmemstats *ksp = (struct kmemstats *)NULL;
#endif KMEMSTATS
	int (*mem)(), memall(), vmemall();
	long indx, npg, allocsize;
	int s = splimp();

	mem = (flags & KM_NOWAIT) ? memall : vmemall;

#ifdef KMEMSTATS
	if(size <= 0) 
		panic("km_alloc: bad size");
#ifdef vax
	/* make sure sleep option is not set if on interrupt stack */
	if( !(flags & KM_NOWAIT) && (movpsl() & PSL_IS) )
		panic("km_alloc: SLEEP on interrupt stack");
#endif vax

	ksp = &kmemstats[type];
	if(ksp->ks_inuse >= ksp->ks_limit)
		goto bad;

	if(M_request_debug) {
		if(M_request_location >= KM_REQSIZ) 
			M_request_location = 0;
		M_requests[M_request_location].size = size;
		M_requests[M_request_location].type = type;
		M_requests[M_request_location++].flags = flags;
	}
#endif KMEMSTATS

	indx = BUCKETINDX(size);

#ifdef KMEMSTATS
	if(indx < MINBUCKET || indx > NBUCKET)
	        panic("km_alloc: bad index");
#endif KMEMSTATS

	kbp = &bucket[indx];

/* SMP: need to lock bucket chain (kbp->kb_next) */
	if(kbp->kb_next == (caddr_t)NULL || (flags & KM_CONTIG) ) {
		register struct pte *pte = (struct pte *)NULL;
		register int i = 0;

		if(size <= MAXALLOCSAVE) 
			allocsize = 1 << indx;
		 else 
			allocsize = roundup(size, CLBYTES);
		npg = clrnd(btoc(allocsize));
		if((flags & KM_NOWAIT) && freemem < npg) 
			goto bad;

		/* get the GUARD ptes allocated too */
		while((alloc = rmalloc(kmemmap, npg+GUARDPAGES)) == 0) {
			if(flags & KM_NOWAIT) 
				goto bad;
			sleep((caddr_t)&kmemmap, PSWP+2);
		}

		pte = (struct pte *) &kmempt[alloc];
		if(flags & KM_CONTIG) {
			register unsigned pfn = 0;
			if((pfn = pfalloc(CSYS, npg/CLSIZE)) == 0) 
				goto bad;
			for (i = npg; i-- ; pte++, pfn++) {
#ifdef KMEMSTATS
			/* pte's are cleared in km_free */
				if(*(int *)pte != 0) 
					panic("km_alloc: bad pte1");
#endif KMEMSTATS
#ifdef mips
				pte->pg_pfnum = pfn;
#endif mips
#ifdef vax
				*(int *) pte |=  pfn;
#endif vax
			}
		} else {
#ifdef KMEMSTATS
			/* pte's are cleared in km_free */
			for (i = 0; i < npg+GUARDPAGES; i++, pte++) {
				if(*(int *)pte != 0) 
					panic("km_alloc: bad pte2");
			}
#endif KMEMSTATS
			if(mem(&kmempt[alloc], npg, &proc[0], CSYS) == 0) 
				goto bad;
		}

		va = (caddr_t) kmemxtob(alloc);
#ifdef mips
		vmaccess(&kmempt[alloc], va, npg, DO_CACHE);
#endif	mips
#ifdef vax
		vmaccess(&kmempt[alloc], va, npg);
#endif vax
#ifdef KMEMSTATS
		kbp->kb_total += kbp->kb_elmpercl;
#endif	KMEMSTATS
/* SMP: kup needs to be locked (usage structure) */
		kup = btokup(va);
		kup->ku_indx = indx;
		kup->ku_refcnt = 0;
		if(allocsize > MAXALLOCSAVE) {
			kup->ku_pagecnt = npg;
			goto done;
		}
#ifdef KMEMSTATS
		kup->ku_freecnt = kbp->kb_elmpercl;
		kbp->kb_totalfree += kbp->kb_elmpercl;
#endif KMEMSTATS
		if(kbp->kb_next == NULL) {
			register caddr_t cp = (caddr_t) NULL;
			kbp->kb_next = va + (npg * NBPG) - allocsize;
			for(cp = kbp->kb_next; cp > va; cp -= allocsize) 
				*(caddr_t *)cp = cp - allocsize;
			*(caddr_t *)cp = NULL;
		} else {
			*((caddr_t *)va) = kbp->kb_next;
			kbp->kb_next = va;
		}
	}

	va = kbp->kb_next;
	if( !IS_KMEM_VA(va)) 
		panic("km_alloc: bucket corruption");

	kbp->kb_next = *(caddr_t *)va;
	kup = btokup(va);
#ifdef KMEMSTATS
	if(kup->ku_indx != indx) 
		panic("km_alloc: wrong bucket");
	if(kup->ku_freecnt == 0)
		panic("km_alloc: lost data");

	kup->ku_freecnt--;
	kbp->kb_totalfree--;
done:
	kbp->kb_calls++;
	ksp->ks_inuse++;
	ksp->ks_calls++;
	if(ksp->ks_inuse > ksp->ks_maxused) {
		ksp->ks_maxused = ksp->ks_inuse;
	}
#else	KMEMSTATS
done:
#endif	KMEMSTATS
	kup->ku_refcnt++;
	kmemu[type]++;
	splx(s);
	if(flags & KM_CLEAR) 
		blkclr(va, size);
	return (va);
bad:
	if(alloc > 0) {
		/* remember to put back the GUARDPAGE ptes */
		rmfree(kmemmap, npg+GUARDPAGES, alloc);
		wakeup((caddr_t)&kmemmap);
	}
	splx(s);
	return (NULL);
}

/*
 * Free a block of memory allocated by malloc.
 */
void
km_free(addr, type)
	caddr_t addr;
	long type;
{
	register struct kmemusage *kup = (struct kmemusage *)NULL;
	register int s = splimp();

#ifdef mips
	XPRINTF(XPR_VM,"enter km_free",0,0,0,0);
#endif mips
	if( !IS_KMEM_VA(addr)) 
  		panic("km_free: bad addr\n");

	kup = btokup(addr);
	if(kup->ku_indx < MINBUCKET || kup->ku_indx > NBUCKET)
	        panic("km_free: bad index");

/* SMP: lock usage (kup) struct and bucket */
#ifdef KMEMSTATS
	if(--kup->ku_refcnt < 0) 
		panic("km_free: multiple frees");
#endif	KMEMSTATS
	if(kup->ku_indx < KMBUCKET || kup->ku_refcnt == 0) {
		if(kup->ku_indx > MAXBUCKETSAVE) {
			register long alloc = btokmemx(addr);
			register int i = 0;

			(void) memfree(&kmempt[alloc], kup->ku_pagecnt, 0);
			/* 
			 * set pte's (and GUARD's) to NOACCESS 
			 * and invalidate tb
			 */
			for (i = 0; i < kup->ku_pagecnt+GUARDPAGES; i++) {
				*((int *)&kmempt[alloc+i]) = 0;
#ifdef vax
				mtpr(TBIS, addr);
#endif vax
				addr += NBPG;
			}

			/* remember to put back the GUARDPAGE ptes */
			(void) rmfree(kmemmap, (long)kup->ku_pagecnt+GUARDPAGES, alloc);
			wakeup((caddr_t)&kmemmap);
			kup->ku_indx = kup->ku_pagecnt = 0;
		} else  {
			register struct kmembuckets *kbp = (struct kmembuckets *)NULL;

			kbp = &bucket[kup->ku_indx];
#ifdef KMEMSTATS
			kup->ku_freecnt++;
			if(kup->ku_freecnt >= kbp->kb_elmpercl) {
				if(kup->ku_freecnt > kbp->kb_elmpercl) {
					panic("km_free: multiple frees");
				} else if(kbp->kb_totalfree > kbp->kb_highwat) {
					kbp->kb_couldfree++;
				}
			}
			kbp->kb_totalfree++;
#endif KMEMSTATS
			*(caddr_t *)addr = kbp->kb_next;
			kbp->kb_next = addr;
		}
#ifdef KMEMSTATS
		kmemstats[type].ks_inuse--;
#endif KMEMSTATS
		kmemu[type]--;
	}
	splx(s);
}

#ifdef KMEM_SCRUB
int kmem_pages_freed = 0;	/* how many freed over life of system */

/* This array gives the number of free entries */
/* needed before we try to free some - this needs tuning!*/
int kmem_pages_left[NBUCKET] = {
	 0, 0, 0, 0,
	64,32,16, 8,	/*  16b,  32b,  64b, 128b  */
	 8, 8, 8, 8,	/* 256b, 512b,   1K,   2K  */
	 8, 8, 8, 1,	/*   4K,   8K,  16K,  32K  */
	 1, 1, 1, 1	/*  64K, 128K, 256K, 512K+ */
};
int kmem_scrub_time = 60;	/* in seconds  - tunable */

void
kmem_scrub()
{
	register struct kmembuckets *kbp;
	register struct kmemusage *kup;
	register caddr_t va;
	register int pages;
	register int *nfree;
	register int pieces;
	int s = splimp();

#ifdef mips
	XPRINTF(XPR_VM,"enter kmem_scrub",0,0,0,0);
#endif mips
	/* do the cluster size and up buckets */
	for (nfree = &kmem_pages_left[KMBUCKET], kbp = &bucket[KMBUCKET];
	kbp <= &bucket[MAXBUCKETSAVE]; kbp++, nfree++) {
		for(pieces=0,va = kbp->kb_next;va != NULL;va = *(caddr_t *)va) {
			pieces++;	/* actually just chunks on bucket */
		}
		while (pieces >= *nfree && kbp->kb_next) {
			va = (caddr_t) kbp->kb_next;
			kbp->kb_next = *(caddr_t *)va;
			kup = btokup(va);
			pages = (1<<kup->ku_indx)/NBPG;	/* number to free */
			kmem_pages_freed += pages;	/* count them */
			(void) memfree(&kmempt[btokmemx(va)],pages,0);
			/* remember to put back the GUARDPAGE ptes */
			rmfree(kmemmap, pages+GUARDPAGES,btokmemx(va));
			wakeup((caddr_t)&kmemmap);

			/* zero out usage struct */
			kup->ku_indx = 0;
			kup->ku_refcnt = 0;
			kup->ku_pagecnt = 0;
			pieces--;
		}
	}
#ifdef SMALL_SCRUBBER
	/* do the small buckets (< cluster size) */
	for (nfree = &kmem_pages_left[MINBUCKET], kbp = &bucket[MINBUCKET];
	kbp < &bucket[KMBUCKET]; kbp++, nfree++) {
		for(pieces=0,va = kbp->kb_next;va != NULL;va = *(caddr_t *)va) {
			if((btokup(va))->ku_refcnt == 0)
				pieces++;/* actually just chunks from cluster */
		}
		if(pieces < *nfree ||
		    pieces < CLBYTES/(1<<(btokup(kbp->kb_next)->ku_indx))) {
			continue;
		}
#ifdef notdef
		va = kbp->kb_next;
		cprintf(
	"There are %d chunks free of %d bytes each (total = %d)\n",
	pieces,1<<(btokup(va)->ku_indx),pieces*(1<<(btokup(va)->ku_indx)));
		while (va) {
			if((btokup(va))->ku_refcnt == 0) {
			cprintf("Cluster 0x%x refcnt %d is freeable.\n",
				va, btokup(va)->ku_refcnt);
			}
			va = *(caddr_t *)va;
		}
#endif notdef
	}
#endif SMALL_SCRUBBER
	splx(s);
	timeout(kmem_scrub,(caddr_t)0,kmem_scrub_time*hz);
}
#endif KMEM_SCRUB

/*
 * Initialize the kernel memory allocator
 */
void
kmeminit()
{
#ifdef KMEMSTATS
	register long indx;

#ifdef mips
	XPRINTF(XPR_VM,"enter kmeminit",0,0,0,0);
#endif mips
	if(!powerof2(MAXALLOCSAVE))
		panic("kmeminit: MAXALLOCSAVE not power of 2");
	if(MAXALLOCSAVE > MINALLOCSIZE * 32768)
		panic("kmeminit: MAXALLOCSAVE too big");
	if(MAXALLOCSAVE < CLBYTES)
		panic("kmeminit: MAXALLOCSAVE too small");
#endif KMEMSTATS
	rminit(kmemmap, ((ekmempt - kmempt) - (long)2), (long)2,
		"malloc map", (ekmempt - kmempt));
#ifdef KMEMSTATS
	if(km_debug) {
		cprintf("m_limit = 0x%x; m_name %s; m_size = %d; m_addr 0x%x\n",
		((struct map *)kmemmap)->m_limit,
		((struct map *)kmemmap)->m_name,
		((struct mapent *)kmemmap)->m_size,
		((struct mapent *)kmemmap)->m_addr);
	}

	for (indx = 0; indx < MINBUCKET + 16; indx++) {
		if(indx >= KMBUCKET) 
			bucket[indx].kb_elmpercl = 1;
		else 
			bucket[indx].kb_elmpercl = CLBYTES / (1 << indx);
		bucket[indx].kb_highwat = 5 * bucket[indx].kb_elmpercl;
	}
	for (indx = 0; indx < KM_LAST; indx++) 
		kmemstats[indx].ks_limit = 0x7fffffff;
#endif KMEMSTATS
}
