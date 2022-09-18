#ifndef lint
static	char	*sccsid = "@(#)fg.c	4.1	(ULTRIX)	7/2/90";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1985, 86, 87 by			*
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
 *   This software is  derived  from  software  received  from  the	*
 *   University    of   California,   Berkeley,   and   from   Bell	*
 *   Laboratories.  Use, duplication, or disclosure is  subject  to	*
 *   restrictions  under  license  agreements  with  University  of	*
 *   California and with AT&T.						*
 *									*
 *   The information in this software is subject to change  without	*
 *   notice  and should not be construed as a commitment by Digital	*
 *   Equipment Corporation.						*
 *									*
 *   Digital assumes no responsibility for the use  or  reliability	*
 *   of its software on equipment which is not supplied by Digital.	*
 *									*
 ************************************************************************/

/***********************************************************************
 *
 * Modification History:
 *
 * 06-Mar-90 -- jaw
 *	add missing splx's.
 *
 * 01-Feb-90 -- rafiey (Ali Rafieymehr)
 *	Common area (fg_comm_base) was not initialized before being used
 *	in interrupt routine (fgiint). This caused a panic when mouse was
 *	moved while server was coming up.
 *
 * 15-Dec-89 -- Alan Frechette
 *	Changes to "ws_display_type" and "ws_display_units".
 *
 * 23-Jul-89 -- Randall Brown
 *	In the default case of an ioctl() call, check the return value
 *	from ttioctl() to see if the cmd was invalid.  If it was, return
 *	an error.
 *
 * 08-May-89 -- rafiey (Ali Rafieymehr)
 *	When going to physical mode (crash dump) the video could be turned
 *	off by the server (screen saver). Therefore we turn the video on.
 *
 * 08-May-89 -- rafiey (Ali Rafieymehr)
 *	When going to single user mode, the keys would behave as if the
 *	<Ctrl> was also pressed. We now clear the flags for shift and
 *	control in fgclose routine.
 *
 * 08-May-89 -- rafiey (Ali Rafieymehr)
 *	The input interrupt routine (fgiint) was printing error messages when
 *	the event queue was full (in high IPL). These error messages are not
 *	really usefull.	The solution is to increase the size of the event
 *	queue (this should be done in the next release). For now there is no
 *	need to print the error	messages, therefore the "overflow" print
 *	statements are commented out.
 *
 * 12-Jan-89 - jaw
 *	merge Xe changes for FFox drivers
 *
 * 28-Dec-88 -- carito (Allen Carito)
 *      Fixed random character patterns appearing while during input
 *      session of fg driver by clearing the display area of the frame
 *      buffer during close point of driver.
 *
 * 12-Dec-88 -- carito (Allen Carito)
 *      Fixed the cases where the Keyboard lock LED failed to light and 
 *      keyboard bell failed to sound.  The fix required FG_PRGKBD ioctl 
 *      to output cmdbuf->param1.  The code has previously been outputting
 *      cmdbuf->cmd.  Also removed fgacc... symbols.
 *
 * 23-Nov-88 -- darrell (for Allen Carito)
 *	Update the console cursor positon to track this drivers idea of
 *	the cursor position whenever we move the cursor.
 *
 * 11-Nov-88  -- darrell (for Allen Carito)
 *      Cleanup, deleted commented out source code, debugging hooks.
 *	Add LEGSS physical address to be used when running with
 *	memory management disabled.
 *
 * 21-Sep-88  -- carito (Allen Carito)
 *      Added VOC table preservation code, fixed TOY macro, added save/restore.
 *      Note, the save/restore does not work properly.  It is not clear where
 *      (ie. server(Xgb) or firmware or driver(fg.c).  The current sympton is
 *      when the machine is halt the processor properly recovers, the display
 *      does not.  For some reason the mouse also becomes inactive.  Using
 *      the "It's bettern than nothing!", the save/restore function will be
 *      included anyway. 
 *
 * 13-Sep-88  -- rafiey (Ali Rafieymehr)
 *	Fixed a bug which was causing the "select" not to work for
 *	alternate console.
 *
 * 18-Jan-88  -- rafiey (Ali Rafieymehr)
 *      Created the Firefox driver.
 *
 * 02-Sep-88  -- rafiey (Ali Rafieymehr) for Allen Carito and Paul Jensen
 *	Added require changes for the DVT pass2 units. Also made changes
 *	to correct the cursor bug.
 *
 **********************************************************************/

#if NFG > 0 || defined(BINARY)
#include "../data/fg_data.c"	/* include external references to data file */

/* Talk to Darrell about including fcreg.h here */
#include "fcreg.h"

/*
 * Firefox LEGSS register offsets 
 */

#define	BOOT_ROM	0x00000000    /* Diagnostic/boot rom */
#define	WEITEK 		0x00100000    /* Weitek code memory */
#define	DRAM		0x00400000    /* Dram */
#define	I_VRAM		0x00800000    /* I-MODE Vram */
#define	X_VRAM		0x01800000    /* X-MODE Vram */
#define	I_PATTERN	0x01900000    /* Pattern_I memory */
#define	X_PATTERN	0x01904000    /* Pattern_X memory */
#define	TCHIP		0x01A00000    /* T chip */
#define	DCHIP		0x01B00000    /* D chip */
#define	ACHIP		0x01C00000    /* A chip */
#define	ALPHA		0x01D00000    /* Alpha memory */
#define	FBIC		0x01FFFFC4    /* Fbic */

/*
 * Firefox LEGSS physical register addresses
 */

#define FG_PHYS_FGBASE  0x3e000000
#define FG_PHYS_BOOT_ROM  FG_PHYS_FGBASE
#define FG_PHYS_WEITEK_CODE  FG_PHYS_FGBASE + WEITEK
#define FG_PHYS_DRAM  FG_PHYS_FGBASE + DRAM		
#define FG_PHYS_I_VRAM	  FG_PHYS_FGBASE + I_VRAM	
#define FG_PHYS_X_VRAM	  FG_PHYS_FGBASE + X_VRAM	
#define FG_PHYS_I_PATTERN  FG_PHYS_FGBASE + I_PATTERN
#define FG_PHYS_X_PATTERN  FG_PHYS_FGBASE + X_PATTERN
#define FG_PHYS_TCHIP	  FG_PHYS_FGBASE + TCHIP	
#define FG_PHYS_DCHIP	  FG_PHYS_FGBASE + DCHIP	
#define FG_PHYS_ACHIP	  FG_PHYS_FGBASE + ACHIP	
#define FG_PHYS_ALPHA	  FG_PHYS_FGBASE + ALPHA	
#define FG_PHYS_FBIC	  FG_PHYS_FGBASE + FBIC	

/*
 * CTSI definitions
 */

#define FG_TABLE_X 1280
#define FG_TABLE_Y 974
#define FG_TABLE_WIDTH 2048-1280
#define FG_TABLE_HEIGHT 25
#define FG_CTSITABLE_X 1280
#define FG_CTSITABLE_Y 999
#define FG_CTSITABLE_WIDTH FG_TABLE_WIDTH
#define FG_CTSITABLE_HEIGHT FG_TABLE_HEIGHT
#define FG_CTSIDSPLY_X 1280 /* 0 or 1280 */
#define FG_CTSIDSPLY_Y 686  /* 0 or 686 */
#define FG_CTSIDSPLY_W 768  /* 1280 or 768 */
#define FG_CTSIDSPLY_H 286  /* 1024 or 286 */

/*
 * Gvax defintions, note the following defintions must match the 
 * definition of FG_CONSOLE_OFF and FG_SCB_OFF, respectively, 
 * in decwgb_dram.h
 */

#define FG_CONSOLE_OFF  1043456
#define FG_CONSOLE		(DRAM + FG_CONSOLE_OFF)
#define FG_SCB_OFF	753664
#define FG_SCB		(DRAM + FG_SCB_OFF)

/*
 * Tchip definitions
 */

#define TIMING_TL1_OFF		    3
#define TIMING_TL2_OFF		    7
#define TIMING_WIDTH_MASK	    7
#define TIMING_TL1_MASK		 0x78
#define TIMING_TL2_MASK        0x3F80
#define	TIMING_HI_OFF		   11
#define	TIMING_HI_MASK		0x7FF
#define	TIMING_XBS_MASK		0x7FF

/*
 * Miscellaneous definitions
 */

#define FG_PHYSNEXUS 0x20080000
#define SGPRIOR (PZERO-1)               /* must be negative */
#define FALSE 0
#define TRUE 1
#define CHAR_S 0xc7
#define CHAR_Q 0xc1
#define FG_MAJOR 59




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

/*
 * macro to create a system virtual page number from system virtual adrs
 */

#define VTOP(x)  (((int)x & ~0xC0000000) >> PGSHIFT) /* convert address */
						     /* to system page # */
/* 
 * macro to convert system virtual to page table 
*/

#define SVTOPTE(v) (&Sysmap[btop((int)(v) & ~VA_SYS)])

/*
 * General macroes
 */

#define FG_BROOK_LOW( addr )       ( 0 | ((addr&0x00FF) << 4) )
#define FG_BROOK_HIGH( addr )      ( 1 | ((addr&0xFF00) >> 4) )
#define Fixed( num ) ( (int)((float)num * 65536.0) )
#define FONT_OFFSET	((MAX_SCREEN_X/CHAR_WIDTH)*CHAR_HEIGHT)
#define KBD_INIT_LENGTH	sizeof(fg_kbdinitstring)/sizeof(short)
#define TOY ((time.tv_sec * 1000) + (time.tv_usec / 1000))

/*
 * Global variables and structures
 */

/* 
 * Physical pointers to LEGSS physical registers
 */
struct achip *fg_phys_achip;
struct dchip *fg_phys_dchip;
struct tchip *fg_phys_tchip;
struct fbic *fg_phys_fbic;
struct fc_regs *fg_phys_fcaddr;

/*
 * v_consputc is the switch that is used to redirect the console cnputc to the
 * virtual console  vputc.  v_consgetc is the  switch that is used to redirect
 * the console getchar to the virtual console vgetc.
 */
extern (*v_consputc)();
extern (*v_consgetc)();

/*
 * Keyboard state
 */
struct fg_keyboard {
	int shift;			/* state variables	*/
	int cntrl;
	int lock;
	int hold;
	char last;			/* last character	*/
} fg_keyboard;

short fg_divdefaults[15] = { LK_DOWN,	/* 0 doesn't exist */
	LK_AUTODOWN, LK_AUTODOWN, LK_AUTODOWN, LK_DOWN,
	LK_UPDOWN,   LK_UPDOWN,   LK_AUTODOWN, LK_AUTODOWN, 
	LK_AUTODOWN, LK_AUTODOWN, LK_AUTODOWN, LK_AUTODOWN, 
	LK_DOWN, LK_AUTODOWN };

short fg_kbdinitstring[] = {		/* reset any random keyboard stuff */
	LK_AR_ENABLE,			/* we want autorepeat by default */
	LK_CL_ENABLE,			/* keyclick */
	0x84,				/* keyclick volume */
	LK_KBD_ENABLE,			/* the keyboard itself */
	LK_BELL_ENABLE,			/* keyboard bell */
	0x84,				/* bell volume */
	LK_LED_DISABLE,			/* keyboard leds */
	LED_ALL };

struct fgcurstab {
	short x;			/* cursor x in screen coords */
	short y;			/* cursor y in screen coords */
	short hot_x;			/* offset from hotspot to curs origin */
	short hot_y;			/* offset from hotspot to curs origin */
	union {
		struct {
			short pat_x;	/* cursor pat ULH x-coord */
			short pat_y;	/* cursor pat ULH x-coord */
		} coords;
		int position;
	} XY;
	} fgcurstab;


struct fgcurstabupd fgcurstabupd;

typedef struct _fg_commarea {
    char gvax_code [2048];
    unsigned int request_queue [2]; /* interlocked queue of requests  */
    unsigned int free_queue [2];/* queue of free packets            */
    unsigned short int size;    /* The size of the comunication area  */
    unsigned char type;         /* Global system identification     */
    unsigned char sub_type;     /* Legss subsystem identifier       */
    unsigned long int HOST_waiting; /* Host waiting for GVAX to set */
    unsigned long int GVAX_waiting;
    unsigned long int GVAX_heartbeat;
    struct  {
        unsigned char cursor_update; /* need to update cursor       */
        unsigned char vdac_type;/* LVDAC or Bt461                   */
        char filler [2];
        } HG_flags;
    struct  {
        unsigned char error;    /* Gvax error                       */
        char filler [3];
        } GH_flags;
    struct  {
        unsigned char XY_position; /* load new cursor pos           */
        unsigned char color;    /* load new cursor color            */
        unsigned char pattern;  /* load new cursor pattern          */
        unsigned char filler;
        } Lego;
    unsigned long int XY_position; /* new cursor position           */
    unsigned short int cursor_red_fg; /* Red cursor foreground      */
    unsigned short int cursor_green_fg; /* Green cursor foreground  */
    unsigned short int cursor_blue_fg; /* Blue cursor foreground    */
    unsigned short int cursor_red_bg; /* Red cursor background      */
    unsigned short int cursor_green_bg; /* Green cursor background  */
    unsigned short int cursor_blue_bg; /* Blue cursor background    */
    unsigned long int cursor_mask [32]; /* The cursor mask.         */
    unsigned long int cursor_image [32]; /* The cursor image.       */
    } fg_commarea;

struct _fg_commarea *fg_comm_area;


int	fg_ipl_lo = 1;			/* IPL low flag			*/


extern	u_short	sm_pointer_id;	/* id of pointer device (mouse,tablet)-ss.c */
u_short	fg_mouseon = 0;                	/* Mouse is enable when 1 */
u_short	fg_open = 0;			/* graphics device is open when 1 */
int	save_logic[4];
u_long fg_fgpixel = -1;
u_long fg_bgpixel = 0;

/**************************************************************/
struct fgctsi *fgctsi;
int     fg_physmode = 0;
int	save_cursor[2];
int	ioctl_array[64];
int	ioctl_counter = 0;
int	fbic_exists = 0;
int	fg_num_planes = 0;
int	Mixed_up_planes = 0;
int	fg_lego_p0_mask;
int	fg_lego_p0;
int	fg_wc_offset;
int	fg_Wplanes;
int	Width = 0;
int	Height = 0;
/**************************************************************/
int	fgdebug = 0;

struct	uba_device *fgdinfo[NFG];
struct	mouse_report last_rep;
extern	struct	mouse_report current_rep;	/* now in ss.c */
extern	struct	tty	sm_tty;			/* now in ss.c */
extern	struct	tty	fc_tty[];

/*
 * Definition of the driver for the auto-configuration program.
 */
int	fgprobe(), fgattach(), fgvint(), fgaint(), fgiint();

u_short	fgstd[] = { 0 };
struct	uba_driver fgdriver =
	{ fgprobe, 0, fgattach, 0, fgstd, "fg", fgdinfo };

struct proc *rsel;			/* process waiting for select */

int	fgstart(), fgputc(), fggetc(), ttrstrt();
long	fg_save(), fg_restore();
u_long  fg_savebuf[100];
int  fg_savei;
u_long *fg_saveaddr;

/*
 * Keyboard translation and font tables
 */
extern  char *q_special[],q_font[];
extern  u_short q_key[],q_shift_key[];
extern  u_short fg_font[];


extern	struct	nexus	nexus[];

/*
 * Default cursor (plane A and plane B)
 *
 */


