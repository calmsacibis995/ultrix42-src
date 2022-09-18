#ifndef lint
static char *sccsid = "@(#)s2681cons.c	4.1	ULTRIX	7/2/90";
#endif lint
/* ------------------------------------------------------------------ */
/* | Copyright Unpublished, MIPS Computer Systems, Inc.  All Rights | */
/* | Reserved.  This software contains proprietary and confidential | */
/* | information of MIPS and its suppliers.  Use, disclosure or     | */
/* | reproduction is prohibited without the prior express written   | */
/* | consent of MIPS.                                               | */
/* ------------------------------------------------------------------ */

#include "../h/param.h"
#include "../h/conf.h"
#include "../h/vm.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/proc.h"
#include "../h/ioctl.h"
#include "../h/tty.h"
#include "../h/uio.h"
#include "../h/file.h"
#include "../h/kernel.h"
#include "../h/syslog.h"

#include "../machine/s2681cons.h"
#include "../machine/cpu.h"

#define	ISPEED	B9600
#define	IFLAGS	(EVENP|ODDP|ECHO|XTABS|CRMOD)

#define	CN_REG_BASE	(*(struct cn_reg*)PHYS_TO_K1(0x1e007fff))
#define	CN_REG_ADDR	(&CN_REG_BASE)
#define CN_CHAN_A	(&(CN_REG_BASE.cn_chan_a))
#define CN_CHAN_B	(&(CN_REG_BASE.cn_chan_b))
#define CN_REG	register volatile struct cn_reg
#define CN_CHAN	register volatile struct cn_chan

char	cn_soft_imr;	/* copy of write-only interrupt mask register. */
int	cn_overrun;	/* reciever over-run counter */
char	soft_cn_inport;
struct uartstat {
	u_char	us_mr1;
	u_char	us_mr2;
	u_char	us_baud;
} uartstat[2];


struct	tty cons[2];
int	cnstart(), ttrstrt();
int	cnscan();	/* watch modem control lines */
extern char	partab[];

/*
 * This routine is called directly from startup so that printing on the
 * console will be possible very early. The boot ROM has probably done 
 * enough initialization to be able to do polled operation, but we want
 * to be as independent of the boot ROM as possible.
 */
cnattach()
{
	CN_REG *cnr = CN_REG_ADDR;
	CN_CHAN *cnc;

	/* 
	 * Put channel B into a known state. It is assumed that channel A
	 * has been programmed by the boot ROMS.
	 */
	cnc = CN_CHAN_B;
	cnc->cnc_cmd = CNCMD_RXRESET; wbflush();
	cnc->cnc_cmd = CNCMD_TXRESET; wbflush();
	cnc->cnc_cmd = CNCMD_ERRESET; wbflush();
	cnc->cnc_cmd = (CNCMD_BREAKRESET | CNCMD_RXENABLE | CNCMD_TXENABLE);

	cnr->cn_outport_conf = 0x0;	/* quiet port b until open */
	cnr->cn_auxreg = (CNAUX_SET1|	/* select baud rate table 1 */
			CNAUX_ENIP2);	/* accept carrier change on b */
	cnr->cn_imr = 0x0;	/* sync hardware/software intr mask */
	cn_soft_imr = 0x0;	/* sync hardware/software intr mask */
	cn_overrun = 0;		/* no overruns yet */
}

#define CNIDLE 0
#define CNALERT 1
static unsigned char cntimer_check[2];	/* start out idle */
static unsigned char cntimer_started;

static int
cntimer()
{
	register i;
	register s;

	s = spltty();
	for (i=0; i<2; i++) {
		if (cons[i].t_state & TS_BUSY) {
			if (cntimer_check[i] == CNALERT) {
				cnxint(i);
				log(LOG_WARNING,
				   "cntimer: dropped tx_intr on unit %d\n", i);
			} else {
				cntimer_check[i] = CNALERT;
			}
		}
	}
	splx(s);
	timeout(cntimer, (caddr_t)0, (7*hz)/2);	/* every 3.5 seconds */
}

