#ifndef lint
static char *sccsid = "@(#)tftpd.c	4.1	ULTRIX	7/2/90";
#endif not lint
/************************************************************************
 *									*
 *			Copyright (c) 1988, 1989 by			*
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
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifndef lint
char copyright[] =
"Copyright (c) 1983 Regents of the University of California.\n\
 All rights reserved.\n";
#endif not lint


#define SUN_BOOT	/* JSD */
/*
 * Trivial file transfer protocol server.
 *
 * This version includes many modifications by Jim Guyton <guyton@rand-unix>
 * merged with Sun features for tftp booting.
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/time.h>

#include <netinet/in.h>

#include <arpa/tftp.h>

#include <sys/dir.h>
#include <signal.h>
#include <stdio.h>
#include <errno.h>
#include <ctype.h>
#include <netdb.h>
#include <setjmp.h>
#include <syslog.h>

#define	TIMEOUT		5

extern	int errno;
struct	sockaddr_in sin = { AF_INET };
int	peer;
int	rexmtval = TIMEOUT;
int	maxtimeout = 5*TIMEOUT;

#define	PKTSIZE	SEGSIZE+4
char	buf[PKTSIZE];
char	ackbuf[PKTSIZE];
struct	sockaddr_in from;
int	fromlen;
int	debug = 0;
FILE  *file = NULL;
char	*newroot = (char *)0;


#ifdef SUN_BOOT
/*
 *  compatibility suffixes for diskless workstation booting
 */
char	*compatexten [] = { ".SUN2", ".SUN3", ".SUN4", ""};

/*
 * Default directory for unqualified names
 * Used by TFTP boot procedures
 */
char	*tftphomedir = "/tftpboot";
#endif SUN_BOOT


main(argc, argv)
int argc;
char **argv;
{
	register struct tftphdr *tp;
	register int n;
	int on = 1;

#ifdef	LOG_DAEMON
	openlog("tftpd", LOG_PID, LOG_DAEMON);
#else
	openlog("tftpd", LOG_PID);
#endif	LOG_DAEMON

	while ((argc > 1) && (argv[1][0] == '-')) {
	    switch (argv[1][1]) {
		case 'r':	/* -r newroot */
		    if (argc < 2) {
			    syslog(LOG_WARNING, "-r is missing homedir arg, using default %s\n", tftphomedir);
			    newroot = tftphomedir;
		    } else {
			    newroot = argv[2];
		    }
		    argv++;
		    argc--;
		    break;

		case 'd':
		    debug++;
		    break;
	    
		default:
		    usage();
		    break;
	    }
	    argv++;
	    argc--;
	}
#ifdef SUN_BOOT
	/*
	 * warn about any other ignored arguments.
	 */
	while (argc > 1) {
		syslog(LOG_ERR, "ignored argument: %s", argv[1]);
		argc--; argv++;
	}
	if (debug)
		syslog(LOG_DEBUG, ">>> start: homedir %s", newroot);
#endif SUN_BOOT

	if (ioctl(0, FIONBIO, &on) < 0) {
		syslog(LOG_ERR, "ioctl(FIONBIO): %m\n");
		exit(1);
	}
	fromlen = sizeof (from);
	n = recvfrom(0, buf, sizeof (buf), 0,
	    (caddr_t)&from, &fromlen);
	if (n < 0) {
		syslog(LOG_ERR, "recvfrom: %m\n");
		exit(1);
	}
	/*
	 * Now that we have read the message out of the UDP
	 * socket, we fork and exit.  Thus, inetd will go back
	 * to listening to the tftp port, and the next request
	 * to come in will start up a new instance of tftpd.
	 *
	 * We do this so that inetd can run tftpd in "wait" mode.
	 * The problem with tftpd running in "nowait" mode is that
	 * inetd may get one or more successful "selects" on the
	 * tftp port before we do our receive, so more than one
	 * instance of tftpd may be started up.  Worse, if tftpd
	 * break before doing the above "recvfrom", inetd would
	 * spawn endless instances, clogging the system.
	 */
	{
		int pid;
		int i, j;

		for (i = 1; i < 20; i++) {
		    pid = fork();
		    if (pid < 0) {
				sleep(i);
				/*
				 * flush out to most recently sent request.
				 *
				 * This may drop some request, but those
				 * will be resent by the clients when
				 * they timeout.  The positive effect of
				 * this flush is to (try to) prevent more
				 * than one tftpd being started up to service
				 * a single request from a single client.
				 */
				j = sizeof from;
				i = recvfrom(0, buf, sizeof (buf), 0,
				    (caddr_t)&from, &j);
				if (i > 0) {
					n = i;
					fromlen = j;
				}
		    } else {
				break;
		    }
		}
		if (pid < 0) {
			syslog(LOG_ERR, "fork: %m");
			exit(1);
		} else if (pid != 0) {
			exit(0);
		}
	}
	from.sin_family = AF_INET;
	alarm(0);
	close(0);
	close(1);

	if (newroot) {
	    if (chroot(newroot) < 0) {
		syslog(LOG_ERR, "chroot: %m");
		exit(1);
	    }
	    (void)chdir("/");
	    if (setuid(getuid()) < 0) {
		syslog(LOG_ERR, "setuid: %m");
		exit(1);
	    }
	}

	peer = socket(AF_INET, SOCK_DGRAM, 0);
	if (peer < 0) {
		syslog(LOG_ERR, "socket: %m");
		exit(1);
	}
	if (bind(peer, (caddr_t)&sin, sizeof (sin)) < 0) {
		syslog(LOG_ERR, "bind: %m");
		exit(1);
	}
	if (connect(peer, (caddr_t)&from, sizeof(from)) < 0) {
		syslog(LOG_ERR, "connect: %m");
		exit(1);
	}
	tp = (struct tftphdr *)buf;
	tp->th_opcode = ntohs(tp->th_opcode);
	if (tp->th_opcode == RRQ || tp->th_opcode == WRQ)
		tftp(tp, n);
	exit(1);
}

