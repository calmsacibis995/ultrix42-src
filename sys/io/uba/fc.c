#ifndef lint
static char *sccsid = "@(#)fc.c	4.1	(ULTRIX)	7/2/90";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1985, 1986, 1987 by		*
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

#include "fc.h"
#if NFC > 0 || defined(BINARY)
/*
 *  Firefox serial line unit driver
 *
 *  Modification History:
 *
 * 31-Oct-89 - Randall Brown
 *
 *	Added the support to allow the device to determine if baudrate is 
 *	supported before it is set in the tty data structures.
 *
 * 15-Aug-89 - Randall Brown
 *
 *	Changed all references of TCSADFLUSH to TCSAFLUSH 
 *
 * 21-Jul-89 - Randall Brown
 *
 *	Moved default open and default close code to tty.c and call it
 *	using tty_def_open() and tty_def_close().  In the close routine,
 *	don't clear the baudrate so that subsequent opens will keep the
 *	present attributes.  This only applies to a Berkeley environment.
 *
 * 12-Jun-89 - dws
 *	Added trusted path support.
 *
 * 15-Feb-89 - darrell
 *	Removed the global variable ka60_expect_memerr.
 *
 * 12-Jan-89 - jaw
 *	merge Xe changes for FFox drivers
 *
 * 25-Jan-89 - Randall Brown
 *
 *	Changed cd_drop to look at LNOHANG.  Changed close routine to look
 *	at HUPCL.
 *
 * 20-Jan-89 - darrell
 *	Cleanup, and added a module counter to keep track of the number
 *	of memory modules in the system.
 *
 * 18-Nov-88 - darrell (for tim)
 *	Fixed a bug where the system would hang during boot if a character
 *	was typed on the keyboard or if the mouse was moved.
 *
 * 28-Sep-88 - darrell
 *	Changed all writes to fbicsr to be read-modif-write accesses
 *	so as to not change the value of the LEDS on any hardware modules.
 *
 * 28-Sep-88 - Randall Brown
 *	Fixed a bug in fcxint so that the transmitter interrupt will be
 *	acknowledged when the terminal is in the stop state.
 *
 * 27-Sep-88 - darrell
 *	Added mapping code for FGCTSIXSmap.
 *
 * 13-Sep-88 - Ali Rafieymehr
 *
 *	Fixed a bug which was causing the "select" not to work for
 *	alternate console.
 *
 * 02-Sep-88 - Tim Burke
 *
 *	Return EINVAL instead of ENOTTY for POSIX programs on invalid ioctls.
 *
 * 18-Aug-88 - Tim Burke
 *
 *	If PARMRK is set and a BREAK occurs, return '\0377','\0','\0'.
 *
 * 5-Aug-88 - Tim Burke
 *
 *	Return the 2 character sequence 0377, 0377 upon receipt of a valid
 *	0377 character only when PARMRK is set under the termio line disc.
 *
 * 12-15-87	darrell
 *	Copied this file from ss.c.
 */

#include "../data/fc_data.c"


int fcdebug = 0;
int fcbail1 = 0;	/* Bit spin count failures */
int fcbail2 = 0;
int fcbail3 = 0;
int ff_diagcons = 0;	/* 0 = LEGSS, 1 = VT220 */

/*
* Driver information for auto-configuration stuff.
*/
int	fcprobe(), fcattach(), fcrint();
int	fc_cd_drop(), fc_dsr_check(), fc_cd_down(), fc_tty_drop(); /* Modem */
u_short	fcstd[] = { 0 };
struct	uba_driver fcdriver =
{ fcprobe, 0, fcattach, 0, fcstd, "fc", fcinfo };


int	fcstart(), fcxint(), fcdma(), fcbaudrate();
int	ttrstrt();
int	fcact;

/*
 * Graphics device driver entry points.
 * Used to call graphics device driver as needed.
 */
extern	(*vs_gdopen)();
extern	(*vs_gdclose)();
extern	(*vs_gdread)();
extern	(*vs_gdwrite)();
extern	(*vs_gdselect)();
extern	(*vs_gdkint)();
extern	(*vs_gdioctl)();
extern	(*vs_gdstop)();

#define	FASTTIMER	(hz/30)		/* rate to drain silos, when in use */
#define MODEM_UNIT	2		/* Modem control only on unit 2     */
#define LINEMASK        0x03            /* line unit mask */

int	fcsilos;			/* mask of SLU's with silo in use */
int	fctimerintvl;			/* time interval for fctimer */
int	fchighrate = 100;		/* silo on if fcchars > fchighrate */
int	fclowrate = 75;			/* silo off if fcrate < fclowrate */

/*
* The SLU doesn't interrupt on carrier transitions, so
* we have to use a timer to watch it.
*/
char	fc_timer;		/* timer started? */

char	fc_speeds[] =
{ 0,020,021,022,023,024,0,025,026,027,030,032,034,036,037,0 };

short	fc_valid_speeds = 0x7fbf; /* 0,1,1,1, 1,1,1,1, 1,0,1,1, 1,1,1,1 */

u_char fccan_previous;		/* Used to detect modem transitions */

extern long cpu_fbic_addr;
extern struct mb_node mbus_nodes[];
extern struct cpusw *cpup;	/* pointer to cpusw entry */

long cpu_fbic_addr;

struct	tty		sm_tty;

fcprobe(reg)
	caddr_t reg;
{
	register struct fc_regs *fcaddr = (struct fc_regs *)reg;
	int	 i;
	int	 tries;

	/*
	 * ONLY on a Firefox
	 */
	if(cpu != VAX_60)
		return(0);
	/*
	 * Hit master clear to reset chip to known state.
	 * Give time for self test to pass.
	 */
	tries = 0;
	fcaddr->fccsr = FC_CLR;
	while ((fcaddr->fccsr & FC_CLR) & (tries++ < 10)) {
		DELAY(100000);
	}
#ifdef notdef
	/*
	 * Cause the DZ to interrupt
	 */
	fcaddr->fccsr = FC_TIE | FC_MSE;
	fcaddr->fctcr = 0x8;            /*  enable line 3 */
	/* The DZ should interrupt during this delay */
	DELAY(100000);
	fcaddr->fccsr = FC_MSE;
	fcaddr->fctcr = 0;
#endif notdef
	return (1);	/* 1 not sizeof anything, just says probe succeeded */
}

fcattach(ui)
	register struct uba_device *ui;
{
	register struct pdma *pdp = &fcpdma[ui->ui_unit*4];
	register struct tty *tp = &fc_tty[ui->ui_unit*4];
	register int cntr;
	register struct fc_regs *fcaddr = (struct fc_regs *)ffcons;
	extern fcscan();

	for (cntr = 0; cntr < 4; cntr++) {
		/* dzdevice looks wrong, but see vaxuba/pdma.h for reason */
		pdp->p_addr = (struct dzdevice *)&fcaddr->fccsr;
		pdp->p_arg = (int)tp;
		pdp->p_fcn = fcxint;
		pdp++, tp++;
	}
	fcsoftCAR[ui->ui_unit] = ui->ui_flags;
	fcdefaultCAR[ui->ui_unit] = ui->ui_flags;
	fcmodem = 0; 
	if (fc_timer == 0) {
		fc_timer++;
		timeout(fcscan, (caddr_t)0, hz);
		fctimerintvl = FASTTIMER;
	}
}

