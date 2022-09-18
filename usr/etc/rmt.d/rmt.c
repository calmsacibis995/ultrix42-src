# ifndef lint
static char *sccsid = "@(#)rmt.c	4.1	(ULTRIX)	7/2/90";
# endif not lint

/************************************************************************
 *									*
 *			Copyright (c) 1985 by				*
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
 *			Modification History				*
 *									*
 *	Jeffrey M. Fries,   3-Nov-1986					*
 *      Added SO_KEEPALIVE to force process abortion on network connect *
 *      loss.								*
 *									*
 *	Jeffrey M. Fries,   7-Feb-1986					*
 *      Added O_CREAT to open to create files that do not exist         *
 *									*
 *	Jeffrey M. Fries,   7-Feb-1986					*
 *      Add comments to clarify code.                                   *
 *									*
 *	David L Ballenger, 22-Apr-1985					*
 * 0001	Rename local gets() routine to getstr() to avoid conflicts	*
 *	with the gets() from <stdio.h>.					*
 *									*
 *									*
 ************************************************************************/
/********************************************************
 *                Additional Information		*
 *  This code is started up after the 'rdump' code      *
 *  requests the remote shell to start up /etc/rmt      *
 ********************************************************/
/*
 * rmt
 */
#include <stdio.h>
#include <sgtty.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/devio.h>
#include <sys/mtio.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <sys/fs.h>
#include <sys/socket.h>
#include <syslog.h>
#include <errno.h>

#define	MAXRECSIZ	(10*1024)	/* small enuf for pdp-11's too */
#define	RSIZE	64

int	tape = -1;

char	record[MAXRECSIZ];
char	device[RSIZE];
char	count[RSIZE], mode[RSIZE], pos[RSIZE], op[RSIZE];

extern	errno;
extern	char *sys_errlist[];
char	resp[BUFSIZ];

long	lseek();


FILE	*debug;