int	validate_access();
int	sendfile(), recvfile();

struct formats {
	char	*f_mode;
	int	(*f_validate)();
	int	(*f_send)();
	int	(*f_recv)();
	int	f_convert;
} formats[] = {
	{ "netascii",	validate_access,	sendfile,	recvfile, 1 },
	{ "octet",	validate_access,	sendfile,	recvfile, 0 },
#ifdef notdef
	{ "mail",	validate_user,		sendmail,	recvmail, 1 },
#endif
	{ 0 }
};

/*
 * Handle initial connection protocol.
 */
tftp(tp, size)
	struct tftphdr *tp;
	int size;
{
	register char *cp;
	int first = 1, ecode;
	register struct formats *pf;
	char *filename, *mode;

	filename = cp = tp->th_stuff;
again:
	while (cp < buf + size) {
		if (*cp == '\0')
			break;
		cp++;
	}
	if (*cp != '\0') {
		nak(EBADOP);
		exit(1);
	}
	if (first) {
		mode = ++cp;
		first = 0;
		goto again;
	}
	for (cp = mode; *cp; cp++)
		if (isupper(*cp))
			*cp = tolower(*cp);
	for (pf = formats; pf->f_mode; pf++)
		if (strcmp(pf->f_mode, mode) == 0)
			break;
	if (pf->f_mode == 0) {
		nak(EBADOP);
		exit(1);
	}
	ecode = (*pf->f_validate)(filename, tp->th_opcode);
	if (ecode) {
		nak(ecode);
		exit(1);
	}
	if (tp->th_opcode == WRQ)
		(*pf->f_recv)(pf);
	else
		(*pf->f_send)(pf);
	exit(0);
}


/*
 * Validate file access.  Since we
 * have no uid or gid, for now require
 * file to exist and be publicly
 * readable/writable.
 * Note also, full path name must be
 * given as we have no login directory (unless we have done a chroot)
 */
