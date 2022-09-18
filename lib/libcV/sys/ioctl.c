#ifndef lint
static	char	*sccsid = "@(#)ioctl.c	4.1	(ULTRIX)	7/3/90";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1985-1990 by			*
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
 *	David L Ballenger, 28-Mar-1985					*
 * 0001	Modify for use with merged ULTRIX/SYSTEM-V header files.	*
 *									*
 *	Tim Burke, 07-Dec-1987						*
 *	No need to map the termio ioctl calls with pack() and unpack().	*
 *	The ioctl calls can be nade directly.				*
 *									*
 *	Tim Burke, 04-Feb-1988						*
 *	Fixed up handling of tabs in pack and unpack.			*
 *									*
 *	Grant Sullivan, 16-Jan-1990					*
 *	conditionalized word struct so MIPSEL (includes mips) is the	*
 *	  same as "vax"							*
 ************************************************************************/

/*
	ioctl -- system call emulation for 4.2BSD

	last edit:	28-Dec-1984	D A Gwyn

	Because there is not a 1-1 mapping between Bell and Berkeley
	terminal driver modes, some flag bits have an "adjusted" meaning
	in an attempt to provide improved mapping reversibility.

	Special note:  sg_flags is an int, not a short, and it contains
	both the standard sgttyb flags and Berkeley's added local flags.

	Beware!  Setting NOFLSH in c_flag will set ALL the Berkeley
	local flags unless you have fixed this bug in the tty driver.
	(On 4.1cBSD, there is a similar problem with BSDLY.)
*/

/* Make sure this is compiled for ULTRIX System V emulation.
 */
#ifndef SYSTEM_FIVE
#define SYSTEM_FIVE
#endif  SYSTEM_FIVE

#include	<errno.h>
#include	<sys/termio.h>
#include	<sys/ttold.h>
#include	<sys/time.h>

/*	differing 4.2BSD sg_flag bits:	*/

#define X_TANDEM	0x00000001	/* automatic flow control */
#define X_CBREAK	0x00000002	/* half-cooked mode */
#define X_TBDELAY	0x00000c00	/* tab delay code: */
#define X_XTABS 	0x00000c00	/* map tabs to spaces */

/*	added 4.2BSD sg_flag bits:	*/

#define X_CRTBS 	0x00010000	/* fancy BS erase */
#define X_PRTERA	0x00020000	/* \.../ erase */
#define X_CRTERA	0x00040000	/* BS-SP-BS erase */
#define X_TILDE 	0x00080000	/* Hazeltine kludge */
#define X_MDMBUF	0x00100000	/* DTR stall kludge */
#define X_LITOUT	0x00200000	/* literal output */
#define X_TOSTOP	0x00400000	/* SIGSTOP on bkgnd output */
#define X_FLUSHO	0x00800000	/* set by ^O */
#define X_NOHANG	0x01000000	/* no SIGHUP on hangup */
#define X_ETXACK	0x02000000	/* ETX->ACK protocol */
#define X_CRTKIL	0x04000000	/* BS-SP-BS kill */
#define X_INTRUP	0x08000000	/* SIGTINT when input ready */
#define X_CTLECH	0x10000000	/* echo ctrl-X as "^X" */
#define X_PENDIN	0x20000000	/* reread raw queue */
#define X_DECCTQ	0x40000000	/* strict DC3/DC1 protocol */
#define X_NOFLSH	0x80000000	/* no output flush on signal */

/*	Kludge for accessing "local flags" part of sg_flags:	*/

#if vax | MIPSEL /* VAX and RISC MISPEL machines have same order */
typedef struct
	{
	short	low;			/* low half (standard flags) */
	short	high;			/* high half (local flags) */
	}	word;			/* map onto sg_flags */
#else	/* Gould, etc. */
typedef struct
	{
	short	high;			/* high half (local flags) */
	short	low;			/* low half (standard flags) */
	}	word;			/* map onto sg_flags */
#endif

extern int	_ioctl(), select();

static void	nap(), new_tty(), unfudge();
static int	fudge(), get_sgttyb(), pack(), set_sgttyb(), unpack();


