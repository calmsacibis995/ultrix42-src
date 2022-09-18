#ifndef lint
static char *sccsid = "@(#)stty.c	4.1      ULTRIX  7/2/90";
#endif not lint

/************************************************************************
 *									*
 *			Copyright (c) 1987,88 by			*
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
 *
 *		Modification History
 *
 *	4.14 (Berkeley) 7/6/83"
 *
 * 01	16-Jun-86, Greg Tarsa
 *	Conditionalized code to remove LPASS8 until it is implemented.
 *
 * 02	4-Dec-86, Tim Burke
 *	Moved TIOCLSET so that it will be done before a TIOCSETN call.  The
 *	reason is that TIOCSETN depends on values in TIOCLSET (in device
 *	param routines - specifically in reguards to litout).
 *
 *
 * 03   1-Jun-87, Tim Burke
 *	Check return value of open for errors on a "stty speed" command.
 *
 * 04	24-Feb-87, Tim Burke
 *
 *	Combined Berkeley stty.c with SystemV stty.c in an
 *      effort to provide complete termio support.
 *
 *	Two changes from Ultrix-11.  First is a check on access
 *	permissions and a check on tty type.  Second is the
 *	"excl" and "-excl" options to set terminal to exclusive
 *	use (note: these are not documented).
 *
 * 05   2-Sept-87, Tim Burke
 *	Added autoflow for local mode word.
 *
 * 06	8-Jan-88, Tim Burke
 *	Print out POSIX paramters on 'stty posix'.
 *	A restriction exists for the SVID LOBLK cflag bit.  It is
 *	functionaly the same as POSIX lflag TOSTOP, but can't be
 *	automatically inserted in tty.c because a set and get of 
 *	the POSIX termios data structure would yeild inconsistencies.
 *	This isn't much of a problem becuase LOBLK is not in SVID R2V2.
 *
 * 07	8-Jun-88, Tim Burke
 *	Insure that all ioctls are done on fd 1.
 *
 * 08	20-May-88, Tim Burke
 *	Updated for POSIX Draft 12.3.  Added "iexten" to local mode and
 * 	print out STOP/START chars for POSIX now that they are changable.
 *
 * 09	12-Jun-89, Randall Brown
 *	Changed the '-raw' and 'cooked' modes to set the CRMOD bit.  This
 *	was to correct a problem of doing a 'stty raw' followed by a 
 *	'stty -raw'.  The terminal would not have the CRMOD bit set.
 *
 ************************************************************************/

/**********************************************************************
*								      *
*           The original version of stty.c came from UCB. As  the     *
*      Ultrix  terminal  interface has changed to include SystemV     *
*      compatibility, the stty.c program  has  been  modified  to     *
*      allow  the  passing  of  SystemV stty arguments as well as     *
*      displaying systemV characteristics.     			      *
*     								      *
*           To implement the new functionality the SystemV stty.c     *
*      program  has  been merged with the UCB stty.c program. The     *
*      original UCB stty.c remains mainly intact  in  the  stty_U     *
*      routine.  Similarly  the  former SystemV stty.c is now the     *
*      stty_V routine.  It is important to note  that  these  two     *
*      routines  operate on two different data structures; namely     *
*      the sgtty (UCB) and termio (sysv).  Since  the  previously     *
*      existing  Ultrix ioctl calls have been modified for termio     *
*      capability, changes in one structure should  be  reflected     *
*      in the other.     					      *
*								      *
*           Each argument passed to stty will be examined  as  to     *
*      whether  it should be handled by the stty_U routine or the     *
*      stty_V routine.	 Due to the fact that the     		      *
*      termio and sgtty structures do overlap it is necessary  to     *
*      do  a  "get"  (as in TIOCGETP or TCGETA) prior to changing     *
*      any terminal characteristics (via  TIOCSETA  or  TCSETAW).     *
*      This  is  done  to  prevent one structure from incorrectly     *
*      "stomping over" the other structure.     		      *
*								      *
*			Tim Burke - 5/3/86			      *
*								      *
***********************************************************************/

/*
 * set teletype modes
 */

#include <stdio.h>
#include <sgtty.h>
#include <sys/types.h>
#include <sys/termio.h>
#include <sys/termios.h>

#define ASYNC	0	/* Asynchronous terminal operation */
#define SYNC	1	/* Synchronous terminal - not handled */
#define TRUE 1
#define FALSE 0

/*
 * General flag values to be used within the stty program.
 */
#define STTY_POSIX	1	/* Command was `stty posix` */
unsigned int stty_flag = 0;

extern char *getenv();

/***
 ***	Ultrix mode arguments to stty.  If an stty argument is common
 ***    to both Ultrix and sysv then the Ultrix form of stty will be
 ***    performed.  In so doing the sysv termio structure will be
 ***	indirectly modified due to the addition of termio to Ultrix ioctls.
 ***	To see the effects of changes a TCGETA must be done.
 ***/

