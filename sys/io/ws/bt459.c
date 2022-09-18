#ifndef lint
static char *sccsid = "@(#)bt459.c	4.2      (ULTRIX)  2/28/91";
#endif lint
/************************************************************************
 *									*
 *			Copyright (c) 1989 by				*
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
 * support for Brooktree 459.
 * written by J. Gettys, from cfb original
 */
#include "../h/types.h"
#include "../h/workstation.h"
#include "../io/ws/bt459.h"
#include "../h/param.h"
#include "../h/buf.h"
#include "../io/uba/ubavar.h"

extern struct bt459info bt459_type[];


/*
 * Position the cursor to a particular location.  Note that most of the
 * work is done by the main driver, which does bounds checking on hot spots,
 * and keeps track of the cursor location.
 */

/* ARGSUSED */
bt_set_cursor_position(closure, sp, x, y)
	caddr_t closure;
	ws_screen_descriptor *sp;
	register int x, y;
{
	register struct bt459info *bti = (struct bt459info *)closure;
   	register volatile struct bt459 *btp = bti->btaddr;
	register int xt, yt;
	xt = x + bti->fb_xoffset - bti->x_hot;
	yt = y + bti->fb_yoffset - bti->y_hot;
	bt_load_reg(btp, CURSOR_X_LOW,  (xt & 0xff));
	bt_load_reg(btp, CURSOR_X_HIGH, (xt & 0x0f00) >> 8);
	bt_load_reg(btp, CURSOR_Y_LOW,  (yt & 0xff));
	bt_load_reg(btp, CURSOR_Y_HIGH, (yt & 0x0f00) >> 8);
	bt_cursor_on_off(closure, 1);  /* make sure cursor is still on */
	return(0);
}


int bt_load_cursor(closure, screen, cursor)
	caddr_t closure;
	ws_screen_descriptor *screen;
	ws_cursor_data *cursor;
{
	register struct bt459info *bti = (struct bt459info *)closure;
	bt_reformat_cursor(bti, cursor);
	bti->x_hot = cursor->x_hot;
	bti->y_hot = cursor->y_hot;
	bt_set_cursor_position(closure, screen, screen->x, screen->y);
	bti->dirty_cursor = 1;			/* cursor needs reloading */
	if (bti->enable_interrupt) 
		(*bti->enable_interrupt)(closure);
	else 
		bt_load_formatted_cursor(bti);
	return(0);
}

static unsigned char bt_lookup_table[256] = {
 0x0,0x80,0x20,0xa0, 0x8,0x88,0x28,0xa8,
 0x2,0x82,0x22,0xa2, 0xa,0x8a,0x2a,0xaa,
0x40,0xc0,0x60,0xe0,0x48,0xc8,0x68,0xe8,
0x42,0xc2,0x62,0xe2,0x4a,0xca,0x6a,0xea,
0x10,0x90,0x30,0xb0,0x18,0x98,0x38,0xb8,
0x12,0x92,0x32,0xb2,0x1a,0x9a,0x3a,0xba,
0x50,0xd0,0x70,0xf0,0x58,0xd8,0x78,0xf8,
0x52,0xd2,0x72,0xf2,0x5a,0xda,0x7a,0xfa,
 0x4,0x84,0x24,0xa4, 0xc,0x8c,0x2c,0xac,
 0x6,0x86,0x26,0xa6, 0xe,0x8e,0x2e,0xae,
0x44,0xc4,0x64,0xe4,0x4c,0xcc,0x6c,0xec,
0x46,0xc6,0x66,0xe6,0x4e,0xce,0x6e,0xee,
0x14,0x94,0x34,0xb4,0x1c,0x9c,0x3c,0xbc,
0x16,0x96,0x36,0xb6,0x1e,0x9e,0x3e,0xbe,
0x54,0xd4,0x74,0xf4,0x5c,0xdc,0x7c,0xfc,
0x56,0xd6,0x76,0xf6,0x5e,0xde,0x7e,0xfe,
 0x1,0x81,0x21,0xa1, 0x9,0x89,0x29,0xa9,
 0x3,0x83,0x23,0xa3, 0xb,0x8b,0x2b,0xab,
0x41,0xc1,0x61,0xe1,0x49,0xc9,0x69,0xe9,
0x43,0xc3,0x63,0xe3,0x4b,0xcb,0x6b,0xeb,
0x11,0x91,0x31,0xb1,0x19,0x99,0x39,0xb9,
0x13,0x93,0x33,0xb3,0x1b,0x9b,0x3b,0xbb,
0x51,0xd1,0x71,0xf1,0x59,0xd9,0x79,0xf9,
0x53,0xd3,0x73,0xf3,0x5b,0xdb,0x7b,0xfb,
 0x5,0x85,0x25,0xa5, 0xd,0x8d,0x2d,0xad,
 0x7,0x87,0x27,0xa7, 0xf,0x8f,0x2f,0xaf,
0x45,0xc5,0x65,0xe5,0x4d,0xcd,0x6d,0xed,
0x47,0xc7,0x67,0xe7,0x4f,0xcf,0x6f,0xef,
0x15,0x95,0x35,0xb5,0x1d,0x9d,0x3d,0xbd,
0x17,0x97,0x37,0xb7,0x1f,0x9f,0x3f,0xbf,
0x55,0xd5,0x75,0xf5,0x5d,0xdd,0x7d,0xfd,
0x57,0xd7,0x77,0xf7,0x5f,0xdf,0x7f,0xff,
};

