#ifndef lint
static	char	*sccsid = "@(#)vm_kmalloc.c	4.1	(ULTRIX)	7/2/90";
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
 * 19-Jun-90 -- jmartin
 *	Fixes for "panic: vrelvm rssize"
 *
 * 03-Apr-90	bp/gmm
 *	Changes for pte freeing at the correct ipl
 *
 * 30-Dec-89	bp
 *	Added logic for our memory producers so they don't deadlock when
 *	there isn't any free memory.  
 *
 * 10-Dec-89	bp
 *	Fixed wired bucket algorithm error in km_find.  Changed kmeminit
 *	to use km_wmapsize instead of computed value in vmparam.h which
 *	is not visible to binary modules.
 *
 * 12-Jun-89	bp
 *	Moved kernel memory allocator from vm_mem.c to here.
 *	Substantial changes in allocator to support high water
 *	mark trimming,  allocator resource depletion, SMP TB
 *	consistency and other general enhancements.
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
#include "../machine/psl.h"
#include "../h/cpudata.h"

extern struct lock_data lock_kmtbsync_d;
struct lock_t lk_kmtbsync;

/*
 * B_PROTECTION turns on bucket protection.
 * A bucket can only be written to when B_UNPROT is called.
 * B_PROT disables writes to the buckets.
 */


#if	defined(B_PROTECTION) && defined(KMEM_DEBUG) && defined(vax)
struct kmembuckets bigbucket[(NBPG*2)/sizeof (struct kmembuckets)];
struct pte *bpte;
struct kmembuckets *bucket;
struct kmembuckets *wbucket;
#else
struct kmembuckets bucket[MAXBUCKETSAVE+1];
struct kmembuckets wbucket[MAXBUCKETSAVE+1];
#endif	defined(B_PROTECTION) && defined(KMEM_DEBUG) && defined(vax)


/*
 * BUTRAP provides debugging for a buckets' memory elements.
 * It is machine dependent and currently works for VAXEN only.
 * Before you boot the machine you must patch bui with the
 * bucket index you wish to track.  This code will trap references
 * to a memory element that was freed and an access is being attemped
 * while it is on its free list.
 */


#if	defined(KMEM_DEBUG) && defined(BUTRAP) && defined(vax)

/*
 * Trace depth for freed trapped buffers.
 */

#define	BUTRAP_TD	4
caddr_t bhu, btu;
int bui = -1;
#endif	BUTRAP



#if	defined(B_PROTECTION) && defined(KMEM_DEBUG) && defined(vax)
#define	B_PROT B_PCHANGE(PG_KR)
#define	B_UNPROT B_PCHANGE(PG_KW)

#define	B_PCHANGE(PROT) {						\
	* (int *) bpte &= ~PG_PROT;					\
	* (int *) bpte |= (PROT);					\
	mtpr(TBIS,(caddr_t) (bpte));					\
}
#else
#define	B_PROT
#define	B_UNPROT
#define	B_PCHANGE(PROT)
#endif	defined(B_PROTECTION) && defined(KMEM_DEBUG) && defined(vax)

#ifdef	KMEM_DEBUG

/*
 * Allocator debug memory map usage
 */

caddr_t kmdmap;

/*
 * Dead pte size for DMZ testing
 */

#define	KM_DDMZINT	20
int kmdmzflush = -1;
int kmddmz;

#endif	KMEM_DEBUG 

/*
 * Kernel memory allocator data
 */


struct kmemusage *kmemusage, *uwkmemusage;
int kmemu[KM_LAST];
struct kmemusage *kmemdeadpte;
int kmemneedmap, kmemneedwmap;


/* 
 * allocate system virtual and pte space 
 */

caddr_t 
get_sys_ptes(npg, pte)
int npg;
struct pte **pte;
{
	register long alloc = 0;
	register int fraction;