cnopen (dev, flag)
dev_t dev;
{
	register struct tty *tp;
	register int unit;
	register int s;
	CN_REG *cnr = CN_REG_ADDR;
	CN_CHAN *cnc;
	int speed, prom_getenv();

	unit = minor (dev);
	if (unit >= 2)
		return (ENXIO);
	tp = &cons[unit];

	tp->t_oproc = cnstart;
	tp->t_state |= TS_WOPEN;
	if ((tp->t_state & TS_ISOPEN) == 0) {
		ttychars (tp);
		if (unit==1) {
			atob(prom_getenv("rbaud"), &speed);
		} else {
			atob(prom_getenv("lbaud"), &speed);
		}
		speed = map_speed(speed);
		if (speed == -1)
			speed = ISPEED;
#ifdef ultrix
		tp->t_cflag = speed & CBAUD;
		tp->t_cflag_ext = speed & CBAUD;
#else !ultrix
		tp->t_ospeed = tp->t_ispeed = speed;
#endif ultrix
		tp->t_flags = IFLAGS;
		/* tp->t_state |= TS_HUPCLS; */
		cnparam (unit);
		/*
		 * Assert DTR and accept RX and CD interrupts.
		 */
		if (unit==1) {
			cnr->cn_outport_set = CNOP_DTR_B|CNOP_RTS_B;
			cn_soft_imr |= (CNIMR_RXRDY_B | CNIMR_CD_B);
		} else {
			/* no modem control on channel A */
			cn_soft_imr |= CNIMR_RXRDY_A;
		}
		cnr->cn_imr = cn_soft_imr;
	} else if (tp->t_state & TS_XCLUDE && u.u_uid != 0)
		return (EBUSY);
	/*
	 * Wait for DSR, if applicable
	 */
#ifdef notdef
	/*
	 * I think this code was here to assist with multiplexing line 
	 * between ACU and auto-answer functions.
	 */
	if (flag & O_NDELAY) {
		tp->t_state |= TS_CARR_ON;
		tp->t_flags |= NOHANG;
	}
#endif
	s = spltty();
	if (unit == 1) {
		if (cnr->cn_inport & CNIP_CD_B) {
			while ((tp->t_state & TS_CARR_ON) == 0) {
				tp->t_state |= TS_WOPEN;
				sleep ((caddr_t) & tp->t_rawq, TTIPRI);
			}
		} else {
			tp->t_state |= TS_CARR_ON;
		}
	} else {
		tp->t_state |= TS_CARR_ON;
	}
	splx(s);

	if (cntimer_started == 0) {
		cntimer();		/* start the watchdog */
		cntimer_started = 1;
	}

	return ((*linesw[tp->t_line].l_open) (dev, tp));
}

cnclose(dev, flag)
dev_t dev;
{
	register struct tty *tp;
	register int unit;
	CN_REG *cnr = CN_REG_ADDR;

	unit = minor(dev);
	if (unit >= 2)		/* For safety, not in dz.c */
		return (ENXIO);
	tp = &cons[unit];
	(*linesw[tp->t_line].l_close)(tp);
	if ((tp->t_state & (TS_HUPCLS|TS_WOPEN))
	    || (tp->t_state & TS_ISOPEN) == 0) {
		/*
		 * Drop DTR|RTS, clear breaks, disable interrupts
		 */
		if (unit==1) {
			cnr->cn_outport_clear = CNOP_DTR_B|CNOP_RTS_B;
			cnr->cn_chan_b.cnc_cmd = CNCMD_BREAKSTOP;
			cn_soft_imr &= ~(CNIMR_RXRDY_B | CNIMR_TXRDY_B);
			cn_soft_imr &= ~CNIMR_CD_B;
		} else {
			cnr->cn_chan_a.cnc_cmd = CNCMD_BREAKSTOP;
			cn_soft_imr &= ~(CNIMR_RXRDY_A | CNIMR_TXRDY_A);
		}
		cnr->cn_isr = cn_soft_imr;
	}
	ttyclose(tp);
}

cnread (dev, uio)
dev_t dev;
struct uio *uio;
{
	register struct tty *tp;

	tp = &cons[minor (dev)];
	return ((*linesw[tp->t_line].l_read) (tp, uio));
}

cnwrite (dev, uio)
dev_t dev;
struct uio *uio;
{
	register struct tty *tp;

	tp = &cons[minor (dev)];
	return ((*linesw[tp->t_line].l_write) (tp, uio));
}

