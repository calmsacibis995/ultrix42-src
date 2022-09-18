/* 	@(#)vmmac.h	4.6	(ULTRIX)	2/14/91 	*/

/************************************************************************
 *									*
 *			Copyright (c) 1986 - 1989 by			*
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
 *   Modification History:
 *
 * 13-Feb-91 -- jmartin
 *	Define SM_PSM_UPDATE if not already defined in ../machine/vm/vmmac_md.h
 *
 * 13-Aug-90 -- sekhar
 *      added a vax version of  the vtokpfnum macro.
 *
 * 15-Jun-90 -- sekhar
 *      Added the vtokpfnum macro for use by memory mapped devices.
 *
 * 30-Nov-89 -- jmartin
 *	Support for coprocessor access to process virtual memory.
 *
 * 27-Sep-89 -- Pete Keilty
 *	Merge ISIS pool vmmac.h into V4.0 pool.
 *	Remove gmm svtopte(v) macro, use generalized.
 *
 * 14-Jun-89 -- jaa
 *	moved SM_PSM_CLEAR to "arch"/vmmac_md.h
 *
 * 08-Jun-89 -- gmm
 *	Added svtopte(v) macro for mips
 *
 * 02-May-89 -- jaw, jmartin
 *	fix forkutl to work on mips.
 *
 * 13-Apr-89 -- Pete Keilty
 *	Generalized svtopte to use the MAPPED_SYSBASE define.
 *
 * 22-Feb-89 -- map (Mark Parenti)
 *	Add #ifdef around include for user level programs
 *
 * 17-Jan-1989	Todd M. Katz		TMK0001
 *	Add macro sptetosva() and constant MAPPED_SYSBASE.
 *
 * 15 Dec 88 -- jmartin
 *	SMP locking for shared memory data structures (cf. h/shm.h)
 *	Integration of v3.0 shared memory changes, i.e. KM_ALLOC of
 *	per-process shared memory structures.
 *
 *  6 Dec 88 -- jmartin
 *	Add SMP locking to the SM_LOCK/SM_UNLOCK macros.  Add svtopte
 *	macro.
 *
 * 31 Aug 88 -- jmartin
 *	Add SMP locking to the X_LOCK/X_UNLOCK macros.
 *
 * 25 Jul 88 -- jmartin
 *	Add SMP locking to the MLOCK/MUNLOCK macros.
 *
 * 27 Jan 88 -- us
 *	Removed calls to dmxtob, btodmx, as per jaa changes for 2.4
 *	allocator.
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
 * 14 Dec 87 -- jaa
 * 	Removed dmxtob, btodmx macros
 *
 * 29 Apr 86 -- depp
 *	Added new locking macros to replace the associated kernel functions.
 *	New lock/unlock/wait macros have been added for page locking,
 *	shared segment locking and text segment locking:
 *		MLOCK/MUNLOCK/MWAIT	  replace  mlock/munlock/mwait
 *		SM_LOCK/SM_UNLOCK/SM_WAIT replace  smlock/smunlock/smwait
 *		X_LOCK/X_UNLOCK/X_WAIT	  replace  xlock/xunlock/xwait
 *
 * 24 Feb 86 -- depp
 *	Added new macros "dmxtob" and "btodmx" for kernel memory allocation
 *	Plus, I added "rbtop" "ispgbnd" "isclbnd" for general use.  NOTE:
 *	"rbtop" SHOULD be used in place of "btoc" EXCEPT where it's known
 *	that clicks are used (a click may not always equal a page!!).
 *
 * 	Also added new macro to convert system virtual addresses to physical
 *	"svtophy"
 */

#ifdef KERNEL
#include "../h/ansi_compat.h"
#include "../machine/vm/vmmac_md.h"
#else
#include <ansi_compat.h>
#include <machine/vm/vmmac_md.h>
#endif /* KERNEL */

/*
 * Virtual memory related conversion macros
 */

/* Core clicks to number of pages of page tables needed to map that much */
#define	ctopt(x)	(((x)+NPTEPG-1)/NPTEPG)

