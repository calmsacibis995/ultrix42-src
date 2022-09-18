#ifndef lint
static char *sccsid = "@(#)vm_subr.c	4.3	(ULTRIX)	9/6/90";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1986, 1988 by			*
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
 * 4-Sep-90 dlh
 *	added include of vectors.h
 *
 * 13-Jul-90 -- jmartin
 *	Apply "clrnd" to sm_size as well as "btoc".
 *
 * 05-Jun-90 -- jaw
 *	On mips, clear out global bit on Forkmap in uaccess.  This is to 
 * 	fix uninitialized u-area problem.
 *
 * 12-Dec-89 -- jas
 *      moved "endif mips" to make uvtophy() a non-mips-specific routine.
 *
 * 11-Dec-89 -- jas
 *      added uvtophy() routine to convert mips kuseg address to physical
 *      address.
 *
 * 11 Dec 89 jaa
 *	moved kmcopy() here
 *
 * 21-Jul-89 -- jaa
 *	moved chksiz() from machine/{mips,vax}/vm_machdep.c to here
 *	and fixed overflow problem
 *
 * 02-May-89 -- jaw, jmartin
 *	fix forkutl to work on mips.
 *
 * 24 Mar 89 -- Kong
 *	Added a mips only routine that converts system virtual address
 *	into physical address.  This is needed by the VAX device drivers
 *	that are now ported to MIPS.
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

#ifdef vax
#include "../machine/mtpr.h"
#include "../machine/cpu.h"
#include "../../machine/common/cpuconf.h"
#include "../machine/vectors.h"
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

	XPRINTF(XPR_VM,"enter uaccess",0,0,0,0);
#ifdef mips
	flush_tlb();
#endif mips
	for (i = 0; i < UPAGES; i++) {
#ifdef vax
		*(int *)mp = 0;
		mp->pg_pfnum = p->p_addr[i].pg_pfnum;
		mp++;
#endif vax
#ifdef mips
                *(int *)(p->p_addr + i) |= PG_KW | PG_V | PG_G | PG_M | DO_CACHE;
		map[i] = p->p_addr[i];
		map[i].pg_g = 0;

#endif mips


	}
#ifdef mips
/*        vmaccess(map, (caddr_t)uarea, UPAGES, DO_CACHE); */
#endif mips
#ifdef vax
	vmaccess(map, (caddr_t)uarea, UPAGES, DO_CACHE);
#endif vax
}

#ifdef vax
/*
 * Validate the kernel map for size ptes which
 * start at ppte in the sysmap, and which map
 * kernel virtual addresses starting with vaddr.
 */
vmaccess(ppte0, vaddr, size0, dummy)
	struct pte *ppte0;
	register caddr_t vaddr;
	int size0;
	int dummy;        /* noop on vax, used for consistency with mips */
{
	register struct pte *ppte = ppte0;
	register int size = size0;

	XPRINTF(XPR_VM,"enter vmaccess",0,0,0,0);
	while (size != 0) {
		mapin(ppte, btop(vaddr), (unsigned)(*(int *)ppte & PG_PFNUM), 1,
			(int)(PG_V|PG_KW));
		ppte++;
		vaddr += NBPG;
		--size;
	}
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
	int cache_cntrl;	/* DO_CACHE or NO_CACHE ../machine/pte.h */
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
 * Convert a virtual page number to a pte address.  This function is
 * called by
 * 	1) a process filling its own pages,
 * 	2) the pagedaemon
 * 	3) the swapper
 * 	4) interrupt service routines.
 * Except for the first case, access to process page tables must be
 * synchronized with other processors and with the process itself, in
 * the case of an ISR.  The page tables may be "on loan" to a vfork()ed
 * child and thus not reachable via the proc structure.  The process may
 * be expanding (one of) its page tables, in which case the system
 * virtual address of the page will not be in a consistent state until
 * the operation is completed.
 *
 * lk_p_vm is used in conjunction with the linked list of vforked()
 * processes to locate the process which has page tables.
 */
struct pte *
vtopte(p, v)
	register struct proc *p;
	register unsigned v;
{
	register struct pte *pte;
	int smindex;
	int s;

	s = splimp();
	smp_lock(&lk_p_vm, LK_RETRY);
	/*
	 * The vfork chain must always be followed, even when
	 * (p == u.u_procp).  This is because the process may be
	 * interrupted at the end of vpassvm(), after it has given up
	 * its page tables and before it sleeps.
	 */
	while (p->p_vm & SNOVM)
		p = p->p_xlink;
	/* TBD:  cope with a ptexpand happening in "p" on another processor. */

	if (isatsv(p, v))
		pte = tptopte(p, vtotp(p, v));
#ifdef vax
	else if (isadsv(p, v))
		if((vtodp(p, v) >= p->p_dsize) && p->p_dsize){/* begin SHMEM */
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
				if(xp >= p->p_sm[i].sm_saddr &&
					xp < p->p_sm[i].sm_saddr +
					clrnd(btoc(p->p_sm[i].sm_p->sm_size)))
					break;
			}
			if(i >= sminfo.smseg)
				panic("vtopte SMEM");
			pte = p->p_sm[i].sm_p->sm_ptaddr +
				(xp - p->p_sm[i].sm_saddr);
		}	/* end SHMEM */
		else
			pte = dptopte(p, vtodp(p, v));
	else 
		pte = sptopte(p, vtosp(p, v));