static bt_reformat_cursor(bti, cursor)
	register ws_cursor_data *cursor;
	register struct bt459info *bti;
{
	register unsigned int cw, mw;
	register int i, j;
	register int nwords, shifts;
	register unsigned int mask, emask, omask;
	/* yuk, but C doesn't have good enough initialization */
	unsigned char *cbp = (unsigned char *) bti->bits;
	bzero(cbp, 1024);
	nwords = cursor->height;
	mask = 0xffffffff;
	if(cursor->width > 32) {
		nwords *= 2;
		shifts = 32 - (cursor->width - 32);
		emask = 0xffffffff;
		omask = (emask << shifts) >> shifts;
	}
	else {
		shifts = 32 - cursor->width;
		emask = omask = (mask << shifts) >> shifts;
	}
	
	for (i = 0; i < nwords; i++) {
		mask = emask;
		if (i & 1) mask = omask;
		cw = cursor->cursor[i] & mask;
		mw = cursor->mask[i] & mask;
		for (j = 0; j < 8; j++)	 {
		    *cbp++ = bt_lookup_table[((cw << 4) | (mw & 0xf)) & 0xff];
		    cw >>= 4;
		    mw >>= 4;
		}
		if (cursor->width <= 32) cbp += 8;
	}
}
/* XXX should do load check based on chip version.
/*
 * given precomputed cursor, load it.
 */
bt_load_formatted_cursor(bti)
	register struct bt459info *bti;
{
   	register volatile struct bt459 *btp = bti->btaddr;
  	register int bad_ram, counter = 0, i;
	char *cbp = (char *) bti->bits;
    
	bad_ram = 1;	/* assume cursor is bad until loaded */
	while (bad_ram && (counter++ < 25)) {
		/* write cursor data to the chip */
		btp->addr_low = CURSOR_RAM_BASE & ADDR_LOW_MASK; wbflush();
		btp->addr_high = 
			(CURSOR_RAM_BASE & ADDR_HIGH_MASK) >> 8; wbflush();
		for (i = 0; i < 1024; i++) {
			  btp->bt_reg = cbp[i];		 wbflush();
		}
		bad_ram = 0;	
		/* set bad_ram to 0 now that new data is loaded */
		btp->addr_low = CURSOR_RAM_BASE & ADDR_LOW_MASK; wbflush();
		btp->addr_high = 
			(CURSOR_RAM_BASE & ADDR_HIGH_MASK) >> 8; wbflush();
		/* read and verify each byte of cursor map */
		for (i = 0; i < 1024; i++) {
			if (btp->bt_reg != cbp[i]) {
#ifdef notdef
				printf("method1: bad byte read on verify, i = %d, counter = %d\n", i, counter);
#endif
				bad_ram = 1;
				break;
			}
		}
	}
	bti->dirty_cursor = 0;			/* no longer dirty */
}


bt_recolor_cursor (closure, screen, fg, bg)
	caddr_t closure;
	ws_screen_descriptor *screen;
	ws_color_cell *fg, *bg;
{
	register struct bt459info *bti 	= (struct bt459info *)closure;

	bti->cursor_fg = *fg;
	bti->cursor_bg = *bg;
	bt_restore_cursor_color(closure);
	return 0;
}


