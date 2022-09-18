/*
*	@(#)machdefs.h	1.3	(ULTRIX)	1/15/86
*/

/************************************************************************
 *									*
 *			Copyright (c) 1983 by				*
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
*		David Metsky,	14-Jan-86
*
* 001	Replaced old version with BSD 4.3 version as part of upgrade
*
*	Based on:	machdefs.h
*
*************************************************************************/
/*
 * machdefs.h
 *
 * Machine definitions for f77 compiler, pass 1.  VAX version.
 *
 * University of Utah CS Dept modification history:
 *
 * $Header: machdefs.h,v 2.2 85/01/14 03:41:13 donn Exp $
 * $Log:	machdefs.h,v $
 * Revision 2.2  85/01/14  03:41:13  donn
 * Added changes to make the storage used for constants that are passed
 * as arguments to functions come out read-only.
 * 
 * Revision 2.1  84/07/19  12:55:11  donn
 * Changed comment header to UofU.
 * 
 * Revision 1.3  84/07/02  12:04:42  donn
 * I goofed when I added the original changes for SZFLOAT and SZDOUBLE.
 * Thanks to Raleigh Romine for pointing this out (sigh).
 * 
 * Revision 1.2  84/02/28  20:46:16  donn
 * Added definitions for SZFLOAT and SZDOUBLE, needed by Berkeley changes to
 * produce shorter offsets to variables in machine code.
 * 
 */

#ifndef TARGET
TARGET NOT DEFINED !!!
#endif
#if TARGET!=VAX
Target= TARGET OUT OF RANGE!!
#endif

#ifndef FAMILY
FAMILY NOT DEFINED!!!
#endif
#if FAMILY!=PCC && FAMILY!=DMR
Family = FAMILY OUT OF RANGE
#endif

#define SDB 1

#define TYLENG		TYLONG

#define TYINT		TYLONG
#define SZADDR		4
#define SZSHORT		2
#define SZINT		4
#define SZFLOAT		4
#define SZDOUBLE	8

#define SZLONG		4
#define SZLENG		SZLONG

#define ALIADDR SZADDR
#define ALISHORT SZSHORT
#define ALILONG 4
#define ALIDOUBLE 4
#define ALIINT	ALILONG
#define ALILENG	ALILONG

#define AUTOREG 13
#define ARGREG 12
#define CARGREG 12
#define ARGOFFSET 4
#define SAVESPACE 40


#define FUDGEOFFSET 1
#define BITSPERCHAR 8
#define XL 16

#define USETEXT	".text\t0"
#define USECONST	".text\t1"
#define USEBSS	".data\t0"
#define USEINIT	".data\t1"

#define BLANKCOMMON "_BLNK_"

#define LABELFMT "%s:\n"

#define MAXREGVAR 5
#define TYIREG TYLONG
#define MSKIREG  (M(TYSHORT)|M(TYLONG))

#define MAXINT	2147483647
#define MININT  ((-2147483647) - 1)

#define MAXBYTE	255
#define BYTESIZE	8

#define SMALLVAR(x) ((x) < 512)
