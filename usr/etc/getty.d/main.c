#ifndef lint
static char *sccsid = "@(#)main.c	4.1      ULTRIX  7/2/90";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1984, 1987, 1989 by		*
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
 ************************************************************************
 *
 *		Modification History
 *
 *	4/28/90 - Kuo-Hsiung Hsieh
 *		Set LPASS8 if the serial line is configed as 8 bits.
 *
 *	6/9/89 - D. A. Long
 *		Do not give world read or write access to tty.  Give write
 *		access to group tty and make owner uucp if line is "shared".
 *
 *	4/1/86 - Modified the reassignment of baud rates in "NX" by
 *		 introducing a new variable "b_rate".	-Tim
 *
 *	1/7/86 - Tim Burke
 *		 Crude flow control mechanism added to ignore start and
 *	 	 stop characters which were otherwise becoming part of the
 *		 username in getname().  This problem would mainfest itself
 *		 on VT100's in smooth scroll - particularly on DMF's.
 *
 *	2/23/87 - Tim Burke
 *		Look to see if "termio" is specified in /etc/ttys which will
 *		cause the line to be opened with System V style defaults.
 *
 *	6/15/87 - Tim Burke (submitting for Andy Gadsby)
 *		Modified putchr() to only set the 8th bit for parity if EP or
 *		OP is specified.
 *
 *      9/15/86 - Fred L. Templin (templin@decvax)
 *                Changed the way getty sets up file descriptors after
 *                the open of the tty succeeds. Previously, it was assumed
 *                that the open would place the control terminal descriptor
 *                in file descriptor slot 0, to be duped into descriptors 1
 *                and 2. Now, since Yellow Pages leaves an open socket desc.
 *                lying around, this assumption is no longer valid.
 *
 *	9/17/87 - Tim Burke
 *		If baud rate sensing is being done, wait 1/4 of a second upon
 *		seeing a break.  This is done to prevent a possible problem
 *		in the DHU device where 2 breaks may be seen if the parity and
 *		bitsize are changed in the middle of a break.
 *
 */

/* static char sccsid[] = "@(#)main.c	1.10 (Berkeley) 86/04/21"; */

/*
 * getty -- adapt to terminal speed on dialup, and call login
 *
 * Melbourne getty, June 83, kre.
 */

#include <sys/ioctl.h>
#include <sgtty.h>
#include <signal.h>
#include <ctype.h>
#include <setjmp.h>
#include <syslog.h>
#include <sys/file.h>
#include "gettytab.h"
#include <errno.h>
#include <ttyent.h>
#include <stdio.h>
#include <sys/time.h>
#include <grp.h>
#include <pwd.h>

extern	char **environ;

struct	sgttyb tmode = {
	0, 0, CERASE, CKILL, 0
};
struct	tchars tc = {
	CINTR, CQUIT, CSTART,
	CSTOP, CEOF, CBRK,
};
struct	ltchars ltc = {
	CSUSP, CDSUSP, CRPRNT,
	CFLUSH, CWERASE, CLNEXT
};

int	crmod;
int	upper;
int	lower;
int	digit;



char	*rindex();
char	hostname[32];
char	name[16];
char	dev[] = "/dev/";
char	ctty[] = "/dev/console";
char	ttyn[32];
char	*tty;
char	*portselector();
char	*ttyname();
int 	shared, local;
int 	first = 1;

#define	OBUFSIZ		128
#define	TABBUFSIZ	512

char	defent[TABBUFSIZ];
char	defstrs[TABBUFSIZ];
char	tabent[TABBUFSIZ];
char	tabstrs[TABBUFSIZ];

char	outbuf[OBUFSIZ];
int	obufcnt = 0;

char	*env[128];

