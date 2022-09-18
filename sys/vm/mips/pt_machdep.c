#ifndef lint
static	char	*sccsid = "@(#)pt_machdep.c	4.1	(ULTRIX)	7/2/90";
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
 * 19-Jun-90 -- jmartin
 *	Fixes for "panic: vrelvm rssize"
 *
 * 05 Mar 90 jaw
 *	IPL level lowered when text lock is released in vinitpt.
 *
 * 06 Feb 90 gmm
 *	Get lk_text if vinitpt() returns with value 0. The calling routine
 *	expects lk_text to be held on return.
 *
 * 11 Dec 89 jaa
 *	fixed mips to round from clicks to pt's in vinitpt
 *
 * 24 Jul 89 -- jmartin
 *	Don't call swapout; just ask the swapper and sleep.
 *
 * 06-Jun-89  -- jaa
 *    Creation date
 *    split machdep routines from ../vm_pt.c 
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

#include "../machine/pte.h"

#include "../machine/cpu.h"
#include "../h/ipc.h"
#include "../h/shm.h"
#include "../h/cpudata.h"

extern struct sminfo sminfo;
extern int swapfrag;

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
	register int i, s, takelock;
	extern int memfree();
#ifdef CACHETRICKS
	register struct cmap *c;
	register unsigned pf;
#endif CACHETRICKS

