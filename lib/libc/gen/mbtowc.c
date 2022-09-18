/*	@(#)mbtowc.c	4.1 ULTRIX 7/3/90 */
/************************************************************************
 *									*
 *			Copyright (c) 1989 by				*
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
/*
 *
 * Routine for ANSI multi-byte/wide character conversions.
 * mbtowc - convert a multi-byte character to a wchar_t type.
 * ANSI Draft December 7, 1988, section 4.10.7.2
 *
 *                       Modification History
 *
 * 001  DECwest ANSI 4.10.7.2 djd djd025 1990 Jan 15
 *
 *              Created function.
 */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#define	LOAD_PWC(val)	if(pwc != NULL) *pwc = val;

#ifdef	__STDC__
int
mbtowc(pwc, s, n)
wchar_t		*pwc;
const	char	*s;
size_t		n;
#else
int
mbtowc(pwc, s, n)
wchar_t	*pwc;
char    *s;
size_t	n;
#endif
{
	wchar_t	a_wchar;
	int	octet_cnt;
	unsigned char	*cur_octet;
	extern	unsigned long	_pctype_siz;

	if(s == NULL)
		return(0);

	if(*s == '\0') {
		LOAD_PWC(0);
		return(0);
	}
	cur_octet = (unsigned char *)s;
	a_wchar = (wchar_t) 0;

	for(octet_cnt = 0;octet_cnt < n && octet_cnt < MB_CUR_MAX;octet_cnt++){

		if(*cur_octet == '\0') /* illegal char 	*/
			break;

		a_wchar = a_wchar<<8;
		a_wchar = a_wchar | (0xff & *cur_octet);

		if(a_wchar > _pctype_siz) /* are we off the ctype tbl, illegal*/
			break;

		if(((_pctype+1)[a_wchar]) != 0) {/*legal char return it	*/
			LOAD_PWC(a_wchar);
			return(octet_cnt+1);
		}
		cur_octet++;
	}
	/*
	 * scaned n (or at least MB_CUR_MAX) and could not construct
	 * a legal character so return error.
	 */
	return(-1);
}