int
ioctl( fildes, request, arg )		/* returns 0 if ok, else -1 */
	int		fildes; 	/* file descriptor */
	int		request;	/* command */
	int		arg;		/* command arguments */
	{
	struct sgttyb	tb;		/* [gs]tty values */

	switch ( request )
		{
	case TIOCGETP:
		new_tty( fildes );
		if ( get_sgttyb( fildes, (struct sgttyb *)arg ) < 0 )
			return -1;	/* errno already set */
		unfudge( (struct sgttyb *)arg );
		return 0;

	case TIOCSETP:
		new_tty( fildes );
		tb = *(struct sgttyb *)arg;	/* local copy */
		if ( fudge( fildes, &tb ) < 0
		  || set_sgttyb( fildes, &tb, 1 ) < 0
		   )
			return -1;	/* errno already set */
		return 0;

		/*
	 	 * It is no longer to map the termio ioctl calls.  They can be
	 	 * made directly.
	 	 */
	default:
		return _ioctl( fildes, request, (char *)arg );
		}
	}


static void
new_tty( fildes )		/* make sure new tty handler is used */
	{
	static int	ldisc = OTTYDISC;	/* line discipline */

	if ( ldisc != NTTYDISC		/* first time this process */
	  && (ioctl( fildes, TIOCGETD, &ldisc ) != 0	/* unknown */
	   || ldisc != NTTYDISC		/* known but not "new tty" */
	     )
	   )	{
		ldisc = NTTYDISC;	/* force new tty handler */
		(void)_ioctl( fildes, TIOCSETD, &ldisc );
		}
	}


/*	I used to take a really ugly efficiency shortcut in the next
	two routines, but the Gould byte order put a stop to that.   */

static int
get_sgttyb( fildes, tbp )		/* extended gtty */
	int			fildes; /* file descriptor */
	register struct sgttyb	*tbp;	/* -> where to put data */
	{
	int			lf;	/* local flags */
	struct sgttyb_ULTRIX	xb;	/* native data */

	if ( _ioctl( fildes, TIOCGETP, (char *)&xb ) < 0 )
		return -1;		/* errno already set */
	tbp->sg_ispeed = xb.sg_ispeed;
	tbp->sg_ospeed = xb.sg_ospeed;
	tbp->sg_erase = xb.sg_erase;
	tbp->sg_kill = xb.sg_kill;
	((word *)&tbp->sg_flags)->low = xb.sg_flags;

	if ( _ioctl( fildes, TIOCLGET, (char *)&lf ) < 0 )
		return -1;		/* errno already set */
	((word *)&tbp->sg_flags)->high = (short)lf;

	return 0;
	}


static int
set_sgttyb( fildes, tbp, wait ) 	/* extended stty */
	int			fildes; /* file descriptor */
	register struct sgttyb	*tbp;	/* -> data to be set */
	int			wait;	/* "wait for output to drain" */
{
	int			lf;	/* local flags */
	struct sgttyb_ULTRIX	xb;	/* native data */

	xb.sg_ispeed = tbp->sg_ispeed;
	xb.sg_ospeed = tbp->sg_ospeed;
	xb.sg_erase = tbp->sg_erase;
	xb.sg_kill = tbp->sg_kill;
	xb.sg_flags = ((word *)&tbp->sg_flags)->low;
	if ( _ioctl( fildes, wait != 0 ? TIOCSETP : TIOCSETN,
		     (char *)&xb
		   ) < 0
	   )
		return -1;		/* errno already set */

	lf = (int)((word *)&tbp->sg_flags)->high;
	if ( _ioctl( fildes, TIOCLSET, (char *)&lf ) < 0 )
		return -1;		/* errno already set */

	return 0;
}


