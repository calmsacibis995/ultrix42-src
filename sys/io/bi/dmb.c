#ifndef lint
static char *sccsid = "@(#)dmb.c	4.5	ULTRIX	4/30/91";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1986-1988 by			*
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
 * dmb.c
 *
 * Modification history
 *
 * DMB32 terminal driver
 *
 * 25-Apr-91 - R. Craig Peterson
 *
 *	Don't generate a formfeed when closing a dmb parallel port
 *	unless the device is currently on-line and ready to print.
 *
 *	Don't allow a minor number for dmb printer that is out of
 *	the range of the number of dmb's on the system.
 *
 * 28-Jan-91 - Stuart Hollander
 *	Replaced code with functionally equivalent code to avoid
 *	bug in C optimizer.  in dmbxint()
 *
 * 08-Aug-90 - Kuo-Hsiung Hsieh
 *
 *	Added a missing case for a break condition.  If neither 
 *	IGNBRK or BRKINT is set, and PARMRK is set, a break condition
 *	is read as '\377', '\0' , '\0'.
 *
 * 31-Oct-89 - Randall Brown
 *
 *	Added the support to allow the device to determine if baudrate is 
 *	supported before it is set in the tty data structures.
 *
 * 11-Oct-89 - Randall Brown
 *
 *	Added support for MIPS based machines.  Converted the DMA to be done
 *	in physical mode.
 *
 * 15-Aug-89 - Randall Brown
 *
 *	Changed all references of TCSADFLUSH to TCSAFLUSH 
 *
 * 11-Aug-89 - dws
 *
 *	Release dmb lock after trusted path processing in dmbrintr().
 *
 * 21-Jul-89 - Randall Brown
 *
 *	Moved default open and default close code to tty.c and call it
 *	using tty_def_open() and tty_def_close().  In the close routine,
 *	don't clear the baudrate so that subsequent opens will keep the
 *	present attributes.  This only applies to a Berkeley environment.
 *
 * 30-Jun-89 - Tim Burke
 *
 *	Keep dmb lock held while examining dmb_tbuffct in dmbxint.  This
 *	is needed to keep the line number in acsr fixed to determine an
 *	accurate transfer count.  Also in dmbxint, don't mask off the error
 *	bits in tbuf before examining them.
 *
 * 19-Jun-89 - Randall Brown
 *	Change to dmblopen to not allow open of the printer on DHB.
 *
 * 15-Jun-89 - darrell
 *	Removed cpup as function arg.
 *
 * 12-Jun-89 - dws
 *
 *	Added trusted path support.
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
 * 07-Jul-88 - Tim Burke
 *	Use address of post_wakeup when calling dmb_start_tty.
 *
 * 16-May-88 - Tim Burke
 *
 * 	Call param routine for setting of local mode word because it can 
 * 	affect bit size and parity.
 *
 * 22-Mar-88 - Tim Burke
 *	Added initial version of locking for SMP.  Locks are used on the
 *	individual tty line as well as one lock per dmb board.
 *
 * 09-Mar-88 - Tim Burke
 *	Changed flow control mechanism to enable support for the DHB32.
 *	Previously if a stop char was received the dmbstop routine would do
 *	a dma abort, the start routine would resetup for the resumption.
 *	Now in dmbstop, simply turn off transmit enable, and make note of the
 *	fact in the state (TS_OABORT).  Dmbstart now will turn on transmit 
 *	enable to resume a previously stopped transmission.
 *
 * 20-Feb-88 - Tim Burke
 *	Changed most softCAR[unit&LINEMASK] references to use the CLOCAL
 *	bit of the control flags to determine if the line is set to be a
 *	modem line or a direct connect.  The setting of softCAR[] remains
 *	to allow one to set default settings for device open.
 *
 *	Provide Support for the DHB32.  This device is a 16 line terminal
 *	multiplexer.  It does not have a printer port or sync port.  Modem
 *	control is not provided on all lines, only the first two lines if 
 *	configured for modem control.
 *
 *  3-Feb-88 - Tim Burke
 *  	Only examine O_NONBLOCK for POSIX mode.
 *	A valid character of 0377 is returned as 0377,0377 in TERMIODISC only.
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
 *	Defined LINEMASK for this driver and replaced references where 
 *	appropriate. Also fixed DEVIOCGET code to use LINEMASK.	
 *
 *  2-Sept-87 - Tim Burke
 *
 *	Added support for hardware auto flow control on the outgoing side.  This
 *	will provide quick response to start/stop characters which will reduce
 *	buffer overflow on the receiving device.
 *
 *  1-Sept-87 - Tim Burke
 *
 *	Put a timer in dmbstart to prevent possible system hang if the DMA
 *	start bit doesn't clear due to hardware failure.
 * 19-Mar-87 - Tim Burke
 *
 *	Added full TERMIO functionality to terminal subsystem.
 *
 *
 * 28-Jan-87 - Tim Burke
 *
 *	Added the capability to ignore the "DSR" modem signal.  This is being
 *	done to allow modems that do not follow DEC Standard 52 to still 
 *	function as they would have prior to the addition of DECSTD52 code
 *	into the drivers.  If the driver is setup to ignore "DSR" then it will
 *	not be following DECSTD52.  To follow DECSTD52 set dmbdsr to "1", to
 *	ignore "DSR" set dmbdsr to be "0";
 *
 *	Removed unnecessary return values from dmblint, dmb_dsr_check, 
 *	dmb_cd_drop, dmb_tty_drop. A few other lint cleanups with Jim
 *	Woodward.
 *
 *  9-Jan-87 - Tim Burke
 *
 *	Bug fix to TIOCMODEM to clear modem flags if signals are not up.
 *
 * 15-Dec-86 - Tim Burke
 *
 *	When a break occurs, (interpreted as a framing error) set the variable
 *	c to be the interrupt character.  There was a problem here due to the
 *	fact that sign extension is done which causes unwanted side affects. To
 *	solve this bug, the character is stripped to 8 bits.
 *
 *	Fix DEVIOGET to return propper modem status information.
 *
 *  4-Dec-86 - Tim Burke
 *
 *	Bug fix to modem control.  In dmb_tty_drop routine, clear the stopped
 *	state to prevent modem lines from hanging on close.  
 *
 * 26-Sep-86 - afd (Al Delorey)
 *
 *	Enable external vector in the User Interface Interrupt Control
 *	Register (a BIIC reg).  Original DMB32 always had this bit set, but
 *	that violated BI standard. So driver does it in dmbinit() & dmbreset().
 *	
 * 26-Aug-86 - rsp (Ricky Palmer)
 *
 *	Cleaned up devioctl code to (1) zero out devget structure
 *	upon entry and (2) use strlen instead of fixed storage
 *	for bcopy's.
 *
 * 25-Aug-86 - Tim Burke
 *
 *	Insure that a modem will drop and restart line on a flase call.  Also
 *	set state to ~TS_CARR_ON on line drop to terminate associated processes.
 *
 * 05-Aug-86 - Tim Burke
 *
 *	In dmbrint, record the present timestamp in the event of a
 *	carrier drop.
 *
 * 11-Jul-86 - ricky palmer
 *
 *	Added adpt and nexus fields to DEVIOCGET code.
 *
 * 29-May-86 - afd & tim burke
 *	Enable DTR & RTS in open routine for both hardwired and modem lines.
 *	This was being done only for modem lines.
 *
 * 26-Apr-86 - ricky palmer
 *	Added new DEVIOCGET ioctl request code. V2.0
 *
 *  4-Apr-86 - afd
 *	Call "bidev_vec()" form dmbinit() to set up interrupt vector handlers.
 *
 */

#include "dmb.h"
#if NDMB > 0 || defined (BINARY)

#include "../data/dmb_data.c"

/*
 * The line printer on dmbx is indicated by a minor device code of 128+x
 * The flags field of the config file is interpreted as follows:
 *
 * bits 	meaning
 * ---- 	-------
 * 0-7		soft carrier bits for the  8 async tty lines on DMB32
 * 0-15		soft carrier bits for the 16 async tty lines on DHB32
 * 8-15 	number of cols/line on the line printer (if 0, 132 will be used)
 * 16-23	number of lines/page on the line printer (if 0, 66 will be used)
 *
 */


#ifdef DEBUG
int dmbdebug = 0;
#define printd if (dmbdebug) mprintf
#define printd10 if (dmbdebug >= 10) mprintf
#endif

extern struct bidata bidata[];

/*
 * Definition of the driver for the auto-configuration program.
 */
int	dmbinit(), dmbattach(), dmbaint(), dmbsint(), dmblint(), dmbbaudrate();
struct	uba_device *dmbinfo[NDMB];
u_short dmbstd[] = { 0 };
struct	uba_driver dmbdriver =
	{ dmbinit, 0, dmbattach, 0, dmbstd, "dmb", dmbinfo };

/* function type definitions */

int	dmbstart(), ttrstrt();
int	wakeup();

/* For for DEC std 52, modem control */

int	dmb_cd_drop(), dmb_dsr_check_timeout(), dmb_cd_down(), dmb_tty_drop();
struct	timeval dmbzerotime = {0,0};
int	dmbcdtime = 2;
#ifndef MODEM_CD
#define MODEM_CD   0x01
#define MODEM_DSR  0x02
#define MODEM_CTS  0x04
#define MODEM_DSR_START  0x08
#endif

#define LINEMASK        0xF    /* line number mask */
#define LINEBITS	0x4    /* 4 bits are needed to represent line number */

/*
 * Local variables for the driver
 */
int	dmb_timeout = 10;		/* receive fifo timeout, in ms */

char dmb_speeds[] =
	{ 0, 0, 01, 02, 03, 04, 0, 05, 06, 07, 010, 012, 013, 015, 016, 017 };
	/*  50	75  110 134 150   300 600 1200 1800 2400 4800 9600 19.2 38.4 */

short	dmb_valid_speeds = 0xffbf; /* 1,1,1,1, 1,1,1,1, 1,0,1,1, 1,1,1,1 */

#define ASLP 1		/* waiting for interrupt from dmb for printer port */
#define OPEN 2		/* line printer is open */
#define ERROR 4 	/* error while printing, driver
			   refuses to do anything till closed */