u_long fg_def_cursor[] = { 

        /* plane A */
        0x00000fff, 0x00000fff, 0x00000fff, 0x00000fff, 
        0x00000fff, 0x00000fff, 0x00000fff, 0x00000fff, 
        0x00000fff, 0x00000fff, 0x00000fff, 0x00000fff, 
        0x00000fff, 0x00000fff, 0x00000fff, 0x00000fff, 
        0x00000fff, 0x00000fff, 0x00000fff, 0x00000fff, 
        0x00000fff, 0x00000000, 0x00000000, 0x00000000, 
	0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000,

        /* plane B, same as plane A */
        0x00000fff, 0x00000fff, 0x00000fff, 0x00000fff, 
        0x00000fff, 0x00000fff, 0x00000fff, 0x00000fff, 
        0x00000fff, 0x00000fff, 0x00000fff, 0x00000fff, 
        0x00000fff, 0x00000fff, 0x00000fff, 0x00000fff, 
        0x00000fff, 0x00000fff, 0x00000fff, 0x00000fff, 
        0x00000fff, 0x00000000, 0x00000000, 0x00000000, 
	0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000,
	};


int	num_planes;
int	mixed_up_planes = -1;
int	fgred = 255;
int	fggreen = 255;
int	fgblue = 255;;

/*******************************************************************
 **                                                               **
 **  Routine to see if the graphic device will interrupt.         **
 **                                                               **
 *******************************************************************/

extern	int	ws_display_type;
extern	int	ws_display_units;

fgprobe(reg)
	caddr_t reg;
{
        register struct nb5_regs *fgaddr = (struct nb5_regs *)fgmem;
	register struct achip *fgachip;
	register struct dchip *fgdchip;
	register struct tchip *fgtchip;

        if (v_consputc != fgputc) {
	    fgbase = (caddr_t) ((u_long)fgaddr);
	    fgmap.boot_rom = fgbase;
	    fgmap.weitek_code = fgbase + WEITEK;
	    fgmap.dram = fgbase + DRAM;
            fgmap.achip = fgbase + ACHIP;
            fgmap.dchip = fgbase + DCHIP;
            fgmap.tchip = fgbase + TCHIP;
            fgmap.fbic = fgbase + FBIC;
	    fgmap.i_vram = fgbase + I_VRAM;
	    fgmap.x_vram = fgbase + X_VRAM;
	    fgmap.i_pattern_mem = fgbase + I_PATTERN;
	    fgmap.x_pattern_mem = fgbase + X_PATTERN;
	    fgmap.console = fgbase + FG_CONSOLE;
	    fgmap.scb = fgbase + FG_SCB;
            fg_config();

	    fgachip = (struct achip *) fgmap.achip;
	    fgdchip = (struct dchip *) fgmap.dchip;
	    fgtchip = (struct tchip *) fgmap.tchip;

	    fg_init_achip(fgachip);
	    fg_init_dchip(fgdchip);
	    fg_init_tchip(fgtchip);
	    fg_clip(0, 0, 2047, 2047);
	    fg_clr_screen();
	    fgtchip->tchip_csr |= TCHIP_UNBLANK;
	    fg_init_shared();
	}

/* Darrell said we don't need next two lines  */
	cvec = 0x248;
	br = 0x14;

	return(sizeof(int));
}









/******************************************************************
 **                                                              **
 ** Routine to attach to the graphic device.                     **
 **                                                              **
 ******************************************************************/

fgattach(ui)
	struct uba_device *ui;
{
	register int *pte;
	int	i;

/*
 * init "fgflags"
 */
        fgflags.inuse = 0;              /* init inuse variable EARLY! */
        fgflags.mapped = 0;
        fgflags.kernel_loop = -1;       /* default is now kern_loop on */
        fgflags.curs_acc = ACC_OFF;
        fgflags.curs_thr = 128;
        fgflags.tab_res = 2;            /* default tablet resolution factor */
        fgflags.duart_imask = 0;        /* init shadow variables */


/*
 * init structures used in kbd/mouse interrupt routine.	This code must
 * come after the "sg_init_shared()" routine has run since that routine inits
 * the eq_header structure used here.
 */

/* init the "latest mouse report" structure */

	last_rep.state = 0;
	last_rep.dx = 0;
	last_rep.dy = 0;
	last_rep.bytcnt = 0;

/* init the event queue (except mouse position) */

	eq_header->header.events = (struct _vs_event *)
				  ((int)eq_header + sizeof(struct fginput));

	eq_header->header.size = MAXEVENTS;
	eq_header->header.head = 0;
	eq_header->header.tail = 0;

/* init single process access lock switch */

	fg_open = 0;

/*
 * Map the bitmap for use by users.
 */

	pte = (int *)(FGMEMmap[0]);
	for( i=0 ; i<65536 ; i++, pte++ )
		*pte = (*pte & ~PG_PROT) | PG_UW | PG_V;

	pte = (int *)(FGCTSIXSmap);
	*pte = (*pte & ~PG_PROT) | PG_UW | PG_V;
}





/******************************************************************
 **                                                              **
 ** Routine to open the graphic device.                          **
 **                                                              **
 ******************************************************************/

extern struct pdma fcpdma[];
extern	int ssparam();

/*ARGSUSED*/
fgopen(dev, flag)
	dev_t dev;
	int   flag;
{
        register int unit = minor(dev);
        register struct tty *tp;
        register struct nb_regs *sgiaddr = (struct nb_regs *)nexus;

/*
 * The graphics device can be open only by one person
 */
        if (unit == 1) {
if (fgdebug)
	mprintf("Open mouse fgmouse = %d unit = %d\n",fg_mouseon,unit);
            if (fg_open != 0)
                return(EBUSY);
            else
                fg_open = 1;
            fgflags.inuse |= GRAPHIC_DEV;  /* graphics dev is open */
        } else {
            fgflags.inuse |= CONS_DEV;  /* mark console as open */
        }
        if ((unit == 2) && (major(dev) == CONSOLEMAJOR))
            tp = &sm_tty;
        else
            tp = &fc_tty[unit];

	if (tp->t_state&TS_XCLUDE && u.u_uid!=0)
	    return (EBUSY);
	tp->t_addr = (caddr_t)&fcpdma[unit];
	tp->t_oproc = fgstart;

	/*---------------------------------------------------------------------
	* Look at the compatibility mode to specify correct default parameters
	* and to insure only standard specified functionality. */
	if ((u.u_procp->p_progenv == A_SYSV) || 
		(u.u_procp->p_progenv == A_POSIX)) {
		flag |= O_TERMIO;
		tp->t_line = TERMIODISC;
	}
	if ((tp->t_state&TS_ISOPEN) == 0) {
	    ttychars(tp);
	    tp->t_state = TS_ISOPEN|TS_CARR_ON;
	    tp->t_cflag = tp->t_cflag_ext = B4800;
	    tp->t_iflag_ext = 0;
	    tp->t_oflag_ext = 0;
	    tp->t_lflag_ext = 0;
	    if( unit == 0 ) {
			/*----------------------------------------------------
			* Ultrix defaults to a "COOKED" mode on the first
			* open, while termio defaults to a "RAW" style.
			* Base this decision by a flag set in the termio
			* emulation routine for open, or set by an explicit
			* ioctl call. */
			
			if ( flag & O_TERMIO ) {
				/*--------------------------------------
				* Provide a termio style environment.
				* "RAW" style by default. */
				
				tp->t_flags = RAW;   
				tp->t_iflag = 0;
				tp->t_oflag = 0;
				tp->t_cflag |= CS8|CREAD|HUPCL; 
				tp->t_lflag = 0;
	
				/*-------------------------------------
				 * Change to System V line discipline.*/
				 
				tp->t_line = TERMIODISC;
				/*-----------------------------------------
				* The following three control chars have 
				* different default values than ULTRIX.	*/
				
	 			tp->t_cc[VERASE] = '#';
	 			tp->t_cc[VKILL] = '@';
	 			tp->t_cc[VINTR] = 0177;
 				tp->t_cc[VMIN] = 6;
 				tp->t_cc[VTIME] = 1;
			} else {
				/*--------------------------------------
				* Provide a backward compatible ULTRIX 
				* environment.  "COOKED" style.	*/
				
				tp->t_flags = IFLAGS;
				tp->t_iflag = IFLAG;
				tp->t_oflag = OFLAG;
				tp->t_lflag = LFLAG;
				tp->t_cflag |= CFLAG;
			}
	    }
	    else {
		tp->t_flags = RAW;
		tp->t_iflag = 0;
		tp->t_oflag = 0;
		tp->t_cflag |= CS8|CREAD|HUPCL; 
		tp->t_lflag = 0;
	    }
	    if(tp != &sm_tty)
		fcparam(unit);
	    else
                    tp->t_iflag |= IXOFF;	/* flow control for qconsole */
	}

/*
 * Process line discipline specific open if its not the mouse.
 */
	if (unit != 1)
	    return ((*linesw[tp->t_line].l_open)(dev, tp));
	else {
	    fg_mouseon = 1;
if (fgdebug)
mprintf("Exiting from open fg_mouse = %d  Unit = %d\n",fg_mouseon, unit);
	    return(0);
	}
}


/******************************************************************
 **                                                              **
 ** Routine to close the graphic device.                         **
 **                                                              **
 ******************************************************************/

/*ARGSUSED*/
fgclose(dev, flag)
	dev_t dev;
        int   flag;
{
        register struct tty *tp;
        register int unit = minor(dev);
	register struct achip *fgachip;
	register struct dchip *fgdchip;
	register struct tchip *fgtchip;
	register struct fbic *fgfbic;

        unit = minor(dev);
	if (fgdebug)
	  mprintf("Close Unit = %d mouse = %d\n",unit, fg_mouseon);
        if ((unit == 2) && (major(dev) == CONSOLEMAJOR))
            tp = &sm_tty;
        else
            tp = &fc_tty[unit];
/*
 * If unit is not the mouse call the line disc. otherwise clear the state
 * flag, and put the keyboard into down/up.
 */
        if( unit != 1 ){
            (*linesw[tp->t_line].l_close)(tp);
            ttyclose(tp);
            fgflags.inuse &= ~CONS_DEV;
	    fg_keyboard.cntrl = fg_keyboard.shift = 0;
        } else {
            fg_mouseon = 0;
            if (fg_open != 1)
                return(EBUSY);
            else
                fg_open = 0;     /* mark the graphics device available */
            fgflags.inuse &= ~GRAPHIC_DEV;
/*
 * should setup the hardware again
 */
	    fgachip = (struct achip *) fgmap.achip;
	    fgdchip = (struct dchip *) fgmap.dchip;
	    fgtchip = (struct tchip *) fgmap.tchip;
            fgfbic  = (struct fbic *) fgmap.fbic;

	    if ((fgfbic->fbic_fbicsr & BIT_13) == 0) {
	      fgfbic->fbic_fbicsr &= ~HALTCPU;	 /* for pass-2 chips */
	      fgfbic->fbic_fbicsr |= HALT_ENB;   /* enable halt */
	      fgfbic->fbic_fbicsr &= ~BIT_13;    /* clear bit 13 */
	      fgfbic->fbic_fbicsr |= HALTCPU;    /* halt GVAX */
	      DELAY(100);                        /* wait for bit 13 */
	    }
#ifndef XX_NOINIT_LEGSS
	    fg_init_achip(fgachip);
	    fg_init_dchip(fgdchip);
	    fg_init_tchip(fgtchip);
#endif XX_NOINIT_LEGSS
	    fg_clip(0, 0, 2047, 2047);
	    fg_clr_screen();
	    fgtchip->tchip_csr |= TCHIP_UNBLANK;
	    fg_init_shared();

            cursor.x = 0;
            cursor.y = 0;
	  }

        tp->t_state = 0;
        /* Remove termio flags that do not map */
        tp->t_iflag &= ~TERMIO_ONLY_IFLAG;
        tp->t_oflag &= ~TERMIO_ONLY_OFLAG;
        tp->t_cflag &= ~TERMIO_ONLY_CFLAG;
        tp->t_lflag &= ~TERMIO_ONLY_LFLAG;

if (fgdebug)
mprintf("Exiting Close Unit = %d mouse = %d\n",unit, fg_mouseon);

}



/******************************************************************
 **                                                              **
 ** Routine to read from the graphic device.                     **
 **                                                              **
 ******************************************************************/

extern fg_strategy();

fgread(dev, uio)
dev_t dev;
struct uio *uio;
{
        register struct tty *tp;
        register int minor_dev;
        register int unit;

        minor_dev = minor(dev);
        unit = (minor_dev >> 2) & 0x07;

/* If this is the console... */

        if ((minor_dev & 0x03) != 1  &&
             fgflags.inuse & CONS_DEV) {
            if ((minor_dev == 2) && (major(dev) == CONSOLEMAJOR))
                tp = &sm_tty;
            else
                tp = &fc_tty[minor_dev];
            return ((*linesw[tp->t_line].l_read)(tp, uio));
        }

/*
 * else this must be a FIFO xfer from user space
 */

}


/******************************************************************
 **                                                              **
 ** Routine to write to the graphic device.                      **
 **                                                              **
 ******************************************************************/

extern fg_strategy();

fgwrite(dev, uio)
	dev_t dev;
	struct uio *uio;
{
        register struct tty *tp;
        register int minor_dev;
        register int unit;

        minor_dev = minor(dev);
        unit = (minor_dev >> 2) & 0x07;

/* If this is the console... */

        if ((minor_dev & 0x03) != 1  &&
             fgflags.inuse & CONS_DEV) {
            if ((minor_dev == 2) && (major(dev) == CONSOLEMAJOR))
                tp = &sm_tty;
            else
                tp = &fc_tty[minor_dev];
            return ((*linesw[tp->t_line].l_write)(tp, uio));
        }

/*
 * else this must be a FIFO xfer from user space
 */

/*
        else if (sgflags.inuse & GRAPHIC_DEV) {
           return (physio(sg_strategy, &sgbuf[unit],
                           dev, B_WRITE, minphys, uio));
        }
*/

}





/******************************************************************
 **                                                              **
 ** Strategy routine to do FIFO                                  **
 **                                                              **
 ******************************************************************/

fg_strategy(bp)
register struct buf *bp;
{

}






/******************************************************************
 **                                                              **
 ** Mouse activity select routine.                               **
 **                                                              **
 ******************************************************************/

fgselect(dev, rw)
dev_t dev;
{
        register int s = spl6();
        register int unit = minor(dev);
        register struct tty *tp;

            switch(rw) {
            case FREAD:                                 /* event available */

        	if (unit == 1) {
                        if(!(ISEMPTY(eq_header))) {
                            splx(s);
                            return(1);
                        }
                        rsel = u.u_procp;
                        fgflags.selmask |= SEL_READ;
                        splx(s);
                        return(0);
		}
		else {
                    tp = &sm_tty;
                    if (ttnread(tp)) {
		    	splx(s);
                        return(1);
		    }
                    tp->t_rsel = u.u_procp;
                    splx(s);
                    return(0);
		}

            case FWRITE:                /* FIFO done? */

                if (unit == 1) {

                        rsel = u.u_procp;
                        fgflags.selmask |= SEL_WRITE;
                        splx(s);
                        return(0);
		}
		else {
                    tp = &sm_tty;
                    if (tp->t_outq.c_cc <= TTLOWAT(tp)) {
		        splx(s);
                        return(1);
		    }
                    tp->t_wsel = u.u_procp;
                    splx(s);
                    return(0);
		}
            }
}


/******************************************************************
 **                                                              **
 ** Graphic device ioctl routine.                                **
 **                                                              **
 ******************************************************************/