/*ARGSUSED*/
fcopen(dev, flag)
	dev_t dev;
{
	register struct fc_regs *fcaddr = (struct fc_regs *)ffcons;
	register struct tty *tp;
	register int unit;
	register int maj;
	int inuse;  /*hold state of inuse bit while blocked waiting for carr*/

#	ifdef DEBUG
	if (fcdebug > 1)
		cprintf("fcopen\n");
#	endif DEBUG
	maj = major(dev);
	/*
	 * If a diagnostic console is attached to SLU line 3,
	 * don't allow open of the printer port (also line 3).
	 * This could cause lpr to write to the console.
	 */
	if ((ff_diagcons) && (maj == FCMAJOR)) {
		if((minor(dev)&LINEMASK) == 3) {
		cprintf("out ENXIO 1");
			return (ENXIO);
		}
	}
	unit = minor(dev);
#	ifdef DEBUG
	if (fcdebug > 1)
		cprintf("fcopen: unit = %d\n",unit);
#	endif DEBUG
	if((ff_diagcons) && (maj == CONSOLEMAJOR) && ((unit&LINEMASK) == 0))
		unit |= 3;
	if (unit >= fc_cnt || fcpdma[unit].p_addr == 0) {
#		ifdef DEBUG
		if (fcdebug)
			cprintf("fcopen:fc_cnt=%d, p_addr=%x\n",fc_cnt, fcpdma[unit].p_addr);
#		endif DEBUG
		return (ENXIO);
	}
	/*
	 * Never allow open of device 54/0 (/dev/tty00)
	 * because it conflicts with 0/0 (/dev/console).
	 */
	if ((unit == 0) && (maj == FCMAJOR)) {
#		ifdef DEBUG
		cprintf("out ENXIO 3");
#		endif DEBUG
		return (ENXIO);
	}
	/*
	 * If the console is a graphics device (VAXstation 2000),
	 * don't allow open of device 54/1 (/dev/tty01)
	 * because it conflicts with 0/1 (graphics pointer device).
	if (vs_gdopen && (unit == 1) && (maj == FCMAJOR)) {
#		ifdef DEBUG
		if (fcdebug)
			cprintf("fcopen: out ENXIO 3.5\n");
#		endif DEBUG
		return (ENXIO);
	}
	/*
	 * Call the graphics device open routine
	 * if there is one and the open if for the fancy tube.
	 */
	if ((vs_gdopen && (unit <= 1)) || (vs_gdopen && (unit == 2) &&
	    (maj == CONSOLEMAJOR)))
		return((*vs_gdopen)(dev, flag));
	tp = &fc_tty[unit];
	if (tp->t_state&TS_XCLUDE && u.u_uid != 0){
		return (EBUSY);
	}
	while (tp->t_state&TS_CLOSING) { /* let DTR stay down for awhile */
		sleep((caddr_t)&tp->t_rawq, TTIPRI);
	}
	tp->t_addr = (caddr_t)&fcpdma[unit];
	tp->t_oproc = fcstart;
	tp->t_baudrate = fcbaudrate;

	tty_def_open(tp, dev, flag, (fcsoftCAR[unit>>2]&(1<<(unit&LINEMASK))));

	if ((tp->t_state & TS_ISOPEN) == 0) {
		if (unit == MODEM_UNIT)
			fcmodem = MODEM_DSR_START;
		if((maj == CONSOLEMAJOR) && ((minor(dev)&3) == 0)) {
		    tp->t_cflag &= ~CBAUD;
		    tp->t_cflag = B9600;
		    tp->t_cflag_ext &= ~CBAUD;
		    tp->t_cflag_ext = B9600;
		    tp->t_flags = ANYP|ECHO|CRMOD;
		    tp->t_iflag |= ICRNL; /* Map CRMOD */
		    tp->t_oflag |= ONLCR; /* Map CRMOD */
		}
	} 

	fcparam(unit);		/* enables interrupts */

	(void) spl6();

	/*
	 * No modem control provided for lines with softCAR set.
	 * Modem control provided only for line 2.
	 */
        if ((unit != MODEM_UNIT) || (tp->t_cflag & CLOCAL)) {
		/*
		 * This is a local connection - ignore carrier 
		 * receive enable interrupts enabled above via fcparam() 
		 */
		tp->t_state |= TS_CARR_ON;		/* fcscan sets */
		if (unit == MODEM_UNIT)
			fcaddr->fcdtr |= (FC_RDTR|FC_RRTS);
		/*
	 	 * Set state bit to tell tty.c not to assign this line as the 
	 	 * controlling terminal for the process which opens this line.
	 	 */
		if ((flag & O_NOCTTY) && (u.u_procp->p_progenv == A_POSIX))
			tp->t_state |= TS_ONOCTTY;
		(void) spl0();
		return ((*linesw[tp->t_line].l_open)(dev, tp));
	}
	/* 
	 *  this is a modem line 
	 */
	/* receive enable interrupts enabled above via fcparam() */
	fcaddr->fcdtr |= (FC_RDTR|FC_RRTS);

	/*
	 * After DSR first comes up we must wait for the other signals
	 * before commencing transmission.
         */
        if ((flag & (O_NDELAY|O_NONBLOCK)) == 0) {
		/*
		 * Delay before examining other signals if DSR is being followed
		 * otherwise proceed directly to fc_dsr_check to look for 
		 * carrier detect and clear to send.
		 */
		if (fcdsr) {
			 if ((fcaddr->fcmsr)&FC_RDSR) {
				fcmodem |= (MODEM_DSR_START|MODEM_DSR);
				tp->t_dev = dev; /* need it for timeouts */
				/* 
		 		* Give Carrier and CTS 30 sec. to come up.  
		 		* Prevent any transmission in the first 500ms.
		 		*/
				timeout(fc_dsr_check, tp, hz*30);  
				timeout(fc_dsr_check, tp, hz/2);
			}
		}
		/* 
	 	 * Ignoring DSR so immediately check for CD & CTS.
		 */
		else {
				fcmodem |= (MODEM_DSR_START|MODEM_DSR);
				fc_dsr_check(tp);
		}
	}
#	ifdef DEBUG
	if (fcdebug)
		cprintf("fcopen:  line=%d, state=%x, tp=%x\n", unit,
			tp->t_state, tp);
#	endif DEBUG
        if (flag & (O_NDELAY|O_NONBLOCK))
		tp->t_state |= TS_ONDELAY;
	else
		while ((tp->t_state & TS_CARR_ON) == 0) {
			tp->t_state |= TS_WOPEN;
			inuse = tp->t_state&TS_INUSE;
			sleep((caddr_t)&tp->t_rawq, TTIPRI);
			/*
			 * See if we were awoken by a false call to the modem
			 * line by a non-modem.
 			 */
			if (fcmodem&MODEM_BADCALL){
				(void) spl0();
				return(EWOULDBLOCK);
			}
			/* if we opened "block if in use"  and
			 *  the terminal was not inuse at that time
			 *  but is became "in use" while we were
			 *  waiting for carrier then return
			 */
			if ((flag & O_BLKINUSE) && (inuse==0) &&
				(tp->t_state&TS_INUSE)) {
					(void) spl0();
					return(EALREADY);
			}
		}
	/*
	 * Set state bit to tell tty.c not to assign this line as the 
	 * controlling terminal for the process which opens this line.
	 */
	if ((flag & O_NOCTTY) && (u.u_procp->p_progenv == A_POSIX))
		tp->t_state |= TS_ONOCTTY;
	(void) spl0();
	return ((*linesw[tp->t_line].l_open)(dev, tp));
}

/*ARGSUSED*/
fcclose(dev, flag)
	dev_t dev;
{
	register struct tty *tp;
	register int unit;
	register int s;
	register struct fc_regs *fcaddr = (struct fc_regs *)ffcons;
	int fc;
	extern int wakeup();

	unit = minor(dev);
	if((ff_diagcons) && (major(dev) == CONSOLEMAJOR) && ((unit&LINEMASK) == 0))
		unit |= 3;
	/*
	 * Call the craphics device close routine
	 * if ther is one and the close is for it.
	 */
	if ((vs_gdclose && (unit <= 1)) || (vs_gdclose && (unit == 2) &&
	    (major(dev) == CONSOLEMAJOR))){
		(*vs_gdclose)(dev, flag);
		return;
	}
	fc = unit >> 2;
#	ifdef DEBUG
	if (fcdebug)
		cprintf("fcclose: unit=%x, fc=%x\n",unit,fc);
#	endif DEBUG
	tp = &fc_tty[unit];
	/*
	 * Do line discipline specific close functions then return here
	 * in the old line disc for final closing.
	 */
	if (tp->t_line)
		(*linesw[tp->t_line].l_close)(tp);
	/*
	 * fcbrk is write-only and sends a BREAK (SPACE condition) until
         * the break control bit is cleared. Here we are clearing any
 	 * breaks for this line on close.
	 */
	fcaddr->fcbrk = (fc_brk[fc] &= ~(1 << (unit&LINEMASK)));
	if ((tp->t_cflag&HUPCL) || (tp->t_state&TS_WOPEN) || (tp->t_state&TS_ISOPEN)==0) {
		tp->t_state &= ~TS_CARR_ON;   /* prevents recv intr. timeouts */
		if (unit == MODEM_UNIT) {
			/*
		 	 * Drop appropriate signals to terminate the connection.
		 	 */
			fcaddr->fcdtr &= ~(FC_RDTR|FC_RRTS);
                        if ((tp->t_cflag & CLOCAL) == 0) {
				s = spl6();
				/*drop DTR for at least a sec. if modem line*/
#				ifdef DEBUG
				if (fcdebug)
					cprintf("fcclose: DTR drop, state =%x\n"
						,tp->t_state);
#				endif DEBUG
				tp->t_state |= TS_CLOSING;
				/*
			 	 * Wait at most 5 sec for DSR to go off.  
			 	 * Also hold DTR down for a period.
			 	 */
				if (fcdsr && (fcaddr->fcmsr & FC_RDSR)) {
					timeout(wakeup,(caddr_t)&tp->t_dev,5*hz);
					sleep((caddr_t)&tp->t_dev, PZERO-10);
				}
				/*
			 	 * Hold DTR down for 200+ ms.
			 	 */
				timeout(wakeup, (caddr_t) &tp->t_dev, hz/5);
				sleep((caddr_t)&tp->t_dev, PZERO-10);
	
				tp->t_state &= ~(TS_CLOSING);
				wakeup((caddr_t)&tp->t_rawq);
				splx(s);
			}
		}
		/*
		 * No disabling of interrupts is done.  Characters read in on
		 * a non-open line will be discarded.
		 */
	}
	/* reset line to default mode */
	fcsoftCAR[fc] &= ~(1<<(unit&LINEMASK));
	fcsoftCAR[fc] |= (1<<(unit&LINEMASK)) & fcdefaultCAR[fc];
	if (unit == MODEM_UNIT)
		fcmodem = 0;
	ttyclose(tp);
	tty_def_close(tp);
}

/*
 * fcread() shared with graphics device drivers (sm & sg).
 */

fcread(dev, uio)
	dev_t dev;
	struct uio *uio;
{
	register struct tty *tp;
	register int unit;

	unit = minor(dev);
	if((ff_diagcons) && (major(dev) == CONSOLEMAJOR) && ((unit&LINEMASK) == 0))
		unit |= 3;
	if((unit == 1) && vs_gdread)
	    return((*vs_gdread)(dev, uio));
	if (vs_gdopen && (unit == 2) && (major(dev) == CONSOLEMAJOR))
	    tp = &sm_tty;
	else
	    tp = &fc_tty[unit];
	return ((*linesw[tp->t_line].l_read)(tp, uio));
}

