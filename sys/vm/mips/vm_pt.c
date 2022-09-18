#ifndef lint
static	char	*sccsid = "@(#)vm_pt.c	4.1	(ULTRIX)	7/2/90";
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

/*----------------------------------------------------------------------
 * Modification History
 *
 * 14 Sep 88 -- jaa
 *	Fixed vgetu() for mips box
 *
 * 15 Dec 86 -- depp
 *	Added fix to shared memory handling, in regards to a process' SM 
 *	page table swapping space.  
 *
 * 11 Sep 86 -- koehler
 *	gnode name change
 *
 * 18 Jun 86 -- depp
 *	Added ZFOD to distsmpte
 *
 * 17 Mar 86 -- depp
 *	Added fix to "stale stack" problem in "vrelu" and "vrelpt"
 *
 * 12-Feb-86 -- jrs
 *	Added calls to tbsync() for mp translation buffer control
 *
 * 03 Jan 86 -- depp
 *	Added check in "distsmpte" for NULL sm_caddr.
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
 * 19 Jul 85 -- depp
 *	Changed method of updating pte's in "distsmpte" so that only
 *	pg_v and pg_pfnum are updated everytime and pg_m is updated
 *	only if the pg_cm bit is set in the gpte.
 *
 *	Removed the pg_m set in "dirtysm".  It wasn't needed.
 *
 * 03 Apr 85 -- depp
 *	Added new routine "dirtysm" to indicate if any shared memory pte
 *	is dirty
 *
 * 11 Mar 85 -- depp
 *	Add in System V shared memory support including the following new
 *	routines:
 *		vgetsmpt	allocation shared memory pte
 *		vinitsmpt	initialize shared memory pte
 *		distsmpte	update all pte's in all processes that are
 *				sharing a given page
 *		
 * 19 Dec 84 -- jrs
 *	Changed setjmp's to savectx for swap recovery fixes
 *	Derived from 4.2 BSD, labeled:
 *		vm_pt.c	6.1	84/07/29
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
#include "../h/interlock.h"

#include "../machine/pte.h"
#include "../machine/cpu.h"

#ifdef vax
#include "../vax/mtpr.h"
#endif vax

#include "../h/ipc.h"
#include "../h/shm.h"
#ifndef V_NOOP
#define V_NOOP 0
#define V_CACHE 1
#endif V_NOOP
extern struct sminfo sminfo;
extern int extracpu;

#ifdef vax
/*
 * Get page tables for process p.  Allocator
 * for memory is argument; process must be locked
 * from swapping if vmemall is used; if memall is
 * used, call will return w/o waiting for memory.
 * In any case an error return results if no user
 * page table space is available.
 */
vgetpt(p, pmemall)
	register struct proc *p;
	int (*pmemall)();
{
	register long a;
	register int i;
	extern memall();
	int ret;

	if (p->p_szpt == 0)
		panic("vgetpt");
	/*
	 * Allocate space in the kernel map for this process.
	 * Then allocate page table pages, and initialize the
	 * process' p0br and addr pointer to be the kernel
	 * virtual addresses of the base of the page tables and
	 * the pte for the process pcb (at the base of the u.).
	 */
	a = rmalloc(kernelmap, (long)p->p_szpt);
	if (a == 0)
		return (0);
	if(pmemall == memall)
		ret = (*pmemall)(&Usrptmap[a], p->p_szpt, p, CSYS, NULL,V_NOOP);
	else
		ret = (*pmemall)(&Usrptmap[a], p->p_szpt, p, CSYS);
	if(ret == 0) {
		rmfree(kernelmap, (long)p->p_szpt, a);
		return (0);
	}
	p->p_p0br = kmxtob(a);
	p->p_addr = uaddr(p);
	/*
	 * Now validate the system page table entries for the
	 * user page table pages, flushing old translations
	 * for these kernel virtual addresses.  Clear the new
	 * page table pages for clean post-mortems.
	 */
	vmaccess(&Usrptmap[a], (caddr_t)p->p_p0br, p->p_szpt);
	for (i = 0; i < p->p_szpt; i++)
		clearseg(Usrptmap[a + i].pg_pfnum);
	return (1);
}
#endif vax


/* VGETSMPT - Get page tables for shared memory segment.	*/
/*	Process must be locked from swapping. In any case	*/
/*	an error return results if no user page table space is	*/
/*	available.	SHMEM					*/
vgetsmpt(sp)
    register struct smem *sp;
{
	register long a;
	register int i;
	register int smszpt;		/* in page table pages	*/

#ifdef mips
	XPRINTF(XPR_SM,"enter vgetsmpt",0,0,0,0);
#endif mips
	if (sp->sm_size == 0)
		panic("vgetsmpt");

			/* Allocate space in the kernel map for	*/
			/* this shared memory segment. Then	*/
			/* allocate page table pages, and	*/
			/* initialize the segments' "sm_ptaddr"	*/
			/* to be the kernel virtual address of	*/
			/* the base of the page table.		*/
	smszpt = clrnd(ctopt(btoc(sp->sm_size)));
	a = rmalloc(kernelmap, (long)smszpt);
	if (a == 0)
		return (0);
	if (memall(&Usrptmap[a], smszpt, &proc[0], CSYS, NULL, V_NOOP) == 0) {
		rmfree(kernelmap, (long)smszpt, a);
		return (0);
	}
	sp->sm_ptaddr = kmxtob(a);


			/* Now validate the system page table	*/
			/* entries for the user page table	*/
			/* pages, flushing old translations for	*/
			/* these kernel virtual addresses.	*/
			/* Clear the new page table pages for	*/
			/* clean post-mortems.			*/
	vmaccess(&Usrptmap[a], (caddr_t)sp->sm_ptaddr, smszpt, DO_CACHE);
	for (i=0; i < smszpt; i++)
		clearseg(Usrptmap[a + i].pg_pfnum);
#ifdef mips
	XPRINTF(XPR_SM,"vgetsmpt ptaddr 0x%x; #ptpgs %d\n",
		sp->sm_ptaddr,smszpt,0,0);
#endif
	return(1);
}

