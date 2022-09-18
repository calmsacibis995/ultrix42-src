#ifndef lint
static char *sccsid = "@(#)cache.c	4.2	(ULTRIX)	11/9/90";
#endif lint

/************************************************************************
 *                                                                      *
 *                      Copyright (c) 1989 by                           *
 *              Digital Equipment Corporation, Maynard, MA              *
 *                      All rights reserved.                            *
 *                                                                      *
 *   This software is furnished under a license and may be used and     *
 *   copied  only  in accordance with the terms of such license and     *
 *   with the  inclusion  of  the  above  copyright  notice.   This     *
 *   software  or  any  other copies thereof may not be provided or     *
 *   otherwise made available to any other person.  No title to and     *
 *   ownership of the software is hereby transferred.                   *
 *                                                                      *
 *   The information in this software is subject to change  without     *
 *   notice  and should not be construed as a commitment by Digital     *
 *   Equipment Corporation.                                             *
 *                                                                      *
 *   Digital assumes no responsibility for the use  or  reliability     *
 *   of its software on equipment which is not supplied by Digital.     *
 *                                                             		*
 ************************************************************************/

/* ------------------------------------------------------------------ */
/* | Copyright Unpublished, MIPS Computer Systems, Inc.  All Rights | */
/* | Reserved.  This software contains proprietary and confidential | */
/* | information of MIPS and its suppliers.  Use, disclosure or     | */
/* | reproduction is prohibited without the prior express written   | */
/* | consent of MIPS.                                               | */
/* ------------------------------------------------------------------ */

/*
 * cache.c -- MIPS cache control system calls
 */

/*
 * Change history:
 *
 * 17 Apr 90 jaa
 *	fixed cacheflush()/cachectl() syscalls to get args from u_ap
 *	also fixed smp issues in cacheflush()/cachectl() 
 *
 * 11-Dec-1989 sekhar
 *	Changed user_flush to set the SXCTDAT bit in the proc structure
 * 	when icache is flushed but the addresses being flushed are
 * 	in the stack, data.  If the cache flush is for a shared memroy
 *	segment then the bit SMXCTDAT is set in the shared memory
 * 	structure. The intent is that when this page is reallocated 
 * 	in memall, memall will do the flush whenever any page is allocated 
 *	to this process or shared segment.
 *
 * 15-Nov-1989 Robin
 *	Changed bufflush routine to flush only the data cache.  It was
 *	flushing the instruction cache also and this could cause system
 *	performance to be affected.
 *
 * 24-June-1989	Kong
 *	Changed routine bufflush to flush an ULTRIX buffer.
 *
 */

#include "../machine/reg.h"
#include "../machine/pte.h"
#include "../machine/cpu.h"
#include "../machine/cachectl.h"

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/kernel.h"
#include "../h/map.h"
#include "../h/vm.h"
#include "../h/proc.h"
#include "../h/buf.h"
#include "../h/reboot.h"
#include "../h/conf.h"
#include "../h/file.h"
#include "../h/text.h"
#include "../h/clist.h"
#include "../h/callout.h"
#include "../h/cmap.h"
#include "../h/mbuf.h"
#include "../h/quota.h"
#include "../h/ipc.h"
#include "../h/shm.h"

/*
 * cacheflush() -- cacheflush(addr, bcount, cache) system call routine
 */
int
cacheflush()
{
	register struct a {
		caddr_t	addr;
		unsigned len;
		int	cache;
	} *uap = (struct a *)u.u_ap;

	caddr_t addr = uap->addr;
	unsigned bcount = uap->len;
	int cache = uap->cache;

	return(user_flush(addr, bcount, cache));
}

/*
 * cachectl() -- cachectl(addr, bcount, op) system call routine
 */
cachectl()
{
	register struct a {
		caddr_t	addr;
		unsigned len;
		unsigned op;
	} *uap = (struct a *)u.u_ap;

	caddr_t addr = uap->addr;
	unsigned bcount = uap->len;
	unsigned op = uap->op;

	register unsigned rem;
	register struct pte *pte;
	register int was_uncached;
	int error = 0;

	if (((unsigned)addr & PGOFSET) || (bcount & PGOFSET)
	    || (op != CACHEABLE && op != UNCACHEABLE))
		return(EINVAL);
	if (!useracc(addr, bcount, B_READ) && !grow(addr))
		return(EFAULT);
	for (; bcount; bcount -= NBPG, addr += NBPG) {
		pte = vtopte(u.u_procp, btop(addr));
		if (!pte)
			/* Shouldn't happen */
			return(EFAULT);
		was_uncached = pte->pg_n;
		pte->pg_n = (op == UNCACHEABLE);
		/*
		 * If pte not currently associated with phys mem, don't
		 * worry about flushing caches or tlb
		 */
		if (!pte->pg_v && (pte->pg_fod || pte->pg_pfnum == 0))
			continue;
		if (pte->pg_v && u.u_procp->p_tlbpid != -1)
			unmaptlb(u.u_procp->p_tlbpid, btop(addr));
		if (op == CACHEABLE && was_uncached) {
			page_flush(PHYS_TO_K0(ptob(pte->pg_pfnum)));
			if (smp) 
				produce_icache_clears(pte->pg_pfnum);
		}
	}
}

