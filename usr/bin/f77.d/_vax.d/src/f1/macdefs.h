/*
* @(#)macdefs.h	4.1	(ULTRIX)	7/17/90
*/

/************************************************************************
 *									*
 *			Copyright (c) 1986 by				*
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

/************************************************************************
*
*			Modification History
*
*	David Metsky		14-Jan-86
*
* 001	Replaced old version with BSD 4.3 version as part of upgrade.
*
*	Based on:	macdefs.h	4.2		85/08/23
*
*************************************************************************/

#ifndef _MACDEFS_
#define	_MACDEFS_

#define makecc(val,i)	lastcon = (lastcon<<8)|((val<<24)>>24);  

#define ARGINIT		32 
#define AUTOINIT	0 

/*
 * Storage space requirements
 */
#define SZCHAR		8
#define SZINT		32
#define SZFLOAT		32
#define SZDOUBLE	64
#define SZLONG		32
#define SZSHORT		16
#define SZPOINT		32

/*
 * Alignment constraints
 */
#define ALCHAR		8
#define ALINT		32
#define ALFLOAT		32
#define ALDOUBLE	32
#define ALLONG		32
#define ALSHORT		16
#define ALPOINT		32
#define ALSTRUCT	8
#define ALSTACK		32 

typedef	long	CONSZ;		/* size in which constants are converted */
typedef	long	OFFSZ;		/* size in which offsets are kept */

#define CONFMT	"%ld"		/* format for printing constants */
#define LABFMT	"L%d"		/* format for printing labels */

#define CCTRANS(x) x		/* character set macro */

/*
 * Register cookies for stack pointer and argument pointer
 */
#define STKREG	13		/* stack pointer */
#define ARGREG	12		/* off frame pointer */

/*
 * Maximum and minimum register variables
 */
#define MINRVAR	6		/* use R6 thru ... */
#define MAXRVAR	11		/* ... R11 */

#define BACKAUTO		/* stack grows negatively for automatics */
#define BACKTEMP		/* stack grows negatively for temporaries */
#define FIELDOPS		/* show field hardware support on VAX */
#define RTOLBYTES		/* bytes are numbered from right to left */

#define ENUMSIZE(high,low) INT	/* enums are always stored in full int */

#define ADDROREG
#define FIXDEF(p) outstab(p)
#define FIXARG(p) fixarg(p)
#ifndef ncopy
#define	ncopy(q, p)	((q)->in = (p)->in)
#endif
#endif