/*
 * fcwrite() shared with graphics device drivers (sm & sg).
 */

fcwrite(dev, uio)
	dev_t dev;
	struct uio *uio;
{
	register struct tty *tp;
	register int unit;

	unit = minor(dev);
	if((ff_diagcons) && (major(dev) == CONSOLEMAJOR) && ((unit&LINEMASK) == 0))
		unit |= 3;
	if((unit == 1) && vs_gdwrite)	
	    return((*vs_gdwrite)(dev, uio));
	/*
	 * Don't allow writes to the mouse,
	 * just fake the I/O and return.
	 */
	if (vs_gdopen && (unit == 1)) {
		uio->uio_offset = uio->uio_resid;
		uio->uio_resid = 0;
		return(0);
	}
	if (vs_gdopen && (unit == 2) && (major(dev) == CONSOLEMAJOR))
	    tp = &sm_tty;
	else
	    tp = &fc_tty[unit];
	return ((*linesw[tp->t_line].l_write)(tp, uio));
}

fcselect(dev, rw)
dev_t dev;
{
	register int unit = minor(dev);
	
	if((ff_diagcons) && (major(dev) == CONSOLEMAJOR) && ((unit&3) == 0))
		dev |= 3;
	if(((unit == 1) || (unit == 2)) && vs_gdselect &&
	   (major(dev) == CONSOLEMAJOR))
		return((*vs_gdselect)(dev, rw));
	else
		return(ttselect(dev, rw));
}

/*
 * Used to pass mouse (or tablet) reports to the graphics
 * device driver interrupt service routine.
 * Entire report passed instead of byte at a time.
 */
struct	mouse_report	current_rep;
u_short	sm_pointer_id;
#define	MOUSE_ID	0x2
 
/*ARGSUSED*/
fcrint(fc)
	int fc;
{
	register struct tty *tp;
	register int c;
	register struct fc_regs *fcaddr = (struct fc_regs *)ffcons;
	register struct tty *tp0;
	register int unit;
	register int flg;
	int overrun = 0;
	struct fc_softc *sc;
	struct mouse_report *new_rep;
	u_short data;

	if ((fcact & (1<<fc)) == 0)
		return;
	unit = fc * 4;
	tp0 = &fc_tty[unit];
	new_rep = &current_rep; 			/* mouse report pointer */

	while (fcaddr->fccsr&FC_RDONE) {	/* character present */
		c = fcaddr->fcrbuf;
		fcchars[fc]++;
		unit = (c>>8)&LINEMASK;
		tp = tp0 + unit;
		if (tp >= &fc_tty[fc_cnt])
			continue;
		sc = &fc_softc[fc];
		/*
		 * If console is a graphics device,
		 * pass keyboard input characters to
		 * its device driver's receive interrupt routine.
		 * Save up complete mouse report and pass it.
		 */
/*cprintf("FCDRIVER unit = %d  c = %d ID = %d\n",unit, c, sm_pointer_id);*/
		if ((unit <= 1) && vs_gdkint) {
		    if(unit == 0) {		/* keyboard char */
			(*vs_gdkint)(c);
			continue;
		    } else {			/* mouse or tablet report */
			if (sm_pointer_id == MOUSE_ID) { /* mouse report */
			    data = c & 0xff;	/* get report byte */
			    ++new_rep->bytcnt;	/* inc report byte count */

			    if (data & START_FRAME) { /* 1st byte of report? */
				new_rep->state = data;
				if (new_rep->bytcnt > 1)
			            new_rep->bytcnt = 1;  /* start new frame */
			    }

			    else if (new_rep->bytcnt == 2)	/* 2nd byte */
			            new_rep->dx = data;

			    else if (new_rep->bytcnt == 3) {	/* 3rd byte */
				    new_rep->dy = data;
				    new_rep->bytcnt = 0;
				    (*vs_gdkint)(0400);	/* 400 says line 1 */
			    }
			    continue;
			} else { /* tablet report */
			    data = c;	/* get report byte */
			    ++new_rep->bytcnt;	/* inc report byte count */

			    if (data & START_FRAME) { /* 1st byte of report? */
				new_rep->state = data;
				if (new_rep->bytcnt > 1)
			            new_rep->bytcnt = 1;  /* start new frame */
			    }

			    else if (new_rep->bytcnt == 2)	/* 2nd byte */
			            new_rep->dx = data & 0x3f;

			    else if (new_rep->bytcnt == 3)  	/* 3rd byte */
				    new_rep->dx |= (data & 0x3f) << 6;

			    else if (new_rep->bytcnt == 4)  	/* 4th byte */
			            new_rep->dy = data & 0x3f;

			    else if (new_rep->bytcnt == 5){ 	/* 5th byte */
				    new_rep->dy |= (data & 0x3f) << 6;
				    new_rep->bytcnt = 0;
				    (*vs_gdkint)(0400);	/* 400 says line 1 */
			    }
			    continue;
			}
		    }
		}
		if ((tp->t_state & TS_ISOPEN) == 0) {
			wakeup((caddr_t)&tp->t_rawq);
#ifdef PORTSELECTOR
			if ((tp->t_state&TS_WOPEN) == 0)
#endif
			continue;
		}
		flg = tp->t_iflag;

/*		This code handles the following termio input flags.  Also
 *		listed is what the default should be for propper Ultrix
 *		backward compatibility.
 *
 *		IGNBRK		FALSE
 *		BRKINT		TRUE
 *		IGNPAR		TRUE
 *		PARMRK		FALSE
 *		INPCK		TRUE
 *		ISTRIP		TRUE 		
 */


		/* FC_FE is interpreted as a break */
		if (c & FC_FE) {
			/*
			 * If configured for trusted path, initiate
			 * trusted path handling.
			 */
			if (do_tpath) {
				tp->t_tpath |= TP_DOSAK;
				(*linesw[tp->t_line].l_rint)(c, tp);
				break;
			}
			if (flg & IGNBRK)
				continue;
			if (flg & BRKINT) {
#				ifdef FCDEBUG
				if (fcdebug)
					mprintf("fcrint: BREAK RECEIVED\n");
#				endif FCDEBUG
				if ((tp->t_lflag_ext & PRAW) && 
					(tp->t_line != TERMIODISC))
					c = 0;
				else {
				    ttyflush(tp, FREAD|FWRITE);
#ifdef FCDEBUG
				    if (fcdebug)
					mprintf("sending signal to tp->t_pgrp = %d\n", tp->t_pgrp);
#endif FCDEBUG
				    gsignal(tp->t_pgrp, SIGINT);
				    continue;
				}
			}
			/*
			 * TERMIO: If neither IGNBRK or BRKINT is set, a
			 * break condition is read as a single '\0',
			 * or if PARMRK is set as '\377','\0,'\0'.
			 */
			else {
				if (flg & PARMRK){
					(*linesw[tp->t_line].l_rint)(0377,tp);
					(*linesw[tp->t_line].l_rint)(0,tp);
				}
				c = 0;
			}
		
		}
		/* Parity Error */
		else if (c & FC_PE){
			/* 
			 * If input parity checking is not enabled, clear out
			 * parity error in this character.
			 */
#			ifdef FCDEBUG
			if (fcdebug > 1)
				mprintf("fcrint: Parity Error\n");
#			endif FCDEBUG
			if ((flg & INPCK) == 0)
				c &= ~FC_PE;
			else {
				if (flg & IGNPAR)
					continue;
				/* If PARMRK is set, return a character with
				 * framing or parity errors as a 3 character
				 * sequence (0377,0,c).
				 */
				if (flg & PARMRK){
					(*linesw[tp->t_line].l_rint)(0377,tp);
					(*linesw[tp->t_line].l_rint)(0,tp);
				}
				/*
				 * TERMIO: If neither PARMRK or IGNPAR is set, a
				 * parity error is read as a single '\0'.
				 */
				else 
					c = 0;
			}
		}

		/* SVID does not say what to do with overrun errors */
		if (c&FC_DO) {
			if(overrun == 0) {
				printf("fc%d: input silo overflow\n", fc);
				overrun = 1;
			}
			sc->sc_softcnt[unit]++;
		}

		if (flg & ISTRIP){
			c &= 0177;	
		}
		else {
			c &= 0377;	
			/* If ISTRIP is not set a valid character of 377
			 * is read as 0377,0377 to avoid ambiguity with
			 * the PARMARK sequence.
			 */ 
			if ((c == 0377) && (tp->t_line == TERMIODISC) &&
			    (flg & PARMRK))
				(*linesw[tp->t_line].l_rint)(0377,tp);
		}
#if NHC > 0
		if (tp->t_line == HCLDISC) {
			HCINPUT(c, tp);
		} else
#endif
		(*linesw[tp->t_line].l_rint)(c, tp);
	}
}