#ifdef mips
/*
 * Get data and stack page tables for process p.  Allocator
 * for memory is argument; process must be locked
 * from swapping if vmemall is used; if memall is
 * used, call will return w/o waiting for memory.
 * In any case an error return results if no user
 * page table space is available.
 */
vgetpt(p, pmemall, pmemfree)
	register struct proc *p;
	int (*pmemall)();
	int (*pmemfree)();
{
	register long ad = 0;
	register long as = 0;
	register int i, ret;
#ifdef CACHETRICKS
	register struct cmap *c;
	register unsigned pf;
#endif CACHETRICKS

XPRINTF(XPR_VM,"enter vgetpt",0,0,0,0);
	/*
	 * Allocate space in the kernel map for this process.
	 * Then allocate page table pages, and initialize the
	 * process' base registers and addr pointers to be the kernel
	 * virtual addresses of the base of the page tables and
	 * the pte for the process pcb (at the base of the u.).
	 */
	if (p->p_datapt) {
		ad = rmalloc(kernelmap, (long)p->p_datapt);
		if (ad == 0)
			return (0);
		if(pmemall == memall)
			ret = (*pmemall)(&Usrptmap[ad], p->p_datapt, p, CSYS, NULL,V_NOOP);
		else
			ret = (*pmemall)(&Usrptmap[ad], p->p_datapt, p, CSYS);
		if(ret == 0) {
			rmfree(kernelmap, (long)p->p_datapt, ad);
			return (0);
		}
	}

	if (p->p_stakpt) {
		as = rmalloc(kernelmap, (long)p->p_stakpt);
		if (as == 0) {
			if (p->p_datapt) {
				(*pmemfree)(&Usrptmap[ad], p->p_datapt);
				rmfree(kernelmap, (long)p->p_datapt, ad);
			}
			return (0);
		}
		if(pmemall == memall)
			ret = (*pmemall)(&Usrptmap[as], p->p_stakpt, p, CSYS, NULL,V_NOOP);
		else
			ret = (*pmemall)(&Usrptmap[as], p->p_stakpt, p, CSYS);
		if(ret == 0) {
			if (p->p_datapt) {
				(*pmemfree)(&Usrptmap[ad], p->p_datapt);
				rmfree(kernelmap, (long)p->p_datapt, ad);
			}
			rmfree(kernelmap, (long)p->p_stakpt, as);
			return (0);
		}
	}

	p->p_databr = kmxtob(ad);
	p->p_stakbr = kmxtob(as);
	p->p_textbr = 0;

	/*
	 * Now validate the system page table entries for the
	 * user page table pages, flushing old translations
	 * for these kernel virtual addresses.  Clear the new
	 * page table pages for clean post-mortems.
	 */
	if (p->p_datapt) {
#ifdef mips
		vmaccess(&Usrptmap[ad], (caddr_t)p->p_databr, p->p_datapt, DO_CACHE);
#else !mips
		vmaccess(&Usrptmap[ad], (caddr_t)p->p_databr, p->p_datapt);
#endif !mips
		for (i = 0; i < p->p_datapt; i++) {
#ifdef USE_IDLE
			/* don't bother if it's already done */
			if (cmap[pgtocm(Usrptmap[ad + i].pg_pfnum)].c_zero) {
				extern int v_zero_pt_hits;

				v_zero_pt_hits++;
				cmap[pgtocm(Usrptmap[ad+i].pg_pfnum)].c_zero=0;
			}
			else {
				extern int v_zero_pt_misses;

				v_zero_pt_misses++;
				clearseg(Usrptmap[ad + i].pg_pfnum);
			}
#else
			clearseg(Usrptmap[ad + i].pg_pfnum);
#endif
#ifdef CACHETRICKS
			if (Usrptmap[ad + i].pg_n == 0) {
				pf = Usrptmap[ad + i].pg_pfnum;
				c = &cmap[pgtocm(pf)];
				c->c_icachecnt = icachecnt[pf & icachemask];
				c->c_dcachecnt = dcachecnt[pf & dcachemask];
			}
#endif CACHETRICKS
		}
	}

	if (p->p_stakpt) {
#ifdef mips
		vmaccess(&Usrptmap[as], (caddr_t)p->p_stakbr, p->p_stakpt, DO_CACHE);
#else !mips
		vmaccess(&Usrptmap[as], (caddr_t)p->p_stakbr, p->p_stakpt);
#endif !mips
		for (i = 0; i < p->p_stakpt; i++) {
#ifdef USE_IDLE
			/* don't bother if it's already done */
			if (cmap[pgtocm(Usrptmap[as + i].pg_pfnum)].c_zero) {
				extern int v_zero_pt_hits;

				v_zero_pt_hits++;
				cmap[pgtocm(Usrptmap[as+i].pg_pfnum)].c_zero=0;
			}
			else {
				extern int v_zero_pt_misses;

				v_zero_pt_misses++;
				clearseg(Usrptmap[as + i].pg_pfnum);
			}
#else
			clearseg(Usrptmap[as + i].pg_pfnum);
#endif
#ifdef CACHETRICKS
			if (Usrptmap[as + i].pg_n == 0) {
				pf = Usrptmap[as + i].pg_pfnum;
				c = &cmap[pgtocm(pf)];
				c->c_icachecnt = icachecnt[pf & icachemask];
				c->c_dcachecnt = dcachecnt[pf & dcachemask];
			}
#endif CACHETRICKS
		}
	}

	return (1);
}
#endif mips

/*
 * Initialize text portion of page table.
 */
