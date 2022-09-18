#ifndef lint
static	char	*sccsid = "@(#)vm_subr.c	4.1	(ULTRIX)	7/2/90";
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
 * 15 Jan 86 -- depp
 *	Fixed SM bug in vtopte().
 *
 * 23 Oct 86 -- chet
 *	Add arg to GBMAP call
 *
 * 11 Sep 86 -- koehler
 *	Added bmap function
 *
 * 11 Nov 85 -- depp
 *	Removed all conditional compiles for System V IPC.
 *
 * 11 Mar 85 -- depp
 *	Added System V shared memory support
 *
 */

#include "../machine/pte.h"

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/vm.h"
#include "../h/proc.h"
#include "../h/cmap.h"
#include "../h/gnode.h"
#include "../h/buf.h"
#include "../h/text.h"
#include "../h/ipc.h"
#include "../h/shm.h"
#include "../h/mount.h"
extern struct sminfo sminfo;
extern int extracpu;

#ifdef vax
#include "../vax/mtpr.h"
#endif
#include "../machine/cpu.h"

/*
 * Make uarea of process p addressible at kernel virtual
 * address uarea through sysmap locations starting at map.
 */
uaccess(p, map, uarea)
	register struct proc *p;
	struct pte *map;
	register struct user *uarea;
{
	register int i;
	register struct pte *mp = map;

#ifdef mips
	XPRINTF(XPR_VM,"enter uaccess",0,0,0,0);
#endif mips
	for (i = 0; i < UPAGES; i++) {
		*(int *)mp = 0;
		mp->pg_pfnum = p->p_addr[i].pg_pfnum;
		mp++;
	}
#ifdef mips
        vmaccess(map, (caddr_t)uarea, UPAGES, DO_CACHE);
#endif mips
#ifdef vax
	vmaccess(map, (caddr_t)uarea, UPAGES);
#endif vax
}

#ifdef vax
/*
 * Validate the kernel map for size ptes which
 * start at ppte in the sysmap, and which map
 * kernel virtual addresses starting with vaddr.
 */
vmaccess(ppte0, vaddr, size0)
	struct pte *ppte0;
	register caddr_t vaddr;
	int size0;
{
	register struct pte *ppte = ppte0;
	register int size = size0;

#ifdef mips
	XPRINTF(XPR_VM,"enter vmaccess",0,0,0,0);
#endif mips
	while (size != 0) {
		mapin(ppte, btop(vaddr), (unsigned)(*(int *)ppte & PG_PFNUM), 1,
			(int)(PG_V|PG_KW));
		ppte++;
		vaddr += NBPG;
		--size;
	}
	if(extracpu)
		tbsync();
}
#endif vax

#ifdef mips
/* 
 * Validate the kernel map for size ptes which
 * start at ppte in the sysmap, and which map
 * kernel virtual addresses starting with vaddr.
 *
 * Whether the page is to be cached or not is decided by the cache_cntrl flag.
 */

vmaccess(ppte0, vaddr, size0, cache_cntrl)
	struct pte *ppte0;
	register caddr_t vaddr;
	int size0;
	int cache_cntrl;	/* DO_CACHE or NO_CACHE, from ../mips/pte.h */
{
	register struct pte *ppte = ppte0;
	register int size = size0;

XPRINTF(XPR_VM,"enter vmaccess",0,0,0,0);
	while (size != 0) {
		mapin(btop(vaddr), ppte->pg_pfnum,
			(int)(PG_V|PG_KW|cache_cntrl));
		ppte++;
		vaddr += NBPG;
		--size;
	}
}
#endif mips

/* 
 * Convert a pte pointer to
 * a virtual page number.
 */
ptetov(p, pte)
	register struct proc *p;
	register struct pte *pte;
{
	XPRINTF(XPR_VM,"enter ptetov",0,0,0,0);
        if (isatpte(p, pte))
                return (tptov(p, ptetotp(p, pte)));
        else if (isadpte(p, pte))
                return (dptov(p, ptetodp(p, pte)));
        else
                return (sptov(p, ptetosp(p, pte)));
}