cn_intr()
{
	register u_char isr;
	CN_REG *cnr = CN_REG_ADDR;

	CURRENT_CPUDATA->cpu_intr++;

	while (isr = (cnr->cn_isr & cn_soft_imr)) {
		if (isr & CNIMR_RXRDY_A)
			cnrint(0);	/* read channel a receiver */
		if (isr & CNIMR_RXRDY_B)
			cnrint(1);	/* read channel b receiver */
		if (isr & CNIMR_TXRDY_A)
			cnxint(0);	/* feed channel a transmitter */
		if (isr & CNIMR_TXRDY_B)
			cnxint(1);	/* feed channel b transmitter */
		if (isr & CNIMR_CD_B)
			cncarrier();	/* service change in CD (chan B) */
	}
}

cnrint(unit)
{
	register struct tty *tp;
	CN_CHAN *cnc;
	register c;
	register status;
	int overrun = 0;

	tp = &cons[unit];
	if (unit==1)
		cnc = CN_CHAN_B;
	else
		cnc = CN_CHAN_A;
	if ((status = cnc->cnc_stat) & CNSTAT_RXRDY)
		c = cnc->cnc_data;
	if ((tp->t_state & (TS_ISOPEN))==0) {
		return;
	}
	if (status & CNSTAT_PARITY) {
		if ((tp->t_flags & (EVENP|ODDP)) == EVENP
		    || (tp->t_flags & (EVENP|ODDP)) == ODDP) {
			return;
		}
	}
	if ((status & CNSTAT_OVERRUN) && overrun == 0) {
		overrun = 1;
		/*printf("cons[%d]: overrun error\n",unit);*/
		overrun = 0;
		cn_overrun++;
		cnc->cnc_cmd = CNCMD_ERRESET;
	}
	if ((status & CNSTAT_FRAME) || (status & CNSTAT_RXBREAK)) {
		if (tp->t_flags & RAW) {
			c = 0;
		} else {
#ifdef ultrix
			c = tp->t_cc[VINTR];
#else !ultrix
			c = tp->t_intrc;
#endif ultrix
		}
	}
	(*linesw[tp->t_line].l_rint)(c, tp);
}


cnxint(unit)
{
	CN_REG *cnr = CN_REG_ADDR;
	register struct tty *tp;

	tp = &cons[unit];
	cntimer_check[unit] = CNIDLE;		/* pacify the watchdog */
	tp->t_state &= ~TS_BUSY;
	if (unit==1) {
		cn_soft_imr &= ~CNIMR_TXRDY_B;
	} else {
		cn_soft_imr &= ~CNIMR_TXRDY_A;
	}
	cnr->cn_imr = cn_soft_imr;
	if (tp->t_state & TS_FLUSH)
		tp->t_state &= ~TS_FLUSH;
	if (tp->t_line)
		(*linesw[tp->t_line].l_start)(tp);
	else
		cnstart(tp);
}


cnioctl (dev, cmd, data, flag)
dev_t dev;
caddr_t data;
{
	register struct	tty *tp;
	register int unit;
	CN_REG *cnr = CN_REG_ADDR;
	CN_CHAN *cnc;
	int error;

	unit = minor (dev);
	tp = &cons[unit];
	if (unit==1)
		cnc = CN_CHAN_B;
	else
		cnc = CN_CHAN_A;

	error = (*linesw[tp->t_line].l_ioctl) (tp, cmd, data, flag);
	if (error >= 0)
		return (error);
	error = ttioctl (tp, cmd, data, flag);
	if (error >= 0) {
		if (cmd == TIOCSETP || cmd == TIOCSETN)
			cnparam (unit);
		return (error);
	}
	switch (cmd) {

	case TIOCSBRK:
		cnc->cnc_cmd = CNCMD_BREAKSTART;
		break;

	case TIOCCBRK:
		cnc->cnc_cmd = CNCMD_BREAKSTOP;
		break;

	case TIOCSDTR:
		if (unit==1) {
			cnr->cn_outport_set = CNOP_DTR_B;
		} else
			return(ENOTTY);		/* no modem control */
		break;

	case TIOCCDTR:
		if (unit==1) {
			cnr->cn_outport_clear = CNOP_DTR_B;
		} else
			return(ENOTTY);		/* no modem control */
		break;

	case TIOCMSET:	/* XXX */
	case TIOCMBIS:	/* XXX */
	case TIOCMBIC:	/* XXX */
	case TIOCMGET:	/* XXX */
	default: 
		return (ENOTTY);
	}
	return (0);
}

