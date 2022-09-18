#ifndef lint
static char *sccsid = "@(#)sm.c	4.1	(ULTRIX)	7/2/90";
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
 * 15-Dec-89 -- Alan Frechette
 *	Changes to "ws_display_type" and "ws_display_units".
 *
 * 04-Nov-89 -- Fred Canter
 *	Remove the waitef spin loop (which syncs cursor loading with the
 *	vertical end of frame interrupt) via adb hook sm_waitef_on.
 *
 * 23-Jul-89 -- Randall Brown
 *	In the default case of an ioctl() call, check the return value
 *	from ttioctl() to see if the cmd was invalid.  If it was, return
 *	an error.
 *
 * 24-May-89	darrell
 *	Changed the #include for cpuconf.h to find it in it's new home --
 *	sys/machine/common/cpuconf.h
 *
 * 08-May-89 -- rafiey (Ali Rafieymehr)
 *	When going to physical mode (crash dump) the video could be turned
 *	off by the server (screen saver). Therefore we turn the video on.
 *
 * 08-May-89 -- rafiey (Ali Rafieymehr)
 *	When going to single user mode, the keys would behave as if the
 *	<Ctrl> was also pressed. We now clear the flags for shift and
 *	control in smclose routine.
 *
 * 08-May-89 - rafiey (for Fred Canter)
 *	Fix stray interrupts thru SCB vector 0x1fc caused by polling
 *	with interrupts enabled while waiting for video EOF and
 *	sending characters to the keyboard in sm_key_out().
 *
 * 13-Sep-88 - Ali Rafieymehr
 *
 *	Fixed a bug which was causing the "select" not to work for
 *	alternate console.
 *
 * 05-Aug-88 - Tim Burke
 *
 *	Return the 2 character sequence 0377, 0377 upon receipt of a valid
 *	0377 character only when PARMRK is set under the termio line disc.
 *
 * 16-Jul-88 -- fred (Fred Canter)
 *	Bug fix for cursor loading (replaced original code, only way it works).
 *
 *  14-Jul-88 - Vasudev K. Bhandarkar
 *    
 *      Removed references to VAX assembler linked-list manipulation
 *      instructions.  Removed references to the data structures that
 *      that these instructions manipulated.  Change the millisec
 *      timestamp to accept TOY.  Remove all timers.
 *
 * 13-Jun-88 -- rafiey (Ali Rafieymehr)
 *	Fixed the tablet problem. Tablet wasn't initialized correctly.
 *
 * 07-Jun-88 - Fred Canter
 *	Temp bug fix for cursor loading. Spin loop in waitef too short.
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
 * 12-Oct-87 - Tim Burke
 *	Modified driver to provide 8-bit support to the console port.  
 *	Function keys now set the 9-th bit in the key code.
 *
 *  21-Jun-87 - Tim Burke
 *
 *      Added full TERMIO functionality to terminal subsystem.
 *      Changed open routine to setup propper default settings if the line
 *      is not being used as a graphics device.  Added defines of defaults.
 *
 *   1-Aug-87 -- fred (Fred Canter)
 *	Use TANDEM mode on smsceen for flow control of writes to
 *	/dev/console.
 *	Also changed all cprintf to mprintf for consistency with sg.c.
 *
 *  18-Jun-87 -- Rafiey (Ali Rafieymehr)
 *	If X is in kernel then let X load the cursor by using ioctl. If
 *	X is in the kernel then we won't enable end-of-frame interrupt.
 *	Also fixed the bug which was causing the cursor to be moved after
 *	printing console messages.
 *
 *  3-Jun-87  -- fred (Fred Canter)
 *	Fixed a bug in the hold screen code. It was changing the
 *	TTSTART/TTSTOP bits in tp->t_state on hold screen and
 *	CTRL/S and CTRL/Q.
 *
 * 14-May-87  -- fred (Fred Canter)
 *	Added QD_KERN_UNLOOP ioctl definition. This is a duplicate of
 *	QIOKERNUNLOOP, but is needed!
 *
 *  14-May-87 -- Fred Canter
 *	Bug fix to allow xcons to work (loop kernel messages to
 *	login window), pass sm_tty to ioctl(), not ss_tty.
 *	Changed default kern_loop state to on.
 *
 *  16-Apr-87 -- Fred Canter
 *	Multi-head GPX changes (yes really).
 *
 *  19-Mar-87 -- Fred Canter
 *	Added X in the kernel support.
 *	Fixed the TODO in ALi's last changes (13-Dec).
 *
 *  13-Dec-86 -- Rafiey (Ali Rafieymehr)
 *	Don't allow sm to config if sg present.
 * TODO: not the correct thing to do, but can't fix it
 *	 until new color hardware arrives -- Fred
 *
 *  16-Oct-86 -- fred (Fred Canter)
 *	Fix structure name collision (sminfo) with system code.
 *	Changed it to smdinfo (what it should have been originally).
 *
 *   4-Sep-86 -- rafiey (Ali Rafieymehr)
 *	Added hold screen key support for when X is running.
 *
 *  30-Aug-86 -- fred (Fred Canter)
 *	Ali Rafieymehr added smscreen support (console message window).
 *	Removed smread and smwrite routines.
 *	Final devioctl support.
 *	Several bux fixes by Ali.
 *	Ali added hold screen key support and exclusive open.
 *	Fixed console putc in physical mode (for crash dump).
 *
 *  26-Aug-86 -- rsp (Ricky Palmer)
 *	Cleaned up devioctl code to (1) zero out devget structure
 *	upon entry and (2) use strlen instead of fixed storage
 *	for bcopy's.
 *
 *  14-Aug-86  rafiey (Ali Rafieymehr)
 *	Several driver improvements and add tablet support.
 *
 *   5-Aug-86  rafiey (Ali Rafieymehr)
 *	Major changes for real VAXstar bitmap graphics driver.
 *
 *   2-Jul-86  rafiey (Ali Rafieymehr)
 *	Changed SMMAJOR to 49 and removed unused code.
 *
 *  18-Jun-86  rafiey (Ali Rafieymehr)
 *	Created this VAXstar monochrome display driver.
 *	Derived from qv.c.
 *
 **********************************************************************/


#include "sm.h"
#if NSM > 0  || defined(BINARY)

#include "../h/devio.h"
#include "../machine/pte.h"

#include "../h/param.h"
#include "../h/conf.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../io/uba/smioctl.h"
#include "../h/tty.h"
#include "../h/map.h"
#include "../h/buf.h"
#include "../h/vm.h"
#include "../h/bk.h"
#include "../h/clist.h"
#include "../h/file.h"
#include "../h/uio.h"
#include "../h/kernel.h"
#include "../../machine/common/cpuconf.h"
#include "../h/exec.h"
#include "../h/proc.h"