/*ARGSUSED*/
fcioctl(dev, cmd, data, flag)
	dev_t dev;
	register int cmd;
	caddr_t data;
	int flag;
{
	register int unit;
	register struct tty *tp;
	register int fc;
	register struct fc_regs *fcaddr = (struct fc_regs *)ffcons;
	register int s;
	struct uba_device *ui;
	struct fc_softc *sc;
	struct devget *devget;
	int error;

	unit = minor(dev);
	if((ff_diagcons) && ((unit&LINEMASK) == 0))
		unit |= 3;
	/*
	 * If there is a graphics device and the ioctl call
	 * is for it, pass the call to the graphics driver.
	 */
	if (vs_gdioctl && (unit <= 1)) {
		error = (*vs_gdioctl)(dev, cmd, data, flag);
		return(error);
	}
	if (vs_gdioctl && (unit == 2) && (major(dev) == CONSOLEMAJOR)) {
		error = (*vs_gdioctl)(dev, cmd, data, flag);
		return(error);
	}
	tp = &fc_tty[unit];
	fc = unit >> 2;
	ui = fcinfo[fc];
	sc = &fc_softc[ui->ui_unit];
	error = (*linesw[tp->t_line].l_ioctl)(tp, cmd, data, flag);
	if (error >= 0)
		return (error);
	error = ttioctl(tp, cmd, data, flag);
	if (error >= 0) {
		/*
		 * If the call is to set terminal attributes which are
		 * represented in the device's line parameter register then
		 * call the param routine to update the device registers.
		 */
 		switch(cmd) {
			case TCSANOW:			/* POSIX termios */
			case TCSADRAIN:			/* POSIX termios */
			case TCSAFLUSH:			/* POSIX termios */
 			case TCSETA:			/* SVID termio */
 			case TCSETAW:			/* SVID termio */
 			case TCSETAF:			/* SVID termio */
 			case TIOCSETP:			/* Berkeley sgttyb */
 			case TIOCSETN:			/* Berkeley sgttyb */
			case TIOCLBIS:			/* Berkeley lmode */
			case TIOCLBIC:			/* Berkeley lmode */
			case TIOCLSET:			/* Berkeley lmode */
			case TIOCLGET:			/* Berkeley lmode */
 				fcparam(unit);
 				break;
 		}
		return (error);
	}
	switch (cmd) {

	case TIOCSBRK:
		fcaddr->fcbrk = (fc_brk[fc] |= 1 << (unit&LINEMASK));
		break;

	case TIOCCBRK:
		fcaddr->fcbrk = (fc_brk[fc] &= ~(1 << (unit&LINEMASK)));
		break;

	case TIOCSDTR:
		(void) fcmctl(dev, FC_DTR|FC_RTS, DMBIS);
		break;

	case TIOCCDTR:
		(void) fcmctl(dev, FC_DTR|FC_RTS, DMBIC);
		break;

	case TIOCMSET:
		(void) fcmctl(dev, dmtofc(*(int *)data), DMSET);
		break;

	case TIOCMBIS:
		(void) fcmctl(dev, dmtofc(*(int *)data), DMBIS);
		break;

	case TIOCMBIC:
		(void) fcmctl(dev, dmtofc(*(int *)data), DMBIC);
		break;

	case TIOCMGET:
		*(int *)data = fctodm(fcmctl(dev, 0, DMGET));
		break;

	case TIOCNMODEM:  /* ignore modem status */
		/* 
		 * By setting the software representation of modem signals
		 * to "on" we fake the system into thinking that this is an
		 * established modem connection.
		 */
		s = spl6();
		fcsoftCAR[fc] |= (1<<(unit&LINEMASK));  
		if (*(int *)data) /* make mode permanent */
			fcdefaultCAR[fc] |= (1<<(unit&LINEMASK));  
		tp->t_state |= TS_CARR_ON;
		tp->t_cflag |= CLOCAL;		/* Map to termio */
		splx(s);
		break;

	case TIOCMODEM:  /* look at modem status - sleep if no carrier */
		if(unit != MODEM_UNIT)
			break;		/* ONLY line 2 has modem control */
		s = spl6();
		fcsoftCAR[fc] &= ~(1<<(unit&LINEMASK));  
		if (*(int *)data) /* make mode permanent */
			fcdefaultCAR[fc] &= ~(1<<(unit&LINEMASK));  
		/*
		 * See if all signals necessary for modem connection are present
		 * 
	    	 * If fcdsr is set look for DSR|CTS|CD, otherwise look 
	    	 * for CD|CTS only.
	    	 */
		if ((fcdsr && ((fcaddr->fcmsr&FC_XMIT) == FC_XMIT)) ||
		   ((fcdsr==0) && ((fcaddr->fcmsr&FC_NODSR) == FC_NODSR))) {
			tp->t_state &= ~(TS_ONDELAY);
			tp->t_state |= TS_CARR_ON;
			fcmodem = MODEM_CTS|MODEM_CD|MODEM_DSR;
		}
		else {
			tp->t_state &= ~(TS_CARR_ON);
			fcmodem &= ~(MODEM_CTS|MODEM_CD|MODEM_DSR);
		}
		tp->t_cflag &= ~CLOCAL;		/* Map to termio */
		splx(s);
		break;

	case TIOCWONLINE:
		if(unit != MODEM_UNIT)
			break;		/* ONLY line 2 has modem control */
		s = spl6();
		/*
		 * See if all signals necessary for modem connection are present
		 * 
	    	 * If fcdsr is set look for DSR|CTS|CD, otherwise look 
	    	 * for CD|CTS only.
	    	 */
		if ((fcdsr && ((fcaddr->fcmsr&FC_XMIT) == FC_XMIT)) ||
		   ((fcdsr==0) && ((fcaddr->fcmsr&FC_NODSR) == FC_NODSR))) {
			tp->t_state |= TS_CARR_ON;
			tp->t_state &= ~(TS_ONDELAY);
			fcmodem = MODEM_CTS|MODEM_CD|MODEM_DSR;
		}
		else
			while ((tp->t_state & TS_CARR_ON) == 0) 
				sleep((caddr_t)&tp->t_rawq, TTIPRI);
		splx(s);
		break;

	case DEVIOCGET: 			/* device status */
		devget = (struct devget *)data;
		bzero(devget,sizeof(struct devget));
		if(unit == MODEM_UNIT) {
                    if (tp->t_cflag & CLOCAL) {
		       sc->sc_category_flags[unit&LINEMASK] |= DEV_MODEM;
		       sc->sc_category_flags[unit&LINEMASK] &= ~DEV_MODEM_ON;
		    } else
		       sc->sc_category_flags[unit&LINEMASK] |= (DEV_MODEM|DEV_MODEM_ON);
		}
		devget->category = DEV_TERMINAL;	/* terminal cat.*/
		devget->bus = DEV_NB;			/* NO bus	*/
		bcopy(DEV_FF_SLU,devget->interface,
		      strlen(DEV_FF_SLU));		/* interface	*/
		bcopy(DEV_UNKNOWN,devget->device,
		      strlen(DEV_UNKNOWN));		/* terminal	*/
		devget->adpt_num = 0;			/* NO adapter	*/
		devget->nexus_num = 0;			/* fake nexus 0	*/
		devget->bus_num = 0;			/* NO bus	*/
		devget->ctlr_num = fc;			/* cntlr number */
		devget->slave_num = unit&LINEMASK;	/* line number 	*/
		bcopy(ui->ui_driver->ud_dname,
		      devget->dev_name,
		      strlen(ui->ui_driver->ud_dname)); /* Ultrix "fc"	*/
		devget->unit_num = unit&LINEMASK;	/* fc line?	*/
		devget->soft_count =
		      sc->sc_softcnt[unit&LINEMASK];	/* soft err cnt	*/
		devget->hard_count =
		      sc->sc_hardcnt[unit&LINEMASK];	/* hard err cnt	*/
		devget->stat = sc->sc_flags[unit&LINEMASK]; /* status	*/
		devget->category_stat =
		      sc->sc_category_flags[unit&LINEMASK]; /* cat. stat. */
		break;

	default:
		if (u.u_procp->p_progenv == A_POSIX) 
			return (EINVAL);
		return (ENOTTY);
	}
	return (0);
}

dmtofc(bits)
	register int bits;
{
	register int b;

	b = (bits >>1) & 0370;
	if (bits & SML_ST) b |= FC_ST;
	if (bits & SML_RTS) b |= FC_RTS;
	if (bits & SML_DTR) b |= FC_DTR;
	if (bits & SML_LE) b |= FC_LE;
	return(b);
}

fctodm(bits)
	register int bits;
{
	register int b;

	b = (bits << 1) & 0360;
	if (bits & FC_DSR) b |= SML_DSR;
	if (bits & FC_DTR) b |= SML_DTR;
	if (bits & FC_ST) b |= SML_ST;
	if (bits & FC_RTS) b |= SML_RTS;
	return(b);
}

