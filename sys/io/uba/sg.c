#ifndef lint
static char *sccsid = "@(#)sg.c	4.1      (ULTRIX)        7/2/90";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1985-1989 by			*
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
 * 23-Feb-90 -- sekhar
 *      Merged Joe Martin's fix for 3.1 cld. When copying user PTEs,
 *      check for page crossing and reevaluate vtopte.
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
 *	When going to physical mode (crash dump) or closing the graphics
 *	device (server going down), the video could be turned
 *	off by the server (screen saver). Therefore we turn the video on.
 *
 * 08-May-89 -- rafiey (Ali Rafieymehr)
 *	When going to single user mode, the keys would behave as if the
 *	<Ctrl> was also pressed. We now clear the flags for shift and
 *	control in sgclose routine.
 *
 * 08-May-89 -- rafiey (Ali Rafieymehr)
 *	Fixed a bug in mapping pages for color_buf. The driver was not
 *	mapping enough pages to be used by color_buf on eight plane
 *	machines.
 *
 * 08-May-89 - rafiey (for Fred Canter)
 *	Fix stray interrupts thru SCB vector 0x1fc caused by polling
 *	with interrupts enabled while sending characters to
 *	the keyboard in sg_key_out().
 *
 * 08-May-89 -- rafiey (Ali Rafieymehr)
 *	The input interrupt routine (sgiint) was printing error messages when
 *	the event queue was full (in high IPL). These error messages are not
 *	really usefull.	The solution is to increase the size of the event
 *	queue (this should be done in the next release). For now there is no
 *	need to print the error	messages, therefore the "overflow" print
 *	statements are commented out.
 *
 * 16-Feb-89 - Mark Parenti
 *
 *	Remove xinfo and xoutput. They were xos remnants.
 *
 * 13-Sep-88 - Ali Rafieymehr
 *
 *	Fixed a bug which was causing the "select" not to work for
 *	alternate console.
 *
 * 17-Aug-88 - Ali Rafieymehr
 *
 *	Fixed a bug in wait_status() routine. We were not waiting long enough
 *	for the vertical sync. status bit to be set.
 *
 *  5-Aug-88 - Tim Burke
 *
 *	Return the 2 character sequence 0377, 0377 upon receipt of a valid
 *	0377 character only when PARMRK is set under the termio line disc.
 *
 *  14-July-88 - Vasudev K. Bhandarkar
 *
 *      Changed TOY from 16 bits to 32 bits.  X11 server needs 32-bits
 *      of detail associated with event time stamps.
 *
 * 13-Jun-88 -- rafiey (Ali Rafieymehr)
 *	Fixed the tablet problem. Tablet wasn't initialized correctly.
 *
 * 07-Mar-88 -- vasudev (Vasudev K. Bhandarkar)
 *	X11 changes.  Made the GPX accessible to user processes, such
 *      as the X11 server.
 *
 * 15-Feb-88 - Fred Canter
 *	Changes for VAX420 (CVAXstar/PVAX) support.
 *
 *  1-Dec-87 - Tim Burke
 *
 *	Added support for both System V termio(7) and POSIX termios(7).  These
 *	changes also include support for 8-bit canonical processing.  Changes
 *	involve:
 *
 *	- Default settings on first open depend on mode of open.  For termio
 *	  opens the defaults are "RAW" style, while non-termio opens default
 *	  to the traditional "cooked" style.
 *	- The driver now represents its terminal attributes and special 
 *	  characters in the POSIX termios data structure.  This contrasts the
 *	  original approach of storing attributes and special chars in the
 *	  t_flags, ltchars and tchars.
 *	- Addition of LPASS8 to local mode word for 8-bit canonical support.
 *
 * 28-Oct-87 -- Ali Rafieymehr
 *	Changed the logic for turning the video on/off for the 8 plane.
 *	Turning video on/off didn't work with the new board.
 *
 * 12-Oct-87 - Tim Burke
 *	Modified driver to provide 8-bit support to the console port.  To do 
 *	this characters are defined in the remainder of the first 15 rows of
 *	off screen memory as well as in next 15 rows.
 *
 *  21-Jun-87 - Tim Burke
 *
 *      Added full TERMIO functionality to terminal subsystem.
 *      Changed open routine to setup propper default settings if the line
 *      is not being used as a graphics device.  Added defines of defaults.
 *
 * 18-Aug-87 -- Ali Rafieymehr
 *	Included the 8 plane support. Also modified "sg_save()" and
 *	"sg_restore()" to save and restore color maps during "halt"
 *	and "continue" process. Fixed a bug in the sgclose() routine.
 *	It was not changing the cursor and color maps when X was killed.
 *
 *  4-Aug-87 -- Ali Rafieymehr
 *	Moved sg_next_fifo declaration from "../qdddx/globals.h" to
 *	"../vaxuba/sguser.h".
 *
 *  3-Aug-87 -- Ali Rafieymehr
 *	Fixed a bug which was causing the system to hang if we tried
 *	to continue after the system was halted by pressing the halt
 *	switch.
 *
 *  1-Aug-87 -- Fred Canter
 *	Use TANDEM mode on sgscreen for flow control of writes to
 *	/dev/console.
 *
 * 27-Jul-87 -- Fred Canter
 *	Fix devioget bus and interface types.
 *
 *  9-Jun-87 -- Ali Rafieymehr
 *	Corrected the value for the cursor color (during power up).
 *
 *  3-Jun-87  -- Fred Canter
 *	Fixed a bug in the hold screen code. It was changing the
 *	TTSTART/TTSTOP bits in tp->t_state on hold screen and
 *	CTRL/S and CTRL/Q.
 *
 *  22-May-87 -- Ali Rafieymehr
 *	Corrected type of a variable and value loaded into the cbcsr
 *	register of the FCC.
 *
 *  14-May-87 -- Ali Rafieymehr
 *	Changed the interrupt routine to write directly to the FIFO
 *	rather than copying to the buffer first.
 *
 *  14-May-87 -- Fred Canter
 *	Bug fix to allow xcons to work (loop kernel messages to
 *	login window), pass sm_tty to ioctl(), not ss_tty.
 *	Changed default kern_loop state to on.
 *	Removed unused variable (nsg) and tty struct (sg_tty).
 *
 *  21-Apr-87 - Brian Stevens
 *	keep VSYNC on when graphics device is open, save color mapping work.
 *
 *  16-Apr-87 fred (Fred Canter)
 *	Brian Steven's Multi-head GPX support (yes really).
 *
 *  19-Mar-87 rafiey (Ali Rafieymehr)
 *	Fix putc so crash dumps work on color VAXstar.
 *	Fix screen strangess.
 *
 *  19-Mar-87  fred (Fred Canter for Ali Rafieymehr)
 *	Added X in the kernel support for color VAXstar.
 *
 *  02-Mar-87  rafiey (Ali Rafieymehr)
 *	The video being turned off wasn't fixed correctly before.
 *
 *  11-Feb-87  rafiey (Ali Rafieymehr)
 *	Fixed a bug which was not turning the video on. Cleaned the code
 *	to include ioctls for the color support, and changed the chip
 *	select due to change of the hardware document and more cleanup.
 *
 *  07-Jan-87  rafiey (Ali Rafieymehr)
 *	Added the cursor to the driver and did some cleanup.
 *
 *  13-Dec-86  rafiey (Ali Rafieymehr)
 *	Converted the first pass color driver in to a semi-working
 *	version, which runs of the prototype VAXstar color board.
 *
 *   3-Sep-86  fred (Fred Canter)
 *	Make sure probe fails if CPU not a VAXstation 2200.
 *	Select video interrupt source (instead of sh driver).
 *
 *   2-Jul-86  rafiey (Ali Rafieymehr)
 *	Changed SGMAJOR to 50 and removed unused code.
 *
 *  18-Jun-86  rafiey (Ali Rafieymehr)
 *	Created this VAXstar color driver.
 *	Derived from qd.c.
 *
 **********************************************************************/

#if NSG > 0 || defined(BINARY)
#include "../data/sg_data.c"	/* include external references to data file */

/*
 * Following allow sgputc to function in
 * the CPU in physical mode (during crash dump).
 * One way transition, can't go back to virtual.
 */
 
#define	VS_PHYSNEXUS	0x20080000
#define	VS_PHYSADDER	0x3c000000
#define	VS_PHYSFCC	0x3c000200
#define	VS_PHYSVDAC	0x3c000300
#define	VS_PHYSCURSOR	0x3c000400

int	sg_physmode = 0;
/*
 * Definitions needed to access the VAXstar SLU.
 * Couldn't include sreg.h (too many compiler errors).
 */
#define	sscsr	nb_sercsr
#define	ssrbuf	nb_serrbuf_lpr
#define	sslpr	nb_serrbuf_lpr
#define	sstcr	nb_sertcr.c[0]
#define	sstbuf	nb_sermsr_tdr.c[0]
#define	SS_TRDY		0x8000		/* Transmit ready */
#define	SS_RDONE	0x80		/* Receiver done		*/
#define SS_PE		0x1000		/* Parity error			*/
#define SS_FE		0x2000		/* Framing error		*/
#define SS_DO		0x4000		/* Data overrun error		*/
#define	SINT_ST		0100		/* SLU xmit interrupt bit	*/

/*
 * VAXstar (color option) register address offsets from start of VAXstar (color)
 * address space.
 */

#define	ADDER	0x0000    /* ADDER chip address */
#define	FCC	0x0200    /* Fifo Compression Chip address */
#define	VDAC	0x0300    /* Video DAC address */
#define	CUR	0x0400    /* CURsor chip address */
#define	VRBACK	0x0500    /* Video ReadBACK address */
#define	FIFORAM	0x8000    /* FIFO/template RAM */

/*
 * general defines
 */

#define SGPRIOR (PZERO-1)               /* must be negative */

#define FALSE	0
#define TRUE	1
#define CHAR_S	0xc7
#define CHAR_Q	0xc1


struct	uba_device *sgdinfo[NSG];
struct	mouse_report last_rep;
extern	struct	mouse_report current_rep;	/* now in ss.c */
extern	struct	tty	sm_tty;			/* now in ss.c */
extern	struct	tty	ss_tty[];

/*
 * macro to create a system virtual page number from system virtual adrs
 */

#define VTOP(x)  (((int)x & ~0xC0000000) >> PGSHIFT) /* convert address */
						     /* to system page # */

/*
 * Definition of the driver for the auto-configuration program.
 */
int	sgprobe(), sgattach(), sgfint(), sgaint(), sgiint();

u_short	sgstd[] = { 0 };
struct	uba_driver sgdriver =
	{ sgprobe, 0, sgattach, 0, sgstd, "sg", sgdinfo };

struct	sg_fifo_space	SG_bufmap[];
/*
 * v_consputc is the switch that is used to redirect the console cnputc to the
 * virtual console vputc.
 * v_consgetc is the switch that is used to redirect the console getchar to the
 * virtual console vgetc.
 */
extern (*v_consputc)();
extern (*v_consgetc)();

#define	CONSOLEMAJOR	0
#define SG_MAJOR        50

/*
 * Keyboard state
 */
struct sg_keyboard {
	int shift;			/* state variables	*/
	int cntrl;
	int lock;
	int hold;
	char last;			/* last character	*/
} sg_keyboard;

short sg_divdefaults[15] = { LK_DOWN,	/* 0 doesn't exist */
	LK_AUTODOWN, LK_AUTODOWN, LK_AUTODOWN, LK_DOWN,
	LK_UPDOWN,   LK_UPDOWN,   LK_AUTODOWN, LK_AUTODOWN, 
	LK_AUTODOWN, LK_AUTODOWN, LK_AUTODOWN, LK_AUTODOWN, 
	LK_DOWN, LK_AUTODOWN };

short sg_kbdinitstring[] = {		/* reset any random keyboard stuff */
	LK_AR_ENABLE,			/* we want autorepeat by default */
	LK_CL_ENABLE,			/* keyclick */
	0x84,				/* keyclick volume */
	LK_KBD_ENABLE,			/* the keyboard itself */
	LK_BELL_ENABLE,			/* keyboard bell */
	0x84,				/* bell volume */
	LK_LED_DISABLE,			/* keyboard leds */
	LED_ALL };
#define KBD_INIT_LENGTH	sizeof(sg_kbdinitstring)/sizeof(short)

#define TOY ((time.tv_sec * 1000) + (time.tv_usec / 1000))

int	sg_ipl_lo = 1;			/* IPL low flag			*/

extern	u_short	sm_pointer_id;	/* id of pointer device (mouse,tablet)-ss.c */
u_short	sg_mouseon = 0;                	/* Mouse is enable when 1 */
u_short	sg_open = 0;			/* graphics device is open when 1 */
u_short	sg_cur_reg = 0;  	  /* Register to keep track of cursor register bits*/
u_short	sg_vdac_reg = 0;	  /* Register to keep track of vdac mode register */
u_short	sgdebug = 0;			/* Debug is enable when #0 */

char	*sg_fifo_addr;
char	*sg_ptr;
u_short	req_length;
short	*sg_int_flag;
short	*change_section;	/* flag to indicate we are changing FIFO section */
u_short *sg_next_fifo;
u_short nbytes_req, nbytes_left;
extern	u_long	scr_ram_addr;	/* system scratch RAM phys. address defined */
 				/* in ss.c                                  */
short	sg_num_planes;
u_short	sgsave_regs[5];		/* Used by sg_save() , sg_restore() routines */
				/* to save and restore some hardware regs. */
struct adder *sg_addr;		/* Used by sg_save() , sg_restore() routines */
struct fcc *sg_fcc;		/* Used by sg_save() , sg_restore() routines */
struct vdac *sg_vdac;		/* Used by sg_save() routine */
struct nb4_regs *sgsys_scr_ram;	/* Used by sg_save() , sg_restore() routines */
short  *vdac_address;		/* Used by sg_save() routine */
short  sg_colormaps[256*4];
short  sg_red_save = 0;
short  sg_green_save = 0;
short  sg_blue_save = 0;
short  colormaps_index = 0;
short  num_colormaps;
short  sg_counter1 = 0;
short  sg_counter2;
short  sg_video_off = 0;
#define	REBOOT_IN_PROG	0x10
#define	CPMBX_REBOOT	(0x2<<2)