XPRINTF(XPR_VM,"enter vgetpt",0,0,0,0);

	/*
	 * Must take MP lock for memfree during error path frees.
	 */

	takelock = (pmemfree == memfree);

	/*
	 * Allocate space in the kernel map for this process.
	 * Then allocate page table pages, and initialize the
	 * process' base registers and addr pointers to be the kernel
	 * virtual addresses of the base of the page tables and
	 * the pte for the process pcb (at the base of the u.).
	 */
	if (p->p_datapt) {
		if ((ad = rmalloc(kernelmap, (long)p->p_datapt)) == 0)
			return (0);
		if ((*pmemall)(&Usrptmap[ad], p->p_datapt, p, 
				     CSYS, NULL, V_NOOP) == 0) {
			rmfree(kernelmap, (long)p->p_datapt, ad);
			return (0);
		}
	}

	if (p->p_stakpt) {
		if ((as = rmalloc(kernelmap, (long)p->p_stakpt)) == 0) {
			if (p->p_datapt) {
				if (takelock) {
					s = splimp();
					smp_lock(&lk_cmap, LK_RETRY);
				}
				(*pmemfree)(&Usrptmap[ad], p->p_datapt);
				if (takelock) {
					smp_unlock(&lk_cmap);
					(void) splx(s);
				}
				rmfree(kernelmap, (long)p->p_datapt, ad);
			}
			return (0);
		}
		if ((*pmemall)(&Usrptmap[as], p->p_stakpt, p, 
				     CSYS, NULL, V_NOOP) == 0) {
			if (p->p_datapt) {
				if (takelock) {
					s = splimp();
					smp_lock(&lk_cmap, LK_RETRY);
				}
				(*pmemfree)(&Usrptmap[ad], p->p_datapt);
				if (takelock) {
					smp_unlock(&lk_cmap);
					(void) splx(s);
				}
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
		vmaccess(&Usrptmap[ad], (caddr_t)p->p_databr, p->p_datapt, DO_CACHE);
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
		vmaccess(&Usrptmap[as], (caddr_t)p->p_stakbr, p->p_stakpt, DO_CACHE);
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

/*
 * Initialize text portion of page table.
 */
vinitpt(p,s)
	struct proc *p;
	int s;
{
	register struct text *xp;
	register struct proc *q;
	register struct pte *pte;
	register int i;
	struct pte proto;
	unsigned unlock_flag = NULL;
        register long at = 0;

	XPRINTF(XPR_VM,"enter vinitpt",0,0,0,0);
	xp = p->p_textp;
	if (xp == 0)
                return (1);
	pte = tptopte(p, 0);
	if (q = xp->x_caddr) {
	/*
	 * If there is another instance of same text in core
	 * then just copy page tables from other process.
	 *
	 * This operation can be thought of as a large "distpte", so
	 * we lock it under the same protocol as we lock "distpte".
	 * We don't give up the lock until the process is linked to
	 * the text chain (see xlink(p) in sys/vm_text.c), so that
	 * real "distpte"s will see this process.
	 */
                p->p_textbr = q->p_textbr;
                p->p_textpt = q->p_textpt;
		goto done;
	}
	/*
	 * OK to give up the spin lock, as we are the one and only
	 * process using this text and have excluded others by
	 * maintaining ((xp->x_flag & X_LOCK) == 1).
	 */
	unlock_flag = !NULL;
	smp_unlock(&lk_text);
	splx(s);
	/*
	 * allocate text page table if it cannot be shared
	 */
	p->p_textpt = clrnd(ctopt(xp->x_size));
	at = rmalloc(kernelmap, (long)p->p_textpt);
	if (at == 0){
		(void)splimp();
		smp_lock(&lk_text, LK_RETRY);
		return (0);
	}
	if (vmemall(&Usrptmap[at], p->p_textpt, p, CSYS, NULL, V_NOOP) == 0) {
		rmfree(kernelmap, (long)p->p_textpt, at);
		(void)splimp();
		smp_lock(&lk_text, LK_RETRY);
		return (0);
	}
	p->p_textbr = kmxtob(at);
	vmaccess(&Usrptmap[at], (caddr_t)p->p_textbr, p->p_textpt, DO_CACHE);
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

	/*
	 * Initialize text page tables, zfod if we are loading
	 * the text now; unless the process is demand loaded,
	 * this will suffice as the text will henceforth either be
	 * read from a file or demand paged in.
	 */
	*(int *)&proto = PG_URKR;
	if (xp->x_flag & XLOAD || xp->x_flag & XNOSPCE) {
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
	if (xp->x_flag & XLOAD || xp->x_flag & XNOSPCE){
		vinifod((struct fpte *)tptopte(p, 0), PG_FTEXT, xp->x_gptr,
		    (daddr_t)0, xp->x_size);

		xp->x_flag &= ~XNOSPCE; /* clear the XNOSPCE flag setin xccdec*/
	} else {
		register int *dp, poff, ptsize,nfrag;
		dp = xp->x_dmap->dm_ptdaddr;
		nfrag = dtob(swapfrag);
		poff = 0;
		ptsize = xp->x_size * sizeof (struct pte);
		while(ptsize > nfrag){
			if(*dp == 0)
				panic("vinitpt: text pt swap addr 0");
			swap(p, *dp++, (caddr_t)tptopte(p, poff),
			nfrag, B_READ, B_PAGET, swapdev,0);

			ptsize -= nfrag;
			poff += nfrag/sizeof(struct pte);
		}
		if(*dp == 0)
			panic("vinitpt: text pt swap addr 0");
		swap(p, *dp, (caddr_t)tptopte(p, poff),
		    ptsize, B_READ, B_PAGET, swapdev, 0);
	}
done:
	/*
	 * In the case where we are overlaying ourself with new page
	 * table entries, old user-space translations should be flushed.
	 */
        /*
         * I don't think this really has to be done. mad
         */
        newptes(p, tptov(p, 0), xp->x_size);
	if (unlock_flag) {
		(void) splimp();
		smp_lock(&lk_text, LK_RETRY);
	}
	return(1);
}

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

#define	Xu(a)  {register int t; \
		t = up->u_pcb.a; up->u_pcb.a = uq->u_pcb.a; uq->u_pcb.a = t;}
#define	Xup(a) {register struct pte *tp; \
	        tp = up->u_pcb.a; up->u_pcb.a = uq->u_pcb.a; uq->u_pcb.a = tp;}
#define	Xp(a)  {register int t; t = p->a; p->a = q->a; q->a = t;}
#define	Xpp(a) {register struct pte *tp; tp = p->a; p->a = q->a; q->a = tp;}

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
vpasspt(p, q, up, uq, umap)
	register struct proc *p, *q;
	register struct user *up, *uq;
	struct pte *umap;
{
	int s;
	XPRINTF(XPR_VM,"enter vpasspt",0,0,0,0);
/*
 * This mips version is symetric since the u-area is not mapped in the
 * page tables that are being swapped. Note that the umap argument is
 * not used for the mips version.
 * TODO: should the processes exchange tlbpids? They are exchanging mappings
 * except for the u-block which probably has the G bit set.
 */

	s = splhigh();	/* conservative, and slightly paranoid */;
	smp_lock(&lk_rq, LK_RETRY);
        /* p_addr is not going anywhere because it does not point into
         * the the text or data or stack page maps. We do not swap tlbpids,
         * rather we let parent and child fault in separate mappings.
         */
        Xpp(p_textbr); Xpp(p_databr); 
        Xp(p_textpt); Xp(p_datapt); 

	q->p_stakbr = p->p_stakbr;
	q->p_stakpt = p->p_stakpt;
	if (up != &u) {	/* from child to parent */
		p->p_stakbr=0;
		p->p_stakpt=0;
	}

	smp_unlock(&lk_rq);
	splx(s);
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

	XPRINTF(XPR_VM,"enter vgetu",0,0,0,0);

	if((*palloc)(p->p_addr, clrnd(UPAGES), p, CSYS, NULL,V_NOOP) == 0)
		return (0);
	/*
	 * New u. pages are to be accessible in map/newu as well
	 * as in process p's virtual memory.
	 */
	for (i = 0; i < UPAGES; i++) {
                *(int *)(p->p_addr + i) |= PG_KW | PG_V | PG_G | PG_M;
		*(int *)(map+i) = *(int *)(p->p_addr+i)
			& PG_PFNUM | PG_V | PG_M | PG_KW;
	}
	setredzone(p->p_addr, (caddr_t)0);
   /*     vmaccess(map, (caddr_t)newu, UPAGES, DO_CACHE); */
	newptes(u.u_procp, btop(newu), UPAGES);
	/*
	 * New u.'s come from forking or inswap.
	 */
	if (oldu) {
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
		newu->u_procp = p;
	} else {
		swap(p, *(p->p_smap->dm_ptdaddr), (caddr_t)0, ctob(UPAGES),
			B_READ, B_UAREA, swapdev, 0);
		if (
		    newu->u_tsize != p->p_tsize || newu->u_dsize != p->p_dsize ||
		    newu->u_ssize != p->p_ssize || newu->u_procp != p)
			panic("vgetu");
	}
	/*
	 * Initialize the pcb copies of the p0 and p1 region bases and
	 * software page table size from the information in the proc structure.
	 */
	return (1);
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

	XPRINTF(XPR_VM,"enter vrelu",0,0,0,0);
	if (swapu)
		swap(p, *(p->p_smap->dm_ptdaddr), (caddr_t)0, ctob(UPAGES),
		    B_WRITE, B_UAREA, swapdev, 0);
	for (i = 0; i < UPAGES; i++)
		uu[i] = p->p_addr[i];

        /*
         * If freeing the user structure and kernel stack
         * for the current process, have to run a bit longer
         * using the pages which have already been freed...
         * block memory allocation from the network by raising ipl.
         */
	if (u.u_procp == p) {
		(void) splimp();	/* XXX */
		(void) vmemfree(uu, -(clrnd(UPAGES)));
	}
	else
		(void) vmemfree(uu, clrnd(UPAGES));
}

/*
 * Expand a page table, assigning new kernel virtual
 * space and copying the page table entries over both
 * in the system map and as necessary in the user page table space.
 */
ptexpand(change, region)
	register int change;
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
	 */
	SET_P_VM(u.u_procp, SSWAP);
	do {
		swapself++;
		sleep((caddr_t)&swapself, PSWP);
	} while (u.u_procp->p_vm & SSWAP);
}

blkcpy(src, des, count)
caddr_t src;
caddr_t des;
int count;
{
	bcopy(src, des, count);
}
