/*
 * @(#)gx.h	4.5	(ULTRIX)	12/6/90
 *
 * 06-Dec-90 -- Randall Brown 
 *	Added #define LK_MODE_CHG_ACK.
 *
 * 22-Oct-90 -- Sam Hsu
 *	Change tablet puck button definitions to match tablet.h.
 *
 * 20-Apr-90  -- Sam Hsu
 *	Add GX_CONSOLE.  Remove extraneous test in GX_HAVESERVER.  Define
 *	GX_MULTIOPEN.  Add STIC_INT_WE.  Remove QIO_MAPSLOT.  Moved cursor
 *	and colormap to common area, then backed out of that change due to
 *	apprehension over interface change.
 *
 * 04-Apr-90  -- Sam Hsu
 *	Turn off debugging for UEG copy.  Add IPLTTY() macro.
 *
 * 28-Mar-90  -- Sam Hsu
 *	gxInfo->gxo is now a permanent fixture.  Put in bug fix for STIC
 *	addressing found by Paul.  Set debug level on panic to reduce output.
 *	Remove some no-longer-used #define's.  Add mapslot ioctl for bmk.
 *	IAMSERVER/HAVESERVER macros.  Console font cell size 12x21->11x20.
 *	Console text fg index 0x1 instead of 0xff.  Clean up DUPBYTEs to
 *      generate less code.
 *
 * 29-Jan-90  -- Sam Hsu
 *	Redefine debugging macros to be dynamic, so one can change them
 *	in a running kernel.
 *
 * 17-Jan-90  -- Sam Hsu
 *	Move STIC definitions from stamp.h into here... get rid of 3.1C
 *	ioctl definitions.
 *
 * 18-Dec-89  -- Sam Hsu
 *	Include Paul J's macros for cons'ing up STIC DMA poll addresses,
 *	with the usual name changes from GA_ to GX_.  Both 2/3DA systems
 *	use the same scheme (originally in ga.h).
 *
 * 12-Dec-89  -- Paul Jensen
 *	Use new Bt459 interface.  Change _TEXT_FG to something which
 *	works on 8- and 24-plane systems.  Define verbosity levels
 *	for FDEBUG, as per Sam's suggestion.
 *
 *  7-Dec-89  -- Paul Jensen
 *	Define the ring buffer to be an offset from the top of the
 *	gxPriv structure (ringbufferoffset), instead of an
 *	array-of-int.
 *
 * 30-Nov-89  -- Sam Hsu
 *	Convert/merge into 4.0 source pool.
 *
 * 16-Oct-89  -- Sam Hsu
 *	non-KERNEL programs needn't include stamp.h and gao.h anymore.
 *
 * 21-Sep-89  -- Sam Hsu
 *	KMAX3D ioctl.h file.  Removed extraneous fields from pm_info.
 *
 * 22-Aug-89  -- Sam Hsu
 *	Based on pm.h  Add KMAX3D changes.  Include gao.h and stamp.h.
 *
 *    $Header: /nfs/kodak/local/4.0-12/io/tc/RCS/gx.h,v 1.32 90/06/25 11:06:27 fhsu Exp Locker: fhsu $;
 */
#ifndef _GX_H_
#define _GX_H_
/*
 * Graphics Accelerator Option common header file
 */

#define GX_MULTIOPEN			/* multi-open */

#define GX_SILENT	0
#define GX_CONSOLE	1		/* allow output (at all) on SLU3 */
#define GX_PSST		2
#define GX_TERSE	3
#define GX_TALK		4
#define GX_YAK		5
#define GX_GAB		7
#define GX_BLAB		10
#define GX_YOW		13
#define GX_NEVER	99
#define GX_DEBUGGING	GX_TERSE	/* default debug level */
#define GX_PANIC	GX_SILENT	/* msgs allowed when panic'ing */

#define GX_NODEBUG			/* define this to compile out */
					/* debugging code */
#ifdef  GX_NODEBUG
#	define GX_DEBUG(L,S)
#else
#	define GX_DEBUG(L,S)	if ((gx_level=(L)) <= gx_debug) { S }
#endif

#ifdef KERNEL
#      include "../io/tc/pmevent.h"
#      include "../h/types.h"
#      include "../h/ioctl.h"
#      include "../io/tc/stamp.h"
#      define WBFLUSH()			/* 3MAX wb doesn't merge writes */
/* only do an spltty() if we are at a lower spl. */
#      define IPLTTY(L)		{ (L) = getspl(); \
				  if (whatspl((L)) < SPLTTY) \
				  	(L) = spltty(); \
				}