struct proc *rsel;			/* process waiting for select */

int	sgstart(), sgputc(), sggetc(), ttrstrt();
long	sg_save(), sg_restore();

/*
 * Keyboard translation and font tables
 */
extern  char *q_special[],q_font[];
extern  u_short q_key[],q_shift_key[];
#define FONT_OFFSET	((MAX_SCREEN_X/CHAR_WIDTH)*CHAR_HEIGHT)

extern	struct	nexus	nexus[];

/*
 * Default cursor (plane A and plane B)
 *
 */

unsigned  short def_cur[32] = { 

/* plane A */ 0x00FF, 0x00FF, 0x00FF, 0x00FF, 0x00FF, 0x00FF, 0x00FF, 0x00FF,
 	      0x00FF, 0x00FF, 0x00FF, 0x00FF, 0x00FF, 0x00FF, 0x00FF, 0x00FF,
/* plane B */ 0x00FF, 0x00FF, 0x00FF, 0x00FF, 0x00FF, 0x00FF, 0x00FF, 0x00FF,
              0x00FF, 0x00FF, 0x00FF, 0x00FF, 0x00FF, 0x00FF, 0x00FF, 0x00FF

	};

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

/******************************************************************
 **                                                              **
 ** Routine to see if the graphic device will interrupt.         **
 **                                                              **
 ******************************************************************/

extern	int	ws_display_type;
extern	int	ws_display_units;

sgprobe(reg)
	caddr_t reg;
{
	register struct nb_regs *sgaddr = (struct nb_regs *)nexus;
	register struct nb3_regs *sgaddr3 = (struct nb3_regs *)sgmem;
	register struct fcc *sgfcc;
	register struct vdac *sgvdac;

/*
 * Only on a VAXstation 2200 (not MicroVAX 2000)
 * Also on a CVAXstar.
 * TODO: MULTU bit meaning changed, may use ARCHID bits
 */
	if (((cpu != VAXSTAR) && (cpu != C_VAXSTAR)) ||
	    (vs_cfgtst&VS_MULTU))
		return(0);
/*
 * Only if color option present
 */
	if ((vs_cfgtst&VS_VIDOPT) == 0)
	    return(0);

/*
 * Only if SG is the graphics device.
 */

	if (ws_display_type && (ws_display_type != SG_DTYPE))
		return(0);

	sgaddr->nb_vdc_sel = 1;		/* select color option interrupts */
	sgaddr->nb_int_reqclr = SINT_VF;
	sgaddr->nb_int_msk |= SINT_VF;


	if (v_consputc != sgputc) {
	    sgbase = (caddr_t) ((u_long)sgaddr3);
	    sgmap.adder = sgbase + ADDER;
	    sgmap.fcc = sgbase + FCC;
	    sgmap.vdac = sgbase + VDAC;
	    sgmap.cur = sgbase + CUR;
	    sgmap.vrback = sgbase + VRBACK;
	    sgmap.fiforam = sgbase + FIFORAM;
	    sgvdac = (struct vdac *) sgmap.vdac;
	    sg_vdac_reg = 0x47;
	    sgvdac-> mode = sg_vdac_reg;
	    cursor.x = 0;
	    cursor.y = 0;
	    sg_init_shared();		/* init shared memory */
	}
	sgfcc = (struct fcc *) sgmap.fcc;
/*
 * Initialize the FCC by issuing HALT command (bits 9, 10 cbcsr to zero)
 */

	*(u_long *) &sgfcc->cbcsr = (u_long) HALT;

/* Enable FIFO compression interrupt */

/* set fcc to display list mode */

	sgaddr->nb_int_msk |= SINT_VS;
	sgfcc->cbcsr &= ~FLUSH;
	*(u_long *)&sgfcc->put = (u_long) 0;
	sgfcc->thresh = 0x100;
	*(unsigned long *) &sgfcc->cbcsr |= (unsigned long)(((sgfcc->icsr|ENTHRSH) << 16)|DL_ENB);
	DELAY(20000);  /* wait  */

	if (cvec && cvec != 0x200)
	    cvec -= 4;
	return(8);
}


/******************************************************************
 **                                                              **
 ** Routine to attach to the graphic device.                     **
 **                                                              **
 ******************************************************************/

sgattach(ui)
	struct uba_device *ui;
{
	register int *pte;
	int	i;


        sg_keyboard.hold = 0; /* "Hold Screen" key is pressed if 1 */

/*
 * init "sgflags"
 */

	sgflags.inuse = 0;		/* init inuse variable EARLY! */
	sgflags.mapped = 0;
	sgflags.kernel_loop = -1;	/* default is now kern_loop on */
	sgflags.user_fifo = 0;
	sgflags.curs_acc = ACC_OFF;
	sgflags.curs_thr = 128;
	sgflags.tab_res = 2;		/* default tablet resolution factor */
	sgflags.duart_imask = 0;	/* init shadow variables */
	sgflags.adder_ie = 0;

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
				  ((int)eq_header + sizeof(struct sginput));

	eq_header->header.size = MAXEVENTS;
	eq_header->header.head = 0;
	eq_header->header.tail = 0;

/* init single process access lock switch */

	sg_open = 0;


/*
 * Do the following only for the color display.
 */
	if (v_consputc == sgputc) {
/*
 * Map the bitmap for use by users.
 */

	    pte = (int *)(SGMEMmap[0]);
	    for( i=0 ; i<192 ; i++, pte++ )
		*pte = (*pte & ~PG_PROT) | PG_UW | PG_V;

	}
}





/******************************************************************
 **                                                              **
 ** Routine to open the graphic device.                          **
 **                                                              **
 ******************************************************************/

extern struct pdma sspdma[];
extern	int ssparam();

/*ARGSUSED*/
sgopen(dev, flag)
	dev_t dev;
{
	register int unit = minor(dev);
	register struct tty *tp;
	register struct nb_regs *sgiaddr = (struct nb_regs *)nexus;
	register struct vdac *sgvdac;
	register struct fcc *sgfcc;
	register struct adder *sgaddr = (struct adder *) sgmap.adder;

	sgvdac = (struct vdac *)sgmap.vdac;
	sgfcc = (struct fcc *) sgmap.fcc;

/*
 * The graphics device can be open only by one person 
 */
	if (unit == 1) {
	    if (sg_open != 0)
		return(EBUSY);
	    else
		sg_open = 1;
            sgflags.inuse |= GRAPHIC_DEV;  /* graphics dev is open */
	    sgflags.adder_ie |= VSYNC;
	    *(unsigned long *)&sgaddr->interrupt_enable = 
   		(unsigned long)((sgaddr->status << 16) | sgflags.adder_ie);
	} else {
            sgflags.inuse |= CONS_DEV;  /* mark console as open */
	}
	if ((unit == 2) && (major(dev) == CONSOLEMAJOR))
	    tp = &sm_tty;
	else
	    tp = &ss_tty[unit];

	if (tp->t_state&TS_XCLUDE && u.u_uid!=0)
	    return (EBUSY);
	tp->t_addr = (caddr_t)&sspdma[unit];
	tp->t_oproc = sgstart;

	/*---------------------------------------------------------------------
	* Look at the compatibility mode to specify correct default parameters
	* and to insure only standard specified functionality. */
	if ((u.u_procp->p_progenv == A_SYSV) || 
		(u.u_procp->p_progenv == A_POSIX)) {
		flag |= O_TERMIO;
		tp->t_line = TERMIODISC;
	}
	/*--------------------------------------------------------------------
	 * Set state bit to tell tty.c not to assign this line as the 
	 * controlling terminal for the process which opens this line. */
	if ((flag & O_NOCTTY) && (u.u_procp->p_progenv == A_POSIX))
		tp->t_state |= TS_ONOCTTY;
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
		ssparam(unit);
	    else
                    tp->t_iflag |= IXOFF;	/* flow control for qconsole */
	}
	sgiaddr->nb_int_msk |= SINT_VS;		/* enable interrupts */
/*
 * Process line discipline specific open if its not the mouse.
 */
	if (unit != 1)
	    return ((*linesw[tp->t_line].l_open)(dev, tp));
	else {
	    sg_mouseon = 1;
	    return(0);
	}
}


/******************************************************************
 **                                                              **
 ** Routine to close the graphic device.                         **
 **                                                              **
 ******************************************************************/

/*ARGSUSED*/
sgclose(dev, flag)
	dev_t dev;
	int flag;
{
	register struct tty *tp;
	register int unit = minor(dev);

	unit = minor(dev);
	if ((unit == 2) && (major(dev) == CONSOLEMAJOR))
	    tp = &sm_tty;
	else
	    tp = &ss_tty[unit];

/*
 * If unit is not the mouse call the line disc. otherwise clear the state
 * flag, and put the keyboard into down/up.
 */
	if( unit != 1 ){
	    (*linesw[tp->t_line].l_close)(tp);
	    ttyclose(tp);
	    sgflags.inuse &= ~CONS_DEV;
	    sg_keyboard.cntrl = sg_keyboard.shift = 0;
	} else {
	    sg_mouseon = 0;
	    if (sg_open != 1)
		return(EBUSY);
	    else
		sg_open = 0;	 /* mark the graphics device available */
	    sgflags.inuse &= ~GRAPHIC_DEV;
	    sg_init_shared();		/* init shared memory */
	    sg_load_colormaps();	/* Load B/F and cursor color maps */
	    sg_load_cursor( def_cur);	/* load default cursor map */
	    sg_ld_font();		/* load the console font */
	    cursor.x = 0;
	    cursor.y = 0;
	    if (sg_video_off)
		sg_video_on();
	}
	tp->t_state = 0;
	/* Remove termio flags that do not map */
	tp->t_iflag &= ~TERMIO_ONLY_IFLAG;
	tp->t_oflag &= ~TERMIO_ONLY_OFLAG;
	tp->t_cflag &= ~TERMIO_ONLY_CFLAG;
	tp->t_lflag &= ~TERMIO_ONLY_LFLAG;
}



/******************************************************************
 **                                                              **
 ** Routine to read from the graphic device.                     **
 **                                                              **
 ******************************************************************/

extern sg_strategy();

sgread(dev, uio)
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
             sgflags.inuse & CONS_DEV) {
	    if ((minor_dev == 2) && (major(dev) == CONSOLEMAJOR))
	    	tp = &sm_tty;
	    else
            	tp = &ss_tty[minor_dev];
            return ((*linesw[tp->t_line].l_read)(tp, uio));
        }

/*
 * else this must be a FIFO xfer from user space
 */

        else if (sgflags.inuse & GRAPHIC_DEV) {
           return (physio(sg_strategy, &sgbuf[unit],
                           dev, B_READ, minphys, uio));
        }

}


/******************************************************************
 **                                                              **
 ** Routine to write to the graphic device.                      **
 **                                                              **
 ******************************************************************/

extern sg_strategy();

sgwrite(dev, uio)
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
             sgflags.inuse & CONS_DEV) {
	    if ((minor_dev == 2) && (major(dev) == CONSOLEMAJOR))
	    	tp = &sm_tty;
	    else
            	tp = &ss_tty[minor_dev];
            return ((*linesw[tp->t_line].l_write)(tp, uio));
        }

/*
 * else this must be a FIFO xfer from user space
 */

        else if (sgflags.inuse & GRAPHIC_DEV) {
           return (physio(sg_strategy, &sgbuf[unit],
                           dev, B_WRITE, minphys, uio));
        }
}





/******************************************************************
 **                                                              **
 ** Strategy routine to do FIFO                                  **
 **                                                              **
 ******************************************************************/

sg_strategy(bp)
register struct buf *bp;
{

	register int npf, o;
	register struct pte *pte;
	register struct pte *mpte;
	register struct proc *rp;
	register struct fcc *sgfcc;
	register struct adder *sgaddr;
	register u_short *bufp;
	int	s;
	int	unit, i;
	unsigned v;

	unit = (minor(bp->b_dev) >> 2) & 0x07;

	s = spl5();

/*
 * following code figures out the proper ptes to
 * remap into system space so interrupt routine can
 * copy into buf structure.
 */ 
	v = btop(bp->b_un.b_addr);
	o = (int)bp->b_un.b_addr & PGOFSET;
	npf = btoc(bp->b_bcount + o);
	rp = bp->b_flags&B_DIRTY ? &proc[2] : bp->b_proc;
	if ((bp->b_flags & B_PHYS) == 0)
	{
		sg_fifo_addr = bp->b_un.b_addr;			
	}
	else {
		int user_addr = 0;

		if (bp->b_flags & B_UAREA)
			pte = &rp->p_addr[v];
		else if (bp->b_flags & B_PAGET)
			pte = &Usrptmap[btokmx((struct pte *)bp->b_un.b_addr)];
		else if ((bp->b_flags & B_SMEM)  &&	/* SHMEM */
					((bp->b_flags & B_DIRTY) == 0))
			pte = ((struct smem *)rp)->sm_ptaddr + v;
		else {
			pte = (struct pte *)0;
			user_addr++;
		}


		sg_fifo_addr = (char *)((int)SG_bufmap + (int)o); 
		mpte = (struct pte *)sgbufmap; 

		for (i = 0; i< npf; i++, v++) {
			if (user_addr &&
			    (((int)pte & PGOFSET) < CLSIZE*sizeof(struct pte)
			     || pte->pg_pfnum == 0))
				pte = vtopte(rp, v);
			if(pte->pg_pfnum == 0)
				panic("sg: zero pfn in pte");
			*(int *)mpte++ = pte++->pg_pfnum | PG_V | PG_KW;
			mtpr(TBIS, (char *) SG_bufmap + (i*NBPG)); 
		}
		*(int *)mpte = 0;
		mtpr(TBIS, (char *)SG_bufmap + (i * NBPG));
	}

	sgfcc = (struct fcc *) sgmap.fcc;
	sgaddr = (struct adder *) sgmap.adder;
	sg_ptr = sgmap.fiforam;
	sgflags.user_fifo = -1;
	if (!(bp->b_flags & B_READ)) {
	    nbytes_req = bp->b_bcount;
	    bcopy(sg_fifo_addr, sg_ptr, 0x020);
	    sgfcc->put += 0x010;
	    sg_fifo_addr += 0x020;
	    sg_ptr += 0x020;
	    nbytes_left = nbytes_req - 0x020;
	}
	sgfcc->icsr |= ENTHRSH;
	while (sgflags.user_fifo) {
            sleep((caddr_t)&sgflags.user_fifo, SGPRIOR);
        }

        splx(s);
	iodone(bp);
}






