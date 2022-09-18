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
#include "../h/workstation.h"
#include "bt431.h"
#include "../h/param.h"
#include "../h/buf.h"
#include "../io/uba/ubavar.h"

bt431_set_cursor_position(closure, sp, x, y)
	caddr_t closure;
	ws_screen_descriptor *sp;
	register int x, y;
{
	register struct bt431info *bti = (struct bt431info *)closure;
   	register volatile struct bt431 *btp = bti->btaddr;
	register int xt, yt;
	xt = x + bti->fb_xoffset - bti->x_hot;
	yt = y + bti->fb_yoffset - bti->y_hot;

	btp->addr_low =  DUP431_B0(bt431_CUR_XLO); 	wbflush();
	btp->addr_high	= 0;		wbflush();
	
	btp->control =  DUP431_B0(xt & 0xff);
	btp->control =  DUP431_B0((xt & 0xff00) >> 8);
	btp->control =  DUP431_B0(yt & 0xff);
	btp->control =  DUP431_B0((yt & 0xff00) >> 8);

	return(0);
}


static unsigned char flip[256] = {
0x0,  0x80,  0x40,  0xc0,  0x20,  0xa0,  0x60,  0xe0,
0x10,  0x90,  0x50,  0xd0,  0x30,  0xb0,  0x70,  0xf0,
0x8,  0x88,  0x48,  0xc8,  0x28,  0xa8,  0x68,  0xe8,
0x18,  0x98,  0x58,  0xd8,  0x38,  0xb8,  0x78,  0xf8,
0x4,  0x84,  0x44,  0xc4,  0x24,  0xa4,  0x64,  0xe4,
0x14,  0x94,  0x54,  0xd4,  0x34,  0xb4,  0x74,  0xf4,
0xc,  0x8c,  0x4c,  0xcc,  0x2c,  0xac,  0x6c,  0xec,
0x1c,  0x9c,  0x5c,  0xdc,  0x3c,  0xbc,  0x7c,  0xfc,
0x2,  0x82,  0x42,  0xc2,  0x22,  0xa2,  0x62,  0xe2,
0x12,  0x92,  0x52,  0xd2,  0x32,  0xb2,  0x72,  0xf2,
0xa,  0x8a,  0x4a,  0xca,  0x2a,  0xaa,  0x6a,  0xea,
0x1a,  0x9a,  0x5a,  0xda,  0x3a,  0xba,  0x7a,  0xfa,
0x6,  0x86,  0x46,  0xc6,  0x26,  0xa6,  0x66,  0xe6,
0x16,  0x96,  0x56,  0xd6,  0x36,  0xb6,  0x76,  0xf6,
0xe,  0x8e,  0x4e,  0xce,  0x2e,  0xae,  0x6e,  0xee,
0x1e,  0x9e,  0x5e,  0xde,  0x3e,  0xbe,  0x7e,  0xfe,
0x1,  0x81,  0x41,  0xc1,  0x21,  0xa1,  0x61,  0xe1,
0x11,  0x91,  0x51,  0xd1,  0x31,  0xb1,  0x71,  0xf1,
0x9,  0x89,  0x49,  0xc9,  0x29,  0xa9,  0x69,  0xe9,
0x19,  0x99,  0x59,  0xd9,  0x39,  0xb9,  0x79,  0xf9,
0x5,  0x85,  0x45,  0xc5,  0x25,  0xa5,  0x65,  0xe5,
0x15,  0x95,  0x55,  0xd5,  0x35,  0xb5,  0x75,  0xf5,
0xd,  0x8d,  0x4d,  0xcd,  0x2d,  0xad,  0x6d,  0xed,
0x1d,  0x9d,  0x5d,  0xdd,  0x3d,  0xbd,  0x7d,  0xfd,
0x3,  0x83,  0x43,  0xc3,  0x23,  0xa3,  0x63,  0xe3,
0x13,  0x93,  0x53,  0xd3,  0x33,  0xb3,  0x73,  0xf3,
0xb,  0x8b,  0x4b,  0xcb,  0x2b,  0xab,  0x6b,  0xeb,
0x1b,  0x9b,  0x5b,  0xdb,  0x3b,  0xbb,  0x7b,  0xfb,
0x7,  0x87,  0x47,  0xc7,  0x27,  0xa7,  0x67,  0xe7,
0x17,  0x97,  0x57,  0xd7,  0x37,  0xb7,  0x77,  0xf7,
0xf,  0x8f,  0x4f,  0xcf,  0x2f,  0xaf,  0x6f,  0xef,
0x1f,  0x9f,  0x5f,  0xdf,  0x3f,  0xbf,  0x7f,  0xff,
};