static int
fudge( fildes, tbp )			/* map Sys V stty to 4.2BSD */
	int			fildes; /* file descriptor */
	register struct sgttyb	*tbp;	/* -> data about to be set */
	{
	if ( (tbp->sg_flags & O_XTABS) != 0 )
		tbp->sg_flags |= X_XTABS;
	if ( (tbp->sg_flags & O_HUPCL) != 0
	  && _ioctl( fildes, TIOCHPCL, (char *)0 ) < 0
	   )
		return -1;		/* errno already set */
	tbp->sg_flags &= ~(O_XTABS | O_HUPCL);
	return 0;
	}


static void
unfudge( tbp )				/* map 4.2BSD gtty to Sys V */
	register struct sgttyb	*tbp;	/* -> data just gotten */
	{
	if ( (tbp->sg_flags & X_CBREAK) != 0 )
		tbp->sg_flags |= O_RAW; /* approximation */
	tbp->sg_flags &= ~(X_CBREAK | X_TANDEM);
	if ( (tbp->sg_flags & X_TBDELAY) == X_XTABS )
		{
		tbp->sg_flags &= ~X_TBDELAY;
		tbp->sg_flags |= O_XTABS;
		}
	else if ( (tbp->sg_flags & X_TBDELAY) != 0 )
		{
		tbp->sg_flags |= O_TBDELAY;
		tbp->sg_flags &= ~O_NOAL;
		}
	}


static int
pack( fildes, argp, tbp )		/* map termio to 4.2BSD stty */
	int			fildes; /* file descriptor */
	register struct termio	*argp;	/* -> desired state info */
	register struct sgttyb	*tbp;	/* -> stty buffer */
	{
	register int		flag;	/* holds sg_flags */
	struct tchars		tc;	/* 4.2BSD magic characters */
	struct ltchars		ltc;	/* more 4.2BSD magic chars */

	if ( (argp->c_lflag & ICANON) != 0 )	/* no MIN, TIME */
		{
		if ( (tc.t_eofc = argp->c_cc[VEOF]) == CNUL )
			tc.t_eofc = (char)-1;
		if ( (tc.t_brkc = argp->c_cc[VEOL]) == CNUL )
			tc.t_brkc = (char)-1;
		}
	else if ( _ioctl( fildes, TIOCGETC, (char *)&tc ) < 0 )
		return -1;		/* errno already set */
	if ( (argp->c_lflag & (ICANON | ISIG)) == ICANON )
		tc.t_intrc = tc.t_quitc = (char)-1;	/* disable */
	else	{
		if ( (tc.t_intrc = argp->c_cc[VINTR]) == CNUL )
			tc.t_intrc = (char)-1;
		if ( (tc.t_quitc = argp->c_cc[VQUIT]) == CNUL )
			tc.t_quitc = (char)-1;
		}
	if ( (argp->c_iflag & IXON) == 0 )
		tc.t_startc = tc.t_stopc = (char)-1;	/* disable */
	else	{
		tc.t_startc = CSTART;
		tc.t_stopc = CSTOP;
		}
	if ( _ioctl( fildes, TIOCSETC, (char *)&tc ) < 0 )
		return -1;		/* errno already set */

	if ( _ioctl( fildes, TIOCGLTC, (char *)&ltc ) == 0 )
		{			/* new tty handler */		
		if ( (ltc.t_suspc = argp->c_cc[VSWTCH]) == CNUL )
			ltc.t_suspc = (char)-1;
		if ( _ioctl( fildes, TIOCSLTC, (char *)&ltc ) < 0 )
			return -1;	/* errno already set */
		}

	if ( (argp->c_cflag & HUPCL) != 0
	  && _ioctl( fildes, TIOCHPCL, (char *)0 ) < 0
	   )
		return -1;		/* errno already set */

	tbp->sg_erase = argp->c_cc[VERASE];
	tbp->sg_kill = argp->c_cc[VKILL];
	tbp->sg_ispeed = tbp->sg_ospeed = argp->c_cflag & CBAUD;

	flag = X_CTLECH;		/* everybody gets this */

	if ( (argp->c_lflag & ICANON) == 0 )
		flag |= O_RAW;
	if ( (argp->c_lflag & XCASE) != 0 )
		flag |= O_LCASE;
	if ( (argp->c_lflag & ECHO) != 0 )
		flag |= O_ECHO;
	if ( (argp->c_lflag & ECHOE) != 0 )
		{
		flag |= X_CRTBS;
		if ( tbp->sg_ospeed >= B1200 )
			flag |= X_CRTERA | X_CRTKIL;
		}
	else
		flag |= X_PRTERA;
#ifndef gould	/* at least for now */
	if ( (argp->c_lflag & NOFLSH) != 0 )
		flag |= X_NOFLSH;
#endif
	if ( (argp->c_cflag & PARODD) != 0 )
		flag |= O_ODDP;
	else if ( (argp->c_iflag & INPCK) != 0 )
		flag |= O_EVENP;
	else
		flag |= O_ODDP | O_EVENP;
	if ( (argp->c_cflag & CLOCAL) != 0 )
		flag |= X_MDMBUF | X_NOHANG;
	/* The following is done even if OPOST is off, to keep track: */
	if ( (argp->c_oflag & ONLCR) != 0 )
		{
		flag |= O_CRMOD;
		if ( (argp->c_oflag & CRDLY) == CR1 )
			flag |= O_NL1;	/* sorry `bout that */
		else if ( (argp->c_oflag & CRDLY) == CR2 )
			flag |= O_CR1;	/* approximation */
		else if ( (argp->c_oflag & CRDLY) != 0 )
			flag |= O_CR2;	/* approximation to CR3 */
		}
	else if ( (argp->c_oflag & ONLRET) != 0 )
		{
		if ( (argp->c_oflag & CR2) != 0 )	/* CR2 or CR3 */
			flag |= O_NL2;
		else if ( (argp->c_oflag & CR1) != 0 )	/* CR1 */
			flag |= O_NL1;
		}
	else
		if ( (argp->c_oflag & NLDLY) != 0 )
			flag |= O_NL2;
	flag |= (long)((argp->c_oflag & TABDLY) );
	if ( (argp->c_oflag & (VTDLY | FFDLY)) != 0 )
		flag |= O_VTDELAY;
#ifndef gould	/* at least for now */
	if ( (argp->c_oflag & BSDLY) != 0 )
		flag |= O_BSDELAY;
#endif
	if ( (argp->c_iflag & (IXON | IXANY)) == IXON )
		flag |= X_DECCTQ;
	if ( (argp->c_iflag & IXOFF) != 0 )
		flag |= X_TANDEM;

	tbp->sg_flags = flag;

	return 0;
	}


