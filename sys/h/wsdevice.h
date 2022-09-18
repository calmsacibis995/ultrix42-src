/*
 * @(#)wsdevice.h	4.1	(ULTRIX)	8/13/90
 */
/************************************************************************
 *									*
 *			Copyright (c)  1989 by				*
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
 * This file defines the interface that all workstation devices must use
 * to register themselves with the ws pseudo-device.  This should allow
 * new devices to be added without having to modify other drivers,
 * with large code sharing among screens or devices (cursor chips, LUTs)
 * of like types.  New frame buffer devices should be able to run without
 * modification of the X server to support them.
 *
 * The first section of this file documents the functions the driver
 * uses to identify itself to the ws driver.
 * I wish I had function prototypes available....  Sigh...
 */
/*
 * Each function will always be called with a handle (closure), to reduce
 * code required.  While a single closure would work, it would
 * reduce code sharing, as too many details of a single driver would
 * be visible in various device specific routines, like position cursor,
 * which really only needs the coordinates and the address of the cursor
 * chip.
 */

typedef struct {
	caddr_t  (*init_closure)();
	int  (*init_color_map)();
	int  (*load_color_map_entry)();
	int  (*video_on)();	/* not clear where these should go */
	int  (*video_off)();	/* but existing hardware its in cursor chips */
	caddr_t cmc;
} ws_color_map_functions;

typedef struct {
	caddr_t  (*init_closure)();
	int  (*load_cursor)();
	int  (*recolor_cursor)();
	int  (*set_cursor_position)();
	int  (*cursor_on_off)();
	caddr_t cc;
} ws_cursor_functions;

typedef struct {
	caddr_t  (*init_closure)();
	int  (*init_screen)();
	int  (*clear_screen)();
        int  (*scroll_screen)();
	int  (*blitc)();		/* output one character */
	int  (*map_unmap_screen)();
	int  (*ioctl)();
	void (*close)();
	caddr_t sc;
} ws_screen_functions;

extern int ws_define_screen();

/* 
 * all devices must at least have ws_device entries, plus whatever private
 * information they need.  Note that this makes ordering critical.
 */
typedef struct {
	int  hardware_type;
	caddr_t dc;
	int axis_count;			/* only used by extension devices */
	caddr_t (*init_closure)();
	int  (*ioctl)();
	void (*init_device)();
	void (*reset_device)();
	void (*enable_device)();
	void (*disable_device)();
	void (*set_device_mode)();
	void (*get_device_info)();
} ws_device;	

typedef struct {
	int  hardware_type;
	caddr_t kc;
	int axis_count;			/* not used */
	caddr_t (*init_closure)();
	int  (*ioctl)();
	void (*init_keyboard)();
	void (*reset_keyboard)();
	void (*enable_keyboard)();
	void (*disable_keyboard)();
	int (*set_keyboard_control)();
	void (*get_keyboard_info)();
	void (*ring_bell)();
	ws_keyboard_control control;
	ws_keyboard_definition *definition;
	ws_keycode_modifiers *modifiers;
	unsigned int *keysyms;
	unsigned char *keycodes;
} ws_keyboard;

typedef struct {
	int  hardware_type;
	caddr_t pc;
	int axis_count;			/* not used */
	caddr_t (*init_closure)();
	int  (*ioctl)();
	void (*init_pointer)();
	void (*reset_pointer)();
	void (*enable_pointer)();
	void (*disable_pointer)();
	void (*set_pointer_mode)();
	void (*get_pointer_info)();
	void (*get_position_report)();
	int mswitches;			/* current pointer switches     */
	ws_pointer_control pr;		/* pointer rates		*/
	ws_cursor_position position;	/* current pointer position	*/
	ws_pointer_box suppress;	/* suppress motion when inside  */
					/* this box.			*/
	ws_pointer_box constrain;	/* prevent cursor from leaving	*/
	int tablet_scale_x;		/* scale factors for tablet pointer */
	int tablet_scale_y;
} ws_pointer;

/* ws provided interface interface for mapping to user space */
caddr_t ws_map_region(/* addr, nbytes, how */);
/*	caddr_t addr;
	int nbytes;
	int how;
*/
