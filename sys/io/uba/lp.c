#ifndef lint
static char *sccsid = "@(#)lp.c	4.2	(ULTRIX)	11/9/90";
#endif lint
/************************************************************************
 *									*
 *			Copyright (c) 1985 by				*
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
/*	lp.c	6.1	84/07/11  LP11 driver	*/

#include "lp.h"
#if NLP > 0 || defined(BINARY)
/*
 * LP-11 Line printer driver (LN01)
 *
 * This driver has been modified to work on printers where
 * leaving IENABLE set would cause continuous interrupts.
 *
 * 27-July-89 -- Giles Atkinson
 *	Add SPL macro to allow compilation for DECsystem 5400.
 *
 * 04-sept-87 -- jhw
 *      Moved raw flag to lp_data.c so customer can redefine it.
 *
 *	The LN01 can be down loaded with different fonts.
 *	this capability along with the fact that the fonts
 *	are not monospaced makes keeping track of a fixed
 *	number of lines or columns a job for the filter.
 *	Because the font widths are not known at this the 
 *	driver level. 
 * 18-mar-86  -- jaw     br/cvec changed to NOT use registers.
 *
 */

#include "../data/lp_data.c"

#ifdef vax
#define SPL spl4
#else mips
#define SPL spltty
#endif

#define	LPPRI	(PZERO+8)
#define	IENABLE	0100
#define	DONE	0200
#define	ERROR	0100000
#define	LPLWAT	650
#define	LPHWAT	800

#define MAXCOL	132
#define CAP	1

#define LPUNIT(dev) (minor(dev) >> 3)

extern int LPRAW;

struct lpdevice {
	short	lpsr;
	short	lpbuf;
};



int lpprobe(), lpattach(), lptout();
u_short lpstd[] = { 0177514 };
struct uba_driver lpdriver =
	{ lpprobe, 0, lpattach, 0, lpstd, "lp", lpinfo };

/* bits for state */
#define	OPEN		1	/* device is open */
#define	TOUT		2	/* timeout is active */
#define	MOD		4	/* device state has been modified */
#define	ASLP		8	/* awaiting draining of printer */
/********************************************************/
/*	added for escape sequence handling ln01		*/
/*							*/
#define ESC		'\033'  /* escape sequence introducer */
#define UCP		'\120'  /* uppercase P */
#define BSLH		'\134'  /* backslash  \ */
#define escend(x)	((x!='\120')&&(x!='\133')&&(x>='\100')&&(x<='\176'))
int	last=0;
int	escflg;	/* escape sequence flag = 0 not in  progress */
		/*                        1 escape char recieved*/
		/*
/********************************************************/
int	lptout();

lpattach(ui)
	struct uba_device *ui;
{
	register struct lp_softc *sc;

	sc = &lp_softc[ui->ui_unit];
	sc->sc_lpchar = -1;
	if (ui->ui_flags)
		sc->sc_maxcol = ui->ui_flags;
	else
		sc->sc_maxcol = MAXCOL;
}

lpprobe(reg)
	caddr_t reg;
{
	register struct lpdevice *lpaddr = (struct lpdevice *)reg;
#ifdef lint
	lpintr(0);
#endif

	lpaddr->lpsr = IENABLE;
	DELAY(5);
	lpaddr->lpsr = 0;
	return (sizeof (struct lpdevice));
}

/*ARGSUSED*/
lpopen(dev, flag)
	dev_t dev;
	int flag;
{
	register int unit;
	register struct lpdevice *lpaddr;
	register struct lp_softc *sc;
	register struct uba_device *ui;
	escflg=0;
	last=0;
	if ((unit = LPUNIT(dev)) >= nNLP ||
	    (sc = &lp_softc[unit])->sc_state&OPEN ||
	    (ui = lpinfo[unit]) == 0 || ui->ui_alive == 0)
		return (ENXIO);
	lpaddr = (struct lpdevice *)ui->ui_addr;
	if (lpaddr->lpsr&ERROR)
		return (EIO);
	sc->sc_state |= OPEN;
	sc->sc_inbuf = geteblk(512);
	sc->sc_flags = minor(dev) & 07;
	(void) SPL();
	if ((sc->sc_state&TOUT) == 0) {
		sc->sc_state |= TOUT;
		timeout(lptout, (caddr_t)dev, 10*hz);
	}
	(void) spl0();
	return (0);
}