validate_access(filename, mode)
	char *filename;
	int mode;
{
	struct stat stbuf;
	int	fd, i;
	int	stated = 0;
	char	newfilename [MAXNAMLEN + 1];

	if (debug)
		syslog(LOG_DEBUG, "validate %s access for \"%s\"",
			mode == RRQ? "read" : "write", filename);
#ifdef SUN_BOOT
	/*
	 * Need to perform access check as someone who will only
	 * be allowed "public" access to the file.  There is no
	 * such uid/gid reserved so we set it to -2/-2 ("nobody").
	 * (Can't use -1/-1 'cause that means "don't change".)
	 */
	if (setgid(-2) < 0) {
		syslog(LOG_ERR, "setgid: %m");
		exit(1);
	}
	if (setuid(-2) < 0) {
		syslog(LOG_ERR, "setuid: %m");
		exit(1);
	}
#endif SUN_BOOT

	if (*filename != '/')
	   if (newroot == (char *)0)
		return (EACCESS);

#ifdef SUN_BOOT
	if (stat(filename, &stbuf) >= 0)
		stated = 1;

	if (!stated) {

	  /* check that requested filename is in boot file format */
	  if (filename [0] == '/')
	    return (errno == ENOENT ? ENOTFOUND : EACCESS);

	  for (i = 0; i < 7; i++)
	    if (!(filename [i] >= '0' && filename [i] <= '9') &&
		!(filename [i] >= 'A' && filename [i] <= 'F'))
	      return (ENOTFOUND);


	  /* 
	   * For compatibility with the old format of boot filenames,
	   * check a variety of extentions.
	   */

	  if (strlen (filename) <= 8) {

	    /* try adding extentions to filename */

	    for (i = 0; compatexten [i] [0] != '\0'; i++) {
	      strcpy (newfilename, filename);
	      strcat (newfilename, compatexten [i]);
	      if (stat (newfilename, &stbuf) >= 0) {
		filename = &newfilename [0];
		stated = 1;
		break;
	      }
	    }
	  } else {

	    /* take off the extention */

	    strncpy (newfilename, filename, 8);
	    newfilename [8] = '\0';
	    if (stat (newfilename, &stbuf) >= 0) {
	      filename = &newfilename [0];
	      stated = 1;
	    }
	  }
	}

	if (!stated)
	  return (ENOTFOUND);
#endif SUN_BOOT

	if (mode == RRQ) {
		if ((stbuf.st_mode&(S_IREAD >> 6)) == 0)
			return (EACCESS);
	} else {
		if ((stbuf.st_mode&(S_IWRITE >> 6)) == 0)
			return (EACCESS);
	}
#ifdef SUN_BOOT
	if ((stbuf.st_mode & S_IFMT) != S_IFREG)
		return (EACCESS);
#endif SUN_BOOT
	fd = open(filename, mode == RRQ ? 0 : 1);
	if (fd < 0)
		return (errno + 100);
	file = fdopen(fd, (mode == RRQ)? "r":"w");
	if (file == NULL) {
		(void) close(fd);
		return errno+100;
	}
	if (debug)
		syslog(LOG_DEBUG, "\"%s\" opened ok", filename);
	return (0);
}

int	timeout;
jmp_buf	timeoutbuf;

timer()
{

	timeout += rexmtval;
	if (timeout >= maxtimeout)
		exit(1);
	longjmp(timeoutbuf, 1);
}

/*
 * Send the requested file.
 */
sendfile(pf)
	struct formats *pf;
{
	struct tftphdr *dp, *r_init();
	register struct tftphdr *ap;    /* ack packet */
	register int block = 1, size, n;

	signal(SIGALRM, timer);
	dp = r_init();
	ap = (struct tftphdr *)ackbuf;
	do {
		size = readit(file, &dp, pf->f_convert);
		if (size < 0) {
			nak(errno + 100);
			goto abort;
		}
		dp->th_opcode = htons((u_short)DATA);
		dp->th_block = htons((u_short)block);
		timeout = 0;
		(void) setjmp(timeoutbuf);

send_data:
		if (send(peer, dp, size + 4, 0) != size + 4) {
			if ((errno == ENETUNREACH) ||
			    (errno == EHOSTUNREACH) ||
			    (errno == ECONNREFUSED))
				syslog(LOG_WARNING, "send (data): %m");
	       		else
				syslog(LOG_ERR, "send (data): %m");
			goto abort;
		}
		read_ahead(file, pf->f_convert);
		for ( ; ; ) {
			alarm(rexmtval);        /* read the ack */
			n = recv(peer, ackbuf, sizeof (ackbuf), 0);
			alarm(0);
			if (n < 0) {
				if (errno == EINTR)
				  continue;
				if ((errno == ENETUNREACH) || 
				    (errno == EHOSTUNREACH) || 
				    (errno == ECONNREFUSED))
				  syslog(LOG_WARNING, "recv (ack): %m");
				else
				  syslog(LOG_ERR, "recv (ack): %m");
				goto abort;
			}
			ap->th_opcode = ntohs((u_short)ap->th_opcode);
			ap->th_block = ntohs((u_short)ap->th_block);

			if (ap->th_opcode == ERROR)
				goto abort;
			
			if (ap->th_opcode == ACK) {
				if (ap->th_block == block) {
					break;
				}
				/* Re-synchronize with the other side */
				(void) synchnet(peer);
				if (ap->th_block == (block -1)) {
					goto send_data;
				}
			}

		}
		block++;
	} while (size == SEGSIZE);
abort:
	(void) fclose(file);
}

justquit()
{
	exit(0);
}


/*
 * Receive a file.
 */