vinitpt(p)
	struct proc *p;
{
	register struct text *xp;
	register struct proc *q;
	register struct pte *pte;
	register int i;
	struct pte proto;
#ifdef mips
        register long at = 0;
#endif mips

#ifdef mips
	XPRINTF(XPR_VM,"enter vinitpt",0,0,0,0);
#endif mips
	xp = p->p_textp;
	if (xp == 0)
#ifdef vax
		return;
#endif vax
#ifdef mips
                return (1);
#endif mips
	pte = tptopte(p, 0);
	/*
	 * If there is another instance of same text in core
	 * then just copy page tables from other process.
	 */
	if (q = xp->x_caddr) {
#ifdef vax
		bcopy((caddr_t)tptopte(q, 0), (caddr_t)pte,
		    (unsigned) (sizeof(struct pte) * xp->x_size));
#endif vax
#ifdef mips
                p->p_textbr = q->p_textbr;
                p->p_textpt = q->p_textpt;
#endif mips
		goto done;
	}

#ifdef mips
	/*
	 * allocate text page table if it cannot be shared
	 */
	p->p_textpt = ctopt(xp->x_size);
	at = rmalloc(kernelmap, (long)p->p_textpt);
	if (at == 0)
		return (0);
	if (vmemall(&Usrptmap[at], p->p_textpt, p, CSYS) == 0) {
		rmfree(kernelmap, (long)p->p_textpt, at);
		return (0);
	}
	p->p_textbr = kmxtob(at);
#ifdef mips
	vmaccess(&Usrptmap[at], (caddr_t)p->p_textbr, p->p_textpt, DO_CACHE);
#else !mips
	vmaccess(&Usrptmap[at], (caddr_t)p->p_textbr, p->p_textpt);
#endif !mips
	for (i=0; i < p->p_textpt; i++) {
#ifdef USE_IDLE
		/* don't bother if it's already done */
		if (cmap[pgtocm(Usrptmap[at+i].pg_pfnum)].c_zero) {
			extern int v_zero_pt_hits;

			v_zero_pt_hits++;
			cmap[pgtocm(Usrptmap[at+i].pg_pfnum)].c_zero=0;
		}
		else {
			extern int v_zero_pt_misses;

			v_zero_pt_misses++;
			clearseg(Usrptmap[at+i].pg_pfnum);
		}
#else
		clearseg(Usrptmap[at+i].pg_pfnum);
#endif USE_IDLE
	}
	pte = tptopte(p, 0);
#endif mips

	/*
	 * Initialize text page tables, zfod if we are loading
	 * the text now; unless the process is demand loaded,
	 * this will suffice as the text will henceforth either be
	 * read from a file or demand paged in.
	 */
	*(int *)&proto = PG_URKR;
	if (xp->x_flag & XLOAD) {
		proto.pg_fod = 1;
		((struct fpte *)&proto)->pg_fileno = PG_FZERO;
	}
	for (i = 0; i < xp->x_size; i++)
		*pte++ = proto;
	if ((xp->x_flag & XPAGI) == 0)
		goto done;
	/*
	 * Text is demand loaded.  If process is not loaded (i.e. being
	 * swapped in) then retrieve page tables from swap area.  Otherwise
	 * this is the first time and we must initialize the page tables
	 * from the blocks in the file system.
	 */
	if (xp->x_flag & XLOAD)
		vinifod((struct fpte *)tptopte(p, 0), PG_FTEXT, xp->x_gptr,
#ifdef vax
		    (daddr_t)1, 
#endif vax
#ifdef mips
		    (daddr_t)0, 
#endif mips
		    xp->x_size);

	else
		swap(p, xp->x_ptdaddr, (caddr_t)tptopte(p, 0),
		    xp->x_size * sizeof (struct pte), B_READ,
		    B_PAGET, swapdev, 0);
done:
	/*
	 * In the case where we are overlaying ourself with new page
	 * table entries, old user-space translations should be flushed.
	 */
#ifdef mips
        /*
         * I don't think this really has to be done. mad
         */
        newptes(p, tptov(p, 0), xp->x_size);
#endif mips
#ifdef vax
	if (p == u.u_procp)
		newptes(tptopte(p, 0), tptov(p, 0), xp->x_size);
	else
		p->p_flag |= SPTECHG;
#endif vax
	return(1);
}

#ifdef vax
/* VINITSMPT - Initialize shared memory portion of page table	*/
/*	for the given shared memory segment.			*/
/* As a short cut, to get this done in a hurry, I have made	*/
/*	shared memory segment page tables wired-down. Therefore,*/
/*	most of the following code, which is analogous to	*/
/*	VINITPT, is commented out. (To be resurrected in some	*/
/*	future version, I hope.)				*/
/* LeRoy Fundingsland    1/18/85    DEC		SHMEM		*/
vinitsmpt(p, sp)
    register struct proc *p;
    register struct smem *sp;
{
#ifdef mips
	XPRINTF(XPR_VM,"enter vinitsmpt",0,0,0,0);
#endif mips
	register struct pte *pte, *pte1;
	register int i, smindex;
	register int smsize;		/* SMS size in clicks	*/
#ifdef notdef
	struct pte proto;
#endif notdef

	if(sp == NULL)
		return;

	if(p->p_sm == (struct p_sm *) NULL) {
		panic("vinitsmpt: p_sm");
	}

	for(smindex=0; smindex < sminfo.smseg; smindex++)
		if(p->p_sm[smindex].sm_p == sp)
			break;
	if(smindex >= sminfo.smseg)
		panic("vinitsmpt");
	pte = p->p_p0br + p->p_sm[smindex].sm_spte;
	smsize = clrnd(btoc(sp->sm_size));

	/* set up process' ptes */
	pte1 = sp->sm_ptaddr;
	for(i=0; i < smsize; i++)
/*
		if (pte1->pg_fod)
*/
			*(int *)pte++ = *(int *)pte1++ |
				(int)p->p_sm[smindex].sm_pflag;
/*
		else
			*(int *)pte++ = *(int *)pte1++ |  PG_ALLOC |
				(int)p->p_sm[smindex].sm_pflag;
*/
	goto done;

#ifdef notdef
	/*
	 * Initialize text page tables, zfod if we are loading
	 * the text now; unless the process is demand loaded,
	 * this will suffice as the text will henceforth either be
	 * read from a file or demand paged in.
	 */
	*(int *)&proto = PG_URKR;
	if (xp->x_flag & XLOAD) {
		proto.pg_fod = 1;
		((struct fpte *)&proto)->pg_fileno = PG_FZERO;
	}
	for (i = 0; i < xp->x_size; i++)
		*pte++ = proto;
	if ((xp->x_flag & XPAGI) == 0)
		goto done;
	/*
	 * Text is demand loaded.  If process is not loaded (i.e. being
	 * swapped in) then retrieve page tables from swap area.  Otherwise
	 * this is the first time and we must initialize the page tables
	 * from the blocks in the file system.
	 */
	if (xp->x_flag & XLOAD)
		vinifod((struct fpte *)tptopte(p, 0), PG_FTEXT, xp->x_gptr,
		    (daddr_t)1, xp->x_size);
	else
		swap(p, xp->x_ptdaddr, (caddr_t)tptopte(p, 0),
		    xp->x_size * sizeof (struct pte), B_READ,
		    B_PAGET, swapdev, 0);
#endif notdef
done:
	/*
	 * In the case where we are overlaying ourself with new page
	 * table entries, old user-space translations should be flushed.
	 */
	if (p == u.u_procp)
		newptes(tptopte(p,0), tptov(p,p->p_sm[smindex].sm_spte),
							smsize);
	else
		p->p_flag |= SPTECHG;
}