#ifdef __vax
/* Virtual page numbers to text|data|stack segment page numbers and back */
#define	vtotp(p, v)	((int)(v)-LOWPAGES)
#define	vtodp(p, v)	((int)((v) - stoc(ctos((p)->p_tsize)) - LOWPAGES))
#endif /* __vax */

#ifdef __mips
/* Virtual page numbers to text|data|stack segment page numbers and back */
#define	vtotp(p, v)	((int)(v) - btop((p)->p_textstart))
#define vtodp(p, v)	((int)(v) - btop((p)->p_datastart))
#define vtosmp(sp,v)	((int)(v) - (sp)->sm_saddr)
#define vtofp(p, v)	((int)(v) - btop(&forkutl))
#endif /* __mips */
#define	vtosp(p, v)	((int)(btop(USRSTACK) - 1 - (v)))
#ifdef __vax
#define	tptov(p, i)	((unsigned)(i) + LOWPAGES)
#define	dptov(p, i)	((unsigned)(stoc(ctos((p)->p_tsize)) + (i) + LOWPAGES))
#endif /* __vax */
#ifdef __mips
#define	tptov(p, i)	((unsigned)(i) + btop((p)->p_textstart))
#define	dptov(p, i)	((unsigned)(i) + btop((p)->p_datastart))
#define smptov(p, s, i)	((unsigned)(i) + sm_retrieve_sa(p, s))
#endif /* __mips */
#define	sptov(p, i)	((unsigned)(btop(USRSTACK) - 1 - (i)))

/* Tell whether virtual page numbers are in text|data|stack segment */
#ifdef __vax
#define	isassv(p, v)	((v) >= btop(USRSTACK) - (p)->p_ssize)
#define	isatsv(p, v)	(((v) - LOWPAGES) < (p)->p_tsize)
#define	isadsv(p, v)	(((v) - LOWPAGES) >= stoc(ctos((p)->p_tsize)) && \
				!isassv(p, v))
#endif /* __vax */
#ifdef __mips
#define	isassv(p, v)	(((v) >= (btop(USRSTACK) - (p)->p_ssize)) && \
				((v) < btop(USRSTACK)))
#define	isatsv(p, v)	((((v) - btop((p)->p_textstart)) < (p)->p_tsize) && \
				(((v) >= btop((p)->p_textstart))))
#define	isadsv(p, v)	((((v) - btop((p)->p_datastart)) < (p)->p_dsize) && \
				(((v) >= btop((p)->p_datastart))))
#define isasmsv(p, v, s) (sm_retrieve_sms(p, v, s))
#define isafsv(p, v)   ((v) >= btop(&forkutl) && (v) < btop(&forkutl)+FORKPAGES)
#endif /* __mips */

/* Tell whether pte's are text|data|stack */
#ifdef __vax
#define	isaspte(p, pte)		((pte) > sptopte(p, (p)->p_ssize))
#define	isatpte(p, pte)		((pte) < dptopte(p, 0))
#define	isadpte(p, pte)		(!isaspte(p, pte) && !isatpte(p, pte))
#endif /* __vax */

#ifdef __mips
#define	isaspte(p, pte)		(((pte) > sptopte(p, (p)->p_ssize)) && \
					((pte) <= sptopte(p, 0)))
#define	isatpte(p, pte)		(((pte) < tptopte(p, (p)->p_tsize)) && \
					((pte) >= tptopte(p, 0)))
#define	isadpte(p, pte)		(((pte) < dptopte(p, (p)->p_dsize)) && \
					((pte) >= dptopte(p, 0)))
#endif /* __mips */
/* Text|data|stack pte's to segment page numbers and back */
#ifdef __vax
#define	ptetotp(p, pte)		((pte) - (p)->p_p0br)
#define	ptetodp(p, pte)		((pte) - ((p)->p_p0br + (p)->p_tsize))
#endif /* __vax */

#ifdef __mips
#define	ptetotp(p, pte)		((pte) - (p)->p_textbr)
#define	ptetodp(p, pte)		((pte) - (p)->p_databr)
/*
 * u block not located in users virtual address space.
 */
