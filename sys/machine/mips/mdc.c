#ifndef lint
static char *sccsid = "@(#)mdc.c	4.3      (ULTRIX)        9/10/90";
#endif	lint

/************************************************************************
 *									*
 *			Copyright (c) 1990 by				*
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
 *   The information in this software is subject to change  without	*
 *   notice  and should not be construed as a commitment by Digital	*
 *   Equipment Corporation.						*
 *									*
 *   Digital assumes no responsibility for the use  or	reliability	*
 *   of its software on equipment which is not supplied by Digital.	*
 *									*
 ************************************************************************
 *
 * mdc.c
 *
 * Mipsmate DC7085 SLU console driver -the real thing!!!!!
 *
 * Modification history
 *
 *
 *  August 22, 1990     Paul Grist
 *      Changed initilazation of console baud rate to use "baud0", it
 *      was left "baud1" from some early prom debug, hence was broke.
 *
 *  July 14, 1990	Kuo-Hsiung Hsieh
 *	Asserted Speed Select (SS) modem control signal in open routine.
 *	Deasserted SS when process exits.  As requested by PTT test.
 *
 *  July 5, 1990	Kuo-Hsiung Hsieh
 *      Fixed data corrupted problem due to setting break condition
 *      on a transmission line.  On DC type of chip, a specific delay
 *      period has to be imposed on the transmission line if the next
 *      thing to transmit is a break condition.  Data could be corrupted
 *      even though TRDY bit may say it is ready to send the next
 * 	character.
 *
 *  June 4, 1990	Kuo-Hsiung Hsieh
 *	Fixed duplicated assertion of break.  It is caused by reading
 *	the transmit data register which is the modem status register.
 *
 *  May 10, 1990	Kuo-Hsiung Hsieh
 *	Fixed linenum typo which caused system hung and added modem stuff.
 *
 *  March 23, 1990	Tim Burke
 *
 *	Created file. Contents based on dc7085.c file.  Due to time constraints
 *	a separate copy of this driver is being developed instead of modifying
 *	the original source to accomodate all three device types.  The main
 *	changes from the dc7085 sources is to remove all references to graphic
 *	capabilities.  This driver can accomodate up to 3 DC7085 chips while
 *	the original driver could only have 1 such chip.  
 *
 *	Extensive cleanups of the original dc7085.c driver were also done.
 *	These cleanups include using #define constants, overhaul of the buad
 *	rate representation scheme.  Use consistent variable terminology,
 *	specifically unit means dc number, linenum is the particular line of
 *	a dc and minor_num is the device minor number.
 */

#include "../machine/pte.h"
#include "../h/param.h"
#include "../h/systm.h"
#include "../h/ioctl.h"
#include "../h/tty.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/proc.h"
#include "../h/map.h"
#include "../h/buf.h"
#include "../h/vm.h"
#include "../h/conf.h"
#include "../h/file.h"
#include "../h/uio.h"
#include "../h/kernel.h"
#include "../h/devio.h"
#include "../../machine/common/cpuconf.h"
#include "../h/exec.h"
#include "../h/kmalloc.h"
#include "../h/sys_tpath.h"
#include "../io/uba/ubavar.h"	/* auto-config headers */

#include "../machine/cpu.h"
#include "../machine/mips/mdcreg.h"

/*
 * Declare the data structures used to represent the terminal attributes.
 */
struct	tty mdc_tty[NDC * NDCLINE]; /* Allocate 1 tty structure per line*/
u_char	mdcmodem[NDC];	   	   /* keeps track of modem state 	*/
u_short mdcscan_previous[NDC];	   /* Used to detect modem transitions  */
int 	mdcmodem_active;	   /* Determines scan rate		*/
int 	console_baud;		   /* Specifies console baud rate	*/
int	mdcdefaultCAR[NDC];	   /* Default status of modem/nomodem   */
int	mdcsoftCAR[NDC];	   /* Present status of modem/nomodem   */
int	mdc_cnt;	   	   /* The number of configured lines	*/
u_char  mdc_brk[NDC];
extern  int nMDC;                  /* Configuration - # of DC chips     */
struct	timeval mdctimestamp[NDC]; /* Timer for modem control		*/
int	mdc_attach_called = 0;	   /* Attach routine has been called    */

int	mdcprobe(), mdcattach(), mdcrint(), mdc_unit_init();
int	mdc_dsr_check(), mdc_tty_drop(), mdc_cd_drop(); 
int	mdcputc(), mdcgetc();
void	mdcsetbreak();

u_short mdcstd[] = { 0 };
struct uba_device *mdcinfo[1];

/*
 * Unibus device driver definition.  These routines are used at boot time by
 * the configuration program.
 */
struct	uba_driver mdcdriver = { 
	mdcprobe, 			/* probe routine		   */
	0, 				/* slave routine		   */
	mdcattach, 			/* attach routine		   */
	0, 				/* ud_dgo routine		   */
	mdcstd, 			/* device csr address		   */
	"mdc", 				/* device name			   */
	mdcinfo 			/* backpointers to ubdinit structs */
					/* I guess the rest are just 0's   */
					/* name of a controller		   */
					/* backpointers to ubminit structs */
					/* want exclusive use of bdp's	   */
    };

int	mdcstart(), mdcxint(), mdcbaudrate();
int	ttrstrt();

#define MODEM_LINE	2		/* Modem control only on line 2     */
#define NOMODEM_UNIT	2		/* DC #2 does not have full modem   */
#define LINEMASK	0x03		/* line unit mask, each DC has 4 lines*/
#define LINEBITS	2		/* The linemask occupies 2 bits     */


/*
 * Baud Rate Support
 *
 * When the baud rate on the right is specified, the line parameter register
 * is setup with the appropriate bits as specified in the left column.
 */
#define BAUD_UNSUPPORTED 0	/* Device does not provide this baud rate */
#define BAUD_SUPPORTED   1	/* Device does provide this baud rate     */

/*
 * If the BAUD38 bit of the System Control and Status Register is set the
 * chip can do 38400 baud in which case EXTB would be supported.  This bit
 * applies to all 4 lines such that it is not possible to simultaneously do
 * 19200 and 38400.  To keep things simple, only provide support for 19.2.
 * The third column is the minimum delay value for setting a break
 * condition.  If we set a break condition without delaying this minimum
 * interval, we might corrupt character which is still in the shift
 * register.  The delay values are calculated based on the equation:
 * 12 (bits/char) * 256 (hz) / baudrate + 2 (safety factor).

 */
struct baud_support mdc_speeds[] = {
     	{0,			BAUD_UNSUPPORTED,	0},	/* B0    */
	{DC_B50,		BAUD_SUPPORTED,		78},	/* B50   */
	{DC_B75,		BAUD_SUPPORTED,		42},	/* B75   */
	{DC_B110,		BAUD_SUPPORTED,		30},	/* B110  */
	{DC_B134_5,		BAUD_SUPPORTED,		25},	/* B134  */
	{DC_B150,		BAUD_SUPPORTED,		22},	/* B150  */
	{0,			BAUD_UNSUPPORTED,	0},	/* B200  */
	{DC_B300,		BAUD_SUPPORTED,		12},	/* B300  */
	{DC_B600,		BAUD_SUPPORTED,		7},	/* B600  */
	{DC_B1200,		BAUD_SUPPORTED,		5},	/* B1200 */
	{DC_B1800,		BAUD_SUPPORTED,		4},	/* B1800 */
	{DC_B2400,		BAUD_SUPPORTED,		3},	/* B2400 */
	{DC_B4800,		BAUD_SUPPORTED,		2},	/* B4800 */
	{DC_B9600,		BAUD_SUPPORTED,		2},	/* B9600 */
	{DC_B19200,		BAUD_SUPPORTED,		2},	/* EXTA  */
	{0,			BAUD_UNSUPPORTED,	0}, 	/* EXTB  */
   };

