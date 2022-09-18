/*	@(#)wctomb.c	4.1	ULTRIX	7/3/90 */
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
 * wctomb - converts a wchar_t type to a multi-byte character.
 * ANSI Draft December 7, 1988, section 4.10.7.3
 *
 *                       Modification History
 *
 * 001  DECwest ANSI 4.10.7.3 djd djd025 1990 Jan 15
 *
 *              Created function.
 */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>


int
wctomb(s, wchar)
char	*s;
wchar_t	wchar;
{
	extern  unsigned long   _pctype_siz;
	unsigned  char	octets[sizeof(wchar_t)], *cp;
	int	idx, num_oct;
	wchar_t	a_wchar;

	if(s == NULL)
		return(0);

	if(_pctype_siz < wchar)
		return(-1);

	if(((_pctype+1)[wchar]) == 0) 
		return(-1);

	if(wchar == (wchar_t)0) {	/* null wchar translates to null char*/
		*s = '\0';
		return(1);
	}

	cp = (unsigned char *)s;
	a_wchar = wchar;

	for(idx = sizeof(wchar_t)-1;idx >= 0;idx--) {

		octets[idx] = (0xff & a_wchar);
		a_wchar = a_wchar >> 8;
	}

	num_oct =0;
	
	for(idx = 0;idx < sizeof(wchar_t);idx++) {
	
		if(octets[idx] != '\0') {
			*cp = octets[idx];
			num_oct++;
			cp++;
		}
	}
	return(num_oct);
}