fcparam(unit)
	register int unit;
{
	register struct tty *tp;
	register struct fc_regs *fcaddr = (struct fc_regs *)ffcons;
	register int lpr;

	tp = &fc_tty[unit];
	if (fcsilos & (1 << (unit >> 2)))
		fcaddr->fccsr = FC_MSE | FC_SAE;
	else
		fcaddr->fccsr = FC_MSE;
/*
 *	Reversing the order of the following two lines fixes the
 *	problem where the console device locks up if you type a
 *	character during autoconf and you must halt/continue to
 *	unlock the console. Interrupts were being enabled on the SLU
 *	before the fcact flag was set, so the driver would just return
 *	and not process the waiting character (caused by you typing).
 *	This locked up the cosnole SLU (interrupt posted but never really
 *	servcied). Halting the system caused the console firmware to unlock
 *	the SLU because it needs to use it.
 *	Should fcparam() be called as spl6??????
 */
	fcact |= (1<<(unit>>2));
	fcaddr->fccsr |= FC_RIE | FC_TIE;
	if ((((tp->t_cflag&CBAUD)==B0) && (u.u_procp->p_progenv != A_POSIX)) || 
	    (((tp->t_cflag_ext & CBAUD)==B0) && 
		(u.u_procp->p_progenv == A_POSIX))) { 
		(void) fcmctl(unit, FC_OFF, DMSET);	/* hang up line */
		return;
	}
	/*
	 * If diagnostic console on line 3,
	 * line parameters must be: 9600 BPS, 8 BIT, NO PARITY, 1 STOP.
	 * Same for color/monochrome video, except 4800 BPS.
	 * Mouse/tablet: 4800 BPS, 8 BIT, ODD PARITY, 1 STOP.
	 * If none of the above, assume attached console on line 0,
	 * same paramaters as diagnostic console on line 3.
	 */
	if ((unit == 3) && (ff_diagcons))
		fcaddr->fclpr = (FC_RE | FC_B9600 | BITS8 | 3);
	else if (unit == 0) {
		if (vs_gdopen)
			fcaddr->fclpr = (FC_RE | FC_B4800 | BITS8);
		else
			fcaddr->fclpr = (FC_RE | FC_B9600 | BITS8);
	}
	else if (vs_gdopen && (unit == 1))
		fcaddr->fclpr = (FC_RE | FC_B4800 | OPAR | PENABLE | BITS8 | 1);
	/*
	 * Set parameters in accordance with user specification.
	 */
	else {

		lpr = (fc_speeds[tp->t_cflag&CBAUD]<<8) | (unit & LINEMASK);
		/*
	 	 * Berkeley-only dinosaur
	 	 */
		if (tp->t_line != TERMIODISC) {
			if ((tp->t_cflag_ext&CBAUD) == B110){
				lpr |= TWOSB;
				tp->t_cflag |= CSTOPB;
			}
		}
	 	/*
		 * Set device registers according to the specifications of the 
		 * termio structure.
	 	 */
 		if ((tp->t_cflag & CREAD) == 0)
 			lpr &= ~FC_RE;	/* This was set from speeds */
 		if (tp->t_cflag & CSTOPB)
 			lpr |= TWOSB;
 		else
 			lpr &= ~TWOSB;
 		if (tp->t_cflag & PARENB) {
 			if ((tp->t_cflag & PARODD) == 0)
 				/* set even */
 				lpr = (lpr | PENABLE)&~OPAR;
 			else
 				/* else set odd */
 				lpr |= PENABLE|OPAR;
 		}
 		/*
 		 * character size.
 		 * clear bits and check for 6,7,and 8, else its 5 bits.
 		 */
 		lpr &= ~BITS8;
 		switch(tp->t_cflag&CSIZE) {
 			case CS6:
 				lpr |= BITS6;
 				break;
 			case CS7:
 				lpr |= BITS7;
 				break;
 			case CS8:
 				lpr |= BITS8;
 				break;
 		}
#		ifdef DEBUG
		if (fcdebug)
			mprintf("fcparam: tp = %x, lpr = %x\n",tp,lpr);
#		endif DEBUG
		fcaddr->fclpr = lpr;
	}
}

/*ARGUSED*/
fcxint(tp)
	register struct tty *tp;
{
	register struct fc_regs *fcaddr = (struct fc_regs *)ffcons;
	register struct pdma *dp;
	register fc, unit;

	dp = (struct pdma *)tp->t_addr;
	fc = minor(tp->t_dev) >> 2;
	unit = minor(tp->t_dev) & 3;
	if ((ff_diagcons) && (unit == 0) && (major(tp->t_dev) == CONSOLEMAJOR)) {
		unit = 3;
		fc = 0;
	}

	/*
	 * Don't know if the following is true for the
	 * VAXstar SLU, but it stays in just to be safe.
	 * Fred Canter -- 6/28/86
	 */
	/* it appears that the fc will ocassionally give spurious
	   transmit ready interupts even when not enabled.  If the
	   line was never opened, the following is necessary */

	if (dp == NULL) {
		tp->t_addr = (caddr_t) &fcpdma[unit];
		dp = (struct pdma *) tp->t_addr;
	}
	tp->t_state &= ~TS_BUSY;
	if (tp->t_state & TS_FLUSH)
		tp->t_state &= ~TS_FLUSH;
	else {
		ndflush(&tp->t_outq, dp->p_mem-tp->t_outq.c_cf);
		dp->p_end = dp->p_mem = tp->t_outq.c_cf;
	}
	if (tp->t_line)
		(*linesw[tp->t_line].l_start)(tp);
	else
		fcstart(tp);
	/* The BUSY flag will not be set in two cases:		  */
	/*   1: if there are no more chars in the outq  OR	  */
	/*   2. there are chars in the outq but tty is in stopped */
	/*	state.						  */
	if ((tp->t_state&TS_BUSY)==0)
		fcaddr->fctcr &= ~(1<<unit);
	else
		fcaddr->fccsr |= FC_TIE;
}

fcstart(tp)
	register struct tty *tp;
{
	register struct pdma *dp;
	register struct fc_regs *fcaddr = (struct fc_regs *)ffcons;
	register int cc;
	int s, fc, unit;

	dp = (struct pdma *)tp->t_addr;
	s = spl6();
	/*
	 * Do not do anything if currently delaying, or active.  Also only
	 * transmit when CTS is up.
	 */
	unit = minor(tp->t_dev) & 3;
	if ((tp->t_state & (TS_TIMEOUT|TS_BUSY|TS_TTSTOP)) ||
                (unit == MODEM_UNIT) && ((tp->t_cflag & CLOCAL) == 0)
		&& ((tp->t_state&TS_CARR_ON) && (fcmodem&MODEM_CTS)==0))
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
	if ((tp->t_lflag_ext & PRAW) || (tp->t_oflag_ext & PLITOUT) || 
	    ((tp->t_oflag & OPOST) == 0))
		cc = ndqb(&tp->t_outq, 0);
	else {
		cc = ndqb(&tp->t_outq, DELAY_FLAG);
		if (cc == 0) {
			cc = getc(&tp->t_outq);
			timeout(ttrstrt, (caddr_t)tp, (cc&0x7f) + 6);
			tp->t_state |= TS_TIMEOUT;
			goto out;
		}
	}
	tp->t_state |= TS_BUSY;
	dp->p_end = dp->p_mem = tp->t_outq.c_cf;
	dp->p_end += cc;
	if ((ff_diagcons) && (unit == 0) && (major(tp->t_dev) == CONSOLEMAJOR))
		unit = 3;
	/*
	 * Enable transmitter interrupts
	 */
	fcaddr->fccsr |= FC_TIE;
	fcaddr->fctcr |= (1<<unit);
out:
	splx(s);
}

/*
 * Stop output on a line.
 */

/*ARGSUSED*/
fcstop(tp, flag)
	register struct tty *tp;
{
	register struct pdma *dp;
	register int s;
	int	unit;

	/*
	 * If there is a graphics device and the stop call
	 * is for it, pass the call to the graphics device driver.
	 */
	unit = minor(tp->t_dev);
	if (vs_gdstop && (unit <= 1)) {
		(*vs_gdstop)(tp, flag);
		return;
	}
	dp = (struct pdma *)tp->t_addr;
	s = spl6();
	if (tp->t_state & TS_BUSY) {
		dp->p_end = dp->p_mem;
		if ((tp->t_state&TS_TTSTOP)==0)
			tp->t_state |= TS_FLUSH;
	}
	splx(s);
}

/*
 *	***** FOR YOUR INFORMATION *****
 *
 *	     Fred Canter -- 7/1/86
 *
 * 	The VAXstar console SLU supports modem control
 *	only on line 2. Modem control ioctls for other lines
 *	will not return an error, but also will not do anything!
 *	Hopefully, this is the lesser of two evils when it comes to
 *	breaking users' programs.
 *
 *	Line 2 has more modem control signals than ULTRIX uses
 *	right now (see VAXstar system unit spec. chapter 11).
 *
 *	CAUTION: the SML_* definitions in fcreg.h must match the
 *	TIOCM_* definitions in ioctl.h.
 *
 *	The line number in the dev argument to this routine will be
 *	wrong (0 s/b 3) if VS_L3CON is set in the configuration and test
 *	register, i.e., a diagnostic console terminal is attached to
 *	the printer port. This fact is ignored because this routine only
 *	acts on line 2 anyway.
 *
 */

fcmctl(dev, bits, how)
	dev_t dev;
	int bits, how;
{
	register struct fc_regs *fcaddr = (struct fc_regs *)ffcons;
	register int unit, mbits;
	int b, s;

	unit = minor(dev);
	if(unit != MODEM_UNIT)
		return(0);	/* only line 2 has modem control */
	b = 0x4;
	s = spl6();
	mbits = (fcaddr->fcdtr & FC_RDTR) ? FC_DTR : 0;
	mbits |= (fcaddr->fcdtr & FC_RRTS) ? FC_RTS : 0;
	mbits |= (fcaddr->fcmsr & FC_RCD) ? FC_CD : 0;
	mbits |= (fcaddr->fcmsr & FC_RDSR) ? FC_DSR : 0;
	mbits |= (fcaddr->fcmsr & FC_RCTS) ? FC_CTS : 0;
	mbits |= (fcaddr->fctbuf & b) ? FC_RI : 0;
	switch (how) {
	/* No actual bit settings in device are really done ! */
	case DMSET:
		mbits = bits;
		break;

	case DMBIS:
		mbits |= bits;
		break;

	case DMBIC:
		mbits &= ~bits;
		break;

	case DMGET:
		(void) splx(s);
		return(mbits);
	}
	if (mbits & FC_DTR)
		fcaddr->fcdtr |= (b|FC_RRTS);
	else
		fcaddr->fcdtr &= ~(b|FC_RRTS);
	(void) splx(s);
	return(mbits);
}