/*ARGSUSED*/
lpclose(dev, flag)
	dev_t dev;
	int flag;
{
	register struct lp_softc *sc = &lp_softc[LPUNIT(dev)];
	escflg=0;
	last=0;
	brelse(sc->sc_inbuf);
	sc->sc_state &= ~OPEN;
}

lpwrite(dev, uio)
	dev_t dev;
	struct uio *uio;
{
	register unsigned n;
	register char *cp;
	register struct lp_softc *sc = &lp_softc[LPUNIT(dev)];
	int error;

	while (n = min(512, (unsigned)uio->uio_resid)) {
		cp = sc->sc_inbuf->b_un.b_addr;
		error = uiomove(cp, (int)n, UIO_WRITE, uio);
		if (error)
			return (error);
		do
		if(LPRAW)
			lpoutput(dev, *cp++);
		else
			lpcanon(dev, *cp++);
		while (--n);
	}
	return (0);
}

lpcanon(dev, c)
	dev_t dev;
	register int c;
{
	register int logcol, physcol;
	register struct lp_softc *sc = &lp_softc[LPUNIT(dev)];

	if (sc->sc_flags&CAP) {
		register int c2;

		if (c>='a' && c<='z')
			c += 'A'-'a'; else
		switch (c) {

		case '{':
			c2 = '(';
			goto esc;

		case '}':
			c2 = ')';
			goto esc;

		case '`':
			c2 = '\'';
			goto esc;

		case '|':
			c2 = '!';
			goto esc;

		case '~':
			c2 = '^';

		esc:
			lpcanon(dev, c2);
			sc->sc_logcol--;
			c = '-';
		}
	}
logcol = sc->sc_logcol;
physcol = sc->sc_physcol;
/*******			turn on escape sequece flag if needed */
/*			        escflg = 0 => no escape sequence in progress*/
/*				         1 => escape sequence in process   */
if (((escflg==0) && (c == ESC)) || escflg)
	{
	eschdl(dev,c);		/* go handle escape sequence */
	logcol=0;		/* if using escseq then forget */
	physcol=0;		/* line formatting		*/
	}
else	{
	switch(c) {
				case '\t':
					logcol = (logcol+8) & ~7;
					break;

				case '\f':
					if(sc->sc_physline == 0 && physcol == 0)
						break;
				/* fall into ... */

				case '\n':
					lpoutput(dev, c);
					if (c == '\f')
						sc->sc_physline = 0;
					else
						sc->sc_physline++;
					physcol = 0;
				/* fall into ... */

				case '\r':
					logcol = 0;
					(void) SPL();
					lpintr(LPUNIT(dev));
					(void) spl0();
					break;

				case '\b':
					if (logcol > 0)
						logcol--;
					break;

				default:
					if (logcol < physcol) {
						lpoutput(dev, '\r');
						physcol = 0;
					}
					if (logcol < sc->sc_maxcol) {
						while (logcol > physcol) {
							lpoutput(dev, ' ');
							physcol++;
							}
						lpoutput(dev, c);
						physcol++;
						logcol++;
						}
					}
	if (logcol > sc->sc_maxcol)	/* ignore long lines  */
		logcol = sc->sc_maxcol;
	sc->sc_logcol = logcol;
	sc->sc_physcol = physcol;
			}
}

lpoutput(dev, c)
	dev_t dev;
	int c;
{
	register struct lp_softc *sc = &lp_softc[LPUNIT(dev)];

	if (sc->sc_outq.c_cc >= LPHWAT) {
		(void) SPL();
		lpintr(LPUNIT(dev));				/* unchoke */
		while (sc->sc_outq.c_cc >= LPHWAT) {
			sc->sc_state |= ASLP;		/* must be ERROR */
			sleep((caddr_t)sc, LPPRI);
		}
		(void) spl0();
	}
	while (putc(c, &sc->sc_outq))
		sleep((caddr_t)&lbolt, LPPRI);
}