#endif /* __mips */
#ifdef __vax
#define	ptetosp(p, pte)	\
	(((p)->p_p0br + (p)->p_szpt*NPTEPG - HIGHPAGES - 1) - (pte))
#endif /* __vax */
#ifdef __mips
#define	ptetosp(p, pte)	\
	(((p)->p_stakbr + (p)->p_stakpt*NPTEPG - HIGHPAGES - 1) - (pte))
#endif /* __mips */

#ifdef __vax
#define	tptopte(p, i)		((p)->p_p0br + (i))
#define	dptopte(p, i)		((p)->p_p0br + (p)->p_tsize + (i))
#endif /* __vax */

#ifdef __mips
#define	tptopte(p, i)		((p)->p_textbr + (i))
#define	dptopte(p, i)		((p)->p_databr + (i))
#endif /* __mips */
#ifdef __vax
#define	sptopte(p, i) \
	(((p)->p_p0br + (p)->p_szpt*NPTEPG - HIGHPAGES - 1) - (i))
#endif /* __vax */
#ifdef __mips
#define	sptopte(p, i) \
	(((p)->p_stakbr + (p)->p_stakpt*NPTEPG - HIGHPAGES - 1) - (i))
#define fptopte(p, i) \
	(((p)->p_stakbr + (p)->p_stakpt*NPTEPG - (FORKPAGES+REDZONEPAGES) + (i)))
#endif /* __mips */

#ifdef __mips
struct	pte *vtopte();
#endif /* __mips */

/* Bytes to pages with/without rounding, and back */
#define rbtop(x)	(((unsigned)(x) + (NBPG - 1)) >> PGSHIFT)
#define	btop(x)		(((unsigned)(x)) >> PGSHIFT)
#define	ptob(x)		((caddr_t)((x) << PGSHIFT))

/* Does the address/size fall on a page/cluster boundary ? */
#define ispgbnd(x)	(((unsigned)(x) & (NBPG - 1)) == 0)
#define isclbnd(x)	(((unsigned)(x) & (CLBYTES - 1)) == 0)

/* Turn virtual addresses into kernel map indices */
#define	kmxtob(a)	(usrpt + (a) * NPTEPG)
#define	btokmx(b)	(((b) - usrpt) / NPTEPG)

/* User area address and pcb bases */
#ifdef __vax
#define	uaddr(p)	(&((p)->p_p0br[(p)->p_szpt * NPTEPG - UPAGES]))
#endif /* __vax */
#ifdef __mips
#define	uaddr(p)	((p)->p_addr)
#endif /* __mips */
#ifdef __vax
#define	pcbb(p)		((p)->p_addr[0].pg_pfnum)
#endif /* __vax */
#ifdef __mips
#define	pcbb(p)		(p)
#endif /* __mips */

/* Average new into old with aging factor time */
#define	ave(smooth, cnt, time) \
	smooth = ((time - 1) * (smooth) + (cnt)) / (time)

/* Abstract machine dependent operations */
#ifdef __vax
#define	setp0br(x)	(u.u_pcb.pcb_p0br = (x), mtpr(P0BR, x))
#define	setp0lr(x)	(u.u_pcb.pcb_p0lr = \
			    (x) | (u.u_pcb.pcb_p0lr & AST_CLR), \
			 mtpr(P0LR, x))
#define	setp1br(x)	(u.u_pcb.pcb_p1br = (x), mtpr(P1BR, x))
#define	setp1lr(x)	(u.u_pcb.pcb_p1lr = (x), mtpr(P1LR, x))
#define	initp1br(x)	((x) - P1PAGES)
#endif /* __vax */

#ifdef __vax
/* convert system VA to physical */
#define ptetosv(x) \
	((caddr_t)((unsigned)ptob((int)((struct pte *)(x) - Sysmap)) | VA_SYS))
#define	svtophy(v) \
	((int)(ptob((Sysmap[btop((int)(v) & ~VA_SYS)]).pg_pfnum)) | \
	 ((int) (v) & VA_BYTEOFFS))
