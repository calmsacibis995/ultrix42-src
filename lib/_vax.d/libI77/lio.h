/*
*	@(#)lio.h	1.2	(ULTRIX)	1/16/86
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
*	David Metsky		10-Jan-86
*
* 001	Replaced old version with BSD 4.3 version as part of upgrade.
*
*	Based on:	lio.h		5.2		7/30/85
*
*************************************************************************/

/*
 * copy of ftypes from the compiler
 * variable types numeric assumptions:
 *	int < reals < complexes
 *	TYDREAL-TYREAL = TYDCOMPLEX-TYCOMPLEX
 */

#define TYUNKNOWN 0
#define TYADDR 1
#define TYSHORT 2
#define TYLONG 3
#define TYREAL 4
#define TYDREAL 5
#define TYCOMPLEX 6
#define TYDCOMPLEX 7
#define TYLOGICAL 8
#define TYCHAR 9
#define TYSUBR 10
#define TYERROR 11

#define NTYPES (TYERROR+1)
 
#define	LINE	80
#define LINTW	(strlen(buf))
#define	LLOGW	3
#define LSTRW	(len+2)
#define	LLOW	1.0e-1
#define	LHIGH	1.0e+LFD
#define LDHIGH	1.0e+LDFD
#define	LFD	6
#define	LFW	(LFD+4)
#define LDFD	14
#define LDFW	(LDFD+4)
#define	LED	LFD
#define	LEW	LFW+4
#define	LEE	2
#define LDED	LDFD
#define LDEW	LDFW+4
#define LDEE	2
#define LCW	(width(a)+width(b)+5)
#define LDCW	(dwidth(a)+dwidth(b)+5)

#define abs(z)	(z<0?-z:z)
#define width(z) ((z!=0.0 && (abs(z)>=LHIGH || abs(z)<LLOW))?LEW:LFW)
#define dwidth(z) ((z!=0.0 && (abs(z)>=LDHIGH || abs(z)<LLOW))?LDEW:LDFW)
#define ERR(x)	if(n=(x)) err(n>0?errflag:endflag,n,"list io")
#define ERRCHK(x)	if(n=(x)) goto got_err;
#define chk_len(w) if(recpos+w > line_len) PUT('\n');

typedef union
{	short	flshort;
	ftnint	flint;
	float	flreal;
	double	fldouble;
} flex;

extern int (*lioproc)();
extern flag leof;
extern ioflag lquit,l_first;
extern int lcount,line_len;