char partab[] = {
	0001,0201,0201,0001,0201,0001,0001,0201,
	0202,0004,0003,0205,0005,0206,0201,0001,
	0201,0001,0001,0201,0001,0201,0201,0001,
	0001,0201,0201,0001,0201,0001,0001,0201,
	0200,0000,0000,0200,0000,0200,0200,0000,
	0000,0200,0200,0000,0200,0000,0000,0200,
	0000,0200,0200,0000,0200,0000,0000,0200,
	0200,0000,0000,0200,0000,0200,0200,0000,
	0200,0000,0000,0200,0000,0200,0200,0000,
	0000,0200,0200,0000,0200,0000,0000,0200,
	0000,0200,0200,0000,0200,0000,0000,0200,
	0200,0000,0000,0200,0000,0200,0200,0000,
	0000,0200,0200,0000,0200,0000,0000,0200,
	0200,0000,0000,0200,0000,0200,0200,0000,
	0200,0000,0000,0200,0000,0200,0200,0000,
	0000,0200,0200,0000,0200,0000,0000,0201
};

#define	ERASE	tmode.sg_erase
#define	KILL	tmode.sg_kill
#define	EOT	tc.t_eofc
#define STOPC	tc.t_stopc
#define STARTC  tc.t_startc

int errno;

jmp_buf timeout;

dingdong()
{

	alarm(0);
	signal(SIGALRM, SIG_DFL);
	longjmp(timeout, 1);
}

jmp_buf	intrupt;

interrupt()
{

	signal(SIGINT, interrupt);
	longjmp(intrupt, 1);
}

int Cons;