/*
 * Update the page tables of all processes linked
 * to a particular text segment, by distributing
 * dpte to the the text page at virtual frame v.
 *
 * Note that invalidation in the translation buffer for
 * the current process is the responsibility of the caller.
 */
distpte(xp, tp, dpte)
	struct text *xp;
	register size_t tp;
	register struct pte *dpte;
{
	register struct proc *p;
	register struct pte *pte;
	register int i;

	for (p = xp->x_caddr; p; p = p->p_xlink) {
		pte = tptopte(p, tp);
		p->p_flag |= SPTECHG;
		if (pte != dpte)
			for (i = 0; i < CLSIZE; i++)
				pte[i] = dpte[i];
	}
}


/* DISTSMPTE - Update the page tables of all processes linked	*/
/*	to a particular shared memory segment, by distributing	*/
/*	dpte to the the shared memory page at virtual frame smp.*/
/*								*/
/*	Note that invalidation in the translation buffer for	*/
/*	the current process is the responsibility of the caller.*/
/* 			 SHMEM					*/
distsmpte(sp, smp, dpte, cm)
    register struct smem *sp;
    size_t smp;
    register struct pte *dpte;		/* Global PTE */
    int cm;				/* clear pg_m flag */
{
	register struct pte *pte;
	register int i, j;
	register struct proc *p;

	/* if the SMS is currently not attached	*/
	/* to any process then return.		*/
	if(sp->sm_ccount == 0 || sp->sm_caddr == NULL)
		return;

	p = sp->sm_caddr;
	if(p->p_sm == (struct p_sm *) NULL) {
		panic("distmpte: p_sm1");
	}

	for(i=0; i < sminfo.smseg; i++)
		if(p->p_sm[i].sm_p == sp)
			break;
	if(i >= sminfo.smseg)
		panic("distsmpte");

	/* if requested, clear pg_m bit in global PTE */
	if (cm) {
		dpte->pg_m = 0;
		distcl(dpte);
	}

	while(p){
		pte = p->p_p0br + p->p_sm[i].sm_spte+smp;

		/* this panic should eventually go away */
		if (pte->pg_v && dpte->pg_fod) {
			panic("distsmpte: PG_V && PG_FOD");
		}

		p->p_flag |= SPTECHG;
		/* NOTE: If SM ever has FFOD, this will change */
		if (dpte->pg_fod) {
			/* CAREFUL: I'm incrementing 'pte' */
			for (j=0; j < CLSIZE; j++, pte++) {
				((struct fpte *) pte)->pg_fod = 
					((struct fpte *) dpte)->pg_fod;
				((struct fpte *) pte)->pg_fileno = 
					((struct fpte *) dpte)->pg_fileno;
			}
		} else {
			/* CAREFUL: I'm incrementing 'pte' */
			for (j=0; j < CLSIZE; j++, pte++) {
				pte->pg_pfnum = (dpte + j)->pg_pfnum;
				pte->pg_v = (dpte + j)->pg_v;
				pte->pg_alloc = (dpte + j)->pg_alloc; 
				if (cm)
					pte->pg_m = 0;
			}
		}

		/* now for next proc in linked list	*/
		if((p = p->p_sm[i].sm_link) == NULL)
			break;

		if(p->p_sm == (struct p_sm *) NULL) {
			panic("distmpte: p_sm2");
		}

		for(i=0; i < sminfo.smseg; i++)
			if(p->p_sm[i].sm_p == sp)
				break;
		if(i >= sminfo.smseg)
			panic("distsmpte #2");
	}
}

/* 
 *  DIRTYSM -- checks for dirty ptes in process space
 */

dirtysm(sp,smp)
register struct smem *sp;	/* pointer to smem structure */
register size_t smp;		/* offset into SMS */
{
register struct proc *p;	/* Proc pointer */
register struct pte *pte;	/* pointer to Proc's pte */
register int i;		

#ifdef mips
	XPRINTF(XPR_VM,"enter dirtysm",0,0,0,0);
#endif mips
	/* if the SMS is currently not attached to any process then return */
	if(sp->sm_ccount == 0)
		return (0);

	/* SMS offset must be on cluster boundary */
	if (smp % CLSIZE)
		panic("dirtysm: smp");

	/* find SMS of first attached proc */
        p = sp->sm_caddr;

	if(p->p_sm == (struct p_sm *) NULL) {
		panic("dirtysm: p_sm1");
	}

	for(i = 0; i < sminfo.smseg; i++)
		if(p->p_sm[i].sm_p == sp)
			break;
	if(i >= sminfo.smseg)
		panic("dirtysm: no SMS");

	/*
	 * follow attached proc list, looking for associated dirty
	 * PTEs.  If one is found, then return (1), else return (0)
	 */
	while(p){
#ifdef vax
		pte = p->p_p0br + p->p_sm[i].sm_spte+smp;
#endif vax
#ifdef mips
		/* get the pte's */
#endif mips
		if (dirtycl(pte))
			return(1);

		/* now for next proc in linked list	*/
		if((p = p->p_sm[i].sm_link) == NULL)
			break;

		if(p->p_sm == (struct p_sm *) NULL) {
			panic("dirtysm: p_sm2");
		}

		/* find next proc's SMS */
		for(i=0; i < sminfo.smseg; i++)
			if(p->p_sm[i].sm_p == sp)
				break;
		if(i >= sminfo.smseg)
			panic("dirtysm: no SMS #2");
	}
	return (0);
}
#endif vax

