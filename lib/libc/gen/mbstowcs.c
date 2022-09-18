/*	@(#)mbstowcs.c	4.1	ULTRIX	7/3/90 */
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
 * mbstowcs - convert a sequence of multi-byte characters to
 * an arrary of wchar_t types.
 * ANSI Draft December 7, 1988, section 4.10.8.1
 *
 *                       Modification History
 *
 * 001  DECwest ANSI 4.10.8.1 djd djd025 1990 Jan 15
 *
 *              Created function.
 */

#include <stdio.h>
#include <stdlib.h>

#ifdef	__STDC__
size_t
mbstowcs(pwcs, s, n)
wchar_t		*pwcs;
const	char	*s;
size_t		n;
#else
size_t
mbstowcs(pwcs, s, n)
wchar_t	*pwcs;
char	*s;
size_t	n;
#endif
{
	char	*cp;
	int	wc_idx, rc;

	cp = (char *)s;

	for(wc_idx = 0;wc_idx < n;wc_idx++) {

		if(*cp == '\0') {
			pwcs[wc_idx] = (wchar_t)0;
			return(wc_idx);
		}

		rc = mbtowc(&pwcs[wc_idx], cp, MB_CUR_MAX);

		if(rc == -1)
			return(-1);

		if(rc == 0) {
			pwcs[wc_idx] = (wchar_t)0;
			return(wc_idx);
		}

		cp = cp + rc;
	}
	return(n);
}