/* Main Code Entry Point */
main(argc, argv)
	int argc;
	char **argv;
{
	int rval;
	char c;
	int n, i, cc;
	int on = 1;

	/* Set Socket keep alive		    */
	/* This kills the process should the socket */
	/* get uncleanly shut down...               */
	if(setsockopt(0, SOL_SOCKET, SO_KEEPALIVE, &on, sizeof(on)) < 0){
		openlog(argv[0], LOG_PID);
		syslog(LOG_WARNING, "setsockopt (SO_KEEPALIVE): %m");
		closelog();
	}

	argc--, argv++;
	if (argc > 0) {
		debug = fopen(*argv, "w");
		if (debug == 0)
			exit(1);
		(void) setbuf(debug, (char *)0);
	}
/* Top of Command Wait Loop */
do{
	errno = 0;
	rval = 0;

	/* Sit here waiting for the next command */
	if (read(0, &c, 1) != 1)
		exit(0);

	/* Handle Command */
	switch (c) {

	/* Close Remote System Dump Device */
	case 'C':
if (debug) fprintf(debug, "rmtd: C\n");
		getstr(device);		/* discard */
		if (close(tape) < 0){
		   error(errno);
		   break;
		}
		tape = -1;
		respond(rval);
		break;

	/* Get generic tape information of Remote  */
	/* device using DEVIOCGET "ioctl" call     */
	case 'D':
if (debug) fprintf(debug, "rmtd: D\n");
		{ struct devget devinf;
		getstr(device);		/* discard */
		if (ioctl(tape, DEVIOCGET, (char *)&devinf) < 0){
		   error(errno);
		   break;
		}
		rval = sizeof (devinf);
		respond(rval);
		(void) write(1, (char *)&devinf, sizeof (devinf));
		break;
		}

	/* Perform an 'ioctl' on the Remote System dump device */
	case 'I':
		getstr(op); getstr(count);
if (debug) fprintf(debug, "rmtd: I %s %s\n", op, count);
		{ struct mtop mtop;
		mtop.mt_op = atoi(op);
		mtop.mt_count = atoi(count);
		if (ioctl(tape, MTIOCTOP, (char *)&mtop) < 0){
		   error(errno);
		   break;
		}
		rval = mtop.mt_count;
		respond(rval);
		break;
		}

	/* Perform an 'lseek' on Remote System Dump Drive */
	case 'L':
		getstr(count); getstr(pos);
if (debug) fprintf(debug, "rmtd: L %s %s\n", count, pos);
		rval = lseek(tape, (long) atoi(count), atoi(pos));
		if (rval < 0){
	           error(errno);
		   break;
		}	
		respond(rval);
		break;

	/* Open Remote System Dump Device */
	case 'O':
		if (tape >= 0)
			(void) close(tape);

		getstr(device); getstr(mode);
if (debug) fprintf(debug, "rmtd: O %s %s\n", device, mode);
		tape = open(device, atoi(mode)|O_CREAT,0600);
		if (tape < 0){
			error(errno);
			break;
		}
		respond(rval);
		break;

	/* Get disk partition information of Remote  */
	/* disk device using DIOCDGTPT "ioctl" call  */
	case 'P':		/* partition info */
if (debug) fprintf(debug, "rmtd: P\n");
		{ struct pt pt;
		getstr(device);		/* discard */
		if (ioctl(tape, DIOCDGTPT, (char *)&pt) < 0){
		   error(errno);
		   break;
		}
		rval = sizeof (pt);
		respond(rval);
		(void) write(1, (char *)&pt, sizeof (pt));
		break;
		}

	/* Perform a Read from the Remote System Dump Device */
	case 'R':
		getstr(count);
if (debug) fprintf(debug, "rmtd: R %s\n", count);
		n = atoi(count);
		if (n > sizeof (record))
			n = sizeof (record);
		rval = read(tape, record, n);
		if (rval < 0){
		   error(errno);
		   break;
		}	
		respond(rval);
		(void) write(1, record, rval);
		break;

	/* Get status from the Remote System dump device */
	case 'S':		/* status */
if (debug) fprintf(debug, "rmtd: S\n");
		{ struct mtget mtget;
		if (ioctl(tape, MTIOCGET, (char *)&mtget) < 0){
	           error(errno);
		   break;
		}	
		rval = sizeof (mtget);
		respond(rval);
		(void) write(1, (char *)&mtget, sizeof (mtget));
		break;
		}

	/* Perform "stat" on a Remote System file    */ 
	case 'T':		/* status */
if (debug) fprintf(debug, "rmtd: T\n");
		{ struct stat stat_buf;
		  getstr(device);		/* get name of file */

		  if (stat(device, (char *)&stat_buf) < 0){
			error(errno);
			break;
		  }
		  rval = sizeof (stat_buf);
		  respond(rval);
		  (void) write(1, (char *)&stat_buf, sizeof (stat_buf));
		  break;
		}

	/* Perform a Write to the Remote System Dump Device */
	case 'W':
		getstr(count);
		n = atoi(count);
if (debug) fprintf(debug, "rmtd: W %s\n", count);
		for (i = 0; i < n; i += cc) {
			cc = read(0, &record[i], n - i);
			if (cc <= 0) {
if (debug) fprintf(debug, "rmtd: premature eof\n");
				exit(1);
			}
		}
		rval = write(tape, record, n);
		if (rval < 0){
		   error(errno);
		   break;
		}	
		respond(rval);
		break;

	/* Error command request */
	default:
if (debug) fprintf(debug, "rmtd: garbage command %c\n", c);
		exit(1);
	}
}while(1);

}

/* Send response based on requested command */
respond(rval)
  int rval;
{
if (debug) fprintf(debug, "rmtd: A %d\n", rval);
	(void) sprintf(resp, "A%d\n", rval);
	(void) write(1, resp, strlen(resp));
	return;
}

/* Extract parts of a command using '\n' as a delimiter */
getstr(bp)
	char *bp;
{
	int i;
	char *cp = bp;

	for (i = 0; i < RSIZE; i++) {
		if (read(0, cp+i, 1) != 1)
			exit(0);
		if (cp[i] == '\n')
			break;
	}
	cp[i] = '\0';
}

/* Performed when an Error occurs on I/O Operation */
error(num)
	int num;
{
if (debug) fprintf(debug, "rmtd: E %d (%s)\n", num, sys_errlist[num]);
	(void) sprintf(resp, "E%d\n%s\n", num, sys_errlist[num]);
	(void) write(1, resp, strlen (resp));
}

