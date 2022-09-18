
#ifndef lint
static char *sccsid = "@(#)dmf.c	4.2	(ULTRIX)	8/13/90";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1985-88 by			*
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
 * dmf.c  6.1	7/29/83
 *
 * Modification history
 *
 * DMF32 terminal driver
 *
 *  5-May-85 - Larry Cohen
 *
 *	Derived from 4.2BSD labeled: dmf.c	6.1	83/07/29.
 *	Add dma support, switch between dma and silo - derived from UCB.
 *
 * 16-Jan-86 - Larry Cohen
 *
 *	Add full DEC standard 52 support.
 *
 * 18-Mar-86 - jaw
 *
 *	br/cvec changed to NOT use registers.
 *
 * 14-Apr-86 - jaw
 *
 *	remove MAXNUBA references.....use NUBA only!
 *
 * 26-Apr-86 - ricky palmer
 *
 *	Added new DEVIOCGET ioctl request code. V2.0
 *
 * 11-Jul-86 - ricky palmer
 *
 *	Added adpt and nexus fields to DEVIOCGET code.
 *
 * 05-Aug-86 - Tim Burke
 *
 *	In dmfrint, record present time in timestamp in the event of
 *	a carrier drop.
 *
 * 25-Aug-86 - Tim Burke
 *
 *	Insure that line drops and restarts on false calls.  Also change
 *	state to ~TS_CARR_ON to terminate all processes on line.
 *
 * 26-Aug-86 - rsp (Ricky Palmer)
 *
 *	Cleaned up devioctl code to (1) zero out devget structure
 *	upon entry and (2) use strlen instead of fixed storage
 *	for bcopy's.
 *
 *  4-Dec-86 - Tim Burke
 *
 *	Bug fix to modem control.  In dmf_tty_drop routine, clear the stopped
 *	state to prevent modem lines from hanging on close.  
 *
 *  4-Dec-86 - Tim Burke
 *
 *	Bug fix to dmfreset code.  Saves software programmable interrupt
 *	vector as established in the probe routine so that it may be restored
 *	in the reset routine.
 *
 * 15-Dec-86 - Tim Burke
 *
 *	When a break occurs, (interpreted as a framing error) set the variable
 *	c to be the interrupt character.  There was a problem here due to the
 *	fact that sign extension is done which causes unwanted side affects. To
 *	solve this bug, the character is stripped to 8 bits.
 *
 *	Modified probe routine to wait for self test to complete.
 *
 *	Fix DEVIOGET to return propper modem status information.
 *
 *  9-Jan-87 - Tim Burke
 *
 *	Bug fix to TIOCMODEM to clear modem flags if signals are not up.
 *
 * 28-Jan-87 - Tim Burke
 *
 *	Added the capability to ignore the "DSR" modem signal.  This is being
 *	done to allow modems that do not follow DEC Standard 52 to still 
 *	function as they would have prior to the addition of DECSTD52 code
 *	into the drivers.  If the driver is setup to ignore "DSR" then it will
 *	not be following DECSTD52.  To follow DECSTD52 set dmfdsr to "1", to
 *	ignore "DSR" set dmfdsr to be "0";
 *
 *  6-Feb-87 - Tim Burke
 *
 *	Removed printf of master reset failure in probe routine, as it may be
 *	incorrectly appearing. (Particularly in the DMF & DMZ drivers)
 *
 *  3-Mar-87 - Tim Burke
 *
 *	Added full TERMIO functionality to terminal subsystem.
 *
 *  2-Sept-87 - Tim Burke
 *
 *	Added support for hardware auto flow control on the outgoing side.  This
 *	will provide quick response to start/stop characters which will reduce
 *	buffer overflow on the receiving device.
 *
 *  8-Sep-87 - Ricky Palmer (rsp)
 *
 *      Defined LINEMASK for this driver and replaced all hardcoded references
 *      to "07" to be LINEMASK. Also fixed DEVIOCGET code to use LINEMASK
 *      as well.
 *
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
 * 29-Jan-88 - Tim Burke
 *	Changed most softCAR[unit&LINEMASK] references to use the CLOCAL
 *	bit of the control flags to determine if the line is set to be a
 *	modem line or a direct connect.  The setting of softCAR[] remains
 *	to allow one to set default settings for device open.
 *
 * 16-May-88 - Tim Burke
 *
 * 	Call param routine for setting of local mode word because it can 
 * 	affect bit size and parity.
 * 
 * 14-Jul-88 - Tim Burke
 *
 * 	The probe routine has been modified to accept only one parameter.
 *	The controller number is not passed to this routine.
 *
 * 5-Aug-88 - Tim Burke
 *
 *	Return the 2 character sequence 0377, 0377 upon receipt of a valid
 *	0377 character only when PARMRK is set under the termio line disc.
 *
 * 18-Aug-88 - Tim Burke
 *
 *	If PARMRK is set and a BREAK occurs, return '\0377','\0','\0'.
 *
 * 02-Sep-88 - Tim Burke
 *
 *	Return EINVAL instead of ENOTTY for POSIX programs on invalid ioctls.
 *
 * 25-Jan-89 - Randall Brown
 *
 *	Changed cd_drop to look at LNOHANG.  Changed close routine to look
 *	at HUPCL.
 *
 * 12-Jun-89 - dws
 *
 *	Added trusted path support.
 *
 * 21-Jul-89 - Randall Brown
 *
 *	Moved default open and default close code to tty.c and call it
 *	using tty_def_open() and tty_def_close().  In the close routine,
 *	don't clear the baudrate so that subsequent opens will keep the
 *	present attributes.  This only applies to a Berkeley environment.
 *
 * 15-Aug-89 - Randall Brown
 *
 *	Changed all references of TCSADFLUSH to TCSAFLUSH 
 *
 * 31-Oct-89 - Randall Brown
 *
 *	Added the support to allow the device to determine if baudrate is 
 *	supported before it is set in the tty data structures.
 *
 * July 5, 1990        Kuo-Hsiung Hsieh
 *	Added a delay before we set a break condition on a transmission
 *	line.  On dmf async drive, a break might be insert before a 
 *	character if there happened to be a character in the transmission
 *	buffer.	 This will cause some problems in interpreting actual
 *	flow of data in this io stream.  The fix is similar to DC driver,
 *	except there is no data corruption problem involved here.
 */

#include "dmf.h"
#if NDMF > 0  || defined(BINARY)

#include "../data/dmf_data.c"

/*
 * The line printer port is indicated by a minor device code of 128+x.
 * The flags field of the config file is interpreted as:
 *
 * bits 	meaning
 * ---- 	-------
 * 0-7		soft carrier bits for ttys part of dmf32
 * 8-15 	number of cols/line on the line printer
 *			if 0, 132 will be used.
 * 16-23	number of lines/page on the line printer
 *			if 0, 66 will be used.
 *
 */
/*
 * Definition of the driver for the auto-configuration program.
 */
int	dmfprobe(), dmfattach(), dmfrint(), dmfxint(), dmfbaudrate();
int	dmf_cd_drop(), dmf_dsr_check(), dmf_cd_down(), dmf_tty_drop();
void	dmfsetbreak();
struct	timeval dmfzerotime = {0,0};
int	dmfcdtime = 2;
int	dmflint();
struct	uba_device *dmfinfo[NDMF];
u_short dmfstd[] = { 0 };
struct	uba_driver dmfdriver =
	{ dmfprobe, 0, dmfattach, 0, dmfstd, "dmf", dmfinfo };

