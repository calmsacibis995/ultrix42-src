#ifndef lint
static	char	*sccsid = "@(#)vm_drum.c	4.3	(ULTRIX)	9/4/90";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1986, 1987, 1988 by		*
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
 * 17 Aug 90 -- jaa
 *	pass addr of element to vgetsw, not element
 *
 * 13-Jul-90 -- jmartin
 *	Apply "clrnd" to sm_size as well as "btoc".
 *
 *  14 Mar 90 bp
 *	repaired size argument passed into mhremove in vschunk to be
 *	machine independent.
 *
 *  11 Dec 89 jaa
 *	change dynamic swap to account for swap up front (ala v3.1) 
 * 	but actually do the allocation only when pushing the page/process
 *
 * 04 Dec 89 	sekhar
 *	minor changes for tracking swap useage to swalloc_vtod 
 * 	vsxfree, vsxalloc
 *
 * 11 Sep 89 	bp,jmartin
 *	fixed swap hashing bug introduced by dynamic swap
 *
 * 12 Jun 89 -- gg
 *	Dynamic swap changes.
 *	---------------------
 *	Removed routines swpexpand() and vsexpand().
 *	Added a routine swalloc_vtod().
 *	Modified routines vsxalloc(), vsxfree(), vssmalloc(), vssmfree(),
 *	vstodb() and vtod().
 *	Replaced variables dmmin, dmmax and dmtext with swapfrag.
 *	
 * 15 Dec 88 -- jmartin
 *	SMP locking for shared memory data structures (cf. h/shm.h)
 *	Integration of v3.0 shared memory changes, i.e. KM_ALLOC of
 *	per-process shared memory structures.
 *
 * 31 Aug 88 -- jmartin
 *	Lock changes to x_rssize with lk_text.
 *
 * 25 Jul 88 -- jmartin
 *	Lock pte accesses in vsswap using lk_cmap.
 *
 * 9 Jun 88 -- jaa
 *	Added fixes for sm_daddr panic in vssmfree()
 *	
 * 3 Mar 88 - jaa
 *	Added dynamic allocation/deallocation of x_daddr array to 
 *	vsxalloc and vsxfree
 *	
 * 12-11-87	Robin L. and Larry C.
 *      Added portclass/kmalloc support to the system.  Fix pte problem
 *	that caused use of invalid pte's by the aie board.
 *
 * 15 Jan 86 -- depp
 *	Fixed SM bug in vtod().
 *
 * 27 Aug 86 -- depp
 *	Fixed bug in vsswap that caused the swapper to hang
 *
 * 29 Apr 86 -- depp
 *	converted to locking macros from calls routines
 *
 * 17 Mar 86 -- depp
 *	Added dynamic allocation/deallocation of sm_daddr array to 
 *	vssmalloc and vssmfree
 *
 * 11 Nov 85 -- depp
 *	Removed all conditional compiles for System V IPC.
 *
 * 11 Mar 85 -- depp
 *	Added System V shared memory support.  Two new routines added:
 *		vssmalloc	Allocate shared memory swap area
 *		vssmfree	Deallocate shared memory swap area
 *
 */

#include "../machine/pte.h"

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/proc.h"
#include "../h/buf.h"
#include "../h/text.h"
#include "../h/map.h"
#include "../h/vm.h"
#include "../h/cmap.h"
#include "../h/kernel.h"
#include "../h/ipc.h"
#include "../h/shm.h"
#include "../h/kmalloc.h"

extern struct sminfo sminfo;

extern int swapfrag;

void vsxfree(), vssmfree();

/*
 * Allocate swap space array for a text segment
 */
vsxalloc(xp)
	struct text *xp;
{
	register int ptsz = clrnd(ctopt(xp->x_size)), err;

	if (xp->x_size > maxtsiz)
		return(0);
	if((xp->x_dmap = dmalloc(xp->x_size, CTEXT)) == 0) 
		return(0);
	if (xp->x_flag & XPAGI) {
		ALLOC_VAS(ptsz, err);
		if (err || 
		(xp->x_dmap->dm_ptdaddr = vsptalloc(ptsz, CDATA)) == 0){
			if(!err)
				FREE_VAS(ptsz);
			vsxfree(xp);
			return(0);
		}
	}
	return(1);
}

