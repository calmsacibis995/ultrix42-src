#ifndef lint
static	char	*sccsid = "@(#)vm_machdep.c	4.1	(ULTRIX)	7/2/90";
#endif lint
/*
 * Copyright (c) 1982, 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)vm_machdep.c	7.1 (Berkeley) 6/5/86
 */

/* Revision History
 *
 * 06-Mar-90 -- gmm
 *	Call tbsync() in mapin() to fix MP bugs related to stale tlb entries
 *
 * 09-Nov-89 -- bp
 *	added routine kseg0_alloc to allocate contiguous physical
 *	kseg0 space.
 *
 * 19-Oct-89 -- jmartin
 *	For settprot/chgprot, operate on the whole PTE word (and not
 *	a field) because this is how the argument is passed in.
 *
 * 13-Oct-89 -- gmm
 *	smp changes. Changes in newpte() since tlbpids are now allocated on 
 *	a per processor basis.
 */

#include "../machine/pte.h"
#include "../machine/cpu.h"

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/proc.h"
#include "../h/cmap.h"
#include "../h/mount.h"
#include "../net/netinet/in.h"
#include "../fs/nfs/nfs_clnt.h"
#include "../fs/nfs/vfs.h"
#include "../h/vm.h"
#include "../h/text.h"
#include "../h/cpudata.h"

#ifdef PROBE_BUG
#define CKPROBE(x) {register r; if(r=(x))ckprobe(r);}
#endif

/*
 * Set a red zone in the kernel stack after the u. area.
 */
setredzone(pte, vaddr)
	register struct pte *pte;
	caddr_t vaddr;
{
#ifdef notdef
	/*
	 * SHOULD SET A STACK MARKER THAT IS CHECKED AT EXIT AT LEAST
	 */
	pte += (sizeof (struct user) + NBPG - 1) / NBPG;
	Hard(pte) &= ~PG_PROT;
	Hard(pte) |= PG_URKR;
	if (vaddr)
		mtpr(TBIS, vaddr + sizeof (struct user));
#endif
}

/*
 * NOTE: this definition is different from the standard 4.3 def
 */
mapin(vpn, pfnum, prot)
u_int vpn, pfnum;
int prot;
{
	struct pte *pte;
	extern unsigned Syssize;

	if (vpn < btop(K2BASE) || vpn >= btop(K2BASE)+Syssize)
		panic("mapin");

	pte = &Sysmap[vpn - btop(K2BASE)];
	pfnum <<= PTE_PFNSHIFT;
	Hard(pte) = pfnum | prot | PG_M | PG_G;

#ifdef PROBE_BUG
	CKPROBE(tlbdropin(0, ptob(vpn), Hard(pte)));
#else
	tlbdropin(0, ptob(vpn), Hard(pte));
	tbsync();
#endif
}

#ifdef notdef
/*ARGSUSED*/
mapout(pte, size)
	register struct pte *pte;
	int size;
{

	panic("mapout");
}
#endif


/*
 * Flushpte() will flush the tlb for all processes currently using a given 
 * segment.  For data and stack, this will simply passthrough to newptes.
 * For text, all sharing processes will have the shared address range flushed.
 * For shared memory, all sharing processes will have the relative address
 * range flushed.
 */
flushpte(p, vpn, size, type)
register struct proc *p;
register u_int vpn;
register int size;
int type;
{
	register int i;
	register struct smem *sp;
	register struct p_sm *psm;
	int smindex;
	int offset;

	switch (type) {
	      case CDATA:
	      case CSTACK:
		newptes(p, vpn, size);
		break;
		
	      case CTEXT:
		while (p) {
			newptes (p, vpn, size);
			p = p->p_xlink;
		}
		break;

	      case CSMEM:
		/*
		 * We are guaranteed of one proc, flush that one, then 
		 * if more, flush those
		 */
		if (isasmsv(p, vpn, &smindex)) {
			psm = &p->p_sm[smindex];
			sp = psm->sm_p;
			newptes (p, vpn, size);
			offset = vpn - psm->sm_saddr;
			while (p = psm->sm_link) {
				psm = p->p_sm;
				smindex = -1;
				for (i = 0; i < p->p_smcount; i++, psm++) {
					if (sp == psm->sm_p) {
						smindex = i;
						break;
					}
				}
				if (smindex == -1)
					panic("flushpte: smindex == -1");
				newptes(p, psm->sm_saddr + offset, size);
			}
		}
		else
			panic("flushpte: !isasms");
		break;
	}
}

