#ifndef lint
static char *sccsid = "@(#)bt455.c	4.2      (ULTRIX)  10/16/90";
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
 * September, 1990	Joel Gringorten
 *
 */

#include "../h/types.h"
#include "vfb03.h"
#include "../h/workstation.h"
#include "../h/inputdevice.h"
#include "../h/wsdevice.h"
#include "bt455.h"
#include "../h/fbinfo.h"
#include "../h/param.h"
#include "../h/buf.h"
#include "../io/uba/ubavar.h"

extern struct fb_info fb_softc[];
extern struct uba_device *fbinfo[];
extern struct bt455info bt455_softc[];
extern struct bt455info bt455_type[];


bt455_init_color_map(closure)
	caddr_t closure;
{
	register int i;
	register struct bt455info *bti 	= (struct bt455info *)closure;
	register volatile struct bt455 *btp 	= bti->btaddr;    

	btp->cmap_addr = 0;		wbflush();
	btp->clear_addr = 0;		wbflush();

	btp->color_map = 0; 		wbflush();
	btp->color_map = 0; 		wbflush();
	btp->color_map = 0; 		wbflush();

	btp->color_map = 0x00; 		wbflush();
	btp->color_map = 0xff; 		wbflush();
	btp->color_map = 0x00; 		wbflush();

    	for(i = 2; i <16; i++) {
		btp->color_map = 0x0;	wbflush();
		btp->color_map = 0x0;	wbflush();
		btp->color_map = 0x0;	wbflush();
	}

	bti->cursor_fg = 0xff;
	bti->cursor_bg = 0x00;
	bt455_restore_cursor_color(closure);
}


/* returns 0 if succeeded, -1 if it couldn't (index too big) */
/* shouldn't be here */

/* ARGSUSED */
int bt455_load_color_map_entry(closure, map, entry)
	caddr_t closure;
	int map;		
	register ws_color_cell *entry;
{
	return (-1);
}

void bt455_clean_colormap(closure)
	caddr_t closure;
{
	register struct bt455info *bti 		= (struct bt455info *)closure;
	register volatile struct bt455 *btp 	= bti->btaddr;
}


int bt455_video_off(closure)
	caddr_t closure;
{
	register struct bt455info *bti 		= (struct bt455info *)closure;
	register volatile struct bt455 *btp 	= bti->btaddr;    
	btp->cmap_addr = 1;		wbflush();
	btp->clear_addr = 0;		wbflush();

	btp->color_map = 0; 		wbflush();
	btp->color_map = 0; 		wbflush();
	btp->color_map = 0; 		wbflush();

	return(0);
}

int bt455_video_on(closure)
	caddr_t closure;
{
	register struct bt455info *bti		= (struct bt455info *)closure;
	register volatile struct bt455 *btp 	= bti->btaddr;    
	btp->cmap_addr = 1;		wbflush();
	btp->clear_addr = 0;		wbflush();

	btp->color_map = 0x00; 		wbflush();
	btp->color_map = 0xff; 		wbflush();
	btp->color_map = 0x00; 		wbflush();

	return(0);
}

bt455_recolor_cursor (closure, screen, fg, bg)
	caddr_t closure;
	ws_screen_descriptor *screen;
	ws_color_cell *fg, *bg;
{
	register struct bt455info *bti 	= (struct bt455info *)closure;
	bti->cursor_fg = fg->green >> 8;
	bti->cursor_bg = bg->green >> 8;
	bt455_restore_cursor_color(closure);
	return 0;
}


bt455_restore_cursor_color(closure)
	caddr_t closure;
{
    	register struct bt455info *bti 	= (struct bt455info *)closure;
	register volatile struct bt455 *btp 	= bti->btaddr;
	bti->dirty_cursormap = 1;	/* cursor needs recoloring */
	if (bti->enable_interrupt) 
		(*bti->enable_interrupt)(closure);
	else 
		bt455_clean_cursormap(bti);
	return(0);


}

bt455_clean_cursormap(closure)
	caddr_t closure;
{
    	register struct bt455info *bti 	= (struct bt455info *)closure;
	register volatile struct bt455 *btp 	= bti->btaddr;
	/* 
	 * funny hardware
	 * colormaps 8 and 9 are cursor background 
	 * overlay is cursor foreground 
	 */
	btp->cmap_addr = 8;		wbflush();
	btp->clear_addr = 0;		wbflush();

	btp->color_map = 0; 		wbflush();
	btp->color_map = bti->cursor_bg; wbflush();
	btp->color_map = 0; 		wbflush();

	btp->color_map  = 0; 		wbflush();
	btp->color_map = bti->cursor_bg; wbflush();
	btp->color_map  = 0; 		wbflush();
	btp->overlay = 0;		wbflush();
	btp->overlay = bti->cursor_fg;   wbflush();
	btp->overlay = 0;		wbflush();
	bti->dirty_cursormap = 0;
}