/*ARGSUSED*/
fgioctl(dev, cmd, data, flag)
	dev_t dev;
	int cmd;
	register caddr_t data;
{
	register int *ptep;		/* page table entry pointer */
        register struct _vs_event *vep;
        register struct tty *tp;
	register struct tchip *fgtchip;
	register struct dchip *fgdchip;
	register struct achip *fgachip;
	register struct fbic *fgfbic;
	register int unit = minor(dev);
        struct fgmap *fg;               /* pointer to device map struct */
	struct prgkbd *cmdbuf;
        struct prg_cursor *curs;

        struct _vs_cursor *pos;
	struct	fgcurstabupd *cur;
	int	i;
	int	s;
	int	error;


/*
 * service the Firefox device ioctl commands
 */

	switch (cmd) {

	    case FG_MAPDEVICE:
		fg = (struct fgmap *) &fgmap;
		bcopy(fg, data, sizeof(struct fgmap));
		break;

/*
 * give user write access to the event queue
 */

            case FG_MAPEVENT:

		fgflags.mapped |= MAPEQ;
                ptep = (int *) ((VTOP(eq_header) * 4)
                                + (mfpr(SBR) | 0x80000000));

                /* allow user write to 1K event queue */

                *ptep++ = (*ptep & ~PG_PROT) | PG_UW | PG_V;
                *ptep = (*ptep & ~PG_PROT) | PG_UW | PG_V;

                mtpr(TBIA, 0);                  /* clr CPU translation buf */

                /* return event queue address */

                *(int *)data = (int) eq_header;
                break;

	    case FG_GETEVENT:	/* extract the oldest event from event queue */

		if (ISEMPTY(eq_header)) {
		    vep = (struct _vs_event *) data;
		    vep->vse_device = VSE_NULL;
		    break;
		}

		vep = (struct _vs_event *) GETBEGIN(eq_header);
		s = spl6();
		GETEND(eq_header);
		splx(s);
		bcopy(vep, data, sizeof(struct _vs_event));
		break;


	    case FG_VIDEOON:		/* turn on the video */

        	fgtchip = (struct tchip *) fgmap.tchip;
		fgtchip->tchip_csr |= TCHIP_UNBLANK;
		break;

	    case FG_VIDEOOFF:		/* turn off the video */
        	fgtchip = (struct tchip *) fgmap.tchip;
		fgtchip->tchip_csr &= ~TCHIP_UNBLANK;
		break;

	    case FG_INITACHIP:		/* initialize achip */
        	fgachip = (struct achip *) fgmap.achip;
		fg_init_achip(fgachip);
		break;

	    case FG_INITDCHIP:		/* initialize dchip */
        	fgdchip = (struct dchip *) fgmap.dchip;
		fg_init_dchip(fgdchip);
		break;

	    case FG_INITTCHIP:		/* initialize tchip */
        	fgtchip = (struct tchip *) fgmap.tchip;
		fg_init_tchip(fgtchip);
		break;

            case FG_POSCURSOR:          /* position the mouse cursor */
		pos = (struct _vs_cursor *) data;
		fg_pos_cursor (pos->x, pos->y);
		eq_header->curs_pos.x = pos->x;
		eq_header->curs_pos.y = pos->y;
		fgcurstab.x = pos->x;
		fgcurstab.y = pos->y;
		break;

            case FG_PRGCURSOR:		/* set the cursor acceleration factor */

                curs = (struct prg_cursor *) data;
                s = spl6();
                fgflags.curs_acc = curs->acc_factor;
                fgflags.curs_thr = curs->threshold;
                splx(s);
                break;

            case FG_PRGKBD:  /* pass caller's programming commands to LK201 */

                cmdbuf = (struct prgkbd *)data;     /* pnt to kbd cmd buf */

                fg_key_out (cmdbuf->cmd);

/*
 * Send param1?
 */
                if (cmdbuf->cmd & LAST_PARAM)
                    break;
                fg_key_out (cmdbuf->param1);

/*
 * Send param2?
 */
                if (cmdbuf->param1 & LAST_PARAM)
                    break;
                fg_key_out (cmdbuf->param2);
                break;

            case FG_KERN_LOOP:          /* redirect kernel messages */
                fgflags.kernel_loop = -1;
                break;

            case FG_KERN_UNLOOP:        /* don't redirect kernel messages */

                fgflags.kernel_loop = 0;
                break;


	    case FG_RESET:	/* init driver variables */
		fg_init_shared();	/* init shared memory */
		fg_clr_screen();
		break;

	    case FG_SET:	/* init driver variables */
		fg_init_shared();	/* init shared memory */
		break;

	    case FG_CLRSCRN:	/* clear the screen */
		fg_clr_screen();
		break;


	    case FG_WTCURSOR:	/* load a cursor */
		fg_load_cursor(data);
		break;

	    case FG_HALTGVAX:	/* halt the GVAX */
		fgfbic = (struct fbic *) fgmap.fbic;

		if ((fgfbic->fbic_fbicsr & BIT_13) == 0) {
		  fgfbic->fbic_fbicsr &= ~HALTCPU;   /* for pass 2 chips */
		  fgfbic->fbic_fbicsr |= HALT_ENB;   /* enable halt */
		  fgfbic->fbic_fbicsr &= ~BIT_13;    /* clear bit 13 */
		  fgfbic->fbic_fbicsr |= HALTCPU;    /* halt CPU */
		  DELAY(100);			   /* wait for bit 13 */
		}
		break;

	    case FG_UNHALTGVAX:	/* unhalt the GVAX */
		fgfbic = (struct fbic *) fgmap.fbic;
		fgfbic->fbic_fbicsr &= ~HALTCPU;   /* unhalt CPU */
		fgfbic->fbic_fbicsr &= ~BIT_13;    /* clear bit 13 */
		DELAY(100);			   /* wait for bit 13 */
		break;

	    case FG_GETTIMING:	/* get tchip value */
		fg_get_tchip(data);
		break;

	    case FG_SETTIMING:	/* set tchip value */
		fg_set_tchip(data);
		break;

	    case FG_GETCOMMAREA:	/* get common area (shared memory) */
		
		fg_init_comm_area(data, 1); /* Initialize both common area and descriptor */
		break;

	    case FG_SETCURSTAB:	/* update fgcurstab */

		cur = (struct fgcurstabupd *) data;
		if (cur->flags & UPDATE_X)
			fgcurstab.x = cur->x;
		if (cur->flags & UPDATE_Y)
			fgcurstab.y = cur->y;
		if (cur->flags & UPDATE_HOTX)
			fgcurstab.hot_x = cur->hot_x;
		if (cur->flags & UPDATE_HOTY)
			fgcurstab.hot_y = cur->hot_y;

		eq_header->curs_pos.x = fgcurstab.x;
		eq_header->curs_pos.y = fgcurstab.y;

		fgcurstab.XY.coords.pat_x =
			fgcurstab.x - fgcurstab.hot_x + fg_wc_offset;
		fgcurstab.XY.coords.pat_y = fgcurstab.y - fgcurstab.hot_y;
/* load new cursor position */
		fg_comm_area->XY_position = fgcurstab.XY.position;

/* request position update */

		fg_comm_area->Lego.XY_position = 1;

/* request pattern color update */

		fg_comm_area->Lego.color = 1;

/* request pattern update */

		fg_comm_area->Lego.pattern = 1;

/* request cursor update */

		fg_comm_area->HG_flags.cursor_update = 1;
		break;

	    case FG_SETFGPIXEL:	/* update fgcurstab */
		fg_fgpixel = *data;
		break;

	    case FG_SETBGPIXEL:	/* update fgcurstab */
		fg_bgpixel = *data;
		break;

	    default:


		if ((unit == 2) && (major(dev) == CONSOLEMAJOR))
		  tp = &sm_tty;
		else
		  tp = &fc_tty[unit];
		error = (*linesw[tp->t_line].l_ioctl)(tp, cmd, data, flag);
		if (error >= 0)
		  return(error);
		
		error = ttioctl(tp, cmd, data, flag);
		if (error >= 0)
		  return(error);
	        /* if error = -1 then ioctl does not exist */
		if (u.u_procp->p_progenv == A_POSIX) 
			return (EINVAL);
		return (ENOTTY);
		    
		break;
		
	}

	return(0);

}



fg_get_tchip(tchip_info)
fg_timinginfo *tchip_info;
{

/* Get the value of the tchip registers */

	register struct tchip *fgtchip;

        fgtchip = (struct tchip *) fgmap.tchip;

	tchip_info->csr = fgtchip->tchip_csr;
	tchip_info->pipe = tchip_info->csr >> 13;
	tchip_info->interrupt = fgtchip->tchip_int_reg;
	tchip_info->display_x = fgtchip->tchip_x_start;
	tchip_info->display_y = fgtchip->tchip_y_start;
	tchip_info->table_x = fgtchip->tchip_table_x_start;
	tchip_info->table_y = fgtchip->tchip_table_y_start;
	tchip_info->table_width = fgtchip->tchip_table_cntl_reg & TIMING_WIDTH_MASK;
	tchip_info->table_L1 = (fgtchip->tchip_table_cntl_reg & TIMING_TL1_MASK)
			>> TIMING_TL1_OFF;
	tchip_info->table_L2 = (fgtchip->tchip_table_cntl_reg & TIMING_TL2_MASK)
			>> TIMING_TL2_OFF;
	tchip_info->VBS = fgtchip->tchip_mon_cntl_reg0 & TIMING_XBS_MASK;
	tchip_info->VBF = fgtchip->tchip_mon_cntl_reg1 & TIMING_XBS_MASK;
	tchip_info->VSS = fgtchip->tchip_mon_cntl_reg2 & TIMING_XBS_MASK;
	tchip_info->VSF = fgtchip->tchip_mon_cntl_reg3 & TIMING_XBS_MASK;
	tchip_info->HBS = fgtchip->tchip_mon_cntl_reg4 & TIMING_XBS_MASK;
	tchip_info->HBF = fgtchip->tchip_mon_cntl_reg5 & TIMING_XBS_MASK;
	tchip_info->HSS = ((fgtchip->tchip_mon_cntl_reg0 >> TIMING_HI_OFF) |
		((fgtchip->tchip_mon_cntl_reg1 & TIMING_HI_MASK) >> 6)) & 0x1ff;
	tchip_info->HSF = ((fgtchip->tchip_mon_cntl_reg2 >> TIMING_HI_OFF) |
		((fgtchip->tchip_mon_cntl_reg3 & TIMING_HI_MASK) >> 6)) & 0x1ff;
	tchip_info->HS2 = ((fgtchip->tchip_mon_cntl_reg4 >> TIMING_HI_OFF) |
		((fgtchip->tchip_mon_cntl_reg5 & TIMING_HI_MASK) >> 6)) & 0x1ff;

	tchip_info->PVI = fgtchip->tchip_vertical_int;
}






fg_set_tchip(tchip_info)
fg_timinginfo *tchip_info;
{

/* Set the value of the tchip registers */

	register struct tchip *fgtchip;

        fgtchip = (struct tchip *) fgmap.tchip;

	fgtchip->tchip_x_start = tchip_info->display_x;
	fgtchip->tchip_y_start = tchip_info->display_y;
	fgtchip->tchip_table_x_start = tchip_info->table_x;
	fgtchip->tchip_table_y_start = tchip_info->table_y;
	fgtchip->tchip_table_cntl_reg = tchip_info->table_width |
		tchip_info->table_L1 << TIMING_TL1_OFF |
		tchip_info->table_L2 << TIMING_TL2_OFF;
	fgtchip->tchip_mon_cntl_reg0 = tchip_info->VBS |
		((tchip_info->HSS & 0x1f) << TIMING_HI_OFF);
	fgtchip->tchip_mon_cntl_reg1 = tchip_info->VBF |
		((tchip_info->HSS >> 5) << TIMING_HI_OFF);
	fgtchip->tchip_mon_cntl_reg2 = tchip_info->VSS |
		((tchip_info->HSF & 0x1f) << TIMING_HI_OFF);
	fgtchip->tchip_mon_cntl_reg3 = tchip_info->VSF |
		((tchip_info->HSF >> 5) << TIMING_HI_OFF);
	fgtchip->tchip_mon_cntl_reg4 = tchip_info->HBS |
		((tchip_info->HS2 & 0x1f) << TIMING_HI_OFF);
	fgtchip->tchip_mon_cntl_reg5 = tchip_info->HBF |
		((tchip_info->HS2 >> 5) << TIMING_HI_OFF);
	fgtchip->tchip_vertical_int = tchip_info->PVI;
	fgtchip->tchip_int_reg = tchip_info->interrupt;
	fgtchip->tchip_csr = tchip_info->csr | tchip_info->pipe << 13;
}





/******************************************************************
 **                                                              **
 ** ADDER interrupt routine.                                     **
 **                                                              **
 ******************************************************************/

fgaint(fg)
register int fg;
{
}





/******************************************************************
 **                                                              **
 ** Sync. interrupt routine.                                     **
 **                                                              **
 ******************************************************************/

fgvint(fg)
	int fg;
{
}



/******************************************************************
 **                                                              **
 ** Graphic device input interrupt Routine.                      **
 **                                                              **
 ******************************************************************/


