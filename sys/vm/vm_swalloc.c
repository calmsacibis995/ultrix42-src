#ifndef lint
static char *sccsid = "@(#)vm_swalloc.c	4.1	ULTRIX	7/2/90";
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
 *  02 Jan 90  jaa
 *	fixed accounting for data and stack in dmexpand.
 *
 *  11 Dec 89 jaa 
 *	Creation date
 *	this used to be vm_pt.c, but now it only deals with swap allocation
 *
 *	change dynamic swap to account for swap up front (ala v3.1) 
 * 	but actually do the allocation only when pushing the page/process
 *	also did some code cleanup
 *
 *	sekhar
 *	minor changes for tracking swap useage to vsalloc, vsfree, 
 * 	vgetsw, vrelsw, vsfree, vsptalloc, vsptfree, vgetswu, vrelswu
 *	changed dmexpand to track the swap wastage.
 *
 *----------------------------------------------------------------------
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
#include "../h/kmalloc.h"

#include "../machine/pte.h"

#ifdef vax
#include "../machine/mtpr.h"
#endif vax

#include "../machine/cpu.h"
#include "../h/ipc.h"
#include "../h/shm.h"
#include "../h/cpudata.h"

extern struct sminfo sminfo;
extern int swapfrag;

int availvas;
int maxtptelem, maxdptelem, maxsptelem, maxsmptelem;
int dmap_elems[5]; /* text, data, stack, shared memory and sys */

/* VGETSMPT - Get page tables for shared memory segment.	*/
/*	Process must be locked from swapping. In any case	*/
/*	an error return results if no user page table space is	*/
/*	available.	SHMEM					*/

/* only remnant of vm_pt.c */

vgetsmpt(sp)
	register struct smem *sp;
{
	register long a;
	register int smszpt;		/* in page table pages	*/

	if (sp->sm_size == 0)
		panic("vgetsmpt");

	/* Allocate space in the kernel map for	*/
	/* this shared memory segment. Then	*/
	/* allocate page table pages, and	*/
	/* initialize the segments' "sm_ptaddr"	*/
	/* to be the kernel virtual address of	*/
	/* the base of the page table.		*/

	smszpt = clrnd(ctopt(btoc(sp->sm_size)));
	if((a = rmalloc(kernelmap, (long)smszpt)) != 0) {
		if(memall(&Usrptmap[a],smszpt,&proc[0],CSYS,NULL,V_NOOP) != 0){
			sp->sm_ptaddr = kmxtob(a);

			/* Now validate the system page table	*/
			/* entries for the user page table	*/
			/* pages, flushing old translations for	*/
			/* these kernel virtual addresses.	*/
			/* Clear the new page table pages for	*/
			/* clean post-mortems.			*/

			vmaccess(&Usrptmap[a], (caddr_t)sp->sm_ptaddr, 
				 smszpt, DO_CACHE);
			blkclr((caddr_t)sp->sm_ptaddr, smszpt * NBPG);
			return(1);
		} else
			rmfree(kernelmap, (long)smszpt, a);
	}
	return(0);
}

/*
 * Get swap space for data and stack pt's and u. area.
 */
vgetswu(p)
	struct proc *p;
{
	register int dsz, ssz, err;

#ifdef vax
	dsz = clrnd(clrnd(ctopt(p->p_tsize + p->p_dsize)) - p->p_tsize/NPTEPG);
	ssz = clrnd(UPAGES + (clrnd(ctopt(p->p_ssize + HIGHPAGES))));
#endif vax
#ifdef mips
	dsz = clrnd(ctopt(p->p_dsize));
	ssz = clrnd(UPAGES) + clrnd(ctopt(p->p_ssize));
#endif mips

	if(p->p_dmap->dm_ptdaddr != 0)
		panic("vgetswu: bad data pt");
	if(p->p_smap->dm_ptdaddr != 0)
		panic("vgetswu: bad stack pt");

	if (dsz > 0) 
		if((p->p_dmap->dm_ptdaddr = vsptalloc(dsz, CDATA)) == 0) 
			return(0);

	if((p->p_smap->dm_ptdaddr = vsptalloc(ssz, CSTACK)) != 0) 
		return(1);

