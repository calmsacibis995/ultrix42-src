#ifndef lint
static char *sccsid = "@(#)lpd.c	4.2      ULTRIX 	4/25/91";
#endif

/************************************************************************
 *									*
 *			Copyright (c) 1988-1990 by			*
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
 * lpd -- line printer daemon.
 *
 * Listen for a connection and perform the requested operation.
 * Operations are:
 *	\1printer\n
 *		check the queue for jobs and print any found.
 *	\2printer\n
 *		receive a job from another machine and queue it.
 *	\3printer [users ...] [jobs ...]\n
 *		return the current state of the queue (short form).
 *	\4printer [users ...] [jobs ...]\n
 *		return the current state of the queue (long form).
 *	\5printer person [users ...] [jobs ...]\n
 *		remove jobs from the queue.
 *	\6printer\n
 *		enable queuing on the specified printer queue.
 *	\7printer\n
 *		disable queuing on the specified printer queue.
 *	\8printer\n
 *		return the queue status (queuing enabled or disabled).
 *
 * Strategy to maintain protected spooling area:
 *	1. Spooling area is writable only by daemon and spooling group
 *	2. lpr runs setuid root and setgrp spooling group; it uses
 *	   root to access any file it wants (verifying things before
 *	   with an access call) and group id to know how it should
 *	   set up ownership of files in the spooling area.
 *	3. Files in spooling area are owned by root, group spooling
 *	   group, with mode 660.
 *	4. lpd, lpq and lprm run setuid daemon and setgrp spooling group to
 *	   access files and printer.  Users can't get to anything
 *	   w/o help of lpq and lprm programs.
 */

/*
 * Modification History:
 *
 * 20-mar-90 -- Adrian Thoms (thoms)
 *	Provide dummy user arguments to parser function to void
 *	dereferencing null pointers on mips
 *
 * 07-feb-90 -- sue
 *	Added code to allow short host names in /etc/hosts.lpd for
 *	machines in current domain.
 *
 * 11-jan-90 -- thoms
 *	Fixed buffer lengths for host names
 *
 *	24-nov-89 -- m irish
 * changed socket group to daemon to allow access to lpc 
 * ( for restarts) when socket protection is implemented. 
 *
 *	9-Aug-89 -- Giles Atkinson and Margaret Irish
 * Added code to set permissions on /dev/printer,
 * allow a `*' in /etc/hosts.equiv and correctly report the queue name
 * to the remote user when printer access is denied.
 *
 *
 */

/* SCCS history beginning
 * ***************************************************************
 *                -- Revision History --
 * ***************************************************************
 * 
 * 1.1  01/11/83 -- sccs
 * date and time created 83/11/01 20:57:38 by sccs
 * 
 * ***************************************************************
 * 
 * 1.2  28/06/85 -- root
 * Comments taken from: /usr/src/usr.lib/lpr/SCCS/s.lpd.c:
 *                      1.2 85/06/24 08:57:18 williams 2 1	00002/00002/00446
 * added printjob argument to init or not depending on whether the daemon
 * has been started up or not.
 * 
 * 
 * 
 * ***************************************************************
 * 
 * 1.3  02/10/86 -- hoffman
 * Added code to check for name of remote computer in new file /etc/hosts.lpd .
 * This check supplements that in /etc/hosts.equiv and provides a means for
 * system managers to allow print requests from remote systems without
 * including those systems in the list of trusted names in hosts.equiv .
 * 
 * 
 * ***************************************************************
 * 
 * 1.4  10/03/88 -- thoms
 * 
 * ***************************************************************
 * 
 * 1.5  22/03/88 -- thoms
 * DCL support added (using dcl output filter)
 * 
 * 
 * ***************************************************************
 *
 * 1.6  19/07/88 -- thoms
 * DCL support now built in to lpd
 * Added copyright notice and modification history
 *
 * ***************************************************************
 *
 * 1.7 28/07/88 -- thoms
 * Moved logging related fns from here to logging.c
 *
 * ***************************************************************
 *
 * SCCS history end
 */