fgiint(ch)
register int ch;
{
	register struct _vs_event *vep;
	register struct fginput *eqh;
	register struct color_cursor *sgcursor;
	struct mouse_report *new_rep;
	struct tty *tp;
	register int unit;
	register u_short c;
	register int i, j;
	u_short data;
	char	wakeup_flag = 0;	/* flag to do a select wakeup call */
	int	cnt;
/*
 * Mouse state info
 */
	static char temp, old_switch, new_switch;

	eqh = eq_header;
	unit = (ch>>8)&03;
	new_rep = &current_rep;
	tp = &fc_tty[unit];

/*
 * If graphic device is turned on
 */

   if (fg_mouseon == 1) {
  
	cnt = 0;
	while (cnt++ == 0) {

/*
 * Pick up LK-201 input (if any)
 */

	    if (unit == 0) {

/* event queue full ? */

		if (ISFULL(eqh) == TRUE) {
/*		    mprintf("\nfg0: fgiint: event queue overflow");*/
		    return(0);
		}
/*
 * Get a character.
 */

		data = ch & 0xff;

/*
 * Check for various keyboard errors
 */

		if( data == LK_POWER_ERROR || data == LK_KDOWN_ERROR ||
	    	    data == LK_INPUT_ERROR || data == LK_OUTPUT_ERROR) {
			mprintf("\nfg0: fgiint: keyboard error, code = %x",data);
			return(0);
		}

		if (data < LK_LOWEST) 
		    	return(0);
		++wakeup_flag;		/* request a select wakeup call */

		vep = PUTBEGIN(eqh);
		PUTEND(eqh);
		vep->vse_direction = VSE_KBTRAW;
		vep->vse_type = VSE_BUTTON;
		vep->vse_device = VSE_DKB;
		vep->vse_x = eqh->curs_pos.x;
		vep->vse_y = eqh->curs_pos.y;
		vep->vse_time = TOY;
		vep->vse_key = data;
	    }

/*
 * Pick up the mouse input (if any)
 */

	    if ((unit == 1) && (sm_pointer_id == MOUSE_ID)) {

/* event queue full ? */

		if (ISFULL(eqh) == TRUE) {
/*		    mprintf("\nfg0: fgiint: event queue overflow");*/
		    return(0);
		}

/*
 * see if mouse position has changed
 */
		if( new_rep->dx != 0 || new_rep->dy != 0) {

/*
 * Check to see if we have to accelerate the mouse
 *
 */
		    if (fgflags.curs_acc > ACC_OFF) {
			if (new_rep->dx >= fgflags.curs_thr)
			    new_rep->dx +=
				(new_rep->dx - fgflags.curs_thr) * fgflags.curs_acc;
			if (new_rep->dy >= fgflags.curs_thr)
			    new_rep->dy +=
				(new_rep->dy - fgflags.curs_thr) * fgflags.curs_acc;
		    }

/*
 * update mouse position
 */
		    if( new_rep->state & X_SIGN) {
			eqh->curs_pos.x += new_rep->dx;
			if( eqh->curs_pos.x > MAX_CUR_X )
			    eqh->curs_pos.x = MAX_CUR_X;
		    }
		    else {
			eqh->curs_pos.x -= new_rep->dx;
			if( eqh->curs_pos.x < -15 )
			    eqh->curs_pos.x = -15;
		    }
		    fgcurstab.x = eqh->curs_pos.x;

		    if( new_rep->state & Y_SIGN) {
			eqh->curs_pos.y -= new_rep->dy;
			if( eqh->curs_pos.y < -15 )
			    eqh->curs_pos.y = -15;
		    }
		    else {
			eqh->curs_pos.y += new_rep->dy;
			if( eqh->curs_pos.y > MAX_CUR_Y )
			    eqh->curs_pos.y = MAX_CUR_Y;
		    }
		    fgcurstab.y = eqh->curs_pos.y;

		    if( tp->t_state & TS_ISOPEN ) {
/* Should modify this for Firefox cursor
			sgcursor = (struct color_cursor *) sgmap.cur;
			sgcursor->xpos = CURS_MIN_X + eqh->curs_pos.x;
			sgcursor->ypos = CURS_MIN_Y + eqh->curs_pos.y;
*/
		    }
		    fgcurstab.XY.coords.pat_x =
			fgcurstab.x - fgcurstab.hot_x + fg_wc_offset;
		    fgcurstab.XY.coords.pat_y = fgcurstab.y - fgcurstab.hot_y;
/* load new cursor position */
		    fg_comm_area->XY_position = fgcurstab.XY.position;
/* request position update */
		    fg_comm_area->Lego.XY_position = 1;
/* request cursor update */
		    fg_comm_area->HG_flags.cursor_update = 1;

		    if (eqh->curs_pos.y <= eqh->curs_box.bottom &&
			eqh->curs_pos.y >=  eqh->curs_box.top &&
			eqh->curs_pos.x <= eqh->curs_box.right &&
			eqh->curs_pos.x >=  eqh->curs_box.left) goto mbuttons;
		    vep = PUTBEGIN(eqh);
		    PUTEND(eqh);
		    ++wakeup_flag;	/* request a select wakeup call */

/*
 * Put event into queue and do select
 */

		    vep->vse_x = eqh->curs_pos.x;
		    vep->vse_y = eqh->curs_pos.y;
		    vep->vse_device = VSE_MOUSE;	/* mouse */
		    vep->vse_type = VSE_MMOTION;	/* position changed */
		    vep->vse_time = TOY;		/* time stamp */
		    vep->vse_direction = 0;
		    vep->vse_key = 0;
		}

/*
 * See if mouse buttons have changed.
 */

mbuttons:

		new_switch = new_rep->state & 0x07;
		old_switch = last_rep.state & 0x07;

		temp = old_switch ^ new_switch;
		if( temp ) {
		    for (j = 1; j < 8; j <<= 1) {/* check each button */
			if (!(j & temp))  /* did this button change? */
			    continue;
/* event queue full? */

			if (ISFULL(eqh) == TRUE) {
/*		    	    mprintf("\nfg0: fgiint: event queue overflow");*/
		    	    return(0);
			}

			vep = PUTBEGIN(eqh);	/* get new event */
			PUTEND(eqh);

			++wakeup_flag;      /* request a select wakeup call */

/* put event into queue */

			switch (j) {
			case RIGHT_BUTTON:
					vep->vse_key = VSE_RIGHT_BUTTON;
					break;

			case MIDDLE_BUTTON:
					vep->vse_key = VSE_MIDDLE_BUTTON;
					break;

			case LEFT_BUTTON:
					vep->vse_key = VSE_LEFT_BUTTON;
					break;

			}
			if (new_switch & j)
				vep->vse_direction = VSE_KBTDOWN;
			else
				vep->vse_direction = VSE_KBTUP;
			vep->vse_type = VSE_BUTTON;
			vep->vse_device = VSE_MOUSE;	/* mouse */
			vep->vse_time = TOY;
		    	vep->vse_x = eqh->curs_pos.x;
		    	vep->vse_y = eqh->curs_pos.y;
		    }

/* update the last report */

		    last_rep = current_rep;
		}
	    } /* Pick up mouse input */

	    else if ((unit == 1) && (sm_pointer_id == TABLET_ID)) {

/* event queue full? */

		    if (ISFULL(eqh) == TRUE) {
/*		    	mprintf("\nfg0: fgiint: event queue overflow");*/
		    	return(0);
		    }


/* update cursor position coordinates */

		    new_rep->dx /= fgflags.tab_res;
		    new_rep->dy = (2200 - new_rep->dy) / fgflags.tab_res;
		    if( new_rep->dx > MAX_CUR_X )
			new_rep->dx = MAX_CUR_X;
		    if( new_rep->dy > MAX_CUR_Y )
			new_rep->dy = MAX_CUR_Y;

/*
 * see if the puck/stylus has moved
 */
		    if( eqh->curs_pos.x != new_rep->dx ||
			eqh->curs_pos.y != new_rep->dy) {

/*
 * update cursor position
 */
		 	eqh->curs_pos.x = new_rep->dx;
		 	eqh->curs_pos.y = new_rep->dy;
			fgcurstab.x = eqh->curs_pos.x;
			fgcurstab.y = eqh->curs_pos.y;

		    	if( tp->t_state & TS_ISOPEN ) {
/* Should modify this for Firefox cursor
			    sgcursor = (struct color_cursor *) sgmap.cur;
			    sgcursor->xpos = CURS_MIN_X + eqh->curs_pos.x;
			    sgcursor->ypos = CURS_MIN_Y + eqh->curs_pos.y;
*/
			}
		    	if (eqh->curs_pos.y < eqh->curs_box.bottom &&
			    eqh->curs_pos.y >=  eqh->curs_box.top &&
			    eqh->curs_pos.x < eqh->curs_box.right &&
			    eqh->curs_pos.x >=  eqh->curs_box.left) goto tbuttons;

			vep = PUTBEGIN(eqh);
			PUTEND(eqh);
			++wakeup_flag;	/* request a select wakeup call */

/* Put event into queue */

/*
 * The type should be "VSE_TMOTION" but,  X doesn't know this type, therefore
 * until X is fixed, we fake it to be "VSE_MMOTION".
 *
 */
		    	vep->vse_type = VSE_MMOTION;
		    	vep->vse_device = VSE_TABLET;	/* tablet */
			vep->vse_direction = 0;
		    	vep->vse_x = eqh->curs_pos.x;
		    	vep->vse_y = eqh->curs_pos.y;
			vep->vse_key = 0;
		    	vep->vse_time = TOY;
		    }

/*
 * See if tablet buttons have changed.
 */

tbuttons:

		new_switch = new_rep->state & 0x1e;
		old_switch = last_rep.state & 0x1e;
		temp = old_switch ^ new_switch;
		if( temp ) {

/* event queue full? */

		    if (ISFULL(eqh) == TRUE) {
/*		    	mprintf("\nfg0: fgiint: event queue overflow");*/
		    	return(0);
		    }

		    vep = PUTBEGIN(eqh);
		    PUTEND(eqh);
		    ++wakeup_flag;	/* request a select wakeup call */

/* put event into queue */

		    vep->vse_device = VSE_TABLET;	/* tablet */
		    vep->vse_type = VSE_BUTTON;
		    vep->vse_x = eqh->curs_pos.x;
		    vep->vse_y = eqh->curs_pos.y;
		    vep->vse_time = TOY;

/* define the changed button and if up or down */

		    for (j = 1; j <= 0x10; j <<= 1) {/* check each button */
			if (!(j & temp))  /* did this button change? */
			    continue;
			switch (j) {
			case T_RIGHT_BUTTON:
					vep->vse_key = VSE_T_RIGHT_BUTTON;
					break;

			case T_FRONT_BUTTON:
					vep->vse_key = VSE_T_FRONT_BUTTON;
					break;

			case T_BACK_BUTTON:
					vep->vse_key = VSE_T_BACK_BUTTON;
					break;

			case T_LEFT_BUTTON:
					vep->vse_key = VSE_T_LEFT_BUTTON;
					break;

			}
		    	if (new_switch & j)
			    vep->vse_direction = VSE_KBTDOWN;
		    	else
			    vep->vse_direction = VSE_KBTUP;
		    }

/* update the last report */

		    last_rep = current_rep;
		}
	    } /* Pick up tablet input */
	} /* While input available */

/*
 * If we have proc waiting, and event has happened, wake him up
 */
	if(rsel && wakeup_flag && fgflags.selmask & SEL_READ) {
	    selwakeup(rsel,0);
	    rsel = 0;
	    fgflags.selmask &= ~SEL_READ;
	    wakeup_flag = 0;
	}
   }

/*
 * If the graphic device is not turned on, this is console input
 */

   else {

/*
 * Get a character from the keyboard.
 */

	if (ch & 0100000) {
	    data = ch & 0xff;

/*
 * Check for various keyboard errors
 */

	    if( data == LK_POWER_ERROR || data == LK_KDOWN_ERROR ||
		data == LK_INPUT_ERROR || data == LK_OUTPUT_ERROR) {
			mprintf("fg0: fgiint: Keyboard error, code = %x\n",data);
			return(0);
	    }
	    if( data < LK_LOWEST ) return(0);

/*
 * See if its a state change key
 */

	    switch ( data ) {
	    case LOCK:
			fg_keyboard.lock ^= 0xffff;	/* toggle */
			if( fg_keyboard.lock )
				fg_key_out( LK_LED_ENABLE );
			else
				fg_key_out( LK_LED_DISABLE );
			fg_key_out( LED_3 );
			return;

	    case SHIFT:
			fg_keyboard.shift ^= 0xffff;
			return;	

	    case CNTRL:
			fg_keyboard.cntrl ^= 0xffff;
			return;

	    case ALLUP:
			fg_keyboard.cntrl = fg_keyboard.shift = 0;
			return;

	    case REPEAT:
			c = fg_keyboard.last;
			break;

	    case HOLD:
/*
 * "Hold Screen" key was pressed, we treat it as if ^s or ^q was typed.
 */
			if (fg_keyboard.hold == 0) {
			    if((tp->t_state & TS_TTSTOP) == 0) {
			    	c = q_key[CHAR_S];
			    	fg_key_out( LK_LED_ENABLE );
			    	fg_key_out( LED_4 );
				fg_keyboard.hold = 1;
			    } else
				c = q_key[CHAR_Q];
			}
			else {
			    c = q_key[CHAR_Q];
			    fg_key_out( LK_LED_DISABLE );
			    fg_key_out( LED_4 );
			    fg_keyboard.hold = 0;
			}
			if( c >= ' ' && c <= '~' )
			    c &= 0x1f;
		    	(*linesw[tp->t_line].l_rint)(c, tp);
			return;

	    default:

/*
 * Test for control characters. If set, see if the character
 * is elligible to become a control character.
 */
			if( fg_keyboard.cntrl ) {
			    c = q_key[ data ];
			    if( c >= ' ' && c <= '~' )
				c &= 0x1f;
			    else if (c >= 0xA1 && c <= 0xFE)
			        c &= 0x9F;
			} else if( fg_keyboard.lock || fg_keyboard.shift )
				    c = q_shift_key[ data ];
				else
				    c = q_key[ data ];
			break;	

	    }

	    fg_keyboard.last = c;

/*
 * Check for special function keys
 */
	    if( c & 0x100 ) {

		register char *string;

		string = q_special[ c & 0x7f ];
		while( *string )
		    (*linesw[tp->t_line].l_rint)(*string++, tp);
	    } else
		    (*linesw[tp->t_line].l_rint)(c, tp);
	    if (fg_keyboard.hold &&((tp->t_state & TS_TTSTOP) == 0)) {
		    fg_key_out( LK_LED_DISABLE );
		    fg_key_out( LED_4 );
		    fg_keyboard.hold = 0;
	    }
	}
   }

	return(0);
}







/******************************************************************
 **                                                              **
 ** Routine to start transmission.                               **
 **                                                              **
 ******************************************************************/

fgstart(tp)
	register struct tty *tp;
{
        register int unit, c;
        register struct tty *tp0;
        int s;

        unit = minor(tp->t_dev);
        tp0 = &sm_tty;
        unit &= 03;
        s = spl6();
/*
 * If it's currently active, or delaying, no need to do anything.
 */
        if (tp->t_state&(TS_TIMEOUT|TS_BUSY|TS_TTSTOP))
            goto out;
/*
 * Display chars until the queue is empty, if the second subchannel is open
 * direct them there. Drop characters from any lines other than 0 on the floor.
 * TANDEM is set on second subchannel for flow control.
 */

        while( tp->t_outq.c_cc ) {
            if (unit == 0) {            /* console device */
                if (tp0->t_state & TS_ISOPEN) {
                    if (tp0->t_state & TS_TBLOCK)
                        goto out;
                    c = getc(&tp->t_outq);
                    (*linesw[tp0->t_line].l_rint)(c, tp0);
                } else {
                    c = getc(&tp->t_outq);
                    fg_blitc((char)(c & 0xff));
                }
            } else if (unit == 2) {     /* sgscreen, do flow control */
                    c = getc(&tp->t_outq);
                    if ((tp0->t_state&TS_TBLOCK) == 0) {
                        tp = &fc_tty[0];
                        unit = minor(tp->t_dev);
                        unit &= 3;
                        continue;
                    } else
                        goto out;
            } else
                c = getc(&tp->t_outq);
        }

/*
 * If there are sleepers, and output has drained below low
 * water mark, wake up the sleepers.
 */
        if ( tp->t_outq.c_cc<=TTLOWAT(tp) )
            if (tp->t_state&TS_ASLEEP){
                tp->t_state &= ~TS_ASLEEP;
                wakeup((caddr_t)&tp->t_outq);
            }
        tp->t_state &= ~TS_BUSY;
out:
        splx(s);

}


/******************************************************************
 **                                                              **
 ** Routine to stop output on the graphic device, e.g. for ^S/^Q **
 ** or output flush.                                             **
 **                                                              **
 ******************************************************************/

/*ARGSUSED*/
fgstop(tp, flag)
	register struct tty *tp;
{
        register int s;

/*
 * Block interrupts while modifying the state.
 */
        s = spl6();
        if (tp->t_state & TS_BUSY)
            if ((tp->t_state&TS_TTSTOP)==0)
                tp->t_state |= TS_FLUSH;
            else
                tp->t_state &= ~TS_BUSY;
        splx(s);

}





/******************************************************************
 **                                                              **
 ** Routine to output a character to the screen                  **
 **                                                              **
 ******************************************************************/