#endif /* __vax */

/* convert system va to a page frame number - for use by memory */
/* mapped devices. */

#ifdef __mips
#define vtokpfnum(v) (vm_svtophy(v) >> PGSHIFT)
#else
#define vtokpfnum(v) (svtophy(v) >> PGSHIFT)
#endif /* __mips */


/* convert system pte address to corresponding system virtual address */
/* NOTE: This must be generalized in a future release! */
#ifdef __vax
#define MAPPED_SYSBASE  VA_SYS
#else
#define MAPPED_SYSBASE  K2BASE
#endif /* __vax */

#define svtopte(v) ((struct pte *)(&Sysmap[btop((unsigned)(v) & ~MAPPED_SYSBASE)]))
#define sptetosva( p ) (unsigned)(MAPPED_SYSBASE | (((p) - Sysmap) << PGSHIFT))

#ifdef __mips
#define svtophy(v)      ((int) vm_svtophy(v))
#endif /* __mips */

#define	outofmem()	wakeup((caddr_t)&proc[2]);

/*
 * Page clustering macros.
 * 
 * dirtycl(pte)			is the page cluster dirty?
 * anycl(pte,fld)		does any pte in the cluster has fld set?
 * zapcl(pte,fld) = val		set all fields fld in the cluster to val
 * distcl(pte)			distribute high bits to cluster; note that
 *				distcl copies everything but pg_pfnum,
 *				INCLUDING pg_m!!!
 *
 * In all cases, pte must be the low pte in the cluster, even if
 * the segment grows backwards (e.g. the stack).
 */

#define	Hard(pte)	(((union pte_words *)(pte))->hardware_word)

#if CLSIZE==1
#define	dirtycl(pte)	dirty(pte)
#define	anycl(pte,fld)	((pte)->fld)
#define	zapcl(pte,fld)	(pte)->fld
#define	distcl(pte)
#endif

#if CLSIZE==2
#define	dirtycl(pte)	(dirty(pte) || dirty((pte)+1))
#define	anycl(pte,fld)	((pte)->fld || (((pte)+1)->fld))
#define	zapcl(pte,fld)	(pte)[1].fld = (pte)[0].fld
#define distcl(pte)	(Hard((pte)+1) = Hard(pte) + (1<<PTE_PFNSHIFT))
#endif

#if CLSIZE==4
#define	dirtycl(pte) \
    (dirty(pte) || dirty((pte)+1) || dirty((pte)+2) || dirty((pte)+3))
#define	anycl(pte,fld) \
    ((pte)->fld || (((pte)+1)->fld) || (((pte)+2)->fld) || (((pte)+3)->fld))
#define	zapcl(pte,fld) \
    (pte)[3].fld = (pte)[2].fld = (pte)[1].fld = (pte)[0].fld
#define distcl(pte) \
	(Hard((pte)+1) = Hard(pte) + (1<<PTE_PFNSHIFT), \
	 Hard((pte)+2) = Hard(pte) + (2<<PTE_PFNSHIFT), \
	 Hard((pte)+3) = Hard(pte) + (3<<PTE_PFNSHIFT))
#endif

/************************************************************************
 ************************************************************************
 *
 * 		LOCKING MACROS
 *
 ************************************************************************
 ************************************************************************/

/*
 * lock/unlock/wait on a PAGE FRAME
 */

/*  Assumes we hold lk_cmap lock */
#define MLOCK(c) { \
	while ((c)->c_lock) { \
		(c)->c_want = 1; \
		sleep_unlock((caddr_t)(c), PSWP+1, &lk_cmap); \
		smp_lock(&lk_cmap, LK_RETRY); \
	} \
	(c)->c_lock = 1; \
}

/*  Assumes we hold lk_cmap lock */
#define MUNLOCK(c) { \
	if ((c)->c_lock == 0) \
		panic("MUNLOCK: dup page unlock"); \
	if ((c)->c_want) { \
		wakeup((caddr_t)(c)); \
		(c)->c_want = 0; \
	} \
	(c)->c_lock = 0; \
}

