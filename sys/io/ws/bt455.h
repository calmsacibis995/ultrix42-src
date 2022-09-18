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

struct bt455 {
    volatile char cmap_addr;
    char pad1[3];
    volatile char color_map;
    char pad2[3];
    volatile char clear_addr;
    char pad3[3];
    volatile char overlay;
    char pad4[3];
};


struct bt455info {
        volatile struct bt455 	*btaddr;
        unsigned char cursor_fg;
        unsigned char cursor_bg;
        void (*enable_interrupt)();     /* enables one interrupt at V.R. */
	char dirty_colormap;
	char dirty_cursormap;
	caddr_t cursor_closure;		/* hack for hook into cursor code */
};

int bt455_video_on();
int bt455_video_off();
void bt455_clean_colormap();
int bt455_init_color_map();
int bt455_load_color_map_entry();
caddr_t bt455_init_closure();
int bt455_recolor_cursor();