fg_blitc( c )
register u_char c;
{

	register int i;
	register u_short *font_row;
	register u_long *pixel;
	register u_long *pixel1;
	register u_long mask;
	register int shift;

	c &= 0xff;

	switch ( c ) {
	case '\t':				/* tab		*/
		    for (i = 8 - ((cursor.x >> CHAR_WIDTH) & 0x07); i > 0; --i) {
		    	fg_blitc( ' ' );
		    }
		    return(0);

	case '\r':				/* return	*/
                    fg_pos_cursor(cursor.x, cursor.y);
		    cursor.x = 0;
/*
		    if (!(fgflags.inuse & GRAPHIC_DEV))
                        ioctl(fd, FG_POSCURSOR, &cursor);
*/

		    return(0);

	case '\b':				/* backspace	*/
		    if (cursor.x > 0) {
                    	fg_pos_cursor(cursor.x, cursor.y);
		    	cursor.x -= CHAR_WIDTH;
                    	fg_pos_cursor(cursor.x, cursor.y);
/*
		    	if (!(sgflags.inuse & GRAPHIC_DEV))
			    ioctl(fd, FG_POSCURSOR, &cursor);
*/
		    }
		    return(0);

	case '\n':				/* linefeed	*/
		    if ((cursor.y += CHAR_HEIGHT) > (MAX_CUR_Y - CHAR_HEIGHT)) {
		    	if (fg_mouseon == 1)
			    cursor.y = 0;
		    	else {
			    cursor.y -= CHAR_HEIGHT;
			    fg_scroll_up();
		    	}
		    }
/*
		    if (!(sgflags.inuse & GRAPHIC_DEV))
			ioctl(fd, FG_POSCURSOR, &cursor);
*/
                    fg_pos_cursor(cursor.x, cursor.y);
		    return(0);

	default:
		/*----------------------------------------------------------
		 * Weed out unprintable characters.  Printable characters fall
		 * between space (0x20) and tilde (0x7E).  For 8-bit support
		 * another range of printable characters are those between
		 * 0xA1 and 0xFD. */

		    if ((c < ' ') || (c > 0xFD) || (c < 0xA1 && c > '~'))
			return(0);
	}

        i = c - ' ';

        if (i < 0 || i > 221)
                i = 0;

        else {
                if (c > '~')
                        c -= 34; /* These are to skip the (32) 8-bit control char
s.
                              as well as DEL and 0xA0 which aren't printable */
                i *= CHAR_HEIGHT;
        }

/*
 * Initialize character cell with background pixel value
 */

        font_row = (u_short *)(fg_font + i);
	pixel = (u_long *)(fgmap.i_vram) + cursor.x + cursor.y*2048;

        for (i = 0, pixel1 = pixel; i < CHAR_WIDTH;  i++, pixel1++ )
	  *pixel1 = fg_bgpixel;

        for (i = 1, pixel1 = pixel + 2048; i < CHAR_HEIGHT;  i++, pixel1+=2048 )
	  bcopy ((char *)(pixel), (char *)(pixel1), CHAR_WIDTH*4);

/*
 * Set respective bits in character cell with foreground pixel value
 */

        for (i = 0; i < CHAR_HEIGHT;  i++, font_row++, pixel+=2048 ) {
	  if (*font_row != 0) {
	    shift = ffs(*font_row) - 1;
	    pixel1 = pixel + shift;
	    *pixel1 = fg_fgpixel;
	    for (shift++; shift < CHAR_WIDTH; shift++) {
	      mask = 1 << shift;
	      if ((mask & *font_row) != 0) {
		pixel1 = pixel + shift;
		*pixel1 = fg_fgpixel;
	      }
	    }
	  }
	}

/*
 * update console cursor coordinates
 */

        cursor.x += CHAR_WIDTH;
/*
	if (!(sgflags.inuse & GRAPHIC_DEV))
	    ioctl(fd, FG_POSCURSOR, &cursor);
*/

        if (cursor.x > (MAX_CUR_X - CHAR_WIDTH)) {
            fg_blitc( '\r' );
            fg_blitc( '\n' );
        }
        fg_pos_cursor(cursor.x, cursor.y);

}








/******************************************************************
 **                                                              **
 ** Routine to scroll up the screen one character height         **
 **                                                              **
 ******************************************************************/


fg_scroll_up()
{

        register struct dchip *fgdchip;
        register struct achip *fgachip;

        int     y_value;
        int     temp;
        int     i;
        u_long  save_reg[4];

        fgdchip = (struct dchip *) fgmap.dchip;
        fgachip = (struct achip *) fgmap.achip;
        fg_wait_done();
        y_value = CHAR_HEIGHT;
        fgachip->achip_x_a = Fixed(0);
        fgachip->achip_x_size = Width;
        fgachip->achip_y_size = 1 ;
        fgachip->achip_x_i = 0;
        fgachip->achip_y_i = 0;
        fgachip->achip_x_offset_dst = 0;
        fgachip->achip_y_offset_dst = -CHAR_HEIGHT;
        fgachip->achip_x_offset_src= 0;
        fgachip->achip_y_offset_src = 0;
        fgachip->achip_width_src = 0;
        fgachip->achip_width_dst = 0;
        fgachip->achip_offset_src = 0;
        fgachip->achip_offset_dst = 0;
        fgdchip->dchip_control_src = 0x0001;
        fgdchip->dchip_logical_z_0 =  -1;
        fgdchip->dchip_logical_z_1 =  -1;
        fgdchip->dchip_logical_z_2 =  0;
        fgdchip->dchip_logical_z_3 =  0;

        for( i = 0; i < (Height - CHAR_HEIGHT); i++) {
            fgachip->achip_y_a = Fixed(y_value);
            fgachip->achip_csr = 0;
            fgachip->achip_counter = 1;
            fg_wait_done();
            y_value++;
        }

/*
 * Save logical registers
 */

/*
        save_reg[0] = fgdchip->dchip_logical_z_0;
        save_reg[1] = fgdchip->dchip_logical_z_1;
        save_reg[2] = fgdchip->dchip_logical_z_2;
        save_reg[3] = fgdchip->dchip_logical_z_3;
*/

/* Blank the last line */

	save_logic[0] = 1;
	save_logic[1] = 1;
	save_logic[2] = 0;
	save_logic[3] = 0;
        fgdchip->dchip_logical_z_0 =  0;
        fgdchip->dchip_logical_z_1 =  0;
        fgdchip->dchip_logical_z_2 =  0;
        fgdchip->dchip_logical_z_3 =  0;
        temp = cursor.y + CHAR_HEIGHT;

        fg_wait_done(); /* Finish the current operation */

        fgachip->achip_y_a = Fixed(temp);
        fgachip->achip_y_size = CHAR_HEIGHT;
        fgachip->achip_csr = 0;
        fgachip->achip_counter = 1;

        fg_wait_done(); /* Finish the current operation */
/*
 * Restore logical registers
 */

        fgdchip->dchip_logical_z_0 = save_logic[0];
        fgdchip->dchip_logical_z_1 = save_logic[1];
        fgdchip->dchip_logical_z_2 = save_logic[2];
        fgdchip->dchip_logical_z_3 = save_logic[3];
}









/********************************************************************
 **                                                                **
 ** Routine to direct kernel console output to display destination **
 **                                                                **
 ********************************************************************/

fgputc( c )
register char c;
{

        register struct tty *tp0;
	register struct tchip *fgtchip;


/*
 * This routine may be called in physical mode by the dump code
 * so we change the driver into physical mode.
 * One way change, can't go back to virtual mode.
 */

        if( (mfpr(MAPEN) & 1) == 0 ) {
	  fg_physmode = 1;
	  fg_mouseon = 0;
	  fgmap.boot_rom = (char *) FG_PHYS_BOOT_ROM;
	  fgmap.weitek_code = fgmap.boot_rom + WEITEK;
	  fgmap.dram = fgmap.boot_rom + DRAM;
	  fgmap.achip = fgmap.boot_rom + ACHIP;
	  fgmap.dchip = fgmap.boot_rom + DCHIP;
	  fgmap.tchip = fgmap.boot_rom + TCHIP;
	  fgmap.fbic = fgmap.boot_rom + FBIC;
	  fgmap.i_vram = fgmap.boot_rom + I_VRAM;
	  fgmap.x_vram = fgmap.boot_rom + X_VRAM;
	  fgmap.i_pattern_mem = fgmap.boot_rom + I_PATTERN;
	  fgmap.x_pattern_mem = fgmap.boot_rom + X_PATTERN;
	  fgmap.console = fgmap.boot_rom + FG_CONSOLE;
	  fgmap.scb = fgmap.boot_rom + FG_SCB;
	  fgtchip = (struct tchip *) fgmap.tchip;
	  fgtchip->tchip_csr |= TCHIP_UNBLANK; /* turn the video on */
	  fg_blitc(c & 0xff);
	  return;
        }


/*
 * direct kernel output char to the proper place
 */

        tp0 = &sm_tty;

        if (fgflags.kernel_loop != 0  &&  tp0->t_state & TS_ISOPEN) {
            (*linesw[tp0->t_line].l_rint)(c, tp0);
        } else {
            fg_blitc(c & 0xff);
        }

}






/******************************************************************
 **                                                              **
 ** Routine to get a character from LK201.                       **
 **                                                              **
 ******************************************************************/

fggetc()
{
	int	c;
	u_short	data;

/*
 * Get a character from the keyboard,
 */

loop:
	data = fcgetc();

/*
 * Check for various keyboard errors
 */

	if( data == LK_POWER_ERROR || data == LK_KDOWN_ERROR ||
            data == LK_INPUT_ERROR || data == LK_OUTPUT_ERROR) {
		mprintf(" fg0: Keyboard error, code = %x\n",data);
		return(0);
	}
	if( data < LK_LOWEST ) return(0);

/*
 * See if its a state change key
 */

	switch ( data ) {
	case LOCK:
		fg_keyboard.lock ^= 0xffff;	/* toggle */
		if( fg_keyboard.lock )
			fg_key_out( LK_LED_ENABLE );
		else
			fg_key_out( LK_LED_DISABLE );
		fg_key_out( LED_3 );
		goto loop;

	case SHIFT:
		fg_keyboard.shift ^= 0xffff;
		goto loop;

	case CNTRL:
		fg_keyboard.cntrl ^= 0xffff;
		goto loop;

	case ALLUP:
		fg_keyboard.cntrl = fg_keyboard.shift = 0;
		goto loop;

	case REPEAT:
		c = fg_keyboard.last;
		break;

	default:

/*
 * Test for control characters. If set, see if the character
 * is elligible to become a control character.
 */
		if( fg_keyboard.cntrl ) {
		    c = q_key[ data ];
		    if( c >= ' ' && c <= '~' )
			c &= 0x1f;
		} else if( fg_keyboard.lock || fg_keyboard.shift )
			    c = q_shift_key[ data ];
		       else
			    c = q_key[ data ];
		break;	

	}

	fg_keyboard.last = c;

/*
 * Check for special function keys
 */
	if( c & 0x80 )
	    return (0);
	else
	    return (c);
}






/*********************************************************************
 **                                                                 **
 ** Routine to initialize virtual console. This routine sets up the **
 ** graphic device so that it can be used as the system console. It **
 ** is invoked before autoconfig and has to do everything necessary **
 ** to allow the device to serve as the system console.             **
 **                                                                 **
 *********************************************************************/

extern	(*vs_gdopen)();
extern	(*vs_gdclose)();
extern	(*vs_gdread)();
extern	(*vs_gdwrite)();
extern	(*vs_gdselect)();
extern	(*vs_gdkint)();
extern	(*vs_gdioctl)();
extern	(*vs_gdstop)();

fgcons_init()
{
	register struct fc_regs *fcaddr = (struct fc_regs *)ffcons;
        register struct nb5_regs *fgaddr = (struct nb5_regs *)fgmem;
        register struct achip *fgachip;
        register struct dchip *fgdchip;
        register struct tchip *fgtchip;
	register fg_commarea_desc *fgdescr;



/*
 * Set the line parameters on SLU line 0 for
 * the LK201 keyboard: 4800 BPS, 8-bit char, 1 stop bit, no parity.
 */
	fcaddr->fclpr = (SER_RXENAB | SER_KBD | SER_SPEED | SER_CHARW);

/*
 * Set the hardware register for save routine. The save routine will be
 * called as the console program enters/exits its console display
 * (halting the cpu).
 *
 * NOTE: SYSTEM SCRATCH RAM IS MAPPED IN SS.C
 */

	ctsi->ct_save_p = (char *) svtophy((int *)fg_save)+2;
	ctsi->ct_restore_p = (char *) svtophy((int *)fg_restore)+2;
	fg_phys_fcaddr = (struct fc_regs *) svtophy(fcaddr);

/*
 * Load fgmap structure with the virtual addresses of the Firefox.
 */

	fgbase = (caddr_t) ((u_long)fgaddr);
	fgmap.boot_rom = fgbase;
	fgmap.weitek_code = fgbase + WEITEK;
	fgmap.dram = fgbase + DRAM;
        fgmap.achip = fgbase + ACHIP;
        fgmap.dchip = fgbase + DCHIP;
        fgmap.tchip = fgbase + TCHIP;
        fgmap.fbic = fgbase + FBIC;
	fgmap.i_vram = fgbase + I_VRAM;
	fgmap.x_vram = fgbase + X_VRAM;
	fgmap.x_pattern_mem = fgbase + X_PATTERN;
	fgmap.i_pattern_mem = fgbase + I_PATTERN;
	fgmap.x_pattern_mem = fgbase + X_PATTERN;
	fgmap.console = fgbase + FG_CONSOLE;
	fgmap.scb = fgbase + FG_SCB;

        fgachip = (struct achip *) fgmap.achip;
        fgdchip = (struct dchip *) fgmap.dchip;
        fgtchip = (struct tchip *) fgmap.tchip;
	
	fgctsi = (struct fgctsi *) (fgctsixs + ((int) ctsi->ct_std_out.ctcb_extdvrstate_p & 0x1ff));
	if ((fgctsi->vocx != FG_CTSITABLE_X) || (fgctsi->vocy != FG_CTSITABLE_Y)) 
	  {
	    fg_copy_planes (fgctsi->vocx, fgctsi->vocy, FG_CTSITABLE_X, FG_CTSITABLE_Y, 
			    FG_TABLE_WIDTH, FG_TABLE_WIDTH, 0xF );
	  }
	fgctsi->vocx = FG_CTSITABLE_X;
	fgctsi->vocy = FG_CTSITABLE_Y;
	fgctsi->vocw = FG_CTSITABLE_WIDTH;
	fgctsi->voch = FG_CTSITABLE_HEIGHT;

/*
 * Initialize the Firefox
 */

	fg_config();			/* configure the graphics subsystem */
        fg_init_achip(fgachip);		/* Initialize the A chip */
        fg_init_dchip(fgdchip);		/* Initialize the D chip */
        fg_init_tchip(fgtchip);		/* Initialize the T chip */
	fg_init_shared();
	fg_init_comm_area(fgdescr, 0);	/* Only initialize the common area */

        fg_clip(0, 0, 2047, 2047);
        fg_clr_screen();
        fgtchip->tchip_csr |= TCHIP_UNBLANK;

        fg_input();			/* init the input devices */
	v_consputc = fgputc;
	v_consgetc = fggetc;
	ws_display_type = FG_DTYPE;	/* Identify FG as graphics device */
	ws_display_units = 1;		/* unit 0 only */
	vs_gdopen = fgopen;
	vs_gdclose = fgclose;
	vs_gdread = fgread;
	vs_gdwrite = fgwrite;
	vs_gdselect = fgselect;
	vs_gdkint = fgiint;
	vs_gdioctl = fgioctl;
	vs_gdstop = fgstop;
}






fgreset()
{
}








/******************************************************************
 **                                                              **
 ** Routine to setup the input devices                           **
 **  (keyboard, mouse, and tablet).                              **
 **                                                              **
 ******************************************************************/

