#ifndef lint
static char *sccsid = "@(#)sh.c	4.1	(ULTRIX)	7/2/90";
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
 * sh.c
 *
 * MicroVAX 2000 serial line expander (8 line SLU) driver
 *
 * Modification history
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
 * 30-May-89 - Darrell Dunnuck
 *	Changed check of system type to check for system type VAXSTAR.
 *
 * 25-Jan-89 - Randall Brown
 *
 *	Changed cd_drop to look at LNOHANG.  Changed close routine to look
 *	at HUPCL.
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
 *  16-May-88 - Tim Burke
 *
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
 *      Fixed DEVIOCGET to use LINEMASK where appropriate to correctly
 *      select line number off of unit.
 *
 *  7-Sep-87 - rsp
 *
 *      Added code in DEVIOCGET to "&" in LINEMASK with unit to
 *      correctly determine line number.
 *
 *  2-Sept-87 - Tim Burke
 *
 *	Added support for hardware auto flow control on the outgoing side.  This
 *	will provide quick response to start/stop characters which will reduce
 *	buffer overflow on the receiving device.
 *
 * 24-Mar-87 - Tim Burke
 *
 *	Added full TERMIO functionality to terminal subsystem.
 *
 *  6-Feb-87 - Tim Burke
 *
 *	Removed printf of master reset failure in probe routine, as it may be
 *	incorrectly appearing. (Particularly in the DMF & DMZ drivers)
 *	This catches sh.c up to all changes in dhu.c thru delta 1.31.
 *
 * 28-Jan-87 - Tim Burke
 *
 *	Added the capability to ignore the "DSR" modem signal.  This is being
 *	done to allow modems that do not follow DEC Standard 52 to still 
 *	function as they would have prior to the addition of DECSTD52 code
 *	into the drivers.  If the driver is setup to ignore "DSR" then it will
 *	not be following DECSTD52.  To follow DECSTD52 set shdsr to "1", to
 *	ignore "DSR" set shdsr to be "0";
 *	This catches sh.c up to all changes in dhu.c thru delta 1.30.
 *
 *  23-Jan-87 -- tim  (Tim Burke)
 *	Bug fix in shclose to prevent lines from hanging after another line has
 *	closed.  The problem is a result of not setting the correct line number
 *	in the csr and masking interrupts.
 *	This catches sh.c up to all changes in dhu.c thru delta 1.29.
 *
 *  18-Jan-87 -- fred (Fred Canter)
 *	Added Tim Burke's latest modem control change:
 *	"Bug fix to TIOCMODEM to clear modem flags if signals are not up".
 *	This catches sh.c up to all changes in dhu.c thru delta 1.28.
 *	Removed unused variable and one line of code from shxint().
 *	Replaced referrences to dhu with sh in comments.
 *
 *  15-Jan-87 -- fred (Fred Canter)
 *	Fixed a bug in the two character line parameter change
 *	delay added in the last modification.
 *
 *   7-Jan-87 -- fred (Fred Canter)
 *	Added a two character delay before changing line parameters,
 *	to get rid of extra characters printed after the login prompt.
 *	Cleaned out all my debug code.
 *	The sh driver is now up to date with shu.c delta 1.27.
 *
 *  16-Dec-86 -- fred (Fred Canter)
 *	Changes for removal of DEV_MODEM_OFF devioctl flag.
 *	Improved driver (fixed flow control).
 *
 *   4-Dec-86 -- fred (Fred Canter)
 *	Converted first pass driver into a real driver.
 *	Cought up to data with all changes in dhu.c (delta 1.26).
 *
 *   3-Sep-86 -- fred (Fred Canter)
 *	Cleaned up some comments.
 *	Only configure the sh device on a MicroVAX 2000.
 *	Select sh as interrupt source instead of video board.
 *
 *  26-Aug-86 -- rsp (Ricky Palmer)
 *	Cleaned up devioctl code to (1) zero out devget structure
 *	upon entry, (2) use strlen instead of fixed storage
 *	for bcopy's, and (3) updated DEVIOCGET request code.
 *
 *   2-Jul-86 -- fred (Fred Canter)
 *	Created this first pass driver for MicroVAX 2000
 *	serial line expander (8 line SLU).
 *	Derived from dhu.c (delta 1.17).
 *
 */

/*
 * Implementation notes:
 *
 *	Multiunit support maintained (except for unit CSR address)
 *	for possible future expansion.
 *
 *	Decstandard 52 modem control support remains in tact,
 *	in case modem control is ever added to this device.
 *	The modem control code is disabled if bit 2 (SH_MSTAT) in
 *	the line status register is set, which it is for the
 *	current implementation of the serial line expander hardware.
 *
 *	Complete deactivation of the latent modem control code depends
 *	on the flags in the config file being set to 0x0ff (all lines
 *	local mode). Turning off any of the flag bit would probably
 *	cause the line to hang waiting for carrier.
 *
 *	This driver was derived from the DHU11 driver. The DHU11
 *	register names and bit names were used for consistency and
 *	to allow easy porting of dhu.c changes to sh.c.
 *
 *	WARNING:
 *		The sh device (really a dhu11 chip) requires a delay of
 *		two character times after a transmitter action interrupt
 *		before the modem signals or line parameters can be
 *		changed. This driver handles this restriction for line
 *		parameter changes, but not for modem control changes.
 *		If modem control is ever added to the hardware, this
 *		restriction must be taken in to account.
 *
 */

#include "sh.h"
#if NSH > 0  || defined(BINARY)

#include "../data/sh_data.c"

int shdebug = 0;
int shcdtime = 2;


char sh_speeds[] = {
	0, 0, 01, 02, 03, 04, 0, 05,
	06, 07, 010, 012, 013, 015, 016, 0
	};

short	sh_valid_speeds = 0xffbf; /* 1,1,1,1, 1,1,1,1, 1,0,1,1, 1,1,1,1 */

/*
 * Number of microseconds per character for each bit
 * rate in the sh_speeds[] tables. Used in shparam()
 * to ensure a delay of two character times between
 * a transmitter interrupt and the driver changing the
 * line parameters. The hardware tells us it is done
 * transmitting two characters before it really is done.
 */

#define	SHDTSIZE	16	/* size of delay table */

int sh_delays[] = {
	0,		/* 0 */
	500000,		/* 50 */
	133333,		/* 75 */
	100000,		/* 110 */
	67159,		/* 134.5 */
	66667,		/* 150 */
	50000,		/* 200 - not supported by the hardware */
	33333,		/* 300 */
	16667,		/* 600 */
	8333,		/* 1200 */
	5556,		/* 1800 */
			/* 2000 - NOT in bit rate table! */
	4167,		/* 2400 */
	2083,		/* 4800 */
			/* 7200 - not in bit rate table! */
	1042,		/* 9600 */
	521,		/* 19200 */
	260,		/* 38400 - not allowed by software */
};