#else
#      include <io/tc/pmevent.h>
#      include <sys/param.h>
#      include <sys/ioctl.h>
#endif


/*
 * This all belongs in a common header file!!!		XXX
 *
 * VAXstar Monochrome definitions.
 */
#define	SELF_TEST	'T'
#define	INCREMENTAL	'R'
#define	PROMPT		'D'

#define	MOUSE_ID	0x2
#define TABLET_ID	0x4

#define START_FRAME	0x80		/* start of report frame bit */
#define X_SIGN		0x10		/* sign bit for X */
#define Y_SIGN		0x08		/* sign bit for Y */

/*
 * Line Prameter Register bits
 */
#define	SER_KBD      000000
#define	SER_POINTER  000001
#define	SER_COMLINE  000002
#define	SER_PRINTER  000003
#define	SER_CHARW    000030
#define	SER_STOP     000040
#define	SER_PARENB   000100
#define	SER_ODDPAR   000200
#define	SER_SPEED    006000
#define	SER_RXENAB   010000

/*
 * Mouse definitions
 */
#define MOTION_BUFFER_SIZE 100
#define	SELF_TEST	'T'

#define EVENT_T_LEFT_BUTTON	0x01
#define EVENT_T_FRONT_BUTTON	0x02
#define EVENT_T_RIGHT_BUTTON	0x03
#define EVENT_T_BACK_BUTTON	0x04

/*
 * puck buttons
 */
#define T_LEFT_BUTTON		0x02
#define T_FRONT_BUTTON		0x04
#define T_RIGHT_BUTTON		0x08
#define T_BACK_BUTTON		0x10

/*
 * Lk201 keyboard 
 */
#define LK_UPDOWN	0x86		/* bits for setting lk201 modes */
#define LK_AUTODOWN	0x82
#define LK_DOWN		0x80
#define LK_DEFAULTS	0xd3		/* reset (some) default settings*/
#define LK_AR_ENABLE	0xe3		/* global auto repeat enable	*/
#define LK_CL_ENABLE	0x1b		/* keyclick enable		*/
#define LK_KBD_ENABLE	0x8b		/* keyboard enable		*/
#define LK_BELL_ENABLE	0x23		/* the bell			*/
#define LK_LED_ENABLE	0x13		/* light led			*/
#define LK_LED_DISABLE	0x11		/* turn off led			*/
#define LK_RING_BELL	0xa7		/* ring keyboard bell		*/
#define LED_1		0x81		/* led bits			*/
#define LED_2		0x82
#define LED_3		0x84
#define LED_4		0x88
#define LED_ALL		0x8f
#define LK_KDOWN_ERROR	0x3d		/* key down on powerup error	*/
#define LK_POWER_ERROR	0x3e		/* keyboard failure on pwrup tst*/
#define LK_OUTPUT_ERROR 0xb5		/* keystrokes lost during inhbt */
#define LK_INPUT_ERROR	0xb6		/* garbage command to keyboard	*/
#define LK_MODE_CHG_ACK	0xba		/* mode change acknowledge	*/
#define LK_LOWEST	0x56		/* lowest significant keycode	*/
#define LK_DIV6_START	0xad		/* start of div 6		*/
#define LK_DIV5_END	0xb2		/* end of div 5			*/

/*
 * Keycodes for special keys and functions
 */
#define SHIFT	0xae
#define LOCK	0xb0
#define REPEAT	0xb4
#define CNTRL	0xaf
#define ALLUP	0xb3
#define	HOLD	0x56

/*
 * And stuff that appears all over creation...
 */
#define	CONSOLEMAJOR	0
#define CONS_DEV	0x01
#define GRAPHIC_DEV	0x02
#define SLU3_DEV	0x04

#define TOY ((time.tv_sec * 1000) + (time.tv_usec / 1000))

#define IS_MONO	  (0)

#define _TEXT_WIDTH	11
#define _TEXT_HEIGHT	20
#define _TEXT_CFG	(0xffffff)	/* cursor color = white */
#define _TEXT_TFG	(0x010101)	/* text color = white */
#define _TEXT_ROWS	((BT459_MAXY/_TEXT_HEIGHT)-1)
#define _TEXT_COLS	((BT459_MAXX/_TEXT_WIDTH)-1)
#define _TEXT_SCROLL	(_TEXT_ROWS/9)

