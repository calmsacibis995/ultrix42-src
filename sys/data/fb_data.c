/*
 * @(#)fb_data.c	4.5	(ULTRIX)	10/16/90
 */

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

/************************************************************************
 * Modification History
 *
 * 13-Oct-90 -- Randall Brown
 *	      
 *		Changed include of cpuconf.h to machine/common
 *
 * 09-Sep-90	Joel Gringorten
 * 
 * 		Added MFB (PMAG-AA)
 *
 * 23-Aug-90 -- Randall Brown
 * 
 *		Changed #ifdef DS5000 to also include DS5000_100 (3MIN)
 *
 * 15-Jan-90 -- Joel Gringorten
 *
 * 		Added PMAX CFB and MFB
 *
 * 15-Nov-89 -- Jim Gettys
 * 
 *	       Created.  From scratch.
 *
 ************************************************************************/

#include "fb.h"
#include "../h/devio.h"
#include "../machine/pte.h"
#include "../h/param.h"
#include "../h/conf.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/workstation.h"
#include "../h/inputdevice.h"
#include "../h/wsdevice.h"
#include "../h/tty.h"
#include "../h/map.h"
#include "../h/buf.h"
#include "../h/vm.h"
#include "../h/bk.h"
#include "../h/clist.h"
#include "../h/file.h"
#include "../h/uio.h"
#include "../h/ipc.h"
#include "../h/shm.h"
#include "../h/kernel.h"
#include "../../machine/common/cpuconf.h"
#include "../h/exec.h"
#include "../machine/cpu.h"
#include "../io/uba/ubavar.h"		/* auto-config headers 		*/
#include "../h/proc.h"
#include "../h/fbinfo.h"

#if defined(DS5000) || defined(DS5000_100)
#include "../io/ws/vfb03.h"		/* specific to 3max cfb module 	*/
#include "../io/ws/bt459.h"		/* specific to BT459 VDAC 	*/
#include "../io/ws/bt455.h"
#include "../io/ws/bt431.h"
#include "../io/ws/pmagaa.h"

#endif
#ifdef DS3100
#include "../io/ws/pmvdac.h" 		/* PMAX def */
#endif
/*
 * The following defines up to three frame buffers.  PMAX (DECstation 3100
 * and 2100's can have only one screen. If, for example
 * some new frame buffer were built for 3MAX, it can be added without modifying
 * the base frame buffer driver by adding a new entry.  A third party or
 * other Digital frame buffer can be added by appropriate additions.
 * (I hope...)
 */
#if !defined(DS5000) && !defined(DS5000_100)
#ifdef DS3100
tc_where_option() { return 0; }		/* XXX I don't like this hack */
#include "../io/tc/tc.h"
struct tc_slot tc_slot[1];
#endif
#endif


/*
 * XXX visual closures must have pointer to device as first element, this
 *is a hack, but I can't think of a better one right now.
 */

struct fb_type {
	ws_screen_descriptor screen;
	ws_depth_descriptor depth[NDEPTHS];
	ws_visual_descriptor visual[NVISUALS];
	int screen_type;
	ws_cursor_functions cf;
	int cursor_type;
	ws_color_map_functions cmf;
	int color_map_type;
	ws_screen_functions sf;
	int (*attach)();
	void (*interrupt)();
};


caddr_t fb_init_closure();
int fb_init_screen();
int fb_clear_screen();
int fb_scroll_screen();
int fb_blitc();
int fb_map_unmap_screen();


#ifdef BINARY

extern struct uba_device *fbinfo[];
extern struct fb_info fb_softc[];
extern struct fb_type fb_type[];
extern ws_monitor monitor_type[];
extern int nfb_types;
extern int nmon_types;
extern int nbt_types;
#ifdef DS5000
extern struct bt459info bt459_softc[];
extern struct bt459info bt459_type[];

extern struct bt431info bt431_softc[];
extern struct bt431info bt431_type[];

extern struct bt455info bt455_softc[];
extern struct bt455info bt455_type[];


#endif


#else

#if NFB > 0

#if defined(DS5000) || defined(DS5000_100)
struct bt459info bt459_softc[NFB];
struct bt459info bt459_type[] = {
    {	
	(struct bt459 *)VFB03_BT459_OFFSET,
	1,			/* screen initally on 	   */
	1,			/* cursor initially on		   */
	0, 0,			/* cursor and colormap celan	   */
	219, 34,		/* magic offsets to video position */
	0, 0,			/* hot spot of current cursor 	   */
	NULL,			/* enable cfb interrupt on V.R.	   */
	{ 0, 0, 0, 0, },
	{ 0, 0, 0, 0, },
	{ 0, 0, 0, 0, },
	256, 0,
    },
};
int nbt_types = sizeof(bt459_type) / sizeof(struct bt459info);