#include "lp.h"

static int	lflag;				/* log requests flag */
static char	*logfile = DEFLOGF;

int	reapchild();
int	cleanup();

main(argc, argv)
	int argc;
	char **argv;
{
	int f, funix, finet, options=0, defreadfds, fromlen;
	struct sockaddr_un sun, fromunix;
	struct sockaddr_in sin, frominet;
	int omask, lfd;
	int daemon = 1, root = 0;

	gethostname(host, sizeof(host));
	name = argv[0];

	while (--argc > 0) {
		argv++;
		if (argv[0][0] == '-')
			switch (argv[0][1]) {
			case 'd':
				options |= SO_DEBUG;
				break;
			case 'l':
				lflag++;
				break;
			case 'L':
				argc--;
				logfile = *++argv;
				break;
			}
	}

#ifndef DEBUG
	/*
	 * Set up standard environment by detaching from the parent.
	 */
	if (fork())
		exit(0);
	for (f = 0; f < 3; f++)
		(void) close(f);
	(void) open("/dev/null", O_RDONLY);
	(void) open("/dev/null", O_WRONLY);
	(void) open(logfile, O_WRONLY|O_APPEND);
	f = open("/dev/tty", O_RDWR);
	if (f > 0) {
		ioctl(f, TIOCNOTTY, 0);
		(void) close(f);
	}
#endif

	(void) umask(0);
	lfd = open(MASTERLOCK, O_WRONLY|O_CREAT, 0644);
	if (lfd < 0) {
		log("cannot create %s", MASTERLOCK);
		exit(1);
	}
	if (flock(lfd, LOCK_EX|LOCK_NB) < 0) {
		if (errno == EWOULDBLOCK)	/* active deamon present */
			exit(0);
		log("cannot lock %s", MASTERLOCK);
		exit(1);
	}
	ftruncate(lfd, 0);
	/*
	 * write process id for others to know
	 */
	sprintf(line, "%u\n", getpid());
	f = strlen(line);
	if (write(lfd, line, f) != f) {
		log("cannot write daemon pid");
		exit(1);
	}
	signal(SIGCHLD, reapchild);
	/*
	 * Restart all the printers.
	 */
	startup();
	(void) unlink(SOCKETNAME);
	funix = socket(AF_UNIX, SOCK_STREAM, 0);
	if (funix < 0) {
		logerr("socket");
		exit(1);
	}
#define	mask(s)	(1 << ((s) - 1))
	omask = sigblock(mask(SIGHUP)|mask(SIGINT)|mask(SIGQUIT)|mask(SIGTERM));
	signal(SIGHUP, cleanup);
	signal(SIGINT, cleanup);
	signal(SIGQUIT, cleanup);
	signal(SIGTERM, cleanup);
	sun.sun_family = AF_UNIX;
	strcpy(sun.sun_path, SOCKETNAME);
	if (bind(funix, &sun, strlen(sun.sun_path) + 2) < 0) {
		logerr("unix domain bind");
		exit(1);
	}
	if (chmod(SOCKETNAME,0770) != 0) {
		logerr("chmod socket");
		exit(1);
	}
	if (chown(SOCKETNAME,root,daemon) != 0){ 
                logerr("chown socket");
                exit(1);
        }

	sigsetmask(omask);
	defreadfds = 1 << funix;
	listen(funix, 5);
	finet = socket(AF_INET, SOCK_STREAM, 0);

	if (finet >= 0) {
		struct servent *sp;

		if (options & SO_DEBUG)
			if (setsockopt(finet, SOL_SOCKET, SO_DEBUG, 0, 0) < 0) {
				logerr("setsockopt (SO_DEBUG)");
				cleanup();
			}
		sp = getservbyname("printer", "tcp");
		if (sp == NULL) {
			log("printer/tcp: unknown service");
			cleanup();
		}
		sin.sin_family = AF_INET;
		sin.sin_port = sp->s_port;
		if (bind(finet, &sin, sizeof(sin), 0) < 0) {
			logerr("internet domain bind");
			cleanup();
		}
		defreadfds |= 1 << finet;
		listen(finet, 5);
	}
	/*
	 * Main loop: accept, do a request, continue.
	 */
	for (;;) {
		int domain, nfds, s, readfds = defreadfds;

		nfds = select(20, &readfds, 0, 0, 0);
		if (nfds <= 0) {
			if (nfds < 0 && errno != EINTR) {
				logerr("select");
				cleanup();
				/*NOTREACHED*/
			}
			continue;
		}
		if (readfds & (1 << funix)) {
			domain = AF_UNIX, fromlen = sizeof(fromunix);
			s = accept(funix, &fromunix, &fromlen);
		} else if (readfds & (1 << finet)) {
			domain = AF_INET, fromlen = sizeof(frominet);
			s = accept(finet, &frominet, &fromlen);
		}
		if (s < 0) {
			if (errno == EINTR)
				continue;
			logerr("accept");
			cleanup();
		}
		if (fork() == 0) {
			signal(SIGCHLD, SIG_IGN);
			signal(SIGHUP, SIG_IGN);
			signal(SIGINT, SIG_IGN);
			signal(SIGQUIT, SIG_IGN);
			signal(SIGTERM, SIG_IGN);
			(void) close(funix);
			(void) close(finet);
			dup2(s, 1);
			(void) close(s);
			if (domain == AF_INET)
				chkhost(&frominet);
			doit();
			exit(0);
		}
		(void) close(s);
	}
}