fg_input()
{

	register struct fc_regs *fcaddr = (struct fc_regs *)ffcons;
	register int	lpr;
	int	i;
	int	status;
	char	id_byte;

/*
 * Set the line parameters on SLU line 0 for
 * the LK201 keyboard: 4800 BPS, 8-bit char, 1 stop bit, no parity.
 */
        fcaddr->fclpr = (SER_RXENAB | SER_KBD | SER_SPEED | SER_CHARW);
/*
 * Reset the keyboard to the default state
 */

	fg_key_out(LK_DEFAULTS);

/*
 * Set SLU line 1 parameters for mouse communication.
 */
	lpr = SER_POINTER | SER_CHARW | SER_PARENB | SER_ODDPAR
		| SER_SPEED | SER_RXENAB;
	fcaddr->fclpr = lpr;

/*
 * Perform a self-test
 */
	fg_putc(SELF_TEST);
/*
 * Wait for the first byte of the self-test report
 *
 */
	status = fg_getc();
	if (status < 0) {
	    mprintf("\nfg: Timeout on 1st byte of self-test report\n");
	    goto OUT;
	}
/*
 * Wait for the hardware ID (the second byte returned by the self-test report)
 *
 */
	id_byte = fg_getc();
	if (id_byte < 0) {
	    mprintf("\nfg: Timeout on 2nd byte of self-test report\n");
	    goto OUT;
	}
/*
 * Wait for the third byte returned by the self-test report)
 *
 */
	status = fg_getc();
	if (status != 0) {
	    mprintf("\nfg: Timeout on 3rd byte of self-test report\n");
	    goto OUT;
	}
/*
 * Wait for the forth byte returned by the self-test report)
 *
 */
	status = fg_getc();
	if (status != 0) {
	    mprintf("\fsg: Timeout on 4th byte of self-test report\n");
	    goto OUT;
	}

/*
 * Wait to be sure that the self-test is done (documentation indicates that
 * it requires 1 second to do the self-test).
 */

for (i = 0; i < 1000000; i++);
/*	DELAY(1000000);*/

/*
 * Set the operating mode
 *
 *   We set the mode for both mouse and the tablet to "Incremental stream mode".
 *
 */
	if ((id_byte & 0x0f) == MOUSE_ID)
		sm_pointer_id = MOUSE_ID;
	else
		sm_pointer_id = TABLET_ID;
	fg_putc(INCREMENTAL);

OUT:
	return(0);
}






/******************************************************************
 **                                                              **
 ** Routine to initialize the shared memory                      **
 **                                                              **
 ******************************************************************/

fg_init_shared()
{

/*
 * initialize the event queue pointers
 */


	eq_header = (struct fginput *)
			  (((int)event_shared & ~(0x01FF)) + 512);
	eq_header->curs_pos.x = 0;
	eq_header->curs_pos.y = 0;
	fgcurstab.x = 0;
	fgcurstab.y = 0;

	eq_header->curs_box.left = 0;
	eq_header->curs_box.right = 0;
	eq_header->curs_box.top = 0;
	eq_header->curs_box.bottom = 0;

}




/******************************************************************
 **                                                              **
 ** Routine to get 1 character from the mouse (SLU line 1).      **
 ** Return an error on timeout or faulty character.              **
 **                                                              **
 ** NOTE:                                                        **
 **	This routine will be used just during initialization     **
 **     (during system boot).                                    **
 **                                                              **
 ******************************************************************/

fg_getc()
{
	register struct fc_regs *fcaddr = (struct fc_regs *)ffcons;
	register int timo;
	register int c;
	register int temp_reg;


	for(timo=50; timo > 0; --timo) {
/*		DELAY(60000);*/
	  for (temp_reg = 0; temp_reg < 100000; temp_reg++);
		if(fcaddr->fccsr&FC_RDONE) {
			c = fcaddr->fcrbuf;
			if(((c >> 8) & 03) != 1)
				continue;
			if(c&(FC_DO|FC_FE|FC_PE))
				continue;
			return(c & 0xff);
		}
	}
	return(-1);
}








/******************************************************************
 **                                                              **
 ** Routine to send a character to the mouse using SLU line 1.   **
 **                                                              **
 ** NOTE:                                                        **
 **	This routine will be used just during initialization     **
 **     (during system boot).                                    **
 **                                                              **
 ******************************************************************/

fg_putc(c)
	register int c;
{
	register struct fc_regs *fcaddr = (struct fc_regs *)ffcons;
	register int timo;

	fcaddr->fctcr |= 0x2;
	timo = 60000;
	while ((fcaddr->fccsr&FC_TRDY) == 0)
		if (--timo == 0)
			break;
	fcaddr->fctbuf = c&0xff;
	timo = 60000;
	while ((fcaddr->fccsr&FC_TRDY) == 0)
		if (--timo == 0)
			break;
/*	DELAY(50000);		** ensure character transmit completed */
	fcaddr->fctcr &= ~0x2;
}





/******************************************************************
 **                                                              **
 ** Routine to load the cursor.                                  **
 **                                                              **
 ******************************************************************/

fg_pos_cursor(x, y)
int     x, y;
{
        register struct dchip *fgdchip;
        register struct tchip *fgtchip;
        int     loop_cnt;
        int     save_mask;
        int     save_plane;
        u_short *vram_wt;
        u_short *vram_rd;
        u_long  rd_data;
        int   x1, y1, x2, y2; /* save clipping values */


        fgdchip = (struct dchip *) fgmap.dchip;
        fgtchip = (struct tchip *) fgmap.tchip;

        vram_wt = vram_rd = (u_short *)fgmap.x_vram;
        save_mask = fgdchip->dchip_write_mask;
        save_plane = fgdchip->dchip_read_mask;
        fgdchip->dchip_write_mask = fg_lego_p0_mask  << 3;
        fgdchip->dchip_read_mask = fg_lego_p0  + 3;
	loop_cnt = 100;
        do {
            vram_wt[ fg_lego_address(0x108,3+FG_TABLE_Y) ] = x + fg_wc_offset;
            rd_data = vram_rd[ fg_lego_address(0x108,3+FG_TABLE_Y) ];
        } while( loop_cnt-- && (x + fg_wc_offset != rd_data));
	if (!loop_cnt)
	    printf("\n\tfg_pos_cursor1 failed, got %x not %x\n",rd_data,(x+fg_wc_offset));

	loop_cnt = 100;
        do {
            vram_wt[ fg_lego_address(0x10A,3+FG_TABLE_Y) ] = y;
            rd_data = vram_rd[ fg_lego_address(0x10A,3+FG_TABLE_Y) ];
        } while( loop_cnt-- && (y != rd_data));
	if (!loop_cnt)
	    printf("\n\tfg_pos_cursor2 failed, got %x not %x\n",rd_data,y);

        fgtchip->tchip_csr |= WC_LOAD;

/* load lego data from bitmap into legos */

        fgdchip->dchip_write_mask = save_mask;
        fgdchip->dchip_read_mask = save_plane;
	fgctsi->curx = x;
	fgctsi->cury = y;

}







/******************************************************************
 **                                                              **
 ** Routine to load the cursor.                                  **
 **                                                              **
 ******************************************************************/

fg_load_cursor(cur)
u_long	cur[64];
{


	register struct dchip *fgdchip;
	int	save_mask;
	int	x, y, count;

        fgdchip = (struct dchip *) fgmap.dchip;
	save_mask = fgdchip->dchip_write_mask;
        fgdchip->dchip_write_mask = fg_lego_p0_mask << 3;
        fgdchip->dchip_read_mask = fg_lego_p0 + 3;
	x = MAX_SCREEN_X / 16;
	y = 3 + FG_TABLE_Y;
/*
 * cursor data (2 planes of 32x32)
 */

	x /= 2;		/* need long word address */
	for (count = 0; count < 64; count++) {
	    fg_wt_lego_long(cur[count], &x, &y);
	}
        fg_load_lego (WC_LOAD);
}




/******************************************************************
 **                                                              **
 ** Routine to output a character to the LK201 keyboard. This	 **
 ** routine polls the tramsmitter on the keyboard to output a    **
 ** code. The timer is to avaoid hanging on a bad device.        **
 **                                                              **
 ******************************************************************/

fg_key_out( c )
char c;
{
	register struct fc_regs *fcaddr;
	register int timo = 30000;
	int s;
	int tcr, ln;

	if (v_consputc != fgputc)
		return;

	if(fg_physmode)
		fcaddr = (struct fc_regs *)FG_PHYSNEXUS;
	else
		fcaddr = (struct fc_regs *)ffcons;

	if(fg_physmode == 0)
		s = spl6();
	tcr = 0;
	fcaddr->fctcr |= 1;
	while (1) {
		while ((fcaddr->fccsr&FC_TRDY) == 0 && timo--) ;
		ln = (fcaddr->fccsr>>8) & 3;
		if (ln != 0) {
			tcr |= (1 << ln);
			fcaddr->fctcr &= ~(1 << ln);
			continue;
		}
		fcaddr->fctbuf = c&0xff;
		while (1) {
			while ((fcaddr->fccsr&FC_TRDY) == 0) ;
			ln = (fcaddr->fccsr>>8) & 3;
			if (ln != 0) {
				tcr |= (1 << ln);
				fcaddr->fctcr &= ~(1 << ln);
				continue;
			}
			break;
		}
		fcaddr->fctcr &= ~0x1;
/* todo: clear interrupt

		if (tcr == 0)
			fcaddr->nb_int_reqclr = SINT_ST;
		else
			fcaddr->fctcr |= tcr;
*/
		break;
	}
	if(fg_physmode == 0)
		splx(s);
}


/******************************************************************
 **                                                              **
 ** Routine to wait for the current operation to complete        **
 **                                                              **
 ******************************************************************/

fg_wait_done()
{
        register struct achip *fgachip;
        int i;

        fgachip = (struct achip *) fgmap.achip;

        for ( i = 10000 ; i > 0  &&  !(fgachip->achip_csr & OP_DONE)
            ; --i);

        if (i == 0) {
            mprintf("\nfg_wait_done: timeout polling for LSB to get set in fgachip->achip_csr");
            return(-1);
        }
        return(0);
}





/******************************************************************
 **                                                              **
 ** Routine to figure out the number of planes.                  **
 **                                                              **
 ******************************************************************/

fg_get_planes()
{
	register struct dchip *fgdchip;
	int	i_res0, i_res1, i_res2, i_res3, i_res6;

	fgdchip->dchip_setup_0 = 0x7f05;
	fgdchip->dchip_setup_1 = 0x7f06;
	fgdchip->dchip_setup_2 = 0x7f07;
	fgdchip->dchip_setup_3 = 0x7f08;
	fgdchip->dchip_setup_6 = 0x7f09;

	i_res0 = fgdchip->dchip_setup_0 & 0x7f0f;
	i_res1 = fgdchip->dchip_setup_1 & 0x7f0f;
	i_res2 = fgdchip->dchip_setup_2 & 0x7f0f;
	i_res3 = fgdchip->dchip_setup_3 & 0x7f0f;
	i_res6 = fgdchip->dchip_setup_6 & 0x7f0f;

	mixed_up_planes = -1;
	if (i_res0 == 0x7f05) {
	    if (i_res3 == 0x7f08) num_planes = 24;
	    else if (i_res1 == 0x7f06) {
			if (i_res2 == 0x7f07) {
			    mixed_up_planes = 0;
			    num_planes = 12;
			}
		 else
			num_planes = 8;
	    }
	    else
		num_planes = 4;
	}
	else {
	    if (i_res2 == 0x7f07) num_planes = 4;
	    else if (i_res6 == 0x7f09) num_planes = 20;
	    mixed_up_planes = 0;
	}
}


/******************************************************************
 **                                                              **
 ** Routine to initialize the common area (shared memory)        **
 **                                                              **
 ** Note: the second argument indicates whether or not we want   **
 **       to initialize the common area descriptor.              **
 **       if (flag == 1)                                         **
 **	       initialize descriptor                             **
 **       else                                                   **
 **            do not                                            **
 **                                                              **
 ******************************************************************/


fg_init_comm_area(data, flag)
register caddr_t data;
int	flag;
{
        register int *ptep;             /* page table entry pointer */
	register fg_commarea_desc *fgdescr = (fg_commarea_desc *) data;
        char    *fg_comm_base;
	int	i;

	fg_comm_base = (char *)((u_long)comm_shared + 512 & ~0x01FF);
	ptep = (int *) SVTOPTE (fg_comm_base);

	for (i = 0; i <(COMM_SIZE/NBPG); i++, ptep++)
		*ptep = (*ptep & ~PG_PROT) | PG_UW | PG_V;
        mtpr(TBIA, 0);                  /* clr CPU translation buf */

/* 
 * Check to see if common area descriptor should be initialized
 */
	if (flag == 1) {
		fgdescr->nsegs = 1;
		fgdescr->MemSeg[0].nbytes = COMM_SIZE;
		fgdescr->MemSeg[0].VA = fg_comm_base;
		fgdescr->MemSeg[0].PhysOffset =
			svtophy(fg_comm_base) - (long) fg_comm_base;
	}
	fg_comm_area = (struct _fg_commarea *)fg_comm_base;
}




/******************************************************************
 **                                                              **
 ** Routine to clear the screen                                  **
 **                                                              **
 ******************************************************************/

fg_clr_screen()
{
        register struct achip *fgachip;
        register struct dchip *fgdchip;

        fgachip = (struct achip *) fgmap.achip;
        fgdchip = (struct dchip *) fgmap.dchip;

	fg_wait_done();		/* wait for current operation to finish */

	save_logic[0] = 1;
	save_logic[1] = 1;
	save_logic[2] = 0;
	save_logic[3] = 0;

/*
 * Setup the logic registers
 */
	fgdchip->dchip_logical_z_0 =  0;
	fgdchip->dchip_logical_z_1 =  0;
	fgdchip->dchip_logical_z_2 =  0;
	fgdchip->dchip_logical_z_3 =  0;
/*
 * Setup the address registers
 */
	fgachip->achip_x_a = Fixed(0);
	fgachip->achip_y_a = Fixed(0);
	fgachip->achip_x_size = MAX_SCREEN_X;
	fgachip->achip_y_size = MAX_SCREEN_Y;
	fgachip->achip_counter = 1;

	fg_wait_done();		/* wait for current operation to finish */

}










/******************************************************************
 **                                                              **
 ** Routine to initialize the A chip                             **
 **                                                              **
 ******************************************************************/

fg_init_achip(fgachip)
register struct achip *fgachip;
{
	fgachip->achip_alpha_cntl = 0x20;
	fgachip->achip_csr = 0x0002;
	fgachip->achip_width_src = 0;
	fgachip->achip_width_dst = 0;
	fgachip->achip_offset_src = 0;
	fgachip->achip_offset_dst = 0;

	fgachip->achip_x_offset_src = 0;
	fgachip->achip_x_offset_dst = 0;
	fgachip->achip_y_offset_src = 0;
	fgachip->achip_y_offset_dst = 0;
}



/******************************************************************
 **                                                              **
 ** Routine to configure the graphics subsystem                  **
 **                                                              **
 ******************************************************************/

fg_config()
{
	register struct dchip *fgdchip;

	int	d0, d1, d2, d3, d6;

	fgdchip = (struct dchip *) fgmap.dchip;

/* Figure out number plans */

	Mixed_up_planes = 0;
	fgdchip->dchip_setup_0 = 0x7F05;
	fgdchip->dchip_setup_1 = 0x7F06;
	fgdchip->dchip_setup_2 = 0x7F07;
	fgdchip->dchip_setup_3 = 0x7F08;
	fgdchip->dchip_setup_6 = 0x7F09;

	d0 = fgdchip->dchip_setup_0 & 0x7F0F;
	d1 = fgdchip->dchip_setup_1 & 0x7F0F;
	d2 = fgdchip->dchip_setup_2 & 0x7F0F;
	d3 = fgdchip->dchip_setup_3 & 0x7F0F;
	d6 = fgdchip->dchip_setup_6 & 0x7F0F;
	if( d0 == 0x7F05 ) {
		if( d3 == 0x7F08 )  fg_num_planes = 24;
    		else if( d1 == 0x7F06 ) {
      			if( d2 == 0x7F07 ) {
				fg_num_planes = 12;
        			Mixed_up_planes = 1;
			}
      		     else      fg_num_planes = 8;
    		}
    		else      fg_num_planes = 4;
  	}
	else {
		Mixed_up_planes = 1;
		if( d2 == 0x7F07 )  fg_num_planes = 4;
		else if( d6 == 0x7F09 )     fg_num_planes = 20;
	}
	Width = MAX_SCREEN_X;
	Height = MAX_SCREEN_Y;

}