main(argc, argv)
	char *argv[];
{
	char *tname;
	long allflags;
	int repcnt = 0;
	int i, fds;
	struct ttyent *t;
	int flags;
	char b_rate[256];


	signal(SIGINT, SIG_IGN);
/*
	signal(SIGQUIT, SIG_DFL);
*/
	openlog("getty", LOG_PID);
	closelog();
	gethostname(hostname, sizeof(hostname));
	if (hostname[0] == '\0')
		strcpy(hostname, "Amnesiac");
	/*
	 * The following is a work around for vhangup interactions which
	 * cause great problems getting window systems started.  If the
	 * 0th argument to getty is '+', we do the old style getty presuming
	 * that the file descriptors are already set up for us. 
	 * J. Gettys - MIT Project Athena.
	 */
	if (argv[0][0] == '+')
		strcpy(ttyn, ttyname(0));
	else {
		struct group *grp;

		strcpy(ttyn, dev);
		strncat(ttyn, argv[2], sizeof(ttyn)-sizeof(dev));
		if(grp=getgrnam("tty"))
			chown(ttyn, 0, grp->gr_gid);
		else
			chown(ttyn, 0, 0);
		tty = rindex(ttyn, '/');
		if (tty == NULL)
			tty = ttyn;
		else
			tty++;
		if ((t = getttynam(tty)) != NULL) {
			initline(t, ttyn);
			local = t->ty_status & TTY_LOCAL; 
			shared = t->ty_status & TTY_SHARED;
		}

		sleep(1);
		if (shared) {
			struct passwd *pwd;

			if(pwd=getpwnam("uucp")) {
				chown(ttyn, pwd->pw_uid, grp->gr_gid);
				chmod(ttyn, 0620);
			} else
				chmod(ttyn, 0666);
			flags = O_RDWR | O_BLKINUSE;
		} 
		else  {
			chmod(ttyn, 0620);
			flags = O_RDWR;
		}
                /*
                 * Brute force the stdin, stdout, stderr descriptors.
                 * "openlog", and other library functions which call
                 * the Yellow Pages can leave an open socket descriptor
                 * in slot 0 otherwise.
                 */
                while ((fds = open(ttyn, flags)) < 0) {
			if (repcnt % 10 == 0) {
				syslog(LOG_NOTICE, "getty: %s: %m", ttyn);
				closelog();
			}
			repcnt++;
			sleep(60);
		}
		signal(SIGHUP, SIG_DFL);
                if ( fds != 0 )
                        dup2(fds, 0);
                dup2(0, 1);
                dup2(1, 2);
		if (shared && ioctl(0, TIOCSINUSE)<0) {
			/* another process grabbed this line - give it up */
			exit(3);
		}
	}

	gettable("default", defent, defstrs);
	gendefaults();
	tname = "default";
	if (argc > 1)
		tname = argv[1];
	for (;;) {
		int ldisp;
		/*
		 * Use SYSVDISC if "termio" was in /etc/ttys line.
		 */
		if (t && (t->ty_status & TTY_TERMIO))
			ldisp = TERMIODISC;
		else
			ldisp = OTTYDISC;

		gettable(tname, tabent, tabstrs);
		if (OPset || EPset || APset)
			APset++, OPset++, EPset++;
		setdefaults();
		ioctl(0, TIOCFLUSH, 0);		/* clear out the crap */
		if (IS)
			tmode.sg_ispeed = speed(IS);
		else if (SP)
			tmode.sg_ispeed = speed(SP);
		if (OS)
			tmode.sg_ospeed = speed(OS);
		else if (SP)
			tmode.sg_ospeed = speed(SP);
		allflags = setflags(0);
		tmode.sg_flags = allflags & 0xffff;	/* flags is a short */
		allflags >>= 16;
		allflags &= LPASS8;
		if(P8)
			allflags |= LPASS8;
		ioctl(0, TIOCLBIS, &allflags);
		ioctl(0, TIOCSETP, &tmode);
		setchars();
		ioctl(0, TIOCSETC, &tc);
		ioctl(0, TIOCSETD, &ldisp);
		if (HC)
			ioctl(0, TIOCHPCL, 0);
		if (AB) {
			extern char *autobaud();

			tname = autobaud();
			continue;
		}
		if (PS) {
			tname = portselector();
			continue;
		}
		if (CL && *CL)
			putpad(CL);
		edithost(HE);
		if (IM && *IM)
			putf(IM);
		if (setjmp(timeout)) {
			tmode.sg_ispeed = tmode.sg_ospeed = 0;
			ioctl(0, TIOCSETP, &tmode);
			exit(1);
		}
		if (TO) {
			signal(SIGALRM, dingdong);
			alarm(TO);
		}
		if (getname()) {
			register int i;

			oflush();
			alarm(0);
			signal(SIGALRM, SIG_DFL);
                        if (name[0] == '-') {	/* close security hole */
                                puts("login names may not start with '-'.");
                                continue;
                        }
			if (!(upper || lower || digit))
				continue;
			allflags = setflags(2);
			tmode.sg_flags = allflags & 0xffff;
			allflags >>= 16;
			if (crmod || NL)
				tmode.sg_flags |= CRMOD;
			if (upper || UC)
				tmode.sg_flags |= LCASE;
			if (lower || LC)
				tmode.sg_flags &= ~LCASE;
			ioctl(0, TIOCSETP, &tmode);
			ioctl(0, TIOCSLTC, &ltc);
			ioctl(0, TIOCLSET, &allflags);
			signal(SIGINT, SIG_DFL);
			for (i = 0; environ[i] != (char *)0; i++)
				env[i] = environ[i];
			makeenv(&env[i]);
			execle(LO, "login", "-p", name, (char *) 0, env);
			exit(1);
		}
		if (shared && ioctl(0, TIOCCINUSE)<0) {
			exit(3);
		}
		alarm(0);
		signal(SIGALRM, SIG_DFL);
		signal(SIGINT, SIG_IGN);
		if (NX && *NX){
                	struct timeval time_delay;
			strcpy(b_rate,NX);
			tname = b_rate;
			/*
			 * Sleep for 1/4 second to let the break sequence pass
			 */
                	time_delay.tv_sec = 0;
                	time_delay.tv_usec = 250000;    /* 250,000 is 1/4 sec */
                	select(0, 0, 0, 0, &time_delay);
		}
	}
}

