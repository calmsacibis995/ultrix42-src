
#ifndef lint
static char *sccsid = "@(#)dmz.c	4.1	(ULTRIX)	7/2/90";
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
 * dmz.c  6.1	7/29/83
 *
 * Modification history
 *
 * DMZ32 terminal driver
 *
 *  5-May-85 - Larry Cohen
 *
 *	Derived from 4.2BSD labeled: dmz.c	6.1	83/07/29.
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
 *	In dmzrint, record present time in timestamp in the event of
 *	a carrier drop.
 *
 * 12-Aug-86 - Tim Burke
 *
 *	Set dmz_mindma to a value of 200 to prevent dma transfers from occuring.
 *	This is to aviod hardware problems with NXM errors, and should be
 *	changed back when the device is fixed.
 *
 * 25-Aug-86 - Tim Burke
 *
 *	Insure that line is dropped and will restart on a false call to modem.
 *	Also change state to ~TS_CARR_ON to kill associated processes.
 *
 * 26-Aug-86 - rsp (Ricky Palmer)
 *
 *	Cleaned up devioctl code to (1) zero out devget structure
 *	upon entry and (2) use strlen instead of fixed storage
 *	for bcopy's. Fixed code to handle 24 lines instead of 32.
 *
 *  4-Dec-86 - Tim Burke
 *
 *	Bug fix to modem control.  In dmz_tty_drop routine, clear the stopped
 *	state to prevent modem lines from hanging on close.  
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
 *   9-Jan-86 - Tim Burke
 *
 *	Bug fix to TIOCMODEM to clear modem flags if signals are not up.
 *
 * 28-Jan-87 - Tim Burke
 *
 *	Added the capability to ignore the "DSR" modem signal.  This is being
 *	done to allow modems that do not follow DEC Standard 52 to still 
 *	function as they would have prior to the addition of DECSTD52 code
 *	into the drivers.  If the driver is setup to ignore "DSR" then it will
 *	not be following DECSTD52.  To follow DECSTD52 set dmzdsr to "1", to
 *	ignore "DSR" set dmzdsr to be "0";
 *
 *  3-Mar-87 - Tim Burke
 *
 *	Added full TERMIO functionality to terminal subsystem.
 *	Fixup master reset code in probe routine to work on a per-octet basis.
 *
 *
 *  27-Jul-87 - Dan Stuart & Tim Burke
 *
 *    Changed to properly link logical and physical lines, and index
 *    properly into dmz_tty.  Finishes 32 -> 24 lines changes by rsp above.
 *
 *  8-Sep-87 - rsp (Ricky Palmer)
 *
 *      Defined and used LINEMASK where appropriate to correctly set
 *      line number.
 *
 *  9-Sept-87 - Tim Burke
 *
 *	Added support for hardware auto flow control on the outgoing side.  This
 *	will provide quick response to start/stop characters which will reduce
 *	buffer overflow on the receiving device.
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
 */

#include "dmz.h"
#if NDMZ > 0  || defined(BINARY)

#include "../data/dmz_data.c"

int dmzdebug = 0;

/*
 * Definition of the driver for the auto-configuration program.
 * There are three sets of interrupt vectors for the dmz32 - one set
 * for each octet of lines.
 */
int	dmzprobe(), dmzattach(), dmzrinta(), dmzxinta(), dmzbaudrate(),
		dmzrintb(), dmzxintb(), dmzrintc(), dmzxintc();
int	dmz_cd_drop(), dmz_dsr_check(), dmz_cd_down(), dmz_tty_drop();
struct	timeval dmzzerotime = {0,0};
int	dmzcdtime = 2;
u_short dmzstd[] = { 0 };
struct	uba_driver dmzdriver =
	{ dmzprobe, 0, dmzattach, 0, dmzstd, "dmz", dmzinfo };

int	dmz_timeout = 10;		/* silo timeout, in ms */
	/*
	 * dmz_mindma should be set to a value of 4, such that dma is performed
	 * on blocks of four or greater characters to transfer.  Presently
	 * there is a hardware problem in the dmz device causing large
	 * numbers of NXM errors to occur.  These errors could potentially
	 * cause system panics.  As a temporary fix, the value of dmz_mindma
	 * is set to be greater that 60 (which is the largest possible clist
	 * size, which prevents dma from being done.  This fix could
	 * potentially degrade performance to produce stable operation.
	 */
int	dmz_mindma = 200;		/* don't dma below this point */

/*
 * Local variables for the driver
 */
char	dmz_speeds[] =
	{ 0, 0, 1, 2, 3, 4, 0, 5, 6, 7, 010, 012, 014, 016, 017, 0 };

short	dmz_valid_speeds = 0x7fbf; /* 0,1,1,1, 1,1,1,1, 1,0,1,1, 1,1,1,1 */

int	dmzstart(), ttrstrt();

#define NUMLINES 24		     	/* 24 lines on a DMZ */

#ifndef MODEM_CD
#define MODEM_CD   0x01
#define MODEM_DSR  0x02
#define MODEM_CTS  0x04
#define MODEM_DSR_START  0x08
#endif

/*
 * The clist space is mapped by the driver onto each UNIBUS.
 * The UBACVT macro converts a clist space address for unibus uban
 * into an i/o space address for the DMA routine.
 */
#define UBACVT(x, uban) 	(cbase[uban] + ((x)-(char *)cfree))

/*
 * Routine for configuration to set dmz interrupt.
 */