#define GX_LOAD(A,B) 	((A)&0xfff) | (((B)&0xfff) << 16)
#define GX_GET2ROWS(C)	GX_LOAD((int)(*(C)),(int)(*((C)+1)));(C)+=2

/*
 * ULTRIX settings for first open.		  
 */
#define IFLAGS	(EVENP|ECHO|XTABS|CRMOD)

/*
 * Termio flags will be set to these default values in non-termio mode to
 * provide a backward compatible ULTRIX environment. 
 */
#define IFLAG (BRKINT|IGNPAR|ISTRIP|IXON|IXANY|ICRNL)
#define OFLAG (OPOST|TAB3|ONLCR)
#define LFLAG (ISIG|ICANON|ECHO)
#define CFLAG (PARENB|CREAD|CS7|CLOCAL)

#define GX_CALL(P)	if (P) (*(P))

#define GX_IAMSERVER	(u.u_procp->p_pid == gx_server.pid)

#ifdef GX_MULTIOPEN
#      define GX_HAVESERVER (gx_serverp && (!gx_info_gone(gx_server.pid)))
#else
#      define GX_HAVESERVER (gx_serverp)
#endif

/*
 * STamp Interface Chip
 */
typedef struct _stic_regs {
    int __pad0;			/* 0x..180000 */
    int __pad1;			/* 0x..180004 */
    int	hsync;			/* 0x..180008 */
    int	hsync2;			/* 0x..18000c */
    int	hblank;			/* 0x..180010 */
    int	vsync;			/* 0x..180014 */
    int	vblank;			/* 0x..180018 */
    int	vtest;			/* 0x..18001c */
    int	ipdvint;		/* 0x..180020 */
    int	__pad2;			/* 0x..180024 */
    int	sticsr;			/* 0x..180028 */
    int	busdat;			/* 0x..18002c */
    int	busadr;			/* 0x..180030 */
    int	__pad3;			/* 0x..180034 */
    int	buscsr;			/* 0x..180038 */
    int	modcl;			/* 0x..18003c */
} sticRegs;
#define STIC_regs	sticRegs

typedef struct _stic_csr {
    unsigned tstfnc : 2;
    unsigned checkpar : 1;
    unsigned startvt : 1;
    unsigned start : 1;
    unsigned reset : 1;
    unsigned autoread : 1;
    unsigned startst : 1;
    unsigned : 24;
} stic_csr;

/* masks for STIC CSR register */
#define STIC_CSR_TSTFNC		0x00000003
#define STIC_CSR_TSTFNC_NORMAL 	0
#define STIC_CSR_TSTFNC_PARITY 	1
#define STIC_CSR_TSTFNC_CNTPIX 	2
#define STIC_CSR_TSTFNC_TSTDAC 	3
#define STIC_CSR_CHECKPAR	0x00000004
#define STIC_CSR_STARTVT	0x00000008
#define STIC_CSR_START		0x00000010
#define STIC_CSR_RESET		0x00000020
#define STIC_CSR_AUTOREAD	0x00000040
#define STIC_CSR_STARTST	0x00000080

typedef struct _stic_intr {
    unsigned	eint_en : 1;	     /* 0 */
    unsigned	eint : 1;	     /* 1 */
    unsigned	eint_mask : 1;	     /* 2 */
    unsigned	: 5;
    unsigned	vint_en : 1;	     /* 8 */
    unsigned	vint : 1;	     /* 9 */
    unsigned	vint_mask : 1;	     /* 10 */
    unsigned	: 5;
    unsigned	pint : 1;	     /* 16 */
    unsigned	pint_en : 1;	     /* 17 */
    unsigned	pint_mask : 1;	     /* 18 */
} sticIntr;
#define STIC_intr	sticIntr

/* masks for %int register */
#define STIC_INT_E_EN 0x00000001
#define STIC_INT_E    0x00000002
#define STIC_INT_E_WE 0x00000004
#define STIC_INT_V_EN 0x00000100
#define STIC_INT_V    0x00000200
#define STIC_INT_V_WE 0x00000400
#define STIC_INT_P_EN 0x00010000
#define STIC_INT_P    0x00020000
#define STIC_INT_P_WE 0x00040000

