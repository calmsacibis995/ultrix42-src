#ifndef lint
static char *sccsid = "@(#)ss.c	4.2	(ULTRIX)	8/13/90";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1985-89 by			*
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

#include "ss.h"
#if NSS > 0 || defined(BINARY)
/*
 *  VAXstar serial line unit driver
 *
 *  Modification History:
 *
 *  6-Jul-1990  - Kuo-Hsiung Hsieh
 *      Fixed data corrupted problem due to setting break condition
 *      on a transmission line.  On DC type of chip, a specific delay
 *      period has to be imposed on the transmission line if the next
 *      thing to transmit is a break condition.  Data could be corrupted
 *      even though TRDY bit may say it is ready to send the next
 * 	character.
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
 * 23-May-89 - darrell
 *	Removed the v_ from the fields of the cpusw structure, and globally
 *	defined cpup -- as part of the new cpusw.
 *
 * 09-May-89 - gmm (v3.1 merge)
 *	Set modem control on for TMII (MicroVAX 3100)
 *
 *  8-May-89	Giles Atkinson
 *    Expand mapped part of system scratch RAM to 3 pages to allow peek
 *    at version numbers.
 *    Changed major device numbers to use cons_maj.h
 *
 * 05-Mar-89 - Fred Canter
 *	Fix stray interrupts thru SCB vector 0x1fc caused by polling
 *	for RDONE with interrupts enabled in the receive interrupt
 *	routine.
 *
 * 25-Jan-89 - Randall Brown
 *
 *	Changed cd_drop to look at LNOHANG.  Changed close routine to look
 *	at HUPCL.
 *
 * 28-Sep-88 - Fred Canter
 *	Clean up comments.
 *
 * 28-Sep-88 - Randall Brown
 *
 *	Fixed a bug in ssxint so that the transmitter interrupt will be
 *	acknowledged when the terminal is in the stop state.
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
 * 19-Aug-88 - Randall Brown
 *
 *	Added modem support for the PVAX.  The PVAX only gives DSR to
 * 	cpu, therefore CD and CTS are ignored.
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
 * 07-Jun-88 - Fred Canter
 *	Back out last change (hardware changed their mind).
 *
 * 06-Jun-88 - Fred Canter
 *	Use different address for SCSI/SCSI controller.
 *
 * 19-May-88 - Fred Canter
 *	Additional virtual address mapping for SCSI support
 *	and using extended I/O mode for CVAXstar/PVAX.
 *
 * 16-May-88 - Tim Burke
 * 	Call param routine for setting of local mode word because it can 
 * 	affect bit size and parity.
 *
 * 15-Feb-88 - Fred Canter
 *	Changes for VAX420 (CVAXstar/PVAX) support.
 *
 * 29-Jan-88 - Tim Burke
 *	Changed most softCAR[unit&LINEMASK] references to use the CLOCAL
 *	bit of the control flags to determine if the line is set to be a
 *	modem line or a direct connect.  The setting of softCAR[] remains
 *	to allow one to set default settings for device open.
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
 *  8-Sep-87 - Ricky Palmer (rsp)
 *
 *	Defined LINEMASK and added to code where appropriate in particular
 *	the DEVIOCGET code.
 *
 *  3-Aug-87 - rafiey (Ali Rafieymehr)
 *      Mapped system scratch RAM which is used by the VAXstar color
 *      driver.
 *
 * 14-May-87 - fred (Fred Canter)
 *	Never allow open of /dev/tty00 (conflicts with /dev/console).
 *	Don't allow open of /dev/tty01 on VAXstation 2000
 *	(conflicts with mouse).
 *	Bug fix to allow xcons to work (loop kernel messages to
 *	login window), was not calling graphics driver ioctl routine.
 *
 * 20-Mar-87 - Tim Burke
 *
 *	Added full TERMIO functionality to terminal subsystem.
 *
 * 11-Feb-87 - rafiey (Ali Rafieymehr)
 *	Changed the driver to call read/write routines of the color driver
 *	for reads/writes.
 *
 * 28-Jan-87 - Tim Burke
 *
 *	Added the capability to ignore the "DSR" modem signal.  This is being
 *	done to allow modems that do not follow DEC Standard 52 to still 
 *	function as they would have prior to the addition of DECSTD52 code
 *	into the drivers.  If the driver is setup to ignore "DSR" then it will
 *	not be following DECSTD52.  To follow DECSTD52 set ssdsr to "1", to
 *	ignore "DSR" set ssdsr to be "0";
 *
 *  07-Jan-87 - rafiey (Ali Rafieymehr)
 *	Corrected the value of the color option in ubareg.h, therefore
 *	uncommented the two cases in sscons_init which were previously
 *	commented out.
 *
 *  16-Dec-86 - fred (Fred Canter)
 *	Changes for removal of DEV_MODEV_OFF devioctl flag.
 *
 *  11-Dec-86 - fred (Fred Canter)
 *	Bug fix: devio soft error count not incremented correctly
 *		 on any line other than zero.
 *	Bug fix: Sign extension of intr char during framing error
 *		 caused false silo overflow error indication.
 *
 *   2-Dec-86 - fred (Fred Canter)
 *	Fix rlogin hanging if console is diagnostic terminal
 *	on the printer port (BCC08 cable).
 *	Change minor device to 3 in ssselect().
 *
 *   4-Nov-86 - fred (Fred Canter)
 *	Disable support for silo alarm mode on VAXstar.
 *	It causes input silo overrun errors when using tip.
 *
 *   4-Nov-86 - tim  (Tim Burke)
 *	Clear TS_TSTOP on close to prevent line hanging if
 *	in stop state.
 *
 *  18-Sept-86 - tim  (Tim Burke)
 *	Lower ipl level upon receipt of a badcall on modem line.  This is
 *	done to insure that future interrupts get serviced.
 *
 *  30-Aug-86 -- fred (Fred Canter)
 *	Merged in Tim Burke's final dec standard 52 modem support changes.
 *	Fixed console putc to work in physical mode (for crash dump).
 *	Change for dummy sgcons_init and smcons_init routines.
 *
 *  26-Aug-86 -- rsp (Ricky Palmer)
 *	Cleaned up devioctl code to (1) zero out devget structure
 *	upon entry and (2) use strlen instead of fixed storage
 *	for bcopy's.
 *
 *  24-Aug-86  -- fred (Fred Canter) and rafiey (Ali Rafieymehr)
 *	Support for smscreen (console message window).
 *	Allow uses to change softCAR only on line 2 (modem control line).
 *	Finish DEVIOCTL support.
 *	Remove ssreset() and other general cleanup.
 *	Hardwire critical line parameters into the driver stty in
 *	users' .profile can't make the line inoperative.
 *
 *  14-Aug-86  -- fred (Fred Canter)
 *	general cleanup, Tim's dec standard 52 stuff,
 *	no silo mode if graphics device present,
 *	complete redesign of ssputc().
 *
 *   5-Aug-86  -- fred (Fred Canter)
 *	Major changes, bug fixes, and passing characters to the
 *	bitmap graphics driver (sm.c).
 *
 *  24-Jul-86  -- tim  (Tim Burke)
 *	Added full DEC Standard 52 support.  Most changes occured in the 
 *	ssscan , ssopen, and ssclose routines to track modem signal status.
 *      Added the following modem control routines:
 *      ss_cd_drop, ss_dsr_check, ss_cd_down, ss_tty_drop, ss_start_tty.
 *
 *   2-Jul-86  -- fred (Fred Canter)
 *	Removed DZ32 code, changed from 8 to 4 lines per unit,
 *	moved ssconsinit(), ssputc(), & ssgetc() from cons.c to ss.c,
 *	other improvements.
 *
 *  18-Jun-86  -- fred (Fred Canter)
 *	Created this file (derived from dz.c).
 */