int 	dmfdebug;
int	dmf_timeout = 10;		/* silo timeout, in ms */
int	dmf_mindma = 4; 		/* don't dma below this point */

/*
 * Local variables for the driver
 */
char	dmf_speeds[] =
	{ 0, 0, 1, 2, 3, 4, 0, 5, 6, 7, 010, 012, 014, 016, 017, 0 };

/* minumum delay value for setting a break condition.  If we set
 * a break condition without delaying this minimum interval, we
 * might insert a break in the middle of a character string.  This
 * could cause side effect in processing io buffer.
 * The delay values are calculated based on the following equation;
 * 12 (bits/char) * 64 (hz) / baudrate + 2 (safety factor).
 * B50 is an exception. It required - 32 (1/2 hz). This  is almost 
 * equivalent to two character time.   
 */
u_char 	dmf_delay[] =
	{0,32,13,9,8,7,0,5,4,3,3,3,2,2,2,0};

short	dmf_valid_speeds = 0x7fbf; /* 0,1,1,1, 1,1,1,1, 1,0,1,1, 1,1,1,1 */

#define ASLP 1		/* waiting for interrupt from dmf */
#define OPEN 2		/* line printer is open */
#define ERROR 4 	/* error while printing, driver
			 refuses to do anything till closed */

int	dmfstart(), ttrstrt();

#ifndef MODEM_CD
#define MODEM_CD   0x01
#define MODEM_DSR  0x02
#define MODEM_CTS  0x04
#define MODEM_DSR_START  0x08
#endif

#define LINEMASK 0x07   /* mask of higher bits of csr to get a line # */

/*
 * The clist space is mapped by the driver onto each UNIBUS.
 * The UBACVT macro converts a clist space address for unibus uban
 * into an i/o space address for the DMA routine.
 */
#define UBACVT(x, uban) 	(cbase[uban] + ((x)-(char *)cfree))


/*
 * Routine for configuration to set dmf interrupt.
 */
/*ARGSUSED*/
dmfprobe(reg)
	caddr_t reg;
{
	register struct dmfdevice *dmfaddr = (struct dmfdevice *)reg;
	register int totaldelay;

#ifdef lint
	dmfxint(0); dmfrint(0);
	dmfsrint(); dmfsxint(); dmfdaint(); dmfdbint(); dmflint();
#endif
	if(dmfdebug)
		printf("dmfprobe\n");
	/*
	 * If a self test is not being done, start one up.  Wait for the
	 * self-test (to a max of 4 sec.) to complete before interrupting.
	 */

	if ((dmfaddr->dmfcsr & DMF_CLR) == 0)
	    dmfaddr->dmfcsr |= DMF_CLR;
	totaldelay = 0;
	while ( (dmfaddr->dmfcsr & DMF_CLR) && ( totaldelay <= 70) ){
	    totaldelay++;
	    DELAY(50000);
	}
	/*
	 * This message may be incorrectly printed - particularly in the
	 * DMF & DMZ drivers.
	if (dmfaddr->dmfcsr & DMF_CLR)
	    printf("Warning: DMF device failed to exit self-test\n");
	 */

	br = 0x15;
	cvec = (uba_hd[numuba].uh_lastiv -= 4*8);
	dmfaddr->dmfccsr0 = (cvec >> 2);
	dmfaddr->dmfl[0] = DMFL_RESET;
	/* NEED TO SAVE IT SOMEWHERE FOR OTHER DEVICES */
	return (sizeof (struct dmfdevice));
}

/*
 * Routine called to attach a dmf.
 */
dmfattach(ui)
	struct uba_device *ui;
{
	register int cols = (ui->ui_flags>>8) & 0xff;
	register int lines = (ui->ui_flags>>16) & 0xff;

	dmfsoftCAR[ui->ui_unit] = ui->ui_flags & 0xff;
	dmfdefaultCAR[ui->ui_unit] = ui->ui_flags & 0xff;
	dmfl_softc[ui->ui_unit].dmfl_cols = cols==0?DMFL_DEFCOLS:cols;
	dmfl_softc[ui->ui_unit].dmfl_lines = lines==0?DMFL_DEFLINES:lines;
}


/*
 * Open a DMF32 line, mapping the clist onto the uba if this
 * is the first dmf on this uba.  Turn on this dmf if this is
 * the first use of it.
 */
/*ARGSUSED*/
dmfopen(dev, flag)
	dev_t dev;
{
	register struct tty *tp;
	register int unit, dmf;
	register struct dmfdevice *addr;
	register struct uba_device *ui;
	int s;
	int inuse;  /*hold state of inuse bit while blocked waiting for carr*/

	unit = minor(dev);
	if(unit & 0200)
		return(dmflopen(dev,flag));
	dmf = unit >> 3;
	if (unit >= ndmf || (ui = dmfinfo[dmf])== 0 || ui->ui_alive == 0)
		return (ENXIO);
	tp = &dmf_tty[unit];
	if (tp->t_state&TS_XCLUDE && u.u_uid!=0)
		return (EBUSY);
	while (tp->t_state&TS_CLOSING) /* let DTR stay down for awhile */
		sleep((caddr_t)&tp->t_rawq, TTIPRI);
	addr = (struct dmfdevice *)ui->ui_addr;
	tp->t_addr = (caddr_t)addr;
	tp->t_oproc = dmfstart;
	tp->t_baudrate = dmfbaudrate;
	tp->t_state |= TS_WOPEN;
	/*
	 * While setting up state for this uba and this dmf,
	 * block uba resets which can clear the state.
	 */
	s = spl5();
	while (tty_ubinfo[ui->ui_ubanum] == -1)
		/* need this lock because uballoc can sleep */
		sleep(&tty_ubinfo[ui->ui_ubanum], TTIPRI);
	if (tty_ubinfo[ui->ui_ubanum] == 0) {
		tty_ubinfo[ui->ui_ubanum] = -1;
		tty_ubinfo[ui->ui_ubanum] =
		    uballoc(ui->ui_ubanum, (caddr_t)cfree,
			nclist*sizeof(struct cblock), 0);
		wakeup(&tty_ubinfo[ui->ui_ubanum]);
	}
	cbase[ui->ui_ubanum] = tty_ubinfo[ui->ui_ubanum]&0x3ffff;
	addr->dmfcsr |= DMF_IE;
	splx(s);

	if ((tp->t_state&TS_ISOPEN) == 0) {
	    dmfmodem[unit] = MODEM_DSR_START; /* prevents spurious startups */
	    addr->dmfrsp = dmf_timeout;
	}
	tty_def_open(tp, dev, flag, (dmfsoftCAR[dmf]&(1<<(unit&LINEMASK))));
	dmfparam(unit);

	/*
	 * Wait for carrier, then process line discipline specific open.
	 */

	s = spl5();
	dmfmctl(dev, DMF_ON, DMSET);	/* set DTR */
	if (tp->t_cflag & CLOCAL) {
#ifdef DMFDEBUG
		if (dmfdebug)
			mprintf("dmfopen: local, unit=%d\n", unit);
#endif
		tp->t_state |= TS_CARR_ON;
		dmfmodem[unit] = MODEM_CTS|MODEM_CD|MODEM_DSR;
	}
	  else
		if ((flag & (O_NDELAY|O_NONBLOCK)) == 0) {
		    /*
		     * DSR should not come up until DTR is asserted
		     * normally.  However if TS_HUPCL is not set it is
		     * possible to get here with all modem signals
		     * already asserted.  Or we could be dealing with
		     * an enormously slow modem and it has not deasserted
		     * DSR yet.
		     */

		    addr->dmfcsr = DMF_IE | DMFIR_TBUF | (unit&LINEMASK);
		    /*
		     * If the DSR signal is being followed, wait at most
		     * 30 seconds for CD, and don't transmit in the first 
		     * 500ms.  Otherwise immediately look for CD|CTS.
		     */
		    if (dmfdsr) {
		    	if (addr->dmfrms&DMF_DSR) {
#ifdef DMFDEBUG
			    if (dmfdebug)
				mprintf("dmfopen: modem, unit=%d\n", unit);
#endif
			    dmfmodem[unit] |= (MODEM_DSR_START|MODEM_DSR);
			    tp->t_dev = dev; /* need it for timeouts */
			    timeout(dmf_dsr_check, tp, hz*30);
			    timeout(dmf_dsr_check, tp, hz/2);
		        }
		    }
		    else {
			    dmfmodem[unit] |= (MODEM_DSR_START|MODEM_DSR);
			    dmf_dsr_check(tp);
		    }
		}

	if (flag & (O_NDELAY|O_NONBLOCK))
		tp->t_state |= TS_ONDELAY;
	else
	  while ((tp->t_state & TS_CARR_ON) == 0) {
		tp->t_state |= TS_WOPEN;
		inuse = tp->t_state&TS_INUSE;
		sleep((caddr_t)&tp->t_rawq, TTIPRI);
		/*
		 * See if wakeup was due to a false call.
		 */
		if (dmfmodem[unit]&MODEM_BADCALL){
			splx(s);
			return(EWOULDBLOCK);
		}
		/* if we opened "block if in use"  and
		 *  the terminal was not inuse at that time
		 *  but is became "in use" while we were
		 *  waiting for carrier then return
		 */
		if ((flag & O_BLKINUSE) && (inuse==0) &&
			(tp->t_state&TS_INUSE)) {
				splx(s);
				return(EALREADY);
		}
	  }
	/*
	 * Set state bit to tell tty.c not to assign this line as the 
	 * controlling terminal for the process which opens this line.
	 */
	if ((flag & O_NOCTTY) && (u.u_procp->p_progenv == A_POSIX))
		tp->t_state |= TS_ONOCTTY;
	splx(s);
	return ((*linesw[tp->t_line].l_open)(dev, tp));
}