#define STIC_INT_WE   (STIC_INT_E_WE|STIC_INT_V_WE|STIC_INT_P_WE)
#define STIC_INT_CLR  (STIC_INT_E_EN|STIC_INT_E_WE|STIC_INT_V_WE|STIC_INT_P_WE)

typedef struct _stic_modtype {
    unsigned	: 8;
    unsigned	vdac : 1;
    unsigned	yconfig : 2;
    unsigned	xconfig : 1;
    unsigned	option : 3;
    unsigned	: 9;
    unsigned	revision : 8;
} sticCf;
#define STIC_cf		sticCf

/* masks for %modcl bits */
#define STIC_CF_VDAC_T		0x00000100 /* <8> */
#define STIC_CF_CONFIG_Y	0x00000600 /* <10:9> */
#define STIC_CF_CONFIG_X	0x00000800 /* <11> */
#define STIC_CF_CONFIG_OPTION	0x00007000 /* <14:12> */
#define STIC_CF_PLANES		0x00004000 /* <14> */
#define STIC_CF_ZPLANES		0x00001000 /* <12> */
#define STIC_CF_REV		0xff000000 /* <31:24> */

/* STIC option types (derived from the STIC modtype option field) */
#define STIC_OPT_2DA		0x0		/* 2D Accelerator */
#define STIC_OPT_2DA_SH		0x0		/* 2D Accelerator */
#define STIC_OPT_3DA_SH		STIC_CF_CONFIG_OPTION

/*
 * 2DA STIC polling address:
 *
 *   For 2DA, <15:17> aren't connected at all.  Ditto for <24:26>.
 *   Input <2:14+15:20> become the STIC's <2:14+18:23> and likewise
 *	for <21:22> to STIC <27:28>.
 *
 *   This means you can specify (input) 32K of physically contiguous
 *	memory to the STIC before bumping against some tie-lines, but...
 *
 *   The STIC can only "see" 23 bits of address (2da), and only if you form
 *	the polling address such that some bits get shift up into the
 *	STIC's <18:23> & <27:28>, since they're tied to <15:20> & <21:22>
 *	on the bus.  This means the packet buffers must be in the 1st
 *	8MB of physical memory.
 */

#define _0to14  (0x00007fff)	/* bits 00-14 set */
#define _0to23	(0x00ffffff)	/* bits 00-23 set */
#define _2to21  (0x003ffffc)	/* bits 02-21 set */
#define _6to8	(0x000001c0)	/* bits 00-08 set */
#define _11to28 (0x1ffff800)	/* bits 11-28 set */
#define _15to17	(0x00038000)	/* bits 15-17 set */
#define _15to20 (0x001f8000)	/* bits 15-20 set */
#define _18to23 (0x00fc0000)	/* bits 18-23 set */
#define _18to28 (0x1ffc0000)	/* bits 18-28 set */
#define _21to22 (0x00600000)	/* bits 21-22 set */
#define _24to28 (0x1f000000)	/* bits 24-28 set */
#define _27to28	(0x18000000)	/* bits 27-28 set */

/* convert a system physical address to a STIC space physical address */
#define GX_SYS2STIC(A) ((((A)&_21to22)<<6)|(((A)&_15to20)<<3)|((A)&_0to14))
#define GX_SYS_TO_STIC(A) GX_SYS2STIC(((u_long)(A)))

/* convert a STIC space physical address to a system physical address */
#define GX_STIC2SYS(A) ((((A)&_27to28)>>6)|(((A)&_18to23)>>3)|((A)&_0to14))
#define GX_STIC_TO_SYS(A) GX_STIC2SYS(((u_long)(A)))

/* convert a system physical address to STIC DMA encoding - architecture spec */
#define GX_PHYS_TO_DMA(A) ((((u_long)(A)) & _11to28) >> 9)

/* convert a system physical address to a DMA encoding - implementation spec */
#define GX_SYS2DMA(A)	( (((A)&~_0to14)<<3) | ((A)&_0to14) )
#define GX_SYS_TO_DMA(A)  GX_PHYS_TO_DMA(GX_SYS2DMA(((u_long)(A))))


/*
 * Bt459 VDAC
 * <C1:C0> control selection of registers.  VDAC sits on the I/O bus,
 * so can read/write to it in the unusual way.
 */
