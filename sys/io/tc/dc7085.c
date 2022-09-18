#ifndef lint
static char *sccsid = "@(#)dc7085.c	4.6      (ULTRIX)  11/9/90";
#endif	lint

/************************************************************************
 *									*
 *			Copyright (c) 1988,89 by			*
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
 * dc7085.c
 *
 * DC7085 SLU console driver
 *
 * Modification history
 *
 *   13-Sep-90 Joe Szczypek
 *	Added new TURBOchannel console ROM support.  osconsole environment
 *	variable now returns 1 slot number is serial line, else 2 slot numbers
 *	if graphics.  Use this to determine how to do setup.  Note that new
 *	ROMs do not support multiple outputs...
 *
 *   4-Aug-90 - Randall Brown
 *	Made modifications to allow the driver to be used on multiple controllers.
 *	Implemented the 'slu' interface for graphics drivers to callback in
 *	to the console driver to access mouse and keyboard.
 *
 *  6-Jul-1990	- Kuo-Hsiung Hsieh
 * 	Fixed data corrupted problem due to setting break condition
 *	on a transmission line.  On DC type of chip, a specific delay
 *	period has to be imposed on the transmission line if the next
 *	thing to transmit is a break condition.  Data could be corrupted
 *	even though TRDY bit may say it is ready to send the next character.
 *
 *  25-Apr-1990 - Kuo-Hsiung Hsieh
 *      Disable line transmission before cleaning up break bit.
 *      Prevented null character being generated following a break
 *      condition.
 *
 *  23-Mar-1990 - Kuo-Hsiung Hsieh
 *	Corrected the improper sequence of closing down the line.
 *	This correction will prevent tty inmaturely turning off
 *	flow control (fails to restart line drive) while there are
 *	still more data in the buffer. ttyclose() should be called 
 *	before clearing up any terminal attributes.  
 *
 *  8-Dec-1989 - Randall Brown
 *  	Added full modem support.  Used variables to describe register
 *	conditions instead of constants because pmax and 3max have the
 *	bits in different places in certain registers.
 *
 *  6-Dec-89 - Randall Brown
 *
 *	Added the support to allow the device to determine if baudrate is 
 *	supported before it is set in the tty data structures.
 *
 *  9-Nov-1989 - Randall Brown
 *	In dc_putc(), take the line number as an argument, don't figure it
 *	out from consDev.
 *
 * 29-Oct-1989 - Randall Brown
 *	Added support for cons_init.  The code from the probe routine is
 * 	now in dc_cons_init.  Added wbflush() to dc_putc to let data get
 *	to chip to clear interrupt.
 *
 * 16-Oct-1989 - Randall Brown
 *	Added autoconfiguration support.
 *
 * 11-Jul-1989 - Randall Brown
 *	Changed all sc->dc_tty to dc_tty since the dc_tty[] structure is not
 *	part of the softc structure anymore.
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
 * 12-Jun-1989 - dws
 *	Added trusted path support.
 *
 * 28-Apr-1989 - Randall Brown
 *	Changed a while loop in cn_putc to a DELAY(5).  This was to fix a 
 *	problem that the system would hang if there was output to the
 *	console and another serial line was active.
 *
 * 24-Feb-1989 - Randall Brown
 *	Changed close routine to look at HUPCL in cflag, instead of HUPCLS in
 *	state flag.
 *
 * 28-Dec-1988 - Randall Brown
 *	Changed when the break bits were being set in the chip.  Previously
 * 	they were being set in the ioctl, but this could cause data 
 *	corruption because the state of the chip is unknown.  The bits are
 *	now set in the transmitter interrupt routine, where the state of the
 * 	chip is known.
 *
 * 22-Dec-1988 - Randall Brown
 *	Changed the open routine to return ENXIO on the open of /dev/xcons
 *	when the graphic device is not being used.
 *
 * 16-Dec-1988 - Randall Brown
 *	Added Pseudo DMA code to the transmit side of the driver.  The start
 *	routine sets up the pointers in the pdma struct for the number
 * 	of continuous chars in the outq.  When a transmitter interrupt is
 *	serviced, the pdma struct is checked to see if there are any more
 *	chars to be output, if not it call dcxint(), to fill in the
 * 	pointers again. 
 *
 * 17-Nov-1988 - Randall Brown
 *	Added modem support.  The driver only looks at DSR, and ignores
 * 	CD and CTS.  Also cleaned up driver so that names are consistent.
 *	Also fixed some problems with byte reads and writes (ie. removed
 *	all byte read and writes, and made them half-word reads and writes)
 *
 *  7-Jul-1988 - rsp (Ricky Palmer)
 *	Created file. Contents based on ss.c file.
 *
 */

#include "../data/dc_data.c"

int	dcprobe(), dcattach(), dcrint();
int	dc_dsr_check(), dc_tty_drop(), dc_cd_drop(); /* Modem */
u_short dcstd[] = { 0 };

struct	uba_driver dcdriver = { dcprobe, 0, dcattach, 0, dcstd, "dc", dcinfo };

#define GRAPHIC_DEV 0x2 /* pick up from pm header file later */

#define LINEMASK	0x03		/* line unit mask */

/*
* The SLU doesn't interrupt on carrier transitions, so
* we have to use a timer to watch it.
*/
char	dc_timer = 0;		/* timer started? */
int	dc_base_board = 0;	/* defines whether there is a dc controller */
                                /* on the base board that is used for the graphics */
                                /* devices */

/*
 * Baud Rate Support
 *
 * When the baud rate on the right is specified, the line parameter register
 * is setup with the appropriate bits as specified in the left column.
 */
#define BAUD_UNSUPPORTED 0	/* Device does not provide this baud rate */
#define BAUD_SUPPORTED   1	/* Device does provide this baud rate     */

/*
 * PMAX does not support 19.2K, but on 3MAX,
 * if the BAUD38 bit of the System Control and Status Register is set the
 * chip can do 38400 baud in which case EXTB would be supported.  This bit
 * applies to all 4 lines such that it is not possible to simultaneously do
 * 19200 and 38400.  To keep things simple, only provide support for 19.2.
 *
 * The option card supports 19.2K.  The setting of this being supported is
 * taken care of in the attach routine.
 */
struct baud_support dc_speeds[] = {
     	{0,			BAUD_UNSUPPORTED},		/* B0    */
	{DC_B50,		BAUD_SUPPORTED},		/* B50   */
	{DC_B75,		BAUD_SUPPORTED},		/* B75   */
	{DC_B110,		BAUD_SUPPORTED},		/* B110  */
	{DC_B134_5,		BAUD_SUPPORTED},		/* B134  */
	{DC_B150,		BAUD_SUPPORTED},		/* B150  */
	{0,			BAUD_UNSUPPORTED},		/* B200  */
	{DC_B300,		BAUD_SUPPORTED},		/* B300  */
	{DC_B600,		BAUD_SUPPORTED},		/* B600  */
	{DC_B1200,		BAUD_SUPPORTED},		/* B1200 */
	{DC_B1800,		BAUD_SUPPORTED},		/* B1800 */
	{DC_B2400,		BAUD_SUPPORTED},		/* B2400 */
	{DC_B4800,		BAUD_SUPPORTED},		/* B4800 */
	{DC_B9600,		BAUD_SUPPORTED},		/* B9600 */
	{DC_B19200,		BAUD_UNSUPPORTED},		/* EXTA  */
	{0,			BAUD_UNSUPPORTED}, 		/* EXTB  */
   };

extern int 	consDev;

#define DS3100_DC_BASE	(PHYS_TO_K1(0x1c000000))
#define DS5000_DC_BASE	(PHYS_TO_K1(0x1fe00000))