static
reapchild()
{
	union wait status;

	while (wait3(&status, WNOHANG, 0) > 0)
		;
}

static
cleanup()
{
	if (lflag)
		log("cleanup()");
	unlink(SOCKETNAME);
	exit(0);
}

/*
 * Stuff for handling job specifications
 */
char	*user[MAXUSERS];	/* users to process */
int	users;			/* # of users in user array */
int	requ[MAXREQUESTS];	/* job number of spool entries */
int	requests;		/* # of spool requests */
char	*person;		/* name of person doing lprm */

static char	fromb[HOSTNAME_LEN + 1];/* buffer for client's machine name */
static char	cbuf[BUFSIZ];	/* command line buffer */
static char	*cmdnames[] = {
	"null",
	"printjob",
	"recvjob",
	"displayq short",
	"displayq long",
	"rmjob"
};

static
doit()
{
	register char *cp;
	register int n;

	for (;;) {
		cp = cbuf;
		do {
			if (cp >= &cbuf[sizeof(cbuf) - 1])
				fatal("Command line too long");
			if ((n = read(1, cp, 1)) != 1) {
				if (n < 0)
					fatal("Lost connection");
				return;
			}
		} while (*cp++ != '\n');
		*--cp = '\0';
		cp = cbuf;
		if (lflag && *cp >= '\1' && *cp <= '\5') {
			printer = NULL;
			log("%s requests %s %s", from, cmdnames[*cp], cp+1);
		}
		switch (*cp++) {
		case '\1':	/* check the queue and print any jobs there */
			printer = cp;
			printjob(0);
			break;
		case '\2':	/* receive files to be queued */
			printer = cp;
			recvjob();
			break;
		case '\3':	/* display the queue (short form) */
		case '\4':	/* display the queue (long form) */
			printer = cp;
			while (*cp) {
				if (*cp != ' ') {
					cp++;
					continue;
				}
				*cp++ = '\0';
				while (isspace(*cp))
					cp++;
				if (*cp == '\0')
					break;
				if (isdigit(*cp)) {
					if (requests >= MAXREQUESTS)
						fatal("Too many requests");
					requ[requests++] = atoi(cp);
				} else {
					if (users >= MAXUSERS)
						fatal("Too many users");
					user[users++] = cp;
				}
			}
			displayq(cbuf[0] - '\3');
			exit(0);
		case '\5':	/* remove a job from the queue */
			printer = cp;
			while (*cp && *cp != ' ')
				cp++;
			if (!*cp)
				break;
			*cp++ = '\0';
			person = cp;
			while (*cp) {
				if (*cp != ' ') {
					cp++;
					continue;
				}
				*cp++ = '\0';
				while (isspace(*cp))
					cp++;
				if (*cp == '\0')
					break;
				if (isdigit(*cp)) {
					if (requests >= MAXREQUESTS)
						fatal("Too many requests");
					requ[requests++] = atoi(cp);
				} else {
					if (users >= MAXUSERS)
						fatal("Too many users");
					user[users++] = cp;
				}
			}
			rmjob();
			break;
		}
		fatal("Illegal service request");
	}
}