/*
 * Allows silo alarm mode, if set.
 * Enabling silo alarm mode will most likely
 * cause silo overrun errors. The system can't
 * seem to keep up?????????
 */

fc_allow_silos = 0;

int fctransitions, fcfasttimers;		/*DEBUG*/
fcscan()
{
	register i;
	register struct fc_regs *fcaddr = (struct fc_regs *)ffcons;
	register bit;
	register struct tty *tp;
	int oldfcsilos = fcsilos;
	int fctimer();
	register u_char fccan_modem;

	for (i = 0; i < fc_cnt; i++) {
		if (fcpdma[i].p_addr == 0)
			continue;
		tp = &fc_tty[i];
		fccan_modem = 0;
		/* Modem Control only on line 2 */
                if ((i == MODEM_UNIT) && ((tp->t_cflag & CLOCAL) == 0)) {
			/*
			 * Drop DTR immediately if DSR has gone away.
	                 * If really an active close then do not
	                 *    send signals.
			 */
			if (!(fcaddr->fcmsr&FC_RDSR)) {
	                        if (tp->t_state&TS_CLOSING) {
	                              untimeout(wakeup, (caddr_t) &tp->t_dev);
	                              wakeup((caddr_t) &tp->t_dev);
	                        }
				if (tp->t_state&TS_CARR_ON) {
					/*
					 * Only drop if DECSTD52 is followed.
					 */
					if (fcdsr)
						fc_tty_drop(tp);
				}
			}
			else {		/* DSR has come up */
				/*
				 * If DSR comes up for the first time we allow
				 * 30 seconds for a live connection.
				 */
				if (fcdsr && ((fcmodem&MODEM_DSR)==0)) {
					fcmodem |= (MODEM_DSR_START|MODEM_DSR);
					/* 
				 	* we should not look for CTS|CD for
				 	* about 500 ms.  
				 	*/
					timeout(fc_dsr_check, tp, hz*30);
					timeout(fc_dsr_check, tp, hz/2);
			    	}
			}
			/*
			 * Look for modem transitions in an already established
			 * connection.
			 */
			if (tp->t_state & TS_CARR_ON) {
				if (fcaddr->fcmsr&FC_RCD) {
					/*
					 * CD has come up again.
					 * Stop timeout from occurring if set.
					 * If interval is more than 2 secs then
					 *  drop DTR.
					 */
					if ((fcmodem&MODEM_CD)==0) { 
						untimeout(fc_cd_drop, tp);
						if (fc_cd_down(tp)){
							/* drop connection */
							fc_tty_drop(tp);
						}
						fcmodem |= MODEM_CD;
					}
				}
				else {
					/* 
					 * Carrier must be down for greater than 2 secs
					 * before closing down the line.
					 */
					if ( fcmodem & MODEM_CD) {
					    /* only start timer once */
					    fcmodem &= ~MODEM_CD;
					    /* 
					     * Record present time so that if carrier
					     * comes up after 2 secs , the line will drop.
					     */
					    fctimestamp = time;
					    timeout(fc_cd_drop, tp, hz*2);
					}
				}
				/* CTS flow control check */
		
				if (!(fcaddr->fcmsr&FC_RCTS)) {
					/* 
					 * Only allow transmission when CTS is set.
					 */
					tp->t_state |= TS_TTSTOP;
					fcmodem &= ~MODEM_CTS;
#					ifdef DEBUG
					if (fcdebug)
					   cprintf("fcscan: CTS stop, tp=%x,line=%d\n", tp,i);
#					endif DEBUG
					fcstop(tp, 0);
				} else if ((fcmodem&MODEM_CTS)==0) { 
					    /* 
					     * Restart transmission upon return of CTS.
					     */
					    tp->t_state &= ~TS_TTSTOP;
					    fcmodem |= MODEM_CTS;
#					    ifdef DEBUG
					    if (fcdebug)
					       cprintf("fcscan: CTS start, tp=%x,line=%d\n", tp,i);
#					    endif DEBUG
					    fcstart(tp);
				}
	
	
			} 
			
			/* 
			 * See if a modem transition has occured.  If we are waiting
			 * for this signal cause action to be taken via fc_tty_start.
			 */
			 if ((fccan_modem=fcaddr->fcmsr&FC_XMIT) != fccan_previous){
				/*
			 	* If 500 ms timer has not expired then dont
			 	* check anything yet.
			 	* Check to see if DSR|CTS|CD are asserted.
			 	* If so we have a live connection.
			 	*/
#				ifdef DEBUG
				if (fcdebug)
					cprintf("fcscan: MODEM Transition,fccan_modem=%x, fccan_prev=%x\n",fccan_modem,fccan_previous);
#				endif DEBUG
	   			/* 
	    	 		 * If fcdsr is set look for DSR|CTS|CD,else look
	    	 		 * for CD|CTS only.
	    	 		 */
				if (fcdsr) {
				    if (((fcaddr->fcmsr&FC_XMIT)==FC_XMIT) 
				        && ((fcmodem&MODEM_DSR_START)==0)
				        && ((tp->t_state&TS_CARR_ON)==0)) {
#					    ifdef DEBUG
					    if (fcdebug)
						cprintf("fcscan:FC_XMIT call fc_start,line=%d\n",i);
#				            endif DEBUG
					    fc_start_tty(tp);
				    }
				}
				/* 
				 * Ignore DSR.	
				 */
				else
					if ((fcaddr->fcmsr&FC_NODSR)==FC_NODSR)
						fc_start_tty(tp);
						
			}
			fccan_previous = fccan_modem; /* save for next iteration */
		}
	}
	for (i = 0; i < nNFC; i++) {
		ave(fcrate[i], fcchars[i], 8);
		/*
		 * Allow silo mode only if no graphics device.
		 * Silo mode blows away with mouse tracking.
		 */
		if (fc_allow_silos) {
		    if (fcchars[i] > fchighrate && ((fcsilos&(1 << i)) == 0)) {
			fcaddr->fccsr = FC_MSE | FC_SAE;
			fcsilos |= (1 << i);
			fctransitions++;		/*DEBUG*/
		    }
		    if ((fcsilos&(1 << i)) && (fcrate[i] < fclowrate)) {
			fcaddr->fccsr = FC_MSE;
			fcsilos &= ~(1 << i);
		    }
		}
		fcchars[i] = 0;
	}
	if (fcsilos && !oldfcsilos)
		timeout(fctimer, (caddr_t)0, fctimerintvl);
	timeout(fcscan, (caddr_t)0, hz);
}

fctimer()
{
	register int fc;
	register int s;

	if (fcsilos == 0)
		return;
	s = spl6();
	fcfasttimers++;		/*DEBUG*/
	for (fc = 0; fc < nNFC; fc++)
		if (fcsilos & (1 << fc))
			fcrint(fc);
	splx(s);
	timeout(fctimer, (caddr_t) 0, fctimerintvl);
}

/* save record of sbis present for sbi error logging for 780 and 8600 */
extern long sbi_there;	/* bits 0-15 for nexi,sbi0; 16-31 for nexi on sbi1*/

/*
 * VAXstar virtual console initialization routine.
 * Configures the VAXstar serial line controller as the console,
 * maps in all of the VAXstar's address space, and
 * decides if LK201 keystrokes must be passed to the bit-map
 * or color driver's console routines.
 */
int	fcputc();
int	fcgetc();

extern	(*v_consputc)();
extern	(*v_consgetc)();
extern	int	fgcons_init();