	npg += (fraction = npg % CLSIZE) ? (CLSIZE - fraction) : 0 ;
	alloc = rmalloc(kmemmap, npg);
	if (alloc <= 0) {
#ifdef KMEM_DEBUG
		if (kmdebug & KM_DGETSYSPTE) 
			cprintf("get_sys_ptes: rmalloc: npg (%d)\n", npg);
#endif KMEM_DEBUG
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
	register struct kmemusage *kup;
	register int s;

	s = splimp();
	smp_lock(&lk_buckets, LK_RETRY);
	kup = btokup(va);
	if (kup->ku_index >= KMBUCKET) {
		kup->ku_refcnt++;
	} else {
		cprintf("km_memdup: va = 0x%x index = %d\n",
		va, kup->ku_index);
		panic("km_memdup not a cluster");
	}
	smp_unlock(&lk_buckets);
	splx(s);
}

#if	defined(mips)
#define	INISR	(CURRENT_CPUDATA->cpu_inisr)
#else
#define	INISR	(movpsl() & PSL_IS)
#endif

/*
 * Allocate a block of memory
 */

extern struct kmemusage *km_alloc_mem();
extern struct kmemusage *km_find();

caddr_t
km_alloc(size, type, flags)
	long size, type, flags;
{
	register caddr_t va = (caddr_t)NULL;
	register struct kmembuckets *kbp;
	register struct kmemusage *kup;
	long index, oindex;
	register int s;
	

	if (!(flags & KM_NOWAIT)) sleep_check(); 

#ifdef	KMEM_DEBUG
	if (kmdebug & KM_DDMZ && ((flags & KM_WIRED) == 0)) {
		if (((flags & KM_INDEX) == 0) && size < CLBYTES) size = CLBYTES;
		else if (size < KMBUCKET) size = KMBUCKET;
	}
#endif	KMEM_DEBUG


	if ((flags & KM_INDEX) == 0) index = BUCKETINDX(size);
	else {
		index = size;
		size = index ? (1 << index) : 0;
	}

#ifdef	mips
	if (index < KMBUCKET) 
#endif	mips
		flags &= ~KM_NOCACHE;

#ifdef KMEM_DEBUG

	/*
	 * KMBUCKET index has to live in KMBUCKET+1
	 */

	if (kmdebug & KM_DCOOKIE && index <= KMBUCKET) index++;

	if (size <= 0) 
		panic("km_alloc: bad size");

	if (index < MINBUCKET || index > NBUCKET)
	        panic("km_alloc: bad index");

	if (kmdebug & KM_DHISTORY) {
		if (kmemd_request_location >= KM_REQSIZ) 
			kmemd_request_location = 0;
		kmemd_requests[kmemd_request_location].size = size;
		kmemd_requests[kmemd_request_location].type = type;
		kmemd_requests[kmemd_request_location++].flags = flags;
	}
#endif KMEM_DEBUG


	if (!size) return 0;

	kbp = (flags & KM_WIRED) ? &wbucket[index] : &bucket[index];
	s = splimp();

	/*
	 * This code assists in low memory conditions where sched or
	 * pageout need some km_alloc resource for purposes of I/O.
	 */

	if (!INISR && (proc_bitmap[0] & 7) && BOOT_CPU &&
		(u.u_procp->p_pid == 0 || u.u_procp->p_pid == 2)) {
		if (flags & KM_NOWAIT) flags |= (KM_SSYS| KM_NWSYS);
		else flags |= (KM_NOWAIT|KM_SSYS);
		oindex = index;
	}

retry:

	/*
	 * Unsaved buckets
	 */

	if (index > MAXBUCKETSAVE || flags & KM_NOCACHE) {
		if ((kup = km_alloc_mem(index,size,flags))) {
			va = kuptova(kup);
			smp_lock(&lk_buckets, LK_RETRY);
			kup->ku_refcnt++;
			smp_unlock(&lk_buckets);
			goto done;
		}
		else goto bad;
	}

#if	defined(KMEM_DEBUG) && defined(BUTRAP) && defined(vax)
	else if (index == bui) {
		smp_lock(&lk_buckets, LK_RETRY);
		if (bhu) {
			va = bhu;
			if (bhu == btu) bhu = (caddr_t) 0;
			else bhu = * (char **) va;
			smp_unlock(&lk_buckets);
			kup = btokup(va);
			* (int *) &kmempt[btokmemx(va)] |= PG_V;
		}
		else {
			smp_unlock(&lk_buckets);
			if ((kup = km_alloc_mem(index,size,flags))) {
				va = kuptova(kup);
				if (index < KMBUCKET) {
					register caddr_t nva = (va + NBPG);

					smp_lock(&lk_buckets, LK_RETRY);
					if (bhu) * (int **) btu = (int *) nva;
					else bhu = nva;
					btu = nva;
					smp_unlock(&lk_buckets);
				}
			}
			else goto done;
		}
	}
#endif	BUTRAP

	/*
	 * Saved buckets
	 */

	else {
		register struct kmemelement *kep;

		smp_lock(&lk_buckets,LK_RETRY);
		kep = kbp->kb_efl;
		if ((struct kmemelement *) kbp == kep) {
			smp_unlock(&lk_buckets);
			if ((kup = km_alloc_mem(index,size,flags))) {
				va = kuptova(kup);
				if (index < KMBUCKET) {
					register int i, j;
					
					j = 1 << index;
					kep = (struct kmemelement *) va;
					kep->ke_fl = kep->ke_bl = kep;
					i = (1 << (CLSHIFT - index)) - 1;
					while (--i) {
						va += j;
						_insque(
						(struct kmemelement *)va,
						(struct kmemelement *)(va - j));
					}
					kup->ku_hele = kep;
					kup->ku_tele = (struct kmemelement *)va;

					smp_lock(&lk_buckets, LK_RETRY);
					B_UNPROT;
					kep->ke_bl = kbp->kb_ebl;
					((struct kmemelement *)va)->ke_fl = 
						(struct kmemelement *) kbp;
					kbp->kb_ebl->ke_fl = kep;
					kbp->kb_ebl = (struct kmemelement *) va;
					va += j;
				}
				else {
					smp_lock(&lk_buckets, LK_RETRY);
					kup->ku_hele = (struct kmemelement *) 0;
					B_UNPROT;
				}
				kbp->kb_total++;
#ifdef	KMEM_DEBUG
				/*
				 * For DMZ we never chain
				 */

				if ((kmdebug & KM_DDMZ) == 0) {
#endif	KMEM_DEBUG
				kup->ku_nku = kbp->kb_kup;
				kbp->kb_kup = kup;
#ifdef	KMEM_DEBUG
				}
#endif	KMEM_DEBUG
			}
			else goto bad;
		}
		else {
			B_UNPROT;
			_remque(kep);
			kup = btokup(kep);
			if (kup->ku_hele == kup->ku_tele)
				kup->ku_hele = (struct kmemelement *) 0;
			else kup->ku_hele = kep->ke_fl;
			va = (caddr_t) kep;
		}
		kup->ku_refcnt++;
		B_PROT;
#ifdef	KMEM_DEBUG
		if (kmdebug & KM_DSANITY) {
			register int mapi = (va - kmembase) >> MINBUCKET;

			*(kmdmap + (mapi >> 3)) |= (1 << (mapi & (NBBY - 1)));
		}
#endif	KMEM_DEBUG
		smp_unlock(&lk_buckets);
	}

	if (!IS_KMEM_VA(va)) 
		panic("km_alloc: bucket corruption");


#ifdef KMEM_DEBUG
	if (kmdebug & KM_DCOOKIE && index <= KMBUCKET) 
		*(int *) (va + (1 << (index - 1))) = KM_DPATTERN;

	if (kup->ku_index != index) 
		panic("km_alloc: wrong bucket");

	if (kmdebug & KM_DSBUCKET) {
		register struct kmemd_bs *kbs = &kmemd_bs[index];

		kbs->bs_total++;
		kbs->bs_inuse++;
		if (kbs->bs_inuse > kbs->bs_maxused)
			kbs->bs_maxused = kbs->bs_inuse;
	}
done:
	if (kmdebug & KM_DSTYPE) {
		register struct kmemd_ts *kts = &kmemd_ts[type];

		kts->ts_inuse++;
		kts->ts_total++;
		if (kts->ts_inuse > kts->ts_maxused)
			kts->ts_maxused = kts->ts_inuse;
	}
#else	KMEM_DEBUG
done:
#endif	KMEM_DEBUG
	kmemu[type]++;
	(void) splx(s);
	if (flags & KM_CLEAR) 
		blkclr(va, size);
	return (va);
bad:
#ifdef	KMEM_DEBUG
	kmemd_ts[type].ts_allocfail++;
	if (index <= MAXBUCKETSAVE) kmemd_bs[index].bs_allocfail++;
	kmemd_gs.gs_hardmem++;
#endif	KMEM_DEBUG
	/*
	 * Code to avoid deadlock for pid0 and pid2.   If freemem goes to 0
	 * and some facility calls km_alloc on behalf of pageout or sched
	 * and we can't obtain the space, then try increasing the allocator 
	 * bucket index incrementally up to MAXBUCKETSAVE.
	 * If this fails we sleep hoping someone will free some allocator
	 * space and retry the entire scheme again.  
	 */

	if (flags & KM_SSYS) {
		if (++index > MAXBUCKETSAVE) {
			if ((flags & KM_NWSYS) == 0) {
				index = oindex;
				(void) sleep((caddr_t)&lbolt,PSWP);
				goto retry;
			}
		}
		else goto retry;
	}
	else if (((flags & KM_WIRED) == 0) && km_hwmscan(KM_HWMHARD)) 
		km_intr_cpus();
	(void) splx(s);
	return (NULL);
}


/*
 * Allocate memory for a bucket
 * Assumptions:
 *	1) Called at splimp
 *	2) SMP lk_bucket not held
 */

struct kmemusage *
km_alloc_mem(index,size,flags)
	long index, size;
	register int flags;
{
	long pindex;
	int npg, allocsize;
	caddr_t va;
	register struct kmemusage *kup = (struct kmemusage *) 0;
	register struct map **mp;
	register int *rp, s;
	

	if ((flags & KM_WIRED) == 0) {
		mp = &kmemmap;
		rp = &kmemneedmap;
	}
	else {
		mp = &kmemwmap;
		rp = &kmemneedwmap;
	}

	if (index <= MAXBUCKETSAVE) 
		allocsize = 1 << ((index < KMBUCKET) ? KMBUCKET : index);
	else
		allocsize = roundup(size,CLBYTES);

	npg = btoc(allocsize);


	if (index <= KMBUCKET) flags &= ~KM_CONTIG;
	

	while ((pindex = rmalloc(*mp, npg + GUARDPAGES)) == 0) {
		if (flags & (KM_NOCACHE | KM_CONTIG) || 
			(kup = km_find(index,flags)) == (struct kmemusage *)0) {
			if ((flags & KM_NOWAIT) == 0) {
				s = splclock();
				smp_lock(&lk_kmtbsync, LK_RETRY);
				if (!(flags & KM_WIRED) || index <= KMBUCKET) {
					(*rp)++;
					(void) sleep_unlock((caddr_t) mp, 
						PSWP+2, &lk_kmtbsync);
				}
				else panic("km_alloc_mem: no wired map");
				(void) splx(s);
				continue;
			}
			else return (struct kmemusage *) 0;
		}
		else return kup;
	}


	if (flags & KM_CONTIG) {
		register long pfn;

		if ((pfn = pfalloc(CSYS, npg/CLSIZE)) == 0) goto done;
		else {
			register int i;
			register struct pte *pte = &kmempt[pindex];

			for (i = 0; i < npg; i++)
#ifdef	mips
				pte++->pg_pfnum = pfn++;
#endif	mips
#ifdef	vax
				*(int *) pte++ |= pfn++;
#endif	vax
		}
	}
	else if (memall(&kmempt[pindex], npg, &proc[0], -CSYS, 
	NULL, V_NOOP) == 0) {
		if (flags & KM_NOCACHE || !(kup = km_find(index,flags))) {
			if ((flags & KM_NOWAIT) == 0) 
				vmemall(&kmempt[pindex], npg, &proc[0], 
				-CSYS, 0, 0);
			else goto done;
		}
		else goto done;
	}
		
	va = kmemxtob(pindex);	

	if (flags & KM_NOCACHE) {
		vmaccess(&kmempt[pindex], va, npg, NO_CACHE);
		index = KMNCBUCKET;
	}
	else vmaccess(&kmempt[pindex], va, npg, DO_CACHE);

	kup = &kmemusage[pindex >> CLSIZELOG2];
	kup->ku_index = index;
	kup->ku_pagecnt = npg;
	kup->ku_refcnt = 0;
	return kup;
done:
	rmfree(*mp, npg + GUARDPAGES, pindex);

	s = splclock();
	smp_lock(&lk_kmtbsync, LK_RETRY);
	if (*rp) {
		*rp = 0;
		(void) wakeup((caddr_t) rp);
	}
	smp_unlock(&lk_kmtbsync);
	(void) splx(s);
	return kup;
}

/*
 * Attempt to steal a kmemusage's associated memory
 */

struct kmemusage *
km_find(index,flags)
	register long index;
	int flags;
{
	register int i, limit;
	int pneed;
	register struct kmemusage *kup, *pkup, *fkup;
	register struct kmembuckets *kbp;

#ifdef	KMEM_DEBUG
	if (kmdebug & (KM_DDMZ|KM_DDISABLE)) return (struct kmemusage *) 0;
	if (kmdebug & KM_DPKMFIND) printf("kmd in km_find\n");
	kmemd_gs.gs_kmfind++;
#endif	KMEM_DEBUG

	if (index <= KMBUCKET) {
		i = MINBUCKET;
		pneed = 1;
	}
	else if (index < MAXBUCKETSAVE && ((flags & KM_WIRED) == 0)) {
		i =  index + 1;
		pneed = 1 << (index - KMBUCKET);
	}
	else return (struct kmemusage *) 0;

	if (flags & KM_WIRED) {
		kbp = &wbucket[i];
		limit = KMBUCKET;
	}
	else {
		kbp = &bucket[i];
		limit = MAXBUCKETSAVE;
	}

	for (; i <= limit; i++, kbp++) {
		if (index == i || kbp->kb_total <= kbp->kb_hwm) continue;
		pkup = (struct kmemusage *) 0;
		smp_lock(&lk_buckets,LK_RETRY);
		fkup = kup = kbp->kb_kup;
		kbp->kb_kup = (struct kmemusage *) 0;
		smp_unlock(&lk_buckets);
		while (kup) 
		if (kup->ku_refcnt == 0) {
			register struct kmemusage *ckup;

			smp_lock(&lk_buckets, LK_RETRY);
			if (kup->ku_refcnt) {
				smp_unlock(&lk_buckets);
				goto stolen;
			}
			KM_RMELEMENTS(kup);
			kbp->kb_total--;
			smp_unlock(&lk_buckets);
			if (pkup)  ckup = pkup->ku_nku = kup->ku_nku;
			else ckup = fkup = kup->ku_nku;
			if (fkup) {
				while (ckup) {
					pkup = ckup;
					ckup = ckup->ku_nku; 
				}
				smp_lock(&lk_buckets, LK_RETRY);
				if (pkup) pkup->ku_nku = kbp->kb_kup;
				else fkup->ku_nku = kbp->kb_kup;
				kbp->kb_kup = fkup;
				smp_unlock(&lk_buckets);
			}
			goto gotone;
		}
		else {
stolen:
			pkup = kup;
			kup = kup->ku_nku;
		}

		/*
		 * None available in this bucket
		 * Must reattach kmemusage list
		 */

		smp_lock(&lk_buckets, LK_RETRY);
		if (fkup) {
			if (pkup) pkup->ku_nku = kbp->kb_kup;
			else fkup->ku_nku = kbp->kb_kup;
			kbp->kb_kup = fkup;
		}
		smp_unlock(&lk_buckets);
		
	}
gotone:

	if (i > limit || !kup)  return (struct kmemusage *) 0;

	/*
	 * Found some free memory
	 */

#ifdef	KMEM_DEBUG
	kmemd_gs.gs_bstolen++;
	if (kmdebug & KM_DSBUCKET) 
		kmemd_bs[kup->ku_index].bs_stolen++;
	if (kmdebug & KM_DPFINDSTOLEN) 
		printf("km_find stolen\n");
	if (kup->ku_refcnt) panic("km_find: ref count not zero");
#endif	KMEM_DEBUG

	if (i > KMBUCKET) {
		int pgot, pfree;
		register struct kmemusage *okup;


		pgot = 1 << (i - KMBUCKET);
		pfree = pgot - pneed;
		okup = kup;
		kup = kup + pfree;
		pfree <<= CLSIZELOG2;
		smp_lock(&lk_cmap, LK_RETRY);
		(void) memfree(&kmempt[kuptokmemx(okup)], pfree, 0);
		smp_unlock(&lk_cmap);
		okup->ku_guard = 0;
		okup->ku_pagecnt = pfree;
		if (km_dead_pte(okup)) (void) km_intr_cpus();
		kup->ku_pagecnt = pneed << CLSIZELOG2;
	}
	kup->ku_index = index;
	kup->ku_refcnt = 0;
	return kup;
}

/*
 * Free a block of memory allocated by kernel allocator.
 */

void
km_free(addr, type)
	register caddr_t addr;
	long type;
{
#if	defined(KMEM_DEBUG) && defined(BUTRAP) && defined(vax)
	register int *fp;
#endif	BUTRAP
	register struct kmemusage *kup;
	register int s;

	if (!IS_KMEM_VA(addr)) panic("km_free: bad addr\n");

	kup = btokup(addr);
	if (kup->ku_index < MINBUCKET || kup->ku_index >= NBUCKET
#ifdef	mips
	&& (kup->ku_index != KMNCBUCKET || !kmempt[kuptokmemx(kup)].pg_n)
#endif	mips
			)
	        panic("km_free: bad index");


	s = splimp();
	smp_lock(&lk_buckets, LK_RETRY);
	if (--kup->ku_refcnt < 0) panic("km_free: bad reference count");
	if (kup->ku_index < KMBUCKET || kup->ku_refcnt == 0) {
		if (kup->ku_index > MAXBUCKETSAVE) {
			register long alloc = btokmemx(addr);

			smp_unlock(&lk_buckets);
			smp_lock(&lk_cmap, LK_RETRY);
			(void) memfree(&kmempt[alloc], kup->ku_pagecnt, 0);
			smp_unlock(&lk_cmap);

			kup->ku_guard = 1;
			if (km_dead_pte(kup)) (void) km_intr_cpus();
		} 
#if	defined(KMEM_DEBUG) && defined(BUTRAP) && defined(vax)
		else if (kup->ku_index == bui) {
			register int j = BUTRAP_TD;
			register int *tbp = (int *) (addr + 4);

			*tbp++ = (int) (u.u_procp);
			asm("movl fp,r10");
			while (j--) {
				*tbp++ = *(fp + 4);
				fp =  (int *) (*(fp + 3));
			}
			if (bhu == NULL) {
				bhu =  btu = addr;
				* (int *) &kmempt[btokmemx(addr)] &= ~ PG_V;
				mtpr(TBIS,addr);
			}
			else {
				register caddr_t tmp;

				tmp = btu;
				* (int *) &kmempt[btokmemx(tmp)] |= PG_V;
				* (char **) tmp = addr;
				btu = addr;
				* (int *) &kmempt[btokmemx(tmp)] &= ~PG_V;
				* (int *) &kmempt[btokmemx(addr)] &= ~PG_V;
				mtpr(TBIS,tmp);
				mtpr(TBIS,addr);
			}
			smp_unlock(&lk_buckets);
			tbsync();
		}
#endif	BUTRAP
		else  {
#ifdef	KMEM_DEBUG
			register struct kmembuckets *kbp;
			kbp = (kup <= uwkmemusage) ? &bucket[kup->ku_index] :
				&wbucket[kup->ku_index];

			if (kmdebug & KM_DSTYPE) 
				kmemd_ts[type].ts_inuse--;
			if (kmdebug & KM_DSBUCKET)
				kmemd_bs[kup->ku_index].bs_inuse--;
			if (kmdebug & KM_DDMZ && kup <= uwkmemusage) {
				kmd_dmz(kup);
				goto kdmzdone;
			}

			/*
			 * Check alignment for tiny buckets.
			 */

			if (kup->ku_index < KMBUCKET) {
			if (((1 << kup->ku_index) - 1) & (unsigned int) addr) {
				printf("km_free: addr 0x%x  kup 0x%x\n",
					addr,kup);
				panic("km_free bogus alignment\n");
			}
			}

			/*
			 * Look for duplicate frees
			 */

			if (kmdebug & KM_DSANITY) {
				register int mapi = 
					(addr - kmembase) >> MINBUCKET;
				register caddr_t mape = kmdmap +
					(mapi >> 3);
				register int mask = 1 << (mapi &  (NBBY - 1));

				if (*mape & mask) *mape &= ~mask;
				else panic("km_free: dup free");
			}

			/*
			 * Look for cookie and zap memory
			 */

			if (kmdebug & KM_DCOOKIE && kup->ku_index <= KMBUCKET) {
				if (* (int *) (addr + (1 < (kup->ku_index - 1)))
					!= KM_DPATTERN)
					(void) panic("km_free: bad cookie");
					(void) blkclr(addr, 
						1 << (kup->ku_index - 1));
			}
#endif	KMEM_DEBUG
			B_UNPROT;
			if (kup->ku_hele == (struct kmemelement *) 0) {
#ifndef	KMEM_DEBUG
				register struct kmembuckets *kbp;
				kbp = (kup <= uwkmemusage) ?
					&bucket[kup->ku_index] :
					&wbucket[kup->ku_index];
#endif	!KMEM_DEBUG
				
				_insque((struct kmemelement *)addr,
					(struct kmemelement *)kbp);
				kup->ku_hele = (struct kmemelement *) addr;
			}
			else _insque((struct kmemelement *)addr,kup->ku_tele);
			B_PROT;
			kup->ku_tele = (struct kmemelement *) addr;
			smp_unlock(&lk_buckets);
		}
#ifdef	KMEM_DEBUG
kdmzdone:
#endif	KMEM_DEBUG
		kmemu[type]--;
	}
	else smp_unlock(&lk_buckets);
	splx(s);
}


/*
 * Prepare to start TB consistency of
 * kernel memory allocator pte(s) that
 * no longer have associated memory.
 */

int
km_dead_pte(kup)
	register struct kmemusage *kup;
{
	register caddr_t va = kuptova(kup);
	register struct pte *pte = &kmempt[kuptokmemx(kup)];
	register int i, cpindex, cpident;
	register struct cpudata *pcpu;

#ifdef	KMEM_DEBUG
	kmemd_gs.gs_tbsync++;
	if (kmdebug & KM_DPDEADPTE)
		printf("kmd in km_dead_pte\n");
	if (kup > uwkmemusage) panic("km_dead_pte: wired memory");
#endif	KMEM_DEBUG

	for (i = 0; i < kup->ku_pagecnt; i++, va += NBPG) {
		* (int *) pte++ = 0;
#ifdef	vax
		mtpr(TBIS,va);
#endif	vax

#ifdef	mips
		unmaptlb(0,btop(va));
#endif	mips
	}

	kup->ku_pmask = 0;
	cpident = CURRENT_CPUDATA->cpu_num;
	for (cpindex = lowcpu; cpindex <= highcpu; cpindex++) 
		if ((pcpu = CPUDATA(cpindex)) && cpindex != cpident)
			kup->ku_pmask |= (1 << cpindex);

	/*
	 * Must inform other processors to sync up
	 * Put it on queue of dead page table entries
	 */

	if (kup->ku_pmask) {
		register int s;

		s = splclock();
		smp_lock(&lk_kmtbsync, LK_RETRY);
		kup->ku_nku = kmemdeadpte;
		kmemdeadpte = kup;
		smp_unlock(&lk_kmtbsync);
		(void) splx(s);
		return 1;
	}

	/*
	 * Single processor
	 */

	else {
		rmfree(kmemmap, 
			kup->ku_pagecnt + (kup->ku_guard ? GUARDPAGES : 0),
			kuptokmemx(kup));
		if (kmemneedmap) {
			kmemneedmap = 0;
			wakeup((caddr_t) &kmemmap);
		}
		return 0;
	}
}


/*
 * Interrupt other CPU to synch up the TB
 */

int
km_intr_cpus()
{
	register int cpindex, cpident, s;
	register struct cpudata *pcpu;
	register int flush;

	s = splclock();
	smp_lock(&lk_kmtbsync, LK_RETRY);
	flush = (kmemdeadpte != (struct kmemusage *) 0);
	smp_unlock(&lk_kmtbsync);
	if (flush) {
		cpident = CURRENT_CPUDATA->cpu_num;
		for (cpindex = lowcpu; cpindex <= highcpu; cpindex++)
			if (cpident != cpindex && (pcpu = CPUDATA(cpindex)))
				intrpt_cpu(cpindex, IPI_KMEMTBFL);
	}
	(void) splx(s);
}

/*
 * Scan buckets for those above their high water mark
 */

int
km_hwmscan(hard)
	int hard;
{
	register int s, i;
	register struct kmembuckets *kbp = &bucket[MAXBUCKETSAVE];
	register struct kmemusage *kup, *pkup, *fkup;
	int hwhit = 0;

#ifdef	KMEM_DEBUG
	if (kmdebug & KM_DDMZ) {
		if (hard) km_intr_cpus();
		return 0;
	}
	else if (kmdebug & KM_DDISABLE) return 0;
	kmemd_gs.gs_hwscan++;
	if (kmdebug & KM_DPHWMSCAN)
		printf("kmd in km_hwmscan\n");
#endif	KMEM_DEBUG
		

	for (i = MAXBUCKETSAVE; i >= MINBUCKET; i--, kbp--) {
		if (kbp->kb_total <= kbp->kb_hwm) continue;
		pkup = (struct kmemusage *) 0;
		s = splimp();
		smp_lock(&lk_buckets, LK_RETRY);

		/*
		 * Isolate kmemusage elements from linked list
		 * This enables us to play with them without
		 * hold our lock for too long.
		 */

		fkup = kup = kbp->kb_kup;
		B_UNPROT;
		kbp->kb_kup = (struct kmemusage *) 0;
		B_PROT;
		smp_unlock(&lk_buckets);
		(void) splx(s);
		while (((freemem <= lotsfree) || hard) && 
			kup && (kbp->kb_total > kbp->kb_hwm))
		if (kup->ku_refcnt == 0) {
			register struct kmemusage *ckup;

			s = splimp();
			smp_lock(&lk_buckets, LK_RETRY);
			if (kup->ku_refcnt) {
				smp_unlock(&lk_buckets);
				(void) splx(s);
				goto lostit;
			}
			B_UNPROT;
			KM_RMELEMENTS(kup);
			kbp->kb_total--;
			B_PROT;
			smp_unlock(&lk_buckets);
			ckup = kup;
			kup = kup->ku_nku;
			if (!pkup) fkup = kup;
			else pkup->ku_nku = kup;
			hwhit++;
#ifdef	KMEM_DEBUG
			if (kmdebug & KM_DSBUCKET) 
				kmemd_bs[ckup->ku_index].bs_hwtrim++;
			if (kmdebug & KM_DPHWMTRIM)
				printf("km_hwmscan trimming bucket\n");
#endif	KMEM_DEBUG
			smp_lock(&lk_cmap, LK_RETRY);
			(void) memfree(&kmempt[kuptokmemx(ckup)], 
				ckup->ku_pagecnt, 0);
			smp_unlock(&lk_cmap);
			ckup->ku_guard = 1;
			(void) km_dead_pte(ckup);
			(void) splx(s);
		}
		else {
lostit:
			pkup = kup;
			kup = kup->ku_nku;
		}

		/*
		 * Reattach bucket's isolated kmemusage list
		 */

		if (fkup) {
			while (kup) {
				pkup = kup;
				kup = kup->ku_nku;
			}
			s = splimp();
			smp_lock(&lk_buckets, LK_RETRY);
			if (pkup) pkup->ku_nku = kbp->kb_kup;
			else fkup->ku_nku = kbp->kb_kup;
			B_UNPROT;
			kbp->kb_kup = fkup;
			B_PROT;
			smp_unlock(&lk_buckets);
			(void) splx(s);
		}
		if ((freemem > lotsfree) && !hard) break;
	}
#ifdef	KMEM_DEBUG
	if (hwhit) kmemd_gs.gs_hwhits++;
	else kmemd_gs.gs_hwmisses++;
#endif	KMEM_DEBUG
	return hwhit;
}

/*
 * Scan the dead ptes for our cpu mask
 */

int
km_scan_pte()
{
	register struct kmemusage *kup, *pkup;
	register int mask = CURRENT_CPUDATA->cpu_mask, s;
	register int i;
	register caddr_t va;
	int we_freed_one = 0;


#ifdef	KMEM_DEBUG
	if (kmdebug & KM_DPSCANPTE)
		printf("kmd in km_scan_pte\n");
#endif	KMEM_DEBUG

	s = splclock();
	smp_lock(&lk_kmtbsync, LK_RETRY);
	kup = kmemdeadpte;
	while (kup) {
		if (kup->ku_pmask & mask) {
			kup->ku_pmask &= ~mask;
			va = kuptova(kup);
			for (i = 0; i < kup->ku_pagecnt; i++) {
#ifdef	vax
				mtpr(TBIS,va);
#endif	vax

#ifdef	mips
				unmaptlb(0,btop(va));
#endif	mips
				va += NBPG;
			}
		}
		kup = kup->ku_nku;
	}

restart:
	kup = kmemdeadpte;
	pkup = (struct kmemusage *) 0;
	while (kup) {
		if (kup->ku_pmask) {
			pkup = kup;
			kup = kup->ku_nku;
		}
		else {
			register struct kmemusage *nkup;

			
			if (pkup) pkup->ku_nku = kup->ku_nku;
			else kmemdeadpte = kup->ku_nku;

			smp_unlock(&lk_kmtbsync);

#ifdef	KMEM_DEBUG
			if (kmdebug & KM_DPPTEPURGE)
				printf("km_scan_pte purging pte(s)\n");
#endif	KMEM_DEBUG
			we_freed_one = 1;

			rmfree(kmemmap,
	                        kup->ku_pagecnt + 
				(kup->ku_guard ? GUARDPAGES : 0),
				kuptokmemx(kup));

			smp_lock(&lk_kmtbsync, LK_RETRY);
			goto restart;
		}
	}

	if (we_freed_one  && kmemneedmap) {
		kmemneedmap = 0;
		wakeup((caddr_t) &kmemmap);
	}
	smp_unlock(&lk_kmtbsync);
	(void) splx(s);
	return;
}

/*
 * Initialize the kernel memory allocator
 */


void
kmeminit()
{
	register struct kmembuckets *kbp, *wkbp;
	register short *hwm, count;

#ifdef KMEM_DEBUG
	if (!powerof2(MAXALLOCSAVE))
		panic("kmeminit: MAXALLOCSAVE not power of 2");
	if (MAXALLOCSAVE > MINALLOCSIZE * 32768)
		panic("kmeminit: MAXALLOCSAVE too big");
	if (MAXALLOCSAVE < CLBYTES)
		panic("kmeminit: MAXALLOCSAVE too small");

	kmdebugboot &= KM_DBOOTONLY;
	kmdebug &= ~(KM_DSTATE|KM_DBOOTONLY);
	kmdebug |= kmdebugboot;
	if (kmdmzflush < 0) kmdmzflush = KM_DDMZINT;
#endif KMEM_DEBUG
	rminit(kmemmap, (ekmempt - kmempt) - (long) 2 - (long) km_wmapsize, 
		(long) 2, "kmalloc unwired map", 
		(ekmempt - kmempt - km_wmapsize));
	rminit(kmemwmap, (long) km_wmapsize , 
		(long) (ekmempt - kmempt) - km_wmapsize,
		"kmalloc wired map", km_wmapsize);
	uwkmemusage = kmemusage + 
		((ekmempt - kmempt - km_wmapsize - CLSIZE) >> CLSIZELOG2);

#if	defined(B_PROTECTION) && defined(KMEM_DEBUG) && defined(vax)
	kbp = bigbucket;
	if ((int) kbp & (NBPG-1)) 
	kbp = (struct kmembuckets *) ((((int) kbp) & ~(NBPG -1)) + NBPG);
	bucket = kbp;
	wbucket = kbp + MAXBUCKETSAVE + 1;
	bpte = &Sysmap[btop((long)kbp & 0x7fffffff)];
#endif	defined(B_PROTECTION) && defined(KMEM_DEBUG) && defined(vax)
	
	/*
	 * Initialize SMP lock
	 */

	lockinit(&lk_buckets, &lock_buckets_d);
	lockinit(&lk_kmtbsync, &lock_kmtbsync_d);

	/*
	 * Initialize the high water marks
	 * and bucket doubly linked lists
	 */

	if (kmbucket_hwm == -1) kmbucket_hwm = KMBUCKET_HWM;
	count = kmbucket_hwm;

	for (wkbp = &wbucket[MINBUCKET], kbp = &bucket[MINBUCKET], 
		hwm = &kmemhwm[MINBUCKET]; 
		kbp <= &bucket[MAXBUCKETSAVE]; kbp++, wkbp++, hwm++) {

		wkbp->kb_efl = wkbp->kb_ebl = (struct kmemelement *) wkbp;
		kbp->kb_efl = kbp->kb_ebl = (struct kmemelement *) kbp;

		if (count != 1 && kbp > &bucket[KMBUCKET]) count--;
		if (*hwm != -1 ) kbp->kb_hwm = *hwm;
		else kbp->kb_hwm = count;

		if (wkbp >= &wbucket[KMBUCKET]) wkbp->kb_hwm = 1;
		else wkbp->kb_hwm = KMBUCKET_HWM;
	}

#ifdef	KMEM_DEBUG

	/*
	 * If checking for duplicate free allocate the map (8K/Meg)
	 */

	if (kmdebug & KM_DSANITY) {
		int mapsize = ctob((ekmempt - kmempt) + 1) >> (MINBUCKET + 3);

		kmdebug &= ~KM_DSANITY;
		kmdmap = (caddr_t) 
			km_alloc(mapsize, KM_DEBUG, KM_NOWAIT|KM_CLEAR);
		if (kmdmap != NULL) kmdebug |= KM_DSANITY;
	}
#endif	KMEM_DEBUG

}

/*
 * Global debug flags
 */

int kmdebug = 0;
int kmdebugboot = 0;

#ifdef KMEM_DEBUG


/*
 * Statistics maintained.
 */

struct kmemd_ts kmemd_ts[KM_LAST];
struct kmemd_bs kmemd_bs[MAXBUCKETSAVE+1];
struct kmemd_gs kmemd_gs;

/*
 * History of requests
 */

struct kmemd_request kmemd_requests[KM_REQSIZ];	

int kmemd_request_location = 0;

/*
 * Timer allocated memory list
 */

struct km_dallocate *kmd_tml;

/*
 * Failed timer alloctions
 */

int kmd_tafailed;

/*
 * Arguments passed during system call
 */

struct km_stallocate kmd_stallocate;

/*
 * Where in memory range we left off
 */

unsigned short kmd_twhere;

/*
 * Current allocation pass.
 */

int kmd_tcount;

extern int kmdtimer();

/*
 * Timer in hz units
 */

int kmd_timer;

/*
 * Process queue of allocated memory
 */

struct km_dallocate *kmd_pml;

/*
 * Memory stolen from kernel
 */

int kmd_stolen;


/*
 * Do dead memory zone cleanup
 * Assumption:
 *	SMP lock lk_buckets is held
 */

int
kmd_dmz(kup)
	register struct kmemusage *kup;
{
	register struct kmembuckets *kbp = &bucket[kup->ku_index];
	int flush;

	if (++kmddmz > kmdmzflush) {
		flush = 1;
		kmddmz = 0;
	}
	else flush = 0;
	kbp->kb_total--;
	smp_unlock(&lk_buckets);
	smp_lock(&lk_cmap, LK_RETRY);
	(void) memfree(&kmempt[kuptokmemx(kup)], kup->ku_pagecnt, 0);
	smp_unlock(&lk_cmap);
	kup->ku_guard = 1;
	if (km_dead_pte(kup) && flush) (void) km_intr_cpus();
}

/*
 * Kernel memory debug syscall
 * Note some lk_debug locks are used 
 * strictly for write buffer visibility.
 */

int
km_debug()
{
	struct a {
		unsigned int syscall;
		caddr_t args;
	} *uap = (struct a *) u.u_ap;
	int flags;

	if (!suser()) return;

	switch (uap->syscall) {
	case KM_SSET:
		kmdebug |= ((int) (uap->args) & ~KM_DNOUSER);
		break;
	case KM_SCLEAR:
		kmdebug &= ~((int) (uap->args) & ~KM_DNOUSER);
		break;
	case KM_SPALLOCATE:
	{
		int size;
		struct km_dallocate kda;
		register struct km_dallocate *kdp = &kda;
		
		if ((size = fuword(uap->args)) < 0) {
			u.u_error = EFAULT;
			return;
		}
		else if (size < (1 << MINBUCKET) || size > MAXALLOCSAVE) {
			u.u_error = EINVAL;
			return;
		}
		else if ((kdp = (struct km_dallocate *) 
			km_alloc(size, KM_DEBUG, 0)) == NULL) {
			u.u_error = ENOMEM;
			return;
		}
		kdp->kd_va = (caddr_t) kdp;
		if (suword(uap->args,kdp->kd_va) < 0) {
			u.u_error = EFAULT;
			km_free(kdp->kd_va,KM_DEBUG);
			return;
		}
		smp_lock(&lk_debug, LK_RETRY);
		if (kmd_pml) kdp->kd_next = kmd_pml;
		else kdp->kd_next = (struct km_dallocate *) 0;
		kmd_pml = kdp;
		smp_unlock(&lk_debug);
		return;
	}
	case KM_SPFREE:
		(void) kmd_dliste(&kmd_pml,uap->args);
		return;
	case KM_SPFALL:
		(void) km_dlist(&kmd_pml);
		return;
	case KM_STALLOCATE:
	{
		register struct km_stallocate *kdp;

		smp_lock(&lk_debug, LK_RETRY);
		if (kmdebug & KM_DTIMERON) {
			smp_unlock(&lk_debug);
			u.u_error = EBUSY;
			return;
		}
		kmdebug |= KM_DTIMERON;
		smp_unlock(&lk_debug);
		if (copyin(uap->args,&kmd_stallocate,
			sizeof (struct km_stallocate))) {
			u.u_error = EFAULT;
			goto setfail;
		}
		kdp = &kmd_stallocate;
		if (!powerof2(kdp->kd_ssize) || !powerof2(kdp->kd_lsize) ||
			kdp->kd_ssize < (1 << MINBUCKET) ||
			kdp->kd_ssize > kdp->kd_lsize || 
			kdp->kd_msecs <= 0) {
			u.u_error = EINVAL;
			goto setfail;
		}
		kmd_tcount = 0;
		kmd_twhere = kdp->kd_ssize;
		if (kdp->kd_msecs > 1000) kdp->kd_msecs = 1000;
		kmd_timer = (1000 * hz) / (1000 / kdp->kd_msecs) / 1000;	
		timeout(kmdtimer, (caddr_t) 0, kmd_timer);
		return;
setfail:
		smp_lock(&lk_debug, LK_RETRY);
		kmdebug &= ~KM_DTIMERON;
		smp_unlock(&lk_debug);
		return;
	}
	case KM_STSTOP:
	{
		register int s = splnet();

		smp_lock(&lk_debug, LK_RETRY);
		if ((kmdebug & KM_DTIMERON) == 0) u.u_error = ENOENT;
		else if ((kmdebug & KM_DTIMEROFF) == 0) kmdebug |= KM_DTIMEROFF;
		smp_unlock(&lk_debug);
		(void) splx(s);
		return;
	}
	case KM_SSTEALMEM:
	{
		register int s, size = (int) (uap->args);

		if (size <= 0) {
			u.u_error = EINVAL;
			return;
		}
		s = splimp();
		smp_lock(&lk_debug, LK_RETRY);
		if (kmd_stolen) u.u_error = EBUSY;
		else {
			smp_lock(&lk_cmap, LK_RETRY);
			if ((freemem - size) < minfree) u.u_error = ENOMEM;
			else {
				freemem -= size;
				(void) wakeup(&proc[2]);
				kmd_stolen = size;
			}
			smp_unlock(&lk_cmap);
		}
		smp_unlock(&lk_debug);
		(void) splx(s);
		return;
	}
	case KM_SRETURNMEM:
	{
		register int s = splimp();

		smp_lock(&lk_debug, LK_RETRY);
		if (kmd_stolen == 0) u.u_error = ENOENT;
		else {
			smp_lock(&lk_cmap, LK_RETRY);
			freemem += kmd_stolen;
			kmd_stolen = 0;
			smp_unlock(&lk_cmap);
		}
		smp_unlock(&lk_debug);
		(void) splx(s);
		return;
	}
	case KM_SAWIRED :
		flags = KM_WIRED;
		goto l;
	case KM_SAUNWIRED :
		flags = 0;
l:		
	{
		caddr_t va;
		int size;

		flags |= (KM_CLEAR | KM_NOWAIT | KM_NOCACHE);

		if ((size = fuword(uap->args)) < 0) u.u_error = EFAULT;
		else {
			va = (caddr_t) km_alloc(size, KM_DEBUG, flags);
			if (va == NULL) u.u_error = ENOMEM;
			else if (suword(uap->args,va) < 0) {
				km_free(va, KM_DEBUG);
				u.u_error =  EFAULT;
			}
		}
		break;
	}
	case KM_SFREE :
		km_free(uap->args, KM_DEBUG);
		break;
	default:
		u.u_error = EINVAL;
	}
	return;
}

int
km_dlist(list)
	struct km_dallocate **list;
{
	register struct km_dallocate *kdp, *nkdp;
	