/*
 * Close a DMF32 line.
 */
/*ARGSUSED*/
dmfclose(dev, flag)
	dev_t dev;
	int flag;
{
	register struct tty *tp;
	register unit;
	register dmf, s;
	register struct dmfdevice *addr;

	unit = minor(dev);
	if(unit & 0200)
		return(dmflclose(dev,flag));
	dmf = unit >> 3;

	tp = &dmf_tty[unit];
	if (tp->t_line)
		(*linesw[tp->t_line].l_close)(tp);
	(void) dmfmctl(unit, DMF_BRK, DMBIC);
	if ((tp->t_cflag&HUPCL) || (tp->t_state&TS_ISOPEN)==0) {
		(void) dmfmctl(unit, DMF_OFF, DMSET);
		tp->t_state &= ~TS_CARR_ON; /* prevents recv intr. timeouts */

		if ((tp->t_cflag & CLOCAL) == 0) {
			/*
			 * Raise spl to insure that no timeouts expire before
			 * the sleep is posted (in the event of interrupt).
			 */
			s = spl5();
			/*drop DTR for at least a half sec. if modem line*/
			tp->t_state |= TS_CLOSING;

			/* wait for DSR to drop */
			addr = (struct dmfdevice *)tp->t_addr;
			addr->dmfcsr = DMF_IE | DMFIR_TBUF | (unit&LINEMASK);
			/*
			 * If the DSR signal is being followed, give the modem
			 * at most 5 seconds to drop.
			 */
			if (dmfdsr && (addr->dmfrms&DMF_DSR)) {
				timeout(wakeup, (caddr_t) &tp->t_dev, 5*hz);
				sleep((caddr_t)&tp->t_dev, PZERO-10);
			}
			timeout(wakeup, (caddr_t) &tp->t_dev, hz/5);
			sleep((caddr_t)&tp->t_dev, PZERO-10);
			tp->t_state &= ~(TS_CLOSING);
			wakeup((caddr_t)&tp->t_rawq);
			splx(s);
		}
	}
	dmfsoftCAR[dmf] &= ~(1<<(unit&LINEMASK));
	dmfsoftCAR[dmf] |= (1<<(unit&LINEMASK)) & dmfdefaultCAR[dmf];
	ttyclose(tp);
	dmfmodem[unit] = 0;
	tty_def_close(tp);
}

dmfread(dev, uio)
	dev_t dev;
	struct uio *uio;
{
	register struct tty *tp;

	if(minor(dev)&0200) return(ENXIO);
	tp = &dmf_tty[minor(dev)];
	return ((*linesw[tp->t_line].l_read)(tp, uio));
}

dmfwrite(dev, uio)
	dev_t dev;
	struct uio *uio;
{
	register struct tty *tp;

	if(minor(dev)&0200)
		return(dmflwrite(dev,uio));
	tp = &dmf_tty[minor(dev)];
	return ((*linesw[tp->t_line].l_write)(tp, uio));
}

/*
 * DMF32 receiver interrupt.
 */