fccons_init()
{
	register struct fc_regs *fcaddr;
	register struct fbic_regs *fbp;
	register struct pte *pteptr;
	register struct mb_node *mbp;
	int	pri_cpuid;
	int	i, base;
	int	constype;
	int	memctlr = 0;
	char *iocsraddr, *io_base_addr;
	int	*scbptr;


	pri_cpuid = mfpr(CPUID);
	/*
	 * The constant MB_2ND_FBIC_OFFSET is used here as which processor
	 * this is, is determined from the CPUID register bit 1.
	 */
	cpu_fbic_addr = (long)(MB_SLOT0_BASE + MB_2ND_FBIC_OFFSET + ((pri_cpuid & 0x1e) << 23));

	/*
	 * I would have liked to do all of the mapping in ka60conf(),
	 * but I need to get the console SLU registers mapped before I
	 * can use the console, and in order to map the console, I need
	 * to set up the FBIC, and so it needs to be mapped.  The probe
	 * of the M-Bus is done in ka60conf() in ka60.c
	 */
	/*
	 * Look for a module in each slot and map it.
	 */
	pteptr = FFIOmap;
	fbp = (struct fbic_regs *)ffiom;
	mbp = mbus_nodes;
	for (i = MB_SLOT0_BASE; i <= MB_SLOT7_BASE; i = i + MB_SLOT_SIZE) {
	    nxaccess((long)(i + MB_FBIC_OFFSET), pteptr++, 512);
	    if((*cpup->badaddr)((caddr_t)&fbp->f_modtype, 4)) {
		fbp++;
		continue;
	    }
	    switch(fbp->f_modtype & FMOD_CLASS) {

		case FMOD_QBUS:
		    ka60_mbfillin(mbp, fbp, (i + MB_FBIC_OFFSET), 0);
		    /*
		     * Map the  CQBIC
		     */
		    nxaccess((long *)cpup->nexaddr(0,1), FFQREGmap, 512);
		    /*
		     * Map the Q-bus Map registers
		     */
		    nxaccess((long *)cpup->nexaddr(0,3), CVQBMmap, (512 * 68));
		    /*
		     * The FQAM from Design Associates has the PROC field
		     * in the cpuid register set to 3, where the FTAM has 
		     * them set to 0.  This is how to determine if the module
		     * is an FTAM or a FQAM.
		     * The FQAM CSR is initialized by power-on diagnostics.
		     * We don't initialize it.
		     */
		    if (fbp->f_cpuid & FCPU_PROCA) {
			/* FQAM */
			nxaccess((long *)FF_FQAM_CSR, FFFQAMCSRmap, 512);
		    }
		    /*
		     * Initiaize the FBIC
		     */
		    fbp->f_busaddr = 0;
		    fbp->f_busctl = 0;
		    fbp->f_buscsr = FBCSR_CLEAR;
		    fbp->f_fbicsr |= FFCSR_IRQE_X | FFCSR_IRQD_X |
			FFCSR_PRI0EN | FFCSR_NORMAL;
		    fbp->f_range = 0x80080008 | FRANGE_ENA;
		    break;

		case FMOD_GRAPHICS:
		    ka60_mbfillin(mbp, fbp, (i + MB_FBIC_OFFSET), 0);
		    nxaccess((long *)i, FGMEMmap[0], NMEMSIZEFG);
		    /*
		     * Initialize the FBIC
		     */
		    fbp->f_busaddr = 0;
		    fbp->f_busctl = 0;
		    fbp->f_buscsr = FBCSR_CLEAR;
		    fbp->f_fbicsr |= FFCSR_IRQE_X | FFCSR_IRQD_X |
			FFCSR_PRI0EN | FFCSR_NORMAL;
		    fbp->f_ipdvint = (u_long)(FIPD_DEVUNIT | (0x100 + (MBUS_SLOT(i) * 0x20)));
		    break;

		case FMOD_IO:
		    ka60_mbfillin(mbp, fbp, (i + MB_FBIC_OFFSET), 0);
		    /*
		     * Map the Console DZ
		     */
		    nxaccess((long *)(i + FF_DZ_OFF), FFCONSmap, (512 * 3));
		    /*
		     * Map the SSC
		     */
		    nxaccess((long *)cpup->nexaddr(0,4), CVQSSCmap, (512 * 3));
		    /*
		     * Map the SII Registers
		     */
		    nxaccess((long *)(i + FF_SI_OFF), CVQMSImap, 512);
		    /*
		     * Map the SII buffer
		     */
		    nxaccess((long *)(i + FF_SIB_OFF), CVQMSIRBmap, (512 * 256));
		    /*
		     * Map the NI Registers
		     * The nxaccess calls for the NI registers, station
		     * address ROM, and RAM buffer must be in the following
		     * order so that the ni_regs structure can be used.
		     */
		    nxaccess((long *)(i + FF_NI_OFF), CVQNImap, 512);
		    nxaccess((long *)(i + FF_SAROM_OFF), (int)CVQNImap + sizeof(struct pte), 512);
		    nxaccess((long *)(i + FF_NIBUF_OFF), (int)CVQNImap + (2 * sizeof(struct pte)), (512 * 256));
		    /*
		     * Map the IOCSR Register.
		     * The NI station address is read through the IOCSR
		     * register.  The IOCSR register is already mapped as
		     * part of CVQNImap (FF_SAROM_OFF), but for maintainability
		     * we will map the IOCSR register again as FFIOCSR.  One
		     * pte seems a small price to pay for maintainability
		     */
		    nxaccess((long *)(i + FF_IOCSR_OFF), FFIOCSRmap, 512);
		    /*
		     * Initiaize the FBIC
		     */
		    fbp->f_busaddr = 0;
		    fbp->f_busctl = 0;
		    fbp->f_buscsr = FBCSR_CLEAR;
		    fbp->f_fbicsr |= FFCSR_IRQE_X | FFCSR_IRQD_X |
			FFCSR_PRI0EN | FFCSR_NORMAL;
		    fbp->f_range = 0x80140000 | FRANGE_ENA;
		    fbp->f_ipdvint = (u_long)(FIPD_DEVUNIT | (0x100 + (MBUS_SLOT(i) * 0x20)));
		    /*
		     * Map the CTSIA.  We map all of the CTSIA, but not all of
		     * the driver segments, and we use only the external driver
		     * state.  This must be done AFTER initializing the FBIC
		     * f_range register.
		     */

		    nxaccess((long *)cvqssc->ssc_ctsi_addr,CTSImap,512);

		    if ((ctsi->ct_std_out.ctcb_dvatr & 0x7) == FF_LEGSS) {
		    	nxaccess((long *)ctsi->ct_std_out.ctcb_extdvrstate_p,
				FGCTSIXSmap,512);
		    }
		    else if ((ctsi->ct_fb_out.ctcb_dvatr & 0x7) == FF_LEGSS) {
		    	nxaccess((long *)ctsi->ct_fb_out.ctcb_extdvrstate_p,
				FGCTSIXSmap,512);
		    }
		    /*
		     * Enable clocks, Console halts, and MRUN only on the
		     * I/O module the Console program is using.  Write only
		     * the high order byte.
		     */
		    iocsraddr = (char *)((int)ffiocsr + 3);
		    *iocsraddr = 0xe0;
		    break;

		case FMOD_CPU:
		    ka60_mbfillin(mbp, fbp, (i + MB_FBIC_OFFSET), 0);
		    /*******************************************
		     * This is where to map the cache tags if we
		     * ever need to disable cache..
		     *******************************************/
		    ka60_initcpufbic(fbp);
		    /*
		     * Look for another CPU on this module
		     */
	    	    nxaccess((long)(i + MB_2ND_FBIC_OFFSET), pteptr++, 512);
		    fbp++;
	    	    if((*cpup->badaddr)((caddr_t)fbp, 4)) {
			continue;
		    }
		    mbp++;
		    ka60_mbfillin(mbp, fbp, (i + MB_2ND_FBIC_OFFSET), 0);
		    /*******************************************
		     * This is where to map the cache tags if we
		     * ever need to disable cache..
		     *******************************************/
		    ka60_initcpufbic(fbp);
		    break;

		case FMOD_MEM:
		    memctlr++;
		    ka60_mbfillin(mbp, fbp, (i + MB_FBIC_OFFSET), memctlr);
		    /*
		     * Initiaize the FBIC
		     */
		    if (fbp->f_modtype & FMOD_INTERFACE != FMOD_FIRESTARTER) {
			fbp->f_busaddr = 0;
			fbp->f_busctl = 0;
		    }
		    fbp->f_buscsr = FBCSR_CLEAR;
		    break;

		default:
		    break;
	    }
	    fbp++; mbp++;
	}

	/*
	 * Determine console device and inititialize it.
	 * If L3CON is set use the diagnostic console,
	 * if not use the console_id from NVR or the CFGTST register MULTU bit,
	 * if console_id says unknown we guess based on the
	 * VIDOPT and CURTEST bits in the configuration register.
	 * Order of precedence is:
	 *	Diagnostic console on SLU port 3 (BCC08 cable).
	 *	Terminal on SLU port 0 (if MULTU bit set).
	 *	Keyboard on SLU port 0 (color video, MULTU bit clear).
	 *	Keyboard on SLU port 0 (base monochrome video, MULTU bit clear).
	 * The BCC08 cable enables halt on break.
	 *
	 * The VAXstar SLU is always present and must always be
	 * initialized, even if the console is the bitmap or color
	 * display. So, we inititialize it here.
	 */
	fcaddr = ffcons;
	fcaddr->fccsr = FC_CLR;
	for(i=0; i<100000; i++)
		if((fcaddr->fccsr&FC_CLR) == 0)
			break;
	fcaddr->fccsr = FC_MSE;
	i = (FC_RE | FC_B9600 | BITS8);
	if(ff_diagcons)
		i |= 0x3;
	fcaddr->fclpr = i;
	cdevsw[0] = cdevsw[FCMAJOR];
	/*
	 * point v_consputc and v_consgetc at fcputc and fcgetc for now,
	 * and if the LEGSS graphics exists, call fgcons_init which will
	 * overwrite v_consputc and v_consgetc.
	 */
	v_consputc = fcputc;
	v_consgetc = fcgetc;
	if (!ff_diagcons)
		fgcons_init();
	return(1);
}

/*
 * VAXstar SLU (dzq like) console putc routine.
 * Calls fc_putc() to output each character.
 */

fcputc(c)
	register int c;
{
	fc_putc(c);
	if (c == '\n')
		fc_putc('\r');
}

/*
 * This routine outputs one character to the console
 * on line 0 or 3 depending on the state of L3CON
 * in the VAXstar configuration and test register.
 * Characters must be printed without disturbing
 * output in progress on other lines!
 * This routines works with the SLU in interrupt or
 * non-interrupt mode of operation.
 * Characters are output as follows:
 *	spl6, remember if console line active.
 *	set console line tcr bit.
 *	wait for TRDY on console line (save status if wrong line).
 *	start output of character.
 *	wait for output complete.
 *	if any lines were active, set their tcr bits,
 *	otherwise clear the xmit ready interrupt.
 *
 */

/*
 * Physical address of Firefox ffcons.
 * Used to access SLU and interrupt cntlr registers
 * when the machine is on physical mode (during crash dump).
 */