#define DC_OPTION_OFFSET	0x80000		/* register offset from beginning of slot */

int	dcstart(), dcxint(), dcbaudrate();
int	ttrstrt();
void	dcsetbreak();

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
extern int (*v_consgetc)();
extern int (*v_consputc)();
extern int pmcons_init();
extern int prom_getenv();
extern int rex_getenv();  /* New TURBOchannel interface */
extern int cpu;

/* minumum delay value for setting a break condition.  If we set
 * a break condition without delaying this minimum interval, we
 * might corrupt character which is still in the shift register.
 * The delay values are calculated based on the following equation;
 * 12 (bits/char) * 256 (hz) / baudrate + 2 (safety factor).
 */
u_char    dc_delay[] =
{ 0,78,42,30,25,22,0,12,7,5,4,3,2,2,2,0 };

#ifdef DEBUG
int dcdebug = 0;

#define PRINT_SIGNALS() { cprintf("Modem signals: "); \
	if (sc->dcmsr&dc_rdsr[2]) cprintf(" DSR2 "); \
	if (sc->dcmsr&dc_rcts[2]) cprintf(" CTS2 "); \
	if (sc->dcmsr&dc_rcd[2]) cprintf(" CD2 "); \
	if (sc->dcmsr&dc_rdsr[3]) cprintf(" DSR3 "); \
	if (sc->dcmsr&dc_rcts[3]) cprintf(" CTS3 "); \
	if (sc->dcmsr&dc_rcd[3]) cprintf(" CD3 "); \
	cprintf("\n"); } \
/*	cprintf("sc->dcmsr %x : %x\n", &(sc->dcmsr), sc->dcmsr);*/
#endif DEBUG


dcprobe(reg)
int reg;
{
        /* the intialization is done through dc_cons_init, so
	 * if we have gotten this far we are alive so return a 1
	 */
	return(1);
}

dcattach(ui)
	struct uba_device *ui;
{
        register int ctlr = ui->ui_unit;
	register struct dc_softc *sc = &dc_softc[ctlr];
	register int i;
	extern dcscan();

	dcsoftCAR[ctlr] = 0xff;
	dcdefaultCAR[ctlr] = 0xff;
	dc_brk[ctlr] = 0;
	for (i = 0; i < NDCLINE; i++) {
		brk_start[(ctlr * NDCLINE) +i] = brk_stop[(ctlr * NDCLINE) +i] = 0;
	}

	switch (cpu) {
	  case DS_3100:
	    dc_base_board = 1;
	    dc_modem_ctl = 0;	/* PMAX has limited modem control */
	    /* line 2 definitions */
	    dc_rdtr[2] = 0x0400;
	    dc_rdsr[2] = 0x0200;
	    dc_xmit[2] = dc_rdsr[2];
	    dc_rrts[2] = dc_rcts[2] = dc_rcd[2] = 0;
	    dc_modem_line[2] = 1;
	    /* line 3 definitions */
	    dc_rdtr[3] = 0x0800;
	    dc_rdsr[3] = 0x0001;
	    dc_xmit[3] = dc_rdsr[3];
	    dc_rrts[3] = dc_rcts[3] = dc_rcd[3] = 0;
	    dc_modem_line[3] = 1;
	    break;

	  case DS_5000:
	    if (ctlr == 0) {
		dc_base_board = 1;
		dc_modem_ctl = 1;	/* 3max has full modem control */
		/* line 2 definitions */
		dc_rdtr[2] = 0x0400;
		dc_rrts[2] = 0x0800;
		dc_rcd[2] = 0x0400;
		dc_rdsr[2] = 0x0200;
		dc_rcts[2] = 0x0100;
		dc_xmit[2] = dc_rcd[2] | dc_rdsr[2] | dc_rcts[2];
		dc_modem_line[2] = 1;
		/* line 3 definitions */
		dc_rdtr[3] = 0x0100;
		dc_rrts[3] = 0x0200;
		dc_rcd[3] = 0x0004;
		dc_rdsr[3] = 0x0002;
		dc_rcts[3] = 0x0001;
		dc_xmit[3] = dc_rcd[3] | dc_rdsr[3] | dc_rcts[3];
		dc_modem_line[3] = 1;
	    } else {
		sc->sc_regs = (struct dc_reg *)(ui->ui_addr + DC_OPTION_OFFSET);
	    }
	    dc_speeds[B19200].baud_support = BAUD_SUPPORTED;
	    break;

	  case DS_5000_100:
	    sc->sc_regs = (struct dc_reg *)(ui->ui_addr + DC_OPTION_OFFSET);
	    dc_speeds[B19200].baud_support = BAUD_SUPPORTED;
	    break;
	    
	  default:
	    printf("Unknown cpu type in dcattach()\n");
	    break;
	}

	if (dc_modem_line[2]) {		/* this is a base board controller, start modem ctl */
	    /* clear modem control signals */
	    sc->dcdtr &= ~(dc_rdtr[2] | dc_rrts[2] | dc_rdtr[3] | dc_rrts[3]);
	    
	    /* Start the modem scan timer */
	    if (!dc_timer) {
		timeout(dcscan, (caddr_t)0, hz);
		dc_timer = 1;
	    }
	}
}

dc_cons_init()
{
       int i, temp_reg;
       int tmp1;                         /* ROM debug only */
       register struct dc_softc *sc;

       extern int console_magic;
       extern int (*vcons_init[])();
       int dc_mouse_init(), dc_mouse_putc(), dc_mouse_getc();
       int dc_kbd_init(), dc_kbd_putc(), dc_kbd_getc(), dc_putc();

       sc = &dc_softc[0];
       switch (cpu) {
	 case DS_3100:	sc->sc_regs = (struct dc_reg *)DS3100_DC_BASE; break;
	 case DS_5000:	sc->sc_regs = (struct dc_reg *)DS5000_DC_BASE; break;
	 default:	printf("Unknown cpu type in dc_cons_init()\n"); break;
       }

       /*
	*
	* Query the prom. The prom can be set such that the user 
	* could use either the alternate tty or the graphics console.
	* You get the graphics console if the first bit is set in
	* osconsole.  The user sets the console variable
	*/
       if (console_magic != 0x30464354) {
	 if ((atoi(prom_getenv("osconsole")) & 0x1) == 1) {
	   slu.mouse_init = dc_mouse_init;
	   slu.mouse_putc = dc_mouse_putc;
	   slu.mouse_getc = dc_mouse_getc;
	   slu.kbd_init = dc_kbd_init;
	   slu.kbd_putc = dc_kbd_putc;
	   slu.kbd_getc = dc_kbd_getc;
	   slu.slu_tty = dc_tty;
	   slu.slu_putc = dc_putc;
	   for( i = 0 ; vcons_init[i] ; i++ )
	       if ((*vcons_init[i])()) {	/* found a virtual console */
		   consDev = GRAPHIC_DEV;
		   break;
	       }
	 } else {
	   sc->dclpr = (DC_RE | DC_B9600 | BITS8 | 3);  /* set up line 3, console line */
	   sc->dctcr = (DC_TCR_EN_3);  /* line 3 transmit enable */
	   sc->dccsr = (DC_MSE);	/* master scan enable */
	 }
       } else {
	 if ((strlen(rex_getenv("osconsole"))) > 1) {
	   slu.mouse_init = dc_mouse_init;
	   slu.mouse_putc = dc_mouse_putc;
	   slu.mouse_getc = dc_mouse_getc;
	   slu.kbd_init = dc_kbd_init;
	   slu.kbd_putc = dc_kbd_putc;
	   slu.kbd_getc = dc_kbd_getc;
	   slu.slu_tty = dc_tty;
	   slu.slu_putc = dc_putc;
	   for( i = 0 ; vcons_init[i] ; i++ )
	       if ((*vcons_init[i])()) {	/* found a virtual console */
		   consDev = GRAPHIC_DEV;
		   break;
	       }
	 } else {
	   sc->dclpr = (DC_RE | DC_B9600 | BITS8 | 3);  /* set up line 3, console line */
	   sc->dctcr = (DC_TCR_EN_3);  /* line 3 transmit enable */
	   sc->dccsr = (DC_MSE);	/* master scan enable */
	 }
       }

}

