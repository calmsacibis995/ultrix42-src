#ifndef lint
static char *sccsid = "@(#)write.c	4.1      ULTRIX  7/17/90";
#endif lint
/************************************************************************
 *									*
 *			Copyright (c) 1985,86,87,88,89 by		*
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
 *
 *   Modification history:
 *
 * 06 May 89 -- dal
 *	Took 4.3BSD version, with void signal handler changes.
 */
/*
	static char *sccsid = "@(#)write.c	4.13 3/13/86";
*/
/*
 * write to another user
 */

#include <stdio.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <utmp.h>
#include <sys/time.h>

#define	NMAX	sizeof(ubuf.ut_name)
#define	LMAX	sizeof(ubuf.ut_line)

char	*strcat();
char	*strcpy();
struct	utmp ubuf;
int	signum[] = {SIGHUP, SIGINT, SIGQUIT, 0};
char	me[NMAX + 1]	= "???";
char	*him;
char	*mytty;
char	histty[32];
char	ttybuf[32];
char	*histtya;
char	*ttyname();
char	*rindex();
int	logcnt;
void	eof();
void	timout();
FILE	*tf;
char	*getenv();

main(argc, argv)
	int argc;
	char *argv[];
{
	struct stat stbuf;
	register i;
	register FILE *uf;
	int c1, c2;
	long clock = time(0);
	int suser = getuid() == 0;
	int nomesg = 0;
	struct tm *localtime();
	struct tm *localclock = localtime( &clock );

	if (argc < 2) {
		fprintf(stderr, "Usage: write user [ttyname]\n");
		exit(1);
	}
	him = argv[1];
	if (argc > 2)
		histtya = argv[2];
	if ((uf = fopen("/etc/utmp", "r")) == NULL) {
		perror("write: Can't open /etc/utmp");
		if (histtya == 0)
			exit(10);
		goto cont;
	}
	mytty = ttyname(2);
	if (mytty == NULL) {
		fprintf(stderr, "write: Can't find your tty\n");
		exit(1);
	}
	if (stat(mytty, &stbuf) < 0) {
		perror("write: Can't stat your tty");
		exit(1);
	}
	if ((stbuf.st_mode&020) == 0) {
		fprintf(stderr,
			"write: You have write permission turned off\n");
		if (!suser)
			exit(1);
	}
	mytty = rindex(mytty, '/') + 1;
	if (histtya) {
		strcpy(histty, "/dev/");
		strcat(histty, histtya);
	}
	while (fread((char *)&ubuf, sizeof(ubuf), 1, uf) == 1) {
		if (ubuf.ut_name[0] == '\0')
			continue;
		if (strcmp(ubuf.ut_line, mytty)==0) {
			for (i=0; i<NMAX; i++) {
				c1 = ubuf.ut_name[i];
				if (c1 == ' ')
					c1 = 0;
				me[i] = c1;
				if (c1 == 0)
					break;
			}
		}
		if (him[0] == '-' && him[1] == 0)
			goto nomat;
		for (i=0; i<NMAX; i++) {
			c1 = him[i];
			c2 = ubuf.ut_name[i];
			if (c1 == 0)
				if (c2 == 0 || c2 == ' ')
					break;
			if (c1 != c2)
				goto nomat;
		}
		if (histtya && strncmp(histtya, ubuf.ut_line,
		    sizeof(ubuf.ut_line)))
			continue;
		logcnt++;
		if (histty[0]==0 || nomesg && histtya == 0) {
			strcpy(ttybuf, "/dev/");
			strcat(ttybuf, ubuf.ut_line);
			if (histty[0]==0)
				strcpy(histty, ttybuf);
			if (access(ttybuf, 0) < 0 || stat(ttybuf, &stbuf) < 0 ||
			    (stbuf.st_mode&020) == 0)
				nomesg++;
			else {
				strcpy(histty, ttybuf);
				nomesg = 0;
			}
		}
	nomat:
		;
	}
	fclose(uf);
	if (logcnt==0) {
		fprintf(stderr, "write: %s not logged in%s\n", him,
			histtya ? " on that tty" : "");
		exit(1);
	}
	if (histtya==0 && logcnt > 1) {
		fprintf(stderr,
		"write: %s logged in more than once ... writing to %s\n",
			him, histty+5);
	}
cont:
	if (access(histty, 0) < 0) {
		fprintf(stderr, "write: No such tty\n");
		exit(1);
	}
	signal(SIGALRM, timout);
	alarm(5);
	if ((tf = fopen(histty, "w")) == NULL) {
		fprintf(stderr, "write: Permission denied\n");
		exit(1);
	}
	alarm(0);
	sigs(eof);
	{ char hostname[32];
	  gethostname(hostname, sizeof (hostname));
	  fprintf(tf,
	      "\r\nMessage from %s@%s on %s at %d:%02d ...\r\n\007\007\007",
	      me, hostname, mytty, localclock->tm_hour, localclock->tm_min);
	fflush(tf);
	}
	for (;;) {
		char buf[BUFSIZ];
		register char *bp;
		i = read(0, buf, sizeof buf);
		if (i <= 0)
			eof();
		if (buf[0] == '!') {
			buf[i] = 0;
			ex(buf);
			continue;
		}
		for (bp = buf; --i >= 0; bp++) {
			if (*bp == '\n')
				putc('\r', tf);

			if (!isascii(*bp)) {
				putc('M', tf);
				putc('-', tf);
				*bp = toascii(*bp);
			}

			if (isprint(*bp) ||
			    *bp == ' ' || *bp == '\t' || *bp == '\n') {
				putc(*bp, tf);
			} else {
				putc('^', tf);
				putc(*bp ^ 0100, tf);
			}

			if (*bp == '\n')
				fflush(tf);

			if (ferror(tf) || feof(tf)) {
				printf("\n\007Write failed (%s logged out?)\n",
					him);
				exit(1);
			}
		}
	}
}

void
timout()
{

	fprintf(stderr, "write: Timeout opening their tty\n");
	exit(1);
}

void
eof()
{

	fprintf(tf, "EOF\r\n");
	exit(0);
}

ex(bp)
	char *bp;
{
	register i;

	sigs(SIG_IGN);
	i = fork();
	if (i < 0) {
		printf("Try again\n");
		goto out;
	}
	if (i == 0) {
		fclose(tf);		/* Close his terminal */
		setgid(getgid());	/* Give up effective group privs */
		sigs((void (*)())0);
		execl(getenv("SHELL") ?
		    getenv("SHELL") : "/bin/sh", "sh", "-c", bp+1, 0);
		exit(0);
	}
	while (wait((int *)NULL) != i)
		;
	printf("!\n");
out:
	sigs(eof);
}

sigs(sig)
	void (*sig)();
{
	register i;

	for (i=0; signum[i]; i++)
		signal(signum[i], sig);
}
