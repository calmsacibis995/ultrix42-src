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
static char Sccsid[] = "@(#)strftime.c	4.1	(ULTRIX)	7/3/90";
#endif

/*LINTLIBRARY*/
/*
 * strftime.c
 *
 *	ANSI international time function
 */

#include "i_defs.h"
#include "locale.h"
#include "i_errno.h"
#include <time.h>

/*
 * strftime -- ANSI international time function
 *
 * SYNOPSIS:
 *	size_t
 *	strftime(s, maxsize, format, timeptr);
 *	char *s;
 *	size_t maxsize;
 * 	const char *format;
 *	const struct tm *timeptr;
 *
 * DESCRIPTION:
 *	The ANSI strftime function.
 *
 * RETURNS:
 *	The length of the returned string or 0 on error.
 */

size_t
strftime(s, maxsize, format, timeptr)
char *s;
int maxsize;
char *format;
struct tm *timeptr;
{
	/*
	 * quick check for args
	 */

	if (format == (char *)0)
		return 0;

	return ((size_t)_asctime(s, maxsize, format, timeptr, _lc_strtab[LC_TIME]));
}