/*
 *	cnparam - set parameter from tty struct into 2681 registers
 */
cnparam(unit)
{
	register struct tty *tp;
	CN_REG *cnr = CN_REG_ADDR;
	CN_CHAN *cnc;
	int s,i;
	int ibaud, obaud;
	u_char mr1, mr2;
	struct uartstat *us;

	tp = &cons[unit];
	if (unit==1) {
		cnc = CN_CHAN_B;
		us = &uartstat[1];
	} else {
		cnc = CN_CHAN_A;
		us = &uartstat[0];
	}
	/* 
	 * Block interrupts so parameters will be set
	 * before the line interrupts.
	 */
	s = spltty();
#ifdef ultrix
	if ((tp->t_cflag&CBAUD)==0) {
#else !ultrix
	if ((tp->t_ispeed)==0) {
#endif ultrix
		tp->t_state |= TS_HUPCLS;
		if (unit == 1)
			cnr->cn_outport_clear = CNOP_DTR_B|CNOP_RTS_B;
		splx(s);
		return;
	}
	/*
	 * Set baud rate. For now will only support ACR[7]=1 range of rates.
	 * See the 2681 manual for details.
	 */
#ifdef ultrix
	if (((ibaud = map_baud(tp->t_cflag&CBAUD)) != -1) &&
	    ((obaud = map_baud(tp->t_cflag_ext&CBAUD)) != -1)) {
#else !ultrix
	if (((ibaud = map_baud(tp->t_ispeed)) != -1) &&
	    ((obaud = map_baud(tp->t_ospeed)) != -1)) {
#endif ultrix
		obaud =  (ibaud << 4) | obaud;
	}

	/* build new mode word */
#ifdef ultrix
	if ((tp->t_cflag&CBAUD) == B134)
#else !ultrix
	if ((tp->t_ispeed) == B134)
#endif ultrix
		mr1 = CNMR1_BITS6 | CNMR1_PARITY;
	else if (tp->t_flags & (RAW|LITOUT|PASS8))
		mr1 = CNMR1_BITS8 | CNMR1_NOPARITY;
	else
		mr1 = CNMR1_BITS7 | CNMR1_PARITY;

	if ((tp->t_flags&(EVENP|ODDP)) == ODDP)
		mr1 |= CNMR1_ODDPARITY;
	else
		mr1 |= CNMR1_EVENPARITY;

#ifdef ultrix
	if ((tp->t_cflag&CBAUD) == B110) /* NO! not the silent 700, pleeeezz..*/
#else !ultrix
	if (tp->t_ispeed == B110)	/* NO! not the silent 700, pleeeezz..*/
#endif ultrix
		mr2 = CNMR2_STOP2;
	else
		mr2 = CNMR2_STOP1;

	/* 
	 * Avoid the busy wait on TXEMPTY if the parameters are not changing.
	 */
	if (us->us_mr1 == mr1 && us->us_mr2 == mr2 && us->us_baud == obaud) {
		goto done;
	}
	us->us_mr1 = mr1;
	us->us_mr2 = mr2;
	us->us_baud = obaud;

	i = 0;
	while (!(cnc->cnc_stat & CNSTAT_TXEMPTY) && i < 20) {
		DELAY(100);
		i++;
	}
	if (i == 20)
		printf("cnparam: timeout on TXEMPTY\n");
	cnc->cnc_stat = obaud;
	cnc->cnc_cmd = CNCMD_MRRESET; wbflush();
	cnc->cnc_mode = mr1; wbflush();
	cnc->cnc_mode = mr2; wbflush();
done:
	splx(s);
}


map_baud (symbol)
char    symbol;
{
	int     speed;

	switch (symbol) {
		case B75: speed = 0; break;
		case B110: speed = 1; break;
		case B134: speed = 2; break;
		case B150: speed = 3; break;
		case B300: speed = 4; break;
		case B600: speed = 5; break;
		case B1200: speed = 6; break;
		case B2400: speed = 8; break;
		case B4800: speed = 9; break;
		case B1800: speed = 10; break;
		case B9600: speed = 11; break;
		case EXTA: speed = 12; break;
		default: speed = -1; break;
	}
	return (speed);
}

map_speed (speed)
 int    speed;
{
	switch (speed) {
		case 75: speed = B75; break;
		case 110: speed = B110; break;
		case 134: speed = B134; break;
		case 150: speed = B150; break;
		case 300: speed = B300; break;
		case 600: speed = B600; break;
		case 1200: speed = B1200; break;
		case 2400: speed = B2400; break;
		case 4800: speed = B4800; break;
		case 1800: speed = B1800; break;
		case 9600: speed = B9600; break;
		case 19200: speed = EXTA; break;
		default: speed = -1; break;
	}
	return (speed);
}

cnstart(tp)
register struct tty *tp;
{
	CN_REG *cnr = CN_REG_ADDR;
	CN_CHAN *cnc;
	register int unit, c, s;

	s = spltty();
	unit = minor(tp->t_dev);
	/* If it's currently active, wait for the completion interrupt.
	 * If it's delaying or stopped, just wait for the timeout to expire
	 * or reception of start character.
	 */
	if (tp->t_state&(TS_TIMEOUT|TS_TTSTOP|TS_BUSY)) {
		goto out;
	}
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
	if (unit==1) {
		cnc = CN_CHAN_B;
	} else {
		cnc = CN_CHAN_A;
	}
again:
	if (tp->t_outq.c_cc == 0)
		goto out;
	c = getc(&tp->t_outq);
	if ((tp->t_flags & (RAW|LITOUT)) == 0) {
		if (c <= 0177) {
			c = (c | (partab[c]&0200))&0xff;
		} else {
			timeout(ttrstrt, (caddr_t)tp, (c&0177));
			tp->t_state |= TS_TIMEOUT;
			goto out;
		}
	}
	tp->t_state |= TS_BUSY;
	if (unit==1) {
		cn_soft_imr |= CNIMR_TXRDY_B;
	} else {
		cn_soft_imr |= CNIMR_TXRDY_A;
	}
	cnr->cn_imr = cn_soft_imr;
	cnc->cnc_data = c;
	wbflush();
	/* Try to use double buffering of transmitter */
	if (cnc->cnc_stat & CNSTAT_TXRDY)
		goto again;
out:
	splx(s);
}


#ifdef notdef
/*
 * Stop output to the 2681; only needed if pdbma is used. XXX
 */
cnstop(tp, flag)
register struct tty *tp;
int flag;
{
	return(0);
}
#endif


/*
 * Check for state change of carrier on port B.
 */
cncarrier()
{
	register struct tty *tp = &cons[1];
	CN_REG *cnr = CN_REG_ADDR;

	if (!(cnr->cn_inport_change & CNIP_CD_B)) {
		/* carrier present */
		if ((tp->t_state & TS_CARR_ON) == 0) {
			wakeup ((caddr_t) & tp->t_rawq);
			tp->t_state |= TS_CARR_ON;
		}
	} else {
		if ((tp->t_state & TS_CARR_ON) &&
		   (tp->t_flags & NOHANG) == 0) {
			/* carrier lost */
			if (tp->t_state & TS_ISOPEN) {
				gsignal (tp->t_pgrp, SIGHUP);
				gsignal (tp->t_pgrp, SIGCONT);
				ttyflush (tp, FREAD | FWRITE);
			}
			tp->t_state &= ~TS_CARR_ON;
		}
	}
}

cnischar()
{
	CN_CHAN *cnc = CN_CHAN_A;
	return (cnc->cnc_stat & CNSTAT_RXRDY);
}

cngetc()
{
	CN_CHAN *cnc = CN_CHAN_A;
	register d;
#ifdef ultrix
	register  s = splcons();
#else
	register  s = splextreme();
#endif ultrix

	while (!(cnc->cnc_stat & CNSTAT_RXRDY))
		wbflush();
	d = cnc->cnc_data;
	splx(s);
	return(d);
}

cnputc(c)
{
	CN_CHAN *cnc = CN_CHAN_A;
#ifdef ultrix
	register  s = splcons();
#else
	register  s = splextreme();
#endif ultrix

	while (!(cnc->cnc_stat & CNSTAT_TXRDY))
		wbflush();
	cnc->cnc_data = c;
	while (!(cnc->cnc_stat & CNSTAT_TXEMPTY))
		wbflush();
	if (c == '\n')
		cnputc('\r');
	splx(s);
}