/*
 * This array holds the new value to be loaded into
 * the line parameter register after the two character
 * timeout (see above) has expired.
 */

int	sh_lpr_load();
unsigned short sh_newlpr[8];

/*
 * Soft copy of CSR low byte. We need this
 * because any read of the CSR can cause a loss
 * of a transmitter action interrupt.
 */

char	sh_softcsr;

/*
 * Definition of the driver for the auto-configuration program.
 */
int	shprobe(), shattach(), shrint(), shxint(), shbaudrate();
int	sh_cd_drop(), sh_dsr_check(), sh_cd_down(), sh_tty_drop();
struct	timeval shzerotime = {0,0};
u_short shstd[] = { 0 };
struct	uba_driver shdriver =
	{ shprobe, 0, shattach, 0, shstd, "sh", shinfo };

int	shstart(), ttrstrt();

#ifndef MODEM_CD
#define MODEM_CD   0x01
#define MODEM_DSR  0x02
#define MODEM_CTS  0x04
#define MODEM_DSR_START  0x08
#endif

#define LINEMASK 0x07	/* mask of higher bits of csr to get a line # */

extern	struct	nexus	nexus[];

/*
 * Routine for configuration to force a sh to interrupt.
 * Set to transmit at 9600 baud, and cause a transmitter interrupt.
 */
/*ARGSUSED*/
shprobe(reg)
	caddr_t reg;		/* NOT USED */
{
	register struct nb_regs *shiaddr = (struct nb_regs *)nexus;
	register struct shdevice *shaddr = (struct shdevice *)shmem;
	int totaldelay; 		/* Self-test timeout counter */

#ifdef lint
	if (sh_cnt == 0) sh_cnt = 1;
	shrint(0); shxint(0);
#endif
	if(shdebug)
		printf("shprobe\n");
	/*
	 * Only on a MicroVAX 2000 and
	 * only if serial line expander installed.
	 * Also on CVAXstar (assume there will be a CTEAMMATE).
	 * CAUTION: watchout for MULTU bit meaning change on CVAXstar!
	 */
	if (((cpu != VAXSTAR) && (cpu != C_VAXSTAR)) ||
	    ((vs_cfgtst&VS_MULTU) == 0) || ((vs_cfgtst&VS_VIDOPT) == 0))
		return(0);
	shiaddr->nb_vdc_sel = 1;	/* select serial option interrupts */
	/*
	 * If a self test is not being done, start one up.  Wait for the
	 * self-test (to a max of 3.5 sec.) to complete before interrupting.
	 * SLU spec. says max of 2 seconds for self test to complete.
	 */
	if ((shaddr->csr.low & SH_MRESET) == 0)
	    shaddr->csr.low = SH_MRESET;
	totaldelay = 0;
	while ( (shaddr->csr.low & SH_MRESET) && ( totaldelay <= 70) ){
	    totaldelay++;
	    DELAY(50000);
	}
	if (shaddr->csr.low & SH_MRESET)
	    printf("sh0: hardware failed to exit self-test\n");
	else if (shaddr->csr.high & SH_DIAGFAIL)
	    printf("se0: hardware failed self-test\n");

	/*
	 * Clear intr cntlr req bits and set enable bits
	 */
	shiaddr->nb_int_reqclr = (SINT_VF|SINT_VS);
	shiaddr->nb_int_msk |= (SINT_VF|SINT_VS);

	shaddr->csr.low = 0;  /* transmit on channel 0 */
	sh_softcsr = 0;
	shaddr->csr.high = SH_XIE; /* enable transmit interrupts */
	shaddr->lpr = sh_speeds[B9600] << 12 | SH_BITS7 | SH_PENABLE;
	shaddr->tbuffad2.high = SH_XEN;
	shaddr->fun.fs.fifosize = (char) 0;
	DELAY(100000);		/* wait 1/10'th of a sec for interrupt */
	{ char temp = shaddr->csr.high; /* clear transmit action */ }
	shaddr->csr.high = 0;	 /* disable transmit interrupts */
	shiaddr->nb_int_reqclr = (SINT_VF|SINT_VS);
	if (cvec && cvec != 0x200) /* check to see if interrupt occurred */
		cvec -= 4;	   /* point to first interrupt vector (recv)*/
	return (1);	/* not sizeof anything, just says probe succeeded */
}

/*
 * Routine called to attach a sh.
 */
shattach(ui)
	struct uba_device *ui;
{
#ifdef SHDEBUG
	if(shdebug)
		printf("shattach %x, %d\n", ui->ui_flags, ui->ui_unit);
#endif SHDEBUG
	shsoftCAR[ui->ui_unit] = ui->ui_flags;
	shdefaultCAR[ui->ui_unit] = ui->ui_flags;
}


/*
 * Open an SH line.
 * Turn on this sh if this is the first use of it.
 */