/*ARGSUSED*/
dmzprobe(reg)
	caddr_t reg;
{
	register struct dmzdevice *dmzaddr = (struct dmzdevice *)reg;
	register struct octet *oaddr;
	register int totaldelay;

#ifdef lint
	dmzxinta(0); dmzrinta(0);
	dmzxintb(0); dmzrintb(0);
	dmzxintc(0); dmzrintc(0);
#endif
	if(dmzdebug)
		printf("dmzprobe\n");
	/*
	 * If a self test is not being done, start one up.  Wait for the
	 * self-test to complete before interrupting. Done on a per-octet basis.
	 */

	for ( oaddr=dmzaddr->octets; oaddr<=dmzaddr->octets+2; ++oaddr ) {
	    if ( (oaddr->dmzcsr&DMZ_CLR) == 0 )
	        oaddr->dmzcsr |= DMZ_CLR;
	    totaldelay = 0;
	    while ( (oaddr->dmzcsr&DMZ_CLR) && (totaldelay<=70) ) {
	        totaldelay++;
	        DELAY(50000);
	    }
	    if ( oaddr->dmzcsr & DMZ_CLR )
	        printf("Warning: DMZ Master Clear failure, octet %d\n",
	            oaddr-dmzaddr->octets);
	}

	if ((dmzaddr->dmzccsr0 & DMZ_IDENT) != DMZ_IDENT) {
		printf("dmzprobe: not a dmz\n");
		return;
	}
	br = 0x15;
	/* load base of interrupt vectors */
	dmzaddr->dmzccsr0 = ((uba_hd[numuba].uh_lastiv -= 4*6) >> 2);

	/* try to generate an interrupt on the first octet */
	oaddr = dmzaddr->octets;  /* first octet */
	oaddr->dmzcsr = DMZ_IE | DMZIR_LCR;
	oaddr->dmzlcr = DMZ_TE | DMZ_FLUSH;
	DELAY(100000);		/* wait 1/10'th of a sec for interrupt */
	{ int temp = oaddr->dmzcsr; /* clear interrupt flag */ }
	if (cvec && cvec != 0x200) /* check to see if interrupt occurred */
		cvec -= 4;	   /* point to first interrupt vector (recv)*/
	else
		uba_hd[numuba].uh_lastiv += 4*6; /* no interrupt
						  * restore floating vector.
						  */
	/* NEED TO SAVE IT SOMEWHERE FOR OTHER DEVICES */
	return (sizeof (struct dmzdevice));
}

/*
 * Routine called to attach a dmz.
 */
dmzattach(ui)
	struct uba_device *ui;
{

	dmzsoftCAR[ui->ui_unit] = ui->ui_flags;
	dmzdefaultCAR[ui->ui_unit] = ui->ui_flags;
	if (dmzdebug)
		printf("dmzattach: unit=%d, flags=%x\n",
			ui->ui_unit, ui->ui_flags);
}


/*
 * Open a DMZ32 line, mapping the clist onto the uba if this
 * is the first dmz on this uba.  Turn on this dmz if this is
 * the first use of it.
 */
