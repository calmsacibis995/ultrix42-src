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
static char Sccsid[] = "@(#)nl_string.c	4.1	(ULTRIX)	7/2/90";
#endif

#include "i_defs.h"
#include "i_errno.h"

/*
 * nl_strcmp -- international version of strcmp
 *
 * SYNOPSIS:
 *	int
 *	nl_strcmp(s1, s2)
 *	char *s1;
 *	char *s2;
 *
 * DESCRIPTION:
 *	nl_strcmp is specified in the X/OPEN guide.
 *
 * RETURNS:
 *	result of collation
 */
int
nl_strcmp(s1, s2)
char *s1;
char *s2;
{
	if (_lc_cldflt)
		return (i_coll(s1, s2, _lc_cldflt));
	else {
		i_errno = I_EBACC; 
		return (strcmp(s1, s2));
	}
}

/*
 * nl_strncmp -- international version of strncmp
 *
 * SYNOPSIS:
 *	int
 *	nl_strncmp(s1, s2, n)
 *	char *s1;
 *	char *s2;
 *	int n;
 *
 * DESCRIPTION:
 *	nl_strncmp is specified in the X/OPEN guide.
 *
 * RETURNS:
 *	result of collation
 */
int
nl_strncmp(s1, s2, n)
char *s1;
char *s2;
int n;
{
	if (_lc_cldflt)
		return (i_ncoll(s1, s2, n, _lc_cldflt));
	else {
		i_errno = I_EBACC; 
		return (strncmp(s1, s2, n));
	}
}
