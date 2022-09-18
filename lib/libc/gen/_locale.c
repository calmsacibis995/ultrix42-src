#ifndef lint
static char     sccsid[] = "@(#)_locale.c	4.1	ULTRIX	7/3/90";
#endif /* lint */

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
 * 000	3-Mar-1987, Andy Gadsby						*
 *	File created.							*
 *									*
 ************************************************************************/
/*
 *
 *	This file contains the definition of global table pointers set
 *	by the ANSI setlocale() call. These are then used in subsequent
 *	calls to determine the action of various international functions.
 *
 */

#include <i_defs.h>
#include <locale.h>
					/* LC_COLLATE			*/
col_tab	  *_lc_cldflt  = (col_tab *)0;	/* pointer to collation table	*/

					/* LC_CTYPE			*/
prp_tab   *_lc_prdflt  = (prp_tab *)0;	/* pointer to property table	*/
cnv_tab	  *_lc_tolower = (cnv_tab *)0;	/* tolower table		*/
cnv_tab	  *_lc_toupper = (cnv_tab *)0;	/* toupper table		*/

					/* LC_NUMERIC group		*/
char	   _lc_thosep  = ',';		/* thousand separator		*/
char	   _lc_radix   = '.';		/* radix character		*/
char	   _lc_exl     = 'e';		/* exponent character, lower    */
char	   _lc_exu     = 'E';		/* exponent character, upper    */

					/* String table pointers for	*/
					/* for each category		*/
str_tab	  *_lc_strtab[_LC_MAX + 1] = { (str_tab *)0 };