#include "../machine/cpu.h"
#include "../machine/mtpr.h"

#include "../io/uba/pdma.h"
#include "../io/uba/ubareg.h"
#include "../io/uba/ubavar.h"


/*
 * Following allow smputc to function in
 * the CPU in physical mode (during crash dump).
 * One way transition, can't go back to virtual.
 */
 
#define	VS_PHYSNEXUS	0x20080000
#define	VS_PHYSBITMAP	0x30000000
#define	VS_PHYSCURSOR	0x200c0000

int	sm_physmode = 0;

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

struct	uba_device *smdinfo[NSM];
struct	mouse_report last_rep;
extern	struct	mouse_report current_rep;	/* now in ss.c */
extern	struct	tty	sm_tty;			/* now in ss.c */
extern	struct	tty	ss_tty[];

int	nNSM = NSM;
int	nsm = NSM*4;

/*
 * Definition of the driver for the auto-configuration program.
 */
int	smprobe(), smattach(), smkint(), smvint();

u_short	smstd[] = { 0 };
struct	uba_driver smdriver =
	{ smprobe, 0, smattach, 0, smstd, "sm", smdinfo };

/*
 * Local variables for the driver. Initialized for 19" screen
 * so that it can be used during the boot process.
 */

/*
 * v_consputc is the switch that is used to redirect the console cnputc to the
 * virtual console vputc.
 * v_consgetc is the switch that is used to redirect the console getchar to the
 * virtual console vgetc.
 */
extern (*v_consputc)();
extern (*v_consgetc)();

#define SMMAXEVQ	64	/* must be power of 2 */
#define EVROUND(x)	((x) & (SMMAXEVQ - 1))
#define TABLET_RES	2

#define	CONSOLEMAJOR	0
#define SM_MAJOR        49
#define CONS_DEV	0x01
#define GRAPHIC_DEV	0x02

/*
 * Screen parameters for the 19 inch monitors. These determine the max size in
 * pixel and character units for the display and cursor positions.
 * Notice that the mouse defaults to original square algorithm, but X
 * will change to its defaults once implemented.
 */



struct sm_info sm_scn_defaults;

sminit_scn_def()
{
	bzero(&sm_scn_defaults,sizeof(struct sm_info));

	sm_scn_defaults.max_row = 56;
	sm_scn_defaults.max_col = 120;
	sm_scn_defaults.max_x = 1024;
	sm_scn_defaults.max_y = 864;
	sm_scn_defaults.max_cur_x = 1023;
	sm_scn_defaults.max_cur_y = 863;
	sm_scn_defaults.version = 11;
	sm_scn_defaults.mthreshold = 4;	
	sm_scn_defaults.mscale = 2;
	sm_scn_defaults.min_cur_x = -15;
	sm_scn_defaults.min_cur_y = -15;

}




/*
 * Screen parameters
 */
struct sm_info  *sm_scn; /* why isn't this indexed by minor device #  XXX */
	
/*
 * Keyboard state
 */
struct sm_keyboard {
	int shift;			/* state variables	*/
	int cntrl;
	int lock;
	int hold;
	char last;			/* last character	*/
} sm_keyboard;

short sm_divdefaults[15] = { LK_DOWN,	/* 0 doesn't exist */
	LK_AUTODOWN, LK_AUTODOWN, LK_AUTODOWN, LK_DOWN,
	LK_UPDOWN,   LK_UPDOWN,   LK_AUTODOWN, LK_AUTODOWN, 
	LK_AUTODOWN, LK_AUTODOWN, LK_AUTODOWN, LK_AUTODOWN, 
	LK_DOWN, LK_AUTODOWN };

short sm_kbdinitstring[] = {		/* reset any random keyboard stuff */
	LK_AR_ENABLE,			/* we want autorepeat by default */
	LK_CL_ENABLE,			/* keyclick */
	0x84,				/* keyclick volume */
	LK_KBD_ENABLE,			/* the keyboard itself */
	LK_BELL_ENABLE,			/* keyboard bell */
	0x84,				/* bell volume */
	LK_LED_DISABLE,			/* keyboard leds */
	LED_ALL };
#define KBD_INIT_LENGTH	sizeof(sm_kbdinitstring)/sizeof(short)

#define TOY ((time.tv_sec * 1000) + (time.tv_usec / 1000))

int	sm_events;
int	sm_ipl_lo = 1;		/* IPL low flag			*/

extern	u_short	sm_pointer_id;	/* id of pointer device (mouse,tablet)-ss.c */
int	sm_mouseon = 0;        	/* Mouse is enable when 1 */
int	cur_reg = 0;		/* Register to keep track of cursor register bits*/
int	monochrome = -1;	/* using VR260 when 0 */
u_int	sm_dev_inuse = 0;	/* which minor dev's are in use */
u_short	sm_kern_loop;		/* redirect kernel console messages to the  */
 				/* alternate console if -1 */
int	open_flag = 0;		/* graphics device is open when 1 */
unsigned short sm_cursor[32];	/* the value of current cursor */
int	smtime_since_input = 0;	/* number of miliseconds since last input */
int	sm_time_limit = 0;

struct proc *rsel;			/* process waiting for select */

int	smstart(), smputc(), smgetc(), ttrstrt();

/*
 * Keyboard translation and font tables
 */
extern  char *q_special[],q_font[];
extern  u_short q_key[],q_shift_key[];

extern	struct	nexus	nexus[];


/*
 * Default cursor (plane A and plane B)
 *
 */

