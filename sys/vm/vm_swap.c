#ifndef lint
static	char	*sccsid = "@(#)vm_swap.c	4.1	(ULTRIX)	7/2/90";
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
/*
 *
 *   Modification history:
 *
 * 19-Jun-90 -- jmartin
 *	Fixes for "panic: vrelvm rssize"
 *
 *  11 Dec 89 jaa
 *	change dynamic swap to account for swap up front (ala v3.1) 
 * 	but actually do the allocation only when pushing the page/process
 *
 * 04 Dec 89 -- sekhar
 *	minor changes to swapout to track swap useage.
 *
 * 30-Nov-89 -- jmartin
 *	Support for coprocessor access to process virtual memory.
 *
 * 13 Oct 89 -- gmm
 *	Changes for mips smp support. Modify the logic for checking invalid
 *	tlbpid
 * 31 Aug 89 -- gg
 *	added checks in swdspt() routine to take care of 0 data/stackptes
 *	case.
 *
 * 30 Aug 89 -- gg
 *	fix panic swdspt - stack
 *
 * 24 Jul 89 -- jmartin
 *	Change swapout to be called only from sched (the swapper).
 *
 * 12 Jun 89 -- gg
 *	Dynamic swap changes:
 *	---------------------
 *	Added call vax_noaccess() in swapin() to clear all ptes from end of
 *	data to beginning of stack.
 *	Added code to calculate and allocate swap before doing actual swapout.
 *	swdspt() routine modified to swap multiple page table entries.
 *	Modified swpt() routine to take the disk block number directly
 *	instead of calculating inside the swpt().
 *
 * 15 Dec 88 -- jmartin
 *	SMP locking for shared memory data structures (cf. h/shm.h)
 *	Integration of v3.0 shared memory changes, i.e. KM_ALLOC of
 *	per-process shared memory structures.
 *
 * 31 Aug 88 -- jmartin
 *	Protect x_rssize and x_ccount with XLOCK bit.
 *
 * 28 Jul 88 -- miche
 *	protect remaining scheduling fields in the process structure
 *
 * 25 Jul 88 -- jmartin
 *	Use macros SET_P_VM and CLEAR_P_VM to manipulate the field
 *	(struct proc *)foo->p_vm under SMP lock.
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
#include "../h/types.h"
#include "../machine/cpu.h"

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
#include "../h/smp_lock.h"
#ifdef mips
#include "../h/cpudata.h"
#endif mips
#ifdef vax
#include "../machine/mtpr.h"
#endif vax

extern struct sminfo sminfo;
extern int swapfrag;

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
	extern memfree();

	if (xp = p->p_textp) 
		X_LOCK(xp);
	if (p->p_smcount > 0) { /* was process using shared memory  SHMEM */
		if (p->p_sm == (struct p_sm *) NULL)
			panic("swapin: p_sm1");
		for(i = 0; i < p->p_smcount; i++)
			if (sp = p->p_sm[i].sm_p)
				SM_LOCK(sp);
	}
#ifdef vax
	p->p_szpt = clrnd(ctopt(p->p_ssize+p->p_dsize +
				p->p_smsize +	/* SHMEM */
				p->p_tsize+HIGHPAGES));
#endif vax
#ifdef mips
        p->p_textpt = clrnd(ctopt(p->p_tsize));
        p->p_datapt = clrnd(ctopt(p->p_dsize));
        p->p_stakpt = clrnd(ctopt(p->p_ssize));
#endif mips
        if (vgetpt(p, memall, memfree) == 0)
                goto nomem;
#ifdef mips
        if (xp) {
                if (!xlink(p)) {
                        vrelpt(p);
                        goto nomem;
                }
        }