bt_init_color_map(closure)
	caddr_t closure;
{
	register int i;
	register struct bt459info *bti 	= (struct bt459info *)closure;
	register volatile struct bt459 *btp 	= bti->btaddr;    
    
	bt_init(closure);
	bt_init_cursor(closure);

	btp->addr_low = COLOR_MAP_BASE & ADDR_LOW_MASK; wbflush();
	btp->addr_high = 0; 				wbflush();
	btp->color_map = 0; 				wbflush();
	btp->color_map = 0; 				wbflush();
	btp->color_map = 0; 				wbflush();

    	for(i = 1; i <256; i++) {
		btp->color_map = 0xffff;		wbflush();
		btp->color_map = 0xffff;		wbflush();
		btp->color_map = 0xffff;		wbflush();
	}

	bti->cursor_fg.red = bti->cursor_fg.green = bti->cursor_fg.blue 
		= 0xffff;
	bti->cursor_bg.red = bti->cursor_bg.green = bti->cursor_bg.blue 
		= 0x0000;
	bt_restore_cursor_color(closure);

}

bt_restore_cursor_color(closure)
	caddr_t closure;
{
    	register struct bt459info *bti 	= (struct bt459info *)closure;
	register volatile struct bt459 *btp 	= bti->btaddr;

	btp->addr_low = CURSOR_COLOR_2 & ADDR_LOW_MASK;    	wbflush();
	btp->addr_high = (CURSOR_COLOR_2 & ADDR_HIGH_MASK) >> 8; wbflush();
	btp->bt_reg = bti->cursor_bg.red >> 8;			wbflush();
	btp->bt_reg = bti->cursor_bg.green >> 8;		wbflush();
	btp->bt_reg = bti->cursor_bg.blue >> 8;			wbflush();
	btp->bt_reg = bti->cursor_fg.red >> 8;			wbflush();
	btp->bt_reg = bti->cursor_fg.green >> 8;		wbflush();
	btp->bt_reg = bti->cursor_fg.blue >> 8;			wbflush();

}

bt_init_cursor(closure)
	caddr_t closure;
{
	register struct bt459info *bti 	= (struct bt459info *)closure;
	register volatile struct bt459 *btp 	= bti->btaddr;    
	register int i;

	btp->addr_low = 0x00; 		wbflush();
	btp->addr_high = 0x04; 		wbflush();

	for (i = 0; i < 1024; i++) {
		btp->bt_reg = 0x0;		/* clear cursor on chip */
		wbflush();
	}
}

struct bt_init_struct {
	short reg;
	short data;
} bt_init_data[] = {
	CMD_REG_0, 0x40,
	CMD_REG_1, 0x00,
	CMD_REG_2, 0xc2,
	PIXEL_READ_MASK, 0xff,
	PIXEL_BLINK_MASK, 0x00,
	OVERLAY_READ_MASK, 0x00,
	OVERLAY_BLINK_MASK, 0x00,
	INTERLEAVE_REG, 0x00,
	TEST_REG, 0x00,
	CURSOR_CMD_REG, 0xc0,
	CURSOR_X_LOW, 0x00,
	CURSOR_X_HIGH, 0x00,
	CURSOR_Y_LOW, 0x00,
	CURSOR_Y_HIGH, 0x00,
	WINDOW_X_LOW, 0x00,
	WINDOW_X_HIGH, 0x00,
	WINDOW_Y_LOW, 0x00,
	WINDOW_Y_HIGH, 0x00,
	WINDOW_WIDTH_LOW, 0x00,
	WINDOW_WIDTH_HIGH, 0x00,
	WINDOW_HEIGHT_LOW, 0x00,
	WINDOW_HEIGHT_HIGH, 0x00
};

bt_init(closure)
	caddr_t closure;
{
	register struct bt459info *bti 		= (struct bt459info *)closure;
	register volatile struct bt459 *btp 	= bti->btaddr;    
	register int ncmds = sizeof(bt_init_data) 
			/ sizeof (struct bt_init_struct);
	register int i;
	for (i = 0; i < ncmds;	i++)
		bt_load_reg (btp, bt_init_data[i].reg, bt_init_data[i].data);
}