static int
unpack( fildes, tbp, argp )		/* map 4.2BSD gtty to termio */
	int			fildes; /* file descriptor */
	struct sgttyb		*tbp;	/* -> gtty buffer */
	register struct termio	*argp;	/* where to put unpacking */
	{
	struct tchars		tc;	/* 4.2BSD magic characters */
	struct ltchars		ltc;	/* more 4.2BSD magic chars */
	register int		flag = tbp->sg_flags;	/* for speed */

	if ( _ioctl( fildes, TIOCGETC, (char *)&tc ) < 0 )
		return -1;		/* errno already set */
	argp->c_cc[VERASE] = tbp->sg_erase;
	argp->c_cc[VKILL] = tbp->sg_kill;
	if ( tc.t_intrc == (char)-1 && tc.t_quitc == (char)-1 )
		{			/* assume defaults */
		argp->c_cc[VINTR] = CINTR;
		argp->c_cc[VQUIT] = CQUIT;
		argp->c_lflag = 0;	/* no ISIG */
		}
	else	{
		if ( (argp->c_cc[VINTR] = tc.t_intrc) == (char)-1 )
			argp->c_cc[VINTR] = CNUL;
		if ( (argp->c_cc[VQUIT] = tc.t_quitc) == (char)-1 )
			argp->c_cc[VQUIT] = CNUL;
		argp->c_lflag = ISIG;
		}
	if ( tc.t_startc == (char)-1 && tc.t_stopc == (char)-1 )
		argp->c_iflag = 0;	/* no IXON */
	else
		argp->c_iflag = (flag & X_DECCTQ) != 0 ? IXON
						       : IXON | IXANY;
	if ( (argp->c_cc[VEOF] = tc.t_eofc) == (char)-1 )
		argp->c_cc[VEOF] = CNUL;
	if ( (argp->c_cc[VEOL] = tc.t_brkc) == (char)-1 )
		argp->c_cc[VEOL] = CNUL;
	argp->c_cc[VEOL2] = CNUL;
	if ( _ioctl( fildes, TIOCGLTC, (char *)&ltc ) < 0
					/* old tty handler */
	  || (argp->c_cc[VSWTCH] = ltc.t_suspc) == (char)-1
	   )
		argp->c_cc[VSWTCH] = CNUL;

	argp->c_oflag = (unsigned short)(flag & X_TBDELAY);

	if ( (argp->c_cflag = tbp->sg_ispeed & CBAUD | CREAD)
	      == (B110 | CREAD)
	   )
		argp->c_cflag |= CSTOPB;

	if ( (flag & (X_MDMBUF | X_NOHANG)) != 0 )
		argp->c_cflag |= CLOCAL;
	if ( (flag & O_LCASE) != 0 )
		{
		argp->c_iflag |= IUCLC;
		argp->c_oflag |= OLCUC;
		argp->c_lflag |= XCASE;
		}
	if ( (flag & O_ECHO) != 0 )
		argp->c_lflag |= ECHO;
	if ( (flag & X_NOFLSH) != 0 )
		argp->c_lflag |= NOFLSH;
	else
		argp->c_lflag |= ECHOK;
	if ( (flag & O_CRMOD) != 0 )
		{
		argp->c_iflag |= ICRNL;
		argp->c_oflag |= ONLCR;
		if ( (flag & O_NL2) != 0 )	/* O_NL2 or O_NL3 */
			argp->c_oflag |= NL1;
		else if ( (flag & O_NL1) != 0 ) /* O_NL1 */
			argp->c_oflag |= CR1;
		else if ( (flag & O_CR2) != 0 ) /* O_CR2 or O_CR3 */
			argp->c_oflag |= ONOCR | CR3;	/* approx. */
		else if ( (flag & O_CR1) != 0 ) /* O_CR1 */
			argp->c_oflag |= ONOCR | CR2;	/* approx. */
		}
	else	{
		argp->c_oflag |= ONLRET;
		if ( (flag & O_NL1) != 0 )
			argp->c_oflag |= CR1;
		if ( (flag & O_NL2) != 0 )
			argp->c_oflag |= CR2;
		}
	if ( (flag & O_VTDELAY) != 0 )
		argp->c_oflag |= FF1 | VT1;
	if ( (flag & O_BSDELAY) != 0 )
		argp->c_oflag |= BS1;
	if ( (flag & O_RAW) != 0 )
		{
		argp->c_cflag |= CS8;
		argp->c_iflag &= ~(ICRNL | IUCLC);
		argp->c_lflag &= ~ISIG;
		}
	else	{
		argp->c_cflag |= CS7 | PARENB;
		argp->c_iflag |= BRKINT | IGNPAR | INPCK | ISTRIP;
		argp->c_oflag |= OPOST;
		argp->c_lflag |= ICANON;
		}
	if ( (flag & O_ODDP) != 0 )
		if ( (flag & O_EVENP) != 0 )
			argp->c_iflag &= ~INPCK;
		else
			argp->c_cflag |= PARODD;
	if ( (flag & X_CRTBS) != 0 )
		argp->c_lflag |= ECHOE;
	if ( (flag & X_TANDEM) != 0 )
		argp->c_iflag |= IXOFF;

	argp->c_line = 0;		/* default line discipline */

	return 0;
	}


static void
nap( usec )				/* returns 0 if ok, else -1 */
	long		usec;		/* delay in microseconds */
	{
	static struct timeval delay;	/* `timeval' */

	delay.tv_sec = usec / 1000000L;
	delay.tv_usec = usec % 1000000L;

	select( 0, (long *)0, (long *)0, (long *)0, &delay );
	}
