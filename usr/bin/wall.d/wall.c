
# ifndef lint
static char *sccsid = "@(#)wall.c	4.1	(ULTRIX)	7/17/90";
# endif not lint

/************************************************************************
 *									*
 *			Copyright (c) 1985, 1989 by			*
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
/******************************************************************
 * 	Modification History
 *
 *	19-Dec-1986	J. Fries
 *			Added code to insure at least one user
 *			prior to issuing /etc/utmp:bad size
 *			message.
 *
 *	26-Sep-88	D. Long
 *			Fixed missuse of ttyname and ttyslot.  An incorrect
 *			number of arguments were being supplied
 *
 *	15-May-89	G. Sullivan
 *			made sure "clock" is properly declared as an internal
 *			variable.
 *
 *******************************************************************/
/*static char *sccsid = "@(#)wall.c	4.7 (Berkeley) 83/07/01";*/
/*
 * wall.c - Broadcast a message to all users.
 *
 * This program is not related to David Wall, whose Stanford Ph.D. thesis
 * is entitled "Mechanisms for Broadcast and Selective Broadcast".
 */

#include <stdio.h>
#include <utmp.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <signal.h>
#define IGNOREUSER	"sleeper"

char	hostname[32];
char	mesg[3000];
int	msize,sline;
struct	utmp *utmp;
int	nusers;
char	*ttyname();
char	*malloc();
char	*strcpy();
char	*strcat();
char	who[9] = "???";
long	time();
struct tm *localtime();
struct tm *localclock;

main(argc, argv)
char *argv[];
{
	register i;
	register char c;
	register struct utmp *p;
	int f;
	struct stat sb;
	FILE *mf;
	long clock;

	/* get utmp first to save user if there is an error */
	if((f = open("/etc/utmp", 0)) < 0) {
		fprintf(stderr, "Cannot open /etc/utmp\n");
		exit(1);
	}
	/* Get info. about utmp file */
	if (fstat(f,&sb) < 0) {
		fprintf(stderr, "Cannot stat /etc/utmp\n");
		exit(1);
	}
	/* If utmp file size = 0... */
	if (sb.st_size) {
		utmp = (struct utmp *) malloc(sb.st_size);
	}
	else {
		/* If remote requested wall with */
		/* and no users are on system    */
	        if(ttyname(2) == NULL)
		{
			exit(0);
		}
		else
		{
			fprintf(stderr, "/etc/utmp: bad size\n");
			exit(1);
		}
	}
	/* initializ users structure and determine # of users */
	nusers = read(f, (char *)utmp, sb.st_size) / (sizeof(struct utmp));
	(void) close(f);
	gethostname(hostname, sizeof (hostname));
	clock = time( 0 );
	localclock = localtime( &clock );
	mf = stdin;
	if(argc >= 2) {
		/* take message from unix file instead of standard input */
		if((mf = fopen(argv[1], "r")) == NULL) {
			fprintf(stderr,"Cannot open %s\n", argv[1]);
			exit(1);
		}
	}
	while((i = getc(mf)) != EOF) {
		if (msize >= sizeof mesg) {
			fprintf(stderr, "Message too long\n");
			exit(1);
		}
		mesg[msize++] = i;
	}
	fclose(mf);

	sline = ttyslot(); /* 'utmp' slot no. of sender */

	/* construct message */
	if (sline)
		strncpy(who, utmp[sline].ut_name, sizeof(utmp[sline].ut_name));
	for(i=0; i<nusers; i++) {
		p = &utmp[i];
		if ((p->ut_name[0] == 0) ||
		    (strncmp (p->ut_name, IGNOREUSER, sizeof(p->ut_name)) == 0))
			continue;
		sendmes(p->ut_line);
	}
	exit(0);
}

/* Function used by wall to send message */
sendmes(tty)
char *tty;
{
	register i;
	char t[50];
	register char *cp;
	register int c, ch;
	FILE *f;

	while ((i = fork()) == -1)
		if (wait((int *)0) == -1) {
			fprintf(stderr, "Try again\n");
			return;
		}
	if(i)
		return;
	strcpy(t, "/dev/");
	strcat(t, tty);

	signal(SIGALRM, SIG_DFL);	/* blow away if open hangs */
	alarm(10);

	/* open user's terminal */
	if((f = fopen(t, "w")) == NULL) {
		fprintf(stderr,"cannot open %s\n", t);
		exit(1);
	}
	/* output message to terminal */
	fprintf(f,
	    "\nBroadcast Message from %s!%s (%.*s) at %d:%02d ...\r\n\n"
		, hostname
		, who
		, sizeof(utmp[sline].ut_line)
		, utmp[sline].ut_line
		, localclock -> tm_hour
		, localclock -> tm_min
	);
	/* fwrite(mesg, msize, 1, f); */
	for (cp = mesg, c = msize; c-- > 0; cp++) {
		ch = *cp;
		if (ch == '\n')
			putc('\r', f);
		putc(ch, f);
	}
	exit(0);
}