struct
{
	char	*string;
	int	speed;
} speeds[] = {			/* common to Ultrix & sysv */
	"0",	B0,
	"50",	B50,
	"75",	B75,
	"110",	B110,
	"134",	B134,
	"134.5",B134,
	"150",	B150,
	"200",	B200,		/* unsupported by multiplexers */
	"300",	B300,
	"600",	B600,
	"1200",	B1200,
	"1800",	B1800,
	"2400",	B2400,
	"4800",	B4800,
	"9600",	B9600,
	"exta",	EXTA,
	"19200", EXTA,
	"extb",	EXTB,
	"38400", EXTB,
	0,
};
struct
{
	char	*string;
	int	set;
	int	reset;
	int	lset;
	int	lreset;
} modes[] = {			       /* Ultrix modes for setting sg_flags */
	"even",		EVENP, 0, 0, 0,
	"-even",	0, EVENP, 0, 0,
	"odd",		ODDP, 0, 0, 0,
	"-odd",		0, ODDP, 0, 0,
	"raw",		RAW, 0, 0, 0,
	"-raw",		CRMOD, RAW, 0, 0,
	"cooked",	CRMOD, RAW, 0, 0,
	"-nl",		CRMOD, 0, 0, 0,
	"nl",		0, CRMOD, 0, 0,
	"echo",		ECHO, 0, 0, 0,
	"-echo",	0, ECHO, 0, 0,
	"LCASE",	LCASE, 0, 0, 0,
	"lcase",	LCASE, 0, 0, 0,
	"-LCASE",	0, LCASE, 0, 0,
	"-lcase",	0, LCASE, 0, 0,
	"-tabs",	XTABS, 0, 0, 0,
	"tabs",		0, XTABS, 0, 0,
	"tandem",	TANDEM, 0, 0, 0,
	"-tandem",	0, TANDEM, 0, 0,
	"cbreak",	CBREAK, 0, 0, 0,
	"-cbreak",	0, CBREAK, 0, 0,
	"cr0",		CR0, CR3, 0, 0,
	"cr1",		CR1, CR3, 0, 0,
	"cr2",		CR2, CR3, 0, 0,
	"cr3",		CR3, CR3, 0, 0,
	"tab0",		TAB0, XTABS, 0, 0,
	"tab1",		TAB1, XTABS, 0, 0,
	"tab2",		TAB2, XTABS, 0, 0,
	"nl0",		NL0, NL3, 0, 0,
	"nl1",		NL1, NL3, 0, 0,
	"nl2",		NL2, NL3, 0, 0,
	"nl3",		NL3, NL3, 0, 0,
	"ff0",		FF0, FF1, 0, 0,
	"ff1",		FF1, FF1, 0, 0,
	"bs0",		BS0, BS1, 0, 0,
	"bs1",		BS1, BS1, 0, 0,
	"33",		CR1, ALLDELAY, 0, 0,
	"tty33",	CR1, ALLDELAY, 0, 0,
	"37",		FF1+CR2+TAB1+NL1, ALLDELAY, 0, 0,
	"tty37",	FF1+CR2+TAB1+NL1, ALLDELAY, 0, 0,
	"05",		NL2, ALLDELAY, 0, 0,
	"vt05",		NL2, ALLDELAY, 0, 0,
	"tn",		CR1, ALLDELAY, 0, 0,
	"tn300",	CR1, ALLDELAY, 0, 0,
	"ti",		CR2, ALLDELAY, 0, 0,
	"ti700",	CR2, ALLDELAY, 0, 0,
	"tek",		FF1, ALLDELAY, 0, 0,
	"crtbs",	0, 0, LCRTBS, LPRTERA,
	"-crtbs",	0, 0, 0, LCRTBS,
	"prterase",	0, 0, LPRTERA, LCRTBS+LCRTKIL+LCRTERA,
	"-prterase",	0, 0, 0, LPRTERA,
	"crterase",	0, 0, LCRTERA, LPRTERA,
	"-crterase",	0, 0, 0, LCRTERA,
	"crtkill",	0, 0, LCRTKIL, LPRTERA,
	"-crtkill",	0, 0, 0, LCRTKIL,
	"tilde",	0, 0, LTILDE, 0,
	"-tilde",	0, 0, 0, LTILDE,
	"litout",	0, 0, LLITOUT, 0,
	"-litout",	0, 0, 0, LLITOUT,
	"pass8",	0, 0, LPASS8, 0,
	"-pass8",	0, 0, 0, LPASS8,
	"tostop",	0, 0, LTOSTOP, 0,
	"-tostop",	0, 0, 0, LTOSTOP,
	"flusho",	0, 0, LFLUSHO, 0,
	"-flusho",	0, 0, 0, LFLUSHO,
	"nohang",	0, 0, LNOHANG, 0,
	"-nohang",	0, 0, 0, LNOHANG,
	"autoflow",	0, 0, LAUTOFLOW, 0,
	"-autoflow",	0, 0, 0, LAUTOFLOW,
	"ctlecho",	0, 0, LCTLECH, 0,
	"-ctlecho",	0, 0, 0, LCTLECH,
	"pendin",	0, 0, LPENDIN, 0,
	"-pendin",	0, 0, 0, LPENDIN,
	"decctlq",	0, 0, LDECCTQ, 0,
	"-decctlq",	0, 0, 0, LDECCTQ,
	"noflsh",	0, 0, LNOFLSH, 0,
	"-noflsh",	0, 0, 0, LNOFLSH,
	0,
};

struct tchars tc;
struct ltchars ltc;
struct sgttyb mode;
struct winsize win;
int	lmode;
int	oldisc, ldisc;

struct	special {			/* Ultrix special characters */
	char	*name;
	char	*cp;
	char	def;
} special[] = {
	"erase",	&mode.sg_erase,		CERASE,
	"kill",		&mode.sg_kill,		CKILL,
	"intr",		&tc.t_intrc,		CINTR,
	"quit",		&tc.t_quitc,		CQUIT,
	"start",	&tc.t_startc,		CSTART,
	"stop",		&tc.t_stopc,		CSTOP,
	"eof",		&tc.t_eofc,		CEOF,
	"brk",		&tc.t_brkc,		CBRK,
	"susp",		&ltc.t_suspc,		CSUSP,
	"dsusp",	&ltc.t_dsuspc,		CDSUSP,
	"rprnt",	&ltc.t_rprntc,		CRPRNT,
	"flush",	&ltc.t_flushc,		CFLUSH,
	"werase",	&ltc.t_werasc,		CWERASE,
	"lnext",	&ltc.t_lnextc,		CLNEXT,
	0
};


/***
 ***	sysv mode arguments to stty
 ***/

struct mds {		/* sysv stty modes */
	char	*string;
	int	set;
	int	reset;
};

					/* sysv control modes */
struct mds cmodes[] = {
	"-parity", CS8, PARENB|CSIZE,
	"-evenp", CS8, PARENB|CSIZE,
	"-oddp", CS8, PARENB|PARODD|CSIZE,
	"parity", PARENB|CS7, PARODD|CSIZE,
	"evenp", PARENB|CS7, PARODD|CSIZE,
	"oddp", PARENB|PARODD|CS7, CSIZE,
	"parenb", PARENB, 0,
	"-parenb", 0, PARENB,
	"parodd", PARODD, 0,
	"-parodd", 0, PARODD,
	"cs8", CS8, CSIZE,
	"cs7", CS7, CSIZE,
	"cs6", CS6, CSIZE,
	"cs5", CS5, CSIZE,
	"cstopb", CSTOPB, 0,
	"-cstopb", 0, CSTOPB,
	"hupcl", HUPCL, 0,
	"-hupcl", 0, HUPCL,
	"clocal", CLOCAL, 0,
	"-clocal", 0, CLOCAL,
	"loblk", LOBLK, 0,
	"-loblk", 0, LOBLK,
	"cread", CREAD, 0,
	"-cread", 0, CREAD,
	"autoflow", TAUTOFLOW, 0,		/* POSIX ONLY */
	"-autoflow", 0, TAUTOFLOW,		/* POSIX ONLY */
	"sane", (CS7|PARENB|CREAD), (CSIZE|PARODD|CLOCAL),
	0
};

				/* sysv input modes */