#endif vax
#ifdef mips
	else if (isadsv(p, v))
		pte = dptopte(p, vtodp(p, v));
	else if (isassv(p, v))
		pte = sptopte(p, vtosp(p, v));
	else if (isasmsv(p, v, &smindex)) {
		register struct p_sm *psm = &p->p_sm[smindex];
		pte = psm->sm_p->sm_ptaddr + vtosmp(psm, v);
	} else if (isafsv(p, v))
		pte = fptopte(p, vtofp(p, v));
	else 
		pte = (struct pte *)0;
#endif mips
	smp_unlock(&lk_p_vm);
	(void)splx(s);
	return pte;
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

	XPRINTF(XPR_VM,"enter vinifod",0,0,0,0);
	nclpbsize = gp->g_mp->m_bsize / CLBYTES;
	while (bfirst < blast) {
		i = bfirst % nclpbsize;
		bn = GBMAP(gp,bfirst/nclpbsize,B_READ,0,0);
		for ( ; i < nclpbsize; i++) {
			pte->pg_fod = 1;
			pte->pg_fileno = fileno;
			if (u.u_error || bn < 0) {
                                pte->pg_blkno = 0;
				pte->pg_fileno = PG_FZERO;
				cnt.v_nzfod += CLSIZE;
			} else {
                                pte->pg_blkno = bn + btodb(i * CLBYTES);
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

/*
 * Check for valid program size
 * NB - Check data and data growth separately as they may overflow 
 * when summed together.
 */
chksize(ts, ids, uds, ss)
	unsigned ts, ids, uds, ss;
{
	extern unsigned maxprocptes; /* param.c */
	extern int maxtsiz; 

	if (ts > maxtsiz ||
	    ctob(ids) > u.u_rlimit[RLIMIT_DATA].rlim_cur ||
	    ctob(uds) > u.u_rlimit[RLIMIT_DATA].rlim_cur ||
	    ctob(ids + uds) > u.u_rlimit[RLIMIT_DATA].rlim_cur ||
	    ctob(ss) > u.u_rlimit[RLIMIT_STACK].rlim_cur) {
		u.u_error = ENOMEM;
		return (1);
	}
#ifdef vax
	/*
	 * Make sure the process isn't bigger than our virtual memory limit.
	 * The default value for maxprocptes is the size of the
	 * address space but may be set lower via param.c
	 */
	if(LOWPAGES + ts +ids+uds+u.u_smsize + ss + HIGHPAGES > maxprocptes) {
		u.u_error = ENOMEM;
		return (1);
	}
#endif vax
	return (0);
}

kmcopy(to, from, count)
	register int to;
	int from;
	register int count;
{
	register struct pte *tp = &Usrptmap[to];
	register struct pte *fp = &Usrptmap[from];
	while (count != 0) {
#ifndef mips
		mapin(tp, btop(kmxtob(to)), fp->pg_pfnum, 1,
		      (int)(*((int *)fp) & (PG_V|PG_PROT)));
#else mips
		mapin(    btop(kmxtob(to)), fp->pg_pfnum,
		      (int)(*((int *)fp) & (PG_V|PG_PROT)));
#endif mips
		tp++;
		fp++;
		to++;
		count--;
	}
}


#ifdef mips
/*
 * convert system virtual address into physical address.
 * prints out warning message if given a user space address.
 *
 * Returns:
 *	physical address if the given virtual address is a valid
 *	system address.
 *	0 if the given virtual address is not a system address.
 *
 * Error conditions:
 *	If the given virtual address is in Kseg 2 space, and the PTE
 *	mapping that page is invalid, the returned physical address
 *	may be wrong.
 */
unsigned vm_svtophy(virt)
unsigned virt;			/* System virtual address */
{
	struct pte apte;
	unsigned phys;
	if (IS_KSEG0(virt)) {
		phys = K0_TO_PHYS(virt);
	} else if (IS_KSEG1(virt)) {
		phys = K1_TO_PHYS(virt);
	} else if (IS_KSEG2(virt)) {
		apte = Sysmap[btop(virt - K2BASE)];
		phys = ((apte.pg_pfnum) << PGSHIFT) + (virt & PGOFSET);
	} else {
		cprintf("WARNING: vm_svtophy with user addr");
		phys = 0;
	}

	return(phys);
}

#endif mips

/*
 * uvtophy(proc,addr) -
 *
 * Converts kuseg address to physical address.  The page "MUST" be resident
 * in main memory prior to the execution of this routine.
 */

uvtophy(p,addr)
struct proc *p;
int addr;

{
  struct pte *pte;

  pte = vtopte(p, btop(addr));     /* get pte for vaddr */

  /* 
   * Get PFN, and stuff index to get phyaddr.
   */

  return((pte->pg_pfnum << PGSHIFT) + (addr & PGOFSET));
                                   
}
