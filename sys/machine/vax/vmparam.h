/*	@(#)vmparam.h	4.3	(ULTRIX)	9/6/90	*/

/************************************************************************
 *									*
 *			Copyright (c) 1985, 1986, 1988 by		*
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
/*	vmparam.h	6.1	83/07/29	*/
/*
 * Modification History:
 *
 * 4-Sep-90	dlh
 *	added vector processor support code
 *
 * 10 Aug 90 -- jaa
 *	MAXDSIZ and MAXSSIZ are in bytes for commonality (vax/mips)
 *
 * 12-Jun-89 bp
 *	Added kernel memory allocator default high water mark.
 *
 * 12-Jun-89    gg
 *	Add ifndef to MAXSSIZ to prevent redefinition of MAXSSIZ
 *
 * 18-Jul-88	jaa
 *	condensed MAXUVA calculation
 *
 * 6 Apr 88 -- jaa
 *	removed tbsync() from mapin macro.
 *	tbsync() should be called outside of any loops
 *
 * 8  Mar 88 -- jaa
 *	Added MAXUVA (max agg user addr space in meg) 
 *	to allow the user page table size to be configurable
 *	by the system administrator.
 *	Also, doubled usrptsize if > 64 meg of memory.
 *
 * 25 Jan 88 -- jmartin
 *	Define newptes as a macro, which never substitutes TBIA for
 *	multiple TBISs.
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
 * 14 Jan 88 -- tresvik
 *	Added VMBINFOPAGES to be used in spt.s and machdep.c
 *
 * 13 Jul 87 -- bstevens
 *	Doubled amount of kmalloc(able) memory for workstations.
 *
 * 12 Feb 87 -- depp
 *	Added new parameters {MIN,MAX}_DMEM_PAGES.  These parameters are
 *	used to size the page table space for the kernel memory allocator
 *
 * 12 Jan 87 -- depp
 *	Added new parameter -- UCLEAR
 *
 * 02 Apr 86 -- depp
 *	Added new parameter for determining the spead of the hands in the
 *	2 hand global replacement clock algorithm (HANDSPREAD).  Also,
 *	changed the method of calculating lotsfree.
 *
 * 12-Feb-86 -- jrs
 *	Added call to tbsync() for mp translation buffer control
 *
 * 22-Aug-85 -tresvik
 *	Increased USRPTSIZE from (8*NPTEPG) to (32*NPTEPG) in order to
 *      allow more processes to run.  Needs to be fixed differently in
 *	the future.
 */

/*
 * Machine dependent constants for VAX
 */
/*
 * USRTEXT is the start of the user text/data space, while USRSTACK
 * is the top (end) of the user stack.  LOWPAGES and HIGHPAGES are
 * the number of pages from the beginning of the P0 region to the
 * beginning of the text and from the beginning of the P1 region to the
 * beginning of the stack respectively.
 */

#define	USRTEXT		0
#define	USRSTACK	(0x80000000-HIGHPAGES*NBPG)
					/* Start of user stack */
#define	P1PAGES		0x200000	/* number of pages in P1 region */
#define	LOWPAGES	0

#ifndef LOCORE
#define	HIGHPAGES	(UPAGES+FORKPAGES)
#endif /*  LOCORE */

#define VMBINFOPAGES	128		/* max pages to map for vmb_info */

/*
 * Virtual memory related constants
 */
#define	SLOP	32
#ifndef MAXDSIZ
#define	MAXDSIZ		(32*1024*1024)		/* max data size bytes */
#endif

#ifndef MAXSSIZ
#define	MAXSSIZ		(32*1024*1024)		/* max stack size bytes */
#endif /*  MAXSSIZ */

/*
 * Sizes of the system and user portions of the system page table.
 */
/* SYSPTSIZE IS SILLY; IT SHOULD BE COMPUTED AT BOOT TIME */
/* it now is computed at boot time in startup() machdep.c */
/* #define	SYSPTSIZE	((20+MAXUSERS)*NPTEPG) */

#ifndef LOCORE
extern int sysptsize;
#endif /*  LOCORE */

/* 
 * MAXUVA is max agg user address space in meg
 * it is used to configure usrptsize
 * 16 = (1 meg) / (# bytes per VAX page) / (# pte's per VAX page)
 */

#ifdef	MAXUVA
#define	USRPTSIZE	(MAXUVA*16)
#else
#if (PHYSMEM < 64)
#define	USRPTSIZE 	(32*NUMPTEPG)
#else
#define	USRPTSIZE 	(64*NUMPTEPG)
#endif
#endif

/*
 * The size of the clock loop.
 */
#define	LOOPPAGES	(maxfree - firstfree)

/*
 * The time for a process to be blocked before being very swappable.
 * This is a number of seconds which the system takes as being a non-trivial
 * amount of real time.  You probably shouldn't change this;
 * it is used in subtle ways (fractions and multiples of it are, that is, like
 * half of a ``long time'', almost a long time, etc.)
 * It is related to human patience and other factors which don't really
 * change over time.
 */