/*
 * Macros used for smp locking.
 */
/*
 * Macro:  DMB_TTY_LOCK(tp,saveipl)
 *
 * Description:
 * Raise ipl to level 15 and save off old
 * value in "saveipl".
 * Lock the tty structure pointed
 * to by "tp".
 */
#define DMB_TTY_LOCK(tp,saveipl) {	\
		saveipl = spltty();\
		smp_lock(&tp->t_lk_tty, LK_RETRY);\
	}

/*
 * Macro:  DMB_TTY_UNLOCK
 *
 * Description:
 * Unlock tty structure pointed to by 
 * "tp".  Restore IPL to "saveipl".
 */
#define DMB_TTY_UNLOCK(tp,saveipl) {\
	smp_unlock(&tp->t_lk_tty);\
	splx(saveipl);\
	}

/*
 * Macro:  DMB_LOCK
 *
 * Description:
 * Lock dmb device.
 */
#define DMB_LOCK(dmb) \
	smp_lock(&lk_dmb[dmb],LK_RETRY);

/*
 * Macro:  DMB_UNLOCK
 *
 * Description:
 * Unlock dmb device.
 */
#define DMB_UNLOCK(dmb) \
	smp_unlock(&lk_dmb[dmb]);
/*
 * Define which address to do a wakeup on.  These are used to insure that
 * wakeups are not called while locks are held.
 */
#define WAKEUP_RAWQ	0x1		/* wakeup on raw queue */
#define WAKEUP_DEV	0x2		/* wakeup on device address */
#define WAKEUP_OUTQ	0x4		/* wakeup on output queue */
#define WAKEUP_STATE	0x8		/* wakeup on line printer state */
#define WAKEUP_BUF	0x10		/* wakeup on line printer buffer */
#define WAKEUP_SELECT	0x20		/* select wakeup */ 

/*
 * Routine for configuration to see if the DMB32 is functional
 */
/*ARGSUSED*/
dmbinit(nxv,nxp,binumber,binode,ui)
	caddr_t nxv;			/* virt addr of device */
	caddr_t nxp;			/* phys addr of device */
	int binumber;			/* this bi number on the system */
	int binode;			/* the node number on this bi */
	struct uba_device *ui;
{
	register struct dmb_device *addr;

	addr = (struct dmb_device *)nxv;
#	ifdef DEBUG
	printd("dmbinit: addr = 0x%x, devtype = 0x%x, revcode = 0x%x, nodeid = %d\n",
		addr,
		addr->dmb_biic.biic_typ & 0xFFFF,
		(addr->dmb_biic.biic_typ >> 16) & 0xFFFF,
		addr->dmb_biic.biic_ctrl & 0xF);
#	endif

	/*
	 * Determine the number of asynchronous lines on the device:
 	 *	DMB has 8 lines
	 *	DHB has 16 lines
	 */
	dmb_lines[ui->ui_unit] = addr->dmb_config & DMB_ASYNC_MASK;
	/*
	 * Check for device malfunction.  Distribution cable is connected.
	 */
	if ((addr->dmb_mainthigh & DMB_ALP) == 0)
		{
		printf("dmbinit: async lines unavailable\n");
		return(0);
		}
	/*
	 * Only the DMB has a printer port. (The DHB doesn't)
	 */
	if ((dmb_lines[ui->ui_unit]==DMB_8_LINES) && ((addr->dmb_mainthigh & DMB_PP) == 0))
		{
		printf("dmbinit: printer port unavailable\n");
		return(0);
		}
	/*
	 * Check results of powerup self tests (if they've completed).
	 */
	if ((addr->dmb_maintlow & DMB_RESET) == 0) {
		if (addr->dmb_mainthigh & DMB_DFAIL)
			printf("dmbinit: warning, self test diagnostic failure\n");
#       	ifdef DEBUG
		/*
		 * See if the 6 ribbon cables are correctly connected to pannel.
		 * This was being printed on functional systems (DMB) ??
		 */
		if (((addr->dmb_mainthigh & DMB_CABLE) == 0) && dmbdebug)
			printf("dmbinit: CABLE error, check ribbon cables, reg=%x\n",
				addr->dmb_mainthigh);
#       	endif
	}
	/*
	 * Enable external vector
	 */
	addr->dmb_biic.biic_int_ctrl |= BIIC_EXVEC;

	/*
	 * Set up interrupt vectors for DMB32.	It has 4.
	 * BIIC (14), sync (15), async (16), & printer (17).
	 * There are 4 interrupt vectors for a dmb.  They are LEVEL14,15,16,17.
	 * These levels do not correspond to ipl levels, for example LEVEL15 on
	 * the async lines cause interrupts at ipl14.
	 */
	bidev_vec(binumber,binode,LEVEL15,ui);
	return(1);
}

dmbattach(ui)
	struct uba_device *ui;
{
	register int cols;
	register int lines, i;
	register struct dmb_device *addr;
	register struct tty *tp;

	addr = (struct dmb_device *)ui->ui_addr;
#	ifdef DEBUG
	printd("dmbattach: addr = 0x%x, flags = 0x%x, unit = %d\n",
		addr, ui->ui_flags, ui->ui_unit);
#	endif
	/*
	 * ui_flags format:
	 * 	DMB: 0-7  dmbsoftcar, 8-15  printer cols, 16-23 printer rows.
	 * 	DHB: 0-15 dmbsoftcar
	 *
	 * Set soft carrier (local lines) & line printer characteristics
	 */
	if (dmb_lines[ui->ui_unit] == DMB_8_LINES)
		{
		dmbsoftCAR[ui->ui_unit] = ui->ui_flags & 0xff;
		dmbdefaultCAR[ui->ui_unit] = ui->ui_flags & 0xff;
		cols = (ui->ui_flags>>8) & 0xff;
		lines = (ui->ui_flags>>16) & 0xff;
		}
	else	/* 16 line dhb device */
		{
		dmbsoftCAR[ui->ui_unit] = ui->ui_flags & 0xffff;
		dmbdefaultCAR[ui->ui_unit] = ui->ui_flags & 0xffff;
		cols = (ui->ui_flags>>16) & 0xff; /* just in case ... */
		lines = (ui->ui_flags>>24) & 0xff;
		}
	dmbl_softc[ui->ui_unit].dmbl_cols = (cols == 0?DMBL_DEFCOLS:cols);
	dmbl_softc[ui->ui_unit].dmbl_lines = (lines == 0?DMBL_DEFLINES:lines);
	/*
	 * Initialize SMP locks.  One lock per dmb board, and one tty lock
	 * for each line on the board.  Specify this driver as being MP safe.
	 */
	lockinit(&lk_dmb[ui->ui_unit], &lock_device15_d);

	for (i=0; i< NUMLINES ; i++) {
		tp = &dmb_tty[(ui->ui_unit * NUMLINES)+i];
		lockinit(&tp->t_lk_tty, &lock_tty_d);
		tp->t_smp = 1;
	}	
}

/*
 * Open a DMB32 line.
 * Turn on this dmb if this is the first use of it.
 */
/*ARGSUSED*/
dmbopen(dev, flag)
	dev_t dev;
	int flag;
{
	register struct tty *tp;
	register int unit, dmb;
	register struct dmb_device *addr;
	register struct uba_device *ui;
	int s,error;
	int inuse;  /* hold state of inuse bit while blocked waiting for carr */
	int post_wakeup = 0;

	unit = minor(dev);
	if (unit & 0200)
		return(dmblopen(dev,flag));
	dmb = unit >> LINEBITS;
#	ifdef DEBUG
	printd("dmbopen: dev = 0x%x, flag = 0x%x, unit = 0x%x\n", dev, flag, unit);
#	endif
	if (unit >= ndmb || (ui = dmbinfo[dmb])== 0 || ui->ui_alive == 0)
		return (ENXIO);
	tp = &dmb_tty[unit];
	DMB_TTY_LOCK(tp,s);
	if (tp->t_state&TS_XCLUDE && u.u_uid != 0) {
		DMB_TTY_UNLOCK(tp,s);
		return (EBUSY);
	}
	while (tp->t_state&TS_CLOSING) { /* let DTR stay down for awhile */
		sleep_unlock((caddr_t)&tp->t_rawq, TTIPRI,&tp->t_lk_tty);
		DMB_TTY_LOCK(tp,s);
	}
	addr = (struct dmb_device *)ui->ui_addr;
	tp->t_addr = (caddr_t)addr;
	tp->t_oproc = dmbstart;
	tp->t_baudrate = dmbbaudrate;
	tp->t_state |= TS_WOPEN;

	DMB_LOCK(dmb);
	tty_def_open(tp, dev, flag, (dmbsoftCAR[dmb]&(1<<(unit&LINEMASK))));
	if ((tp->t_state&TS_ISOPEN) == 0) {
	    dmbmodem[unit] = MODEM_DSR_START; /* prevents spurious startups */
	    /*
	     * Set the receive fifo timeout to this number of miliseconds.
	     */
	    addr->dmb_acsr2 = (dmb_timeout << 16);
	}
	/*
	 * Interrupts are enabled in dmbparam.
	 */
	dmbparam(unit);
	/*
	 * Wait for carrier, then process line discipline specific open.
	 */
	addr->dmb_acsr = (unit & LINEMASK) | DMB_IE;	/* line select */
	addr->dmb_lpr |= (DMB_DTR | DMB_RTS);
	if (tp->t_cflag & CLOCAL)
		{
#		ifdef  DEBUG
		printd("dmbopen: local, tp=0x%x\n", tp);
#		endif
		tp->t_state |= TS_CARR_ON;
		dmbmodem[unit] = MODEM_CTS|MODEM_CD|MODEM_DSR;
		}
	else if ((flag & (O_NDELAY|O_NONBLOCK)) == 0)
		{
		/*
		 * DSR should not normally come up until DTR is asserted
		 * However if TS_HUPCL is not set, it is
		 * possible to get here with all modem signals
		 * already asserted.  Or we could be dealing with
		 * a very slow modem and it has not deasserted DSR yet.
		 * Interrupts are enabled earlier in dmbparam.
		 * 
		 * If the DSR signal is being followed, wait at most
		 * 30 seconds for CD, and don't transmit in the first 
		 * 500ms.  Otherwise immediately look for CD|CTS.
		 */
		if (dmbdsr) {
			if (addr->dmb_lstatlow & DMB_DSR)
				{
#				ifdef DEBUG
				printd("dmbopen: modem, unit=%d\n", unit);
#				endif
				dmbmodem[unit] |= (MODEM_DSR_START|MODEM_DSR);
				tp->t_dev = dev; /* need it for timeouts */
				timeout(dmb_dsr_check_timeout, tp, hz*30);
				timeout(dmb_dsr_check_timeout, tp, hz/2);
				}
			}
		else 	{
			dmbmodem[unit] |= (MODEM_DSR_START|MODEM_DSR);
			dmb_dsr_check(tp,&post_wakeup);
			}
		}
	DMB_UNLOCK(dmb);
	if (flag & (O_NDELAY|O_NONBLOCK))
		tp->t_state |= TS_ONDELAY;
	else
		while ((tp->t_state & TS_CARR_ON) == 0)
			{
			tp->t_state |= TS_WOPEN;
			inuse = tp->t_state&TS_INUSE;
			sleep_unlock((caddr_t)&tp->t_rawq,TTIPRI,&tp->t_lk_tty);
			DMB_TTY_LOCK(tp,s);
			/*
			 * See if wakeup was caused by a false start.
			 */
			if (dmbmodem[unit]&MODEM_BADCALL){
				DMB_TTY_UNLOCK(tp,s);
				if (post_wakeup)
					wakeup((caddr_t)&tp->t_rawq);
				return(EWOULDBLOCK);
			}
			/*
			 *  If we opened "block if in use"  and
			 *  the terminal was not in use at that time
			 *  but it became "in use" while we were
			 *  waiting for carrier then return.
			 */
			if ((flag & O_BLKINUSE) && (inuse==0) &&
			    (tp->t_state&TS_INUSE))
				{
				DMB_TTY_UNLOCK(tp,s);
				if (post_wakeup)
					wakeup((caddr_t)&tp->t_rawq);
				return(EALREADY);
				}
			}
	/*
	 * Set state bit to tell tty.c not to assign this line as the 
	 * controlling terminal for the process which opens this line.
	 */
	if ((flag & O_NOCTTY) && (u.u_procp->p_progenv == A_POSIX))
		tp->t_state |= TS_ONOCTTY;
	error = (*linesw[tp->t_line].l_open)(dev, tp);
	DMB_TTY_UNLOCK(tp,s);
	if (post_wakeup)
		wakeup((caddr_t)&tp->t_rawq);
	return(error);
}