lpintr(lp11)
	int lp11;
{
	register int n;
	register struct lp_softc *sc = &lp_softc[lp11];
	register struct uba_device *ui = lpinfo[lp11];
	register struct lpdevice *lpaddr = (struct lpdevice *)ui->ui_addr;

	lpaddr->lpsr &= ~IENABLE;
	n = sc->sc_outq.c_cc;
	if (sc->sc_lpchar < 0)
		sc->sc_lpchar = getc(&sc->sc_outq);
	while ((lpaddr->lpsr&DONE) && sc->sc_lpchar >= 0) {
		lpaddr->lpbuf = sc->sc_lpchar;
		sc->sc_lpchar = getc(&sc->sc_outq);
	}
	sc->sc_state |= MOD;
	if(((sc->sc_outq.c_cc > 0)||(sc->sc_lpchar >= 0))&&(lpaddr->lpsr&ERROR)==0)
		lpaddr->lpsr |= IENABLE;	/* ok and more to do later */
	if (n>LPLWAT && sc->sc_outq.c_cc<=LPLWAT && sc->sc_state&ASLP) {
		sc->sc_state &= ~ASLP;
		wakeup((caddr_t)sc);		/* top half should go on */
	}
}

lptout(dev)
	dev_t dev;
{
	register struct lp_softc *sc;
	register struct uba_device *ui;
	register struct lpdevice *lpaddr;

	sc = &lp_softc[LPUNIT(dev)];
	ui = lpinfo[LPUNIT(dev)];
	lpaddr = (struct lpdevice *) ui->ui_addr;
	if ((sc->sc_state&MOD) != 0) {
		sc->sc_state &= ~MOD;		/* something happened */
		timeout(lptout, (caddr_t)dev, 2*hz);	/* so don't sweat */
		return;
	}
	if ((sc->sc_state&OPEN) == 0 && sc->sc_outq.c_cc == 0) {
		sc->sc_state &= ~TOUT;		/* no longer open */
		lpaddr->lpsr = 0;
		return;
	}
	if (sc->sc_outq.c_cc && (lpaddr->lpsr&DONE) && (lpaddr->lpsr&ERROR)==0)
		lpintr(LPUNIT(dev));			/* ready to go */
	timeout(lptout, (caddr_t)dev, 10*hz);
}

lpreset(uban)
	int uban;
{
	register struct uba_device *ui;
	register struct lpdevice *lpaddr;
	register int unit;

	for (unit = 0; unit < nNLP; unit++) {
		if ((ui = lpinfo[unit]) == 0 || ui->ui_ubanum != uban ||
		    ui->ui_alive == 0)
			continue;
		printf(" lp%d", unit);
		lpaddr = (struct lpdevice *)ui->ui_addr;
		lpaddr->lpsr |= IENABLE;
	}
}
/****************************************************************/
/*								*/
/*	eschdl - escape sequence handler			*/
/*								*/
/*      This routine intercepts escape sequences for the purpose*/
/*	of pass through.					*/
/*								*/
/****************************************************************/
eschdl(dev,c)
dev_t dev;
int c;
{
if(escflg==0)
	{		/* set escflg=1 => ready to receive 2nd seqchar*/
	escflg=1;
	}
else	switch(escflg)
		{
		case 1:		/* second character of escseq 		*/
  			if (c==UCP)
				{
				escflg=2;  /*ctrl string pass though mode=8 */
				last=c;
				}
			else escflg=3;  /* set escape seq pass through mode*/
			break;
		case 2:		/* ctrl string pass through mode       	*/
			if((last==ESC) && (c==BSLH))
				{
				escflg=0;
				last=0;
				}
			else last=c;	/* save it for next pass */
			break;
		case 3:
			if(escend(c))
				escflg=0;/* turn off esc handler if at end  */

		}
lpoutput(dev,c);
return(0);
}
#endif
