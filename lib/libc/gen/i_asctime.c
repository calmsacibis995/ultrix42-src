/************************************************************************
 *									*
 *			Copyright (c) 1987,1988,1989 by			*
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
static char Sccsid[] = "@(#)i_asctime.c	4.1	(ULTRIX)	7/3/90";
#endif

#include "i_defs.h"
#include "i_errno.h"
#include <time.h>

#define MAXBUF	256		/* length of static buffer for assembly	*/
 
/*
 * i_asctime -- convert time to string
 *
 * SYNOPSIS:
 *	char *
 *	i_asctime(tp, fmt, strtab)
 *	struct tm *tp;
 *	char *fmt;
 *	str_tab *strtab;
 *
 * DESCRIPTION:
 *	i_asctime is the frontend used by the nl_ctime functions. It provides
 *	a jacket and some buffer space for use by _asctime which does the
 *	real formatting.
 *
 * RETURN:
 *	pointer to converted time (ATTENTION: STATIC BUFFER AREA!), i_errno = 0
 *	pointer to national/english mix for failure with i_errno set.
 *
 * LIMITATIONS:
 *	As we can't tell success or failure from the return value, we set
 *	i_errno to zero for success.
 */


/*
 * default date format:
 */
#define D_T_FMT "D_T_FMT"

char *
i_asctime(tp, fmt, strtab)
struct tm *tp;
char *fmt;
str_tab *strtab;
{
	static char buf[MAXBUF];/* buffer to collect resulting string in */
	char *asctime();

	/*
	 * quick check for arg
	 */
	if (strtab == (str_tab *)0 && fmt == (char *)0)
	{
		i_errno = I_EISTI;
		return (asctime(tp));
	}

	/*
	 * assume no error
	 */
	i_errno = 0;

	if (fmt == (char *)0 && (fmt = i_getstr(D_T_FMT, strtab)) == (char *)0)
	{
		i_errno = I_EISTI;
		return(asctime(tp));
	}
	return (_asctime(buf, MAXBUF, fmt, tp, strtab) ? buf : asctime(tp));
}