/*
 * Convert a virtual page 
 * number to a pte address.
 */
struct pte *
vtopte(p, v)
	register struct proc *p;
	register unsigned v;
{
	register struct p_sm *psm;
	int smindex;

	if (isatsv(p, v))
		return (tptopte(p, vtotp(p, v)));
#ifdef vax
	else if (isadsv(p, v))
		if((vtodp(p, v) >= p->p_dsize) && p->p_dsize){/* begin SHMEM */
			register struct pte *pte;
			register int i, xp;

			if(p->p_sm == (struct p_sm *) NULL) {
				panic("vtopte: p_sm");
			}

			xp = vtotp(p, v);

			/* translate the process data-space PTE	*/
			/* to the non-swapped shared memory PTE	*/

			for(i = 0; i < sminfo.smseg; i++){
				if(p->p_sm[i].sm_p == NULL)
					continue;
				if(xp >= p->p_sm[i].sm_spte  &&
					xp < p->p_sm[i].sm_spte +
					btoc(p->p_sm[i].sm_p->sm_size))

					break;
			}
			if(i >= sminfo.smseg)
				panic("vtopte SMEM");
			pte = p->p_sm[i].sm_p->sm_ptaddr +
				(xp - p->p_sm[i].sm_spte);
			return(pte);
		}	/* end SHMEM */
		else
			return (dptopte(p, vtodp(p, v)));
#endif vax
#ifdef mips
	else if (isadsv(p, v))
		return (dptopte(p, vtodp(p, v)));
	else if (isassv(p, v))
		return (sptopte(p, vtosp(p, v)));
	else if (isasmsv(p, v, &smindex)) {
		psm = &p->p_sm[smindex];
		return (psm->sm_p->sm_ptaddr + vtosmp(psm, v));
	}
#endif mips
	else 
		return(0);
}

/*
 * Initialize the page tables for paging from an inode,
 * by scouring up the indirect blocks in order.
 * Corresponding area of memory should have been vmemfree()d
 * first or just created.
 */
vinifod(pte, fileno, gp, bfirst, count)
	register struct fpte *pte;
	int fileno;
	register struct gnode *gp;
	register daddr_t bfirst;
	size_t count;
{
	register int i, j;
	int blast = bfirst + howmany(count, CLSIZE);
	int bn;
	register int nclpbsize;
#ifdef GFSDEBUG
	extern short GFS[];
	if(GFS[18]) {
		cprintf("vinifod: fileno %d gp 0x%x (%d) ",
				fileno, gp, gp->g_number);
		cprintf("bfirst %d count %d\n", bfirst, count);
	}
#endif GFSDEBUG

#ifdef mips
	XPRINTF(XPR_VM,"enter vinifod",0,0,0,0);
#endif mips
	nclpbsize = gp->g_mp->m_bsize / CLBYTES;
	while (bfirst < blast) {
		i = bfirst % nclpbsize;
		bn = GBMAP(gp,bfirst/nclpbsize,B_READ,0,0);
		for ( ; i < nclpbsize; i++) {
			pte->pg_fod = 1;
			pte->pg_fileno = fileno;
			if (u.u_error || bn < 0) {
#ifdef mips
                                PUT_PG_BLKNO(pte, 0);
#endif mips
#ifdef vax
				pte->pg_blkno = 0;
#endif vax
				pte->pg_fileno = PG_FZERO;
				cnt.v_nzfod += CLSIZE;
			} else {
#ifdef mips
                                PUT_PG_BLKNO(pte, bn + btodb(i * CLBYTES));
#endif mips
#ifdef vax
				pte->pg_blkno = bn + btodb(i * CLBYTES);
#endif vax
				cnt.v_nexfod += CLSIZE;
			}
			for (j = 1; j < CLSIZE; j++)
				pte[j] = pte[0];
#ifdef GFSDEBUG
			if(GFS[18])
				cprintf("vinifod:   pte 0x%x bn = %d\n",pte,bn);
#endif
			pte += CLSIZE;
			bfirst++;
			if (bfirst == blast)
				break;
		}
	}
}