dcopen(dev, flag)
	dev_t dev;
{
	register struct dc_softc *sc;
	register struct tty *tp;
	register int ctlr, unit;
	register int maj, error;
	int inuse;  /*hold state of inuse bit while blocked waiting for carr*/

	maj = major(dev);
	unit = minor(dev);
	ctlr = unit >> 2; 

/* rpbfix: need to check to see if the board is actually in the system */
	sc = &dc_softc[ctlr];

	/*
	 * If a diagnostic console is attached to SLU line 3,
	 * don't allow open of the printer port (also line 3).
	 * This could cause lpr to write to the console.
	 *
	 * This is only true of the base board option.
	 */
	if (dc_base_board && (ctlr == 0))
	    if((consDev != GRAPHIC_DEV) && (unit == 3))
		    return (ENXIO);

	/* don't allow open of minor device 0 of major device DCMAJOR */
	/* if this is a base board option, because it is already      */
	/* reserved for /dev/console */
	if (dc_base_board && (maj != CONSOLEMAJOR) && (unit == 0))
	    return (ENXIO);

	/* only allow open of /dev/console of major device 0 */
	if ((maj == CONSOLEMAJOR) && (unit != 0))
	    return (ENXIO);

	if ((consDev != GRAPHIC_DEV) && (maj == CONSOLEMAJOR) && (unit == 0))
		unit |= 3;	/* diag console on SLU line 3 */

	if (unit >= dc_cnt)
		return (ENXIO);
	/*
	 * Call the graphics device open routine
	 * if there is one and the open if for the fancy tube.
	 */
	if (dc_base_board && (ctlr == 0))
	    if (vs_gdopen && (unit <= 1)) {
		error = (*vs_gdopen)(dev, flag);
		if (error == 0)
		    dcparam(unit); 	/* turn on interrupts for kbd and mouse */
		return(error);
	    }

	tp = &dc_tty[unit];
	if (tp->t_state&TS_XCLUDE && u.u_uid != 0) {
		return (EBUSY);
	}

	while (tp->t_state&TS_CLOSING) { /* let DTR stay down for awhile */
		sleep((caddr_t)&tp->t_rawq, TTIPRI);
	}
	tp->t_addr = (caddr_t)tp;
	tp->t_oproc = dcstart;
	tp->t_baudrate = dcbaudrate;

	tty_def_open(tp, dev, flag, (dcsoftCAR[ctlr]&(1<<(unit&LINEMASK))));

	if ((tp->t_state & TS_ISOPEN) == 0) {
	    /*
	     * Prevent spurious startups by making the 500ms timer
	     * initially high.
	     */
	    dcmodem[unit] = MODEM_DSR_START;
	    if((maj == CONSOLEMAJOR) && ((minor(dev)&3) == 0)) {
		tp->t_cflag &= ~CBAUD;
		tp->t_cflag |= B9600;
		/* modem control not supported on console */ 
		tp->t_cflag |= CLOCAL; 
		tp->t_cflag_ext &= ~CBAUD;
		tp->t_cflag_ext |= B9600;
		tp->t_flags = ANYP|ECHO|CRMOD;
		tp->t_iflag |= ICRNL; /* Map CRMOD */
		tp->t_oflag |= ONLCR; /* Map CRMOD */
	    }
	}
	dcparam(unit);		/* enables interrupts */
	(void) spltty();

	/*
	 * No modem control provided for lines with softCAR set.
	 * Modem control provided only for lines 2 and 3 of the base board.
	 */
	if (dc_modem_line[unit] == 0) 
	    tp->t_cflag |= CLOCAL;

#	ifdef DEBUG
	if (dcdebug)
		cprintf("dcopen: UNIT = %x\n",unit);
#	endif DEBUG
	if (tp->t_cflag & CLOCAL) {
		/*
		 * This is a local connection - ignore carrier
		 * receive enable interrupts enabled above via dcparam()
		 */
		tp->t_state |= TS_CARR_ON;		/* dcscan sets */
		sc->dcdtr |= (dc_rdtr[unit] | dc_rrts[unit]);
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

	sc->dcdtr |= (dc_rdtr[unit] | dc_rrts[unit]);

	/*
	 * After DSR first comes up we must wait for the other signals
	 * before commencing transmission.
         */
#ifdef DEBUG
	if (dcdebug) {
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
		if (dcdebug) {
			cprintf("dcopen: ");
			PRINT_SIGNALS();
		}
#endif DEBUG
		if ((sc->dcmsr)&dc_rdsr[unit]) {
			dcmodem[unit] |= (MODEM_DSR_START|MODEM_DSR);
			tp->t_dev = dev; /* need it for timeouts */
			if (!dc_modem_ctl) {
			    /*
			     * Assume carrier will come up in less
			     * than 1 sec. If not DSR will drop
			     * and the line will close
			     */
			    timeout(dc_dsr_check, tp, hz);
			} else {
			    /*
			     * Give CD and CTS 30 sec. to 
			     * come up.  Start transmission
			     * immediately, no longer need
			     * 500ms delay.
			     */
			    timeout(dc_dsr_check, tp, hz*30);
			    dc_dsr_check(tp);
			}
		}
	}
#	ifdef DEBUG
	if (dcdebug)
		cprintf("dcopen:  line=%d, state=%x, tp=%x\n", unit,
			tp->t_state, tp);
#	endif DEBUG
	if (flag & (O_NDELAY|O_NONBLOCK))
		tp->t_state |= TS_ONDELAY;
	else
		while ((tp->t_state & TS_CARR_ON) == 0) {
			tp->t_state |= TS_WOPEN;
			inuse = tp->t_state&TS_INUSE;
#ifdef DEBUG
			if (dcdebug) {
				cprintf("dc_open: going to sleep\n");
			}
#endif DEBUG
			sleep((caddr_t)&tp->t_rawq, TTIPRI);
			/*
			 * See if we were awoken by a false call to the modem
			 * line by a non-modem.
			 */
			if (dcmodem[unit]&MODEM_BADCALL){
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

dcclose(dev, flag)
	dev_t dev;
{
	register struct dc_softc *sc;
	register struct tty *tp;
	register int unit, ctlr, maj;
	register int s;
	extern int wakeup();

	unit = minor(dev);
	ctlr = unit >> 2;
	maj = major(dev);
	sc = &dc_softc[ctlr];

	if((consDev != GRAPHIC_DEV) && (maj == CONSOLEMAJOR) && (unit == 0))
		unit |= 3;	/* diag console on SLU line 3 */
	/*
	 * Call the craphics device close routine
	 * if ther is one and the close is for it.
	 */
	if (dc_base_board && (ctlr == 0))
	    if (vs_gdclose && (unit <= 1)) {
		(*vs_gdclose)(dev, flag);
		return;
	    }
#	ifdef DEBUG
	if (dcdebug)
		cprintf("dcclose: unit=%x\n",unit);
#	endif DEBUG
	tp = &dc_tty[unit];
	/*
	 * Do line discipline specific close functions then return here
	 * in the old line disc for final closing.
	 */
	if (tp->t_line)
		(*linesw[tp->t_line].l_close)(tp);
	/*
	 * dcbrk is write-only and sends a BREAK (SPACE condition) until
	 * the break control bit is cleared. Here we are clearing any
	 * breaks for this line on close.
	 */
	s = spltty();
	brk_stop[unit] = 1;
	sc->dccsr |= DC_TIE;
	sc->dctcr |= (1 << (unit & LINEMASK));
	splx(s);
	if ((tp->t_cflag&HUPCL) || (tp->t_state&TS_WOPEN) || (tp->t_state&TS_ISOPEN)==0) {
		tp->t_state &= ~TS_CARR_ON;   /* prevents recv intr. timeouts */
		/*
		 * Drop appropriate signals to terminate the connection.
		 */
		dcmodem_active[ctlr] &= ~(1 << (unit & LINEMASK));
		sc->dcdtr &= ~(dc_rdtr[unit] | dc_rrts[unit]);
		if ((tp->t_cflag & CLOCAL) == 0) {
		    s = spltty();
		    /*drop DTR for at least a sec. if modem line*/
#				ifdef DEBUG
		    if (dcdebug)
			cprintf("dcclose: DTR drop, state =%x\n"
				,tp->t_state);
#				endif DEBUG
		    tp->t_state |= TS_CLOSING;
		    /*
		     * Wait at most 5 sec for DSR to go off.
		     * Also hold DTR down for a period.
		     */
		    if (sc->dcmsr & dc_rdsr[unit]) {
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
		/*
		 * No disabling of interrupts is done.	Characters read in on
		 * a non-open line will be discarded.
		 */
	}
	/* reset line to default mode */
	dcsoftCAR[ctlr] &= ~(1<<(unit&LINEMASK));
	dcsoftCAR[ctlr] |= (1<<(unit&LINEMASK)) & dcdefaultCAR[ctlr];
	dcmodem[unit] = 0;
	/* ttyclose() must be called before clear up termio flags */
	ttyclose(tp);
	tty_def_close(tp);
}

dcread(dev, uio)
	dev_t dev;
	struct uio *uio;
{
	register struct tty *tp;
	register int unit, ctlr;

	unit = minor(dev);
	ctlr = unit >> 2;

	if((consDev != GRAPHIC_DEV) && (major(dev) == CONSOLEMAJOR) && (unit == 0))
		unit |= 3;	/* diag console on SLU line 3 */
	tp = &dc_tty[unit];
	return ((*linesw[tp->t_line].l_read)(tp, uio));
}

dcwrite(dev, uio)
	dev_t dev;
	struct uio *uio;
{
	register struct tty *tp;
	register int unit, ctlr;

	unit = minor(dev);
	ctlr = unit >> 2;

	if((consDev != GRAPHIC_DEV) && (major(dev) == CONSOLEMAJOR) && (unit == 0))
		unit |= 3;	/* diag console on SLU line 3 */
	if (dc_base_board && (ctlr == 0)) {
	    /*
	     * Don't allow writes to the mouse,
	     * just fake the I/O and return.
	     */
	    if (vs_gdopen && (unit == 1)) {
		uio->uio_offset = uio->uio_resid;
		uio->uio_resid = 0;
		return(0);
	    }
	}

	tp = &dc_tty[unit];
	return ((*linesw[tp->t_line].l_write)(tp, uio));
}

dcselect(dev, rw)
dev_t dev;
{
	register int ctlr, unit;

	unit = minor(dev);
	ctlr = unit >> 2;

/*printf("in dcselect, dev = %x, ctlr = %d, unit = %d\n", dev, ctlr, unit);*/
	if((consDev != GRAPHIC_DEV) && (major(dev) == CONSOLEMAJOR) && (unit == 0))
		dev |= 3;
	if (dc_base_board && (ctlr == 0))
	    if ((unit == 1) && vs_gdselect) {
/*printf("calling vs_gdselect = %x\n", vs_gdselect);	       */
		return((*vs_gdselect)(dev, rw));
}
	return(ttselect(dev, rw));
}

dcintr(ctlr)
int ctlr;
{
	register struct dc_softc *sc;
	register unsigned int csr;

	sc = &dc_softc[ctlr];

	csr = sc->dccsr;
	if (csr & DC_TRDY)
		_dcpdma(ctlr);
	if (csr & DC_RDONE) 
		dcrint(ctlr);
}

/*
 * Used to pass mouse (or tablet) reports to the graphics
 * device driver interrupt service routine.
 * Entire report passed instead of byte at a time.
 */
struct	mouse_report	current_rep;
u_short pointer_id;
#define MOUSE_ID	0x2

dcrint(ctlr)
int ctlr;
{
	register struct dc_softc *sc;
	register struct tty *tp;
	register u_short c;	/* rbuf register is 16 bits long */
	register int unit, linenum;
	register int flg;
	int overrun = 0;
	struct mouse_report *new_rep;
	u_short data;
	int counter = 0;

	sc = &dc_softc[ctlr];
	new_rep = &current_rep;			/* mouse report pointer */
	while (sc->dccsr&DC_RDONE) {		/* character present */
		c = sc->dcrbuf;

		linenum = ((c >> 8) & LINEMASK);
		unit = (ctlr * NDCLINE) + linenum;
		tp = &dc_tty[unit];

		if (tp >= &dc_tty[dc_cnt])
			continue;
		/*
		 * If console is a graphics device,
		 * pass keyboard input characters to
		 * its device driver's receive interrupt routine.
		 * Save up complete mouse report and pass it.
		 */
		if ((dc_base_board) && (ctlr == 0) && (unit <= 1) && vs_gdkint) {
		    if(unit == 0) {		/* keyboard char */
			(*vs_gdkint)(c);
			continue;
		    } else {			/* mouse or tablet report */
			if (pointer_id == MOUSE_ID) { /* mouse report */
			    data = c & 0xff;	/* get report byte */
			    ++new_rep->bytcnt;	/* inc report byte count */

			    if (data & START_FRAME) { /* 1st byte of report? */
				new_rep->state = data;
				if (new_rep->bytcnt > 1)
				    new_rep->bytcnt = 1;  /* start new frame */
			    }

			    else if (new_rep->bytcnt == 2) {	/* 2nd byte */
				new_rep->dx = data;
			    }

			    else if (new_rep->bytcnt == 3) {	/* 3rd byte */
				    new_rep->dy = data;
				    new_rep->bytcnt = 0;
				    (*vs_gdkint)(0400); /* 400 says line 1 */
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

			    else if (new_rep->bytcnt == 3)	/* 3rd byte */
				    new_rep->dx |= (data & 0x3f) << 6;

			    else if (new_rep->bytcnt == 4)	/* 4th byte */
				    new_rep->dy = data & 0x3f;

			    else if (new_rep->bytcnt == 5){	/* 5th byte */
				    new_rep->dy |= (data & 0x3f) << 6;
				    new_rep->bytcnt = 0;
				    (*vs_gdkint)(0400); /* 400 says line 1 */
			    }
			    continue;
			}
		    }
		}
		if ((tp->t_state & TS_ISOPEN) == 0) {
			wakeup((caddr_t)&tp->t_rawq);
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


		/* DC_FE is interpreted as a break */
		if (c & DC_FE) {
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
#				ifdef DCDEBUG
				if (dcdebug)
					mprintf("dcrint: BREAK RECEIVED\n");
#				endif DCDEBUG
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
#			ifdef DCDEBUG
			if (dcdebug > 1)
				mprintf("dcrint: Parity Error\n");
#			endif DCDEBUG
			if ((flg & INPCK) == 0)
				c &= ~DC_PE;
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
		if (c&DC_DO) {
			if(overrun == 0) {
				printf("dc%d: input silo overflow\n", ctlr);
				overrun = 1;
			}
			sc->sc_softcnt[linenum]++;
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
dcioctl(dev, cmd, data, flag)
	dev_t dev;
	register int cmd;
	caddr_t data;
	int flag;
{
	register struct dc_softc *sc;
	register int unit, ctlr;
	register struct tty *tp;
	register int s;
	struct uba_device *ui;
	struct devget *devget;
	int error;

	unit = minor(dev);
	ctlr = unit >> 2;
	sc = &dc_softc[ctlr];

	if((consDev != GRAPHIC_DEV) && (major(dev) == CONSOLEMAJOR) && (unit == 0))
		unit |= 3;	/* diag console on SLU line 3 */

	/*
	 * If there is a graphics device and the ioctl call
	 * is for it, pass the call to the graphics driver.
	 */
	if (dc_base_board && (ctlr == 0))
	    if (vs_gdioctl && (unit <= 1)) {
		return((*vs_gdioctl)(dev, cmd, data, flag));
	    }
	tp = &dc_tty[unit];
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
				dcparam(unit);
				break;
		}
		return (error);
	}
	switch (cmd) {

	case TIOCSBRK:
		timeout(dcsetbreak, tp, dc_delay[tp->t_cflag & CBAUD]);
		TTY_SLEEP(tp, (caddr_t)&tp->t_dev, TTOPRI);
		s = spltty();
		brk_start[unit] = 1;
		sc->dccsr |= DC_TIE;
		sc->dctcr |= (1 << unit);
		splx(s);
		break;

	case TIOCCBRK:
		s = spltty();
		brk_stop[unit] = 1;
		sc->dccsr |= DC_TIE;
		sc->dctcr |= (1 << unit);
		splx(s);
		break;

	case TIOCSDTR:
		(void) dcmctl(dev, DC_DTR | DC_RTS, DMBIS);
		break;

	case TIOCCDTR:
		(void) dcmctl(dev, DC_DTR | DC_RTS, DMBIC);
		break;

	case TIOCMSET:
		(void) dcmctl(dev, dmtodc(*(int *)data), DMSET);
		break;

	case TIOCMBIS:
		(void) dcmctl(dev, dmtodc(*(int *)data), DMBIS);
		break;

	case TIOCMBIC:
		(void) dcmctl(dev, dmtodc(*(int *)data), DMBIC);
		break;

	case TIOCMGET:
		*(int *)data = dctodm(dcmctl(dev, 0, DMGET));
		break;

	case TIOCNMODEM:  /* ignore modem status */
		/*
		 * By setting the software representation of modem signals
		 * to "on" we fake the system into thinking that this is an
		 * established modem connection.
		 */
		s = spltty();
		dcsoftCAR[ctlr] |= (1<<(unit&LINEMASK));
		if (*(int *)data) /* make mode permanent */
			dcdefaultCAR[ctlr] |= (1<<(unit&LINEMASK));
		tp->t_state |= TS_CARR_ON;
		tp->t_cflag |= CLOCAL;		/* Map to termio */
		splx(s);
		break;

	case TIOCMODEM:  /* look at modem status - sleep if no carrier */
		s = spltty();
		dcsoftCAR[ctlr] &= ~(1<<(unit&LINEMASK));
		if (*(int *)data) /* make mode permanent */
			dcdefaultCAR[ctlr] &= ~(1<<(unit&LINEMASK));
		/*
		 * See if signals necessary for modem connection are present
		 *
		 * dc7085 chip on PMAX only provides DSR
		 *
		 */
		if ((sc->dcmsr & dc_xmit[unit]) == dc_xmit[unit]) {
			tp->t_state &= ~(TS_ONDELAY);
			tp->t_state |= TS_CARR_ON;
			dcmodem[unit] = MODEM_CTS|MODEM_CD|MODEM_DSR;
		} else {
			tp->t_state &= ~(TS_CARR_ON);
			dcmodem[unit] &= ~(MODEM_CTS|MODEM_CD|MODEM_DSR);
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
		if ((sc->dcmsr & dc_xmit[unit]) == dc_xmit[unit]) {
			tp->t_state |= TS_CARR_ON;
			tp->t_state &= ~(TS_ONDELAY);
			dcmodem[unit] = MODEM_CTS|MODEM_CD|MODEM_DSR;
		} else {
		    while ((tp->t_state & TS_CARR_ON) == 0)
			sleep((caddr_t)&tp->t_rawq, TTIPRI);
		}
		splx(s);
		break;

	case DEVIOCGET:				/* device status */
		devget = (struct devget *)data;
		bzero(devget,sizeof(struct devget));
		if (dc_modem_line[unit])
		    if (tp->t_cflag & CLOCAL) {
			sc->sc_category_flags[unit&LINEMASK] |= DEV_MODEM;
			sc->sc_category_flags[unit&LINEMASK] &= ~DEV_MODEM_ON;
		    } else
			sc->sc_category_flags[unit&LINEMASK] |= (DEV_MODEM|DEV_MODEM_ON);
		devget->category = DEV_TERMINAL;	/* terminal cat.*/
		devget->bus = DEV_NB;			/* NO bus	*/
		bcopy(DEV_VS_SLU,devget->interface,
		      strlen(DEV_VS_SLU));		/* interface	*/
		bcopy(DEV_UNKNOWN,devget->device,
		      strlen(DEV_UNKNOWN));		/* terminal	*/
		devget->adpt_num = 0;			/* NO adapter	*/
		devget->nexus_num = 0;			/* fake nexus 0 */
		devget->bus_num = 0;			/* NO bus	*/
		devget->ctlr_num = ctlr;		/* cntlr number */
		devget->slave_num = unit&LINEMASK;	/* line number	*/
		bcopy("dc", devget->dev_name, 3);	/* Ultrix "dc"	*/
		devget->unit_num = unit&LINEMASK;	/* dc line?	*/
		devget->soft_count =
		      sc->sc_softcnt[unit&LINEMASK];	/* soft err cnt */
		devget->hard_count =
		      sc->sc_hardcnt[unit&LINEMASK];	/* hard err cnt */
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

dmtodc(bits)
	register int bits;
{
	register int b;

	b = (bits >>1) & 0370;
	if (bits & SML_ST) b |= DC_ST;
	if (bits & SML_RTS) b |= DC_RTS;
	if (bits & SML_DTR) b |= DC_DTR;
	if (bits & SML_LE) b |= DC_LE;
	return(b);
}

dctodm(bits)
	register int bits;
{
	register int b;

	b = (bits << 1) & 0360;
	if (bits & DC_DSR) b |= SML_DSR;
	if (bits & DC_DTR) b |= SML_DTR;
	if (bits & DC_ST) b |= SML_ST;
	if (bits & DC_RTS) b |= SML_RTS;
	return(b);
}

dcparam(unit)
	register int unit;
{
	register struct dc_softc *sc;
	register struct tty *tp;
	register u_short lpr;
	register int ctlr, s;

	ctlr = unit >> 2;
	sc = &dc_softc[ctlr];

	tp = &dc_tty[unit];

	s = spltty();

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
 *	Should dcparam() be called as spl5??????
 */
	if ((((tp->t_cflag&CBAUD)==B0) && (u.u_procp->p_progenv != A_POSIX)) ||
	    (((tp->t_cflag_ext & CBAUD)==B0) &&
	     (u.u_procp->p_progenv == A_POSIX))) {
	    dcmodem_active[ctlr] &= ~(1 << (unit & LINEMASK));
	    sc->dcdtr &= ~(dc_rdtr[unit] | dc_rrts[unit]); /* hang up line */
	    splx(s);
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
	if (dc_base_board && (ctlr == 0)) {
	    if ((unit == 3) && (consDev != GRAPHIC_DEV)) {
		sc->dclpr = (DC_RE | DC_B9600 | BITS8 | 3);
		sc->dccsr = DC_MSE | DC_TIE | DC_RIE;
		splx(s);
		return;
	    }
	    if (unit <= 1) {
		sc->dccsr = DC_MSE |DC_TIE | DC_RIE;
		splx(s);
		return;
	    }
	}

	lpr = (DC_RE) | (dc_speeds[tp->t_cflag&CBAUD].baud_param) | (unit & LINEMASK);
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
	    lpr &= ~DC_RE;	/* This was set from speeds */
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
	sc->dclpr = lpr;
	
	sc->dccsr = DC_MSE | DC_TIE | DC_RIE;
	splx(s);
}

_dcpdma(ctlr)
int ctlr;
{
    	register struct dc_softc *sc;
	register struct dcpdma *dp;
	register int unit, linenum;

	sc = &dc_softc[ctlr];

	linenum = (sc->dccsr >> 8) & LINEMASK;

	unit = (ctlr * NDCLINE) + linenum;
	dp = &sc->dc_pdma[linenum];

	if (brk_start[unit]) {
		brk_start[unit] = 0;
		/* only set break bit if it is not already set */
		if ((dc_brk[ctlr] & (1 << (linenum + 0x8))) == 0) {
		    switch (cpu) {
		      case DS_3100:
			sc->dcbrk_tbuf = (dc_brk[ctlr] |= (1 << (linenum + 0x8)));
			break;

		      case DS_5000:	/* dcbrk is a char on 3MAX and option card */
		      case DS_5000_100:
			sc->dcbrk = ((dc_brk[ctlr] |= (1 << (linenum + 0x8))) >> 8);
			dcxint(unit);	/* turn off intr if not busy */
			break;
		    }
		}
	} else if (brk_stop[unit]) {
		brk_stop[unit] = 0;
		/* only clear break bit if it is already set */
		if (dc_brk[ctlr] & (1 << (linenum + 0x8))) {
		    switch (cpu) {
		      case DS_3100:
                        /* Disable line transmission first */
                        sc->dctcr &= ~(1 << linenum);
			sc->dcbrk_tbuf = (dc_brk[ctlr] &= ~(1 << (linenum + 0x8)));
			break;

		      case DS_5000:	/* dcbrk is a char on 3max and option card */
		      case DS_5000_100:
			sc->dcbrk = ((dc_brk[ctlr] &= ~(1 << (linenum + 0x8))) >> 8);
			dcxint(unit);
			break;
		    }
		}
	} else if (dp->p_mem == dp->p_end) {
	    	dcxint(unit);
	} else {
	    	sc->dcbrk_tbuf = ((dc_brk[ctlr]) | (unsigned char)(*dp->p_mem++));
	}
}

dcxint(unit)
	register int unit;
{
	register struct dc_softc *sc;
	register struct dcpdma *dp;
	register struct tty *tp;
	register int ctlr;

	ctlr = unit >> 2;
	sc = &dc_softc[ctlr];

	tp = &dc_tty[unit];
	if ((consDev != GRAPHIC_DEV) && (unit == 0) && (major(tp->t_dev) == CONSOLEMAJOR)) {
		unit = 3;
	}

#ifdef DEBUG
	if (dcdebug > 4)
		mprintf("dcxint: unit = %d, tp = %x, c_cc = %d\n", unit, tp, tp->t_outq.c_cc);
#endif DEBUG

	tp->t_state &= ~TS_BUSY;
	dp = &sc->dc_pdma[unit & LINEMASK];
	if (tp->t_state & TS_FLUSH) {
		tp->t_state &= ~TS_FLUSH;
	} else {
	    	ndflush(&tp->t_outq, dp->p_mem-tp->t_outq.c_cf);
		dp->p_end = dp->p_mem = tp->t_outq.c_cf;
	}
	if (tp->t_line) {
		(*linesw[tp->t_line].l_start)(tp);
	}
	else {
		dcstart(tp);
	}

	/* The BUSY flag will not be set in two cases:		*/
	/*   1. if there are no more chars in the outq OR	*/
	/*   2. there are chars in the outq but tty is in	*/
	/*      stopped state.					*/
	if ((tp->t_state&TS_BUSY) == 0) {
	    	sc->dctcr &= ~(1<< (unit & LINEMASK));
	}
}

dcstart(tp)
	register struct tty *tp;
{
	register struct dc_softc *sc;
	register struct dcpdma *dp;
	register int cc;
	int s, unit, ctlr;

	unit = minor(tp->t_dev);
	ctlr = unit >> 2;
	sc = &dc_softc[ctlr];

	s = spltty();
	/*
	 * Do not do anything if currently delaying, or active.  Also only
	 * transmit when CTS is up.
	 */
	if ((tp->t_state & (TS_TIMEOUT|TS_BUSY|TS_TTSTOP)) ||
		(((tp->t_cflag & CLOCAL) == 0)
		&& ((tp->t_state&TS_CARR_ON) && (dcmodem[unit]&MODEM_CTS)==0)))
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
	if (dcdebug > 7) {
		mprintf("dcstart: unit = %d, tp = %x, c_cc = %d\n", unit, tp, tp->t_outq.c_cc);
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
	if ((consDev != GRAPHIC_DEV) && (unit == 0) && (major(tp->t_dev) == CONSOLEMAJOR))
		unit = 3;
	dp = &sc->dc_pdma[unit & LINEMASK];
	dp->p_end = dp->p_mem = tp->t_outq.c_cf;
	dp->p_end += cc;
	sc->dccsr |= DC_TIE;
	sc->dctcr |= (1<< (unit & LINEMASK));
out:
	splx(s);
}

dcstop(tp, flag)
	register struct tty *tp;
{
	register struct dc_softc *sc;
	register struct dcpdma *dp;
	register int s;
	int	unit, ctlr;

	/*
	 * If there is a graphics device and the stop call
	 * is for it, pass the call to the graphics device driver.
	 */
	unit = minor(tp->t_dev);
	ctlr = unit >> 2;
	sc = &dc_softc[ctlr];

	if ((consDev != GRAPHIC_DEV) && (unit == 0) && (major(tp->t_dev) == CONSOLEMAJOR)) {
		unit = 3;
	}
	if (dc_base_board && (ctlr == 0)) 
	    if (vs_gdstop && (unit <= 1)) {
		(*vs_gdstop)(tp, flag);
		return;
	    }
	dp = &sc->dc_pdma[unit & LINEMASK];
	s = spltty();
	if (tp->t_state & TS_BUSY) {
	    	dp->p_end = dp->p_mem;
		if ((tp->t_state&TS_TTSTOP)==0)
			tp->t_state |= TS_FLUSH;
	}
	splx(s);
}

dcmctl(dev, bits, how)
	dev_t dev;
	int bits, how;
{
	register struct dc_softc *sc;
	register int unit, ctlr, mbits;
	int b, s;

	unit = minor(dev);
	ctlr = unit >> 2;
	sc = &dc_softc[ctlr];

	if (dc_modem_line[unit] == 0)
		return(0);	/* only line 2 and 3 on base board have modem control */
	s = spltty();
	mbits = (sc->dcdtr & dc_rdtr[unit]) ? DC_DTR : 0;
	mbits |= (sc->dcdtr & dc_rrts[unit]) ? DC_RTS : 0;
	mbits |= (sc->dcmsr & dc_rcd[unit]) ? DC_CD : 0;
	mbits |= (sc->dcmsr & dc_rdsr[unit]) ? DC_DSR : 0;
	mbits |= (sc->dcmsr & dc_rcts[unit]) ? DC_CTS : 0;
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
	if (mbits & DC_DTR)
		sc->dcdtr |= (dc_rdtr[unit] | dc_rrts[unit]);
	else
		sc->dcdtr &= ~(dc_rdtr[unit] | dc_rrts[unit]);
	(void) splx(s);
	return(mbits);
}

#ifdef DEBUG
int dcscan_ctr = 1;
#endif DEBUG

/*
 *	WARNING:	This routine should only be called if the dc is on the 
 *			base board.  The option card has NO modem control .
 */
dcscan()
{
	register int i;
	register struct dc_softc *sc = &dc_softc[0];
	register struct tty *tp;
	register u_short dcscan_modem;
	static	u_short dcscan_previous;		/* Used to detect modem transitions */

#ifdef DEBUG
	if (dcdebug) {
		if (dcscan_ctr++ == 45) {
			cprintf("dcscan: ");
			PRINT_SIGNALS();
			dcscan_ctr = 1;
		}
	}
#endif DEBUG

	dcscan_modem = sc->dcmsr;	/* read copy of modem status register */
	if (dcscan_modem == dcscan_previous) {
	    /* no work to do reschedule scan routine */
	    if (dcmodem_active)
		timeout(dcscan, (caddr_t)0, hz/40);
	    else
		timeout(dcscan, (caddr_t)0, hz);
	    return;
	}
	for (i = 2; i < 4; i++) {
	    tp = &dc_tty[i];
	    if ((tp->t_cflag & CLOCAL) == 0) {
		/*
		 * Drop DTR immediately if DSR has gone away.
		 * If really an active close then do not
		 *    send signals.
		 */
		if (!(dcscan_modem & dc_rdsr[i])) {
		    if (tp->t_state&TS_CLOSING) {
			untimeout(wakeup, (caddr_t) &tp->t_dev);
			wakeup((caddr_t) &tp->t_dev);
		    }
		    if (tp->t_state&TS_CARR_ON) {
			dc_tty_drop(tp);
		    }
		} else {		/* DSR has come up */
		    /*
		     * If DSR comes up for the first time we allow
		     * 30 seconds for a live connection.
		     */
		    if ((dcmodem[i] & MODEM_DSR)==0) {
			dcmodem[i] |= (MODEM_DSR_START|MODEM_DSR);
			if (!dc_modem_ctl) {
			    /*
			     * Assume carrier will come up in less
			     * than 1 sec. If not DSR will drop
			     * and the line will close
			     */
			    timeout(dc_dsr_check, tp, hz);
			} else {
			    /*
			     * we should not look for CTS|CD for about
			     * 500 ms.
			     */
			    timeout(dc_dsr_check, tp, hz*30);
			    dc_dsr_check(tp);
			}
		    }
		}

		/*
		 * look for modem transitions in an already
		 * established connection.
		 *
		 * Ignore CD and CTS for PMAX.  These signals 
		 * don't exist on the PMAX.
		 */
		if (dc_modem_ctl) {
		    if (tp->t_state & TS_CARR_ON) {
			if (dcscan_modem & dc_rcd[i]) {
			    /*
			     * CD has come up again.
			     * Stop timeout from occurring if set.
			     * If interval is more than 2 secs then
			     * drop DTR.
			     */
			    if ((dcmodem[i] & MODEM_CD) == 0) {
				untimeout(dc_cd_drop, tp);
				if (dc_cd_down(tp)) {
				    /* drop connection */
				    dc_tty_drop(tp);
				}
				dcmodem[i] |= MODEM_CD;
			    }
			} else {
			    /*
			     * Carrier must be down for greater than
			     * 2 secs before closing down the line.
			     */
			    if (dcmodem[i] & MODEM_CD) {
				/* only start timer once */
				dcmodem[i] &= ~MODEM_CD;
				/*
				 * Record present time so that if carrier
				 * comes up after 2 secs, the line will drop.
				 */
				dctimestamp[i] = time;
				timeout(dc_cd_drop, tp, hz * 2);
			    }
			}
			
			/* CTS flow control check */
			
			if (!(dcscan_modem & dc_rcts[i])) {
			    /*
			     * Only allow transmission when CTS is set.
			     */
			    tp->t_state |= TS_TTSTOP;
			    dcmodem[i] &= ~MODEM_CTS;
#ifdef DEBUG
			    if (dcdebug)
				cprintf("dcscan: CTS stop, tp=%x,line=%d\n",tp,i);
#endif DEBUG
			    dcstop(tp, 0);
			} else if (!(dcmodem[i] & MODEM_CTS)) {
			    /*
			     * Restart transmission upon return of CTS.
			     */
			    tp->t_state &= ~TS_TTSTOP;
			    dcmodem[i] |= MODEM_CTS;
#ifdef DEBUG
			    if (dcdebug)
				cprintf("dcscan: CTS start, tp=%x,line=%d\n",tp,i);
#endif DEBUG
			    dcstart(tp);
			}
		    }

		    /*
		     * See if a modem transition has occured.  If we are waiting
		     * for this signal, cause action to be take via
		     * dc_start_tty.
		     */
		    if (((dcscan_modem & dc_xmit[i]) == dc_xmit[i]) &&
			(!(dcmodem[i] & MODEM_DSR_START)) &&
			(!(tp->t_state & TS_CARR_ON))) {
#ifdef DEBUG
			if (dcdebug)
			    cprintf("dcscan: MODEM transition: dcscan_modem = %x, dcscan_previous = %x\n", dcscan_modem, dcscan_previous);
#endif DEBUG
			dc_start_tty(tp);
		    }
		}
	    }
	}
	
	dcscan_previous = dcscan_modem; /* save for next iteration */

	if (dcmodem_active)
	    timeout(dcscan, (caddr_t)0, hz/40);
	else
	    timeout(dcscan, (caddr_t)0, hz);
}

int	dcputc();
int	dcgetc();

dcputc(c)
	register int c;
{
    if (consDev == GRAPHIC_DEV) 
    {
        if ( v_consputc ) {
	    (*v_consputc) (c);
	    if ( c == '\n' )
	        (*v_consputc)( '\r' );
	    return;
        }
    }
    dc_putc(3, c);
    if ( c == '\n')
	dc_putc(3, '\r');
}

/*
 * This routine outputs one character to the console.
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
dc_putc(unit, c)
        int unit;
	register int c;
{
	register struct dc_softc *sc = &dc_softc[0];
	register int	s; 
	register u_short tcr;
	register int	ln, tln = unit, timo;

	s = splhigh();
	tcr = (sc->dctcr & (1<<tln));
	sc->dctcr |= (1<<tln);
	while (1) {
		timo = 1000000;
		while ((sc->dccsr&DC_TRDY) == 0)	/* while not ready */
			if(--timo == 0)
				break;
		if(timo == 0)
			break;
		ln = (sc->dccsr>>8) & 3;
		if (ln != tln) {
			tcr |= (1 << ln);
			sc->dctcr &= ~(1 << ln);
			continue;
		}
		
		/* stuff char out, don't upset upper 8 bits */
		sc->dcbrk_tbuf = ((dc_brk[0]) | (c&0xff));
		DELAY(5); /* alow time for TRDY bit to become valid */
		while (1) {
			while ((sc->dccsr&DC_TRDY) == 0); /* while not ready */
			ln = (sc->dccsr>>8) & 3;
			if (ln != tln) {
				tcr |= (1 << ln);
				sc->dctcr &= ~(1 << ln);
				continue;
			}
			break;
		}
		break;
	}
	sc->dctcr &= ~(1<<tln);
	if (tcr != 0)
		sc->dctcr |= tcr;
	wbflush();
	splx(s);
}

/*
 * This routine operates on the following assumptions:
 * 1. putc must have happened first, so SLU already inited.
 * 2. getc will happed before slu reveive interrupt enabled so
 *    don't need to worry about int_req or int_msk registers.
 */
dcgetc()
{
	register u_short c;
	register int line;

	/*
	 * Line number we expect input from.
	 */
	if(consDev == GRAPHIC_DEV) 
		line = 0x0;
	else
		line = 0x3;

	c = dc_getc(line);

	if(v_consgetc)
		return((*v_consgetc)(c & 0xff));
	else
		return(c & 0xff);
}

dc_getc(unit)
int unit;
{
    struct	dc_softc *sc = &dc_softc[0];
    register int timo;
    register u_short c;
    
    for(timo=1000000; timo > 0; --timo) {
	if(sc->dccsr&DC_RDONE) {
	    c = sc->dcrbuf;
	    DELAY(50000);
	    if(((c >> 8) & 03) != unit)
		continue;
	    if(c&(DC_DO|DC_FE|DC_PE))
		continue;
	    return(c & 0xff);
	}
    }
    return(-1);
}
    
dc_mouse_init()
{
	struct	dc_softc *sc = &dc_softc[0];

	/*
 	 * Set SLU line 1 parameters for mouse communication.
 	 */
	sc->dclpr = (SER_POINTER | SER_CHARW | SER_PARENB | SER_ODDPAR
		| SER_SPEED | SER_RXENAB );
}

dc_mouse_putc(c)
int c;
{
    dc_putc(1, c);
}

dc_mouse_getc()
{
    return (dc_getc(1));
}

dc_kbd_init()
{
    struct	dc_softc *sc = &dc_softc[0];
    
    /*
     * Set the line parameters on SLU line 0 for
     * the LK201 keyboard: 4800 BPS, 8-bit char, 1 stop bit, no parity.
     */
    sc->dclpr = (SER_RXENAB | SER_KBD | SER_SPEED | SER_CHARW);
}

dc_kbd_putc(c)
int c;
{
    dc_putc(0, c);
}

dc_kbd_getc()
{
    return (dc_getc(0));
}
/*
 * Modem Control Routines
 */

/*
 *
 * Function:
 *
 *	dc_cd_drop
 *
 * Functional description:
 *
 * 	Determine if carrier has dropped.  If so call dc_tty_drop to terminate
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
dc_cd_drop(tp)
register struct tty *tp;
{
        register struct dc_softc *sc;
	register int unit, ctlr;

	unit = minor(tp->t_dev);
	ctlr = unit >> 2;
	sc = &dc_softc[ctlr];

	if ((tp->t_state & TS_CARR_ON) && (!(sc->dcmsr & dc_rcd[unit]))) {
#ifdef DEBUG
	    if (dcdebug)
		cprintf("dc_cd_drop: no CD, tp = %x, line = %d\n", tp, unit);
#endif DEBUG
	    dc_tty_drop(tp);
	    return;
	}
	dcmodem[unit] |= MODEM_CD;
#ifdef DEBUG
	if (dcdebug)
	    cprintf("dc_cd_drop:  CD is up, tp = %x, line = %d\n", tp, unit);
#endif DEBUG
}
 
/*
 *
 * Function:
 *
 *	dc_dsr_check
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
dc_dsr_check(tp)
register struct tty *tp;
{
	register struct dc_softc *sc;
	register int unit, ctlr;

	unit = minor(tp->t_dev);
	ctlr = unit >> 2;
	sc = &dc_softc[ctlr];

#	ifdef DEBUG
       	if (dcdebug) {
       	    cprintf("dc_dsr_check0:  tp=%x\n", tp);
	    PRINT_SIGNALS();
	}
#	endif DEBUG

	if (dcmodem[unit] & MODEM_DSR_START) {
	    dcmodem[unit] &= ~MODEM_DSR_START;
	    /*
	     * since dc7085 chip on PMAX only provides DSR then assume that CD
	     * has come up after 1 sec and start tty.  If CD has not
	     * come up the modem should deassert DSR thus closing the line
	     *
	     * On 3max, we look for DSR|CTS|CD before establishing a
	     * connection.
	     */
	    if ((!dc_modem_ctl) || 
		((sc->dcmsr & dc_xmit[unit]) == dc_xmit[unit])) {
		dc_start_tty(tp);
	    }
	    return;
	}
	if ((tp->t_state&TS_CARR_ON)==0)
		dc_tty_drop(tp);
}

/*
 *
 * Function:
 *
 *	dc_cd_down
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
dc_cd_down(tp)
register struct tty *tp;
{
        register int msecs, unit;

	unit = minor(tp->t_dev);
	msecs = 1000000 * (time.tv_sec - dctimestamp[unit].tv_sec) + 
		(time.tv_usec - dctimestamp[unit].tv_usec);
	if (msecs > 2000000){
#		ifdef DEBUG
		if (dcdebug)
			cprintf("dc_cd_down: msecs > 20000000\n");
#		endif DEBUG
		return(1);
	}
	else{
#		ifdef DEBUG
		if (dcdebug)
			cprintf("dc_cd_down: msecs < 20000000\n");
#		endif DEBUG
		return(0);
	}
}

/*
 *
 * Function:
 *
 *	dc_tty_drop
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
dc_tty_drop(tp)
struct tty *tp;
{
	register struct dc_softc *sc;
	register int unit, ctlr;

	unit = minor(tp->t_dev);
	ctlr = unit >> 2;
	sc = &dc_softc[ctlr];

	dcmodem_active[ctlr] &= ~(1 << (unit & LINEMASK));
	if (tp->t_flags & NOHANG)
		return;
#	ifdef DEBUG
	if (dcdebug)
		cprintf("dc_tty_drop: unit=%d\n",minor(tp->t_dev));
#	endif DEBUG
	/* 
	 * Notify any processes waiting to open this line.  Useful in the
	 * case of a false start.
	 */
	dcmodem[unit] = MODEM_BADCALL;
	tp->t_state &= ~(TS_CARR_ON|TS_TTSTOP|TS_BUSY|TS_ISUSP);
	wakeup((caddr_t)&tp->t_rawq);
	gsignal(tp->t_pgrp, SIGHUP);
	gsignal(tp->t_pgrp, SIGCONT);
	sc->dcdtr &= ~(dc_rdtr[unit] | dc_rrts[unit]);
}
/*
 *
 * Function:
 *
 *	dc_start_tty
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
dc_start_tty(tp)
	register struct tty *tp;
{
        register int unit, ctlr;

	unit = minor(tp->t_dev);
	ctlr = unit >> 2;

	dcmodem_active[ctlr] |= (1 << (unit & LINEMASK));
	tp->t_state &= ~(TS_ONDELAY);
	tp->t_state |= TS_CARR_ON;
#	ifdef DEBUG
        if (dcdebug)
	       cprintf("dc_start_tty:  tp=%x\n", tp);
#	endif DEBUG
	if (dcmodem[unit] & MODEM_DSR)
		untimeout(dc_dsr_check, tp);
	dcmodem[unit] |= MODEM_CD|MODEM_CTS|MODEM_DSR;
	dctimestamp[unit].tv_sec = dctimestamp[unit].tv_usec = 0;
	wakeup((caddr_t)&tp->t_rawq);
}

dcbaudrate(speed)
int speed;
{
    return(dc_speeds[speed & CBAUD].baud_support);
}

void dcsetbreak(tp)
register struct tty *tp;
{
	wakeup((caddr_t)&tp->t_dev);
}