#include "../data/ss_data.c"

int ssdebug = 0;

/*
* Driver information for auto-configuration stuff.
*/
int	ssprobe(), ssattach(), ssrint();
int	ss_cd_drop(), ss_dsr_check(), ss_cd_down(), ss_tty_drop(); /* Modem */
u_short	ssstd[] = { 0 };
struct	uba_driver ssdriver =
{ ssprobe, 0, ssattach, 0, ssstd, "ss", ssinfo };


int	ssstart(), ssxint(), ssdma(), ssbaudrate();
int	ttrstrt();
int	ssact;
void 	sssetbreak();

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

int	sssilos;			/* mask of SLU's with silo in use */
int	sstimerintvl;			/* time interval for sstimer */
int	sshighrate = 100;		/* silo on if sschars > sshighrate */
int	sslowrate = 75;			/* silo off if ssrate < sslowrate */

/*
* The SLU doesn't interrupt on carrier transitions, so
* we have to use a timer to watch it.
*/
char	ss_timer;		/* timer started? */

char	ss_speeds[] =
{ 0,020,021,022,023,024,0,025,026,027,030,032,034,036,037,0 };

/* minumum delay value for setting a break condition.  If we set
 * a break condition without delaying this minimum interval, we
 * might corrupt character which is still in the shift register.
 * The delay values are calculated based on the following equation;
 * 12 (bits/char) * 100 (hz) / baudrate + 2 (safety factor).
 */
u_char	ss_delay[] =
{ 0,26,18,13,11,10,0,6,4,3,3,3,2,2,2,0 };

short	ss_valid_speeds = 0x7fbf; /* 0,1,1,1, 1,1,1,1, 1,0,1,1, 1,1,1,1 */

u_char sscan_previous;		/* Used to detect modem transitions */

extern	struct	nexus	nexus[];
struct	tty		sm_tty;
extern struct cpusw *cpup;	/* pointer to cpusw entry */

#ifdef DEBUG
#define PRINT_SIGNALS() { cprintf("Modem signals: "); \
	if (ssaddr->ssmsr&SS_RDSR) cprintf(" DSR "); \
	if (ssaddr->ssmsr&SS_RCTS) cprintf(" CTS "); \
	if (ssaddr->ssmsr&SS_RCD) cprintf(" CD "); \
	cprintf("\n"); } \
/*	cprintf("ssaddr->ssmsr %x : %x\n", &(ssaddr->ssmsr), ssaddr->ssmsr);*/
#endif DEBUG

extern int cpu_sub_subtype;
ssprobe(reg)
	caddr_t reg;
{
	register struct nb_regs *ssaddr = (struct nb_regs *)nexus;

	/*
	 * ONLY on a VAXstation 2000 or MicroVAX 2000
	 * or CVAXstar.
	 */
	if ((cpu != VAXSTAR) && (cpu != C_VAXSTAR)) {
	    return(0);
	}
	if ( (cpu != C_VAXSTAR) || (cpu_sub_subtype == SB_TMII) ) { /* if not a PVAX then set modem_ctl on */
	    ss_modem_ctl = 1;
	}
#ifdef lint
	ssrint(0); ssxint((struct tty *)0);
#endif
	ssaddr->nb_int_msk |= SINT_ST;
	ssaddr->sstcr = 0x8;		/*  enable line 3 */
	DELAY(100000);
	ssaddr->sstcr = 0;
	ssaddr->nb_int_msk &= ~SINT_ST;
	ssaddr->nb_int_reqclr = SINT_ST;
	if (cvec && cvec != 0x200)
		cvec -= 4;
	return (1);	/* 1 not sizeof anything, just says probe succeeded */
}

ssattach(ui)
	register struct uba_device *ui;
{
	register struct pdma *pdp = &sspdma[ui->ui_unit*4];
	register struct tty *tp = &ss_tty[ui->ui_unit*4];
	register int cntr;
	register struct nb_regs *ssaddr = (struct nb_regs *)nexus;
	extern ssscan();

	for (cntr = 0; cntr < 4; cntr++) {
		/* dzdevice looks wrong, but see vaxuba/pdma.h for reason */
		pdp->p_addr = (struct dzdevice *)&ssaddr->sscsr;
		pdp->p_arg = (int)tp;
		pdp->p_fcn = ssxint;
		pdp++, tp++;
	}
	sssoftCAR[ui->ui_unit] = ui->ui_flags;
	ssdefaultCAR[ui->ui_unit] = ui->ui_flags;
	ssmodem = 0; 
	if (ss_timer == 0) {
		ss_timer++;
		timeout(ssscan, (caddr_t)0, hz);
		sstimerintvl = FASTTIMER;
	}
}

