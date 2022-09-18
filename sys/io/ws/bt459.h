/*
 * @(#)bt459.h	4.1	(ULTRIX)	8/13/90
 */

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

struct bt459 {
    volatile char addr_low;
    char pad1[3];
    volatile char addr_high;
    char pad2[3];
    volatile char bt_reg;
    char pad3[3];
    volatile char color_map;
    char pad4[3];
};

/*
 * the reason a different color cell representation is used is
 * to reduce kernel memory usage.  This form is sufficient for
 * an 256 entry color map, and saves 8 bytes/cell, or 2k bytes/screen.
 */
struct bt459_color_cell {
	unsigned char dirty_cell;
	unsigned char red;	/* only need 8 bits */
	unsigned char green;
	unsigned char blue;
};

struct bt459info {
	volatile struct bt459 *btaddr;
	char screen_on;			/* whether screen is on */
	char on_off;			/* whether cursor is on...*/
	char dirty_cursor;		/* has cursor been reloaded?*/
	char dirty_colormap;		/* has cmap been reloaded?*/
	short fb_xoffset;		/* offset to video */
	short fb_yoffset;
	short x_hot;			/* hot spot of current cursor*/
	short y_hot;
	void (*enable_interrupt)();	/* enables one interrupt at V.R. */
	ws_color_cell saved_entry;
	ws_color_cell cursor_fg;
	ws_color_cell cursor_bg;
	short min_dirty, max_dirty;	/* range of dirty entries */
	unsigned long bits[256];	/* only put zero items after here*/
	struct bt459_color_cell cells[256];
};

#define ADDR_LOW_MASK		0x00ff
#define ADDR_HIGH_MASK		0xff00

#define COLOR_MAP_BASE		0

#define	OVERLAY_COLOR_BASE	0x0100
#define CURSOR_COLOR_1		0x0181
#define	CURSOR_COLOR_2		0x0182
#define	CURSOR_COLOR_3		0x0183
#define	ID_REG			0x0200
#define CMD_REG_0		0x0201
#define	CMD_REG_1		0x0202
#define CMD_REG_2		0x0203
#define PIXEL_READ_MASK		0x0204
#define PIXEL_BLINK_MASK	0x0206
#define OVERLAY_READ_MASK	0x0208
#define OVERLAY_BLINK_MASK	0x0209
#define	INTERLEAVE_REG		0x020a
#define	TEST_REG		0x020b
#define	RED_SIGNATURE		0x020c
#define GREEN_SIGNATURE		0x020d
#define BLUE_SIGNATURE		0x020e
#define CURSOR_CMD_REG		0x0300
#define CURSOR_X_LOW		0x0301
#define CURSOR_X_HIGH		0x0302
#define CURSOR_Y_LOW		0x0303
#define CURSOR_Y_HIGH		0x0304
#define WINDOW_X_LOW		0x0305
#define WINDOW_X_HIGH		0x0306
#define WINDOW_Y_LOW		0x0307
#define WINDOW_Y_HIGH		0x0308
#define WINDOW_WIDTH_LOW	0x0309
#define WINDOW_WIDTH_HIGH	0x030a
#define	WINDOW_HEIGHT_LOW	0x030b
#define WINDOW_HEIGHT_HIGH 	0x030c
#define CURSOR_RAM_BASE		0x0400

int bt_load_cursor();
int bt_init_color_map();
int bt_load_color_map_entry();
int bt_load_cursor();
int bt_recolor_cursor();
int bt_set_cursor_position();
int bt_cursor_on_off();
int bt_load_formatted_cursor();
int bt_video_on();
int bt_video_off();
void bt_clean_colormap();