/*
 * Close a DMB32 line.
 */
/*ARGSUSED*/
dmbclose(dev, flag)
	dev_t dev;
	int flag;
{
	register struct tty *tp;
	register int unit;
	register int dmb;
	register struct dmb_device *addr;
	int post_wakeup = 0;
	int s;

	unit = minor(dev);
	if (unit & 0200){
		dmblclose(dev,flag);
		return;
	}
	dmb = unit >> LINEBITS;

	tp = &dmb_tty[unit];
	addr = (struct dmb_device *)tp->t_addr;
#	ifdef DEBUG
	printd("dmbclose: dev = 0x%x, flag = 0x%x, unit = 0x%x, addr = 0x%x\n",
		dev, flag, unit, addr);
#	endif
	DMB_TTY_LOCK(tp,s);
	if (tp->t_line)
		(*linesw[tp->t_line].l_close)(tp);
	DMB_LOCK(dmb);
	if ((tp->t_cflag&HUPCL) || (tp->t_state&TS_ISOPEN)==0)
		{
		addr->dmb_acsr = (unit & LINEMASK) | DMB_IE;
		addr->dmb_lpr &= ~(DMB_DTR | DMB_RTS);
		tp->t_state &= ~TS_CARR_ON; /* prevents recv intr. timeouts */
		if ((tp->t_cflag & CLOCAL) == 0)
			{
			/*
			 * Drop DTR for at least a half sec. if modem line
			 */
			tp->t_state |= TS_CLOSING;
			/*
			 * Wait for DSR to drop
			 */
			addr = (struct dmb_device *)tp->t_addr;
			addr->dmb_acsr = DMB_IE | (unit&LINEMASK);
			/*
 			 * If the DSR signal is being followed, give the
			 * modem at most 5 seconds to deassert it.
 			 */
			if (dmbdsr && (addr->dmb_lstatlow & DMB_DSR))
				{
				timeout(wakeup, (caddr_t) &tp->t_dev, 5*hz);
				DMB_UNLOCK(dmb);
				sleep_unlock((caddr_t)&tp->t_dev, PZERO-10,
					&tp->t_lk_tty);
				DMB_TTY_LOCK(tp,s);
				DMB_LOCK(dmb);
				}
			timeout(wakeup, (caddr_t) &tp->t_dev, hz/5);
			DMB_UNLOCK(dmb);
			sleep_unlock((caddr_t)&tp->t_dev, PZERO-10,
					&tp->t_lk_tty);
			DMB_TTY_LOCK(tp,s);
			DMB_LOCK(dmb);
			tp->t_state &= ~(TS_CLOSING);
			post_wakeup = WAKEUP_RAWQ;
			}
		addr->dmb_acsr = (unit & LINEMASK) | DMB_IE;
		addr->dmb_lpr &= ~DMB_RXENA;
		}
	dmbsoftCAR[dmb] &= ~(1<<(unit&LINEMASK));
	dmbsoftCAR[dmb] |= (1<<(unit&LINEMASK)) & dmbdefaultCAR[dmb];
	DMB_UNLOCK(dmb);
	ttyclose(tp);
	dmbmodem[unit] = 0;
	tty_def_close(tp);
	DMB_TTY_UNLOCK(tp,s);
	if (post_wakeup)
		wakeup((caddr_t)&tp->t_rawq);
}


dmbread(dev, uio)
	dev_t dev;
	struct uio *uio;
{
	register struct tty *tp;
        register int unit;

        unit = minor(dev);

	if (unit & 0200)		/* read from lp device */
		return(ENXIO);
	tp = &dmb_tty[unit];
	return ((*linesw[tp->t_line].l_read)(tp, uio));
}

dmbwrite(dev, uio)
	dev_t dev;
	struct uio *uio;
{
        register int unit;
	register struct tty *tp;

        unit = minor(dev);

	if (unit & 0200)		/* write to parallel port */
		return(dmblwrite(dev,uio));
	tp = &dmb_tty[unit];
	return ((*linesw[tp->t_line].l_write)(tp, uio));
}

/*
 * DMB32 interrupts at one location with async interrupts.
 * Examine Transmitter Action bit in "tbuf" register to determine
 *   if there is a transmit interrupt.
 * Examine Data Valid bit in "rbuf" register to determine
 *   if there is a receive interrupt.
 */

dmbaint(dmb)
	int dmb;
{
	register struct dmb_device *addr;
	register struct uba_device *ui;
	/*
	 * The dmb is interrupting at ipl14.
	 * For this reason an spltty is used to bump the ipl up to ipl15.
	 */
	spltty();

	ui = dmbinfo[dmb];
	addr = (struct dmb_device *)ui->ui_addr;
	if (addr->dmb_tbuf & DMB_TXACT)
	    dmbxint(dmb);
	if (addr->dmb_rbuf & DMB_DATAVALID)
	    dmbrint(dmb);
}

/*
 * DMB32 receiver interrupt.
 */
