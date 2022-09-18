/************************************************************************
 *									*
 *			Copyright (c) 1987,1988 by			*
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

#ifndef lint
static char Sccsid[] = "@(#)strcoll.c	4.1	(ULTRIX)	7/3/90";
#endif

#include "i_defs.h"
#include "i_errno.h"

/*
 * strcoll -- international version of strcmp
 *
 * SYNOPSIS:
 *	int
 *	strcoll(s1, s2)
 *	char *s1;
 *	char *s2;
 *
 * DESCRIPTION:
 *	strcoll is specified in the X3J11 ANSI 'C' standard
 *
 * RETURNS:
 *	result of collation
 */
int
strcoll(s1, s2)
char *s1;
char *s2;
{
	if (_lc_cldflt)
		return (i_coll(s1, s2, _lc_cldflt));
	else {
		i_errno = I_EBACC; 
		/*
		 * we must us unsigned comparsion here to ensure collation is
		 * in the correct order
		 */
		return (_ustrcmp(s1, s2));
	}
}

/*
 * Compare strings:  s1>s2: >0  s1==s2: 0  s1<s2: <0
 *
 * This uses UNSIGNED characters to ensure that characters with the 
 * high bit set collate higher than characters with the high bit clear.
 */

_ustrcmp(s1, s2)
register unsigned char *s1, *s2;
{

	while (*s1 == *s2++)
		if (*s1++=='\0')
			return(0);
	return(*s1 - *--s2);
}
