/*
 * @(#)pte.h	4.1  (ULTRIX)        7/2/90    
 */

/************************************************************************
 *									*
 *			Copyright (c) 1985 - 1989 by			*
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
/*	pte.h	6.1	83/07/29	*/

/*
 * Modification History:
 *
 * 19-Oct-89 -- jmartin
 *	Delete definitions of PG_BLKNO, PUT_PG_BLKNO, and struct hpte.
 *	Invent union pte_words.
 *
 * 14 June 89 -- jaa
 *	added dummy constants for arch compatability
 *
 * 27 Jan 88 -- us
 *	Added 2.4 memory allocator.  
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
 ******** smp change history above *******
 * 14-Feb-89 -- jmartin
 *	#define PG_BLKNO, PUT_PG_BLKNO, and PTE_PFNSHIFT for mips
 *	compatibility.
 *
 * 17-Jan-1989	Todd M. Katz		TMK0001
 *	Add access control constants( PROT_{ NOACC, KW, KR, URKW, URKR }).
 *
 * 18-Jul-88 jaa
 *	added eUsrptmap[2] as end of user page table map redzone
 *
 * 15-Feb-88 -- fred (Fred Canter)
 *	Also define VAXstar device driver maps for VAX420 (CVAXstar/PVAX).
 *
 * 19-Jan-88 -- jaw
 *	Added changes for calypso cvax.
 *
 * 12-11-87	Robin L. and Larry C.
 *      Added portclass/kmalloc support to the system.
 *
 * 26 Mar 86 -- depp
 *	changed fields in struct fpte, to permit partitions of up to 8 Gb
 *
 * 13-Dec-86 -- rafiey (Ali Rafieymehr)
 *	Added sgbufmap[] for VAXstar color driver.
 *
 *  5-Aug-86 -- darrell (Darrell Dunnuck)
 *	Added mapping for VAXstar TZK50 driver buffer.
 *
 * 18-Jun-86 -- fred (Fred Canter)
 *	Changes for VAXstar kernel support.
 *
 * 02-Apr-86 -- jaw  add support for nautilus console and memory adapter
 *
 * 11-Mar-86 tresvik
 *	add support for SAS memory driver
 *
 * 05 Mar 86 -- bjg
 *	Removed msgbufmap; (replaced by error logger)
 *
 * 26 Feb 86 -- depp
 *	Added Virtual Address constants (VA_)
 *
 * 24 Feb 86 -- depp
 *	Added externs for kernel memory allocation maps (dmempt*)
 *
 * 18-Feb-86 -- jaw  add routines to cpu switch for nexus/unibus addreses
 *		     also got rid of some globals like nexnum.
 *		     ka8800 cleanup.
 *
 * 04-Feb-86 -- tresvik
 *	added VMB boot path support
 *
 * 04 Feb 86 -- jaw  add mapping for 8800 adpters.
 *
 * 12 Nov 85 -- depp
 *	Removed "pg_cm" define due to possible conflict.  Created constants
 *	to indicate whether or not to clear the shmem modification bit.
 *
 * 18 Jul 85 -- depp
 *	Added "pg_cm" define.  This is used in shared memory.  If this
 *	bit is set in the "global" pte, then clear the pg_m bit in
 *	all of the associated pte clusters of all processes that are
 *	attached.
 *
 * 19-Jun-85 -- jaw
 *	VAX8200 name change.
 *	
 * 18-Jun 85 -- Jaw
 *	added extern pte definition for floppy on 8200.
 *
 * 29 Apr 85 -- depp
 *	Removed SHMEM conditional compiles
 *
 *  9 Apr 85 -- depp
 *	Added shared memory allocated bit to PTE
 *
 * 12-MAR-85 - JAW
 *	Changes for support of the VAX8200 were merged in.
 *
 * 27-Feb-85 -tresvik
 *	Changes for support of the VAX8600 were merged in.
 *
 * 11-March-85 -Larry Cohen
 *   	Change definition of PG_FZERO and PG_FTEXT to support larger NOFILE
 *
 *
 */

/*
 * VAX page table entry
 *
 * There are two major kinds of pte's: those which have ever existed (and are
 * thus either now in core or on the swap device), and those which have
 * never existed, but which will be filled on demand at first reference.
 * There is a structure describing each.  There is also an ancillary
 * structure used in page clustering.
 *
 *	LIMITS
 *
 *  pg_pfnum	21 bits		512 Mb physical memory
 *  pg_blkno	24 bits		8 Gb in a disk partition
 *  pg_fileno	 1 bit		FTEXT and FZERO only!
 */