dmbrint(dmb)
	int dmb;
{
	register struct tty *tp;
	register long c;
	register struct dmb_device *addr;
	struct tty *tp0;
	register struct uba_device *ui;
	register int line;
	register int flg;
	int overrun = 0;
	u_char *modem0, *modem;
	int modem_cont;
	int post_wakeup = 0;

	ui = dmbinfo[dmb];
	if (ui == 0 || ui->ui_alive == 0)
		return;
	addr = (struct dmb_device *)ui->ui_addr;
	tp0 = &dmb_tty[dmb<<LINEBITS];
	modem0 = &dmbmodem[dmb<<LINEBITS];
	/*
	 * Loop fetching characters from the receive fifo for this
	 * dmb until there are no more in the fifo.
 	 *
	 * First get a copy of the rbuf then make sure no other processor
 	 * is servicing this interrupt by seeing if the rbuf contents have
	 * changed in the meantime.
	 */
	while ((c = addr->dmb_rbuf) < 0)
		{	/* if c < 0 then data valid bit is set */
		line = (c>>16)&LINEMASK;
		tp = tp0 + line;
		smp_lock(&tp->t_lk_tty,LK_RETRY);
		DMB_LOCK(dmb);
		if ((c = addr->dmb_rbuf) >= 0) {
			DMB_UNLOCK(dmb);
			smp_unlock(&tp->t_lk_tty);
			break;
		}
		if (line != (((addr->dmb_rbuf)>>16) & LINEMASK)) {
			DMB_UNLOCK(dmb);
			smp_unlock(&tp->t_lk_tty);
  			continue;
  		}
		addr->dmb_rbuf = 0;		/* pop the fifo */
		DMB_UNLOCK(dmb);
		flg = tp->t_iflag;
		modem = modem0 + line;
#		ifdef  DEBUG
		printd10("dmbrint: tp = 0x%x, c = 0x%x\n", tp, c);
#		endif
		/*
		 * Check for modem transitions
		 */
		if (c & DMB_NONCHAR)
			{
			if (c & DMB_DIAG)
				goto dmbrunlock;	/* ignore diagnostic info */
#			ifdef DEBUG
			printd("dmbrint: modem change, line = %d, tp = 0x%x, c = 0x%x, lstat = 0x%x\n", line, tp, c, addr->dmb_lstatlow);
#			endif DEBUG
			if (tp->t_cflag & CLOCAL)
				goto dmbrunlock;
			DMB_LOCK(dmb);
			addr->dmb_acsr = (line | DMB_IE);
			modem_cont = 0;
			/*
			 * Drop DTR immediately if DSR gone away.
			 * If really an active close then do not
			 *    send signals.
			 */
			if ((addr->dmb_lstatlow & DMB_DSR) == 0)
				{
				if (tp->t_state&TS_CLOSING)
					{
					untimeout(wakeup, (caddr_t) &tp->t_dev);
					post_wakeup = WAKEUP_DEV;
#						ifdef	DEBUG
						printd("dmbrint: dsr closing down, line=%d\n",line);
#						endif	DEBUG
					DMB_UNLOCK(dmb);
					goto dmbrunlock;
					}
				 if (tp->t_state & TS_CARR_ON)
					{
#						ifdef	DEBUG
						printd("dmbrint: DSR drop, line = %d\n",line);
#						endif	DEBUG
					/*
 					 * Drop line if DSR is being followed.
 					 */
					if (dmbdsr)
						{
						dmb_tty_drop(tp,&post_wakeup);
						/*
						 * Move continue here in order
						 * to examine other transitions.
						 */
						DMB_UNLOCK(dmb);
						goto dmbrunlock;
						}
					}
				}
			/*
			 * Check for transient CD drops.
			 * Only drop DTR if CD is down for more than 2 secs.
			 */

			if (tp->t_state&TS_CARR_ON)
				if ((addr->dmb_lstatlow & DMB_DCD)==0)
					{
					if (*modem & MODEM_CD)
						{
						/* only start timer once */
#						ifdef DEBUG
						printd("dmbrint, cd_drop,line = %d\n",line);
#						endif DEBUG
						*modem &= ~MODEM_CD;
						dmbtimestamp[minor(tp->t_dev)] = time;
						timeout(dmb_cd_drop, tp, hz*dmbcdtime);
						modem_cont = 1;
						}
					}
				else
				    /*
				     * CD has come up again.
				     * Stop timeout from occurring if set.
				     * If interval is more than 2 secs then
				     *	drop DTR.
				     */
				    if ((*modem & MODEM_CD)==0)
					    {
					    untimeout(dmb_cd_drop, tp);
					    if (dmb_cd_down(tp)){
						    /* drop connection */
						dmb_tty_drop(tp,&post_wakeup);
					    }
					    *modem |= MODEM_CD;
					    modem_cont = 1;
					    }
			/* CTS flow control check */

			if (tp->t_state&TS_CARR_ON)
				if ((addr->dmb_lstatlow & DMB_CTS)==0)
					{
					tp->t_state |= TS_TTSTOP;
					*modem &= ~MODEM_CTS;
#					ifdef DEBUG
					printd("dmbrint: CTS stop, line=%d\n",line);
#					endif DEBUG
					DMB_UNLOCK(dmb);
					dmbstop(tp, 0);
					goto dmbrunlock;
					}
				else if ((*modem&MODEM_CTS)==0)
					{
					tp->t_state &= ~TS_TTSTOP;
					*modem |= MODEM_CTS;
#					ifdef DEBUG
					printd("dmbrint: CTS start, line=%d\n",line);
#					endif DEBUG
					DMB_UNLOCK(dmb);
					dmbstart(tp);
					goto dmbrunlock;
					}
			/*
			 * Avoid calling dmb_start_tty due to a CD transition 
			 */
			if (modem_cont) {
				DMB_UNLOCK(dmb);
				goto dmbrunlock;
			}


			/*
			 * If 500 ms timer has not expired then don't
			 * check anything yet.
			 * Check to see if DSR|CTS|DCD are asserted.
			 * If so we have a live connection.
			 * If DSR is set for the first time we allow
			 * 30 seconds for a live connection.
			 *
			 * If the DSR signal is being followed, wait at most
			 * 30 seconds for CD, and don't transmit in the first 
			 * 500ms.  Otherwise immediately look for CD|CTS.
			 */
			if (dmbdsr) {
				if ((addr->dmb_lstatlow & DMB_XMIT) == DMB_XMIT
			    	&& (*modem & MODEM_DSR_START)==0)
					dmb_start_tty(tp,&post_wakeup);
				else
			    	if ((addr->dmb_lstatlow & DMB_DSR) &&
					(*modem & MODEM_DSR)==0)
					{
					*modem |= (MODEM_DSR_START|MODEM_DSR);
					/*
				 	* We should not look for CTS|CD for
				 	* about 500 ms.
				 	*/
					timeout(dmb_dsr_check_timeout,tp,hz*30);
					timeout(dmb_dsr_check_timeout,tp,hz/2);
					}
			}
			/*
			 * Ignore DSR
			 */
			else
				if ((addr->dmb_lstatlow & DMB_NODSR) == DMB_NODSR)
					dmb_start_tty(tp,&post_wakeup);

			DMB_UNLOCK(dmb);
			goto dmbrunlock;
			} /* end of modem transition tests */
		if ((tp->t_state&TS_ISOPEN)==0)
			{
			post_wakeup = WAKEUP_RAWQ;
			goto dmbrunlock;
			}

		/*
		 * Check for framing or parity errors.
		 */
		if (c & (DMB_PARITYERR|DMB_FRAMEERR)) {
			/* 
			 * If input parity checking is not enabled, clear out
			 * parity error in this character.  Don't leave the
			 * error checks because it could still contain a
			 * framing error.
			 */
			if ((c & DMB_PARITYERR) && ((flg & INPCK) == 0)) 
				c &= ~DMB_PARITYERR;
			/*
			 * For POSIX:
			 * A break is defined to be a character with a framing
			 * error and data bits of all zero.  Note that it is
			 * possible to have a character with a framing error
			 * that is not a break.
			 * For non-POSIX:
			 * For backward compatiblity, it used to be the case 
			 * that anything with a framing error was considered
			 * to be a break.
			 */
			if ((c & DMB_FRAMEERR) && ((tp->t_line != TERMIODISC) ||
				((c & CHAR_MASK) == 0))) {
				/*
				 * If configured for trusted path, initiate
				 * trusted path handling.
				 */
				if (do_tpath) {
					tp->t_tpath |= TP_DOSAK;
					(*linesw[tp->t_line].l_rint)(c, tp);
					goto dmbrunlock;
				}
				if (flg & IGNBRK)  
					goto dmbrunlock;
				if (flg & BRKINT) {
					if ((tp->t_lflag_ext & PRAW) && 
						(tp->t_line != TERMIODISC))
						c = 0;
					else {
					    ttyflush(tp, FREAD|FWRITE);
					    gsignal(tp->t_pgrp, SIGINT);
					    smp_unlock(&tp->t_lk_tty);
					    continue;
					}
				}
				/*
			 	 * TERMIO: If neither IGNBRK or BRKINT is set, a
			 	 * break condition is read as a single '\0'.
				 * or if PARMRK is set as '\377', '\0' , '\0'.
			 	 */
				else {
					if (flg & PARMRK){
						(*linesw[tp->t_line].l_rint)(0377,tp);
						(*linesw[tp->t_line].l_rint)(0,tp);
					}
					c = 0;
				}
			}
			else if (c & (DMB_PARITYERR|DMB_FRAMEERR)) {
				if (flg & IGNPAR) 
					goto dmbrunlock;   /* dispose of char */
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
		/*
		 * This is frequently an indication that the bus is overloaded.
		 * For example a heavily used disk on the same uba may
		 * cause overruns to occur.
		 */
		if ((c & DMB_OVERRUNERR) && overrun == 0) {
			printf("dmb%d: recv. fifo overflow\n", dmb);
			overrun = 1;
		}

		if (flg & ISTRIP)
			c &= 0177;	
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

		(*linesw[tp->t_line].l_rint)(c, tp);
dmbrunlock:
		smp_unlock(&tp->t_lk_tty);
		if (post_wakeup) 
			{
			if (post_wakeup & WAKEUP_RAWQ)
				wakeup((caddr_t)&tp->t_rawq);
			else
				wakeup((caddr_t) &tp->t_dev);
			}
		}  /* end while c < 0 */
}

/*
 * Ioctl for DMB32.
 */
/*ARGSUSED*/
dmbioctl(dev, cmd, data, flag)
	dev_t dev;
	register int cmd;
	caddr_t data;
	int flag;
{
	register int unit = minor(dev);
	register struct tty *tp;
	register int dmb;
	register struct dmb_device *addr;
	register int s;
	struct uba_device *ui;
	struct dmb_softc *sc;
	struct devget *devget;
	int error;

	if (unit & 0200)
		return (ENOTTY);
	tp = &dmb_tty[unit];
	dmb = unit >> LINEBITS;
	ui = dmbinfo[dmb];
	sc = &dmb_softc[ui->ui_unit];
	error = (*linesw[tp->t_line].l_ioctl)(tp, cmd, data, flag);
	if (error >= 0)
		return (error);
	error = ttioctl(tp, cmd, data, flag);
	DMB_TTY_LOCK(tp,s);
	if (error >= 0)
		{
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
				DMB_LOCK(dmb);
 				dmbparam(unit);
				DMB_UNLOCK(dmb);
 				break;
 			}
		DMB_TTY_UNLOCK(tp,s);
		return (error);
		}
	addr = (struct dmb_device *)tp->t_addr;

#	ifdef DEBUG
	if (dmbdebug)
	   printd("dmbioctl: unit = %d, cmd = %d, data = 0x%x, flag = 0x%x\n",
		unit, cmd & 0xff, data, flag);
#	endif

	DMB_LOCK(dmb);
	switch (cmd) {

	case TIOCSBRK:
		addr->dmb_acsr = (unit & LINEMASK) | DMB_IE;
		addr->dmb_lpr |= DMB_BREAK;
		break;

	case TIOCCBRK:
		addr->dmb_acsr = (unit & LINEMASK) | DMB_IE;
		addr->dmb_lpr &= ~DMB_BREAK;
		break;

	case TIOCSDTR:
		addr->dmb_acsr = (unit & LINEMASK) | DMB_IE;
		addr->dmb_lpr |= (DMB_DTR | DMB_RTS);
		break;

	case TIOCCDTR:
		addr->dmb_acsr = (unit & LINEMASK) | DMB_IE;
		addr->dmb_lpr &= ~(DMB_DTR | DMB_RTS);
		break;

	case TIOCMSET:
		addr->dmb_acsr = (unit & LINEMASK) | DMB_IE;
		addr->dmb_lpr &= ~(DMB_DTR | DMB_RTS);
		addr->dmb_lpr |= dmtodmb(*(int *)data);
		break;

	case TIOCMBIS:
		addr->dmb_acsr = (unit & LINEMASK) | DMB_IE;
		addr->dmb_lpr |= dmtodmb(*(int *)data);
		break;

	case TIOCMBIC:
		addr->dmb_acsr = (unit & LINEMASK) | DMB_IE;
		addr->dmb_lpr &= ~(dmtodmb(*(int *)data));
		break;

	case TIOCMGET:
		addr->dmb_acsr = (unit & LINEMASK) | DMB_IE;
		*(int *)data = dmbtodm(addr->dmb_lpr, addr->dmb_lstatlow);
		break;

	case TIOCNMODEM:  /* ignore modem status */
		dmbsoftCAR[dmb] |= (1<<(unit&LINEMASK));
		if (*(int *)data) /* make mode permanent */
			dmbdefaultCAR[dmb] |= (1<<(unit&LINEMASK));
		dmbmodem[unit] = MODEM_CTS|MODEM_CD|MODEM_DSR;
		tp->t_state |= TS_CARR_ON;
		addr->dmb_acsr = (unit & LINEMASK) | DMB_IE;
		addr->dmb_lpr &= ~(DMB_REPORT);
		tp->t_cflag |= CLOCAL;		/* Map to termio */
		break;

	case TIOCMODEM:  /* look at modem status */
		dmbsoftCAR[dmb] &= ~(1<<(unit&LINEMASK));
		if (*(int *)data) /* make mode permanent */
			dmbdefaultCAR[dmb] &= ~(1<<(unit&LINEMASK));
		addr->dmb_acsr = (unit & LINEMASK) | DMB_IE;   /* line select */
		addr->dmb_lpr |= DMB_REPORT;
	   	/* 
	    	 * If dmbdsr is set look for DSR|CTS|CD, otherwise look 
	    	 * for CD|CTS only.
	    	 */
		if ((dmbdsr && ((addr->dmb_lstatlow & DMB_XMIT) == DMB_XMIT)) ||
		    ((dmbdsr == 0) && ((addr->dmb_lstatlow & DMB_NODSR) == DMB_NODSR)))
			{
			tp->t_state |= TS_CARR_ON;
			tp->t_state &= ~TS_ONDELAY;
			dmbmodem[unit] = MODEM_CTS|MODEM_CD|MODEM_DSR;
			}
		else {
			dmbmodem[unit] & = ~(MODEM_CTS|MODEM_CD|MODEM_DSR);
			tp->t_state &= ~(TS_CARR_ON);
		}
		tp->t_cflag &= ~CLOCAL;		/* Map to termio */
		break;
	case TIOCWONLINE:
		addr->dmb_acsr = (unit & LINEMASK) | DMB_IE;   /* line select */
	   	/* 
	    	 * If dmbdsr is set look for DSR|CTS|CD, otherwise look 
	    	 * for CD|CTS only.
	    	 */
		if ((dmbdsr && ((addr->dmb_lstatlow & DMB_XMIT) == DMB_XMIT)) ||
		    ((dmbdsr == 0) && ((addr->dmb_lstatlow & DMB_NODSR) == DMB_NODSR)))
			{
			tp->t_state |= TS_CARR_ON;
			tp->t_state &= ~TS_ONDELAY;
			dmbmodem[unit] = MODEM_CTS|MODEM_CD|MODEM_DSR;
			}
		else {
			while ((tp->t_state & TS_CARR_ON) == 0) {
				DMB_UNLOCK(dmb);
				sleep_unlock((caddr_t)&tp->t_rawq, TTIPRI,
					&tp->t_lk_tty);
				DMB_TTY_LOCK(tp,s);
				DMB_LOCK(dmb);
			}
		}
		break;

	/* handle maintenance mode */
	case TIOCSMLB:
		if (u.u_uid) {
			DMB_UNLOCK(dmb);
			DMB_TTY_UNLOCK(tp,s);
			return(EPERM);
		}
		addr->dmb_acsr = (unit & LINEMASK) | DMB_IE;
		addr->dmb_lpr |= (DMB_MAINT);
		break;

	case TIOCCMLB:
		if (u.u_uid) {
			DMB_UNLOCK(dmb);
			DMB_TTY_UNLOCK(tp,s);
			return(EPERM);
		}
		addr->dmb_acsr = (unit & LINEMASK) | DMB_IE;
		addr->dmb_lpr &= ~(DMB_MAINT);
		break;

	case DEVIOCGET: 			/* device status */
		DMB_UNLOCK(dmb);
		DMB_TTY_UNLOCK(tp,s);
		devget = (struct devget *)data;
		bzero(devget,sizeof(struct devget));

		if (tp->t_cflag & CLOCAL) {
			sc->sc_category_flags[unit&LINEMASK] |= DEV_MODEM;
			sc->sc_category_flags[unit&LINEMASK] &= ~DEV_MODEM_ON;
		}
		else
			sc->sc_category_flags[unit&LINEMASK] |= (DEV_MODEM|DEV_MODEM_ON);

		devget->category = DEV_TERMINAL;
		devget->bus = DEV_BI;
		if (dmb_lines[dmb] == DMB_8_LINES)
			bcopy(DEV_DMB32,devget->interface,strlen(DEV_DMB32));
		else
			bcopy(DEV_DHB32,devget->interface,strlen(DEV_DHB32));
		bcopy(DEV_UNKNOWN,devget->device,
		      strlen(DEV_UNKNOWN));		/* terminal	*/
		devget->adpt_num = ui->ui_adpt; 	/* which adapter*/
		devget->nexus_num = ui->ui_nexus;	/* which node	*/
		devget->bus_num = ui->ui_ubanum;	/* which BI	*/
		devget->ctlr_num = dmb; 		/* which interf.*/
		devget->slave_num = unit&LINEMASK;	/* which line	*/
		bcopy(ui->ui_driver->ud_dname,
		      devget->dev_name,
		      strlen(ui->ui_driver->ud_dname)); /* Ultrix "dmb" */
		devget->unit_num = unit&LINEMASK;	/* which dmb?	*/
		devget->soft_count =
		      sc->sc_softcnt[unit&LINEMASK];	/* soft er. cnt.*/
		devget->hard_count =
		      sc->sc_hardcnt[unit&LINEMASK];	/* hard er cnt. */
		devget->stat = sc->sc_flags[unit&LINEMASK]; /* status	*/
		devget->category_stat =
		      sc->sc_category_flags[unit&LINEMASK]; /* cat. stat. */
		return(0);


	/*
	 * Used to specify that this device has outgoing auto flow control.
	 * Set appropriate bit in lpar.
	 */
	case TIOAUTO:
		/*
	 	* Outgoing Auto flow control.
	 	* No auto flow control allowed if startc != ^q and startc !=
	 	* ^s.  Most drivers do not allow these chars to be changed.
	 	*/
                if ((tp->t_cflag_ext&PAUTOFLOW) && (tp->t_cc[VSTOP] != CTRL('s'))
                        || (tp->t_cc[VSTART] != CTRL('q')))
                        tp->t_cflag_ext &= ~PAUTOFLOW;
		/*
		 * OAUTO doesn't work on the DHB.
 		 * Take this out if it ever does work.
		 */
		if (dmb_lines[dmb] == DMB_16_LINES) 
                        tp->t_cflag_ext &= ~PAUTOFLOW;
		dmbparam(unit);
		break;


	default:
		DMB_UNLOCK(dmb);
		DMB_TTY_UNLOCK(tp,s);
		if (u.u_procp->p_progenv == A_POSIX)
			return (EINVAL);
		return (ENOTTY);
	}
	DMB_UNLOCK(dmb);
	DMB_TTY_UNLOCK(tp,s);
	return (0);
}
dmbtodm(lpr,lstat)
	register long lpr;
	register u_short lstat;
{
	register int b = 0;
	if (lpr&DMB_RTS)  b |= TIOCM_RTS;
	if (lpr&DMB_DTR)  b |= TIOCM_DTR;
	if (lstat&DMB_DCD)  b |= TIOCM_CD;
	if (lstat&DMB_CTS)  b |= TIOCM_CTS;
	if (lstat&DMB_RI)  b |= TIOCM_RI;
	if (lstat&DMB_DSR)  b |= TIOCM_DSR;
	return(b);
}


dmtodmb(bits)
	register int bits;
{
	register long lpr = 0;
	if (bits&TIOCM_RTS) lpr |= DMB_RTS;
	if (bits&TIOCM_DTR) lpr |= DMB_DTR;
	return(lpr);
}


/*
 * Set parameters from open or stty into the DMB hardware
 * registers.
 */
dmbparam(unit)
	register int unit;
{
	register struct tty *tp;
	register struct dmb_device *addr;
	register long lpar;

	tp = &dmb_tty[unit];
	if (tp->t_state & TS_BUSY) {
	    tp->t_state |= TS_NEED_PARAM;
	    return;
	}
	addr = (struct dmb_device *)tp->t_addr;
	addr->dmb_acsr = (unit & LINEMASK) | DMB_IE;	/* line select */
	/* 
 	 * Disconnect modem line if baudrate is zero.  POSIX checks the output
	 * baud rate, while non-POSIX checks the input baud rate.
	 */
	if ((((tp->t_cflag&CBAUD)==B0) && (u.u_procp->p_progenv != A_POSIX)) || 
	    (((tp->t_cflag_ext & CBAUD)==B0) && 
		(u.u_procp->p_progenv == A_POSIX))) 
		{
		tp->t_cflag |= HUPCL;
		addr->dmb_lpr &= ~(DMB_DTR | DMB_RTS); /* turn off DTR & RTS but leave enabled */
		return;
		}
	/* else - set line parameters */
	lpar = (dmb_speeds[tp->t_cflag_ext&CBAUD]<<28) | 	/* ospeed */
		(dmb_speeds[tp->t_cflag&CBAUD]<<24);		/* ispeed */
	/*
	 * Berkeley-only dinosaurs
	 */
	if (tp->t_line != TERMIODISC) {
		if ((tp->t_cflag & CBAUD) == B134){
			lpar |= DMB_BITS6|DMB_PARITYENAB; 
			tp->t_cflag |= CS6|PARENB;
		}
		else if ((tp->t_cflag_ext & CBAUD) == B110){
			lpar |= DMB_TWOSTOPB;
			tp->t_cflag |= CSTOPB;
		}
	}
 	/*
 	 * System V termio functionality.
	 * Set device registers according to the specifications of the termio
	 * structure.
 	 */
 	if (tp->t_cflag & CREAD)
 		lpar |= DMB_RXENA;
	else
 		lpar &= ~DMB_RXENA;
 	if (tp->t_cflag & CSTOPB)
 		lpar |= DMB_TWOSTOPB;
 	else
 		lpar &= ~DMB_TWOSTOPB;
 	/* parity is enable */
 	if (tp->t_cflag & PARENB) {
 		if ((tp->t_cflag & PARODD) == 0)
 			/* set even */
 			lpar |= DMB_PARITYENAB|DMB_EVENPARITY;
 		else
 			/* else set odd */
 			lpar = (lpar | DMB_PARITYENAB)&~DMB_EVENPARITY;
 	}
 	/*
 	 * character size.
 	 * clear bits and check for 6,7,and 8, else its 5 bits.
 	 */
 	lpar &= ~DMB_BITS8;
 	switch(tp->t_cflag&CSIZE) {
 		case CS6:
 			lpar |= DMB_BITS6;
 			break;
 		case CS7:
 			lpar |= DMB_BITS7;
 			break;
 		case CS8:
 			lpar |= DMB_BITS8;
 			break;
 	}
	/*
	 * Outgoing Auto flow control.
	 * No auto flow control allowed if startc != ^q and startc !=
	 * ^s.  Most drivers do not allow this to be changed.
	 */
	if ((tp->t_cflag_ext & PAUTOFLOW) && (tp->t_cc[VSTOP] == CTRL('s')) && 
		(tp->t_cc[VSTART] == CTRL('q'))) {
		/*
		 * OAUTO doesn't work on the DHB.
 		 * Take this out if it ever does work.
		 */
		if (dmb_lines[unit >> LINEBITS] != DMB_16_LINES) 
			lpar |= DMB_OAUTOFLOW;
		else
                        tp->t_cflag_ext &= ~PAUTOFLOW;
	}
 	
#	ifdef DEBUG
	if (dmbdebug)
		mprintf("dmbparam: tp = %x, lpar = %x\n",tp, lpar);
#	endif DEBUG
	/*
	 * Set "Report Modem" bit to get interrupts on modem status changes.
	 * Enable receiver, and set Transmission interrupt delay so that
	 *   xmit interrupt does not actually happen until all characters
	 *   in current DMA have been transmitted.
	 * Make all lines modem lines, in case a line is
	 *   changed from local to remote (modem).
	 */
	lpar |= (DMB_REPORT | DMB_TXINTDELAY | DMB_DTR | DMB_RTS);
        /*
         * If outgoing auto flow control is enabled, the hardware will
         * control the transmit enable bit.
         */
        if ((tp->t_cflag_ext & PAUTOFLOW) == 0)
		addr->dmb_lstathigh |= DMB_TXENA;
	addr->dmb_lpr = lpar;
}


/*
 * DMB32 transmitter interrupt.
 * Restart each line which used to be active but has
 * terminated transmission since the last interrupt.
 */

dmbxint(dmb)
	int dmb;
{
	int unit = dmb << LINEBITS;
	struct tty *tp0 = &dmb_tty[unit];
	register struct tty *tp;
	register struct dmb_device *addr;
	register struct uba_device *ui;
	register long tbuf;
	u_short cntr;
	register temp_reg;
	int totaldelay;

	ui = dmbinfo[dmb];
	addr = (struct dmb_device *)ui->ui_addr;
	while ((tbuf = addr->dmb_tbuf) < 0)
		{			/* xmitter action is set if "< 0" */
		tbuf = tbuf & LINEMASK;
		tp = tp0 + tbuf;
		smp_lock(&tp->t_lk_tty,LK_RETRY);
		DMB_LOCK(dmb);
		/*
		 * Make sure another processor hasn't serviced the interrupt.
		 */
		if ((tbuf = addr->dmb_tbuf) >= 0){
			DMB_UNLOCK(dmb);
			smp_unlock(&tp->t_lk_tty);
			break;
		}
		if (tp != (tp0 + (tbuf & LINEMASK))) {
			DMB_UNLOCK(dmb);
			smp_unlock(&tp->t_lk_tty);
			continue;
		}
		addr->dmb_tbuf = 0; /* must write to clear xmitter act bit */
		smp_unlock(&lk_dmb[dmb]);
		/*
		 * Check Error byte and if any error bits set,
		 * decode the error.
		 */
		switch (tbuf & DMB_ERMASK) {
		case DMB_TXDMAERROR:
			printf("dmb%d: DMA Error. tbuf = 0x%x\n", dmb, tbuf);
			break;
		case DMB_MSGERR:
			printf("dmb%d: Message Error. tbuf = 0x%x\n", dmb, tbuf);
			break;
		case DMB_LASTCHERR:
			printf("dmb%d: Last character Incomplete. tbuf = 0x%x\n", dmb, tbuf);
			break;
		case DMB_BUFERR:
			printf("dmb%d: Buffer Error. tbuf = 0x%x\n", dmb, tbuf);
			break;
		case DMB_MODEMERR:
			printf("dmb%d: Modem Error. tbuf = 0x%x\n", dmb, tbuf);
			break;
		case DMB_INTERNALERR:
			printf("dmb%d: Internal Error. tbuf = 0x%x\n", dmb, tbuf);
			break;
		}
#		ifdef DEBUG
		printd10("dmbxint: unit=0x%x, line=%d, tp=0x%x, c_cc=%d\n",
				unit, tbuf, tp, tp->t_outq.c_cc);
#		endif
		tbuf = tbuf & LINEMASK;
		tp->t_state &= ~(TS_BUSY|TS_OABORT);
		totaldelay = 0;
		DMB_LOCK(dmb);
		addr->dmb_acsr = DMB_IE | tbuf;
		while ((addr->dmb_startdma&DMB_TXDMASTART)&&(totaldelay <= 100)){
			totaldelay++;
			DELAY(10000);
		}
		if (addr->dmb_startdma&DMB_TXDMASTART) {
		    printf("dmbxint: Resetting DMA START bit on line %d\n",tbuf);
		    addr->dmb_startdma &= ~DMB_TXDMASTART;
		}
		if (tp->t_state&TS_FLUSH)
			tp->t_state &= ~TS_FLUSH;	/* was a non ^S stop */
		else	{
			/*
			 * Determine number of characters transmitted so far
			 * and flush these from the tty output queue.
			 * (unit is the dmb unit number so add in the line #.)
			 * unit is already left shifted by the number of lines
			 */

			/*
	The original code below is replaced by functionally equivalent code.
	We do this to work around a bug in the C optimizer which optimizes
	the original code which was an ASHL instruction followed by BICL
	to be an EXTZV instruction.  It is not permitted to have an EXTZV
	instruction read from I/O space.  By using a register variable
	as is done below, the optimization is avoided.
	This could be reconsidered once the C optimizer is corrected.
			 */
#ifdef ORIGINAL_CODE
			cntr = dmb_numchars[unit+tbuf] -
				((addr->dmb_tbuffct >> DMB_TXCHARCT) & 0xffff);
#else
			temp_reg = (addr->dmb_tbuffct >> DMB_TXCHARCT);
			temp_reg &= 0xffff;
			cntr = dmb_numchars[unit+tbuf] - temp_reg;
#endif
			ndflush(&tp->t_outq, (int)cntr);
			}
		if (tp->t_state & TS_NEED_PARAM) {
		    tp->t_state &= ~TS_NEED_PARAM;
		    /*
		     * (unit is the dmb unit number so add in the line #.)
		     * unit is already left shifted by the number of lines
		     */
		    dmbparam(unit+tbuf);
		}
		DMB_UNLOCK(dmb);
		if (tp->t_line)
			(*linesw[tp->t_line].l_start)(tp);
		else
			dmbstart(tp);
		smp_unlock(&tp->t_lk_tty);
		}
}


/*
 * Start (restart) transmission on the given DMB32 line.
 */

dmbstart(tp)
	register struct tty *tp;
{
	register struct dmb_device *addr;
	register int unit, nch;
	register int totaldelay;
	register int dmb;
	int post_wakeup = 0;

	TTY_ASSERT(tp);
	unit = minor(tp->t_dev);
	dmb = unit >> LINEBITS;
	addr = (struct dmb_device *)tp->t_addr;

	/*
	 * If it's currently active, or delaying, no need to do anything.
	 * Also do not transmit if not CTS.
	 */
	if ((tp->t_state&(TS_TIMEOUT|TS_BUSY|TS_TTSTOP)) ||
	   ((tp->t_state & TS_CARR_ON) && (dmbmodem[unit]&MODEM_CTS)==0))
		return;
	/*
	 * If there are sleepers, and output has drained below low
	 * water mark, wake up the sleepers.
	 */
	if (tp->t_outq.c_cc<=TTLOWAT(tp))
		{
		if (tp->t_state&TS_ASLEEP)
			{
			tp->t_state &= ~TS_ASLEEP;
			post_wakeup = WAKEUP_OUTQ;
			}
		if (tp->t_wsel)
			{		/* for select system call */
			post_wakeup |= WAKEUP_SELECT;
			tp->t_wsel = 0;
			tp->t_state &= ~TS_WCOLL;
			}
		}
	/*
	 * Output had been aborted by a stop character.  Resume output by
 	 * turning on transmit enable.
	 */
	if ((tp->t_state & TS_OABORT) && ((tp->t_cflag_ext & PAUTOFLOW) == 0)) {
			DMB_LOCK(dmb);
			addr->dmb_acsr = DMB_IE | (unit & LINEMASK);
			addr->dmb_lstathigh |= DMB_TXENA;
			DMB_UNLOCK(dmb);
			tp->t_state |= TS_BUSY;
			tp->t_state &= ~TS_OABORT;
			goto dmbendstart;
	}
	/*
	 * Now restart transmission unless the output queue is empty.
	 */
	if (tp->t_outq.c_cc == 0)
		goto dmbendstart;
	if ((tp->t_lflag_ext & PRAW) || (tp->t_oflag_ext & PLITOUT) || 
	    ((tp->t_oflag & OPOST) == 0))
		nch = ndqb(&tp->t_outq, 0);	/* # of consecutive chars */
	else	{
		nch = ndqb(&tp->t_outq, DELAY_FLAG);
		/*
		 * If first thing on queue is a delay process it.
		 */
		if (nch == 0)
			{
			nch = getc(&tp->t_outq);
			timeout(ttrstrt, (caddr_t)tp, (nch&0x7f)+6);
			tp->t_state |= TS_TIMEOUT;
			goto dmbendstart;
			}
		}
	/*
	 * If characters to transmit, restart transmission.
	 */
	if (nch)
		{
		DMB_LOCK(dmb);
		addr->dmb_acsr = DMB_IE | (unit & LINEMASK);	/* line select */
		/*
		 * Wait for dma start to clear to a maximum of 1 second to
		 * prevent a system hang on hardware failure.  After the bit
		 * has stuck once, do not repeat this check.
		 */
		totaldelay = 0;
		while ((addr->dmb_startdma&DMB_TXDMASTART)&&(totaldelay <= 100)){
		  	if ((dmb_softc[unit/16].sc_flags[unit&LINEMASK]) == 0){
	    			totaldelay++;
	    			DELAY(10000);
			}
			else{
				totaldelay = 90000;
			}
		}
		if ((addr->dmb_startdma & DMB_TXDMASTART) && 
		  ((dmb_softc[unit/16].sc_flags[unit&LINEMASK]) == 0)) {
				printf("dmb%d,line%d DMB HARDWARE ERROR.  TX.DMA.START failed\n",unit/16,unit&LINEMASK); 
				/*
				 * Prevent further checks on this line by
				 * setting state flag to be nonzero.
				 */
		  		dmb_softc[unit/16].sc_flags[unit&LINEMASK]++;
				DMB_UNLOCK(dmb);
				goto dmbendstart;
		}
		if (addr->dmb_abort & DMB_TXOUTABORT) {
			addr->dmb_abort &= ~(DMB_TXOUTABORT);
		}
                /*
                 * If outgoing auto flow control is enabled, the hardware will
                 * control the transmit enable bit.
                 */
		if ((tp->t_cflag_ext & PAUTOFLOW) == 0)
			addr->dmb_lstathigh |= DMB_TXENA;
		/*
		 * TODO: if (nch == 1) { use programmed (preempt) transfer }
		 * 	 This will save on DMA setup overhead.
		 */
		/* hand the board the physical address of the buffer */
		addr->dmb_tbuffadd = (long)svtophy(tp->t_outq.c_cf);
		addr->dmb_tbuffct = (nch << DMB_TXCHARCT);
		/*
		 * Save the number of characters that we are DMA'ing,
		 * for use in the transmit interrupt routine.
		 * (unit is the whole minor device, unit # & line #.)
		 */
		dmb_numchars[unit] = nch;
		addr->dmb_startdma |= (DMB_TXDMASTART | DMB_TXDMAPHYS);
		tp->t_state |= TS_BUSY;
		DMB_UNLOCK(dmb);
		}
		/*
		 * Unfortunately these wakeups will be done while the tty lock
		 * is still held.  The penalty is not so bad because most of
		 * the callers of this routine will release locks very shortly
		 * after calling this routine. 
		 */
dmbendstart:	if (post_wakeup & WAKEUP_OUTQ) 
			wakeup((caddr_t)&tp->t_outq);
		if (post_wakeup & WAKEUP_SELECT)
			selwakeup(tp->t_wsel, tp->t_state & TS_WCOLL);
}


/*
 * Stop output on a line, e.g. for ^S/^Q or output flush.
 */
/*ARGSUSED*/
dmbstop(tp, flag)
	register struct tty *tp;
{
	register struct dmb_device *addr;
	register int unit, s, dmb;

	TTY_ASSERT(tp);

	dmb = (minor(tp->t_dev)) >> LINEBITS;
	addr = (struct dmb_device *)tp->t_addr;
	/*
	 * Block input/output interrupts while messing with state.
	 * (tty lock must be taken out prior to calling this routine)
	 */
	if (tp->t_state & TS_BUSY)
		{
		/*
		 * Device is transmitting; stop output.
		 * We can continue later by examining the character count.
		 */
		unit = minor(tp->t_dev) & LINEMASK;
		DMB_LOCK(dmb);
		addr->dmb_acsr = unit | DMB_IE;
		if ((tp->t_state&TS_TTSTOP)==0) {
		    if (addr->dmb_startdma & DMB_TXDMASTART) {
			addr->dmb_abort |= DMB_TXOUTABORT;  /* abort DMA transmission */
			tp->t_state |= TS_FLUSH;	/* NOT a ctl-S */
		    }
		}
		/*
		 * Suspend output by turning off transmit enable.  Output will
		 * resume when a start char is received.
		 */
		else if ((tp->t_cflag_ext & PAUTOFLOW) == 0) 
			{
			addr->dmb_lstathigh &= ~DMB_TXENA;
			tp->t_state &= ~TS_BUSY;
			tp->t_state |= TS_OABORT;
			}
		DMB_UNLOCK(dmb);
		}
}


/*
 * Reset state of driver if BI reset was necessary.
 * Reset the csr and lpr registers on open lines, and
 * restart transmitters.
 */
dmbreset(binum)
	int binum;				/* bi adapter that reset */
{
	register int dmb, unit;
	register struct tty *tp;
	register struct uba_device *ui;
	register struct dmb_device *addr;
	register int i;

	for (dmb = 0; dmb < nNDMB; dmb++)
		{
		ui = dmbinfo[dmb];
		if (ui == 0 || ui->ui_alive == 0 || ui->ui_adpt != binum) 
			continue;
		printf(" dmb%d", dmb);
		addr = (struct dmb_device *)ui->ui_addr;
		/*
		 * Enable external vector
		 */
		DMB_LOCK(dmb);
		addr->dmb_biic.biic_int_ctrl |= BIIC_EXVEC;
		addr->dmb_acsr2 = (dmb_timeout << 16);	/* 10 ms */
		DMB_UNLOCK(dmb);
		unit = dmb * NUMLINES;
		for (i = 0; i < NUMLINES; i++)
			{
			tp = &dmb_tty[unit];
			smp_lock(&tp->t_lk_tty,LK_RETRY);
			if (tp->t_state & (TS_ISOPEN|TS_WOPEN))
				{
				DMB_LOCK(dmb);
				dmbparam(unit); 	/* resets lpr reg */
				DMB_UNLOCK(dmb);
				tp->t_state &= ~TS_BUSY;
				dmbstart(tp);
				}
			smp_unlock(&tp->t_lk_tty);
			unit++; 		/* increment the line number */
			}
		}
}


/*
 * dmblopen -- open the line printer port on a dmb32
 */
/*ARGSUSED*/
dmblopen(dev,flag)
	dev_t dev;
	int flag;
{
	register int dmb;
	register struct dmbl_softc *sc;
	register struct uba_device *ui;
	register struct dmb_device *addr;
	register int s;

	dmb = minor(dev) & LINEMASK ;
	if (dmb >= nNDMB)
		return (ENXIO);
	ui = dmbinfo[dmb];
	sc = &dmbl_softc[dmb];
	if ((sc->dmbl_state & OPEN) || (ui == 0) || (ui->ui_alive == 0) || (dmb_lines[ui->ui_unit] != DMB_8_LINES))
		{
		return(ENXIO);
		}
	addr = (struct dmb_device *)ui->ui_addr;
#	ifdef DEBUG
	if ((addr->dmb_pcsrhigh & DMB_PROFFLINE))
		{
		printd("dmb%d: line printer offline/jammed\n", dmb);
		return(EIO);
		}
#	endif
	if ((addr->dmb_pcsrhigh & DMB_PRCONNECT) == 0)
		{
		printf("dmb%d: line printer disconnected\n", dmb);
		return(EIO);
		}
	/*
	 * disable interrupts
	 */
	s = spltty();
	DMB_LOCK(dmb);
	addr->dmb_pcsrlow = 0;
	sc->dmbl_state |= OPEN;
	DMB_UNLOCK(dmb);
	splx(s);
	return(0);
}

/*ARGSUSED*/
dmblclose(dev,flag)
	dev_t dev;
	int flag;
{
	register int dmb;
	register struct dmbl_softc *sc;
	register int s;

	dmb = minor(dev) & LINEMASK;
	sc = &dmbl_softc[dmb];
	s = spltty();
	DMB_LOCK(dmb);
	/* If we're not waiting for something to complete, then
           go ahead and print the form feed.  R. Craig Peterson */
	if (!(sc->dmbl_state & ASLP))
		dmblout(dev,"\f",1);
	sc->dmbl_state = 0;
	/*
	 * disable interrupts
	 */
	((struct dmb_device *)(dmbinfo[dmb]->ui_addr))->dmb_pcsrlow = 0;
	DMB_UNLOCK(dmb);
	splx(s);
	return(0);
}

dmblwrite(dev,uio)
	dev_t dev;
	struct uio *uio;
{
	register unsigned int n;
	register int error;
	register struct dmbl_softc *sc;
	register int s, dmb;

	dmb = minor(dev) & LINEMASK;

	sc = &dmbl_softc[dmb];
	if (sc->dmbl_state & ERROR)
		return(EIO);
	while(n = min(DMBL_BUFSIZE,(unsigned)uio->uio_resid))
		{
		if (error = uiomove(&sc->dmbl_buf[0],(int)n,UIO_WRITE,uio))
			{
			printf("uio move error\n");
			return(error);
			}
		s = spltty();
		DMB_LOCK(dmb);
		error = dmblout(dev,&sc->dmbl_buf[0],n);
		DMB_UNLOCK(dmb);
		splx(s);
		if (error)
			return(error);
		}
	return(0);
}


/*
 * dmblout -- start io operation to dmb line printer
 *		cp is addr of buf of n chars to be sent.
 *
 *	-- dmb will NOT be put in formatted output mode, this will
 *		be selectable from an ioctl if the
 *		need ever arises.
 */
dmblout(dev,cp,n)
	dev_t dev;
	char *cp;
	int n;
{
	register struct dmbl_softc *sc;
	register int dmb;
	register struct uba_device *ui;
	register struct dmb_device *addr;

	dmb = minor(dev) & LINEMASK;
	sc = &dmbl_softc[dmb];
	if (sc->dmbl_state & ERROR)
		return(EIO);
	ui = dmbinfo[dmb];
	addr = (struct dmb_device *)ui->ui_addr;
	addr->dmb_pbufad = (long) svtophy(cp);
	addr->dmb_pbufct = (n << DMB_PRCHARCT);
	addr->dmb_psiz = (sc->dmbl_lines << DMB_PRPAGE) | (sc->dmbl_cols);
	sc->dmbl_state |= ASLP;
	addr->dmb_pcsrlow = DMB_PRIE;
	addr->dmb_pctrl |= (DMB_PRSTART | DMB_PRPHYS);
	while(sc->dmbl_state & ASLP)
		{
		sleep_unlock((caddr_t)&sc->dmbl_buf[0],(PZERO+8),&lk_dmb[dmb]);
		spltty();
		DMB_LOCK(dmb);
		while(sc->dmbl_state&ERROR)
			{
			timeout(dmblint,dmb,10*hz);
			sleep_unlock((caddr_t)&sc->dmbl_state,(PZERO+8),
				&lk_dmb[dmb]);
			spltty();
			DMB_LOCK(dmb);
			}
		/*if (sc->dmbl_state&ERROR) return (EIO);*/
		}
	return(0);
}

/*
 * dmblint -- handle an interrupt from the line printer part of the dmb32
 */

dmblint(dmb)
	register int dmb;
{
	register struct uba_device *ui;
	register struct dmbl_softc *sc;
	register struct dmb_device *addr;
	register int s;
	int post_wakeup = 0;

	ui = dmbinfo[dmb];
	sc = &dmbl_softc[dmb];
	addr = (struct dmb_device *)ui->ui_addr;
	s = spltty();
	DMB_LOCK(dmb);
	addr->dmb_pcsrlow = 0;		/* disable interrupts */

	if (sc->dmbl_state & ERROR)
		{
#		ifdef DEBUG
		printd("dmb%d: intr while in error state\n", dmb);
#		endif
		if ((addr->dmb_pcsrhigh & DMB_PROFFLINE) == 0)
			sc->dmbl_state &= ~ERROR;
		post_wakeup = WAKEUP_STATE;
		goto dmbendlint;
		}
	if (addr->dmb_pctrl & DMB_PRERROR)
		printf("dmb%d: Printer DMA Error %x\n", dmb,
			((addr->dmb_pctrl & DMB_PRERROR)>>16));
	if (addr->dmb_pcsrhigh & DMB_PROFFLINE)
		{
#		ifdef DEBUG
		printd("dmb%d: printer offline\n", dmb);
#		endif
		sc->dmbl_state |= ERROR;
		}
#	ifdef DEBUG
	if (addr->dmb_pctrl & DMB_PRSTART)
		printd("DMB%d printer intr w/ DMA START still set\n", dmb);
	else
		{
		printd("bytes= %d\n", addr->dmb_pcar & DMB_PRCHAR);
		printd("lines= %d\n",addr->dmb_pcar & DMB_PRLINE);
		}
#	endif
	sc->dmbl_state &= ~ASLP;
	post_wakeup = WAKEUP_BUF;
dmbendlint:
	DMB_UNLOCK(dmb);
	if (post_wakeup) {
		if (post_wakeup & WAKEUP_STATE)
			wakeup(&sc->dmbl_state);
		else
			wakeup(&sc->dmbl_buf[0]);
	}
	splx(s);
}

/* stub for synchronous device interrupt routine which is not supported */

dmbsint() { printf("dmbsint\n"); }


/*
 * Called from a timeout routine.
 */
dmb_cd_drop(tp)
	register struct tty *tp;
{
	register struct dmb_device *addr;
	register int unit, dmb;
	int post_wakeup;

	unit = minor(tp->t_dev);
	dmb = unit >> LINEBITS;
	addr = (struct dmb_device *)tp->t_addr;
	DMB_LOCK(dmb);
	addr->dmb_acsr = DMB_IE | (unit&LINEMASK);
	if ((tp->t_state&TS_CARR_ON) &&
	   ((addr->dmb_lstatlow & DMB_DCD) == 0))
		{
#		ifdef DEBUG
		printd("dmb_cd:  no CD, tp=0x%x\n", tp);
#		endif
		dmb_tty_drop(tp,&post_wakeup);
		goto dmb_cd_end;
		}
	dmbmodem[unit] |= MODEM_CD;
#	ifdef DEBUG
	printd("dmb_cd:  CD is up, tp=0x%x\n", tp);
#	endif
dmb_cd_end:
	DMB_UNLOCK(dmb);
	if (post_wakeup)
		wakeup((caddr_t)&tp->t_rawq);
}

/*
 * dmb_dsr_check_timeout
 *
 * Re-acquire locks on the tty line and multiplexer board before checking
 * to see if the modem signals are present.  If connection establishment
 * fails wakeup on the raw queue to allow another try.
 */
dmb_dsr_check_timeout(tp) 
	register struct tty *tp;
{
	register int s,dmb;
	int post_wakeup = 0;

	dmb = minor(tp->t_dev)>>LINEBITS;

	DMB_TTY_LOCK(tp,s);
	DMB_LOCK(dmb);
	dmb_dsr_check(tp,&post_wakeup);
	DMB_UNLOCK(dmb);
	DMB_TTY_UNLOCK(tp,s);
	if (post_wakeup)
		wakeup((caddr_t)&tp->t_rawq);
}

/*
 * dmb_dsr_check
 *
 * When initiating a modem connection you must first leave the line idle for
 * 500ms.  If all the appropriate modem leads do not rise within 30 seconds
 * then throw in the towel.
 */
dmb_dsr_check(tp,post_wakeup)
	register struct tty *tp;
	register int *post_wakeup;
{
	int unit;
	register struct dmb_device *addr;

	unit = minor(tp->t_dev);
	addr = (struct dmb_device *)tp->t_addr;
	/*
	 * The line must remain dormant for the first 500ms.  Now that the
	 * time period has expired, look to see if the appropriate modem
	 * signals have been asserted to establish the connection.
   	 */
	if (dmbmodem[unit] & MODEM_DSR_START)
		{
		dmbmodem[unit] &= ~MODEM_DSR_START;
		addr->dmb_acsr = DMB_IE | (unit&LINEMASK);
	   	/* 
	    	 * If dmbdsr is set look for DSR|CTS|CD, otherwise look 
	    	 * for CD|CTS only.
	    	 */
		if (dmbdsr) {
			if ((addr->dmb_lstatlow & DMB_XMIT) == DMB_XMIT)
				dmb_start_tty(tp,post_wakeup);
		}
		else {
			if ((addr->dmb_lstatlow & DMB_NODSR) == DMB_NODSR) 
				dmb_start_tty(tp,post_wakeup);
		}
		return;
		}
	/*
	 * CD has not come up within the 30 second interval.  Give up on
	 * establishing this connection.
	 */
	if ((tp->t_state&TS_CARR_ON)==0)
		{
		dmb_tty_drop(tp,post_wakeup);
#		ifdef DEBUG
		printd("dmb_dsr: no carrier, tp=0x%x\n", tp);
#		endif
		}
#	ifdef DEBUG
	else
		printd("dmb_dsr:  carrier is up, tp=0x%x\n", tp);
#	endif
}

/*
 *  cd_down return 1 if carrier has been down for at least 2 secs.
 */
dmb_cd_down(tp)
	register struct tty *tp;
{
	register int msecs;
	register int unit;

	unit = minor(tp->t_dev);
	msecs = 1000000 * (time.tv_sec - dmbtimestamp[unit].tv_sec) +
		(time.tv_usec - dmbtimestamp[unit].tv_usec);
	if (msecs > 2000000)
		return(1);
	else
		return(0);
}

/*
 * dmb_tty_drop
 *
 * Close down a modem line by deasserting DTR.  Wakeup on rawq to allert
 * any processes which may be waiting for an open to succeed which has
 * timed out.
 */
dmb_tty_drop(tp,post_wakeup)
	register struct tty *tp;
	register int *post_wakeup;
{
	register struct dmb_device *addr;
	register int unit;

	if (tp->t_flags&NOHANG)
		return;
	unit = minor(tp->t_dev);
	dmbmodem[unit] = MODEM_BADCALL;
	tp->t_state &= ~(TS_CARR_ON|TS_TTSTOP|TS_BUSY|TS_ISUSP);
	*post_wakeup = WAKEUP_RAWQ;
	gsignal(tp->t_pgrp, SIGHUP);
	gsignal(tp->t_pgrp, SIGCONT);
	addr = (struct dmb_device *)tp->t_addr;
	addr->dmb_acsr = DMB_IE | ((minor(tp->t_dev)) & LINEMASK);
	addr->dmb_lpr &= ~(DMB_DTR | DMB_RTS); /* turn off DTR & RTS */
}

/*
 * dmb_start_tty
 *
 * Mark line as usable after all modem control leads have been asserted.
 * Wakeup on rawq to allert processes which have been waiting for the open
 * to succeed.
 */
dmb_start_tty(tp,post_wakeup)
	register struct tty *tp;
	register int *post_wakeup;
{
	register int unit = minor(tp->t_dev);

	unit = minor(tp->t_dev);
	tp->t_state &= ~TS_ONDELAY;
	tp->t_state |= TS_CARR_ON;
	if (dmbmodem[unit]&MODEM_DSR)
		untimeout(dmb_dsr_check_timeout, tp);
	dmbmodem[unit] |= MODEM_CD|MODEM_CTS|MODEM_DSR;
	dmbtimestamp[unit] = dmbzerotime;
	*post_wakeup = WAKEUP_RAWQ;
}

dmbbaudrate(speed)
register int speed;
{
    if (dmb_valid_speeds & (1 << speed))
	return(1);
    else
	return(0);
}

#endif