#ifdef vax
/*
 * Release page tables of process p.  If the process is in context, then
 * "vmemfree" is notified that the released page frames should be placed
 * on the "ucmap" free list until after context switch.
 */
vrelpt(p)
	register struct proc *p;
{
	register int a;

	if (p->p_szpt == 0)
		return;
	a = btokmx(p->p_p0br);
	if (u.u_procp == p)
		(void) vmemfree(&Usrptmap[a],-(p->p_szpt));
	else
		(void) vmemfree(&Usrptmap[a], p->p_szpt);
	rmfree(kernelmap, (long)p->p_szpt, (long)a);
}
#endif vax

#ifdef mips
/*
 * Release data and stack page tables of process p.
 */
vrelpt(p)
	register struct proc *p;
{
	register int a;

	XPRINTF(XPR_VM,"enter vrelpt",0,0,0,0);
	if (p->p_datapt) {
		a = btokmx(p->p_databr);
		(void) vmemfree(&Usrptmap[a], p->p_datapt);
		rmfree(kernelmap, (long)p->p_datapt, (long)a);
	}
	if (p->p_stakpt) {
		a = btokmx(p->p_stakbr);
		(void) vmemfree(&Usrptmap[a], p->p_stakpt);
		rmfree(kernelmap, (long)p->p_stakpt, (long)a);
	}
	p->p_datapt = 0;
	p->p_stakpt = 0;
	p->p_databr = (struct pte *)0;
	p->p_stakbr = (struct pte *)0;
}
#endif mips

#define	Xu(a)	t = up->u_pcb.a; up->u_pcb.a = uq ->u_pcb.a; uq->u_pcb.a = t;
#define	Xup(a)	tp = up->u_pcb.a; up->u_pcb.a = uq ->u_pcb.a; uq->u_pcb.a = tp;
#define	Xp(a)	t = p->a; p->a = q->a; q->a = t;
#define	Xpp(a)	tp = p->a; p->a = q->a; q->a = tp;

/*
 * Pass the page tables of process p to process q.
 * Used during vfork().  P and q are not symmetric;
 * p is the giver and q the receiver; after calling vpasspt
 * p will be ``cleaned out''.  Thus before vfork() we call vpasspt
 * with the child as q and give it our resources; after vfork() we
 * call vpasspt with the child as p to steal our resources back.
 * We are cognizant of whether we are p or q because we have to
 * be careful to keep our u. area and restore the other u. area from
 * umap after we temporarily put our u. area in both p and q's page tables.
 */
#ifdef mips
/*
 * This mips version is symetric since the u-area is not mapped in the
 * page tables that are being swapped. Note that the umap argument is
 * not used for the mips version.
 * TODO: should the processes exchange tlbpids? They are exchanging mappings
 * except for the u-block which probably has the G bit set.
 */
#endif mips
vpasspt(p, q, up, uq, umap)
	register struct proc *p, *q;
	register struct user *up, *uq;
	struct pte *umap;
{
	int t;
	int s;
	struct pte *tp;
	register int i;

#ifdef mips
	XPRINTF(XPR_VM,"enter vpasspt",0,0,0,0);
#endif mips
#ifdef vax
	s = spl7();	/* conservative, and slightly paranoid */
#endif vax
#ifdef mips
	s = splhigh();	/* conservative, and slightly paranoid */
#endif mips
	lock(LOCK_RQ);
#ifdef vax
	Xu(pcb_szpt); Xu(pcb_p0lr); Xu(pcb_p1lr);
	Xup(pcb_p0br); Xup(pcb_p1br);

	/*
	 * The u. area is contained in the process' p1 region.
	 * Thus we map the current u. area into the process virtual space
	 * of both sets of page tables we will deal with so that it
	 * will stay with us as we rearrange memory management.
	 */
	for (i = 0; i < UPAGES; i++)
		if (up == &u)
			q->p_addr[i] = p->p_addr[i];
		else
			p->p_addr[i] = q->p_addr[i];
#ifdef vax
	mtpr(TBIA, 0);
#endif
	/*
	 * Now have u. double mapped, and have flushed
	 * any stale translations to new u. area.
	 * Switch the page tables.
	 */
	Xpp(p_p0br); Xp(p_szpt); Xpp(p_addr);
#ifdef vax
	mtpr(P0BR, u.u_pcb.pcb_p0br);
	mtpr(P1BR, u.u_pcb.pcb_p1br);
	mtpr(P0LR, u.u_pcb.pcb_p0lr &~ AST_CLR);
	mtpr(P1LR, u.u_pcb.pcb_p1lr);
#endif
	/*
	 * Now running on the ``other'' set of page tables.
	 * Flush translation to insure that we get correct u.
	 * Resurrect the u. for the other process in the other
	 * (our old) set of page tables.  Thus the other u. has moved
	 * from its old (our current) set of page tables to our old
	 * (its current) set of page tables, while we have kept our
	 * u. by mapping it into the other page table and then keeping
	 * the other page table.
	 */
#ifdef vax
	mtpr(TBIA, 0);
#endif
	for (i = 0; i < UPAGES; i++) {
		int pf;
		struct pte *pte;
		if (up == &u) {
			pf = umap[i].pg_pfnum;
			pte = &q->p_addr[i];
			pte->pg_pfnum = pf;
		} else {
			pf = umap[i].pg_pfnum;
			pte = &p->p_addr[i];
			pte->pg_pfnum = pf;
		}
	}
#ifdef vax
	mtpr(TBIA, 0);
#endif
#endif vax
#ifdef mips
        /* p_addr is not going anywhere because it does not point into
         * the the text or data or stack page maps. We do not swap tlbpids,
         * rather we let parent and child fault in separate mappings.
         */
        Xpp(p_textbr); Xpp(p_databr); Xpp(p_stakbr);
        Xp(p_textpt); Xp(p_datapt); Xp(p_stakpt);
#endif mips
	unlock(LOCK_RQ);
	splx(s);
}