#ifndef LOCORE
struct pte
{
unsigned int	pg_pfnum:21,		/* core page frame number or 0 */
		pg_alloc:1,		/* alloc bit - used in shmem */
		:1,
		pg_vreadm:1,		/* modified since vread (or with _m) */
		pg_swapm:1,		/* have to write back to swap */
		pg_fod:1,		/* is fill on demand (=0) */
		pg_m:1,			/* hardware maintained modified bit */
		pg_prot:4,		/* access control */
		pg_v:1;			/* valid bit */
};

struct fpte
{
unsigned int	pg_blkno:24,		/* file system block number */
		pg_fileno:1,		/* file mapped from or TEXT or ZERO */
		pg_fod:1,		/* is fill on demand (=1) */
		:1,
		pg_prot:4,
		pg_v:1;
};

union pte_words {
	int hardware_word;
	struct pte whole_pte;
};
#endif

#define	PG_V		0x80000000
#define	PG_PROT		0x78000000
#define	PG_M		0x04000000
#define	PG_FOD		0x02000000
#define	PG_VREADM	0x00800000
#define PG_ALLOC	0x00200000
#define	PG_PFNUM	0x001fffff

#define	PG_FZERO	0
#define	PG_FTEXT	1
#define	PG_FMAX		(PG_FTEXT)

#define	PG_NOACC	0
#define	PG_KW		0x10000000
#define	PG_KR		0x18000000
#define	PG_UW		0x20000000
#define	PG_URKW		0x70000000
#define	PG_URKR		0x78000000

/* these are dummy args for mips compatability */
#define DO_CACHE	0x0
#define NO_CACHE	0x0

#define	PROT_NOACC	 0
#define	PROT_KW		 2
#define	PROT_KR		 3
#define	PROT_UW		 4
#define	PROT_URKW	14
#define	PROT_URKR	15

/* Virtual address mask constants */
#define VA_BYTEOFFS	(NBPG - 1)
#define VA_VPN		(0x3fffffff - VA_BYTEOFFS)
#define VA_USER		(0x40000000)
#define VA_SYS		(0x80000000)
#define VA_SPACE	(VA_SYS | VA_USER)

#define PTE_PFNSHIFT	0

/*
 * Pte related macros
 */
#define	dirty(pte)	((pte)->pg_fod == 0 && (pte)->pg_pfnum && \
			    ((pte)->pg_m || (pte)->pg_swapm))

#define SET_SWDIRTY(pte) ((pte)->pg_m = 1)

#define CLEAR_DIRTY(pte) ((pte)->pg_m = 0)

#ifndef LOCORE
#ifdef KERNEL
struct	pte *vtopte();

/* utilities defined in locore.s */
extern	struct pte Sysmap[];
extern	struct pte Usrptmap[];
extern	struct pte usrpt[];
extern	struct pte Swapmap[];
#define Forkmap (u.u_procp->p_addr - FORKPAGES)
extern	struct pte Xswapmap[];
extern	struct pte Xswap2map[];
extern	struct pte Pushmap[];
extern	struct pte mmap[];
extern	struct pte vmbinfomap[];
extern	struct pte Nexmap[][16];
extern	struct pte Ioamap[][1];
extern	struct pte kmempt[];  /* us */
extern  struct pte ekmempt[]; /* us */
extern	struct pte scsmemptmap[];
extern  struct pte scsmempt[];
extern  struct pte eUsrptmap[];

#ifdef VAX8800
extern	struct pte V8800mem[];
extern  struct pte ADPT_loopback[][1];
extern struct pte Nbia[][1];
#endif /* VAX8800 */
#ifdef VAX6200
extern 	struct pte V6200csr[];
extern 	struct pte CCAmap[];
#endif

#ifdef VAX8200
extern	struct pte V8200wmap[];
extern	struct pte V8200pmap[];
extern	struct pte V8200rmap[];
#endif /* VAX8200 */
#if defined(VAX8200) || defined(VAX8800)
extern  struct pte rxbufmap[];
#endif 
#ifdef SAS
extern	struct pte mdbufmap[];
#endif /* SAS */
#if defined (MVAX) || defined (VAX420)
extern  struct pte sdbufmap[];
extern  struct pte stbufmap[];
extern  struct pte sgbufmap[];
#endif /*	MVAX || VAX420 */
#ifdef	KDEBUG
extern	struct pte Kdbmap[];
#endif /*  KDEBUG */
#endif
#endif