struct mds imodes[] = {
	"ignbrk", IGNBRK, 0,
	"-ignbrk", 0, IGNBRK,
	"brkint", BRKINT, 0,
	"-brkint", 0, BRKINT,
	"ignpar", IGNPAR, 0,
	"-ignpar", 0, IGNPAR,
	"parmrk", PARMRK, 0,
	"-parmrk", 0, PARMRK,
	"inpck", INPCK, 0,
	"-inpck", 0,INPCK,
	"istrip", ISTRIP, 0,
	"-istrip", 0, ISTRIP,
	"inlcr", INLCR, 0,
	"-inlcr", 0, INLCR,
	"igncr", IGNCR, 0,
	"-igncr", 0, IGNCR,
	"icrnl", ICRNL, 0,
	"-icrnl", 0, ICRNL,
	"iuclc", IUCLC, 0,
	"-iuclc", 0, IUCLC,
	"ixon", IXON, 0,
	"-ixon", 0, IXON,
	"ixany", IXANY, 0,
	"-ixany", 0, IXANY,
	"ixoff", IXOFF, 0,
	"-ixoff", 0, IXOFF,
	"sane", (BRKINT|IGNPAR|ISTRIP|ICRNL|IXON),
		(IGNBRK|PARMRK|INPCK|INLCR|IGNCR|IUCLC|IXOFF),
	0
};

				/* sysv local line modes */
struct mds lmodes[] = {
	"isig", ISIG, 0,
	"-isig", 0, ISIG,
	"icanon", ICANON, 0,
	"-icanon", 0, ICANON,
	"xcase", XCASE, 0,
	"-xcase", 0, XCASE,
	"echo", ECHO, 0,
	"-echo", 0, ECHO,
	"echoe", ECHOE, 0,
	"-echoe", 0, ECHOE,
	"echok", ECHOK, 0,
	"-echok", 0, ECHOK,
	"echonl", ECHONL, 0,
	"-echonl", 0, ECHONL,
	"noflsh", NOFLSH, 0,
	"-noflsh", 0, NOFLSH,
	"ctlech", TCTLECH, 0,			/* POSIX ONLY */
	"-ctlech", 0, TCTLECH,			/* POSIX ONLY */
	"prtera", TPRTERA, 0,			/* POSIX ONLY */
	"-prtera", 0, TPRTERA,			/* POSIX ONLY */
	"crtera", TCRTERA, 0,			/* POSIX ONLY */
	"-crtera", 0, TCRTERA,			/* POSIX ONLY */
	"crtkil", TCRTKIL, 0,			/* POSIX ONLY */
	"-crtkil", 0, TCRTKIL,			/* POSIX ONLY */
	"iexten", IEXTEN, 0,			/* POSIX ONLY */
	"-iexten", 0, IEXTEN,			/* POSIX ONLY */
	"sane", (ISIG|ICANON|ECHO|ECHOK),
		(XCASE|ECHOE|ECHONL|NOFLSH),
	0,
};

				/* sysv output modes */
struct mds omodes[] = {
	"opost", OPOST, 0,
	"-opost", 0, OPOST,
	"olcuc", OLCUC, 0,
	"-olcuc", 0, OLCUC,
	"onlcr", ONLCR, 0,
	"-onlcr", 0, ONLCR,
	"ocrnl", OCRNL, 0,
	"-ocrnl",0, OCRNL,
	"onocr", ONOCR, 0,
	"-onocr", 0, ONOCR,
	"onlret", ONLRET, 0,
	"-onlret", 0, ONLRET,
	"fill", OFILL, OFDEL,
	"-fill", 0, OFILL|OFDEL,
	"nul-fill", OFILL, OFDEL,
	"del-fill", OFILL|OFDEL, 0,
	"ofill", OFILL, 0,
	"-ofill", 0, OFILL,
	"ofdel", OFDEL, 0,
	"-ofdel", 0, OFDEL,
	"nl0", NL0, NLDLY,
	"nl1", NL1, NLDLY,
	"nl2", TNL2, 0,				/* POSIX ONLY */
	"-nl2", 0, TNL2,			/* POSIX ONLY */
	"cr0", CR0, CRDLY,
	"cr1", CR1, CRDLY,
	"cr2", CR2, CRDLY,
	"cr3", CR3, CRDLY,
 	"tab0", TAB0, TABDLY,
 	"tab1", TAB1, TABDLY,
 	"tab2", TAB2, TABDLY,
 	"tab3", TAB3, TABDLY,
	"vt0", VT0, VTDLY,
	"vt1", VT1, VTDLY,
	"bs0", BS0, BSDLY,
	"bs1", BS1, BSDLY,
	"ff0", FF0, FFDLY,
	"ff1", FF1, FFDLY,
	"tilde", TTILDE, 0,			/* POSIX ONLY */
	"-tilde", 0, TTILDE,			/* POSIX ONLY */
	"sane", (OPOST|ONLCR), (OLCUC|OCRNL|ONOCR|ONLRET|OFILL|OFDEL|
			NLDLY|CRDLY|TABDLY|BSDLY|VTDLY|FFDLY),
	0,
};

char	*argV;
int	match;
char	*STTY="stty: ";
char	*USAGE="usage: stty [-ag] [modes]\n";
int	pitVt = 0;
struct termios cb;		/* posix style terminal data structure */
int term;
char	*arg;



int lookedatB = 0;    /* Tells if the arg has been searched in Ultrix stty */
int lookedatV = 0;    /* Has sysv been searched */

int	argc;
char	**argv;

	/* main is a dummy routine to get the stty started.  It first
	 * attempts Ultrix style stty and may transfer to sysv style stty
	 * as the arguments dictate.
	 */

main(margc, margv)
char **margv;
{
	if(ioctl(1, TCGETP, &cb) == -1) {
		perror(STTY);
		exit(2);
	}

	if (cb.c_line == TERMIODISC)
		stty_V(margc, margv);
	else
		stty_U(margc, margv);
}

