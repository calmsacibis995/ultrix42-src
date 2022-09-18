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
static char Sccsid[] = "@(#)nl_ctime.c	4.1	(ULTRIX)	7/2/90";
#endif

/*LINTLIBRARY*/
/*
 * nl_ctime.c
 *
 *	NLS functions for time
 */

#include "i_defs.h"
#include "locale.h"
#include "i_errno.h"
#include <time.h>

/*
 * declaration of externals
 */
char *asctime();
char *ctime();
struct tm *localtime();

/*
 * default date format:
 */
#define D_T_FMT "D_T_FMT"

/*
 * nl_cxtime -- international version of ctime
 *
 * SYNOPSIS:
 *	char *
 *	nl_cxtime(clock, fmt)
 *	long *clock;
 *	char *fmt;
 *
 * DESCRIPTION:
 *	nl_cxtime is specified in the X/OPEN guide.
 *
 * RETURNS:
 *	time as string
 */
char *
nl_cxtime(clock, fmt)
long *clock;
char *fmt;
{
	if (*fmt == '\0')
		fmt = (char *)0;
	return (i_asctime(localtime(clock), fmt, _lc_strtab[LC_TIME]));
}

/*
 * nl_ascxtime -- international version of asctime
 *
 * SYNOPSIS:
 *	char *
 *	nl_ascxtime(tp, fmt)
 *	struct tm *tp;
 *	char *fmt;
 *
 * DESCRIPTION:
 *	nl_ascxtime is specified in the X/OPEN guide.
 *
 * RETURNS:
 *	time converted to string.
 */
char *
nl_ascxtime(tp, fmt)
struct tm *tp;
char *fmt;
{
	if (*fmt == '\0')
		fmt = (char *)0;
	return (i_asctime(tp, fmt, _lc_strtab[LC_TIME]));
}