int bt431_load_cursor(closure, screen, cursor)
	caddr_t closure;
	ws_screen_descriptor *screen;
	ws_cursor_data *cursor;
{
	register struct bt431info *bti = (struct bt431info *)closure;
	if(!bti->inited) {
	    bti->inited = 1;
	    bt431_init(closure);
	}
	bt431_reformat_cursor(bti, cursor);
	bti->x_hot = cursor->x_hot;
	bti->y_hot = cursor->y_hot;
	bt431_set_cursor_position(closure, screen, screen->x, screen->y);
	bti->dirty_cursor = 1;			/* cursor needs reloading */
	if (bti->enable_interrupt) 
		(*bti->enable_interrupt)(closure);
	else 
		bt431_load_formatted_cursor(bti);
	return(0);
}

static bt431_reformat_cursor(bti, cursor)
	register ws_cursor_data *cursor;
	register struct bt431info *bti;
{
	register unsigned int cw, mw;
	register int i, j;
	register int nwords, shifts;
	register unsigned int mask, emask, omask;
	unsigned short *cbp = (unsigned short *) bti->bits;
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
		for (j = 0; j < 4; j++)	 {
		    /* *cbp = ((flip[mw & 0xff] << 8) | (flip[cw & 0xff])); */
		 *cbp = ((flip[mw & 0xff] << 8) | 
			(flip[(cw & 0xff) & (mw & 0xff)]));
		    cbp++;
		    cw >>= 8;
		    mw >>= 8;
		}
		if (cursor->width <= 32) cbp += 4;
	}
}

/*
 * given precomputed cursor, load it.
 */
bt431_load_formatted_cursor(bti)
	register struct bt431info *bti;
{
   	register volatile struct bt431 *btp = bti->btaddr;
	register u_short *bgp = (u_short *) bti->bits;
  	register int  i;
	btp->addr_low = 0;
	btp->addr_high = 0;
	for(i=0; i<512; i++) {
	    btp->cursor_ram = *bgp;
	    bgp++;
	    wbflush();
	}
	bti->dirty_cursor = 0;			/* no longer dirty */
}


bt431_init(closure)
	caddr_t closure;
{
	register struct bt431info *bti 		= (struct bt431info *)closure;
	register volatile struct bt431 *btp 	= bti->btaddr;    
	register int i;
	btp->addr_low = 0x0000;  	wbflush();
	btp->addr_high = 0x0000; 	wbflush();
	for (i = 0; i < 13; i++) {
	    btp->control = 0; 		
	    wbflush();
	}
	btp->addr_low = 0x0000;		wbflush();
	btp->addr_high = 0x0000;	wbflush();
	for (i = 0; i < 512; i++) {
	    btp->control = 0;
	    wbflush();
	}
	bt431_cursor_on_off(closure,1);
}


bt431_cursor_on_off(closure, on_off)
	caddr_t closure;
	int on_off;
{
	register struct bt431info *bti		= (struct bt431info *)closure;
	register volatile struct bt431 *btp 	= bti->btaddr;
	if (on_off) bt431_load_reg(btp, bt431_CUR_CMD, 0x44);/*  on cursor */
	else 	    bt431_load_reg(btp, bt431_CUR_CMD, 0x04); /* off cursor */
	bti->on_off = on_off;

	return(0);
}

bt431_load_reg(btp, reg, val)
        register volatile struct bt431 *btp;
        u_short reg;
        u_char val;
{
        btp->addr_low  =  DUP431_B0(reg & 0xff);        wbflush();
        btp->addr_high = 0x0000;			wbflush();
        btp->control =  DUP431_B0(val);                 wbflush();
}
