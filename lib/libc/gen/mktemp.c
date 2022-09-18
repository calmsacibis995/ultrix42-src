#ifndef lint
static	char	*sccsid = "@(#)mktemp.c	4.1	(ULTRIX)	7/3/90";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1985, 1987 by			*
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
 *			Modification History
 *
 *	David L Ballenger, 29-May-1985
 * 001	Add conditional code for System V emulation.
 *
 * 	Mark A Parenti, 09-Oct-1987
 * 002	Fix so will generate name with letter before access check.
 *
 *	Jon Reeves, 1989-Sep-25
 * 003	Add code from System V to not stomp terminal null if no X's
 *	(needed due to 002 change).
 *
 ************************************************************************/

#include <stdio.h>

char *
mktemp(as)
char *as;
{
	register char *s;
	register unsigned pid;
	register i;

	pid = getpid();
	s = as;
	while (*s++)
		;
	s--;
	while (*--s == 'X') {
		*s = (pid%10) + '0';
		pid /= 10;
	}
	i = 'a';
	if (*++s) {		/* May not have been any 'X's  003 */
		*s = i++;	/* 002 */
		while (access(as, 0) != -1) {
			if (i=='z')
#ifndef SYSTEM_FIVE
				return("/");
#else	SYSTEM_FIVE
				return(NULL);
#endif	SYSTEM_FIVE
			*s = i++;
		}
	} else {	/* 003 */
		if (access(as, 0) != -1)
#ifndef SYSTEM_FIVE
			return("/");
#else	SYSTEM_FIVE
			return(NULL);
#endif	SYSTEM_FIVE
	}
	return(as);
}
