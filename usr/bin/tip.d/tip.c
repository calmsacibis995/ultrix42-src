/*
 * tip.c
 */
#ifndef lint
static char *sccsid = "@(#)tip.c	4.1      ULTRIX  7/17/90";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1985 by				*
 *		Digital Equipment Corporation, Maynard, MA		*
 *			All rights reserved.				*
 *									*
 *   This software is furnished under a license and may be used and	*
 *   copied  only  in accordance with the terms of such license and	*
 *   with the  inclusion  of  the  above  copyright  notice.   This	*
 *   software  or  any	other copies thereof may not be provided or	*
 *   otherwise made available to any other person.  No title to and	*
 *   ownership of the software is hereby transferred.			*
 *									*
 *   This software is  derived	from  software	received  from	the	*
 *   University    of	California,   Berkeley,   and	from   Bell	*
 *   Laboratories.  Use, duplication, or disclosure is	subject  to	*
 *   restrictions  under  license  agreements  with  University  of	*
 *   California and with AT&T.						*
 *									*
 *   The information in this software is subject to change  without	*
 *   notice  and should not be construed as a commitment by Digital	*
 *   Equipment Corporation.						*
 *									*
 *   Digital assumes no responsibility for the use  or	reliability	*
 *   of its software on equipment which is not supplied by Digital.	*
 *									*
 ************************************************************************/

/*
 * 20-Jul-89	Randall Brown
 *	Added eight bit support.
 *
 * 09-Jun-88	Mark Parenti
 *	Changed signal handlers to void.
 *
 *  6-20-87  Tim Burke
 * Made speed() a global routine so that it can be called from aculib.
 *
 * Added generic dialing support as well as single process mode (via
 * select system call). Systems without select(2) cannot use this feature.
 *
 *	3/12/85 lp@decvax
 *
 * EDIT HISTORY -
 *
 *  14-Jan-1987  Marc Teitelbaum
 *	Added check for '\n' in addition to '\r' to recognize beginning
 *	of line (bol).  If tip is run over script, then script changes
 *	'\r''s into '\n''s and tip wasen't able to go into command mode - 
 *	which isn't good when it comes time to disconnect from a hardwired
 *	port...
 *	
 */

/*
 * tip - UNIX link to other systems
 *  tip [-v] [-speed] system-name
 * or
 *  cu phone-number [-s speed] [-l line] [-a acu]
 */
#include "tip.h"


int	disc = OTTYDISC;		/* tip normally runs this way */
void	intprompt();
void	timeout();
void	cleanup();
char	*sname();
extern  int	speed();
extern char *sprintf();

int	nbits = 16, nfound, *readmask, *writemask = 0, *exceptmask=0;
int	mask, maskin, maskout;
struct	timeval *time0 = 0;
FILE	*phonesfile = (FILE *)NULL;
int	alreadyopen=0;	/* flag for phones file */
extern jmp_buf sigbuf;

