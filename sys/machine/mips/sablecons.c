/* ------------------------------------------------------------------ */
/* | Copyright Unpublished, MIPS Computer Systems, Inc.  All Rights | */
/* | Reserved.  This software contains proprietary and confidential | */
/* | information of MIPS and its suppliers.  Use, disclosure or     | */
/* | reproduction is prohibited without the prior express written   | */
/* | consent of MIPS.                                               | */
/* ------------------------------------------------------------------ */

#include "../h/param.h"
#include "../h/conf.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/proc.h"
#include "../h/ioctl.h"
#include "../h/tty.h"
#include "../h/systm.h"
#include "../h/uio.h"

#include "../mips/cpu.h"
#include "../mips/sablecons.h"

struct	tty cons;
int	cnstart();
int	ttrstrt();
extern char	partab[];

static struct scons_device *scdevp = SCONS0_BASE;

cnattach()
{
	return(1);
}

/*ARGSUSED*/
cnopen(dev, flag)
	dev_t dev;
{
	register struct tty *tp = &cons;

	tp->t_oproc = cnstart;
	if ((tp->t_state&TS_ISOPEN) == 0) {
		ttychars(tp);
		tp->t_state = TS_ISOPEN|TS_CARR_ON;
		tp->t_flags = EVENP|ECHO|XTABS|CRMOD|CRTBS;
		tp->t_line = NTTYDISC;
	}
	if (tp->t_state&TS_XCLUDE && u.u_uid != 0)
		return (EBUSY);

	/* enable receiver interrupts */
	scdevp->sc_command = SC_CMD_RXIE;
	return ((*linesw[tp->t_line].l_open)(dev, tp));
}

/*ARGSUSED*/
cnclose(dev)
	dev_t dev;
{
	register struct tty *tp = &cons;

	(*linesw[tp->t_line].l_close)(tp);
	ttyclose(tp);
}

/*ARGSUSED*/
cnread(dev, uio)
	dev_t dev;
	struct uio *uio;
{
	register struct tty *tp = &cons;

	return ((*linesw[tp->t_line].l_read)(tp, uio));
}

/*ARGSUSED*/
cnwrite(dev, uio)
	dev_t dev;
	struct uio *uio;
{
	register struct tty *tp = &cons;

	return ((*linesw[tp->t_line].l_write)(tp, uio));
}

/*
 * Got receive interrupt
 */
/*ARGSUSED*/
cn_intr(dev)
	dev_t dev;
{
	register int c;
	register struct tty *tp;

	while (scdevp->sc_status & SC_STAT_RXRDY) {
		c = (int)scdevp->sc_rx;
		tp = &cons;
		(*linesw[tp->t_line].l_rint)(c, tp);
	}
}

/*ARGSUSED*/
cnioctl(dev, cmd, addr, flag)
	dev_t dev;
	caddr_t addr;
{
	register struct tty *tp = &cons;
	int error;
 
	error = (*linesw[tp->t_line].l_ioctl)(tp, cmd, addr);
	if (error >= 0)
		return (error);
	error = ttioctl(tp, cmd, addr, flag);
	if (error < 0)
		error = ENOTTY;
	return (error);
}

cnstart(tp)
	register struct tty *tp;
{
	register int c, s;

	s = spltty();
loop:
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
	c = getc(&tp->t_outq);

	if (tp->t_flags&(RAW|LITOUT)) {
		/*
		 * TODO:
		 * Try to buffer using the run length technique used
		 * with dma out devices.
		 */ 
		scdevp->sc_txbuf = c;
	} else if (c <= 0177) {
		/*
		 * TODO: 
		 * Try to buffer using the run length technique used
		 * with dma out devices.
		 */ 
#ifdef notdef
		/* sun window driver hates parity bit */
		c |= ((partab[c]&0200)&0xff);
#else
		c &= 0177;
#endif
		scdevp->sc_txbuf = c;
	} else {
		timeout(ttrstrt, (caddr_t)tp, (c&0177));
		tp->t_state |= TS_TIMEOUT;
		goto out;
	}
	goto loop;
#ifdef notdef
	tp->t_state |= TS_BUSY;
#endif
out:
	scdevp->sc_command |= SC_CMD_TXFLUSH;
	splx(s);
}

/*
 * Print a character on console. (buffered)
 */
cnputc(c)
	register int c;
{
	scdevp->sc_txbuf = c;
	if (c == '\n')
		scdevp->sc_txbuf = '\r';
}

/*
 * Print a character on console. (un-buffered)
 */
cnputc_flush(c)
	register int c;
{
	if (c == '\n') {
		scdevp->sc_txbuf = c;
		scdevp->sc_tx = '\r';
	} else if (c == '\0') {
		scdevp->sc_command |= SC_CMD_TXFLUSH;
	} else {
		scdevp->sc_tx = c;
	}
}
/*
 * Read a character from the console.
 */
cngetc()
{
	while (!(scdevp->sc_status & SC_STAT_RXRDY))
		continue;
	return((int)scdevp->sc_rx);
}
