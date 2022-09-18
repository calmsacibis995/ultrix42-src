#ifndef lint
static char *sccsid = "@(#)vm_swap.c	4.1      (ULTRIX)        7/2/90";
#endif lint

/***********************************************************************
 *									*
 *			Copyright (c) 1984,86 by			*
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
/*
 *
 *   Modification history:
 *
 * 15 Dec 86 -- depp
 *	Added fix to shared memory handling, in regards to a process' SM 
 *	page table swapping space.  
 *
 * 29 Apr 86 -- depp
 *	converted to locking macros from calls routines
 *
 * 17 Mar 86 -- depp
 *	Removed "XXX memory hack" from swapout, as it's no longer needed.
 *
 * 16 Jul 85 -- jrs
 *	Add run queue locking
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
#include "../h/proc.h"
#include "../h/text.h"
#include "../h/map.h"
#include "../h/buf.h"
#include "../h/cmap.h"
#include "../h/vm.h"
#include "../h/ipc.h"
#include "../h/shm.h"
#include "../h/interlock.h"
#include "../machine/cpu.h"
extern struct sminfo sminfo;

/*
 * Swap a process in.
 */
swapin(p)
	register struct proc *p;
{
	register struct text *xp;
	register struct p_sm *psmp;	/* see proc.h	SHMEM	*/
	register struct smem *sp;
	register int i, s;
#ifdef mips
	extern memfree();
#endif mips

#ifdef mips
	XPRINTF(XPR_VM,"enter swapin",0,0,0,0);
#endif mips
	if (xp = p->p_textp) 
		X_LOCK(xp);
/* was process using shared memory  SHMEM */
	if (p->p_smbeg > 0) {
		if(p->p_sm == (struct p_sm *) NULL) {
			panic("swapin: p_sm1");
		}
		for(i=0; i < sminfo.smseg; i++) {
			if(sp = p->p_sm[i].sm_p) {
				SM_LOCK(sp);
			}
		}
	}
#ifdef vax
	p->p_szpt = clrnd(ctopt(p->p_ssize+p->p_dsize +
					p->p_smsize +	/* SHMEM */
					p->p_tsize+UPAGES));
#endif vax
#ifdef mips
        p->p_textpt = clrnd(ctopt(p->p_tsize));
        p->p_datapt = clrnd(ctopt(p->p_dsize));
        p->p_stakpt = clrnd(ctopt(p->p_ssize));
#endif mips
#ifdef vax
	if (vgetpt(p, memall) == 0)
		goto nomem;
#endif vax
#ifdef mips
        if (vgetpt(p, memall, memfree) == 0)
                goto nomem;
        if (xp) {
                if (xlink(p) == 0) {
                        X_UNLOCK(xp);
                        vrelpt(p);
                        goto nomem;
                }
                X_UNLOCK(xp);
        }
#endif mips
	if(vgetu(p, memall, Swapmap, &swaputl, (struct user *)0) == 0){
#ifdef mips
                if (xp)
                        xdetach(xp, p);
#endif mips
		vrelpt(p);
		goto nomem;
	}

	swdspt(p, &swaputl, B_READ);

#ifdef vax
	/*
	 * Make sure swdspt didn't smash u. pte's
	 */
	for (i = 0; i < UPAGES; i++) {
		if (Swapmap[i].pg_pfnum != p->p_addr[i].pg_pfnum)
			panic("swapin");
	}
#endif vax
	vrelswu(p, &swaputl);
#ifdef vax
	if (xp) {
		xlink(p);
		X_UNLOCK(xp);
	}
#endif vax
#ifdef mips
        /*
         * TODO: this is probably not needed
         */
        if (p->p_tlbpid != -1)
                printf("swapin: tlbpid=%d\n", p->p_tlbpid);
        p->p_tlbpid = -1;
        clear_tlbmappings(p);
#endif mips
	if(p->p_smbeg > 0){		/* SHMEM */
		if(p->p_sm == (struct p_sm *) NULL) {
			panic("swapin: p_sm2");
		}
		psmp = &p->p_sm[0];
		for(i=0; i < sminfo.smseg; i++, psmp++){
			if(sp = psmp->sm_p){
				smlink(p, sp);
				SM_UNLOCK(sp);
			}
		}
	}

	p->p_rssize = 0;
#ifdef vax
	s = spl6();
#endif vax
#ifdef mips
	s = splclock();
#endif mips
	if (p->p_stat == SRUN) {
		lock(LOCK_RQ);
		setrq(p);
		unlock(LOCK_RQ);
	}
	p->p_flag |= SLOAD;
	if (p->p_flag & SSWAP) {
		swaputl.u_pcb.pcb_sswap = (int *)&u.u_ssave;
		p->p_flag &= ~SSWAP;
	}
	splx(s);
	p->p_time = 0;
	multprog++;
	cnt.v_swpin++;
	return (1);

nomem:
	if (xp)
		X_UNLOCK(xp);

	if(p->p_smbeg > 0){		/* SHMEM */
		if(p->p_sm == (struct p_sm *) NULL) {
			panic("swapin: p_sm3");
		}
		psmp = &p->p_sm[0];
		for(i=0; i < sminfo.smseg; i++, psmp++){
			if(sp = psmp->sm_p){
				SM_UNLOCK(sp);
			}
		}
	}

	return (0);
}