dmfrint(dmf)
	int dmf;
{
	register struct tty *tp;
	register c, flg;
	register struct dmfdevice *addr;
	register struct tty *tp0;
	struct uba_device *ui;
	register int line;
	int overrun = 0;
	register u_char *modem0, *modem;
	int modem_cont;

	ui = dmfinfo[dmf];
	if (ui == 0 || ui->ui_alive == 0)
		return;
	addr = (struct dmfdevice *)ui->ui_addr;
	tp0 = &dmf_tty[dmf<<3];
	modem0 = &dmfmodem[dmf<<3];
	/*
	 * Loop fetching characters from the silo for this
	 * dmf until there are no more in the silo.
	 */
	while ((c = addr->dmfrbuf) < 0) {
		line = (c>>8)&LINEMASK;
		tp = tp0 + line;
		flg = tp->t_iflag;
		modem = modem0 + line;
#ifdef	DMFDEBUG
		if (dmfdebug>1)
			mprintf("dmfrint: tp=%x, c=%x\n", tp, c);
#endif
		/* check for modem transitions */
		if (c & DMF_DSC) {
#ifdef	DMFDEBUG
			if (dmfdebug)
				mprintf("dmfrint: DSC, tp=%x, rms=%x\n",
					tp, addr->dmfrms);
#endif
			if (tp->t_cflag & CLOCAL)
				continue;
			addr->dmfcsr = DMF_IE | DMFIR_TBUF | line;

			modem_cont = 0;

			/*
			 * Drop DTR immediately if DSR gone away.
			 * If really an active close then do not
			 *    send signals.
			 */


			if ((addr->dmfrms&DMF_DSR)==0)	{
				 if (tp->t_state&TS_CLOSING) {
					untimeout(wakeup, (caddr_t) &tp->t_dev);
					wakeup((caddr_t) &tp->t_dev);
#ifdef DMFDEBUG
					if (dmfdebug)
					   mprintf("dmfrint: dsr closing down, tp=%x\n", tp);
#endif
					continue;
				 }

				 if (tp->t_state&TS_CARR_ON) {
					/*
 					 * Only drop line if DSR is being followed.
 					 */
					if (dmfdsr) {
						dmf_tty_drop(tp);
						continue;
					}
				 }
			}
			/*
			 * Check for transient CD drops.
			 * Only drop DTR if CD is down for more than 2 secs.
			 */

			if (tp->t_state&TS_CARR_ON)
			    if ((addr->dmfrms&DMF_CAR)==0) {
				if (*modem & MODEM_CD) {
				    /* only start timer once */
				    if (dmfdebug)
					mprintf("dmfrint, cd_drop, tp=%x\n", tp);
				    *modem &= ~MODEM_CD;
				    dmftimestamp[minor(tp->t_dev)] = time;
				    timeout(dmf_cd_drop, tp, hz*dmfcdtime);
				    modem_cont = 1;
				}
			    } else
				/*
				 * CD has come up again.
				 * Stop timeout from occurring if set.
				 * If interval is more than 2 secs then
				 *  drop DTR.
				 */
			       if ((*modem&MODEM_CD)==0) {
					untimeout(dmf_cd_drop, tp);
					if (dmf_cd_down(tp))
						/* drop connection */
						dmf_tty_drop(tp);
					*modem |= MODEM_CD;
				        modem_cont = 1;
			       }

			/* CTS flow control check */

			if (tp->t_state&TS_CARR_ON)
				if ((addr->dmfrms&DMF_CTS)==0) {
					tp->t_state |= TS_TTSTOP;
					*modem &= ~MODEM_CTS;
					if (dmfdebug)
					   mprintf("dmfrint: CTS stop, tp=%x\n", tp);
					dmfstop(tp, 0);
					continue;
				} else if ((*modem&MODEM_CTS)==0) {
					    tp->t_state &= ~TS_TTSTOP;
					    *modem |= MODEM_CTS;
					    if (dmfdebug)
					       mprintf("dmfrint: CTS start, tp=%x\n", tp);
					    dmfstart(tp);
					    continue;
					}

			/*
			 * Avoid calling dmfstart due to a carrier transition.
			 */
			if (modem_cont)
				continue;

			/*
			 * If 500 ms timer has not expired then dont
			 * check anything yet.
			 * Check to see if DSR|CTS|CD are asserted.
			 * If so we have a live connection.
			 * If DSR is set for the first time we allow
			 * 30 seconds for a live connection.
			 *
			 * If the DSR signal is being followed, wait at most
			 * 30 seconds for CD, and don't transmit in the first 
			 * 500ms.  Otherwise immediately look for CD|CTS.
			 */
			if (dmfdsr) {
				if ((addr->dmfrms&DMF_XMIT)==DMF_XMIT
				    && (*modem&MODEM_DSR_START)==0)
					dmf_start_tty(tp);
				else
				    if ((addr->dmfrms&DMF_DSR) &&
					(*modem&MODEM_DSR)==0) {
					*modem |= (MODEM_DSR_START|MODEM_DSR);
					/*
					 * we should not look for CTS|CD for
					 * about 500 ms.
					 */
					timeout(dmf_dsr_check, tp, hz*30);
					timeout(dmf_dsr_check, tp, hz/2);
				    }
			}
			/*
			 * Ignore DSR 
			 */
			else
				if ((addr->dmfrms & DMF_NODSR) == DMF_NODSR)
					dmf_start_tty(tp);


			continue;
			} /* end of modem transition tests */

		if ((tp->t_state&TS_ISOPEN)==0) {
			wakeup((caddr_t)&tp->t_rawq);
			continue;
		}

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

		/* DMF_FE is interpreted as a break */
		if (c & DMF_FE) {
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
#ifdef DMFDEBUG
				if (dmfdebug)
					mprintf("dmfrint: BREAK RECEIVED\n");
#endif DMFDEBUG
				if ((tp->t_lflag_ext & PRAW) && 
					(tp->t_line != TERMIODISC))
					c = 0;
				else {
				    ttyflush(tp, FREAD|FWRITE);
#ifdef DMFDEBUG
				    if (dmfdebug)
					mprintf("sending signal to tp->t_pgrp = %d\n", tp->t_pgrp);
#endif DMFDEBUG
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
		else if (c & DMF_PE){
			/* 
			 * If input parity checking is not enabled, clear out
			 * parity error in this character.
			 */
#ifdef DMFDEBUG
			if (dmfdebug > 1)
				mprintf("dmfrint: Parity Error\n");
#endif DMFDEBUG
			if ((flg & INPCK) == 0)
				c &= ~DMF_PE;
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
		if ((c & DMF_DO) && overrun == 0) {
			printf("dmf%d: recv. fifo overflow\n", dmf);
			overrun = 1;
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

/*
 * Ioctl for DMF32.
 */
/*ARGSUSED*/
dmfioctl(dev, cmd, data, flag)
	dev_t dev;
	register int cmd;
	caddr_t data;
	int flag;
{
	register int unit = minor(dev);
	register struct dmfdevice *dmfaddr;
	register struct tty *tp;
	register int dmf;
	register int s;
	struct uba_device *ui;
	struct dmf_softc *sc;
	struct devget *devget;
	int error;

	if(unit & 0200)
		return (ENOTTY);
	tp = &dmf_tty[unit];
	dmf = unit >> 3;
	ui = dmfinfo[dmf];
	sc = &dmf_softc[ui->ui_unit];
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
 				dmfparam(unit);
 				break;
 		}
		return (error);
	}
	switch (cmd) {

	case TIOCSBRK:
		timeout(dmfsetbreak, tp, dmf_delay[tp->t_cflag & CBAUD]);
		TTY_SLEEP(tp, (caddr_t)&tp->t_dev, TTOPRI);
		(void) dmfmctl(dev, DMF_BRK, DMBIS);
		break;

	case TIOCCBRK:
		(void) dmfmctl(dev, DMF_BRK, DMBIC);
		break;

	case TIOCSDTR:
		(void) dmfmctl(dev, DMF_DTR|DMF_RTS, DMBIS);
		break;

	case TIOCCDTR:
		(void) dmfmctl(dev, DMF_DTR|DMF_RTS, DMBIC);
		break;

	case TIOCMSET:
		(void) dmfmctl(dev, dmtodmf(*(int *)data), DMSET);
		break;

	case TIOCMBIS:
		(void) dmfmctl(dev, dmtodmf(*(int *)data), DMBIS);
		break;

	case TIOCMBIC:
		(void) dmfmctl(dev, dmtodmf(*(int *)data), DMBIC);
		break;

	case TIOCMGET:
		*(int *)data = dmftodm(dmfmctl(dev, 0, DMGET));
		break;

#ifdef 0
	/*
	 * Doesn't work yet !
	 */
	/* Set local loopback mode */
	case TIOCSMLB:
		if (u.u_uid)
			return(EPERM);
		s=spl5();
		dmfaddr = (struct dmfdevice *)(dmf_tty[unit].t_addr);
		dmfaddr->dmfcsr = DMF_IE | DMFIR_LCR | (unit & LINEMASK);
		SETLCR(dmfaddr, dmfaddr->dmflcr|DMF_MAINT);
		splx(s);
		break;

	/* Clear local loopback mode */
	case TIOCCMLB:
		if (u.u_uid)
			return(EPERM);
		s=spl5();
		dmfaddr = (struct dmfdevice *)(dmf_tty[unit].t_addr);
		dmfaddr->dmfcsr = DMF_IE | DMFIR_LCR | (unit & LINEMASK);
		SETLCR(dmfaddr, dmfaddr->dmflcr&~DMF_MAINT);
		splx(s);
		break;
#endif 0

	case TIOCNMODEM:  /* ignore modem status */
		s = spl5();
		dmfsoftCAR[dmf] |= (1<<(unit&LINEMASK));
		if (*(int *)data) /* make mode permanent */
			dmfdefaultCAR[dmf] |= (1<<(unit&LINEMASK));
		dmfmodem[unit] = MODEM_CTS|MODEM_CD|MODEM_DSR;
		tp->t_state |= TS_CARR_ON;
		tp->t_cflag |= CLOCAL;		/* Map to termio */
		splx(s);
		break;
	case TIOCMODEM:  /* look at modem status - sleep if no carrier */
		s = spl5();
		dmfsoftCAR[dmf] &= ~(1<<(unit&LINEMASK));
		if (*(int *)data) /* make mode permanent */
			dmfdefaultCAR[dmf] &= ~(1<<(unit&LINEMASK));
		dmfaddr = (struct dmfdevice *)(dmf_tty[unit].t_addr);
		dmfaddr->dmfcsr = DMF_IE | DMFIR_TBUF | (unit&LINEMASK);
	   	/* 
	    	 * If dmfdsr is set look for DSR|CTS|CD, otherwise look 
	    	 * for CD|CTS only.
	    	 */
		if ((dmfdsr && ((dmfaddr->dmfrms & DMF_XMIT)==DMF_XMIT)) ||
		    ((dmfdsr == 0) && ((dmfaddr->dmfrms & DMF_NODSR)==DMF_NODSR))){
			tp->t_state |= TS_CARR_ON;
			tp->t_state &= ~TS_ONDELAY;
			dmfmodem[unit] = MODEM_CTS|MODEM_CD|MODEM_DSR;
		}
		else {
			tp->t_state &= ~(TS_CARR_ON);
			dmfmodem[unit] &= ~(MODEM_CTS|MODEM_CD|MODEM_DSR);
		}
		tp->t_cflag &= ~CLOCAL;		/* Map to termio */
		splx(s);
		break;
	case TIOCWONLINE:
		s = spl5();
#ifdef DMFDEBUG
		if (dmfdebug)
			mprintf("dmfioctl: WONLINE, tp=%x, state=%x\n",
				tp, tp->t_state);
#endif
		dmfaddr = (struct dmfdevice *)(dmf_tty[unit].t_addr);
		dmfaddr->dmfcsr = DMF_IE | DMFIR_TBUF | (unit&LINEMASK);
	   	/* 
	    	 * If dmfdsr is set look for DSR|CTS|CD, otherwise look 
	    	 * for CD|CTS only.
	    	 */
		if ((dmfdsr && ((dmfaddr->dmfrms & DMF_XMIT)==DMF_XMIT)) ||
		    ((dmfdsr == 0) && ((dmfaddr->dmfrms & DMF_NODSR)==DMF_NODSR))){
			tp->t_state |= TS_CARR_ON;
			tp->t_state &= ~TS_ONDELAY;
			dmfmodem[unit] = MODEM_CTS|MODEM_CD|MODEM_DSR;
		}
		else
			while ((tp->t_state & TS_CARR_ON) == 0)
				sleep((caddr_t)&tp->t_rawq, TTIPRI);
		splx(s);
		break;

	case DEVIOCGET: 			/* device status */
		devget = (struct devget *)data;
		bzero(devget,sizeof(struct devget));

		if (tp->t_cflag & CLOCAL) {
			sc->sc_category_flags[unit&LINEMASK] |= DEV_MODEM;
			sc->sc_category_flags[unit&LINEMASK] &= ~DEV_MODEM_ON;
		}
		else
			sc->sc_category_flags[unit&LINEMASK] |= (DEV_MODEM|DEV_MODEM_ON);

		devget->category = DEV_TERMINAL;
		devget->bus = DEV_UB;
		bcopy(DEV_DMF32,devget->interface,
		      strlen(DEV_DMF32));
		bcopy(DEV_UNKNOWN,devget->device,
		      strlen(DEV_UNKNOWN));		/* terminal	*/
		devget->adpt_num = ui->ui_adpt; 	/* which adapter*/
		devget->nexus_num = ui->ui_nexus;	/* which nexus	*/
		devget->bus_num = ui->ui_ubanum;	/* which UBA	*/
		devget->ctlr_num = dmf; 		/* which interf.*/
		devget->slave_num = unit&LINEMASK;	/* which line	*/
		bcopy(ui->ui_driver->ud_dname,
		      devget->dev_name,
		      strlen(ui->ui_driver->ud_dname)); /* Ultrix "dmf" */
		devget->unit_num = unit&LINEMASK;	/* which dmf line ? */
		devget->soft_count =
		      sc->sc_softcnt[unit&LINEMASK];	/* soft er. cnt.*/
		devget->hard_count =
		      sc->sc_hardcnt[unit&LINEMASK];	/* hard er cnt. */
		devget->stat = sc->sc_flags[unit&LINEMASK]; /* status	*/
		devget->category_stat =
		      sc->sc_category_flags[unit&LINEMASK]; /* cat. stat.  */
		break;

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
		dmfparam(unit);
		break;

	default:
		if (u.u_procp->p_progenv == A_POSIX) 
			return (EINVAL);
		return (ENOTTY);
	}
	return (0);
}

dmtodmf(bits)
	register int bits;
{
	register int b;

	b = bits & 012;
	if (bits & DML_ST) b |= DMF_RATE;
	if (bits & DML_RTS) b |= DMF_RTS;
	if (bits & DML_USR) b |= DMF_USRW;
	return(b);
}

dmftodm(bits)
	register int bits;
{
	register int b;

	b = (bits & 012) | ((bits >> 7) & 0760) | DML_LE;
	if (bits & DMF_USRR) b |= DML_USR;
	if (bits & DMF_RTS) b |= DML_RTS;
	return(b);
}


/*
 * Set parameters from open or stty into the DMF hardware
 * registers.
 */
dmfparam(unit)
	register int unit;
{
	register struct tty *tp;
	register struct dmfdevice *addr;
	register int lpar, lcr;
	int s;

	tp = &dmf_tty[unit];
	addr = (struct dmfdevice *)tp->t_addr;
	/*
	 * Block interrupts so parameters will be set
	 * before the line interrupts.
	 */
	s = spl5();
	addr->dmfcsr = (unit&LINEMASK) | DMFIR_LCR | DMF_IE;
	/* 
 	 * Disconnect modem line if baudrate is zero.  POSIX checks the output
	 * baud rate, while non-POSIX checks the input baud rate.
	 */
	if ((((tp->t_cflag&CBAUD)==B0) && (u.u_procp->p_progenv != A_POSIX)) || 
	    (((tp->t_cflag_ext&CBAUD)==B0) && (u.u_procp->p_progenv==A_POSIX))){
		tp->t_cflag |= HUPCL;
		(void) dmfmctl(unit, DMF_OFF, DMSET);
		splx(s);
		return;
	}
	lpar = (dmf_speeds[tp->t_cflag_ext&CBAUD]<<12) | 	/* ospeed */
		(dmf_speeds[tp->t_cflag&CBAUD]<<8);	/* ispeed */
	lcr = DMFLCR_ENA;
	/*
	 * Berkeley-only dinosaurs
	 */
	if (tp->t_line != TERMIODISC) {
		if ((tp->t_cflag & CBAUD) == B134){
			lpar |= BITS6|PENABLE; 
			tp->t_cflag |= CS6|PARENB;
		}
		else if ((tp->t_cflag_ext & CBAUD) == B110){
			lpar |= TWOSB;
			tp->t_cflag |= CSTOPB;
		}
	}
 	/*
	 * Set device registers according to the specifications of the termio
	 * structure.
 	 */
 	if (tp->t_cflag & CREAD)
 		lcr |= DMF_RE;
	else
 		lcr &= ~DMF_RE;
 	if (tp->t_cflag & CSTOPB)
 		lpar |= TWOSB;
 	else
 		lpar &= ~TWOSB;
 	/* parity is enable */
 	if (tp->t_cflag & PARENB) {
 		if ((tp->t_cflag & PARODD) == 0)
 			/* set even */
 			lpar |= PENABLE|EPAR;
 		else
 			/* else set odd */
 			lpar = (lpar | PENABLE)&~EPAR;
 	}
 	/*
 	 * character size.
 	 * clear bits and check for 6,7,and 8, else its 5 bits.
 	 */
 	lpar &= ~BITS8;
 	switch(tp->t_cflag&CSIZE) {
 		case CS6:
 			lpar |= BITS6;
 			break;
 		case CS7:
 			lpar |= BITS7;
 			break;
 		case CS8:
 			lpar |= BITS8;
 			break;
 	}
	/*
	 * Outgoing Auto flow control.
	 * No auto flow control allowed if startc != ^q and startc !=
	 * ^s.  Most drivers do not allow this to be changed.
	 */
	if ((tp->t_cflag_ext & PAUTOFLOW) && (tp->t_cc[VSTOP] == CTRL('s')) && 
		(tp->t_cc[VSTART] == CTRL('q')))
		lcr |= DMF_AUTOX;
	lpar |= (unit&LINEMASK);
	addr->dmflpr = lpar;
	SETLCR(addr, lcr);
#ifdef DMFDEBUG
	if (dmfdebug)
		mprintf("dmfparam: tp=%x, lpr=%x, lcr=%o\n",
			tp, addr->dmflpr, addr->dmfun.dmfirw);
#endif
	splx(s);
}

/*
 * DMF32 transmitter interrupt.
 * Restart the idle line.
 */


dmfxint(dmf)
	int dmf;
{
	int u = dmf * 8;
	struct tty *tp0 = &dmf_tty[u];
	register struct tty *tp;
	register struct dmfdevice *addr;
	register struct uba_device *ui;
	register int t;
	short cntr;

	ui = dmfinfo[dmf];
	addr = (struct dmfdevice *)ui->ui_addr;
	while ((t = addr->dmfcsr) & DMF_TI) {
		if (t & DMF_NXM)
			/* SHOULD RESTART OR SOMETHING... */
			printf("dmf%d: NXM line %d\n", dmf, t >> 8 & 7);
		t = t >> 8 & 7;
		tp = tp0 + t;
		tp->t_state &= ~TS_BUSY;
		if (tp->t_state&TS_FLUSH)
			tp->t_state &= ~TS_FLUSH;
		else if (dmf_dma[u + t]) {
			/*
			 * Do arithmetic in a short to make up
			 * for lost 16&17 bits.
			 */
			addr->dmfcsr = DMFIR_TBA | DMF_IE | t;
			cntr = addr->dmftba -
			    UBACVT(tp->t_outq.c_cf, ui->ui_ubanum);
			ndflush(&tp->t_outq, (int)cntr);
		}
		if (tp->t_line)
			(*linesw[tp->t_line].l_start)(tp);
		else
			dmfstart(tp);
	}
}




/*
 * Start (restart) transmission on the given DMF32 line.
 */

dmfstart(tp)
	register struct tty *tp;
{
	register struct dmfdevice *addr;
	register int unit, nch, line;
	int s;
	register int dmf;

	unit = minor(tp->t_dev);
	dmf = unit >> 3;
	line = unit & LINEMASK;
	addr = (struct dmfdevice *)tp->t_addr;

	/*
	 * Must hold interrupts in following code to prevent
	 * state of the tp from changing.
	 */
	s = spl5();
	/*
	 * If it's currently active, or delaying, no need to do anything.
	 * Also do not transmit if not CTS.
	 */
	if ((tp->t_state&(TS_TIMEOUT|TS_BUSY|TS_TTSTOP)) ||
		 ((tp->t_state & TS_CARR_ON) && (dmfmodem[unit]&MODEM_CTS)==0))
		goto out;
	/*
	 * If there are still characters in the silo,
	 * just reenable the transmitter.
	 */
	addr->dmfcsr = DMF_IE | DMFIR_TBUF | line;
	if (addr->dmftsc) {
		addr->dmfcsr = DMF_IE | DMFIR_LCR | line;
		SETLCR(addr, addr->dmflcr|DMF_TE);
		tp->t_state |= TS_BUSY;
		goto out;
	}
	/*
	 * If there are sleepers, and output has drained below low
	 * water mark, wake up the sleepers.
	 */
	if (tp->t_outq.c_cc<=TTLOWAT(tp)) {
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
	/*
	 * Now restart transmission unless the output queue is
	 * empty.
	 */
	if (tp->t_outq.c_cc == 0)
		goto out;
	if ((tp->t_lflag_ext & PRAW) || (tp->t_oflag_ext & PLITOUT) || 
	    ((tp->t_oflag & OPOST) == 0))
		nch = ndqb(&tp->t_outq, 0);
	else {
		if ((nch = ndqb(&tp->t_outq, DELAY_FLAG)) == 0) {
			/*
			* If first thing on queue is a delay process it.
			*/
			nch = getc(&tp->t_outq);
			timeout(ttrstrt, (caddr_t)tp, (nch&0x7f)+6);
			tp->t_state |= TS_TIMEOUT;
			goto out;
		}
	}
	/*
	 * If characters to transmit, restart transmission.
	 */
	if (nch >= dmf_mindma) {
		register car;

		dmf_dma[unit] = 1;
		addr->dmfcsr = DMF_IE | DMFIR_LCR | line;
                /*
                 * If outgoing auto flow control is enabled, the hardware will
                 * control the transmit enable bit.
                 */
                if ((tp->t_cflag_ext & PAUTOFLOW) == 0)
			SETLCR(addr, addr->dmflcr|DMF_TE);
		car = UBACVT(tp->t_outq.c_cf, dmfinfo[dmf]->ui_ubanum);
		addr->dmfcsr = DMF_IE | DMFIR_TBA | line;
		/*
		 * Give the device the starting address of a DMA transfer.  
		 * Translate the system virtual address into a physical
		 * address.
		 */
		addr->dmftba = svtophy(car);
		addr->dmftcc = ((car >> 2) & 0xc000) | nch;
		tp->t_state |= TS_BUSY;
	} else if (nch) {
		register char *cp = tp->t_outq.c_cf;
		register int i;

		dmf_dma[unit] = 0;
		nch = MIN(nch, DMF_SILOCNT);
		addr->dmfcsr = DMF_IE | DMFIR_LCR | line;
                /*
                 * If outgoing auto flow control is enabled, the hardware will
                 * control the transmit enable bit.
                 */
                if ((tp->t_cflag_ext & PAUTOFLOW) == 0)
			SETLCR(addr, addr->dmflcr|DMF_TE);
		addr->dmfcsr = DMF_IE | DMFIR_TBUF | line;
		for (i = 0; i < nch; i++)
			addr->dmftbuf = *cp++;
		ndflush(&tp->t_outq, nch);
		tp->t_state |= TS_BUSY;
	}
out:
	splx(s);
}




/*
 * Stop output on a line, e.g. for ^S/^Q or output flush.
 */
/*ARGSUSED*/
dmfstop(tp, flag)
	register struct tty *tp;
{
	register struct dmfdevice *addr;
	register int unit, s;

	addr = (struct dmfdevice *)tp->t_addr;
	/*
	 * Block input/output interrupts while messing with state.
	 */
	s = spl5();
	if (tp->t_state & TS_BUSY) {
		/*
		 * Device is transmitting; stop output
		 * by selecting the line and disabling
		 * the transmitter.  If this is a flush
		 * request then flush the output silo,
		 * otherwise we will pick up where we
		 * left off by enabling the transmitter.
		 */
		unit = minor(tp->t_dev);
		addr->dmfcsr = DMFIR_LCR | (unit&LINEMASK) | DMF_IE;
		/*
		 * If this test isn't done, the hardware will need to see a
		 * start character to get it going again if DMF_AUTOX is set.
		 */
		if ((tp->t_cflag_ext & PAUTOFLOW) == 0)
			SETLCR(addr, addr->dmflcr &~ DMF_TE);
		if ((tp->t_state&TS_TTSTOP)==0) {
			tp->t_state |= TS_FLUSH;
			SETLCR(addr, addr->dmflcr|DMF_FLUSH);
		} else
			tp->t_state &= ~TS_BUSY;
	}
	splx(s);
}

/*
 * DMF32 modem control
 */
dmfmctl(dev, bits, how)
	dev_t dev;
	int bits, how;
{
	register struct dmfdevice *dmfaddr;
	register int unit, mbits, lcr;
	int s;

	unit = minor(dev);
	dmfaddr = (struct dmfdevice *)(dmf_tty[unit].t_addr);
	unit &= LINEMASK;
	s = spl5();
	dmfaddr->dmfcsr = DMF_IE | DMFIR_TBUF | unit;
	mbits = dmfaddr->dmfrms << 8;
	dmfaddr->dmfcsr = DMF_IE | DMFIR_LCR | unit;
	mbits |= dmfaddr->dmftms;
	lcr = dmfaddr->dmflcr;
	switch (how) {
	case DMSET:
		mbits = (mbits &0xff00) | bits;
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
	if (mbits & DMF_BRK)
		lcr |= DMF_RBRK;
	else
		lcr &= ~DMF_RBRK;
	lcr = ((mbits & 037) << 8) | (lcr & 0xff);
	dmfaddr->dmfun.dmfirw = lcr;
	(void) splx(s);
	return(mbits);
}

/*
 * Reset state of driver if UBA reset was necessary.
 * Reset the csr, lpr, and lcr registers on open lines, and
 * restart transmitters.
 */
dmfreset(uban)
	int uban;
{
	register int dmf, unit;
	register struct tty *tp;
	register struct uba_device *ui;
	register struct dmfdevice *addr;
	int i;

	if (tty_ubinfo[uban] == 0)
		return;
	cbase[uban] = tty_ubinfo[uban]&0x3ffff;
	for (dmf = 0; dmf < nNDMF; dmf++) {
		ui = dmfinfo[dmf];
		if (ui == 0 || ui->ui_alive == 0 || ui->ui_ubanum != uban)
			continue;
		printf(" dmf%d", dmf);
		addr = (struct dmfdevice *)ui->ui_addr;
		addr->dmfcsr = DMF_IE;
		addr->dmfrsp = dmf_timeout;
		unit = dmf * 8;
		for (i = 0; i < 8; i++) {
			tp = &dmf_tty[unit];
			if (tp->t_state & (TS_ISOPEN|TS_WOPEN)) {
				dmfparam(unit);
				(void) dmfmctl(unit, DMF_ON, DMSET);
				tp->t_state &= ~TS_BUSY;
				dmfstart(tp);
			}
			unit++;
		}
	}
}

/* dmflopen -- open the line printer port on a dmf32
 *
 */
dmflopen(dev,flag)
dev_t dev;
int flag;
{
	register int dmf;
	register struct dmfl_softc *sc;
	register struct uba_device *ui;
	register struct dmfdevice *addr;


	dmf = minor(dev) & LINEMASK ;
	if(((sc= &dmfl_softc[dmf])->dmfl_state & OPEN) ||
		((ui=dmfinfo[dmf]) == 0) || ui->ui_alive == 0)
			return(ENXIO);
	addr = (struct dmfdevice *)ui->ui_addr;
	if((addr->dmfl[0] & DMFL_OFFLINE))
	{
		/*printf("dmf%d: line printer offline/jammed\n", dmf);*/
		return(EIO);
	}
	if((addr->dmfl[0]&DMFL_CONV))
	{
		printf("dmf%d: line printer disconnected\n", dmf);
		return(EIO);
	}

	addr->dmfl[0] = 0;
	sc->dmfl_state |= OPEN;
	return 0;
}

dmflclose(dev,flag)
dev_t dev;
int flag;
{
	register int dmf= minor(dev) & LINEMASK;
	register struct dmfl_softc *sc = &dmfl_softc[dmf];

	dmflout(dev,"\f",1);
	sc->dmfl_state = 0;
	if(sc->dmfl_info != 0)
		ubarelse((struct dmfdevice *)(dmfinfo[dmf])->ui_ubanum,
			&(sc->dmfl_info));

	((struct dmfdevice *)(dmfinfo[dmf]->ui_addr))->dmfl[0]=0;
	return 0;
}

dmflwrite(dev,uio)
dev_t dev;
struct uio *uio;
{
	register unsigned int n;
	register int error;
	register struct dmfl_softc *sc;

	sc = &dmfl_softc[minor(dev)&LINEMASK];
	if(sc->dmfl_state&ERROR) return(EIO);
	while(n=min(DMFL_BUFSIZ,(unsigned)uio->uio_resid))
	{
		if(error=uiomove(&sc->dmfl_buf[0],(int)n,
			UIO_WRITE,uio))
		{
			printf("uio move error\n");
			return(error);
		}
		if(error=dmflout(dev,&sc->dmfl_buf[0],n))
		{
			return(error);
		}
	}
	return 0;
}


/* dmflout -- start io operation to dmf line printer
 *		cp is addr of buf of n chars to be sent.
 *
 *	-- dmf will be put in formatted output mode, this will
 *		be selectable from an ioctl if the
 *		need ever arises.
 */
dmflout(dev,cp,n)
dev_t dev;
char *cp;
int n;
{
	register struct dmfl_softc *sc;
	register int dmf;
	register struct uba_device *ui;
	register struct dmfdevice *d;
	register unsigned info;
	register unsigned i;

	dmf = minor(dev) & LINEMASK ;
	sc= &dmfl_softc[dmf];
	if(sc->dmfl_state&ERROR) return(EIO);
	ui= dmfinfo[dmf];
	/* allocate unibus resources, will be released when io
	 * operation is done
	 */
	sc->dmfl_info=
	info=
		uballoc(ui->ui_ubanum,cp,n,0);
	d= (struct dmfdevice *)ui->ui_addr;
	d->dmfl[0] = (2<<8) | DMFL_FORMAT; /* indir reg 2 */
	/* indir reg auto increments on r/w */
	/* SO DON'T CHANGE THE ORDER OF THIS CODE */
	d->dmfl[1] = 0; /* prefix chars & num */
	d->dmfl[1] = 0; /* suffix chars & num */
	d->dmfl[1] = info;	/* dma lo 16 bits addr */

	/* NOT DOCUMENTED !! */
	d->dmfl[1] = -n;		/* number of chars */
	/* ----------^-------- */

	d->dmfl[1] = ((info>>16)&3) /* dma hi 2 bits addr */
		| (1<<8) /* auto cr insert */
		| (1<<9) /* use real ff */
		| (1<<15); /* no u/l conversion */
	d->dmfl[1] = sc->dmfl_lines	/* lines per page */
		| (sc->dmfl_cols<<8);	/* carriage width */
	sc->dmfl_state |= ASLP;
	i=spl5();
	d->dmfl[0] |= DMFL_PEN|DMFL_IE;
	while(sc->dmfl_state & ASLP)
	{
		sleep(&sc->dmfl_buf[0],(PZERO+8));
		while(sc->dmfl_state&ERROR)
		{
			timeout(dmflint,dmf,10*hz);
			sleep(&sc->dmfl_state,(PZERO+8));
		}
		/*if(sc->dmfl_state&ERROR) return (EIO);*/
	}
	splx(i);
	return(0);
}
/* dmflint -- handle an interrupt from the line printer part of the dmf32
 *
 */

dmflint(dmf)
int dmf;
{

	register struct uba_device *ui;
	register struct dmfl_softc *sc;
	register struct dmfdevice *d;

	ui= dmfinfo[dmf];
	sc= &dmfl_softc[dmf];
	d= (struct dmfdevice *)ui->ui_addr;

	d->dmfl[0] &= ~DMFL_IE;

	if(sc->dmfl_state&ERROR)
	{
		/*printf("dmf%d: intr while in error state \n", dmf);*/
		if((d->dmfl[0]&DMFL_OFFLINE) == 0)
			sc->dmfl_state &= ~ERROR;
		wakeup(&sc->dmfl_state);
		return;
	}
	if(d->dmfl[0]&DMFL_DMAERR)
	{
		printf("dmf%d:NXM\n", dmf);
	}
	if(d->dmfl[0]&DMFL_OFFLINE)
	{
		/*printf("dmf%d:printer error\n", dmf);*/
		sc->dmfl_state |= ERROR;
	}
	if(d->dmfl[0]&DMFL_PDONE)
	{
#ifdef notdef
		printf("bytes= %d\n",d->dmfl[1]);
		printf("lines= %d\n",d->dmfl[1]);
#endif
	}
	sc->dmfl_state &= ~ASLP;
	wakeup(&sc->dmfl_buf[0]);
	if(sc->dmfl_info != 0)
		ubarelse(ui->ui_ubanum,&sc->dmfl_info);
	sc->dmfl_info = 0;

}

/* stubs for interrupt routines for devices not yet supported */

dmfsrint() { printf("dmfsrint\n"); }

dmfsxint() { printf("dmfsxint\n"); }

dmfdaint() { printf("dmfdaint\n"); }

dmfdbint() { printf("dmfdbint\n"); }

dmf_cd_drop(tp)
register struct tty *tp;
{
	register struct dmfdevice *addr = (struct dmfdevice *)tp->t_addr;
	register unit = minor(tp->t_dev);

	addr->dmfcsr = DMF_IE | DMFIR_TBUF | (unit&LINEMASK);
	if ((tp->t_state&TS_CARR_ON) &&
		((addr->dmfrms&DMF_CAR) == 0)) {
#ifdef DMFDEBUG
		if (dmfdebug)
		    mprintf("dmf_cd:  no CD, tp=%x\n", tp);
#endif
		dmf_tty_drop(tp);
		return;
	}
	dmfmodem[unit] |= MODEM_CD;
#ifdef DMFDEBUG
	if (dmfdebug)
	    mprintf("dmf_cd:  CD is up, tp=%x\n", tp);
#endif
}


dmf_dsr_check(tp)
register struct tty *tp;
{
	int unit = minor(tp->t_dev);
	register struct dmfdevice *addr = (struct dmfdevice *)tp->t_addr;
	if (dmfmodem[unit]&MODEM_DSR_START) {
		dmfmodem[unit] &= ~MODEM_DSR_START;
		addr->dmfcsr = DMF_IE | DMFIR_TBUF | (unit&LINEMASK);
	   	/* 
	    	 * If dmfdsr is set look for DSR|CTS|CD, otherwise look 
	    	 * for CD|CTS only.
	    	 */
		if (dmfdsr) {
			if ((addr->dmfrms&DMF_XMIT)==DMF_XMIT)
				dmf_start_tty(tp);
		}
		else {
			if ((addr->dmfrms&DMF_NODSR)==DMF_NODSR)
				dmf_start_tty(tp);
		}
		return;
	}
	if ((tp->t_state&TS_CARR_ON)==0)  {
		dmf_tty_drop(tp);
		if (dmfdebug)
		    mprintf("dmf_dsr:  no carrier, tp=%x\n", tp);
	}
#ifdef DMFDEBUG
	else
		if (dmfdebug)
		    mprintf("dmf_dsr:  carrier is up, tp=%x\n", tp);
#endif
}

/*
 *  cd_down return 1 if carrier has been down for at least 2 secs.
 */
dmf_cd_down(tp)
struct tty *tp;
{
	int msecs;
	register int unit = minor(tp->t_dev);

	msecs = 1000000 * (time.tv_sec - dmftimestamp[unit].tv_sec) +
		(time.tv_usec - dmftimestamp[unit].tv_usec);
	if (msecs > 2000000)
		return(1);
	else
		return(0);
}

dmf_tty_drop(tp)
struct tty *tp;
{
	register struct dmfdevice *addr = (struct dmfdevice *)tp->t_addr;
	register int line;
	register int unit;
	if (tp->t_flags&NOHANG)
		return;
	unit = minor(tp->t_dev);
	line = unit & LINEMASK;
	dmfmodem[unit] = MODEM_BADCALL;
	tp->t_state &= ~(TS_CARR_ON|TS_TTSTOP|TS_BUSY|TS_ISUSP);
	wakeup((caddr_t)&tp->t_rawq);
	gsignal(tp->t_pgrp, SIGHUP);
	gsignal(tp->t_pgrp, SIGCONT);
	addr->dmfcsr=DMF_IE | DMFIR_LCR | line;
	addr->dmftms = 0;
}

dmf_start_tty(tp)
	register struct tty *tp;
{
	register int unit = minor(tp->t_dev);
	tp->t_state &= ~TS_ONDELAY;
	tp->t_state |= TS_CARR_ON;
	if (dmfmodem[unit]&MODEM_DSR)
		untimeout(dmf_dsr_check, tp);
	dmfmodem[unit] |= MODEM_CD|MODEM_CTS|MODEM_DSR;
	dmftimestamp[unit] = dmfzerotime;
	wakeup((caddr_t)&tp->t_rawq);
}

dmfbaudrate(speed)
register int speed;
{
    if (dmf_valid_speeds & (1 << speed))
	return (1);
    else
	return (0);
}

/*
 * Time up for process to set a break on a transmission line.
 */
void dmfsetbreak(tp)
register struct tty *tp;
{
	wakeup((caddr_t)&tp->t_dev);
}

#endif
