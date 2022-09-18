#ifndef	lint
static	char	*sccsid	=	"@(#)ad.c	4.1	(ULTRIX)	7/2/90";
#endif 	lint

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
/*	ad.c	6.1	83/07/29	*/
/* 
 * 30-jan-87  -- rob	 fixed syntax errors around since 1.1
 */
/*
 * 18-mar-86  -- jaw     br/cvec changed to NOT use registers.
 *
 */

#include "ad.h"
#if NAD > 0 || defined(BINARY)
/*
 * Data translation AD converter interface -- Bill Reeves
 */
#include "../data/ad_data.c"
 
#define ADBUSY 01
#define ADWAITPRI (PZERO+1)

int adprobe(), adattach();
u_short adstd[] = { 0770400, 0000000, 0 };
struct uba_driver addriver =
	{ adprobe, 0, adattach, 0, adstd, "ad", addinfo, 0, 0 };

#define ADUNIT(dev) (minor(dev))

adprobe(reg)
	caddr_t reg;
{
	register struct addevice *adaddr = (struct addevice *) reg;

	adaddr->ad_csr = AD_IENABLE | AD_START;
	DELAY(40000);
	adaddr->ad_csr = 0;
	return (sizeof (struct addevice));
}

/*ARGSUSED*/
adattach(ui)
	struct uba_device *ui;
{

}

adopen(dev)
	dev_t dev;
{
	register struct ad *adp;
	register struct uba_device *ui;

	if (ADUNIT(dev) >= nNAD || (adp = &ad[ADUNIT(dev)])->ad_open ||
	    (ui = addinfo[ADUNIT(dev)]) == 0 || ui->ui_alive == 0)
		return (ENXIO);
	adp->ad_open = 1;
	adp->ad_icnt = 0;
	adp->ad_state = 0;
	adp->ad_uid = u.u_uid;
	return (0);
}

adclose(dev)
	dev_t dev;
{

	ad[ADUNIT(dev)].ad_open = 0;
	ad[ADUNIT(dev)].ad_state = 0;
}

/*ARGSUSED*/
adioctl(dev, cmd, data, flag)
	dev_t dev;
	register caddr_t data;
{
	register struct addevice *adaddr =
	    (struct addevice *) addinfo[ADUNIT(dev)]->ui_addr;
	register struct uba_device *ui = addinfo[ADUNIT(dev)];
	register struct ad *adp;
	register int i;
	short int chan;

	switch (cmd) {

	case ADIOSCHAN:
		adp = &ad[ADUNIT(dev)];
		adp->ad_chan = (*(short *)data)<<8;
		break;

	case ADIOGETW:
		adp = &ad[ADUNIT(dev)];
		spl6();
		adaddr->ad_csr = adp->ad_chan;
		i = 1000;
		while (i-- > 0 && (adaddr->ad_csr&037400) != adp->ad_chan) {
			adp->ad_loop++;
			adaddr->ad_csr = adp->ad_chan;
		}
		adp->ad_state |= ADBUSY;
		adaddr->ad_csr |= AD_IENABLE|AD_START;
		while (adp->ad_state&ADBUSY)
			sleep((caddr_t)adp, ADWAITPRI);
		spl0();
		*(short *)data = adp->ad_softdata;
		break;

	default:
		return (ENOTTY);	/* Not a legal ioctl cmd. */
	}
	return (0);
}

/*ARGSUSED*/
adintr(dev)
	dev_t dev;
{
	register struct addevice *adaddr =
			(struct addevice *) addinfo[ADUNIT(dev)]->ui_addr;
	register struct ad *adp = &ad[ADUNIT(dev)];

	adp->ad_icnt++;
	adp->ad_softcsr = adaddr->ad_csr;
	adp->ad_softdata = adaddr->ad_data;
	if(adp->ad_state&ADBUSY) {
		adp->ad_state &= ~ADBUSY;
		wakeup((caddr_t)adp);
	}
}

adreset(uban)
	int uban;
{
	register int i;
	register struct uba_device *ui;
	register struct ad *adp = ad;
	register struct addevice *adaddr;

	for(i = 0; i < nNAD; i++, adp++) {
		if((ui = addinfo[i]) == 0 || ui->ui_alive == 0 ||
				ui->ui_ubanum != uban || adp->ad_open == 0)
			continue;
		mprintf(" ad%d", i);
		if(adp->ad_state&ADBUSY == 0)
			continue;
		adaddr = (struct addevice *) ui->ui_addr;
		adaddr->ad_csr = 0;
		adaddr->ad_csr = adp->ad_chan|AD_IENABLE|AD_START;
	}
}
#endif
