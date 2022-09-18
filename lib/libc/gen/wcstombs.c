/*	@(#)wcstombs.c	4.1	ULTRIX	7/3/90 */
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
 * wcstombs - converts the array of wchar_t types to multi-byte
 * sequence.
 * ANSI Draft December 7, 1988, section 4.10.8.2
 *
 *                       Modification History
 *
 * 001  DECwest ANSI 4.10.8.2 djd djd025 1990 Jan 15
 *
 *              Created function.
 */

#include <stdlib.h>

#ifdef	__STDC__
size_t
wcstombs(s, pwcs, n)
char	*s;
const wchar_t	*pwcs;
size_t	n;
#else
size_t
wcstombs(s, pwcs, n)
char	*s;
wchar_t	*pwcs;
size_t	n;
#endif
{
	unsigned char	*cp, temp[MB_CUR_MAX];
	int	wc_idx, num_oct, rc;

	if(s == NULL)
		return(-1);

	num_oct = 0;
	cp = (unsigned char *)s;

	for(wc_idx = 0;pwcs[wc_idx] != (wchar_t)0 && num_oct < n;wc_idx++) {

		rc = wctomb((char *)temp, pwcs[wc_idx]);

		if(rc == -1)
			return(-1);

		if((num_oct + rc) > n) {
			*cp = '\0';
			return(num_oct);
		}

		memcpy(cp, temp, rc);
		num_oct = num_oct + rc;
		cp = cp + rc;
	}
	if(num_oct < n)
		*cp = '\0';
	
	return(num_oct);
}