unsigned  short def_cursor[32] = { 

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

/*
 * These variables identify the type of
 * graphics device (set in ??cons_init().
 */

extern	int	ws_display_type;
extern	int	ws_display_units;

smprobe(reg)
	caddr_t reg;
{
	register struct nb_regs *smaddr = (struct nb_regs *)nexus;

/*
 * Only on a VAXstation 2200 (not MicroVAX 2000).
 * Also on CVAXstar.
 * TODO: watchout for MULTU bit meaning change!
 */
	if (((cpu != VAXSTAR) && (cpu != C_VAXSTAR)) ||
	    (vs_cfgtst&VS_MULTU) ||
	    (vs_cfgtst&VS_VIDOPT))
		return(0);

/*
 * Only if monochrome display controller present.
 */

	if ((vs_cfgtst&VS_CURTEST) == 0)
		return(0);

/*
 * Only if SM is the graphics device.
 */

	if (ws_display_type && (ws_display_type != SM_DTYPE))
		return(0);

/* Enable video end-of-frame interrupt */

	smaddr->nb_vdc_sel = 0;
	smaddr->nb_int_msk |= SINT_VF;

	DELAY(20000);  /* wait  */

/* Disable video end-of-frame interrupt */

	smaddr->nb_int_msk &= ~SINT_VF;
	return(8);
}


/******************************************************************
 **                                                              **
 ** Routine to attach to the graphic device.                     **
 **                                                              **
 ******************************************************************/

smattach(ui)
	struct uba_device *ui;
{

	register int *pte;
	register struct sm_info *qp = sm_scn;
	int	 i;


/*
 * init the "latest mouse report" structure
 */

	last_rep.state = 0;
	last_rep.dx = 0;
	last_rep.dy = 0;
	last_rep.bytcnt = 0;

	sm_keyboard.hold = 0; /* "Hold Screen" key is pressed if 1 */

/*
 * set the flag not to redirect kernel console messages to the alternate
 * console.
 * Changed default to kern_loop on - 5/14/87 -- Fred Canter
 */
	sm_kern_loop = -1;

/*
 * Do the following only for the monochrome display.
 */
	if (monochrome != -1 ) {
/*
 * Map the bitmap for use by users.
 */

	    pte = (int *)(NMEMmap[0]);
	    for( i=0 ; i<256 ; i++, pte++ )
		*pte = (*pte & ~PG_PROT) | PG_UW | PG_V;

/*
 * Clear out the cursor (this is not cursor ram, this area will be used by
 * the X system. We do this because we didn't want to change the device
 * dependent portion of X for the qvss.
 */
	    for (i = 0; i < 32; i++) {
	    	qp->cursorbits[i] = 0;
		sm_cursor[i] = 0;
	    }
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
smopen(dev, flag)
	dev_t dev;
{
	register int unit = minor(dev);
	register struct tty *tp;
	register struct nb_regs *smiaddr = (struct nb_regs *)nexus;
	register struct sm_info *qp = sm_scn;

/*
 * The graphics device can be open only by one person 
 */
	if (unit == 1) {
	    if (open_flag != 0)
		return(EBUSY);
	    else
		open_flag = 1;
            sm_dev_inuse |= GRAPHIC_DEV;  /* graphics dev is open */
	} else
            sm_dev_inuse |= CONS_DEV;  /* mark console as open */
	if ((unit == 2) && (major(dev) == CONSOLEMAJOR))
	    tp = &sm_tty;
	else
	    tp = &ss_tty[unit];
	if (tp->t_state&TS_XCLUDE && u.u_uid!=0)
	    return (EBUSY);
        sm_scn->smaddr = smiaddr;
	tp->t_addr = (caddr_t)&sspdma[unit];
	tp->t_oproc = smstart;

	/*---------------------------------------------------------------------
	* Look at the compatibility mode to specify correct default parameters
	* and to insure only standard specified functionality. */
	if ((u.u_procp->p_progenv == A_SYSV) || 
		(u.u_procp->p_progenv == A_POSIX)) {
		flag |= O_TERMIO;
		tp->t_line = TERMIODISC;
	}
	/*
	 * Set state bit to tell tty.c not to assign this line as the 
	 * controlling terminal for the process which opens this line.
	 */
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

/*
 * Process line discipline specific open if its not the mouse.
 * For the mouse we init the ring ptr's.
 */
	if (unit != 1)
	    return ((*linesw[tp->t_line].l_open)(dev, tp));
	else {

/*
 * set up event queue for later
 */
	    sm_mouseon = 1;
	    smiaddr->nb_int_msk |= SINT_VF;

	    qp->qe.events = (smEvent *)qp - SMMAXEVQ;
	    qp->qe.eSize = SMMAXEVQ;
	    qp->qe.eHead = qp->qe.eTail = 0;
	    qp->qe.tcs = (smTimeCoord *)qp->qe.events - MOTION_BUFFER_SIZE;
	    qp->qe.tcSize = MOTION_BUFFER_SIZE;
	    qp->qe.tcNext = 0;

	    return(0);
	}
}


/******************************************************************
 **                                                              **
 ** Routine to close the graphic device.                         **
 **                                                              **
 ******************************************************************/

/*ARGSUSED*/
smclose(dev, flag)
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
	    sm_dev_inuse &= ~CONS_DEV;
	    sm_keyboard.cntrl = sm_keyboard.shift = 0;
	} else {
	    sm_mouseon = 0;
	    if (open_flag != 1)
		return(EBUSY);
	    else
		open_flag = 0; /* mark the graphics device available */
	    sm_dev_inuse &= ~GRAPHIC_DEV;
	    sm_init();
	    if (cur_reg & FOPB)	{ /* user must have turned the screen off */
	        register struct nb1_regs *smaddr1 = (struct nb1_regs *)qmem;
		cur_reg |= ENPA;
		cur_reg &= ~(FOPB);
		smaddr1->nb_cur_cmd = cur_reg;
		}
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
 ** Mouse activity select routine.                               **
 **                                                              **
 ******************************************************************/

smselect(dev, rw)
dev_t dev;
{
	register int s = spl5();
	register int unit = minor(dev);
	register struct sm_info *qp = sm_scn;
        register struct tty *tp;


	    switch(rw) {
	    case FREAD:			/* if events okay */

		if( unit == 1 ) {
			if (qp->qe.eHead != qp->qe.eTail) {
			    splx(s);
			    return(1);
			}
			rsel = u.u_procp;
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

	    case FWRITE:		/* can never write */

		if( unit == 1 ) {
			splx(s);
			return(EACCES);
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


#define CHAR_S	0xc7
#define CHAR_Q	0xc1

/******************************************************************
 **                                                              **
 ** Graphice device interrupt routine.                           **
 **                                                              **
 ******************************************************************/

smkint(ch)
register int ch;
{
	register smEvent *qep;
	register struct sm_info *qp = sm_scn;
	struct mouse_report *new_rep;
	struct tty *tp;
	register int unit;
	register u_short c;
	register int i, j;
	u_short data;
	int	cnt;

/*
 * Mouse state info
 */
	static char temp, old_switch, new_switch;


	unit = (ch>>8)&03;
	new_rep = &current_rep;
	tp = &ss_tty[unit];

/*
 * If graphic device is turned on
 */

   if (sm_mouseon == 1) {
  
	cnt = 0;
	while (cnt++ == 0) {

/*
 * Pick up LK-201 input (if any)
 */

	    if (unit == 0) {

/*
 * Get a character.
 */

		data = ch & 0xff;

/*
 * Check for various keyboard errors
 */

		if( data == LK_POWER_ERROR || data == LK_KDOWN_ERROR ||
	    	    data == LK_INPUT_ERROR || data == LK_OUTPUT_ERROR) {
			mprintf("\nsm0: smkint: keyboard error, code = %x",data);
			return(0);
		}

		if (data < LK_LOWEST) 
		    	return(0);

		 if ((i = EVROUND(qp->qe.eTail+1)) == qp->qe.eHead) return(0);


		    /* for any key */
		    qep = &qp->qe.events[qp->qe.eTail];

		    qep->type = BUTTON_RAW_TYPE;
		    qep->device = KEYBOARD_DEVICE;
		    qep->x = qp->mouse.x;
		    qep->y = qp->mouse.y;
		    qep->time = TOY;
		    qep->key = data;
		    qp->qe.eTail = i;
	            if((i=EVROUND(qp->qe.eTail+1)) == qp->qe.eHead) return(0);
		    qep = &qp->qe.events[qp->qe.eTail];

			if (data == CNTRL)
			    sm_keyboard.cntrl = 1;
	    }

/*
 * Pick up the mouse input (if any)
 */

	    if ((unit == 1) && (sm_pointer_id == MOUSE_ID)) {

/*
 * see if mouse position has changed
 */
		if( new_rep->dx != 0 || new_rep->dy != 0) {
			unsigned int millis = TOY;

/*
 * Check to see if we have to accelerate the mouse
 *
 */
		    if (qp->mscale >=0) {
			if (new_rep->dx >= qp->mthreshold)
			    new_rep->dx +=
				(new_rep->dx - qp->mthreshold)*qp->mscale;
			if (new_rep->dy >= qp->mthreshold)
			    new_rep->dy +=
				(new_rep->dy - qp->mthreshold)*qp->mscale;
		    }

/*
 * update mouse position
 */
		    if( new_rep->state & X_SIGN) {
			qp->mouse.x += new_rep->dx;
			if( qp->mouse.x > qp->max_cur_x )
			    qp->mouse.x = qp->max_cur_x;
		    }
		    else {
			qp->mouse.x -= new_rep->dx;
			if( qp->mouse.x < qp->min_cur_x )
			    qp->mouse.x = qp->min_cur_x;
		    }
		    if( new_rep->state & Y_SIGN) {
			qp->mouse.y -= new_rep->dy;
			if( qp->mouse.y < qp->min_cur_y )
			    qp->mouse.y = qp->min_cur_y;
		    }
		    else {
			qp->mouse.y += new_rep->dy;
			if( qp->mouse.y > qp->max_cur_y )
			    qp->mouse.y = qp->max_cur_y;
		    }
		    if( tp->t_state & TS_ISOPEN )
			sm_pos_cur( qp->mouse.x, qp->mouse.y );
		    qp->qe.tcs[qp->qe.tcNext].time = millis;
		    qp->qe.tcs[qp->qe.tcNext].x = qp->mouse.x;
		    qp->qe.tcs[qp->qe.tcNext].y = qp->mouse.y;
		    if (++(qp->qe.tcNext) >= MOTION_BUFFER_SIZE)
			qp->qe.tcNext = 0;
		    if (qp->mouse.y < qp->mbox.bottom &&
			qp->mouse.y >=  qp->mbox.top &&
			qp->mouse.x < qp->mbox.right &&
			qp->mouse.x >=  qp->mbox.left) goto mbuttons;
		    qp->mbox.bottom = 0;	/* trash box */
		    if (EVROUND(qp->qe.eTail+1) == qp->qe.eHead)
				goto mbuttons;

		    i = EVROUND(qp->qe.eTail -1);
		    if ((qp->qe.eTail != qp->qe.eHead) && (i != qp->qe.eHead)) 
		    {
		        qep = & qp->qe.events[i];
		        if(qep->type == MOTION_TYPE) 
			{
			    qep->x = qp->mouse.x;
			    qep->y = qp->mouse.y;
			    qep->time = millis;
			    qep->device = MOUSE_DEVICE;
			    goto mbuttons;
			}
		    }
/*
 * Put event into queue and do select
 */
		    qep = &qp->qe.events[qp->qe.eTail];
		    qep->type = MOTION_TYPE;
		    qep->time = millis;
		    qep->x = qp->mouse.x;
		    qep->y = qp->mouse.y;
		    qep->device = MOUSE_DEVICE;
                    qp->qe.eTail = EVROUND(qp->qe.eTail+1);
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

/*
 * Check for room in the queue
 */
                        if ((i = EVROUND(qp->qe.eTail+1)) == qp->qe.eHead) 
			    return(0);


/* put event into queue and do select */

                        qep = &qp->qe.events[qp->qe.eTail];

			switch (j) {
			case RIGHT_BUTTON:
					qep->key = EVENT_RIGHT_BUTTON;
					break;

			case MIDDLE_BUTTON:
					qep->key = EVENT_MIDDLE_BUTTON;
					break;

			case LEFT_BUTTON:
					qep->key = EVENT_LEFT_BUTTON;
					break;

			}
			if (new_switch & j)
				qep->type = BUTTON_DOWN_TYPE;
			else
				qep->type = BUTTON_UP_TYPE;
			qep->device = MOUSE_DEVICE;
			qep->time = TOY;
			qep->x = qp->mouse.x;
			qep->y = qp->mouse.y;
		    }
		    qp->qe.eTail = i;

/* update the last report */

		    last_rep = current_rep;
		    qp->mswitches = new_switch;
		}
	    } /* Pick up mouse input */

	    else if ((unit == 1) && (sm_pointer_id == TABLET_ID)) {

/* update cursor position coordinates */

		    new_rep->dx /= TABLET_RES;
		    new_rep->dy = (2200 - new_rep->dy) / TABLET_RES;
		    if( new_rep->dx > qp->max_cur_x )
			new_rep->dx = qp->max_cur_x;
		    if( new_rep->dy > qp->max_cur_y )
			new_rep->dy = qp->max_cur_y;

/*
 * see if the puck/stylus has moved
 */
		    if( qp->mouse.x != new_rep->dx ||
			qp->mouse.y != new_rep->dy) {

/*
 * update cursor position
 */
		 	qp->mouse.x = new_rep->dx;
		 	qp->mouse.y = new_rep->dy;

		    	if( tp->t_state & TS_ISOPEN )
			    sm_pos_cur( qp->mouse.x, qp->mouse.y );
		    	if (qp->mouse.y < qp->mbox.bottom &&
			    qp->mouse.y >=  qp->mbox.top &&
			    qp->mouse.x < qp->mbox.right &&
			    qp->mouse.x >=  qp->mbox.left) goto tbuttons;
		    	qp->mbox.bottom = 0;	/* trash box */
                        if (EVROUND(qp->qe.eTail+1) == qp->qe.eHead)
			    goto tbuttons;

/*
 * Put event into queue and do select
 */
			qep = &qp->qe.events[qp->qe.eTail];
		    	qep->type = MOTION_TYPE;
		    	qep->device = TABLET_DEVICE;
			qep->x = qp->mouse.x;
			qep->y = qp->mouse.y;
			qep->key = 0;
		    	qep->time = TOY;
                        qep = & qp->qe.events[qp->qe.eTail];
		    }

/*
 * See if tablet buttons have changed.
 */

tbuttons:

		new_switch = new_rep->state & 0x1e;
		old_switch = last_rep.state & 0x1e;
		temp = old_switch ^ new_switch;
		if( temp ) {
	            if((i=EVROUND(qp->qe.eTail+1)) == qp->qe.eHead) return(0);

/* put event into queue and do select */

		    qep = &qp->qe.events[qp->qe.eTail];
		    qep->device = TABLET_DEVICE;
		    qep->x = qp->mouse.x;
		    qep->y = qp->mouse.y;
		    qep->time = TOY;

/* define the changed button and if up or down */

		    for (j = 1; j <= 0x10; j <<= 1) {/* check each button */
			if (!(j & temp))  /* did this button change? */
			    continue;
			switch (j) {
			case T_RIGHT_BUTTON:
					qep->key = EVENT_T_RIGHT_BUTTON;
					break;

			case T_FRONT_BUTTON:
					qep->key = EVENT_T_FRONT_BUTTON;
					break;

			case T_BACK_BUTTON:
					qep->key = EVENT_T_BACK_BUTTON;
					break;

			case T_LEFT_BUTTON:
					qep->key = EVENT_T_LEFT_BUTTON;
					break;

			}
		    	if (new_switch & j)
			    qep->type = BUTTON_DOWN_TYPE;
		    	else
			    qep->type = BUTTON_UP_TYPE;
		    }
                    qp->qe.eTail =  i;

/* update the last report */

		    last_rep = current_rep;
		}
	    } /* Pick up tablet input */
	} /* While input available */

/*
 * If we have proc waiting, and event has happened, wake him up
 */

	if(rsel && (qp->qe.eHead != qp->qe.eTail)) {
	    selwakeup(rsel,0);
	    rsel = 0;
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
			mprintf("sm0: Keyboard error, code = %x\n",data);
			return(0);
	    }
	    if( data < LK_LOWEST ) return(0);

/*
 * See if its a state change key
 */

	    switch ( data ) {
	    case LOCK:
			sm_keyboard.lock ^= 0xffff;	/* toggle */
			if( sm_keyboard.lock )
				sm_key_out( LK_LED_ENABLE );
			else
				sm_key_out( LK_LED_DISABLE );
			sm_key_out( LED_3 );
			return;

	    case SHIFT:
			sm_keyboard.shift ^= 0xffff;
			return;	

	    case CNTRL:
			sm_keyboard.cntrl ^= 0xffff;
			return;

	    case ALLUP:
			sm_keyboard.cntrl = sm_keyboard.shift = 0;
			return;

	    case REPEAT:
			c = sm_keyboard.last;
			break;

	    case HOLD:
/*
 * "Hold Screen" key was pressed, we treat it as if ^s or ^q was typed.
 */
			if (sm_keyboard.hold == 0) {
			    if((tp->t_state & TS_TTSTOP) == 0) {
			    	c = q_key[CHAR_S];
			    	sm_key_out( LK_LED_ENABLE );
			    	sm_key_out( LED_4 );
				sm_keyboard.hold = 1;
			    } else
				c = q_key[CHAR_Q];
			}
			else {
			    c = q_key[CHAR_Q];
			    sm_key_out( LK_LED_DISABLE );
			    sm_key_out( LED_4 );
			    sm_keyboard.hold = 0;
			}
			if( c >= ' ' && c <= '~' )
			    c &= 0x1f;
			else if (c >= 0xA1 && c <= 0xFE)
			    c &= 0x9F;
		    	(*linesw[tp->t_line].l_rint)(c, tp);
			return;

	    default:

/*
 * Test for control characters. If set, see if the character
 * is elligible to become a control character.
 */
			if( sm_keyboard.cntrl ) {
			    c = q_key[ data ];
			    if( c >= ' ' && c <= '~' )
				c &= 0x1f;
			} else if( sm_keyboard.lock || sm_keyboard.shift )
				    c = q_shift_key[ data ];
				else
				    c = q_key[ data ];
			break;	

	    }

	    sm_keyboard.last = c;

/*
 * Check for special function keys
 */
	    if( c & 0x100 ) {

		register char *string;

		string = q_special[ c & 0x7f ];
		while( *string )
		    (*linesw[tp->t_line].l_rint)(*string++, tp);
	    } else {
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
	    if (sm_keyboard.hold &&((tp->t_state & TS_TTSTOP) == 0)) {
		    sm_key_out( LK_LED_DISABLE );
		    sm_key_out( LED_4 );
		    sm_keyboard.hold = 0;
	    }
	}
   }

	return(0);

} /* smkint */





/******************************************************************
 **                                                              **
 ** Graphic device ioctl routine.                                **
 **                                                              **
 ******************************************************************/

/*ARGSUSED*/
smioctl(dev, cmd, data, flag)
	dev_t dev;
	register caddr_t data;
{
	register struct tty *tp;
	register int unit = minor(dev);
	register struct sm_info *qp = sm_scn;
	register struct sm_kpcmd *qk;
	register unsigned char *cp;
	struct nb1_regs *smaddr1;
	int error;
	int i;
	struct devget *devget;
 
/*
 * Check for and process VAXstar monochrome specific ioctl's
 */

	switch( cmd ) {
	case QIOCGINFO:					/* return screen info */
		bcopy(qp, data, sizeof (struct sm_info));
		break;

	case QIOCSMSTATE:				/* set mouse state */
		qp->mouse = *((smCursor *)data);
		sm_pos_cur( qp->mouse.x, qp->mouse.y );
		break;

	case QIOCINIT:					/* init screen	*/
		sm_init();
		break;

	case QIOCKPCMD:
		qk = (struct sm_kpcmd *)data;
		if(qk->nbytes == 0) qk->cmd |= 0200;
		if(sm_mouseon == 0) qk->cmd |= 1;	/* no mode changes */
		sm_key_out(qk->cmd);
		cp = &qk->par[0];
		while(qk->nbytes-- > 0) {	/* terminate parameters */
			if(qk->nbytes <= 0) *cp |= 0200;
			sm_key_out(*cp++);
		}
		break;

	case QIOCADDR:					/* get struct addr */
		*(struct sm_info **) data = qp;
		break;

        case QIOVIDEOON:				/* turn on the video */
		smaddr1 = (struct nb1_regs *)qmem;
		cur_reg &= ~FOPB;
		cur_reg |= ENPA;
		smaddr1->nb_cur_cmd = cur_reg;
		break;

	case QIOVIDEOOFF:				/* turn off the video */
		smaddr1 = (struct nb1_regs *)qmem;
		cur_reg |= FOPB;
		cur_reg &= ~(ENPA | FOPA);
		smaddr1->nb_cur_cmd = cur_reg;
		break;

	case QIOWCURSOR:				/* Write cursor bit map */

		sm_load_cursor(data);
		break;

	case QIOKERNLOOP:      /* redirect kernel console output */
		sm_kern_loop = -1;
		break;

	case QIOKERNUNLOOP:      /* don't redirect kernel console output */
	case QD_KERN_UNLOOP:	/* duplicate, called from inside kernel */
		sm_kern_loop = 0;
		break;

#ifdef debby
	case QIODISPON:					/* display on	*/
		{
		register struct nb1_regs *smaddr1 = (struct nb1_regs *)qmem;
		cur_reg |= ENPA;
		cur_reg &= ~(FOPB);
		smaddr1->nb_cur_cmd = cur_reg;
		break;
		}

	case QIODISPOFF:				/* display on	*/
		{
		register struct nb1_regs *smaddr1 = (struct nb1_regs *)qmem;
		cur_reg |= FOPB;
		cur_reg &= ~(ENPA);
		smaddr1->nb_cur_cmd = cur_reg;
		break;
		}
#endif

	case DEVIOCGET: 			/* device status */
		devget = (struct devget *)data;
		bzero(devget,sizeof(struct devget));
		devget->category = DEV_TERMINAL;	/* terminal cat.*/
		devget->bus = DEV_NB;			/* NO bus	*/
		bcopy(DEV_VS_SLU,devget->interface,
		      strlen(DEV_VS_SLU));		/* interface	*/
		if(unit == 0)
		    bcopy(DEV_VR260,devget->device,
			  strlen(DEV_VR260));		/* device	*/
		else if(sm_pointer_id == MOUSE_ID)
		    bcopy(DEV_MOUSE,devget->device,
			  strlen(DEV_MOUSE));
		else if(sm_pointer_id == TABLET_ID)
		    bcopy(DEV_TABLET,devget->device,
			  strlen(DEV_TABLET));
		else
		    bcopy(DEV_UNKNOWN,devget->device,
			  strlen(DEV_UNKNOWN));
		devget->adpt_num = 0;			/* NO adapter	*/
		devget->nexus_num = 0;			/* fake nexus 0	*/
		devget->bus_num = 0;			/* NO bus	*/
		devget->ctlr_num = 0;			/* cntlr number */
		devget->slave_num = unit;		/* line number 	*/
		bcopy("sm",devget->dev_name, 3);	/* Ultrix "sm"	*/
		devget->unit_num = unit;		/* ss line?	*/
		/* TODO: should say not supported instead of zero!	*/
		devget->soft_count = 0;			/* soft err cnt */
		devget->hard_count = 0;			/* hard err cnt */
		devget->stat = 0;			/* status	*/
		devget->category_stat = 0;		/* cat. stat.	*/
		break;

	default:					/* not ours ??  */
		if ((unit == 2) && (major(dev) == CONSOLEMAJOR))
		    tp = &sm_tty;
		else
		    tp = &ss_tty[unit];
		error = (*linesw[tp->t_line].l_ioctl)(tp, cmd, data, flag);
		if (error >= 0)
			return (error);
		error = ttioctl(tp, cmd, data, flag);
		if (error >= 0) {
			return (error);
		}
		/* if error = -1 then ioctl does not exist */
		if (u.u_procp->p_progenv == A_POSIX) 
		    return (EINVAL);
		return (ENOTTY);
		
		break;
	}
	return (0);
}




/******************************************************************
 **                                                              **
 ** Graphic device End-Of-Frame interrupt Routine.               **
 **                                                              **
 ******************************************************************/

#define SM_TIME1	  100
#define SM_TIME2	  500
#define SM_MAX_LOOP	10200
int sm_max_loop = SM_MAX_LOOP;
int sm_time2 = SM_TIME2;
int sm_time1 = SM_TIME1;

smvint(sm)
	int sm;
{
	register struct sm_info *qp = sm_scn;

/*
 * Test and set the sm_ipl_lo flag. If the result is not zero then
 * someone else must have already gotten here.
 */

	if( --sm_ipl_lo )
	    return;

	qp->qe.timestamp_ms = TOY;

	sm_ipl_lo = 1;
}




/******************************************************************
 **                                                              **
 ** Routine to start transmission. 				**
 **                                                              **
 ******************************************************************/

smstart(tp)
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
		    sm_blitc( c & 0xff );
		}
	    } else if (unit == 2) {	/* smscreen, do flow control */
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
 * Position the cursor to the next character location.
 */

	if (!(sm_dev_inuse & GRAPHIC_DEV))
	    sm_pos_cur( sm_scn->col*8, sm_scn->row*15 );

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
smstop(tp, flag)
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

sm_blitc( c )
register u_char c;
{

	register char *b_row, *f_row;
	register int i;
	register int ote = 128;
	register struct sm_info *qp = sm_scn;

	c &= 0xff;

	switch ( c ) {
	case '\t':				/* tab		*/
		    for( i = 8 - (qp->col & 0x7) ; i > 0 ; i-- )
			sm_blitc( ' ' );
		    return(0);

	case '\r':				/* return	*/
		    qp->col = 0;
		    return(0);

	case '\b':				/* backspace	*/
		    if( --qp->col < 0 )
			qp->col = 0;
		    return(0);

	case '\n':				/* linefeed	*/
		    if( qp->row+1 >= qp->max_row )
			smscroll();
		    else
			qp->row++;

/*
 * Position the cursor to the next character location.
 */

	    	    if (!(sm_dev_inuse & GRAPHIC_DEV))
		    	sm_pos_cur( qp->col*8, qp->row*15 );
		    return(0);

	case '\007':				/* bell		*/

/*
 * We don't do anything to the keyboard until after autoconfigure.
 */

		    if( qp->smaddr )
			sm_key_out( LK_RING_BELL );
		    return(0);

	default:
		    /*
		     * xA1 to XFD are the printable characters added with 8-bit
		     * support.
		     */
		    if(( c >= ' ' && c <= '~' ) || ( c >= 0xA1 && c <= 0xFD)) {
                        b_row = qp->bitmap+(qp->row*15 & 0x3ff)*128+qp->col;
			i = c - ' ';
			if( i < 0 || i > 221 )
			    i = 0;
			else {
				/* These are to skip the (32) 8-bit 
				 * control chars, as well as DEL 
			      	 * and 0xA0 which aren't printable */
				if (c > '~') 
					i -= 34; 
			    i *= 15;
			}
			f_row = (char *)((int)q_font + i);

/* inline expansion for speed */

			*b_row = *f_row++; b_row += ote;
			*b_row = *f_row++; b_row += ote;
			*b_row = *f_row++; b_row += ote;
			*b_row = *f_row++; b_row += ote;
			*b_row = *f_row++; b_row += ote;
			*b_row = *f_row++; b_row += ote;
			*b_row = *f_row++; b_row += ote;
			*b_row = *f_row++; b_row += ote;
			*b_row = *f_row++; b_row += ote;
			*b_row = *f_row++; b_row += ote;
			*b_row = *f_row++; b_row += ote;
			*b_row = *f_row++; b_row += ote;
			*b_row = *f_row++; b_row += ote;
			*b_row = *f_row++; b_row += ote;
			*b_row = *f_row++; b_row += ote;

			if( ++qp->col >= qp->max_col ) {
			    qp->col = 0 ;
			    if( qp->row+1 >= qp->max_row )
				smscroll();
			    else
				qp->row++;
			}
		}
		return(0);
	}
}




/********************************************************************
 **                                                                **
 ** Routine to direct kernel console output to display destination **
 **                                                                **
 ********************************************************************/

smputc( c )
register char c;
{

	register struct tty *tp0;
	register struct sm_info *qp;
	register struct nb1_regs *smaddr1;


/*
 * This routine may be called in physical mode by the dump code
 * so we change the driver into physical mode.
 * One way change, can't go back to virtual mode.
 */
	if( (mfpr(MAPEN) & 1) == 0 ) {
		sm_physmode = 1;
		sm_mouseon = 0;
		if (cur_reg & FOPB) { /* user must have turned the screen off */
		    smaddr1 = (struct nb1_regs *)VS_PHYSCURSOR;
		    cur_reg |= ENPA;
		    cur_reg &= ~(FOPB);
		    smaddr1->nb_cur_cmd = cur_reg;
		}
		sm_scn = (struct sm_info *)((u_int)VS_PHYSBITMAP + 125*1024);
		qp = sm_scn;
		qp->version = 11;
		qp->bitmap = (char *)((u_long)VS_PHYSBITMAP);
		sm_blitc(c & 0xff);
		return;
	}

/*
 * direct kernel output char to the proper place
 */

	tp0 = &sm_tty;

	if (sm_kern_loop != 0  &&  tp0->t_state & TS_ISOPEN) {
		(*linesw[tp0->t_line].l_rint)(c, tp0);
	} else {
		sm_blitc(c & 0xff);
	}

}



/******************************************************************
 **                                                              **
 ** Routine to get a character from LK201.                       **
 **                                                              **
 ******************************************************************/

int	ssgetc();

smgetc()
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
		mprintf(" Keyboard error, code = %x\n",data);
		return(0);
	}
	if( data < LK_LOWEST ) return(0);

/*
 * See if its a state change key
 */

	switch ( data ) {
	case LOCK:
		sm_keyboard.lock ^= 0xffff;	/* toggle */
		if( sm_keyboard.lock )
			sm_key_out( LK_LED_ENABLE );
		else
			sm_key_out( LK_LED_DISABLE );
		sm_key_out( LED_3 );
		goto loop;

	case SHIFT:
		sm_keyboard.shift ^= 0xffff;
		goto loop;

	case CNTRL:
		sm_keyboard.cntrl ^= 0xffff;
		goto loop;

	case ALLUP:
		sm_keyboard.cntrl = sm_keyboard.shift = 0;
		goto loop;

	case REPEAT:
		c = sm_keyboard.last;
		break;

	default:

/*
 * Test for control characters. If set, see if the character
 * is elligible to become a control character.
 */
		if( sm_keyboard.cntrl ) {
		    c = q_key[ data ];
		    if( c >= ' ' && c <= '~' )
			c &= 0x1f;
		} else if( sm_keyboard.lock || sm_keyboard.shift )
			    c = q_shift_key[ data ];
		       else
			    c = q_key[ data ];
		break;	

	}

	sm_keyboard.last = c;

/*
 * Check for special function keys
 */
	if( c & 0x80 )
	    return (0);
	else
	    return (c);
}



/******************************************************************
 **                                                              **
 ** Routine to Position the cursor to a particular spot.         **
 **                                                              **
 ******************************************************************/

sm_pos_cur( x, y)
register int x,y;
{
	register struct nb1_regs *smaddr1;
	register struct sm_info *qp = sm_scn;

	if(sm_physmode)
		smaddr1 = (struct nb1_regs *)VS_PHYSCURSOR;
	else
		smaddr1 = (struct nb1_regs *)qmem;
	if(qp->smaddr ) {
	    if( y < qp->min_cur_y || y > qp->max_cur_y )
		y = qp->max_cur_y;
	    if( x < qp->min_cur_x || x > qp->max_cur_x )
		x = qp->max_cur_x;
	    qp->cursor.x = x;		/* keep track of real cursor*/
	    qp->cursor.y = y;		/* position, indep. of mouse*/
	    smaddr1->nb_cur_xpos = XOFFSET + x;
	    smaddr1->nb_cur_ypos = YOFFSET + y;
	}
}



/******************************************************************
 **                                                              **
 ** Routine to scroll.                                           **
 **                                                              **
 ******************************************************************/

smscroll()
{
	register struct sm_info *qp = sm_scn;
	int	thirtylines = 30*1920;


/*
 * If the mouse is on we don't scroll so that the bit map remains sane.
 */

	if( sm_mouseon ) {
	    qp->row = 0;
	    return;
	}

/* First copy 30 lines starting from the second line on the screen to the first
 * line on the screen. (we are moving 30 lines up one line)
 */
	bcopy(qp->bitmap+1920,qp->bitmap,thirtylines);

/* Do the same thing for the rest of the lines on the screen */

	bcopy(qp->bitmap+1920+thirtylines,qp->bitmap+thirtylines,(qp->row - 30)* 1920);

/* Now zero out the last two lines */

	bzero(qp->bitmap+(qp->row*1920),3840);
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
extern	(*vs_gdselect)();
extern	(*vs_gdkint)();
extern	(*vs_gdioctl)();
extern	(*vs_gdstop)();

smcons_init()
{

        sm_setup();
	v_consputc = smputc;
	v_consgetc = smgetc;
	vs_gdopen = smopen;
	vs_gdclose = smclose;
	vs_gdselect = smselect;
	vs_gdkint = smkint;
	vs_gdioctl = smioctl;
	vs_gdstop = smstop;
	ws_display_type = SM_DTYPE;	/* Identify SM as display device */
	ws_display_units = 1;		/* Say unit 0 present */
}



/******************************************************************
 **                                                              **
 ** Routine to do the board specific setup.                      **
 **                                                              **
 ******************************************************************/

sm_setup()
{
	register struct nb_regs *smaddr = (struct nb_regs *)nexus;
	register struct nb1_regs *smaddr1 = (struct nb1_regs *)qmem;
	register struct nb2_regs *smaddr2 = (struct nb2_regs *)nmem;
	register struct sm_info *qp;
	char	*smmem;

/*
 * Set the line parameters on SLU line 0 for
 * the LK201 keyboard: 4800 BPS, 8-bit char, 1 stop bit, no parity.
 */
	smaddr->sslpr = (SER_RXENAB | SER_KBD | SER_SPEED | SER_CHARW);
/*
 * Initialize the screen
 *
 */

	sm_scrn_init(smaddr1);
	cur_reg |= (HSHI | ENPA | ENPB);
	smaddr1->nb_cur_cmd = cur_reg;
	smmem = (char *)((u_long)smaddr2);
	if ((long)smmem != ((long)smmem & ~0x7))
	    printf("smmem: %x not longword aligned\n",smmem);
        sm_scn = (struct sm_info *)((u_int)smmem + 125*1024);
	qp = sm_scn;

	sminit_scn_def();
        *sm_scn = sm_scn_defaults;
 	qp->bitmap = smmem;
        qp->smaddr = smaddr;
        qp->cursorbits = (short *)((u_int)smmem + 127*1024);

/*
 * Set a flag to indicate we are using VR260
 */

	monochrome = 1;

/*
 * Initialize the graphic device
 *
 */

	sm_init();

/*
 * Initialize the mouse
 *
 */
	sm_mouse_init();

}



/******************************************************************
 **                                                              **
 * Routine to initialize the screen.                             **
 **                                                              **
 ******************************************************************/

sm_init()
{
	register int i;
	register char *ptr;
	register struct sm_info *qp = sm_scn;

/*
 * Clear the bit map
 */

	for( i=0 , ptr = qp->bitmap ; i<60 ; i++ , ptr += 2048)
	    bzero( ptr, 2048 );

/*
 * Home the cursor
 */

	qp->row = qp->col = 0;

/*
 * Load the cursor with the default values
 *
 */

	sm_load_cursor(def_cursor);


/*
 * Reset keyboard to default state.
 */

	sm_key_out(LK_DEFAULTS);
}



smreset()
{
}




/******************************************************************
 **                                                              **
 * Routine to initialize the screen.                             **
 **                                                              **
 ******************************************************************/

sm_scrn_init(smaddr1)
struct nb1_regs *smaddr1;
{

	register int	i;

	DELAY(100000);  /* wait  */

	i = FOPB | VBHI;
	smaddr1->nb_cur_cmd = i;
}




/******************************************************************
 **                                                              **
 * Routine to initialize the mouse.                              **
 **                                                              **
 ******************************************************************/

/*
 * NOTE:
 *	This routine communicates with the mouse by directly
 *	manipulating the VAXstar SLU registers. This is allowed
 *	ONLY because the mouse is initialized before the system
 *	is up far enough to need the SLU in interrupt mode.
 */

int	sm_getc();

sm_mouse_init()
{

	register struct nb_regs *ssaddr = (struct nb_regs *)nexus;
	register int	lpr;
	int	i;
	int	status;
	char	id_byte;


/*
 * Set SLU line 1 parameters for mouse communication.
 */
	lpr = SER_POINTER | SER_CHARW | SER_PARENB | SER_ODDPAR
		| SER_SPEED | SER_RXENAB;
	ssaddr->sslpr = lpr;

/*
 * Perform a self-test
 */
	sm_putc(SELF_TEST);
/*
 * Wait for the first byte of the self-test report
 *
 */
	status = sm_getc();
	if (status < 0) {
	    mprintf("\nsm: Timeout on 1st byte of self-test report\n");
	    goto OUT;
	}
/*
 * Wait for the hardware ID (the second byte returned by the self-test report)
 *
 */
	id_byte = sm_getc();
	if (id_byte < 0) {
	    mprintf("\nsm: Timeout on 2nd byte of self-test report\n");
	    goto OUT;
	}
/*
 * Wait for the third byte returned by the self-test report)
 *
 */
	status = sm_getc();
	if (status != 0) {
	    mprintf("\nsm: Timeout on 3rd byte of self-test report\n");
	    goto OUT;
	}
/*
 * Wait for the forth byte returned by the self-test report)
 *
 */
	status = sm_getc();
	if (status != 0) {
	    mprintf("\nsm: Timeout on 4th byte of self-test report\n");
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
	sm_putc(INCREMENTAL);

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

sm_getc()
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

sm_putc(c)
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

sm_key_out( c )
char c;
{
	register struct sm_info *qp = sm_scn;
	register struct nb_regs *ssaddr;
	register int timo = 30000;
	int s;
	int tcr, ln;
	int int_msk;

	if(qp->smaddr == 0)
		return;

	if(sm_physmode)
		ssaddr = (struct nb_regs *)VS_PHYSNEXUS;
	else
		ssaddr = (struct nb_regs *)nexus;

	if(sm_physmode == 0) {
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
	if(sm_physmode == 0) {
		if (int_msk & SINT_ST)
			ssaddr->nb_int_msk |= SINT_ST;
		splx(s);
	}
}




/******************************************************************
 **                                                              **
 ** Routine to load the cursor sprite pattern.                   **
 **                                                              **
 ******************************************************************/

/*
 * 3/5/89 -- Fred Canter
 * Bug fix for stray interrupts thru SCB vector 0x1fc.
 * Removed waitef routine and moved the code in-line because
 * waitef was only called from sm_load_cursor.
 * Removed clear of video EOF interrupt request bit.
 */

int	sm_waitef_on = 0;	/* If 1, sync cursor load with vert. EOF intr */

sm_load_cursor(cur)
unsigned short cur[32];
{

	register struct nb1_regs *smaddr1 = (struct nb1_regs *)qmem;
	register struct nb_regs *smaddr = (struct nb_regs *)nexus;
	register int	i;
	int j;

/*	waitef();	*/
/*	smaddr->nb_int_reqclr = SINT_VF;	*/
	if (sm_waitef_on) {
	    for (j = 1000000; j > 0; --j) {
		if (smaddr->nb_int_reqclr & SINT_VF)
		    break;
	    }
	    if (j <= 0) {
		mprintf("\nsm: timeout polling for end-of-frame interrupt\n");
	    }
	}
	cur_reg |= LODSA;
	smaddr1->nb_cur_cmd = cur_reg;
	for (i = 0; i < 32; ++i)
	    smaddr1->nb_load = cur[i];
	cur_reg &= ~LODSA;
	smaddr1->nb_cur_cmd = cur_reg;
}
#endif