extern int pmcons_init();
extern int prom_getenv();
extern int cpu;

#ifdef DEBUG
int mdcdebug = 0;

/*
 * Print out the modem leads for this DC unit.
 */
#define PRINT_SIGNALS(dcaddr) { cprintf("Modem signals: "); \
	if (dcaddr->dcmsr & DC_DSR) cprintf(" DSR "); \
	if (dcaddr->dcmsr & DC_CTS) cprintf(" CTS "); \
	if (dcaddr->dcmsr & DC_CD) cprintf(" CD "); \
	cprintf("\n"); } 
#endif DEBUG


/*
 * Probe to see if the device is alive.  Since this is a bounded configuration
 * there must be a DC chip present for the console.  The intialization is done
 * through dc_cons_init out of startup().
 */
mdcprobe(reg)
int reg;
{
	return(1);
}

/*
 * Initialize the device and setup initial values of relevant variables.
 */
mdcattach()
{
    register int unit;
    extern mdcscan();

    /*
     * If the option card is in place this routine may end up being called
     * more than once.  Use mdc_attach_called to insure that the body of this
     * routine is only executed once.
     */
    if (mdc_attach_called == 0) {
	mdc_attach_called = 1;
	/*
	 * Setup per-DC registers and software status.
	 */
	mdc_cnt = 0;
	if (nMDC <= 0) {
		panic("mdcattach: no units configured.\n");
	}
	if (nMDC > MAX_NDC) {
		nMDC = MAX_NDC;
		cprintf("Too many DC chips configured: nMDC = %d\n",nMDC);
	}
	for (unit = 0; unit < nMDC; unit++) {
		mdc_brk[unit] = 0;
		mdc_unit_init(unit);
		mdc_cnt += NDCLINE;
	}
	/*
 	 * Initialize the scanner process.  Since the chip does not 
	 * interrupt on modem transitions it is necessary to have a scanner
	 * thread that occasionally checks the modem leads and looks for
	 * changes.  Start up this to examine the modem leads once per second.
	 */
 	mdcmodem_active = 0;
	timeout(mdcscan, (caddr_t)0, hz);
    }
}

/*
 * Initalizes the DC registers and associated software attributes.
 * Called on a per unit basis from attach().
 */
mdc_unit_init(unit)
	register int unit;
{
	register volatile struct mdz_reg *dcaddr;

#	ifdef DEBUG
	if (mdcdebug)
		cprintf("mdc_unit_init: unit = %d.\n",unit);
#	endif DEBUG
	if (unit > MAX_NDC) {		/* paranoia */
		return;
	}
	dcaddr = mdz_regs[unit];
	/*
	 * Note that the dcsoftCAR and dcdefaultCAR do not look at the flags
	 * field of the config file.  This may need to be fixed up.
	 */
	mdcsoftCAR[unit] = 0xf;		/* Set lines to direct connect     */
	mdcdefaultCAR[unit] = 0xf;	/* Default lines to direct connect */
	mdcscan_previous[unit] = 0;	/* Initial modem status		   */
	/*
	 * DC0 and DC1 allow modem control on line 2.  Initialize these leads
	 * to clear the modem control signals.
	 */
	if (unit != NOMODEM_UNIT) {
		dcaddr->dctcr &= ~(DC_RDTR | DC_RRTS | DC_RSS);
	}
}
/*
 * Called early from startup.  The purpose of this routine is to setup the
 * console line to enable output so the startup messages can be displayed.
 */
mdc_cons_init()
{
	register int linenum = CONSOLE_LINE;
	register volatile struct mdz_reg *dcaddr = mdz_regs[CONSOLE_UNIT];
	register int prom_baud;

	/*
	 * The console will default to 9600 baud.  This can be changed via
	 * the baud environment variable.  Read in the console baud rate
	 * for later use.  This presently only supports a selected number of
	 * baud rates.
	 */
	prom_baud = atoi(prom_getenv("baud0"));
	switch (prom_baud) {
		case 75:	console_baud = B75; 	break;
		case 110:	console_baud = B110; 	break;
		case 150:	console_baud = B150; 	break;
		case 300:	console_baud = B300; 	break;
		case 600:	console_baud = B600; 	break;
		case 1200:	console_baud = B1200; 	break;
		case 2400:	console_baud = B2400; 	break;
		case 4800:	console_baud = B4800; 	break;
		case 9600:	console_baud = B9600; 	break;
		default:	console_baud = B9600;
	}

	dcaddr->dclpr = linenum | mdc_speeds[console_baud].baud_param 
			| BITS8 | DC_RE;
}

/*
 * Open line and set default parameters.  If this is a modem line wait for
 * the appropriate leads to be asserted.
 */
