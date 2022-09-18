/*
 * @(#)param.h	4.3	(ULTRIX)	11/15/90
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

/* ------------------------------------------------------------------ */
/* | Copyright Unpublished, MIPS Computer Systems, Inc.  All Rights | */
/* | Reserved.  This software contains proprietary and confidential | */
/* | information of MIPS and its suppliers.  Use, disclosure or     | */
/* | reproduction is prohibited without the prior express written   | */
/* | consent of MIPS.                                               | */
/* ------------------------------------------------------------------ */

/*-----------------------------------------------------------------------
 *
 * Modification History
 *
 * 15-Nov-90	Randall Brown
 *	Changed MAXMEM_MB to 480 meg to support 3max.
 *
 * 06-Sep-90	Randall Brown
 *	Changed BASEPRI macro to use sr_usermask.
 *
 * 12-Dec-89	burns
 *	Removed SYSPTSIZE. Now caculated by genassym based upon
 *	physmem from the config file.
 *
 * 12-Sep-89	burns
 *	Bumped MAXMEM_MB to 256 for DECsystem 58xx series.
 *
 * 15-May-89	Kong
 *	Changed MAXMEM_MB from 24 to 64 to support Mipsfair.
 *
 * 19-Jan-89	Kong
 *	Changed delay to call microdelay in machdep.c
 *
 * 10 Nov 88 -- chet
 *	Added minimum and maximum supported memory constants.
 *
 *-----------------------------------------------------------------------
 */

/*
 * Machine dependent constants for mips.
 */

/*
 * Define the MINIMUN and MAXIMUM amounts of memory that Ultrix supports.
 * This information is used by config and startup code.
 * Give the values in both (4096 byte) pages and in megabytes.
 */
#define PGS_PER_MB	256
#define MINMEM_MB	6
#define MINMEM_PGS	(MINMEM_MB * PGS_PER_MB)
#define MAXMEM_MB	480
#define MAXMEM_PGS	(MAXMEM_MB * PGS_PER_MB)

#define	NBPG		4096		/* bytes/page */
#define	PGOFSET		(NBPG-1)	/* byte offset into page */
#define	PGSHIFT		12		/* LOG2(NBPG) */
#define	DBSHIFT		9		/* LOG2(Disk block size) */

#define	CLSIZE		1
#define	CLSIZELOG2	0

#define	SSIZE		1		/* initial stack size/NBPG */
#define	SINCR		1		/* increment of stack/NBPG */

#define	UPAGES		2		/* pages of u-area */
#define FORKPAGES	UPAGES		/* pages of forkutl */
#define	KERNELSTACK	0xffffe000	/* Top of kernel stack */
#define	UADDR		0xffffc000	/* address of u */
#define	UVPN		(UADDR>>PGSHIFT)/* virtual page number of u */
#ifndef UCLEAR
#define UCLEAR		4
#endif /*	UCLEAR */

/*
 * Some macros for units conversion
 */
/* Core clicks (4096 bytes) to segments and vice versa */
#define	ctos(x)	(x)
#define	stoc(x)	(x)

/* Core clicks (4096 bytes) to disk blocks */
#define	ctod(x)	((x)<<(PGSHIFT-DBSHIFT))
#define	dtoc(x)	((unsigned)(x)>>(PGSHIFT-DBSHIFT))
#define	dtob(x)	((x)<<DBSHIFT)

/* clicks to bytes */
#define	ctob(x)	((x)<<PGSHIFT)

/* bytes to clicks */
#define	btoc(x)	(((unsigned)(x)+PGOFSET)>>PGSHIFT)

/*
 * Macros to decode processor status word.
 */
#define	USERMODE(sr)	(((sr) & SR_KUP) == SR_KUP)
#ifndef LOCORE
extern unsigned int sr_usermask;
#endif 
#define	BASEPRI(sr)	(((sr) & SR_IMASK) == (sr_usermask & SR_IMASK))

/*
 * DELAY(n) should be n microseconds, roughly.  This is a first guess.
 */
#ifdef SABLE
#define	DELAY(n)	{ register int N = 3*(n); while (--N > 0); }
#else /* SABLE */
#define	DELAY(n) { microdelay(n);}
#endif /* SABLE */