stty_U(iargc, iargv)
char	**iargv;
{
	int i, uid, gid;
	char *termname;
	register struct special *sp;
	char obuf[BUFSIZ];

	setbuf(stderr, obuf);
	argc = iargc;
	argv = iargv;
	/* Fetch already existing terminal data structures.  This includes
	 * both the Ultrix TIOC calls as well as the sysv TCGETA.
	 */

	ioctl(1, TIOCGETP, &mode);
	ioctl(1, TIOCGETD, &ldisc);
	oldisc = ldisc;
	ioctl(1, TIOCGETC, &tc);
	ioctl(1, TIOCLGET, &lmode);
	ioctl(1, TIOCGLTC, &ltc);
	ioctl(1, TIOCGWINSZ, &win);
	if(ioctl(1, TCGETP, &cb) == -1) {
		perror(STTY);
		exit(2);
	}
	if(argc == 1) {
		prmodes(0);
		exit(0);
	}
	if (argc == 2 && !strcmp(argv[1], "all")) {
		prmodes(1);
		exit(0);
	}
	if (argc == 2 && !strcmp(argv[1], "everything")) {
		prmodes(2);
		exit(0);
	}
	if (argc == 2 && !strcmp(argv[1], "-e")) {
		prmodes(2);
		exit(0);
	}
	if (argc == 2 && !strcmp(argv[1], "posix")) {
		stty_flag |= STTY_POSIX;
		pramodes(2);
		exit(0);
	}
	/* Perform a few simple checks before doing anything */
	if ((uid=getuid(1)) == 0)
		;	/* super-user OK */
	else {
		gid=getgid();
		if (isatty(1) < 0) {
			fprintf(stderr, "Not a tty.\n");
			exit(1);
		}
		if ((termname = (char *)ttyname(1)) == NULL) {
			fprintf(stderr,"Cannot get ttyname\n");
			exit(1);
		}
		if (access(termname, 4) < 0) {
			fprintf(stderr,"%s: Permission denied\n",termname);
			exit(1);
		}
	}
	while(--argc > 0) {
		arg = *++argv;
		if (eq("ek")){
			mode.sg_erase = '#';
			mode.sg_kill = '@';
			continue;
		}
		if (eq("new")){
			ldisc = NTTYDISC;
			if (ioctl(1, TIOCSETD, &ldisc)<0)
				perror("ioctl");
			continue;
		}
		if (eq("termiod")) {
			ldisc = TERMIODISC;
			if (ioctl(1, TIOCSETD, &ldisc)<0)
				perror("ioctl");
			continue;
		}
		if (eq("newcrt")){
			ldisc = NTTYDISC;
			lmode &= ~LPRTERA;
			lmode |= LCRTBS|LCTLECH;
			if (mode.sg_ospeed >= B1200)
				lmode |= LCRTERA|LCRTKIL;
			if (ioctl(1, TIOCSETD, &ldisc)<0)
				perror("ioctl");
			continue;
		}
		if (eq("crt")){
			lmode &= ~LPRTERA;
			lmode |= LCRTBS|LCTLECH;
			if (mode.sg_ospeed >= B1200)
				lmode |= LCRTERA|LCRTKIL;
			continue;
		}
		if (eq("old")){
			ldisc = OTTYDISC;
			if (ioctl(1, TIOCSETD, &ldisc)<0)
				perror("ioctl");
			continue;
		}
		if (eq("disc")){
			printdisc(ldisc,0);
			continue;
		}
		if (eq("dec")){
			mode.sg_erase = 0177;
			mode.sg_kill = CTRL('u');
			tc.t_intrc = CTRL('c');
			ldisc = NTTYDISC;
			lmode &= ~LPRTERA;
			lmode |= LCRTBS|LCTLECH|LDECCTQ;
			if (mode.sg_ospeed >= B1200)
				lmode |= LCRTERA|LCRTKIL;
			if (ioctl(1, TIOCSETD, &ldisc)<0)
				perror("ioctl");
			continue;
		}
		for (sp = special; sp->name; sp++)
			if (eq(sp->name)) {
				if (--argc == 0)
					goto done;
				if (**++argv == 'u')
					*sp->cp = 0377;
				else if (**argv == '^')
					*sp->cp = ((*argv)[1] == '?') ?
					    0177 : (*argv)[1] & 037;
				else
					*sp->cp = **argv;
				goto cont;
			}
		if (eq("gspeed")) {
			mode.sg_ispeed = B300;
			mode.sg_ospeed = B9600;
			continue;
		}
		if (eq("hup")) {
			ioctl(1, TIOCHPCL, NULL);
			continue;
		}
		if (eq("rows")) {
			if (--argc == 0)
				goto done;
			win.ws_row = atoi(*++argv);
			continue;
		}
		if (eq("cols") || eq("columns")) {
			if (--argc == 0)
				goto done;
			win.ws_col = atoi(*++argv);
			continue;
		}
		if (eq("size")) {
			printf("%d %d\n", win.ws_row, win.ws_col);
			continue;
		}
		/* These "excl" ioctls were in Ultrix-11. 
		 * They are used to set and
		 * clear exclusive use of a tty.
		 */
		if (eq("excl")) {
			ioctl(1,TIOCEXCL, NULL);
			continue;
		}
		if (eq("-excl")) {
			ioctl(1,TIOCNXCL, NULL);
			continue;
		}
		for(i=0; speeds[i].string; i++)
			if(eq(speeds[i].string)) {
				mode.sg_ispeed = mode.sg_ospeed = speeds[i].speed;
				goto cont;
			}
		if (eq("speed")) {
			int fd;
			if ((fd = open("/dev/tty",0)) < 0) {
				printf("invalid permission for open\n");
				exit(1);
			}
			ioctl(fd, TIOCGETP, &mode);
			for(i=0; speeds[i].string; i++)
				if (mode.sg_ospeed == speeds[i].speed) {
					printf("%s\n", speeds[i].string);
					exit(0);
				}
			printf("unknown\n");
			exit(1);
		}
		match = 0;
		for(i=0; modes[i].string; i++)
			if(eq(modes[i].string)) {
				mode.sg_flags &= ~modes[i].reset;
				mode.sg_flags |= modes[i].set;
				lmode &= ~modes[i].lreset;
				lmode |= modes[i].lset;
				match = 1;
			}
		if(match == 0){
			/*  This is not a Ultrix argument.  If the sysv
			 *  arguments have been examined then it is truly
			 *  invlaid.  Look at sysv args if we already
			 *  haven't by calling changestateV.
			 */
			if (lookedatV){
			 	fprintf(stderr,"unknown mode: %s\n", arg);
				exit(2);
			}
			else {
				changestateV(argc, argv);
				exit(0);
			}
		}
cont:
		;
		/* Since we are going on to another arg, it hasn't yet been
		 * examined in sysv stty.
		 */
		lookedatV = FALSE;
	}
done:
	ioctl(1, TIOCLSET, &lmode);
	ioctl(1, TIOCSETN, &mode);
	ioctl(1, TIOCSETC, &tc);
	ioctl(1, TIOCSLTC, &ltc);
	ioctl(1, TIOCSWINSZ, &win);
	exit(0);
}

eq(string)
char *string;
{
	int i;

	if(!arg)
		return(0);
	i = 0;
loop:
	if(arg[i] != string[i])
		return(0);
	if(arg[i++] != '\0')
		goto loop;
	arg = 0;
	return(1);
}

