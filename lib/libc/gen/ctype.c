#ifndef lint
static	char	*sccsid = "@(#)ctype.c	4.1	ULTRIX	7/3/90";
#endif lint

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
 *									*
 * 004  Dennis Doherty, Jan 15 1990					*
 *	Added new extern that is set to the size of the _pctype         *
 *	table. (_pctype_siz) This is used by the multi-byte to 		*
 *	Wide char functions.						*
 *									*
 * 003  Andy Gadsby,  27-Oct-1987					*
 *	Extended table to make each entry 2 bytes and to cater for	*
 *	8 bit characters. This is required for internationalization.	*
 *	To prevent old applications picking up the wrong table the name *
 *	has been modified by removing an underscore.			*
 *									*
 *	David L Ballenger, 11-Oct-1985					*
 * 002	Change definitions so that iscntrl(), ispunct(), and isgraph()	*
 *	behave as documented, and so that the array is the same size in *
 *	both the System V and ULTRIX environments.			*
 *									*
 *	David L Ballenger, 30-Mar-1985					*
 * 0001	Add defintions for System V compatibility.			*
 *									*
 ************************************************************************/

/* @(#)ctype_.c	4.2 (Berkeley) 7/8/83 */

#include	<ctype.h>

/* This array is used by the following macros: isalpha(), isupper(), 
 * islower(), isdigit(), isxdigit(), isalnum(), isspace(), ispunct(),
 * isprint(), isgraph(), iscntrl(), and provides defined results for
 * all integer values for which the macro isascii() is true (ie. 0-0177),
 * plus 0200-0377 for internationalization, and the non-ASCII value 
 * EOF (ie. -1).  All of the macros which use this array index off the 
 * base address + 1 to achieve this range, ie. _ctype__+1[c].
 */

unsigned short _ctype__[1 + 256] = {
	0,
	_C,	_C,	_C,	_C,	_C,	_C,	_C,	_C,
	_C,	_C|_S,	_C|_S,	_C|_S,	_C|_S,	_C|_S,	_C,	_C,
	_C,	_C,	_C,	_C,	_C,	_C,	_C,	_C,
	_C,	_C,	_C,	_C,	_C,	_C,	_C,	_C,
	_S|_B,	_P,	_P,	_P,	_P,	_P,	_P,	_P,
	_P,	_P,	_P,	_P,	_P,	_P,	_P,	_P,
	_N,	_N,	_N,	_N,	_N,	_N,	_N,	_N,
	_N,	_N,	_P,	_P,	_P,	_P,	_P,	_P,
	_P,	_U|_X,	_U|_X,	_U|_X,	_U|_X,	_U|_X,	_U|_X,	_U,
	_U,	_U,	_U,	_U,	_U,	_U,	_U,	_U,
	_U,	_U,	_U,	_U,	_U,	_U,	_U,	_U,
	_U,	_U,	_U,	_P,	_P,	_P,	_P,	_P,
	_P,	_L|_X,	_L|_X,	_L|_X,	_L|_X,	_L|_X,	_L|_X,	_L,
	_L,	_L,	_L,	_L,	_L,	_L,	_L,	_L,
	_L,	_L,	_L,	_L,	_L,	_L,	_L,	_L,
	_L,	_L,	_L,	_P,	_P,	_P,	_P,	_C,
	 0,	 0,	 0,	 0,	 0,	 0,	 0,	 0,
	 0,	 0,	 0,	 0,	 0,	 0,	 0,	 0,
	 0,	 0,	 0,	 0,	 0,	 0,	 0,	 0,
	 0,	 0,	 0,	 0,	 0,	 0,	 0,	 0,
	 0,	 0,	 0,	 0,	 0,	 0,	 0,	 0,
	 0,	 0,	 0,	 0,	 0,	 0,	 0,	 0,
	 0,	 0,	 0,	 0,	 0,	 0,	 0,	 0,
	 0,	 0,	 0,	 0,	 0,	 0,	 0,	 0,
	 0,	 0,	 0,	 0,	 0,	 0,	 0,	 0,
	 0,	 0,	 0,	 0,	 0,	 0,	 0,	 0,
	 0,	 0,	 0,	 0,	 0,	 0,	 0,	 0,
	 0,	 0,	 0,	 0,	 0,	 0,	 0,	 0,
	 0,	 0,	 0,	 0,	 0,	 0,	 0,	 0,
	 0,	 0,	 0,	 0,	 0,	 0,	 0,	 0,
	 0,	 0,	 0,	 0,	 0,	 0,	 0,	 0,
	 0,	 0,	 0,	 0,	 0,	 0,	 0,	 0
};

unsigned short *_pctype = _ctype__;	/* pointer to current table */

unsigned long _pctype_siz = 255;	/* max char code for this table */
