/************************************************************************
 *									*
 *		      Copyright (c) 1987,1988,1989 by			*
 *		Digital Equipment Corporation, Maynard, MA		*
 *		            Bull, France				*
 *			   Siemens AG, FR Germany			*
 *			All rights reserved.				*
 *									*
 *   This software is furnished under license and may be used and	*
 *   copied  only  in accordance with the terms of such license and	*
 *   with the  inclusion  of  the  above  copyright  notice.   This	*
 *   software  or  any  other copies thereof may not be provided or	*
 *   otherwise made available to any other person.  No title to and	*
 *   ownership of the software is hereby transferred.			*
 *									*
 *   The information in this software is subject to change  without	*
 *   notice  and should not be construed as a commitment by Digital	*
 *   Equipment Corporation.						*
 *									*
 *   Digital assumes no responsibility for the use  or  reliability	*
 *   of its software on equipment which is not supplied by Digital.	*
 *									*
 ************************************************************************/

/*	@(#)locale.h	4.1	(ULTRIX)	7/2/90	*/

/*
 *
 *   File name:		locale.h
 *
 *   Source file description:
 *	This file contains the declarations used by the ANSI setlocale()
 *	and localeconv() functions.
 *
 *   Modification history:
 *
 *      21-Feb-1990, Dan Smith
 *              Add structure definition for struct lconv. It was
 *              previously a typedef, but the standard requires the
 *              struct definition. Added missing member frac_digits to
 *              this structure. 
 *
 *	 3-Mar-1987, ARG.
 *		Created.
 *	31-May-1989, JLR.
 *		Added NULL, lconv for ANSI compliance.  Note that at
 *		this writing, localeconv does not actually exist.
 */

#define LC_ALL		0		/* set everything below		*/
#define LC_COLLATE	1		/* affects strcoll, strxfrm	*/
#define LC_CTYPE	2		/* affects is* macros		*/
#define LC_NUMERIC	3		/* affects radix character	*/
#define LC_TIME		4		/* affects strftime		*/
#define LC_MONETARY	5		/* affects monetary symbols	*/

#define _LC_MAX		5		/* highest LC_ category		*/

#define NULL 0

#ifndef _LCONV_
#define	_LCONV_
struct lconv {
	char	*decimal_point;
	char	*thousands_sep;
	char	*grouping;
	char	*int_curr_symbol;
	char	*currency_symbol;
	char	*mon_decimal_point;
	char	*mon_thousands_sep;
	char	*mon_grouping;
	char	*positive_sign;
	char	*negative_sign;
	char	int_frac_digits;
	char	frac_digits;
	char	p_cs_precedes;
	char	p_sep_by_space;
	char	n_cs_precedes;
	char	n_sep_by_space;
	char	p_sign_posn;
	char	n_sign_posn;
	} ;
#endif /* _LCONV_ */

#ifdef __STDC__
/*
 *  prototypes
 *
 */
struct lconv *	localeconv( void );	
extern	char *	setlocale( int __category, const char *__locale );

#else

struct lconv *	localeconv();		/* fetch current values	*/
extern char *	setlocale();		/* the locale call	*/

#endif /* __STDC__ */
