#ifndef lint
static char *sccsid = "@(#)vm_machdep.c	4.2  (ULTRIX)        9/6/90";
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

/*-----------------------------------------------------------------------
 *
 * Modification History
 *
 * 4 Sep 90 -- dlh
 *	added vector processor support code
 *
 * 21 Jul 89 -- jaa
 *	moved chksiz() to vm/vm_subr.c
 *
 * 12 Jun 89 -- gg
 * 	chksize() modified to check for swap map overflow -- dynamic
 *	swap change
 *
 * 5 May 89 -- Adrian Thoms
 *	chksize() modified to check against maxprocptes for VVAX
 *
 * 10 Feb 89 -- jmartin
 *	PMAX/v3.0 merge:  make unused argument of newptes a "struct proc *"
 *	instead of "struct pte *".
 *
 * 25 Jul 88 -- jmartin
 *	Protect PTE changes with lk_cmap.
 *
 * 25 Jan 88 -- jmartin
 *	As with mapin, newptes is now a macro defined in ./vmparam.h
 *
 * 11 Sep 86 -- koehler
 *	Moved a few things into registers
 *
 * 02-Apr-86 -- jrs
 *	Remove tbsync() call as this code is mp safe
 *
 * 12-Feb-86 -- jrs
 *	Added call to tbsync() to control mp translation buffer
 *
 *	Derived from 4.2 BSD labelled:
 *		vm_machdep.c	6.1	83/07/29
 *
 *-----------------------------------------------------------------------
 */

#include "../machine/pte.h"
#include "../machine/vmparam.h"

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/proc.h"
#include "../h/cmap.h"
#include "../h/mount.h"
#include "../h/vm.h"
#include "../h/text.h"

#include "../machine/mtpr.h"
#ifdef vax
#include "../machine/cpu.h"
#include "../../machine/common/cpuconf.h"
#include "../machine/vectors.h"
#endif vax

extern int swapfrag;
extern int dmap_elems[];

/*
 * Set a red zone in the kernel stack after the u. area.
 */
setredzone(pte, vaddr)
	register struct pte *pte;
	caddr_t vaddr;
{

	pte += (sizeof (struct user) + NBPG - 1) / NBPG;
	*(int *)pte &= ~PG_PROT;
	*(int *)pte |= PG_URKR;
	if (vaddr)
		mtpr(TBIS, vaddr + sizeof (struct user));
}

#ifndef mapin
mapin(pte, v, pfnum, count, prot)
	struct pte *pte;
	u_int v, pfnum;
	int count, prot;
{
	/* Quiesce vector processor if necessary... */
	VPSYNC();

	while (count > 0) {
		*(int *)pte++ = pfnum | prot;
		mtpr(TBIS, ptob(v));
		v++;
		pfnum++;
		count--;
	}
	tbsync();
}
#endif

#ifdef notdef
/*ARGSUSED*/
mapout(pte, size)
	register struct pte *pte;
	int size;
{

	panic("mapout");
}
#endif

#ifndef newptes
/*ARGSUSED*/
newptes(p, v, size)
	register struct proc *p;
	u_int v;
	register int size;
{
	register caddr_t a = ptob(v);

#ifdef lint
	p = p;
#endif
	/* Quiesce vector processor if necessary... */
	VPSYNC();

	if (size >= 8) {
		mtpr(TBIA, 0);
	} else {
		while (size > 0) {
			mtpr(TBIS, a);
			a += NBPG;
			size--;
		}
	}
}
#endif newptes

/*
 * Change protection codes of text segment.
 * Have to flush translation buffer since this
 * affects virtual memory mapping of current process.
 */
chgprot(addr, tprot)
	caddr_t addr;
	long tprot;
{
	register unsigned v;
	register int tp;
	register struct pte *pte;
	register struct cmap *c;
	register struct proc *p = u.u_procp;
	int s;

	v = clbase(btop(addr));
	if (!isatsv(p, v)) {
		u.u_error = EFAULT;
		return (0);
	}
	tp = vtotp(p, v);
	pte = tptopte(p, tp);
	s = splimp();
	smp_lock(&lk_cmap, LK_RETRY);
	if (pte->pg_fod == 0 && pte->pg_pfnum) {
		c = &cmap[pgtocm(pte->pg_pfnum)];
		if (c->c_blkno && c->c_mdev != MSWAPX)
			munhash(mount[c->c_mdev].m_dev,
			    (daddr_t)(u_long)c->c_blkno,p->p_textp->x_gptr);
	}
	*(int *)pte &= ~PG_PROT;
	*(int *)pte |= tprot;
	distcl(pte);
	tbiscl(v);
	smp_unlock(&lk_cmap);
	(void) splx(s);
	return (1);
}

settprot(tprot)
	long tprot;
{
	register int *ptaddr, i;

	ptaddr = (int *)mfpr(P0BR);
	for (i = 0; i < u.u_tsize; i++) {
		ptaddr[i] &= ~PG_PROT;
		ptaddr[i] |= tprot;
	}
	newptes(u.u_procp, 0, u.u_tsize);
}

/*
 * Rest are machine-dependent
 */

getmemc(addr)
	caddr_t addr;
{
	register int c;
	struct pte savemap;

	savemap = mmap[0];
	*(int *)mmap = PG_V | PG_KR | btop(addr);
	mtpr(TBIS, vmmap);
	c = *(char *)&vmmap[(int)addr & PGOFSET];
	mmap[0] = savemap;
	mtpr(TBIS, vmmap);
	return (c & 0377);
}

putmemc(addr, val)
	caddr_t addr;
{
	struct pte savemap;

	savemap = mmap[0];
	*(int *)mmap = PG_V | PG_KW | btop(addr);
	mtpr(TBIS, vmmap);
	*(char *)&vmmap[(int)addr & PGOFSET] = val;
	mmap[0] = savemap;
	mtpr(TBIS, vmmap);
}

/*
 * Move pages from one kernel virtual address to another.
 * Both addresses are assumed to reside in the Sysmap,
 * and size must be a multiple of CLSIZE.
 */
pagemove(from, to, size)
	register caddr_t from, to;
	register int size;
{
	register struct pte *fpte, *tpte;

	if (size % CLBYTES)
		panic("pagemove");
	fpte = &Sysmap[btop(from - 0x80000000)];
	tpte = &Sysmap[btop(to - 0x80000000)];
	while (size > 0) {
		*tpte++ = *fpte;
		*(int *)fpte++ = 0;
		mtpr(TBIS, from);
		mtpr(TBIS, to);
		from += NBPG;
		to += NBPG;
		size -= NBPG;
	}
	tbsync();
}