/*ARGSUSED*/
ssopen(dev, flag)
	dev_t dev;
{
	register struct nb_regs *ssaddr = (struct nb_regs *)nexus;
	register struct tty *tp;
	register int unit;
	register int maj;
	int inuse;  /*hold state of inuse bit while blocked waiting for carr*/

	maj = major(dev);
	/*
	 * If a diagnostic console is attached to SLU line 3,
	 * don't allow open of the printer port (also line 3).
	 * This could cause lpr to write to the console.
	 */
	if((vs_cfgtst&VS_L3CON) && (maj == SS_MAJOR)) {
		if((minor(dev)&LINEMASK) == 3)
			return (ENXIO);
	}
	unit = minor(dev);
	if((vs_cfgtst&VS_L3CON) &&
	   (maj == CONSOLE_MAJOR) &&
	   ((unit&LINEMASK) == 0))
		unit |= 3;	/* diag console on SLU line 3 */
	if (unit >= ss_cnt || sspdma[unit].p_addr == 0)
		return (ENXIO);
	/*
 	 * Never allow open of device 45/0 (/dev/tty00)
 	 * because it conflicts with 0/0 (/dev/console).
 	 */
 	if ((unit == 0) && (maj == SS_MAJOR))
 		return (ENXIO);
 	/*
 	 * If the console is a graphics device (VAXstation 2000),
 	 * don't allow open of device 45/1 (/dev/tty01)
 	 * because it conflicts with 0/1 (graphics pointer device).
 	 */
 	if (vs_gdopen && (unit == 1) && (maj == SS_MAJOR))
 		return (ENXIO);
 	/*
	/*
	 * Call the graphics device open routine
	 * if there is one and the open if for the fancy tube.
	 */
	if ((vs_gdopen && (unit <= 1)) || (vs_gdopen && (unit == 2) &&
	    (maj == CONSOLE_MAJOR)))
		return((*vs_gdopen)(dev, flag));
	tp = &ss_tty[unit];
	if (tp->t_state&TS_XCLUDE && u.u_uid != 0){
		return (EBUSY);
	}
	while (tp->t_state&TS_CLOSING) { /* let DTR stay down for awhile */
		sleep((caddr_t)&tp->t_rawq, TTIPRI);
	}
	tp->t_addr = (caddr_t)&sspdma[unit];
	tp->t_oproc = ssstart;
	tp->t_baudrate = ssbaudrate;

	tty_def_open(tp, dev, flag, (sssoftCAR[unit>>2]&(1<<(unit&LINEMASK))));

	if ((tp->t_state & TS_ISOPEN) == 0) {
		if (unit == MODEM_UNIT)
			ssmodem = MODEM_DSR_START;
		if((maj == CONSOLE_MAJOR) && ((minor(dev)&3) == 0)) {
		    tp->t_cflag &= ~CBAUD;
		    tp->t_cflag = B9600;
		    tp->t_cflag_ext &= ~CBAUD;
		    tp->t_cflag_ext = B9600;
		    tp->t_flags = ANYP|ECHO|CRMOD;
		    tp->t_iflag |= ICRNL; /* Map CRMOD */
		    tp->t_oflag |= ONLCR; /* Map CRMOD */
		}
	} 

	ssparam(unit);		/* enables interrupts */

	(void) spl5();

	/*
	 * No modem control provided for lines with softCAR set.
	 * Modem control provided only for line 2.
	 */
#	ifdef DEBUG
	if (ssdebug)
		cprintf("ssopen: UNIT = %x\n",unit);
#	endif DEBUG
	if ((unit != MODEM_UNIT) || (tp->t_cflag & CLOCAL)) {
		/*
		 * This is a local connection - ignore carrier 
		 * receive enable interrupts enabled above via ssparam() 
		 */
		tp->t_state |= TS_CARR_ON;		/* ssscan sets */
		if (unit == MODEM_UNIT)
			ssaddr->ssdtr |= (SS_RDTR|SS_RRTS);
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
	/* receive enable interrupts enabled above via ssparam() */
	ssaddr->ssdtr |= (SS_RDTR|SS_RRTS);

	/*
	 * After DSR first comes up we must wait for the other signals
	 * before commencing transmission.
         */
#ifdef DEBUG
	if (ssdebug) {
		cprintf("open flag : %x\n", flag);
		if (flag & (O_NDELAY|O_NONBLOCK)) 
			cprintf("flag & (O_NDELAY|O_NONBLOCK)\n");
	}
#endif DEBUG
	if ((flag & (O_NDELAY|O_NONBLOCK)) == 0) {
		/*
		 * Delay before examining other signals if DSR is being followed
		 * otherwise proceed directly to ss_dsr_check to look for 
		 * carrier detect and clear to send.
		 */
#ifdef DEBUG
		if (ssdebug) {
			cprintf("ssopen: ");
			PRINT_SIGNALS();
		}
#endif DEBUG
		if (ssdsr) {
			 if ((ssaddr->ssmsr)&SS_RDSR) {
				ssmodem |= (MODEM_DSR_START|MODEM_DSR);
				tp->t_dev = dev; /* need it for timeouts */
				if (!ss_modem_ctl) {
				    /*
				    * Assume carrier will come up in less
				    * than 1 sec. If not DSR will drop
				    * and the line will close
				    */
				    timeout(ss_dsr_check, tp, hz);
				} else {
				    /* 
		 		    * Give Carrier and CTS 30 sec. to
				    * come up.  Prevent any transmission
				    * in the first 500ms.
		 		    */
				    timeout(ss_dsr_check, tp, hz*30);
				    timeout(ss_dsr_check, tp, hz/2);
				}
			}
		}
		/* 
	 	 * Ignoring DSR so immediately check for CD & CTS.
		 */
		else {
				ssmodem |= (MODEM_DSR_START|MODEM_DSR);
				ss_dsr_check(tp);
		}
	}
#	ifdef DEBUG
	if (ssdebug)
		cprintf("ssopen:  line=%d, state=%x, tp=%x\n", unit,
			tp->t_state, tp);
#	endif DEBUG
	if (flag & (O_NDELAY|O_NONBLOCK))
		tp->t_state |= TS_ONDELAY;
	else
		while ((tp->t_state & TS_CARR_ON) == 0) {
			tp->t_state |= TS_WOPEN;
			inuse = tp->t_state&TS_INUSE;
#ifdef DEBUG
			if (ssdebug) {
				cprintf("ss_open: going to sleep\n");
			}
#endif DEBUG
			sleep((caddr_t)&tp->t_rawq, TTIPRI);
			/*
			 * See if we were awoken by a false call to the modem
			 * line by a non-modem.
 			 */
			if (ssmodem&MODEM_BADCALL){
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
ssclose(dev, flag)
	dev_t dev;
{
	register struct tty *tp;
	register int unit;
	register struct nb_regs *ssaddr = (struct nb_regs *)nexus;
	register int s;
	int ss;
	extern int wakeup();

	unit = minor(dev);
	if((vs_cfgtst&VS_L3CON) &&
	   (major(dev) == CONSOLE_MAJOR) &&
	   ((unit&LINEMASK) == 0))
		unit |= 3;	/* diag console on SLU line 3 */
	/*
	 * Call the craphics device close routine
	 * if ther is one and the close is for it.
	 */
	if ((vs_gdclose && (unit <= 1)) || (vs_gdclose && (unit == 2) &&
	    (major(dev) == CONSOLE_MAJOR))){
		(*vs_gdclose)(dev, flag);
		return;
	}
	ss = unit >> 2;
#	ifdef DEBUG
	if (ssdebug)
		cprintf("ssclose: unit=%x, ss=%x\n",unit,ss);
#	endif DEBUG
	tp = &ss_tty[unit];
	/*
	 * Do line discipline specific close functions then return here
	 * in the old line disc for final closing.
	 */
	if (tp->t_line)
		(*linesw[tp->t_line].l_close)(tp);
	/*
	 * ssbrk is write-only and sends a BREAK (SPACE condition) until
         * the break control bit is cleared. Here we are clearing any
 	 * breaks for this line on close.
	 */
	ssaddr->ssbrk = (ss_brk[ss] &= ~(1 << (unit&LINEMASK)));
	if ((tp->t_cflag&HUPCL) || (tp->t_state&TS_WOPEN) || (tp->t_state&TS_ISOPEN)==0) {
		tp->t_state &= ~TS_CARR_ON;   /* prevents recv intr. timeouts */
		if (unit == MODEM_UNIT) {
			/*
		 	 * Drop appropriate signals to terminate the connection.
		 	 */
			ssaddr->ssdtr &= ~(SS_RDTR|SS_RRTS);
			if ((tp->t_cflag & CLOCAL) == 0) {
				s = spl5();
				/*drop DTR for at least a sec. if modem line*/
#				ifdef DEBUG
				if (ssdebug)
					cprintf("ssclose: DTR drop, state =%x\n"
						,tp->t_state);
#				endif DEBUG
				tp->t_state |= TS_CLOSING;
				/*
			 	 * Wait at most 5 sec for DSR to go off.  
			 	 * Also hold DTR down for a period.
			 	 */
				if (ssdsr && (ssaddr->ssmsr & SS_RDSR)) {
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
	sssoftCAR[ss] &= ~(1<<(unit&LINEMASK));
	sssoftCAR[ss] |= (1<<(unit&LINEMASK)) & ssdefaultCAR[ss];
	if (unit == MODEM_UNIT)
		ssmodem = 0;
	ttyclose(tp);
	tty_def_close(tp);
}

/*
 * ssread() shared with graphics device drivers (sm & sg).
 */

ssread(dev, uio)
	dev_t dev;
	struct uio *uio;
{
	register struct tty *tp;
	register int unit;

	unit = minor(dev);
	if((vs_cfgtst&VS_L3CON) &&
	   (major(dev) == CONSOLE_MAJOR) &&
	   ((unit&LINEMASK) == 0))
		unit |= 3;	/* diag console on SLU line 3 */
	if((unit == 1) && vs_gdread)
	    return((*vs_gdread)(dev, uio));  /* color option */
	if (vs_gdopen && (unit == 2) && (major(dev) == CONSOLE_MAJOR))
	    tp = &sm_tty;
	else
	    tp = &ss_tty[unit];
	return ((*linesw[tp->t_line].l_read)(tp, uio));
}

/*
 * sswrite() shared with graphics device drivers (sm & sg).
 */

sswrite(dev, uio)
	dev_t dev;
	struct uio *uio;
{
	register struct tty *tp;
	register int unit;

	unit = minor(dev);
	if((vs_cfgtst&VS_L3CON) &&
	   (major(dev) == CONSOLE_MAJOR) &&
	   ((unit&LINEMASK) == 0))
		unit |= 3;	/* diag console on SLU line 3 */
	if((unit == 1) && vs_gdwrite)	
	    return((*vs_gdwrite)(dev, uio)); /* color option */
	/*
	 * Don't allow writes to the mouse,
	 * just fake the I/O and return.
	 */
	if (vs_gdopen && (unit == 1)) {
		uio->uio_offset = uio->uio_resid;
		uio->uio_resid = 0;
		return(0);
	}
	if (vs_gdopen && (unit == 2) && (major(dev) == CONSOLE_MAJOR))
	    tp = &sm_tty;
	else
	    tp = &ss_tty[unit];
	return ((*linesw[tp->t_line].l_write)(tp, uio));
}

ssselect(dev, rw)
dev_t dev;
{
	register int unit = minor(dev);
	
	if((vs_cfgtst&VS_L3CON) &&
	   (major(dev) == CONSOLE_MAJOR) &&
	   ((unit&3) == 0))
		dev |= 3;
	if(((unit == 1) || (unit == 2)) && vs_gdselect &&
	   (major(dev) == CONSOLE_MAJOR))
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
ssrint(ss)
	int ss;
{
	register struct tty *tp;
	register int c;
	register struct nb_regs *ssaddr = (struct nb_regs *)nexus;
	register struct tty *tp0;
	register int unit;
	register int flg;
	int overrun = 0;
	struct ss_softc *sc;
	struct mouse_report *new_rep;
	u_short data;

	if ((ssact & (1<<ss)) == 0)
		return;
	unit = ss * 4;
	tp0 = &ss_tty[unit];
	new_rep = &current_rep;			/* mouse report pointer */
	/*
	 * 3/5/89 -- Fred Canter
	 * Disable interrupts to prevent stray vectors
	 * thru 0x1fc caused by polling with interrupts on.
	 */
	ssaddr->nb_int_msk &= ~SINT_SR;
	while (ssaddr->sscsr&SS_RDONE) {	/* character present */
		ssaddr->nb_int_reqclr = SINT_SR;
		c = ssaddr->ssrbuf;
		sschars[ss]++;
		unit = (c>>8)&LINEMASK;
		tp = tp0 + unit;
		if (tp >= &ss_tty[ss_cnt])
			continue;
		sc = &ss_softc[ss];
		/*
		 * If console is a graphics device,
		 * pass keyboard input characters to
		 * its device driver's receive interrupt routine.
		 * Save up complete mouse report and pass it.
		 */
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


		/* SS_FE is interpreted as a break */
		if (c & SS_FE) {
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
#				ifdef SSDEBUG
				if (ssdebug)
					mprintf("ssrint: BREAK RECEIVED\n");
#				endif SSDEBUG
				if ((tp->t_lflag_ext & PRAW) && 
					(tp->t_line != TERMIODISC))
					c = 0;
				else {
				    ttyflush(tp, FREAD|FWRITE);
#ifdef SSDEBUG
				    if (ssdebug)
					mprintf("sending signal to tp->t_pgrp = %d\n", tp->t_pgrp);
#endif SSDEBUG
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
		else if (c & SS_PE){
			/* 
			 * If input parity checking is not enabled, clear out
			 * parity error in this character.
			 */
#			ifdef SSDEBUG
			if (ssdebug > 1)
				mprintf("ssrint: Parity Error\n");
#			endif SSDEBUG
			if ((flg & INPCK) == 0)
				c &= ~SS_PE;
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
		if (c&SS_DO) {
			if(overrun == 0) {
				printf("ss%d: input silo overflow\n", ss);
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
	ssaddr->nb_int_msk |= SINT_SR;
}

/*ARGSUSED*/
ssioctl(dev, cmd, data, flag)
	dev_t dev;
	register int cmd;
	caddr_t data;
	int flag;
{
	register int unit;
	register struct tty *tp;
	register int ss;
	register struct nb_regs *ssaddr = (struct nb_regs *)nexus;
	register int s;
	struct uba_device *ui;
	struct ss_softc *sc;
	struct devget *devget;
	int error;

	unit = minor(dev);
	if((vs_cfgtst&VS_L3CON) &&
	   (major(dev) == CONSOLE_MAJOR) &&
	   ((unit&LINEMASK) == 0))
		unit |= 3;	/* diag console on SLU line 3 */
	/*
	 * If there is a graphics device and the ioctl call
	 * is for it, pass the call to the graphics driver.
	 */
	if (vs_gdioctl && (unit <= 1)) {
		error = (*vs_gdioctl)(dev, cmd, data, flag);
 		return(error);
 	}
 	if (vs_gdioctl && (unit == 2) && (major(dev) == CONSOLE_MAJOR)) {
		error = (*vs_gdioctl)(dev, cmd, data, flag);
		return(error);
	}
	tp = &ss_tty[unit];
	ss = unit >> 2;
	ui = ssinfo[ss];
	sc = &ss_softc[ui->ui_unit];
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
 				ssparam(unit);
 				break;
 		}
		return (error);
	}
	switch (cmd) {

	case TIOCSBRK:
		timeout(sssetbreak, tp, ss_delay[tp->t_cflag & CBAUD]);
		TTY_SLEEP(tp, (caddr_t)&tp->t_dev, TTOPRI);
		s = spltty();
		ssaddr->ssbrk = (ss_brk[ss] |= 1 << (unit&LINEMASK));
		splx(s);
		break;

	case TIOCCBRK:
		s = spltty();
		ssaddr->ssbrk = (ss_brk[ss] &= ~(1 << (unit&LINEMASK)));
		splx(s);
		break;

	case TIOCSDTR:
		(void) ssmctl(dev, SS_DTR|SS_RTS, DMBIS);
		break;

	case TIOCCDTR:
		(void) ssmctl(dev, SS_DTR|SS_RTS, DMBIC);
		break;

	case TIOCMSET:
		(void) ssmctl(dev, dmtoss(*(int *)data), DMSET);
		break;

	case TIOCMBIS:
		(void) ssmctl(dev, dmtoss(*(int *)data), DMBIS);
		break;

	case TIOCMBIC:
		(void) ssmctl(dev, dmtoss(*(int *)data), DMBIC);
		break;

	case TIOCMGET:
		*(int *)data = sstodm(ssmctl(dev, 0, DMGET));
		break;

	case TIOCNMODEM:  /* ignore modem status */
		/* 
		 * By setting the software representation of modem signals
		 * to "on" we fake the system into thinking that this is an
		 * established modem connection.
		 */
		s = spl5();
		sssoftCAR[ss] |= (1<<(unit&LINEMASK));  
		if (*(int *)data) /* make mode permanent */
			ssdefaultCAR[ss] |= (1<<(unit&LINEMASK));  
		tp->t_state |= TS_CARR_ON;
		tp->t_cflag |= CLOCAL;		/* Map to termio */
		splx(s);
		break;

	case TIOCMODEM:  /* look at modem status - sleep if no carrier */
		if(unit != MODEM_UNIT)
			break;		/* ONLY line 2 has modem control */
		s = spl5();
		sssoftCAR[ss] &= ~(1<<(unit&LINEMASK));  
		if (*(int *)data) /* make mode permanent */
			ssdefaultCAR[ss] &= ~(1<<(unit&LINEMASK));  
		/*
		 * See if all signals necessary for modem connection are present
		 * 
	    	 * If ssdsr is set look for DSR|CTS|CD, otherwise look 
	    	 * for CD|CTS only.
	    	 */
		if ((ssdsr && ((ssaddr->ssmsr&SS_XMIT) == SS_XMIT)) ||
		   ((ssdsr==0) && ((ssaddr->ssmsr&SS_NODSR) == SS_NODSR)) ||
		   ((!ss_modem_ctl) && (ssaddr->ssmsr&SS_RDSR))) {
			tp->t_state &= ~(TS_ONDELAY);
			tp->t_state |= TS_CARR_ON;
			ssmodem = MODEM_CTS|MODEM_CD|MODEM_DSR;
		}
		else {
			tp->t_state &= ~(TS_CARR_ON);
			ssmodem &= ~(MODEM_CTS|MODEM_CD|MODEM_DSR);
		}
		tp->t_cflag &= ~CLOCAL;		/* Map to termio */
		splx(s);
		break;

	case TIOCWONLINE:
		if(unit != MODEM_UNIT)
			break;		/* ONLY line 2 has modem control */
		s = spl5();
		/*
		 * See if all signals necessary for modem connection are present
		 * 
	    	 * If ssdsr is set look for DSR|CTS|CD, otherwise look 
	    	 * for CD|CTS only.
	    	 */
		if ((ssdsr && ((ssaddr->ssmsr&SS_XMIT) == SS_XMIT)) ||
		   ((ssdsr==0) && ((ssaddr->ssmsr&SS_NODSR) == SS_NODSR)) ||
		   ((!ss_modem_ctl) && (ssaddr->ssmsr&SS_RDSR))) {
			tp->t_state |= TS_CARR_ON;
			tp->t_state &= ~(TS_ONDELAY);
			ssmodem = MODEM_CTS|MODEM_CD|MODEM_DSR;
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
		bcopy(DEV_VS_SLU,devget->interface,
		      strlen(DEV_VS_SLU));		/* interface	*/
		bcopy(DEV_UNKNOWN,devget->device,
		      strlen(DEV_UNKNOWN));		/* terminal	*/
		devget->adpt_num = 0;			/* NO adapter	*/
		devget->nexus_num = 0;			/* fake nexus 0	*/
		devget->bus_num = 0;			/* NO bus	*/
		devget->ctlr_num = ss;			/* cntlr number */
		devget->slave_num = unit&LINEMASK;	/* line number 	*/
		bcopy(ui->ui_driver->ud_dname,
		      devget->dev_name,
		      strlen(ui->ui_driver->ud_dname)); /* Ultrix "ss"	*/
		devget->unit_num = unit&LINEMASK;	/* ss line?	*/
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

dmtoss(bits)
	register int bits;
{
	register int b;

	b = (bits >>1) & 0370;
	if (bits & SML_ST) b |= SS_ST;
	if (bits & SML_RTS) b |= SS_RTS;
	if (bits & SML_DTR) b |= SS_DTR;
	if (bits & SML_LE) b |= SS_LE;
	return(b);
}

sstodm(bits)
	register int bits;
{
	register int b;

	b = (bits << 1) & 0360;
	if (bits & SS_DSR) b |= SML_DSR;
	if (bits & SS_DTR) b |= SML_DTR;
	if (bits & SS_ST) b |= SML_ST;
	if (bits & SS_RTS) b |= SML_RTS;
	return(b);
}

ssparam(unit)
	register int unit;
{
	register struct tty *tp;
	register struct nb_regs *ssaddr = (struct nb_regs *)nexus;
	register int lpr;

	tp = &ss_tty[unit];
	if (sssilos & (1 << (unit >> 2)))
		ssaddr->sscsr = SS_MSE | SS_SAE;
	else
		ssaddr->sscsr = SS_MSE;
/*
 *	Reversing the order of the following two lines fixes the
 *	problem where the console device locks up if you type a
 *	character during autoconf and you must halt/continue to
 *	unlock the console. Interrupts were being enabled on the SLU
 *	before the ssact flag was set, so the driver would just return
 *	and not process the waiting character (caused by you typing).
 *	This locked up the cosnole SLU (interrupt posted but never really
 *	servcied). Halting the system caused the console firmware to unlock
 *	the SLU because it needs to use it.
 *	Should ssparam() be called as spl5??????
 */
	ssact |= (1<<(unit>>2));
	ssaddr->nb_int_msk |= (SINT_ST|SINT_SR);
	if ((((tp->t_cflag&CBAUD)==B0) && (u.u_procp->p_progenv != A_POSIX)) || 
	    (((tp->t_cflag_ext & CBAUD)==B0) && 
		(u.u_procp->p_progenv == A_POSIX))) { 
		(void) ssmctl(unit, SS_OFF, DMSET);	/* hang up line */
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
	if ((unit == 3) && (vs_cfgtst&VS_L3CON))
		ssaddr->sslpr = (SS_RE | SS_B9600 | BITS8 | 3);
	else if (unit == 0) {
		if (vs_gdopen)
			ssaddr->sslpr = (SS_RE | SS_B4800 | BITS8);
		else
			ssaddr->sslpr = (SS_RE | SS_B9600 | BITS8);
	} else if (vs_gdopen && (unit == 1))
		ssaddr->sslpr = (SS_RE | SS_B4800 | OPAR | PENABLE | BITS8 | 1);
	/*
	 * Set parameters in accordance with user specification.
	 */
	else {
		lpr = (ss_speeds[tp->t_cflag&CBAUD]<<8) | (unit & LINEMASK);
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
 			lpr &= ~SS_RE;	/* This was set from speeds */
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
		if (ssdebug)
			mprintf("ssparam: tp = %x, lpr = %x\n",tp,lpr);
#		endif DEBUG
		ssaddr->sslpr = lpr;
	}
}

ssxint(tp)
	register struct tty *tp;
{
	register struct nb_regs *ssaddr = (struct nb_regs *)nexus;
	register struct pdma *dp;
	register unit;

	dp = (struct pdma *)tp->t_addr;
	unit = minor(tp->t_dev) & 3;
	if ((vs_cfgtst&VS_L3CON) && (unit == 0) &&
	    (major(tp->t_dev) == CONSOLE_MAJOR)) {
		unit = 3;
	}

	/*
	 * Don't know if the following is true for the
	 * VAXstar SLU, but it stays in just to be safe.
	 * Fred Canter -- 6/28/86
	 */
	/* it appears that the ss will ocassionally give spurious
	   transmit ready interupts even when not enabled.  If the
	   line was never opened, the following is necessary */

	if (dp == NULL) {
		tp->t_addr = (caddr_t) &sspdma[unit];
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
		ssstart(tp);
	/* The BUSY flag will not be set in two cases:		  */
	/*   1: if there are no more chars in the outq  OR	  */
	/*   2. there are chars in the outq but tty is in stopped */
	/*	state.						  */
	if ((tp->t_state&TS_BUSY)==0) {
		ssaddr->sstcr &= ~(1<<unit);
	}
}

ssstart(tp)
	register struct tty *tp;
{
	register struct pdma *dp;
	register struct nb_regs *ssaddr = (struct nb_regs *)nexus;
	register int cc;
	int s, unit;

	dp = (struct pdma *)tp->t_addr;
	s = spl5();
	/*
	 * Do not do anything if currently delaying, or active.  Also only
	 * transmit when CTS is up.
	 */
	unit = minor(tp->t_dev) & 3;
	if ((tp->t_state & (TS_TIMEOUT|TS_BUSY|TS_TTSTOP)) ||
		(unit == MODEM_UNIT) && ((tp->t_cflag & CLOCAL) == 0)
		&& ((tp->t_state&TS_CARR_ON) && (ssmodem&MODEM_CTS)==0))
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
	if ((vs_cfgtst&VS_L3CON) && (unit == 0) && 
	    (major(tp->t_dev) == CONSOLE_MAJOR))
		unit = 3;
	ssaddr->sstcr |= (1<<unit);
out:
	splx(s);
}

/*
 * Stop output on a line.
 */

/*ARGSUSED*/
ssstop(tp, flag)
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
	s = spl5();
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
 *	CAUTION: the SML_* definitions in ssreg.h must match the
 *	TIOCM_* definitions in ioctl.h.
 *
 *	The line number in the dev argument to this routine will be
 *	wrong (0 s/b 3) if VS_L3CON is set in the configuration and test
 *	register, i.e., a diagnostic console terminal is attached to
 *	the printer port. This fact is ignored because this routine only
 *	acts on line 2 anyway.
 *
 */

ssmctl(dev, bits, how)
	dev_t dev;
	int bits, how;
{
	register struct nb_regs *ssaddr = (struct nb_regs *)nexus;
	register int unit, mbits;
	int b, s;

	unit = minor(dev);
	if(unit != MODEM_UNIT)
		return(0);	/* only line 2 has modem control */
	b = 0x4;
	s = spl5();
	mbits = (ssaddr->ssdtr & SS_RDTR) ? SS_DTR : 0;
	mbits |= (ssaddr->ssdtr & SS_RRTS) ? SS_RTS : 0;
	mbits |= (ssaddr->ssmsr & SS_RCD) ? SS_CD : 0;
	mbits |= (ssaddr->ssmsr & SS_RDSR) ? SS_DSR : 0;
	mbits |= (ssaddr->ssmsr & SS_RCTS) ? SS_CTS : 0;
	mbits |= (ssaddr->sstbuf & b) ? SS_RI : 0;
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
	if (mbits & SS_DTR)
		ssaddr->ssdtr |= (b|SS_RRTS);
	else
		ssaddr->ssdtr &= ~(b|SS_RRTS);
	(void) splx(s);
	return(mbits);
}

/*
 * Allows silo alarm mode, if set.
 * Enabling silo alarm mode will most likely
 * cause silo overrun errors. The system can't
 * seem to keep up?????????
 */

ss_allow_silos = 0;

int sstransitions, ssfasttimers;		/*DEBUG*/

#ifdef DEBUG
int sscan_ctr = 1;
#endif DEBUG

ssscan()
{
	register i;
	register struct nb_regs *ssaddr = (struct nb_regs *)nexus;
	register struct tty *tp;
	int oldsssilos = sssilos;
	int sstimer();
	register u_char sscan_modem;

#ifdef DEBUG
	if (ssdebug) {
		if (sscan_ctr++ == 45) {
			cprintf("ssscan: ");
			PRINT_SIGNALS();
			sscan_ctr = 1;
		}
	}
#endif DEBUG
	for (i = 0; i < ss_cnt; i++) {
		if (sspdma[i].p_addr == 0)
			continue;
		tp = &ss_tty[i];
		sscan_modem = 0;
		/* Modem Control only on line 2 */
		if ((i == MODEM_UNIT) && ((tp->t_cflag & CLOCAL) == 0)) {
			/*
			 * Drop DTR immediately if DSR has gone away.
	                 * If really an active close then do not
	                 *    send signals.
			 */
			if (!(ssaddr->ssmsr&SS_RDSR)) {
	                        if (tp->t_state&TS_CLOSING) {
	                              untimeout(wakeup, (caddr_t) &tp->t_dev);
	                              wakeup((caddr_t) &tp->t_dev);
	                        }
				if (tp->t_state&TS_CARR_ON) {
					/*
					 * Only drop if DECSTD52 is followed.
					 */
					if (ssdsr)
						ss_tty_drop(tp);
				}
			}
			else {		/* DSR has come up */
				/*
				 * If DSR comes up for the first time we allow
				 * 30 seconds for a live connection.
				 */
				if (ssdsr && ((ssmodem&MODEM_DSR)==0)) {
				    ssmodem |= (MODEM_DSR_START|MODEM_DSR);
				    /* 
				     * we should not look for CTS|CD for
				     * about 500 ms.  
				     */
				    if (!ss_modem_ctl) {
				        /*
				        * Assume carrier will come up in less
				        * than 1 sec. If not DSR will drop
				        * and the line will close
				        */
					timeout(ss_dsr_check, tp, hz);
				    } else {
					timeout(ss_dsr_check, tp, hz*30);
					timeout(ss_dsr_check, tp, hz/2);
				    }
				}
			}
			/*
			 * Look for modem transitions in an already
			 * established connection.
			 *
			 * Ignore CD and CTS for PVAX. These signals
			 * don't exist on the PVAX.
			 */
			if (ss_modem_ctl) {
			    if (tp->t_state & TS_CARR_ON) {
				if (ssaddr->ssmsr&SS_RCD) {
					/*
					 * CD has come up again.
					 * Stop timeout from occurring if set.
					 * If interval is more than 2 secs then
					 *  drop DTR.
					 */
					if ((ssmodem&MODEM_CD)==0) { 
						untimeout(ss_cd_drop, tp);
						if (ss_cd_down(tp)){
							/* drop connection */
							ss_tty_drop(tp);
						}
						ssmodem |= MODEM_CD;
					}
				} else {
					/* 
					 * Carrier must be down for greater than 2 secs
					 * before closing down the line.
					 */
					if ( ssmodem & MODEM_CD) {
					    /* only start timer once */
					    ssmodem &= ~MODEM_CD;
					    /* 
					     * Record present time so that if carrier
					     * comes up after 2 secs , the line will drop.
					     */
					    sstimestamp = time;
					    timeout(ss_cd_drop, tp, hz*2);
					}
				}
				/* CTS flow control check */
		
				if (!(ssaddr->ssmsr&SS_RCTS)) {
					/* 
					 * Only allow transmission when CTS is set.
					 */
					tp->t_state |= TS_TTSTOP;
					ssmodem &= ~MODEM_CTS;
#					ifdef DEBUG
					if (ssdebug)
					   cprintf("ssscan: CTS stop, tp=%x,line=%d\n", tp,i);
#					endif DEBUG
					ssstop(tp, 0);
				} else if ((ssmodem&MODEM_CTS)==0) { 
					    /* 
					     * Restart transmission upon return of CTS.
					     */
					    tp->t_state &= ~TS_TTSTOP;
					    ssmodem |= MODEM_CTS;
#					    ifdef DEBUG
					    if (ssdebug)
					       cprintf("ssscan: CTS start, tp=%x,line=%d\n", tp,i);
#					    endif DEBUG
					    ssstart(tp);
				}
			    }
			
			    /* 
			     * See if a modem transition has occured.  If we are waiting
			     * for this signal cause action to be taken via ss_tty_start.
			     */
			    if ((sscan_modem=ssaddr->ssmsr&SS_XMIT) != sscan_previous){
				/*
			 	* If 500 ms timer has not expired then dont
			 	* check anything yet.
			 	* Check to see if DSR|CTS|CD are asserted.
			 	* If so we have a live connection.
			 	*/
#				ifdef DEBUG
				if (ssdebug)
					cprintf("ssscan: MODEM Transition,sscan_modem=%x, sscan_prev=%x\n",sscan_modem,sscan_previous);
#				endif DEBUG
	   			/* 
	    	 		 * If ssdsr is set look for DSR|CTS|CD,else look
	    	 		 * for CD|CTS only.
	    	 		 */
				if (ssdsr) {
				    if (((ssaddr->ssmsr&SS_XMIT)==SS_XMIT) 
				        && ((ssmodem&MODEM_DSR_START)==0)
				        && ((tp->t_state&TS_CARR_ON)==0)) {
#					    ifdef DEBUG
					    if (ssdebug)
						cprintf("ssscan:SS_XMIT call ss_start,line=%d\n",i);
#				            endif DEBUG
					    ss_start_tty(tp);
				    }
				}
				/* 
				 * Ignore DSR.	
				 */
				else
					if ((ssaddr->ssmsr&SS_NODSR)==SS_NODSR)
						ss_start_tty(tp);
						
			    }
			    sscan_previous = sscan_modem; /* save for next iteration */
			}
		}
	}

	for (i = 0; i < nNSS; i++) {
		ave(ssrate[i], sschars[i], 8);
		/*
		 * Allow silo mode only if no graphics device.
		 * Silo mode blows away with mouse tracking.
		 */
		if (ss_allow_silos && (vs_gdopen == 0)) {
		    if (sschars[i] > sshighrate && ((sssilos&(1 << i)) == 0)) {
			ssaddr->sscsr = SS_MSE | SS_SAE;
			ssaddr->nb_int_msk |= (SINT_ST|SINT_SR);
			sssilos |= (1 << i);
			sstransitions++;		/*DEBUG*/
		    } else if ((sssilos&(1 << i)) && (ssrate[i] < sslowrate)) {
			ssaddr->sscsr = SS_MSE;
			ssaddr->nb_int_msk |= (SINT_ST|SINT_SR);
			sssilos &= ~(1 << i);
		    }
		}
		sschars[i] = 0;
	}
	if (sssilos && !oldsssilos)
		timeout(sstimer, (caddr_t)0, sstimerintvl);
	timeout(ssscan, (caddr_t)0, hz);
}

sstimer()
{
	register int ss;
	register int s;

	if (sssilos == 0)
		return;
	s = spl5();
	ssfasttimers++;		/*DEBUG*/
	for (ss = 0; ss < nNSS; ss++)
		if (sssilos & (1 << ss))
			ssrint(ss);
	splx(s);
	timeout(sstimer, (caddr_t) 0, sstimerintvl);
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
int	ssputc();
int	ssgetc();
u_long  scr_ram_addr;           /* system scratch RAM phys. address used by */
                                /* color driver.                            */

extern	(*v_consputc)();
extern	(*v_consgetc)();
extern	int	sgcons_init();
extern	int	smcons_init();

/*
 * This variable tells ssputc, smputc, and sgputc thhether or
 * not it is safe actually print characters. Those routines
 * just return if vs_safe2print is zero. This is nexcessary
 * because the putc routines must not attempt to access VAXstar
 * I/O space until it has been mapped (bu sscons_inions_init).
 */
/* TODO: not fully implemented, finish it or remove it! */
int	vs_safe2print = 0;

int	cvs_exmode_on = 1;	/* written to STC_MODE register */

sscons_init()
{
	register struct nb_regs *ssaddr;
	register char	*nxv, *nxp;	/* nexus virtual/phys addr pointers */
	int	i;
	int	constype;

	/*
	 * ONLY on a VAXstation 2000 or MicroVAX 2000
	 * or CVAXstar.
	 */
	if ((cpu != VAXSTAR) && (cpu != C_VAXSTAR))
		return(0);
        
	/*
	 * Map the nexus.
	 */
	nxv = (char *)nexus;
	nxp = (char *)cpup->nexaddr(0,0);
	nxaccess (nxp, Nexmap[0], cpup->pc_nexsize);
	/*
	 * See if there is anything there.
	 */
	/* NOTE: VAXstars/CVAXstars don't trap on bad addresses */
	if ((*cpup->badaddr)((caddr_t) nxv, 4))
		return(-1);
	sbi_there |= 1<<0;
	/*
	 * May as well map the rest of I/O space
	 * while we are at it.
	 */
	nxp = (char *)cpup->umaddr(0,0);
	nxaccess (nxp, QMEMmap[0], QMEMSIZEVS);
	nxp = (char *)NMEMVAXSTAR;
	if (cpu == C_VAXSTAR) {
		nxaccess (nxp, NMEMmap[0], NMEMSIZECVS);
	}
	else
		nxaccess (nxp, NMEMmap[0], NMEMSIZEVS);
	/*
	 * TODO:
	 *	Following is place holder for color driver
	 *	Why map device if it is not present?
	 */
	nxp = (char *)SGMEMVAXSTAR;
	nxaccess (nxp, SGMEMmap[0], SGMEMSIZEVS);
	nxp = (char *)SHMEMVAXSTAR;
	nxaccess (nxp, SHMEMmap[0], SHMEMSIZEVS);
        ssaddr = (struct nb_regs *)nexus;
	/*
	 * Map CVAXstar 2nd level cache data storage
	 * Map CVAXstar SCSI registers
	 */
	if (cpu == C_VAXSTAR) {
		nxp = (char *)SZMEMCVAXSTAR;
		nxaccess (nxp, SZMEMmap[0], SZMEMSIZECVS);

		nxp = (char *)CVSCACHEADDR;
		nxaccess (nxp, CVSCACHEmap[0], CVSCACHESIZE);

		nxp = (char *)CVSEDDBADDR;
		nxaccess (nxp, CVSEDDBmap[0], CVSEDDBSIZE);
		/* NOTE: extended mode must always be enabled! */
		((struct nb1_regs *)qmem)->nb_stc_mode = (char)cvs_exmode_on;
	}
        /*
         * Calculate the physical address of system scratch RAM and
         * map the first three pages (used by the color driver).
         */
        scr_ram_addr = (ssaddr->nb_scr[3] & 0x3FC) << 22;
        scr_ram_addr |= ((ssaddr->nb_scr[2] & 0x3FC) << 14);
        scr_ram_addr |= ((ssaddr->nb_scr[1] & 0x3FC) << 6);
        scr_ram_addr |= ((ssaddr->nb_scr[0] & 0x3FC) >> 2);
        nxp = (char *)scr_ram_addr;
        nxaccess (nxp, SGSYSmap[0], 3);
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
	ssaddr->sscsr = SS_CLR;
	for(i=0; i<100000; i++)
		if((ssaddr->sscsr&SS_CLR) == 0)
			break;
	ssaddr->nb_int_msk &= ~(SINT_ST|SINT_SR);
	ssaddr->nb_int_reqclr = (SINT_ST|SINT_SR);
	ssaddr->sscsr = SS_MSE;
	i = (SS_RE | SS_B9600 | BITS8);
	if(vs_cfgtst&VS_L3CON)
		i |= 0x3;	/* diag. console on line 3 */
	ssaddr->sslpr = i;
	cdevsw[0] = cdevsw[SS_MAJOR];
	constype = (ssaddr->nb_console_id >> 2) & 0xff;
	if ((vs_cfgtst&VS_L3CON) || (vs_cfgtst&VS_MULTU)) {
		v_consputc = ssputc;
		v_consgetc = ssgetc;
	} else {
		switch(constype) {

		case VS_CID_COLOR:
			sgcons_init();
			break;
		case VS_CID_BITMAP:
			smcons_init();
			break;
		case VS_CID_UNKNOWN:		/* FALLTHROUGH */
		default:
			if(vs_cfgtst&VS_VIDOPT)
				sgcons_init();
			else if(vs_cfgtst&VS_CURTEST)
				smcons_init();
			else {
				v_consputc = ssputc;
				v_consgetc = ssgetc;
			}
			break;
		}
	}
	vs_safe2print = 1;
	return(1);
}

/*
 * VAXstar SLU (dzq like) console putc routine.
 * Calls ss_putc() to output each character.
 */

ssputc(c)
	register int c;
{
	ss_putc(c);
	if (c == '\n')
		ss_putc('\r');
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
 *	spl5, remember if console line active.
 *	set console line tcr bit.
 *	wait for TRDY on console line (save status if wrong line).
 *	start output of character.
 *	wait for output complete.
 *	if any lines were active, set their tcr bits,
 *	otherwise clear the xmit ready interrupt.
 *
 */

/*
 * Physical address of VAXstar "fake" nexus.
 * Used to access SLU and interrupt cntlr registers
 * when the machine is on physical mode (during crash dump).
 */
#define	VS_PHYSNEXUS	0x20080000

ss_putc(c)
	register int c;
{
	register struct nb_regs *ssaddr;
	int	s, tcr, ln, tln, timo;
	int	physmode;

	if( (mfpr(MAPEN) & 1) == 0 ) {
		physmode = 1;
		ssaddr = (struct nb_regs *)VS_PHYSNEXUS;
	} else {
		/*
		 * I/O space not mapped yet, can't print.
		 */
		if (vs_safe2print == 0)
			return;
		physmode = 0;
		ssaddr = (struct nb_regs *)nexus;
	}
	if(physmode == 0)
		s = spl5();
	tln = (vs_cfgtst&VS_L3CON) ? 0x3 : 0x0;
	tcr = (ssaddr->sstcr & (1<<tln));
	ssaddr->sstcr |= (1<<tln);
	while (1) {
		timo = 1000000;
		while ((ssaddr->sscsr&SS_TRDY) == 0)
			if(--timo == 0)
				break;
		if(timo == 0)
			break;
		ln = (ssaddr->sscsr>>8) & 3;
		if (ln != tln) {
			tcr |= (1 << ln);
			ssaddr->sstcr &= ~(1 << ln);
			continue;
		}
		ssaddr->sstbuf = c&0xff;
		while (1) {
			while ((ssaddr->sscsr&SS_TRDY) == 0) ;
			ln = (ssaddr->sscsr>>8) & 3;
			if (ln != tln) {
				tcr |= (1 << ln);
				ssaddr->sstcr &= ~(1 << ln);
				continue;
			}
			break;
		}
		break;
	}
	ssaddr->sstcr &= ~(1<<tln);
	if (tcr == 0)
		ssaddr->nb_int_reqclr = SINT_ST;
	else
		ssaddr->sstcr |= tcr;
	if(physmode == 0)
		splx(s);
}

/*
 * This routine operates on the following assumptions:
 * 1. putc must have happened first, so SLU already inited.
 * 2. getc will happed before slu reveive interrupt enabled so
 *    don't need to worry about int_req or int_msk registers.
 */
ssgetc()
{
	register struct nb_regs *ssaddr = (struct nb_regs *)nexus;
	register int c, line;

	/*
	 * Line number we expect input from.
	 */
	if(vs_cfgtst&VS_L3CON)
		line = 3;
	else
		line = 0;
	while (1) {
		while ((ssaddr->sscsr&SS_RDONE) == 0) ;
		c = ssaddr->ssrbuf;
		if(((c >> 8) & 3) != line)	/* wrong line mumber */
			continue;
		if(c&(SS_DO|SS_FE|SS_PE))	/* error */
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
 *	ss_cd_drop
 *
 * Functional description:
 *
 * 	Determine if carrier has dropped.  If so call ss_tty_drop to terminate
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
ss_cd_drop(tp)
register struct tty *tp;
{
	register struct nb_regs *ssaddr = (struct nb_regs *)nexus;

	if ((tp->t_state&TS_CARR_ON) &&
		((ssaddr->ssmsr&SS_RCD) == 0)) {
#		ifdef DEBUG
        	if (ssdebug)
	       	    cprintf("ss_cd:  no CD, tp=%x\n", tp);
#		endif DEBUG
		ss_tty_drop(tp);
		return;
	}
	ssmodem |= MODEM_CD;
#	ifdef DEBUG
        if (ssdebug)
	    cprintf("ss_cd:  CD is up, tp=%x\n", tp);
#	endif DEBUG
}
/*
 *
 * Function:
 *
 *	ss_dsr_check
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
ss_dsr_check(tp)
register struct tty *tp;
{
	register struct nb_regs *ssaddr = (struct nb_regs *)nexus;
#	ifdef DEBUG
       	if (ssdebug) {
       	    cprintf("ss_dsr_check0:  tp=%x\n", tp);
	    PRINT_SIGNALS();
	}
#	endif DEBUG
	if (ssmodem&MODEM_DSR_START) {
	    ssmodem &= ~MODEM_DSR_START;
	    /* 
	     * If ssdsr is set look for DSR|CTS|CD, otherwise look 
	     * for CD|CTS only.
	     *
	     * If cpu is PVAX then don't bother looking at CD or CTS,
	     * just let the connection continue.
	     */
	    if (ssdsr) {
		if ((!ss_modem_ctl) || ((ssaddr->ssmsr&SS_XMIT) == SS_XMIT))
		    ss_start_tty(tp);
	    } else {
		if ((ssaddr->ssmsr&SS_NODSR) == SS_NODSR)
		    ss_start_tty(tp);
	    }
	    return;
	}
	if ((tp->t_state&TS_CARR_ON)==0)  
		ss_tty_drop(tp);
}

/*
 *
 * Function:
 *
 *	ss_cd_down
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
ss_cd_down(tp)
struct tty *tp;
{
	int msecs;

	msecs = 1000000 * (time.tv_sec - sstimestamp.tv_sec) + 
		(time.tv_usec - sstimestamp.tv_usec);
	if (msecs > 2000000){
#		ifdef DEBUG
		if (ssdebug)
			cprintf("ss_cd_down: msecs > 20000000\n");
#		endif DEBUG
		return(1);
	}
	else{
#		ifdef DEBUG
		if (ssdebug)
			cprintf("ss_cd_down: msecs < 20000000\n");
#		endif DEBUG
		return(0);
	}
}
/*
 *
 * Function:
 *
 *	ss_tty_drop
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
ss_tty_drop(tp)
struct tty *tp;
{
	register struct nb_regs *ssaddr = (struct nb_regs *)nexus;
	if (tp->t_flags&NOHANG)
		return;
#	ifdef DEBUG
	if (ssdebug)
		cprintf("ss_tty_drop: unit=%d\n",minor(tp->t_dev));
#	endif DEBUG
	/* 
	 * Notify any processes waiting to open this line.  Useful in the
	 * case of a false start.
	 */
	ssmodem = MODEM_BADCALL;
	tp->t_state &= ~(TS_CARR_ON|TS_TTSTOP|TS_BUSY|TS_ISUSP);
	wakeup((caddr_t)&tp->t_rawq);
  	gsignal(tp->t_pgrp, SIGHUP);
	gsignal(tp->t_pgrp, SIGCONT);
	ssaddr->ssdtr &= ~(SS_RDTR|SS_RRTS);
}
/*
 *
 * Function:
 *
 *	ss_start_tty
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
ss_start_tty(tp)
	register struct tty *tp;
{
	tp->t_state &= ~(TS_ONDELAY);
	tp->t_state |= TS_CARR_ON;
#	ifdef DEBUG
        if (ssdebug)
	       cprintf("ss_start_tty:  tp=%x\n", tp);
#	endif DEBUG
	if (ssmodem&MODEM_DSR)
		untimeout(ss_dsr_check, tp);
	ssmodem |= MODEM_CD|MODEM_CTS|MODEM_DSR;
	sstimestamp.tv_sec = sstimestamp.tv_usec = 0;
	wakeup((caddr_t)&tp->t_rawq);
}

ssbaudrate(speed)
register int speed;
{
    if (ss_valid_speeds & (1 << speed))
	return (1);
    else
	return (0);
}

void sssetbreak(tp)
register struct tty *tp;
{
	wakeup((caddr_t)&tp->t_dev);
}

#endif