/*
 * user_flush(addr, bcnt, cache) -- flush caches indicated by cache for
 * user addresses addr .. addr+bcnt-1
 */
user_flush(addr, bcnt, cache)
register unsigned addr, bcnt;
{
	register struct pte *pte;
	register unsigned rem;
	register unsigned k0addr;
	extern struct pte eSysmap[];
	int process_executes_data = 0;
	int smindex;

#ifdef notdef		/* We'll do the access check in the loop -- G. Depp */
	if (!useracc((caddr_t)addr, bcnt, B_READ) && !grow(addr))
		return(EFAULT);
#else  notdef		/* But ... we still need to check for addressability */
	if (!IS_KUSEG(addr)) 
		return(EFAULT);
#endif notdef

	for (; bcnt; bcnt -= rem, addr += rem) {
		/*
		 * calculate bytes to flush in this page
		 */
		if((rem = NBPG - (addr & PGOFSET)) > bcnt)
			rem = bcnt;

		/*
		 * calculate appropriate physical address
		 */
		pte = vtopte(u.u_procp, btop(addr));
		if (!pte || pte->pg_n
		    || (!pte->pg_v && (pte->pg_fod || pte->pg_pfnum == 0)))
			continue;
 		if (cache & ICACHE) {
 			if (isadsv(u.u_procp, btop(addr)) ||
 			    isassv(u.u_procp, btop(addr)))
 				process_executes_data = 1;
 			else if (isasmsv(u.u_procp, btop(addr), &smindex)) {
				smp_lock(&lk_smem, LK_RETRY);
 				u.u_procp->p_sm[smindex].sm_p->sm_flag
 					|= SMXCTDAT;
				smp_unlock(&lk_smem);
			}
		}

		k0addr = PHYS_TO_K0(ptob(pte->pg_pfnum)) | (addr & PGOFSET);
		switch (cache) {
		case ICACHE:
			clean_icache(k0addr, rem);
			if (smp) 
				produce_icache_clears(pte->pg_pfnum);
			break;
		case DCACHE:
			clean_dcache(k0addr, rem);
			break;
		case BCACHE:
			clean_cache(k0addr, rem);
			if (smp) 
				produce_icache_clears(pte->pg_pfnum);
			break;
		default:
			return(EINVAL);
		}
	}
 	if (process_executes_data)
 		SET_P_VM(u.u_procp, SXCTDAT);
	return(0);
}

int noctricks = 1;
#ifdef CACHETRICKS
/* temporary -- please remove or legitimize counts */
long count_text_flush;
long count_data_flush;
long count_text_noflush;
long count_data_noflush;
#endif CACHTRICKS

/*
 * bufflush(bp) -- flush cache that aliases to buffer bp if necessary
 */
bufflush(bp)
register struct buf *bp;
{
	register struct pte *pte;
	struct proc *rp;
	register unsigned addr;
	register unsigned bcnt;
	register unsigned rem;
	register unsigned k0_addr; /* K0 address of 1st byte in page */

	/*
	 * Note that the following assume the buffer resides
	 * in either K0, K1, K2, or Ku seg, and does not cross
	 * a segment boundary.
	 */
	addr = (unsigned)bp->b_un.b_addr;
	if (IS_KSEG1(addr))	/* No need to flush cache */
		return;
	/* Buffer is in K0, K2, or Ku seg */
	rp = bp->b_flags&B_DIRTY ? &proc[2] : bp->b_proc;
	bcnt = bp->b_bcount;

	for (; bcnt; bcnt -= rem, addr += rem) {
		rem = NBPG - (addr & PGOFSET);
		if (rem > bcnt)
			rem = bcnt;
		if ((bp->b_flags & B_PHYS) == 0) {
			if (IS_KSEG0(addr)) {
				pte = 0;
				k0_addr = addr & ~PGOFSET;
			} else if (IS_KSEG2(addr)) {
				pte = &Sysmap[btop(addr - K2BASE)];
			}
		} else if (bp->b_flags & B_UAREA)
			pte = (struct pte *)&rp->p_addr[btop(addr)];
		else if (bp->b_flags & B_PAGET)
			pte = &Usrptmap[btokmx((struct pte *)addr)];
		else if ((bp->b_flags & B_SMEM) && ((bp->b_flags &
				B_DIRTY) == 0)) {
			pte = ((struct smem *)rp)->sm_ptaddr + btop(addr);
		}
		else
			pte = vtopte(rp, btop(addr));

		if (pte) {
			if (pte->pg_n)
				continue;
			k0_addr = PHYS_TO_K0((pte->pg_pfnum) << PGSHIFT);
		}
		page_dflush(k0_addr);	/* Flush the page in data cache*/
	}
}

/*
 * page_flush(addr) -- flush i and d caches that aliases with page btop(addr)
 */
page_flush(addr)
caddr_t addr;
{
	page_iflush(addr);
	page_dflush(addr);
}

/*
 * clean_cache(addr, bcnt) -- flush i and d caches lines that alias with
 * address range addr .. addr+bcnt-1
 */
clean_cache(addr, bcnt)
caddr_t addr;
unsigned bcnt;
{
	clean_icache(addr, bcnt);
	clean_dcache(addr, bcnt);
}
