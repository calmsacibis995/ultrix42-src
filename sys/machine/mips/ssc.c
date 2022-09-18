#ifndef lint
static char *sccsid = "@(#)ssc.c	4.2      (ULTRIX)  8/9/90";
#endif lint
/************************************************************************
 *									*
 *			Copyright (c) 1986,87,88,89 by			*
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
 ************************************************************************/

/***********************************************************************
 *
 * Modification History:	ssc.c
 *
 * 4/30/90 - Randall Brown
 *	Use the values from the cpu switch for todrzero. 
 *
 * 3/29/90 - gmm
 *	changed splhigh() to splextreme() since now splhigh() same as 
 *	splclock()
 *
 * 3/6/90 - jaw
 *	fix ipl handling in console putchar routine.
 *
 * 3/9/89 - created by burns. To collect all ssc related code into one
 *	place and decouple it from vaxes.
 */


/*
 * Include files
 */
#include "../h/param.h"
#include "../h/conf.h"
#include "../h/time.h"
#include "../h/errno.h"
#include "../h/systm.h"
#include "../h/types.h"
#include "../h/errlog.h"
#include "../h/ioctl.h"
#include "../h/user.h"
#include "../h/tty.h"
#include "../machine/ssc.h"
#include "../machine/clock.h"
#include "../machine/cons.h"
#include "../machine/cpu.h"
#include "../../machine/common/cpuconf.h"

/*
 * Globals - with default values
 */
struct ssc_regs *ssc_ptr =
		(struct ssc_regs *)PHYS_TO_K1(DEFAULT_SSC_PHYS);
int ssc_console_timeout = DEFAULT_SSC_TIMEOUT;


/*
 * SSC - console routines
 *
 * These routines are generic in that they work on many implemantations that
 * use the SSC chip.
 */
/*
 * 
 */
int	ssc_cnstart();
struct tty	cons;
extern	int	ttrstrt();
extern	int	ttydef_open();
extern	int	ttydef_close();



ssc_cnopen(dev, flag)
    dev_t dev;
    unsigned int flag;
{
    register struct tty *tp;
    register int s;

    if (minor(dev) != 0) {
	return (ENXIO);
    }

    tp = &cons;
    tp->t_oproc = ssc_cnstart;

    cons_def_open(tp, flag);

    if ((tp->t_state & TS_XCLUDE) && (u.u_uid != 0)) {
	return (EBUSY);
    }

    s = spltty();
    ssc_ptr->ssc_crcs |= RXCS_IE; /* enable receiver interrupts */
    splx(s);

    return((*linesw[tp->t_line].l_open)(dev, tp));
}


ssc_cnclose(dev)
    dev_t dev;
{
    register struct tty *tp = &cons;

    if (minor(dev) != 0) {
	return (ENXIO);
    }

    (*linesw[tp->t_line].l_close)(tp);

    ssc_ptr->ssc_crcs &= ~RXCS_IE; /* disable receiver interrupts */
    ttyclose(tp);

    cons_def_close(tp);
}


ssc_cnread(dev, uio)
    dev_t dev;
    struct uio *uio;
{
    register struct tty *tp = &cons;

    if (minor(dev) != 0) {
	return (ENXIO);
    }

    return ((*linesw[tp->t_line].l_read)(tp, uio));
}


ssc_cnwrite(dev, uio)
    dev_t dev;
    struct uio *uio;
{
    register struct tty *tp = &cons;

    if (minor(dev) != 0) {
	return (ENXIO);
    }

    return ((*linesw[tp->t_line].l_write)(tp, uio));
}


ssc_cnioctl(dev, cmd, addr, flag)
    dev_t dev;
    unsigned int cmd;
    caddr_t addr;
    unsigned int flag;
{
    register struct tty *tp = &cons;
    int error;

    if (minor(dev) != 0) {
	return (ENXIO);
    }

    error = (*linesw[tp->t_line].l_ioctl)(tp, cmd, addr);
    if (error >= 0) {
	return(error);
    }
    error = ttioctl(tp, cmd, addr, flag);
    if (error < 0) {
	error = ENOTTY;
    }
    return (error);
}


ssc_cnrint(dev)
    dev_t dev;
{
    register struct tty *tp = &cons;
    register int c;

    c = ssc_ptr->ssc_crdb;

    if (tp->t_state & TS_ISOPEN) {
	if (tp->t_iflag & ISTRIP) {
	    c &= 0177;
	} else {
	    c &= CHAR_MASK;
	    
	    /* If ISTRIP is not set a valid character of 377
	     * is read as 0377,0377 to avoid ambiguity with
	     * the PARMARK sequence.
	     */ 
	    if ((c == 0377) && (tp->t_line == TERMIODISC)) {
		(*linesw[tp->t_line].l_rint)(c, tp);
	    }
	}
	(*linesw[tp->t_line].l_rint)(c, tp);
    }
}


