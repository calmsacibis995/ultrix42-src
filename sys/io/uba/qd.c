#ifndef lint
static char *sccsid = "@(#)qd.c	4.1	(ULTRIX)	7/2/90";
#endif lint

/************************************************************************
*									*
*			Copyright (c) 1985-1988 by			*
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
*************************************************************************/

/*
 * qd.c
 *
 * Modification history
 *
 * QDSS workstation driver
 *
 * 15-Dec-89 -- Alan Frechette
 *	Changes to "ws_display_type" and "ws_display_units".
 *
 * 15-Aug-89 -- carito (Allen Carito)
 *	Changed qdwrite to check for graphic device first then console or
 *	alternate console.  This fixes the blank screens on GPX's only.
 *	The fix does have an impact to any other workstation driver.
 *
 * 23-Jul-89 -- Randall Brown
 *	In the default case of an ioctl() call, check the return value
 *	from ttioctl() to see if the cmd was invalid.  If it was, return
 *	an error.
 *
 * 23-May-89 -- darrell
 *	Removed the v_ prefix from umaddr and globally defined cpup - as
 *	part of the new cpusw.
 *
 * 08-May-89 -- rafiey (Ali Rafieymehr)
 *
 *	When going to physical mode (crash dump) the video could be turned
 *	off by the server (screen saver). Therefore we turn the video on.
 *
 * 08-May-89 -- rafiey (Ali Rafieymehr)
 *
 *	When going to single user mode, the keys would behave as if the
 *	<Ctrl> was also pressed. We now clear the flags for shift and
 *	control in qdclose routine.
 *
 * 08-May-89 -- rafiey (Ali Rafieymehr)
 *
 *	The input interrupt routine (qdiint) was printing error messages when
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
 *  4-Mar-88 - Mary Larson
 *      Took out change that left vertical sync on.  This caused process
 *      based X's to crash the machine when closing and then reopening
 *      the device.
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
 * 28-Sep-87 - Ali Rafieymehr
 *    Fixed a bug in qddint() routine. The bug was discovered on CVAX.
 *    Incorrect logic was used when taking an entry from the request queue.
 *
 * 12-Oct-87 - Tim Burke
 *	Modified driver to provide 8-bit support to the console port.  To do 
 *	this characters are defined in the remainder of the first 15 rows of
 *	off screen memory as well as in next 15 rows.
 *
 *  2-Aug-87 - Fred Canter
 *    Use TANDEM mode on qconsole for flow control of writes to
 *    /dev/console.
 *
 * 12-Jun-87 - Tim Burke
 *
 *	Added full TERMIO functionality to terminal subsystem.
 *	Changed open routine to setup propper default settings if the line
 *	is not being used as a graphics device.  Added defines of defaults.
 *
 *
 * 14-May-87 - Fred Canter
 *	Changed default state of kernel_loop to on.
 *
 * 21-Apr-87 - Brian Stevens
 *	Xos support
 *
 * 16-Apr-87 - Fred Canter (for Brian Stevens)
 *	Multi-head GPX changes.
 *
 * 19-Mar-87 - Fred Canter (for Brian Stevens)
 *	Added X in the kernel support.
 *
 * 16-Dec-86 - Brian Stevens
 *
 *	added support so "select" could be called on tty type device
 *      fixed rlogin bug
 *
 * 30-Oct-86 - Brian Stevens
 *
 *	Removed the mprintf for "unexpected interrupts"
 *
 * 26-Aug-86 - rsp (Ricky Palmer)
 *
 *	Cleaned up devioctl code to (1) zero out devget structure
 *	upon entry and (2) use strlen instead of fixed storage
 *	for bcopy's.
 *
 * 21-Jul-86 - Ram Rao
 *	allowed cursor rectangle to hang (partially) off the
 *	top and left of screen
 *
 * 11-Jul-86 - ricky palmer
 *
 *	Added adpt and nexus fields to DEVIOCGET code.
 *
 * 02-July-86 - Brian Stevens
 *
 *	added support for console writing to second QDSS display
 *
 * 20-May-86 - ricky palmer
 *
 *	Added new DEVIOCGET ioctl request code. V2.0
 *
 * 16-Apr-86 -- darrell
 *	 badaddr is now called via the macro BADADDR
 *
 * 14-Apr-86 -- afd
 *	 Changed UMEMmap to QMEMmap and umem to qmem.
 *
 *	 v_console() is now refered to as v_consputc, and there is a
 *	 corresponding v_consgetc() (defined in /sys/vax/conf.c).
 *
 *	 Added "qdgetc()" routine for console read.  Needed to read
 *	 user's answer to the "root device?" prompt with a generic kernel.
 *
 * 19-Mar-86 -- pmk
 *	 Change DELAY to 20000, because microdelay now in real microsec.
 *
 * 18-mar-86  -- jaw	 br/cvec changed to NOT use registers.
 *
 * 11 mar 86  darrell	replaced percpu with cpusw, and removed all but
 *			 one global reference
 * 19 feb 86  bstevens	no report of motion event on puck/stylus button action
 * 18 feb 86  bstevens	put in cursor box support for tablets
 * 18-Mar-86 -- jaw  add routines to cpu switch for nexus/unibus addreses
 *		      also got rid of some globals like nexnum.
 *		      ka8800 cleanup.
 * 06 dec 85  longo  added LK-201 error reporting for graphics device ops
 * 03 dec 85  longo  made qddint() clear active bit on error
 * 02 dec 85  longo  fixed up some crocks in the error messages
 * 25 nov 85  longo  added error handling to DMA ISR and single user locking
 * 19 nov 85  longo  eliminated "set_defaults()" by breaking out sub-calls.
 *		     Changed init_shared to do real init of scroll struct
 * 12 nov 85  longo  fixed bug in open that broke alternate console re-direct
 * 11 nov 85  longo  changed "_vs_eventqueue" references to "qdinput"
 * 08 nov 85  longo  improved select service for read/write select wakeup.
 *		     Also set ISR's to ipl4 to allow the interval timer in.
 * 04 nov 85  longo  fixed bugs in mouse button reporting and dma request stuff
 * 30 oct 85  longo  DMA to/from user space is in place
 * 14 oct 85  longo  added kernel msg redirect and QD_RDCURSOR ioctl
 * 03 oct 85  longo  added support for multiple QDSS's
 * 02 oct 85  longo  added color map loading services in qdioctl() & qdaint()
 * 30 sep 85  longo  added DMA interrupt services
 * 18 sep 85  longo  added scroll services to "qdaint()" adder intrpt service
 *		     and put in supporting ioctl's
 * 04 sep 85  longo  initial implementation of DMA is working
 * 17 aug 85  longo  added support for the QDSS to be system console
 * 05 aug 85  longo  now using qfont (QVSS & QDSS) as linked object
 * 12 jun 85  longo  added mouse event loading to "qdiint()"
 * 31 may 85  longo  put live interrupts into the probe() routine
 * 30 may 85  longo  event queue shared memory implementation is now alive
 * 29 may 85  longo  LK-201 input is now interrupt driven
 * 25 apr 85  longo  MAPDEVICE works
 * 14 mar 85  longo  created
 *
 *	 todo:	 fix rlogin bug in console stuff
 *		 cat -u console redirection
 *		 check error return from strategy routine
 *		 verify TOY time stuff (what format?)
 *		 look at system based macro implementation of VTOP
 *
 */

#include "../data/qd_data.c" /* include external references to data file */


#define QDSSMAJOR       42              /* QDSS major device number */
extern	int	ws_display_type;
extern	int	ws_display_units;
extern struct cpusw *cpup;		/* pointer to cpusw entry */

int     QDlast_DMAtype;                 /* type of the last DMA operation */

/*---------------------------------------------------------------------
* macro to get system time.  Used to time stamp event queue entries */

#define TOY ((time.tv_sec * 1000) + (time.tv_usec / 1000))

/*--------------------------------------------------------------------------
* the "ioconf.c" program, built and used by auto config, externally refers
* to definitions below.  */

	int qdprobe();
	int qdattach();
	int qddint();			/* DMA gate array intrpt service */
	int qdaint();			/* Dragon ADDER intrpt service */
	int qdiint();

	u_short qdstd[] = { 0 };

	struct uba_driver qddriver = {	/* externally referenced: ioconf.c */

	    qdprobe,			/* device probe entry */
	    0,				/* no slave device */
	    qdattach,			/* device attach entry */
	    0,				/* no "fill csr/ba to start" */
	    qdstd,			/* device addresses */
	    "qd",			/* device name string */
	    qdinfo			/* ptr to QDSS's uba_device struct */
	};

/*-------------------
* general defines */

#define QDPRIOR (PZERO-1)		/* must be negative */

#define FALSE	0
#define TRUE	~FALSE

#define BAD	-1
#define GOOD	0

/*-----------------------------------------------------------------------
* macro to create a system virtual page number from system virtual adrs */

#define VTOP(x)  (((int)x & ~0xC0000000) >> PGSHIFT) /* convert qmem adrs */
						     /* to system page # */

/*------------------------------------------------------------------
* QDSS register address offsets from start of QDSS address space */

#define QDSIZE	 (52 * 1024)	/* size of entire QDSS foot print */

#define TMPSIZE  (16 * 1024)	/* template RAM is 8k SHORT WORDS */
#define TMPSTART 0x8000 	/* offset of template RAM from base adrs */

#define REGSIZE  (5 * 512)	/* regs touch 2.5k (5 pages) of addr space */
#define REGSTART 0xC000 	/* offset of reg pages from base adrs */

#define ADDER	(REGSTART+0x000)
#define DGA	(REGSTART+0x200)
#define DUART	(REGSTART+0x400)
#define MEMCSR	(REGSTART+0x800)

#define CLRSIZE  (3 * 512)		/* color map size */
#define CLRSTART (REGSTART+0xA00)	/* color map start offset from base */
					/*  0x0C00 really */
#define RED	(CLRSTART+0x000)
#define BLUE	(CLRSTART+0x200)
#define GREEN	(CLRSTART+0x400)

/*---------------------------------------------------------------
* values used in mapping QDSS hardware into the Q memory space */

#define CHUNK	  (64 * 1024)
#define QMEMSIZE  (1024 * 1024 * 4)	/* 4 meg */

/*----------------------------------------------------------------------
* QDSS minor device numbers.  The *real* minor device numbers are in
* the bottom two bits of the major/minor device spec.  Bits 2 and up are
* used to specify the QDSS device number (ie: which one?) */


#define CONS		0
#define ALTCONS 	1
#define GRAPHIC 	2

/*----------------------------------------------
* console cursor bitmap (block cursor type)  */

	short cons_cursor[32] = {      /* white block cursor */

 /* A */ 0x00FF, 0x00FF, 0x00FF, 0x00FF, 0x00FF, 0x00FF, 0x00FF, 0x00FF,
	 0x00FF, 0x00FF, 0x00FF, 0x00FF, 0x00FF, 0x00FF, 0x00FF, 0x00FF,
 /* B */ 0x00FF, 0x00FF, 0x00FF, 0x00FF, 0x00FF, 0x00FF, 0x00FF, 0x00FF,
	 0x00FF, 0x00FF, 0x00FF, 0x00FF, 0x00FF, 0x00FF, 0x00FF, 0x00FF

	};

/*-------------------------------------
* constants used in font operations */


/* Originaly this was CHARS 95 */
#define CHARS		190 			/* # of chars in the font */

#define CHAR_HEIGHT	15			/* char height in pixels */
#define CHAR_WIDTH	8			/* char width in pixels*/
#define FONT_WIDTH	(CHAR_WIDTH * CHARS)	/* font width in pixels */
#define ROWS		CHAR_HEIGHT


#define FONT_X		0			/* font's off screen adrs */
#define FONT_Y		(2048 - CHAR_HEIGHT)

					/* Offset to second row characters */
#define FONT_OFFSET	((MAX_SCREEN_X/CHAR_WIDTH)*CHAR_HEIGHT)

	extern char q_font[];		/* reference font object code */

	extern	u_short q_key[];		/* reference key xlation tables */
	extern	u_short q_shift_key[];
	extern	char *q_special[];

/*--------------------------------------------------
* definitions for cursor acceleration reporting  */

#define ACC_OFF 	0x01		/* acceleration is inactive */

/*--------------------------------------------------------------------------
* v_consputc is the switch that is used to redirect the console cnputc() to
* the virtual console qdputc().
* v_consgetc is the switch that is used to redirect the console getchar() to
* the virtual console qdgetc().
*/

	extern (*v_consputc)();
	int qdputc();		/* used to direct kernel console output */
	extern (*v_consgetc)();
	int qdgetc();		/* used to read kernel console input */

	int qdstart();		/* used to direct /dev/console output */

/*------------------------------------------------------------------------
* LK-201 state storage for input console keyboard conversion to ASCII */

	struct q_keyboard {

	    int shift;			/* state variables	*/
	    int cntrl;
	    int lock;
	    int lastcode;		/* last keycode typed	*/
	    unsigned kup[8];		/* bits for each keycode*/
	    unsigned dkeys[8];		/* down/up mode keys	*/
	    char last;			/* last character	*/

	 } q_keyboard;

/*--------------------------------------------------
* ULTRIX settings for first open.		  */
 
#define IFLAGS	(EVENP|ECHO|XTABS|CRMOD)

/*------------------------------------------------------------------------
* termio flags will be set to these default values in non-termio mode to
* provide a backward compatible ULTRIX environment. */

#define IFLAG (BRKINT|IGNPAR|ISTRIP|IXON|IXANY|ICRNL)
#define OFLAG (OPOST|TAB3|ONLCR)
#define LFLAG (ISIG|ICANON|ECHO)
#define CFLAG (PARENB|CREAD|CS7|CLOCAL)

/*****************************************************************
******************************************************************
******************************************************************
*
*	DRIVER FUNCTIONS START HERE:
*
******************************************************************
******************************************************************
*****************************************************************/

/*********************************************************************
*
*	qdcons_init()... init QDSS as console (before probe routine)
*
*********************************************************************/

qdcons_init()
{
	register u_int unit;

	int *ptep;			/* page table entry pointer */
	caddr_t phys_adr;		/* physical QDSS base adrs */
	u_int mapix;			/* index into QMEMmap[] array */

	u_short *qdaddr;		/* address of QDSS IO page CSR */
	u_short *devptr;		/* vitual device space */

#define QDSSCSR 0x1F00

	unit = 0;

/*------------------------------------------------------
* Map the Q-bus memory space into the system memory. */

	ubaaccess(((*cpup->umaddr)(0)), QMEMmap[0],
			cpup->pc_umsize, PG_V | PG_KW);

	ubaaccess(((*cpup->udevaddr)(0)), QMEMmap[0]+btop(cpup->pc_umsize),
			DEVSPACESIZE ,PG_V|PG_KW);

/*---------------------------------------------------------------------
* map the QDSS into the Qbus memory (which is now in system space)  */

	devptr = (u_short *)((char *)qmem[0]+cpup->pc_umsize);
	qdaddr = (u_short *)((u_int)devptr + ubdevreg(QDSSCSR));

	if (BADADDR(qdaddr, sizeof(short)))
		return(0);

	/*---------------------------------------------------
	* tell QDSS which Q memory address base to decode */

	mapix = (int) VTOP(QMEMSIZE - CHUNK);
	ptep = (int *) QMEMmap[0] + mapix;
	phys_adr = (caddr_t) (((int)*ptep & 0x001FFFFF) << PGSHIFT);
	*qdaddr = (u_short) ((int)phys_adr >> 16);

	qdflags[unit].config = *(u_short *)qdaddr;

/*----------------------------------------------------------------------
* load qdmap struct with the virtual addresses of the QDSS elements */

	qdbase[unit] = (caddr_t) (qmem[0] + QMEMSIZE - CHUNK);

	qdmap[unit].template = qdbase[unit] + TMPSTART;
	qdmap[unit].adder = qdbase[unit] + ADDER;
	qdmap[unit].dga = qdbase[unit] + DGA;
	qdmap[unit].duart = qdbase[unit] + DUART;
	qdmap[unit].memcsr = qdbase[unit] + MEMCSR;
	qdmap[unit].red = qdbase[unit] + RED;
	qdmap[unit].blue = qdbase[unit] + BLUE;
	qdmap[unit].green = qdbase[unit] + GREEN;

	qdflags[unit].duart_imask = 0;	/* init shadow variables */

/*------------------
* init the QDSS  */

	*(short *)qdmap[unit].memcsr |= SYNC_ON; /* once only: turn on sync */

	cursor[unit].x = 0;
	cursor[unit].y = 0;
	init_shared(unit);		/* init shared memory */
	setup_dragon(unit);		/* init the ADDER/VIPER stuff */
	clear_qd_screen(unit);		/* clear the screen */
	ldfont(unit);			/* load the console font */
	ldcursor(unit, cons_cursor);	/* load default cursor map */
	setup_input(unit);		/* init the DUART */

/*----------------------------------------------------
* smash the system's virtual console address table */

	v_consputc = qdputc;
	v_consgetc = qdgetc;
	cdevsw[0] = cdevsw[QDSSMAJOR];
 	ws_display_type = QDSS_DTYPE;	/* Idenify QDSS as graphics device */
	return(1);

} /* qdcons_init */

