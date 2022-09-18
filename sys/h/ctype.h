/*
 *		@(#)ctype.h	4.1	(ULTRIX)	7/2/90
 *		ctype.h	4.1	83/05/03
 */

/************************************************************************
 *									*
 *			Copyright (c) 1985, 1988 by			*
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
 *			Modification History				*
 *									*
 *	Jon Reeves, 07-Dec-1989						*
 * 006	Namespace protection.						*
 *	Jon Reeves, 10-Jul-1989						*
 * 005	Added toupper, tolower declarations for X/Open XPG3		*
 *      Andy Gadsby, 28-Mar-1988					*
 * 004  Modified isspace so that it is now modified according to the    *
 *	LC_CTYPE locale. This is to conform with X/Open XPG-3		*
 *	Andy Gadsby, 27-Oct-1987					*
 * 003  Changed the declaration for _ctype__ to be unsigned short	*
 *	and changed its name so that any explicit references will       *
 *	produce a compilation error. Added pointer to table to add in   *
 *      the ANSI setlocale() support.					*
 *	David L Ballenger, 11-Oct-1985					*
 * 002	Remove the defintion of the toupper() and tolower() macros so	*
 *	that the toupper() and tolower() routines will be used as	*
 *	documented.  The _toupper() and _tolower() macros are still	*
 *	provided.  Also add _B so that isgraph() and ispunct() can	*
 *	exclude space (ie. blank) and isprint() can include it.  	*
 *									*
 *	David L Ballenger, 28-Mar-1985					*
 * 0001 Add definitions for System V compatibility			*
 *									*
 ************************************************************************/

#ifndef _CTYPE_H_
#define _CTYPE_H_


#define	_U	0001	/* Upper case */
#define	_L	0002	/* Lower case */
#define	_N	0004	/* Numeral (digit) */
#define	_S	0010	/* Spacing character */
#define _P	0020	/* Punctuation */
#define _C	0040	/* Control character */
#define _X	0100	/* Hexadecimal */
#define _B	0200	/* Blank */

extern	unsigned short *_pctype;
extern  unsigned short _ctype__[];

#ifdef __STDC__
/*
 *  prototype
 *
 */
int	isalnum( int __c );
int	isalpha( int __c );
int	isascii( int __c );
int	iscntrl( int __c );
int	isdigit( int __c );
int	isgraph( int __c );
int	islower( int __c );
int	isprint( int __c );
int	ispunct( int __c );
int	isspace( int __c );
int	isupper( int __c );
int	isxdigit( int __c );
int	toascii( int __c );
int	_tolower( int __c );
int	_toupper( int __c );
int	tolower( int __c );
int	toupper( int __c );
#else
extern	int	tolower(), toupper();
#endif /* __STDC__ */

#define	isalpha(c)	((_pctype+1)[c]&(_U|_L))
#define	isupper(c)	((_pctype+1)[c]&_U)
#define	islower(c)	((_pctype+1)[c]&_L)
#define	isdigit(c)	((_ctype__+1)[c]&_N)
#define	isxdigit(c)	((_ctype__+1)[c]&(_N|_X))
#define	isspace(c)	((_pctype+1)[c]&_S)
#define ispunct(c)	((_pctype+1)[c]&_P)
#define isalnum(c)	((_pctype+1)[c]&(_U|_L|_N))
#define isprint(c)	((_pctype+1)[c]&(_P|_U|_L|_N|_B))
#define isgraph(c)	((_pctype+1)[c]&(_P|_U|_L|_N))
#define iscntrl(c)	((_pctype+1)[c]&_C)
#define isascii(c)	((unsigned)(c)<=0177)
#if !defined(_POSIX_SOURCE) || defined(_XOPEN_SOURCE)
#define _toupper(c)	((c)-'a'+'A')
#define _tolower(c)	((c)-'A'+'a')
#define toascii(c)	((c)&0177)
#endif

#endif /* _CTYPE_H_ */