/* returns 0 if succeeded, -1 if it couldn't (index too big) */
/* ARGSUSED */
int bt_load_color_map_entry(closure, map, entry)
	caddr_t closure;
	int map;		/* not used; only single map in this device */
	register ws_color_cell *entry;
{
	register struct bt459info *bti 		= (struct bt459info *)closure;
	register volatile struct bt459 *btp 	= bti->btaddr;
	register int index = entry->index;
	int s;
	
	if(index >= 256) 
		return -1;
	s = spltty();
	bti->cells[index].red   = entry->red   >> 8;
	bti->cells[index].green = entry->green >> 8;
	bti->cells[index].blue  = entry->blue  >> 8;
	bti->cells[index].dirty_cell = 1;
	if (index < bti->min_dirty) bti->min_dirty = index;
	if (index > bti->max_dirty) bti->max_dirty = index;
	bti->dirty_colormap = 1;
	splx(s);
	if (bti->enable_interrupt) 
		(*bti->enable_interrupt)(closure);
	else 
		bt_clean_colormap(bti);
	return 0;
}

void bt_clean_colormap(closure)
	caddr_t closure;
{
	register struct bt459info *bti 		= (struct bt459info *)closure;
	register volatile struct bt459 *btp 	= bti->btaddr;
	register int i;
	register struct bt459_color_cell *entry;

	for (i = bti->min_dirty; i <= bti->max_dirty ; i++) {
		entry = &bti->cells[i];
		if (entry->dirty_cell) {
			if (bti->screen_on == 0 && i == 0)
			    continue;	/* skip cmap[0] if screen saver on */
			entry->dirty_cell = 0;
			btp->addr_low = i;		wbflush();
			btp->addr_high = 0; 		wbflush();
			btp->color_map = entry->red; 	wbflush();
			btp->color_map = entry->green; 	wbflush();
			btp->color_map = entry->blue; 	wbflush();
		}
	}
	if(bti->screen_on == 0 && bti->cells[0].dirty_cell)
		bti->min_dirty = 0;	/* cmap[0] is left dirty */
	else {
		bti->min_dirty = 256;
		bti->dirty_colormap = 0;
	}
	bti->max_dirty = 0;
}

int bt_video_off(closure)
	caddr_t closure;
{
	register struct bt459info *bti 		= (struct bt459info *)closure;
	register volatile struct bt459 *btp 	= bti->btaddr;    

	btp->addr_low  = COLOR_MAP_BASE; 	wbflush();
	btp->addr_high = 0; 			wbflush();
	bti->saved_entry.red   = btp->color_map;
	bti->saved_entry.green = btp->color_map;
	bti->saved_entry.blue  = btp->color_map;
	btp->addr_low = COLOR_MAP_BASE;		wbflush();
	btp->addr_high = 0; 			wbflush();
	btp->color_map = 0; 			wbflush();
	btp->color_map = 0; 			wbflush();
	btp->color_map = 0; 			wbflush();
	bt_load_reg(btp, PIXEL_READ_MASK, 0);
	bt_load_reg(btp, CURSOR_CMD_REG, 0);		/* turn off cursor */
	bti->screen_on = 0;
	return(0);
}

int bt_video_on(closure)
	caddr_t closure;
{
	register struct bt459info *bti		= (struct bt459info *)closure;
	register volatile struct bt459 *btp 	= bti->btaddr;    

	btp->addr_low  = COLOR_MAP_BASE; 		wbflush();
	btp->addr_high = 0; 				wbflush();
	btp->color_map = bti->saved_entry.red;		wbflush();
	btp->color_map = bti->saved_entry.green;	wbflush();
	btp->color_map = bti->saved_entry.blue; 	wbflush();
	bt_load_reg(btp, PIXEL_READ_MASK, 0xff);
	if (bti->on_off) 
		bt_load_reg(btp, CURSOR_CMD_REG, 0xc0);	/* turn on cursor */
	bti->screen_on = 1;
	if (bti->dirty_colormap)
		if(bti->enable_interrupt) 
			(*bti->enable_interrupt)(closure);
		else
			bt_clean_colormap(bti);
	return(0);
}

bt_cursor_on_off(closure, on_off)
	caddr_t closure;
	int on_off;
{
	register struct bt459info *bti		= (struct bt459info *)closure;
	register volatile struct bt459 *btp 	= bti->btaddr;    

	if (on_off) bt_load_reg(btp, CURSOR_CMD_REG, 0xc0);/* turn on cursor */
	else 	   bt_load_reg(btp, CURSOR_CMD_REG, 0x00);/* turn off cursor */
	bti->on_off = on_off;
	return(0);
}
		
bt_load_reg(btp, reg, val)
	register volatile struct bt459 *btp;
	u_short reg;
	u_char val;
{
	btp->addr_low  = (reg & ADDR_LOW_MASK);		wbflush();
	btp->addr_high = (reg & ADDR_HIGH_MASK) >> 8;	wbflush();
	btp->bt_reg = val;				wbflush();
}