getname()
{
	register char *np;
	register c;
	char cs;
	int rfds = 0x1;

	/*
	 * Interrupt may happen if we use CBREAK mode
	 */
	if (setjmp(intrupt)) {
		signal(SIGINT, SIG_IGN);
		return (0);
	}
	signal(SIGINT, interrupt);
	tmode.sg_flags = setflags(0);
	ioctl(0, TIOCSETP, &tmode);
	tmode.sg_flags = setflags(1);
	if (!(shared && first && local)) {
		prompt();
		if (PF > 0) {
			oflush();
			sleep(PF);
			PF = 0;
		}
		oflush();
	}
	ioctl(0, TIOCSETP, &tmode);
	if (shared && ioctl(0, TIOCCINUSE)<0) {
		exit(3);
	}
	/*
	 *  wait for data to arrive.  this handles the case
	 *  of hard wired local lines that do not handle RS-232
	 *  signals properly
	 */
	if ( shared && local && select(2, &rfds, 0, 0, 0) < 0) {
		exit(2);
	}
	if (shared && first && local) 
		obufcnt = 0;
/*
		prompt();
		if (PF > 0) {
			oflush();
			sleep(PF);
			PF = 0;
		}
	}
*/
	first = 0;  /* indicate that something has arrived and we can
		     *  put prompt out without waiting for input.
	     	     */
	if (shared && ioctl(0, TIOCSINUSE)<0) {
		/*
		 * another process grabbed this line while we were waiting
		 * for  something to be typed in - give up the line
		 */
		exit(3);
	}
	crmod = 0;
	upper = 0;
	lower = 0;
	digit = 0;
	np = name;
	for (;;) {
		oflush();
		if (read(0, &cs, 1) <= 0) {
			exit(0);
		}
		if ((c = cs&0177) == 0)
			return (0);
		if (c == EOT) {
			exit(1);
		}
		if (c == '\r' || c == '\n' || np >= &name[sizeof name]) {
			putf("\r\n");
			break;
		}
		if (c >= 'a' && c <= 'z')
			lower++;
		else if (c >= 'A' && c <= 'Z')
			upper++;
		else if (c == ERASE || c == '#' || c == '\b') {
			if (np > name) {
				np--;
				if (tmode.sg_ospeed >= B1200)
					puts("\b \b");
				else
					putchr(cs);
			}
			continue;
		} else if (c == KILL || c == '@') {
			putchr(cs);
			putchr('\r');
			if (tmode.sg_ospeed < B1200)
				putchr('\n');
			/* this is the way they do it down under ... */
			else if (np > name)
				puts("                                     \r");
			prompt();
			np = name;
			continue;
		} else if (c >= '0' && c <= '9')
			digit++;
		if (IG && (c <= ' ' || c > 0176))
			continue;
		/*
		 * Throw away flow control characters which therefore are not
		 * valid characters in a username.
		 */
		if ((c == STARTC) || (c == STOPC))
			continue;
		*np++ = c;
		if (EC)
			putchr(cs);
	}
	signal(SIGINT, SIG_IGN);
	*np = 0;
	if (c == '\r')
		crmod++;
	if (upper && !lower && !LC || UC)
		for (np = name; *np; np++)
			if (isupper(*np))
				*np = tolower(*np);
	return (1);
}

static
short	tmspc10[] = {
	0, 2000, 1333, 909, 743, 666, 500, 333, 166, 83, 55, 41, 20, 10, 5, 15
};

putpad(s)
	register char *s;
{
	register pad = 0;
	register mspc10;

	if (isdigit(*s)) {
		while (isdigit(*s)) {
			pad *= 10;
			pad += *s++ - '0';
		}
		pad *= 10;
		if (*s == '.' && isdigit(s[1])) {
			pad += s[1] - '0';
			s += 2;
		}
	}

	puts(s);
	/*
	 * If no delay needed, or output speed is
	 * not comprehensible, then don't try to delay.
	 */
	if (pad == 0)
		return;
	if (tmode.sg_ospeed <= 0 ||
	    tmode.sg_ospeed >= (sizeof tmspc10 / sizeof tmspc10[0]))
		return;

	/*
	 * Round up by a half a character frame,
	 * and then do the delay.
	 * Too bad there are no user program accessible programmed delays.
	 * Transmitting pad characters slows many
	 * terminals down and also loads the system.
	 */
	mspc10 = tmspc10[tmode.sg_ospeed];
	pad += mspc10 / 2;
	for (pad /= mspc10; pad > 0; pad--)
		putchr(*PC);
}

puts(s)
	register char *s;
{

	while (*s)
		putchr(*s++);
}


