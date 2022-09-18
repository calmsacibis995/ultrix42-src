#ifndef lint
static char *sccsid = "@(#)pmagaa.c	4.2      (ULTRIX)  10/16/90";
#endif lint

/************************************************************************
 *                                                                      *
 *                      Copyright (c) 1990 by                           *
 *              Digital Equipment Corporation, Maynard, MA              *
 *                      All rights reserved.                            *
 *                                                                      *
 *   This software is furnished under a license and may be used and     *
 *   copied  only  in accordance with the terms of such license and     *
 *   with the  inclusion  of  the  above  copyright  notice.   This     *
 *   software  or  any  other copies thereof may not be provided or     *
 *   otherwise made available to any other person.  No title to and     *
 *   ownership of the software is hereby transferred.                   *
 *                                                                      *
 *   The information in this software is subject to change  without     *
 *   notice  and should not be construed as a commitment by Digital     *
 *   Equipment Corporation.                                             *
 *                                                                      *
 *   Digital assumes no responsibility for the use  or  reliability     *
 *   of its software on equipment which is not supplied by Digital.     *
 *                                                                      *
 ************************************************************************/

/*
 * Modification History
 *	31-Aug-90 --  Joel Gringorten  
 *
 *	Created from vfb03.
 */

#include "../h/types.h"
#include "pmagaa.h"
#include "../h/workstation.h"
#include "../h/inputdevice.h"
#include "../h/wsdevice.h"
#include "bt455.h"
#include "bt431.h"
#include "../h/fbinfo.h"
#include "../h/param.h"
#include "../h/buf.h"
#include "../io/uba/ubavar.h"

extern struct fb_info fb_softc[];
extern struct uba_device *fbinfo[];
extern struct bt455info bt455_softc[];
extern struct bt455info bt455_type[];
extern struct bt431info bt431_softc[];
extern struct bt431info bt431_type[];


/* all we need really do is install interrupt enable functions */
int pmagaa_attach(ui)
        register struct uba_device *ui;
{
        register struct fb_info *fbp = &fb_softc[ui->ui_unit];
        /* we know we have a bt455 and bt431 */
        register struct bt455info *bti_455 = (struct bt455info *)fbp->cmf.cmc;
        register struct bt431info *bti_431 = (struct bt431info *)fbp->cf.cc;
        bti_455->enable_interrupt = pmagaa_bt455_enable_interrupt;
	bti_431->enable_interrupt = pmagaa_bt431_enable_interrupt;
}

void pmagaa_interrupt(ui, closure)
        register struct uba_device *ui;
        caddr_t closure;
{
	static int junk;
        struct fb_info *fbp = (struct fb_info *) closure;
        struct bt455info *bti_455 = (struct bt455info *)fbp->cmf.cmc;
        struct bt431info *bti_431 = (struct bt431info *)fbp->cf.cc;
	junk =  *(ui->ui_addr +  PMAGAA_IREQ_OFFSET); /* clear */
        *(ui->ui_addr + PMAGAA_IREQ_OFFSET) = 0; /* disable interrupt */
        if (bti_431->dirty_cursor) bt431_load_formatted_cursor(bti_431);
        if (bti_455->dirty_colormap) bt455_clean_colormap(bti_455);
	if (bti_455->dirty_cursormap) bt455_clean_cursormap(bti_455);	
        tc_disable_option(fbinfo[fbp - fb_softc]);
}

void pmagaa_bt455_enable_interrupt(closure)
        caddr_t closure;
{
        register struct bt455info *bti = (struct bt455info *) closure;
        register int unit = bti - bt455_softc;
        register struct uba_device *ui = fbinfo[unit];
	static junk;
	*(ui->ui_addr +  PMAGAA_IREQ_OFFSET) = 0;    /* disable intr */
        junk = *(ui->ui_addr +  PMAGAA_IREQ_OFFSET); /* clear int */
	 *(ui->ui_addr +  PMAGAA_IREQ_OFFSET) = 1;  /* enable intr */
        tc_enable_option(fbinfo[unit]);
}
void pmagaa_bt431_enable_interrupt(closure)
        caddr_t closure;
{
        register struct bt431info *bti = (struct bt431info *) closure;
        register int unit = bti - bt431_softc;
        register struct uba_device *ui = fbinfo[unit];
	static junk;
	*(ui->ui_addr +  PMAGAA_IREQ_OFFSET) = 0;    /* disable intr */
        junk = *(ui->ui_addr +  PMAGAA_IREQ_OFFSET); /* clear int */
	 *(ui->ui_addr +  PMAGAA_IREQ_OFFSET) = 1;  /* enable intr */
        tc_enable_option(fbinfo[unit]);
}

caddr_t pmagaa_bt431_init_closure(closure, address, unit, type)
        caddr_t closure;
        caddr_t address;
        int unit;
        int type;
{
        struct bt431info *bp = (struct bt431info *)closure;
        bp = bp + unit;                 /* set to correct unit */
        *bp = bt431_type[type];         /* set to initial values */
        bp->btaddr = (struct bt431 *)(address +
                        (int)bt431_type[type].btaddr);
	bp->cmap_closure = (caddr_t)&bt455_softc[unit];
        return (caddr_t)bp;
}


caddr_t pmagaa_bt455_init_closure(closure, address, unit, type)
	caddr_t closure;
	caddr_t address;
	int unit;
	int type;
{
        struct bt455info *bp = (struct bt455info *)closure;
        bp = bp + unit;                 /* set to correct unit */
        *bp = bt455_type[type];         /* set to initial values */
        bp->btaddr = (struct bt455 *)(address +
                        (int)bt455_type[type].btaddr);
	bp->cursor_closure =  (caddr_t)&bt431_softc[unit];
        return (caddr_t)bp;
}

pmagaa_recolor_cursor(closure, screen, fg, bg)
	caddr_t closure;
	ws_screen_descriptor *screen;
	ws_color_cell *fg, *bg;
{
	register struct bt431info *btii	= (struct bt431info *)closure;
	return (bt455_recolor_cursor((struct bt455info *) btii->cmap_closure,
		 screen, fg, bg));
}

pmagaa_video_off(closure)
	caddr_t closure;
{
   	register struct bt455info *btii = (struct bt455info *)closure;	
    	register struct bt431info *bti = (struct bt431info *)
						 btii->cursor_closure;
	bt455_video_off(closure);
	if(bti->on_off) {
	    bt431_cursor_on_off(bti, 0);
	    bti->cursor_was_on = 1;
	}
}

pmagaa_video_on(closure)
	caddr_t closure;
{
	register struct bt455info *btii	= (struct bt455info *)closure;
	register struct bt431info *bti = (struct bt431info *)
						 btii->cursor_closure;
	bt455_video_on(closure);
	if(bti->cursor_was_on) {
	    bt431_cursor_on_off(bti, 1);
	    bti->cursor_was_on = 0;
	}
}