#define	MAXSLP 		20

/*
 * A swapped in process is given a small amount of core without being bothered
 * by the page replacement algorithm.  Basically this says that if you are
 * swapped in you deserve some resources.  We protect the last SAFERSS
 * pages against paging and will just swap you out rather than paging you.
 * Note that each process has at least UPAGES+CLSIZE pages which are not
 * paged anyways (this is currently 8+2=10 pages or 5k bytes), so this
 * number just means a swapped in process is given around 25k bytes.
 * Just for fun: current memory prices are 4600$ a megabyte on VAX (4/22/81),
 * so we loan each swapped in process memory worth 100$, or just admit
 * that we don't consider it worthwhile and swap it out to disk which costs
 * $30/mb or about $0.75.
 */
#define	SAFERSS		32		/* nominal ``small'' resident set size
					   protected against replacement */

/*
 * If the text + data areas of a process that is being exec'ed is 
 * smaller than this threshold, then it's entirely brought in rather
 * than demand paged in.  This size is in bytes.
 */
#define PGTHRESH	(64*1024)

/*
 * DISKRPM is used to estimate the number of paging i/o operations
 * which one can expect from a single disk controller.
 */
#define	DISKRPM		60

/*
 * Klustering constants.  Klustering is the gathering
 * of pages together for pagein/pageout, while clustering
 * is the treatment of hardware page size as though it were
 * larger than it really is.
 *
 * KLMAX gives maximum cluster size in CLSIZE page (cluster-page)
 * units.  Note that KLMAX*CLSIZE must be <= SWAPFRAG in dmap.h.
 */

#define	KLMAX	(32/CLSIZE)
#define	KLSEQL	(16/CLSIZE)		/* in klust if vadvise(VA_SEQL) */
#define	KLIN	(8/CLSIZE)		/* default data/stack in klust */
#define	KLTXT	(4/CLSIZE)		/* default text in klust */
#define	KLOUT	(32/CLSIZE)

/*
 * KLSDIST is the advance or retard of the fifo reclaim for sequential
 * processes data space.
 */
#define	KLSDIST	3		/* klusters advance/retard for seq. fifo */

/*
 * Paging thresholds (see vm_sched.c).
 * Strategy of 3/18/86 (assuming 2Mb min configuration)
 *	lotsfree is 512k bytes (was 1/4 memory)
 *	desfree is 200k bytes, but at most 1/8 of memory
 *	minfree is 64k bytes, but at most 1/2 of desfree
 */
#define	LOTSFREE	(512 * 1024)
#define	DESFREE		(200 * 1024)
#define	DESFREEFRACT	8
#define	MINFREE		(64 * 1024)
#define	MINFREEFRACT	2

/*
 * There are two clock hands, initially separated by HANDSPREAD bytes.
 * If the amount of user memory is less than HANDSPREAD bytes, then
 * the clock hands are initialized to the amount of user memory.
 * The amount of time to reclaim a page once the pageout process examines
 * it increases with this distance and decreases as the scan rate rises.
 */
#define HANDSPREAD	(2 * 1024 *1024)

/*
 * Threshold to force clearing of the "u" area list while memfree'ing 
 * flg == MF_UAREA pages.  The use of a threshold will insure that
 * the list doesn't get too large, but there doesn't seem to be a 
 * need to clear it each and every time either.
 */
#define UCLEAR		(UPAGES*4)

/*
 * Believed threshold (in megabytes) for which interleaved
 * swapping area is desirable.
 */
#define	LOTSOFMEM	2

/*
 * minimum and maximum kernel allocatable memory (in pages)
 */
#define MIN_DMEM_PAGES	4096
#define MAX_DMEM_PAGES  16384


/*
 * add VPSYNC to mapin() and newptes()
 */
/*
 * BEWARE THIS DEFINITION WORKS ONLY WITH COUNT OF 1
 */
#define	mapin(pte, v, pfnum, count, prot) \
	VPSYNC(); \
	(*(int *)(pte) = (pfnum) | (prot), mtpr(TBIS, ptob(v)))

#define newptes(pte, v, size) if (1) \
{register caddr_t a, limit; \
   VPSYNC(); \
   for (a=ptob(v),limit=a+(size)*NBPG; a<limit; a+=NBPG) mtpr(TBIS, a);} \
else

/*
 * This defines the default kernel memory allocator
 * bucket high water mark value.
 */

#define	KMBUCKET_HWM	(4)

/*
 * definitions to size the number of page table entries
 * for the kernel memory allocator
 */

#define LOW_WATER 3
#define HIGH_WATER 10

#if (PHYSMEM/10 < LOW_WATER)
#define KMEMUMAP LOW_WATER*2048
#else
#if (PHYSMEM/10 > HIGH_WATER)
#define KMEMUMAP HIGH_WATER*2048
#else
#define KMEMUMAP PHYSMEM/10*2048
#endif
#endif

#define	KMEMWMAP (KMEMUMAP/4)
#define	KMEMMAP	(KMEMUMAP+KMEMWMAP)