putchr(cc)
{
	char c;

	c = cc;
	/* only mess with parity if 8-bit disabled and parity enabled */
	if ( !PD && !P8 ) {	
		c |= partab[c&0177] & 0200;
		if (OP)
			c ^= 0200;
	}
	if (!UB) {
		outbuf[obufcnt++] = c;
		if (obufcnt >= OBUFSIZ)
			oflush();
	} else
		write(1, &c, 1);
}

oflush()
{
	if (obufcnt)
		write(1, outbuf, obufcnt);
	obufcnt = 0;
}

prompt()
{

	putf(LM);
	if (CO)
		putchr('\n');
}

putf(cp)
	register char *cp;
{
	char *ttyn, *slash;
	char datebuffer[60];
	extern char editedhost[];
	extern char *ttyname(), *rindex();

	while (*cp) {
		if (*cp != '%') {
			putchr(*cp++);
			continue;
		}
		switch (*++cp) {

		case 't':
			ttyn = ttyname(0);
			slash = rindex(ttyn, '/');
			if (slash == (char *) 0)
				puts(ttyn);
			else
				puts(&slash[1]);
			break;

		case 'h':
			puts(editedhost);
			break;

		case 'd':
			get_date(datebuffer);
			puts(datebuffer);
			break;

		case '%':
			putchr('%');
			break;
		}
		cp++;
	}
}


jmp_buf	noopenbuf;

openfail()
{
	longjmp(noopenbuf,1);
}

initline(t, tty)
	register struct ttyent *t;
	char *tty;
{

	int ttyfd = -1;
	int f, command, ret;
	int perm = 1;
	register int pid;
	if (setjmp(noopenbuf)) {
		syslog(LOG_ERR, "getty: '%s %s' failing, open blocked", t->ty_getty, tty);
		closelog();

		return;
	}


	signal(SIGALRM, openfail);
	alarm(5); /* just in case this device does have O_NDELAY */
#ifdef DEBUG
	if (strncmp(tty, "/dev/tty99", 10) == 0)  {
		syslog(LOG_INFO,"getty: testing");
		closelog();
		while(1);
	}
#endif
	if ((f = open(tty, O_RDWR | O_NDELAY | O_BLKINUSE)) < 0) {

		/*
	  	 * if the line is "in use" then dont touch it.
	 	 */
		if (shared && errno== EWOULDBLOCK)  {
			syslog(LOG_INFO, "getty: in use line %s", tty);
			closelog();
			alarm(0);
			signal(SIGALRM, SIG_DFL);
			return;
		}
		syslog(LOG_ERR,"getty: '%s %s' open failed %m",t->ty_getty,tty);
		closelog();

		alarm(0);
		signal(SIGALRM, SIG_DFL);
		return;
	}
	alarm(0);
	signal(SIGALRM, SIG_DFL);
	command = (t->ty_status & TTY_LOCAL) ? TIOCNMODEM : TIOCMODEM;
	ioctl(f, command, &perm);
	vhangup();
	close(f); 
	if (setpgrp(0,0) < 0) {
		syslog(LOG_INFO,"getty could not set pgrp, %m");
		closelog();
	}
#ifdef notdef
	/* we now have a clean line except for having a controlling terminal
	 * for this process.  We do not want a controlling terminal until
	 * the "real" open succeeds.
	 */
	if ((f = open(tty, O_RDWR | O_NDELAY)) < 0) {
		syslog(LOG_ERR,"getty: '%s %s' reopen failed %m",t->ty_getty,tty);
		closelog();
		return(0);
	}

	if ((ttyfd = open("/dev/tty", O_RDWR | O_NDELAY)) < 0) {
		syslog(LOG_INFO, "getty: could not open /dev/tty, %m");
		closelog();
	}
	if (ttyfd >= 0) {
		if (ioctl(ttyfd, TIOCNOTTY, 0) < 0){
			syslog(LOG_INFO, "getty: ioctl NOTTY %s, %m", tty);
			closelog();
		}
		close(ttyfd);
		ttyfd = -1;
	}
	close(f);
#endif
	
}