int	xswapwant, xswaplock;
/*
 * Swap out process p.
 * ds and ss are the old data size and the stack size
 * of the process, and are supplied during page table
 * expansion swaps.
 */
swapout(p, ds, ss, sms)
	register struct proc *p;
	size_t ds, ss;
	size_t sms;		/* SHMEM */
{
	register struct pte *map;
	register struct user *utl;
	register int a;
	int s;
	int rc = 1;
	int i;
#ifdef mips
	XPRINTF(XPR_VM,"enter swapout",0,0,0,0);
        utl =
            (struct user *)PHYS_TO_K0(ptob(p->p_addr[0].pg_pfnum));
        checkfp(p,0);
#endif mips
#ifdef vax
	s = 1;
	map = Xswapmap;
	utl = &xswaputl;
	if (xswaplock & s)
		if ((xswaplock & 2) == 0) {
			s = 2;
			map = Xswap2map;
			utl = &xswap2utl;
		}
	a = spl6();
	while (xswaplock & s) {
		xswapwant |= s;
		sleep((caddr_t)map, PSWP);
	}
	xswaplock |= s;
	splx(a);
	uaccess(p, map, utl);
#endif vax
	if (vgetswu(p, utl) == 0) {
		swkill(p, "swapout: no swap u available");
		rc = 0;
		goto out;
	}
	utl->u_ru.ru_nswap++;
	utl->u_odsize = ds;
	utl->u_ossize = ss;
	utl->u_osmsize = sms;
	p->p_flag |= SLOCK;
	if (p->p_textp) {
		if (p->p_textp->x_ccount == 1)
			p->p_textp->x_swrss = p->p_textp->x_rssize;
		xccdec(p->p_textp, p);
	}
	if(p->p_smbeg > 0)	{/* SHMEM */
		if(p->p_sm == (struct p_sm *) NULL) {
			panic("swapout: p_sm");
		}
		for(i=0; i < sminfo.smseg; i++) {
			if(p->p_sm[i].sm_p) {
				smccdec(p->p_sm[i].sm_p, p);
			}
		}
	}
	p->p_swrss = p->p_rssize;
	vsswap(p, dptopte(p, 0), CDATA, 0, ds, &utl->u_dmap);
	vsswap(p, sptopte(p, CLSIZE-1), CSTACK, 0, ss, &utl->u_smap);
	if (p->p_rssize != 0)
		panic("swapout rssize");

	swdspt(p, utl, B_WRITE);

	vrelu(p, 1);
#ifdef mips
        if (p->p_tlbpid != -1)
                release_tlbpid(p);
        /* clear_tlbmappings(p); */
#endif mips
	if ((p->p_flag & SLOAD) && (p->p_stat != SRUN || p != u.u_procp))
		panic("swapout");
	p->p_flag &= ~SLOAD;
	vrelpt(p);
	p->p_flag &= ~SLOCK;
	p->p_time = 0;

	multprog--;
	cnt.v_swpout++;

	if (runout) {
		runout = 0;
		wakeup((caddr_t)&runout);
	}

out:
#ifdef vax
	xswaplock &= ~s;
	if (xswapwant & s) {
		xswapwant &= ~s;
		wakeup((caddr_t)map);
	}
#endif vax
	if (rc == 0) {
#ifdef vax
		a = spl6();
#endif vax
#ifdef mips
		a = splclock();
#endif mips
		p->p_flag |= SLOAD;
		if (p != u.u_procp && p->p_stat == SRUN) {
			lock(LOCK_RQ);
			setrq(p);
			unlock(LOCK_RQ);
		}
		splx(a);
	}
	return (rc);
}

#ifdef vax
/*
 * Swap the data and stack page tables in or out.
 * Only hard thing is swapping out when new pt size is different than old.
 * If we are growing new pt pages, then we must spread pages with 2 swaps.
 * If we are shrinking pt pages, then we must merge stack pte's into last
 * data page so as not to lose them (and also do two swaps).
 */