/*
 * Compute number of pages to be allocated to the u. area
 * and data and stack area page tables, which are stored on the
 * disk immediately after the u. area.
 */
/*ARGSUSED*/
vusize(p, utl)
register struct proc *p;
struct user *utl;
{
#ifdef vax
	register int tsz = p->p_tsize / NPTEPG;

	/*
	 * We do not need page table space on the disk for page
	 * table pages wholly containing text.  This is well
	 * understood in the code in vmswap.c.
	 */
	return (clrnd(UPAGES +
	    clrnd(ctopt(p->p_tsize + p->p_dsize
					+ p->p_smsize	/* SHMEM */
					+ p->p_ssize+UPAGES)) - tsz));
#endif vax
#ifdef mips
XPRINTF(XPR_VM,"enter vusize",0,0,0,0);
        /*
         * We do not need page table space on the disk for page
         * table pages wholly containing text.  This is well
         * understood in the code in vmswap.c.
         */
        return (clrnd(UPAGES) + clrnd(ctopt(p->p_dsize))
                + clrnd(ctopt(p->p_ssize)));
#endif mips
}

/*
 * Get u area for process p.  If a old u area is given,
 * then copy the new area from the old, else
 * swap in as specified in the proc structure.
 *
 * Since argument map/newu is potentially shared
 * when an old u. is provided we have to be careful not
 * to block after beginning to use them in this case.
 * (This is not true when called from swapin() with no old u.)
 */
vgetu(p, palloc, map, newu, oldu)
	register struct proc *p;
	int (*palloc)();
	register struct pte *map;
	register struct user *newu;
	struct user *oldu;
{
	register int i;
	int ret, memall();

#ifdef mips
	XPRINTF(XPR_VM,"enter vgetu",0,0,0,0);
#endif mips

	if(palloc == memall)
		ret = (*palloc)(p->p_addr, clrnd(UPAGES), p, CSYS, NULL,V_NOOP);
	else
		ret = (*palloc)(p->p_addr, clrnd(UPAGES), p, CSYS);

	if(ret == 0)
		return (0);
	/*
	 * New u. pages are to be accessible in map/newu as well
	 * as in process p's virtual memory.
	 */
	for (i = 0; i < UPAGES; i++) {
		map[i] = p->p_addr[i];
#ifdef mips
                *(int *)(p->p_addr + i) |= PG_KW | PG_V | PG_G | PG_M;
#endif mips
#ifdef vax
		*(int *)(p->p_addr + i) |= PG_URKW | PG_V;
#endif vax
	}
	setredzone(p->p_addr, (caddr_t)0);
#ifdef mips
        vmaccess(map, (caddr_t)newu, UPAGES, DO_CACHE);
#endif mips
#ifdef vax
	vmaccess(map, (caddr_t)newu, UPAGES);
#endif vax
	/*
	 * New u.'s come from forking or inswap.
	 */
	if (oldu) {
#ifdef vax
		bcopy((caddr_t)oldu, (caddr_t)newu, UPAGES * NBPG);
#endif vax
#ifdef mips
		/*
 		 * Avoid copying the entire ublock by just doing what is
		 * known to be active. TODO: this hack knows that if an old
		 * u block is provided, it is indeed the active u and 
		 * therefore measuring the current stack depth is the right
		 * thing to do. Also, if stack depth measurement is to be
		 * done, pattern initialization should take place here.
		 */
		if (oldu != &u)
			panic("vgetu bad upage");
		bcopy((caddr_t)oldu, (caddr_t)newu, sizeof(struct user));
		i = stackdepth();
		bcopy(	(caddr_t)((int)oldu + UPAGES*NBPG -i), 
			(caddr_t)((int)newu + UPAGES*NBPG -i),
			i);
#endif mips
		newu->u_procp = p;

	} else {
		swap(p, p->p_swaddr, (caddr_t)0, ctob(UPAGES), 
			B_READ, B_UAREA, swapdev, 0);
		if (
#ifdef vax
		    newu->u_pcb.pcb_ssp != -1 || newu->u_pcb.pcb_esp != -1 ||
#endif
		    newu->u_tsize != p->p_tsize || newu->u_dsize != p->p_dsize ||
		    newu->u_ssize != p->p_ssize || newu->u_procp != p)
			panic("vgetu");
	}
	/*
	 * Initialize the pcb copies of the p0 and p1 region bases and
	 * software page table size from the information in the proc structure.
	 */
#ifdef vax
	newu->u_pcb.pcb_p0br = p->p_p0br;
	newu->u_pcb.pcb_p1br = initp1br(p->p_p0br + p->p_szpt * NPTEPG);
	newu->u_pcb.pcb_szpt = p->p_szpt;
#endif vax
	return (1);
}

/*
 * Release swap space for a u. area.
 */
vrelswu(p, utl)
	struct proc *p;
	struct user *utl;
{
#ifdef mips
	XPRINTF(XPR_VM,"enter vrelswu",0,0,0,0);
#endif mips

	rmfree(swapmap, (long)ctod(vusize(p, utl)), p->p_swaddr);
	/* p->p_swaddr = 0; */	/* leave for post-mortems */
}

/*
 * Get swap space for a u. area.
 */
vgetswu(p, utl)
	struct proc *p;
	struct user *utl;
{

#ifdef mips
	XPRINTF(XPR_VM,"enter vgetswu",0,0,0,0);
#endif mips
	p->p_swaddr = rmalloc(swapmap, (long)ctod(vusize(p, utl)));
	return (p->p_swaddr);
}