#endif mips
	if (vgetu(p, memall, Swapmap, &swaputl, (struct user *)0) == 0) {
#ifdef mips
                if (xp) 
			xccdec(xp, p);
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
	vrelswu(p);
#ifdef vax
	/*
	 * clear all ptes from end of data to p0lr -
	 * shmem ptes will be initialised anyway by smlink
	 */
	vax_noaccess(p);
	if (xp) 
		xlink(p);
#endif vax
	if (xp)
		X_UNLOCK(xp);
#ifdef mips
        p->p_tlbpid = -1;
        clear_tlbmappings(p);
#endif mips
	if (p->p_smcount > 0){		/* SHMEM */
		if (p->p_sm == (struct p_sm *) NULL)
			panic("swapin: p_sm2");
		psmp = &p->p_sm[0];
		for(i = 0; i < p->p_smcount; i++, psmp++)
			if (sp = psmp->sm_p){
				smlink(p, sp);
				SM_UNLOCK(sp);
			}
		}

#ifndef mips
	s = splimp();
#else mips
	s = splclock();
#endif mips
	smp_lock(&lk_p_vm, LK_RETRY);
	p->p_rssize = 0;
	smp_unlock(&lk_p_vm);
#ifdef vax
	(void)spl6();
#endif vax
	smp_lock(&lk_rq,LK_RETRY);
	if (p->p_stat == SRUN) {
		setrq(p);
	}
	p->p_sched |= SLOAD;
	smp_unlock(&lk_rq);
	splx(s);
	p->p_time = 0;
	multprog++;
	cnt.v_swpin++;
	return (1);

nomem:
	if (xp)
		X_UNLOCK(xp);

	if (p->p_smcount > 0){		/* SHMEM */
		if (p->p_sm == (struct p_sm *) NULL)
			panic("swapin: p_sm3");
		psmp = &p->p_sm[0];
		for(i=0; i < p->p_smcount; i++, psmp++){
			if (sp = psmp->sm_p)
				SM_UNLOCK(sp);
		}
	}

	return (0);
}

/*
 * Swap out process p.
 * ds and ss are the old data size and the stack size
 * of the process, and are supplied during page table
 * expansion swaps.
 */
