#ifndef lint
static char *sccsid = "@(#)startdqs.c	4.1	ULTRIX	7/2/90";
#endif

/************************************************************************
 *                                                                      *
 *                      Copyright (c) 1988 by                           *
 *              Digital Equipment Corporation, Maynard, MA              *
 *                      All rights reserved.                            *
 *                                                                      *
 *   This software is furnished under a license and may be used and     *
 *   copied  only  in accordance with the terms of such license and     *
 *   with the  inclusion  of  the  above  copyright  notice.   This     *
 *   software  or  any  other copis thereof may not be provided or      *
 *   otherwise made available to any other person.  No title to and     *
 *   ownership of the software is hereby transferred.                   *
 *                                                                      *
 *   This software is  derived  from  software  received  from  the     *
 *   University    of   California,   Berkeley,   and   from   Bell     *
 *   Laboratories.  Use, duplication, or disclosure is  subject  to     *
 *   restrictions  under  license  agreements  with  University  of     *
 *   California and with AT&T.                                          *
 *                                                                      *
 *   The information in this software is subject to change  without     *
 *   notice  and should not be construed as a commitment by Digital     *
 *   Equipment Corporation.                                             *
 *                                                                      *
 *   Digital assumes no responsibility for the use  or  reliability     *
 *   of its software on equipment which is not supplied by Digital.     *
 *                                                                      *
 ************************************************************************/

/*
 * startdqs.c -- Cause new daemon to be spawned
 *
 * Description:
 *	Tell the printer daemon that there are new files in the
 *	in the DQS spool directory.
 *
 *  Added by BOB BURTON
 *  this routine is identical to 'startdaemon' which is called by 'lpr' to
 *  'punch' 'lpd' when there is a print job ready.  This routine performs
 *   same function on the DQS daemon.
 *  The only difference is line 65 where the UNIX domain socket uses DQSNAME,
 *  (defined '/dev/dqsport'), instead of SOCKETNAME.
 *
 */

#include "lp.h"

enum job_status_e
startdqs(printer,cxp,command_file)
	char *printer;
	register CXP cxp;
	register char *command_file;
{
	struct sockaddr_un sun;
	register int s, n;
	char buf[BUFSIZ];

	s = socket(AF_UNIX, SOCK_STREAM, 0);
	if (s < 0) {
		perr("socket");
		return(js_failed);
	}
	sun.sun_family = AF_UNIX;
	strcpy(sun.sun_path, DQSNAME);
	if (connect(s, &sun, strlen(sun.sun_path) + 2) < 0) {
		perr("connect");
		(void) close(s);
		return(js_failed);
	}
	(void) sprintf(buf, "\1%s\n", printer);
	n = strlen(buf);
	if (write(s, buf, n) != n) {
		perr("write");
		(void) close(s);
		return(js_failed);
	}
	if (read(s, buf, 1) == 1) {
		if (buf[0] == '\0') {		/* everything is OK */
			(void) close(s);
			log("startdqs: got ACK - return ok");
			return(js_ok);
		}
		log("stardqs: buf not 0");
		putchar(buf[0]);
	}
	while ((n = read(s, buf, sizeof(buf))) > 0)
		fwrite(buf, 1, n, stdout);
	(void) close(s);
	log("startdqs: ACK failed");
	return(js_failed);
}

static
perr(msg)
	char *msg;
{
	extern char *name;
	extern int sys_nerr;
	extern char *sys_errlist[];
	extern int errno;

	printf("%s: %s: ", name, msg);
	fputs(errno < sys_nerr ? sys_errlist[errno] : "Unknown error" , stdout);
	putchar('\n');
}
