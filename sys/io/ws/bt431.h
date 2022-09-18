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


struct bt431 {
        volatile unsigned short addr_low;
        u_short pad1;
    	volatile unsigned short addr_high;
	u_short pad2;
	volatile unsigned short cursor_ram;
	u_short pad3;
	volatile unsigned short control;
	u_short pad4;
};

struct bt431info {
        volatile struct bt431 	*btaddr;
        char screen_on;                 /* whether screen is on */
        char on_off;                    /* whether cursor is on...*/
        char dirty_cursor;              /* has cursor been reloaded?*/
        short fb_xoffset;               /* offset to video */
        short fb_yoffset;
        short x_hot;                    /* hot spot of current cursor*/
        short y_hot;
	char inited;			/* kludge for cursor init */
        void (*enable_interrupt)();     /* enables one interrupt at V.R. */
	char cursor_was_on;		/* cursor state when video-offed */
	caddr_t cmap_closure;		/* hack for hook into colormap code */
        u_short bits[512];        
};


int bt431_load_cursor();
int bt431_recolor_cursor();
int bt431_set_cursor_position();
int bt431_load_formatted_cursor();
int bt431_cursor_on_off();
int bt431_clean_cursormap();

/*
 * 2 bytes of bt431 internal address.
 */

#define bt431_CUR_CMD           0x0000 /* cursor command reg */

#define bt431_CUR_XLO           0x0001 /* cursor x(lo) */
#define bt431_CUR_XHI           0x0002 /*  */
#define bt431_CUR_YLO           0x0003 /* cursor y(lo) */
#define bt431_CUR_YHI           0x0004 /*  */

#define bt431_WIN_XLO           0x0005 /* window x(lo) */
#define bt431_WIN_XHI           0x0006 /*  */
#define bt431_WIN_YLO           0x0007 /* window y(lo) */
#define bt431_WIN_YHI           0x0008 /*  */

#define bt431_WIN_WLO           0x0009 /* window width(lo) */
#define bt431_WIN_WHI           0x000a /*  */
#define bt431_WIN_HLO           0x000b /* window height(lo) */
#define bt431_WIN_HHI           0x000c /*  */

#define DUP431_B0(X) \
(unsigned short) ((((unsigned short)(X) << 8) & 0xff00) \
			 | (((unsigned short)(X)) & 0xff))