typedef struct _bt459_regs {
    int               addr_lo;		/* +0x00 <~c1,~c0> */
    int               addr_hi;		/* +0x04 <~c1, c0> */
    int               data;		/* +0x08 < c1,~c0> */
    int               cmap;		/* +0x0c < c1, c0> */
} bt459Regs;
#define BT459_regs	bt459Regs

#define VDAC_RDAT	(*vdac_data & gx_planemask)

#define _DUPBYTE0(X) \
    (((((int)(X))<<16)&0xff0000)|((((int)(X))<<8)&0xff00)|(((int)(X))&0xff))
#define _DUPBYTE1(X) \
    (((((int)(X))<<8)&0xff0000)|(((int)(X))&0xff00)|((((int)(X))>>8)&0xff))
#define _DUPBYTE2(X) \
    ((((int)(X))&0xff0000)|((((int)(X))>>8)&0xff00)|((((int)(X))>>16)&0xff))

/*
 * 2 bytes of BT459 internal address.
 */
#define BT459_PIX_COLOR		0x0000 /* pixel color map */
#define BT459_OVRLY_COLOR0	0x0100 /* overlay color 0 of 16 */
#define BT459_CUR_COLOR1	0x0181 /* rgb: cursor color 1 */
#define BT459_CUR_COLOR2	0x0182 /* rgb */
#define BT459_CUR_COLOR3	0x0183 /* rgb */
#define BT459_ID_REG		0x0200 /* should be 0x4a */
#define BT459_CMD_0		0x0201 /* command reg 0 */
#define BT459_CMD_1		0x0202 /*  */
#define BT459_CMD_2		0x0203 /*  */
#define BT459_PIX_RMASK		0x0204 /* pixel read mask */
#define BT459_PIX_BMASK		0x0205 /* pixel blink mask */
#define BT459_INTERL_REG	0x020a /* interleave reg */
#define BT459_TEST_REG		0x020b /*  */
#define BT459_SIG_RED		0x020c /* signature reg's */
#define BT459_SIG_GREEN		0x020d /*  */
#define BT459_SIG_BLUE		0x020e /*  */
#define BT459_CUR_CMD		0x0300 /* cursor command reg */
#define BT459_CUR_XLO		0x0301 /* cursor x(lo) */
#define BT459_CUR_XHI		0x0302 /*  */
#define BT459_CUR_YLO		0x0303 /* cursor y(lo) */
#define BT459_CUR_YHI		0x0304 /*  */
#define BT459_WIN_XLO		0x0305 /* window x(lo) */
#define BT459_WIN_XHI		0x0306 /*  */
#define BT459_WIN_YLO		0x0307 /* window y(lo) */
#define BT459_WIN_YHI		0x0308 /*  */
#define BT459_WIN_WLO		0x0309 /* window width(lo) */
#define BT459_WIN_WHI		0x030a /*  */
#define BT459_WIN_HLO		0x030b /* window height(lo) */
#define BT459_WIN_HHI		0x030c /*  */
#define BT459_CUR_RAM		0x0400 /* cursor bitmap: to 0x7ff */

#define BT459_MAXERR		10
#define BT459_CURH		64
#define BT459_CURW		64
#define BT459_CURSORBYTES	(BT459_CURH*BT459_CURW/4)
#define BT459_MAXX		1279
#define BT459_MAXY		1023
#define BT459_X			31
#define BT459_Y			31
#define BT459_CURMAXX		(BT459_MAXX+BT459_X)
#define BT459_CURMAXY		(BT459_MAXY+BT459_Y)
#define BT459_CURMINX		(1-BT459_CURW)
#define BT459_CURMINY		(1-BT459_CURH)

#define BT459_H			(370-BT459_X) /* Sony timings in STIC */
#define BT459_V			(37-BT459_Y)  /* ditto */
#define BT459_OFFX		(0)
#define BT459_OFFY		(1500)

#define BT459_SETLOADDR(V,A)	((bt459Regs *)V)->addr_lo = _DUPBYTE0(A);\
                                wbflush()
#define BT459_SETHIADDR(V,A)	((bt459Regs *)V)->addr_hi = _DUPBYTE1(A);\
                                wbflush()
#define BT459_SETADDR(V,A)	((bt459Regs *)V)->addr_hi = _DUPBYTE1(A);\
                                BT459_SETLOADDR(V,A)