#define MWAIT(c) { \
	MLOCK(c); \
	MUNLOCK(c); \
}


/* 
 * lock/ulock/wait on a SHARED MEMORY SEGMENT
 */

#define SM_LOCK(s) {						\
	int _S = splimp();					\
	smp_lock(&lk_smem, LK_RETRY);				\
	while((s)->sm_flag & SMLOCK) {				\
		(s)->sm_flag |= SMWANT;				\
		sleep_unlock((caddr_t)(s), PSWP, &lk_smem);	\
		smp_lock(&lk_smem, LK_RETRY);			\
	}							\
	(s)->sm_flag |= SMLOCK;					\
	smp_unlock(&lk_smem);					\
	(void)splx(_S);						\
}

#define SM_UNLOCK(s) {						\
	int _S = splimp();					\
	smp_lock(&lk_smem, LK_RETRY);				\
	if (((s)->sm_flag ^= SMLOCK) & SMLOCK)			\
		panic("SM_UNLOCK: shared memory not locked");	\
	if ((s)->sm_flag & SMWANT) {				\
		(s)->sm_flag &= ~SMWANT;			\
		smp_unlock(&lk_smem);				\
		wakeup((caddr_t)(s));				\
	} else							\
		smp_unlock(&lk_smem);				\
	(void)splx(_S);						\
}

/*
 * lock/unlock/wait on TEXT SEGMENT
 */

#define X_LOCK(x) {						\
	int _S = splimp();					\
	smp_lock(&lk_text, LK_RETRY);				\
	while ((x)->x_flag & XLOCK) {				\
		(x)->x_flag |= XWANT;				\
		sleep_unlock((caddr_t)(x), PSWP, &lk_text);	\
		smp_lock(&lk_text, LK_RETRY);			\
	}							\
	(x)->x_flag |= XLOCK;					\
	(x)->x_ownlock = u.u_procp;				\
	smp_unlock(&lk_text);					\
	(void)splx(_S);						\
}

#define X_UNLOCK(x) {					\
	int _S = splimp();				\
	smp_lock(&lk_text, LK_RETRY);			\
	if (((x)->x_flag ^= XLOCK) & XLOCK)		\
		panic("X_UNLOCK: text not locked");	\
	(x)->x_ownlock = (struct proc *) 0;		\
	if ((x)->x_flag & XWANT) {			\
		(x)->x_flag &= ~XWANT;			\
		smp_unlock(&lk_text);			\
		wakeup((caddr_t)(x));			\
	} else						\
		smp_unlock(&lk_text);			\
	(void)splx(_S);					\
}

/*
 * macro to update per process shared memory global information
 */

#ifndef SM_PSM_UPDATE
#define SM_PSM_UPDATE(p) { \
	(p)->p_smbeg = (p)->p_sm[0].sm_saddr; \
	(p)->p_smend = (p)->p_sm[(p)->p_smcount - 1].sm_eaddr; \
	(p)->p_smsize = (p)->p_smend - (p)->p_smbeg; \
}
#endif /*  SM_PSM_UPDATE */

#define SM_PSM_COPY(pp,cp) { \
        (cp)->p_smsize = (pp)->p_smsize; \
        (cp)->p_smbeg = (pp)->p_smbeg;   \
        (cp)->p_smend = (pp)->p_smend;   \
}

#ifndef clear_cpu_tlbs
#define clear_cpu_tlbs(segment, vpn, pagecount, type)	1
#endif /* clear_cpu_tlbs */

#ifndef clear_dev_tlbs
#define clear_dev_tlbs(segment, vpn, pagecount, type)	1
#endif /* clear_dev_tlbs */

#ifndef release_dev_VM_maint
#define release_dev_VM_maint(p)				NULL
#endif /* release_dev_VM_maint */

#define clear_foreign_tlbs(segment, vpn, pagecount, type) \
 (clear_cpu_tlbs(segment, vpn, pagecount, type) && \
  clear_dev_tlbs(segment, vpn, pagecount, type))