/*ARGSUSED*/
shopen(dev, flag)
	dev_t dev;
{
	register struct tty *tp;
	register int unit, sh;
	register struct shdevice *addr;
	register struct uba_device *ui;
	int s;
	int inuse;  /*hold state of inuse bit while blocked waiting for carr*/

	unit = minor(dev);
	sh = unit >> 3;
	if (unit >= nNSH*8 || (ui = shinfo[sh])== 0 || ui->ui_alive == 0)
		return (ENXIO);
	tp = &sh_tty[unit];
	if (tp->t_state&TS_XCLUDE && u.u_uid!=0)
		return (EBUSY);
#ifdef SHDEBUG
	if (shdebug)
		mprintf("shopen:  line=%d, state=%x, pid=%d\n", unit,
			tp->t_state, u.u_procp->p_pid);
#endif
	while (tp->t_state&TS_CLOSING) /* let DTR stay down for awhile */
		sleep((caddr_t)&tp->t_rawq, TTIPRI);
	addr = (struct shdevice *)shmem;

	tp->t_addr = (caddr_t)addr;
	tp->t_oproc = shstart;
	tp->t_baudrate = shbaudrate;
	tp->t_state |= TS_WOPEN;

	if ((tp->t_state&TS_ISOPEN) == 0) {
	    shmodem[unit] = MODEM_DSR_START; /* prevents spurious startups */
	}
	tty_def_open(tp, dev, flag, (shsoftCAR[sh]&(1<<(unit&LINEMASK))));
	shparam(unit);

	/*
	 * Wait for carrier, then process line discipline specific open.
	 */

	/*
	 * Set up receive hold off timer. Manual says we have
	 * to point to line 0 before we set the timer.
	 */
	addr->csr.low = SH_RIE|(0 & LINEMASK);	/* set to line 0 */
	sh_softcsr = SH_RIE|(0 & LINEMASK);
	addr->run.rxtimer = 10;			/* 10 MS hold off time */

	s=spl5();
	if (tp->t_cflag & CLOCAL) {
		/* this is a local connection - ignore carrier */
		tp->t_state |= TS_CARR_ON;
		shmodem[unit] |= MODEM_CTS|MODEM_CD|MODEM_DSR;
		addr->csr.low = SH_RIE|(unit & LINEMASK); /* set to line #*/
		sh_softcsr = SH_RIE|(unit & LINEMASK); /* set to line #*/
							  /* enable interrupts*/
		addr->lnctrl &= ~(SH_MODEM);
		addr->lnctrl |= (SH_DTR|SH_RTS|SH_REN);
		/*
	 	 * Set state bit to tell tty.c not to assign this line as the 
	 	 * controlling terminal for the process which opens this line.
	 	 */
		if ((flag & O_NOCTTY) && (u.u_procp->p_progenv == A_POSIX))
			tp->t_state |= TS_ONOCTTY;
		splx(s);
		return ((*linesw[tp->t_line].l_open)(dev, tp));
	}
/* NOTREACHED (softCAR and CLOCAL always set, i.e., no modem control) */
	addr->csr.low = SH_RIE|(unit & LINEMASK);
	sh_softcsr = SH_RIE|(unit & LINEMASK);
	addr->lnctrl |= (SH_DTR|SH_RTS|SH_MODEM|SH_REN);

	if ((flag & (O_NDELAY|O_NONBLOCK)) == 0) {
		if (shdsr) {
			if ((addr->fun.fs.stat)&SH_DSR) {
				shmodem[unit] |= (MODEM_DSR_START|MODEM_DSR);
				tp->t_dev = dev; /* need it for timeouts */
				timeout(sh_dsr_check, tp, hz*30);
				timeout(sh_dsr_check, tp, hz/2);
			}
		}
		/*
		 * Ignore DSR and immediately look for CD & CTS.
		 */
		else {
			shmodem[unit] |= (MODEM_DSR_START|MODEM_DSR);
			sh_dsr_check(tp);
		}
	}
#ifdef SHDEBUG
	if (shdebug)
		mprintf("shopen:  line=%d, state=%x, pid=%d\n", unit,
			tp->t_state, u.u_procp->p_pid);
#endif
	if (flag & (O_NDELAY|O_NONBLOCK))
		tp->t_state |= TS_ONDELAY;
	else
		while ((tp->t_state&TS_CARR_ON)==0) {
			inuse = tp->t_state&TS_INUSE;
			sleep((caddr_t)&tp->t_rawq, TTIPRI);
 			/*
 			 * See if wakeup is due to a false call.
 			 */
 			if (shmodem[unit]&MODEM_BADCALL){
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
 * Close a SH line.
 */
/*ARGSUSED*/
shclose(dev, flag)
	dev_t dev;
	int flag;
{
	register struct tty *tp;
	register int unit, s;
	register struct shdevice *addr;
	register sh;
	extern int wakeup();
	int turnoff = 0;

	unit = minor(dev);
	sh = unit >> 3;
	tp = &sh_tty[unit];
	addr = (struct shdevice *)tp->t_addr;
	tp->t_state |= TS_CLOSING;
	if (tp->t_line)
		(*linesw[tp->t_line].l_close)(tp);
	if ((tp->t_cflag&HUPCL) || (tp->t_state&TS_ISOPEN)==0) {
		s = spl5();
		turnoff++;
		addr->csr.low = SH_RIE|(unit & LINEMASK);
		sh_softcsr = SH_RIE|(unit & LINEMASK);
		addr->lnctrl &= ~(SH_DTR|SH_RTS);  /* turn off DTR */
		tp->t_state &= ~TS_CARR_ON; /* prevents recv intr. timeouts */
		if ((tp->t_cflag & CLOCAL) == 0) {
#ifdef SHDEBUG
	if (shdebug)
		mprintf("shclose: DTR drop line=%d, state=%x, pid=%d\n", unit,
			tp->t_state, u.u_procp->p_pid);
#endif

			/* wait an additional 5 seconds for DSR to drop */
			if (shdsr && (addr->fun.fs.stat&SH_DSR)) {
				timeout(wakeup, (caddr_t) &tp->t_dev, 5*hz);
				sleep((caddr_t)&tp->t_dev, PZERO-10);
			}
			timeout(wakeup, (caddr_t) &tp->t_dev, hz/5);
			sleep((caddr_t)&tp->t_dev, PZERO-10);
		}
		splx(s);
	}
#ifdef SHDEBUG
	if (shdebug)
		mprintf("shclose: line=%d, state=%x, pid=%d\n", unit,
			tp->t_state, u.u_procp->p_pid);
#endif
	shsoftCAR[sh] &= ~(1<<(unit&LINEMASK));
	shsoftCAR[sh] |= (1<<(unit&LINEMASK)) & shdefaultCAR[sh];
	ttyclose(tp);  /* remember this will clear out t_state */

	if (turnoff) {
		/* we have to do this after the ttyclose so that output
		 * can still drain
		 */
		s = spl5();
		addr->csr.low = SH_RIE|(unit & LINEMASK);
		addr->lnctrl = NULL; /* turn off interrupts also */
		splx(s);
	}
	shmodem[unit] = 0;
	tty_def_close(tp);
	wakeup((caddr_t)&tp->t_rawq); /* wake up anyone in shopen */
}

shread(dev, uio)
	dev_t dev;
	struct uio *uio;
{
	register struct tty *tp = &sh_tty[minor(dev)];

	return ((*linesw[tp->t_line].l_read)(tp, uio));
}

shwrite(dev, uio)
	dev_t dev;
	struct uio *uio;
{
	register struct tty *tp = &sh_tty[minor(dev)];

	return ((*linesw[tp->t_line].l_write)(tp, uio));
}

/*
 * SH receiver interrupt.
 */
shrint(sh)
	int sh; /* module number */
{
	register struct tty *tp;
	register c, unit;
	register struct shdevice *addr;
	struct tty *tp0;
	register struct uba_device *ui;
	register flg;
	int overrun = 0;
	u_char *modem0, *modem;
	int modem_cont;

	ui = shinfo[sh];
	if (ui == 0 || ui->ui_alive == 0)
		return;
	addr = (struct shdevice *)shmem;
	tp0 = &sh_tty[sh<<3];  /* first tty structure that corresponds
				* to this sh module
				*/
	modem0 = &shmodem[sh<<3];
	/*
	 * Loop fetching characters from receive fifo for this
	 * sh until there are no more in the receive fifo.
	 */
	while ((c = addr->run.rbuf) < 0) {
		/* if c < 0 then data valid is set */
		unit = (c>>8)&LINEMASK;
		tp = tp0 + unit; /* tty struct for this line */
		flg = tp->t_iflag;
		modem = modem0 + unit;
		modem_cont = 0;
#ifdef SHDEBUG
		if (shdebug > 6)
			mprintf("shrint0: c=%x, tp=%x\n", c, tp);
#endif
		/* check for modem transitions */
		if ((c & SH_STAT)==SH_STAT) {
			if (c & SH_DIAG) /* ignore diagnostic info */
				continue;
#ifdef SHDEBUG
			if (shdebug > 6)
				mprintf("shrint: c=%x, tp=%x\n", c, tp);
#endif
			if (tp->t_cflag & CLOCAL)
				continue;

			/* set to line #*/
			addr->csr.low = SH_RIE|(unit & LINEMASK);
			sh_softcsr = SH_RIE|(unit & LINEMASK);

			/* examine modem status */
			
			/*
			 * Drop DTR immediately if DSR has gone away.
			 * If really an active close then do not
			 *    send signals.
			 */

			if ((addr->fun.fs.stat&SH_DSR)==0) {
				if (tp->t_state&TS_CLOSING) {
					untimeout(wakeup, (caddr_t) &tp->t_dev);
					wakeup((caddr_t) &tp->t_dev);
					continue;
				}
				if (tp->t_state&TS_CARR_ON) {
					/*
					 * Only drop line if DSR is followed.
					 */
					if (shdsr)
						sh_tty_drop(tp);
					continue;
				}
			}
			/*
			 * Check for transient CD drops.
			 * Only drop DTR if CD is down for more than 2 secs.
			 */
			if (tp->t_state&TS_CARR_ON)
			    if ((addr->fun.fs.stat&SH_CD)==0){
				if ( *modem & MODEM_CD) {
				    /* only start timer once */
				    if (shdebug)
					mprintf("shrint, cd_drop, tp=%x\n", tp);
				    *modem &= ~MODEM_CD;
				    shtimestamp[minor(tp->t_dev)] = time;
				    timeout(sh_cd_drop, tp, hz*shcdtime);
				    modem_cont = 1;;
				}
			    } else
				/*
				 * CD has come up again.
				 * Stop timeout from occurring if set.
				 * If interval is more than 2 secs then
				 *  drop DTR.
				 */
				if ((*modem&MODEM_CD)==0) {
					untimeout(sh_cd_drop, tp);
					if (sh_cd_down(tp))
						/* drop connection */
						sh_tty_drop(tp);
					*modem |= MODEM_CD;
					modem_cont = 1;;
				}


			/* CTS flow control check */

			if (tp->t_state&TS_CARR_ON)
				if ((addr->fun.fs.stat&SH_CTS)==0) {
					tp->t_state |= TS_TTSTOP;
					*modem &= ~MODEM_CTS;
					if (shdebug)
					   mprintf("shrint: CTS stop, tp=%x\n", tp);
					shstop(tp, 0);
					continue;
				} else if ((*modem&MODEM_CTS)==0) {
					    tp->t_state &= ~TS_TTSTOP;
					    *modem |= MODEM_CTS;
					    if (shdebug)
					       mprintf("shrint: CTS start, tp=%x\n", tp);
					    shstart(tp);
					    continue;
					}

			/*
			 * Avoid calling sh_start_tty for a CD transition if
			 * the connection has already been established.
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
			 */
			if (shdsr) {
				if ((addr->fun.fs.stat&SH_XMIT)==SH_XMIT
				    && (*modem&MODEM_DSR_START)==0)
					sh_start_tty(tp);
				else
				    if ((addr->fun.fs.stat&SH_DSR) &&
					(*modem&MODEM_DSR)==0) {
					*modem |= (MODEM_DSR_START|MODEM_DSR);
					/*
					 * we should not look for CTS|CD for
					 * about 500 ms.
					 */
					timeout(sh_dsr_check, tp, hz*30);
					timeout(sh_dsr_check, tp, hz/2);
				    }
			}
			/*
			 * Ignore DSR but look for CD and CTS.
			 */
			else {
				if ((addr->fun.fs.stat&SH_NODSR)==SH_NODSR)
					sh_start_tty(tp);
			}



			continue;
		}


#ifndef PORTSELECTOR
		if ((tp->t_state&TS_ISOPEN)==0) {
#else
		if ((tp->t_state&(TS_ISOPEN|TS_WOPEN))==0) {
#endif
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

					/* tp->t_flags should be register */
		/* SH_FERR is interpreted as a break */
		if (c & SH_FERR) {
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
#ifdef SHDEBUG
				if (shdebug)
					mprintf("shrint: BREAK RECEIVED\n");
#endif SHDEBUG
				if ((tp->t_lflag_ext & PRAW) && 
					(tp->t_line != TERMIODISC))
					c = 0;
				else {
				    ttyflush(tp, FREAD|FWRITE);
#ifdef SHDEBUG
				    if (shdebug)
					mprintf("sending signal to tp->t_pgrp = %d\n", tp->t_pgrp);
#endif SHDEBUG
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
		else if (c & SH_PERR){
			/* 
			 * If input parity checking is not enabled, clear out
			 * parity error in this character.
			 */
#ifdef SHDEBUG
			if (shdebug > 1)
				mprintf("shrint: Parity Error\n");
#endif SHDEBUG
			if ((flg & INPCK) == 0)
				c &= ~SH_PERR;
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
		if ((c & SH_OVERR) && overrun == 0) {
			printf("sh%d: recv. fifo overflow\n", sh);
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
 * Ioctl for SH.
 */
/*ARGSUSED*/
shioctl(dev, cmd, data, flag)
	dev_t dev;
	register int cmd;
	caddr_t data;
	int flag;
{
	register int sh, unit;
	register struct shdevice *addr;
	register struct tty *tp;
	register int s;
	struct uba_device *ui;
	struct sh_softc *sc;
	struct devget *devget;
	int error;

	unit = minor(dev);
	tp = &sh_tty[unit];
	sh = unit >> 3;	   /* module number */
	ui = shinfo[sh];
	sc = &sh_softc[sh];
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
 				shparam(unit);
 				break;
 		}
		return (error);
	}
	addr = (struct shdevice *)tp->t_addr;
#ifdef SHDEBUG
	if (shdebug)
		mprintf("shioctl: unit=%d, cmd=%d\n", unit, cmd&0xff);
#endif
	switch (cmd) {

	case TIOCSBRK:
		s = spl5();
		addr->csr.low = SH_RIE|(unit & LINEMASK);
		sh_softcsr = SH_RIE|(unit & LINEMASK);
		addr->lnctrl |= SH_BREAK;
		splx(s);
		break;

	case TIOCCBRK:
		s = spl5();
		addr->csr.low = SH_RIE|(unit & LINEMASK);
		sh_softcsr = SH_RIE|(unit & LINEMASK);
		addr->lnctrl &= ~SH_BREAK;
		splx(s);
		break;

			/* next 2 allowed whether or not we have modem cntl */
	case TIOCSDTR:
		s = spl5();
		addr->csr.low = SH_RIE|(unit & LINEMASK);
		sh_softcsr = SH_RIE|(unit & LINEMASK);
		addr->lnctrl |= (SH_DTR|SH_RTS);
		splx(s);
		break;

	case TIOCCDTR:
		s = spl5();
		addr->csr.low = SH_RIE|(unit & LINEMASK);
		sh_softcsr = SH_RIE|(unit & LINEMASK);
		addr->lnctrl &= ~(SH_DTR|SH_RTS);
		splx(s);
		break;

	/* handle maintenance mode */
	case TIOCSMLB:
		if (u.u_uid)
			return(EPERM);
		s=spl5();
		addr->csr.low = SH_RIE|(unit&LINEMASK ); /*enable interrupts*/
		sh_softcsr = SH_RIE|(unit&LINEMASK ); /*enable interrupts*/
		addr->lnctrl |= (SH_MAINT);
		splx(s);
		break;

	case TIOCCMLB:
		if (u.u_uid)
			return(EPERM);
		s=spl5();
		addr->csr.low = SH_RIE|(unit & LINEMASK);
		sh_softcsr = SH_RIE|(unit & LINEMASK);
		addr->lnctrl &= ~(SH_MAINT);
		splx(s);
		break;

	case TIOCNMODEM:  /* ignore modem status */
			  /* allowed whether or not modem cntl available */
		s = spl5();
		shsoftCAR[sh] |= (1<<(unit&LINEMASK));
		if (*(int *)data) /* make mode permanent */
			shdefaultCAR[sh] |= (1<<(unit&LINEMASK));
		tp->t_state |= TS_CARR_ON;
		addr->csr.low = SH_RIE|(unit & LINEMASK);
		sh_softcsr = SH_RIE|(unit & LINEMASK);
		addr->lnctrl &= ~(SH_MODEM);
		shmodem[unit] = MODEM_CTS|MODEM_CD|MODEM_DSR;
		tp->t_cflag |= CLOCAL;	/* Map to termio */
		splx(s);
		break;

	case TIOCMODEM:  /* dont ignore modem status  */
		s = spl5();
		addr->csr.low = SH_RIE|(unit & LINEMASK);
		sh_softcsr = SH_RIE|(unit & LINEMASK);
		if ((addr->fun.fs.stat&SH_MSTAT) == 0) { /* have modem cntl */
			shsoftCAR[sh] &= ~(1<<(unit&LINEMASK));
			if (*(int *)data) /* make mode permanent */
				shdefaultCAR[sh] &= ~(1<<(unit&LINEMASK));
			addr->lnctrl |= SH_MODEM;
		    	/* 
		    	 * If shdsr is set look for DSR|CTS|CD, otherwise look 
		    	 * for CD|CTS only.
		    	 */
			if ((shdsr && ((addr->fun.fs.stat&SH_XMIT)==SH_XMIT)) ||
			   ((shdsr == 0) && ((addr->fun.fs.stat&SH_NODSR)==SH_NODSR))) {
				tp->t_state |= TS_CARR_ON;
				tp->t_state &= ~TS_ONDELAY;
				shmodem[unit] = MODEM_CTS|MODEM_CD|MODEM_DSR;
			}
			else {
				tp->t_state &= ~(TS_CARR_ON);
				shmodem[unit] &= ~(MODEM_CTS|MODEM_CD|MODEM_DSR);
			}
		}
		tp->t_cflag &= ~CLOCAL;	/* Map to termio */
		splx(s);
		break;

	case TIOCWONLINE:
		s = spl5();
		addr->csr.low = SH_RIE|(unit & LINEMASK);
		sh_softcsr = SH_RIE|(unit & LINEMASK);
		if ((addr->fun.fs.stat&SH_MSTAT) == 0) { /* have modem cntl */
		    /* 
		     * If shdsr is set look for DSR|CTS|CD, otherwise look 
		     * for CD|CTS only.
		     */
		    if ((shdsr && ((addr->fun.fs.stat&SH_XMIT)==SH_XMIT)) ||
			   ((shdsr == 0) && ((addr->fun.fs.stat&SH_NODSR)==SH_NODSR))) {
			tp->t_state |= TS_CARR_ON;
			tp->t_state &= ~TS_ONDELAY;
			shmodem[unit] = MODEM_CTS|MODEM_CD|MODEM_DSR;
		    }
		    else
			while ((tp->t_state & TS_CARR_ON) == 0)
				sleep((caddr_t)&tp->t_rawq, TTIPRI);
		}
		splx(s);
		break;

			/* next 4 allowed whether or not we have modem cntl */
	case TIOCMGET:
		s = spl5();
		addr->csr.low = SH_RIE|(unit & LINEMASK);
		sh_softcsr = SH_RIE|(unit & LINEMASK);
		*(int *)data = shtodm(addr->lnctrl,addr->fun.fs.stat);
		splx(s);
		break;

	case TIOCMSET:
		s = spl5();
		addr->csr.low = SH_RIE|(unit & LINEMASK);
		sh_softcsr = SH_RIE|(unit & LINEMASK);
		addr->lnctrl = dmtosh(*(int *)data);
		splx(s);
		break;

	case TIOCMBIS:
		s = spl5();
		addr->csr.low = SH_RIE|(unit & LINEMASK);
		sh_softcsr = SH_RIE|(unit & LINEMASK);
		addr->lnctrl |= dmtosh(*(int *)data);
		splx(s);
		break;

	case TIOCMBIC:
		s = spl5();
		addr->csr.low = SH_RIE|(unit & LINEMASK);
		sh_softcsr = SH_RIE|(unit & LINEMASK);
		addr->lnctrl &= ~(dmtosh(*(int *)data));
		splx(s);
		break;

	case DEVIOCGET: 			/* device status */
		devget = (struct devget *)data;
		bzero(devget,sizeof(struct devget));

		s = spl5();
		addr->csr.low = SH_RIE|(unit & LINEMASK);
		sh_softcsr = SH_RIE|(unit & LINEMASK);
		if ((addr->fun.fs.stat&SH_MSTAT) == 0) { /* have modem cntl */
			sc->sc_category_flags[unit&LINEMASK] |= DEV_MODEM;
			if (tp->t_cflag & CLOCAL) 
			    sc->sc_category_flags[unit&LINEMASK] &= ~DEV_MODEM_ON;
			else
			    sc->sc_category_flags[unit&LINEMASK] |= DEV_MODEM_ON;
		}
		else
			sc->sc_category_flags[unit&LINEMASK] &=
				~(DEV_MODEM|DEV_MODEM_ON);
		splx(s);

		devget->category = DEV_TERMINAL;	/* terminal cat.*/

		devget->bus = DEV_NB;			/* NO bus	*/
		bcopy(DEV_TM_SLE,devget->interface,
		      strlen(DEV_VS_SLU));		/* interface	*/
		bcopy(DEV_UNKNOWN,devget->device,
		      strlen(DEV_UNKNOWN));		/* terminal	*/
		devget->adpt_num = 0;			/* NO adapter	*/
		devget->nexus_num = 0;			/* fake nexus 0 */
		devget->bus_num = 0;			/* NO bus	*/
		devget->ctlr_num = sh;			/* cntlr number */
		devget->slave_num = unit&LINEMASK;	/* line number	*/
		bcopy(ui->ui_driver->ud_dname,
		      devget->dev_name,
		      strlen(ui->ui_driver->ud_dname)); /* Ultrix "sh"	*/
		devget->unit_num = unit&LINEMASK;	/* sh line?	*/
		devget->soft_count =
		      sc->sc_softcnt[unit&LINEMASK];	/* soft err cnt */
		devget->hard_count =
		      sc->sc_hardcnt[unit&LINEMASK];	/* hard err cnt */
		devget->stat = sc->sc_flags[unit&LINEMASK]; /* status	*/
		devget->category_stat =
		      sc->sc_category_flags[unit&LINEMASK]; /* cat. stat. */
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
		shparam(unit);
		break;

	default:
		if (u.u_procp->p_progenv == A_POSIX) 
			return (EINVAL);
		return (ENOTTY);
	}
	return (0);
}

shtodm(lnctrl,lstat)
	register u_short lnctrl;
	register char lstat;
{
	register int b = 0;
	if (lnctrl&SH_RTS)  b |= TIOCM_RTS;
	if (lnctrl&SH_DTR)  b |= TIOCM_DTR;
	if (lstat&SH_CD)  b |= TIOCM_CD;
	if (lstat&SH_CTS)  b |= TIOCM_CTS;
	if (lstat&SH_RING)  b |= TIOCM_RI;
	if (lstat&SH_DSR)  b |= TIOCM_DSR;
	return(b);
}


dmtosh(bits)
	register int bits;
{
	register u_short lnctrl = 0;
	if (bits&TIOCM_RTS) lnctrl |= SH_RTS;
	if (bits&TIOCM_DTR) lnctrl |= SH_DTR;
	return(lnctrl);
}

/*
 * Set parameters from open or stty into the SH hardware
 * registers.
 */
shparam(unit)
	register int unit;
{
	register struct tty *tp;
	register struct shdevice *addr;
	register int lpar;
	int tcd, br, i;
	int s;

	tp = &sh_tty[unit];
	addr = (struct shdevice *)tp->t_addr;
	/*
	 * Block interrupts so parameters will be set
	 * before the line interrupts.
	 */
	s = spl5();
	addr->csr.low = SH_RIE|(unit&LINEMASK);
	sh_softcsr = SH_RIE|(unit&LINEMASK);
	if ((((tp->t_cflag&CBAUD)==B0) && (u.u_procp->p_progenv != A_POSIX)) || 
	    (((tp->t_cflag_ext & CBAUD)==B0) && 
		(u.u_procp->p_progenv == A_POSIX))) { 
		tp->t_cflag |= HUPCL;
		addr->lnctrl = SH_REN; /*turn off DTR & RTS but leave enabled*/
		splx(s);
		return;
	}
	lpar = ((sh_speeds[tp->t_cflag_ext&CBAUD])<<12) | 
		((sh_speeds[tp->t_cflag&CBAUD])<<8);
	/*
	 * Berkeley-only dinosaurs
	 */
	if (tp->t_line != TERMIODISC) {
		if ((tp->t_cflag & CBAUD) == B134){
			lpar |= SH_BITS6|SH_PENABLE; /* no half duplex on sh11 ? */
			tp->t_cflag |= CS6|PARENB;
		}
		if ((tp->t_cflag_ext & CBAUD) == B110){
			lpar |= SH_TWOSB;
			tp->t_cflag |= CSTOPB;
		}
	}
 	/*
 	 * System V termio functionality.
	 * Set device registers according to the specifications of the termio
	 * structure.
 	 */
 	if (tp->t_cflag & CREAD)
 		addr->lnctrl |= SH_REN;
	else
 		addr->lnctrl &= ~SH_REN;
 	if (tp->t_cflag & CSTOPB)
 		lpar |= SH_TWOSB;
 	else
 		lpar &= ~SH_TWOSB;
 	/* parity is enable */
 	if (tp->t_cflag & PARENB) {
 		if ((tp->t_cflag & PARODD) == 0)
 			/* set even */
 			lpar |= SH_PENABLE|SH_EVENPAR;
 		else
 			/* else set odd */
 			lpar = (lpar | SH_PENABLE)&~SH_EVENPAR;
 	}
 	/*
 	 * character size.
 	 * clear bits and check for 6,7,and 8, else its 5 bits.
 	 */
 	lpar &= ~SH_BITS8;
 	switch(tp->t_cflag&CSIZE) {
 		case CS6:
 			lpar |= SH_BITS6;
 			break;
 		case CS7:
 			lpar |= SH_BITS7;
 			break;
 		case CS8:
 			lpar |= SH_BITS8;
 			break;
 	}
	/*
	 * Outgoing Auto flow control.
	 * No auto flow control allowed if startc != ^q and stopc !=
	 * ^s.  Most drivers do not allow this to be changed.
	 */
	if ((tp->t_cflag_ext & PAUTOFLOW) && (tp->t_cc[VSTOP] == CTRL('s')) && 
		(tp->t_cc[VSTART] == CTRL('q')))
		addr->lnctrl |= SH_XFLOW;
	else
		addr->lnctrl &= ~SH_XFLOW;
#ifdef SHDEBUG
	if (shdebug)
		mprintf("shparam: tp=%x, lpr=%x\n", tp, lpar);
#endif
	/*
	 * Only write the line parameter register if the
	 * new parameters differ from the current ones.
	 * This avoids some nasty delay timing (see below).
	 */
	if (addr->lpr != lpar) {
		/*
		 * Must delay two character times before
		 * we change the line parameters.
		 * Find the number of microseconds of delay required,
		 * based on the current speed setting.
		 */
		br = (addr->lpr>>12)&0x0f;
		for (i=0; i<SHDTSIZE; i++)
			if (br == sh_speeds[i]) {
				tcd = sh_delays[i] * 2;
				break;
			}
		/*
		 * Delay long enough for two characters to be transmitted
		 * (see comments above "#define SHDTSIZE").
		 * Use microdelay for times less than 10 Msec
		 * (speeds >= 2400 BPS), otherwise use a timeout.
		 * Timeouts are inaccurate, but better than spinning at ipl.
		 */
		if (tcd) {
		    if (tcd < 10000) {
			DELAY(tcd);
		    } else {
			i = (tcd / 10000);	/* timeout value in ticks */
			if (tcd % 10000)
			    i += 1;		/* fraction of a tick */
			i += 1;			/* 10 Msec clock granularity */
			sh_newlpr[unit] = lpar;
			timeout(sh_lpr_load, tp, i);
			sleep((caddr_t)&tp->t_addr, PZERO-10);
			splx(s);
			return;
		    }
		}
		addr->lpr = lpar;
	}
	splx(s);
}

/*
 * This routine loads a new value into the line
 * parameter register, after the two character
 * delay has expired (see shparam() abobe).
 * The spl7() makes sure the line select bits in
 * the CSR are saved and restored properly. This is
 * necessary because this routine is called via timeout,
 * which is clock driven (clock interrupts at spl6).
 * The current line number is saved in sh_softcsr, because
 * we can't read it from the csr without lossing xmit interrupts.
 */
sh_lpr_load(tp)
register struct tty *tp;
{
	register struct shdevice *addr = (struct shdevice *)tp->t_addr;
	register int unit = minor(tp->t_dev);
	register int s, oldunit;

	s = spl7();	/* THIS IS REALLY NECESSARY */
	oldunit = (sh_softcsr & LINEMASK);
	addr->csr.low = SH_RIE|(unit & LINEMASK);
	addr->lpr = sh_newlpr[unit & LINEMASK];
	wakeup((caddr_t)&tp->t_addr);
	addr->csr.low = (SH_RIE|oldunit);
	splx(s);
}

/*
 * SH transmitter interrupt.
 * Restart each line which used to be active but has
 * terminated transmission since the last interrupt.
 */
shxint(sh)
	int sh;  /* module number */
{
	register struct tty *tp;
	register struct shdevice *addr;
	register int unit;
	char csrxmt;

	addr = (struct shdevice *)shmem;
	while ((csrxmt = addr->csr.high) < 0) {
		if (csrxmt & SH_DIAGFAIL)
			printf("sh%d: DIAG. FAILURE\n", sh);
		unit = sh * 8;
		unit |= csrxmt&LINEMASK;
		tp = &sh_tty[unit];
#ifdef SHDEBUG
		if (shdebug > 7)
			mprintf("shxint: unit=%x, tp=%x, c_cc=%d\n",
				unit, tp, tp->t_outq.c_cc);
#endif
		tp->t_state &= ~TS_BUSY;
		if (tp->t_state&TS_FLUSH)
			tp->t_state &= ~TS_FLUSH;
		if (tp->t_line)
			(*linesw[tp->t_line].l_start)(tp);
		else
			shstart(tp);
	}
}

/*
 * Start (restart) transmission on the given SH line.
 */
shstart(tp)
	register struct tty *tp;
{
	register struct shdevice *addr;
	register int unit, nch, line;
	int s;
	int free;
	char *cp = tp->t_outq.c_cf;
	union {
		int	x;
		char	y[2];
	} wdbuf;

	unit = minor(tp->t_dev);
	line = unit & LINEMASK; /* unit now equals the line number */
	addr = (struct shdevice *)tp->t_addr;

	/*
	 * Must hold interrupts in following code to prevent
	 * state of the tp from changing.
	 */
	s = spl5();
	/*
	 * If it's currently active, or delaying, no need to do anything.
	 * Also do not transmit if not CTS
	 */
	if ((tp->t_state&(TS_TIMEOUT|TS_BUSY|TS_TTSTOP)) ||
		 ((tp->t_state&TS_CARR_ON) && (shmodem[unit]&MODEM_CTS)==0))
		goto out;
	/*
	 * If there are sleepers, and output has drained below low
	 * water mark, wake up the sleepers.
	 */
#ifdef SHDEBUG
	if (shdebug > 9)
		mprintf("shstart0: tp=%x, LO=%d, cc=%d \n", tp,
			TTLOWAT(tp), tp->t_outq.c_cc);
#endif
	if (tp->t_outq.c_cc<=TTLOWAT(tp)) {
		if (tp->t_state&TS_ASLEEP) {  /* wake up when output done */
			tp->t_state &= ~TS_ASLEEP;
			wakeup((caddr_t)&tp->t_outq);
		}
		if (tp->t_wsel) {   /* for select system call */
			selwakeup(tp->t_wsel, tp->t_state & TS_WCOLL);
			tp->t_wsel = 0;
			tp->t_state &= ~TS_WCOLL;
		}
	}
	/*
	 * If we stopped output due to XOFF
	 * restart it by re-setting xmit enable.
	 * Must be done before checking if the queue is empty.
	 * Otherwise, last line of output may not restart.
	 */
	addr->csr.low = SH_RIE|line; /* select line */
	sh_softcsr = SH_RIE|line; /* select line */
	if ((addr->tbuffad2.high&SH_XEN) == 0) {
		if (addr->lnctrl&SH_XABORT)
			addr->lnctrl &= ~SH_XABORT;
		free = addr->fun.fs.fifosize;
		addr->tbuffad2.high = SH_XEN;
		if (free < 64) {
			tp->t_state |= TS_BUSY;
			goto out;
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
		nch = ndqb(&tp->t_outq, 0); /* number of consecutive chars */
	else {
		nch = ndqb(&tp->t_outq, DELAY_FLAG);
		/*
		 * If first thing on queue is a delay process it.
		 */
		if (nch == 0) {
			nch = getc(&tp->t_outq);
			timeout(ttrstrt, (caddr_t)tp, (nch&0x7f)+6);
			tp->t_state |= TS_TIMEOUT;
			goto out;
		}
	}
	/*
	 * If characters to transmit, restart transmission.
	 */
	if (nch) {
#ifdef SHDEBUG
		if (shdebug > 9)
			mprintf("shstart1: line=%x, nch=%d\n", line, nch);
#endif
		addr->csr.low = SH_RIE|line; /* select line */
		sh_softcsr = SH_RIE|line; /* select line */
		addr->csr.high = SH_XIE;
		if (addr->lnctrl&SH_XABORT)
			addr->lnctrl &= ~SH_XABORT;
		free = addr->fun.fs.fifosize;
		if (nch > free)
			nch = free;
		else
			free = nch;
		while (free > 1) {
			wdbuf.y[0] = *cp++;
			wdbuf.y[1] = *cp++;
			addr->fun.fifodata = wdbuf.x;
			free -= 2;
		}
		if (free == 1)
			addr->fun.fs.fifosize = *cp++; /* lo byte wrt */
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
shstop(tp, flag)
	register struct tty *tp;
{
	register struct shdevice *addr;
	register int unit, s;

	addr = (struct shdevice *)tp->t_addr;
	/*
	 * Block input/output interrupts while messing with state.
	 */
	s = spl5();
	if (tp->t_state & TS_BUSY) {
		/*
		 * Device is transmitting; stop output.
		 * We can continue later
		 * by examining the address where the sh stopped.
		 */
		unit = minor(tp->t_dev);
		addr->csr.low = (unit&LINEMASK)|SH_RIE;
		sh_softcsr = (unit&LINEMASK)|SH_RIE;
		/*
		 * Abort xmit only if flushing tty queues, i.e.,
		 * called by ttyflush().
		 * Otherwise, turn off Xmit enable to stop output.
		 */
		if ((tp->t_state&TS_TTSTOP)==0) {
			tp->t_state |= TS_FLUSH;
			addr->lnctrl |= SH_XABORT;  /* abort transmission */
		} else {
			/*
			 * If flushing output in AUTOFLOW mode we have no
			 * choice but to let the remainder drain.  If this
			 * test wasn't here and SH_XFLOW was set the device
			 * would be waiting for ^Q once we cleared xmit
			 * enable.
			 */ 
			if ((tp->t_cflag_ext & PAUTOFLOW) == 0) {
				/* turn off xmit enable */
				addr->tbuffad2.high = 0;    
				tp->t_state &= ~TS_BUSY;
			}
		}
	}
	splx(s);
}

shreset(uban)
	int uban;
{
}


sh_cd_drop(tp)
register struct tty *tp;
{
	register struct shdevice *addr = (struct shdevice *)tp->t_addr;
	register int unit = minor(tp->t_dev);

	addr->csr.low = SH_RIE|(unit & LINEMASK);
	sh_softcsr = SH_RIE|(unit & LINEMASK);
	if ((tp->t_state&TS_CARR_ON) &&
		((addr->fun.fs.stat&SH_CD) == 0)) {
		if (shdebug)
		    mprintf("sh_cd:  no CD, tp=%x\n", tp);
		sh_tty_drop(tp);
		return;
	}
	shmodem[minor(tp->t_dev)] |= MODEM_CD;
	if (shdebug)
	    mprintf("sh_cd:  CD is up, tp=%x\n", tp);
}
sh_dsr_check(tp)
register struct tty *tp;
{
	int unit = minor(tp->t_dev);
	register struct shdevice *addr = (struct shdevice *)tp->t_addr;
	if (shmodem[unit]&MODEM_DSR_START) {
		if (shdebug)
		    mprintf("sh_dsr_check0:  tp=%x\n", tp);
		shmodem[unit] &= ~MODEM_DSR_START;
		addr->csr.low = SH_RIE|(unit&LINEMASK);
		sh_softcsr = SH_RIE|(unit&LINEMASK);
		/* 
		 * If shdsr is set look for DSR|CTS|CD, otherwise look 
		 * for CD|CTS only.
		 */
		if (shdsr) {
			if ((addr->fun.fs.stat&SH_XMIT)==SH_XMIT)
				sh_start_tty(tp);
		}
		else
			if ((addr->fun.fs.stat&SH_NODSR)==SH_NODSR)
				sh_start_tty(tp);
		return;
	}
	if ((tp->t_state&TS_CARR_ON)==0)  {
		sh_tty_drop(tp);
		if (shdebug)
		    mprintf("sh_dsr_check:  no carrier, tp=%x\n", tp);
	}
	else
		if (shdebug)
		    mprintf("sh_dsr_check:  carrier is up, tp=%x\n", tp);
}

/*
 *  cd_down return 1 if carrier has been down for at least 2 secs.
 */
sh_cd_down(tp)
struct tty *tp;
{
	int msecs;
	int unit = minor(tp->t_dev);

	msecs = 1000000 * (time.tv_sec - shtimestamp[unit].tv_sec) +
		(time.tv_usec - shtimestamp[unit].tv_usec);
	if (msecs > 2000000)
		return(1);
	else
		return(0);
}

sh_tty_drop(tp)
struct tty *tp;
{
	register struct shdevice *addr = (struct shdevice *)tp->t_addr;
	register int unit;
	if (tp->t_flags&NOHANG)
		return;
	unit = minor(tp->t_dev);
 	shmodem[unit] = MODEM_BADCALL;
 	tp->t_state &= ~(TS_CARR_ON|TS_TTSTOP|TS_BUSY|TS_ISUSP);
 	wakeup((caddr_t)&tp->t_rawq);
	gsignal(tp->t_pgrp, SIGHUP);
	gsignal(tp->t_pgrp, SIGCONT);
	addr->csr.low = SH_RIE|(unit&LINEMASK);
	sh_softcsr = SH_RIE|(unit&LINEMASK);
	addr->lnctrl &= ~(SH_DTR|SH_RTS);  /* turn off DTR */
}
sh_start_tty(tp)
	register struct tty *tp;
{
	int unit = minor(tp->t_dev);
	tp->t_state &= ~TS_ONDELAY;
	tp->t_state |= TS_CARR_ON;
	if (shdebug)
	       mprintf("sh_start_tty:  tp=%x\n", tp);
	if (shmodem[unit]&MODEM_DSR)
		untimeout(sh_dsr_check, tp);
	shmodem[unit] |= MODEM_CD|MODEM_CTS|MODEM_DSR;
	shtimestamp[unit] = shzerotime;
	wakeup((caddr_t)&tp->t_rawq);
}

shbaudrate(speed)
register int speed;
{
    if (sh_valid_speeds & (1 << speed))
	return (1);
    else
	return (0);
}
#endif