/*
 * Free the swap space of a text segment which
 * has been allocated ts pages.
 */
void
vsxfree(xp)
	struct text *xp;
{
	register int ptsz = clrnd(ctopt(xp->x_size));

	vsfree(xp->x_dmap, CTEXT);
	if ((xp->x_flag & XPAGI) && xp->x_dmap->dm_ptdaddr != 0) {
		vsptfree(xp->x_dmap->dm_ptdaddr, ptsz, CDATA);
		FREE_VAS(ptsz);
		xp->x_dmap->dm_ptdaddr = 0;
	}
	dmfree(xp->x_dmap, xp->x_size, CTEXT);
	xp->x_dmap = (struct dmap *)NULL;
}

/* VSSMALLOC - Allocate array for swap  for shared memory segment */
/* LeRoy Fundingsland    1/22/85    DEC			          */

vssmalloc(sp)
	register struct smem *sp;
{
	/* Shared memory pte's are never swapped */
	return((sp->sm_dmap = 
		dmalloc(clrnd((int)btoc(sp->sm_size)), CSMEM)) == 0 ? 0 : 1);
}

/* VSSMFREE - virtual swap shared memory free. Free the swap	*/
/*	space of a shared memory segment         		*/
/* LeRoy Fundingsland    1/16/85    DEC				*/
void
vssmfree(sp)
	register struct smem *sp;
{
	if (! (sp->sm_flag & IPC_SYSTEM) ) {
		vsfree(sp->sm_dmap, CSMEM);
		dmfree(sp->sm_dmap, clrnd(btoc(sp->sm_size)), CSMEM);
		sp->sm_dmap = (struct dmap *)NULL;
	}
}


/*
 * Swap a segment of virtual memory to disk,
 * by locating the contiguous dirty pte's
 * and calling vschunk with each chunk.
 */
vsswap(p, pte, type, vsbase, vscount, dmp)
	struct proc *p;
	register struct pte *pte;
	int type;
	register int vsbase, vscount;
	struct dmap *dmp;
{
	register int size = 0;
	register struct cmap *c;
	int s;

	if (vscount % CLSIZE)
		panic("vsswap");
	if (vscount == 0)
		return;
	for (;;) {
/*
   Although we are attempting to swap it out, this page may not yet have
   been paged in.  We wait on the lock but do not keep it, because we
   would not be in this routine if any of the other activities which set
   c->c_lock (e.g. dma, pageout) could occur.  These other activities are
   precluded by locks held on overlying structures (e.g. text table entry).
*/
		s = splimp();
		smp_lock(&lk_cmap, LK_RETRY);
		if (pte->pg_fod == 0 && pte->pg_pfnum) {
			c = &cmap[pgtocm(pte->pg_pfnum)];
			MWAIT(c);
		}
		smp_unlock(&lk_cmap);
		(void)splx(s);
		if (!dirtycl(pte)) {
			if (size) {
				vschunk(p, vsbase, size, type, dmp);
				vsbase += size;
				size = 0;
			}
			vsbase += CLSIZE;
			if (pte->pg_fod == 0 && pte->pg_pfnum) {
				struct lock_t *seg_lock;
				int count = vmemfree(pte, CLSIZE);

				seg_lock = lock_by_type[type];
				s = splimp();
				smp_lock(seg_lock, LK_RETRY);
				*((type == CTEXT) ? &p->p_textp->x_rssize :
				  ((type == CSMEM) ?
				   &((struct smem *)p)->sm_rssize :
				   &p->p_rssize)) -= count;
				smp_unlock(seg_lock);
				(void)splx(s);
			}
		} else {
			size += CLSIZE;
                        zapcl(pte,pg_m) = 0;
		}

		if ((vscount -= CLSIZE) == 0) {
			if (size)
				vschunk(p, vsbase, size, type, dmp);
			return;
		}

		if (type == CSTACK)
			pte -= CLSIZE;
		else
			pte += CLSIZE;
	}
}

vschunk(p, base, size, type, dmp)
	register struct proc *p;
	register int base, size;
	register int type;
	struct dmap *dmp;
{
	register struct pte *pte;
	register struct dmap *tmap;
	struct dblock db;
	unsigned v;
	register daddr_t bn;