main(argc, argv)
	char *argv[];
{
	char *system = NOSTR;
	register int i;
	register char *p;
	char sbuf[12];
	int incnt, outcnt;
	unsigned char gch;
	char bol = 1;

	if ((PH = getenv("PHONES")) == NOSTR) {
		PH = "/etc/phones";
	}
	if (equal(sname(argv[0]), "cu")) {
		cumain(argc, argv);
		cumode = 1;
		goto cucommon;
	}

	if (argc > 4) {
		fprintf(stderr, "usage: tip [-v] [-speed] [system-name]\n");
		exit(1);
	}
	if (!isatty(0)) {
		fprintf(stderr, "tip: must be interactive\n");
		exit(1);
	}

	for (; argc > 1; argv++, argc--) {
		if (argv[1][0] != '-')
			system = argv[1];
		else switch (argv[1][1]) {

		case 'v':
			vflag++;
			break;

		case '0': case '1': case '2': case '3': case '4':
		case '5': case '6': case '7': case '8': case '9':
			BR = atoi(&argv[1][1]);
			break;

		default:
			fprintf(stderr, "tip: %s, unknown option\n", argv[1]);
			break;
		}
	}

	if (system == NOSTR)
		goto notnumber;
	for (p = system; *p; p++)
		if (isalpha(*p))
			goto notnumber;
	PN = system;		/* system name is really a phone number */
	system = sprintf(sbuf, "tip%d", BR);

notnumber:
	signal(SIGINT, cleanup);
	signal(SIGQUIT, cleanup);
	signal(SIGHUP, cleanup);
	signal(SIGTERM, cleanup);

	if ((i = hunt(system)) == 0) {
		printf("all ports busy\n");
		exit(3);
	}
	if (i == -1) {
		printf("link down\n");
		delock(uucplock);
		exit(3);
	}
	setbuf(stdout, NULL);
	loginit();

	/*
	 * We have to be tricky opening the PHONES file.  If the
	 * user has defined an alternate phones file then we don't
	 * want to open it until after we have set uids back. Otherwise,
	 * we must do it now since /etc/phones may not be readable
	 * to the world.
	 *
	 * the next two lines were moved above to work with cu also.
	 */
	/*if ((PH = getenv("PHONES")) == NOSTR) {
		PH = "/etc/phones";
	}*/
	if (strcmp(PH, "/etc/phones") == 0) {
		phonesfile = fopen(PH,"r");	/* check success later */
		alreadyopen = 1;
	}
		
	/*
	 *  Now that we have the logfile, the ACU, and the phones file open,
	 *  return to the real uid and gid.  These things will
	 *  be closed on exit.	Note that we can't run as root,
	 *  because locking mechanism on the tty and the accounting
	 *  will be bypassed.
	 *
	 */
	setgid(getgid());
	setuid(getuid());

	if (!alreadyopen) {
		phonesfile = fopen(PH,"r");	/* check success later */
	}

	/*
	 * Kludge, their's no easy way to get the initialization
	 *   in the right order, so force it here
	 */
	vinit();				/* init variables */
	setparity("even");			/* set the parity table */
	if ((i = speed(number(value(BAUDRATE)))) == NULL) {
		printf("tip: bad baud rate %d\n", number(value(BAUDRATE)));
		delock(uucplock);
		exit(3);
	}

	/*
	 * Hardwired connections require the
	 *  line speed set before they make any transmissions
	 *  (this is particularly true of things like a DF03-AC)
	 */
	if (HW)
		ttysetup(i);
	if (p = connect()) {
		printf("\07%s\n[EOT]\n", p);
		delock(uucplock);
		exit(1);
	}
	if (!HW)
		ttysetup(i);
cucommon:
	/*
	 * From here down the code is shared with
	 * the "cu" version of tip.
	 */
	ioctl(0, TIOCGETP, (char *)&defarg);
	ioctl(0, TIOCGETC, (char *)&defchars);
	ioctl(0, TIOCGLTC, (char *)&deflchars);
	ioctl(0, TIOCGETD, (char *)&odisc);
	arg = defarg;
	arg.sg_flags = ANYP | CBREAK;
	tchars = defchars;
	tchars.t_intrc = tchars.t_quitc = -1;
	ltchars = deflchars;
	ltchars.t_suspc = ltchars.t_dsuspc = ltchars.t_flushc
		= ltchars.t_lnextc = -1;
	raw();

	signal(SIGALRM, timeout);

	/*
	 * Everything's set up now:
	 *	connection established (hardwired or diaulup)
	 *	line conditioned (baud rate, mode, etc.)
	 *	internal data structures (variables)
	 * setup select to wakeup when input on local or remote
	 * line is seen.
	 */
	printf(cumode ? "Connected\r\n" : "\07connected (%s)\r\n", DV);
	/*
	 * Possibly turn scripting on
	 */
		tipinit();

	/*
	 * Set up tipout signalling
	 */
		tipoutinit();

	/*
	 * Get pid of this process as this is how we synchronize
	 * signalling.
	 */
		pid = getpid();

	/*
	 * Setup 2 masks for stdin (0) & for the output line (FD)
	 */

	maskin = (1<<0);
	maskout = (1<<FD);

	while(1) {
	    mask = maskin|maskout;
	    readmask = &mask;

	    (void) setjmp(sigbuf);  /* Where we come on after some signals */

	    nfound = select(nbits, readmask, writemask, exceptmask, time0);

	    if(*readmask == maskout || *readmask == (maskin|maskout)) {
		tipout();
	    }

	    if(*readmask == maskin || *readmask == (maskin|maskout)) {
		if(incnt <= 0) {
			(void) ioctl(0, FIONREAD, (caddr_t) &incnt);
		}
		while(incnt--) {
		    read(0,&gch,1);
		    if (!boolean(value(EIGHTBIT)))
			gch &= 0177;
		    if ((gch == character(value(ESCAPE))) && bol) {
			if (!(gch = escape()))
				break;
		    } else if (!cumode && gch == character(value(RAISECHAR))){
			boolean(value(RAISE)) = !boolean(value(RAISE));
				break;
		    } else if (gch == '\r' || gch == '\n') {  /* marc */
			bol = 1;
					      /* keep lock file fresh so */
			chmod(uucplock,0444); /* uucp does not remove it.*/
			pwrite(FD, &gch, 1);
			if (boolean(value(HALFDUPLEX)))
				printf("\r\n");
			break;
		    } else if (!cumode && gch == character(value(FORCE))) {
			gch = getchar();
			if (!boolean(value(EIGHTBIT)))
			    gch &= 0177;
		    }
		    bol = any(gch, value(EOL));
		    if (boolean(value(RAISE)) && islower(gch))
			gch = toupper(gch);
		    pwrite(FD, &gch, 1);
		    if (boolean(value(HALFDUPLEX)))
			printf("%c", gch);
		}
	    }
	}

	/*NOTREACHED*/
}

