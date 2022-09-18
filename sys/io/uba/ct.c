
#ifndef lint
static char *sccsid = "@(#)ct.c	4.1	(ULTRIX)	7/2/90";
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
/*	ct.c	6.1	83/07/29	*/
/*
 * 18-mar-86  -- jaw     br/cvec changed to NOT use registers.
 *
 */

#include "ct.h"
#if NCT > 0 || defined(BINARY)
/*
 * GP DR11C driver used for C/A/T
 *
 * BUGS:
 *	This driver hasn't been tested in 4.1bsd or 4.2bsd
 */

#include "../data/ct_data.c"

#define	PCAT	(PZERO+9)
#define	CATHIWAT	100
#define	CATLOWAT	30

struct ctdevice {
	short	ctcsr;
	short	ctbuf;
};

int	ctprobe(), ctattach(), ctintr();
u_short	ctstd[] = { 0 };
struct	uba_driver ctdriver = 
    { ctprobe, 0, ctattach, 0, ctstd, "ct", ctdinfo };

#define	CTUNIT(dev)	(minor(dev))

ctprobe(reg)
	caddr_t reg;
{
	register struct ctdevice *ctaddr = (struct ctdevice *)reg;

#ifdef lint
	ctintr(0);
#endif
	ctaddr->ctcsr = IENABLE;
	DELAY(10000);
	ctaddr->ctcsr = 0;
	return (sizeof (struct ctdevice));
}

/*ARGSUSED*/
ctattach(ui)
	register struct uba_device *ui;
{

}

ctopen(dev)
	dev_t dev;
{
	register struct ct_softc *sc;
	register struct uba_device *ui;
	register struct ctdevice *ctaddr;

	if (CTUNIT(dev) >= nNCT || (ui = ctdinfo[CTUNIT(dev)]) == 0 ||
	    ui->ui_alive == 0 || (sc = &ct_softc[CTUNIT(dev)])->sc_openf)
		return (ENXIO);
	sc->sc_openf = 1;
	ctaddr->ctcsr |= IENABLE;
	return (0);
}

ctclose(dev)
	dev_t dev;
{

	ct_softc[CTUNIT(dev)].sc_openf = 0;
	ctintr(dev);
}

ctwrite(dev, uio)
	dev_t dev;
	struct uio *uio;
{
	register struct ct_softc *sc = &ct_softc[CTUNIT(dev)];
	register int c;

	while ((c=cupass(uio)) >= 0) {
		(void) spl5();
		while (sc->sc_oq.c_cc > CATHIWAT)
			sleep((caddr_t)&sc->sc_oq, PCAT);
		while (putc(c, &sc->sc_oq) < 0)
			sleep((caddr_t)&lbolt, PCAT);
		ctintr(dev);
		(void) spl0();
	}
}

ctintr(dev)
	dev_t dev;
{
	register int c;
	register struct ct_softc *sc = &ct_softc[CTUNIT(dev)];
	register struct ctdevice *ctaddr =
	    (struct ctdevice *)ctdinfo[CTUNIT(dev)]->ui_addr;

	if (ctaddr->ctcsr&DONE) {
		if ((c = getc(&sc->sc_oq)) >= 0) {
			ctaddr->ctbuf = c;
			if (sc->sc_oq.c_cc==0 || sc->sc_oq.c_cc==CATLOWAT)
				wakeup(&sc->sc_oq);
		} else {
			if (sc->sc_openf==0)
				ctaddr->ctcsr = 0;
		}
	}

}
#endif
