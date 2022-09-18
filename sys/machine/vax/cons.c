
#ifndef lint
static char *sccsid = "@(#)cons.c	4.5	ULTRIX	10/9/90";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1984-1988		 by		*
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
 * Modification History
 *
 * 31-Aug-90	paradis
 *	Code cleanups for VAX9000 support.
 *
 * 03-Aug-90	rafiey (Ali Rafieymehr)
 *	Added VAX9000 (Aquarius) support (its very similar to 8600). Also
 *	cleaned up the logical console command check code for 8600 in
 *	cnstart. Eliminated the "goto" from cnputc. Fixed a couple of 8600
 *	bugs in cnputc routine:
 *		1. if "c == 0", return BEFORE messing with TXCS register.
 *		2. when restoring TXCS, also set write mask bit.
 *
 * 15-Aug-89 - Randall Brown
 *
 *	Changed all references of TCSADFLUSH to TCSAFLUSH 
 *
 * 30-May-89	darrell
 *	Added include of ../../machine/common/cpuconf.h -- cpu types
 *	were moved there.
 *
 * 07-Jun-88	darrell
 *	Added VAX60 (Firefox) support.
 *
 * 26-Apr-88    jaw
 *	Add VAX8820 support.
 *
 * 7-Mar-88	lp
 *	Fixed typo in ioctl routine (cant believe its been wrong this long).
 *
 * 12-Feb-88 - Fred Canter
 *	Changes for VAX420 (CVAXstar/PVAX) support.
 *
 *  3-Feb-88 - Tim Burke
 *	Only examine O_NONBLOCK for POSIX mode.
 *	A valid character of 0377 is returned as 0377,0377 in TERMIODISC only.
 *	Initialize termio flag extensions to be zero on first open.
 *
 *  1-Dec-87 - Tim Burke
 *
 *	Added support for both System V termio(7) and POSIX termios(7).  These
 *	changes also include support for 8-bit canonical processing.  Changes
 *	involve:
 *
 *	- Default settings on first open depend on mode of open.  For termio
 *	  opens the defaults are "RAW" style, while non-termio opens default
 *	  to the traditional "cooked" style.
 *	- The driver now represents its terminal attributes and special 
 *	  characters in the POSIX termios data structure.  This contrasts the
 *	  original approach of storing attributes and special chars in the
 *	  t_flags, ltchars and tchars.
 *	- New termio ioctls: TCSANOW, TCSADRAIN, TCSADFLUSH, TCSETA, TESETAW,
 *	  TCSETAF.	
 *	- Addition of LPASS8 to local mode word for 8-bit canonical support.
 *
 *
 *  20-Apr-87 - afd
 *	Changed name CVAXQ to VAX3600 for Mayfair.
 *
 *  13-Mar-87 - Tim Burke
 *	Added full TERMIO functionality to terminal subsystem.
 *	Formerly the terminal settings were specified by the t_flags and
 *	t_ispeed.  Now they are specified by termio parameters in the cflag.
 *
 *  06-Mar-87 -- afd
 *	Added CVAXQ to MVAX ifdef's in v_consputc for QDSS.
 *
 *
 * 06-Aug-86 -- jaw	fixes to 8800 console support.
 *
 *  2-Jul-86  -- fred (Fred Canter)
 *	Moved sscons_init(), ssputc(), & ssgetc() to ss.c.
 *	No longer any VAXstar specific code in cons.c.
 *
 * 18-Jun-86 -- fred (Fred Canter)
 *	Changes for VAXstar kernel support.
 *
 * 29-Apr-86 -- jaw	fixes to 8800 console.
 *
 * 15-Apr-86 -- afd
 *	v_console() was changed to v_consputc().
 *
 * 09-Apr-86 -- jaw  only VAX8800 now toggles TXCS_IE.
 *
 * 03-Apr-86 -- jaw  left out some ifdefs.
 *
 * 02-Apr-86 -- jaw  add support for nautilus console and memory adapter
 *
 * 05-Mar-86 -- jaw  VAXBI device and controller config code added.
 *		     todr code put in cpusw.
 *
 * 07 Nov 85 -- lp
 *	Disable use of EMM. Only snap file commands allowed on
 *	logical_cons.
 * 10 Oct 85 -- lp
 *	Added 8600 remote line support. Also moved 780 floppy output
 *	to be routed through chrqueue (so we dont get confused
 *	about who a character is intended for). Prevent non-8600
 *	8200 machines from using extra lines code (they can't).
 *	Make sure interrupts are off in 8600 cnputc.
 *
 * 19 Aug 85 -- lp
 *	Changed cnstart routine to only call cnoutput if not aready
 *	outputting characters.
 *
 * 13 Aug 85 -- rjl
 *	Changed virtual console put call to be through a function pointer
 *	instead of directly to allow dynamic configuring of the system
 *	console.
 *
 * 16 Jul 85 -- lp
 *	Cleanup for 1.2 ft. Added ioctl setspeed for Scorpio.
 *
 * 15 Mar 85 -- lp
 *	Added support for Scorpio slu's (serial line units).
 *	Added notion of interrupt queue so we can get characters
 *	in at elevated IPL the reschedule tty action at a lower IPL.
 *
 * 20 Nov 84 -- rjl
 *	Changed qvss support to virtual console support with the idea that
 *	a virtual console could be added at a later time.
 *
 * 11 Aug 84 -- rjl
 *	Added MVAX support for system console
 */

/*
 * VAX console driver (and floppy interface)
 */
#include "../h/param.h"
#include "../h/conf.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/proc.h"
#include "../h/ioctl.h"
#include "../h/tty.h"
#include "../h/termio.h"
#include "../h/file.h"
#include "../h/systm.h"
#include "../h/uio.h"
#include "../h/exec.h"

#include "../machine/cpu.h"
#include "../../machine/common/cpuconf.h"
#include "../machine/cons.h"
#include "../machine/mtpr.h"

#if defined(VAX8200) || defined(VAX8600) || defined (VAX9000)
#define NCONS	4
#else
#define NCONS	1
#endif

#define DEFAULTSPEED B4800
/*
 * ULTRIX settings for first open.
 */
#define IFLAGS	(EVENP|ECHO|XTABS|CRMOD)

/* termio flags will be set to these default values in non-termio mode to
 * provide a backward compatible ULTRIX environment.
 */
#define IFLAG (BRKINT|IGNPAR|ISTRIP|IXON|IXANY|ICRNL)
#define OFLAG (OPOST|TAB3|ONLCR)
#define LFLAG (ISIG|ICANON|ECHO)
#ifdef	VAX8200
#define CFLAG (PARENB|CREAD|CS7|CLOCAL)
#else	VAX8200
#define CFLAG (PARENB|CREAD|CS7|CLOCAL)
#endif	VAX8200

struct	tty cons[NCONS];
int tty8800rec[3] = { 0, 0, 0};
int txcs8800ie;
int rx8800ie;
int	cnstart();
int	ttrstrt();
char	partab[];
int cold;


#ifdef	VAX8200
static int rxcs[] = { RXCS, RXCS1, RXCS2, RXCS3 };
static int rxdb[] = { RXDB, RXDB1, RXDB2, RXDB3 };
static int txcs[] = { TXCS, TXCS1, TXCS2, TXCS3 };
static int txdb[] = { TXDB, TXDB1, TXDB2, TXDB3 };
#else	VAX8200
static int rxcs[] = { RXCS };
static int rxdb[] = { RXDB };
static int txcs[] = { TXCS };
static int txdb[] = { TXDB };
#endif	VAX8200

/*
 *  Note: Aquarius doesn't have the EMM port but this define should be ok.
*/
#if defined(VAX8600) || defined(VAX9000)
static int conson[] = { LOCAL_CONS, REMOTE_PORT, EMM_PORT, LOGICAL_CONS };
static int consid[] = { LOCAL_CONS_ID, REMOTE_PORT_ID, EMM_ID, LOGIC_CONS_ID };
int	mask_update_needed = 0;
int	new_mask = 0;
#endif
#ifdef VAX8800
static int consid8800[] = {N_LCL_CONS,N_LCL_NOLOG,N_RMT_CONS};
#endif 



/*ARGSUSED*/
cnopen(dev, flag)
	dev_t dev;
{
	register int whichcons = minor(dev);
	register int which = minor(dev);
	register struct tty *tp;
	register int s, timo;

	switch (cpu) {
#if defined(VAX8200) || defined(VAX8600) || defined(VAX9000)
	case VAX_9000:
		/* VAX 9000 only supports 0 (local console) and
		 * 1 (remote console).
		 */
		if(which > 1) return(ENODEV);
		/* Fall thru to... */
	case VAX_8600:
		whichcons=0;

	case VAX_8200:
		if (which > 3 ) return(ENODEV);
		break;
#endif	
#ifdef VAX8800
	case VAX_8820:
	case VAX_8800:
		if (which > 2 ) return(ENODEV);
		tty8800rec[which] = 0;
		whichcons=0;
		break;
#endif
	default:
		if (which) return(ENODEV);
		break;
	}	
	tp = &cons[which];
	tp->t_oproc = cnstart;
	/*
	 * Look at the compatibility mode to specify correct default parameters
	 * and to insure only standard specified functionality.
	 */
	if ((u.u_procp->p_progenv == A_SYSV) || 
		(u.u_procp->p_progenv == A_POSIX)) {
		flag |= O_TERMIO;
		tp->t_line = TERMIODISC;
	}
	/*
	 * Set state bit to tell tty.c not to assign this line as the 
	 * controlling terminal for the process which opens this line.
	 */
	if (flag & O_NOCTTY) 
		tp->t_state |= TS_ONOCTTY;
	if ((tp->t_state&TS_ISOPEN) == 0) {
		ttychars(tp);
		tp->t_state = TS_ISOPEN|TS_CARR_ON;
		tp->t_cflag = tp->t_cflag_ext = DEFAULTSPEED;
		tp->t_iflag_ext = 0;
		tp->t_oflag_ext = 0;
		tp->t_lflag_ext = 0;
		/*
		 * Ultrix defaults to a "COOKED" mode on the first
		 * open, while termio defaults to a "RAW" style.
		 * Base this decision by a flag set in the termio
		 * emulation routine for open, or set by an explicit
		 * ioctl call.
		 */
		if ( flag & O_TERMIO ) {
			/* Provide a termio style environment.
			 * "RAW" style by default.
			 */
			tp->t_flags = RAW;   
			tp->t_iflag = 0;
			tp->t_oflag = 0;
			tp->t_cflag |= CS8|CREAD|HUPCL; 
			tp->t_lflag = 0;

			/*
			 * Change to System V line discipline.
			 */
			tp->t_line = TERMIODISC;
			/*
			 * The following three control chars have 
			 * different default values than ULTRIX.
			 */
 			tp->t_cc[VERASE] = '#';
 			tp->t_cc[VKILL] = '@';
 			tp->t_cc[VINTR] = 0177;
 			tp->t_cc[VMIN] = 6;
 			tp->t_cc[VTIME] = 1;
		} else {
			/* Provide a backward compatible ULTRIX 
			 * environment.  "COOKED" style.
			 */
			tp->t_flags = IFLAGS;
			tp->t_iflag = IFLAG;
			tp->t_oflag = OFLAG;
			tp->t_lflag = LFLAG;
			tp->t_cflag |= CFLAG;
		}
	}
	if (tp->t_state&TS_XCLUDE && u.u_uid != 0)
		return (EBUSY);

#if defined(VAX8600) || defined(VAX9000)
	if(cpu == VAX_8600 || cpu == VAX_9000) {
		if(consid[which] == EMM_ID) /* No EMM allowed */
			return(ENODEV);
		
		/* enable that console based on dev */
		mtpr(rxcs[0], (mfpr(rxcs[0])|conson[which]|RXCS_IE));
		cnenable_8600_9000(conson[which], TXCS_IE);
	}
#endif
 	s = spl5();	
	if (((cpu != VAX_9000) && (cpu != VAX_8800) && (cpu != VAX_8820)) || txcs8800ie == 0) {
		txcs8800ie=1;
		mtpr(txcs[whichcons], (mfpr(txcs[whichcons])|TXCS_IE));
	}
	splx(s);
	if((cpu != VAX_8600) && (cpu != VAX_9000)) {
		mtpr(rxcs[whichcons], (mfpr(rxcs[whichcons])|RXCS_IE));
	}

	return ((*linesw[tp->t_line].l_open)(dev, tp));
}

/*ARGSUSED*/
cnclose(dev)
	dev_t dev;
{
	register int which = minor(dev);
	register struct tty *tp = &cons[which];
	register timo;

	(*linesw[tp->t_line].l_close)(tp);


	switch (cpu) {
#if defined(VAX8800) || defined(VAX780)
	case VAX_8820:
	case VAX_8800:
	case VAX_780:
		ttyclose(tp);
		break;
#endif
#if defined(VAX8600) || defined(VAX9000)
	case VAX_8600:
	case VAX_9000:
		ttyclose(tp); 
		/* Mask the line out */
		cndisable_8600_9000(conson[which], 0);
		if(cpu == VAX_8600) {
			mtpr(rxcs[0], (mfpr(rxcs[0])&~conson[which]) | RXCS_IE);
		}
		break;
#endif

	default:
		mtpr(rxcs[which], mfpr(rxcs[which])&~RXCS_IE);
		ttyclose(tp);
#ifdef VAX8200
		if(cpu == VAX_8200) {
			tp->t_cflag = tp->t_cflag_ext = DEFAULTSPEED;
			cnparam(which);
		}
#endif
		/* disable interrupts till line is opened */
		mtpr(txcs[which], mfpr(txcs[which])&~TXCS_IE);
		break;
	}
	/*
	 * Clear input speed so that the next open will have propper defaults.
	 */
	tp->t_cflag &= ~CBAUD;
	tp->t_cflag_ext &= ~CBAUD;
	/* Remove termio flags that do not map */
	tp->t_iflag &= ~TERMIO_ONLY_IFLAG;
	tp->t_oflag &= ~TERMIO_ONLY_OFLAG;
	tp->t_cflag &= ~TERMIO_ONLY_CFLAG;
	tp->t_lflag &= ~TERMIO_ONLY_LFLAG;
}

/*ARGSUSED*/
cnread(dev, uio)
	dev_t dev;
	struct uio *uio;
{
	register struct tty *tp = &cons[minor(dev)];

	return ((*linesw[tp->t_line].l_read)(tp, uio));
}

/*ARGSUSED*/
cnwrite(dev, uio)
	dev_t dev;
	struct uio *uio;
{
	register struct tty *tp = &cons[minor(dev)];

	return ((*linesw[tp->t_line].l_write)(tp, uio));
}

/*
 * Got a level-20 receive interrupt -
 * the LSI wants to give us a character.
 * Catch the character, and see who it goes to.
 */
/*ARGSUSED*/
cnrint(dev)
	dev_t dev;
{
	int chrqueue();
	register int c;
	register int which = minor(dev);
	register struct tty *tp = &cons[which];

	switch (cpu) {

#ifdef VAX8800
	case VAX_8820:
	case VAX_8800:
		c = mfpr(rxdb[0]);
		ka8800requeue(c);
		return;
#endif

#if defined(VAX8600)
	case VAX_8600:
		c = mfpr(rxdb[0]);
		/* Based on Id field, setup who the interrupt goes to */
		/* Valid values are Venus: 0,1,2,3; Aquarius: 0,1,3 */
		if((c&RXDB_ID) == RXDB_ID) /* could be handled better */
			return;
		tp = &cons[(c>>8)&0x3];
		break;
#endif

#ifdef VAX9000
	case VAX_9000:
		/* On the VAX9000 the "dev" argument will always be
		 * zero; the actual device has to be decoded from
		 * the RXDB register.  In fact, this may not be a
		 * character interrupt at all; the console may have
		 * rebooted or carrier may have been detected on the
		 * remote line.
		 */
		c = mfpr(rxdb[0]);
		if((c & RXDB_ID) == RXDB_ID_CARIER) {
			if (c & RXDB_CONRTN) {
				/* SPU has rebooted! */
				ka9000_reinit_spu();
			}
			if((c & RXDB_RTYCD) == RXDB_RTYCD) {
				/* Carrier detect on remote.  Record
				 * this fact and set DTR on for the
				 * remote TTY.
				 */
				cons[1].t_state |= TS_CARR_ON;
				mtpr(rxcs[0], 
					mfpr(rxcs[0])|RXCS_RDTR|RXCS_IE);
			}
			if((c & RXDB_RTYCD) == 0) {
				if(cons[1].t_state & TS_CARR_ON) {
					/* If remote carrier
					 * transitioned from high
					 * to low, then drop the
					 * connection.
					 */
					cons9000_tty_drop(&cons[1]);
				}
			}
			/* If a CARRIER function occurred, then
			 * we do nothing else.
			 */
			return;
		}
		else {
			tp = &cons[(c>>8)&0x3];
			which = tp - &cons[0];
		}
		break;
#endif VAX9000

#ifdef VAX780
	case VAX_780:	
		c = mfpr(rxdb[which]);
		if (c&RXDB_ID) {
			int cnrfl();
			chrqueue(cnrfl, c, 0);
			return;
		}
		break;
#endif VAX780

	default:
		c = mfpr(rxdb[which]);


	}
	if (tp->t_state&TS_ISOPEN) {
                if (tp->t_iflag & ISTRIP)
                        c &= 0177;
                else {
                        c &= CHAR_MASK;
			/* If ISTRIP is not set a valid character of 377
			 * is read as 0377,0377 to avoid ambiguity with
			 * the PARMARK sequence.
			 */ 
			if ((c == 0377) && (tp->t_line == TERMIODISC))
				chrqueue(linesw[tp->t_line].l_rint, c, tp);
		}
		chrqueue(linesw[tp->t_line].l_rint, c, tp);
	}

}

/*ARGSUSED*/
cnioctl(dev, cmd, addr, flag)
	dev_t dev;
	caddr_t addr;
{
	register int which = minor(dev);
	register struct tty *tp = &cons[which];
	int error;

	error = (*linesw[tp->t_line].l_ioctl)(tp, cmd, addr);
	if (error >= 0)
		return(error);
	error = ttioctl(tp, cmd, addr, flag);
#ifdef VAX8200
	if (error >= 0) {
 		switch(cmd) {
			case TCSANOW:			/* POSIX */
			case TCSADRAIN:			/* POSIX */
			case TCSAFLUSH:			/* POSIX */
 			case TCSETA:			/* SVID */
 			case TCSETAW:			/* SVID */
 			case TCSETAF:			/* SVID */
 			case TIOCSETP:			/* Berkeley */
 			case TIOCSETN:			/* Berkeley */
 				cnparam(which);		/* set speed */
 				break;
 		}
	}
#endif
	if (error < 0)
		error = ENOTTY;
	return (error);
}

/*
 * Got a level-20 transmission interrupt -
 * the LSI wants another character.  First,
 * see if we can send something to the typewriter.
 * If not, try the floppy.
 */
/*ARGSUSED*/
int didsendit;

cnxint(dev)
	dev_t dev;
{
	register int which = minor(dev);
	register struct tty *tp = &cons[which];
	register int i;

#if defined(VAX8600) || defined(VAX9000)
	if(cpu == VAX_8600 || cpu == VAX_9000) {
                int k;

                /* See if a mask update is needed... */
                if(mask_update_needed) {
                        int     s = spl5();

                        k = mfpr(txcs[0]);
                        if(k & TXCS_RDY) {
                                mtpr(txcs[0], (k & ~ALLCONS)|WMASKNOW|new_mask);
                                mask_update_needed = 0;
                                splx(s);
                                return;
                        }
                        splx(s);
                }

                /* see who interrupted us */
                k = mfpr(txcs[0]);
                if((k&NO_LINES) == NO_LINES)
                        return;
                tp = &cons[(k>>8)&0x3];
                which = tp - &cons[0];
	}
#endif
#ifdef VAX8800
	if ((cpu == VAX_8800) || (cpu == VAX_8820)) {
		txcs8800ie=0;
		mtpr(TXCS, 0); /* disable transmit interrupts */

		if (rx8800ie) {
			int rx8800_trans();
			chrqueue(rx8800_trans,0, 0);
			return;
		}
		for(i=0;i < 3;i++) {
		    if (tty8800rec[i] == 0 ) {
			tp = &cons[i];
			if (tp->t_state&TS_ISOPEN) {
				tp->t_state &= ~TS_BUSY;
				if (tp->t_line)
					(*linesw[tp->t_line].l_start)(tp);
				else
					cnstart(tp);
				if (txcs8800ie) return;
			}		
		    }
		}
	} else 
		{
#endif
		/*If the line is open & no chance of it being the 780 floppy*/
		if((tp->t_state&TS_ISOPEN) || (cpu == VAX_780)) {
			tp->t_state &= ~TS_BUSY;
			if (tp->t_line)
				(*linesw[tp->t_line].l_start)(tp);
			else
				cnstart(tp);
		}
#ifdef VAX8800
	}
#endif VAX8800
}

int cn_cnoutput_pending;

cnstart(tp)
	register struct tty *tp;
{
	register int c, s;
	register int which;
	int cnoutput();

	/* Dont queue it up if we don't have to */
	if (((cpu == VAX_8800) || (cpu == VAX_8820)) && txcs8800ie) return;


	if (tp->t_state & (TS_TIMEOUT|TS_BUSY|TS_TTSTOP))
		goto out;
	if (tp->t_outq.c_cc <= TTLOWAT(tp)) {
		if (tp->t_state&TS_ASLEEP) {
			tp->t_state &= ~TS_ASLEEP;
			wakeup((caddr_t)&tp->t_outq);
		}
		if (tp->t_wsel) {
			selwakeup(tp->t_wsel, tp->t_state & TS_WCOLL);
			tp->t_wsel = 0;
			tp->t_state &= ~TS_WCOLL;
		}
	}
	if (tp->t_outq.c_cc == 0) 
		goto out;

	which = tp - &cons[0];

#if defined(VAX8600) || defined(VAX9000)
	if((cpu == VAX_8600) || (cpu == VAX_9000)) {
		if(consid[which] == LOGIC_CONS_ID) {
			char *cp = tp->t_outq.c_cf;

			if(cpu == VAX_9000) {
				/* Logical console device not supported for 
				 * Aquarius.  We should never even TRY to 
				 * use it (since cnopen doesn't  allow 
				 * its use...)
	 			 */
				panic("No logical console on VAX 9000");
			}

			/* Hunt for any valid command else throw them away */
			while ((*cp != GETSTAT) 
					&& (*cp != INVAL_1) 
					&& (*cp != INVAL_2)) {
				if(getc(&tp->t_outq) == NULL)
					return;
			/* Peek at next character */
				cp = tp->t_outq.c_cf;
			}
		}
		if((mfpr(txcs[0])&conson[which]) != conson[which]) {
			cnenable_8600_9000(conson[which], 0);
			return;
		}
	}
#endif VAX8600 || VAX9000
	if (cn_cnoutput_pending == 0) {
		txcs8800ie=1;
		cn_cnoutput_pending = 1;
		chrqueue(cnoutput, tp, which);
	}
	return;

out:
#if defined(VAX8600) || defined(VAX9000)
	if (cpu == VAX_8600 || cpu == VAX_9000) {
		which = tp - &cons[0];
		if (tp->t_state & (TS_TIMEOUT|TS_BUSY))
			if((tp->t_state&TS_TTSTOP) == 0)
				return;
		cndisable_8600_9000(conson[which], 0);
	}
#endif
#ifdef VAX780
	/* I think this is impossible right here but I'm making sure */
	if ((cpu == VAX_780) && ((tp->t_state&TS_BUSY) == 0)) {	/* floppy */
		conxfl();	/* so start floppy xfer */
	}
#endif
	;
}

cnoutput(tp, which)
register struct tty *tp;
register int which;
{
	register int c,s,whichcons = which;

#if defined(VAX8600) || defined(VAX8800) || defined(VAX9000)
	if (cpu == VAX_8600 || cpu == VAX_8800 || cpu == VAX_8820
		|| cpu == VAX_9000)
		whichcons = 0;

#endif
 	s = spl5();
	cn_cnoutput_pending=0;

	if ((cpu == VAX_8800) || (cpu == VAX_8820)) {
		txcs8800ie=1;
		mtpr(TXCS,TXCS_IE);
	}
retry:
	if (tp->t_state & (TS_TIMEOUT|TS_BUSY|TS_TTSTOP))
		goto out;
	if (tp->t_outq.c_cc <= TTLOWAT(tp)) {
		if (tp->t_state&TS_ASLEEP) {
			tp->t_state &= ~TS_ASLEEP;
			wakeup((caddr_t)&tp->t_outq);
		}
		if (tp->t_wsel) {
			selwakeup(tp->t_wsel, tp->t_state & TS_WCOLL);
			tp->t_wsel = 0;
			tp->t_state &= ~TS_WCOLL;
		}
	}
	if (tp->t_outq.c_cc == 0) 
		goto out;
#if defined(VAX8600) || defined(VAX9000)
	if(cpu == VAX_8600 || cpu == VAX_9000) {
		int k;

                /* We can't do anything if the console isn't ready for us
                 * (it may not be if there's output taking place on the
                 * other line, f'rinstance...)
                 */
                k = mfpr(txcs[0]);

                if((k & TXCS_RDY) == 0) {
                        splx(s);
                        return;
                }
                /* If this line is not enabled, then enable it. */
                if((k & conson[which]) != conson[which]) {
                        mtpr(txcs[0], k | WMASKNOW | conson[which]);
                        splx(s);
                        return;
                }

                /* If the console buffer that's currently available
                 * is different than the one we're prepared to send
                 * to, see if there's any work to do on the available
                 * line...
                 */
                if((k & NO_LINES) != consid[which]) {
                        which = (k >> 8) & 3;
                        tp = &cons[which];
                        goto retry;
                }
        }
#endif VAX8600 || VAX9000
	c = getc(&tp->t_outq);
	if ((tp->t_lflag_ext & PRAW) || (tp->t_oflag_ext & PLITOUT) || 
	    ((tp->t_oflag & OPOST) == 0)) {
		tp->t_state |= TS_BUSY;
		c &= CHAR_MASK;
#ifdef VAX8800
		if ((cpu == VAX_8800) || (cpu == VAX_8820)) 
				c |= consid8800[which];
#endif VAX8800	
		mtpr(txdb[whichcons], c);
	} else if ((c & DELAY_FLAG) == 0) {
		if ((tp->t_cflag & CS8) != CS8)
			c &= 0177;
		else 
			c &= CHAR_MASK;
		tp->t_state |= TS_BUSY;
#ifdef VAX8800
		if ((cpu == VAX_8800) || ( cpu == VAX_8820)) 
				c |= consid8800[which];
#endif VAX8800
		mtpr(txdb[whichcons], c);
	} else {
		tp->t_state |= TS_TIMEOUT;
		timeout(ttrstrt, (caddr_t)tp, (c&CHAR_MASK));
	}
out:
#if defined(VAX8600) || defined(VAX9000)
	if (cpu == VAX_8600 || cpu == VAX_9000) {
		which = tp - &cons[0];
		if (tp->t_state & (TS_TIMEOUT|TS_BUSY)) {
			if((tp->t_state&TS_TTSTOP) == 0) {
				splx(s);
				return;
			}
		}
		cndisable_8600_9000(conson[which], 0);
	}
#endif
#ifdef VAX780
	if ((cpu == VAX_780) && ((tp->t_state&TS_BUSY) == 0)) {	/* floppy */
		conxfl();	/* so start floppy xfer */
	}
#endif
	splx(s);
}

/*
 * Print a character on console.
 * Attempts to save and restore device
 * status.
 */
cnputc(c)
	register int c;
{
	register struct tty *tp;
	register int loopcnt,savetxcs,s, timo,data;
#if defined(VAX8600) || defined(VAX9000)
	int oldtxcs;
#endif VAX8600 || VAX9000
#if defined (MVAX) || defined (VAX3600) || defined (VAX420) || defined (VAX60)
	extern (*v_consputc)();
#endif MVAX || VAX3600 || VAX420 || VAX60

	/*
	 * Try waiting for the console tty to come ready,
	 * otherwise give up after a reasonable time.
	 */
#if defined (MVAX) || defined (VAX3600) || defined (VAX420) || defined (VAX60)
	if( v_consputc ) {
		s=spl7();
		(*v_consputc)( c );
		if( c == '\n' )
			(*v_consputc)( '\r' );
	} else {
#endif MVAX || VAX3600 || VAX420 || VAX60
		/* make sure we do not lower IPL.  Raise to ipl of
		   the console interrupt routine */
		if ((s = mfpr(IPL)) < SPLTTY) {
			s=spl5();
		}
#ifdef VAX8800
		if ((cpu == VAX_8800) || (cpu == VAX_8820)) {
			timo = 600;
			while (((mfpr(RXCS) & RXCS_DONE) == 0) &&
				(timo != 0)) --timo;

			loopcnt=0;
			while (((mfpr(RXCS) & RXCS_DONE) != 0) || tty8800rec[0]==1) {
				if (loopcnt++ > 20) {
					tty8800rec[0] = 0;
					break;
				}
				if (mfpr(RXCS) & RXCS_DONE) {
					data = mfpr(RXDB);
					if (cold==0) ka8800requeue(data);
					if ((data&0xf7f) == CSTOP) 
							tty8800rec[0] = 1;
					if ((data&0xf7f) == CSTART) 
							tty8800rec[0] = 0;
				}
				timo = 10000;
				while((--timo)!= 0){
					if ((mfpr(RXCS) & RXCS_DONE) != 0) 
						break;
				}

			}
		}
#endif
#if defined(VAX8600) || defined(VAX9000)
		if(cpu == VAX_8600 || cpu == VAX_9000) {
			timo = 30000;
			while ((mfpr(TXCS)&TXCS_RDY) == 0)
				if(--timo == 0)
					break;
			if (c == 0) {
				splx(s);
				return;
			}
			oldtxcs = mfpr(TXCS);
			mtpr(TXCS, (oldtxcs|WMASKNOW|conson[0])&~TXCS_IE);
			timo = 30000;
		}
#endif VAX8600 || VAX9000
			
		timo = 30000;
		while ((mfpr(TXCS)&TXCS_RDY) == 0)
			if(--timo == 0)
				break; 
		if (c == 0) {
			splx(s);
			return;
		}
		if ((cpu != VAX_8800) && (cpu != VAX_8820) ){
			savetxcs = mfpr(TXCS);
			mtpr(TXCS, 0);
		}
		mtpr(TXDB, c&0x7f);
		if (c == '\n')
			cnputc('\r');
		cnputc(0);

		if ((cpu != VAX_8800) && (cpu != VAX_8820) 
		  &&(cpu != VAX_8600) && (cpu != VAX_9000))
			mtpr(TXCS, savetxcs);
#if defined(VAX8600) || defined(VAX9000)
		if(cpu == VAX_8600 || cpu == VAX_9000)
			mtpr(TXCS, oldtxcs|WMASKNOW);
#endif
#if defined (MVAX) || defined (VAX3600) || defined (VAX420) || defined (VAX60)
	}
#endif MVAX || VAX3600 || VAX420 || VAX60
	/* restore IPL...*/
	splx(s);

}

#ifdef VAX8200
#define INVALSP B4800
static int cn_speeds[] = { INVALSP, INVALSP, INVALSP, INVALSP, INVALSP,
		    0x00000000, INVALSP, 0x00000200, 0x00000400, 0x00000600,
		    INVALSP, 0x00000800, 0x00000a00, 0x00000c00, 0x00000e00, INVALSP};

cnparam(which)
int which;
{
	register struct tty *tp = &cons[which];

	if (cpu != VAX_8200)
		return;
	if (which == 0)
		return; 	/* Not brave enough to try this on the console right now! */

	/*
	 * Set speed according to input speed specification.
	 */
	if((tp->t_cflag&CBAUD) <= 0 || (tp->t_cflag&CBAUD) > 16) {
		tp->t_cflag &= ~CBAUD;
		tp->t_cflag_ext &= ~CBAUD;
		tp->t_cflag |= DEFAULTSPEED;
		tp->t_cflag_ext |= DEFAULTSPEED;
	}
	/* Match up input & output speeds */
	if((tp->t_cflag&CBAUD) != (tp->t_cflag_ext&CBAUD)) {
                tp->t_cflag_ext &= ~CBAUD;
		tp->t_cflag_ext |= tp->t_cflag&CBAUD;
	}
	if(cn_speeds[(tp->t_cflag & CBAUD)] != INVALSP)
	{
		mtpr(txcs[which], ((mfpr(txcs[which])&~0xf00)|
			(cn_speeds[(tp->t_cflag & CBAUD)]|TXCS_BRE)));
	}

}

#endif

#if defined(VAX8600) || defined(VAX9000)
cnenable_8600_9000(whichcons, flags)
{
	int s = spl5();
	int k = mfpr(txcs[0]);

	if(k & TXCS_RDY) {
		mtpr(txcs[0], k | WMASKNOW | whichcons | flags);
	}
	else {
		/* The console isn't ready for this update yet...
		 * Squirrel away the new value of the mask,
		 * and we'll update it when the next interrupt
		 * comes in.
		 */
		new_mask = (k & ALLCONS) | whichcons | flags;
		mask_update_needed = 1;
	}
	splx(s);
}

cndisable_8600_9000(whichcons, flags)
{
	int s = spl5();
	int k = mfpr(txcs[0]);

	if(k & TXCS_RDY) {
		mtpr(txcs[0], (k | WMASKNOW | flags) &~ whichcons);
	}
	else {
		/* The console isn't ready for this update yet...
		 * Squirrel away the new value of the mask,
		 * and we'll update it when the next interrupt
		 * comes in.
		 */
		new_mask = ((k & ALLCONS) | flags) &~ whichcons;
		mask_update_needed = 1;
	}
	splx(s);
}
#endif /* VAX8600 || VAX9000 */

#ifdef VAX9000

/* Drop the connection on the remote console line.  */
cons9000_tty_drop(tp)
struct tty *tp;
{
        if (tp->t_flags&NOHANG)
                return;
        tp->t_state &= ~(TS_CARR_ON|TS_TTSTOP|TS_BUSY|TS_ISUSP);
        wakeup((caddr_t)&tp->t_rawq);
        gsignal(tp->t_pgrp, SIGHUP);
        gsignal(tp->t_pgrp, SIGCONT);
}

#endif VAX9000