void
cleanup()
{

	delock(uucplock);
	if (odisc)
		ioctl(0, TIOCSETD, (char *)&odisc);
	exit(0);
}

/*
 * put the controlling keyboard into raw mode
 */
raw()
{

	ioctl(0, TIOCSETP, &arg);
	ioctl(0, TIOCSETC, &tchars);
	ioctl(0, TIOCSLTC, &ltchars);
	ioctl(0, TIOCSETD, (char *)&disc);
}


/*
 * return keyboard to normal mode
 */
unraw()
{

	ioctl(0, TIOCSETD, (char *)&odisc);
	ioctl(0, TIOCSETP, (char *)&defarg);
	ioctl(0, TIOCSETC, (char *)&defchars);
	ioctl(0, TIOCSLTC, (char *)&deflchars);
}

static	jmp_buf promptbuf;

/*
 * Print string ``s'', then read a string
 *  in from the terminal.  Handles signals & allows use of
 *  normal erase and kill characters.
 */
prompt(s, p)
	char *s;
	register char *p;
{
	register char *b = p;
	void (*oint)(), (*oquit)();

	stoprompt = 0;
	oint = signal(SIGINT, intprompt);
	oint = signal(SIGQUIT, SIG_IGN);
	unraw();
	printf("%s", s);
	if (setjmp(promptbuf) == 0)
		while ((*p = getchar()) != EOF && *p != '\n')
			p++;
	*p = '\0';

	raw();
	signal(SIGINT, oint);
	signal(SIGQUIT, oint);
	return (stoprompt || p == b);
}

/*
 * Interrupt service routine during prompting
 */
void
intprompt()
{

	signal(SIGINT, SIG_IGN);
	stoprompt = 1;
	printf("\r\n");
	longjmp(promptbuf, 1);
}

tipinit() {

	if (boolean(value(SCRIPT))) {
		sleep(1);
		setscript();
	}
}

/*
 * Escape handler --
 *  called on recognition of ``escapec'' at the beginning of a line
 */
escape()
{
	register char gch;
	register esctable_t *p;
	char c = character(value(ESCAPE));
	extern esctable_t etable[];

	gch = (getchar()&0177);
	for (p = etable; p->e_char; p++)
		if (p->e_char == gch) {
			if ((p->e_flags&PRIV) && getuid())
				continue;
			printf("%s", ctrl(c));
			(*p->e_func)(gch);
			return (0);
		}
	/* ESCAPE ESCAPE forces ESCAPE */
	if (c != gch)
		pwrite(FD, &c, 1);
	return (gch);
}


