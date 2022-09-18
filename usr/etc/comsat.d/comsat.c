#ifndef lint
static	char sccsid[] = "@(#)comsat.c	4.1	(ULTRIX)	7/2/90";
#endif

/* /usr/src/etc/comsat.c
 */
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

/*-----------------------------------------------------------------------
 *	Modification History
 *
 *	6/9/88 -- map
 *		Changed signal handlers to void.
 *
 *	4/5/85 -- jrs
 *		Revise to allow inetd to perform front end functions,
 *		following the Berkeley model.
 *
 *	Based on 4.2BSD labeled:
 *		comsat.c	4.13	84/09/13
 *
 *-----------------------------------------------------------------------
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/file.h>

#include <netinet/in.h>

#include <stdio.h>
#include <sgtty.h>
#include <utmp.h>
#include <signal.h>
#include <errno.h>
#include <netdb.h>
#include <syslog.h>

/*
 * comsat
 */
int	debug = 0;
#define	dprintf	if (debug) printf

extern	errno;

char	hostname[32];
struct	utmp *utmp = NULL;
int	nutmp;
int	uf;
unsigned utmpmtime;			/* last modification time for utmp */
void	onalrm();
void	reapchildren();
long	lastmsgtime;

#define	MAXIDLE	120
#define NAMLEN (sizeof (uts[0].ut_name) + 1)

main(argc, argv)
	int argc;
	char *argv[];
{
	register int cc;
	char msgbuf[100];
	struct sockaddr_in from;
	int fromlen;

	/* verify proper invocation */
	fromlen = sizeof (from);
	if (getsockname(0, &from, &fromlen) < 0) {
		fprintf(stderr, "%s: ", argv[0]);
		perror("getsockname");
		_exit(1);
	}
	(void) chdir("/usr/spool/mail");
	if ((uf = open("/etc/utmp",0)) < 0) {
		openlog("comsat", 0, 0);
		syslog(LOG_ERR, "/etc/utmp: %m");
		(void) recv(0, msgbuf, sizeof (msgbuf) - 1, 0);
		exit(1);
	}
	lastmsgtime = time(0);
	(void) gethostname(hostname, sizeof (hostname));
	onalrm();
	(void) signal(SIGALRM, onalrm);
	(void) signal(SIGTTOU, SIG_IGN);
	(void) signal(SIGCHLD, reapchildren);
	for (;;) {
		cc = recv(0, msgbuf, sizeof (msgbuf) - 1, 0);
		if (cc <= 0) {
			if (errno != EINTR)
				sleep(1);
			errno = 0;
			continue;
		}
		(void) sigblock(1<<SIGALRM);
		msgbuf[cc] = 0;
		lastmsgtime = time(0);
		mailfor(msgbuf);
		(void) sigsetmask(0);
	}
}

void
reapchildren()
{

	while (wait3((struct wait *)0, WNOHANG, (struct rusage *)0) > 0)
		;
}

void
onalrm()
{
	struct stat statbf;
	char msgbuf[100];

	if (time(0) - lastmsgtime >= MAXIDLE)
		exit(0);
	dprintf("alarm\n");
	(void) alarm(15);
	(void) fstat(uf, &statbf);
	if (statbf.st_mtime > utmpmtime) {
		dprintf(" changed\n");
		utmpmtime = statbf.st_mtime;
		(void) lseek(uf, 0l, 0);
		if (utmp != NULL) free(utmp);
		utmp = (struct utmp *) malloc((unsigned)statbf.st_size);
		if (utmp == NULL) {
			openlog("comsat", 0, 0);
			syslog(LOG_ERR, "malloc of utmp: %m");
			(void) recv(0, msgbuf, sizeof (msgbuf) - 1, 0);
			exit(1);
		}
		nutmp = read(uf,utmp,statbf.st_size)/sizeof(struct utmp);
	} else
		dprintf(" ok\n");
}