/******************************************************************
 **                                                              **
 ** Mouse activity select routine.                               **
 **                                                              **
 ******************************************************************/

sgselect(dev, rw)
dev_t dev;
{

	register int s = spl5();
	register int unit = minor(dev);
        register struct tty *tp;

	    switch(rw) {
	    case FREAD:					/* event available */

		if (unit == 1) {
			if(!(ISEMPTY(eq_header))) {
			    splx(s);
			    return(1);
			}
			rsel = u.u_procp;
			sgflags.selmask |= SEL_READ;
			splx(s);
			return(0);
		}
		else {
                    tp = &sm_tty;
                    if (ttnread(tp))
                        return(1);
                    tp->t_rsel = u.u_procp;
                    splx(s);
                    return(0);
                }

	    case FWRITE:		/* FIFO done? */

		if (unit == 1) {
                	if (DMA_ISEMPTY(FIFOheader)) {
                    	    splx(s);
                    	    return(1);          /* return "1" if FIFO is done */
			}
			rsel = u.u_procp;
			sgflags.selmask |= SEL_WRITE;
			splx(s);
			return(0);
		}
		else {
                    tp = &sm_tty;
                    if (tp->t_outq.c_cc <= TTLOWAT(tp))
                        return(1);
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
sgioctl(dev, cmd, data, flag)
	dev_t dev;
	int cmd;
	register caddr_t data;
{
	register int *ptep;		/* page table entry pointer */
	register struct _vs_event *vep;
	register struct tty *tp;
	register struct color_cursor *pcc;
	register struct adder *sgaddr;
	register struct vdac *sgvdac;
        register short  *eight_planes;
	register struct nb_regs *nbaddr = (struct nb_regs *)nexus;

	struct sgmap *sg;		/* pointer to device map struct */
	struct adder *adder;		/* ADDER reg structure pointer */

	struct prgkbd *cmdbuf;
	struct prg_cursor *curs;
	struct _vs_cursor *pos;

	struct devget *devget;
	register int unit = minor(dev);

	int error;
	int s;

	int i;				/* SIGNED index */
	int sbr;			/* SBR variable (you silly boy) */
	u_int ix;

	short status;
	short *shortp;			/* generic pointer to a short */
	char *chrp;			/* generic character pointer */

	short *temp;			/* a pointer to template RAM */

/*
 * service the VAXstar color device ioctl commands
 */

	switch (cmd) {


	    case QD_MAPDEVICE:
		sg = (struct sgmap *) &sgmap;
		bcopy(sg, data, sizeof(struct sgmap));
		    break;

	    case QD_MAPCOLOR:

		sgflags.mapped |= MAPCOLOR;
		ptep = (int *) ((VTOP(color_buf) * 4)
				+ (mfpr(SBR) | 0x80000000));

		/* allow user write to color map write buffer */

		*ptep++ = (*ptep & ~PG_PROT) | PG_UW | PG_V;
		*ptep++ = (*ptep & ~PG_PROT) | PG_UW | PG_V;
		*ptep = (*ptep & ~PG_PROT) | PG_UW | PG_V;

		mtpr(TBIA, 0);			/* clr CPU translation buf */

		*(int *)data = (int) color_buf;
		break;


	    /*--------------------------------------------------------------
	    * unmap shared color map write buffer and kill VSYNC intrpts */

	    case QD_UNMAPCOLOR:

		if (sgflags.mapped & MAPCOLOR) {

		    sgflags.mapped &= ~MAPCOLOR;

		    ptep = (int *) ((VTOP(color_buf) * 4)
				    + (mfpr(SBR) | 0x80000000));

		    /* re-protect color map write buffer */

		    *ptep++ = (*ptep & ~PG_PROT) | PG_KW | PG_V;
		    *ptep++ = (*ptep & ~PG_PROT) | PG_KW | PG_V;
		    *ptep = (*ptep & ~PG_PROT) | PG_KW | PG_V;

		    mtpr(TBIA, 0);	/* smash CPU's translation buf */
		}
		break;


	    case QD_MAPSCROLL:

		sgflags.mapped |= MAPSCR;
		ptep = (int *) ((VTOP(scroll) * 4)
				+ (mfpr(SBR) | 0x80000000));

		/* allow user write to scroll area */

		*ptep = (*ptep & ~PG_PROT) | PG_UW | PG_V;

		mtpr(TBIA, 0);			/* clr CPU translation buf */

		scroll->status = 0;

		sgaddr = (struct adder *) sgmap.adder;

		sgflags.adder_ie |= FRAME_SYNC;
		*(unsigned long *)&sgaddr->interrupt_enable = 
   			(unsigned long)((sgaddr->status << 16) | sgflags.adder_ie);

		*(int *)data = (int) scroll;
		break;


	    /*-------------------------------------------------------------
	    * unmap shared scroll param area and disable scroll intrpts */

	    case QD_UNMAPSCROLL:

		if (sgflags.mapped & MAPSCR) {

		    sgflags.mapped &= ~MAPSCR;

		    ptep = (int *) ((VTOP(scroll) * 4)
				    + (mfpr(SBR) | 0x80000000));

		    /* re-protect 512 scroll param area */

		    *ptep = (*ptep & ~PG_PROT) | PG_KW | PG_V;

		    mtpr(TBIA, 0);	/* smash CPU's translation buf */

		    sgaddr = (struct adder *) sgmap.adder;
		    sgflags.adder_ie &= ~FRAME_SYNC;
		    *(unsigned long *)&sgaddr->interrupt_enable = 
   			(unsigned long)((sgaddr->status << 16) | sgflags.adder_ie);
		}
		break;



           /*---------------------------------------------
            * give user write access to the event queue */

            case QD_MAPEVENT:

		sgflags.mapped |= MAPEQ;
                ptep = (int *) ((VTOP(eq_header) * 4)
                                + (mfpr(SBR) | 0x80000000));

                /* allow user write to 1K event queue */

                *ptep++ = (*ptep & ~PG_PROT) | PG_UW | PG_V;
                *ptep = (*ptep & ~PG_PROT) | PG_UW | PG_V;

                mtpr(TBIA, 0);                  /* clr CPU translation buf */

                /* return event queue address */

                *(int *)data = (int) eq_header;
                break;


           /*-------------------------------------
            * do setup for FIFO by user process  */

            case QD_MAPIOBUF:

               /*------------------------------------------------
                * set 'user write enable' bits for FIFO buffer  */

                sgflags.mapped |= MAPFIFO;

                ptep = (int *) ((VTOP(FIFOheader) * 4)
                               + (mfpr(SBR) | 0x80000000));

                for (i = (FIFObuf_size >> PGSHIFT); i > 0; --i)
                    *ptep++ = (*ptep & ~PG_PROT) | PG_UW | PG_V;

                mtpr(TBIA, 0);                  /* clr CPU translation buf */

                /*----------------------
               * return I/O buf adr */

                *(int *)data = (int) FIFOheader;
                break;


	    case QD_RDCONFIG:   /* return number of planes */
		*(short *)data = sg_num_planes;
		break;

	    case QD_GETEVENT:	/* extract the oldest event from event queue */

		if (ISEMPTY(eq_header)) {
		    vep = (struct _vs_event *) data;
		    vep->vse_device = VSE_NULL;
		    break;
		}

		vep = (struct _vs_event *) GETBEGIN(eq_header);
		s = spl5();
		GETEND(eq_header);
		splx(s);
		bcopy(vep, data, sizeof(struct _vs_event));
		break;


	    case QD_RESET:    /* init the dragon, DUART, and driver variables */

		sg_init_shared();		/* init shared memory */
		sg_setup_dragon();	/* init the ADDER/VIPER stuff */
		sg_clr_screen();
		sg_load_cursor( def_cur);	/* load default cursor map */
		sg_ld_font();			/* load the console font */
		break;


	    case QD_SET:	/* init the DUART and driver variables */

		sg_init_shared();
		break;


	    case QD_CLRSCRN:	/* clear the screen. This reinits the dragon */

		sg_setup_dragon();
		sg_clr_screen();
		break;


	    case QD_WTCURSOR:	/* load a cursor into template RAM */

		sg_load_cursor(data);
		break;

/*	    case QD_RDCURSOR:

		break;
*/


	    case QD_POSCURSOR:		/* position the mouse cursor */

		pos = (struct _vs_cursor *) data;
		pcc = (struct color_cursor *) sgmap.cur;
		s = spl5();
	        pcc->xpos = CURS_MIN_X + pos->x;
	        pcc->ypos = CURS_MIN_Y + pos->y;
		eq_header->curs_pos.x = pos->x;
		eq_header->curs_pos.y = pos->y;
		splx(s);
		break;

	    /*--------------------------------------
	    * set the cursor acceleration factor */

	    case QD_PRGCURSOR:

		curs = (struct prg_cursor *) data;
		s = spl5();
		sgflags.curs_acc = curs->acc_factor;
		sgflags.curs_thr = curs->threshold;
		splx(s);
		break;


	    /*--------------------------------------
   	     * pass caller's programming commands to LK201 */

	    case QD_PRGKBD:

		cmdbuf = (struct prgkbd *)data;     /* pnt to kbd cmd buf */

		sg_key_out (cmdbuf->cmd);
		
/*
 * Send param1?
 */
		if (cmdbuf->cmd & LAST_PARAM)
		    break;
		sg_key_out (cmdbuf->param1);

/*
 * Send param2?
 */
		if (cmdbuf->param1 & LAST_PARAM)
		    break;
		sg_key_out (cmdbuf->param2);
		break;


	    /*--------------------------------------
   	     * pass caller's programming commands to mouse */

	    case QD_PRGMOUSE:

		break;



	    case QD_KERN_LOOP:		/* redirect kernel messages */
		sgflags.kernel_loop = -1;
		break;

	    case QD_KERN_UNLOOP:	/* don't redirect kernel messages */

		sgflags.kernel_loop = 0;
		break;


	    case QD_PRGTABRES:      /* program the tablet resolution factor*/

		sgflags.tab_res = *(short *)data;
		break;

	    case QD_VIDEOON:		/* turn on the video */
		sg_video_on();
		break;

	    case QD_VIDEOOFF:		/* turn off the video */
		if (sg_num_planes == 8) {
		    eight_planes = (short *)sgmap.vdac;
	    	    *eight_planes++ = 0;		  /* select color map 0 */
		    sg_red_save = *eight_planes & 0xFF;	  /* save value for red */
		    sg_green_save = *eight_planes & 0xFF; /* save value for green */
		    sg_blue_save = *eight_planes & 0xFF;  /* save value for blue */
		    eight_planes = (short *)sgmap.vdac;
	    	    *eight_planes = 6;			/* select control reg. */
	    	    eight_planes += 2;
	    	    *eight_planes = 0x40;		/* disable cursor video */
		    eight_planes = (short *)sgmap.vdac;
	    	    *eight_planes++ = 0;		/* select color map 0 */
		    *eight_planes = 0; 			/* set value for red */
		    *eight_planes = 0; 			/* set value for green */
		    *eight_planes = 0; 			/* set value for blue */
		    eight_planes = (short *)sgmap.vdac;
	    	    *eight_planes = 4;			/* select read mask reg. */
	    	    eight_planes += 2;
		    *eight_planes = 0; 			/* set it to zero */
		}
		else {
		    sgvdac = (struct vdac *)sgmap.vdac;
		    sg_vdac_reg &= ~0x0002;
		    sgvdac->mode = sg_vdac_reg;
		}
		sg_video_off = 1;
		break;

	    case QD_CURSORON:		/* turn on the cursor */
		sgvdac = (struct vdac *)sgmap.vdac;
		sg_vdac_reg |= 0x0001;
		sgvdac->mode = sg_vdac_reg;
		break;

	    case QD_CURSOROFF:		/* turn off the cursor */
		sgvdac = (struct vdac *)sgmap.vdac;
		sg_vdac_reg &= 0xfffe;
		sgvdac->mode = sg_vdac_reg;
		break;

	    case DEVIOCGET:			    /* device status */
		    devget = (struct devget *)data;
		    bzero(devget,sizeof(struct devget));
		    devget->category = DEV_TERMINAL;
		    devget->bus = DEV_NB;
		    bcopy(DEV_VS_SLU,devget->interface,
			  strlen(DEV_VS_SLU));
		    if(unit == 0)
		    	bcopy(DEV_VR290,devget->device,
			  strlen(DEV_VR290));		    /* terminal */
		    else if(sm_pointer_id == MOUSE_ID)
		    	bcopy(DEV_MOUSE,devget->device,
			  strlen(DEV_MOUSE));
		    else if(sm_pointer_id == TABLET_ID)
		    	bcopy(DEV_TABLET,devget->device,
			  strlen(DEV_TABLET));
		    else
		    	bcopy(DEV_UNKNOWN,devget->device,
			  strlen(DEV_UNKNOWN));
		    devget->adpt_num = 0;          	    /* no adapter*/
		    devget->nexus_num = 0;           	    /* fake nexus 0 */
		    devget->bus_num = 0;            	    /* No bus   */
		    devget->ctlr_num = 0;    	    	    /* cntlr number */
		    devget->slave_num = unit;		    /* which line   */
		    bcopy("sg", devget->dev_name, 3);	    /* Ultrix "sg" */
		    devget->unit_num = unit;		    /* sg line?     */
		    devget->soft_count = 0;		    /* soft er. cnt.*/
		    devget->hard_count = 0;		    /* hard er cnt. */
		    devget->stat = 0;           	    /* status	    */
		    devget->category_stat = 0;		    /* cat. stat.   */
		    break;

	    default:

		if ((unit == 2) && (major(dev) == CONSOLEMAJOR))
		    tp = &sm_tty;
		else
		    tp = &ss_tty[unit];
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





/******************************************************************
 **                                                              **
 ** ADDER interrupt routine.                                     **
 **                                                              **
 ******************************************************************/

sgaint(sg)
register int sg;
{
	register struct	adder	*sgaddr;
	register struct fcc	*sgfcc;
	register struct nb_regs *nbaddr = (struct nb_regs *)nexus;
	register struct	vdac	*sgvdac;
	register short  *eight_planes;
	struct color_buf *cbuf;

	short status;
	int i;
	register struct rgb *rgbp;

	spl4(); 			/* allow interval timer in */

	sgaddr = (struct adder *) sgmap.adder;
	sgfcc = (struct fcc *) sgmap.fcc;

	if (sgaddr->status & ADDRESS_COMPLETE) {
	    if (sgflags.adder_ie & ADDRESS_COMPLETE) {
	    	sgfcc->cbcsr |= FLUSH;
	    	if (sgfcc->fwused > 0)
	    	    sgfcc->get += sgfcc->fwused;
	    	sgflags.adder_ie &= ~ADDRESS_COMPLETE;
		*(unsigned long *)&sgaddr->interrupt_enable = 
   			(unsigned long)((sgaddr->status << 16) | sgflags.adder_ie);
	    	while (!(sgfcc->cbcsr & IDLE));
	    	if (sgfcc->fwused > 0)
	    	    sgfcc->get += sgfcc->fwused;
		if (sgflags.user_fifo) {
		    sgflags.user_fifo = 0;
		    wakeup((caddr_t)&sgflags.user_fifo);
		} else
			*sg_int_flag = -1;
	    	sgfcc->cbcsr &= 0xE0FF;
	    }
	}
	if (sgaddr->status & VSYNC) {

/*
 * service the vertical blank interrupt (VSYNC bit) by loading any pending
 * color map load request
 */

	    *(unsigned long *)&sgaddr->interrupt_enable = 
   		(unsigned long)(((sgaddr->status &~VSYNC) << 16) | sgflags.adder_ie);

	    cbuf = color_buf;
	    if (cbuf->status & LOAD_COLOR_MAP) {
		sgvdac = (struct vdac *) sgmap.vdac;

		for (i = cbuf->count, rgbp = cbuf->rgb; --i >= 0; rgbp++) {

		    if (sg_num_planes != 8) {
		    	status = rgbp->green << 8;
		    	status |= (rgbp->blue << 4);
		    	status |= rgbp->red;
		    	sgvdac->a_color_map[rgbp->offset] = status;
		    }
		    else {
			eight_planes = (short *) sgmap.vdac;
			*eight_planes++ = rgbp->offset;
			*eight_planes = rgbp->red;
			*eight_planes = rgbp->green;
			*eight_planes = rgbp->blue;
		    }
		}

		cbuf->status &= ~LOAD_COLOR_MAP;
	    }
	}

/*
 * service the scroll interrupt (FRAME_SYNC bit)
 */

	if (sgaddr->status & FRAME_SYNC) {
	    *(unsigned long *)&sgaddr->interrupt_enable = 
   		(unsigned long)(((sgaddr->status &~FRAME_SYNC) << 16) | sgflags.adder_ie);

	    if (scroll->status & LOAD_REGS) {

		for ( i = 1000, sgaddr->status = 0
		    ; i > 0  &&  !((status = sgaddr->status)
			 & ID_SCROLL_READY)

		    ; --i);

		if (i == 0) {
		    mprintf("\nsg: sgaint: timeout on ID_SCROLL_READY");
		    return;
		}

		sgaddr->ID_scroll_data = scroll->viper_constant;
		sgaddr->ID_scroll_command = ID_LOAD | SCROLL_CONSTANT;

		sgaddr->y_scroll_constant = scroll->y_scroll_constant;
		sgaddr->y_offset_pending = scroll->y_offset;

		if (scroll->status & LOAD_INDEX) {

		    sgaddr->x_index_pending = scroll->x_index_pending;
		    sgaddr->y_index_pending = scroll->y_index_pending;
		}

	    scroll->status = 0x00;
	    }
	}
}





/******************************************************************
 **                                                              **
 ** FCC (FIFO) interrupt routine.                                **
 **                                                              **
 ******************************************************************/

sgfint(sg)
	int sg;
{
	register struct fcc *sgfcc;
	register struct adder *sgaddr;
	register struct FIFOreq_header *header;
	register struct FIFOreq *request;
	int	unit;
	u_short	temp;
	u_short	xfer_type;
	u_short	section_num;
	int	nbytes_rem;

	spl4();
	unit = sg<<2;

	header = FIFOheader;
	sgfcc = (struct fcc *) sgmap.fcc;
	sgaddr = (struct adder *) sgmap.adder;
	nbytes_rem = sgfcc->fwused;
	section_num = sgfcc->cbcsr & 0x3;
	if (*change_section == 1) {
		temp = section_num * 0x4000;
		*(unsigned long *)&sgfcc->put =
		   	(unsigned long)(( temp << 16) | temp);
		sgfcc->icsr = 0;
		*change_section = 0;
		sgfcc->put = *sg_next_fifo;
		return;
	}
	xfer_type = sgfcc->cbcsr & 0x1f00;
	if (xfer_type == DL_ENB) {
	    if (nbytes_rem > 0) {
		if (sgfcc->fwused < sgfcc->thresh)
		    sgfcc->thresh = sgfcc->fwused;
	    	sgfcc->icsr &= ~ITHRESH;
		*sg_int_flag = 0;
		return;
	    } else {
	    	sgfcc->icsr &= ~ENTHRSH;
	    	sgfcc->icsr &= ~ITHRESH;
 	    	*sg_int_flag = -1;
	    }
	} else {
		if ((xfer_type == PTB_ENB) || (xfer_type == PTB_UNPACK_ENB)) {
		    if (nbytes_rem > 0) {
			if (sgfcc->fwused < sgfcc->thresh)
		    	    sgfcc->thresh = sgfcc->fwused;
			sgfcc->icsr &= ~ITHRESH;
			sgfcc->icsr |= ENIDLE;
			sgfcc->cbcsr |= FLUSH;
		 	*sg_int_flag = 0;
			return;
		    } else {
			*sg_int_flag = -1;
		    }
		} else if (xfer_type == BTP_ENB) {
loop:
				sgfcc->get += sgfcc->fwused;
				if (sgfcc->fwused > sgfcc->thresh) goto loop;
		    		sgflags.adder_ie |= ADDRESS_COMPLETE;
				*(unsigned long *)&sgaddr->interrupt_enable = 
   			            (unsigned long)((sgaddr->status << 16) | sgflags.adder_ie);
				*sg_int_flag = 0;
			} else {
				    mprintf("\nsg: sgfint: illegal FIFOtype\n");
				    return;
				}
	}

}





/******************************************************************
 **                                                              **
 ** Graphic device input interrupt Routine.                      **
 **                                                              **
 ******************************************************************/

sgiint(ch)
register int ch;
{
	register struct _vs_event *vep;
	register struct sginput *eqh;
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
	tp = &ss_tty[unit];

/*
 * If graphic device is turned on
 */

   if (sg_mouseon == 1) {
  
	cnt = 0;
	while (cnt++ == 0) {

/*
 * Pick up LK-201 input (if any)
 */

	    if (unit == 0) {

/* event queue full ? */

		if (ISFULL(eqh) == TRUE) {
/*		    mprintf("\nsg0: sgiint: event queue overflow");*/
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
			mprintf("\nsg0: sgiint: keyboard error, code = %x",data);
			return(0);
		}

		if (data < LK_LOWEST) 
		    	return(0);
		++wakeup_flag;		/* request a select wakeup call */

		vep = PUTBEGIN(eqh);
		PUTEND(eqh);
/*
 * Check for special case in which "Hold Screen" key is pressed. If so, treat is
 * as if ^s or ^q was typed.
 */
		if (data == HOLD) {
			vep->vse_direction = VSE_KBTRAW;
			vep->vse_type = VSE_BUTTON;
			vep->vse_device = VSE_DKB;
			vep->vse_x = eqh->curs_pos.x;
			vep->vse_y = eqh->curs_pos.y;
			vep->vse_time = TOY;
			vep->vse_key = CNTRL;    	/* send CTRL */
			vep = PUTBEGIN(eqh);
			PUTEND(eqh);
			vep->vse_direction = VSE_KBTRAW;
			vep->vse_type = VSE_BUTTON;
			vep->vse_device = VSE_DKB;
			vep->vse_x = eqh->curs_pos.x;
			vep->vse_y = eqh->curs_pos.y;
			vep->vse_time = TOY;
			if( sg_keyboard.hold  == 0) {
			    if((tp->t_state & TS_TTSTOP) == 0) {
		            	vep->vse_key = CHAR_S;  /* send character "s" */
			    	sg_key_out( LK_LED_ENABLE );
				sg_key_out(LED_4);
				sg_keyboard.hold = 1;
			    }
			    else {
		            	vep->vse_key = CHAR_Q;  /* send character "q" */
			    }
			}
			else {
		            vep->vse_key = CHAR_Q;     /* send character "q" */
			    sg_key_out( LK_LED_DISABLE );
			    sg_key_out( LED_4 );
			    sg_keyboard.hold = 0;
			}
			vep = PUTBEGIN(eqh);
			PUTEND(eqh);
			vep->vse_direction = VSE_KBTRAW;
			vep->vse_type = VSE_BUTTON;
			vep->vse_device = VSE_DKB;
			vep->vse_x = eqh->curs_pos.x;
			vep->vse_y = eqh->curs_pos.y;
			vep->vse_time = TOY;
			vep->vse_key = ALLUP;
		}
		else {
			if (sg_keyboard.cntrl == 1) {
			    switch (data) {
			    case CHAR_S:
					break;
			    case CHAR_Q:
			    		sg_key_out( LK_LED_DISABLE );
			    		sg_key_out( LED_4 );
			    		sg_keyboard.hold = 0;
					break;
			    default:
					sg_keyboard.cntrl = 0;
			    }
			}
			vep->vse_direction = VSE_KBTRAW;
			vep->vse_type = VSE_BUTTON;
			vep->vse_device = VSE_DKB;
			vep->vse_x = eqh->curs_pos.x;
			vep->vse_y = eqh->curs_pos.y;
			vep->vse_time = TOY;
			vep->vse_key = data;
			if (data == CNTRL)
			    sg_keyboard.cntrl = 1;
		}
	    }

/*
 * Pick up the mouse input (if any)
 */

	    if ((unit == 1) && (sm_pointer_id == MOUSE_ID)) {

/* event queue full ? */

		if (ISFULL(eqh) == TRUE) {
/*		    mprintf("\nsg0: sgiint: event queue overflow");*/
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
		    if (sgflags.curs_acc > ACC_OFF) {
			if (new_rep->dx >= sgflags.curs_thr)
			    new_rep->dx +=
				(new_rep->dx - sgflags.curs_thr) * sgflags.curs_acc;
			if (new_rep->dy >= sgflags.curs_thr)
			    new_rep->dy +=
				(new_rep->dy - sgflags.curs_thr) * sgflags.curs_acc;
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
		    if( tp->t_state & TS_ISOPEN ) {
			sgcursor = (struct color_cursor *) sgmap.cur;
			sgcursor->xpos = CURS_MIN_X + eqh->curs_pos.x;
			sgcursor->ypos = CURS_MIN_Y + eqh->curs_pos.y;
		    }
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
/*		    	    mprintf("\nsg0: sgiint: event queue overflow");*/
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
/*		    	mprintf("\nsg0: sgiint: event queue overflow");*/
		    	return(0);
		    }


/* update cursor position coordinates */

		    new_rep->dx /= sgflags.tab_res;
		    new_rep->dy = (2200 - new_rep->dy) / sgflags.tab_res;
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

		    	if( tp->t_state & TS_ISOPEN ) {
			    sgcursor = (struct color_cursor *) sgmap.cur;
			    sgcursor->xpos = CURS_MIN_X + eqh->curs_pos.x;
			    sgcursor->ypos = CURS_MIN_Y + eqh->curs_pos.y;
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
 * The type should be "VSE_TMOTION" but, the X doesn't know this type, therefore
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
/*		    	mprintf("\nsg0: sgiint: event queue overflow");*/
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
	if(rsel && wakeup_flag && sgflags.selmask & SEL_READ) {
	    selwakeup(rsel,0);
	    rsel = 0;
	    sgflags.selmask &= ~SEL_READ;
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
			mprintf("sg0: sgiint: Keyboard error, code = %x\n",data);
			return(0);
	    }
	    if( data < LK_LOWEST ) return(0);

/*
 * See if its a state change key
 */

	    switch ( data ) {
	    case LOCK:
			sg_keyboard.lock ^= 0xffff;	/* toggle */
			if( sg_keyboard.lock )
				sg_key_out( LK_LED_ENABLE );
			else
				sg_key_out( LK_LED_DISABLE );
			sg_key_out( LED_3 );
			return;

	    case SHIFT:
			sg_keyboard.shift ^= 0xffff;
			return;	

	    case CNTRL:
			sg_keyboard.cntrl ^= 0xffff;
			return;

	    case ALLUP:
			sg_keyboard.cntrl = sg_keyboard.shift = 0;
			return;

	    case REPEAT:
			c = sg_keyboard.last;
			break;

	    case HOLD:
/*
 * "Hold Screen" key was pressed, we treat it as if ^s or ^q was typed.
 */
			if (sg_keyboard.hold == 0) {
			    if((tp->t_state & TS_TTSTOP) == 0) {
			    	c = q_key[CHAR_S];
			    	sg_key_out( LK_LED_ENABLE );
			    	sg_key_out( LED_4 );
				sg_keyboard.hold = 1;
			    } else
				c = q_key[CHAR_Q];
			}
			else {
			    c = q_key[CHAR_Q];
			    sg_key_out( LK_LED_DISABLE );
			    sg_key_out( LED_4 );
			    sg_keyboard.hold = 0;
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
			if( sg_keyboard.cntrl ) {
			    c = q_key[ data ];
			    if( c >= ' ' && c <= '~' )
				c &= 0x1f;
			    else if (c >= 0xA1 && c <= 0xFE)
			        c &= 0x9F;
			} else if( sg_keyboard.lock || sg_keyboard.shift )
				    c = q_shift_key[ data ];
				else
				    c = q_key[ data ];
			break;	

	    }

	    sg_keyboard.last = c;

/*
 * Check for special function keys
 */
	    if( c & 0x100 ) {

		register char *string;

		string = q_special[ c & 0x7f ];
		while( *string )
		    (*linesw[tp->t_line].l_rint)(*string++, tp);
	    } else {
		    /* Give the regular character to the user. */
		    if (tp->t_iflag & ISTRIP)	/* Strip to 7 bits. */
			c &= 0177;	
		    else {			/* Take the full 8-bits */
			/*
			 * If ISTRIP is not set a valid character of 377
		 	 * is read as 0377,0377 to avoid ambiguity with
		 	 * the PARMARK sequence.
		 	 */ 
			if ((c == 0377) && (tp->t_line == TERMIODISC) &&
			    (tp->t_iflag & PARMRK))
				(*linesw[tp->t_line].l_rint)(0377,tp);
		
		    }
		    (*linesw[tp->t_line].l_rint)(c, tp);
	    }
	    if (sg_keyboard.hold &&((tp->t_state & TS_TTSTOP) == 0)) {
		    sg_key_out( LK_LED_DISABLE );
		    sg_key_out( LED_4 );
		    sg_keyboard.hold = 0;
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

sgstart(tp)
	register struct tty *tp;
{
	register int unit, c;
	register struct tty *tp0;
	int s;

	unit = minor(tp->t_dev);
	tp0 = &sm_tty;
	unit &= 03;
	s = spl5();
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
	    if (unit == 0) {		/* console device */
		if (tp0->t_state & TS_ISOPEN) {
		    if (tp0->t_state & TS_TBLOCK)
			goto out;
		    c = getc(&tp->t_outq);
		    (*linesw[tp0->t_line].l_rint)(c, tp0);
		} else {
		    c = getc(&tp->t_outq);
		    sg_blitc((char)(c & 0xff));
		}
	    } else if (unit == 2) {	/* sgscreen, do flow control */
		    c = getc(&tp->t_outq);
		    if ((tp0->t_state&TS_TBLOCK) == 0) {
			tp = &ss_tty[0];
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
sgstop(tp, flag)
	register struct tty *tp;
{
	register int s;

/*
 * Block interrupts while modifying the state.
 */
	s = spl5();
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

sg_blitc( c )
register u_char c;
{

	register struct adder *sgaddr;
	register struct color_cursor *sgcursor;
	register int i;
	register u_char savechar;


/*
 * initialize ADDER
 */

	sgaddr = (struct adder *) sgmap.adder;
	sgcursor = (struct color_cursor *) sgmap.cur;

	c &= 0xff;

	switch ( c ) {
	case '\t':				/* tab		*/
		    for (i = 8 - ((cursor.x >> 3) & 0x07); i > 0; --i) {
		    	sg_blitc( ' ' );
		    }
		    return(0);

	case '\r':				/* return	*/
		    cursor.x = 0;
		    if (!(sgflags.inuse & GRAPHIC_DEV))
		    	sgcursor->xpos = CURS_MIN_X + cursor.x;
		    return(0);

	case '\b':				/* backspace	*/
		    if (cursor.x > 0) {
		    	cursor.x -= CHAR_WIDTH;
		    	sg_blitc( ' ' );
		    	cursor.x -= CHAR_WIDTH;
		    	if (!(sgflags.inuse & GRAPHIC_DEV))
			    sgcursor->xpos = CURS_MIN_X + cursor.x;
		    }
		    return(0);

	case '\n':				/* linefeed	*/
		    if ((cursor.y += CHAR_HEIGHT) > (MAX_CUR_Y - CHAR_HEIGHT)) {
		    	if (sg_mouseon == 1)
			    cursor.y = 0;
		    	else {
			    cursor.y -= CHAR_HEIGHT;
			    sg_scroll_up(sgaddr);
		    	}
		    }
		    if (!(sgflags.inuse & GRAPHIC_DEV))
		    	sgcursor->ypos = CURS_MIN_Y + cursor.y;
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

/*
 * setup VIPER operand control registers
 */

	sg_write_id(sgaddr, CS_UPDATE_MASK, 0x0001);  /* select plane #0 */
	sg_write_id(sgaddr, SRC1_OCR_B,
			EXT_NONE | INT_SOURCE | ID | BAR_SHIFT_DELAY);

	sg_write_id(sgaddr, CS_UPDATE_MASK, 0x00FE);  /* select other planes */
	sg_write_id(sgaddr, SRC1_OCR_B,
			EXT_SOURCE | INT_NONE | NO_ID | BAR_SHIFT_DELAY);

	sg_write_id(sgaddr, CS_UPDATE_MASK, 0x00FF);  /* select all planes */
	sg_write_id(sgaddr, DST_OCR_B,
			EXT_NONE | INT_NONE | NO_ID | NO_BAR_SHIFT_DELAY);

	sg_write_id(sgaddr, MASK_1, 0xFFFF);
	sg_write_id(sgaddr, VIPER_Z_LOAD | FOREGROUND_COLOR_Z, 1);
	sg_write_id(sgaddr, VIPER_Z_LOAD | BACKGROUND_COLOR_Z, 0);

	sgaddr->x_clip_min = 0;
	sgaddr->x_clip_max = 1024;
	sgaddr->y_clip_min = 0;
	sgaddr->y_clip_max = 864;
/*
 * load DESTINATION origin and vectors
 */

	sgaddr->fast_dest_dy = 0;
	sgaddr->slow_dest_dx = 0;
	sgaddr->error_1 = 0;
	sgaddr->error_2 = 0;

	sgaddr->rasterop_mode = DST_WRITE_ENABLE | NORMAL;

	sg_wait_status(sgaddr, RASTEROP_COMPLETE);

	sgaddr->destination_x = cursor.x;
	sgaddr->fast_dest_dx = CHAR_WIDTH;

	sgaddr->destination_y = cursor.y;
	sgaddr->slow_dest_dy = CHAR_HEIGHT;

/*
 * load SOURCE origin and vectors
 */

	if (c > '~') {
		savechar = c;
		c -= 34; /* These are to skip the (32) 8-bit control chars. 
			      as well as DEL and 0xA0 which aren't printable */
	}
	if ((c - ' ') > (CHARS - 1))  {
		mprintf("Invalid character (x)%x in sg_blitc\n",c);
		c = ' ';
	}
	/* X position is modulo the number of characters per line */
	sgaddr->source_1_x = FONT_X + 
		(((c - ' ') % (MAX_SCREEN_X/CHAR_WIDTH)) * CHAR_WIDTH);
	/* Point to either first or second row */
	sgaddr->source_1_y = 2048 - 15 * 
		(((c - ' ')/(MAX_SCREEN_X/CHAR_WIDTH)) + 1);

	sgaddr->source_1_dx = CHAR_WIDTH;
	sgaddr->source_1_dy = CHAR_HEIGHT;

	sg_write_id(sgaddr, LU_FUNCTION_R1, FULL_SRC_RESOLUTION | LF_SOURCE);
	sgaddr->cmd = RASTEROP | OCRB | 0 | S1E | DTE;

/*
* update console cursor coordinates */

	cursor.x += CHAR_WIDTH;
	if (!(sgflags.inuse & GRAPHIC_DEV))
	    sgcursor->xpos = CURS_MIN_X + cursor.x;

	if (cursor.x > (MAX_CUR_X - CHAR_WIDTH)) {
	    sg_blitc( '\r' );
	    sg_blitc( '\n' );
	}
}





/********************************************************************
 **                                                                **
 ** Routine to direct kernel console output to display destination **
 **                                                                **
 ********************************************************************/

sgputc( c )
register char c;
{

	register struct tty *tp0;


/*
 * This routine may be called in physical mode by the dump code
 * so we change the driver into physical mode.
 * One way change, can't go back to virtual mode.
 */
	if( (mfpr(MAPEN) & 1) == 0 ) {
		sg_physmode = 1;
		sg_mouseon = 0;
		sgmap.adder = (char *) ((u_long)VS_PHYSADDER);
		sgmap.cur = (char *) ((u_long)VS_PHYSCURSOR);
		sgmap.vdac = (char *) ((u_long)VS_PHYSVDAC);
	    	if (sg_video_off)
		    sg_video_on();
		sg_blitc(c & 0xff);
		return;
	}

/*
 * direct kernel output char to the proper place
 */

	tp0 = &sm_tty;

	if (sgflags.kernel_loop != 0  &&  tp0->t_state & TS_ISOPEN) {
	    (*linesw[tp0->t_line].l_rint)(c, tp0);
	} else {
	    sg_blitc(c & 0xff);
	}

}






/******************************************************************
 **                                                              **
 ** Routine to get a character from LK201.                       **
 **                                                              **
 ******************************************************************/

sggetc()
{
	int	c;
	u_short	data;

/*
 * Get a character from the keyboard,
 */

loop:
	data = ssgetc();

/*
 * Check for various keyboard errors
 */

	if( data == LK_POWER_ERROR || data == LK_KDOWN_ERROR ||
            data == LK_INPUT_ERROR || data == LK_OUTPUT_ERROR) {
		mprintf(" sg0: Keyboard error, code = %x\n",data);
		return(0);
	}
	if( data < LK_LOWEST ) return(0);

/*
 * See if its a state change key
 */

	switch ( data ) {
	case LOCK:
		sg_keyboard.lock ^= 0xffff;	/* toggle */
		if( sg_keyboard.lock )
			sg_key_out( LK_LED_ENABLE );
		else
			sg_key_out( LK_LED_DISABLE );
		sg_key_out( LED_3 );
		goto loop;

	case SHIFT:
		sg_keyboard.shift ^= 0xffff;
		goto loop;

	case CNTRL:
		sg_keyboard.cntrl ^= 0xffff;
		goto loop;

	case ALLUP:
		sg_keyboard.cntrl = sg_keyboard.shift = 0;
		goto loop;

	case REPEAT:
		c = sg_keyboard.last;
		break;

	default:

/*
 * Test for control characters. If set, see if the character
 * is elligible to become a control character.
 */
		if( sg_keyboard.cntrl ) {
		    c = q_key[ data ];
		    if( c >= ' ' && c <= '~' )
			c &= 0x1f;
		} else if( sg_keyboard.lock || sg_keyboard.shift )
			    c = q_shift_key[ data ];
		       else
			    c = q_key[ data ];
		break;	

	}

	sg_keyboard.last = c;

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

sgcons_init()
{
	register struct nb_regs *ssaddr = (struct nb_regs *)nexus;
	register struct nb3_regs *sgaddr = (struct nb3_regs *)sgmem;
	register struct nb1_regs *sgaddr1 = (struct nb1_regs *)qmem;
	register struct nb4_regs *sg_scratch_ram;
	register struct vdac *sgvdac;
	register struct color_cursor *sgcursor;


/*
 * Set the line parameters on SLU line 0 for
 * the LK201 keyboard: 4800 BPS, 8-bit char, 1 stop bit, no parity.
 */
	ssaddr->sslpr = (SER_RXENAB | SER_KBD | SER_SPEED | SER_CHARW);

/*
 * Set the hardware register for save routine. The save routine will be
 * called as the console program enters/exits its console display
 * (halting the cpu).
 *
 * NOTE: SYSTEM SCRATCH RAM IS MAPPED IN SS.C
 */
	sg_scratch_ram = (struct nb4_regs *)sgsys;
	sg_scratch_ram->save_console = svtophy((int *)sg_save)+2;

/*
 * Load sgmap structure with the virtual addresses of the VAXstar (color).
 */

	sgbase = (caddr_t) ((u_long)sgaddr);

	sgmap.adder = sgbase + ADDER;
	sgmap.fcc = sgbase + FCC;
	sgmap.vdac = sgbase + VDAC;
	sgmap.cur = sgbase + CUR;
	sgmap.vrback = sgbase + VRBACK;
	sgmap.fiforam = sgbase + FIFORAM;
	sg_get_planes();		/* Figure out number of planes */
	sgvdac = (struct vdac *) sgmap.vdac;
	sg_vdac_reg = 0x47;
	sgvdac-> mode = sg_vdac_reg;
	sgcursor = (struct color_cursor *)sgmap.cur;
	sg_cur_reg = (HSHI | VBHI | ENRG1 | ENPA | ENPB);
	sgcursor->cmdr = sg_cur_reg;
	sgcursor->xmin1 = CURS_MIN_X;
	sgcursor->xmax1 = MAX_CUR_X + CURS_MIN_X;
	sgcursor->ymin1 = CURS_MIN_Y;
	sgcursor->ymax1 = MAX_CUR_Y + CURS_MIN_Y;

/*
 * Initialize the VAXstar (color)
 */

	cursor.x = 0;
	cursor.y = 0;
	sg_init_shared();		/* init shared memory */
	sg_setup_dragon();		/* init ADDR/VIPER */
	sg_clr_screen();		/* clear the screen */
	sg_ld_font();			/* load the console font */
	sg_load_cursor(def_cur);	/* load default cursor */
        sg_input();			/* init the input devices */
	v_consputc = sgputc;
	v_consgetc = sggetc;
	ws_display_type = SG_DTYPE;	/* Identify SG as graphics device */
	ws_display_units = 1;		/* unit 0 only */
	vs_gdopen = sgopen;
	vs_gdclose = sgclose;
	vs_gdread = sgread;
	vs_gdwrite = sgwrite;
	vs_gdselect = sgselect;
	vs_gdkint = sgiint;
	vs_gdioctl = sgioctl;
	vs_gdstop = sgstop;
}






sgreset()
{
}






/******************************************************************
 **                                                              **
 ** Routine to setup the input devices                           **
 **  (keyboard, mouse, and tablet).                              **
 **                                                              **
 ******************************************************************/

sg_input()
{

	register struct nb_regs *ssaddr = (struct nb_regs *)nexus;
	register int	lpr;
	int	i;
	int	status;
	char	id_byte;

/*
 * Set the line parameters on SLU line 0 for
 * the LK201 keyboard: 4800 BPS, 8-bit char, 1 stop bit, no parity.
 */
	ssaddr->sslpr = (SER_RXENAB | SER_KBD | SER_SPEED | SER_CHARW);
/*
 * Reset the keyboard to the default state
 */

	sg_key_out(LK_DEFAULTS);

/*
 * Set SLU line 1 parameters for mouse communication.
 */
	lpr = SER_POINTER | SER_CHARW | SER_PARENB | SER_ODDPAR
		| SER_SPEED | SER_RXENAB;
	ssaddr->sslpr = lpr;

/*
 * Perform a self-test
 */
	sg_putc(SELF_TEST);
/*
 * Wait for the first byte of the self-test report
 *
 */
	status = sg_getc();
	if (status < 0) {
	    mprintf("\nsg: Timeout on 1st byte of self-test report\n");
	    goto OUT;
	}
/*
 * Wait for the hardware ID (the second byte returned by the self-test report)
 *
 */
	id_byte = sg_getc();
	if (id_byte < 0) {
	    mprintf("\nsg: Timeout on 2nd byte of self-test report\n");
	    goto OUT;
	}
/*
 * Wait for the third byte returned by the self-test report)
 *
 */
	status = sg_getc();
	if (status != 0) {
	    mprintf("\nsg: Timeout on 3rd byte of self-test report\n");
	    goto OUT;
	}
/*
 * Wait for the forth byte returned by the self-test report)
 *
 */
	status = sg_getc();
	if (status != 0) {
	    mprintf("\nsg: Timeout on 4th byte of self-test report\n");
	    goto OUT;
	}

/*
 * Wait to be sure that the self-test is done (documentation indicates that
 * it requires 1 second to do the self-test).
 */

	DELAY(1000000);

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
	sg_putc(INCREMENTAL);

OUT:
	return(0);
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

sg_getc()
{
	register struct nb_regs *ssaddr = (struct nb_regs *)nexus;
	register int timo;
	register int c;


	for(timo=50; timo > 0; --timo) {
		DELAY(50000);
		if(ssaddr->sscsr&SS_RDONE) {
			c = ssaddr->ssrbuf;
			if(((c >> 8) & 03) != 1)
				continue;
			if(c&(SS_DO|SS_FE|SS_PE))
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

sg_putc(c)
	register int c;
{
	register struct nb_regs *ssaddr = (struct nb_regs *)nexus;
	register int timo;

	ssaddr->sstcr |= 0x2;
	timo = 60000;
	while ((ssaddr->sscsr&SS_TRDY) == 0)
		if (--timo == 0)
			break;
	ssaddr->sstbuf = c&0xff;
	DELAY(50000);		/* ensure character transmit completed */
	ssaddr->sstcr &= ~0x2;
}




/******************************************************************
 **                                                              **
 ** Routine to load the cursor.                                  **
 **                                                              **
 ******************************************************************/

sg_load_cursor(data)
unsigned short data[32];
{

	register struct color_cursor *cur;
	register int	i;

	cur = (struct color_cursor *) sgmap.cur;
	sg_cur_reg |= LODSA;
	cur->cmdr = sg_cur_reg;

	for (i = 0; i < 32; ++i)
	    cur->cmem = data[i];

	sg_cur_reg &= ~LODSA;
	cur->cmdr = sg_cur_reg;
}




/******************************************************************
 **                                                              **
 ** Routine to output a character to the LK201 keyboard. This	 **
 ** routine polls the tramsmitter on the keyboard to output a    **
 ** code. The timer is to avaoid hanging on a bad device.        **
 **                                                              **
 ******************************************************************/

/*
 * 3/5/89 -- Fred Canter
 * Bug fix for stray interrupts thru SCB vector 0x1fc.
 * Turn interrupts off while polling for xmit ready when
 * sending characters to the keyboard.
 */

sg_key_out( c )
char c;
{
	register struct nb_regs *ssaddr;
	register int timo = 30000;
	int s;
	int tcr, ln;
	int int_msk;

	if (v_consputc != sgputc)
		return;

	if(sg_physmode)
		ssaddr = (struct nb_regs *)VS_PHYSNEXUS;
	else
		ssaddr = (struct nb_regs *)nexus;

	if(sg_physmode == 0) {
		s = spl5();
		int_msk = ssaddr->nb_int_msk;
		ssaddr->nb_int_msk &= ~SINT_ST;
	}
	tcr = 0;
	ssaddr->sstcr |= 1;
	while (1) {
		while ((ssaddr->sscsr&SS_TRDY) == 0 && timo--) ;
		ln = (ssaddr->sscsr>>8) & 3;
		if (ln != 0) {
			tcr |= (1 << ln);
			ssaddr->sstcr &= ~(1 << ln);
			continue;
		}
		ssaddr->sstbuf = c&0xff;
		while (1) {
			while ((ssaddr->sscsr&SS_TRDY) == 0) ;
			ln = (ssaddr->sscsr>>8) & 3;
			if (ln != 0) {
				tcr |= (1 << ln);
				ssaddr->sstcr &= ~(1 << ln);
				continue;
			}
			break;
		}
		ssaddr->sstcr &= ~0x1;
		if (tcr == 0)
			ssaddr->nb_int_reqclr = SINT_ST;
		else
			ssaddr->sstcr |= tcr;
		break;
	}
	if(sg_physmode == 0) {
		if (int_msk & SINT_ST)
			ssaddr->nb_int_msk |= SINT_ST;
		splx(s);
	}
}








/******************************************************************
 **                                                              **
 ** Routine to write to the ID bus                               **
 **                                                              **
 ******************************************************************/

sg_write_id(sgaddr, adrs, data)
register struct adder *sgaddr;
register short adrs;
register short data;
{
	int i;
	short status;

	for ( i = 100000, sgaddr->status = 0
	    ; i > 0  &&  !((status = sgaddr->status) &
		 ADDRESS_COMPLETE)
	    ; --i);

	if (i == 0)
	    goto ERR;

	for ( i = 100000, sgaddr->status = 0
	    ; i > 0  &&  !((status = sgaddr->status) &
		 TX_READY)
	    ; --i);

	if (i > 0) {
	    sgaddr->id_data = data;
	    sgaddr->command = ID_LOAD | adrs;
	    return(0);
	}

ERR:
	mprintf("\nsg_write_id: timeout trying to write to VIPER");
	mprintf("\ng_write_id: adrs = %x data = %x\n",adrs, data);
	return(-1);
}





/******************************************************************
 **                                                              **
 ** Routine to delay for at least one display frame time         **
 **                                                              **
 ******************************************************************/

sg_wait_status(sgaddr, mask)
register struct adder *sgaddr;
register int mask;
{
	register short status;
	int i;

	for ( i = 50000, sgaddr->status = 0
	    ; i > 0  &&  !((status = sgaddr->status) & mask)
	    ; --i);

	if (i == 0) {
	    mprintf("\nsg_wait_status: timeout polling for 0x%x in sgaddr->status", mask);
	    return(-1);
	}

	return(0);

}





/******************************************************************
 **                                                              **
 ** Routine to initialize the shared memory                      **
 **                                                              **
 ******************************************************************/

sg_init_shared()
{

	register struct color_cursor *sgcursor;

	sgcursor = (struct color_cursor *) sgmap.cur;
/*
 * initialize the event queue pointers and header
 */

	eq_header = (struct sginput *)
			  (((int)event_shared & ~(0x01FF)) + 512);
	eq_header->curs_pos.x = 0;
	eq_header->curs_pos.y = 0;

	sgcursor->xpos = CURS_MIN_X + eq_header->curs_pos.x;
	sgcursor->ypos = CURS_MIN_Y + eq_header->curs_pos.y;

	eq_header->curs_box.left = 0;
	eq_header->curs_box.right = 0;
	eq_header->curs_box.top = 0;
	eq_header->curs_box.bottom = 0;

/*
 * assign a pointer to the FIFO I/O buffer for this QDSS. 
 */

	FIFOheader = (struct FIFOreq_header *)
			  ((int)(&FIFO_shared[0] + 512) & ~0x1FF);

	FIFOheader->FIFOreq = (struct FIFOreq *) ((int)FIFOheader
				  + sizeof(struct FIFOreq_header));

	FIFOheader->shared_size = FIFObuf_size;
	FIFOheader->used = 0;
	FIFOheader->size = 10;	/* default = 10 requests */
	FIFOheader->oldest = 0;
	FIFOheader->newest = 0;
	FIFOheader->change_section = 0;
	FIFOheader->sg_int_flag = -1;
	FIFOheader->sg_next_fifo = 0;
	change_section = &FIFOheader->change_section;
	sg_int_flag = &FIFOheader->sg_int_flag;
	sg_next_fifo = &FIFOheader->sg_next_fifo;

/*
 * assign a pointer to the scroll structure.
 */

	scroll = (struct scroll *)
			 ((int)(&scroll_shared[0] + 512) & ~0x1FF);
	scroll->status = 0;
	scroll->viper_constant = 0;
	scroll->y_scroll_constant = 0;
	scroll->y_offset = 0;
	scroll->x_index_pending = 0;
	scroll->y_index_pending = 0;

/*
 * assign a pointer to the color map write buffer
 */

	color_buf = (struct color_buf *)
			   ((int)(&color_shared[0] + 512) & ~0x1FF);
	color_buf->status = 0;
	color_buf->count = 0;

}




/******************************************************************
 **                                                              **
 ** Routine to figure out the number of planes.                  **
 **                                                              **
 ******************************************************************/

sg_get_planes()
{
	register short  *sgvrback;
	register short	value;

	sgvrback = (short *) sgmap.vrback;
	value = (*sgvrback >> 4) & 0xF;
	switch (value) {

	    case 8:
			sg_num_planes = 8;
			num_colormaps = 255;
			break;

	    case 0xF:
			sg_num_planes = 4;
			num_colormaps = 15;
			break;
	    default:
	    		mprintf("\nsg%d: sg_get_planes: Invalid number of planes");
			break;
	}
}






/******************************************************************
 **                                                              **
 ** Routine to load Background/Foreground and cursor color maps. **
 **                                                              **
 ******************************************************************/

sg_load_colormaps()
{
	register struct	vdac  *sgvdac;
	register short  *eight_planes;

	if (sg_num_planes == 8) {
		eight_planes = (short *) sgmap.vdac;
		*eight_planes++ = 0;			/* Set bg color */
		*eight_planes = 0; 			/* Value for red */
		*eight_planes = 0;			/* Value for green */
		*eight_planes = 0;			/* Value for blue */
		eight_planes = (short *) sgmap.vdac;
		*eight_planes++ = 1;			/* Set fg color */
		*eight_planes = 0xff;			/* Valur for red */
		*eight_planes = 0xff;			/* Value for green */
		*eight_planes = 0xff;			/* Value for blue */
		eight_planes = (short *) sgmap.vdac;
		*eight_planes = 3;		/* Set fg color for cursor */
		eight_planes += 3;
		*eight_planes = 0xff;			/* Value of red */
		*eight_planes = 0xff;			/* Value of green */
		*eight_planes = 0xff;			/* Value of blue */
		eight_planes = (short *) sgmap.vdac;
		*eight_planes = 1;		/* Set bg color for cursor */
		eight_planes += 3;
		*eight_planes = 0;
		*eight_planes = 0;
		*eight_planes = 0;
	}
	else {
	    	sgvdac = (struct vdac *) sgmap.vdac;
	   	sgvdac->a_color_map[0] = 0x0000;	/* black */
		sgvdac->a_color_map[1] = 0x0FFF;	/* white */

/*
 * set color map for mouse cursor
 */

		sgvdac->b_cur_colorA = 0x0000;		/* black */
		sgvdac->b_cur_colorC = 0x0FFF;		/* white */
	}
}




/******************************************************************
 **                                                              **
 ** Routine to initialize the ADDER, VIPER, bitmaps, and color   **
 ** map.                                                         **
 **                                                              **
 ******************************************************************/

sg_setup_dragon()
{

	register struct adder *sgaddr;
	register struct	vdac  *sgvdac;
	register short  *eight_planes;


	int i;			/* general purpose variables */
	int status;

	short top;		/* clipping/scrolling boundaries */
	short bottom;
	short right;
	short left;

/*
 * init for setup
 */

	sgaddr = (struct adder *) sgmap.adder;
	sgvdac = (struct vdac *) sgmap.vdac;
	if (sg_num_planes == 8) {
	    sg_vdac_reg = 0x43;
	    eight_planes = (short *) sgmap.vdac;
	    *eight_planes = 6;
	    eight_planes += 2;
	    *eight_planes = sg_vdac_reg;
	}
	else {
	    sg_vdac_reg = 0x47;
	    sgvdac-> mode = sg_vdac_reg;
	}
	sgaddr->command = CANCEL;

/*
 * set monitor timing
 */

	sgaddr->x_scan_count_0 = 0x2800;
	sgaddr->x_scan_count_1 = 0x1020;
	sgaddr->x_scan_count_2 = 0x003A;
	sgaddr->x_scan_count_3 = 0x38F0;
	sgaddr->x_scan_count_4 = 0x6128;
	sgaddr->x_scan_count_5 = 0x093A;
	sgaddr->x_scan_count_6 = 0x313C;
	sgaddr->x_scan_conf = 0x00C8;

/*
 * got a bug in secound pass ADDER! lets take care of it
 */

/* normally, just use the code in the following bug fix code, but to
 * make repeated demos look pretty, load the registers as if there was
 * no bug and then test to see if we are getting sync
 */

	sgaddr->y_scan_count_0 = 0x135F;
	sgaddr->y_scan_count_1 = 0x3363;
	sgaddr->y_scan_count_2 = 0x2366;
	sgaddr->y_scan_count_3 = 0x0388;

/* if no sync, do the bug fix code */

	if (sg_wait_status(sgaddr, FRAME_SYNC) == -1) {

/*
 * first load all Y scan registers with very short frame and
 * wait for scroll service. This guarantees at least one SYNC
 * to fix the pass 2 Adder initialization bug (synchronizes
 * XCINCH with DMSEEDH).
 */

	    sgaddr->y_scan_count_0 = 0x01;
	    sgaddr->y_scan_count_1 = 0x01;
	    sgaddr->y_scan_count_2 = 0x01;
	    sgaddr->y_scan_count_3 = 0x01;

	    sg_wait_status(sgaddr, FRAME_SYNC);
	    sg_wait_status(sgaddr, FRAME_SYNC);

/* now load the REAL sync values (in reverse order just to
 *  be safe.
 */

	    sgaddr->y_scan_count_3 = 0x0388;
	    sgaddr->y_scan_count_2 = 0x2366;
	    sgaddr->y_scan_count_1 = 0x3363;
	    sgaddr->y_scan_count_0 = 0x135F;
	}


/*
 * zero the index registers
 */

	sgaddr->x_index_pending = 0;
	sgaddr->y_index_pending = 0;
	sgaddr->x_index_new = 0;
	sgaddr->y_index_new = 0;
	sgaddr->x_index_old = 0;
	sgaddr->y_index_old = 0;

	sgaddr->pause = 0;

/*
 * set rasterop mode to normal pen down
 */

	sgaddr->rasterop_mode = DST_WRITE_ENABLE | DST_INDEX_ENABLE | NORMAL;

/*
 * set the rasterop registers to a default values
 */

	sgaddr->source_1_dx = 1;
	sgaddr->source_1_dy = 1;
	sgaddr->source_1_x = 0;
	sgaddr->source_1_y = 0;
	sgaddr->destination_x = 0;
	sgaddr->destination_y = 0;
	sgaddr->fast_dest_dx = 1;
	sgaddr->fast_dest_dy = 0;
	sgaddr->slow_dest_dx = 0;
	sgaddr->slow_dest_dy = 1;
	sgaddr->error_1 = 0;
	sgaddr->error_2 = 0;

/*
 * scale factor = unity
 */

	sgaddr->fast_scale = UNITY;
	sgaddr->slow_scale = UNITY;

/*
 * set the source 2 parameters
 */

	sgaddr->source_2_x = 0;
	sgaddr->source_2_y = 0;
	sgaddr->source_2_size = 0x0022;

/*
 * initialize plane addresses for all vipers
 */

	sg_write_id(sgaddr, CS_UPDATE_MASK, 0x0001);
	sg_write_id(sgaddr, PLANE_ADDRESS, 0x0000);

	sg_write_id(sgaddr, CS_UPDATE_MASK, 0x0002);
	sg_write_id(sgaddr, PLANE_ADDRESS, 0x0001);

	sg_write_id(sgaddr, CS_UPDATE_MASK, 0x0004);
	sg_write_id(sgaddr, PLANE_ADDRESS, 0x0002);

	sg_write_id(sgaddr, CS_UPDATE_MASK, 0x0008);
	sg_write_id(sgaddr, PLANE_ADDRESS, 0x0003);

	sg_write_id(sgaddr, CS_UPDATE_MASK, 0x0010);
	sg_write_id(sgaddr, PLANE_ADDRESS, 0x0004);

	sg_write_id(sgaddr, CS_UPDATE_MASK, 0x0020);
	sg_write_id(sgaddr, PLANE_ADDRESS, 0x0005);

	sg_write_id(sgaddr, CS_UPDATE_MASK, 0x0040);
	sg_write_id(sgaddr, PLANE_ADDRESS, 0x0006);

	sg_write_id(sgaddr, CS_UPDATE_MASK, 0x0080);
	sg_write_id(sgaddr, PLANE_ADDRESS, 0x0007);

/* initialize the external registers. */

	sg_write_id(sgaddr, CS_UPDATE_MASK, 0x00FF);
	sg_write_id(sgaddr, CS_SCROLL_MASK, 0x00FF);

/* initialize resolution mode */

	sg_write_id(sgaddr, MEMORY_BUS_WIDTH, 0x000C);     /* bus width = 16 */
	sg_write_id(sgaddr, RESOLUTION_MODE, 0x0000);      /* one bit/pixel */

/* initialize viper registers */

	sg_write_id(sgaddr, SCROLL_CONSTANT, SCROLL_ENABLE|VIPER_LEFT|VIPER_UP);
	sg_write_id(sgaddr, SCROLL_FILL, 0x0000);

/*
 * set clipping and scrolling limits to full screen
 */

	for ( i = 1000, sgaddr->status = 0
	    ; i > 0  &&  !((status = sgaddr->status) &
		 ADDRESS_COMPLETE)
	    ; --i);

	if (i == 0)
	    mprintf("\nsg%d: sg_setup_dragon: timeout on ADDRESS_COMPLETE");

	top = 0;
	bottom = 2048;
	left = 0;
	right = 1024;

	sgaddr->x_clip_min = left;
	sgaddr->x_clip_max = right;
	sgaddr->y_clip_min = top;
	sgaddr->y_clip_max = bottom;

	sgaddr->scroll_x_min = left;
	sgaddr->scroll_x_max = right;
	sgaddr->scroll_y_min = top;
	sgaddr->scroll_y_max = bottom;

	sg_wait_status(sgaddr, FRAME_SYNC);
	sg_wait_status(sgaddr, FRAME_SYNC);

	sgaddr->x_index_pending = left;
	sgaddr->y_index_pending = top;
	sgaddr->x_index_new = left;
	sgaddr->y_index_new = top;
	sgaddr->x_index_old = left;
	sgaddr->y_index_old = top;

	for ( i = 1000, sgaddr->status = 0
	    ; i > 0  &&  !((status = sgaddr->status) &
		 ADDRESS_COMPLETE)
	    ; --i);

	if (i == 0)
	    mprintf("\nsg%d: sg_setup_dragon: timeout on ADDRESS_COMPLETE");

	sg_write_id(sgaddr, LEFT_SCROLL_MASK, 0x0000);
	sg_write_id(sgaddr, RIGHT_SCROLL_MASK, 0x0000);

/*
 * set source and the mask register to all ones (ie: white)
 */

	sg_write_id(sgaddr, SOURCE, 0xFFFF);
	sg_write_id(sgaddr, MASK_1, 0xFFFF);
	sg_write_id(sgaddr, VIPER_Z_LOAD | FOREGROUND_COLOR_Z, 255);
	sg_write_id(sgaddr, VIPER_Z_LOAD | BACKGROUND_COLOR_Z, 0);

/*
 * initialize Operand Control Register banks for fill command
 */

	sg_write_id(sgaddr, SRC1_OCR_A, EXT_NONE | INT_M1_M2  | NO_ID | WAIT);
	sg_write_id(sgaddr, SRC2_OCR_A, EXT_NONE | INT_SOURCE | NO_ID | NO_WAIT);
	sg_write_id(sgaddr, DST_OCR_A, EXT_NONE | INT_NONE | NO_ID | NO_WAIT);

	sg_write_id(sgaddr, SRC1_OCR_B, EXT_NONE | INT_SOURCE | NO_ID | WAIT);
	sg_write_id(sgaddr, SRC2_OCR_B, EXT_NONE | INT_M1_M2  | NO_ID | NO_WAIT);
	sg_write_id(sgaddr, DST_OCR_B, EXT_NONE | INT_NONE | NO_ID | NO_WAIT);

/*
 * init Logic Unit Function registers, (these are just common values,
 * and may be changed as required). 
 */

	sg_write_id(sgaddr, LU_FUNCTION_R1, FULL_SRC_RESOLUTION | LF_SOURCE);
	sg_write_id(sgaddr, LU_FUNCTION_R2, FULL_SRC_RESOLUTION | LF_SOURCE | INV_M1_M2);
	sg_write_id(sgaddr, LU_FUNCTION_R3, FULL_SRC_RESOLUTION | LF_D_OR_S);
	sg_write_id(sgaddr, LU_FUNCTION_R4, FULL_SRC_RESOLUTION | LF_D_XOR_S);

/*
 * load the color map 
 */

	for ( i = 0, sgaddr->status = 0
	    ; i < 10000  &&  !((status = sgaddr->status) &
		 FRAME_SYNC)
	    ; ++i);

	if (i == 0)
	    mprintf("\nsg%d: sg_setup_dragon: timeout on FRAME_SYNC");

	sg_load_colormaps();		/* Load B/F and cursor colormaps */

	return(0);

}









/******************************************************************
 **                                                              **
 ** Routine to clear the screen                                  **
 **                                                              **
 **			     >>> NOTE <<<                        **
 **                                                              **
 ** This code requires that certain adder initialization be      **
 ** valid. To assure that this requirement is satisfied, this    **
 ** routine should be called only after calling the              **
 ** "sg_setup_dragon()" function.                                **
 ** Clear the bitmap a piece at a time. Since the fast scroll    **
 ** clear only clears the current displayed portion of the       **
 ** bitmap put a temporary value in the y limit register so we   **
 ** can access whole bitmap.                                     **
 ******************************************************************/

sg_clr_screen()
{
	register struct adder *sgaddr;

	sgaddr = (struct adder *) sgmap.adder;

	sgaddr->x_limit = 1024;
	sgaddr->y_limit = 2048 - CHAR_HEIGHT;
	sgaddr->y_offset_pending = 0;

	sg_wait_status(sgaddr, FRAME_SYNC);
	sg_wait_status(sgaddr, FRAME_SYNC);

	sgaddr->y_scroll_constant = SCROLL_ERASE;

	sg_wait_status(sgaddr, FRAME_SYNC);
	sg_wait_status(sgaddr, FRAME_SYNC);

	sgaddr->y_offset_pending = 864;

	sg_wait_status(sgaddr, FRAME_SYNC);
	sg_wait_status(sgaddr, FRAME_SYNC);

	sgaddr->y_scroll_constant = SCROLL_ERASE;

	sg_wait_status(sgaddr, FRAME_SYNC);
	sg_wait_status(sgaddr, FRAME_SYNC);

	sgaddr->y_offset_pending = 1728;

	sg_wait_status(sgaddr, FRAME_SYNC);
	sg_wait_status(sgaddr, FRAME_SYNC);

	sgaddr->y_scroll_constant = SCROLL_ERASE;

	sg_wait_status(sgaddr, FRAME_SYNC);
	sg_wait_status(sgaddr, FRAME_SYNC);

	sgaddr->y_offset_pending = 0;	/* back to normal */

	sg_wait_status(sgaddr, FRAME_SYNC);
	sg_wait_status(sgaddr, FRAME_SYNC);

	sgaddr->x_limit = MAX_SCREEN_X;
	sgaddr->y_limit = MAX_SCREEN_Y + FONT_HEIGHT;

}




/******************************************************************
 **                                                              **
 ** Routine to put the console font in the off-screen memory     **
 ** of the color VAXstar.                                        **
 **                                                              **
 ******************************************************************/

sg_ld_font()
{
	register struct adder *sgaddr;

	int i;		/* scratch variables */
	int j;
	int k;
	short packed;
	int max_chars_line;


	sgaddr = (struct adder *) sgmap.adder;

/* setup VIPER operand control registers  */

	sg_write_id(sgaddr, MASK_1, 0xFFFF);
	sg_write_id(sgaddr, VIPER_Z_LOAD | FOREGROUND_COLOR_Z, 255);
	sg_write_id(sgaddr, VIPER_Z_LOAD | BACKGROUND_COLOR_Z, 0);

	sg_write_id(sgaddr, SRC1_OCR_B,
			EXT_NONE | INT_NONE | ID | BAR_SHIFT_DELAY);
	sg_write_id(sgaddr, SRC2_OCR_B,
			EXT_NONE | INT_NONE | ID | BAR_SHIFT_DELAY);
	sg_write_id(sgaddr, DST_OCR_B,
			EXT_SOURCE | INT_NONE | NO_ID | NO_BAR_SHIFT_DELAY);

	sgaddr->rasterop_mode = DST_WRITE_ENABLE |
			 DST_INDEX_ENABLE | NORMAL;

/* load destination data  */

	sg_wait_status(sgaddr, RASTEROP_COMPLETE);

	sgaddr->destination_x = FONT_X;
	sgaddr->destination_y = FONT_Y;
	if (FONT_WIDTH > MAX_SCREEN_X)
		sgaddr->fast_dest_dx = MAX_SCREEN_X;
	else
		sgaddr->fast_dest_dx = FONT_WIDTH;
	sgaddr->slow_dest_dy = CHAR_HEIGHT;

/* setup for processor to bitmap xfer  */

	sg_write_id(sgaddr, CS_UPDATE_MASK, 0x0001);
	sgaddr->cmd = PBT | OCRB | 2 | DTE | 2;

/* Determine how many characters can be stored on one "line" of offscreen mem.*/
	    max_chars_line = MAX_SCREEN_X/(CHAR_WIDTH*2);
	    if ((CHARS/2 + CHARS%2) < max_chars_line)
		max_chars_line = CHARS/2 + CHARS%2;

/* iteratively do the processor to bitmap xfer */

	for (i = 0; i < ROWS; ++i) {

	    /* PTOB a scan line */

	    for (j = 0, k = i; j < max_chars_line; ++j) {

		/* PTOB one scan of a char cell */

		packed = q_font[k];
		k += ROWS;
		packed |= ((short)q_font[k] << 8);
		k += ROWS;

		sg_wait_status(sgaddr, TX_READY);
		sgaddr->id_data = packed;
	    }
	}
/*
* Copy the second row of characters.
* Subtract the first row from the total number.  Divide this quantity by 2 
* because 2 chars are stored in a short in the PTOB loop below.
* Figure out how many characters can be stored on one "line" of offscreen memory
*/
	    max_chars_line = MAX_SCREEN_X/(CHAR_WIDTH*2);
	    if ((CHARS/2 + CHARS%2) < max_chars_line)
		return;
	    max_chars_line = (CHARS/2 + CHARS%2) - max_chars_line; /* 95 - 64 */
	    /* Paranoia check to see if 3rd row may be needed */
	    if (max_chars_line > (MAX_SCREEN_X/(CHAR_WIDTH*2)))
	    	max_chars_line = MAX_SCREEN_X/(CHAR_WIDTH*2);


/* load destination data */

	sgaddr->destination_x = FONT_X;
	sgaddr->destination_y = FONT_Y - CHAR_HEIGHT;
	sgaddr->fast_dest_dx = max_chars_line * CHAR_WIDTH * 2;
	sgaddr->slow_dest_dy = CHAR_HEIGHT;

/* setup for processor to bitmap xfer  */

	sg_write_id(sgaddr, CS_UPDATE_MASK, 0x0001);
	sgaddr->cmd = PBT | OCRB | 2 | DTE | 2;

/* iteratively do the processor to bitmap xfer */

	for (i = 0; i < ROWS; ++i) {

	    /* PTOB a scan line */

	    for (j = 0, k = i; j < max_chars_line; ++j) {

		/* PTOB one scan of a char cell */

		packed = q_font[k + FONT_OFFSET];
		k += ROWS;
		packed |= ((short)q_font[k + FONT_OFFSET] << 8);
		k += ROWS;

		sg_wait_status(sgaddr, TX_READY);
		sgaddr->id_data = packed;
	    }
	}
}







/******************************************************************
 **                                                              **
 ** Routine to scroll up the screen one character height         **
 **                                                              **
 ******************************************************************/

sg_scroll_up(sgaddr)
	register struct adder *sgaddr;
{


/*
 * setup VIPER operand control registers
 */

	sg_wait_status(sgaddr, ADDRESS_COMPLETE);

	sg_write_id(sgaddr, CS_UPDATE_MASK, 0x00FF);  /* select all planes */
	sg_write_id(sgaddr, CS_SCROLL_MASK, 0x00FF);  /* select all planes */

	sg_write_id(sgaddr, MASK_1, 0xFFFF);
	sg_write_id(sgaddr, VIPER_Z_LOAD | FOREGROUND_COLOR_Z, 255);
	sg_write_id(sgaddr, VIPER_Z_LOAD | BACKGROUND_COLOR_Z, 0);

	sg_write_id(sgaddr, SRC1_OCR_B,
			EXT_NONE | INT_SOURCE | ID | BAR_SHIFT_DELAY);
	sg_write_id(sgaddr, DST_OCR_B,
			EXT_NONE | INT_NONE | NO_ID | NO_BAR_SHIFT_DELAY);

/*
 * load DESTINATION origin and vectors
 */

	sgaddr->fast_dest_dy = 0;
	sgaddr->slow_dest_dx = 0;
	sgaddr->error_1 = 0;
	sgaddr->error_2 = 0;

	sgaddr->rasterop_mode = DST_WRITE_ENABLE | NORMAL;

	sgaddr->destination_x = 0;
	sgaddr->fast_dest_dx = 1024;

	sgaddr->destination_y = 0;
	sgaddr->slow_dest_dy = 864 - CHAR_HEIGHT;

/*
 * load SOURCE origin and vectors
 */

	sgaddr->source_1_x = 0;
	sgaddr->source_1_dx = 1024;

	sgaddr->source_1_y = CHAR_HEIGHT;
	sgaddr->source_1_dy = 864 - CHAR_HEIGHT;

	sg_write_id(sgaddr, LU_FUNCTION_R1, FULL_SRC_RESOLUTION | LF_SOURCE);
	sgaddr->cmd = RASTEROP | OCRB | 0 | S1E | DTE;

/*
 * do a rectangle clear of last screen line
 */

	sg_write_id(sgaddr, MASK_1, 0xffff);
	sg_write_id(sgaddr, SOURCE, 0xffff);
	sg_write_id(sgaddr,DST_OCR_B,
		(EXT_NONE | INT_NONE | NO_ID | NO_BAR_SHIFT_DELAY));
	sg_write_id(sgaddr, VIPER_Z_LOAD | FOREGROUND_COLOR_Z, 0);
	sgaddr->error_1 = 0;
	sgaddr->error_2 = 0;
	sgaddr->slow_dest_dx = 0;		  /* set up the width of */
	sgaddr->slow_dest_dy = CHAR_HEIGHT;	/* rectangle */

	sgaddr->rasterop_mode = (NORMAL | DST_WRITE_ENABLE) ;
	sg_wait_status(sgaddr, RASTEROP_COMPLETE);
	sgaddr->destination_x = 0;
	sgaddr->destination_y = 864 - CHAR_HEIGHT;

	sgaddr->fast_dest_dx = 1024;	/* set up the height */
	sgaddr->fast_dest_dy = 0;	/* of rectangle */

	sg_write_id(sgaddr, LU_FUNCTION_R2, (FULL_SRC_RESOLUTION | LF_SOURCE));
	sgaddr->cmd = (RASTEROP | OCRB | LF_R2 | DTE ) ;

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

sg_save() {

/*
 * Check to see if reboot in progress or restart if so, no need for saving and
 * restoring registers.
 */
	sgsys_scr_ram =  (struct nb4_regs *) scr_ram_addr;
	if (!(((struct nb_regs *)VS_PHYSNEXUS)->nb_cpmbx & REBOOT_IN_PROG) &&
	    ((((struct nb_regs *)VS_PHYSNEXUS)->nb_cpmbx & CPMBX_REBOOT)!= CPMBX_REBOOT)) {
	    sg_addr = (struct adder *) VS_PHYSADDER;
	    sg_fcc = (struct fcc *) VS_PHYSFCC;
	    while (sg_fcc->fwused);  /* Nothing in the FIFO */
	    sgsave_regs[0] = sg_fcc->get;
	    sgsave_regs[1] = sg_fcc->put;
	    sgsave_regs[2] = sg_fcc->cbcsr;
	    sgsave_regs[3] = sg_fcc->icsr;
	    sgsave_regs[4] = *sg_next_fifo;
	    sg_counter1 = 0;
	    colormaps_index = 0;
/*
 * Save the cursor colors
 */
	    if (sg_num_planes == 8) {
	        vdac_address = (short *) VS_PHYSVDAC;
	        *vdac_address = 1;	/* Read cursor background color */
	        vdac_address += 3;
	        sg_colormaps[colormaps_index++] = *vdac_address&0xFF; /*Red*/
	        sg_colormaps[colormaps_index++] = *vdac_address&0xFF; /*Green*/
	        sg_colormaps[colormaps_index++] = *vdac_address&0xFF; /*Blue*/
	        vdac_address = (short *) VS_PHYSVDAC;
	        *vdac_address = 3;	/* Read cursor foreground color */
	        vdac_address += 3;
	        sg_colormaps[colormaps_index++] = *vdac_address&0xFF; /*Red*/
	        sg_colormaps[colormaps_index++] = *vdac_address&0xFF; /*Green*/
	        sg_colormaps[colormaps_index++] = *vdac_address&0xFF; /*Blue*/
	    }
	    else {
	        vdac_address = (short *) VS_PHYSVDAC;
		vdac_address += 52;
		*vdac_address = 0x20;  /* Cursor background color */
		for ( sg_counter2 = 10000 ; sg_counter2 > 0  &&  !(*vdac_address & 0x1000) ; --sg_counter2);
	        sg_colormaps[colormaps_index++] = *vdac_address&0xFFF;
	        vdac_address = (short *) VS_PHYSVDAC;
		vdac_address += 52;
		*vdac_address = 0x60;  /* Cursor foreground color */
		for ( sg_counter2 = 10000 ; sg_counter2 > 0  &&  !(*vdac_address & 0x1000) ; --sg_counter2);
	        sg_colormaps[colormaps_index++] = *vdac_address&0xFFF;

	    }

/*
 * Save the color maps
 */
	    if (sg_num_planes == 8) {
	    	for (sg_counter1 = 0; sg_counter1 < num_colormaps; sg_counter1++) {
	    	    vdac_address = (short *) VS_PHYSVDAC;
	    	    *vdac_address++ = sg_counter1;
	    	    sg_colormaps[colormaps_index++] = *vdac_address&0xFF; /*Read Red*/
	    	    sg_colormaps[colormaps_index++] = *vdac_address&0xFF; /*Read Green*/
	    	    sg_colormaps[colormaps_index++] = *vdac_address&0xFF; /*Read Blue*/
	    	}
	    }
	    else {
	    	for (sg_counter1 = 0; sg_counter1 < num_colormaps; sg_counter1++) {
	    	    vdac_address = (short *) VS_PHYSVDAC;
		    vdac_address += 52;
		    *vdac_address = sg_counter1;
		    for ( sg_counter2 = 10000 ; sg_counter2 > 0  &&  !(*vdac_address & 0x1000) ; --sg_counter2);
	    	    sg_colormaps[colormaps_index++] = *vdac_address&0xFFF;
	    	}
	    }
/*
 * Last thing we do is to set the hardware register for the restore routine.
 */
	    sgsys_scr_ram->restore_console = svtophy((int *)sg_restore)+2;
	}
	else {
	    sgsys_scr_ram->restore_console = 0;
	}
	asm("rsb");
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

sg_restore() {


/*
 * Set the hardware register for the restore routine to zero to prevent from
 * coming to this routine until futher notice (sg_save routine).
 */

	sgsys_scr_ram =  (struct nb4_regs *) scr_ram_addr;
	sgsys_scr_ram->restore_console = 0;
	sg_addr = (struct adder *) VS_PHYSADDER;
	sg_fcc = (struct fcc *) VS_PHYSFCC;
	sg_vdac = (struct vdac *) VS_PHYSVDAC;
/*
 * Restore some hardware registers.
 */
	sg_fcc->get = sgsave_regs[0];
	sg_fcc->put = sgsave_regs[1];
	sg_fcc->cbcsr = sgsave_regs[2];
	sg_fcc->icsr = sgsave_regs[3];
	*sg_next_fifo = sgsave_regs[4];
	*(unsigned long *)&sg_addr->interrupt_enable = 
   		(unsigned long)((sg_addr->status << 16) | sgflags.adder_ie);
	if (sg_num_planes == 8) {
	    vdac_address = (short *) VS_PHYSVDAC;
	    *vdac_address = 6;
	    vdac_address += 2;
	    *vdac_address = sg_vdac_reg;
	    colormaps_index = 0;
	    sg_counter1 = 0;
/*
 * Restore the cursor colors
 */
	    vdac_address = (short *) VS_PHYSVDAC;
	    *vdac_address = 1;	/* restore cursor background color */
	    vdac_address += 3;
	    *vdac_address = sg_colormaps[colormaps_index++]; /*Write Red*/
	    *vdac_address = sg_colormaps[colormaps_index++]; /*Write Green*/
	    *vdac_address = sg_colormaps[colormaps_index++]; /*Write Blue*/
	    vdac_address = (short *) VS_PHYSVDAC;
	    *vdac_address = 3;	/* restore cursor foreground color */
	    vdac_address += 3;
	    *vdac_address = sg_colormaps[colormaps_index++]; /*Write Red*/
	    *vdac_address = sg_colormaps[colormaps_index++]; /*Write Green*/
	    *vdac_address = sg_colormaps[colormaps_index++]; /*Write Blue*/
/*
 * Restore the color maps
 */
	    for (sg_counter1 = 0; sg_counter1 < num_colormaps; sg_counter1++) {
	    	vdac_address = (short *) VS_PHYSVDAC;
	    	*vdac_address++ = sg_counter1;
	    	*vdac_address = sg_colormaps[colormaps_index++]; /*Write Red*/
	    	*vdac_address = sg_colormaps[colormaps_index++]; /*Write Green*/
	    	*vdac_address = sg_colormaps[colormaps_index++]; /*Write Blue*/
	    }
	}
	else {
	    sg_vdac = (struct vdac *) VS_PHYSVDAC;
	    sg_vdac-> mode = sg_vdac_reg;
	    colormaps_index = 0;
	    sg_counter1 = 0;
/*
 * Restore the cursor colors
 */
	    vdac_address = (short *) VS_PHYSVDAC;
	    vdac_address += 33;
	    *vdac_address = sg_colormaps[colormaps_index++];
	    vdac_address += 2;
	    *vdac_address = sg_colormaps[colormaps_index++];
/*
 * Restore the color maps
 */
	    vdac_address = (short *) VS_PHYSVDAC;
	    for (sg_counter1 = 0; sg_counter1 < num_colormaps; sg_counter1++)
	    	*vdac_address++ = sg_colormaps[colormaps_index++];
	}
/*
 * Last thing we do is to set the hardware register for the save routine.
 */
	sgsys_scr_ram->save_console = svtophy((int *)sg_save)+2;
	asm("rsb");
}

/*
 * Routine to turn the video on
 */

sg_video_on()
{
        register struct vdac *sgvdac;
        register short  *eight_planes;

	if (sg_num_planes == 8) {
	    eight_planes = (short *)sgmap.vdac;
	    *eight_planes = 4;			/* select read mask reg. */
	    eight_planes += 2;
	    *eight_planes = 0xFF; 		/* set it */
	    eight_planes = (short *)sgmap.vdac;
	    *eight_planes = 6;			/* select control reg. */
	    eight_planes += 2;
	    *eight_planes = 0x43;		/* enable cursor video */
	    eight_planes = (short *)sgmap.vdac;
	    *eight_planes++ = 0;		/* select color map 0 */
	    *eight_planes = sg_red_save;	/* write value of red */
	    *eight_planes = sg_green_save;	/* write value of green */
	    *eight_planes = sg_blue_save;	/* write value of blue */
	}
	else {
	    sgvdac = (struct vdac *)sgmap.vdac;
	    sg_vdac_reg |= 0x0002;
	    sgvdac->mode = sg_vdac_reg;
	}
	sg_video_off = 0;
}

#endif
