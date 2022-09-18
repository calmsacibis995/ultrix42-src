#ifndef lint
static	char	*sccsid = "@(#)vm_drum.c	4.1	(ULTRIX)	7/2/90";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1986,87 by			*
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

/* macro to calculate sm_daddr size in bytes (vssmalloc, vssmfree) */
#define DADDR_SIZE(npg)	(((ctod(npg) + (dmtext - 1)) / dmtext) * 4)

/*
 * Expand the swap area for both the data and stack segments.
 * If space is not available for both, retract and return 0.
 */
swpexpand(ds, ss, dmp, smp)
	size_t ds, ss;
	register struct dmap *dmp, *smp;
{
	register struct dmap *tmp;
	register int ts;
	size_t ods;

#ifdef mips
	XPRINTF(XPR_VM,"enter swpexpand",0,0,0,0);
#endif mips
	/*
	 * If dmap isn't growing, do smap first.
	 * This avoids anomalies if smap will try to grow and
	 * fail, which otherwise would shrink ds without expanding
	 * ss, a rather curious side effect!
	 */
	if (dmp->dm_alloc > ds) {
		tmp = dmp; ts = ds;
		dmp = smp; ds = ss;
		smp = tmp; ss = ts;
	}
	ods = dmp->dm_size;
	if (vsexpand(ds, dmp, 0) == 0)
		goto bad;
	if (vsexpand(ss, smp, 0) == 0) {
		(void) vsexpand(ods, dmp, 1);
		goto bad;
	}
	return (1);

bad:
	u.u_error = ENOMEM;
	return (0);
}

/*
 * Expand or contract the virtual swap segment mapped
 * by the argument diskmap so as to just allow the given size.
 *
 * FOR NOW CANT RELEASE UNLESS SHRINKING TO ZERO, SINCE PAGEOUTS MAY
 * BE IN PROGRESS... TYPICALLY NEVER SHRINK ANYWAYS, SO DOESNT MATTER MUCH
 */
vsexpand(vssize, dmp, canshrink)
	register size_t vssize;
	register struct dmap *dmp;
{
	register long blk = dmmin;
	register int vsbase = 0;
	register swblk_t *ip = dmp->dm_map;
	size_t oldsize = dmp->dm_size;
	size_t oldalloc = dmp->dm_alloc;

#ifdef mips
	XPRINTF(XPR_VM,"enter vsexpand",0,0,0,0);
#endif mips
	vssize = ctod(vssize);
	while (vsbase < oldalloc || vsbase < vssize) {
		if (ip - dmp->dm_map >= NDMAP)
			panic("vmdrum NDMAP");
		if (vsbase >= oldalloc) {
			*ip = rmalloc(swapmap, blk);
			if (*ip == 0) {
				dmp->dm_size = vsbase;
				if (vsexpand(dtoc(oldsize), dmp, 1) == 0)
					panic("vsexpand");
				return (0);
			}
			dmp->dm_alloc += blk;
		} else if (vssize == 0 ||
		    vsbase >= vssize && canshrink) {
			rmfree(swapmap, blk, *ip);
			*ip = 0;
			dmp->dm_alloc -= blk;
		}
		vsbase += blk;
		if (blk < dmmax)
			blk *= 2;
		ip++;
	}
	dmp->dm_size = vssize;
	return (1);
}

/*
 * Allocate swap space for a text segment,
 * in chunks of at most dmtext pages.
 */
vsxalloc(xp)
	struct text *xp;
{
	register long blk;
	register swblk_t *dp;
	swblk_t vsbase;

#ifdef mips
	XPRINTF(XPR_VM,"enter vsxalloc",0,0,0,0);
#endif mips
	if(xp->x_size > maxtsiz)
		return (0);

	/* allocate space for text segment swap blocks */
	KM_ALLOC((xp->x_daddr), swblk_t *, 
	(ctod(maxtsiz)/dmtext)*sizeof(swblk_t), KM_TXTSW, KM_CLEAR);
	if((dp = xp->x_daddr) == (swblk_t *) NULL)
	        return(0);