	base = ctod(base);
	size = ctod(size);
		while (size > 0) {
		int count, s;
		struct lock_t *seg_lock = lock_by_type[type];
		
		if (type == CTEXT || type == CSMEM) {
			db.db_size = swapfrag - base % swapfrag;
			if (db.db_size > size)
				db.db_size = size;
		} else {
			vstodb(base, size, dmp, &db, type == CSTACK);
			v = (type==CSTACK ?
				sptov(p, dtoc(base+db.db_size)-1) :
				dptov(p, dtoc(base)));
		}

		switch (type) {
		case CTEXT:
			tmap= p->p_textp->x_dmap;
			bn = tmap->dm_map[base/swapfrag] + base % swapfrag;
			pte = tptopte(p, dtoc(base));
			mhremove(pte,bn,
				dtoc(db.db_size) >> CLSIZELOG2,
				LK_FALSE);
			swap(p, bn, ptob(tptov(p, dtoc(base))), 
				(int)dtob(db.db_size),
			    B_WRITE, 0, swapdev, 0);
			break;
		case CSMEM:
			swap(p, ((struct smem *)p)->
			sm_dmap->dm_map[base/swapfrag] + base%swapfrag,
				ptob(dtoc(base)), (int)dtob(db.db_size),
				B_WRITE, B_SMEM, swapdev, 0);
			pte = ((struct smem *)p)->sm_ptaddr + dtoc(base);
			break;
		case CDATA:
		case CSTACK:
		swap(p, db.db_base, ptob(v), (int)dtob(db.db_size),
		    B_WRITE, 0, swapdev, 0);
			pte = (type==CSTACK) ?
		    sptopte(p, dtoc(base+db.db_size)-1) :
		    dptopte(p, dtoc(base));
			break;
		}

		count = vmemfree(pte, (int)dtoc(db.db_size));
		s = splimp();
		smp_lock(seg_lock, LK_RETRY);
		*((type == CTEXT) ? &p->p_textp->x_rssize :
			((type == CSMEM) ? &((struct smem *)p)->sm_rssize :
				&p->p_rssize)) -= count;
		smp_unlock(seg_lock);
		(void)splx(s);
		base += db.db_size;
		size -= db.db_size;
	}
}

/*
 * Given a base/size pair in virtual swap area,
 * return a physical base/size pair which is the
 * (largest) initial, physically contiguous block.
 */
vstodb(vsbase, vssize, dmp, dbp, rev)
	register int vsbase, vssize;
	struct dmap *dmp;
	register struct dblock *dbp;
{
	register int index;
	register int blk = swapfrag;

	if (vsbase < 0 || vssize < 0 || (vsbase + vssize > (dmp->dm_last *
					blk)))
		panic("vstodb");
	index = vsbase/blk;
	vsbase %= blk;
	if(dmp->dm_cnt == 0 || dmp->dm_map[index] == 0){
		dbp->db_base = -1;
		return;
	}
	if (dmp->dm_map[index] + blk > nswap)
		panic("vstodb exceeding nswap");
	dbp->db_size = imin(vssize, blk - vsbase);
	dbp->db_base = dmp->dm_map[index] 
			+ (rev ? blk - (vsbase + dbp->db_size) : vsbase);
}

/*
 * Convert a virtual page number 
 * to its corresponding disk block number.
 * Used in pagein/pageout to initiate single page transfers.
 */
swblk_t
vtod(p, v, dmap, smap)
	register struct proc *p;
	unsigned v;
	struct dmap *dmap, *smap;
{
	register int tp, index;
	register int smpage;
	struct dblock db;
	register struct dmap *tmap, *shmap;
	int smindex;