swapout(p)
	register struct proc *p;
{
	register struct user *utl;
	register int a, i;
	register struct text *xp;
	int rc = 1;

	if (u.u_procp == p)
		panic("swapout: attempt to swap self");
#ifdef mips
        utl =
            (struct user *)PHYS_TO_K0(ptob(p->p_addr[0].pg_pfnum));
        checkfp(p, 0);
#endif mips
#ifdef vax
	utl = &forkutl;
	uaccess(p, Forkmap, utl);
#endif vax
	if (vgetswu(p) == 0) {
		swfail_stat.uarea_fail++;
		swkill(p, "swapout: no swap u available");
		rc = 0;
		goto out;
	}

	/*
	 * vsalloc may return 0 due to swapmap fragmentation.
	 */
	if(vsalloc(p->p_dmap) == 0 || vsalloc(p->p_smap) == 0){
		swfail_stat.frag_fail++;
		vrelswu(p);
		swkill(p, "swapout: no swap available for data or stack ");
		rc = 0;
		goto out;
	}
	utl->u_ru.ru_nswap++;
	SET_P_VM(p, SLOCK);

	/*
	 * With the process immobilized, make sure its page tables are
	 * up-to-date on this processor.
	 */
#ifdef vax
	mtpr(TBIA, 0);
#else mips
	flush_tlb();
#endif vax ^ mips

	if (xp = p->p_textp) {
		X_LOCK(xp);
		if (xccdec(xp, p))
			if (xp->x_ccount == 0)
				xp->x_swrss = xp->x_rssize;
		X_UNLOCK(xp);
	}
	if (p->p_smcount > 0) {	/* SHMEM */
		if (p->p_sm == (struct p_sm *) NULL)
			panic("swapout: p_sm");
		for(i = 0; i < p->p_smcount; i++) {
			register struct smem *sp;
			if (sp = p->p_sm[i].sm_p) {
				SM_LOCK(sp);
				smccdec(sp, p);
				SM_UNLOCK(sp);
			}
		}
	}
	a = splimp();
	smp_lock(&lk_p_vm, LK_RETRY);
	p->p_swrss = p->p_rssize;
	if ((p->p_vm & SSWAP) == 0) {
		utl->u_odsize = utl->u_dsize;
		utl->u_osmsize = utl->u_smsize;
		utl->u_ossize = utl->u_ssize;
	}
	smp_unlock(&lk_p_vm);
	(void) splx(a);
#ifdef mips
	(void) clear_foreign_tlbs(p, btop(p->p_databr), p->p_datapt, CSYS);
#else vax
	(void) clear_foreign_tlbs(p, btop(p->p_p0br), p->p_szpt, CSYS);
#endif vax or mips
	vsswap(p, dptopte(p, 0), CDATA, 0, utl->u_odsize, p->p_dmap);
	vsswap(p, sptopte(p, CLSIZE-1), CSTACK, 0, utl->u_ossize, p->p_smap);
	a = splimp();
	smp_lock(&lk_p_vm, LK_RETRY);
	if (p->p_rssize != 0)
		panic("swapout rssize");
	smp_unlock(&lk_p_vm);
	(void) splx(a);

	swdspt(p, utl, B_WRITE);

	vrelu(p, 1);
	a = spl6();
	smp_lock(&lk_rq, LK_RETRY);
#ifdef mips
        if ((p->p_tlbpid != -1) && (CURRENT_CPUDATA->cpu_tps[p->p_tlbpid].tps_procpid == p->p_pid))
                release_tlbpid(p);
        /* clear_tlbmappings(p); */
#endif mips
	if (p->p_sched & SLOAD)
		panic("swapout");
	smp_unlock(&lk_rq);
	splx(a);
	vrelpt(p);
	CLEAR_P_VM(p, SLOCK|SSWAP);
	p->p_time = 0;

	multprog--;
	cnt.v_swpout++;

out:
	if (rc == 0) {
#ifdef vax
		a = spl6();
#endif vax
#ifdef mips
		a = splclock();
#endif mips
		smp_lock(&lk_rq,LK_RETRY);
		p->p_sched |= SLOAD;
		if (p->p_stat == SRUN)
			setrq(p);
		smp_unlock(&lk_rq);
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
	register int *dp, poff;
	register int i;
	unsigned int dataptes_tosave, stackptes_tosave;
	register int temp = (dtob(swapfrag) - ctob(UPAGES))/sizeof(struct pte);
	register int nfrag = dtob(swapfrag)/sizeof(struct pte);

	szpt = clrnd(ctopt(p->p_tsize+p->p_dsize +
				p->p_smsize +		/* SHMEM */
				p->p_ssize+HIGHPAGES));

	dataptes_tosave = min(p->p_dsize, utl->u_odsize);
	stackptes_tosave = min(p->p_ssize, utl->u_ossize);

	tsz = p->p_tsize / NPTEPG;
	if (szpt == p->p_szpt) {
		swptstat.pteasy++;
		if (dataptes_tosave) {
			dp = p->p_dmap->dm_ptdaddr;
			i = (p->p_tsize+dataptes_tosave) - (tsz * NPTEPG);
			poff = tsz*NPTEPG;
			while(i > nfrag) {
				swpt(rdwri, p, *dp++, poff,dtob(swapfrag));
				i -= nfrag;
				poff += nfrag;
			}
			if (i)
				swpt(rdwri, p, *dp, poff, i * sizeof(struct pte));
		}
		poff = (szpt * NPTEPG) - (HIGHPAGES+stackptes_tosave);
		dp = p->p_smap->dm_ptdaddr;
		i = stackptes_tosave;
		if (i > temp){
			swpt(rdwri, p, *dp+ctod(UPAGES), poff, dtob(swapfrag)
							- ctob(UPAGES));
			i -= temp;
			dp++;
			poff += temp;
			while(i > nfrag) {
				swpt(rdwri, p, *dp++, poff, dtob(swapfrag));
				poff += nfrag;
				i -= nfrag;
			}
			swpt(rdwri, p, *dp, poff, i * sizeof(struct pte));
			goto check;
		}
		if (i)
			swpt(rdwri, p, *dp+ctod(UPAGES), poff, i * sizeof(struct pte));
		goto check;
	}
	if (szpt < p->p_szpt)
		swptstat.ptshrink++;
	else
		swptstat.ptexpand++;
	ssz = clrnd(ctopt(utl->u_ossize+HIGHPAGES));
	if (szpt < p->p_szpt && utl->u_odsize && (utl->u_ossize+HIGHPAGES)) {
		/*
		 * Page tables shrinking... see if last text+data and
		 * last stack page must be merged... if so, copy
		 * stack pte's from last stack page to end of last
		 * data page, and decrease size of stack pt to be swapped.
		 */
		tdlast = (p->p_tsize + utl->u_odsize
					+ utl->u_osmsize)	/* SHMEM */
					% (NPTEPG * CLSIZE);
		slast = (utl->u_ossize + HIGHPAGES) % (NPTEPG * CLSIZE);
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
	poff = (p->p_szpt*NPTEPG) - (stackptes_tosave+HIGHPAGES);
	dp = p->p_smap->dm_ptdaddr;
	i = stackptes_tosave+HIGHPAGES;
	if (i > temp){
		swpt(rdwri, p, *dp+ctod(UPAGES), poff, dtob(swapfrag)
						-ctob(UPAGES));
		i -= temp;
		dp++;
		poff += temp;
		while(i > nfrag) {
			swpt(rdwri, p, *dp++, poff, dtob(swapfrag));
			poff += nfrag;
			i -= nfrag;
		}
		swpt(rdwri, p, *dp, poff, i * sizeof(struct pte));
		goto merge;
	}
	if (i)
		swpt(rdwri, p, *dp+ctod(UPAGES), poff, i * sizeof(struct pte));
merge:
	if (dataptes_tosave) {
		dp = p->p_dmap->dm_ptdaddr;
		i = (p->p_tsize+dataptes_tosave) - (tsz * NPTEPG);
		poff = tsz*NPTEPG;
		while(i > nfrag) {
			swpt(rdwri, p, *dp++, poff,dtob(swapfrag));
			i -= nfrag;
			poff += nfrag;
		}
		if (i)
			swpt(rdwri, p, *dp, poff, i * sizeof(struct pte));
	}
check:
	for (i = 0; i < dataptes_tosave; i++) {
		pte = dptopte(p, i);
		if (pte->pg_v || pte->pg_fod == 0 && 
					(pte->pg_pfnum||pte->pg_m))
			panic("swdspt: data");
	}
	for (i = 0; i < stackptes_tosave; i++) {
		pte = sptopte(p, i);
		if (pte->pg_v || pte->pg_fod == 0 && (pte->pg_pfnum||pte->pg_m))
			panic("swdspt: stack");
	}
}


swpt(rdwri, p, addr, a, n)
	int rdwri;
	struct proc *p;
	int addr, a, n;
{

	if (n <= 0)
		return;
	swap(p, addr,
	    (caddr_t)&p->p_p0br[a], n, rdwri, B_PAGET, swapdev, 0);
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
	register int *dp, poff;
	register int i;
	int start;
	unsigned int dataptes_tosave, stackptes_tosave;
	register int nfrag = dtob(swapfrag)/sizeof(struct pte);

	dataptes_tosave = min(p->p_dsize, utl->u_odsize);
	stackptes_tosave = min(p->p_ssize, utl->u_ossize);

	dszpt = clrnd(ctopt(p->p_dsize));
	if (dszpt == p->p_datapt)
		swptstat.dpteasy++;
	else if (dszpt < p->p_datapt)
		swptstat.dptshrink++;
	else
		swptstat.dptexpand++;

	if (dataptes_tosave != 0) {
		dp = p->p_dmap->dm_ptdaddr;
		i =  (clrnd(ctopt(dataptes_tosave)) * NPTEPG);
		poff = 0;
		while(i > nfrag) {
		(void)swap(p, *dp++, (caddr_t)&p->p_databr[poff],
		   	dtob(swapfrag), rdwri, B_PAGET, swapdev, 0);
			i -= nfrag;
			poff += nfrag;
		}
		(void)swap(p, *dp, (caddr_t)&p->p_databr[poff],
		   i * sizeof(struct pte), rdwri, B_PAGET, swapdev, 0);
	}

	for (i = 0; i < dataptes_tosave; i++) {
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

	if (stackptes_tosave != 0) {
		register int temp ;
		i = (clrnd(ctopt(stackptes_tosave)) * NPTEPG);
		dp = p->p_smap->dm_ptdaddr;
		start = sszpt - clrnd(ctopt(stackptes_tosave));
		temp = (dtob(swapfrag) -ctob(clrnd(UPAGES))- ctob(start))/sizeof(struct pte);
		poff = (rdwri==B_READ)? start*NPTEPG : 0;
		if (i > temp){
		    (void)swap(p, *dp+ctod(clrnd(UPAGES))+ctod(start), 
		    	(caddr_t)&p->p_stakbr[poff],
			(dtob(swapfrag) - ctob(clrnd(UPAGES)) - ctob(start)), 
			rdwri, B_PAGET, swapdev, 0);
			i -= temp;
			dp++;
			poff += temp;
			while(i > nfrag) {
		    		(void)swap(p, *dp++,
		    		(caddr_t)&p->p_stakbr[poff], dtob(swapfrag),
		    		rdwri, B_PAGET, swapdev, 0);
				poff += nfrag;
				i -= nfrag;
			}
		    	(void)swap(p, *dp,
		    	(caddr_t)&p->p_stakbr[poff], i * sizeof(struct pte),
		    	rdwri, B_PAGET, swapdev, 0);
			goto merge;
		}
		(void)swap(p, *dp+ctod(clrnd(UPAGES))+ctod(start),
		(caddr_t)&p->p_stakbr[poff], i * sizeof(struct pte),
		rdwri, B_PAGET, swapdev, 0);
	}
merge:
	for (i = 0; i < stackptes_tosave; i++) {
		pte = sptopte(p, i);
		if (pte->pg_v || pte->pg_fod == 0
		    && (pte->pg_pfnum || pte->pg_m || pte->pg_swapm))
			panic("swdspt");
	}
}
#endif mips