/******************************************************************
 **                                                              **
 ** Routine to initialize the D chip                             **
 **                                                              **
 ******************************************************************/

fg_init_dchip(fgdchip)
register struct dchip *fgdchip;
{
	fgdchip->dchip_red_start = Fixed (fgred);
	fgdchip->dchip_green_start = Fixed (fggreen);
	fgdchip->dchip_blue_start = Fixed (fgblue);
	fgdchip->dchip_span_step_red = 0;
	fgdchip->dchip_span_step_green = 0;
	fgdchip->dchip_span_step_blue = 0;
	fgdchip->dchip_edge_step_red = 0;
	fgdchip->dchip_edge_step_green = 0;
	fgdchip->dchip_edge_step_blue = 0;

	fgdchip->dchip_src_start_error = 0;
	fgdchip->dchip_src_start_int = 0;
	fgdchip->dchip_src_edge_error = 0;
	fgdchip->dchip_src_edge_int = 0;
	fgdchip->dchip_src_span_step_error = 0;
	fgdchip->dchip_src_span_step_int = 0;

	fgdchip->dchip_test_0 = 0;
	fgdchip->dchip_test_1 = 0;
	fgdchip->dchip_test_2 = 0;
	fgdchip->dchip_test_3 = 0;
	fgdchip->dchip_test_4 = 0;
	fgdchip->dchip_test_5 = 0;

/*
 * In case a bad dchip caused a wrong configuration indication, we better
 * try to turn off all dchips even if we don't think they are there.
 */

        fgdchip->dchip_setup_0 = I_NIBBLE_15;
        fgdchip->dchip_setup_1 = I_NIBBLE_15;
        fgdchip->dchip_setup_2 = I_NIBBLE_15;
        fgdchip->dchip_setup_3 = I_NIBBLE_15;
        fgdchip->dchip_setup_4 = I_NIBBLE_15;
        fgdchip->dchip_setup_5 = I_NIBBLE_15;
        fgdchip->dchip_setup_6 = I_NIBBLE_15;
        fgdchip->dchip_setup_7 = I_NIBBLE_15;
        fgdchip->dchip_setup_8 = I_NIBBLE_15;
        fgdchip->dchip_setup_9 = I_NIBBLE_15;
        fgdchip->dchip_setup_10 = I_NIBBLE_15;
        fgdchip->dchip_setup_11 = I_NIBBLE_15;
        fgdchip->dchip_setup_12 = I_NIBBLE_15;
        fgdchip->dchip_setup_13 = I_NIBBLE_15;
        fgdchip->dchip_setup_14 = I_NIBBLE_15;
        fgdchip->dchip_setup_15 = I_NIBBLE_15;
/*
 * TODO
 *  do it for 24 chips 
 */

	if (fg_num_planes == 4) {  /* 4 planes */
	    if (Mixed_up_planes) {
		fgdchip->dchip_setup_2 = I_NIBBLE_2 | Z_NIBBLE_5 | RED_PLANE |
				HIGH_NIBBLE | MASTER_DCHIP | NO_LOW_NIBBLE;

	    }
	    else {
		fgdchip->dchip_setup_0 = I_NIBBLE_0 | Z_NIBBLE_5 | RED_PLANE |
				HIGH_NIBBLE | MASTER_DCHIP | NO_LOW_NIBBLE;
	    }
	}
	else if (fg_num_planes == 12) {  /* 12 planes */
 /* red */
	    fgdchip->dchip_setup_2 = I_NIBBLE_2 | Z_NIBBLE_3 | RED_PLANE |
	    			HIGH_NIBBLE | MASTER_DCHIP | NO_LOW_NIBBLE;
 /* green */
	    fgdchip->dchip_setup_1 = I_NIBBLE_1 | Z_NIBBLE_5 | GREEN_PLANE |
	    			HIGH_NIBBLE;
 /* blue */
	    fgdchip->dchip_setup_0 = I_NIBBLE_0 | Z_NIBBLE_4 | BLUE_PLANE |
	    			HIGH_NIBBLE;
	}
	else if (fg_num_planes == 20) {  /* 20 planes */
 /* red */
	    fgdchip->dchip_setup_6 = I_NIBBLE_6 | Z_NIBBLE_1 | RED_PLANE |
	    			HIGH_NIBBLE | MASTER_DCHIP | NO_LOW_NIBBLE;
 /* green */
	    fgdchip->dchip_setup_2 = I_NIBBLE_2 | Z_NIBBLE_2 | GREEN_PLANE;
	    fgdchip->dchip_setup_3 = I_NIBBLE_3 | Z_NIBBLE_3 | GREEN_PLANE|
				HIGH_NIBBLE;
 /* blue */
	    fgdchip->dchip_setup_4 = I_NIBBLE_4 | Z_NIBBLE_4 | BLUE_PLANE;
	    fgdchip->dchip_setup_5 = I_NIBBLE_5 | Z_NIBBLE_5 | BLUE_PLANE |
				HIGH_NIBBLE;
	}
	else if (fg_num_planes == 8) {  /* 8 planes */
 /* red */
	    fgdchip->dchip_setup_0 = I_NIBBLE_0 | Z_NIBBLE_4 | RED_PLANE |
	    			MASTER_DCHIP;
	    fgdchip->dchip_setup_1 = I_NIBBLE_1 | Z_NIBBLE_5 | RED_PLANE |
				HIGH_NIBBLE;
	}
	else { /* must be 24 planes */
 /* red */
	    fgdchip->dchip_setup_0 = I_NIBBLE_0 | Z_NIBBLE_0 | RED_PLANE |
	    			MASTER_DCHIP;
	    fgdchip->dchip_setup_1 = I_NIBBLE_1 | Z_NIBBLE_1 | RED_PLANE |
				HIGH_NIBBLE;
 /* green */
	    fgdchip->dchip_setup_2 = I_NIBBLE_2 | Z_NIBBLE_2 | GREEN_PLANE;
	    fgdchip->dchip_setup_3 = I_NIBBLE_3 | Z_NIBBLE_3 | GREEN_PLANE|
				HIGH_NIBBLE;
 /* blue */
	    fgdchip->dchip_setup_4 = I_NIBBLE_4 | Z_NIBBLE_4 | BLUE_PLANE;
	    fgdchip->dchip_setup_5 = I_NIBBLE_5 | Z_NIBBLE_5 | BLUE_PLANE |
				HIGH_NIBBLE;
	}

/*
 * Setup the logic registers
 */
	fgdchip->dchip_logical_z_0 =  -1;
	fgdchip->dchip_logical_z_1 =  -1;
	fgdchip->dchip_logical_z_2 =  -1;
	fgdchip->dchip_logical_z_3 =  -1;

/*
 * Setup control registers
 */
	fgdchip->dchip_control_red =  0x00000000;
	fgdchip->dchip_control_green =  0x00000000;
	fgdchip->dchip_control_blue =  0x00000000;
	fgdchip->dchip_control_src =  0x0000;

}







/******************************************************************
 **                                                              **
 ** Routine to initialize the T chip                             **
 **                                                              **
 ******************************************************************/

fg_init_tchip(fgtchip)
register struct tchip *fgtchip;
{
	int	loop_count;

	fgtchip->tchip_csr = 0;  /* reset and blank */
	fgtchip->tchip_csr = 0xD000 | ONE_LOAD;
        fgtchip->tchip_vertical_int = 512;
        fgtchip->tchip_int_reg = 7;
        fgtchip->tchip_y_start = 0;
        fgtchip->tchip_x_start = 0;
        fgtchip->tchip_table_y_start = FG_TABLE_Y;
        fgtchip->tchip_table_x_start = MAX_SCREEN_X / 16; /* word boundry for start */
        fgtchip->tchip_table_cntl_reg = (22 << 7) | (3<<3) | 3;
	fg_monitor_timing(fgtchip);
	if (Mixed_up_planes) {
	    if(fg_num_planes == 20) {

/* lego data is in planes 24:27 (DCHIP # 6) */

		fg_lego_p0_mask = 0x01000000;
		fg_lego_p0 = 24;
	    }
	    else {

/* lego data is in planes 8:11 (DCHIP # 2) */

		fg_lego_p0_mask = 0x0100;
		fg_lego_p0 = 8;
	    }
	}
	else {

/* lego data is in planes 0:3 (DCHIP # 0) */

	    fg_lego_p0_mask = 0x01;
	    fg_lego_p0 = 0;
	}

/* Load lego data into bitmap */

	fg_init_wcchip(fgtchip);	/* window cursor */
	fg_init_pmchip(fgtchip);	/* pixel map */
	fg_init_vdac(fgtchip);		/* vdac */

/* Load lego data from bitmap into legos */

/*	fg_load_lego (WC_LOAD | VDAC_LOAD | PMAP_LOAD);*/
        fgtchip->tchip_csr |= WC_LOAD | VDAC_LOAD | PMAP_LOAD;
        loop_count = 10000;
loop2: if (fgtchip->tchip_csr & (WC_LOAD | VDAC_LOAD | PMAP_LOAD)) {
            if (!(--loop_count)) {
                loop_count = 100;
         mprintf("\n\nWaiting for LEGO load! CSR = %08X.",fgtchip->tchip_csr);
            }
            goto loop2;
        }

}










/******************************************************************
 **                                                              **
 ** Routine for monitor timing                                   **
 **                                                              **
 ******************************************************************/

fg_monitor_timing(fgtchip)
register struct tchip *fgtchip;
{

	int	vbs, vbf, vss, vsf, hbs, hbf, hss, hsf, hs2;

/* Vertical timing in lines */

	vbs = 1024;
	vbf = 1059;
	vss = 1024 + 1;
	vsf = 1027 + 1;

/* Horizantal timing in nibbles */

	hbs = 319;
	hbf = 404;
	hss = 324;
	hsf = 364;
	hs2 = 304;

/* Adjust Hblank */

	hbs -= 6;
	hbf -= 6;
	hss += 7;
	hsf += 7;
	hs2 += 7;

	fgtchip->tchip_mon_cntl_reg0 = (hss << 11) | vbs;
	fgtchip->tchip_mon_cntl_reg1 = ((hss << 11-5) & 0x7800) | vbf;
	fgtchip->tchip_mon_cntl_reg2 = (hsf << 11) | vss;
	fgtchip->tchip_mon_cntl_reg3 = ((hsf << 11-5) & 0x7800) | vsf;
	fgtchip->tchip_mon_cntl_reg4 = (hs2 << 11) | hbs;
	fgtchip->tchip_mon_cntl_reg5 = ((hs2 << 11-5) & 0x7800) | hbf;
}




fg_init_wcchip(fgtchip)
register struct tchip *fgtchip;
{
	register struct dchip *fgdchip;
	int	save_mask;
	int	x, y, count;

        fgdchip = (struct dchip *) fgmap.dchip;
	save_mask = fgdchip->dchip_write_mask;
	fg_wc_offset = 236;
        fgdchip->dchip_write_mask = fg_lego_p0_mask << 3;
        fgdchip->dchip_read_mask = fg_lego_p0 + 3;
	x = MAX_SCREEN_X / 16;
	y = 3 + FG_TABLE_Y;
/*
 * cursor data (2 planes of 32x32)
 */

	x /= 2;		/* need long word address */
	for (count = 0; count < 64; count++) {
	    fg_wt_lego_long(fg_def_cursor[count], &x, &y);
	}
	x *= 2;		/* back to word address */
/* NOTE:
 * Do the following two lines only if we are not loading the cursor
 *
	x += 32;
	y += 2;
 */

/* cursor clipping */
	fg_wt_lego_short(fg_wc_offset, &x, &y);
	fg_wt_lego_short(Width+fg_wc_offset, &x, &y);
	fg_wt_lego_short(0, &x, &y);
	fg_wt_lego_short(Height, &x, &y);

/* cursor position, middle of screen for now */

	fg_wt_lego_short((Width/2)+fg_wc_offset, &x, &y);
	fg_wt_lego_short(Height/2, &x, &y);

/* cursor mode, don't extend clip to cursor clip */

	fg_wt_lego_short(0x0020, &x, &y);

/* window mode, default window is 0 */

	fg_wt_lego_short(0, &x, &y);

/* window Xmin, Xmax, Ymain, Ymax 
 * window 0 is for screen */

	fg_wt_lego_short(fg_wc_offset, &x, &y);
	fg_wt_lego_short(Width+fg_wc_offset, &x, &y);
	fg_wt_lego_short(0, &x, &y);
	fg_wt_lego_short(Height, &x, &y);

/* window 1 through 63 are not zero width/height for now */

	for (count = 1; count < 64; count++) {
	    fg_wt_lego_short(0, &x, &y);
	    fg_wt_lego_short(0, &x, &y);
	    fg_wt_lego_short(0, &x, &y);
	    fg_wt_lego_short(0, &x, &y);
	}

/* system configuration, active low sync and blank, window delay = 1,
 * double buffer delay = 7.
 */

/*	fg_wt_lego_short(0x1F00, &x, &y);*/
	fg_wt_lego_short(0x0300, &x, &y);

/* test mask, test control, signature */

	fg_wt_lego_short(0, &x, &y);
	fg_wt_lego_short(0, &x, &y);
	fg_wt_lego_short(0, &x, &y);
	fgdchip->dchip_write_mask = save_mask;
}


fg_init_pmchip(fgtchip)
register struct tchip *fgtchip;
{
	register struct dchip *fgdchip;
	int	save_mask;
	int	x, y, count;

        fgdchip = (struct dchip *) fgmap.dchip;
	save_mask = fgdchip->dchip_write_mask;

	fgdchip->dchip_write_mask = fg_lego_p0_mask | (fg_lego_p0_mask<<1) |
		  	    (fg_lego_p0_mask<<2) | (fg_lego_p0_mask<<3);
	fgdchip->dchip_read_mask = fg_lego_p0;

	x = MAX_SCREEN_X /16;
	y = FG_TABLE_Y;

/* 64 longword config registers */

	x /= 2;		/* need long word address */

/* don't need more than 10 planes for 10 bit vdac */

	fg_Wplanes = fg_num_planes < 10 ? fg_num_planes : 10;
	fg_wt_lego_long(0x00000000 | (fg_Wplanes << 12), &x, &y);
	for (count = 1; count < 64; count++) {
	    fg_wt_lego_long(0, &x, &y);
	}
	x *= 2;		/* back to word address */

	fg_wt_lego_short(0, &x, &y);

/* background base used for "invalid" planes */

	fg_wt_lego_short(0, &x, &y);

/* system configuration, no delay difference between MAP_SEL and PIX_IN */

	if (fg_num_planes == 12)  /* 12 planes */
	    fg_wt_lego_short(1, &x, &y);
	else
	    fg_wt_lego_short(0, &x, &y);

/* test mask, test control, signature */

	fg_wt_lego_short(0, &x, &y);
	fg_wt_lego_short(0, &x, &y);
	fg_wt_lego_short(0, &x, &y);
	fgdchip->dchip_write_mask = save_mask;
}