fc_putc(c)
	register int c;
{
	register struct fc_regs *fcaddr;
	int	s, tcr, ln, tln, timo, tim2;
	int	physmode;

	if( (mfpr(MAPEN) & 1) == 0 ) {
		physmode = 1;
		fcaddr = (struct fc_regs *)(MBUS_BASEADDR(mb_slot) + FF_DZ_OFF);
	} else {
		physmode = 0;
		fcaddr = (struct fc_regs *)ffcons;
	}
	if(physmode == 0)
		s = spl6();
	tln = (ff_diagcons) ? 0x3 : 0x0;
	tcr = (fcaddr->fctcr & (1<<tln));
	fcaddr->fctcr |= (1<<tln);
	while (1) {
		timo = 1000000;
		while ((fcaddr->fccsr&FC_TRDY) == 0)
			if(--timo == 0)
				break;
		if(timo == 0) {
			/*
			 * Bit failed to clear.  Rather than hanging forever
			 * give up after logging an error.
			 */
			if (fcbail1 < 10)
				mprintf("fc_putc: FC_TRDY failed to set\n");
			fcbail1++;
			break;
		}
		ln = (fcaddr->fccsr>>8) & 3;
		if (ln != tln) {
			tcr |= (1 << ln);
			fcaddr->fctcr &= ~(1 << ln);
			continue;
		}
		fcaddr->fctbuf = c&0xff;
		while (1) {
			tim2 = 1000000;
			while ((fcaddr->fccsr&FC_TRDY) == 0) 
				if(--tim2 == 0)
					break;
			if(tim2 == 0) {
				/*
			 	 * Bit failed to clear.  Rather than hanging 
			 	 * forever give up after logging an error.
			 	 */
				if (fcbail2 < 10)
					mprintf("fc_putc: FC_TRDY 2 failed to set\n");
				fcbail2++;
				break;
			}
			ln = (fcaddr->fccsr>>8) & 3;
			if (ln != tln) {
				tcr |= (1 << ln);
				fcaddr->fctcr &= ~(1 << ln);
				continue;
			}
			break;
		}
		break;
	}
	fcaddr->fctcr &= ~(1<<tln);
	if (tcr == 0) {
		;
	}
	else
		fcaddr->fctcr |= tcr;
	if(physmode == 0)
		splx(s);
}

/*
 * This routine operates on the following assumptions:
 * 1. putc must have happened first, so SLU already inited.
 * 2. getc will happed before slu reveive interrupt enabled so
 *    don't need to worry about int_req or int_msk registers.
 */
fcgetc()
{
	register struct fc_regs *fcaddr = (struct fc_regs *)ffcons;
	register int c, line, timo;

	/*
	 * Line number we expect input from.
	 */
	if(ff_diagcons)
		line = 3;
	else
		line = 0;
	while (1) {
		timo = 1000000;
		while ((fcaddr->fccsr&FC_RDONE) == 0) 
			if(--timo == 0)
				break;
		if(timo == 0) {
			/*
		 	 * Bit failed to clear.  Rather than hanging forever
		 	 * give up after logging an error.
		 	 */
			if (fcbail3 < 10)
				mprintf("fcgetc: FC_RDONE failed to set\n");
			fcbail3++;
			break;
		}
		c = fcaddr->fcrbuf;
		if(((c >> 8) & 3) != line)	/* wrong line mumber */
			continue;
		if(c&(FC_DO|FC_FE|FC_PE))	/* error */
			continue;
		break;
	}
	return(c & 0xff);
}

/*
 * Modem Control Routines
 */
/*
 *
 * Function:
 *
 *	fc_cd_drop
 *
 * Functional description:
 *
 * 	Determine if carrier has dropped.  If so call fc_tty_drop to terminate
 * 	the connection.
 *
 * Arguements:
 *
 *	register struct tty *tp  -  terminal pointer ( for terminal attributes )
 *
 * Return value:
 *
 *	none
 *
 */
fc_cd_drop(tp)
register struct tty *tp;
{
	register struct fc_regs *fcaddr = (struct fc_regs *)ffcons;
	register int unit = minor(tp->t_dev);

	if ((tp->t_state&TS_CARR_ON) &&
		((fcaddr->fcmsr&FC_RCD) == 0)) {
#		ifdef DEBUG
        	if (fcdebug)
	       	    cprintf("fc_cd:  no CD, tp=%x\n", tp);
#		endif DEBUG
		fc_tty_drop(tp);
		return;
	}
	fcmodem |= MODEM_CD;
#	ifdef DEBUG
        if (fcdebug)
	    cprintf("fc_cd:  CD is up, tp=%x\n", tp);
#	endif DEBUG
}
/*
 *
 * Function:
 *
 *	fc_dsr_check
 *
 * Functional description:
 *
 * 	DSR must be asserted for a connection to be established.  Here we either
 * 	start or terminate a connection on the basis of DSR.
 *
 * Arguements:
 *
 *	register struct tty *tp  -  terminal pointer ( for terminal attributes )
 *
 * Return value:
 *
 *	none
 *
 */
fc_dsr_check(tp)
register struct tty *tp;
{
	int unit = minor(tp->t_dev);
	register struct fc_regs *fcaddr = (struct fc_regs *)ffcons;
	if (fcmodem&MODEM_DSR_START) {
#		ifdef DEBUG
        	if (fcdebug)
	       	    cprintf("fc_dsr_check0:  tp=%x\n", tp);
#		endif DEBUG
		fcmodem &= ~MODEM_DSR_START;
	   	/* 
	    	 * If fcdsr is set look for DSR|CTS|CD, otherwise look 
	    	 * for CD|CTS only.
	    	 */
		if (fcdsr) {
			if ((fcaddr->fcmsr&FC_XMIT) == FC_XMIT)
				fc_start_tty(tp);
		}
		else {
			if ((fcaddr->fcmsr&FC_NODSR) == FC_NODSR)
				fc_start_tty(tp);
		}
		return;
	}
	if ((tp->t_state&TS_CARR_ON)==0)  
		fc_tty_drop(tp);
}

/*
 *
 * Function:
 *
 *	fc_cd_down
 *
 * Functional description:
 *
 *	Determine whether or not carrier has been down for > 2 sec.
 *
 * Arguements:
 *
 *	register struct tty *tp  -  terminal pointer ( for terminal attributes )
 *
 * Return value:
 *
 *	1 - if carrier was down for > 2 sec.
 *	0 - if carrier down <= 2 sec.
 *
 */
fc_cd_down(tp)
struct tty *tp;
{
	int msecs;
	int unit = minor(tp->t_dev);

	msecs = 1000000 * (time.tv_sec - fctimestamp.tv_sec) + 
		(time.tv_usec - fctimestamp.tv_usec);
	if (msecs > 2000000){
#		ifdef DEBUG
		if (fcdebug)
			cprintf("fc_cd_down: msecs > 20000000\n");
#		endif DEBUG
		return(1);
	}
	else{
#		ifdef DEBUG
		if (fcdebug)
			cprintf("fc_cd_down: msecs < 20000000\n");
#		endif DEBUG
		return(0);
	}
}
/*
 *
 * Function:
 *
 *	fc_tty_drop
 *
 * Functional description:
 *
 *	Terminate a connection.
 *
 * Arguements:
 *
 *	register struct tty *tp  -  terminal pointer ( for terminal attributes )
 *
 * Return value:
 *
 *	none
 *
 */
fc_tty_drop(tp)
struct tty *tp;
{
	register struct fc_regs *fcaddr = (struct fc_regs *)ffcons;
	register int unit;
  	if (tp->t_flags&NOHANG) 
		return;
	unit = minor(tp->t_dev);
#	ifdef DEBUG
	if (fcdebug)
		cprintf("fc_tty_drop: unit=%d\n",unit);
#	endif DEBUG
	/* 
	 * Notify any processes waiting to open this line.  Useful in the
	 * case of a false start.
	 */
	fcmodem = MODEM_BADCALL;
	tp->t_state &= ~(TS_CARR_ON|TS_TTSTOP|TS_BUSY|TS_ISUSP);
	wakeup((caddr_t)&tp->t_rawq);
  	gsignal(tp->t_pgrp, SIGHUP);
	gsignal(tp->t_pgrp, SIGCONT);
	fcaddr->fcdtr &= ~(FC_RDTR|FC_RRTS);
}
/*
 *
 * Function:
 *
 *	fc_start_tty
 *
 * Functional description:
 *
 *	Establish a connection.
 *
 * Arguements:
 *
 *	register struct tty *tp  -  terminal pointer ( for terminal attributes )
 *
 * Return value:
 *
 *	none
 *
 */
fc_start_tty(tp)
	register struct tty *tp;
{
	int unit = minor(tp->t_dev);
	tp->t_state &= ~(TS_ONDELAY);
	tp->t_state |= TS_CARR_ON;
#	ifdef DEBUG
        if (fcdebug)
	       cprintf("fc_start_tty:  tp=%x\n", tp);
#	endif DEBUG
	if (fcmodem&MODEM_DSR)
		untimeout(fc_dsr_check, tp);
	fcmodem |= MODEM_CD|MODEM_CTS|MODEM_DSR;
	fctimestamp.tv_sec = fctimestamp.tv_usec = 0;
	wakeup((caddr_t)&tp->t_rawq);
}

fcbaudrate(speed)
register int speed;
{
    if (fc_valid_speeds & (1 << speed))
	return (1);
    else
	return (0);
}
#endif
