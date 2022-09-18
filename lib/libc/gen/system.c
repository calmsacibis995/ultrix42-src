#ifndef lint
static	char	*sccsid = "@(#)system.c	4.1	(ULTRIX)	7/3/90";
#endif lint
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
/*
 * Modification history
 *
 * 10/3/89 - D. Long
 *	Added check for NULL argument for ANSI compliance.
 */

/* @(#)system.c	4.1 (Berkeley) 12/21/80 */

#include	<signal.h>
#include <unistd.h>

system(s)
char *s;
{
	int status, pid, w;
	void (*istat)(), (*qstat)();
	static char *shell = "/bin/sh";

	if(!s)
		if(access(shell, X_OK) == 0)
			return 1;
		else
			return 0;
/* else */
	if ((pid = vfork()) == 0) {
		execl(shell, "sh", "-c", s, 0);
		_exit(127);
	}
	istat = signal(SIGINT, SIG_IGN);
	qstat = signal(SIGQUIT, SIG_IGN);
	while ((w = wait(&status)) != pid && w != -1)
		;
	if (w == -1)
		status = -1;
	signal(SIGINT, istat);
	signal(SIGQUIT, qstat);
	return(status);
}
