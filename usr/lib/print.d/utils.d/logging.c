#ifndef lint
static char *sccsid = "@(#)logging.c	4.1      ULTRIX 7/2/90";
#endif

/************************************************************************
 *									*
 *			Copyright (c) 1988 by				*
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

#include "lp.h"

int DB;				/* debugging level */

static void
log_args(msg, args)
     char *msg;
     va_list args;
{
	short console = isatty(fileno(stderr));

	fprintf(stderr, console ? "\r\n%s: " : "%s: ", name);
	if (printer)
		fprintf(stderr, "%s: ", printer);

	vfprintf(stderr, msg, args);

	if (console)
		putc('\r', stderr);
	putc('\n', stderr);
	fflush(stderr);
}

/*VARARGS1*/
log(msg, va_alist)
     char *msg;
     va_dcl
{
	va_list args;

	va_start(args);
	log_args(msg, args);
	va_end(args);
}

/*VARARGS2*/
dlog(level, msg, va_alist)
     int level;
     char *msg;
     va_dcl
{
	va_list args;

	if (DB > level) {
		va_start(args);
		log_args(msg, args);
		va_end(args);
	}
}

/*VARARGS1*/
logerr(msg, va_alist)
     char *msg;
     va_dcl
{
	va_list args;
	register int err = errno;
	short console = isatty(fileno(stderr));
	extern int sys_nerr;
	extern char *sys_errlist[];

	va_start(args);
	if (msg) {
		log_args(msg, args);
	} else {
		log_args("", args);
	}
	    
	fputs(err < sys_nerr ? sys_errlist[err] : "Unknown error" , stderr);
	if (console)
		putc('\r', stderr);
	putc('\n', stderr);
	fflush(stderr);
}