/*
 * Make a pass through the printcap database and start printing any
 * files left from the last time the machine went down.
 */
static
startup()
{
	char buf[BUFSIZ];
	register char *cp;
	int pid;

	printer = buf;

	/*
	 * Restart the daemons.
	 */
	while (getprent(buf) > 0) {
		for (cp = buf; *cp; cp++)
			if (*cp == '|' || *cp == ':') {
				*cp = '\0';
				break;
			}
		if ((pid = fork()) < 0) {
			log("startup: cannot fork");
			cleanup();
		}
		if (!pid) {
			endprent();
			printjob(1);
		}
	}
}

/*
 * Check to see if the from host has access to the line printer.
 */
static
chkhost(f)
	struct sockaddr_in *f;
{
	register struct hostent *hp;
	register FILE *hostf;
	register char *cp, *sp;
	char ahost[HOSTNAME_LEN + 1];
	char temp_printer[34];  /* Holds printer name if access fails,
				   used to generate error message ONLY */
	int n, length;
	extern char *inet_ntoa();
	char *dummy_user="dummy"; /* Needed to avoid bug in libc.a:rcmd.o */

	f->sin_port = ntohs(f->sin_port);
	if (f->sin_family != AF_INET || f->sin_port >= IPPORT_RESERVED)
		fatal("Malformed from address");
	hp = gethostbyaddr(&f->sin_addr, sizeof(struct in_addr), f->sin_family);
	if (hp == 0)
		fatal("Host name for your address (%s) unknown",
			inet_ntoa(f->sin_addr));

	strcpy(fromb, hp->h_name);
	from = fromb;
	if (!strcmp(from, host))
		return;

	sp = fromb;
	cp = ahost;
	while (*sp) {
		if (*sp == '.')
			*cp++ = *sp++;
		else
			*cp++ = isupper(*sp) ? tolower(*sp++) : *sp++;
	}

	hostf = fopen("/etc/hosts.equiv", "r");
	if (hostf) {
		if (!parser(hostf, from, dummy_user, dummy_user, NULL)) {
 			(void) fclose(hostf);
			return;
		}
 		(void) fclose(hostf);
	}

 	hostf = fopen("/etc/hosts.lpd", "r");
	if (hostf) {
 		while (fgets(ahost, sizeof(ahost), hostf)) {
			if (ahost[0]=='*') {
 				(void) fclose(hostf);
 				return;
 			}
 			if (cp = index(ahost, '\n'))
 				*cp = '\0';
 			cp = index(ahost, ' ');
 			if (cp == NULL) {
 				if (!strcmp(from, ahost)) {
 					(void) fclose(hostf);
 					return;
				}
				else if (((length = local_hostname_length(from)) != NULL) && strlen(ahost) == length && (!strncmp(from, ahost, length))) {
						(void) fclose(hostf);
						return;
				}
 			}
 		}
 		(void) fclose(hostf);
	}

	/* We come here when the printer does not have access...however
	   read the printer name off the socket for error message */

	if ((n = read(1, temp_printer, sizeof temp_printer)) < 0)
		fatal("Lost Connection");
	cp = &temp_printer[1];
	while (cp < &temp_printer[sizeof temp_printer] && *cp != '\n' &&
	       *cp != ' ')
			++cp;
	*cp = '\0';
	printer = &temp_printer[1];  /* Ignore 1st char on socket */
	fatal("Your host does not have line printer access");
}