	for (vsbase = 0; vsbase < ctod(xp->x_size); vsbase += dmtext) {
		blk = ctod(xp->x_size) - vsbase;
		if (blk > dmtext)
			blk = dmtext;
		if ((*dp++ = rmalloc(swapmap, blk)) == 0) {
			vsxfree(xp, dtoc(vsbase));
			return (0);
		}
	}
	if (xp->x_flag & XPAGI) {
		xp->x_ptdaddr = rmalloc(swapmap,
				(long)ctod(clrnd(ctopt(xp->x_size))));
		if (xp->x_ptdaddr == 0) {
			vsxfree(xp, (long)xp->x_size);
			return (0);
		}
	}
	return (1);
}


/* VSSMALLOC - Allocate swap space for a shared memory segment,	*/
/*	in chunks of at most dmtext pages.			*/
/* LeRoy Fundingsland    1/22/85    DEC				*/

vssmalloc(sp)
    register struct smem *sp;
{
	register long blk;
	register swblk_t *dp;
	register int smsize;		/* in clicks		*/
	swblk_t vsbase;

#ifdef mips
	XPRINTF(XPR_VM,"vssmalloc",0,0,0,0);
#endif mips
	smsize = clrnd((int)btoc(sp->sm_size));

	/* allocate space for shared memory segment swap blocks */
	KM_ALLOC((sp->sm_daddr), swblk_t *, DADDR_SIZE(smsize), KM_TEMP, KM_CLEAR);
	if((dp = sp->sm_daddr) == (swblk_t *) NULL)
	        return(0);

	/* allocate swap space in at most dmtext chunks */
	for (vsbase=0; vsbase < ctod(smsize); vsbase+=dmtext) {
		blk = ctod(smsize) - vsbase;
		if (blk > dmtext)
			blk = dmtext;
		if ((*dp++ = rmalloc(swapmap, blk)) == 0) {
			vssmfree(sp, vsbase);
			return (0);
		}
	}

	sp->sm_ptdaddr = rmalloc(swapmap,
				(long)ctod(clrnd(ctopt(smsize))));
	if (sp->sm_ptdaddr == 0) {
		vssmfree(sp, (long)smsize);
		return(0);
	}

	return(1);
}

/*
 * Free the swap space of a text segment which
 * has been allocated ts pages.
 */
vsxfree(xp, ts)
	struct text *xp;
	long ts;
{
	register long blk;
	register swblk_t *dp;
	swblk_t vsbase;

#ifdef mips
	XPRINTF(XPR_VM,"enter vsxfree",0,0,0,0);
#endif mips
	ts = ctod(ts);
	if ((dp = xp->x_daddr) == 0)
		panic("vsxfree: x_daddr");
	for (vsbase = 0; vsbase < ts; vsbase += dmtext) {
		blk = ts - vsbase;
		if (blk > dmtext)
			blk = dmtext;
		rmfree(swapmap, blk, *dp);
		*dp++ = 0;
	}
	if ((xp->x_flag&XPAGI) && xp->x_ptdaddr) {
		rmfree(swapmap, (long)ctod(clrnd(ctopt(xp->x_size))),
		    xp->x_ptdaddr);
		xp->x_ptdaddr = 0;
	}
	/* free the daddr space */
	KM_FREE((xp->x_daddr), KM_TXTSW);
	xp->x_daddr = 0;
}

