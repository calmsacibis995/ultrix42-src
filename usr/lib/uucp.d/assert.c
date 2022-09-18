#ifndef lint
static char sccsid[] = "@(#)assert.c	4.1 (decvax!larry) 7/2/90";
#endif

#include "uucp.h"
#include <sys/time.h>
#include <sys/types.h>
#include <errno.h>

/*******
 *	assert - print out assetion error
 *	       - prints error message to screen if debugging turned on
 *	       - places error message in ERRLOG if debugging turned off
 *
 *	return code - none
 */

/************************************************************************
 *									*
 *			Copyright (c) 1984 by				*
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


assert(s1, s2, i1)
char *s1, *s2;
{
	FILE *errlog;
	struct tm *tp;
	extern struct tm *localtime();
	extern time_t time();
	time_t clock;
	int pid;

	if (Debug)
		errlog = stderr;
	else {
		int savemask;
		savemask = umask(LOGMASK);
		errlog = fopen(ERRLOG, "a");
		umask(savemask);
	}
	if (errlog == NULL)
		return;

	pid = getpid();
	fprintf(errlog, "ASSERT ERROR (%.9s)  ", Progname);
	fprintf(errlog, "pid: %d  ", pid);
	time(&clock);
	tp = localtime(&clock);
	fprintf(errlog, "(%d/%d-%d:%02d) ", tp->tm_mon + 1,
		tp->tm_mday, tp->tm_hour, tp->tm_min);
	fprintf(errlog, "%s %s (%d)\n", s1, s2, i1);
	if (!Debug)
		fclose(errlog);
	return;
}