	smp_lock(&lk_debug,LK_RETRY);
	kdp = *list;
	*list = (struct km_dallocate *) 0;
	smp_unlock(&lk_debug);
	while (kdp) {
		nkdp = kdp->kd_next;
		(void) km_free(kdp->kd_va, KM_DEBUG);
		kdp = nkdp;
	}
}

int
kmd_dliste(list,va)
	struct km_dallocate **list;
	register caddr_t va;
{
	register struct km_dallocate *kdp, *pkdp;

	pkdp = (struct km_dallocate *) 0;
	smp_lock(&lk_debug, LK_RETRY);
	kdp = *list;
	while (kdp) {
		if (kdp->kd_va == va) {
			if (pkdp) pkdp->kd_next = kdp->kd_next;
			else *list = kdp->kd_next;
			smp_unlock(&lk_debug);
			(void) km_free(kdp->kd_va, KM_DEBUG);
			break;
		}
		else {
			pkdp = kdp;
			kdp = kdp->kd_next;
		}
	}
	if (!kdp) {
		u.u_error = ENOENT;
		smp_unlock(&lk_debug);
	}
	return;
}

/*
 * Timer memory stealer
 */

kmdtimer()
{
	register struct km_stallocate *ktp = &kmd_stallocate;
	register struct km_dallocate *kdp;

	if (kmd_tcount == ktp->kd_count) {
		if (kmd_tml) {
			caddr_t va = kmd_tml->kd_va;
			kmd_tml = kmd_tml->kd_next;
			(void) km_free(va, KM_DEBUG);

		}
		else kmd_tcount = 0;

		/*
		 * List is empty and timer is off.
		 * Clear timer activity for next user.
		 */

		if (!kmd_tml && (kmdebug & KM_DTIMEROFF)) 
			kmdebug &= ~(KM_DTIMEROFF|KM_DTIMERON);
		else timeout(kmdtimer, (caddr_t) 0, kmd_timer);
		return;
	}

	/*
	 * When timer goes off artificially inflate
	 * the timer allocation position to remove memory.
	 */

	if (kmdebug & KM_DTIMEROFF) {
		if (kmd_tcount) {
			kmd_tcount = ktp->kd_count;
			(void) timeout(kmdtimer, (caddr_t) 0, kmd_timer);
		}
		else kmdebug &= ~(KM_DTIMEROFF|KM_DTIMERON);
		return;
	}

	if (kmd_twhere > ktp->kd_lsize) 
		kmd_twhere = ktp->kd_ssize;

	if ((kdp = (struct km_dallocate *) 
		km_alloc(kmd_twhere, KM_DEBUG, KM_NOWAIT))) {
		kmd_twhere <<= 1;
		kdp->kd_va = (caddr_t) kdp;
		kdp->kd_next = kmd_tml;
		kmd_tml = kdp;
		kmd_tcount++;
	}
	else kmd_tafailed++;
	(void) timeout(kmdtimer, (caddr_t) 0, kmd_timer);
}

#endif KMEM_DEBUG
