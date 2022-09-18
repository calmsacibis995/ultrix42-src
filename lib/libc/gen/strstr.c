/*	@(#)strstr.c	4.1	ULTRIX	7/3/90	*/
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
	Find string within another string.  Coded to satisfy ANSI
	standard.  This is a rather brute force method, but for many
	cases it beat the pants off a Boyer-Moore approach, largely
	because of the setup time for BM.  Amazing what a few register
	declarations did for this one, though.

	BM works better in an environment where the setup time doesn't
	have to happen every time through.  In fact, the original paper
	acknowledges this, also noting that "If the expected penetration
	_i_ at which the pattern is found is small, the preprocessing
	time is significant and one might therefore consider using the
	obvious intuitive algorithm." (Boyer & Moore, "A Fast String
	Searching Algorithm", CACM, 10/77 -- vol 20, #10, pp. 762-772.)
	Since this is probably the case, we use brute force here.

	--Jon Reeves, May 1989.

*/

#define NULL 0

char *
strstr(s, t)
	register char *s;
	char *t;
{
	if (*t == '\0')
		return(s);

	while ( *s != '\0') {
		register char *s2=s, *t2=t;

		while (*t2++ == *s2++) 
			if (*t2 == '\0')
				return (s);
		s++;
	}
	return ((char *)NULL);
}