	if(dsz > 0) {
		vsptfree(p->p_dmap->dm_ptdaddr, dsz, CDATA);
		p->p_dmap->dm_ptdaddr = 0;
	}
	return(0);
}

/*
 * Release swap space for a u. area.
 */
void
vrelswu(p)
	struct proc *p;
{
	register int dsz, ssz;

#ifdef vax
	dsz = clrnd(clrnd(ctopt(p->p_tsize + p->p_dsize)) - p->p_tsize/NPTEPG);
	ssz = clrnd(UPAGES + (clrnd(ctopt(p->p_ssize + HIGHPAGES))));
#endif vax
#ifdef mips
	dsz = clrnd(ctopt(p->p_dsize));
	ssz = clrnd(UPAGES) + clrnd(ctopt(p->p_ssize));
#endif mips

	if (dsz) {
		if(p->p_dmap->dm_ptdaddr == 0)
			panic("vrelswu: bad data pt");
		vsptfree(p->p_dmap->dm_ptdaddr, dsz, CDATA);
		p->p_dmap->dm_ptdaddr = 0;
	}

	if(p->p_smap->dm_ptdaddr == 0)
		panic("vrelswu: bad stack pt");
	vsptfree(p->p_smap->dm_ptdaddr, ssz, CSTACK);
	p->p_smap->dm_ptdaddr = 0;
}

/*
 * fill in swap up to swapfrag size
 * size in dblocks. segtype is used to
 * keep track of swap useage by segment
 * type.
 */
vgetsw(siz, dp, segtype)
	int siz, *dp, segtype;
{
	int frag, savsiz = siz, *savedp = dp;

	while(siz > 0) {
		if(*dp != 0)
			panic("vgetsw");
		frag = (siz > swapfrag) ? swapfrag : siz;
		if((*dp = rmalloc(swapmap, frag)) == 0) {
			if(savsiz - siz)
				vrelsw(savsiz - siz, savedp, segtype);
			return(0);
		}
		ALLOC_SWAP(frag, segtype);
		siz -= frag;
		dp++;
	}
	return(1);
}

void
vrelsw(siz, dp, segtype)
	int siz, *dp, segtype;
{
	int frag;

	while(siz > 0) {
		if(*dp == 0)
			panic("vrelsw");
		frag = (siz > swapfrag) ? swapfrag : siz;
		rmfree(swapmap, frag, *dp);
		FREE_SWAP(frag, segtype);
		*dp++ = 0; 
		siz -= frag;
	}
}


/* 
 * fill holes in swap array
 * in sizes of swapfrag only
 * we don't unwind on failure because the 
 * pageout daemon will probably use it anyway
 */
vsalloc(dmp, segtype)
	struct dmap *dmp;
	int segtype;	/* type of segment for swap useage */
{
	register int i;
	swblk_t *dp;

	if(dmp == (struct dmap *) NULL)
		panic("vsalloc: NULL dmap");

	if(dmp->dm_last == dmp->dm_cnt) 
		return(1);

	for(i = 0, dp = dmp->dm_map; i < dmp->dm_last; i++, dp++){
		if(*dp == 0){
			if(vgetsw(swapfrag, dp, segtype) == 0)
				return(0);
			dmp->dm_cnt++;
		}
	}
	return(1);
}

void
vsfree(dmp, segtype)
	struct dmap *dmp;
	int segtype;
{
	register int i;
	swblk_t *dp;

	if(dmp == (struct dmap *)NULL)
		panic("vsfree: NULL dmap");

	if(dmp->dm_last < 0 || dmp->dm_last > dmap_elems[segtype])
		panic("vsfree: Invalid no. of elems");
	 
	for(i = 0, dp = dmp->dm_map; i < dmp->dm_last; i++, dp++){
		if(*dp != 0){
			vrelsw(swapfrag, dp, segtype);
			dmp->dm_cnt--;
		}
	}
	if(dmp->dm_cnt != 0)
		panic("vsfree: Invalid count");
}