prmodes(all)
{
	register m;
	int any;

	if(ldisc==NETLDISC)
		fprintf(stderr, "net discipline, ");
	else if(ldisc==NTTYDISC)
		fprintf(stderr, "new tty, ");
	else if(ldisc==TERMIODISC)
		fprintf(stderr, "termio, ");
	else if(all==2)
		fprintf(stderr, "old tty, ");
	if(mode.sg_ispeed != mode.sg_ospeed) {
		prspeed("input speed ", mode.sg_ispeed);
		prspeed("output speed ", mode.sg_ospeed);
	} else
		prspeed("speed ", mode.sg_ispeed);
	if (all)
		fprintf(stderr, ", %d rows, %d columns", win.ws_row, win.ws_col);
	fprintf(stderr, all==2 ? "\n" : "; ");
	m = mode.sg_flags;
	if(all==2 || (m&(EVENP|ODDP))!=(EVENP|ODDP)) {
		if(m & EVENP)	fprintf(stderr,"even ");
		if(m & ODDP)	fprintf(stderr,"odd ");
		if (!(m&(EVENP|ODDP)))	 fprintf(stderr,"-parity ");
	}
	if(all==2 || m&RAW)
		fprintf(stderr,"-raw "+((m&RAW)!=0));
	if(all==2 || (m&CRMOD)==0)
		fprintf(stderr,"-nl "+((m&CRMOD)==0));
	if(all==2 || (m&ECHO)==0)
		fprintf(stderr,"-echo "+((m&ECHO)!=0));
	if(all==2 || (m&LCASE))
		fprintf(stderr,"-lcase "+((m&LCASE)!=0));
	if(all==2 || (m&TANDEM))
		fprintf(stderr,"-tandem "+((m&TANDEM)!=0));
	fprintf(stderr,"-tabs "+((m&XTABS)!=XTABS));
	if(all==2 || (m&CBREAK))
		fprintf(stderr,"-cbreak "+((m&CBREAK)!=0));
	if(all==2 || (m&NLDELAY))
		delay((m&NLDELAY)/NL1,	"nl");
	if ((m&TBDELAY)!=XTABS)
		delay((m&TBDELAY)/TAB1,	"tab");
	if(all==2 || (m&CRDELAY))
		delay((m&CRDELAY)/CR1,	"cr");
	if(all==2 || (m&VTDELAY))
		delay((m&VTDELAY)/FF1,	"ff");
	if(all==2 || (m&BSDELAY))
		delay((m&BSDELAY)/BS1,	"bs");
	if (all)
		fprintf(stderr,"\n");
#define	lpit(what,str) \
	if (all==2||(lmode&what)) { \
		fprintf(stderr,str+((lmode&what)!=0)); any++; \
	}
	if ((ldisc == NTTYDISC) || (stty_flag & STTY_POSIX)) {
		int newcrt = (lmode&(LCTLECH|LCRTBS)) == (LCTLECH|LCRTBS) &&
		    (lmode&(LCRTERA|LCRTKIL)) ==
		      ((mode.sg_ospeed > B300) ? LCRTERA|LCRTKIL : 0);
		int nothing = 1;
		if (newcrt) {
			if (all==2)
				fprintf(stderr, "crt: (crtbs crterase crtkill ctlecho) ");
			else
				fprintf(stderr, "crt ");
			any++;
		} else {
			lpit(LCRTBS, "-crtbs ");
			lpit(LCRTERA, "-crterase ");
			lpit(LCRTKIL, "-crtkill ");
			lpit(LCTLECH, "-ctlecho ");
			lpit(LPRTERA, "-prterase ");
		}
		lpit(LTOSTOP, "-tostop ");
		if (all==2) {
			fprintf(stderr, "\n");
			any = 0;
			nothing = 0;
		}
		lpit(LTILDE, "-tilde ");
		lpit(LFLUSHO, "-flusho ");
		lpit(LLITOUT, "-litout ");
		lpit(LPASS8, "-pass8 ");
		lpit(LNOHANG, "-nohang ");
		lpit(LAUTOFLOW, "-autoflow ");
		if (any) {
			fprintf(stderr,"\n");
			any = 0;
			nothing = 0;
		}
		lpit(LPENDIN, "-pendin ");
		lpit(LDECCTQ, "-decctlq ");
		lpit(LNOFLSH, "-noflsh ");
		if (any || nothing)
			fprintf(stderr,"\n");
	} else if (!all)
		fprintf(stderr,"\n");
	if (all) {
		switch (ldisc) {

		case OTTYDISC:
			fprintf(stderr,"\
erase  kill   intr   quit   stop   eof\
\n");
			pcol(mode.sg_erase, -1);
			pcol(mode.sg_kill, -1);
			pcol(tc.t_intrc, -1);
			pcol(tc.t_quitc, -1);
			pcol(tc.t_stopc, tc.t_startc);
			pcol(tc.t_eofc, tc.t_brkc);
			fprintf(stderr,"\n");
			break;

		case NTTYDISC:
			fprintf(stderr,"\
erase  kill   werase rprnt  flush  lnext  susp   intr   quit   stop   eof\
\n"); 
			pcol(mode.sg_erase, -1);
			pcol(mode.sg_kill, -1);
			pcol(ltc.t_werasc, -1);
			pcol(ltc.t_rprntc, -1);
			pcol(ltc.t_flushc, -1);
			pcol(ltc.t_lnextc, -1);
			pcol(ltc.t_suspc, ltc.t_dsuspc);
			pcol(tc.t_intrc, -1);
			pcol(tc.t_quitc, -1);
			pcol(tc.t_stopc, tc.t_startc);
			pcol(tc.t_eofc, tc.t_brkc);
			fprintf(stderr,"\n");
			break;
		}
	} else if (ldisc != NETLDISC) {
		register struct special *sp;
		int first = 1;

		for (sp = special; sp->name; sp++) {
			if ((*sp->cp&0377) != (sp->def&0377)) {
				pit(*sp->cp, sp->name, first ? "" : ", ");
				first = 0;
			};
			if (sp->cp == &tc.t_brkc && ldisc == OTTYDISC)
				break;
		}
		if (!first)
			fprintf(stderr, "\n");
	}

}

pcol(ch1, ch2)
	int ch1, ch2;
{
	int nout = 0;

	ch1 &= 0377;
	ch2 &= 0377;
	if (ch1 == ch2)
		ch2 = 0377;
	for (; ch1 != 0377 || ch2 != 0377; ch1 = ch2, ch2 = 0377) {
		if (ch1 == 0377)
			continue;
		if (ch1 & 0200) {
			fprintf(stderr, "M-");
			nout += 2;
			ch1 &= ~ 0200;
		}
		if (ch1 == 0177) {
			fprintf(stderr, "^");
			nout++;
			ch1 = '?';
		} else if (ch1 < ' ') {
			fprintf(stderr, "^");
			nout++;
			ch1 += '@';
		}
		fprintf(stderr, "%c", ch1);
		nout++;
		if (ch2 != 0377) {
			fprintf(stderr, "/");
			nout++;
		}
	}
	while (nout < 7) {
		fprintf(stderr, " ");
		nout++;
	}
}

pit(what, itsname, sep)
	unsigned what;
	char *itsname, *sep;
{

	what &= 0377;
	fprintf(stderr, "%s%s", sep, itsname);
	if (what == 0377) {
		fprintf(stderr, " <undef>");
		return;
	}
	fprintf(stderr, " = ");
	if (what & 0200) {
		fprintf(stderr, "M-");
		what &= ~ 0200;
	}
	if (what == 0177) {
		fprintf(stderr, "^");
		what = '?';
	} else if (what < ' ') {
		fprintf(stderr, "^");
		what += '@';
	}
	fprintf(stderr, "%c", what);
}

delay(m, s)
char *s;
{

	if(m)
		fprintf(stderr,"%s%d ", s, m);
}

int	speed[] = {
	0,50,75,110,134,150,200,300,600,1200,1800,2400,4800,9600,19200,38400
};