/*********************************************************************
*
*	qdprobe()... configure QDSS into Q memory and make it intrpt
*
**********************************************************************
*
*  calling convention:
*			qdprobe(reg, ctlr);
*			caddr_t reg;
*			int ctlr;
*
*	where: reg - a character pointer to the QDSS I/O page register
*	       ctlr - controller number (?)
*
*  side effects: QDSS gets mapped into Qbus memory space at the first
*		 vacant 64kb boundary counting back from the top of
*		 Qbus memory space (qmem+4mb)
*
*  return: QDSS bus request level and vector address returned in
*	   registers by UNIX convention.
*
*****************/

qdprobe(reg)
caddr_t reg;
{
	/* the variables MUST reside in the first two register declarations
	* by UNIX convention in order that they be loaded and returned
	* properly by the interrupt catching mechanism.  */

	register int unit;

	struct dga *dga;		/* pointer to gate array structure */
	int *ptep;			/* page table entry pointer */
	int vector;

	caddr_t phys_adr;		/* physical QDSS base adrs */
	u_int mapix;

/*---------------------------------------------------------------
* calculate board unit number from I/O page register address  */

	unit = (int) (((int)reg >> 1) & 0x0007);

/*---------------------------------------------------------------------------
* QDSS regs must be mapped to Qbus memory space at a 64kb physical boundary.
* The Qbus memory space is mapped into the system memory space at config
* time.  After config runs, "qmem[0]" (ubavar.h) holds the system virtual adrs
* of the start of Qbus memory.	The Qbus memory page table is found via
* an array of pte ptrs called "QMEMmap[]" (ubavar.h) which is also loaded at
* config time.	These are the variables used below to find a vacant 64kb
* boundary in Qbus memory, and load it's corresponding physical adrs into
* the QDSS's I/O page CSR.  */

	/*
	 * Only if QD is the graphics device.
	 */

	if (ws_display_type && ((ws_display_type & 0xff) != QDSS_DTYPE))
		return(0);

	/* if this QDSS is NOT the console, then do init here.. */

	if (v_consputc != qdputc  ||  unit != 0) {

	    /*-------------------------
	    * read QDSS config info */

	    qdflags[unit].config = *(u_short *)reg;

	    /*------------------------------------
	    * find an empty 64kb adrs boundary */

	    qdbase[unit] = (caddr_t) (qmem[0] + QMEMSIZE - CHUNK);

	    while ( !(BADADDR(qdbase[unit], sizeof(short))) )
		qdbase[unit] -= CHUNK;

	    /*---------------------------------------------------
	    * tell QDSS which Q memory address base to decode */

	    mapix = (int) (VTOP(qdbase[unit]) - VTOP(qmem[0]));
	    ptep = (int *) QMEMmap[0] + mapix;
	    phys_adr = (caddr_t) (((int)*ptep & 0x001FFFFF) << PGSHIFT);
	    *(u_short *)reg = (u_short) ((int)phys_adr >> 16);

	    /*-----------------------------------------------------------
	    * load QDSS adrs map with system addresses of device regs */

	    qdmap[unit].template = qdbase[unit] + TMPSTART;
	    qdmap[unit].adder = qdbase[unit] + ADDER;
	    qdmap[unit].dga = qdbase[unit] + DGA;
	    qdmap[unit].duart = qdbase[unit] + DUART;
	    qdmap[unit].memcsr = qdbase[unit] + MEMCSR;
	    qdmap[unit].red = qdbase[unit] + RED;
	    qdmap[unit].blue = qdbase[unit] + BLUE;
	    qdmap[unit].green = qdbase[unit] + GREEN;

	    /* device init */

	    cursor[unit].x = 0;
	    cursor[unit].y = 0;
	    init_shared(unit);		/* init shared memory */
	    setup_dragon(unit); 	/* init the ADDER/VIPER stuff */
	    ldcursor(unit, cons_cursor);	/* load default cursor map */
	    setup_input(unit);		/* init the DUART */
	    clear_qd_screen(unit);
	    ldfont(unit);			/* load the console font */

	    /* once only: turn on sync */

	    *(short *)qdmap[unit].memcsr |= SYNC_ON;
	}

/*--------------------------------------------------------------------------
* the QDSS interrupts at HEX vectors xx0 (DMA) xx4 (ADDER) and xx8 (DUART).
* Therefore, we take three vectors from the vector pool, and then continue
* to take them until we get a xx0 HEX vector.  The pool provides vectors
* in contiguous decending order.  */

	vector = (uba_hd[0].uh_lastiv -= 4*3);	/* take three vectors */

	while (vector & 0x0F) { 		   /* if lo nibble != 0.. */
	    vector = (uba_hd[0].uh_lastiv -= 4);  /* ..take another vector */
	}

	/*---------------------------------------------------------
	* setup DGA to do a DMA interrupt (transfer count = 0)	*/

	dga = (struct dga *) qdmap[unit].dga;

	dga->csr = (short) HALT;	      /* disable everything */
	dga->ivr = (short) vector;	      /* load intrpt base vector */
	dga->bytcnt_lo = (short) 0;	      /* DMA xfer count = 0 */
	dga->bytcnt_hi = (short) 0;

	/* turn on DMA interrupts */

	dga->csr &= ~SET_DONE_FIFO;
	dga->csr |= DMA_IE | DL_ENB;

	DELAY(20000);			/* wait for the intrpt */

	dga->csr = HALT;		/* stop the wheels */

/*----------
* exits  */

	if (cvec != vector)		/* if vector != base vector.. */
	    return(0);			/* ..return = 'no device' */

	/*
	 * score this as an existing qdss
	 */
	qdcount++;
 	ws_display_units |= (1 << unit);
	ws_display_type |= (QDSS_DTYPE << (8*unit));

	return(sizeof(short));	    /* return size of QDSS I/O page reg */

} /* qdprobe */

/*****************************************************************
*
*	qdattach()... do the one-time initialization
*
******************************************************************
*
*  calling convention:
*			qdattach(ui);
*			struct uba_device *ui;
*
*		where: ui - pointer to the QDSS's uba_device structure
*
*  side effects: none
*	 return: none
*
*************************/

qdattach(ui)
struct uba_device *ui;
{
	register u_int unit;		/* QDSS module # for this call */

	unit = ui->ui_unit;		/* get QDSS number */

/*----------------------------------
* init "qdflags[]" for this QDSS */

	qdflags[unit].inuse = 0;	/* init inuse variable EARLY! */
	qdflags[unit].mapped = 0;
	qdflags[unit].kernel_loop = -1;
	qdflags[unit].user_dma = 0;
	qdflags[unit].curs_acc = ACC_OFF;
	qdflags[unit].curs_thr = 128;
	qdflags[unit].tab_res = 2;	/* default tablet resolution factor */
	qdflags[unit].duart_imask = 0;	/* init shadow variables */
	qdflags[unit].adder_ie = 0;

/*----------------------------------------------------------------------
* init structures used in kbd/mouse interrupt service.	This code must
* come after the "init_shared()" routine has run since that routine inits
* the eq_header[unit] structure used here.   */

	/*--------------------------------------------
	* init the "latest mouse report" structure */

	last_rep[unit].state = 0;
	last_rep[unit].dx = 0;
	last_rep[unit].dy = 0;
	last_rep[unit].bytcnt = 0;

	/*------------------------------------------------
	* init the event queue (except mouse position) */

	eq_header[unit]->header.events = (struct _vs_event *)
					  ((int)eq_header[unit]
					   + sizeof(struct qdinput));

	eq_header[unit]->header.size = MAXEVENTS;
	eq_header[unit]->header.head = 0;
	eq_header[unit]->header.tail = 0;

/*------------------------------------------
* init single process access lock switch */

	one_only[unit] = 0;

} /* qdattach */

/***************************************************************
*
*	qdopen()... open a minor device
*
****************************************************************
*
*  calling convention: qdopen(dev, flag);
*		       dev_t dev;
*		       int flag;
*
*  side effects: none
*
*********************/