#define GX_CURW		64		/* cursor width in bits		*/
#define GX_CURH		64		/* cursor height ditto		*/
#define GX_CURX		0		/* cursor pos relative to upper	*/
#define GX_CURY		0		/* left of cursor block		*/
#define GX_CURMAXX	(BT459_MAXX)
#define GX_CURMAXY	(BT459_MAXY)
#define GX_CURMINX	(1-GX_CURW)
#define GX_CURMINY	(1-GX_CURH)
#define GX_CURSORBYTES	(GX_CURH * GX_CURW/4)
#define GX_POS_CUR(X,Y)	gx_pos_cur((X)+1,(Y)+_TEXT_HEIGHT-4)
#define GX_CURBEG	BT459_CUR_RAM
#define GX_CUREND	(GX_CURBEG+(0x010*GX_CURH))


typedef struct gx_info {
	pmEventQueue qe;		/* event & motion queues	*/
	short	mswitches;		/* current value of mouse buttons */
	pmCursor tablet;		/* current tablet position	*/
	short	tswitches;		/* current tablet buttons NI!	*/
	pmCursor cursor;		/* current cursor position	*/
	short	row;			/* screen row			*/
	short	col;			/* screen col			*/
	short	max_row;		/* max character row		*/
	short	max_col;		/* max character col		*/
	short	max_x;			/* max x position		*/
	short	max_y;			/* max y position		*/
	short	max_cur_x;		/* max cursor x position 	*/
	short	max_cur_y;		/* max cursor y position	*/
	int	version;		/* version of driver		*/
#define GX_F_NEW_CMAP	0x40000000	/* plz update VDAC colormap	*/
#define GX_F_VIDEO_ON	0x10000000	/* screen saver disabled	*/
#define GX_F_CMAP_LEN	0x000000ff      /* # entries updatable / vsync  */
	int	flags;			/* sync flags			*/
	int	*gram;		        /* SRAM on graphics board	*/
	int	*rb_addr;		/* ring buffer vaddr		*/
	int	rb_phys;		/* ring buffer phys addr	*/
	int	rb_size;		/* ring buffer char length	*/
	pmCursor mouse;			/* atomic read/write		*/
	pmBox	mbox;			/* atomic read/write		*/
	short	mthreshold;		/* mouse motion parameter	*/
	short	mscale;			/* mouse scale factor (if 
					   negative, then do square).	*/
	short	min_cur_x;		/* min cursor x position	*/
	short	min_cur_y;		/* min cursor y position	*/
	char	*gxo;			/* board base addr		*/
	char	stamp_width;
	char	stamp_height;
	char	nplanes;		/* primary buffer		*/
	char	n10_present;		/* geometry accelerator		*/
	char	dplanes;		/* double buffer		*/
	char	zplanes;		/* Z buffer			*/
	char	zzplanes;		/* extra buffer - high-end only */
	u_char	curs_bits[GX_CURSORBYTES];
	int	curs_fg;		/* fg rgb			*/
	int	curs_bg;		/* bg rgb			*/
	u_short	cmap_index;		/* start CMAP update here	*/
	u_short	cmap_count;		/* update this # of CMAP entries*/
	int	colormap[256];

	/* 2D - r3000 polls stic */
	int	*stic_dma_rb;		/* STIC polling register	*/
	int	*stic_reg;		/* STIC control registers	*/
					/* (may not be mapped)		*/
	/* 3D - co-processor support (not mapped to user)		*/
	int	ptpt_phys;		/* phys addr: ptpt		*/
	int	ptpt_size;		/* sizeof(ptpt) in entries	*/
	int *	ptpt_pgin;		/* request (server) pagein	*/

	/* helpful info for (3D) performance tuning (8 words alloc'd)	*/
	u_int	host_idle;		/* time in cpu[0] idle state	*/
	u_int	host_idleCount;		/* (see cpudata.h)		*/
	int	endpad[6];
} gxInfo;
#define GX_info		gxInfo


typedef struct gx_infos {
    int		pid;		     /* last pid using this entry */
    gxInfo	*shmat;		     /* where attached */
    gxInfo	info;		     /* the stuff... */
} gxInfos;