ssc_cnxint(dev)
    dev_t dev;
{
    struct tty *tp = &cons;


    tp->t_state &= ~TS_BUSY;

    if (tp->t_line) {
	(*linesw[tp->t_line].l_start)(tp);
    } else {
	ssc_cnstart(tp);
    }
    if ((tp->t_state & TS_BUSY) == 0) {
	ssc_ptr->ssc_ctcs &= ~TXCS_IE; /* no chars to send diable transmitter intr */
    }
}


ssc_cnstart(tp)
    register struct tty *tp;
{
    register int c, s;

    if (tp->t_state & (TS_TIMEOUT|TS_BUSY|TS_TTSTOP)) {
	goto out;
    }
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
    if (tp->t_outq.c_cc == 0) {
	goto out;
    }

    s = spltty();

    c = getc(&tp->t_outq);

    if ((tp->t_lflag_ext & PRAW) || (tp->t_oflag_ext & PLITOUT) || ((tp->t_oflag & OPOST) == 0)) {
	tp->t_state |= TS_BUSY;
	c &= CHAR_MASK;
	ssc_ptr->ssc_ctdb = c;
	ssc_ptr->ssc_ctcs |= TXCS_IE;	/* let chip interrupt for next char */
    } else if ((c & DELAY_FLAG) == 0) {
	if ((tp->t_cflag & CS8) != CS8) {
	    c &= 0177; /* mask to 7 bits */
	} else {
	    c &= CHAR_MASK;
	}
	tp->t_state |= TS_BUSY;
	ssc_ptr->ssc_ctdb = c;
	ssc_ptr->ssc_ctcs |= TXCS_IE;	/* let chip interrupt for next char */
    } else {
	tp->t_state |= TS_TIMEOUT;
	timeout(ttrstrt, (caddr_t)tp, (c&CHAR_MASK));
    }

    splx(s);

out:
	return;
}


ssc_cnputc(c)
    register int c;
{
    register int s, timo, savessc_ctcs;
    
    /* if spl is less then tty we must raise priority to block out
       tty interrupts.  If IPL is higher, we don't want to lower it */

    if (whatspl(getspl()) < SPLTTY) {
    	s = spltty(); 
    } else {
    	s = getspl();
    }
    timo = ssc_console_timeout;

    while ((ssc_ptr->ssc_ctcs&TXCS_RDY) == 0) {
	if (--timo == 0) {
	    break;
	}
    }

    if (c == 0) {
	splx(s);
	return;
    }

    savessc_ctcs = ssc_ptr->ssc_ctcs; /* save present state of control register */
    ssc_ptr->ssc_ctcs = 0;	   /* disable transmitter interrupts */
    ssc_ptr->ssc_ctdb = c & 0xff; /* output char to transmitter buffer */

    if (c == '\n') {		/* map carriage return - line feed */
	ssc_cnputc('\r');
    }
/*    ssc_cnputc(0);*/

    ssc_ptr->ssc_ctcs = savessc_ctcs; /* restore state of control register */

    splx(s);
}

ssc_cngetc()
{
	register int s, c;

	s = splextreme();
	while ((ssc_ptr->ssc_crcs & RXCS_DONE) == 0)
		;
	c = (ssc_ptr->ssc_crdb) & 0xff;
	if (c == '\r')
		c = '\n';
	splx(s);
/*	cnputc(c);*/ 
	return (c);
}


/* taken from vax/machdep.c -- burns
 * delay for n microseconds, limited to somewhat over 2000 microseconds
 * using SSC (CVAX system support chip) programmable timer for lack of ICR.
 */

uSSCdelay(n)
int n;
{
	int s;
	register struct ssc_regs *addr;

	addr = ssc_ptr;

#ifdef	vax
	s = spl6();
#endif	vax
#ifdef	mips
	s = splclock();
#endif	mips
	addr->ssc_tnir0 = -n; 	/* load neg n */
	wbflush();
	addr->ssc_tcr0 = ICCS_RUN+ICCS_TRANS+ICCS_INT+ICCS_ERR+TCR_STP;
	wbflush();
	while ( !(addr->ssc_tcr0 & ICCS_INT))
		;			/* wait */
	splx(s);
	return(0);
}


/*
 * ssc_readtodr - read ssc toy clock register.
 */
ssc_readtodr()
{
	return((int)ssc_ptr->ssc_toy);
}

/*
 * ssc_writetodr - write ssc toy clock register.
 */
ssc_writetodr(yrtime)
unsigned long yrtime;
{
        extern struct cpusw *cpup;

	ssc_ptr->ssc_toy = (cpup->todrzero + (100 * yrtime));
}