qdopen(dev, flag)
dev_t dev;
int flag;
{
	register struct uba_device *ui; /* ptr to uba structures */
	register struct dga *dga;	/* ptr to gate array struct */
	register struct tty *tp;

	struct adder *adder;
	struct duart *duart;

	u_int unit;
	u_int minor_dev;
	int s;

	minor_dev = minor(dev); /* get QDSS minor device number */
	unit = minor_dev >> 2;

/*---------------------------------
* check for illegal conditions	*/

	ui = qdinfo[unit];		/* get ptr to QDSS device struct */

	if (ui == 0  || ui->ui_alive == 0)
	    return(ENXIO);		/* no such device or address */

/*--------------
* init stuff */

	adder = (struct adder *) qdmap[unit].adder;
	duart = (struct duart *) qdmap[unit].duart;
	dga = (struct dga *) qdmap[unit].dga;

/*------------------------------------
* if this is the graphic device... */

	if ((minor_dev & 0x03) == 2) {

	    if (one_only[unit] != 0)
		return(EBUSY);
	    else
		one_only[unit] = 1;

	    qdflags[unit].inuse |= GRAPHIC_DEV;  /* graphics dev is open */

	    /* enble kbd & mouse intrpts in DUART mask reg */

	    qdflags[unit].duart_imask |= 0x22;
	    duart->imask = qdflags[unit].duart_imask;


/*------------------------------------------------------------------
* if the open call is to the console or the alternate console... */

	} else if ((minor_dev & 0x03) != 2) {

	    qdflags[unit].inuse |= CONS_DEV;  /* mark console as open */
	    dga->csr |= CURS_ENB;

	    qdflags[unit].duart_imask |= 0x02;
	    duart->imask = qdflags[unit].duart_imask;

	    /*-------------------------------
	    * some setup for tty handling */

	    tp = &qd_tty[minor_dev];

	    tp->t_addr = ui->ui_addr;
	    tp->t_oproc = qdstart;
	/*---------------------------------------------------------------------
	* Look at the compatibility mode to specify correct default parameters
	* and to insure only standard specified functionality. */
	if ((u.u_procp->p_progenv == A_SYSV) || 
		(u.u_procp->p_progenv == A_POSIX)) {
		flag |= O_TERMIO;
		tp->t_line = TERMIODISC;
	}
	/*--------------------------------------------------------------
	* Set state bit to tell tty.c not to assign this line as the 
	* controlling terminal for the process which opens this line. */
	if ((flag & O_NOCTTY) && (u.u_procp->p_progenv == A_POSIX))
		tp->t_state |= TS_ONOCTTY;

	    if ((tp->t_state & TS_ISOPEN) == 0) {

		ttychars(tp);
		tp->t_state = TS_ISOPEN | TS_CARR_ON;
		tp->t_cflag = tp->t_cflag_ext = B9600;
		tp->t_iflag_ext = 0;
		tp->t_oflag_ext = 0;
		tp->t_lflag_ext = 0;

		if( (minor_dev & 0x03) == 0 ) {
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
                if( (minor_dev & 0x03) == 1 )
                    tp->t_iflag |= IXOFF;	/* flow control for qconsole */
	    }

	    /*----------------------------------------
	    * enable intrpts, open line discipline */

	    dga->csr |= GLOBAL_IE;	/* turn on the interrupts */
	    return ((*linesw[tp->t_line].l_open)(dev, tp));
	}

	dga->csr |= GLOBAL_IE;	/* turn on the interrupts */
	return(0);

} /* qdopen */

/***************************************************************
*
*	qdclose()... clean up on the way out
*
****************************************************************
*
*  calling convention: qdclose();
*
*  side effects: none
*
*  return: none
*
*********************/

qdclose(dev, flag)
dev_t dev;
int flag;
{
	register struct tty *tp;
	register struct qdmap *qd;
	register int *ptep;
	int i;				/* SIGNED index */

	struct dga *dga;		/* gate array register map pointer */
	struct duart *duart;
	struct adder *adder;

	u_int unit;
	u_int minor_dev;
	u_int mapix;

	minor_dev = minor(dev); 	/* get minor device number */
	unit = minor_dev >> 2;		/* get QDSS number */
	qd = &qdmap[unit];

/*------------------------------------
* if this is the graphic device... */

	if ((minor_dev & 0x03) == 2) {

	    /*-----------------
	    * unlock driver */

	    if (one_only[unit] != 1)
		return(EBUSY);
	    else
		one_only[unit] = 0;

	    /*----------------------------
	    * re-protect device memory */

	    if (qdflags[unit].mapped & MAPDEV) {

		/*----------------
		* TEMPLATE RAM */

		mapix = VTOP((int)qd->template) - VTOP(qmem[0]);
		ptep = (int *)(QMEMmap[0] + mapix);

		for (i = VTOP(TMPSIZE); i > 0; --i)
		    *ptep++ = (*ptep & ~PG_PROT) | PG_V | PG_KW;

		/*---------
		* ADDER */

		mapix = VTOP((int)qd->adder) - VTOP(qmem[0]);
		ptep = (int *)(QMEMmap[0] + mapix);

		for (i = VTOP(REGSIZE); i > 0; --i)
		    *ptep++ = (*ptep & ~PG_PROT) | PG_V | PG_KW;

		/*--------------
		* COLOR MAPS */

		mapix = VTOP((int)qd->red) - VTOP(qmem[0]);
		ptep = (int *)(QMEMmap[0] + mapix);

		for (i = VTOP(CLRSIZE); i > 0; --i)
		    *ptep++ = (*ptep & ~PG_PROT) | PG_V | PG_KW;
	    }

	    /*----------------------------------------------------
	    * re-protect DMA buffer and free the map registers */

	    if (qdflags[unit].mapped & MAPDMA) {

		dga = (struct dga *) qdmap[unit].dga;
		adder = (struct adder *) qdmap[unit].adder;

		dga->csr &= ~DMA_IE;
		dga->csr &= ~0x0600;	     /* kill DMA */
		adder->command = CANCEL;

		/* if DMA was running, flush spurious intrpt */

		if (dga->bytcnt_lo != 0) {
		    dga->bytcnt_lo = 0;
		    dga->bytcnt_hi = 0;
		    DMA_SETIGNORE(DMAheader[unit]);
		    dga->csr |= DMA_IE;
		    dga->csr &= ~DMA_IE;
		}

		ptep = (int *)
			((VTOP(DMAheader[unit]*4)) + (mfpr(SBR)|0x80000000));

		for (i = (DMAbuf_size >> PGSHIFT); i > 0; --i)
		    *ptep++ = (*ptep & ~PG_PROT) | PG_V | PG_KW;

		ubarelse(0, &Qbus_unmap[unit]);
	    }

	    /*---------------------------------------
	    * re-protect 1K (2 pages) event queue */

	    if (qdflags[unit].mapped & MAPEQ) {

		ptep = (int *)
			((VTOP(eq_header[unit])*4) + (mfpr(SBR)|0x80000000));

		*ptep++ = (*ptep & ~PG_PROT) | PG_KW | PG_V;
		*ptep = (*ptep & ~PG_PROT) | PG_KW | PG_V;
	    }

	    /*------------------------------------------------------------
	    * re-protect scroll param area and disable scroll intrpts  */

	    if (qdflags[unit].mapped & MAPSCR) {

		ptep = (int *) ((VTOP(scroll[unit]) * 4)
				    + (mfpr(SBR) | 0x80000000));

		/* re-protect 512 scroll param area */

		*ptep = (*ptep & ~PG_PROT) | PG_KW | PG_V;

		adder = (struct adder *) qdmap[unit].adder;
		qdflags[unit].adder_ie &= ~FRAME_SYNC;
		adder->interrupt_enable = qdflags[unit].adder_ie;
	    }

	    /*-----------------------------------------------------------
	    * re-protect color map write buffer area and kill intrpts */

	    if (qdflags[unit].mapped & MAPCOLOR) {

		ptep = (int *) ((VTOP(color_buf[unit]) * 4)
				    + (mfpr(SBR) | 0x80000000));

		*ptep++ = (*ptep & ~PG_PROT) | PG_KW | PG_V;
		*ptep = (*ptep & ~PG_PROT) | PG_KW | PG_V;

		color_buf[unit]->status = 0;

                adder = (struct adder *) qdmap[unit].adder;
                qdflags[unit].adder_ie &= ~VSYNC;
                adder->interrupt_enable = qdflags[unit].adder_ie;
	    }

	    /*-----------------------------------
	    * flag that everthing is unmapped */

	    mtpr(TBIA, 0);		/* smash CPU's translation buf */
	    qdflags[unit].mapped = 0;	/* flag everything now unmapped */
	    qdflags[unit].inuse &= ~GRAPHIC_DEV;
	    qdflags[unit].curs_acc = ACC_OFF;
	    qdflags[unit].curs_thr = 128;

	    /*---------------------
	    * restore the console */

		dga = (struct dga *) qdmap[unit].dga;
		adder = (struct adder *) qdmap[unit].adder;

		dga->csr &= ~DMA_IE;
		dga->csr &= ~0x0600;	/* halt the DMA! (just in case...) */
		dga->csr |= DMA_ERR;	/* clear error condition */
		adder->command = CANCEL;

		/* if DMA was running, flush spurious intrpt */

		if (dga->bytcnt_lo != 0) {
		    dga->bytcnt_lo = 0;
		    dga->bytcnt_hi = 0;
		    DMA_SETIGNORE(DMAheader[unit]);
		    dga->csr |= DMA_IE;
		    dga->csr &= ~DMA_IE;
		}

		init_shared(unit);		/* init shared memory */
		setup_dragon(unit);		/* init ADDER/VIPER */
		ldcursor(unit, cons_cursor);	/* load default cursor map */
		setup_input(unit);		/* init the DUART */
		ldfont(unit);
		cursor[unit].x = 0;
		cursor[unit].y = 0;

	    /* shut off the mouse rcv intrpt and turn on kbd intrpts */

	    duart = (struct duart *) qdmap[unit].duart;
	    qdflags[unit].duart_imask &= ~(0x20);
	    qdflags[unit].duart_imask |= 0x02;
	    duart->imask = qdflags[unit].duart_imask;

	    /*-----------------------------------------
	    * shut off interrupts if all is closed  */

	    if (!(qdflags[unit].inuse & (CONS_DEV | ALTCONS_DEV))) {

		dga = (struct dga *) qdmap[unit].dga;
		dga->csr &= ~(GLOBAL_IE | DMA_IE);
	    }
	}

/*----------------------------------------------------
* if this is the console or the alternate console  */

	else {

	    tp = &qd_tty[minor_dev];

	    (*linesw[tp->t_line].l_close)(tp);
	    ttyclose(tp);

	    tp->t_state = 0;
	    /* Remove termio flags that do not map */
	    tp->t_iflag &= ~TERMIO_ONLY_IFLAG;
	    tp->t_oflag &= ~TERMIO_ONLY_OFLAG;
	    tp->t_cflag &= ~TERMIO_ONLY_CFLAG;
	    tp->t_lflag &= ~TERMIO_ONLY_LFLAG;

	    qdflags[unit].inuse &= ~CONS_DEV;
	    q_keyboard.cntrl = q_keyboard.shift = 0;

	    /*-------------------------------------------------
	    * if graphics device is closed, kill interrupts */

	    if (!(qdflags[unit].inuse & GRAPHIC_DEV)) {
		dga = (struct dga *) qdmap[unit].dga;
		dga->csr &= ~(GLOBAL_IE | DMA_IE);
	    }
	}

/*--------
* exit */

	return(0);

} /* qdclose */

/***************************************************************
*
*	qdioctl()... provide QDSS control services
*
****************************************************************
*
*  calling convention:	qdioctl(dev, cmd, datap, flags);
*
*		where:	dev - the major/minor device number
*			cmd - the user-passed command argument
*			datap - ptr to user input buff (128 bytes max)
*			flags - "f_flags" from "struct file" in file.h
*
*
*	- here is the format for the input "cmd" argument
*
*	31     29 28	23 22	      16 15		8 7		 0
*	+----------------------------------------------------------------+
*	|I/O type|	  | buff length | device ID char |  user command |
*	+----------------------------------------------------------------+
*
*  Return data is in the data buffer pointed to by "datap" input spec
*
*********************/

qdioctl(dev, cmd, datap, flags)
dev_t dev;
int cmd;
caddr_t datap;
int flags;
{
	register int *ptep;		/* page table entry pointer */
	register int mapix;		/* QMEMmap[] page table index */
	register struct _vs_event *event;
	register struct tty *tp;

	struct qdmap *qd;		/* pointer to device map struct */
	struct dga *dga;		/* Gate Array reg structure pntr */
	struct duart *duart;		/* DUART reg structure pointer */
	struct adder *adder;		/* ADDER reg structure pointer */

	struct prgkbd *cmdbuf;
	struct prg_cursor *curs;
	struct _vs_cursor *pos;

	u_int unit = minor(dev) >> 2;	/* number of caller's QDSS */
	u_int minor_dev = minor(dev);
	struct uba_device *ui = qdinfo[unit];
	struct qd_softc *sc = &qd_softc[ui->ui_unit];
	struct devget *devget;

	int error;
	int s;

	int i;				/* SIGNED index */
	int sbr;			/* SBR variable (you silly boy) */
	u_int ix;

	short status;
	short *shortp;			/* generic pointer to a short */
	char *chrp;			/* generic character pointer */

	short *temp;			/* a pointer to template RAM */

/*-----------------------------------------
* service graphic device ioctl commands */

	switch (cmd) {

	    /*-------------------------------------------------
	    * extract the oldest event from the event queue */

	    case QD_GETEVENT:

		if (ISEMPTY(eq_header[unit])) {
		    event = (struct _vs_event *) datap;
		    event->vse_device = VSE_NULL;
		    break;
		}

		event = (struct _vs_event *) GETBEGIN(eq_header[unit]);
		s = spl5();
		GETEND(eq_header[unit]);
		splx(s);
		bcopy(event, datap, sizeof(struct _vs_event));
		break;

	    /*-------------------------------------------------------
	    * init the dragon stuff, DUART, and driver variables  */

	    case QD_RESET:

		init_shared(unit);		/* init shared memory */
		setup_dragon(unit);	      /* init the ADDER/VIPER stuff */
		clear_qd_screen(unit);
		ldcursor(unit, cons_cursor);	/* load default cursor map */
		ldfont(unit);			/* load the console font */
		setup_input(unit);		/* init the DUART */
		break;

	    /*----------------------------------------
	    * init the DUART and driver variables  */

	    case QD_SET:

		init_shared(unit);
		setup_input(unit);
		break;

	    /*---------------------------------------------------------------
	    * clear the QDSS screen.  (NOTE that this reinits the dragon) */

	    case QD_CLRSCRN:

		setup_dragon(unit);
		clear_qd_screen(unit);
		break;

	    /*------------------------------------
	    * load a cursor into template RAM  */

	    case QD_WTCURSOR:

		ldcursor(unit, datap);
		break;

	    case QD_RDCURSOR:

		temp = (short *) qdmap[unit].template;

		/* cursor is 32 WORDS from the end of the 8k WORD...
		*  ...template space */

		temp += (8 * 1024) - 32;

		for (i = 0; i < 32; ++i, datap += sizeof(short))
		    *(short *)datap = *temp++;
		break;

	    /*------------------------------
	    * position the mouse cursor  */

	    case QD_POSCURSOR:

		dga = (struct dga *) qdmap[unit].dga;
		pos = (struct _vs_cursor *) datap;
		s = spl5();
		dga->x_cursor = TRANX(pos->x);
		dga->y_cursor = TRANY(pos->y);
		eq_header[unit]->curs_pos.x = pos->x;
		eq_header[unit]->curs_pos.y = pos->y;
		splx(s);
		break;

	    /*--------------------------------------
	    * set the cursor acceleration factor */

	    case QD_PRGCURSOR:

		curs = (struct prg_cursor *) datap;
		s = spl5();
		qdflags[unit].curs_acc = curs->acc_factor;
		qdflags[unit].curs_thr = curs->threshold;
		splx(s);
		break;

	    /*---------------------------------------
	    * enable 'user write' to device pages */

	    case QD_MAPDEVICE:

		/*--------------
		* init stuff */

		qdflags[unit].mapped |= MAPDEV;
		qd = (struct qdmap *) &qdmap[unit];

		/*-------------------------------------
		* enable user write to template RAM */

		mapix = VTOP((int)qd->template) - VTOP(qmem[0]);
		ptep = (int *)(QMEMmap[0] + mapix);

		for (i = VTOP(TMPSIZE); i > 0; --i)
		    *ptep++ = (*ptep & ~PG_PROT) | PG_UW | PG_V;

		/*----------------------------------
		* enable user write to registers */

		mapix = VTOP((int)qd->adder) - VTOP(qmem[0]);
		ptep = (int *)(QMEMmap[0] + mapix);

		for (i = VTOP(REGSIZE); i > 0; --i)
		    *ptep++ = (*ptep & ~PG_PROT) | PG_UW | PG_V;

		/*-----------------------------------
		* enable user write to color maps */

		mapix = VTOP((int)qd->red) - VTOP(qmem[0]);
		ptep = (int *)(QMEMmap[0] + mapix);

		for (i = VTOP(CLRSIZE); i > 0; --i)
		    *ptep++ = (*ptep & ~PG_PROT) | PG_UW | PG_V;

		/*------------------------------
		* enable user write to DUART */

		mapix = VTOP((int)qd->duart) - VTOP(qmem[0]);
		ptep = (int *)(QMEMmap[0] + mapix);
		*ptep = (*ptep & ~PG_PROT) | PG_UW | PG_V; /* duart page */

		mtpr(TBIA, 0);		/* smash CPU's translation buffer */

		/*------------------------------------------
		* stuff qdmap structure in return buffer */

		bcopy(qd, datap, sizeof(struct qdmap));
		break;

	    /*-------------------------------------
	    * do setup for DMA by user process	*/

	    case QD_MAPIOBUF:

		/*------------------------------------------------
		* set 'user write enable' bits for DMA buffer  */

		qdflags[unit].mapped |= MAPDMA;

		ptep = (int *) ((VTOP(DMAheader[unit]) * 4)
				+ (mfpr(SBR) | 0x80000000));

		for (i = (DMAbuf_size >> PGSHIFT); i > 0; --i)
		    *ptep++ = (*ptep & ~PG_PROT) | PG_UW | PG_V;

		mtpr(TBIA, 0);			/* clr CPU translation buf */

		/*-------------------------------------
		* set up QBUS map registers for DMA */

		DMAheader[unit]->QBAreg =
				uballoc(0, DMAheader[unit], DMAbuf_size, 0);

		if (DMAheader[unit]->QBAreg == 0)
		    mprintf("\nqd%d: qdioctl: QBA setup error", unit);

		Qbus_unmap[unit] = DMAheader[unit]->QBAreg;
		DMAheader[unit]->QBAreg &= 0x3FFFF;

		/*----------------------
		* return I/O buf adr */

		*(int *)datap = (int) DMAheader[unit];
		break;

	    /*----------------------------------------------------------------
	    * map the shared scroll param area and enable scroll interpts  */

	    case QD_MAPSCROLL:

		qdflags[unit].mapped |= MAPSCR;

		ptep = (int *) ((VTOP(scroll[unit]) * 4)
				+ (mfpr(SBR) | 0x80000000));

		/* allow user write to scroll area */

		*ptep = (*ptep & ~PG_PROT) | PG_UW | PG_V;

		mtpr(TBIA, 0);			/* clr CPU translation buf */

		scroll[unit]->status = 0;

		adder = (struct adder *) qdmap[unit].adder;

		qdflags[unit].adder_ie |= FRAME_SYNC;
		adder->interrupt_enable = qdflags[unit].adder_ie;

		/* return scroll area address */

		*(int *)datap = (int) scroll[unit];
		break;

	    /*-------------------------------------------------------------
	    * unmap shared scroll param area and disable scroll intrpts */

	    case QD_UNMAPSCROLL:

		if (qdflags[unit].mapped & MAPSCR) {

		    qdflags[unit].mapped &= ~MAPSCR;

		    ptep = (int *) ((VTOP(scroll[unit]) * 4)
				    + (mfpr(SBR) | 0x80000000));

		    /* re-protect 512 scroll param area */

		    *ptep = (*ptep & ~PG_PROT) | PG_KW | PG_V;

		    mtpr(TBIA, 0);	/* smash CPU's translation buf */

		    adder = (struct adder *) qdmap[unit].adder;
		    qdflags[unit].adder_ie &= ~FRAME_SYNC;
		    adder->interrupt_enable = qdflags[unit].adder_ie;
		}
		break;

	    /*-----------------------------------------------------------
	    * map shared color map write buf and turn on vsync intrpt */

	    case QD_MAPCOLOR:

		qdflags[unit].mapped |= MAPCOLOR;

		ptep = (int *) ((VTOP(color_buf[unit]) * 4)
				+ (mfpr(SBR) | 0x80000000));

		/* allow user write to color map write buffer */

		*ptep++ = (*ptep & ~PG_PROT) | PG_UW | PG_V;
		*ptep = (*ptep & ~PG_PROT) | PG_UW | PG_V;

		mtpr(TBIA, 0);			/* clr CPU translation buf */

                adder = (struct adder *) qdmap[unit].adder;

                qdflags[unit].adder_ie |= VSYNC;
                adder->interrupt_enable = qdflags[unit].adder_ie;

		/* return color area address */

		*(int *)datap = (int) color_buf[unit];
		break;

	    /*--------------------------------------------------------------
	    * unmap shared color map write buffer and kill VSYNC intrpts */

	    case QD_UNMAPCOLOR:

		if (qdflags[unit].mapped & MAPCOLOR) {

		    qdflags[unit].mapped &= ~MAPCOLOR;

		    ptep = (int *) ((VTOP(color_buf[unit]) * 4)
				    + (mfpr(SBR) | 0x80000000));

		    /* re-protect color map write buffer */

		    *ptep++ = (*ptep & ~PG_PROT) | PG_KW | PG_V;
		    *ptep = (*ptep & ~PG_PROT) | PG_KW | PG_V;

		    mtpr(TBIA, 0);	/* smash CPU's translation buf */

                    adder = (struct adder *) qdmap[unit].adder;

                    qdflags[unit].adder_ie &= ~VSYNC;
                    adder->interrupt_enable = qdflags[unit].adder_ie;
		}
		break;

	    /*---------------------------------------------
	    * give user write access to the event queue */

	    case QD_MAPEVENT:

		qdflags[unit].mapped |= MAPEQ;

		ptep = (int *) ((VTOP(eq_header[unit]) * 4)
				+ (mfpr(SBR) | 0x80000000));

		/* allow user write to 1K event queue */

		*ptep++ = (*ptep & ~PG_PROT) | PG_UW | PG_V;
		*ptep = (*ptep & ~PG_PROT) | PG_UW | PG_V;

		mtpr(TBIA, 0);			/* clr CPU translation buf */

		/* return event queue address */

		*(int *)datap = (int) eq_header[unit];
		break;

	    /*-----------------------------------------------
	    * pass caller's programming commands to LK201 */

	    case QD_PRGKBD:

		duart = (struct duart *) qdmap[unit].duart;
		cmdbuf = (struct prgkbd *) datap;    /* pnt to kbd cmd buf */

		/*----------------
		* send command */

		for (i = 1000; i > 0; --i) {
		    if ((status = duart->statusA) & XMT_RDY) {
			duart->dataA = cmdbuf->cmd;
			break;
		    }
		}

		if (i == 0) {
		    mprintf("\nqd%d: qdioctl: timeout on XMT_RDY [1]", unit);
		    break;
		}

		/*----------------
		* send param1? */

		if (cmdbuf->cmd & LAST_PARAM)
		    break;

		for (i = 1000; i > 0; --i) {
		    if ((status = duart->statusA) & XMT_RDY) {
			duart->dataA = cmdbuf->param1;
			break;
		    }
		}

		if (i == 0) {
		    mprintf("\nqd%d: qdioctl: timeout on XMT_RDY [2]", unit);
		    break;
		}

		/*----------------
		* send param2? */

		if (cmdbuf->param1 & LAST_PARAM)
		    break;

		for (i = 1000; i > 0; --i) {
		    if ((status = duart->statusA) & XMT_RDY) {
			duart->dataA = cmdbuf->param2;
			break;
		    }
		}

		if (i == 0) {
		    mprintf("\nqd%d: qdioctl: timeout on XMT_RDY [3]", unit);
		    break;
		}

		break;

	    /*----------------------------------------------------
	    * pass caller's programming commands to the mouse  */

	    case QD_PRGMOUSE:

		duart = (struct duart *) qdmap[unit].duart;

		for (i = 1000; i > 0; --i) {
		    if ((status = duart->statusB) & XMT_RDY) {
			duart->dataB = *datap;
			break;
		    }
		}

		if (i == 0) {
		    mprintf("\nqd%d: qdioctl: timeout on XMT_RDY [4]", unit);
		}

		break;

	    /*----------------------------------------------
	    * get QDSS configuration word and return it  */

	    case QD_RDCONFIG:

		*(short *)datap = qdflags[unit].config;
		break;

	    /*--------------------------------------------------------------
	    * re-route kernel console messages to the alternate console  */

	    case QD_KERN_LOOP:

		qdflags[unit].kernel_loop = -1;
		break;

	    case QD_KERN_UNLOOP:

		qdflags[unit].kernel_loop = 0;
		break;

	    /*----------------------
	    * program the tablet */

	    case QD_PRGTABLET:

		duart = (struct duart *) qdmap[unit].duart;

		for (i = 1000; i > 0; --i) {
		    if ((status = duart->statusB) & XMT_RDY) {
			duart->dataB = *datap;
			break;
		    }
		}

		if (i == 0) {
		    mprintf("\nqd%d: qdioctl: timeout on XMT_RDY [5]", unit);
		}

		break;

	    /*-----------------------------------------------
	    * program the tablet report resolution factor */

	    case QD_PRGTABRES:

		qdflags[unit].tab_res = *(short *)datap;
		break;

	    case DEVIOCGET:			    /* device status */
		    devget = (struct devget *)datap;
		    bzero(devget,sizeof(struct devget));
		    devget->category = DEV_TERMINAL;
		    devget->bus = DEV_QB;
		    bcopy(DEV_VCB02,devget->interface,
			  strlen(DEV_VCB02));
		    bcopy(DEV_VR290,devget->device,
			  strlen(DEV_VR290));		    /* terminal */
		    devget->adpt_num = ui->ui_adpt;	    /* which adapter*/
		    devget->nexus_num = ui->ui_nexus;	    /* which nexus  */
		    devget->bus_num = ui->ui_ubanum;	    /* which QB     */
		    devget->ctlr_num = unit;		    /* which interf.*/
		    devget->slave_num = unit;		    /* which line   */
		    bcopy(ui->ui_driver->ud_dname,
			  devget->dev_name,
			  strlen(ui->ui_driver->ud_dname)); /* Ultrix "qd"  */
		    devget->unit_num = unit;		    /* qd line?     */
		    devget->soft_count =
			  sc->sc_softcnt;		    /* soft er. cnt.*/
		    devget->hard_count =
			  sc->sc_hardcnt;		    /* hard er cnt. */
		    devget->stat = sc->sc_flags;	    /* status	    */
		    devget->category_stat =
			  sc->sc_category_flags;	    /* cat. stat.   */
		    break;

	    default:
		/*-----------------------------
		* service tty type ioctl's  */

		    if (!(minor_dev & 0x02)) {

			tp = &qd_tty[minor_dev];

			error = (*linesw[tp->t_line].l_ioctl)(tp, cmd, datap, flags);
			if (error >= 0) {
			    return(error);
			}

			error = ttioctl(tp, cmd, datap, flags);
			if (error >= 0) {
			    return(error);
			}
			/* if error = -1 then ioctl does not exist */
			if (u.u_procp->p_progenv == A_POSIX) 
			    return (EINVAL);
			return (ENOTTY);
		    
		    }
		    break;
	}

/*--------------------------------
* clean up and get outta here  */

	return(0);

} /* qdioctl */

/**********************************************************************
*
*	qdselect()... service select call for event queue input
*
**********************************************************************/

qdselect(dev, rw)
dev_t dev;
int rw;
{
	register int s;
	register int unit;
	register struct tty *tp;
	u_int minor_dev = minor(dev);

	s = spl5();
	unit = minor_dev >> 2;

	switch (rw) {

	    case FREAD:

		if ((minor_dev & 0x03) == 2)
		{
		    /*
		     * this is a graphics device, so check for events
		     */
		    if(!(ISEMPTY(eq_header[unit])))
		    {
		        splx(s);
		        return(1);
		    }
		    rsel[unit] = u.u_procp;
		    qdflags[unit].selmask |= SEL_READ;
		    splx(s);
		    return(0);
		}
		else
		{
		    /*
		     * this is a tty device
		     */
	    	    tp = &qd_tty[minor_dev];
		    if (ttnread(tp))
			return(1);
		    tp->t_rsel = u.u_procp;
		    splx(s);
		    return(0);
		}

	    case FWRITE:

		if ((minor(dev) & 0x03) == 2)
		{
		    /*
		     * this is a graphics device, so check for dma buffers
		     */
		    if (DMA_ISEMPTY(DMAheader[unit]))
		    {
		        splx(s);
		        return(1);
		    }
		    rsel[unit] = u.u_procp;
		    qdflags[unit].selmask |= SEL_WRITE;
		    splx(s);
		    return(0);
		}
		else
		{
		    /*
		     * this is a tty device
		     */
	    	    tp = &qd_tty[minor_dev];
		    if (tp->t_outq.c_cc <= TTLOWAT(tp))
			return(1);
		    tp->t_wsel = u.u_procp;
		    splx(s);
		    return(0);
		}
	}

} /* qdselect() */

/***************************************************************
*
*	qdwrite()... output to the QDSS screen as a TTY
*
***************************************************************/

extern qd_strategy();

qdwrite(dev, uio)
dev_t dev;
struct uio *uio;
{
	register struct tty *tp;
	register int minor_dev;
	register int unit;

	minor_dev = minor(dev);
	unit = (minor_dev >> 2) & 0x07;

        /*------------------------------*/
        /* graphic device ... DMA xfers */

	if (((minor_dev & 3) == GRAPHIC) && 
	    (qdflags[unit].inuse & GRAPHIC_DEV)) {
	    return (physio(qd_strategy, &qdbuf[unit],
			   dev, B_WRITE, minphys, uio));
	}

        /*------------------------------*/
        /* console or alternate console */

	else {
	    tp = &qd_tty[minor_dev];
	    return ((*linesw[tp->t_line].l_write)(tp, uio));
	}
}

/***************************************************************
*
*	qdread()... read from QDSS keyboard as a TTY
*
***************************************************************/

qdread(dev, uio)
dev_t dev;
struct uio *uio;
{
	register struct tty *tp;
	register int minor_dev;
	register int unit;

	minor_dev = minor(dev);
	unit = (minor_dev >> 2) & 0x07;

	/*------------------------------
	* if this is the console...  */

	if ((minor_dev & 0x03) != 0x02	&&
	     qdflags[unit].inuse & CONS_DEV) {
	    tp = &qd_tty[minor_dev];
	    return ((*linesw[tp->t_line].l_read)(tp, uio));
	}

	/*------------------------------------------------
	* else this must be a bitmap-to-processor xfer */

	else if (qdflags[unit].inuse & GRAPHIC_DEV) {
	    return (physio(qd_strategy, &qdbuf[unit],
			   dev, B_READ, minphys, uio));
	}
}

/***************************************************************
*
*	qd_strategy()... strategy routine to do DMA
*
***************************************************************/

qd_strategy(bp)
register struct buf *bp;
{
	register struct dga *dga;
	register struct adder *adder;

	char *DMAbufp;

	int QBAreg;
	int bytcnt;
	int s;
	int unit;
	int cookie;

	int i,j,k;

	unit = (minor(bp->b_dev) >> 2) & 0x07;

/*-----------------
* init pointers */

	if ((QBAreg = ubasetup(0, bp, 0)) == 0) {
	    mprintf("\nqd%d: qd_strategy: QBA setup error", unit);
	    goto STRAT_ERR;
	}

	dga = (struct dga *) qdmap[unit].dga;

	s = spl5();

	qdflags[unit].user_dma = -1;

	dga->csr |= DMA_IE;

	cookie = QBAreg & 0x3FFFF;
	dga->adrs_lo = (short) cookie;
	dga->adrs_hi = (short) (cookie >> 16);

	dga->bytcnt_lo = (short) bp->b_bcount;
	dga->bytcnt_hi = (short) (bp->b_bcount >> 16);

	while (qdflags[unit].user_dma) {
	    sleep((caddr_t)&qdflags[unit].user_dma, QDPRIOR);
	}

	splx(s);
	ubarelse(0, &QBAreg);

	if (!(dga->csr & DMA_ERR)) {
	    iodone(bp);
	    return;
	}

STRAT_ERR:
	adder = (struct adder *) qdmap[unit].adder;
	adder->command = CANCEL;		/* cancel adder activity */
	dga->csr &= ~DMA_IE;
	dga->csr &= ~0x0600;		/* halt DMA (reset fifo) */
	dga->csr |= DMA_ERR;		/* clear error condition */
	bp->b_flags |= B_ERROR; 	/* flag an error to physio() */

	/* if DMA was running, flush spurious intrpt */

	if (dga->bytcnt_lo != 0) {
	    dga->bytcnt_lo = 0;
	    dga->bytcnt_hi = 0;
	    DMA_SETIGNORE(DMAheader[unit]);
	    dga->csr |= DMA_IE;
	}

	iodone(bp);

} /* qd_strategy */

/*******************************************************************
*
*	qdstart()... startup output to the console screen
*
********************************************************************
*
*	calling convention:
*
*		qdstart(tp);
*		struct tty *tp; 	;pointer to tty structure
*
********/

qdstart(tp)
register struct tty *tp;
{
	register int which_unit, unit, c;
	register struct tty *tp0;
	int s;

	int curs_on;
	struct dga *dga;

	unit = minor(tp->t_dev);

	tp0 = &qd_tty[(unit & 0x0FC)+1];
	which_unit = (unit >> 2) & 0x3;
	unit &= 0x03;

	s = spl5();

/*------------------------------------------------------------------
* If it's currently active, or delaying, no need to do anything. */

	if (tp->t_state & (TS_TIMEOUT|TS_BUSY|TS_TTSTOP))
		goto out;

/*-------------------------------------------------------------------
* Display chars until the queue is empty, if the alternate console device
* is open direct chars there.  Drop input from anything but the console
* device on the floor.	*/
/* TANDEM is set on the second subchannel for flow control. */

	while ( tp->t_outq.c_cc ) {
	    if (unit == 0) {		/* console device */
		if (tp0->t_state & TS_ISOPEN) {
		    if (tp0->t_state & TS_TBLOCK)
			goto out;
		    c = getc(&tp->t_outq);
		    (*linesw[tp0->t_line].l_rint)(c, tp0);
		} else {
		    c = getc(&tp->t_outq);
		    blitc(which_unit, (char)(c & 0xFF));
		}
	    } else if (unit == 1) {	/* qconsole, do flow control */
		    c = getc(&tp->t_outq);
		    if ((tp0->t_state&TS_TBLOCK) == 0) {
			tp = &qd_tty[0];
			unit = minor(tp->t_dev);
			unit &= 0x03;
			continue;
		    } else
			goto out;
	    } else
		c = getc(&tp->t_outq);
	}

/*--------------------------------------------------------
* If there are sleepers, and output has drained below low
* water mark, wake up the sleepers. */

	if ( tp->t_outq.c_cc <= TTLOWAT(tp) ) {
		if (tp->t_state & TS_ASLEEP){
			tp->t_state &= ~TS_ASLEEP;
			wakeup((caddr_t) &tp->t_outq);
		}
	}

	tp->t_state &= ~TS_BUSY;

out:
	splx(s);

} /* qdstart */


/*******************************************************************
*
*	qdstop()... stop the tty
*
*******************************************************************/

qdstop(tp, flag)
register struct tty *tp;
int flag;
{
	register int s;

	s = spl5();	/* block intrpts during state modification */

	if (tp->t_state & TS_BUSY) {
	    if ((tp->t_state & TS_TTSTOP) == 0) {
		tp->t_state |= TS_FLUSH;
	    } else
		tp->t_state &= ~TS_BUSY;
	}
	splx(s);
}

/*******************************************************************
*
*	blitc()... output a character to the QDSS screen
*
********************************************************************
*
*	calling convention:
*
*		blitc(chr);
*		char chr;		;character to be displayed
*
********/

blitc(unit, chr)
int unit;
unsigned char chr;
{
	register struct adder *adder;
	register struct dga *dga;
	register int i;

	short x;
	unsigned char savechar;

/*---------------
* init stuff  */

	adder = (struct adder *) qdmap[unit].adder;
	dga = (struct dga *) qdmap[unit].dga;

/*---------------------------
* non display character?  */

	chr &= 0xFF;

	switch (chr) {

	    case '\r':			/* return char */
		cursor[unit].x = 0;
	        if (!(qdflags[unit].inuse & GRAPHIC_DEV))
		    dga->x_cursor = TRANX(cursor[unit].x);
		return(0);

	    case '\t':			/* tab char */

		for (i = 8 - ((cursor[unit].x >> 3) & 0x07); i > 0; --i) {
		    blitc(unit, ' ');
		}
		return(0);

	    case '\n':			/* line feed char */

		if ((cursor[unit].y += CHAR_HEIGHT) > (863 - CHAR_HEIGHT)) {
		    if (qdflags[unit].inuse & GRAPHIC_DEV) {
			cursor[unit].y = 0;
		    } else {
			cursor[unit].y -= CHAR_HEIGHT;
			scroll_up(adder);
		    }
		}
	        if (!(qdflags[unit].inuse & GRAPHIC_DEV))
		    dga->y_cursor = TRANY(cursor[unit].y);
		return(0);

	    case '\b':			/* backspace char */
		if (cursor[unit].x > 0) {
		    cursor[unit].x -= CHAR_WIDTH;
		    blitc(unit, ' ');
		    cursor[unit].x -= CHAR_WIDTH;
	            if (!(qdflags[unit].inuse & GRAPHIC_DEV))
		        dga->x_cursor = TRANX(cursor[unit].x);
		}
		return(0);

	    default:
		/*----------------------------------------------------------
		 * Weed out unprintable characters.  Printable characters fall
		 * between space (0x20) and tilde (0x7E).  For 8-bit support
		 * another range of printable characters are those between
		 * 0xA1 and 0xFD. */

		if ((chr < ' ') || (chr > 0xFD) || (chr < 0xA1 && chr > '~'))
		    return(0);
	}

/*------------------------------------------
* setup VIPER operand control registers  */

	write_ID(adder, CS_UPDATE_MASK, 0x0001);  /* select plane #0 */
	write_ID(adder, SRC1_OCR_B,
			EXT_NONE | INT_SOURCE | ID | BAR_SHIFT_DELAY);

	write_ID(adder, CS_UPDATE_MASK, 0x00FE);  /* select other planes */
	write_ID(adder, SRC1_OCR_B,
			EXT_SOURCE | INT_NONE | NO_ID | BAR_SHIFT_DELAY);

	write_ID(adder, CS_UPDATE_MASK, 0x00FF);  /* select all planes */
	write_ID(adder, DST_OCR_B,
			EXT_NONE | INT_NONE | NO_ID | NO_BAR_SHIFT_DELAY);

	write_ID(adder, MASK_1, 0xFFFF);
	write_ID(adder, VIPER_Z_LOAD | FOREGROUND_COLOR_Z, 1);
	write_ID(adder, VIPER_Z_LOAD | BACKGROUND_COLOR_Z, 0);

        adder->x_clip_min = 0;
        adder->x_clip_max = 1024;
        adder->y_clip_min = 0;
        adder->y_clip_max = 864;

/*----------------------------------------
* load DESTINATION origin and vectors  */

	adder->fast_dest_dy = 0;
	adder->slow_dest_dx = 0;
	adder->error_1 = 0;
	adder->error_2 = 0;

	adder->rasterop_mode = DST_WRITE_ENABLE | NORMAL;

	wait_status(adder, RASTEROP_COMPLETE);

	adder->destination_x = cursor[unit].x;
	adder->fast_dest_dx = CHAR_WIDTH;

	adder->destination_y = cursor[unit].y;
	adder->slow_dest_dy = CHAR_HEIGHT;

/*-----------------------------------
* load SOURCE origin and vectors  */

	if (chr > '~') {
		savechar = chr;
		chr -= 34; /* These are to skip the (32) 8-bit control chars. 
			      as well as DEL and 0xA0 which aren't printable */
	}
	if ((chr - ' ') > (CHARS - 1))  {
		mprintf("Invalid character (x)%x in blitc\n",chr);
		chr = ' ';
	}
	/* X position is modulo the number of characters per line */
	adder->source_1_x = FONT_X + 
		(((chr - ' ') % (MAX_SCREEN_X/CHAR_WIDTH)) * CHAR_WIDTH);
	/* Point to either first or second row */
	adder->source_1_y = 2048 - 15 * 
		(((chr - ' ')/(MAX_SCREEN_X/CHAR_WIDTH)) + 1);

	adder->source_1_dx = CHAR_WIDTH;
	adder->source_1_dy = CHAR_HEIGHT;

	write_ID(adder, LU_FUNCTION_R1, FULL_SRC_RESOLUTION | LF_SOURCE);
	adder->cmd = RASTEROP | OCRB | 0 | S1E | DTE;

/*-------------------------------------
* update console cursor coordinates */

	cursor[unit].x += CHAR_WIDTH;
	if (!(qdflags[unit].inuse & GRAPHIC_DEV))
	    dga->x_cursor = TRANX(cursor[unit].x);

	if (cursor[unit].x > (1024 - CHAR_WIDTH)) {
	    blitc(unit, '\r');
	    blitc(unit, '\n');
	}

} /* blitc */

qdreset(){}
qd_init(){}

/******************************************************************
*******************************************************************
*******************************************************************
*
*	INTERRUPT SERVICE ROUTINES START HERE:
*
*******************************************************************
*******************************************************************
******************************************************************/

/*****************************************************************
*
*	qddint()... service "DMA DONE" interrupt condition
*
*****************************************************************/

qddint(qd)
int qd;
{
	register struct DMAreq_header *header;
	register struct DMAreq *request;
	register struct dga *dga;
	struct adder *adder;

	int cookie;			/* DMA adrs for QDSS */
	int i;

	spl4(); 			/* allow interval timer in */

/*-----------------
* init pointers */

	header = DMAheader[qd]; 	    /* register for optimization */
	dga = (struct dga *) qdmap[qd].dga;
	adder = (struct adder *) qdmap[qd].adder;

/*------------------------------------------------------------------------
* if this interrupt flagged as bogus for interrupt flushing purposes.. */

	if (DMA_ISIGNORE(header)) {
	    DMA_CLRIGNORE(header);
	    return;
	}

/*----------------------------------------------------
* dump a DMA hardware error message if appropriate */

	if (dga->csr & DMA_ERR) {

	    if (dga->csr & PARITY_ERR)
		mprintf("\nqd%d: qddint: DMA hardware parity fault.", qd);

	    if (dga->csr & BUS_ERR)
		mprintf("\nqd%d: qddint: DMA hardware bus error.", qd);
	}

/*----------------------------------------
* if this was a DMA from user space... */

	if (qdflags[qd].user_dma) {
	    qdflags[qd].user_dma = 0;
	    wakeup((caddr_t)&qdflags[qd].user_dma);
	    return;
	}

/*------------------------------------------------------------------------
* if we're doing DMA request queue services, field the error condition */

	if (dga->csr & DMA_ERR) {

	    dga->csr &= ~0x0600;		/* halt DMA (reset fifo) */
	    dga->csr |= DMA_ERR;		/* clear error condition */
	    adder->command = CANCEL;		/* cancel adder activity */

	    DMA_SETERROR(header);	/* flag error in header status word */
	    DMA_CLRACTIVE(header);
	    header->DMAreq[header->oldest].DMAdone |= HARD_ERROR;
	    header->newest = header->oldest;
	    header->used = 0;

	    if (rsel[qd] && qdflags[qd].selmask & SEL_WRITE) {
		selwakeup(rsel[qd], 0);
		rsel[qd] = 0;
		qdflags[qd].selmask &= ~SEL_WRITE;
	    }

	    if (dga->bytcnt_lo != 0) {
		dga->bytcnt_lo = 0;
		dga->bytcnt_hi = 0;
		DMA_SETIGNORE(header);
	    }

	    return;
	}

/*----------------------------------------------------------------------------
* if the DMA request queue is now becoming non-full, wakeup "select" client */

	if (DMA_ISFULL(header)) {

	    if (rsel[qd] && qdflags[qd].selmask & SEL_WRITE) {
		selwakeup(rsel[qd], 0);
		rsel[qd] = 0;
		qdflags[qd].selmask &= ~SEL_WRITE;
	    }
	}

	header->DMAreq[header->oldest].DMAdone |= REQUEST_DONE;
        QDlast_DMAtype = header->DMAreq[header->oldest].DMAtype;

	/* check for unexpected interrupt */
	if (DMA_ISEMPTY(header))
	    return;

	DMA_GETEND(header);	/* update request queue indices */

/*------------------------------------------------------------
* if no more DMA pending, wake up "select" client and exit */

	if (DMA_ISEMPTY(header)) {

	    if (rsel[qd] && qdflags[qd].selmask & SEL_WRITE) {
		selwakeup(rsel[qd], 0);
		rsel[qd] = 0;
		qdflags[qd].selmask &= ~SEL_WRITE;
	    }

	    DMA_CLRACTIVE(header);  /* flag DMA done */
	    return;
	}

/*---------------------------
* initiate next DMA xfer  */

	request = DMA_GETBEGIN(header);
	if (request->DMAtype != QDlast_DMAtype) {
	    	dga->csr &= ~0x0600;	  /* halt DMA (reset fifo) */
	    	adder->command = CANCEL;  /* cancel adder activity */
	}


	switch (request->DMAtype) {

	    case DISPLIST:
		if (request->DMAtype != QDlast_DMAtype) {
			dga->csr |= DL_ENB;
			dga->csr &= ~(BTOP_ENB | BYTE_DMA);
		}
		break;

	    case PTOB:
		if (request->DMAtype != QDlast_DMAtype) {
			if (request->DMAdone & BYTE_PACK)
				dga->csr |= (PTOB_ENB | BYTE_DMA);
			else {
				dga->csr |= PTOB_ENB;
				dga->csr &= ~BYTE_DMA;
			}
		}
		break;

	    case BTOP:
		if (request->DMAtype != QDlast_DMAtype) {
			if (request->DMAdone & BYTE_PACK) {
				dga->csr &= ~DL_ENB;
				dga->csr |= (BTOP_ENB | BYTE_DMA);
			}
			else {
				dga->csr |= BTOP_ENB;
				dga->csr &= ~(BYTE_DMA | DL_ENB);
			}
		}
		break;
	    default:
		mprintf("\nqd%d: qddint: illegal DMAtype parameter.", qd);
		DMA_CLRACTIVE(header);	/* flag DMA done */
		return;
	}

	if (request->DMAdone & COUNT_ZERO) {
	    dga->csr &= ~SET_DONE_FIFO;
	} else if (request->DMAdone & FIFO_EMPTY) {
	    dga->csr |= SET_DONE_FIFO;
	}

	if (request->DMAdone & WORD_PACK)
	    dga->csr &= ~BYTE_DMA;
	else if (request->DMAdone & BYTE_PACK)
	    dga->csr |= BYTE_DMA;

	dga->csr |= DMA_IE;
        QDlast_DMAtype = request->DMAtype;

	cookie = ((int)request->bufp - (int)header) + (int)header->QBAreg;

	dga->adrs_lo = (short) cookie;
	dga->adrs_hi = (short) (cookie >> 16);

	dga->bytcnt_lo = (short) request->length;
	dga->bytcnt_hi = (short) (request->length >> 16);

	return;
}

/*****************************************************************
*
*	qdaint()... ADDER interrupt service
*
*****************************************************************/

qdaint(qd)
register int qd;
{
	register struct adder *adder;
	struct color_buf *cbuf;

	short stat;
	int i;
	register struct rgb *rgbp;
	register short *red;
	register short *green;
	register short *blue;

	spl4(); 			/* allow interval timer in */

	adder = (struct adder *) qdmap[qd].adder;

/*------------------------------------------------------------------------
* service the vertical blank interrupt (VSYNC bit) by loading any pending
* color map load request  */

	if (adder->status & VSYNC) {
	    adder->status &= ~VSYNC;		/* clear the interrupt */

	    cbuf = color_buf[qd];
	    if (cbuf->status & LOAD_COLOR_MAP) {

		red = (short *) qdmap[qd].red;
		green = (short *) qdmap[qd].green;
		blue = (short *) qdmap[qd].blue;

		for (i = cbuf->count, rgbp = cbuf->rgb; --i >= 0; rgbp++) {

		    red[rgbp->offset] = (short) rgbp->red;
		    green[rgbp->offset] = (short) rgbp->green;
		    blue[rgbp->offset] = (short) rgbp->blue;
		}

		cbuf->status &= ~LOAD_COLOR_MAP;
	    }
	}

/*-------------------------------------------------
* service the scroll interrupt (FRAME_SYNC bit) */

	if (adder->status & FRAME_SYNC) {
	    adder->status &= ~FRAME_SYNC;	/* clear the interrupt */

	    if (scroll[qd]->status & LOAD_REGS) {

		for ( i = 1000, adder->status = 0
		    ; i > 0  &&  !((stat = adder->status) & ID_SCROLL_READY)
		    ; --i);

		if (i == 0) {
		    mprintf("\nqd%d: qdaint: timeout on ID_SCROLL_READY", qd);
		    return;
		}

		adder->ID_scroll_data = scroll[qd]->viper_constant;
		adder->ID_scroll_command = ID_LOAD | SCROLL_CONSTANT;

		adder->y_scroll_constant = scroll[qd]->y_scroll_constant;
		adder->y_offset_pending = scroll[qd]->y_offset;

		if (scroll[qd]->status & LOAD_INDEX) {

		    adder->x_index_pending = scroll[qd]->x_index_pending;
		    adder->y_index_pending = scroll[qd]->y_index_pending;
		}

	    scroll[qd]->status = 0x00;
	    }
	}
}

/*****************************************************************
*
*	qdiint()... DUART input interrupt service routine
*
*****************************************************************/

qdiint(qd)
register int qd;
{
	register struct _vs_event *event;
	register struct qdinput *eqh;

	struct dga *dga;
	struct duart *duart;
	struct mouse_report *new_rep;

	struct uba_device *ui;
	struct tty *tp;

	u_short chr;
	int i,j;
	int k,l;

	u_short status;
	u_short data;
	u_short key;

	char do_wakeup = 0;		/* flag to do a select wakeup call */
	char a, b, c;			/* mouse button test variables */

	spl4(); 			/* allow interval timer in */

	eqh = eq_header[qd];		/* optimized as a register */
	new_rep = &current_rep[qd];
	duart = (struct duart *) qdmap[qd].duart;

/*-----------------------------------------
* if the graphic device is turned on..	*/

	if (qdflags[qd].inuse & GRAPHIC_DEV) {

	    /*---------------
	    * empty DUART */

	    while ((status = duart->statusA) & RCV_RDY	||
		   (status = duart->statusB) & RCV_RDY) {

		/*---------------------------------
		* pick up LK-201 input (if any) */

		if ((status = duart->statusA) & RCV_RDY) {

		    /* if error condition, then reset it */

		    if ((status = duart->statusA) & 0x70) {
			duart->cmdA = 0x40;
			continue;
		    }

		    /* event queue full now? (overflow condition) */

		    if (ISFULL(eqh) == TRUE) {
/*			mprintf("\nqd%d: qdiint: event queue overflow", qd);*/
			break;
		    }

		    /*--------------------------------------
		    * Check for various keyboard errors  */

		    key = duart->dataA & 0xFF;

		    if( key == LK_POWER_ERROR || key == LK_KDOWN_ERROR ||
			key == LK_INPUT_ERROR || key == LK_OUTPUT_ERROR) {
			mprintf("\nqd%d: qdiint: keyboard error, code = %x",qd,key);
			return(0);
		    }

		    if (key < LK_LOWEST)
			return(0);

		    ++do_wakeup;	/* request a select wakeup call */

		    event = PUTBEGIN(eqh);
		    PUTEND(eqh);

		    event->vse_key = key;
		    event->vse_key &= 0x00FF;
		    event->vse_x = eqh->curs_pos.x;
		    event->vse_y = eqh->curs_pos.y;
		    event->vse_time = TOY;
		    event->vse_type = VSE_BUTTON;
		    event->vse_direction = VSE_KBTRAW;
		    event->vse_device = VSE_DKB;
		}

		/*-------------------------------------
		* pick up the mouse input (if any)  */

		if ((status = duart->statusB) & RCV_RDY  &&
		     qdflags[qd].pntr_id == MOUSE_ID) {

		    if (status & 0x70) {
			duart->cmdB = 0x40;
			continue;
		    }

		    /* event queue full now? (overflow condition) */

		    if (ISFULL(eqh) == TRUE) {
/*			mprintf("\nqd%d: qdiint: event queue overflow", qd);*/
			break;
		    }

		    data = duart->dataB;      /* get report byte */
		    ++new_rep->bytcnt;	      /* bump report byte count */

		    /*---------------------------
		    * if 1st byte of report.. */

		    if ( data & START_FRAME) {
			new_rep->state = data;
			if (new_rep->bytcnt > 1) {
			    new_rep->bytcnt = 1;    /* start of new frame */
			    continue;		    /* ..continue looking */
			}
		    }

		    /*---------------------------
		    * if 2nd byte of report.. */

		    else if (new_rep->bytcnt == 2) {
			new_rep->dx = data & 0x00FF;
		    }

		    /*-------------------------------------------------
		    * if 3rd byte of report, load input event queue */

		    else if (new_rep->bytcnt == 3) {

			new_rep->dy = data & 0x00FF;
			new_rep->bytcnt = 0;

			/*-----------------------------------
			* if mouse position has changed.. */

			if (new_rep->dx != 0  ||  new_rep->dy != 0) {

			    /*---------------------------------------------
			    * calculate acceleration factor, if needed	*/

			    if (qdflags[qd].curs_acc > ACC_OFF) {

				if (qdflags[qd].curs_thr <= new_rep->dx)
				    new_rep->dx +=
					(new_rep->dx - qdflags[qd].curs_thr)
					 * qdflags[qd].curs_acc;

				if (qdflags[qd].curs_thr <= new_rep->dy)
				    new_rep->dy +=
					(new_rep->dy - qdflags[qd].curs_thr)
					 * qdflags[qd].curs_acc;
			    }

			    /*-------------------------------------
			    * update cursor position coordinates */

			    if (new_rep->state & X_SIGN) {
				eqh->curs_pos.x += new_rep->dx;
				if (eqh->curs_pos.x > 1023)
				    eqh->curs_pos.x = 1023;
			    }
			    else {
				eqh->curs_pos.x -= new_rep->dx;
				if (eqh->curs_pos.x < -15)
				    eqh->curs_pos.x = -15;
			    }

			    if (new_rep->state & Y_SIGN) {
				 eqh->curs_pos.y -= new_rep->dy;
				 if (eqh->curs_pos.y < -15)
				     eqh->curs_pos.y = -15;
			    }
			    else {
				eqh->curs_pos.y += new_rep->dy;
				if (eqh->curs_pos.y > 863)
				    eqh->curs_pos.y = 863;
			    }

			    /*---------------------------------
			    * update cursor screen position */

			    dga = (struct dga *) qdmap[qd].dga;
			    dga->x_cursor = TRANX(eqh->curs_pos.x);
			    dga->y_cursor = TRANY(eqh->curs_pos.y);

			    /*--------------------------------------------
			    * if cursor is in the box, no event report */

			    if (eqh->curs_pos.x <= eqh->curs_box.right	&&
				eqh->curs_pos.x >= eqh->curs_box.left  &&
				eqh->curs_pos.y >= eqh->curs_box.top  &&
				eqh->curs_pos.y <= eqh->curs_box.bottom ) {
				    goto GET_MBUTTON;
			    }

			    /*---------------------------------
			    * report the mouse motion event */

			    event = PUTBEGIN(eqh);
			    PUTEND(eqh);

			    ++do_wakeup;   /* request a select wakeup call */

			    event->vse_x = eqh->curs_pos.x;
			    event->vse_y = eqh->curs_pos.y;

			    event->vse_device = VSE_MOUSE;  /* mouse */
			    event->vse_type = VSE_MMOTION;  /* pos changed */
			    event->vse_key = 0;
			    event->vse_direction = 0;
			    event->vse_time = TOY;	/* time stamp */
			}

GET_MBUTTON:
			/*-------------------------------
			* if button state has changed */

			a = new_rep->state & 0x07;    /*mask nonbutton bits */
			b = last_rep[qd].state & 0x07;

			if (a ^ b) {

			    for ( c = 1;  c < 8; c <<= 1) {

				if (!( c & (a ^ b))) /* this button change? */
				    continue;

				/* event queue full? (overflow condition) */

				if (ISFULL(eqh) == TRUE) {
/*				    mprintf("\nqd%d: qdiint: event queue overflow", qd);*/
				    break;
				}

				event = PUTBEGIN(eqh);	/* get new event */
				PUTEND(eqh);

				++do_wakeup;   /* request select wakeup */

				event->vse_x = eqh->curs_pos.x;
				event->vse_y = eqh->curs_pos.y;

				event->vse_device = VSE_MOUSE;	/* mouse */
				event->vse_type = VSE_BUTTON; /* new button */
				event->vse_time = TOY;	      /* time stamp */

				/* flag changed button and if up or down */

				if (c == RIGHT_BUTTON)
				    event->vse_key = VSE_RIGHT_BUTTON;
				else if (c == MIDDLE_BUTTON)
				    event->vse_key = VSE_MIDDLE_BUTTON;
				else if (c == LEFT_BUTTON)
				    event->vse_key = VSE_LEFT_BUTTON;

				/* set bit = button depressed */

				if (c & a)
				    event->vse_direction = VSE_KBTDOWN;
				else
				    event->vse_direction = VSE_KBTUP;
			    }
			}

			/* refresh last report */

			last_rep[qd] = current_rep[qd];

		    }  /* get last byte of report */
		} /* pickup mouse input */

		/*--------------------------------
		* pickup tablet input, if any  */

		else if ((status = duart->statusB) & RCV_RDY  &&
			 qdflags[qd].pntr_id == TABLET_ID) {

		    if (status & 0x70) {
			duart->cmdB = 0x40;
			continue;
		    }

		    /* event queue full now? (overflow condition) */

		    if (ISFULL(eqh) == TRUE) {
/*			mprintf("\nqd%d: qdiint: event queue overflow", qd);*/
			break;
		    }

		    data = duart->dataB;      /* get report byte */
		    ++new_rep->bytcnt;	      /* bump report byte count */

		    /*---------------------------
		    * if 1st byte of report.. */

		    if (data & START_FRAME) {
			new_rep->state = data;
			if (new_rep->bytcnt > 1) {
			    new_rep->bytcnt = 1;    /* start of new frame */
			    continue;		    /* ..continue looking */
			}
		    }

		    /*---------------------------
		    * if 2nd byte of report.. */

		    else if (new_rep->bytcnt == 2) {
			new_rep->dx = data & 0x3F;
		    }

		    /*---------------------------
		    * if 3rd byte of report.. */

		    else if (new_rep->bytcnt == 3) {
			new_rep->dx |= (data & 0x3F) << 6;
		    }

		    /*---------------------------
		    * if 4th byte of report.. */

		    else if (new_rep->bytcnt == 4) {
			new_rep->dy = data & 0x3F;
		    }

		    /*-------------------------------------------------
		    * if 5th byte of report, load input event queue */

		    else if (new_rep->bytcnt == 5) {

			new_rep->dy |= (data & 0x3F) << 6;
			new_rep->bytcnt = 0;

			/*-------------------------------------
			* update cursor position coordinates */

			new_rep->dx /= qdflags[qd].tab_res;
			new_rep->dy = (2200 - new_rep->dy)
				      / qdflags[qd].tab_res;

			if (new_rep->dx > 1023) {
			    new_rep->dx = 1023;
			}
			if (new_rep->dy > 863) {
			    new_rep->dy = 863;
			}

			/*
			 * report an event if the puck/stylus has moved
			 */

			if (eqh->curs_pos.x != new_rep->dx ||
			    eqh->curs_pos.y != new_rep->dy) {

			    eqh->curs_pos.x = new_rep->dx;
			    eqh->curs_pos.y = new_rep->dy;

			    /*---------------------------------
			    * update cursor screen position */

			    dga = (struct dga *) qdmap[qd].dga;
			    dga->x_cursor = TRANX(eqh->curs_pos.x);
			    dga->y_cursor = TRANY(eqh->curs_pos.y);

			    /*
			     * if cursor is in the box, no event report
			     */

			    if (eqh->curs_pos.x <= eqh->curs_box.right	&&
				eqh->curs_pos.x >= eqh->curs_box.left  &&
				eqh->curs_pos.y >= eqh->curs_box.top  &&
				eqh->curs_pos.y <= eqh->curs_box.bottom ) {
				goto GET_TBUTTON;
			    }

			    /*---------------------------------
			    * report the tablet motion event */

			    event = PUTBEGIN(eqh);
			    PUTEND(eqh);

			    ++do_wakeup;   /* request a select wakeup call */

			    event->vse_x = eqh->curs_pos.x;
			    event->vse_y = eqh->curs_pos.y;

			    event->vse_device = VSE_TABLET;  /* tablet */
			    /*
			     * right now, X handles tablet motion the same
			     * as mouse motion
			     */
			    event->vse_type = VSE_MMOTION;   /* pos changed */
			    event->vse_key = 0;
			    event->vse_direction = 0;
			    event->vse_time = TOY;	/* time stamp */
			}
GET_TBUTTON:
			/*-------------------------------
			* if button state has changed */

			a = new_rep->state & 0x1E;   /* mask nonbutton bits */
			b = last_rep[qd].state & 0x1E;

			if (a ^ b) {

			    /* event queue full now? (overflow condition) */

			    if (ISFULL(eqh) == TRUE) {
/*				mprintf("\nqd%d: qdiint: event queue overflow",qd);*/
				break;
			    }

			    event = PUTBEGIN(eqh);  /* get new event */
			    PUTEND(eqh);

			    ++do_wakeup;   /* request a select wakeup call */

			    event->vse_x = eqh->curs_pos.x;
			    event->vse_y = eqh->curs_pos.y;

			    event->vse_device = VSE_TABLET;  /* tablet */
			    event->vse_type = VSE_BUTTON; /* button changed */
			    event->vse_time = TOY;	   /* time stamp */

			    /* define the changed button and if up or down */

			    for ( c = 1;  c <= 0x10; c <<= 1) {
				if (c & (a ^ b)) {
				    if (c == T_LEFT_BUTTON)
					event->vse_key = VSE_T_LEFT_BUTTON;
				    else if (c == T_FRONT_BUTTON)
					event->vse_key = VSE_T_FRONT_BUTTON;
				    else if (c == T_RIGHT_BUTTON)
					event->vse_key = VSE_T_RIGHT_BUTTON;
				    else if (c == T_BACK_BUTTON)
					event->vse_key = VSE_T_BACK_BUTTON;
				    break;
				}
			    }

			    /* set bit = button depressed */

			    if (c & a)
				event->vse_direction = VSE_KBTDOWN;
			    else
				event->vse_direction = VSE_KBTUP;
			}

			/* refresh last report */

			last_rep[qd] = current_rep[qd];

		    } /* get last byte of report */
		} /* pick up tablet input */

	    } /* while input available.. */

	    /*---------------------
	    * do select wakeup	*/


	    if (rsel[qd] && do_wakeup && qdflags[qd].selmask & SEL_READ) {
		selwakeup(rsel[qd], 0);
		rsel[qd] = 0;
		qdflags[qd].selmask &= ~SEL_READ;
		do_wakeup = 0;
	    }
	}

/*-----------------------------------------------------------------
* if the graphic device is not turned on, this is console input */

	else {

	    ui = qdinfo[qd];
	    if (ui == 0 || ui->ui_alive == 0)
		return(0);

	    tp = &qd_tty[qd << 2];

	    /*--------------------------------------
	    * Get a character from the keyboard. */

	    while ((status = duart->statusA) & RCV_RDY) {

		key = duart->dataA;
		key &= 0xFF;

		/*--------------------------------------
		* Check for various keyboard errors  */

		if( key == LK_POWER_ERROR || key == LK_KDOWN_ERROR ||
		    key == LK_INPUT_ERROR || key == LK_OUTPUT_ERROR) {
			mprintf("\nqd%d: qdiint: Keyboard error, code = %x",qd,key);
			return(0);
		}

		if (key < LK_LOWEST)
		    return(0);

		/*---------------------------------
		* See if its a state change key */

		switch (key) {

		    case LOCK:
			q_keyboard.lock ^= 0xffff;	/* toggle */
			if (q_keyboard.lock)
			    led_control(qd, LK_LED_ENABLE, LK_LED_LOCK);
			else
			    led_control(qd, LK_LED_DISABLE, LK_LED_LOCK);
			return;

		    case SHIFT:
			q_keyboard.shift ^= 0xFFFF;
			return;

		    case CNTRL:
			q_keyboard.cntrl ^= 0xFFFF;
			return;

		    case ALLUP:
			q_keyboard.cntrl = 0;
			q_keyboard.shift = 0;
			return;

		    case REPEAT:
			chr = q_keyboard.last;
			break;

		    /*-------------------------------------------------------
		    * Test for cntrl characters. If set, see if the character
		    * is elligible to become a control character. */

		    default:

			if (q_keyboard.cntrl) {
				chr = q_key[key];
				if (chr >= ' ' && chr <= '~')
				    chr &= 0x1F;
				else if (chr >= 0xA1 && chr <= 0xFE)
				    chr &= 0x9F;
			}
			else if( q_keyboard.lock || q_keyboard.shift )
			    chr = q_shift_key[key];
			else
			    chr = q_key[key];
			break;
		}

		q_keyboard.last = chr;

		/*-----------------------------------
		* Check for special function keys */

		if (chr & 0x100) {
			char *string;
			string = q_special[chr & 0x7F];
			while(*string)
			    (*linesw[tp->t_line].l_rint)(*string++, tp);
		}
		else {
			if (tp->t_iflag & ISTRIP)	/* Strip to 7 bits. */
				c &= 0177;	
			else {			/* Take the full 8-bits */
				/* If ISTRIP is not set a valid character of 377
			 	* is read as 0377,0377 to avoid ambiguity with
			 	* the PARMARK sequence.
			 	*/ 
			        if ((c == 0377) && (tp->t_line == TERMIODISC) &&
			            (tp->t_iflag & PARMRK))
					(*linesw[tp->t_line].l_rint)(0377,tp);
			
			}
			(*linesw[tp->t_line].l_rint)(chr, tp);
		}
	    }
	}

/*----------------------
* cleanup and exit  */

	return(0);

} /* qdiint */

/******************************************************************
*******************************************************************
*******************************************************************
*
*	THE SUBROUTINES START HERE:
*
******************************************************************/

/*****************************************************************
*
*	clear_qd_screen()... clear the QDSS screen
*
******************************************************************
*
*			     >>> NOTE <<<
*
*   This code requires that certain adder initialization be valid.  To
*   assure that this requirement is satisfied, this routine should be
*   called only after calling the "setup_dragon()" function.
*
*   Clear the bitmap a piece at a time. Since the fast scroll clear
*   only clears the current displayed portion of the bitmap put a
*   temporary value in the y limit register so we can access whole
*   bitmap
*
****************/

clear_qd_screen(unit)
int unit;
{
	register struct adder *adder;
	adder = (struct adder *) qdmap[unit].adder;

	adder->x_limit = 1024;
	adder->y_limit = 2048 - CHAR_HEIGHT;
	adder->y_offset_pending = 0;

	wait_status(adder, VSYNC);	/* wait at LEAST 1 full frame */
	wait_status(adder, VSYNC);

	adder->y_scroll_constant = SCROLL_ERASE;

	wait_status(adder, VSYNC);
	wait_status(adder, VSYNC);

	adder->y_offset_pending = 864;

	wait_status(adder, VSYNC);
	wait_status(adder, VSYNC);

	adder->y_scroll_constant = SCROLL_ERASE;

	wait_status(adder, VSYNC);
	wait_status(adder, VSYNC);

	adder->y_offset_pending = 1728;

	wait_status(adder, VSYNC);
	wait_status(adder, VSYNC);

	adder->y_scroll_constant = SCROLL_ERASE;

	wait_status(adder, VSYNC);
	wait_status(adder, VSYNC);

	adder->y_offset_pending = 0;	 /* back to normal */

	wait_status(adder, VSYNC);
	wait_status(adder, VSYNC);

	adder->x_limit = MAX_SCREEN_X;
	adder->y_limit = MAX_SCREEN_Y + FONT_HEIGHT;

} /* clear_qd_screen */

/**********************************************************************
*
*	qdputc()... route kernel console output to display destination
*
***********************************************************************
*
*	calling convention:
*
*		qdputc(chr);
*
*	where:	char chr;	 ;character for output
*
****************/

qdputc(chr)
register char chr;
{
	register struct tty *tp0;
	struct uba_device *ui;

/*---------------------------------------------------------
* if system is now physical, forget it (ie: crash DUMP) */

	if ( (mfpr(MAPEN) & 1) == 0 ) {
	    return;
	}

	*(short *)qdmap[0].memcsr = SYNC_ON | UNBLANK; /* turn off leds and turn on video */

/*--------------------------------------------------
* direct kernel output char to the proper place  */

	tp0 = &qd_tty[1];

	if (qdflags[0].kernel_loop != 0  &&  tp0->t_state & TS_ISOPEN) {
	    (*linesw[tp0->t_line].l_rint)(chr, tp0);
	} else {
	    blitc(0, chr & 0xff);
	}

} /* qdputc */

/*******************************************************************
*
*	qdgetc()... get a character from the LK201
*
*******************************************************************
*
*	calling convention:
*
*		qdgetc();
*
*	returns:  the character read.
*
****************/

qdgetc()
{
	register short key;
	register char chr;
	register struct duart *duart;

	u_int status;

	duart = (struct duart *) qdmap[0].duart;

	/*--------------------------------------
	* Get a character from the keyboard. */

LOOP:
	while (!((status = duart->statusA) & RCV_RDY))
			;

	key = duart->dataA;
	key &= 0xFF;

	/*--------------------------------------
	* Check for various keyboard errors  */

	if( key == LK_POWER_ERROR || key == LK_KDOWN_ERROR ||
	    key == LK_INPUT_ERROR || key == LK_OUTPUT_ERROR) {
		printf("Keyboard error, code = %x\n", key);
		return(0);
	}

	if (key < LK_LOWEST)
	    return(0);

	/*---------------------------------
	* See if its a state change key */

	switch (key) {

	    case LOCK:
		q_keyboard.lock ^= 0xffff;	/* toggle */
		if (q_keyboard.lock)
		    led_control(LK_LED_ENABLE, LK_LED_LOCK);
		else
		    led_control(LK_LED_DISABLE, LK_LED_LOCK);
		goto LOOP;

	    case SHIFT:
		q_keyboard.shift ^= 0xFFFF;
		goto LOOP;

	    case CNTRL:
		q_keyboard.cntrl ^= 0xFFFF;
		goto LOOP;

	    case ALLUP:
		q_keyboard.cntrl = 0;
		q_keyboard.shift = 0;
		goto LOOP;

	    case REPEAT:
		chr = q_keyboard.last;
		break;

	    /*-------------------------------------------------------
	    * Test for cntrl characters. If set, see if the character
	    * is elligible to become a control character. */

	    default:

		if (q_keyboard.cntrl) {
		    chr = q_key[key];
		    if (chr >= ' ' && chr <= '~')
			chr &= 0x1F;
		}
		else if ( q_keyboard.lock || q_keyboard.shift )
		    chr = q_shift_key[key];
		else
		    chr = q_key[key];
		break;
	}

	if (chr < ' ' && chr > '~')	/* if input is non-displayable */
	    return(0);			/* ..then pitch it! */

	q_keyboard.last = chr;

	/*-----------------------------------
	* Check for special function keys */

	if (chr & 0x80) 		/* pitch the function keys */
	    return(0);
	else
	    return(chr);

} /* qdgetc */

/**********************************************************************
*
*	ldcursor()... load the mouse cursor's template RAM bitmap
*
*********************************************************************
*
*	calling convention:
*
*		ldcursor(unit, bitmap);
*		u_int unit;
*		short *bitmap;
*
****************/

ldcursor(unit, bitmap)
u_int unit;
short *bitmap;
{
	register struct dga *dga;
	register short *temp;
	register int i;

	int cursor;

	dga = (struct dga *) qdmap[unit].dga;
	temp = (short *) qdmap[unit].template;

	if (dga->csr & CURS_ENB) {	/* if the cursor is enabled.. */
	    cursor = -1;		/* ..note that.. */
	    dga->csr &= ~CURS_ENB;	/* ..and shut it off */
	}
	else {
	    cursor = 0;
	}

	dga->csr &= ~CURS_ENB;		/* shut off the cursor */

	temp += (8 * 1024) - 32;	/* cursor is 32 WORDS from the end */
					/* ..of the 8k WORD template space */
	for (i = 0; i < 32; ++i)
	    *temp++ = *bitmap++;

	if (cursor) {			/* if cursor was enabled.. */
	    dga->csr |= CURS_ENB;	/* ..turn it back on */
	}

	return(0);

} /* ldcursor */

/**********************************************************************
*
*	ldfont()... put the console font in the QDSS off-screen memory
*
***********************************************************************
*
*	calling convention:
*
*		ldfont(unit);
*		u_int unit;	;QDSS unit number
*
****************/

ldfont(unit)
u_int unit;
{
	register struct adder *adder;

	int i;		/* scratch variables */
	int j;
	int k;
	short packed;
	int max_chars_line;

	adder = (struct adder *) qdmap[unit].adder;

/*------------------------------------------
* setup VIPER operand control registers  */

	write_ID(adder, MASK_1, 0xFFFF);
	write_ID(adder, VIPER_Z_LOAD | FOREGROUND_COLOR_Z, 255);
	write_ID(adder, VIPER_Z_LOAD | BACKGROUND_COLOR_Z, 0);

	write_ID(adder, SRC1_OCR_B,
			EXT_NONE | INT_NONE | ID | BAR_SHIFT_DELAY);
	write_ID(adder, SRC2_OCR_B,
			EXT_NONE | INT_NONE | ID | BAR_SHIFT_DELAY);
	write_ID(adder, DST_OCR_B,
			EXT_SOURCE | INT_NONE | NO_ID | NO_BAR_SHIFT_DELAY);

	adder->rasterop_mode = DST_WRITE_ENABLE | DST_INDEX_ENABLE | NORMAL;

/*--------------------------
* load destination data  */

	wait_status(adder, RASTEROP_COMPLETE);

	adder->destination_x = FONT_X;
	adder->destination_y = FONT_Y;
	if (FONT_WIDTH > MAX_SCREEN_X)
		adder->fast_dest_dx = MAX_SCREEN_X;
	else
		adder->fast_dest_dx = FONT_WIDTH;
	adder->slow_dest_dy = CHAR_HEIGHT;

/*---------------------------------------
* setup for processor to bitmap xfer  */

	write_ID(adder, CS_UPDATE_MASK, 0x0001);
	adder->cmd = PBT | OCRB | 2 | DTE | 2;

/*---------------------------------------
* Figure out how many characters can be stored on one "line" of offscreen memory
*/
	    max_chars_line = MAX_SCREEN_X/(CHAR_WIDTH*2);
	    if ((CHARS/2 + CHARS%2) < max_chars_line)
		max_chars_line = CHARS/2 + CHARS%2;

/*-----------------------------------------------
* iteratively do the processor to bitmap xfer */

	for (i = 0; i < ROWS; ++i) {

	    /* PTOB a scan line */

	    for (j = 0, k = i; j < max_chars_line; ++j) {
		/* PTOB one scan of a char cell */

		packed = q_font[k];
		k += ROWS;
		packed |= ((short)q_font[k] << 8);
		k += ROWS;

		wait_status(adder, TX_READY);
		adder->id_data = packed;
	    }
	}

/*---------------------------------------
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


/*-----------------------------------------------*/

	adder->destination_x = FONT_X;
	adder->destination_y = FONT_Y - CHAR_HEIGHT;
	adder->fast_dest_dx = max_chars_line * CHAR_WIDTH * 2;
	adder->slow_dest_dy = CHAR_HEIGHT;

/*---------------------------------------
* setup for processor to bitmap xfer  */

	write_ID(adder, CS_UPDATE_MASK, 0x0001);
	adder->cmd = PBT | OCRB | 2 | DTE | 2;

/*-----------------------------------------------
* iteratively do the processor to bitmap xfer */

	for (i = 0; i < ROWS; ++i) {

	    /* PTOB a scan line */

	    for (j = 0, k = i; j < max_chars_line; ++j) {

		/* PTOB one scan of a char cell */

		packed = q_font[k + FONT_OFFSET];
		k += ROWS;
		packed |= ((short)q_font[k + FONT_OFFSET] << 8);
		k += ROWS;

		wait_status(adder, TX_READY);
		adder->id_data = packed;
	    }
	}

}  /* ldfont */

/*********************************************************************
*
*	led_control()... twiddle LK-201 LED's
*
**********************************************************************
*
*	led_control(unit, cmd, led_mask);
*	u_int unit;	QDSS number
*	int cmd;	LED enable/disable command
*	int led_mask;	which LED(s) to twiddle
*
*************/

led_control(unit, cmd, led_mask)
u_int unit;
int cmd;
int led_mask;
{
	register int i;
	register int status;
	register struct duart *duart;

	duart = (struct duart *) qdmap[unit].duart;

	for (i = 1000; i > 0; --i) {
	    if ((status = duart->statusA) & XMT_RDY) {
		duart->dataA = cmd;
		break;
	    }
	}

	for (i = 1000; i > 0; --i) {
	    if ((status = duart->statusA) & XMT_RDY) {
		duart->dataA = led_mask;
		break;
	    }
	}

	if (i == 0)
	    return(BAD);

	return(GOOD);

} /* led_control */

/*******************************************************************
*
*	scroll_up()... move the screen up one character height
*
********************************************************************
*
*	calling convention:
*
*		scroll_up(adder);
*		struct adder *adder;	;address of adder
*
********/

scroll_up(adder)
register struct adder *adder;
{

/*------------------------------------------
* setup VIPER operand control registers  */

	wait_status(adder, ADDRESS_COMPLETE);

	write_ID(adder, CS_UPDATE_MASK, 0x00FF);  /* select all planes */

	write_ID(adder, MASK_1, 0xFFFF);
	write_ID(adder, VIPER_Z_LOAD | FOREGROUND_COLOR_Z, 255);
	write_ID(adder, VIPER_Z_LOAD | BACKGROUND_COLOR_Z, 0);

	write_ID(adder, SRC1_OCR_B,
			EXT_NONE | INT_SOURCE | ID | BAR_SHIFT_DELAY);
	write_ID(adder, DST_OCR_B,
			EXT_NONE | INT_NONE | NO_ID | NO_BAR_SHIFT_DELAY);

/*----------------------------------------
* load DESTINATION origin and vectors  */

	adder->fast_dest_dy = 0;
	adder->slow_dest_dx = 0;
	adder->error_1 = 0;
	adder->error_2 = 0;

	adder->rasterop_mode = DST_WRITE_ENABLE | NORMAL;

	adder->destination_x = 0;
	adder->fast_dest_dx = 1024;

	adder->destination_y = 0;
	adder->slow_dest_dy = 864 - CHAR_HEIGHT;

/*-----------------------------------
* load SOURCE origin and vectors  */

	adder->source_1_x = 0;
	adder->source_1_dx = 1024;

	adder->source_1_y = 0 + CHAR_HEIGHT;
	adder->source_1_dy = 864 - CHAR_HEIGHT;

	write_ID(adder, LU_FUNCTION_R1, FULL_SRC_RESOLUTION | LF_SOURCE);
	adder->cmd = RASTEROP | OCRB | 0 | S1E | DTE;

/*--------------------------------------------
* do a rectangle clear of last screen line */

	write_ID(adder, MASK_1, 0xffff);
	write_ID(adder, SOURCE, 0xffff);
	write_ID(adder,DST_OCR_B,
		(EXT_NONE | INT_NONE | NO_ID | NO_BAR_SHIFT_DELAY));
	write_ID(adder, VIPER_Z_LOAD | FOREGROUND_COLOR_Z, 0);
	adder->error_1 = 0;
	adder->error_2 = 0;
	adder->slow_dest_dx = 0;	/* set up the width of	*/
	adder->slow_dest_dy = CHAR_HEIGHT;	/* rectangle */

	adder->rasterop_mode = (NORMAL | DST_WRITE_ENABLE) ;
	wait_status(adder, RASTEROP_COMPLETE);
	adder->destination_x = 0;
	adder->destination_y = 864 - CHAR_HEIGHT;

	adder->fast_dest_dx = 1024;	/* set up the height	*/
	adder->fast_dest_dy = 0;	/* of rectangle 	*/

	write_ID(adder, LU_FUNCTION_R2, (FULL_SRC_RESOLUTION | LF_SOURCE));
	adder->cmd = (RASTEROP | OCRB | LF_R2 | DTE ) ;

} /* scroll_up */

/********************************************************************
*
*	init_shared()... init shared memory pointers and structures
*
*********************************************************************
*
*	calling convention:
*
*		init_shared(unit);
*		u_int unit;
*
****************/

init_shared(unit)
register u_int unit;
{
	register struct dga *dga;

	dga = (struct dga *) qdmap[unit].dga;

/*--------------------------------------------------
* initialize the event queue pointers and header */

	eq_header[unit] = (struct qdinput *)
			  ((((int)event_shared & ~(0x01FF)) + 512)
			   + (EVENT_BUFSIZE * unit));

	eq_header[unit]->curs_pos.x = 0;
	eq_header[unit]->curs_pos.y = 0;

	dga->x_cursor = TRANX(eq_header[unit]->curs_pos.x);
	dga->y_cursor = TRANY(eq_header[unit]->curs_pos.y);

	eq_header[unit]->curs_box.left = 0;
	eq_header[unit]->curs_box.right = 0;
	eq_header[unit]->curs_box.top = 0;
	eq_header[unit]->curs_box.bottom = 0;

/*---------------------------------------------------------
* assign a pointer to the DMA I/O buffer for this QDSS. */

	DMAheader[unit] = (struct DMAreq_header *)
			  (((int)(&DMA_shared[0] + 512) & ~0x1FF)
			   + (DMAbuf_size * unit));

	DMAheader[unit]->DMAreq = (struct DMAreq *) ((int)DMAheader[unit]
				  + sizeof(struct DMAreq_header));

	DMAheader[unit]->QBAreg = 0;
	DMAheader[unit]->status = 0;
	DMAheader[unit]->shared_size = DMAbuf_size;
	DMAheader[unit]->used = 0;
	DMAheader[unit]->size = 10;	/* default = 10 requests */
	DMAheader[unit]->oldest = 0;
	DMAheader[unit]->newest = 0;

/*-----------------------------------------------------------
* assign a pointer to the scroll structure for this QDSS. */

	scroll[unit] = (struct scroll *)
			 (((int)(&scroll_shared[0] + 512) & ~0x1FF)
			   + (sizeof(struct scroll) * unit));

	scroll[unit]->status = 0;
	scroll[unit]->viper_constant = 0;
	scroll[unit]->y_scroll_constant = 0;
	scroll[unit]->y_offset = 0;
	scroll[unit]->x_index_pending = 0;
	scroll[unit]->y_index_pending = 0;

/*----------------------------------------------------------------
* assign a pointer to the color map write buffer for this QDSS */

	color_buf[unit] = (struct color_buf *)
			   (((int)(&color_shared[0] + 512) & ~0x1FF)
			    + (COLOR_BUFSIZ * unit));

	color_buf[unit]->status = 0;
	color_buf[unit]->count = 0;

} /* init_shared */

/*********************************************************************
*
*	setup_dragon()... init the ADDER, VIPER, bitmaps, & color map
*
**********************************************************************
*
*	calling convention:
*
*		setup_dragon();
*
*	return: NONE
*
************************/

setup_dragon(unit)
u_int unit;
{

	register struct adder *adder;
	register struct dga *dga;
	short *memcsr;

	int i;			/* general purpose variables */
	int status;

	short top;		/* clipping/scrolling boundaries */
	short bottom;
	short right;
	short left;

	short *red;		/* color map pointers */
	short *green;
	short *blue;

/*------------------
* init for setup */

	adder = (struct adder *) qdmap[unit].adder;
	dga = (struct dga *) qdmap[unit].dga;
	memcsr = (short *) qdmap[unit].memcsr;

	dga->csr &= ~(DMA_IE | 0x700);	/* halt DMA and kill the intrpts */
	*memcsr = SYNC_ON;		/* blank screen and turn off LED's */
	adder->command = CANCEL;

/*----------------------
* set monitor timing */

	adder->x_scan_count_0 = 0x2800;
	adder->x_scan_count_1 = 0x1020;
	adder->x_scan_count_2 = 0x003A;
	adder->x_scan_count_3 = 0x38F0;
	adder->x_scan_count_4 = 0x6128;
	adder->x_scan_count_5 = 0x093A;
	adder->x_scan_count_6 = 0x313C;
	adder->sync_phase_adj = 0x0100;
	adder->x_scan_conf = 0x00C8;

/*---------------------------------------------------------
* got a bug in secound pass ADDER! lets take care of it */

	/* normally, just use the code in the following bug fix code, but to
	* make repeated demos look pretty, load the registers as if there was
	* no bug and then test to see if we are getting sync */

	adder->y_scan_count_0 = 0x135F;
	adder->y_scan_count_1 = 0x3363;
	adder->y_scan_count_2 = 0x2366;
	adder->y_scan_count_3 = 0x0388;

	/* if no sync, do the bug fix code */

	if (wait_status(adder, VSYNC) == BAD) {

	    /* first load all Y scan registers with very short frame and
	    * wait for scroll service.	This guarantees at least one SYNC
	    * to fix the pass 2 Adder initialization bug (synchronizes
	    * XCINCH with DMSEEDH) */

	    adder->y_scan_count_0 = 0x01;
	    adder->y_scan_count_1 = 0x01;
	    adder->y_scan_count_2 = 0x01;
	    adder->y_scan_count_3 = 0x01;

	    wait_status(adder, VSYNC);	/* delay at least 1 full frame time */
	    wait_status(adder, VSYNC);

	    /* now load the REAL sync values (in reverse order just to
	    *  be safe.  */

	    adder->y_scan_count_3 = 0x0388;
	    adder->y_scan_count_2 = 0x2366;
	    adder->y_scan_count_1 = 0x3363;
	    adder->y_scan_count_0 = 0x135F;
	}

	*memcsr = SYNC_ON | UNBLANK;	/* turn off leds and turn on video */

/*----------------------------
* zero the index registers */

	adder->x_index_pending = 0;
	adder->y_index_pending = 0;
	adder->x_index_new = 0;
	adder->y_index_new = 0;
	adder->x_index_old = 0;
	adder->y_index_old = 0;

	adder->pause = 0;

/*----------------------------------------
* set rasterop mode to normal pen down */

	adder->rasterop_mode = DST_WRITE_ENABLE | DST_INDEX_ENABLE | NORMAL;

/*--------------------------------------------------
* set the rasterop registers to a default values */

	adder->source_1_dx = 1;
	adder->source_1_dy = 1;
	adder->source_1_x = 0;
	adder->source_1_y = 0;
	adder->destination_x = 0;
	adder->destination_y = 0;
	adder->fast_dest_dx = 1;
	adder->fast_dest_dy = 0;
	adder->slow_dest_dx = 0;
	adder->slow_dest_dy = 1;
	adder->error_1 = 0;
	adder->error_2 = 0;

/*------------------------
* scale factor = unity */

	adder->fast_scale = UNITY;
	adder->slow_scale = UNITY;

/*-------------------------------
* set the source 2 parameters */

	adder->source_2_x = 0;
	adder->source_2_y = 0;
	adder->source_2_size = 0x0022;

/*-----------------------------------------------
* initialize plane addresses for eight vipers */

	write_ID(adder, CS_UPDATE_MASK, 0x0001);
	write_ID(adder, PLANE_ADDRESS, 0x0000);

	write_ID(adder, CS_UPDATE_MASK, 0x0002);
	write_ID(adder, PLANE_ADDRESS, 0x0001);

	write_ID(adder, CS_UPDATE_MASK, 0x0004);
	write_ID(adder, PLANE_ADDRESS, 0x0002);

	write_ID(adder, CS_UPDATE_MASK, 0x0008);
	write_ID(adder, PLANE_ADDRESS, 0x0003);

	write_ID(adder, CS_UPDATE_MASK, 0x0010);
	write_ID(adder, PLANE_ADDRESS, 0x0004);

	write_ID(adder, CS_UPDATE_MASK, 0x0020);
	write_ID(adder, PLANE_ADDRESS, 0x0005);

	write_ID(adder, CS_UPDATE_MASK, 0x0040);
	write_ID(adder, PLANE_ADDRESS, 0x0006);

	write_ID(adder, CS_UPDATE_MASK, 0x0080);
	write_ID(adder, PLANE_ADDRESS, 0x0007);

	/* initialize the external registers. */

	write_ID(adder, CS_UPDATE_MASK, 0x00FF);
	write_ID(adder, CS_SCROLL_MASK, 0x00FF);

	/* initialize resolution mode */

	write_ID(adder, MEMORY_BUS_WIDTH, 0x000C);     /* bus width = 16 */
	write_ID(adder, RESOLUTION_MODE, 0x0000);      /* one bit/pixel */

	/* initialize viper registers */

	write_ID(adder, SCROLL_CONSTANT, SCROLL_ENABLE|VIPER_LEFT|VIPER_UP);
	write_ID(adder, SCROLL_FILL, 0x0000);

/*----------------------------------------------------
* set clipping and scrolling limits to full screen */

	for ( i = 1000, adder->status = 0
	    ; i > 0  &&  !((status = adder->status) & ADDRESS_COMPLETE)
	    ; --i);

	if (i == 0)
	    mprintf("\nqd%d: setup_dragon: timeout on ADDRESS_COMPLETE",unit);

	top = 0;
	bottom = 2048;
	left = 0;
	right = 1024;

	adder->x_clip_min = left;
	adder->x_clip_max = right;
	adder->y_clip_min = top;
	adder->y_clip_max = bottom;

	adder->scroll_x_min = left;
	adder->scroll_x_max = right;
	adder->scroll_y_min = top;
	adder->scroll_y_max = bottom;

	wait_status(adder, VSYNC);	/* wait at LEAST 1 full frame */
	wait_status(adder, VSYNC);

	adder->x_index_pending = left;
	adder->y_index_pending = top;
	adder->x_index_new = left;
	adder->y_index_new = top;
	adder->x_index_old = left;
	adder->y_index_old = top;

	for ( i = 1000, adder->status = 0
	    ; i > 0  &&  !((status = adder->status) & ADDRESS_COMPLETE)
	    ; --i);

	if (i == 0)
	    mprintf("\nqd%d: setup_dragon: timeout on ADDRESS_COMPLETE",unit);

	write_ID(adder, LEFT_SCROLL_MASK, 0x0000);
	write_ID(adder, RIGHT_SCROLL_MASK, 0x0000);

/*------------------------------------------------------------
* set source and the mask register to all ones (ie: white) */

	write_ID(adder, SOURCE, 0xFFFF);
	write_ID(adder, MASK_1, 0xFFFF);
	write_ID(adder, VIPER_Z_LOAD | FOREGROUND_COLOR_Z, 255);
	write_ID(adder, VIPER_Z_LOAD | BACKGROUND_COLOR_Z, 0);

/*--------------------------------------------------------------
* initialize Operand Control Register banks for fill command */

	write_ID(adder, SRC1_OCR_A, EXT_NONE | INT_M1_M2  | NO_ID | WAIT);
	write_ID(adder, SRC2_OCR_A, EXT_NONE | INT_SOURCE | NO_ID | NO_WAIT);
	write_ID(adder, DST_OCR_A, EXT_NONE | INT_NONE	 | NO_ID | NO_WAIT);

	write_ID(adder, SRC1_OCR_B, EXT_NONE | INT_SOURCE | NO_ID | WAIT);
	write_ID(adder, SRC2_OCR_B, EXT_NONE | INT_M1_M2  | NO_ID | NO_WAIT);
	write_ID(adder, DST_OCR_B, EXT_NONE | INT_NONE | NO_ID | NO_WAIT);

/*------------------------------------------------------------------
* init Logic Unit Function registers, (these are just common values,
* and may be changed as required).  */

	write_ID(adder, LU_FUNCTION_R1, FULL_SRC_RESOLUTION | LF_SOURCE);
	write_ID(adder, LU_FUNCTION_R2, FULL_SRC_RESOLUTION | LF_SOURCE | INV_M1_M2);
	write_ID(adder, LU_FUNCTION_R3, FULL_SRC_RESOLUTION | LF_D_OR_S);
	write_ID(adder, LU_FUNCTION_R4, FULL_SRC_RESOLUTION | LF_D_XOR_S);

/*----------------------------------------
* load the color map for black & white */

	for ( i = 0, adder->status = 0
	    ; i < 10000  &&  !((status = adder->status) & VSYNC)
	    ; ++i);

	if (i == 0)
	    mprintf("\nqd%d: setup_dragon: timeout on VSYNC", unit);

	red = (short *) qdmap[unit].red;
	green = (short *) qdmap[unit].green;
	blue = (short *) qdmap[unit].blue;

	*red++ = 0x00;			/* black */
	*green++ = 0x00;
	*blue++ = 0x00;

	*red-- = 0xFF;			/* white */
	*green-- = 0xFF;
	*blue-- = 0xFF;

	/*----------------------------------
	* set color map for mouse cursor */

	red += 254;
	green += 254;
	blue += 254;

	*red++ = 0x00;			/* black */
	*green++ = 0x00;
	*blue++ = 0x00;

	*red = 0xFF;			/* white */
	*green = 0xFF;
	*blue = 0xFF;

	return(0);

} /* setup_dragon */

/******************************************************************
*
*	setup_input()... init the DUART and set defaults in input
*			 devices
*
*******************************************************************
*
*	calling convention:
*
*		setup_input(unit);
*
*	where: unit - is the QDSS unit number to be setup
*
*********/

setup_input(unit)
u_int unit;
{
	register struct duart *duart;	/* DUART register structure pointer */
	register int i; 		/* scratch variable */
	register int bits;

	char id_byte;
	short status;

/*---------------
* init stuff */

	duart = (struct duart *) qdmap[unit].duart;
	duart->imask = 0;

/*---------------------------------------------
* setup the DUART for kbd & pointing device */

	duart->cmdA = RESET_M;	  /* reset mode reg ptr for kbd */
	duart->modeA = 0x13;	  /* 8 bits, no parity, rcv IE, */
				  /* no RTS control,char error mode */
	duart->modeA = 0x07;	  /* 1 stop bit,CTS does not IE XMT */
				  /* no RTS control,no echo or loop */
	duart->cmdB = RESET_M;	  /* reset mode reg pntr for host */
	duart->modeB = 0x07;	  /* 8 bits, odd parity, rcv IE.. */
				  /* ..no RTS cntrl, char error mode */
	duart->modeB = 0x07;	  /* 1 stop bit,CTS does not IE XMT */
				  /* no RTS control,no echo or loop */

	duart->auxctl = 0x00;	  /* baud rate set 1 */

	duart->clkselA = 0x99;	  /* 4800 baud for kbd */
	duart->clkselB = 0x99;	  /* 4800 baud for mouse */

	/* reset everything for keyboard */

	for (bits = RESET_M; bits < START_BREAK; bits += 0x10)
	    duart->cmdA = bits;

	/* reset everything for host */

	for (bits = RESET_M; bits < START_BREAK; bits += 0x10)
	     duart->cmdB = bits;

	duart->cmdA = EN_RCV | EN_XMT; /* enbl xmt & rcv for kbd */
	duart->cmdB = EN_RCV | EN_XMT; /* enbl xmt & rcv for pointer device */

/*--------------------------------------------
* init keyboard defaults (DUART channel A) */

	for (i = 500; i > 0; --i) {
	    if ((status = duart->statusA) & XMT_RDY) {
		duart->dataA = LK_DEFAULTS;
		break;
	    }
	}

	for (i = 100000; i > 0; --i) {
	    if ((status = duart->statusA) & RCV_RDY) {
		break;
	    }
	}

	status = duart->dataA;		/* flush the ACK */

/*--------------------------------
* identify the pointing device */

	for (i = 500; i > 0; --i) {
	    if ((status = duart->statusB) & XMT_RDY) {
		duart->dataB = SELF_TEST;
		break;
	    }
	}

	/*-----------------------------------------
	* wait for 1st byte of self test report */

	for (i = 100000; i > 0; --i) {
	    if ((status = duart->statusB) & RCV_RDY) {
		break;
	    }
	}

	if (i == 0) {
	    mprintf("\nqd[%d]: setup_input: timeout on 1st byte of self test",unit);
	    goto OUT;
	}

	status = duart->dataB;

	/*-----------------------------------------
	* wait for ID byte of self test report	*/

	for (i = 100000; i > 0; --i) {
	    if ((status = duart->statusB) & RCV_RDY) {
		break;
	    }
	}

	if (i == 0) {
	    mprintf("\nqd[%d]: setup_input: timeout on 2nd byte of self test", unit);
	    goto OUT;
	}

	id_byte = duart->dataB;

	/*------------------------------------
	* wait for other bytes to come in  */

	for (i = 100000; i > 0; --i) {
	    if ((status = duart->statusB) & RCV_RDY) {
		status = duart->dataB;
		break;
	    }
	}

	if (i == 0) {
	    mprintf("\nqd[%d]: setup_input: timeout on 3rd byte of self test", unit);
	    goto OUT;
	}

	for (i = 100000; i > 0; --i) {
	    if ((status = duart->statusB) & RCV_RDY) {
		status = duart->dataB;
		break;
	    }
	}

	if (i == 0) {
	    mprintf("\nqd[%d]: setup_input: timeout on 4th byte of self test\n", unit);
	    goto OUT;
	}

	/*----------------------------------------------
	* flag pointing device type and set defaults */

	for (i=100000; i>0; --i);

	if ((id_byte & 0x0F) != TABLET_ID) {

	    qdflags[unit].pntr_id = MOUSE_ID;

	    for (i = 500; i > 0; --i) {
		if ((status = duart->statusB) & XMT_RDY) {
		    duart->dataB = INC_STREAM_MODE;
		    break;
		}
	    }
	} else {

	    qdflags[unit].pntr_id = TABLET_ID;

	    for (i = 500; i > 0; --i) {
		if ((status = duart->statusB) & XMT_RDY) {
		    duart->dataB = T_STREAM;
		    break;
		}
	    }
	}

/*--------
* exit */

OUT:
	duart->imask = qdflags[unit].duart_imask;
	return(0);

} /* setup_input */

/**********************************************************************
*
*	wait_status()... delay for at least one display frame time
*
***********************************************************************
*
*	calling convention:
*
*		wait_status(adder, mask);
*		struct *adder adder;
*		int mask;
*
*	return: BAD means that we timed out without ever seeing the
*		      vertical sync status bit
*		GOOD otherwise
*
**************/

wait_status(adder, mask)
register struct adder *adder;
register int mask;
{
	register short status;
	int i;

	for ( i = 50000, adder->status = 0
	    ; i > 0  &&  !((status = adder->status) & mask)
	    ; --i);

	if (i == 0) {
	    mprintf("\nwait_status: timeout polling for 0x%x in adder->status", mask);
	    return(BAD);
	}

	return(GOOD);

} /* wait_status */

/**********************************************************************
*
*	write_ID()... write out onto the ID bus
*
***********************************************************************
*
*	calling convention:
*
*		struct *adder adder;	;pntr to ADDER structure
*		short adrs;		;VIPER address
*		short data;		;data to be written
*		write_ID(adder);
*
*	return: BAD means that we timed out waiting for status bits
*		      VIPER-access-specific status bits
*		GOOD otherwise
*
**************/

write_ID(adder, adrs, data)
register struct adder *adder;
register short adrs;
register short data;
{
	int i;
	short status;

	for ( i = 100000, adder->status = 0
	    ; i > 0  &&  !((status = adder->status) & ADDRESS_COMPLETE)
	    ; --i);

	if (i == 0)
	    goto ERR;

	for ( i = 100000, adder->status = 0
	    ; i > 0  &&  !((status = adder->status) & TX_READY)
	    ; --i);

	if (i > 0) {
	    adder->id_data = data;
	    adder->command = ID_LOAD | adrs;
	    return(GOOD);
	}

ERR:
	mprintf("\nwrite_ID: timeout trying to write to VIPER");
	return(BAD);

} /* write_ID */