/*ARGSUSED*/
dmzopen(dev, flag)
	dev_t dev;
{
	register struct tty *tp;
	register int unit, dmz, octetnum;
	register struct octet *oaddr;	/* pointer to octet set */
	register struct uba_device *ui;
        register int maxdmzlines;
	int s;
	int inuse;  /*hold state of inuse bit while blocked waiting for carr*/

	unit = minor(dev);
	dmz = unit / NUMLINES;  /* module number */
        maxdmzlines = ndmz;     /* NDMZ * 24 = maximun number of line */
        if (unit >= maxdmzlines || (ui = dmzinfo[dmz])== 0 || ui->ui_alive == 0)
              return (ENXIO);
	tp = &dmz_tty[unit];
	if (tp->t_state&TS_XCLUDE && u.u_uid!=0)
		return (EBUSY);
	while (tp->t_state&TS_CLOSING) /* let DTR stay down for awhile */
		sleep((caddr_t)&tp->t_rawq, TTIPRI);
        octetnum = ( unit % NUMLINES ) / 8;           /* get octet #, line # later */
	oaddr = &((struct dmzdevice *)ui->ui_addr)->octets[octetnum];
	tp->t_addr = (caddr_t)oaddr;
	tp->t_oproc = dmzstart;
	tp->t_baudrate = dmzbaudrate;
	tp->t_state |= TS_WOPEN;
#ifdef DMZDEBUG
	if (dmzdebug)
		mprintf("dmzopen: unit=%d, oaddr=%x, tp=%x\n", unit, oaddr, tp);
#endif
	/*
	 * While setting up state for this uba and this dmz,
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
	oaddr->dmzcsr |= DMZ_IE;
	splx(s);

	if ((tp->t_state&TS_ISOPEN) == 0) {
		dmzmodem[unit] = MODEM_DSR_START;
		oaddr->dmzrsp = dmz_timeout;
	}
	tty_def_open(tp, dev, flag, (dmzsoftCAR[dmz]&(1<<(unit%NUMLINES))));
	dmzparam(unit);

	/*
	 * Wait for carrier, then process line discipline specific open.
	 */
	s = spl5();
	dmzmctl(dev, DMZ_ON, DMSET);	/* assert DTR */
	if (tp->t_cflag & CLOCAL) {
#ifdef DMZDEBUG
		if (dmzdebug)
			mprintf("dmzopen: local, unit=%d\n", unit);
#endif
		tp->t_state |= TS_CARR_ON;
		dmzmodem[unit] = MODEM_CTS|MODEM_CD|MODEM_DSR;
	} else
		if ((flag & (O_NDELAY|O_NONBLOCK)) == 0) {
		    oaddr->dmzcsr = DMZ_IE | DMZIR_TBUF | (unit&07);
		    /*
		     * DSR should not come up until DTR is asserted
		     * normally.  However if TS_HUPCL is not set it is
		     * possible to get here with all modem signals
		     * already asserted.  Or we could be dealing with
		     * an enormously slow modem and it has not deasserted
		     * DSR yet.
		     */
		   if (oaddr->dmzrms&DMZ_DSR) {
#ifdef notdef
			timeout(wakeup, (caddr_t) &tp->t_dev, 2*hz);
			sleep((caddr_t)&tp->t_dev, PZERO-10);
#endif
#ifdef DMZDEBUG
			if ((dmzdebug) && (oaddr->dmzrms&DMZ_DSR))
					mprintf("dmz%d: DSR up before DTR\n", unit);
#endif DMZDEBUG
		    }
		     /*
		      * lets assume we have a valid DSR signal now
		      */

		    oaddr->dmzcsr = DMZ_IE | DMZIR_TBUF | (unit&07);
#ifdef DMZDEBUG
			if (dmzdebug)
				mprintf("dmzopen: is dsr up?, unit=%d\n", unit);
#endif
			/*
			 * If the DSR signal is being followed, wait at most
			 * 30 seconds for CD, and don't transmit in the first 
			 * 500ms.  Otherwise immediately look for CD|CTS.
			 */
			if (dmzdsr) {
			    if (oaddr->dmzrms&DMZ_DSR) {
#ifdef DMZDEBUG
				if (dmzdebug)
					mprintf("dmzopen: dsr up, unit=%d\n", unit);
#endif
				dmzmodem[unit] |= (MODEM_DSR_START|MODEM_DSR);
				tp->t_dev = dev; /* need it for timeouts */
				timeout(dmz_dsr_check, tp, hz*30);
				timeout(dmz_dsr_check, tp, hz/2);
			    }
			}
			else {
				dmzmodem[unit] |= (MODEM_DSR_START|MODEM_DSR);
				dmz_dsr_check(tp);
			}
		}

	if (flag & (O_NDELAY|O_NONBLOCK)) {
#ifdef DMZDEBUG
		if (dmzdebug)
			mprintf("dmzopen: O_NDELAY, unit=%d\n", unit);
#endif
		tp->t_state |= TS_ONDELAY;
	}
	else
	   while ((tp->t_state & TS_CARR_ON) == 0) {
		tp->t_state |= TS_WOPEN;
		inuse = tp->t_state&TS_INUSE;
		sleep((caddr_t)&tp->t_rawq, TTIPRI);
 		/*
 		 * See if wakeup is due to a false call.
 		 */
 		if (dmzmodem[unit]&MODEM_BADCALL){
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
 * Close a DMZ32 line.
 */
/*ARGSUSED*/
dmzclose(dev, flag)
	dev_t dev;
	int flag;
{
	register struct tty *tp;
	register unit;
	register dmz, s;
	register struct octet *addr;

	unit = minor(dev);
	dmz = unit / NUMLINES;
	tp = &dmz_tty[unit];
#ifdef DMZDEBUG
	if (dmzdebug)
		mprintf("dmzclose: unit=%d, oaddr=%x\n", unit, tp->t_addr);
#endif
	s = spl5();
	if (tp->t_line)
		(*linesw[tp->t_line].l_close)(tp);
	(void) dmzmctl(unit, DMZ_BRK, DMBIC);
	if ((tp->t_cflag&HUPCL) || (tp->t_state&TS_ISOPEN)==0) {
		(void) dmzmctl(unit, DMZ_OFF, DMSET);
		tp->t_state &= ~TS_CARR_ON; /* prevents recv intr. timeouts */
		if ((tp->t_cflag & CLOCAL) == 0) {
#ifdef DMZDEBUG
			if (dmzdebug)
				mprintf("dmzclose: drop dtr, unit=%d, oaddr=%x\n", unit, tp->t_addr);
#endif
			tp->t_state |= TS_CLOSING;

			/* wait for DSR to drop */
			addr = (struct octet *)tp->t_addr;
			addr->dmzcsr = DMZ_IE | DMZIR_TBUF | (unit&07);
			/*
			 * If the DSR signal is being followed, give the modem
			 * 5 seconds to deasset it.
			 */
			if (dmzdsr && (addr->dmzrms&DMZ_DSR)) {
				timeout(wakeup, (caddr_t) &tp->t_dev, 5*hz);
				sleep((caddr_t)&tp->t_dev, PZERO-10);
			}
			timeout(wakeup, (caddr_t) &tp->t_dev, hz/5);
			sleep((caddr_t)&tp->t_dev, PZERO-10);

#ifdef DMZDEBUG
			if (dmzdebug && (addr->dmzrms&DMZ_DSR) )
				mprintf("dmzclose: dsr still on, unit=%d, oaddr=%x\n", unit, tp->t_addr);
#endif
			tp->t_state &= ~(TS_CLOSING);
			wakeup((caddr_t)&tp->t_rawq);
		}
	}
	/* reset line to default mode */
	dmzsoftCAR[dmz] &= ~(1<<(unit%NUMLINES));
	dmzsoftCAR[dmz] |= (1<<(unit%NUMLINES)) & dmzdefaultCAR[dmz];
	ttyclose(tp);
	dmzmodem[unit] = 0;
	tty_def_close(tp);
	splx(s);
}

dmzread(dev, uio)
	dev_t dev;
	struct uio *uio;
{
	register struct tty *tp;

	tp = &dmz_tty[minor(dev)];
	return ((*linesw[tp->t_line].l_read)(tp, uio));
}

dmzwrite(dev, uio)
	dev_t dev;
	struct uio *uio;
{
	register struct tty *tp;

	tp = &dmz_tty[minor(dev)];
	return ((*linesw[tp->t_line].l_write)(tp, uio));
}

/*
 * DMZ32 receiver interrupt.
 */
dmzrint(dmz, octetnum)
	int dmz, octetnum;
{
	register struct tty *tp;
	register c, flg;
	register struct octet *oaddr;
	register struct tty *tp0;
	struct uba_device *ui;
	register unsigned short line;
	int overrun = 0;
	u_char *modem0, *modem;
	int modem_cont;

	ui = dmzinfo[dmz];  /* dmz is the module number that interrupted */
	if (ui == 0 || ui->ui_alive == 0)
		return;
	/* point to the octet that interrupted */
	oaddr = &((struct dmzdevice *)ui->ui_addr)->octets[octetnum];
	/* tp0 points to the first tty structure on the desired octet */
        line = (dmz*NUMLINES) + (octetnum<<3);  /* Get DMZ number and octet */
        tp0 = &dmz_tty[line];                   /* line number filled in    */
        modem0 = &dmzmodem[line];               /* later. */
	/*
	 * Loop fetching characters from the silo for this
	 * dmz until there are no more in the silo.
	 */
	while ((c = oaddr->dmzrbuf) < 0) {
		line = (c>>8)&07;  /* interrupting line */
		tp = tp0 + line;
		flg = tp->t_iflag;
		modem = modem0 + line;
#ifdef DMZDEBUG
		if (dmzdebug>2)
			mprintf("dmzrint0: oct#=%d, c=%x, oaddr=%x, tp=%x\n",
				octetnum, c, tp->t_addr, tp);
#endif
		if ((c&DMZ_DSC)&& ((tp->t_cflag & CLOCAL) == 0)) {
		/* modem transition and we are not ignoring modem signals*/
			oaddr->dmzcsr = DMZ_IE | DMZIR_TBUF | ((c>>8)&07);
			modem_cont = 0;

#ifdef DMZDEBUG
	if (dmzdebug>1)
		mprintf("dmzrint1: line=%d, md=%x, %x, rms=%x\n", line, modem,
			*modem, oaddr->dmzrms);
#endif


			/*
			 * Drop DTR immediately if DSR gone away.
			 * If really an active close then do not
			 *    send signals.
			 */

			if ((oaddr->dmzrms&DMZ_DSR)==0)  {
				 if (tp->t_state&TS_CLOSING) {
					untimeout(wakeup, (caddr_t) &tp->t_dev);
					wakeup((caddr_t) &tp->t_dev);
#ifdef DMZDEBUG
					if (dmzdebug)
					   mprintf("dmzrint: dsr closing down , tp=%x\n", tp);
#endif
					continue;
				 }
				 if (tp->t_state&TS_CARR_ON) {
					/*
					 * Only drop if DECSTD52 is followed.
					 */
					if (dmzdsr) {
						dmz_tty_drop(tp);
						continue;
					}
				 }
			}
			/*
			 * Check for transient CD drops.
			 * Only drop DTR if CD is down for more than 2 secs.
			 */

			if (tp->t_state&TS_CARR_ON)
			    if ((oaddr->dmzrms&DMZ_CAR)==0) {
				if (*modem & MODEM_CD) {
				    /* only start timer once */
#ifdef DMZDEBUG
				    if (dmzdebug)
					mprintf("dmzrint, cd_drop, tp=%x\n", tp);
#endif
				    *modem &= ~MODEM_CD;
				    dmztimestamp[minor(tp->t_dev)] = time;
				    timeout(dmz_cd_drop, tp, hz*dmzcdtime);
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
					untimeout(dmz_cd_drop, tp);
					if (dmz_cd_down(tp))
						/* drop connection */
						dmz_tty_drop(tp);
					*modem |= MODEM_CD;
				    	modem_cont = 1;
			       }


			/* CTS flow control check */

			if (tp->t_state&TS_CARR_ON)
				if ((oaddr->dmzrms&DMZ_CTS)==0) {
					tp->t_state |= TS_TTSTOP;
					*modem &= ~MODEM_CTS;
#ifdef DMZDEBUG
					if (dmzdebug)
					   mprintf("dmzrint: CTS stop, tp=%x\n", tp);
#endif
					dmzstop(tp, 0);
					continue;
				} else if ((*modem&MODEM_CTS)==0) {
					    tp->t_state &= ~TS_TTSTOP;
					    *modem |= MODEM_CTS;
#ifdef DMZDEBUG
					    if (dmzdebug)
					       mprintf("dmzrint: CTS start, tp=%x\n", tp);
#endif
					    dmzstart(tp);
					    continue;
					}

			/* Avoid calling dmz_start_tty due to a CD transition */
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
			 if (dmzdsr) {
				if ((oaddr->dmzrms&DMZ_XMIT)==DMZ_XMIT
				    && (*modem&MODEM_DSR_START)==0)
					dmz_start_tty(tp);
				else
				    if ((oaddr->dmzrms&DMZ_DSR) &&
					(*modem&MODEM_DSR)==0) {
					/*
					 * we should not look for CTS|CD for
					 * about 500 ms.  lets assume that
					 * there is 200 ms latency for timeouts.
					 */
					*modem |= MODEM_DSR;
					timeout(dmz_dsr_check, tp, hz*30);
					timeout(dmz_dsr_check, tp, hz/2);
				    }
			}
			/* Ignore DSR */
			else
				if ((oaddr->dmzrms&DMZ_NODSR)==DMZ_NODSR)
					dmz_start_tty(tp);
				

			continue;
		} /* end of modem transition tests */

		if ((tp->t_state&TS_ISOPEN)==0) {
			wakeup((caddr_t)tp);
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

		/* DMZ_FE is interpreted as a break */
		if (c & DMZ_FE) {
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
#ifdef DMZDEBUG
				if (dmzdebug)
					mprintf("dmzrint: BREAK RECEIVED\n");
#endif DMZDEBUG
				if ((tp->t_lflag_ext & PRAW) && 
					(tp->t_line != TERMIODISC))
					c = 0;
				else {
				    ttyflush(tp, FREAD|FWRITE);
#ifdef DMZDEBUG
				    if (dmzdebug)
					mprintf("sending signal to tp->t_pgrp = %d\n", tp->t_pgrp);
#endif DMZDEBUG
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
		else if (c & DMZ_PE){
			/* 
			 * If input parity checking is not enabled, clear out
			 * parity error in this character.
			 */
#ifdef DMZDEBUG
			if (dmzdebug > 1)
				mprintf("dmzrint: Parity Error\n");
#endif DMZDEBUG
			if ((flg & INPCK) == 0)
				c &= ~DMZ_PE;
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
		if ((c & DMZ_DO) && overrun == 0) {
			printf("dmz%d: recv. fifo overflow\n", dmz);
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
 * Ioctl for DMZ32.
 */
/*ARGSUSED*/
dmzioctl(dev, cmd, data, flag)
	dev_t dev;
	register int cmd;
	caddr_t data;
	int flag;
{
	register int unit = minor(dev);
	register struct tty *tp = &dmz_tty[unit];
	register int dmz = unit / NUMLINES;
	register struct octet *oaddr;
	struct uba_device *ui = dmzinfo[dmz];
	struct dmz_softc *sc = &dmz_softc[ui->ui_unit];
	struct devget *devget;
	int error, s;

#ifdef DMZDEBUG
	if (dmzdebug)
		mprintf("dmzioctl: oaddr=%x, cmd=%d\n", tp->t_addr, cmd&0xff);
#endif
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
 				dmzparam(unit);
 				break;
 		}
		return (error);
	}
	switch (cmd) {

	case TIOCSBRK:
		(void) dmzmctl(dev, DMZ_BRK, DMBIS);
		break;

	case TIOCCBRK:
		(void) dmzmctl(dev, DMZ_BRK, DMBIC);
		break;

	case TIOCSDTR:
		(void) dmzmctl(dev, DMZ_DTR|DMZ_RTS, DMBIS);
		break;

	case TIOCCDTR:
		(void) dmzmctl(dev, DMZ_DTR|DMZ_RTS, DMBIC);
		break;

	case TIOCMSET:
		(void) dmzmctl(dev, dmtodmz(*(int *)data), DMSET);
		break;

	case TIOCMBIS:
		(void) dmzmctl(dev, dmtodmz(*(int *)data), DMBIS);
		break;

	case TIOCMBIC:
		(void) dmzmctl(dev, dmtodmz(*(int *)data), DMBIC);
		break;

	case TIOCMGET:
		*(int *)data = dmztodm(dmzmctl(dev, 0, DMGET));
		break;

#ifdef 0 
	/* Doesn't work yet! */
	/* Set local loopback mode */
	case TIOCSMLB:
		if (u.u_uid)
			return(EPERM);
		s=spl5();
		oaddr = (struct octet *)tp->t_addr;
		oaddr->dmzcsr = DMZ_IE | DMZIR_LCR | (unit & 07);
		SETLCR(oaddr, oaddr->dmzlcr|DMZ_MAINT);
		splx(s);
		break;

	/* Clear local loopback mode */
	case TIOCCMLB:
		if (u.u_uid)
			return(EPERM);
		s=spl5();
		oaddr = (struct octet *)tp->t_addr;
		oaddr->dmzcsr = DMZ_IE | DMZIR_LCR | (unit & 07);
		SETLCR(oaddr, oaddr->dmzlcr&~DMZ_MAINT);
		splx(s);
		break;
#endif 0

	case TIOCNMODEM:  /* ignore modem status */
		s = spl5();
		dmzsoftCAR[dmz] |= (1<<(unit%NUMLINES));
		if (*(int *)data) /* make mode permanent */
			dmzdefaultCAR[dmz] |= (1<<(unit%NUMLINES));
#ifdef DMZDEBUG
		if (dmzdebug)
			mprintf("dmzioctl:NMODEM unit=%d, flags=%x\n",
				unit, dmzsoftCAR[dmz]);
#endif
		tp->t_state |= TS_CARR_ON;
		dmzmodem[unit] = MODEM_CTS|MODEM_CD|MODEM_DSR;
		tp->t_cflag |= CLOCAL;		/* Map to termio */
		splx(s);
		break;
	case TIOCMODEM:  /* look at modem status - sleep if no carrier */
		s = spl5();
		dmzsoftCAR[dmz] &= ~(1<<(unit%NUMLINES));
#ifdef DMZDEBUG
		if (dmzdebug)
			mprintf("dmzioctl:MODEM unit=%d, flags=%x\n",
				unit, dmzsoftCAR[dmz]);
#endif
		if (*(int *)data) /* make mode permanent */
			dmzdefaultCAR[dmz] &= ~(1<<(unit%NUMLINES));
		oaddr = (struct octet *)tp->t_addr;
		oaddr->dmzcsr = DMZ_IE | DMZIR_TBUF | (unit & 07);
	   	/* 
	    	 * If dmzdsr is set look for DSR|CTS|CD, otherwise look 
	    	 * for CD|CTS only.
	    	 */
		if ((dmzdsr && ((oaddr->dmzrms & DMZ_XMIT)==DMZ_XMIT)) ||
		   ((dmzdsr==0) && ((oaddr->dmzrms & DMZ_NODSR)==DMZ_NODSR))) {
			tp->t_state |= TS_CARR_ON;
			tp->t_state &= ~TS_ONDELAY;
			dmzmodem[unit] = MODEM_CTS|MODEM_CD|MODEM_DSR;
		}
		else {
			tp->t_state &= ~(TS_CARR_ON);
			dmzmodem[unit] &= ~(MODEM_CTS|MODEM_CD|MODEM_DSR);
		}
		tp->t_cflag &= ~CLOCAL;		/* Map to termio */
		splx(s);
		break;
	case TIOCWONLINE:
		s = spl5();
		oaddr = (struct octet *)tp->t_addr;
		oaddr->dmzcsr = DMZ_IE | DMZIR_TBUF | (unit & 07);
	   	/* 
	    	 * If dmzdsr is set look for DSR|CTS|CD, otherwise look 
	    	 * for CD|CTS only.
	    	 */
		if ((dmzdsr && ((oaddr->dmzrms & DMZ_XMIT)==DMZ_XMIT)) ||
		   ((dmzdsr==0) && ((oaddr->dmzrms & DMZ_NODSR)==DMZ_NODSR))) {
			tp->t_state |= TS_CARR_ON;
			tp->t_state &= ~TS_ONDELAY;
			dmzmodem[unit] = MODEM_CTS|MODEM_CD|MODEM_DSR;
		}
		else
			while ((tp->t_state & TS_CARR_ON) == 0) {
#ifdef DMZDEBUG
			if (dmzdebug)
				mprintf("dmzioctl: wait, unit=%d\n", unit);
#endif
			sleep((caddr_t)&tp->t_rawq, TTIPRI);
			}
		splx(s);
		break;

	case DEVIOCGET: 			/* device status */
		devget = (struct devget *)data;
		bzero(devget,sizeof(struct devget));
 
		if (tp->t_cflag & CLOCAL) {
			sc->sc_category_flags[unit%NUMLINES] |= DEV_MODEM;
			sc->sc_category_flags[unit%NUMLINES] &= ~DEV_MODEM_ON;
		}
		else
			sc->sc_category_flags[unit%NUMLINES] |= (DEV_MODEM|DEV_MODEM_ON);
 
		devget->category = DEV_TERMINAL;
		devget->bus = DEV_UB;
		bcopy(DEV_DMZ32,devget->interface,
		      strlen(DEV_DMZ32));
		bcopy(DEV_UNKNOWN,devget->device,
		      strlen(DEV_UNKNOWN));		/* terminal	*/
		devget->adpt_num = ui->ui_adpt; 	/* which adapter*/
		devget->nexus_num = ui->ui_nexus;	/* which nexus	*/
		devget->bus_num = ui->ui_ubanum;	/* which UBA	*/
		devget->ctlr_num = dmz; 		/* which interf.*/
		devget->slave_num = unit%NUMLINES;	/* which line	*/
		bcopy(ui->ui_driver->ud_dname,
		      devget->dev_name,
		      strlen(ui->ui_driver->ud_dname)); /* Ultrix "dmz" */
		devget->unit_num = unit%NUMLINES;	/* dmz line?	*/
		devget->soft_count =
		      sc->sc_softcnt[unit%NUMLINES];	/* soft er. cnt.*/
		devget->hard_count =
		      sc->sc_hardcnt[unit%NUMLINES];	/* hard er cnt. */
		devget->stat = sc->sc_flags[unit%NUMLINES]; /* status	*/
		devget->category_stat =
		      sc->sc_category_flags[unit%NUMLINES]; /* cat. stat. */
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
		dmzparam(unit);
		break;

	default:
		if (u.u_procp->p_progenv == A_POSIX) 
			return (EINVAL);
		return (ENOTTY);
	}
	return (0);
}

dmtodmz(bits)
	register int bits;
{
	register int b;

	b = bits & 012;
	if (bits & DML_ST) b |= DMZ_RATE;
	if (bits & DML_RTS) b |= DMZ_RTS;
	if (bits & DML_USR) b |= DMZ_USRW;
	return(b);
}

dmztodm(bits)
	register int bits;
{
	register int b;

	b = (bits & 012) | ((bits >> 7) & 0760) | DML_LE;
	if (bits & DMZ_USRR) b |= DML_USR;
	if (bits & DMZ_RTS) b |= DML_RTS;
	return(b);
}


/*
 * Set parameters from open or stty into the DMZ hardware
 * registers.
 */
dmzparam(unit)
	register int unit;    /* unit is the minor device number */
{
	register struct tty *tp;
	register struct octet *oaddr;
	register int lpar, lcr;
	int s;

	tp = &dmz_tty[unit];
	oaddr = (struct octet *)tp->t_addr;
	/*
	 * Block interrupts so parameters will be set
	 * before the line interrupts.
	 */
	s = spl5();
	oaddr->dmzcsr = (unit&07) | DMZIR_LCR | DMZ_IE;
	/* 
 	 * Disconnect modem line if baudrate is zero.  POSIX checks the output
	 * baud rate, while non-POSIX checks the input baud rate.
	 */
	if ((((tp->t_cflag&CBAUD)==B0) && (u.u_procp->p_progenv != A_POSIX)) || 
	    (((tp->t_cflag_ext & CBAUD)==B0) && 
		(u.u_procp->p_progenv == A_POSIX))) { 
		tp->t_cflag |= HUPCL;
		(void) dmzmctl(unit, DMZ_OFF, DMSET);
		splx(s);
		return;
	}
	lpar = (dmz_speeds[tp->t_cflag_ext&CBAUD]<<12) | 	/* ospeed */
		(dmz_speeds[tp->t_cflag&CBAUD]<<8);		/* ispeed */
	lcr = DMZLCR_ENA;
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
 	 * System V termio functionality.
	 * Set device registers according to the specifications of the termio
	 * structure.
 	 */
 	if (tp->t_cflag & CREAD)
 		lcr |= DMZ_RE;
	else
 		lcr &= ~DMZ_RE;
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
		lcr |= DMZ_AUTOX;
	lpar |= (unit&07);
	oaddr->dmzlpr = lpar;
	SETLCR(oaddr, lcr);
#ifdef DMZDEBUG
	if (dmzdebug)
		mprintf("dmzparam: lpr=%x, lcr=%x, oaddr=%x\n",
			oaddr->dmzlpr, oaddr->dmzun.dmzirw, oaddr);
#endif
	splx(s);
}

/*
 * DMZ32 transmitter interrupt.
 * Restart the idle line.
 */


dmzxint(dmz, octetnum)
	int dmz, octetnum;  /* dmz is the module that interrupted */
			    /* octetnum is the octet within the module */
{
	struct tty *tp0;
	register struct tty *tp;
	register struct octet *addr;
	register struct uba_device *ui;
	register int t;
	short cntr;
        /*
         * Index to propper tty structure by dmz number and octet.  Later move
         * pointer up for the respective line number.
         */
        int unit0 = (dmz * NUMLINES) + (octetnum<<3);

	ui = dmzinfo[dmz];

	addr = &((struct dmzdevice *)ui->ui_addr)->octets[octetnum];
	tp0 = &dmz_tty[unit0];


	while ((t = addr->dmzcsr) & DMZ_TI) {
		if (t & DMZ_NXM)
			/* SHOULD RESTART OR SOMETHING... */
			printf("dmz%d: NXM line %d\n", dmz, t >> 8 & 7);
		t = t >> 8 & 7;
		tp = tp0 + t;
		tp->t_state &= ~TS_BUSY;
		if (tp->t_state&TS_FLUSH)
			tp->t_state &= ~TS_FLUSH;
		else if (dmz_dma[unit0 + t]) {
			/*
			 * Do arithmetic in a short to make up
			 * for lost 16&17 bits.
			 */
			addr->dmzcsr = DMZIR_TBA | DMZ_IE | t;
			cntr = addr->dmztba -
			    UBACVT(tp->t_outq.c_cf, ui->ui_ubanum);
			ndflush(&tp->t_outq, (int)cntr);
		}
		if (tp->t_line)
			(*linesw[tp->t_line].l_start)(tp);
		else
			dmzstart(tp);
	}
}




/*
 * Start (restart) transmission on the given DMZ32 line.
 */

dmzstart(tp)
	register struct tty *tp;
{
	register struct octet *addr;
	register int unit, nch, line;
	int s;
	register int dmz;

	unit = minor(tp->t_dev);
	dmz = unit / NUMLINES;
	line = unit & 07;  /* specific line on octetnum */
	addr = (struct octet *)tp->t_addr;

	/*
	 * Must hold interrupts in following code to prevent
	 * state of the tp from changing.
	 */
	s = spl5();
	/*
	 * If it's currently active, or delaying, no need to do anything.
	 * Also dont transmit if not CTS.
	 */
	if ((tp->t_state&(TS_TIMEOUT|TS_BUSY|TS_TTSTOP)) ||
		 ((tp->t_state & TS_CARR_ON) && (dmzmodem[unit]&MODEM_CTS)==0))
		goto out;
	/*
	 * If there are still characters in the silo,
	 * just reenable the transmitter.
	 */
	addr->dmzcsr = DMZ_IE | DMZIR_TBUF | line;
	if (addr->dmztsc) {
		addr->dmzcsr = DMZ_IE | DMZIR_LCR | line;
                /*
                 * If outgoing auto flow control is enabled, the hardware will
                 * control the transmit enable bit.
                 */
                if ((tp->t_cflag_ext & PAUTOFLOW) == 0)
			SETLCR(addr, addr->dmzlcr|DMZ_TE);
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
	if (nch >= dmz_mindma) {
		register car;

		dmz_dma[minor(tp->t_dev)] = 1;
		addr->dmzcsr = DMZ_IE | DMZIR_LCR | line;
                /*
                 * If outgoing auto flow control is enabled, the hardware will
                 * control the transmit enable bit.
                 */
		if ((tp->t_cflag_ext & PAUTOFLOW) == 0)
			SETLCR(addr, addr->dmzlcr|DMZ_TE);
		car = UBACVT(tp->t_outq.c_cf, dmzinfo[dmz]->ui_ubanum);
		addr->dmzcsr = DMZ_IE | DMZIR_TBA | line;
		/*
		 * Give the device the starting address of a DMA transfer.  
		 * Translate the system virtual address into a physical
		 * address.
		 */
		addr->dmztba = svtophy(car);
		addr->dmztcc = ((car >> 2) & 0xc000) | nch;
		tp->t_state |= TS_BUSY;
	} else if (nch) {
		register char *cp = tp->t_outq.c_cf;
		register int i;

		dmz_dma[minor(tp->t_dev)] = 0;
		nch = MIN(nch, DMZ_SILOCNT);
		addr->dmzcsr = DMZ_IE | DMZIR_LCR | line;
                /*
                 * If outgoing auto flow control is enabled, the hardware will
                 * control the transmit enable bit.
                 */
                if ((tp->t_cflag_ext & PAUTOFLOW) == 0)
			SETLCR(addr, addr->dmzlcr|DMZ_TE);
		addr->dmzcsr = DMZ_IE | DMZIR_TBUF | line;
		for (i = 0; i < nch; i++)
			addr->dmztbuf = *cp++;
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
dmzstop(tp, flag)
	register struct tty *tp;
{
	register struct octet *oaddr;
	register int unit, s;

	unit = minor(tp->t_dev);
	unit &= 07;   /* line number on octet */
	oaddr = (struct octet *)tp->t_addr;
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
		oaddr->dmzcsr = DMZIR_LCR | unit | DMZ_IE;
		/*
		 * If this test isn't done, the hardware will need to see a
		 * start character to get it going again if DMZ_AUTOX is set.
		 */
                if ((tp->t_cflag_ext & PAUTOFLOW) == 0)
			SETLCR(oaddr, oaddr->dmzlcr &~ DMZ_TE);
		if ((tp->t_state&TS_TTSTOP)==0) {
			tp->t_state |= TS_FLUSH;
			SETLCR(oaddr, oaddr->dmzlcr|DMZ_FLUSH);
		} else
			tp->t_state &= ~TS_BUSY;
	}
	splx(s);
}

/*
 * DMZ32 modem control
 */
dmzmctl(dev, bits, how)
	dev_t dev;
	int bits, how;
{
	register struct octet *dmzaddr;
	register int line, mbits, lcr;
	int unit;
	int s;

	unit = minor(dev);
	dmzaddr = (struct octet *)(dmz_tty[unit].t_addr);
	line = unit & 07;  /* line number */
	s = spl5();
	dmzaddr->dmzcsr = DMZ_IE | DMZIR_TBUF | line;
	mbits = dmzaddr->dmzrms << 8;
	dmzaddr->dmzcsr = DMZ_IE | DMZIR_LCR | line;
	mbits |= dmzaddr->dmztms;
	lcr = dmzaddr->dmzlcr;
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
	if (mbits & DMZ_BRK)
		lcr |= DMZ_RBRK;
	else
		lcr &= ~DMZ_RBRK;
	lcr = ((mbits & 037) << 8) | (lcr & 0xff);
	dmzaddr->dmzun.dmzirw = lcr;
	(void) splx(s);
#ifdef DMZDEBUG
	if (dmzdebug)
		mprintf("dmzmctl: unit=%d, l#=%d, lcr=%x, oaddr=%x\n",
			unit, line, dmzaddr->dmzun.dmzirw, dmzaddr);
#endif
	return(mbits);
}

/*
 * Reset state of driver if UBA reset was necessary.
 * Reset the csr, lpr, and lcr registers on open lines, and
 * restart transmitters.
 */
dmzreset(uban)
	int uban;
{
	register int dmz, unit;
	register struct tty *tp;
	register struct uba_device *ui;
	register struct octet *oaddr;
	int i, o;

	if (tty_ubinfo[uban] == 0)
		return;
	cbase[uban] = tty_ubinfo[uban]&0x3ffff;
	for (dmz = 0; dmz < nNDMZ; dmz++) {  /* for each dmz module */
		ui = dmzinfo[dmz];
		if (ui == 0 || ui->ui_alive == 0 || ui->ui_ubanum != uban)
			continue;
		printf(" dmz%d", dmz);
		unit = (dmz << 5);
		for (o = 0; o < 3; o++) {  /* for each octet */
			oaddr = &((struct dmzdevice *)ui->ui_addr)->octets[o];
			oaddr->dmzcsr = DMZ_IE;
			oaddr->dmzrsp = dmz_timeout;  /* receive timer should be higher */
			for (i = 0; i < 8; i++) {  /* for each line */
				tp = &dmz_tty[unit];
				if (tp->t_state & (TS_ISOPEN|TS_WOPEN)) {
					dmzparam(unit);
					(void) dmzmctl(unit, DMZ_ON, DMSET);
					tp->t_state &= ~TS_BUSY;
					dmzstart(tp);
				}
				unit++;
			}
		}
	}
}

/* octet interrupt handlers */
/* In version 1.2 these are not used - dmzr/xint is called with the octet number */

dmzrinta(dmz)
	int dmz;
{
	dmzrint(dmz, 0);
}
dmzxinta(dmz)
	int dmz;
{
	dmzxint(dmz, 0);
}



dmzrintb(dmz)
	int dmz;
{
	dmzrint(dmz, 1);
}
dmzxintb(dmz)
	int dmz;
{
	dmzxint(dmz, 1);
}



dmzrintc(dmz)
	int dmz;
{
	dmzrint(dmz, 2);
}
dmzxintc(dmz)
	int dmz;
{
	dmzxint(dmz, 2);
}



dmz_cd_drop(tp)
register struct tty *tp;
{
	register struct octet *addr = (struct octet *)tp->t_addr;
	register unit = minor(tp->t_dev);

	addr->dmzcsr = DMZ_IE | DMZIR_TBUF | (unit&07);
	if ((tp->t_state&TS_CARR_ON) &&
		((addr->dmzrms&DMZ_CAR) == 0)) {
#ifdef DMZDEBUG
		if (dmzdebug)
		    mprintf("dmz_cd:  no CD, tp=%x, unit=%x, addr=%x\n",
			tp, unit, addr);
#endif
		dmz_tty_drop(tp);
		return;
	}
	dmzmodem[unit] |= MODEM_CD;
#ifdef DMZDEBUG
	if (dmzdebug)
	    mprintf("dmz_cd:  CD is up, tp=%x\n", tp);
#endif
}


dmz_dsr_check(tp)
register struct tty *tp;
{
	int unit = minor(tp->t_dev);
	register struct octet *addr = (struct octet *)tp->t_addr;
	if (dmzmodem[unit]&MODEM_DSR_START) {
#ifdef DMZDEBUG
		if (dmzdebug)
		    mprintf("dmz_dsr_check0:  tp=%x\n", tp);
#endif
		dmzmodem[unit] &= ~MODEM_DSR_START;
		addr->dmzcsr = DMZ_IE | DMZIR_TBUF | (unit&07);
	   	/* 
	    	 * If dmzdsr is set look for DSR|CTS|CD, otherwise look 
	    	 * for CD|CTS only.
	    	 */
		if (dmzdsr) {
			if ((addr->dmzrms&DMZ_XMIT)==DMZ_XMIT)
				dmz_start_tty(tp);
		}
		else {
			if ((addr->dmzrms&DMZ_NODSR)==DMZ_NODSR)
				dmz_start_tty(tp);
		}
		return;
	}
	if ((tp->t_state&TS_CARR_ON)==0)  {
		dmz_tty_drop(tp);
#ifdef DMZDEBUG
		if (dmzdebug)
		    mprintf("dmz_dsr_check:  no carrier, tp=%x\n", tp);
#endif
	}
#ifdef DMZDEBUG
	else
		if (dmzdebug)
		    mprintf("dmz_dsr_check:  carrier is up, tp=%x\n", tp);
#endif
}

/*
 *  cd_down return 1 if carrier has been down for at least 2 secs.
 */
dmz_cd_down(tp)
struct tty *tp;
{
	int msecs;
	register int unit = minor(tp->t_dev);


	msecs = 1000000 * (time.tv_sec - dmztimestamp[unit].tv_sec) +
		(time.tv_usec - dmztimestamp[unit].tv_usec);
	if (msecs > 2000000)
		return(1);
	else
		return(0);
}

dmz_tty_drop(tp)
struct tty *tp;
{
	register struct octet *addr = (struct octet *)tp->t_addr;
	register int line;
 	register int unit;
#ifdef DMZDEBUG
		if (dmzdebug)
			mprintf("dmz_start_tty:  drop tty , tp=%x, flags=%x\n", tp, tp->t_flags);
#endif
	if (tp->t_flags&NOHANG)
		return;
 	unit = minor(tp->t_dev);
 	line = unit & 07;
 	dmzmodem[unit] = MODEM_BADCALL;
 	tp->t_state &= ~(TS_CARR_ON|TS_TTSTOP|TS_BUSY|TS_ISUSP);
 	wakeup((caddr_t)&tp->t_rawq);
	gsignal(tp->t_pgrp, SIGHUP);
	gsignal(tp->t_pgrp, SIGCONT);
	addr->dmzcsr=DMZ_IE | DMZIR_LCR | line;
	addr->dmztms = 0;
}

dmz_start_tty(tp)
	register struct tty *tp;
{
	register int unit = minor(tp->t_dev);
	if (tp->t_state&TS_CARR_ON) {
#ifdef DMZDEBUG
		if (dmzdebug)
			mprintf("dmz_start_tty:  carr on, tp=%x\n", tp);
#endif
		return;
	}
	tp->t_state &= ~TS_ONDELAY;
	tp->t_state |= TS_CARR_ON;
	if (dmzmodem[unit]&MODEM_DSR)
		untimeout(dmz_dsr_check, tp);
	dmzmodem[unit] |= MODEM_CD|MODEM_CTS|MODEM_DSR;
	dmztimestamp[unit] = dmzzerotime;
	wakeup((caddr_t)&tp->t_rawq);
}

dmzbaudrate(speed)
register int speed;
{
    if (dmz_valid_speeds & (1 << speed))
	return (1);
    else
	return (0);
}
#endif