mdcopen(dev, flag)
	dev_t dev;
{
	register volatile struct mdz_reg *dcaddr;
	register struct tty *tp;
	register int unit;
	register int linenum;
	register int minor_num;
	int inuse;  /*hold state of inuse bit while blocked waiting for carr*/

	minor_num = minor(dev);
	unit = minor_num >> LINEBITS;
	dcaddr = mdz_regs[unit];
	linenum = minor_num & LINEMASK;
	if (minor_num >= mdc_cnt) {
		return (ENXIO);
	}
	tp = &mdc_tty[minor_num];
	if (tp->t_state&TS_XCLUDE && u.u_uid != 0) {
		return (EBUSY);
	}

	while (tp->t_state&TS_CLOSING) { /* let DTR stay down for awhile */
		sleep((caddr_t)&tp->t_rawq, TTIPRI);
	}
	tp->t_addr = (caddr_t)tp;
	tp->t_oproc = mdcstart;
	tp->t_baudrate = mdcbaudrate;

	tty_def_open(tp, dev, flag, (mdcsoftCAR[unit]&(1<<(linenum))));

	/*
	 * If the line is not presently open setup the default terminal
	 * attributes.
	 */
	if ((tp->t_state & TS_ISOPEN) == 0) {
	    /*
	     * Prevent spurious startups by making the 500ms timer
	     * initially high.
	     */
	    /* Tim  - Should this only be set if opening modem line? */
	    mdcmodem[unit] = MODEM_DSR_START;
	    /*
	     * Specify console terminal attributes.  Do not allow modem control 
	     * on the console.  Setup <NL> to <CR> <LF> mapping.
	     */
	    if ((unit == CONSOLE_UNIT) && (linenum == CONSOLE_LINE)) {
		tp->t_cflag |= CLOCAL;
		tp->t_flags = ANYP|ECHO|CRMOD;
		tp->t_iflag |= ICRNL;
		tp->t_oflag |= ONLCR;
	    }
	}
	mdcparam(minor_num);		/* enables interrupts */
	(void) spltty();


	/*
	 * No modem control provided for lines with softCAR set.
	 * Modem control provided only for line 2.
	 */
#	ifdef DEBUG
	if (mdcdebug)
		cprintf("mdcopen: unit = %d, linenum = %d\n",unit, linenum);
#	endif DEBUG
	/*
	 * Modem control is only provided on line 2 of DC0 and DC1.  All other
	 * lines are restricted to be direct connect.  For this reason set
	 * CLOCAL as a sanity check.
	 */
	if ((unit == NOMODEM_UNIT) || (linenum != MODEM_LINE)) {
		tp->t_cflag |= CLOCAL;
	}
	if (tp->t_cflag & CLOCAL) {
		/*
		 * This is a local connection - ignore carrier
		 * receive enable interrupts enabled above via dcparam()
		 */
		tp->t_state |= TS_CARR_ON;		/* dcscan sets */
		if (linenum == MODEM_LINE) {
			dcaddr->dctcr |= (DC_RDTR | DC_RRTS | DC_RSS);
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
	/*
	 *  this is a modem line
	 */
	/* receive enable interrupts enabled above via dcparam() */

	if (linenum == MODEM_LINE) {
		dcaddr->dctcr |= (DC_RDTR | DC_RRTS | DC_RSS);
	}

	/*
	 * After DSR first comes up we must wait for the other signals
	 * before commencing transmission.
         */
#ifdef DEBUG
	if (mdcdebug) {
		cprintf("open flag : %x\n", flag);
		if (flag & (O_NDELAY|O_NONBLOCK)) 
			cprintf("flag & (O_NDELAY|O_NONBLOCK)\n");
	}
#endif DEBUG
	if ((flag & (O_NDELAY|O_NONBLOCK)) == 0) {
		/*
		 * Delay before examining other signals if DSR is being followed
		 * otherwise proceed directly to dc_dsr_check to look for
		 * carrier detect and clear to send.
		 */
#ifdef DEBUG
		if (mdcdebug) {
			cprintf("mdcopen: ");
			PRINT_SIGNALS(dcaddr);
		}
#endif DEBUG
		if (dcaddr->dcmsr & DC_DSR) {
			mdcmodem[unit] |= (MODEM_DSR_START|MODEM_DSR);
			tp->t_dev = dev; /* need it for timeouts */
			    /*
			     * Give CD and CTS 30 sec. to 
			     * come up.  Start transmission
			     * immediately, no longer need
			     * 500ms delay.
			     */
			    timeout(mdc_dsr_check, tp, hz*30);
			    mdc_dsr_check(tp);
		}
	}
#	ifdef DEBUG
	if (mdcdebug)
		cprintf("mdcopen:  linenum=%d, state=0x%x, tp=0x%x\n", linenum,
			tp->t_state, tp);
#	endif DEBUG
	if (flag & (O_NDELAY|O_NONBLOCK))
		tp->t_state |= TS_ONDELAY;
	else
		while ((tp->t_state & TS_CARR_ON) == 0) {
			tp->t_state |= TS_WOPEN;
			inuse = tp->t_state&TS_INUSE;
#ifdef DEBUG
			if (mdcdebug) {
				cprintf("mdc_open: going to sleep\n");
			}
#endif DEBUG
			sleep((caddr_t)&tp->t_rawq, TTIPRI);
			/*
			 * See if we were awoken by a false call to the modem
			 * line by a non-modem.
			 */
			if (mdcmodem[unit]&MODEM_BADCALL){
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

mdcclose(dev, flag)
	dev_t dev;
{
	register volatile struct mdz_reg *dcaddr;
	register struct tty *tp;
	register int unit;
	register int linenum;
	register int minor_num;
	register int s;
	extern int wakeup();

	minor_num = minor(dev);
	unit = minor_num >> LINEBITS;
	dcaddr = mdz_regs[unit];
	linenum = minor_num & LINEMASK;

	if (minor_num >= mdc_cnt) {
		return (ENXIO);
	}

#	ifdef DEBUG
	if (mdcdebug)
		cprintf("mdcclose: unit=%d, linenum=%d\n",unit, linenum);
#	endif DEBUG
	tp = &mdc_tty[minor_num];
	/*
	 * Do line discipline specific close functions then return here
	 * in the old line disc for final closing.
	 */
	if (tp->t_line)
		(*linesw[tp->t_line].l_close)(tp);
	/*
	 * dcbreak is write-only and sends a BREAK (SPACE condition) until
	 * the break control bit is cleared. Here we are clearing any
	 * breaks for this line on close.  
	 */
	s = spltty();
	if(mdc_brk[unit] & (1<< linenum)) {
		dcaddr->dcbreak = (mdc_brk[unit] &= ~(1 << linenum));
		wbflush();
	}
	splx(s);
	if ((tp->t_cflag&HUPCL) || (tp->t_state&TS_WOPEN) || 
	    (tp->t_state&TS_ISOPEN)==0) {
		tp->t_state &= ~TS_CARR_ON;   /* prevents recv intr. timeouts */
		/*
		 * Drop appropriate signals to terminate the connection.
		 */
		mdcmodem_active &= ~(1 << minor_num);
		/* 
		 * De-assert modem control leads.  
		 * No disabling of interrupts is done.	Characters read in on
		 * a non-open line will be discarded.
		 */
		if (((tp->t_cflag & CLOCAL) == 0) && (unit != NOMODEM_UNIT) &&
		    (linenum == MODEM_LINE)) {
		    s = spltty();
		    dcaddr->dctcr &= ~(DC_RDTR | DC_RRTS | DC_RSS);
		    /*drop DTR for at least a sec. if modem line*/
#				ifdef DEBUG
		    if (mdcdebug)
			cprintf("mdcclose: DTR drop, state =%x\n"
				,tp->t_state);
#				endif DEBUG
		    tp->t_state |= TS_CLOSING;
		    /*
		     * Wait at most 5 sec for DSR to go off.
		     * Also hold DTR down for a period.
		     */
		    if (dcaddr->dcmsr &  DC_DSR) {
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
		    mdcmodem[unit] = 0;
		}
	}
	/* reset line to default mode */
	mdcsoftCAR[unit] &= ~(1<<(linenum));
	mdcsoftCAR[unit] |= (1<<(linenum)) & mdcdefaultCAR[unit];
	ttyclose(tp);
	tty_def_close(tp);
}

mdcread(dev, uio)
	dev_t dev;
	struct uio *uio;
{
	register struct tty *tp;
	register int minor_num;

	minor_num = minor(dev);
	tp = &mdc_tty[minor_num];
	return ((*linesw[tp->t_line].l_read)(tp, uio));
}

mdcwrite(dev, uio)
	dev_t dev;
	struct uio *uio;
{
	register struct tty *tp;
	register int minor_num;

	minor_num = minor(dev);
	tp = &mdc_tty[minor_num];
	return ((*linesw[tp->t_line].l_write)(tp, uio));
}

/*
 * Device interrupt service routine.  Examine the csr to see what the 
 * interrupt is all about and dispatch the appropriate handler.
 */
mdcintr(unit)
	register int unit;
{
	register volatile struct mdz_reg *dcaddr;
	register unsigned int csr;

	if (unit > nMDC) {	/* sanity check */
		cprintf("mdcintr: invalid unit %d.\n",unit);
		return;
	}
	dcaddr = mdz_regs[unit];
	csr = dcaddr->dccsr;
	/*
	 * OUTPUT:
	 * The transmitter buffer is now empty for the specified line.  Call
	 * the pdma routine to place a new character in the tbuf.
	 */
	if (csr & DC_TRDY) {
		_mdcpdma(unit);
	}
	/*
	 * INPUT:
	 * A character has been received.  Pass this up to the user application.
	 */
	if (csr & DC_RDONE) {
		mdcrint(unit);
	}
}

/*
 * A character has been received.  Pass this character up to the user level
 * application after examining for errors such as parity errors or breaks.
 */
mdcrint(unit)
	register int unit;
{
	register volatile struct mdz_reg *dcaddr;
	register struct tty *tp;
	register u_short c;	/* rbuf register is 16 bits long */
	struct tty *tp0;
	register int flg;
	register int linenum;
	int overrun = 0;
	u_short data;
	int counter = 0;


	tp0 = &mdc_tty[0];
	dcaddr = mdz_regs[unit];
	while (dcaddr->dccsr&DC_RDONE) {		/* character present */
		c = dcaddr->dcrbuf;
		linenum = (c>>8)&LINEMASK;
		tp = tp0 + linenum + (NDCLINE * unit);
		if (tp >= &mdc_tty[mdc_cnt])
			continue;
		if ((tp->t_state & TS_ISOPEN) == 0) {
			wakeup((caddr_t)&tp->t_rawq);
			continue;
		}
		flg = tp->t_iflag;

		/* DC_FE is interpreted as a break */
		if (c & DC_FE) {
			/*
			 * If configured for trusted path, initiate
			 * trusted path handling.
			 */
#			ifdef DEBUG
			if (mdcdebug)
				mprintf("mdcrint: BREAK RECEIVED on unit = %d, linenum = %d\n", unit, linenum);
#			endif DEBUG
			if (do_tpath) {
				tp->t_tpath |= TP_DOSAK;
				(*linesw[tp->t_line].l_rint)(c, tp);
				break;
			}
			if (flg & IGNBRK)
				continue;
			if (flg & BRKINT) {
				if ((tp->t_lflag_ext & PRAW) && 
					(tp->t_line != TERMIODISC))
					c = 0;
				else {
				        ttyflush(tp, FREAD | FWRITE);
					gsignal(tp->t_pgrp, SIGINT);
					continue;
				}
			}
			/*
			 * TERMIO: If neither IGNBRK or BRKINT is set, a
			 * break condition is read as a single '\0',
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
		/* Parity Error */
		else if (c & DC_PE){
			/*
			 * If input parity checking is not enabled, clear out
			 * parity error in this character.
			 */
#			ifdef DEBUG
			if (mdcdebug > 1)
				mprintf("mdcrint: Parity Error\n");
#			endif DEBUG
			if ((flg & INPCK) == 0) {
				c &= ~DC_PE;
			}
			else {
				if (flg & IGNPAR) {
					continue;
				}
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
				else {
					c = 0;
				}
			}
		}
		/*
		 * Data Overrun.  Only complain about this once per routine
		 * entry.  Increment error count to record the number of
		 * errors.
		 */
		if (c&DC_DO) {
			if(overrun == 0) {
				printf("mdc%d: input silo overflow\n", 0);
				overrun = 1;
			}
		        mdc_softc[unit].sc_softcnt[linenum]++; /* overrun errs */
		}
		/*
		 * Strip the character to be 7 bits in length.
		 */
		if (flg & ISTRIP){
			c &= 0177;
		}
		/*
		 * Strip the character to be 8 bits in length.
		 */
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
mdcioctl(dev, cmd, data, flag)
	dev_t dev;
	register int cmd;
	caddr_t data;
	int flag;
{
	register volatile struct mdz_reg *dcaddr;
	register int unit;
	register int linenum;
	register struct tty *tp;
	register int s;
	struct uba_device *ui;
	struct devget *devget;
	int error;
	int minor_num;

	minor_num = minor(dev);
	unit = minor_num >> LINEBITS;
	dcaddr = mdz_regs[unit];
	linenum = minor_num & LINEMASK;

	if (minor_num >= mdc_cnt) {
		return (ENXIO);
	}
	tp = &mdc_tty[minor_num];
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
				mdcparam(minor_num);
				break;
		}
		return (error);
	}
	switch (cmd) {

	/*
	 * Begin a break sequence.  The line will remain in a break state
	 * until cleared.
	 */
	case TIOCSBRK:
	    	timeout(mdcsetbreak, tp, mdc_speeds[tp->t_cflag & CBAUD].delay_set_break); 
		TTY_SLEEP(tp, (caddr_t)&tp->t_dev, TTOPRI);
		s = spltty();
		if((mdc_brk[unit] & (1<< linenum)) == 0) {
		  dcaddr->dcbreak = (mdc_brk[unit] |= (1 << linenum));
		  wbflush();
		}
		splx(s);
#ifdef DEBUG
	if (mdcdebug)
	   mprintf("TIOCSBRK: unit = %d, linenum = %d, mdc_brk = %d, dccsr = 0x%x\n", unit, linenum, mdc_brk[unit], dcaddr->dccsr);
#endif DEBUG
		break;
	/*
	 * End a break sequence.
	 */
	case TIOCCBRK:
		s = spltty();
		if(mdc_brk[unit] & (1<< linenum)) {
		   dcaddr->dcbreak = (mdc_brk[unit] &= ~(1 << linenum));
		   wbflush();
		}
		splx(s);
#ifdef DEBUG
	if (mdcdebug)
	   mprintf("TIOCCBRK: unit = %d, linenum = %d, mdc_brk = %d, dccsr = 0x%x\n", unit, linenum, mdc_brk[unit], dcaddr->dccsr);
#endif DEBUG
		break;

	/*
	 * Sets DTR and RTS in the tcr register.
	 */
	case TIOCSDTR:
		(void) mdcmctl(dev, SML_DTR | SML_RTS, DMBIS);
		break;
	/*
	 * Clears DTR and RTS in the tcr register.
	 */
	case TIOCCDTR:
		(void) mdcmctl(dev, SML_DTR | SML_RTS, DMBIC);
		break;
	/*
	 * TIOCSMLB and TIOCCMLB are used to set and clear the maintenance
	 * loopback mode.  This is useful to test the lines without the need
	 * for loopback connectors.  Restrict access to root users.
	 *
	 * Note that setting this bit will loopback ALL the lines on this
	 * particular DC chip,  not just the specific line refered to by tp!
	 * Just to be on the safe side, do not allow this on the console unit.
	 * Perhaps this ioctl is to dangerous to include.
	 */
	case TIOCSMLB:
		if ((u.u_uid) || (unit == CONSOLE_UNIT)) {
			return(EPERM);
		}
		s = spltty();
		dcaddr->dccsr |= (DC_MAINT);
		wbflush();
		splx(s);
		break;

	case TIOCCMLB:
		if ((u.u_uid) || (unit == CONSOLE_UNIT)) {
			return(EPERM);
		}
		s = spltty();
		dcaddr->dccsr &= ~(DC_MAINT);
		wbflush();
		splx(s);
		break;
	/*
	 * Set the specified generic modem control attributes.
	 * No way this works!  The only relevant settings are DTR and RTS,
	 * all others are not even looked at.
	 */
	case TIOCMSET:
		(void) mdcmctl(dev, dmtomdc(*(int *)data), DMSET);
		break;
	/*
	 * Set the specified bits in the generic modem control attributes.
	 * No way this works!  The only relevant settings are DTR and RTS,
	 * all others are not even looked at.
	 */
	case TIOCMBIS:
		(void) mdcmctl(dev, dmtomdc(*(int *)data), DMBIS);
		break;
	/*
	 * Clear the specified bits from the generic modem control attributes.
	 * No way this works!  The only relevant settings are DTR and RTS,
	 * all others are not even looked at.
	 */
	case TIOCMBIC:
		(void) mdcmctl(dev, dmtomdc(*(int *)data), DMBIC);
		break;
	/*
	 * Return the current generic modem control attributes.  Generally
	 * speaking this will return garbage.
	 * No way this works!  The only relevant settings are DTR and RTS,
	 * all others are not even looked at.
	 */
	case TIOCMGET:
		*(int *)data = mdctodm(mdcmctl(dev, 0, DMGET));
		break;

	/*
	 * Specify that this line is to be considered a direct connect
	 * (no modem) line.
	 */
	case TIOCNMODEM:  /* ignore modem status */
		/*
		 * By setting the software representation of modem signals
		 * to "on" we fake the system into thinking that this is an
		 * established modem connection.
		 */
		s = spltty();
		mdcsoftCAR[unit] |= (1<<(linenum));
		if (*(int *)data) /* make mode permanent */
			mdcdefaultCAR[unit] |= (1<<(linenum));
		tp->t_state |= TS_CARR_ON;
		tp->t_cflag |= CLOCAL;		/* Map to termio */
		splx(s);
		break;

	/*
	 * Specify that this line is to be considered a modem 
	 * (not direct connect) line.  This means pay attention to the 
	 * modem control leads.
	 */
	case TIOCMODEM:  /* look at modem status - sleep if no carrier */
		s = spltty();
		/*
		 * The only lines that allow modem control are line 2 of 
		 * DC0 and DC1.  Should the ioctl fail?
		 */
		if ((unit == NOMODEM_UNIT) || (linenum != MODEM_LINE)) {
			tp->t_cflag |= CLOCAL;	/* paranoia */
			splx(s);
			break;
		}
		mdcsoftCAR[unit] &= ~(1<<(linenum));
		if (*(int *)data) { /* make mode permanent */
			mdcdefaultCAR[unit] &= ~(1<<(linenum));
		}
		/*
		 * See if signals necessary for modem connection are present
		 *
		 * dc7085 chip on PMAX only provides DSR
		 *
		 */
		if ((dcaddr->dcmsr & DC_XMIT) == DC_XMIT) {
			tp->t_state &= ~(TS_ONDELAY);
			tp->t_state |= TS_CARR_ON;
			mdcmodem[unit] = MODEM_CTS|MODEM_CD|MODEM_DSR;
		} else {
			tp->t_state &= ~(TS_CARR_ON);
			mdcmodem[unit] &= ~(MODEM_CTS|MODEM_CD|MODEM_DSR);
		}
		tp->t_cflag &= ~CLOCAL;		/* Map to termio */
		splx(s);
		break;

	case TIOCWONLINE:
		s = spltty();
		/*
		 * See if signals necessary for modem connection are present
		 *
		 * dc7085 chip on PMAX only provides DSR
		 * 
		 */
		if ((dcaddr->dcmsr & DC_XMIT) == DC_XMIT) {
			tp->t_state |= TS_CARR_ON;
			tp->t_state &= ~(TS_ONDELAY);
			mdcmodem[unit] = MODEM_CTS|MODEM_CD|MODEM_DSR;
		} else {
		    while ((tp->t_state & TS_CARR_ON) == 0)
			sleep((caddr_t)&tp->t_rawq, TTIPRI);
		}
		splx(s);
		break;

	case DEVIOCGET:				/* device status */
		devget = (struct devget *)data;
		bzero(devget,sizeof(struct devget));
		devget->category = DEV_TERMINAL;	/* terminal cat.*/
		devget->bus = DEV_NB;			/* NO bus	*/
		bcopy(DEV_MF_SLU,devget->interface,
		      strlen(DEV_MF_SLU));		/* interface	*/
		bcopy(DEV_UNKNOWN,devget->device,
		      strlen(DEV_UNKNOWN));		/* terminal	*/
		devget->adpt_num = 0;			/* NO adapter	*/
		devget->nexus_num = 0;			/* fake nexus 0 */
		devget->bus_num = 0;			/* NO bus	*/
		devget->ctlr_num = unit;		/* cntlr number */
		devget->slave_num = linenum;		/* line number	*/
		bcopy("mdc", devget->dev_name, 4);	/* Ultrix "mdc"	*/
		devget->unit_num = linenum;		/* dc line?	*/
		devget->soft_count =
		      mdc_softc[unit].sc_softcnt[linenum]; /* overrun errs */
		devget->hard_count = 0;			/* hard err cnt */
		devget->stat = 0;
		devget->category_stat = (DEV_MODEM|DEV_MODEM_ON);
		if ((unit == NOMODEM_UNIT) || (linenum != MODEM_LINE)) {
			devget->category_stat &= DEV_MODEM;
		}
		if (tp->t_cflag & CLOCAL) {
		    devget->category_stat &= ~DEV_MODEM_ON;
		} 
		break;

	default:
		if (u.u_procp->p_progenv == A_POSIX) 
			return (EINVAL);
		return (ENOTTY);
	}
	return (0);
}
/*
 * Part of a worthless generic modem lead handling mechanism.  The only 2 bits
 * here that are of any use are DC_RRTS and DC_RDTR.
 */
dmtomdc(bits)
	register int bits;
{
	register int b;

	b = (bits >>1) & 0370;
	if (bits & SML_RTS) b |= DC_RRTS;
	if (bits & SML_DTR) b |= DC_RDTR;
	return(b);
}
/*
 * Part of a worthless generic modem lead handling mechanism.  The only 2 bits
 * here that are of any use are DC_RRTS and DC_RDTR.
 */
mdctodm(bits)
	register int bits;
{
	register int b;

	b = (bits << 1) & 0360;
	if (bits & DC_DSR) b |= SML_DSR;
	if (bits & DC_RDTR) b |= SML_DTR;
	if (bits & DC_RRTS) b |= SML_RTS;
	return(b);
}

/*
 * Set device line parameters.
 */
mdcparam(minor_num)
	register int minor_num;
{
	register volatile struct mdz_reg *dcaddr;
	register struct tty *tp;
	register u_short lpr;
	register int unit;
	register int linenum;

	unit = minor_num >> LINEBITS;
	linenum = minor_num & LINEMASK;
	dcaddr = mdz_regs[unit];
	tp = &mdc_tty[minor_num];

	/*
	 * Set master scan enable for this line.  This also clears out the
	 * receive and transmit interrupt enables which will be conditionally
	 * applied later.  Preserve maintenance loopback mode if specified.
	 */
	if (dcaddr->dccsr & DC_MAINT) {
		dcaddr->dccsr = (DC_MSE | DC_MAINT); 
	}
	else {
		dcaddr->dccsr = DC_MSE; 
	}
	/*
	 * Setting a baud rate of 0 closes down a line (environment dependent).
	 * Deassert DTR and RTS to drop the modem connection.
	 */
	if ((((tp->t_cflag&CBAUD)==B0) && (u.u_procp->p_progenv != A_POSIX)) ||
	    (((tp->t_cflag_ext & CBAUD)==B0) &&
	     (u.u_procp->p_progenv == A_POSIX))) {
	    mdcmodem_active &= ~(1 << minor_num);	
	    /*
	     * We only have control for DTR and RTS on the modem line.
	     */
	    if ((unit != NOMODEM_UNIT) && (linenum == MODEM_LINE)) {
		dcaddr->dctcr &= ~(DC_RDTR | DC_RRTS | DC_RSS);
	    }
	    return;
	}
	/*
 	 * The console line must be set to 8 bits no parity.  The baud
	 * rate may be changed from its default value of 1200 by an
	 * environment variable.  May want to make sure the tty struct
	 * gets modified to specify these attributes as well.
	 */
	if ((unit == CONSOLE_UNIT) && (linenum == CONSOLE_LINE)) {
		tp->t_cflag &= ~(CBAUD);
		tp->t_cflag_ext &= ~(CBAUD);
		tp->t_cflag |= (console_baud & CBAUD);
		tp->t_cflag_ext |= (console_baud & CBAUD);
		dcaddr->dclpr = (linenum | mdc_speeds[console_baud].baud_param |
				BITS8 | DC_RE);
	} 
	/*
	 * Set parameters in accordance with user specification.
	 */
	else {
		lpr = (mdc_speeds[tp->t_cflag&CBAUD].baud_param) | (linenum);
		/*
		 * Berkeley-only dinosaur
		 */
		if (tp->t_line != TERMIODISC) {
			if ((tp->t_cflag_ext&CBAUD) == B110) {
				lpr |= TWOSB;
				tp->t_cflag |= CSTOPB;
			}
		}
		/*
		 * Set device registers according to the specifications of the
		 * termio structure.
		 */
		if (tp->t_cflag & CREAD) {
			lpr |= DC_RE;	
		}
		else  {
			lpr &= ~DC_RE;	
		}
		if (tp->t_cflag & CSTOPB) {
			lpr |= TWOSB;
		}
		else {
			lpr &= ~TWOSB;
		}
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
		dcaddr->dclpr = lpr;
	}
	dcaddr->dccsr |= (DC_TIE | DC_RIE);
	wbflush();
}

/*
 * A transmitter ready interrupt has been generated on this unit.  This means
 * that the transmitter buffer for a line is empty and now ready to transmit
 * a new character.  If there are already characters stacked up in the pdma
 * structure then just stuff the tbuf with the character.  Otherwise the
 * present list of characters is empty so call the xint routine to setup a
 * new pdma chain if there are any new characters to be transmitted.
 */
_mdcpdma(unit)
	register int unit;
{
        register volatile struct mdz_reg *dcaddr;
	register struct dcpdma *dp;
	register int linenum;

        dcaddr = mdz_regs[unit];
	linenum = (dcaddr->dccsr >> 8) & LINEMASK;	

	dp = &mdc_pdma[unit][linenum];

	if (dp->p_mem == dp->p_end) {
	    	mdcxint(unit, linenum);
	} else {
	    	dcaddr->dctbuf = (unsigned char)(*dp->p_mem++);
		wbflush();
	}
}

/*
 * A transmitter interrupt has been generated for a line that does not have
 * a pdma chain setup.  Setup the pdma chain and call the start routine to
 * initiate output.
 */
mdcxint(unit, linenum)
	register int unit;
	register int linenum;
{
	register volatile struct mdz_reg *dcaddr;
	register struct dcpdma *dp;
	register struct tty *tp;

	dcaddr = mdz_regs[unit];
	tp = &mdc_tty[linenum + (unit * NDCLINE)];

#ifdef DEBUG
	if (mdcdebug > 4)
		mprintf("mdcxint: linenum = %d, tp = %x, c_cc = %d\n", linenum, tp, tp->t_outq.c_cc);
#endif DEBUG

	tp->t_state &= ~TS_BUSY;
	dp = &mdc_pdma[unit][linenum];
	if (tp->t_state & TS_FLUSH) {
		tp->t_state &= ~TS_FLUSH;
	} else {
		/*
		 * Remove characters that have already been transmitted from
		 * the tty output queue.  Set the p_end and p_mem pointers to
		 * be the same to signify an empty pdma chain.
		 */
	    	ndflush(&tp->t_outq, dp->p_mem-tp->t_outq.c_cf);
		dp->p_end = dp->p_mem = tp->t_outq.c_cf;
	}
	if (tp->t_line) {
		(*linesw[tp->t_line].l_start)(tp);
	}
	else {
		mdcstart(tp);
	}

	/*
	 * The BUSY flag will not be set in two cases:		
	 *   1. if there are no more chars in the outq OR
	 *   2. there are chars in the outq but tty is in
	 *      stopped state.				
	 *
	 * Turn off line enable.
	 */
	if ((tp->t_state&TS_BUSY) == 0) {
	    	dcaddr->dctcr &= ~(1<<linenum);
	}
}

/*
 * Begin transmission on a line.  Obtain a count of the number of characters
 * to be sent.  If there are characters to send, update the pdma struct to
 * point to the beginning of these characters followed by a count.  Then set
 * the transmitter interrupt enable for this line.  This will cause an
 * interrupt when the line is ready to accept a character.  The interrupt will
 * call the mdcxint routine to stuff the character in the transmitter buffer.
 */
mdcstart(tp)
	register struct tty *tp;
{
	register volatile struct mdz_reg *dcaddr;
	register struct dcpdma *dp;
	register int cc;
	register int unit, linenum;
	int s;

	s = spltty();
	linenum = minor(tp->t_dev) & LINEMASK;
	unit = minor(tp->t_dev) >> LINEBITS;
	dcaddr = mdz_regs[unit];
	/*
	 * Do not do anything if currently delaying, or active.  Also only
	 * transmit when CTS is up.
	 */
	if ((tp->t_state & (TS_TIMEOUT|TS_BUSY|TS_TTSTOP)) ||
		(((tp->t_cflag & CLOCAL) == 0)
		&& ((tp->t_state&TS_CARR_ON) && (mdcmodem[unit]&MODEM_CTS)==0)))
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

#ifdef DEBUG
	if (mdcdebug > 7) {
		mprintf("mdcstart: linenum = %d, tp = %x, c_cc = %d\n", linenum, tp, tp->t_outq.c_cc);
	}
#endif DEBUG

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
	dp = &mdc_pdma[unit][linenum];
	dp->p_end = dp->p_mem = tp->t_outq.c_cf;
	dp->p_end += cc;
	dcaddr->dccsr |= DC_TIE;
	dcaddr->dctcr |= (1<<linenum);
	wbflush();
out:
	splx(s);
}

mdcstop(tp, flag)
	register struct tty *tp;
{
	register struct dcpdma *dp;
	register int s;
	register int unit, linenum;

	unit = minor(tp->t_dev) >> LINEBITS;
        linenum = minor(tp->t_dev) & LINEMASK;
	dp = &mdc_pdma[unit][linenum];
	s = spltty();
	if (tp->t_state & TS_BUSY) {
	    	dp->p_end = dp->p_mem;
		if ((tp->t_state&TS_TTSTOP)==0)
			tp->t_state |= TS_FLUSH;
	}
	splx(s);
}

/*
 * This routine is part of a scheme to provide a generic modem control
 * interface.  Strictly speaking it is not supported and is more bother than
 * it is worth.  This routine reads the status of the present modem control
 * leads and represents them generically in the mbits return value.
 */
mdcmctl(dev, bits, how)
	dev_t dev;
	int bits, how;
{
	register volatile struct mdz_reg *dcaddr;
	register int unit, linenum, mbits;
	register int b, s;

	unit = minor(dev) >> LINEBITS;
	dcaddr = mdz_regs[unit];
	linenum = minor(dev) & LINEMASK;
	if ((unit == NOMODEM_UNIT) || (linenum != MODEM_LINE))
		return(0);	
	s = spltty();
	mbits = (dcaddr->dctcr & DC_RDTR) ? SML_DTR : 0;
	mbits |= (dcaddr->dctcr & DC_RRTS) ? SML_RTS : 0;
	mbits |= (dcaddr->dcmsr & DC_CD) ? SML_CAR : 0;
	mbits |= (dcaddr->dcmsr &  DC_DSR) ? SML_DSR : 0;
	mbits |= (dcaddr->dcmsr & DC_CTS) ? SML_CTS : 0;
	switch (how) {
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
	if (mbits & SML_DTR) {
		dcaddr->dctcr |= (DC_RDTR | DC_RRTS);
	}
	else {
		dcaddr->dctcr &= ~(DC_RDTR | DC_RRTS | DC_RSS);
	}
	wbflush();
	(void) splx(s);
	return(mbits);
}

#ifdef DEBUG
int mdcscan_ctr = 1;
#endif DEBUG

/*
 * The DC chip does not interrupt on modem transitions.  For this reason a
 * scanner routine is needed to be called frequently to check the status of
 * the modem control leads and to react to changes of these leads.
 */
mdcscan()
{
	register volatile struct mdz_reg *dcaddr;
	register struct tty *tp;
	register int unit;
	register int linenum;
	register u_short dcscan_modem;

    for (unit = 0; unit < nMDC; unit++) {
        if (unit != NOMODEM_UNIT) {
	    /*
	     * Obtain a copy of the modem status register.  If the value has not
	     * changed since the last iteration then there is no work to be done
	     * here.  
	     */
	    dcaddr = mdz_regs[unit];
#ifdef DEBUG
	    if (mdcdebug > 5) {
		if (mdcscan_ctr++ == 45) {
			cprintf("dcscan: ");
			PRINT_SIGNALS(dcaddr);
			mdcscan_ctr = 1;
		}
	    }
#endif DEBUG
	    dcscan_modem = dcaddr->dcmsr;	
	    if (dcscan_modem != mdcscan_previous[unit]) {
	        tp = &mdc_tty[MODEM_LINE + (unit * NDCLINE)];
	        if ((tp->t_cflag & CLOCAL) == 0) {
		    /*
		     * Drop DTR immediately if DSR has gone away.
		     * If really an active close then do not
		     *    send signals.
		     */
		    if (!(dcscan_modem &  DC_DSR)) {
		        if (tp->t_state&TS_CLOSING) {
			    untimeout(wakeup, (caddr_t) &tp->t_dev);
			    wakeup((caddr_t) &tp->t_dev);
		        }
		        if (tp->t_state&TS_CARR_ON) {
			    mdc_tty_drop(tp);
		        }
		    } else {		/* DSR has come up */
		        /*
		         * If DSR comes up for the first time we allow
		         * 30 seconds for a live connection.
		         */
		        if ((mdcmodem[unit] & MODEM_DSR)==0) {
			    mdcmodem[unit] |= (MODEM_DSR_START|MODEM_DSR);
			        /*
			         * we should not look for CTS|CD for about
			         * 500 ms.
			         */
			        timeout(mdc_dsr_check, tp, hz*30);
			        mdc_dsr_check(tp);
		        }
		    }
		    /*
		     * look for modem transitions in an already
		     * established connection.
		     *
		     */
		    if (tp->t_state & TS_CARR_ON) {
			    if (dcscan_modem & DC_CD) {
			        /*
			         * CD has come up again.
			         * Stop timeout from occurring if set.
			         * If interval is more than 2 secs then
			         * drop DTR.
			         */
			        if ((mdcmodem[unit] & MODEM_CD) == 0) {
				    untimeout(mdc_cd_drop, tp);
				    if (mdc_cd_down(tp)) {
				        /* drop connection */
				        mdc_tty_drop(tp);
				    }
				    mdcmodem[unit] |= MODEM_CD;
			        }
			    } else {
			        /*
			         * Carrier must be down for greater than
			         * 2 secs before closing down the line.
			         */
			        if (mdcmodem[unit] & MODEM_CD) {
				    /* only start timer once */
				    mdcmodem[unit] &= ~MODEM_CD;
				    /*
				     * Record present time so that if carrier
				     * comes up after 2 secs, the line will drop
				     */
				    mdctimestamp[unit] = time;
				    timeout(mdc_cd_drop, tp, hz * 2);
			        }
			    }
		    	
			    /* CTS flow control check */
		    	
			    if (!(dcscan_modem & DC_CTS)) {
			        /*
			         * Only allow transmission when CTS is set.
			         */
			        tp->t_state |= TS_TTSTOP;
			        mdcmodem[unit] &= ~MODEM_CTS;
#ifdef DEBUG
			        if (mdcdebug)
				    cprintf("mdcscan: CTS stop, tp=%x,unit=%d\n",tp,unit);
#endif DEBUG
			        mdcstop(tp, 0);
			    } else if (!(mdcmodem[unit] & MODEM_CTS)) {
			        /*
			         * Restart transmission upon return of CTS.
			         */
			        tp->t_state &= ~TS_TTSTOP;
			        mdcmodem[unit] |= MODEM_CTS;
#ifdef DEBUG
			        if (mdcdebug)
				    cprintf("mdcscan: CTS start, tp=%x,unit=%d\n",tp,unit);
#endif DEBUG
			        mdcstart(tp);
			    }
		        }
    	
		        /*
		         * See if a modem transition has occured.  If we are 
		         * waiting for this signal, cause action to be take via
		         * mdc_start_tty.
		         */
		        if (((dcscan_modem & DC_XMIT) == DC_XMIT) &&
			    (!(mdcmodem[unit] & MODEM_DSR_START)) &&
			    (!(tp->t_state & TS_CARR_ON))) {
#ifdef DEBUG
				    if (mdcdebug)
				        cprintf("mdcscan: MODEM transition: dcscan_modem = %x, mdcscan_previous = %x\n", dcscan_modem, mdcscan_previous[unit]);
#endif DEBUG
				    mdc_start_tty(tp);
		        }
	        }
		/*
		 * Save a copy of the modem status for comparison during the
		 * next iteration.
		 */
	        mdcscan_previous[unit] = dcscan_modem; 
	    }
      }
    }
    /*
     * Schedule the next scanner iteration.  Scan at a much higher frequency
     * when a modem line is active.
     */
    if (mdcmodem_active) {
        timeout(mdcscan, (caddr_t)0, hz/40);
    }
    else {
        timeout(mdcscan, (caddr_t)0, hz);
    }
}

/*
 * Write a character to the console.
 */
mdcputc(c)
	register int c;
{
    mdc_putc(c);
    if ( c == '\n')
	mdc_putc('\r');
}

/*
 * This routine outputs one character to the console.
 *
 * Called for system startup messages.  Also used for kernel printf's.
 * Not called from user level; ie a console getty does not use this.
 *
 * Characters must be printed without disturbing
 * output in progress on other lines!
 * This routines works with the SLU in interrupt or
 * non-interrupt mode of operation. BULL!
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
mdc_putc(c)
	int c;
{
	register volatile struct mdz_reg *dcaddr;
	register int	s; 
	register u_short oldtcr;
	register int	ln, linenum, timo;

	dcaddr = mdz_regs[CONSOLE_UNIT];
	linenum = CONSOLE_LINE;
	c &= CHAR_MASK;			/* Strip character to 8-bits */
	s = splhigh();
	/*
	 * Get a copy of the present transmit enable and DTR, RTS.
	 * Next set transmit enable for this line.
	 */
	oldtcr = (dcaddr->dctcr & (1<<linenum));
	dcaddr->dctcr |= (1<<linenum);
	while (1) {
		/*
		 * Wait for the chip to be ready.  Impose a timer so that the
		 * system does not hang if this bit fails to clear due to a
		 * hardware error.  If the bit fails to clear then don't even
		 * try to send out this character.
		 */
		timo = 1000000;
		while ((dcaddr->dccsr&DC_TRDY) == 0) {	
			if(--timo == 0)
				break;
		}
		if(timo == 0)
			break;
		/*
		 * The DC chip has stoppped on a line that is ready to be
		 * loaded with another transmit character.  If the line ready
		 * for transmission is not the console line then clear
		 * transmitter enable for the non-console line so that we
		 * won't be bothered by that line again until the completion of
		 * this putc operation.
		 */
		ln = (dcaddr->dccsr>>8) & LINEMASK;
		if (ln != linenum) {
			oldtcr |= (1 << ln);
			dcaddr->dctcr &= ~(1 << ln);
			continue;
		}
		/*
		 * Output the character by writing into the transmitter buffer.
		 * Provide a delay to allow time for the char to go out.  The
 		 * registers should not be read for 1.4 microseconds.
		 * Should this routine check to see if the console line is
		 * presently asserting a break?
		 */
		dcaddr->dctbuf = c;
		DELAY(5); 
		/*
		 * Wait for the character to be transmitted.
		 */
		while (1) {
			timo = 1000000;
			while ((dcaddr->dccsr&DC_TRDY) == 0) {	
				if(--timo == 0)
					break;
			}
			if(timo == 0)
				break;
			ln = (dcaddr->dccsr>>8) & LINEMASK;
			if (ln != linenum) {
				oldtcr |= (1 << ln);
				dcaddr->dctcr &= ~(1 << ln);
				continue;
			}
			break;
		}
		break;
	}
	/*
	 * Clear transmit enable for this line.  If this bit was set when this
	 * routine was originaly entered it will then be restored.
	 */
	dcaddr->dctcr &= ~(1<<linenum);
	if (oldtcr != 0)
		dcaddr->dctcr |= oldtcr;
	wbflush();
	splx(s);
}

/*
 * This routine operates on the following assumptions:
 * 1. putc must have happened first, so SLU already inited.
 * 2. getc will happed before slu reveive interrupt enabled so
 *    don't need to worry about int_req or int_msk registers.
 */
mdcgetc()
{
	register volatile struct mdz_reg *dcaddr;
	register u_short c;
	register int linenum;

	dcaddr = mdz_regs[CONSOLE_UNIT];
	linenum = CONSOLE_LINE;

	while (1) {
		/*
		 * This will hang the system if RDONE never gets set!
		 */
		while ((dcaddr->dccsr&DC_RDONE) == 0) ;
		c = dcaddr->dcrbuf;
		if(((c >> 8) & LINEMASK) != linenum)	/* wrong line mumber */
			continue;
		/*
		 * Toss the character away if there is an error.  I wonder if
		 * throwing away parity errors is a bit harsh.
		 */
		if(c & (DC_DO|DC_FE|DC_PE))	
			continue;
		break;
	}
	return(c & CHAR_MASK);
}

/*
 * Modem Control Routines
 */

/*
 *
 * Function:
 *
 *	mdc_cd_drop
 *
 * Functional description:
 *
 * 	Determine if carrier has dropped.  If so call mdc_tty_drop to terminate
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
mdc_cd_drop(tp)
register struct tty *tp;
{
	register volatile struct mdz_reg *dcaddr;
	register int unit;

	unit = minor(tp->t_dev) >> LINEBITS;
	dcaddr = mdz_regs[unit];
	if ((tp->t_state & TS_CARR_ON) && (!(dcaddr->dcmsr & DC_CD))) {
#ifdef DEBUG
	    if (mdcdebug)
		cprintf("mdc_cd_drop: no CD, tp = %x, unit = %d\n", tp, unit);
#endif DEBUG
	    mdc_tty_drop(tp);
	    return;
	}
	mdcmodem[unit] |= MODEM_CD;
#ifdef DEBUG
	if (mdcdebug)
	    cprintf("mdc_cd_drop:  CD is up, tp = %x, unit = %d\n", tp, unit);
#endif DEBUG
}
 
/*
 *
 * Function:
 *
 *	mdc_dsr_check
 *
 * Functional description:
 *
 *	DSR must be asserted for a connection to be established.  Here we 
 *	either start or terminate a connection on the basis of DSR.
 *
 * Arguements:
 *
 *	register struct tty *tp  -  terminal pointer (for terminal attributes)
 *
 * Return value:
 *
 *	none
 *
 */
mdc_dsr_check(tp)
register struct tty *tp;
{
	register volatile struct mdz_reg *dcaddr;
	register int unit;

	unit = minor(tp->t_dev) >> LINEBITS;
	dcaddr = mdz_regs[unit];
#	ifdef DEBUG
       	if (mdcdebug) {
       	    cprintf("mdc_dsr_check0:  tp=%x\n", tp);
	    PRINT_SIGNALS(dcaddr);
	}
#	endif DEBUG
	if (mdcmodem[unit] & MODEM_DSR_START) {
	    mdcmodem[unit] &= ~MODEM_DSR_START;
	    if ((dcaddr->dcmsr & DC_XMIT) == DC_XMIT) {
		mdc_start_tty(tp);
	    }
	    return;
	}
	if ((tp->t_state&TS_CARR_ON)==0)
		mdc_tty_drop(tp);
}

/*
 *
 * Function:
 *
 *	mdc_cd_down
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
mdc_cd_down(tp)
register struct tty *tp;
{
        register int msecs, unit;

	unit = minor(tp->t_dev) >> LINEBITS;
	msecs = 1000000 * (time.tv_sec - mdctimestamp[unit].tv_sec) + 
		(time.tv_usec - mdctimestamp[unit].tv_usec);
	if (msecs > 2000000){
#		ifdef DEBUG
		if (mdcdebug)
			cprintf("mdc_cd_down: msecs > 20000000\n");
#		endif DEBUG
		return(1);
	}
	else{
#		ifdef DEBUG
		if (mdcdebug)
			cprintf("mdc_cd_down: msecs < 20000000\n");
#		endif DEBUG
		return(0);
	}
}

/*
 *
 * Function:
 *
 *	mdc_tty_drop
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
mdc_tty_drop(tp)
struct tty *tp;
{
	register volatile struct mdz_reg *dcaddr;
	register int unit;
	register int minor_num;

	minor_num = minor(tp->t_dev);
	unit = minor_num >> LINEBITS;
	dcaddr = mdz_regs[unit];
	mdcmodem_active &= ~(1 << minor_num);
	if (tp->t_flags & NOHANG)
		return;
#	ifdef DEBUG
	if (mdcdebug)
		cprintf("mdc_tty_drop: minor=%d\n",minor_num);
#	endif DEBUG
	/* 
	 * Notify any processes waiting to open this line.  Useful in the
	 * case of a false start.
	 */
	mdcmodem[unit] = MODEM_BADCALL; 
	tp->t_state &= ~(TS_CARR_ON|TS_TTSTOP|TS_BUSY|TS_ISUSP);
	wakeup((caddr_t)&tp->t_rawq);
	gsignal(tp->t_pgrp, SIGHUP);
	gsignal(tp->t_pgrp, SIGCONT);
	dcaddr->dctcr &= ~(DC_RDTR | DC_RRTS | DC_RSS);
}
/*
 *
 * Function:
 *
 *	mdc_start_tty
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
mdc_start_tty(tp)
	register struct tty *tp;
{
        register int unit;
        register int linenum;
	register int minor_num;

	minor_num = minor(tp->t_dev);
	unit = minor_num >> LINEBITS;
	linenum = minor_num & LINEMASK;
	/*
	 * Setting this variable will cause the scanner to be called with
	 * increased frequency.  As a sanity check, only set the bit if this
	 * is a modem line to prevent the possible case where the scanner could
	 * be increased for a no modem line.
	 */
	if ((unit != NOMODEM_UNIT) && (linenum == MODEM_LINE)) {
		mdcmodem_active |= (1 << minor_num);	
	}
	tp->t_state &= ~(TS_ONDELAY);
	tp->t_state |= TS_CARR_ON;
#	ifdef DEBUG
        if (mdcdebug)
	       cprintf("mdc_start_tty:  tp=%x\n", tp);
#	endif DEBUG
	if (mdcmodem[unit] & MODEM_DSR)
		untimeout(mdc_dsr_check, tp);
	mdcmodem[unit] |= MODEM_CD|MODEM_CTS|MODEM_DSR;
	mdctimestamp[unit].tv_sec = mdctimestamp[unit].tv_usec = 0;
	wakeup((caddr_t)&tp->t_rawq);
}

/*
 * Return 1 if the baud rate is supported, 0 if not supported.
 *
 * This scheme could be improved for the case of the console line which
 * really can't be set to any arbirtrary baud rate.
 */
mdcbaudrate(speed)
int speed;
{
	return(mdc_speeds[speed & CBAUD].baud_support);
}

/* 
 * Time up for process to set a break on a transmission line.
 */
void mdcsetbreak(tp)
register struct tty *tp;
{
	wakeup((caddr_t)&tp->t_dev);
}