struct bt431info bt431_softc[NFB];
struct bt431info bt431_type[] = {

    {	
	(struct bt431 *)PMAGAA_BT431_OFFSET,
	1,			/* screen initally on 	   	   */
	1,			/* cursor initially on		   */
	0,		        /* cursor clean 		   */
	0x168, 0x24,		/* magic offsets to video position */
	0, 0,			/* hot spot of current cursor 	   */
	0,			/* not initialized yet - hack!!!   */
	NULL,			/* enable cfb interrupt on V.R.	   */
    },
};



struct bt455info bt455_softc[NFB];
struct bt455info bt455_type[] = {

    {	
	(struct bt455 *)PMAGAA_BT455_OFFSET,
	0, 0xFF,		/* cursor fg and bg value      	   */
	NULL,
	0,
	0
    },
};



#endif

#ifdef DS3100

struct pmvdacinfo pm_softc[1];
struct pmvdacinfo pmvdac_type[] = {
    {
	0, 			/* vdac */
	0, 			/* pcc */
	212, 34,		/* magic offsets to video position */
	0, 0,			/* hot spot of current cursor 	   */
	{ 0, 0, 0, 0, },
	{ 0, 0, 0, 0, },
	{ 0, 0, 0, 0, },
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    },
};
#endif

/* the flags field from the configuration is used to set the monitor type */
struct fb_info fb_softc[NFB];
#define P4 1		/* P4 phosphor */
#define PW 2		/* paper white */
#define PVRE 3		/* yellow-orange EL, approx 585 nm */
#define SONY 4		/* Sony color */
#define MAT 5		/* Matshusita color */
#define PHILLIPS 6	/* Phillips color */
#define HITACHI	7	/* Hitachi color */
/* This is so an X server can deal with different phosphor types */
ws_monitor monitor_type[] = {
	{ "VRUNK ", 333, 281, COLOR, 0 },	/* who knows, presume 19"*/
	{ "VR299 ", 333, 281, COLOR, MAT },	/* 19" Color Matsushita */
	{ "VR290 ", 333, 281, COLOR, PHILLIPS },/* 19" Color Phillips 	*/
	{ "VR260 ", 333, 281, MONOCHROME, P4 },	/* 19" Mono DEC 	*/
	{ "VR262 ", 333, 281, MONOCHROME, P4 },	/* 19" Mono DEC improved*/
	{ "VR150 ", 240, 203, MONOCHROME, PW },	/* 15" Monochrome Hitachi*/
	{ "VRE01 ", 346, 292, MONOCHROME, PVRE},/* VRE01 flat panel	*/
	{ "VR160 ", 240, 203, COLOR, HITACHI },	/* 15" Color Hitachi	*/
	{ "VR297 ", 280, 236, COLOR, SONY },	/* 16" Sony	(wow!)	*/
};

