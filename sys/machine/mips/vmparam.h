/*
 *	@(#)vmparam.h	4.2	(ULTRIX)	8/13/90
 */

/************************************************************************
 *									*
 *			Copyright (c) 1988 by				*
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
 * 10 Aug 90 -- jaa
 *	DFLSSIZ and DFLDSIZ are unused
 *	Change default max data seg size to 128 Mb
 *
 * 12-Jun-89 bp
 *	Added kernel memory allocator default high water mark.
 *
 * 12 Jun 89 -- ggopal
 * 	 Removed definition 	DMMIN , DMMAX and DMTEXT
 *
 * 27 Mar 89 -- jmartin
 *	Redefine HIGHPAGES and USRSTACK to make process virtual address
 *	space for forkutl.
 *
 * 10 Nov 88 -- chet
 *	Moved definition of SYSPTSIZE to machine/param.h.
 *
 *-----------------------------------------------------------------------
 */

/*
 * Machine dependent constants for MIPS
 */

/*
 * USRTEXT is the start of the user text, USRDATA is the start of the
 * user data, and USRSTACK is the top (end) of the user stack.  
 * LOWPAGES is the number of pages from the beginning of the text region
 * to the beginning of text.
 * HIGHPAGES is the number of pages from the beginning of the stack region
 * to the beginning of the stack.
 */

#define	LOWPAGES	0
#define	REDZONEPAGES	1
#define	HIGHPAGES	(REDZONEPAGES+FORKPAGES+REDZONEPAGES)
#define	USRTEXT		0x400000	/* user text starts at 4 MB */
#define USRDATA		0x10000000	/* user data starts at 256 MB */
#define	EA_SIZE		32		/* EMULATE_AREA size */
#define	USRSTACK	(0x80000000-HIGHPAGES*NBPG)/* Top of user stack */
#define EMULATE_AREA	USRSTACK-EA_SIZE/* area for bp emulation */

/*
 * Virtual memory related constants, all in bytes
 */
#ifndef MAXTSIZ
#define	MAXTSIZ		(24*1024*1024)		/* max text size */
#endif

#ifndef MAXDSIZ
#define	MAXDSIZ		(128*1024*1024)		/* max data size */
#endif

#ifndef	MAXSSIZ
#define	MAXSSIZ		(32*1024*1024)		/* max stack size */
#endif

/*
 * Size of the system and user page table.
 *
 * The declaration of SYSPTSIZE has been moved to machine/param.h.
 */
#define	USRPTSIZE 	(3*32*MAXUSERS+20*3) /* 8 procs/user + 20 daemons */

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
#define	SAFERSS		6		/* nominal ``small'' resident set size
					   protected against replacement */

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
 * units.  Note that KLMAX*CLSIZE must be <= dtoc(SWAPFRAG) in dmap.h.
 */

#define	KLMAX	(4/CLSIZE)
#define	KLSEQL	(2/CLSIZE)		/* in klust if vadvise(VA_SEQL) */
#define	KLIN	(1/CLSIZE)		/* default data/stack in klust */
#define	KLTXT	(1/CLSIZE)		/* default text in klust */
#define	KLOUT	(4/CLSIZE)

/*
 * KLSDIST is the advance or retard of the fifo reclaim for sequential
 * processes data space.
 */
#define	KLSDIST	2		/* klusters advance/retard for seq. fifo */

/*
 * Paging thresholds (see vm_sched.c).
 * Strategy of 4/22/81:
 *	lotsfree is 512k bytes, but at most 1/4 of memory
 *	desfree is 256K bytes, but at most 1/8 of memory
 *	minfree is 96K bytes, but at most 1/2 of desfree
 */
#define	LOTSFREE	(128 * 4096)
#define	LOTSFREEFRACT	4
#define	DESFREE		(64 * 4096)
#define	DESFREEFRACT	8
#define	MINFREE		(24 * 4096)
#define	MINFREEFRACT	2

/*
 * There are two clock hands, initially separated by HANDSPREAD bytes
 * (but at most all of user memory).  The amount of time to reclaim
 * a page once the pageout process examines it increases with this
 * distance and decreases as the scan rate rises.
 */
#define	HANDSPREAD	(2 * 1024 * 4096)

/*
 * The number of times per second to recompute the desired paging rate
 * and poke the pagedaemon.
 */
#define	RATETOSCHEDPAGING	4

/*
 * Pages per second to scan when out of memory (targeted toward ~10%
 * of cpu)
 */
#define	FASTSCAN	400

/*
 * Scan all of memory no more frequently than SCANSECS
 */
#define	SCANSECS	3

/*
 * Believed threshold (in megabytes) for which interleaved
 * swapping area is desirable.
 */
#define	LOTSOFMEM	2

/*
 * Paged text files that are less than PGTHRESH bytes may be swapped
 * in instead of paged in.
 */
#define PGTHRESH        (100 * 1024)
#define SLOP		20

/*
 * This defines the default kernel memory allocator
 * bucket high water mark value.
 */

#define	KMBUCKET_HWM	(1)

/*
 * KMEMSLOP is additional ptes required for get_sys_ptes calls
 * which allocates ptes from the non-wired map for double mapping
 * of memory and other things.
 */

#define	KMEMSLOP	(512)

#define LOW_WATER 3
#define HIGH_WATER 10

#if (PHYSMEM/10 < LOW_WATER)
#define KMEMUMAP (LOW_WATER*256+KMEMSLOP)
#else
#if (PHYSMEM/10 > HIGH_WATER)
#define KMEMUMAP (HIGH_WATER*256+KMEMSLOP)
#else
#define KMEMUMAP (PHYSMEM/10*256+KMEMSLOP)
#endif
#endif


#define	KMEMWMAP 	((KMEMUMAP-KMEMSLOP)/4)
#define	KMEMSIZE	(KMEMWMAP+KMEMUMAP)