/*
 * These need to be mapped into user space.   I've glumped these
 * together to save on resources, etc.  There's too many of these
 * little rinky-dink things...
 *
 * The 'ringbufferoffset' field is defined to be the offset in ints
 * (i.e. 32-bit quantities) from the start of gxPriv to the ring
 * buffer.  In the 2DA, this may be some distance from tcs because
 * of alignment restrictions.
 *
 * WARNING: do *not* change gxPriv without also consulting ga.c:ga_dummy().
 */
#ifdef GX_MULTIOPEN
#      define GX_MAX_INFOS	(8)
#else
#      define GX_MAX_INFOS	(1)
#      define gx_info_get(N)	(0)
#endif

#define GX_ERR_NOOP	-1
#define GX_ERR_NONE	0
#define GX_ERR_PRIV	1
#define GX_ERR_GQO	2
#define GX_ERR_GAO	3
#define GX_ERR_POLL	4
#define GX_ERR_STIC	5
#define GX_ERR_SRAM	6

typedef struct gx_priv {
    gxInfos		infos[GX_MAX_INFOS];
    pmEvent		events[PMMAXEVQ];
    pmTimeCoord		tcs[MOTION_BUFFER_SIZE];
    int			ringbufferoffset;	/* in ints, not bytes */
} gxPriv;

#define gx_events	gx_priv->events
#define gx_tcs		gx_priv->tcs
#define gx_cursor	gxp->curs_bits
#define gx_colormap	gxp->colormap
#define gx_rboffset	gx_priv->ringbufferoffset
#define gx_ringbuffer	((int *)gx_priv + gx_rboffset)

#define gx_infos	gx_priv->infos
#define gx_server	gx_infos[0]
#define gx_info		gx_server.info


typedef struct gx_kpcmd {
	char nbytes;		/* number of bytes in parameter */
	unsigned char cmd;	/* command to be sent, peripheral bit will */
				/* be forced by driver */
	unsigned char par[2];	/* bytes of parameters to be sent */
} gxKPCmd;
#define GX_kpcmd	gxKPCmd


/*
 * Keyboard state structure
 */
typedef struct gx_keyboard {
    int shift;			/* state variables	*/
    int cntrl;
    int lock;
    int hold;
    char last;			/* last character	*/
} gxKeyboard;


typedef struct _MyColorMap {
	short  Map;
	unsigned short index;
	struct {
		unsigned short red;
		unsigned short green;
		unsigned short blue;
	} Entry;
} MyColorMap;


/*
 * CAUTION:
 *	The numbers of these ioctls must match
 *	the ioctls in qvioctl.h
 */
#define QIOCGINFO 	_IOR('q', 1, gxInfo *) /* get the info	 */
#define QIOCGXSTATE	_IOW('q', 2, pmCursor) /* set mouse pos */
#define QIOWCURSORCOLOR _IOW('q', 3, unsigned int[6]) /* bg/fg r/g/b */
#define QIOCINIT	_IO('q', 4)    /* init screen */
#define QIOCKPCMD	_IOW('q', 5, gxKPCmd) /* keybd. per. cmd */
#define QIOCADDR	_IOR('q', 6, gxInfo *) /* get phys address */
#define QIOWCURSOR	_IOW('q', 7, short[32])
#define QIOKERNLOOP	_IO('q', 8)	/* re-route kernel console output */
#define QIOKERNUNLOOP	_IO('q', 9)	/* dont re-rte kernel console output */
#define QIOVIDEOON	_IO('q', 10)   /* turn on the video */
#define QIOVIDEOOFF	_IO('q', 11)   /* turn off the video */
#define QIOSETCMAP      _IOW('q', 12, MyColorMap)
#define QIOWLCURSOR	_IOW('q', 13, char[1024]) /* 3MAX/CFB 64x64 cursor */
#define QIO_WCCOLOR	_IO('q', 14)   /* 3MAX/nDA cursor colors */
#define QIO_WCURSOR	_IO('q', 15)   /* 3MAX/nDA cursor */
#define QIO_SETCMAP     _IO('q', 16)   /* 3MAX/nDA colormap */
#define QIO_N10RESET	_IO('q', 17)   /* 3MAX/3DA halt N10 */
#define QIO_N10START	_IO('q', 18)	/* 3MAX/3DA start N10 */
#define QIO_N10INTR	_IOWR('q', 19, int) /* 3MAX/3DA intr N10 */
#define	QD_KERN_UNLOOP	_IO('g', 21)   /* RESERVED for DIGITAL, DON'T CHANGE */

#endif _gx_h_