fg_init_vdac(fgtchip)
register struct tchip *fgtchip;
{
	register struct dchip *fgdchip;
	int	save_mask;
	int	x, y, count;

        fgdchip = (struct dchip *) fgmap.dchip;
	save_mask = fgdchip->dchip_write_mask;

/* for now load red, green, and blue all the same */

	fgdchip->dchip_write_mask = fg_lego_p0_mask | (fg_lego_p0_mask<<1) |
		  	    (fg_lego_p0_mask<<2);
	fgdchip->dchip_read_mask = fg_lego_p0;

	x = MAX_SCREEN_X / 16;
	y = 3 + FG_TABLE_Y;

	fg_init_color_map (&x, &y);


/* cursor (overlay) colors 0, 1, 2, and 3 (is 0 still transparent?) */

	fg_wt_lego_short (FG_BROOK_LOW(0x100), &x, &y);
	fg_wt_lego_short (FG_BROOK_HIGH(0x100), &x, &y);
	fg_wt_lego_short (2 | (0 <<4), &x, &y);
	fg_wt_lego_short (FG_BROOK_LOW(0x101), &x, &y);
	fg_wt_lego_short (2 | (0x80 <<4), &x, &y);
	fg_wt_lego_short (FG_BROOK_LOW(0x102), &x, &y);
	fg_wt_lego_short (2 | (0x80 <<4), &x, &y);
	fg_wt_lego_short (FG_BROOK_LOW(0x103), &x, &y);
	fg_wt_lego_short (2 | (0xFF <<4), &x, &y);

/* CR 0, 4:1, cursor 0 transparent */

	fg_wt_lego_short (FG_BROOK_LOW(0x201), &x, &y);
	fg_wt_lego_short (FG_BROOK_HIGH(0x201), &x, &y);
	fg_wt_lego_short (2 | (0x40 <<4), &x, &y);

/* CR 1, no panning */

	fg_wt_lego_short (FG_BROOK_LOW(0x202), &x, &y);
	fg_wt_lego_short (2 | (0 <<4), &x, &y);

/* CR 2, enable SYNC, normal pallete loads, blank pedistal */

	fg_wt_lego_short (FG_BROOK_LOW(0x203), &x, &y);
	fg_wt_lego_short (2 | (0xC0 <<4), &x, &y);

/* PIX_RD_MASK L */

	fg_wt_lego_short (FG_BROOK_LOW(0x204), &x, &y);
	fg_wt_lego_short (2 | (0xFF <<4), &x, &y);

/* PIX_RD_MASK H */

	fg_wt_lego_short (FG_BROOK_LOW(0x205), &x, &y);
	fg_wt_lego_short (2 | (0x03 <<4), &x, &y);

/* PIX_BLINK_MASK L */

	fg_wt_lego_short (FG_BROOK_LOW(0x206), &x, &y);
	fg_wt_lego_short (2 | (0 <<4), &x, &y);

/* PIX_BLINK_MASK H */

	fg_wt_lego_short (FG_BROOK_LOW(0x207), &x, &y);
	fg_wt_lego_short (2 | (0 <<4), &x, &y);

/* OVERLAY_RD_MASK */

	fg_wt_lego_short (FG_BROOK_LOW(0x208), &x, &y);
	fg_wt_lego_short (2 | (0x03 <<4), &x, &y);

/* OVERLAY_BLINK_MASK */

	fg_wt_lego_short (FG_BROOK_LOW(0x209), &x, &y);
	fg_wt_lego_short (2 | (0x0 <<4), &x, &y);

/* TEST */

	fg_wt_lego_short (FG_BROOK_LOW(0x20C), &x, &y);
	fg_wt_lego_short (2 | (0 <<4), &x, &y);

	while (y < 25 + FG_TABLE_Y)
		fg_wt_lego_short (0, &x, &y);  /* nop's */

	fgdchip->dchip_write_mask = save_mask;

}





fg_init_color_map(x, y)
     int	*x;
     int	*y;
{
  short	ramp;
  int	count;
  
  fg_wt_lego_short (FG_BROOK_LOW(0), x, y);
  fg_wt_lego_short (FG_BROOK_HIGH(0), x, y);
  for (ramp = 0, count = 0; count < 1024; count++) {
    if (ramp > 255) ramp = 0;
    fg_wt_lego_short(3 | (ramp << 4), x, y);
    
    /* make 256 through 511 inc. by 16 for a 12 plane direct */
    
    if ((count >= 256) && (count < 512))
      ramp += 16;
    else
      ramp++;
  }
}


fg_wt_lego_long (data, x, y)
u_long	data;
int	*x;
int	*y;
{
  int	i,j,k,tmp;
  register struct achip *fgachip;
  register struct dchip *fgdchip;
  register struct tchip *fgtchip;
  long int *ivram;
  long int write_mask;

  fgachip = (struct achip *) fgmap.achip;
  fgdchip = (struct dchip *) fgmap.dchip;
  fgtchip = (struct tchip *) fgmap.tchip;
  ivram = (long int *) (fgmap.i_vram) + *x*32 + (*y * 2048);
  
  write_mask = fgdchip->dchip_write_mask & 0xf;
  
  for (i=0, j=1;  i<sizeof(data)*8;  i++, j=j<<1, ivram++ )
    {
      tmp = *ivram & ~write_mask;
      if (data & j)
	tmp |= write_mask;
      *ivram = tmp;
    }
  if ( ++(*x) >= (2048 / 32)) 
    {
      *x = MAX_SCREEN_X / 32;
      ++(*y);
    }
}

fg_wt_lego_short (data, x, y)
u_short	data;
int	*x;
int	*y;
{
  int	i,j,k,tmp;
  register struct achip *fgachip;
  register struct dchip *fgdchip;
  register struct tchip *fgtchip;
  long int *ivram;
  long int write_mask;

  fgachip = (struct achip *) fgmap.achip;
  fgdchip = (struct dchip *) fgmap.dchip;
  fgtchip = (struct tchip *) fgmap.tchip;
  ivram = (long int *) (fgmap.i_vram) + *x*16 + (*y * 2048);
  
  write_mask = fgdchip->dchip_write_mask & 0xf;
  
  for (i=0, j=1;  i<sizeof(data)*8;  i++, j=j<<1, ivram++ )
    {
      tmp = *ivram & ~write_mask;
      if (data & j)
	tmp |= write_mask;
      *ivram = tmp;
    }
  if ( ++(*x) >= (2048 / 16)) 
    {
      *x = MAX_SCREEN_X / 16;
      ++(*y);
    }
}


fg_load_lego (bits_set)
int     bits_set;
{
        register struct tchip *fgtchip;

        fgtchip = (struct tchip *) fgmap.tchip;
        fgtchip->tchip_csr |= bits_set;

/* if bit is clear it means that we are currently in blank, so we have
 * to wait till were out
 */
        if( !(fgtchip->tchip_csr & bits_set) )    fg_wait_active( );

/* Wait thru one blank (ie:till edge of active) (ie:load is complete) */

        fg_wait_active( );

/* Lets check to make sure it realy happened, for now */

        if(fgtchip->tchip_csr & bits_set) {
            mprintf( "\n\007ERROR:Couldn't load LEGO! CSR=%08X.",  fgtchip->tchip_csr);
        }
}


fg_wait_active()
{
        register struct tchip *fgtchip;

        fgtchip = (struct tchip *) fgmap.tchip;

        fgtchip->tchip_int_reg = VBF_IRQ; /* clear Vblank bit */

/* wait for vblank */

loop1: if( !(fgtchip->tchip_int_reg & VBF_IRQ) )  goto loop1;
DELAY(100);
}


fg_clip(x1, y1, x2, y2)
int     x1, y1, x2, y2;
{
        register struct achip *fgachip;

        fgachip = (struct achip *) fgmap.achip;

        fg_wait_done();

        fgachip->achip_clip_x_1 = x1;
        fgachip->achip_clip_y_1 = y1;
        fgachip->achip_clip_x_2 = x2;
        fgachip->achip_clip_y_2 = y2;
}

fg_lego_address( byte_addr, y_offset )
int     byte_addr, y_offset;
{
  int   temp;
  /*
   *    48 is the table width of 768 divided by 16.
   *    80 is the table start x of 1280 divided by 16.
   *    128 is the bitmap width divided by 16.
   */

  return(

  /* word_addr */
  ((((byte_addr / 2)

  /* logical y */
  / 48)

  /* PLUS phys y */
  + y_offset)

  /* convert y to words from 0,0 */
  * 128)

  /*   word_addr,   logical x */
  + (( (byte_addr/2) % 48)

  /* phys x */
  + 80)
  );
}






/******************************************************************
 **                                                              **
 ** Routine to save some of the hardware registers. This routine **
 ** will be called as the console program enters its console     **
 ** display (halting the cpu). This routine will be called with  **
 ** a JSB instruction at IPL 31, in kernel mode with memory      **
 ** management disabled.                                         **
 **                                                              **
 ******************************************************************/
fg_save () {
  asm ("  pushr   $0xfff");                   /* save registers*/
  fg_phys_tchip = (struct tchip *) (FG_PHYS_TCHIP);
  fg_savebuf[2] = fg_phys_tchip->tchip_csr;
  fg_savebuf[3] = fg_phys_tchip->tchip_y_start;
  fg_savebuf[4] = fg_phys_tchip->tchip_table_y_start;
  fg_savebuf[5] = fg_phys_tchip->tchip_x_start;
  fg_savebuf[6] = fg_phys_tchip->tchip_table_x_start;
  fg_savebuf[7] = fg_phys_tchip->tchip_table_cntl_reg;
  fg_savebuf[8] = fg_phys_tchip->tchip_mon_cntl_reg0;
  fg_savebuf[9] = fg_phys_tchip->tchip_mon_cntl_reg1;
  fg_savebuf[10] = fg_phys_tchip->tchip_mon_cntl_reg2;
  fg_savebuf[11] = fg_phys_tchip->tchip_mon_cntl_reg3;
  fg_savebuf[12] = fg_phys_tchip->tchip_mon_cntl_reg4;
  asm ("  popr    $0xfff");                    /* restore reg */
  asm ("  rsb"); 
}


/******************************************************************
 **                                                              **
 ** Routine to restore some of the hardware registers. This      **
 ** routine will be called as the console program exits its      **
 ** console display. This routine will be called with a JSB      **
 ** instruction at IPL 31, in kernel mode with memory management **
 ** disabled.                                                    **
 **                                                              **
 ******************************************************************/
fg_restore () {
  asm ("  pushr   $0xfff");                   /* save registers*/
  fg_phys_tchip = (struct tchip *) (FG_PHYS_TCHIP);
  fg_phys_tchip->tchip_csr &= ~0x1000;
  fg_phys_tchip->tchip_y_start = fg_savebuf[3];
  fg_phys_tchip->tchip_table_y_start = fg_savebuf[4];
  fg_phys_tchip->tchip_x_start = fg_savebuf[5];
  fg_phys_tchip->tchip_table_x_start = fg_savebuf[6];
  fg_phys_tchip->tchip_table_cntl_reg = fg_savebuf[7];
  fg_phys_tchip->tchip_mon_cntl_reg0 = fg_savebuf[8];
  fg_phys_tchip->tchip_mon_cntl_reg1 = fg_savebuf[9];
  fg_phys_tchip->tchip_mon_cntl_reg2 = fg_savebuf[10];
  fg_phys_tchip->tchip_mon_cntl_reg3 = fg_savebuf[11];
  fg_phys_tchip->tchip_mon_cntl_reg4 = fg_savebuf[12];
  fg_phys_tchip->tchip_csr = fg_savebuf[2] | WC_LOAD | VDAC_LOAD | PMAP_LOAD;
  for (fg_savei=0; fg_savei<1000; fg_savei++);
  fg_phys_fcaddr->fclpr = (SER_RXENAB | SER_KBD | SER_SPEED | SER_CHARW);
  fg_savei = SER_POINTER | SER_CHARW | SER_PARENB | SER_ODDPAR | SER_SPEED | SER_RXENAB;
  fg_phys_fcaddr->fclpr = fg_savei;
}

/*****************************************************************************
 * fg_copy_planes 
 *
 * This function will copy a rectangle area  (width x height pixels) of planes
 * (planemask) from  point address (src_x,   src_y) to point   address (dst_x,
 * dst_y).  from  one location  to another.    Note copying from  one  area to
 * another area where the areas overlap can destroy the  underlying area.  For
 * example, fg_copy_planes ( 0, 0, 0, 0, 100, 100) will produce unpredicatable
 * results.    However, fg_copy_planes  (0,  10, 0,  0,  100, 100)  will  work
 * correctly.
 *
 * Note, the routine does various forms of optimization.  One might cause some
 * grief.  When the  area dimension  exceeds  the hardware capability  then it
 * will be broken up.  One can move horizontal lines height times  or vertical
 * lines  width  times.  The  loop  requireing  the  least  amount of graphics
 * operations is used.  This  means  fg_copy  (0,  10, 0,  0, 100,  200) would
 * product    upredictable    results    because it would read and
 * write the same vertical line.
 *****************************************************************************/
fg_copy_planes ( src_x, src_y, dst_x, dst_y, width, height, planemask )
unsigned int src_x, src_y, dst_x, dst_y, width, height, planemask;
{
  register struct achip *fgachip;
  register struct dchip *fgdchip;

  int i,x,y;

/* get pointers and save registers */ 

  fgachip = (struct achip *) fgmap.achip;
  fgdchip = (struct dchip *) fgmap.dchip;

  fg_wait_done();

/* Set up Dchip registers

  fgachip->csr = 0;                     /* Pattern mode = block mode */
  fgdchip->dchip_logical_z_0 = planemask;     /* operation = */
  fgdchip->dchip_logical_z_1 = planemask;     /* SRC (ie: pattern buffer) */
  fgdchip->dchip_logical_z_2 = 0;
  fgdchip->dchip_logical_z_3 = 0;
  fgdchip->dchip_control_src = 0x0001;        /* src op = "Ipat = Imem"

  fgachip->achip_x_i = 0;
  fgachip->achip_y_i = 0;
  fgachip->achip_x_offset_src = 0;
  fgachip->achip_y_offset_src = 0;
  fgachip->achip_x_offset_dst = dst_x - src_x;
  fgachip->achip_y_offset_dst = dst_y - src_y;

/*
 * The LEGSS hardware can copy a maximum of 2048 pixels.  For performance if 
 * the area is within that range then this section will perform the copy in
 * one graphics operation.
 */

  if (width*height <= 2048)
    {
      fgachip->achip_x_a = Fixed(src_x);
      fgachip->achip_y_a = Fixed(src_y);
      fgachip->achip_x_size = width;
      fgachip->achip_y_size = height;
      fgachip->achip_counter = 1;
      fg_wait_done();
    }

/*
 * Area is greater than 2048 pixels!  The copy must be broken up into width 
 * increments.  The next two ifs minimize the number of graphics operations by
 * using the width and height information.  If width is bigger then horizontal
 * lines are moved height times.  If height is bigger then vertical lines are 
 * moved width times.
 */
  
  else
    if (width >= height)
      {
	fgachip->achip_x_a = Fixed(src_x);
	fgachip->achip_x_size = width;
	fgachip->achip_y_size = 1;
	y = src_y;
	for ( i = 0; i < height; i++)
	  {
	    fgachip->achip_y_a = Fixed(y);
	    fgachip->achip_counter = 1;
	    fg_wait_done();
	    y++;
	  }
      }

    else
      {
	fgachip->achip_y_a = Fixed(src_y);
	fgachip->achip_x_size = 1;
	fgachip->achip_y_size = height;
	x = src_x;
	for ( i = 0; i < width; i++)
	  {
	    fgachip->achip_x_a = Fixed(x);
	    fgachip->achip_counter = 1;
	    fg_wait_done();
	    x++;
	  }

      }

/*
 * cleanup and restore registers
 */

  fgachip->achip_x_offset_src = 0;
  fgachip->achip_y_offset_src = 0;
  fgachip->achip_x_offset_dst = 0;
  fgachip->achip_y_offset_dst = 0;
}
#endif