	if (isatsv(p, v)) {
		tmap = p->p_textp->x_dmap;
		if(tmap->dm_cnt == 0)	/* no swap allocated */
			return(-1);
		tp = ctod(vtotp(p,v));
		index = tp/swapfrag;
		if(index > tmap->dm_last)
			panic("vtod: text");
		if(tmap->dm_map[index] == 0)
			return(-1);
		else
			return(tmap->dm_map[index] + tp%swapfrag);
	}
	if(isassv(p,v)){
		if(smap->dm_cnt == 0)	/* no swap allocated */
			return(-1);
		vstodb(ctod(vtosp(p,v)), ctod(1), smap, &db, 1);
	}
#ifdef vax
	else
		/* begin SHMEM */
		if(vtodp(p, v) >= p->p_dsize){
			register int i, xp;

			xp = vtotp(p, v);
			for(i=0; i < sminfo.smseg; i++){
				if(p->p_sm[i].sm_p == NULL)
					continue;
				if(xp >= p->p_sm[i].sm_saddr &&
				   xp < p->p_sm[i].sm_saddr +
				   clrnd(btoc(p->p_sm[i].sm_p->sm_size)))
					break;
			}
			if(i >= sminfo.smseg)
				panic("pagin SMEM");
			smpage = xp - p->p_sm[i].sm_saddr;
			shmap = p->p_sm[i].sm_p->sm_dmap;
			if(shmap->dm_cnt == 0)
				return(-1);
			index = smpage / swapfrag;
			if(index > shmap->dm_last)
				panic("vtod: shmem");
			if(shmap->dm_map[index] == 0)
				return(-1);
			else
			  return(shmap->dm_map[index] + (smpage % swapfrag));
		}
		/* end SHMEM */
		else {
			if(dmap->dm_cnt == 0)	/* no swap allocated */
				return(-1);
			vstodb(ctod(vtodp(p,v)), ctod(1), dmap, &db, 0);
		}
#endif vax
#ifdef mips
	else if (isadsv(p,v)){
		if(dmap->dm_cnt == 0)	/* no swap allocated */
			return(-1);
		vstodb(ctod(vtodp(p,v)), ctod(1), dmap, &db, 0);
	}
	else if (isasmsv(p,v,&smindex)) {
		smpage = ctod(vtosmp(&p->p_sm[smindex], v));
		shmap = p->p_sm[smindex].sm_p->sm_dmap;
		if(shmap->dm_cnt == 0)
			return(-1);
		index = smpage / swapfrag;
		if(index > shmap->dm_last)
			panic("vtod: shmem");
		if(shmap->dm_map[index] == 0)
			return(-1);
		else
		  return(shmap->dm_map[index] + (smpage % swapfrag));
	} else
		panic("vtod: Can not classify page");
#endif mips	
	return (db.db_base);
}


int
swalloc_vtod(p, v)
	register struct proc *p;
	unsigned v;
{
	register int index, ok;
	int smindex;
	register struct dmap *tmap ;
	int segtype;

	if(isatsv(p,v)){
		tmap = p->p_textp->x_dmap;
		index = ctod(vtotp(p,v))/swapfrag;
		segtype = CTEXT;
	} else if(isassv(p,v)){
		tmap = p->p_smap;
		index = ctod(vtosp(p,v))/swapfrag;
		segtype = CSTACK;
	}
#ifdef vax
	else {
		/* begin SHMEM */
		if(vtodp(p, v) >= p->p_dsize){
			register int i, xp, smp;

			xp = vtotp(p, v);
			for(i=0; i < sminfo.smseg; i++){
				if(p->p_sm[i].sm_p == NULL)
					continue;
				if(xp >= p->p_sm[i].sm_saddr &&
					xp < p->p_sm[i].sm_saddr +
					clrnd(btoc(p->p_sm[i].sm_p->sm_size)))

					break;
			}
			if(i >= sminfo.smseg)
				panic("swalloc_vtod:  SMEM");
			smp = xp - p->p_sm[i].sm_saddr;
			tmap = p->p_sm[i].sm_p->sm_dmap;
			index = smp / swapfrag;
			segtype = CSMEM;
		}
	/* end of SHMEM */

		else {
			tmap = p->p_dmap;
			index = ctod(vtodp(p,v))/swapfrag;
			segtype = CDATA;
		}
	}
#endif vax
#ifdef mips
	else if (isadsv(p,v)){
		tmap = p->p_dmap;
		index = ctod(vtodp(p,v))/swapfrag;
		segtype = CDATA;
	} else if (isasmsv(p,v,&smindex)) {
		register int smp;
		smp = ctod(vtosmp(&p->p_sm[smindex], v));
		tmap = p->p_sm[smindex].sm_p->sm_dmap;
		index = smp / swapfrag;
		segtype = CSMEM;
	} else
		panic("swalloc_vtod: Can not classify page");
#endif mips
	if((ok = vgetsw(swapfrag, &(tmap->dm_map)[index], segtype)))
		tmap->dm_cnt++;

	return(ok);
}