prspeed(c, s)
char *c;
{

	fprintf(stderr,"%s%d baud ",  c, speed[s]);
}




stty_V(argVc, argVv)
char	*argVv[];
{
	register i;
	char *ptr;

	term = ASYNC;
	
	if(ioctl(1, TCGETP, &cb) == -1) {
		perror(STTY);
		exit(2);
	}

	if (argVc == 1) {
		prmodesV();
		exit(0);
	}
	if ((argVc == 2) && (argVv[1][0] == '-') && (argVv[1][2] == '\0'))
	switch(argVv[1][1]) {
		case 'a':
			pramodes();
			exit(0);
		case 'g':
			prencode();
			exit(0);
		case 'p':
			stty_flag |= STTY_POSIX;
			pramodes(2);
			exit(0);
		default:
			fprintf(stderr, "%s", USAGE);
			exit(2);
	}
	while(--argVc > 0) {

		argV = *++argVv;
		match = 0;
		if (term == ASYNC) {
			if (eqV("erase") && --argVc)
				cb.c_cc[VERASE] = gct(*++argVv);
			else if (eqV("intr") && --argVc)
				cb.c_cc[VINTR] = gct(*++argVv);
			else if (eqV("quit") && --argVc)
				cb.c_cc[VQUIT] = gct(*++argVv);
			else if (eqV("eof") && --argVc)
				cb.c_cc[VEOF] = gct(*++argVv);
			else if (eqV("min") && --argVc)
				cb.c_cc[VMIN] = gct(*++argVv);
			else if (eqV("eol") && --argVc)
				cb.c_cc[VEOL] = gct(*++argVv);
			else if (eqV("time") && --argVc)
				cb.c_cc[VTIME] = gct(*++argVv);
			else if (eqV("kill") && --argVc)
				cb.c_cc[VKILL] = gct(*++argVv);
			/*
			 * POSIX only extensions to special characters.
			 */
			else if (eqV("susp") && --argVc)
				cb.c_cc[VSUSP] = gct(*++argVv);
			else if (eqV("dsusp") && --argVc)
				cb.c_cc[VDSUSP] = gct(*++argVv);
			else if (eqV("rprnt") && --argVc)
				cb.c_cc[VRPRNT] = gct(*++argVv);
			else if (eqV("flush") && --argVc)
				cb.c_cc[VFLUSH] = gct(*++argVv);
			else if (eqV("werase") && --argVc)
				cb.c_cc[VWERASE] = gct(*++argVv);
			else if (eqV("lnext") && --argVc)
				cb.c_cc[VLNEXT] = gct(*++argVv);
			else if (eqV("quote") && --argVc)
				cb.c_cc[VQUOTE] = gct(*++argVv);
			/* End of POSIX special chars. */
			else if (eqV("ek")) {
				cb.c_cc[VERASE] = CERASE;
				cb.c_cc[VKILL] = CKILL;
			}
			else if (eqV("line") && --argVc)
				cb.c_line = atoi(*++argVv);
			else if(eqV("sane")) {
				cb.c_cc[VERASE] = CERASE;
				cb.c_cc[VKILL] = CKILL;
				cb.c_cc[VQUIT] = CQUIT;
				cb.c_cc[VINTR] = CINTR;
				cb.c_cc[VEOF] = CEOF;
				cb.c_cc[VEOL] = CNUL;
			}
			for(i=0; speeds[i].string; i++)
				if(eqV(speeds[i].string)) {
					cb.c_cflag &= ~CBAUD;
					cb.c_cflag |= speeds[i].speed&CBAUD;
				}
		}
		for(i=0; imodes[i].string; i++)
			if(eqV(imodes[i].string)) {
				cb.c_iflag &= ~imodes[i].reset;
				cb.c_iflag |= imodes[i].set;
			}
		for(i=0; omodes[i].string; i++)
			if(eqV(omodes[i].string)) {
				cb.c_oflag &= ~omodes[i].reset;
				cb.c_oflag |= omodes[i].set;
			}
		for(i=0; cmodes[i].string; i++)
			if(eqV(cmodes[i].string)) {
				cb.c_cflag &= ~cmodes[i].reset;
				cb.c_cflag |= cmodes[i].set;
			}
		for(i=0; lmodes[i].string; i++)
			if(eqV(lmodes[i].string)) {
				cb.c_lflag &= ~lmodes[i].reset;
				cb.c_lflag |= lmodes[i].set;
			}
		if(!match)
			/*  This is not a system V argument.  If the Ultrix
			 *  arguments have been examined then it is truly
			 *  invlaid.  Look at Ultrix args if we already
			 *  haven't by calling changestateB
			 */
			if(!encode(argV)) {
				if (lookedatB){
					fprintf(stderr, "unknown mode: %s\n", argV);
					exit(2);
				}
				else {
					changestateB(argVc, argVv);
					exit(0);
				}
			}
		/* Since we are going on to another argument, it has not yet
		 * been examined in the Ultrix style stty.
		 */
		lookedatB = FALSE;
	}
	if (term == ASYNC) {
		if(ioctl(1, TCSADRAIN, &cb) == -1) {
			perror(STTY);
			exit(2);
		}
	}
	exit(0);
}

eqV(string)
char *string;
{
	register i;

	if(!argV)
		return(0);
	i = 0;
loop:
	if(argV[i] != string[i])
		return(0);
	if(argV[i++] != '\0')
		goto loop;
	match++;
	return(1);
}