/*
 * Release u. area, swapping it out if desired.
 *
 * To fix the "stale u-area" problem, this routine will indicate to 
 * [v]memall that the u-area must be put in a temp "u" list until
 * after the context switch.  This will only happen if the u-area in
 * question is currently in context.
 */

vrelu(p, swapu)
	register struct proc *p;
{
	register int i;
	struct pte uu[UPAGES];

#ifdef mips
	XPRINTF(XPR_VM,"enter vrelu",0,0,0,0);
#endif mips
	if (swapu)
		swap(p, p->p_swaddr, (caddr_t)0, ctob(UPAGES),
		    B_WRITE, B_UAREA, swapdev, 0);
	for (i = 0; i < UPAGES; i++)
		uu[i] = p->p_addr[i];

#ifdef vax
	if (u.u_procp == p)
		(void) vmemfree(uu, -(clrnd(UPAGES)));
	else
		(void) vmemfree(uu, clrnd(UPAGES));
#endif vax
#ifdef mips
        /*
         * If freeing the user structure and kernel stack
         * for the current process, have to run a bit longer
         * using the pages which have already been freed...
         * block memory allocation from the network by raising ipl.
         */
#ifdef ultrix
	if (u.u_procp == p) {
		(void) splimp();	/* XXX */
		(void) vmemfree(uu, -(clrnd(UPAGES)));
	}
	else
		(void) vmemfree(uu, clrnd(UPAGES));
#else mips
        if (p == u.u_procp)
                (void) splimp();                /* XXX */
        (void) vmemfree(uu, clrnd(UPAGES));
#endif mips
#endif mips
}

#ifdef unneeded
int	ptforceswap;
#endif
#ifdef vax
/*
 * Expand a page table, assigning new kernel virtual
 * space and copying the page table entries over both
 * in the system map and as necessary in the user page table space.
 */
ptexpand(change, ods, oss, osms)
	register int change;
	size_t ods, oss;
	size_t osms;			/* SHMEM */
{
	register struct pte *p1, *p2;
	register int i;
	register int spages, ss = P1PAGES - u.u_pcb.pcb_p1lr;
	register int kold = btokmx(u.u_pcb.pcb_p0br);
	int knew, tdpages;
	int szpt = u.u_pcb.pcb_szpt;
	int s;

	if (change <= 0 || change % CLSIZE)
		panic("ptexpand");
	/*
	 * Change is the number of new page table pages needed.
	 * Kold is the old index in the kernelmap of the page tables.
	 * Allocate a new kernel map segment of size szpt+change for
	 * the page tables, and the new page table pages in the
	 * middle of this new region.
	 */
top:
#ifdef unneeded
	if (ptforceswap)
		goto bad;
#endif
	if ((knew=rmalloc(kernelmap, (long)(szpt+change))) == 0)
		goto bad;
	spages = ss/NPTEPG;
	tdpages = szpt - spages;
	if (memall(&Usrptmap[knew+tdpages], change, u.u_procp, CSYS, NULL, V_NOOP) == 0) {
		rmfree(kernelmap, (long)(szpt+change), (long)knew);
		goto bad;
	}

	/*
	 * Spages pages of u.+stack page tables go over unchanged.
	 * Tdpages of text+data page table may contain a few stack
	 * pages which need to go in one of the newly allocated pages;
	 * this is a rough cut.
	 */
	kmcopy(knew, kold, tdpages);
	kmcopy(knew+tdpages+change, kold+tdpages, spages);

	/*
	 * Validate and clear the newly allocated page table pages in the
	 * center of the new region of the kernelmap.
	 */
	i = knew + tdpages;
	p1 = &Usrptmap[i];
	p2 = p1 + change;
	while (p1 < p2) {
		/* tptov BELOW WORKS ONLY FOR VAX */
		mapin(p1, tptov(u.u_procp, i), p1->pg_pfnum, 1,
		    (int)(PG_V|PG_KW));
		clearseg(p1->pg_pfnum);
		p1++;
		i++;
	}
#ifdef vax
	mtpr(TBIA, 0);
#endif

	/*
	 * Move the stack and u. pte's which are before the newly
	 * allocated pages into the last of the newly allocated pages.
	 * They are taken from the end of the current p1 region,
	 * and moved to the end of the new p1 region.
	 */
	p1 = u.u_pcb.pcb_p1br + u.u_pcb.pcb_p1lr;
	p2 = initp1br(kmxtob(knew+szpt+change)) + u.u_pcb.pcb_p1lr;
	for (i = kmxtob(kold+szpt) - p1; i != 0; i--)
		*p2++ = *p1++;

	/*
	 * Now switch to the new page tables.
	 */
#ifdef vax
	mtpr(TBIA, 0);	/* paranoid */
#endif
	if(extracpu)
		tbsync();
	s = spl7();	/* conservative */
	lock(LOCK_RQ);
	u.u_procp->p_p0br = kmxtob(knew);
	setp0br(u.u_procp->p_p0br);
	u.u_pcb.pcb_p1br = initp1br(kmxtob(knew+szpt+change));
	setp1br(u.u_pcb.pcb_p1br);
	u.u_pcb.pcb_szpt += change;
	u.u_procp->p_szpt += change;
	u.u_procp->p_addr = uaddr(u.u_procp);
#ifdef vax
	mtpr(TBIA, 0);
#endif
	unlock(LOCK_RQ);
	splx(s);

	/*
	 * Finally, free old kernelmap.
	 */
	if (szpt)
		rmfree(kernelmap, (long)szpt, (long)kold);
	return;

bad:
	/*
	 * Swap out the process so that the unavailable 
	 * resource will be allocated upon swapin.
	 *
	 * When resume is executed for the process, 
	 * here is where it will resume.
	 */
	resume(pcbb(u.u_procp));
	if (savectx(&u.u_ssave)) {
		splx(s);
		return;
	}
	if (swapout(u.u_procp, ods, oss, osms) == 0) {
		/*
		 * No space to swap... it is inconvenient to try
		 * to exit, so just wait a bit and hope something
		 * turns up.  Could deadlock here.
		 *
		 * SOMEDAY REFLECT ERROR BACK THROUGH expand TO CALLERS
		 * (grow, sbreak) SO CAN'T DEADLOCK HERE.
		 */
		sleep((caddr_t)&lbolt, PRIBIO);
		goto top;
	}
	/*
	 * Set SSWAP bit, so that when process is swapped back in
	 * swapin will set u.u_pcb.pcb_sswap to u_sswap and force a
	 * return from the savectx() above.
	 */
	u.u_procp->p_flag |= SSWAP | SMASTER;
	(void) spl6();
	lock(LOCK_RQ);
	swtch();
	/* no return */
}
#endif vax