/* VSSMFREE - virtual swap shared memory free. Free the swap	*/
/*	space of a shared memory segment which has been		*/
/*	allocated size pages.					*/
/* LeRoy Fundingsland    1/16/85    DEC				*/
vssmfree(sp, size)
    register struct smem *sp;	/* pointer to shared memory header */
    long size;
{
	register long blk;
	register swblk_t *dp;
	register int smsize;		/* in clicks		*/
	swblk_t vsbase;

#ifdef mips
	XPRINTF(XPR_VM,"vssmfree",0,0,0,0);
#endif mips
	size = ctod(size);
	smsize = clrnd((int)btoc(sp->sm_size));

	if ((dp = sp->sm_daddr) == 0)
		panic("vssmfree: sm_daddr");

	/* free swap space for segment		*/
	for (vsbase=0; vsbase < size; vsbase+=dmtext) {
		blk = size - vsbase;
		if (blk > dmtext)
			blk = dmtext;
		rmfree(swapmap, blk, *dp);
		dp++;
	}

	/* free the daddr space */
	KM_FREE((sp->sm_daddr), KM_TEMP);
	sp->sm_daddr = 0;

	/* free swap space for segment PTEs	*/
	if (sp->sm_ptdaddr) {
		rmfree(swapmap, (long)ctod(clrnd(ctopt(smsize))),
						sp->sm_ptdaddr);
		sp->sm_ptdaddr = 0;
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

#ifdef mips
	if (type == CSMEM)
		XPRINTF(XPR_SM,"enter vsswap: pte 0x%x, vsbase 0x%x vscount 0x%x dmp 0x%x",pte, vsbase, vscount, dmp);
#endif mips
	if (vscount % CLSIZE)
		panic("vsswap");
#ifdef vax
	if (vscount == 0)
		return;
#endif vax
	for (;;) {
#ifdef vax
		if (pte->pg_fod == 0 && pte->pg_pfnum) {
			c = &cmap[pgtocm(pte->pg_pfnum)];
			MWAIT(c);
		}
#endif vax
		if (
#ifdef mips
			vscount == 0 ||
#endif mips
			!dirtycl(pte)) {
			if (size) {
				vschunk(p, vsbase, size, type, dmp);
				vsbase += size;
				size = 0;
			}
#ifdef mips
                        if (vscount == 0)
                                return;
#endif mips
			vsbase += CLSIZE;
			if (pte->pg_fod == 0 && pte->pg_pfnum)
				if (type == CTEXT)
					p->p_textp->x_rssize -= 
						vmemfree(pte, CLSIZE);
				/* SHMEM */
				else if (type == CSMEM)
					((struct smem *)p)->sm_rssize -=
						vmemfree(pte, CLSIZE);
				else
					p->p_rssize -= vmemfree(pte, CLSIZE);
		} else {
			size += CLSIZE;
#ifdef vax
                        zapcl(pte,pg_m) = 0;
#endif vax
#ifdef mips
                       c = &cmap[pgtocm(pte->pg_pfnum)];
                       MLOCK(c);
                       MUNLOCK(c);
#endif mips
		}
#ifdef vax
		if ((vscount -= CLSIZE) == 0) {
			if (size)
				vschunk(p, vsbase, size, type, dmp);
			return;
		}
#endif vax
#ifdef mips
                vscount -= CLSIZE;
#endif mips
		if (type == CSTACK)
			pte -= CLSIZE;
		else
			pte += CLSIZE;
	}
}

vschunk(p, base, size, type, dmp)
	register struct proc *p;
	register int base, size;
	int type;
	struct dmap *dmp;
{
	register struct pte *pte;
	struct dblock db;
	unsigned v;

#ifdef mips
	XPRINTF(XPR_SM,"vschunk",0,0,0,0);
	       if (type == CSMEM)
		       XPRINTF(XPR_SM,"enter vschunk: base 0x%x size 0x%x",
			       base, size, 0,0);
#endif mips
	base = ctod(base);
	size = ctod(size);
	if (type == CTEXT) {
		while (size > 0) {
			db.db_size = dmtext - base % dmtext;
			if (db.db_size > size)
				db.db_size = size;
			swap(p, p->p_textp->x_daddr[base/dmtext] + base%dmtext,
			    ptob(tptov(p, dtoc(base))), (int)dtob(db.db_size),
			    B_WRITE, 0, swapdev, 0);
			pte = tptopte(p, dtoc(base));
			p->p_textp->x_rssize -=
			    vmemfree(pte, (int)dtoc(db.db_size));
			base += db.db_size;
			size -= db.db_size;
		}
		return;
	}
	/* begin SHMEM */
	if (type == CSMEM) {
		while (size > 0) {
			db.db_size = dmtext - base % dmtext;
			if (db.db_size > size)
				db.db_size = size;
			swap(p, ((struct smem *)p)->
				sm_daddr[base/dmtext] + base%dmtext,
				ptob(dtoc(base)), (int)dtob(db.db_size),
				B_WRITE, B_SMEM, swapdev, 0);
			pte = ((struct smem *)p)->sm_ptaddr +
							dtoc(base);
			((struct smem *)p)->sm_rssize -=
				vmemfree(pte, (int)dtoc(db.db_size));
			base += db.db_size;
			size -= db.db_size;
		}
		return;
	}
	/* end SHMEM */
	do {
		vstodb(base, size, dmp, &db, type == CSTACK);
		v = type==CSTACK ?
		    sptov(p, dtoc(base+db.db_size)-1) :
		    dptov(p, dtoc(base));
		swap(p, db.db_base, ptob(v), (int)dtob(db.db_size),
		    B_WRITE, 0, swapdev, 0);
		pte = type==CSTACK ?
		    sptopte(p, dtoc(base+db.db_size)-1) :
		    dptopte(p, dtoc(base));
		p->p_rssize -= vmemfree(pte, (int)dtoc(db.db_size));
		base += db.db_size;
		size -= db.db_size;
	} while (size != 0);
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
	register int blk = dmmin;
	register swblk_t *ip = dmp->dm_map;

#ifdef mips
	XPRINTF(XPR_VM,"enter vstodb",0,0,0,0);
#endif mips
	if (vsbase < 0 || vssize < 0 || vsbase + vssize > dmp->dm_size)
		panic("vstodb");
	while (vsbase >= blk) {
		vsbase -= blk;
		if (blk < dmmax)
			blk *= 2;
		ip++;
	}
	if (*ip + blk > nswap)
		panic("vstodb *ip");
	dbp->db_size = imin(vssize, blk - vsbase);
	dbp->db_base = *ip + (rev ? blk - (vsbase + dbp->db_size) : vsbase);
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
	register int smpage;
	struct dblock db;
	int tp;
	int smindex;

#ifdef mips
	XPRINTF(XPR_VM,"vtod",0,0,0,0);
#endif mips
	if (isatsv(p, v)) {
		tp = ctod(vtotp(p, v));
		return (p->p_textp->x_daddr[tp/dmtext] + tp%dmtext);
	}
	if (isassv(p, v))
		vstodb(ctod(vtosp(p, v)), ctod(1), smap, &db, 1);
#ifdef vax
	else
		/* begin SHMEM */
		if(vtodp(p, v) >= p->p_dsize){
			register int i, xp;

			xp = vtotp(p, v);
			for(i=0; i < sminfo.smseg; i++){
				if(p->p_sm[i].sm_p == NULL)
					continue;
				if(xp >= p->p_sm[i].sm_spte  &&
					xp < p->p_sm[i].sm_spte +
					btoc(p->p_sm[i].sm_p->sm_size))

					break;
			}
			if(i >= sminfo.smseg)
				panic("pagin SMEM");
			smpage = xp - p->p_sm[i].sm_spte;
			return(p->p_sm[i].sm_p->sm_daddr[smpage/dmtext] +
			       smpage%dmtext);
		}
		/* end SHMEM */
		else
			vstodb(ctod(vtodp(p, v)), ctod(1), dmap, &db, 0);
#endif vax
#ifdef mips
	else if (isadsv(p,v))
		vstodb(ctod(vtodp(p, v)), ctod(1), dmap, &db, 0);
	else if (isasmsv(p,v,&smindex)) {
		smpage = ctod(vtosmp(&p->p_sm[smindex], v));
		return(p->p_sm[smindex].sm_p->sm_daddr[smpage/dmtext] +
		       smpage%dmtext);
	} else
		panic("vtod: Can not classify page");
#endif mips	
	return (db.db_base);
}