prmodesV()
{
	register m;

	if (term == ASYNC) {
		m = cb.c_cflag;
		prspeed("speed ", m&CBAUD);
		if (m&PARENB)
			if (m&PARODD)
				fprintf(stderr, "oddp ");
			else
				fprintf(stderr, "evenp ");
		else
			fprintf(stderr, "-parity ");
		if(((m&PARENB) && !(m&CS7)) || (!(m&PARENB) && !(m&CS8)))
			fprintf(stderr, "cs%c ",'5'+(m&CSIZE)/CS6);
		if (m&CSTOPB)
			fprintf(stderr, "cstopb ");
		if (m&HUPCL)
			fprintf(stderr, "hupcl ");
		if (!(m&CREAD))
			fprintf(stderr, "cread ");
		if (m&CLOCAL)
			fprintf(stderr, "clocal ");
		if ((m&LOBLK) && ((stty_flag & STTY_POSIX) == 0))
			fprintf(stderr, "loblk");
		fprintf(stderr, "\n");
		printdisc(cb.c_line,1);
		if(cb.c_cc[VINTR] != CINTR)
			pitV(cb.c_cc[VINTR], "intr", "; ");
		if(cb.c_cc[VQUIT] != CQUIT)
			pitV(cb.c_cc[VQUIT], "quit", "; ");
		if(cb.c_cc[VERASE] != CERASE)
			pitV(cb.c_cc[VERASE], "erase", "; ");
		if(cb.c_cc[VKILL] != CKILL)
			pitV(cb.c_cc[VKILL], "kill", "; ");
		if(cb.c_cc[VMIN] != CMIN)
			pitV(cb.c_cc[VMIN], "min", "; ");
		if(cb.c_cc[VTIME] != CTIME)
			pitV(cb.c_cc[VTIME], "time", "; ");
		if(cb.c_cc[VEOF] != CEOF)
 			pitV(cb.c_cc[VEOF], "eof", "; ");
 		if(cb.c_cc[VEOL] != CNUL)
 			pitV(cb.c_cc[VEOL], "eol", "; ");
 
		if(pitVt) fprintf(stderr, "\n");
		m = cb.c_iflag;
		if (m&IGNBRK)
			fprintf(stderr, "ignbrk ");
		else if (m&BRKINT)
			fprintf(stderr, "brkint ");
		if (!(m&INPCK))
			fprintf(stderr, "-inpck ");
		else if (m&IGNPAR)
			fprintf(stderr, "ignpar ");
		if (m&PARMRK)
			fprintf(stderr, "parmrk ");
		if (!(m&ISTRIP))
			fprintf(stderr, "-istrip ");
		if (m&INLCR)
			fprintf(stderr, "inlcr ");
		if (m&IGNCR)
			fprintf(stderr, "igncr ");
		if (m&ICRNL)
			fprintf(stderr, "icrnl ");
		if (m&IUCLC)
			fprintf(stderr, "iuclc ");
		if (!(m&IXON))
			fprintf(stderr, "-ixon ");
		else if (!(m&IXANY))
			fprintf(stderr, "-ixany ");
		if (m&IXOFF)
			fprintf(stderr, "ixoff ");
		m = cb.c_oflag;
		if (!(m&OPOST))
			fprintf(stderr, "-opost ");
		else {
		if (m&OLCUC)
			fprintf(stderr, "olcuc ");
		if (m&ONLCR)
			fprintf(stderr, "onlcr ");
		if (m&OCRNL)
			fprintf(stderr, "ocrnl ");
		if (m&ONOCR)
			fprintf(stderr, "onocr ");
		if (m&ONLRET)
			fprintf(stderr, "onlret ");
		if (m&OFILL)
			if (m&OFDEL)
				fprintf(stderr, "del-fill ");
			else
				fprintf(stderr, "nul-fill ");
		delay((m&CRDLY)/CR1, "cr");
		delay((m&NLDLY)/NL1, "nl");
		delay((m&TABDLY)/TAB1, "tab");
		delay((m&BSDLY)/BS1, "bs");
		delay((m&VTDLY)/VT1, "vt");
		delay((m&FFDLY)/FF1, "ff");
		}
		fprintf(stderr, "\n");
		m = cb.c_lflag;
		if (!(m&ISIG))
			fprintf(stderr, "-isig ");
		if (!(m&ICANON))
			fprintf(stderr, "-icanon ");
		if (m&XCASE)
			fprintf(stderr, "xcase ");
		fprintf(stderr, "-echo "+((m&ECHO)!=0));
		fprintf(stderr, "-echoe "+((m&ECHOE)!=0));
		fprintf(stderr, "-echok "+((m&ECHOK)!=0));
		if (m&ECHONL)
			fprintf(stderr, "echonl ");
		if (m&NOFLSH)
			fprintf(stderr, "noflsh ");
		fprintf(stderr, "-tostop "+((m&TOSTOP)!=0));
		fprintf(stderr, "\n");
	}
}

pramodes()
{
	register m;

	if(term == ASYNC) {
		printdisc(cb.c_line,1);
		prspeed("speed ", cb.c_cflag&CBAUD);
		fprintf(stderr, "\n");
		pitV(cb.c_cc[VERASE], "erase", "; ");
		pitV(cb.c_cc[VKILL], "kill", "; ");
		pitV_D(cb.c_cc[VMIN], "min", "; ");
		pitV_D(cb.c_cc[VTIME], "time", "; ");
		pitV(cb.c_cc[VINTR], "intr", "; ");
		pitV(cb.c_cc[VQUIT], "quit", "; ");
		pitV(cb.c_cc[VEOF], "eof", "; ");
		fprintf(stderr, "\n");
 		pitV(cb.c_cc[VEOL], "eol", "; ");
 		pitV(cb.c_cc[VSTART], "start", "; ");
 		pitV(cb.c_cc[VSTOP], "stop", "; ");
		if (stty_flag & STTY_POSIX) {
			fprintf(stderr, "\n");
 			pitV(cb.c_cc[VSUSP], "susp", "; ");
 			pitV(cb.c_cc[VDSUSP], "dsusp", "; ");
 			pitV(cb.c_cc[VRPRNT], "rprnt", "; ");
 			pitV(cb.c_cc[VFLUSH], "flush", "; ");
 			pitV(cb.c_cc[VWERASE], "werase", "; ");
 			pitV(cb.c_cc[VLNEXT], "lnext", "; ");
 			pitV(cb.c_cc[VQUOTE], "quote", "; ");
			fprintf(stderr, "\n");
		}
		else
			fprintf(stderr, "\n");
 
	} 
	m = cb.c_cflag;
	fprintf(stderr, "-parenb "+((m&PARENB)!=0));
	fprintf(stderr, "-parodd "+((m&PARODD)!=0));
	fprintf(stderr, "cs%c ",'5'+(m&CSIZE)/CS6);
	fprintf(stderr, "-cstopb "+((m&CSTOPB)!=0));
	fprintf(stderr, "-hupcl "+((m&HUPCL)!=0));
	fprintf(stderr, "-cread "+((m&CREAD)!=0));
	fprintf(stderr, "-clocal "+((m&CLOCAL)!=0));
	if (stty_flag & STTY_POSIX) 
		fprintf(stderr, "-aflow "+((m&TAUTOFLOW)!=0));
	else	/* SVID only */
		fprintf(stderr, "-loblk "+((m&LOBLK)!=0));

	fprintf(stderr, "\n");
	m = cb.c_iflag;
	fprintf(stderr, "-ignbrk "+((m&IGNBRK)!=0));
	fprintf(stderr, "-brkint "+((m&BRKINT)!=0));
	fprintf(stderr, "-ignpar "+((m&IGNPAR)!=0));
	fprintf(stderr, "-parmrk "+((m&PARMRK)!=0));
	fprintf(stderr, "-inpck "+((m&INPCK)!=0));
	fprintf(stderr, "-istrip "+((m&ISTRIP)!=0));
	fprintf(stderr, "-inlcr "+((m&INLCR)!=0));
	fprintf(stderr, "-igncr "+((m&IGNCR)!=0));
	fprintf(stderr, "-icrnl "+((m&ICRNL)!=0));
	fprintf(stderr, "-iuclc "+((m&IUCLC)!=0));
	fprintf(stderr, "\n");
	fprintf(stderr, "-ixon "+((m&IXON)!=0));
	fprintf(stderr, "-ixany "+((m&IXANY)!=0));
	fprintf(stderr, "-ixoff "+((m&IXOFF)!=0));
	fprintf(stderr, "\n");
	m = cb.c_lflag;
	fprintf(stderr, "-isig "+((m&ISIG)!=0));
	fprintf(stderr, "-icanon "+((m&ICANON)!=0));
	fprintf(stderr, "-xcase "+((m&XCASE)!=0));
	fprintf(stderr, "-echo "+((m&ECHO)!=0));
	fprintf(stderr, "-echoe "+((m&ECHOE)!=0));
	fprintf(stderr, "-echok "+((m&ECHOK)!=0));
	fprintf(stderr, "-echonl "+((m&ECHONL)!=0));
	fprintf(stderr, "-noflsh "+((m&NOFLSH)!=0));
	fprintf(stderr, "\n");
	if (stty_flag & STTY_POSIX) {
		fprintf(stderr, "-iexten "+((m&IEXTEN)!=0));
		fprintf(stderr, "-tostop "+((m&TOSTOP)!=0));
		fprintf(stderr, "-ctlech "+((m&TCTLECH)!=0));
		fprintf(stderr, "-prtera "+((m&TPRTERA)!=0));
		fprintf(stderr, "-crtbs "+((m&TCRTBS)!=0));
		fprintf(stderr, "-crtera "+((m&TCRTERA)!=0));
		fprintf(stderr, "-crtkil "+((m&TCRTKIL)!=0));
		fprintf(stderr, "\n");
	}
	m = cb.c_oflag;
	fprintf(stderr, "-opost "+((m&OPOST)!=0));
	fprintf(stderr, "-olcuc "+((m&OLCUC)!=0));
	fprintf(stderr, "-onlcr "+((m&ONLCR)!=0));
	fprintf(stderr, "-ocrnl "+((m&OCRNL)!=0));
	fprintf(stderr, "-onocr "+((m&ONOCR)!=0));
	fprintf(stderr, "-onlret "+((m&ONLRET)!=0));
	fprintf(stderr, "-ofill "+((m&OFILL)!=0));
	fprintf(stderr, "-ofdel "+((m&OFDEL)!=0));
	if (stty_flag & STTY_POSIX) 
		fprintf(stderr, "-tilde "+((m&TTILDE)!=0));
	delay((m&CRDLY)/CR1, "cr");
	delay((m&NLDLY)/NL1, "nl");
	delay((m&TABDLY)/TAB1, "tab");
	delay((m&BSDLY)/BS1, "bs");
	delay((m&VTDLY)/VT1, "vt");
	delay((m&FFDLY)/FF1, "ff");
	fprintf(stderr, "\n");
}