mailfor(name)
	char *name;
{
	register struct utmp *utp = &utmp[nutmp];
	register char *cp;
	char *rindex();
	int offset;

	dprintf("mailfor %s\n", name);
	cp = name;
	while (*cp && *cp != '@')
		cp++;
	if (*cp == 0) {
		dprintf("bad format\n");
		return;
	}
	*cp = 0;
	offset = atoi(cp+1);
	while (--utp >= utmp)
		if (!strncmp(utp->ut_name, name, sizeof(utmp[0].ut_name)))
			notify(utp, offset);
}

char	*cr;

notify(utp, offset)
	register struct utmp *utp;
{
	FILE *tp;
	struct sgttyb gttybuf;
	char tty[20], name[sizeof (utmp[0].ut_name) + 1];
	struct stat stb;

	(void) strcpy(tty, "/dev/");
	(void) strncat(tty, utp->ut_line, sizeof(utp->ut_line));
	dprintf("notify %s on %s\n", utp->ut_name, tty);
	if (stat(tty, &stb) == 0 && (stb.st_mode & 0100) == 0) {
		dprintf("wrong mode\n");
		return;
	}
	if (fork())
		return;
	(void) signal(SIGALRM, SIG_DFL);
	(void) alarm(30);
	if ((tp = fopen(tty,"w")) == 0) {
		dprintf("fopen failed\n");
		exit(-1);
	}
	ioctl(fileno(tp), TIOCGETP, &gttybuf);
	cr = (gttybuf.sg_flags & CRMOD) ? "" : "\r";
	(void) strncpy(name, utp->ut_name, sizeof (utp->ut_name));
	name[sizeof (name) - 1] = '\0';
	fprintf(tp,"%s\n\007New mail for %s@%s\007 has arrived:%s\n",
	    cr, name, hostname, cr);
	fprintf(tp,"----%s\n", cr);
	jkfprintf(tp, name, offset);
	exit(0);
}

jkfprintf(tp, name, offset)
	register FILE *tp;
{
	register FILE *fi;
	register int linecnt, charcnt;
	char line[BUFSIZ];
	int inheader;
	char mailbox[sizeof(name*3)];
	dprintf("HERE %s's mail starting at %d\n",
	    name, offset);

	strcpy(mailbox,name);

        if (isdir(mailbox)) {          /*if /usr/spool/mail/name is a directory, then
                                         *mailbox = /usr/spool/mail/name/name
                                         */
                strcat(mailbox, "/");
                strcat(mailbox, name);
        }

	if ((fi = fopen(mailbox,"r")) == NULL) {
		dprintf("Cant read the mail\n");
		return;
	}
	(void) fseek(fi, offset, L_SET);
	/* 
	 * Print the first 7 lines or 560 characters of the new mail
	 * (whichever comes first).  Skip header crap other than
	 * From, Subject, To, and Date.
	 */
	linecnt = 7;
	charcnt = 560;
	inheader = 1;
	while (fgets(line, sizeof (line), fi) != NULL) {
		register char *cp;
		char *index();
		int cnt;

		if (linecnt <= 0 || charcnt <= 0) {  
			fprintf(tp,"...more...%s\n", cr);
			return;
		}
		if (strncmp(line, "From ", 5) == 0)
			continue;
		if (inheader && (line[0] == ' ' || line[0] == '\t'))
			continue;
		cp = index(line, ':');
		if (cp == 0 || (index(line, ' ') && index(line, ' ') < cp))
			inheader = 0;
		else
			cnt = cp - line;
		if (inheader &&
		    strncmp(line, "Date", cnt) &&
		    strncmp(line, "From", cnt) &&
		    strncmp(line, "Subject", cnt) &&
		    strncmp(line, "To", cnt))
			continue;
		cp = index(line, '\n');
		if (cp)
			*cp = '\0';
		fprintf(tp,"%s%s\n", line, cr);
		linecnt--, charcnt -= strlen(line);
	}
	fprintf(tp,"----%s\n", cr);
}


/*
 * Test to see if the passed file name is a directory.
 * Return true if it is.
 */

isdir(name)
        char name[];
{
        struct stat sbuf;

        if (stat(name, &sbuf) < 0)
                return(0);
        return((sbuf.st_mode & S_IFMT) == S_IFDIR);
}