void
dmapinit()
{
	register int bfrag, s;

	if(swapfrag <= 0)
		panic("dmapinit: bad swap fragment size");

	bfrag = dtob(swapfrag);

	dmap_elems[CTEXT] = (ctod(maxtsiz) + swapfrag - 1) / swapfrag;
	dmap_elems[CDATA] = (ctod(maxdsiz) + swapfrag - 1) / swapfrag;
	dmap_elems[CSTACK] = (ctod(maxssiz) + swapfrag - 1) / swapfrag;
	dmap_elems[CSMEM] =
	 (ctod(clrnd(btoc(sminfo.smmax))) + swapfrag - 1) / swapfrag;
	
	maxtptelem = ((maxtsiz * sizeof(struct pte))+ bfrag - 1)/bfrag;
	maxdptelem = ((maxdsiz * sizeof(struct pte))+ bfrag - 1)/bfrag;
	maxsptelem = ((maxssiz *  sizeof(struct pte))+ bfrag - 1)/bfrag;
	maxsmptelem = 
		((clrnd(btoc(sminfo.smmax)) * sizeof(struct pte)) + 
		 bfrag - 1)/bfrag;

	lockinit(&lk_totalswap, &lock_totalswap_d);
	s = splimp();
	availvas = 0;
	(void)splx(s);
}

/*
 * allocate array to hold swap blocks
 * size in clicks
 * type is fron cmap.h
 */
struct dmap *
dmalloc(segsize, segtype)
	int segsize, segtype;
{
	register int nelems, i, err, ptsz;
	register struct dmap *dmp;

	if(segsize) {
		switch (segtype) {
		case CTEXT:
		case CSMEM:
			/* we don't page/swap smem pt's */
			/* text pts already have swap */
			ptsz = 0;
			break;
		case CDATA:
			/* rough guess for accounting */
			ptsz = clrnd(ctopt(segsize));
			break;
		case CSTACK:
			/* try to account for uarea */
#ifdef vax
			ptsz = clrnd(UPAGES + 
			       (clrnd(ctopt(segsize + HIGHPAGES))));
#endif vax
#ifdef mips
			ptsz = clrnd(UPAGES) + clrnd(ctopt(segsize));
#endif mips
			break;
		default:
			panic("dmalloc: Illegal segtype");
		}

		/* convert segsize to disk block size */
		nelems = (ctod(segsize) + swapfrag - 1) / swapfrag;
		if(nelems <= 0 || nelems > dmap_elems[segtype]) 
			panic("dmalloc: bad segsize");

		for(i = 0; i < maxretry; i++){
			ALLOC_VAS(segsize + ptsz, err);
			if(err)
				break;
			KM_ALLOC(dmp, struct dmap *, sizeof_dmap(nelems), 
			    KM_DMAP, KM_NOW_CL);
			if(dmp != 0) {
				dmp->dm_last = nelems;
				return(dmp);
			}
			FREE_VAS(segsize + ptsz);
			sleep((caddr_t)&lbolt, PZERO);
		}
		u.u_error = ENOMEM;
	}
	return((struct dmap *)0);
}

/*
 * free array that held swap blocks
 * size in clicks, type from cmap.h
 * both ptdaddr and the dm_map had better be empty
 */
void
dmfree(dmp, size, segtype)
	struct dmap *dmp;
	int segtype, size;
{
	register int ptsz;

	switch (segtype) {
	case CTEXT:
	case CSMEM:
		/* we don't page/swap smem pt's */
		/* text pts already have swap */
		ptsz = 0;
		break;
	case CDATA:
		ptsz = clrnd(ctopt(size));
		break;
	case CSTACK:
#ifdef vax
		ptsz = clrnd(UPAGES + 
		       (clrnd(ctopt(size + HIGHPAGES))));
#endif vax
#ifdef mips
		ptsz = clrnd(UPAGES) + clrnd(ctopt(size));
#endif mips
		break;
	default:
		panic("dmfree: Illegal segtype");
	}
	FREE_VAS(size + ptsz);
	KM_FREE(dmp, KM_DMAP);
}

/*
 *	Routine to expand or contract the dmap structure. When it
 *	shrinks it releases the existing swap space
 */