any(c, p)
	register char c, *p;
{
	while (p && *p)
		if (*p++ == c)
			return (1);
	return (0);
}

size(s)
	register char	*s;
{
	register int i = 0;

	while (s && *s++)
		i++;
	return (i);
}

char *
interp(s)
	register char *s;
{
	static char buf[256];
	register char *p = buf, c, *q;

	while (c = *s++) {
		for (q = "\nn\rr\tt\ff\033E\bb"; *q; q++)
			if (*q++ == c) {
				*p++ = '\\'; *p++ = *q;
				goto next;
			}
		if (c < 040) {
			*p++ = '^'; *p++ = c + 'A'-1;
		} else if (c == 0177) {
			*p++ = '^'; *p++ = '?';
		} else
			*p++ = c;
	next:
		;
	}
	*p = '\0';
	return (buf);
}

char *
ctrl(c)
	char c;
{
	static char s[3];

	if (c < 040 || c == 0177) {
		s[0] = '^';
		s[1] = c == 0177 ? '?' : c+'A'-1;
		s[2] = '\0';
	} else {
		s[0] = c;
		s[1] = '\0';
	}
	return (s);
}

/*
 * Help command
 */
help(c)
	char c;
{
	register esctable_t *p;
	extern esctable_t etable[];

	printf("%c\r\n", c);
	for (p = etable; p->e_char; p++) {
		if ((p->e_flags&PRIV) && getuid())
			continue;
		printf("%2s", ctrl(character(value(ESCAPE))));
		printf("%-2s %c   %s\r\n", ctrl(p->e_char),
			p->e_flags&EXP ? '*': ' ', p->e_help);
	}
}

/*
 * Set up the "remote" tty's state
 */
ttysetup(speed)
	int speed;
{
	unsigned bits = LDECCTQ;

	arg.sg_ispeed = arg.sg_ospeed = speed;
	arg.sg_flags = RAW;
	if (boolean(value(TAND)))
		arg.sg_flags |= TANDEM;
	ioctl(FD, TIOCSETP, (char *)&arg);
	ioctl(FD, TIOCLBIS, (char *)&bits);
}

/*
 * Return "simple" name from a file name,
 * strip leading directories.
 */
char *
sname(s)
	register char *s;
{
	register char *p = s;

	while (*s)
		if (*s++ == '/')
			p = s;
	return (p);
}

static char partab[0200];

/*
 * Do a write to the remote machine with the correct parity.
 * We are doing 8 bit wide output, so we just generate a character
 * with the right parity and output it.
 *
 * If EIGHTBIT is set don't put parity bit in to chars.
 */
pwrite(fd, buf, n)
	int fd;
	char *buf;
	register int n;
{
	register int i;
	register char *bp;

	if (!boolean(value(EIGHTBIT))) {
	    bp = buf;
	    for (i = 0; i < n; i++) {
		*bp = partab[(*bp) & 0177];
		bp++;
	    }
	}
	write(fd, buf, n);
}

/*
 * Build a parity table with appropriate high-order bit.
 */
setparity(defparity)
	char *defparity;
{
	register int i;
	char *parity;
	extern char evenpartab[];

	if (value(PARITY) == NOSTR)
		value(PARITY) = defparity;
	parity = value(PARITY);
	for (i = 0; i < 0200; i++)
		partab[i] = evenpartab[i];
	if (equal(parity, "even"))
		return;
	if (equal(parity, "odd")) {
		for (i = 0; i < 0200; i++)
			partab[i] ^= 0200;	/* reverse bit 7 */
		return;
	}
	if (equal(parity, "none") || equal(parity, "zero")) {
		for (i = 0; i < 0200; i++)
			partab[i] &= ~0200;	/* turn off bit 7 */
		return;
	}
	if (equal(parity, "one")) {
		for (i = 0; i < 0200; i++)
			partab[i] |= 0200;	/* turn on bit 7 */
		return;
	}
	fprintf(stderr, "%s: unknown parity value\n", PA);
	fflush(stderr);
}
