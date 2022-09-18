#ifndef lint
static char *sccsid = "@(#)vfb03.c	4.1      (ULTRIX)  8/13/90";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1990 by				*
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
 *   The information in this software is subject to change  without	*
 *   notice  and should not be construed as a commitment by Digital	*
 *   Equipment Corporation.						*
 *									*
 *   Digital assumes no responsibility for the use  or  reliability	*
 *   of its software on equipment which is not supplied by Digital.	*
 *									*
 ************************************************************************/

/*
 * Modification History
 *
 * November, 1989	Jim Gettys
 *
 *		Complete rewrite for multiscreen support.  Boy how kludges
 *		accumulate over time....
 *
 */
#include "../h/types.h"
#include "vfb03.h"
#include "../h/workstation.h"
#include "../h/inputdevice.h"
#include "../h/wsdevice.h"
#include "bt459.h"
#include "../h/fbinfo.h"
#include "../h/param.h"
#include "../h/buf.h"
#include "../io/uba/ubavar.h"

extern struct fb_info fb_softc[];
extern struct uba_device *fbinfo[];
extern struct bt459info bt459_softc[];
extern struct bt459info bt459_type[];

void cfb_enable_interrupt();
/* all we need really do is install an interrupt enable function */
int vfb03_attach(ui)
	register struct uba_device *ui;
{
        register struct fb_info *fbp = &fb_softc[ui->ui_unit];
	/* we know we have a bt459 */
	register struct bt459info *bti = (struct bt459info *)fbp->cf.cc;
	bti->enable_interrupt = vfb03_enable_interrupt;
}

void vfb03_interrupt(ui, closure)
	register struct uba_device *ui;
	caddr_t closure;
{
	struct fb_info *fbp = (struct fb_info *) closure;
	struct bt459info *bti = (struct bt459info *)fbp->cf.cc;
	*(ui->ui_addr + VFB03_IREQ_OFFSET) = 0;	/* clear the interrupt */
	if (bti->dirty_cursor) bt_load_formatted_cursor(bti);
	if (bti->dirty_colormap) bt_clean_colormap(bti);
	tc_disable_option(fbinfo[fbp - fb_softc]);
}

void vfb03_enable_interrupt(closure)
	caddr_t closure;
{
	register struct bt459info *bti = (struct bt459info *) closure;
	register int unit = bti - bt459_softc;
	register struct uba_device *ui = fbinfo[unit];
	*(ui->ui_addr + VFB03_IREQ_OFFSET) = 0;	/* clear the interrupt */
	tc_enable_option(fbinfo[unit]);
}

caddr_t vfb03_bt_init_closure(closure, address, unit, type)
	caddr_t closure;
	caddr_t address;
	int unit;
	int type;
{
	struct bt459info *bp = (struct bt459info *)closure;
	bp = bp + unit;			/* set to correct unit */
	*bp = bt459_type[type];		/* set to initial values */
	bp->btaddr = (struct bt459 *)(address + 
			(int)bt459_type[type].btaddr);
	return (caddr_t)bp;
}

