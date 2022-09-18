/*
 *	@(#)pte.h	4.2	(ULTRIX)	9/4/90
 */

/* ------------------------------------------------------------------ */
/* | Copyright Unpublished, MIPS Computer Systems, Inc.  All Rights | */
/* | Reserved.  This software contains proprietary and confidential | */
/* | information of MIPS and its suppliers.  Use, disclosure or     | */
/* | reproduction is prohibited without the prior express written   | */
/* | consent of MIPS.                                               | */
/* ------------------------------------------------------------------ */

/*
 * MIPS page table entry
 *
 * There are two major kinds of pte's: those which have ever existed
 * (and are thus either now in core or on the swap device), and those
 * which have never existed, but which will be filled on demand at first
 * reference.  There is a structure describing each.
 */

/*
 * Modification History:
 *
 * 19-Oct-89 -- jmartin
 *	Shuffle the fields of (struct pte) to create an "abstract"
 *	format, which is transformed to machine format just before
 *	being written to the address translation hardware.
 */

#ifdef KERNEL
#include "../h/ansi_compat.h"
#else
#include <ansi_compat.h>
#endif

#ifndef LOCORE
struct pte
{
#ifdef __MIPSEB
unsigned int	pg_swapm:1,		/* SW: page must be forced to swap */
		pg_fod:1,		/* SW: is fill on demand (=0) */
		pg_prot:2,		/* SW: access control */
		pg_pfnum:24,		/* HW: core page frame number or 0 */
		pg_n:1,			/* HW: non-cacheable bit */
		pg_m:1,			/* HW: modified (dirty) bit */
		pg_v:1,			/* HW: valid bit */
		pg_g:1;			/* HW: ignore pid bit */
#endif /* __MIPSEB */
#ifdef __MIPSEL
unsigned int	pg_g:1,			/* HW: ignore pid bit */
		pg_v:1,			/* HW: valid bit */
		pg_m:1,			/* HW: modified (dirty) bit */
		pg_n:1,			/* HW: non-cacheable bit */
		pg_pfnum:24,		/* HW: core page frame number or 0 */
		pg_prot:2,		/* SW: access control */
		pg_fod:1,		/* SW: is fill on demand (=0) */
		pg_swapm:1;		/* SW: page must be forced to swap */
#endif /* __MIPSEL */
};

struct fpte
{
#ifdef __MIPSEB
unsigned int	pg_fileno:1,		/* file mapped from or TEXT or ZERO */
		pg_fod:1,		/* is fill on demand (=1) */
		pg_prot:2,
		pg_blkno:24,		/* file system block number */
		:4;			/* overlays v,m,g,n bits */
#endif /* __MIPSEB */
#ifdef __MIPSEL
unsigned int	:4,			/* overlays v,m,g,n bits */
		pg_blkno:24,		/* file system block number */
		pg_prot:2,
		pg_fod:1,		/* is fill on demand (=1) */
		pg_fileno:1;		/* file mapped from or TEXT or ZERO */
#endif /* __MIPSEL */
};

union pte_words {
	int hardware_word;
	struct pte whole_pte;
};
#endif /* !LOCORE */

#define	PG_PFNUM	0x0ffffff0
#define	PG_N		0x00000008
#define	PG_M		0x00000004
#define	PG_V		0x00000002
#define	PG_G		0x00000001
#define	PG_FILENO	0x80000000
#define	PG_SWAPM	0x80000000
#define	PG_FOD		0x40000000
#define	PG_PROT		0x30000000

#define	PG_FZERO	0
#define	PG_FTEXT	1
#define	PG_FMAX		(PG_FTEXT)

/* protection MASKS */
#define	PG_KR		0x00000000
#define	PG_KW		0x10000000
#define	PG_URKR		0x20000000
#define	PG_UW		0x30000000

/* protection VALUES */
#define	PROT_KR		0
#define	PROT_KW		1
#define	PROT_URKR	2
#define	PROT_UW		3

#define	PTE_PFNSHIFT	4
#define	PTE_FILENOSHIFT	31

#define DO_CACHE	0x0
#define NO_CACHE	PG_N

/*
 * Pte related macros
 */
#define	dirty(pte)	\
    ((pte)->pg_fod == 0 && (pte)->pg_pfnum && ((pte)->pg_m || (pte)->pg_swapm))

/* this macro will insure readonly is maintained */
#define SET_SWDIRTY(pte)  { \
	 if ((pte)->pg_prot < PROT_UW) \
		 (pte)->pg_swapm = 1; \
	 else \
		 (pte)->pg_m = 1;  \
}

#define CLEAR_DIRTY(pte) (*(int *)(pte) &= ~(PG_M | PG_SWAPM))

#ifndef LOCORE
#ifdef KERNEL

/* utilities defined in locore.s */
extern	struct pte Sysmap[];
extern	struct pte Usrptmap[];
extern	struct pte usrpt[];
extern	struct pte msgbufmap[];
extern	struct pte camap[];
#define Forkmap	((struct pte *)(KPTEBASE+KPTESIZE)-FORKPAGES-REDZONEPAGES)
#define VA_forkutl	((char *)(USRSTACK+(REDZONEPAGES*NBPG)))
#define VA_vfutl	VA_forkutl
#define	forkutl	(*(struct user *)VA_forkutl)
extern	struct pte Swapmap[];
extern	struct pte Vfmap[];
extern	struct pte mdbufmap[];
extern	struct pte scsmemptmap[];
extern	struct pte scsmempt[];
extern	struct pte V6200csr[];
extern	struct pte Isis_vec[];
extern	struct pte Nexmap[][2];
extern	struct pte CCAmap[];

#endif /* KERNEL */
#endif /* !LOCORE */