#ifdef mips
/*
 * Expand a page table, assigning new kernel virtual
 * space and copying the page table entries over both
 * in the system map and as necessary in the user page table space.
 */
ptexpand(change, ods, oss, region)
	register int change;
	size_t ods, oss;
	int region;
{
	register struct pte *p1, *p2;
	register int i;
	register int kold;
	int knew;
	int szpt;
	int s;

XPRINTF(XPR_VM,"enter ptexpand",0,0,0,0);
	if (change <= 0 || change % CLSIZE)
		panic("ptexpand");
	/*
	 * Change is the number of new page table pages needed.
	 * Kold is the old index in the kernelmap of the page tables.
	 * Allocate a new kernel map segment of size szpt+change for
	 * the page tables.
	 */
top:
#ifdef unneeded
	if (ptforceswap)
		goto bad;
#endif unneeded
	if (region == 0) {
		szpt = u.u_procp->p_datapt;
		kold = btokmx(u.u_procp->p_databr);
	} else {
		szpt = u.u_procp->p_stakpt;
		kold = btokmx(u.u_procp->p_stakbr);
	}

	if ((knew = rmalloc(kernelmap, (long)(szpt+change))) == 0)
		goto bad;
	if (region == 0) {
	    if (memall(&Usrptmap[knew+szpt], change, u.u_procp, CSYS, NULL, V_NOOP) == 0) {
		rmfree(kernelmap, (long)(szpt+change), (long)knew);
		goto bad;
	    }
	} else {
	    if (memall(&Usrptmap[knew], change, u.u_procp, CSYS, NULL, V_NOOP) == 0) {
		rmfree(kernelmap, (long)(szpt+change), (long)knew);
		goto bad;
	    }
	}

	/*
	 * Copy over stack and data page tables.
	 */
	if (region == 0)
		kmcopy(knew, kold, szpt);
	else
		kmcopy(knew+change, kold, szpt);

	/*
	 * Validate and clear the newly allocated page table pages in
	 * the new region of the kernelmap.
	 */
	if (region == 0)
		i = knew + szpt;
	else
		i = knew;
	p1 = &Usrptmap[i];
	p2 = p1 + change;
	while (p1 < p2) {
		mapin(btop(kmxtob(i)), p1->pg_pfnum, (int)(PG_V|PG_KW));
#ifdef USE_IDLE
		/* don't bother if it's already done */
		if (cmap[pgtocm(p1->pg_pfnum)].c_zero) {
			extern int v_zero_pt_hits;

			v_zero_pt_hits++;
			cmap[pgtocm(p1->pg_pfnum)].c_zero=0;
		}
		else {
			extern int v_zero_pt_misses;

			v_zero_pt_misses++;
			clearseg(p1->pg_pfnum);
		}
#else
		clearseg(p1->pg_pfnum);
#endif USE_IDLE
		p1++;
		i++;
	}

	/*
	 * Now switch to the new page tables.
	 */
	s = splhigh();	/* conservative */
	if (region == 0) {
		u.u_procp->p_databr = kmxtob(knew);
		u.u_procp->p_datapt += change;
	} else {
		u.u_procp->p_stakpt += change;
		u.u_procp->p_stakbr = kmxtob(knew);
	}
	splx(s);

	/*
	 * Finally, free old kernelmap.
	 */
	if (szpt)
		rmfree(kernelmap, (long)szpt, (long)kold);
	return;

bad:
	/*
	 * Swap out the process so that the unavailable 
	 * resource will be allocated upon swapin.
	 *
	 * When resume is executed for the process, 
	 * here is where it will resume.
	 */
	save();
	if (setjmp(&u.u_ssave))
		return;
	if (swapout(u.u_procp, ods, oss, u.u_osmsize) == 0) {
		/*
		 * No space to swap... it is inconvenient to try
		 * to exit, so just wait a bit and hope something
		 * turns up.  Could deadlock here.
		 *
		 * SOMEDAY REFLECT ERROR BACK THROUGH expand TO CALLERS
		 * (grow, sbreak) SO CAN'T DEADLOCK HERE.
		 */
		sleep((caddr_t)&lbolt, PRIBIO);
		goto top;
	}
	/*
	 * Set SSWAP bit, so that when process is swapped back in
	 * swapin will set u.u_pcb.pcb_sswap to u_sswap and force a
	 * return from the setjmp() above.
	 */
	u.u_procp->p_flag |= SSWAP;
	swtch();
	/* no return */
}
#endif mips

kmcopy(to, from, count)
	register int to;
	int from;
	register int count;
{
	register struct pte *tp = &Usrptmap[to];
	register struct pte *fp = &Usrptmap[from];

#ifdef mips
	XPRINTF(XPR_VM,"enter kmcopy",0,0,0,0);
#endif mips
	while (count != 0) {
#ifdef vax
		mapin(tp, tptov(u.u_procp, to), fp->pg_pfnum, 1,
		    (int)(*((int *)fp) & (PG_V|PG_PROT)));
#endif vax
#ifdef mips
                mapin(btop(kmxtob(to)), fp->pg_pfnum,
                    (int)(*((int *)fp) & (PG_V|PG_PROT)));
#endif mips
		tp++;
		fp++;
		to++;
		count--;
	}
}