struct fb_type fb_type[] = {
#ifdef DS3100
{
	/* PMAX COLOR */
        {
	0, 
	{ "VR297 ", 333, 281, COLOR, 7 },
	"PMAX-CFB",
	1024, 864,		/* width, height */
	0,			/* depth */
	1,			/* number of depths present		*/
	1,			/* number of visual types of screen 	*/
	0, 0,			/* current pointer position 		*/
	0, 0,			/* current text position		*/
	56, 80,			/* maximum row, col text position 	*/
	8, 15,			/* console font width and height	*/
	16, 16,			/* maximal size cursor for screen	*/
	1, 1,			/* min, max of visual types		*/
	},

	{			/* depth descriptor of root window */
	0, 0, 			/* which screen and depth		*/
	1024, 1024,		/* frame buffer size in pixels		*/
	8,			/* returns the depth (out)		*/
	8,			/* stride of pixel (out)		*/
	32,			/* scan line pad			*/
    	(caddr_t) 0,		/* bitmap starts at beginning */
	0,			/* only filled in when mapped		*/
	(caddr_t)(PLANE_REG - FRAME_BUF), /*plane mask offset */
	0,			/* only filled in when mapped		*/
	},

	{			/* visual descriptor */
	0,			/* which screen (in)			*/
    	0,			/* which visual of screen (in) 		*/
	PseudoColor,		/* class of visual 			*/
	8,			/* number of bits per pixel		*/
	0, 0, 0,		/* zero since pseudo; mask of subfields */
	8,			/* bits per RGB 			*/
	256,			/* color map entries */
    	},
	0,
	{
	pmax_init_closure,
	pmax_load_cursor,
	pmax_recolor_cursor,
	pmax_set_cursor_position,
	pmax_cursor_on_off,
	(caddr_t)pm_softc,
        },
	0,
	{
	pmax_init_closure,
	pmax_init_color_map,
	pmax_load_color_map_entry,
	pmax_video_on,
	pmax_video_off,
	(caddr_t)pm_softc,
	},
	0,
	{
	fb_init_closure,
	fb_init_screen,
	fb_clear_screen,
	fb_scroll_screen,
	fb_blitc,
	fb_map_unmap_screen,
	NULL,			/* ioctl optional */
	NULL,			/* close optional */
	(caddr_t)fb_softc,
	},
	NULL,			/* not yet interrupt driven */
	NULL,			/* not yet interrupt driven */
},
{
	/* PMAX */
        {
	0, 
	{ "VR260 ", 333, 281,  MONOCHROME, 1 },
	"PMAX-MFB",
	1024, 864,		/* width, height */
	0,			/* depth */
	1,			/* number of depths present		*/
	1,			/* number of visual types of screen 	*/
	0, 0,			/* current pointer position 		*/
	0, 0,			/* current text position		*/
	56, 80,			/* maximum row, col text position 	*/
	8, 15,			/* console font width and height	*/
	16, 16,			/* maximal size cursor for screen	*/
	1, 1,			/* min, max of visual types		*/
	},

	{			/* depth descriptor of root window */
	0, 0, 			/* which screen and depth		*/
	2048, 1024,		/* frame buffer size in pixels		*/
	1,			/* returns the depth (out)		*/
	1,			/* stride of pixel (out)		*/
	32,			/* scan line pad			*/
    	(caddr_t) 0,		/* bitmap starts at beginning */
	0,			/* only filled in when mapped		*/
	(caddr_t)(PLANE_REG - FRAME_BUF), /*plane mask offset */
	0,			/* only filled in when mapped		*/
	},

	{			/* visual descriptor */
	0,			/* which screen (in)			*/
    	0,			/* which visual of screen (in) 		*/
	StaticGray,		/* class of visual 			*/
	8,			/* number of bits per pixel		*/
	0, 0, 0,		/* zero since pseudo; mask of subfields */
	8,			/* bits per RGB 			*/
	256,			/* is this correct? */
    	},
	0,
	{
	pmax_init_closure,
	pmax_load_cursor,
	pmax_recolor_cursor,
	pmax_set_cursor_position,
	pmax_cursor_on_off,
	(caddr_t)pm_softc,
        },
	0,
	{
	pmax_init_closure,
	pmax_init_color_map,
	pmax_load_color_map_entry,
	pmax_video_on,
	pmax_video_off,
	(caddr_t)pm_softc,
	},
	0,
	{
	fb_init_closure,
	fb_init_screen,
	fb_clear_screen,
	fb_scroll_screen,
	fb_blitc,
	fb_map_unmap_screen,
	NULL,			/* ioctl optional */
	NULL,			/* close optional */
	(caddr_t)fb_softc,
	},
	NULL,			/* not yet interrupt driven */
	NULL,			/* not yet interrupt driven */
},
#endif
#if defined(DS5000) || defined(DS5000_100)
{	
        {
	0, 			/* screen number (in) */
	{ "VR297 ", 333, 281, COLOR, 7 },
	"PMAG-BA ", 		/* exact string in 3MAX rom option ID	*/
	1024, 864,		/* width, height */
	0,			/* depth */
	1,			/* number of depths present		*/
	1,			/* number of visual types of screen 	*/
	0, 0,			/* current pointer position 		*/
	0, 0,			/* current text position		*/
	56, 80,			/* maximum row, col text position 	*/
	8, 15,			/* console font width and height	*/
	64, 64,			/* maximal size cursor for screen	*/
	1, 1,			/* min, max of visual types		*/
	},

	{			/* depth descriptor of root window */
	0, 0, 			/* which screen and depth		*/
	1024, 864,		/* frame buffer size in pixels		*/
	8,			/* returns the depth (out)		*/
	8,			/* stride of pixel (out)		*/
	32,			/* scan line pad			*/
    	VFB03_FB_OFFSET,	/* frame buffer starts at 0 in fb slot  */
	0,			/* only filled in when mapped		*/
	0,			/* no plane mask on 3MAX CFB, or offset */
	0,			/* only filled in when mapped		*/
	},

	{			/* visual descriptor */
	0,			/* which screen (in)			*/
    	0,			/* which visual of screen (in) 		*/
	PseudoColor,		/* class of visual 			*/
	8,			/* number of bits per pixel		*/
	0, 0, 0,		/* zero since pseudo; mask of subfields */
	8,			/* bits per RGB 			*/
	256,			/* color map entries */
    	},
	0,
	{
	vfb03_bt_init_closure,
	bt_load_cursor,
	bt_recolor_cursor,
	bt_set_cursor_position,
	bt_cursor_on_off,
	(caddr_t)bt459_softc,
        },
	0,
	{
	vfb03_bt_init_closure,
	bt_init_color_map,
	bt_load_color_map_entry,
	bt_video_on,
	bt_video_off,
	(caddr_t)bt459_softc,
	},
	0,
	{
	fb_init_closure,
	fb_init_screen,
	fb_clear_screen,
	fb_scroll_screen,
	fb_blitc,
	fb_map_unmap_screen,
	NULL,			/* ioctl optional */
	NULL,			/* close optional */
	(caddr_t)fb_softc,
	},	
	vfb03_attach,
	vfb03_interrupt,
    },
    {
        {
	0, 			/* screen number (in) */
	{ "VR297 ", 333, 281, COLOR, 7 },
	"PMAG-AA ", 		/* exact string in 3MAX rom option ID	*/
	1280, 1024,		/* width, height */
	0,			/* depth */
	1,			/* number of depths present		*/
	1,			/* number of visual types of screen 	*/
	0, 0,			/* current pointer position 		*/
	0, 0,			/* current text position		*/
	67, 80,			/* maximum row, col text position 	*/
	8, 15,			/* console font width and height	*/
	64, 64,			/* maximal size cursor for screen	*/
	1, 1,			/* min, max of visual types		*/
	},

	{			/* depth descriptor of root window */
	0, 0, 			/* which screen and depth		*/
	2048, 1024,		/* frame buffer size in pixels		*/
	1,			/* returns the depth (out)		*/
	8,			/* stride of pixel (out)		*/
	32,			/* scan line pad			*/
    	(caddr_t)PMAGAA_FB_OFFSET,  /* frame buffer starts at 0 in fb slot  */
	0,			/* only filled in when mapped		*/
	0,			/* no plane mask on 3MAX CFB, or offset */
	0,			/* only filled in when mapped		*/
	},

	{			/* visual descriptor */
	0,			/* which screen (in)			*/
    	0,			/* which visual of screen (in) 		*/
	StaticGray,		/* class of visual 			*/
	8,			/* number of bits per pixel		*/
	0, 0, 0,		/* zero since pseudo; mask of subfields */
	8,			/* bits per RGB 			*/
	2,			/* color map entries */
    	},
	0,
	{
	pmagaa_bt431_init_closure,
	bt431_load_cursor,
	pmagaa_recolor_cursor,
	bt431_set_cursor_position,
	bt431_cursor_on_off,
	(caddr_t)bt431_softc,
        },
	0,
	{
	pmagaa_bt455_init_closure,
	bt455_init_color_map,
	bt455_load_color_map_entry,
	pmagaa_video_on,
	pmagaa_video_off,
	(caddr_t)bt455_softc,
	},
	0,
	{
	fb_init_closure,
	fb_init_screen,
	fb_clear_screen,
	fb_scroll_screen,
	fb_blitc,
	fb_map_unmap_screen,
	NULL,			/* ioctl optional */
	NULL,			/* close optional */
	(caddr_t)fb_softc,
	},	
	 pmagaa_attach,		/* attach */
	 pmagaa_interrupt,	/* interrupt */
    },


#endif
};

int nfb_types = sizeof(fb_type) / sizeof (struct fb_type);
int nmon_types = sizeof(monitor_type) / sizeof(ws_monitor);
struct	uba_device *fbinfo[NFB];

#else
struct fb_info 	fb_softc[1];
struct	uba_device *fbinfo[1];

int nfb_types = 0;
int nmon_types = 0;
int nbt_types = 0;
#endif

#endif BINARY