dmexpand(dmp, osegsiz, segsiz, segtype)
	struct dmap **dmp;
	int segsiz, osegsiz, segtype;
{
	register int i, nelems, err;
	register int o_size, n_size, chng;
	register struct dmap *savdmp;
	swblk_t	odent, ndent;

	if(*dmp == (struct dmap *) NULL) {
		if(segsiz && (osegsiz == 0))
			return((*dmp = dmalloc(segsiz, segtype)) == 
			(struct dmap *)NULL ? 0 : 1);
		panic("dmexpand: NULL dmap");
	}
	
	nelems = (ctod(segsiz) + swapfrag - 1) / swapfrag;
	if(nelems < 0 || nelems > dmap_elems[segtype])
		panic("dmexpand: bad number of elements");

	chng = (segsiz - osegsiz) + 
		( (segtype == CTEXT || segtype == CSMEM) ? 0 :
		clrnd(ctopt(segsiz)) - clrnd(ctopt(osegsiz)) );

	/* handle shrinking */
	if(nelems <= (*dmp)->dm_last) {
		for(i = (*dmp)->dm_last - 1; i >= nelems; i--){
		/* last is 1 based and map[] is 0 based */
			if((*dmp)->dm_map[i] != 0) {
				vrelsw(swapfrag, &(*dmp)->dm_map[i], segtype);
				(*dmp)->dm_cnt-- ;
			}
		}
		/* SWAPSHR_WASTAGE adjust the amount of swap space wasted */
		odent = (*dmp)->dm_map[(*dmp)->dm_last-1];
		ndent = (*dmp)->dm_map[nelems-1];
		SWAPSHR_WASTAGE(osegsiz, odent, segsiz, ndent);
		FREE_VAS(-chng);
		(*dmp)->dm_last = nelems;
		return(1);
	}

	o_size = sizeof_dmap((*dmp)->dm_last);
	n_size = sizeof_dmap(nelems);
	savdmp = *dmp; /* save the old ptr before calling KM_REALLOC */
	for(i = 0; i < maxretry; i++) {
		ALLOC_VAS(chng, err);
		if(err) 
			break;
		KM_REALLOC(*dmp, struct dmap *, n_size,
			   o_size, KM_DMAP, KM_NOW_CL);
		if(*dmp != (struct dmap *) NULL) {
			SWAPEXP_WASTAGE(osegsiz, (*dmp)->dm_map[(*dmp)->dm_last-1]);
			(*dmp)->dm_last = nelems;
			return(1);
		}
		FREE_VAS(chng);
		*dmp = savdmp; /* restore the old dmap */
		sleep((caddr_t)&lbolt, PZERO);
	}
	*dmp = savdmp;
	u.u_error = ENOMEM;
	return(0);
}

/*
 * allocate array and swap space for page tables
 * size is pt size in clicks
 * we unwind on error since 
 *	a) we're starting a proc and can't get swap for text pts
 *	so its going to get a swkill anyway
 * or	b) we can't get space for data/stack pt's to swap
 *	proc out, so release it and try someone else
 * don't count vas for pt's since these pt's are being swapped out
 * or we accounted for them when we got the text 
 */
int *
vsptalloc(ptsz, segtype)
	int ptsz, segtype;
{
	int nelems, noop, *dp;

	if((nelems = (ctod(ptsz) + swapfrag - 1)/swapfrag) <= 0)
		panic("vsptalloc: bad size");
	KM_ALLOC(dp, int *, nelems * sizeof(int), KM_DMAP, KM_NOW_CL);
	if(dp != 0) {
		if(vgetsw(ctod(ptsz), dp, segtype)) 
			return(dp);
		vsptfree(dp, 0, segtype);
	}
	return((int *)0);
}

void
vsptfree(dp, ptsz, segtype)
	int *dp, ptsz, segtype;
{
	vrelsw(ctod(ptsz), dp, segtype);
	KM_FREE(dp, KM_DMAP);
}

sizeof_dmap(nelems)
int nelems;
{
	struct dmap *dmap;

	return(sizeof(dmap->dm_last) + sizeof(dmap->dm_cnt) +
		sizeof(dmap->dm_ptdaddr) + (nelems * sizeof(dmap->dm_map)));
}