gct(cp)
register char *cp;
{
	register c;

	c = *cp++;
	if (c == '^') {
		c = *cp;
		if (c == '?')
			c = 0177;
		else if (c == '-')
			c = 0377;
		else
			c &= 037;
	}
	return(c);
}

pitV(what, itsname, sep)
	unsigned char what;
	char *itsname, *sep;
{

	pitVt++;
	fprintf(stderr, "%s", itsname);
	if (what == 0377) {
		fprintf(stderr, " <undef>%s", sep);
		return;
	}
	fprintf(stderr, " = ");
	if (what & 0200) {
		fprintf(stderr, "-");
		what &= ~ 0200;
	}
	if (what == 0177) {
		fprintf(stderr, "DEL%s", sep);
		return;
	} else if (what < ' ') {
		fprintf(stderr, "^");
		what += '`';
	}
	fprintf(stderr, "%c%s", what, sep);
}

pitV_D(what, itsname, sep)
	unsigned char what;
	char *itsname, *sep;
{

	pitVt++;
	fprintf(stderr, "%s", itsname);
	fprintf(stderr, " = ");
	fprintf(stderr, "%d%s", what, sep);
}

printdisc(ldisc,val)
	int ldisc;
	int val;
{
	if (val)
		fprintf(stderr, "line = ");
	switch (ldisc) {
		case OTTYDISC: fprintf(stderr,"OTTYDISC");break;
		case NETLDISC: fprintf(stderr,"NETLDISC");break;
		case NTTYDISC: fprintf(stderr,"NTTYDISC");break;
		case TABLDISC: fprintf(stderr,"TABLDISC");break;
		case NTABLDISC: fprintf(stderr,"NTABLDISC");break;
		case HCLDISC: fprintf(stderr,"HCLDISC");break;
		case TERMIODISC: fprintf(stderr,"TERMIODISC");break;
		case SLPDISC: fprintf(stderr,"SLPDISC");break;
		default: fprintf(stderr, "%d", ldisc);
	}
	if (val)
		fprintf(stderr, "; ");
	else
		fprintf(stderr, "\n");
}

prencode()
{
	fprintf(stderr, "%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x\n",
	cb.c_iflag,cb.c_oflag,cb.c_cflag,cb.c_lflag,cb.c_cc[0],
	cb.c_cc[1],cb.c_cc[2],cb.c_cc[3],cb.c_cc[4],cb.c_cc[5],
	cb.c_cc[6],cb.c_cc[7]);
}

encode(argV)
char *argV;
{
	int grab[12], i;
	i = sscanf(argV, "%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x",
	&grab[0],&grab[1],&grab[2],&grab[3],&grab[4],&grab[5],&grab[6],
	&grab[7],&grab[8],&grab[9],&grab[10],&grab[11]);

	if(i != 12) return(0);

	cb.c_iflag = (ushort) grab[0];
	cb.c_oflag = (ushort) grab[1];
	cb.c_cflag = (ushort) grab[2];
	cb.c_lflag = (ushort) grab[3];

	for(i=0; i<8; i++)
		cb.c_cc[i] = (char) grab[i+4];
	return(1);
}

/*  This routine is called when an arg is encountered which is not a valid
 *  sysv stty argument, OR it is a stty argument common to both
 *  Ultrix and sysv.
 */

changestateB(argVc, argVv)
char *argVv[];
int argVc;
{
	lookedatV = TRUE;
	/* Set sysv stty args processed thusfar */
	if(ioctl(1,TCSADRAIN,&cb) == -1){
		perror(STTY);
		exit(2);
	}
	else
		stty_U(++argVc, --argVv);
}		


/*  This routine is called when an arg is encountered which is not a valid
 *  Ultrix stty argument. 
 */

changestateV(argBc, argBv)
char *argBv[];
int argBc;
{
	lookedatB = TRUE;
	/* Set Ultrix stty args processed thusfar */
	ioctl(1, TIOCLSET, &lmode);
	ioctl(1, TIOCSETN, &mode);
	ioctl(1, TIOCSETC, &tc);
	ioctl(1, TIOCSLTC, &ltc);
	ioctl(1, TIOCSWINSZ, &win);

	stty_V(++argBc,--argBv);
}		