newptes(p, vpn, size)
register struct proc *p;
u_int vpn;
register int size;
{
	/*
	 * Could drop the new mappings in here but utlbmiss is probably
	 * faster.
	 */
	/*
	 * p_tlbpid set to -1 only in nexproc() and swapin(). p_tlbpid != -1
	   does not mean that it is valid, since every processor keeps track
	   of its own set of tlbpids individually. Here the assumption is
	   that newptes() will get called either for u.u_procp or every
	   processor will flush its tlb entries soon (ie. after pagein)
	 */

	if ((p->p_tlbpid == -1) || (CURRENT_CPUDATA->cpu_tps[p->p_tlbpid].tps_procpid != p->p_pid))
		return;
	/*
	 * ????? WAG: if more than 7 pages to unmap, cheaper to just flush our
	 * entire tlb context by assigning a new pid
	 */
	if (size > 7) {
		release_tlbpid(p);
		if (p == u.u_procp) {
			get_tlbpid(p);
			set_tlbwired(p);
		}
	} else {
		while (size > 0) {
#ifdef PROBE_BUG
			CKPROBE(unmaptlb(p->p_tlbpid, vpn++));
#else
			unmaptlb(p->p_tlbpid, vpn++);
#endif
			size--;
		}
	}
}

/*
 * TODO: check cache issues for these two routines
 */
/*
 * Change protection codes of text segment.
 * Have to flush translation buffer since this
 * affect virtual memory mapping of current process.
 */
chgprot(addr, tprot)
	caddr_t addr;
	long tprot;
{
	unsigned v;
	int tp;
	register struct pte *pte;
	register struct cmap *c;

	v = clbase(btop(addr));
	if (!isatsv(u.u_procp, v)) {
		u.u_error = EFAULT;
		return (0);
	}
	tp = vtotp(u.u_procp, v);
	pte = tptopte(u.u_procp, tp);
	if (pte->pg_fod == 0 && pte->pg_pfnum) {
		c = &cmap[pgtocm(pte->pg_pfnum)];
#ifdef TODO
	check this
#endif TODO
#ifdef ultrix
		if (c->c_blkno && c->c_mdev != MSWAPX)
                        munhash(mount[c->c_mdev].m_dev,
                                (daddr_t)(u_long)c->c_blkno,
                                u.u_procp->p_textp->x_gptr);

#else ultrix
		if (c->c_blkno && c->c_vp) {
			if (mfind(c->c_vp, c->c_blkno))
				munhash(c->c_vp, (daddr_t)(u_long)c->c_blkno);
		}
#endif ultrix
	}
	Hard(pte) &= ~PG_PROT;
	Hard(pte) |= tprot;
	if (pte->pg_m) {
		pte->pg_swapm = 1;
		pte->pg_m = 0;
	}
	distcl(pte);
	newptes(u.u_procp, v, CLSIZE);
	return (1);
}

settprot(tprot)
	long tprot;
{
	register int i;
	register struct pte *pte;

	pte = u.u_procp->p_textbr;
	for (i = 0; i < u.u_tsize; i++, pte++) {
		Hard(pte) &= ~PG_PROT;
		Hard(pte) |= tprot;
		if (pte->pg_m) {
			pte->pg_swapm = 1;
			pte->pg_m = 0;
		}
	}
	newptes(u.u_procp, btop(u.u_procp->p_textstart), u.u_tsize);
}

/*
 * Rest are machine-dependent
 */

getmemc(addr)
	caddr_t addr;
{
	register int c;

	c = *addr;
	return (c & 0377);
}

putmemc(addr, val)
	caddr_t addr;
{
	*addr = val;
}

/*
 * Move pages from one kernel virtual address to another.
 * Both addresses are assumed to reside in the Sysmap,
 * and size must be a multiple of CLSIZE.
 */
pagemove(from, to, size)
	register caddr_t from, to;
	int size;
{
	register struct pte *fpte, *tpte;

	if (size % CLBYTES || (u_int)from < K2BASE || (u_int)to < K2BASE)
		panic("pagemove");
	fpte = &Sysmap[btop(from - K2BASE)];
	tpte = &Sysmap[btop(to - K2BASE)];
	while (size > 0) {
		*tpte = *fpte;
		Hard(fpte) = 0;
#ifdef PROBE_BUG
		CKPROBE(unmaptlb(0, btop(from)));
		CKPROBE(tlbdropin(0, to, Hard(tpte)));
#else
		unmaptlb(0, btop(from));
		tlbdropin(0, to, Hard(tpte));
#endif
		tpte++;
		fpte++;
		from += NBPG;
		to += NBPG;
		size -= NBPG;
	}
}

/*
 * initialize per process tlb mapping information used for second level
 * mappings
 */
clear_tlbmappings(p)
	register struct proc *p;
{
	register int j;
	register struct tlbinfo *ti;

	ti = p->p_tlbinfo;
	for (j = 0; j < NPAGEMAP; j++) {
		(ti+j)->hi.th_word = K0BASE;
		(ti+j)->lo.tl_word = 0;
	}
	p->p_tlbindx = 0;
}

/*
 * Allocate a contiguous chunk of memory in KSEG0 space
 */

kseg0_alloc(size)
{
	int pf;

	if (size <= 0) panic("kseg0_alloc: bad size");
	if ((pf = pfalloc(CSYS,btoc(size)))) return PHYS_TO_K0(ctob(pf));
	else return 0;
}