swdspt(p, utl, rdwri)
	register struct proc *p;
	register struct user *utl;
{
	register int szpt, tsz, ssz;
	int tdlast, slast, tdsz;
	register struct pte *pte;
	register int i;

	szpt = clrnd(ctopt(p->p_tsize+p->p_dsize +
				p->p_smsize +		/* SHMEM */
				p->p_ssize+UPAGES));
	tsz = p->p_tsize / NPTEPG;
	if (szpt == p->p_szpt) {
		swptstat.pteasy++;
		swpt(rdwri, p, 0, tsz,
		    (p->p_szpt - tsz) * NBPG - UPAGES * sizeof (struct pte));
		goto check;
	}
	if (szpt < p->p_szpt)
		swptstat.ptshrink++;
	else
		swptstat.ptexpand++;
	ssz = clrnd(ctopt(utl->u_ossize+UPAGES));
	if (szpt < p->p_szpt && utl->u_odsize && (utl->u_ossize+UPAGES)) {
		/*
		 * Page tables shrinking... see if last text+data and
		 * last stack page must be merged... if so, copy
		 * stack pte's from last stack page to end of last
		 * data page, and decrease size of stack pt to be swapped.
		 */
		tdlast = (p->p_tsize + utl->u_odsize
					+ utl->u_osmsize)	/* SHMEM */
					% (NPTEPG * CLSIZE);
		slast = (utl->u_ossize + UPAGES) % (NPTEPG * CLSIZE);
		if (tdlast && slast && tdlast + slast <= (NPTEPG * CLSIZE)) {
			swptstat.ptpack++;
			tdsz = clrnd(ctopt(p->p_tsize + utl->u_odsize
					+ utl->u_osmsize));	/* SHMEM */
			bcopy((caddr_t)sptopte(p, utl->u_ossize - 1),
			    (caddr_t)&p->p_p0br[tdsz * NPTEPG - slast],
			    (unsigned)(slast * sizeof (struct pte)));
			ssz -= CLSIZE;
		}
	}
	if (ssz)
		swpt(rdwri, p, szpt - ssz - tsz, p->p_szpt - ssz, ssz * NBPG);
	if (utl->u_odsize)
		swpt(rdwri, p, 0, tsz,
		    (clrnd(ctopt(p->p_tsize + utl->u_odsize
					+ utl->u_osmsize))	/* SHMEM */
					- tsz) * NBPG);
check:
	for (i = 0; i < utl->u_odsize; i++) {
		pte = dptopte(p, i);
		if (pte->pg_v || pte->pg_fod == 0 && 
					(pte->pg_pfnum||pte->pg_m))
			panic("swdspt: data");
	}
	for (i = 0; i < utl->u_ossize; i++) {
		pte = sptopte(p, i);
		if (pte->pg_v || pte->pg_fod == 0 && (pte->pg_pfnum||pte->pg_m))
			panic("swdspt: stack");
	}
}

swpt(rdwri, p, doff, a, n)
	int rdwri;
	struct proc *p;
	int doff, a, n;
{

	if (n <= 0)
		return;
	swap(p, p->p_swaddr + ctod(UPAGES) + ctod(doff),
	    (caddr_t)&p->p_p0br[a * NPTEPG], n, rdwri, B_PAGET, swapdev, 0);
}
#endif vax

#ifdef mips
/*
 * Swap the data and stack page tables in or out.
 * Only hard thing is swapping out when new pt size is different than old.
 * If we are growing new pt pages, then we must spread pages with 2 swaps.
 * If we are shrinking pt pages, then we must merge stack pte's into last
 * data page so as not to lose them (and also do two swaps).
 */
swdspt(p, utl, rdwri)
	register struct proc *p;
	register struct user *utl;
	int rdwri;
{
	register int dszpt, sszpt;
	register struct pte *pte;
	register int i;
	int start;

XPRINTF(XPR_VM,"enter swdspt",0,0,0,0);
	dszpt = clrnd(ctopt(p->p_dsize));
	if (dszpt == p->p_datapt)
		swptstat.dpteasy++;
	else if (dszpt < p->p_datapt)
		swptstat.dptshrink++;
	else
		swptstat.dptexpand++;

	if (utl->u_odsize != 0) {
		(void)swap(p, p->p_swaddr + ctod(clrnd(UPAGES)), 
		    (caddr_t)&p->p_databr[0],
		    clrnd(ctopt(utl->u_odsize)) * NBPG,
		    rdwri, B_PAGET, swapdev, 0);
	}

	for (i = 0; i < utl->u_odsize; i++) {
		pte = dptopte(p, i);
		if (pte->pg_v || pte->pg_fod == 0
		    && (pte->pg_pfnum || pte->pg_m || pte->pg_swapm))
			panic("swdspt");
	}

	sszpt = clrnd(ctopt(p->p_ssize));
	if (sszpt == p->p_stakpt)
		swptstat.spteasy++;
	else if (sszpt < p->p_stakpt)
		swptstat.sptshrink++;
	else
		swptstat.sptexpand++;

	if (utl->u_ossize != 0) {
		start = sszpt - clrnd(ctopt(utl->u_ossize));
		(void)swap(p,
		    p->p_swaddr+ctod(clrnd(UPAGES))+ctod(dszpt)+ctod(start),
		    (caddr_t)&p->p_stakbr[(rdwri==B_READ) ? start*NPTEPG : 0], 
		    clrnd(ctopt(utl->u_ossize)) * NBPG, 
		    rdwri, B_PAGET, swapdev, 0);
	}

	for (i = 0; i < utl->u_ossize; i++) {
		pte = sptopte(p, i);
		if (pte->pg_v || pte->pg_fod == 0
		    && (pte->pg_pfnum || pte->pg_m || pte->pg_swapm))
			panic("swdspt");
		XPRINTF(XPR_SWAP,"swdspt stack pte %r",*(int *)pte,pte_desc,0,0);
	}
}
#endif mips
