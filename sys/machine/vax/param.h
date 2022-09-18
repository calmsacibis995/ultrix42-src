/*
 * 	@(#)param.h	4.1	(ULTRIX)	7/2/90";
 */

/************************************************************************
 *									*
 *			Copyright (c) 1983,86 by			*
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
 * 09-Nov-89 -- bp
 *	Bump vax physical memory up to almost 512 Mb.
 *
 * 25-Jul-89 -- tresvik
 *	Bumped min memory from 4 Meg to 5 Meg
 *
 *  6 Mar 89 -- jmartin
 *	#define FORKPAGES
 *
 * 19 Jul 88
 *	ifdef'd CLSIZE, NISP, UPAGES
 *
 * 13-Jun-88 -- chet
 *	Added MINMEM_MB, MINMEM_PGS, MAXMEM_MB, MAXMEM_PGS defines.
 *	These are the minimum (maximum) amounts of memory supported
 *	by this version of ULTRIX.
 *
 * 12-Feb-86 -- jrs
 *	Changed BASEPRI defn to handle new idle loop
 *
 *	Derived from 4.2 BSD labelled:
 *		param.h	6.1	83/07/29
 *
 *-----------------------------------------------------------------------
 */

/*
 * Define the MINIMUN and MAXIMUM amounts of memory that Ultrix supports.
 * This information is used by config, genassym, and startup().
 * Give the values in both (512 byte) pages and in megabytes.
 */
#define MINMEM_MB	5
#define MINMEM_PGS	(2048*MINMEM_MB)
#define MAXMEM_MB	512
#define MAXMEM_PGS	((2048*MAXMEM_MB) - 1024)

/*
 * Machine dependent constants for vax.
 */
#define	NBPG	512		/* bytes/page */
#define	PGOFSET	(NBPG-1)	/* byte offset into page */
#define	PGSHIFT	9		/* LOG2(NBPG) */

#ifndef	CLSIZE
#define	CLSIZE		2
#endif /*	CLSIZE */

#define	CLSIZELOG2	1

#define	SSIZE	4		/* initial stack size/NBPG */
#define	SINCR	4		/* increment of stack/NBPG */

#ifndef	UPAGES
#define	UPAGES	14		/* pages of u-area */
#endif /*	UPAGES */

#ifndef	FORKPAGES
#define	FORKPAGES	UPAGES	/* pages for window on child process */
#endif /*	FORKPAGES */

#ifndef	NISP
#define	NISP	9		/* pages of interrupt stack */
#endif /*	NISP */

/*
 * Some macros for units conversion
 */
/* Core clicks (512 bytes) to segments and vice versa */
#define	ctos(x)	(x)
#define	stoc(x)	(x)

/* Core clicks (512 bytes) to disk blocks */
#define	ctod(x)	(x)
#define	dtoc(x)	(x)
#define	dtob(x)	((x)<<9)

/* clicks to bytes */
#define	ctob(x)	((x)<<9)

/* bytes to clicks */
#define	btoc(x)	((((unsigned)(x)+511)>>9))

/*
 * Macros to decode processor status word.
 */
#define	USERMODE(ps)	(((ps) & PSL_CURMOD) == PSL_CURMOD)
#define	BASEPRI(ps)	(((ps) & PSL_IPL) <= PSL_IPL_LOW)

#define DELAY(n)	{ microdelay(n); }