recvfile(pf)
	struct formats *pf;
{
	struct tftphdr *dp, *w_init();
	register struct tftphdr *ap;    /* ack buffer */
	register int block = 0, n, size;

	signal(SIGALRM, timer);
	dp = w_init();
	ap = (struct tftphdr *)ackbuf;
	do {
		timeout = 0;
		ap->th_opcode = htons((u_short)ACK);
		ap->th_block = htons((u_short)block);
		block++;
		(void) setjmp(timeoutbuf);
send_ack:
		if (send(peer, ackbuf, 4, 0) != 4) {
			syslog(LOG_ERR, "send (ack): %m");
			goto abort;
		}
		write_behind(file, pf->f_convert);
		for ( ; ; ) {
			alarm(rexmtval);
			n = recv(peer, dp, PKTSIZE, 0);
			alarm(0);
			if (n < 0) {            /* really? */
				syslog(LOG_ERR, "recv (data): %m\n");
				goto abort;
			}
			dp->th_opcode = ntohs((u_short)dp->th_opcode);
			dp->th_block = ntohs((u_short)dp->th_block);
			if (dp->th_opcode == ERROR)
				goto abort;
			if (dp->th_opcode == DATA) {
				if (dp->th_block == block) {
					break;   /* normal */
				}
				/* Re-synchronize with the other side */
				(void) synchnet(peer);
				if (dp->th_block == (block-1))
					goto send_ack;          /* rexmit */
			}
		}
		/*  size = write(file, dp->th_data, n - 4); */
		size = writeit(file, &dp, n - 4, pf->f_convert);
		if (size != (n-4)) {                    /* ahem */
			if (size < 0) nak(errno + 100);
			else nak(ENOSPACE);
			goto abort;
		}
	} while (size == SEGSIZE);
	write_behind(file, pf->f_convert);
	(void) fclose(file);            /* close data file */

	ap->th_opcode = htons((u_short)ACK);    /* send the "final" ack */
	ap->th_block = htons((u_short)(block));
	(void) send(peer, ackbuf, 4, 0);

	signal(SIGALRM, justquit);      /* just quit on timeout */
	alarm(rexmtval);
	n = recv(peer, buf, sizeof (buf), 0); /* normally times out and quits */
	alarm(0);
	if (n >= 4 &&                   /* if read some data */
	    dp->th_opcode == DATA &&    /* and got a data block */
	    block == dp->th_block) {	/* then my last ack was lost */
		(void) send(peer, ackbuf, 4, 0);     /* resend final ack */
	}
abort:
	(void) fclose(file);            /* close data file */
	return;
}

struct errmsg {
	int	e_code;
	char	*e_msg;
} errmsgs[] = {
	{ EUNDEF,	"Undefined error code" },
	{ ENOTFOUND,	"File not found" },
	{ EACCESS,	"Access violation" },
	{ ENOSPACE,	"Disk full or allocation exceeded" },
	{ EBADOP,	"Illegal TFTP operation" },
	{ EBADID,	"Unknown transfer ID" },
	{ EEXISTS,	"File already exists" },
	{ ENOUSER,	"No such user" },
	{ -1,		0 }
};

/*
 * Send a nak packet (error message).
 * Error code passed in is one of the
 * standard TFTP codes, or a UNIX errno
 * offset by 100.
 */
nak(error)
	int error;
{
	register struct tftphdr *tp;
	int length;
	register struct errmsg *pe;
	extern char *sys_errlist[];

	tp = (struct tftphdr *)buf;
	tp->th_opcode = htons((u_short)ERROR);
	tp->th_code = htons((u_short)error);
	for (pe = errmsgs; pe->e_code >= 0; pe++)
		if (pe->e_code == error)
			break;
	if (pe->e_code < 0) {
		pe->e_msg = sys_errlist[error - 100];
		tp->th_code = EUNDEF;   /* set 'undef' errorcode */
	}
	strcpy(tp->th_msg, pe->e_msg);
	length = strlen(pe->e_msg);
	tp->th_msg[length] = '\0';
	length += 5;
	if (send(peer, buf, length, 0) != length) {
		if ((errno == ENETUNREACH) || (errno == EHOSTUNREACH) || 
		    (errno == ECONNREFUSED))
			syslog(LOG_WARNING, "send (nak): %m");
		else
			syslog(LOG_ERR, "send (nak): %m");
	}
}

usage()
{
  (void) fprintf (stderr, "usage: tftpd [-d] [-r] [ home-directory ]\n");
  exit (1);
}
