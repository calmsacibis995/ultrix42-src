#ifndef lint
static char *sccsid = "@(#)gx.c	4.8      (ULTRIX)  4/4/91";
#endif lint

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
 * 03-Apr-91	Sam Hsu
 *	Fix tablet position bug - when cursor pattern changes, position
 *	changes even though pointer not moved.  Needs related server
 *	change (pass hotspot coords to driver).
 *
 * 06-Mar-91	Sam Hsu
 *	EdG's "loosing events" fix - move eTail=i into loop checking
 *	pointer buttons (tablet and mouse).  JoelG's hack for disappearing
 *	cursor problem with Bt459 (turn on with each motion event).
 *
 * 06-Dec-90	Randall Brown
 *	Reworked how commands are sent to the keyboard so that the ioctl()
 *	sleeps after it has sent a mode change command until an
 *	ack has been returned from the keyboard. 
 *
 * 16-Jul-90	Randall Brown
 *	Fixed bug in keyboard interrupt routine that was hardwired to dc device.
 *
 *  4-Jul-90	Randall Brown
 *	Removed all hardwired references to dc device.  Now uses xcons
 *	driver for alternate console.
 *
 *   28-Jun-90 Sam Hsu
 *	Another OOPS.  Closing dev when saver is on doesn't turn saver off.
 *
 *   01-May-90 Sam Hsu
 *	OOPS.  Fix plane-0 VDAC ID readback for 8-plane (mask off junk).
 *	Reflect change in planemask also (note difference b/t 2da/low-3da!).
 *	Do not hold spltty for cursor load *and* readback/reloads.  Release
 *	spl and reacquire as needed.  Dynamically select fg/bg each time
 *	colormap reloaded.  Fix \n!\r while server running bug.
 *
 *   19-Apr-90 Sam Hsu
 *	Use GX_CONSOLE debug level for serial line shadow output.  Replace
 *	HWDEBUG with GX_MULTIOPEN for multiple opens on /dev/mouse.
 *
 *   04-Apr-90 Sam Hsu
 *	Careful not to lower spl if can be callable from elsewhere in kernel.
 *	KB cmds made atomic.  Pick up tablet fixes from cfb.  Switch cursor
 *	fg/bg yet again due to fix in RealizeCursor in GX server.
 *	
 *   28-Mar-90 Sam Hsu
 *	Swap fg/bg of cursor color.  Add gx_check_vdac to read back
 *	vdac cmd registers & check for corruption.  Screen saver must
 *	save/restore cmap[].  Load cmap[] with standard black[0]/
 *	white[1..255] values. Don't allow colormap loads to vdac while
 *	screen saver is on.  Check for 8/24-plane low/mid-3D.  Single
 *	pixel readspans doesn't seem to work too well - change to line.
 *
 *   07-Mar-90 Sam Hsu
 *	VDAC reset now available for both platforms.  Collect checks
 *	for "am I server?" and "server running?" into macros.  Fix
 *	blitc() to use _TEXT_{WIDTH|HEIGHT} when drawing.  Add panic
 *	check to putc().  SLU3_DEV for output to serial line only and
 *	don't write to graphics console when server is running (check
 *	return value from getPacket()).
 *
 *   26-Feb-90 Sam Hsu
 *	Maybe I should use opague stipple for the fonts?  Yes.
 *	Collect more common code (gx_ioctl).  Add fix for vdac
 *	reset (needs corresponding fix in ga/gq before it can be
 *	turned on).  Remove a bunch of unnecessary spltty()'s.
 *
 *   29-Jan-90 Sam Hsu
 *	Dynamic debug levels.  Reinstate mouse cursor as console
 *	text cursor in prep for move to Firefox console font.  Load
 *	default cursor with text cursor pattern.  Clean up cursor
 *	load (redundant setaddr's).  Remove init_vdac from init_
 *	colormap.  Fix cursor readback to mask off high-byte(s).
 *	Would be nice if debug level could be turned on by "area"
 *	selectively, too.
 *
 *   25-Jan-90 Sam Hsu
 *	Change cursor update to blast entire pattern until it comes
 *	out right, or we give up.  Add some debugging to mouse events.
 *
 *   19-Dec-89 Sam Hsu
 *	Some changes to previous change.  Can't use gxo field,
 *	since it may be a process virtual address.  Move into
 *	this file: gx_init_stic(_gx_stic) and use STIC's Stamp
 *	space to set Stamp configuration blah blah blah...
 *	Code to figure out how many planes of Z are out there.
 *	More linkage vectors to allow specific drivers to do some
 *	action at certain times (eg, open, close, init).
 *
 *   11-Dec-89 Paul Jensen
 *	Support DEVIOCGET ioctl().  Don't wait forever for
 *	cursor to be loaded correctly.
 *
 *   11-Dec-89 Paul Jensen
 *	Support new Bt459 interface.  Support multiple
 *	levels of FDEBUG.
 *
 *   30-Nov-89 Sam Hsu
 *	$Header: /nfs/kodak/local/4.0-12/io/tc/RCS/gx.c,v 1.39 90/05/15 17:22:20 fhsu Exp $
 *	Created.  Common routines for ga/gq drivers.
 *
 *   11-Jul-89 Randall P. Brown
 *		Changed all the sc->dc_tty to dc_tty.
 *
 *    9-Dec-88 Randall P. Brown
 *		Moved the check for scroll before printing each character
 * 		instead of after, and changed max_cols to be 80.  This 
 * 		fixes the problem of the 81st char on a line missing.
 *		When the keyboard is closed, the flags for shift and control
 * 		are cleared.  This was to fix the problem of the control
 *		key being stuck on shutdown.
 *
 *    1-Dec-88 Vasudev K. Bhandarkar
 *		
 *	       Let VM determine start of shared memory location.
 *	       Set start of shared memory location to 0.  Don't need
 *	       tmpva either (temporary virtual address).
 *
 *   23-Nov-88 Randall P. Brown
 *
 *		Fixed the flow control on the xconsdev since it is now
 * 		a different device than the console.
 *
 *   17-Nov-88 Randall P. Brown
 *
 *		Changed names of all structs and routines to correspond
 * 		to name changes in dc7085cons.[ch]
 *
 *   17-Nov-88 Vasudev K. Bhandarkar
 *	
 *		Massive cleanup.  Now use only one fontmask array.
 *		Use only one Colormap initialization routine.
 *		Introduce colorization of cursors, one vdac init
 *		routine, one restore color cursor routine, etc, etc.
 *
 *   10-Oct-88 Larry Palmer
 *		Fixed how keyboard resets are done. Call back to
 *		console driver to put characters out (rather than
 *		having a local routine. Also change where init is
 *		done to fix panic of no video simm in system. Couple
 *		of changes from Vasu to scroll correctly on startup.
 *
 *   3-Oct-88  Vasudev K. Bhandarkar
 *	
 *		Home the cursor at the bottom of the screen.  So no
 *	        console messages are lost at boot time.  Do not clear
 *		the screen at device open and close.  devicenames are
 *		set to COLOR and MONO for the file command to recognize
 *	    	the device.
 *
 *   1-Oct-88  Ricky Palmer
 *	       
 *	        dcsc is now kmalloced.  So we have to have it declared as
 *		extern.  dcgetc is now recursive.  Change pmgetc to reflect
 *		this. See dc7085.c for dcgetc.  
 *
 *  25-Sep-88  Larry Palmer
 *
 *		Rewrite of how Xcons is done. The tty used for xconsdev is
 *		now the LAST tty structure in the console driver (stored in
 *		dc_softc). The other way was ugly and caused pmselect to
 *		reproduce part of the select code. This way the driver just
 *		uses the device specified (0,4 for pmax) and everything lines
 *		up. Also removed 'flow control' for xconsdev. There's no way
 *		this works as xconsdev is not tied to real hardware.
 *
 *  22-Sep-88  Vasudev K. Bhandarkar
 *	       
 *	       Stronger keyboard initialization to unwedge the keyboard
 *	       if power-up self-test fails.  Console width is now 81 
 *	       characters to account for an extra character for LF.
 *	       pmselect fixed to account for selects done on the xcons(0,4)
 *	       device.
 *
 *  16-Sep-88  Vasudev K. Bhandarkar
 *	       
 *	       Bug fix..  For the color device, console error messages
 *	       were appearing black on black, hence unreadable.  When
 * 	       X is running, pixels 0 and 1 are chosen as bg and fg for
 *	       console error messages.  Also initialize the color map
 *	       at open and close.
 *
 *   6-Sep-88  Vasudev K. Bhandarkar
 *	       
 *	       Several changes.  All driver routines and data structs
 *	       are now pm not sm.  A lot of unused globals have been
 *	       deleted.  Most importantly, a new global has been 
 *	       introduced.  This variable is now needed because the
 *	       shared memory is not working as advertised.  The kernel
 *	       cannot access it.  When this gets fixed, the frame buffer
 *	       variable can be reset to qp->bitmap.
 *
 *  30-Aug-88  Vasudev K. Bhandarkar
 *
 *	       Bug fix.  Xmfbpmax/Xcfbpmax now run at first invocation.
 *	       Deleted spurious local declaration of tcs that conflicted
 *	       with the global declaration.
 *
 *  22-Aug-88  Vasudev K. Bhandarkar
 *
 *	       Merge four ioctls into one.  map info, events, tcs and 
 *	       bitmap with one ioctl.  Assign one more page for the
 *	       plane mask.
 *	      
 *  17-Aug-88  Richard Hyde/Vasudev K. Bhandarkar
 *	       
 *	       Color Map initialization and fast scrolling. Ioctls
 *	       for color map setting.
 *
 *  12-Aug-88  Vasudev K. Bhandarkar/Gregory Depp
 *
 *	       Implemented ioctls for using shared memory.  discard
 *             Rich's "ifdef SLIME" code.
 *
 *  10-Aug-88  Vasudev K. Bhandarkar
 *
 *	       Console Emulation, pm_blitc, pm_putc, pm_scroll
 *
 *   4-Aug-88  Vasudev K. Bhandarkar
 *
 *	       Cursor Motion is possible.  Test ioctls to make cursor
 *	       motion possible
 *
 *   3-Aug-88  Rich Hyde/Phil Karlton
 *	       
 *	       Make mouse pointer visible.
 *             
 *   2-Aug-88  Richard Hyde
 *	
 *	       Stopgap shared memory interface.  Until the real system V
 *	       shared memory gets installed.
 *
 *   1-Aug-88  Vasudev K. Bhandarkar
 *	  
 *	       Mouse and Keyboard self-test routines.
 *
 * 31-July-88  Vasudev K. Bhandarkar
 *
 *	       Dummy mouse and keyboard interrupt service called from
 *	       the serial line driver.
 *
 * 29-July-99  Vasudev K. Bhandarkar
 *             
 *             Map system space.  Initialize frame buffer through pm_init.
 *	       Monomap initialization.
 *
 * 28-July-99  Vasudev K. Bhandarkar/Richard Hyde
 *	
 *	       Probe, Attach, cons_init.  
 * 
 * 19-July-88  Vasudev K. Bhandarkar
 * 
 *	       Created.  Based on the qv, qd and sm drivers.
 *
 ************************************************************************/
#define _GX_C_

#include "../data/gx_data.c"

static short gx_divdefaults[15] = 
    {
	LK_DOWN,		/* 0 doesn't exist */
	LK_AUTODOWN, 
	LK_AUTODOWN, 
	LK_AUTODOWN, 
	LK_DOWN,
	LK_UPDOWN,   
	LK_UPDOWN,   
	LK_AUTODOWN, 
	LK_AUTODOWN, 
	LK_AUTODOWN, 
	LK_AUTODOWN, 
	LK_AUTODOWN, 
	LK_AUTODOWN, 
	LK_DOWN, 
	LK_AUTODOWN 
	};

static  short gx_kbdinitstring[] = { /* reset any random keyboard stuff */
	LK_AR_ENABLE,		     /* we want autorepeat by default */
	LK_CL_ENABLE,		     /* keyclick */
	0x84,			     /* keyclick volume */
	LK_KBD_ENABLE,		     /* the keyboard itself */
	LK_BELL_ENABLE,		     /* keyboard bell */
	0x84,			     /* bell volume */
	LK_LED_DISABLE,		     /* keyboard leds */
	LED_ALL };
#define KBD_INIT_LENGTH	sizeof(gx_kbdinitstring)/sizeof(short)

static int gx_bt459CR1[] = {
    0xc0c0c0,			     /* cmd reg 0 */
    0x000000,			     /* cmd reg 1 */
    0xc2c2c2,			     /* cmd reg 2 - X-windows cursor */
    0xffffff,			     /* pix rd msk */
    0x000000,			     /* reserved */
    0x000000,			     /* pix blink msk */
    0x000000,			     /* reserved */
    0x000000,			     /* ovrly rd msk */
    0x000000,			     /* ovrly blink msk */
    0x000000,			     /* interleave */
    0x000000			     /* test */
    };
#define BT459CR1_SIZE	(sizeof(gx_bt459CR1)/sizeof(int))

/*
 * Indexed by option field of %MODCL STIC register.
 */
static char *devgetstr[] = {		/* as per gringorten */
    "3M_2D",
    "3M_1?",
    "3M_3D",				/* 3DA_LOW */
    "3M_3D",				/* 3DA_LOWZ */
    "3M_3D",				/* 3DA_MID */
    "3M_3D",				/* 3DA_MIDZ */
    "3M_6?",
    "3M_3D"				/* 3DA_HI */
    };


/******************************************************************
 **                                                              **
 ** Routine to open the graphic device.                          **
 **                                                              **
 ******************************************************************/
/*ARGSUSED*/
gxopen(dev, flag)
    dev_t dev;
{
    register int unit = minor(dev);
    register struct tty *tp;

    if ((consDev & GRAPHIC_DEV) == 0)
	return(ENODEV);
    /*
     * Only one "server" (debugging code allows multiple open's, but only
     * one "server" - identified by O_NDELAY open flag).
     *
     * nb: when multiple open()s are allowed, the device is reference
     *	   counted, so close() only gets called when the last process
     *	   dies!
     */
    if (unit == 1) {

#	ifdef GX_MULTIOPEN
	if ((flag & O_NDELAY) == 0) {
	    register int i = gx_info_get(1);
	    if (i < 0) {
		return(EBUSY);	     /* no slots left */
	    }
	    gx_infos[i].pid = u.u_procp->p_pid;

	    GX_DEBUG(GX_GAB,
		     gx_printf("gxopen: slave pid %d\n", u.u_procp->p_pid);
		     );

	    GX_CALL(_gx_open)(dev, flag);

	    return(0);
	}
	else
#	endif gx_multiopen
	{
	    if (gx_openflag) {
#	        ifdef GX_MULTIOPEN
		if (gx_info_gone(gx_server.pid)) {
		    gx_init();	     /* didn't get done yet... */
		}
		else
#		endif gx_multiopen
		{
		    GX_DEBUG(GX_GAB,
			     gx_printf("gxopen: server pid %d exists\n",
				       gx_server.pid);
			     );
		    return (EBUSY);
		}
	    }
	    gx_openflag = 1;
	    gx_serverp = u.u_procp;
	    if (gx_server.pid != gx_serverp->p_pid)
		gx_server.shmat = 0;
	    gx_server.pid = gx_serverp->p_pid;

	    GX_DEBUG(GX_GAB,
		     gx_printf("gxopen: server pid %d\n", gx_serverp->p_pid);
		     );
	}
	gx_dev_inuse |= GRAPHIC_DEV; /* graphics dev is open */
	gx_init_colormap();

	tp = &slu.slu_tty[unit];
	ttychars(tp);
	tp->t_state = TS_ISOPEN|TS_CARR_ON;
	tp->t_cflag = tp->t_cflag_ext = B4800;
	tp->t_iflag_ext = 0;
	tp->t_oflag_ext = 0;
	tp->t_lflag_ext = 0;

	gx_mouseon = 1;


	/*
	 * set up event queue for later
	 */
	gx_config(gxp);

	GX_CALL(_gx_open)(dev, flag);

	return(0);
    }
	
    gx_dev_inuse |= CONS_DEV;    /* mark console as open */

    /* set tp to the address of the console tty struct */
    tp = &cdevsw[0].d_ttys[0];
    if (tp->t_state&TS_XCLUDE && u.u_uid!=0)
	return (EBUSY);
    tp->t_addr = (caddr_t)tp;
    tp->t_oproc = gxstart;

    /*
     * Look at the compatibility mode to specify correct 
     * default parameters and to insure only standard specified 
     * functionality.
     */
    if ((u.u_procp->p_progenv == A_SYSV) || 
	(u.u_procp->p_progenv == A_POSIX)) {
	flag |= O_TERMIO;
	tp->t_line = TERMIODISC;
    }
#   ifdef O_NOCTTY
    /*
     * Set state bit to tell tty.c not to assign this line as the 
     * controlling terminal for the process which opens this line.
     */
    if ((flag & O_NOCTTY) && (u.u_procp->p_progenv == A_POSIX))
	tp->t_state |= TS_ONOCTTY;
#   endif O_NOCTTY
    if ((tp->t_state&TS_ISOPEN) == 0) {
	ttychars(tp);
	tp->t_state = TS_ISOPEN|TS_CARR_ON;
	tp->t_cflag = tp->t_cflag_ext = B4800;
	tp->t_iflag_ext = 0;
	tp->t_oflag_ext = 0;
	tp->t_lflag_ext = 0;
	/*
	 * Ultrix defaults to a "COOKED" mode on the first
	 * open, while termio defaults to a "RAW" style.
	 * Base this decision by a flag set in the termio
	 * emulation routine for open, or set by an explicit
	 * ioctl call. 
	 */
	if ( flag & O_TERMIO ) {
	    /*
	     * Provide a termio style environment.
	     * "RAW" style by default. 
	     */
	    tp->t_flags = RAW;   
	    tp->t_iflag = 0;
	    tp->t_oflag = 0;
	    tp->t_cflag |= CS8|CREAD|HUPCL; 
	    tp->t_lflag = 0;
	    
	    /*
	     * Change to System V line discipline.
	     */
	    tp->t_line = TERMIODISC;
	    
	    /*
	     * The following three control chars have 
	     * different default values than ULTRIX.	
	     */
	    tp->t_cc[VERASE] = '#';
	    tp->t_cc[VKILL] = '@';
	    tp->t_cc[VINTR] = 0177;
	    tp->t_cc[VMIN] = 6;
	    tp->t_cc[VTIME] = 1;
	    
	} else {
	    /*
	     * Provide a backward compatible ULTRIX 
	     * environment.  "COOKED" style.	
	     */
	    tp->t_flags = IFLAGS;
	    tp->t_iflag = IFLAG;
	    tp->t_oflag = OFLAG;
	    tp->t_lflag = LFLAG;
	    tp->t_cflag |= CFLAG;
	}
    }

    /*
     * Process line discipline specific open.
     */
    return ((*linesw[tp->t_line].l_open)(dev, tp));
}
/* gxopen */


/******************************************************************
 **                                                              **
 ** Routine to close the graphic device.                         **
 **                                                              **
 ******************************************************************/

/*ARGSUSED*/
gxclose(dev, flag)
    dev_t dev;
    int flag;
{
    register struct tty *tp;
    register int unit = minor(dev);

    unit = minor(dev);

    /*
     * If unit is not the mouse call the line disc. 
     * otherwise clear the state
     * flag, and put the keyboard into down/up.
     */
    if( unit == 0 ) {
	tp = &cdevsw[0].d_ttys[0];
	(*linesw[tp->t_line].l_close)(tp);
	ttyclose(tp);
	gx_dev_inuse &= ~CONS_DEV;
	gx_keyboard.cntrl = gx_keyboard.shift = 0;
	tp->t_state = 0;
	
	/* Remove termio flags that do not map */
	tp->t_iflag &= ~TERMIO_ONLY_IFLAG;
	tp->t_oflag &= ~TERMIO_ONLY_OFLAG;
	tp->t_cflag &= ~TERMIO_ONLY_CFLAG;
	tp->t_lflag &= ~TERMIO_ONLY_LFLAG;
    } else {
#	ifdef GX_MULTIOPEN
	register int i;
	/* garbage collect all entries now... */
	for (i = 1; i < GX_MAX_INFOS; i++) {
	    if (gx_infos[i].pid == -1)
		continue;
	    if (   gx_infos[i].pid == u.u_procp->p_pid
		|| gx_info_gone(gx_infos[i].pid))
	    {
		GX_DEBUG(GX_GAB,
			 gx_printf("gxclose: slave pid %d\n", gx_infos[i].pid);
			 );
		gx_infos[i].pid = -1;
		gx_infos[i].shmat = 0;
	    }
	}
#	endif gx_multiopen
	if (gx_openflag != 1) {
	    GX_DEBUG(GX_GAB,
		     gx_puts("gxclose: no server\n");
		     );
	    GX_CALL(_gx_close)(dev, flag);
	    return(EBUSY);
	}
	GX_DEBUG(GX_GAB,
		 gx_printf("gxclose: server %d\n", gx_server.pid);
		 );
	gx_openflag = 0;	     /* mark the graphics device available */
	gx_mouseon = 0;
	GX_CALL(_gx_close)(dev, flag);
	gx_serverp = 0;
	gx_server.pid = -1;
	gx_server.shmat = 0;
	gx_dev_inuse &= ~GRAPHIC_DEV;
	gx_init();
	/*
	 * Prevent past text from getting clobbered
	 */
	gx_scroll(0);
    }
    return(0);			     /* fhsu */
}
/* gxclose */


/******************************************************************
 **                                                              **
 ** Mouse activity select routine.                               **
 **                                                              **
 ******************************************************************/

gxselect(dev, rw)
    dev_t dev;
{
    register int unit = minor(dev);
    register int s;

    IPLTTY(s);

    if( unit == 1 )
    {
#	ifdef GX_MULTIOPEN
	/* should never happen when GX_MAX_INFOS==1 */
	if (!GX_IAMSERVER)
	    return(EACCES);
#	endif gx_multiopen
	switch(rw)
	{
	 case FREAD:		/* if events okay */
	    if (gxp->qe.eHead != gxp->qe.eTail) {
		splx(s);
		return(1);
	    }
	    gx_rsel = u.u_procp;
	    splx(s);
	    return(0);

	 case FWRITE:		/* can never write */
	    splx(s);
	    return(EACCES);
	}
    } else {
	splx(s);
	return( ttselect(dev, rw) );
    }
}
/* gxselect */

gx_ack_mode_change()
{
    if (gx_mode_change) {
	gx_mode_change = 0;
	wakeup(&gx_mode_change);
    }
}

/******************************************************************
 **                                                              **
 ** Graphic device ioctl routine.                                **
 **                                                              **
 ******************************************************************/

/*ARGSUSED*/
gxioctl(dev, cmd, data, flag)
    dev_t dev;
    register caddr_t data;
{
    register struct tty *tp;
    register int unit = minor(dev);
    register gxKPCmd *qk;
    register unsigned char *cp;
    register int s, i;
    int error;
    struct devget *devget;

#   ifdef GX_MULTIOPEN
    /* nb: this may be removed when !defined(GX_MULTIOPEN) since only 1 process
     * may open the graphics device then.
     */
    if (!GX_IAMSERVER) {
	switch (cmd)
	{
	 case QIOCGXSTATE:
	 case QIOCKPCMD:
	 case QIOKERNLOOP:
	 case QIOKERNUNLOOP:
	 case QIO_WCCOLOR:
	 case QIO_WCURSOR:
	 case QIO_SETCMAP:
	    printf("Illegal operation on graphics device:\n");
	    printf("    pid %d not server pid %d\n",
		    u.u_procp->p_pid, (gx_serverp ? gx_serverp->p_pid : -1));
	    return(-1);
	 default:
	    break;
	}
    }
#   endif gx_multiopen
    
    switch( cmd ) 
    {
     case QIOCGINFO:		     /* return screen info */
	i = gx_info_get(0);
	if (i < 0 || gx_infos[i].pid != u.u_procp->p_pid)
	    panic("gxioctl - QIOCGINFO");

	if (gx_infos[i].shmat) {
	    *(gxInfo **)data = gx_infos[i].shmat;
	    break;
	}

	if ((i = (*_gx_ioctl)(makedev(0,i), cmd, data, flag)) > 0)
	{
	    printf ("Could not map shared data structure: %s\nu_error = %d\n",
		     gx_err_msg[i], u.u_error);
	    *(gxInfo **)data = 0;
	    return (-1);
	}
	break;

     case QIOCGXSTATE:		     /* set mouse position */
	gxp->mouse = *((pmCursor *)data);
	gx_pos_cur( (int)(gxp->mouse).x, (int)(gxp->mouse).y );
	break;

     case QIOCINIT:		     /* init screen	*/
	if (GX_IAMSERVER)
	    gx_init();		     /* nb: this halts the N10!!! */
	else
	    gx_init_stic();	     /* non-destructive */
	break;

     case QIOCKPCMD:
	qk = (gxKPCmd *)data;
	if (qk->nbytes == 0) qk->cmd |= 0200;
	if (gx_mouseon == 0) qk->cmd |= 1; /* no mode changes */
	IPLTTY(s);
	(*slu.kbd_putc)(qk->cmd);
#	ifdef notdef
	cprintf ("qk->cmd= %x\n", qk->cmd);
#	endif
	cp = &qk->par[0];
	while (qk->nbytes-- > 0) 
	{			     /* terminate parameters */
	    if (qk->nbytes <= 0) *cp |= 0200;
#	    ifdef notdef
	    cprintf ("nbyte [%d]= %x\n", qk->nbytes, *cp);
#	    endif
	    (*slu.kbd_putc)(*cp++);
	}
	/* low bit clear if a mode change command */
	if ((qk->cmd & 0x1) == 0) {
	    gx_mode_change = 1;
	    /* set timer in case ack never comes in */
	    timeout(gx_ack_mode_change, (caddr_t)0, hz);
	    sleep(&gx_mode_change, TTIPRI);
	}
	splx(s);
	break;

     case QIOCADDR:		     /* get struct addr */
	i = gx_info_get(0);
	if (i < 0 || gx_infos[i].pid != u.u_procp->p_pid)
	    panic("gxioctl: gx_info_get(QIOCADDR)");
	*(gxInfo **) data = (gxInfo *)svtophy(&gx_infos[i].info);
	break;

     case QIO_WCURSOR:
	return gx_load_cursor();

     case QIO_WCCOLOR:
	gx_load_ccolor();
	break;
	     
     case QIO_SETCMAP:
	if (gxp->cmap_index > 255 ||
	    gxp->cmap_count > 256 ||
	    gxp->cmap_index + gxp->cmap_count > 256)
	{
	    u.u_error = EINVAL;
	    return (-1);
	}
	IPLTTY(s);
        gxp->flags |= GX_F_NEW_CMAP;
	splx(s);
	/* load with reckless abandon ... */
	gx_load_colormap();
	break;

     case QIOKERNLOOP:			/* redirect kernel console output */
	xcons_kern_loop = -1;
	break;

     case QIOKERNUNLOOP:		/* dont redirect kernel cons output */
	xcons_kern_loop = 0;
	break;

     case QIOVIDEOON:			/* display on	*/
	gx_video_on();
	break;

     case QIOVIDEOOFF:			/* display off	*/
	gx_video_off();
	break;

     case DEVIOCGET:			/* device status */
	devget = (struct devget *)data;
	bzero(devget,sizeof(struct devget));
	devget->category = DEV_TERMINAL; /* terminal cat.*/
	devget->bus = DEV_NB;		/* NO bus	*/
	bcopy(DEV_VS_SLU,devget->interface,
	      strlen(DEV_VS_SLU));	/* interface	*/
	if((unit == 0) || (unit == 1))
	{
	    bcopy(gx_devtype(), devget->device, 6); /* Ultrix "gq"*/
	}
	if(pointer_id == MOUSE_ID)
	{
	    bcopy(gx_devtype(), devget->device, 6); /* Ultrix "gq"*/
	}
	else if(pointer_id == TABLET_ID)
	    bcopy(DEV_TABLET, devget->device, strlen(DEV_TABLET));
	else
	    bcopy(DEV_UNKNOWN, devget->device, strlen(DEV_UNKNOWN));
	devget->adpt_num = 0;		/* NO adapter	*/
	devget->nexus_num = 0;		/* fake nexus 0	*/
	devget->bus_num = 0;		/* NO bus	*/
	devget->ctlr_num = 0;		/* cntlr number */
	devget->slave_num = unit;	/* line number 	*/
        bcopy(gx_devtype(), devget->dev_name, 6); /* Ultrix "gq" */
	devget->unit_num = unit;	/* dc line?	*/
	/* TODO: should say not supported instead of zero!	*/
	devget->soft_count = 0;		/* soft err cnt */
	devget->hard_count = 0;		/* hard err cnt */
	devget->stat = 0;		/* status	*/
	devget->category_stat = 0;	/* cat. stat.	*/
	break;

      default:
	i = (*_gx_ioctl)(dev, cmd, data, flag);
	/* expected result: a-ok */
	if (i == 0)
	    break;
	/* error encountered - print message and return error */
	if (i > 0)
	{
	    printf("gxioctl: %s\nu_error = %d", gx_err_msg[i], u.u_error);
	    return (-1);
	}
	/* unsupported or ignored - not ours??? */
	tp = &slu.slu_tty[unit];
	error = (*linesw[tp->t_line].l_ioctl)(tp, cmd, data, flag);
	if (error >= 0)
	    return (error);
	error = ttioctl(tp, cmd, data, flag);
	if (error >= 0) 
	    return (error);
	break;
    }
    return (0);
}
/* gxioctl */


/******************************************************************
 **                                                              **
 ** Graphice device interrupt routine.                           **
 **                                                              **
 ******************************************************************/

#define CHAR_S	0xc7
#define CHAR_Q	0xc1

gxkint(ch)           /* XXX this has to run at spltty or lower!!! */
    register int ch;
{
    register pmEvent *qep;
    struct mouse_report *new_rep;
    struct tty *tp;
    register int unit;
    register u_short c;
    register int i, j;
    u_short data;
    int	cnt;
    static char temp, old_switch, new_switch;

    /*
     * Mouse state info
     */
    unit = (ch>>8)&03;
    new_rep = &current_rep;
    tp = &slu.slu_tty[unit];

    /*
     * If graphic device is turned on
     */

    if (gx_mouseon == 1) 
    {
  	cnt = 0;
	while (cnt++ == 0) 
	{
	    /*
	     * Pick up LK-201 input (if any)
	     */
	    if (unit == 0) 
	    {
		data = ch & 0xff;

		if (data == LK_MODE_CHG_ACK) {
		    untimeout(gx_ack_mode_change, (caddr_t)0);
		    gx_ack_mode_change();
		}

		if(data == LK_POWER_ERROR || data == LK_KDOWN_ERROR ||
		   data == LK_INPUT_ERROR || data == LK_OUTPUT_ERROR) 
		{
		    if(!gx_inkbdreset) { /* Ignore errors from reset */
		        mprintf("\ngx0: gxkint: keyboard error,code=%x",data);
		        gx_kbdreset();
		    }
		    return(0);
		}

		if (data < LK_LOWEST) 
		    return(0);

		/* why can't we write this event??? */
		if ((i = EVROUND(gxp->qe.eTail+1)) == gxp->qe.eHead) 
		    return(0);

		/* for any key */
		qep = &gx_events[gxp->qe.eTail];

		qep->type = BUTTON_RAW_TYPE;
		qep->device = KEYBOARD_DEVICE;
		qep->x = gxp->mouse.x;
		qep->y = gxp->mouse.y;
		qep->time = TOY;
		qep->key = data;
		gxp->qe.eTail = i;

		GX_DEBUG(GX_YOW,
			 gx_putevent(qep);
			 );

		if((i=EVROUND(gxp->qe.eTail+1)) == gxp->qe.eHead) 
		    return(0);
		qep = &gx_events[gxp->qe.eTail];

		switch (data)
		{
		 case CNTRL:
		    gx_keyboard.cntrl ^= 0xffff;
		    break;
		 case ALLUP:
		    gx_keyboard.cntrl = gx_keyboard.shift = 0;
		    break;
		 case SHIFT:
		    gx_keyboard.shift ^= 0xffff;
		    break;
		 default:
		    ;
		}
	    }

	    /*
	     * Pick up the mouse input (if any)
	     */
	    if ((unit == 1) && (pointer_id == MOUSE_ID)) 
	    {
	    	/*
		 * see if mouse position has changed
		 */
		if( new_rep->dx != 0 || new_rep->dy != 0) 
		{
		    unsigned int millis = TOY;

		    /*
		     * Check to see if we have to accelerate the mouse
		     *
		     */
		    if (gxp->mscale >=0) 
		    {
			if (new_rep->dx >= gxp->mthreshold)
			    new_rep->dx +=
				(new_rep->dx - gxp->mthreshold)*gxp->mscale;
			if (new_rep->dy >= gxp->mthreshold)
			    new_rep->dy +=
				(new_rep->dy - gxp->mthreshold)*gxp->mscale;
		    }

		    /*
		     * update mouse position
		     */
		    if( new_rep->state & X_SIGN) 
		    {
			gxp->mouse.x += new_rep->dx;
			if( gxp->mouse.x > gxp->max_cur_x )
			    gxp->mouse.x = gxp->max_cur_x;
		    }
		    else 
		    {
			gxp->mouse.x -= new_rep->dx;
			if( gxp->mouse.x < gxp->min_cur_x )
			    gxp->mouse.x = gxp->min_cur_x;
		    }
		    if( new_rep->state & Y_SIGN) 
		    {
			gxp->mouse.y -= new_rep->dy;
			if( gxp->mouse.y < gxp->min_cur_y )
			    gxp->mouse.y = gxp->min_cur_y;
		    }
		    else 
		    {
			gxp->mouse.y += new_rep->dy;
			if( gxp->mouse.y > gxp->max_cur_y )
			    gxp->mouse.y = gxp->max_cur_y;
		    }
		    if( tp->t_state & TS_ISOPEN )
			gx_pos_cur( gxp->mouse.x, gxp->mouse.y );
		    gx_tcs[gxp->qe.tcNext].time = millis;
		    gx_tcs[gxp->qe.tcNext].x = gxp->mouse.x;
		    gx_tcs[gxp->qe.tcNext].y = gxp->mouse.y;
		    if (++(gxp->qe.tcNext) >= MOTION_BUFFER_SIZE)
			gxp->qe.tcNext = 0;
		    if (gxp->mouse.y < gxp->mbox.bottom &&
			gxp->mouse.y >=  gxp->mbox.top &&
			gxp->mouse.x < gxp->mbox.right &&
			gxp->mouse.x >=  gxp->mbox.left) goto mbuttons;
		    gxp->mbox.bottom = 0; /* trash box */
		    if (EVROUND(gxp->qe.eTail+1) == gxp->qe.eHead)
			goto mbuttons;

		    i = EVROUND(gxp->qe.eTail -1);
		    if ((gxp->qe.eTail != gxp->qe.eHead) && (i != gxp->qe.eHead)) 
		    {
		        qep = &gx_events[i];
		        if(qep->type == MOTION_TYPE) 
			{
			    qep->x = gxp->mouse.x;
			    qep->y = gxp->mouse.y;
			    qep->time = millis;
			    qep->device = MOUSE_DEVICE;
			    goto mbuttons;
			}
		    }
		    /*
		     * Put event into queue and do select
		     */
		    qep = &gx_events[gxp->qe.eTail];
		    qep->type = MOTION_TYPE;
		    qep->time = millis;
		    qep->x = gxp->mouse.x;
		    qep->y = gxp->mouse.y;
		    qep->device = MOUSE_DEVICE;
                    gxp->qe.eTail = EVROUND(gxp->qe.eTail+1);

		    GX_DEBUG(GX_YOW,
			     gx_putevent(qep);
			     );
		}

		/*
		 * See if mouse buttons have changed.
		 */
	     mbuttons:

		new_switch = new_rep->state & 0x07;
		old_switch = gx_last_rep.state & 0x07;

		temp = old_switch ^ new_switch;
		if( temp ) 
		{
		    for (j = 1; j < 8; j <<= 1) 
		    {		/* check each button */
			if (!(j & temp)) /* did this button change? */
			    continue;

			/*
			 * Check for room in the queue
			 */
                        if ((i = EVROUND(gxp->qe.eTail+1)) == gxp->qe.eHead) 
			    return(0);

			/* put event into queue and do select */
                        qep = &gx_events[gxp->qe.eTail];

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
			qep->x = gxp->mouse.x;
			qep->y = gxp->mouse.y;
			gxp->qe.eTail = i;
		    }

		    /* update the last report */
		    gx_last_rep = current_rep;
		    gxp->mswitches = new_switch;
		}
	    }			/* Pick up mouse input */

	    else if ((unit == 1) && (pointer_id == TABLET_ID)) 
	    {
		/* tablet coordinates are scaled to match root window */
		new_rep->dx = 
		    ((new_rep->dx * gxp->max_x) / 2200) - /*hotX*/gxp->tablet.x;
		new_rep->dy =
		    (((2200 - new_rep->dy) * gxp->max_y) / 2200)
			- /*hotY*/gxp->tablet.y;

		/* constrain cursor */
		if( new_rep->dx > gxp->max_cur_x )
		    new_rep->dx = gxp->max_cur_x;
		if( new_rep->dx < gxp->min_cur_x )
		    new_rep->dx = gxp->min_cur_x;
		if( new_rep->dy > gxp->max_cur_y )
		    new_rep->dy = gxp->max_cur_y;
		if( new_rep->dy < gxp->min_cur_y )
		    new_rep->dy = gxp->min_cur_y;

		/*
		 * see if the puck/stylus has moved
		 */
		if (gxp->mouse.x != new_rep->dx ||
		    gxp->mouse.y != new_rep->dy) 
		{
		    /*
		     * update cursor position
		     */
		    gxp->mouse.x = new_rep->dx;
		    gxp->mouse.y = new_rep->dy;

		    if( tp->t_state & TS_ISOPEN )
			gx_pos_cur( gxp->mouse.x, gxp->mouse.y );
		    gx_tcs[gxp->qe.tcNext].time = TOY;
		    gx_tcs[gxp->qe.tcNext].x = gxp->mouse.x;
		    gx_tcs[gxp->qe.tcNext].y = gxp->mouse.y;
		    if (++(gxp->qe.tcNext) >= MOTION_BUFFER_SIZE)
			gxp->qe.tcNext = 0;
		    if (gxp->mouse.y < gxp->mbox.bottom &&
			gxp->mouse.y >=  gxp->mbox.top &&
			gxp->mouse.x < gxp->mbox.right &&
			gxp->mouse.x >=  gxp->mbox.left) goto tbuttons;
		    gxp->mbox.bottom = 0; /* trash box */
		    if (EVROUND(gxp->qe.eTail+1) == gxp->qe.eHead)
			goto tbuttons;

		    /*
		     * Put event into queue and do select
		     */
		    qep = &gx_events[gxp->qe.eTail];
		    qep->type = MOTION_TYPE;
		    qep->device = TABLET_DEVICE;
		    qep->x = gxp->mouse.x;
		    qep->y = gxp->mouse.y;
		    qep->key = 0;
		    qep->time = TOY;
		    gxp->qe.eTail = EVROUND(gxp->qe.eTail+1);

		    GX_DEBUG(GX_YOW,
			     gx_putevent(qep);
			     );
		}

		/*
		 * See if tablet buttons have changed.
		 */

	     tbuttons:

		new_switch = new_rep->state & 0x1e;
		old_switch = gx_last_rep.state & 0x1e;

		temp = old_switch ^ new_switch;
		if( temp ) 
		{
		    /* define the changed button and if up or down */
		    for (j = 1; j <= 0x10; j <<= 1) 
		    {			/* check each button */
			if (!(j & temp)) /* did this button change? */
			    continue;

			if((i=EVROUND(gxp->qe.eTail+1)) == gxp->qe.eHead) 
			    return(0);

			/* put event into queue and do select */
			qep = &gx_events[gxp->qe.eTail];

			switch (j) 
			    {
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
			qep->device = TABLET_DEVICE;
			qep->x = gxp->mouse.x;
			qep->y = gxp->mouse.y;
			qep->time = TOY;
			gxp->qe.eTail =  i;
		    }

		    /* update the last report */
		    gx_last_rep = current_rep;
		}
	    }				/* Pick up tablet input */
	}				/* While input available */

	/*
	 * If we have proc waiting, and event has happened, wake him up
	 */
	if(gx_rsel && (gxp->qe.eHead != gxp->qe.eTail)) 
	{
	    GX_DEBUG(GX_YOW,
		     gx_puts("gxkint: select wakeup\n");
		     );
	    selwakeup(gx_rsel,0);
	    gx_rsel = 0;
	} else if (gxp->qe.eHead != gxp->qe.eTail) {
	    ttwakeup(tp);
	}
    }
    else 
    {
	/*
	 * If the graphic device is not turned on, this is console input
	 */

	/*
	 * Get a character from the keyboard.
	 */
	if (unit == 0) 
	{
	    register int s;

	    data = ch & 0xff;
	    /*
	     * Check for various keyboard errors
	     */
	    if (data == LK_POWER_ERROR || data == LK_KDOWN_ERROR ||
		data == LK_INPUT_ERROR || data == LK_OUTPUT_ERROR) 
	    {
		if(!gx_inkbdreset) { /* gets recursive */
		    mprintf("gx0: Keyboard error, code = %x\n",data);
		    gx_kbdreset();
		}
		return(0);
	    }
	    if( data < LK_LOWEST ) 
		return(0);

	    /*
	     * See if its a state change key
	     */

	    switch ( data ) 
	    {
	     case LOCK:
		gx_keyboard.lock ^= 0xffff; /* toggle */
		IPLTTY(s);
		if( gx_keyboard.lock )
		    (*slu.kbd_putc)( LK_LED_ENABLE );
		else
		    (*slu.kbd_putc)( LK_LED_DISABLE );
		(*slu.kbd_putc)( LED_3 );
		splx(s);
		return;

	     case SHIFT:
		gx_keyboard.shift ^= 0xffff;
		return;	

	     case CNTRL:
		gx_keyboard.cntrl ^= 0xffff;
		return;

	     case ALLUP:
		gx_keyboard.cntrl = gx_keyboard.shift = 0;
		return;

	     case REPEAT:
		c = gx_keyboard.last;
		break;

	     case HOLD:
		/*
		 * "Hold Screen" key was pressed, we treat it as 
		 *  if ^s or ^q was typed.  
		 */
		if (gx_keyboard.hold == 0) 
		{

		    if((tp->t_state & TS_TTSTOP) == 0) 
		    {
			register int s;
			c = q_key[CHAR_S];
			IPLTTY(s);
			(*slu.kbd_putc)( LK_LED_ENABLE );
			(*slu.kbd_putc)( LED_4 );
			gx_keyboard.hold = 1;
			splx(s);
		    } else
			c = q_key[CHAR_Q];
		}
		else 
		{
		    register int s;
		    c = q_key[CHAR_Q];
		    IPLTTY(s);
		    (*slu.kbd_putc)( LK_LED_DISABLE );
		    (*slu.kbd_putc)( LED_4 );
		    gx_keyboard.hold = 0;
		    splx(s);
		}
		if( c >= ' ' && c <= '~' )
		    c &= 0x1f;
		else if (c >= 0xA1 && c <= 0xFE)
		    c &= 0x9F;
		(*linesw[tp->t_line].l_rint)(c, tp);
		return;

	     default:

		/*
		 * Test for control characters. If set, see if the 
		 * character is elligible to become a control 
		 * character.
		 */
		if( gx_keyboard.cntrl ) 
		{
		    c = q_key[ data ];
		    if( c >= ' ' && c <= '~' )
			c &= 0x1f;
		} else if( gx_keyboard.lock || gx_keyboard.shift )
		    c = q_shift_key[ data ];
		else
		    c = q_key[ data ];
		break;	

	    }

	    gx_keyboard.last = c;

	    /*
	     * Check for special function keys
	     */
	    if( c & 0x100 ) 
	    {

		register char *string;

		string = q_special[ c & 0x7f ];
		while( *string )
		    (*linesw[tp->t_line].l_rint)(*string++, tp);
	    } 
	    else 
	    {
		if (tp->t_iflag & ISTRIP) /* Strip to 7 bits. */
		    c &= 0177;	
		else 
		{		/* Take the full 8-bits */
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
	    if (gx_keyboard.hold &&((tp->t_state & TS_TTSTOP) == 0)) 
	    {
		register int s;
		IPLTTY(s);
		(*slu.kbd_putc)( LK_LED_DISABLE );
		(*slu.kbd_putc)( LED_4 );
		gx_keyboard.hold = 0;
		splx(s);
	    }
	}
    }

    return(0);

}
/* gxkint */


gx_kbdreset()
{
    register int i;

    gx_inkbdreset = 1;

    (*slu.kbd_putc) (LK_DEFAULTS);
    DELAY(100000);
    for (i=1; i < 15; i++)
	(*slu.kbd_putc) (gx_divdefaults[i] | (i << 3));
    DELAY(100000);
    for (i = 0; i < KBD_INIT_LENGTH; i++)
	(*slu.kbd_putc) (gx_kbdinitstring[i]);

    gx_inkbdreset = 0;
}
/* end gx_kbdreset */


/******************************************************************
 **                                                              **
 ** Routine to start transmission. 				**
 **                                                              **
 ******************************************************************/

gxstart(tp)
    register struct tty *tp;
{
    register int unit, c;
    register struct tty *tp0;
    register int s, xcons_status;

    unit = minor(tp->t_dev);

    IPLTTY(s);
    /*
     * If it's currently active, or delaying, no need to do anything.
     */
    if (tp->t_state&(TS_TIMEOUT|TS_BUSY|TS_TTSTOP))
	goto out;

    /*
     * Display chars until the queue is empty, if the second subchannel 
     * is open irect them there. Drop characters from any lines other 
     * than 0 on the floor. TANDEM is set on second subchannel for flow 
     * control.
     */
    while( tp->t_outq.c_cc ) {
	if (unit == 0) {		/* console device */
	    xcons_status = xcons_chkq();
	    
	    switch (xcons_status) {
	      case XCONS_CLOSED:
		c = getc(&tp->t_outq);
		gx_blitc(c & 0xff);
		break;
		
	      case XCONS_BLOCKED:
		goto out;
		break;
		
	      case XCONS_OK:
		c = getc(&tp->t_outq);
		xconsrint(c);
		break;
	    }
	} else
	    c = getc(&tp->t_outq);
    }

    /*
     * Position the cursor to the next character location.
     */
    if (!(gx_dev_inuse & GRAPHIC_DEV))
	GX_POS_CUR( gxp->col*_TEXT_WIDTH, gxp->row*_TEXT_HEIGHT );

    /*
     * If there are sleepers, and output has drained below low
     * water mark, wake up the sleepers.
     */
    tp->t_state &= ~TS_BUSY;
 out:
    if ( tp->t_outq.c_cc<=TTLOWAT(tp) )
	if (tp->t_state&TS_ASLEEP)
	{
	    tp->t_state &= ~TS_ASLEEP;
	    wakeup((caddr_t)&tp->t_outq);
	}
    splx(s);
}
/* gxstart */


/******************************************************************
 **                                                              **
 ** Routine to stop output on the graphic device, e.g. for ^S/^Q **
 ** or output flush.                                             **
 **                                                              **
 ******************************************************************/
/*ARGSUSED*/
gxstop(tp, flag)
    register struct tty *tp;
{
    register int s;

    /*
     * Block interrupts while modifying the state.
     */
    IPLTTY(s);
    if (tp->t_state & TS_BUSY)
	if ((tp->t_state&TS_TTSTOP)==0)
	    tp->t_state |= TS_FLUSH;
	else
	    tp->t_state &= ~TS_BUSY;
    splx(s);
}
/* gxstop */


gx_rect(x, y, h, w, rgb, buf)
    /* <x,y> = top-left corner of rectangle */
    int x, y, h/*eight*/, w/*idth*/, rgb, buf;
{
    register int *ip, *stampPacket, lw;

    if (stampPacket = ip = (*_gx_getPacket)())
    {
	GX_DEBUG(GX_YOW,
		 gx_printf("gx_rect(x=%d,y=%d,w=%d,h=%d,0x%6x,%d) pkt=0x%x\n",
			   x, y, w, h, rgb, stampPacket, buf);
		 );

	*ip++ = CMD_LINES| RGB_CONST| LW_PERPKT;
	*ip++ = 0x1ffffff;
	*ip++ = 0x0;
	*ip++ = UPD_ENABLE| UMET_COPY| ((buf&0x3)<<3);

	lw = (h << 2) - 1;
	y = (y << 3) + lw;

	*ip++ = lw;
	*ip++ = rgb;				/* rgb */
	*ip++ = ( x            <<19) | y;
	*ip++ = ((((x+w)<<3)-1)<<16) | y;

	return (*_gx_sendPacket)(stampPacket);
    }
    return -1;
}
/* end gx_rect */


/******************************************************************
 **                                                              **
 ** Routine to output a character to the screen                  **
 **                                                              **
 ******************************************************************/

gx_blitc( c )
    char c;
{
    int i;

    c &= 0xff;

    switch ( c ) 
    {
     case '\t':			/* tab		*/
	for( i = 8 - (gxp->col & 0x7) ; i > 0 ; i-- )
	    gx_blitc( ' ' );
	break;

     case '\n':			/* linefeed	*/
	GX_DEBUG(GX_CONSOLE,
		 gx_putchar('\n');
		 if (gx_console & SLU3_DEV) break;
		 );
	if( gxp->row+1 >= gxp->max_row ) {
	    gxp->row -= gx_scroll(_TEXT_SCROLL);
	} else
	    gxp->row++;

	/*
	 * Position the cursor to the next character location.
	 */
	if (!(gx_dev_inuse & GRAPHIC_DEV))
	    GX_POS_CUR( gxp->col*_TEXT_WIDTH, gxp->row*_TEXT_HEIGHT );

     case '\r':			/* return	*/
	GX_DEBUG(GX_CONSOLE,
		 gx_putchar('\r');
		 if (gx_console & SLU3_DEV) break;
		 );
	gxp->col = 0;
	break;

     case '\b':			/* backspace	*/
	GX_DEBUG(GX_CONSOLE,
		 gx_putchar('\b');
		 if (gx_console & SLU3_DEV) break;
		 );
	if( --gxp->col < 0 )
	    gxp->col = 0;
	break;

     case '\007':		/* bell		*/
	/*
	 * Should we wait to see if we have auto configured 
	 * before we do anything to the keyboard? - Vasu
	 */
	(*slu.kbd_putc)( LK_RING_BELL );
	break;

     default:
	/*
	 * If the next character will wrap around then 
	 * increment row counter or scroll screen.
	 */
	if( gxp->col >= gxp->max_col ) {
	    gxp->col = 0 ;
	    if( gxp->row+1 >= gxp->max_row )
		gxp->row -= gx_scroll(_TEXT_SCROLL);
	    else
		gxp->row++;
	}

	/*
	 * 0xA1 to 0xFD are the printable characters added with 8-bit
	 * support.
	 */
	if(( c >= ' ' && c <= '~' ) || ( c >= 0xA1 && c <= 0xFD)) 
	{
	    int xpix = gxp->col * _TEXT_WIDTH;
	    int ypix = gxp->row * _TEXT_HEIGHT;

	    i = c - ' ';
	    if( 0 < i && i <= 221 )
	    {
		register unsigned short *f_row;
		register int *pp;
		int *stampPacket, *xyp, v1, v2, xya;

		GX_DEBUG(GX_CONSOLE,
			 gx_putchar(c);
			 if (gx_console & SLU3_DEV) break;
			 );
		/* These are to skip the (32) 8-bit 
		 * control chars, as well as DEL 
		 * and 0xA0 which aren't printable */

		/* Masked out above with 0x7f:
		if (c > '~') 
		    i -= 34; */

		i *= _TEXT_HEIGHT;

		f_row = &fg_font[i];

		if (stampPacket = pp = (*_gx_getPacket)())
		{
		    *pp++ = CMD_LINES| RGB_FLAT| XY_PERPRIM| LW_PERPRIM;
		    *pp++ = 0x4ffffff;
		    *pp++ = 0x0;
		    *pp++ = UPD_ENABLE| WE_XYMASK| UMET_COPY;

		    xyp = pp;
		    /* fg = character to draw */
		    *pp++ = GX_GET2ROWS(f_row);
		    *pp++ = GX_GET2ROWS(f_row);
		    *pp++ = GX_GET2ROWS(f_row);
		    *pp++ = GX_GET2ROWS(f_row);
		    *pp++ = GX_GET2ROWS(f_row);
		    *pp++ = GX_GET2ROWS(f_row);
		    *pp++ = GX_GET2ROWS(f_row);
		    *pp++ = GX_GET2ROWS(f_row);
		    i = (16<<2)-1;
		    *pp++ = xya = CONSXYADDR(xpix, ypix);
		    *pp++ = v1=   (xpix                    <<19)|((ypix<<3)+i);
		    *pp++ = v2=((((xpix+_TEXT_WIDTH)<<3)-1)<<16)|(v1&0xffff);
		    *pp++ = i;
		    *pp++ = gx_textfg;

		    /* opague background */
		    *pp++ = *xyp++ ^ 0xffffffff;
		    *pp++ = *xyp++ ^ 0xffffffff;
		    *pp++ = *xyp++ ^ 0xffffffff;
		    *pp++ = *xyp++ ^ 0xffffffff;
		    *pp++ = *xyp++ ^ 0xffffffff;
		    *pp++ = *xyp++ ^ 0xffffffff;
		    *pp++ = *xyp++ ^ 0xffffffff;
		    *pp++ = *xyp   ^ 0xffffffff;
		    *pp++ = xya;
		    *pp++ = v1;
		    *pp++ = v2;
		    *pp++ = i;
		    *pp++ = gx_textbg;

		    xyp = pp;
		    /* lower part of fg character */
		    *pp++ = GX_GET2ROWS(f_row);
		    *pp++ = GX_GET2ROWS(f_row);
		    *pp++ = 0x0;
		    *pp++ = 0x0;
		    *pp++ = 0x0;
		    *pp++ = 0x0;
		    *pp++ = 0x0;
		    *pp++ = 0x0;
		    i = ((_TEXT_HEIGHT-16)<<2)-1;  ypix += 16;
		    *pp++ = xya = CONSXYADDR(xpix, ypix);
		    *pp++ = v1=   (xpix                    <<19)|((ypix<<3)+i);
		    *pp++ = v2=((((xpix+_TEXT_WIDTH)<<3)-1)<<16)|(v1&0xffff);
		    *pp++ = i;
		    *pp++ = gx_textfg;

		    /* opague background */
		    *pp++ = *xyp++ ^ 0xffffffff;
		    *pp++ = *xyp   ^ 0xffffffff;
		    *pp++ =          0xffffffff;
		    *pp++ =          0xffffffff;
		    *pp++ =          0xffffffff;
		    *pp++ =          0xffffffff;
		    *pp++ =          0xffffffff;
		    *pp++ =          0xffffffff;
		    *pp++ = xya;
		    *pp++ = v1;
		    *pp++ = v2;
		    *pp++ = i;
		    *pp++ = gx_textbg;

		    (*_gx_sendPacket)(stampPacket);

		    gxp->col++;			/* increment column counter */
		}
	    }
	    else if (i == 0)		/* <space> */
	    {
		GX_DEBUG(GX_CONSOLE,
			 gx_putchar(' ');
			 if (gx_console & SLU3_DEV) break;
			 );
		gx_rect(xpix, ypix, _TEXT_HEIGHT, _TEXT_WIDTH, 0x0, 0);
		gxp->col++;		/* increment column counter */
	    }
	}
	break;
    }

    return(0);
}
/* end gx_blitc */


/********************************************************************
 **                                                                **
 ** Routine to direct kernel console output to display destination **
 **                                                                **
 ********************************************************************/

gxputc( c )
    register char c;
{
    register int xcons_status;

    if (printstate & PANICPRINT) {
	static int startingToPanic = 1;
	gx_debug    = GX_PANIC;
	gx_mouseon  = 0;		/* no mouse events accepted */
	gx_serverp  = 0;		/* server is no longer running */
	gx_console &= ~SLU3_DEV;	/* not just to slu3 console */
	if (startingToPanic) {
	    startingToPanic = 0; 	wbflush();
	    GX_DEBUG(GX_TERSE,
		     gx_puts("\n\007panic: resetting console!\n");
		     );
	    gx_init();			/* back to known output state */
	}
    }
    
    xcons_status = xcons_chkq();
    
    switch (xcons_status) {
      case XCONS_CLOSED:
	/* real console output; either to system console or forced to SLU3 */
	gx_blitc(c & 0xff);
	break;
	
      case XCONS_BLOCKED:
	break;
	
      case XCONS_OK:
	/* not panic'ing, routing to alternate console */
	xconsrint(c);
	break;
    }
}
/* gxputc */


/******************************************************************
 **                                                              **
 ** Routine to get a character from LK201.                       **
 **                                                              **
 ******************************************************************/

gxgetc(data)
    u_short	data;
{
    int	c;

    /*
     * Get a character from the keyboard,
     */

 loop:
    /*
     * Check for various keyboard errors
     */

    if( data == LK_POWER_ERROR || data == LK_KDOWN_ERROR ||
       data == LK_INPUT_ERROR || data == LK_OUTPUT_ERROR) 
    {
	if(!gx_inkbdreset) {
	    mprintf(" Keyboard error, code = %x\n",data);
	    gx_kbdreset();
	}
	return(0);
    }
    if( data < LK_LOWEST ) return(0);

    /*
     * See if its a state change key
     */

    switch ( data ) 
    {
     case LOCK:
	gx_keyboard.lock ^= 0xffff; /* toggle */
	if( gx_keyboard.lock )
	    (*slu.kbd_putc)( LK_LED_ENABLE );
	else
	    (*slu.kbd_putc)( LK_LED_DISABLE );
	(*slu.kbd_putc)( LED_3 );
	data = (*slu.kbd_getc)();
	goto loop;

     case SHIFT:
	gx_keyboard.shift ^= 0xffff;
	data = (*slu.kbd_getc)();
	goto loop;

     case CNTRL:
	gx_keyboard.cntrl ^= 0xffff;
	data = (*slu.kbd_getc)();
	goto loop;

     case ALLUP:
	gx_keyboard.cntrl = gx_keyboard.shift = 0;
	data = (*slu.kbd_getc)();
	goto loop;

     case REPEAT:
	c = gx_keyboard.last;
	break;

     default:

	/*
	 * Test for control characters. If set, see if the character
	 * is elligible to become a control character.
	 */
	if( gx_keyboard.cntrl ) 
	{
	    c = q_key[ data ];
	    if( c >= ' ' && c <= '~' )
		c &= 0x1f;
	} 
	else if( gx_keyboard.lock || gx_keyboard.shift )
	    c = q_shift_key[ data ];
	else
	    c = q_key[ data ];
	break;	

    }

    gx_keyboard.last = c;

    /*
     * Check for special function keys
     */
    if( c & 0x80 )
	return (0);
    else
	return (c);
}
/* gxgetc */


/******************************************************************
 **                                                              **
 ** Routine to position the cursor to a particular spot.         **
 **                                                              **
 ******************************************************************/

gx_pos_cur(x, y)
    int x,y;
{
    int i, x0, y0;
    register int s;

    volatile int *vdac_data = &_gx_vdac->data;

    if( y < gxp->min_cur_y || y > gxp->max_cur_y )
	y = gxp->max_cur_y;
    if( x < gxp->min_cur_x || x > gxp->max_cur_x )
	x = gxp->max_cur_x;

    gxp->cursor.x = x;		     /* keep track of real cursor */
    gxp->cursor.y = y;		     /* position, indep. of mouse */

    x0 = x + (BT459_H+BT459_X);
    y0 = y + (BT459_V+BT459_Y);

    IPLTTY(s);

    BT459_SETADDR(_gx_vdac, BT459_CUR_XLO);
    *vdac_data = _DUPBYTE0(x0); WBFLUSH();
    *vdac_data = _DUPBYTE1(x0); WBFLUSH();
    *vdac_data = _DUPBYTE0(y0); WBFLUSH();
    *vdac_data = _DUPBYTE1(y0); WBFLUSH();

    /* turn cursor back on */
    BT459_SETADDR(_gx_vdac, BT459_CUR_CMD);
    *vdac_data = 0xC0C0C0; WBFLUSH();

    splx(s);

#   ifdef GX_MULTIOPEN
    for (i = 1; i < GX_MAX_INFOS; i++) {
	gx_infos[i].info.cursor.x = x;
	gx_infos[i].info.cursor.y = y;
    }
#   endif gx_multiopen

    GX_DEBUG(GX_YOW,
	     gx_printf("gx_pos_cur(%d,%d)\n", x,y);
	     );
}
/* end gx_pos_cur */


/******************************************************************
 **                                                              **
 ** Routine to scroll.                                           **
 **                                                              **
 ******************************************************************/

gx_scroll(scroll)
    int scroll;
{
    register int *pp, fY, tY, h;
    int *stampPacket;

    /*
     * use vertical wrap-around when server running.
     */
    if (GX_HAVESERVER)
	return (gxp->row);

    fY = (scroll*_TEXT_HEIGHT)<<3;	/* from */
    tY = 0<<3;				/* to */
    h = (_TEXT_ROWS+1-scroll)*_TEXT_HEIGHT; /* # scanlines to move */

    while (h > 0)
    {
	register int n = MIN(h, STAMP_MAX_CMDS);

	h -= n;

	if (stampPacket = pp = (*_gx_getPacket)())
	{
	    *pp++ = CMD_COPYSPANS| LW_PERPKT;
	    *pp++ = (n<<24) | 0xffffff;
	    *pp++ = 0x0;
	    *pp++ = UPD_ENABLE| UMET_COPY| SPAN;
	    *pp++ = 1;				/* linewidth */

	    for ( ; n > 0; n--, fY += 8, tY += 8)
	    {
		*pp++ = (_TEXT_COLS*_TEXT_WIDTH)<<3;
		*pp++ = fY;			/* x := 0 */
		*pp++ = tY;
	    }
	    (*_gx_sendPacket)(stampPacket);
	}
	else {
	    GX_DEBUG(GX_TERSE,
		     gx_printf("gx_scroll - ~packet!\n");
		     );
	    return 0;
	}
    }

    /* clear out bottom scroll lines */
    gx_rect(0, (_TEXT_ROWS-scroll+1)*_TEXT_HEIGHT,
	    scroll*_TEXT_HEIGHT, _TEXT_COLS*_TEXT_WIDTH, 0x0, 0);

    return(scroll-1);
}
/* end gx_scroll */


/*****************************************************************
 * Clear the bitmap						 *
 *****************************************************************/

gx_clearbitmap()
{
    GX_DEBUG(GX_BLAB,
	     gx_puts("gx_clearbitmap()\n");
	     );
    gx_rect(0, 0, 1024, 1280, 0x0, 0);
}
/* end gx_clearbitmap */


/******************************************************************
 **                                                              **
 * Routine to initialize the mouse.                              **
 **                                                              **
 ******************************************************************
 *
 * NOTE:
 *	This routine communicates with the mouse by directly
 *	manipulating the PMAX SLU registers. This is allowed
 *	ONLY because the mouse is initialized before the system
 *	is up far enough to need the SLU in interrupt mode.
 */

gx_init_mouse()
{
    register u_short lpr;
    int	id_byte1, id_byte2, id_byte3, id_byte4;

    GX_DEBUG(GX_BLAB,
	     gx_puts("gx_init_mouse(");
	     );
    /*
     * Set SLU line parameters for mouse communication.
     */
    (*slu.mouse_init)();

    /*
     * Perform a self-test
     */
    (*slu.mouse_putc)(SELF_TEST);

    /*
     * Wait for the first byte of the self-test report
     */
    id_byte1 = (*slu.mouse_getc)();
    if (id_byte1 < 0) 
    {
	mprintf("\ngx: Timeout on 1st byte of self-test report\n");
	goto OUT;
    }

    /*
     * Wait for the hardware ID (the second byte returned by the 
     * self-test report)
     */
    id_byte2 = (*slu.mouse_getc)();
    if (id_byte2 < 0) 
    {
	mprintf("\ngx: Timeout on 2nd byte of self-test report\n");
	goto OUT;
    }

    /*
     * Wait for the third byte returned by the self-test report)
     */
    id_byte3 = (*slu.mouse_getc)();
    if (id_byte3 < 0) 
    {
	mprintf("\ngx: Timeout on 3rd byte of self-test report\n");
	goto OUT;
    }

    /*
     * Wait for the fourth byte returned by the self-test report)
     */
    id_byte4 = (*slu.mouse_getc)();
    if (id_byte4 < 0) 
    {
	mprintf("\ngx: Timeout on 4th byte of self-test report\n");
	goto OUT;
    }

    /*
     * Set the operating mode
     *
     * We set the mode for both mouse and the tablet to 
     * "Incremental stream mode".
     */
    if ((id_byte2 & 0x0f) == MOUSE_ID)
	pointer_id = MOUSE_ID;
    else
	pointer_id = TABLET_ID;

    (*slu.mouse_putc)(INCREMENTAL);

 OUT:
    GX_DEBUG(GX_BLAB,
	     gx_puts(")\n");
	     );
    return(0);
}
/* end gx_init_mouse */


/******************************************************************
 ** gx_load_cursor():                                            **
 **	Routine to load the cursor 64x64 sprite pattern          **
 **                                                              **
 ******************************************************************/
#define BT459_CUREND	(BT459_CUR_RAM+(0x010*BT459_CURH))

int total_VDAC_cursor_errors = 0;

gx_load_cursor()
{
    register int i, x, y, s;
    volatile int *vdac_data = &_gx_vdac->data;
    int current_round = 0;
    int got_it_right = 0;

    GX_DEBUG(GX_SILENT,
	     if (GX_CURH != BT459_CURH || GX_CURW != BT459_CURW)
	         panic("gx_load_cursor");
	     );
    GX_DEBUG(GX_BLAB,
	     gx_printf("gx_load_cursor: 0x%x msk=0x%x\n",
		       _gx_vdac, gx_planemask);
	     );

 Rewrite_Cursor:
    if (current_round >= BT459_MAXERR)
	goto Done_Cursor;

    IPLTTY(s);

    /* turn cursor off while we load the cursor RAM to reduce flicker */
    BT459_SETADDR(_gx_vdac, BT459_CUR_CMD);
    *vdac_data = 0x0; WBFLUSH();

    BT459_SETADDR(_gx_vdac, GX_CURBEG);

    for (y = i = 0; y < BT459_CURH; y++)
    {
	for (x = 0; x < (BT459_CURW/4); x++, i++)
	{
	    *vdac_data = _DUPBYTE0(gx_cursor[i]);
	    WBFLUSH();
	}
    }
    splx(s);

    current_round++;

    IPLTTY(s);

    BT459_SETADDR(_gx_vdac, GX_CURBEG);

    for (y = i = 0; y < BT459_CURH; y++)
    {
	for (x = 0; x < (BT459_CURW/4); x++, i++)
	{
	    if (VDAC_RDAT != (_DUPBYTE0(gx_cursor[i]) & gx_planemask)) {
		splx(s);
		goto Rewrite_Cursor;
	    }
	}
    }
    splx(s);

    got_it_right = 1;

 Done_Cursor:

    total_VDAC_cursor_errors += current_round;

    GX_DEBUG(GX_GAB,
	     gx_printf("gx_load_cursor: %d retries\n", current_round);
	     );
    GX_DEBUG(GX_TERSE,
	     gx_check_vdac(0);
	     );
    if (!got_it_right) {
	printf("gx_load_cursor: %d retries exceeded\n", current_round);
	return (-1);
    }

    /* turn cursor back on */
    BT459_SETADDR(_gx_vdac, BT459_CUR_CMD);
    *vdac_data = 0xC0C0C0; WBFLUSH();
    return (0);
}
/* end gx_load_cursor() */


/*************************************************************************
 * Console text cursor pattern.  Put cursor's <x,y> at northwest corner. *
 *************************************************************************/
gx_load_defcursor()
{
    static   int toggle = 1;

    GX_DEBUG(GX_BLAB,
	     gx_puts("gx_load_defcursor()\n");
	     );

    bzero(gx_cursor, GX_CURSORBYTES);
    /*
     * Assume at least 16x16 cursor
     */
    if (toggle) {
	register u_char *bits;

	bits = gx_cursor;
	*bits++ = 0x00; *bits++ = 0xff; *bits = 0x00;

	bits = gx_cursor + (GX_CURW/4);
	*bits++ = 0x03; *bits++ = 0xff; *bits = 0xc0;

	bits = gx_cursor + (2*(GX_CURW/4));
	*bits++ = 0x0f; *bits++ = 0xff; *bits = 0xf0;

	bits = gx_cursor + (3*(GX_CURW/4));
	*bits++ = 0x3f; *bits++ = 0xff; *bits = 0xfc;
    }
    else
    {
	register int i;
	register u_char *bits = gx_cursor;

	for (i = 0; i < (4*(GX_CURW/4)); i += (GX_CURW/4))
	{
	    bits[i+0] = 0x3f;
	    bits[i+1] = 0xff;
	    bits[i+2] = 0xfc;
	}
    }
    toggle ^= 0x1;

    gx_load_cursor();
}
/* end gx_load_defcursor */


gx_load_ccolor()
{
    register int s;
    register volatile int *vdac_data = &_gx_vdac->data;
    int retries = 3;

    GX_DEBUG(GX_BLAB,
	     gx_printf("gx_load_ccolor(fg 0x%x bg 0x%x)\n",
		       gxp->curs_fg, gxp->curs_bg);
	     );

    IPLTTY(s);

    BT459_SETADDR(_gx_vdac, BT459_CUR_COLOR2);

    *vdac_data = _DUPBYTE2(gxp->curs_bg); WBFLUSH();
    *vdac_data = _DUPBYTE1(gxp->curs_bg); WBFLUSH();
    *vdac_data = _DUPBYTE0(gxp->curs_bg); WBFLUSH();

    *vdac_data = _DUPBYTE2(gxp->curs_fg); WBFLUSH();
    *vdac_data = _DUPBYTE1(gxp->curs_fg); WBFLUSH();
    *vdac_data = _DUPBYTE0(gxp->curs_fg); WBFLUSH();

    splx(s);

#   ifdef GX_MULTIOPEN
    for (s = 1; s < GX_MAX_INFOS; s++) {
	gx_infos[s].info.curs_bg = gxp->curs_bg;
	gx_infos[s].info.curs_fg = gxp->curs_fg;
    }
#   endif

    GX_DEBUG(GX_TERSE,
	     gx_check_vdac(0);
	     );	     
}
/* end gx_load_ccolor */


gx_init_colormap()
{
    register int i;

    GX_DEBUG(GX_BLAB,
	     gx_puts("gx_init_colormap()\n");
	     );

    gx_colormap[0] = 0x0;
    for (i = 1; i < 256 ; i++)
	gx_colormap[i] = 0xffffff;

    gxp->curs_fg = gxp->curs_bg = _TEXT_CFG;

    gxp->cmap_index = 000;
    gxp->cmap_count = 256;

    gxp->flags |= (GX_F_NEW_CMAP| GX_F_VIDEO_ON);

    gx_load_colormap();		     /* load color palette */
    gx_load_ccolor();		     /* load cursor color regs */
}
/* end gx_init_colormap */


gx_load_colormap()
{
    int white = 0x1;
    int black = 0x0;
    int lval  = 0xffffff;
    int hval  = 0x000000;

    GX_DEBUG(GX_GAB,
	     if (gxp->flags & GX_F_VIDEO_ON) {
		 gx_printf("gx_load_colormap(ent %d cnt %d)\n",
			   gxp->cmap_index, gxp->cmap_count);
	     } else {
		 gx_printf("gx_load_colormap(screen saver on)\n");
	     }
	     );

    if ((gxp->flags & (GX_F_NEW_CMAP|GX_F_VIDEO_ON)) ==
	(GX_F_NEW_CMAP|GX_F_VIDEO_ON))
    {
	register int s;
	register int entry, count;
	register volatile int *vdac_cmap = &_gx_vdac->cmap;

	entry = gxp->cmap_index;
	count = gxp->cmap_count;

	IPLTTY(s);

	BT459_SETADDR(_gx_vdac, BT459_PIX_COLOR + entry);

	for ( ; count > 0; count--, entry++)
	{
	    *vdac_cmap = _DUPBYTE2(gx_colormap[entry]); WBFLUSH();
	    *vdac_cmap = _DUPBYTE1(gx_colormap[entry]); WBFLUSH();
	    *vdac_cmap = _DUPBYTE0(gx_colormap[entry]); WBFLUSH();
	}
	gxp->flags &= ~GX_F_NEW_CMAP;

	splx(s);
	/*
	 * Now select the console text foreground/background pixels
	 * based on the new colormap...
	 */
	for (entry = 0; entry < 256; entry++)
	{
	    if (gx_colormap[entry] > hval) {
		hval = gx_colormap[entry];
		white = entry;
	    }
	    if (gx_colormap[entry] < lval) {
		lval = gx_colormap[entry];
		black = entry;
	    }
	}
	gx_textfg = _DUPBYTE0(white);
	gx_textbg = _DUPBYTE0(black);
    }
}
/* end gx_load_colormap */


gx_config(qp)
    gxInfo *qp;
{
    int modtype;

    GX_DEBUG(GX_GAB,
	     gx_printf("gx_config: qp=0x%x\n", svtophy(qp));
	     );

    bzero(qp, sizeof(gxInfo));
    /*
     * Screen parameters for the <XXX>  monitors. These determine the max
     * size in pixel and character units for the display and cursor positions.
     * Notice that the mouse defaults to original square algorithm, but X
     * will change to its defaults once implemented.
     * Local variables for the driver. Initialized for <XXX> screen
     * so that it can be used during the boot process.
     */
    qp->max_row = _TEXT_ROWS+1;
    qp->max_col = _TEXT_COLS;
    qp->max_x = BT459_MAXX+1;
    qp->max_y = BT459_MAXY+1;
    qp->max_cur_x = GX_CURMAXX;
    qp->max_cur_y = GX_CURMAXY;
    qp->min_cur_x = GX_CURMINX;
    qp->min_cur_y = GX_CURMINY;
    qp->version = 11;
    qp->mthreshold = 4;	
    qp->mscale = 2;
    qp->qe.eSize = PMMAXEVQ;
    qp->qe.tcSize = MOTION_BUFFER_SIZE;
    qp->flags = GX_F_VIDEO_ON | 0xff;

    qp->qe.events = gx_events;
    qp->qe.tcs = gx_tcs;
    /*qp->curs_bits = gx_cursor;*/
    /*qp->colormap = gx_colormap;*/
    qp->gram = NULL;

    modtype = gx_decode_option(qp);

    (*_gx_config)(qp, modtype);

    GX_DEBUG(GX_YAK,
	     gx_printf("gx_config: w/h/poll=%d/%d/0x%x\n",
		       qp->stamp_width, qp->stamp_height, qp->stic_dma_rb);
	     );

    qp->qe.timestamp_ms = TOY;
}


/******************************************************************
 **                                                              **
 ** Routine to do the board specific setup.                      **
 **                                                              **
 ******************************************************************/

gx_setup()
{
    register int i, j;

    /*
     * Set the line parameters on SLU line for
     * the LK201 keyboard: 4800 BPS, 8-bit char, 1 stop bit, no parity.
     */
    (*slu.kbd_init)();

    GX_DEBUG(GX_BLAB,
	     gx_puts("gx_setup(beg)\n");
	     );

    for (i = 0; i < GX_MAX_INFOS; i++) {
	gx_infos[i].pid = -1;
	gx_infos[i].shmat = 0;
	gx_config(&gx_infos[i].info);
    }

    gx_init();

    gx_clearbitmap();

    /*
     * Initialize the mouse
     */
    gx_init_mouse();

    v_consputc	= gxputc;
    v_consgetc	= gxgetc;

    vs_gdopen	= gxopen;
    vs_gdclose	= gxclose;
    vs_gdselect	= gxselect;
    vs_gdioctl	= gxioctl;
    vs_gdkint	= gxkint;
    vs_gdstop	= gxstop;

    GX_DEBUG(GX_BLAB,
	     gx_puts("gx_setup(end)\n");
	     );
}
/* end gx_setup */


gx_init_stic()
{
    int modtype, xconfig, yconfig, config;

    GX_DEBUG(GX_GAB,
	     gx_printf("gx_init_stic: stic=0x%x\n", _gx_stic);
	     );

    /*
     *  initialize STIC registers (bmk)
     */
    _gx_stic->sticsr = 0x00000030;	/* sticcsr */
    wbflush();
    DELAY(4000);			/* 4mS */
    _gx_stic->sticsr = 0x00000000;	/* sticcsr */
    _gx_stic->buscsr = 0xffffffff;	/* buscsr */
    DELAY(20000);			/* long time... */

    gx_init_vdac();

    /*
     *  Initialize Stamp config register
     */
    modtype = _gx_stic->modcl;
    xconfig = (modtype & 0x800) >> 11;
    yconfig = (modtype & 0x600) >> 9;
    config = (yconfig << 1) | xconfig;

    /* stamp0 config */
    *(int *)(_gx_stamp+0x000b0) = config;
    *(int *)(_gx_stamp+0x000b4) = 0x0;

    if (yconfig > 0) {
	/* stamp1 config */
	*(int *)(_gx_stamp+0x100b0) = 0x8|config;
	*(int *)(_gx_stamp+0x100b4) = 0x0;
	if (yconfig > 1) {
	    /* stamp 2 & 3 config */
	}
    }

    /*
     * Initialize STIC video registers
     */
    _gx_stic->vblank = (1024 << 16) | 1063;	/* vblank */
    _gx_stic->vsync = (1027 << 16) | 1030;	/* vsync */
    _gx_stic->hblank = (255 << 16) | 340;	/* hblank */
    _gx_stic->hsync2 = 245;			/* hsync2 */
    _gx_stic->hsync = (261 << 16) | 293;	/* hsync */

    _gx_stic->ipdvint = STIC_INT_CLR;		/* ipdvint */
    _gx_stic->sticsr = 0x00000008;		/* sticcsr */
    wbflush();
}
/* end gx_init_stic */


static int
_allzero(ip, msk, cnt)
    register u_int *ip;
    register u_int msk, cnt;
{
    int is0 = 1;

    GX_DEBUG(GX_YAK,
    	     gx_printf("_allzero: 0x%x msk 0x%x cnt %d\n", ip, msk, cnt);
	     );

    for ( ; cnt--; ip++) {
	if (*ip & msk) {
	    is0 = 0;
	    GX_DEBUG(GX_TERSE,
	    	     gx_printf("_allzero: @ 0x%x = 0x%x\n", ip, *ip);
		     );
	}
    }

    return is0;
}

gx_howManyPlanes(buf)
    int buf;
{
    register int *pp, i;
    volatile int *ibuf;
    int *stampPacket, dstptr;
    int planes = 0;

    if (buf < 0 || buf > 3)
	panic("gx_howManyPlanes(buf)");

    if (gx_planes[buf] != 3)
	return ((int)gx_planes[buf] & 0xff);

    /*
     * Find out whether there are 0, 8, 16, or 24 bits in buffer buf.
     *
     * If the display buffer, then use VDAC ID as easy way.  Red is active.
     * Only 8 or 24 planes are valid.
     */
    if (buf == 0)
    {
	register volatile int *vdac_data = &_gx_vdac->data;
	register int s;
	register int id;

	IPLTTY(s);

	BT459_SETADDR(_gx_vdac, BT459_ID_REG);
	id = *vdac_data & 0xffffff;

	splx(s);

	if (id == 0x4a4a4a) {
	    /*return (gx_planes[0] = 24);*/
	    /* could be real 24, or 8 w/ 3 VDACs */
	    goto PokeFB;
	}
	else if ((id & 0xff0000) == 0x4a0000)
	    return (gx_planes[0] = 8);
	else {
	    printf("gx_howManyPlanes: bad VDAC ID 0x%x\n", id);
	    panic("VDAC ID");
	}
    }

    /*
     * Poke the frame buffer and check for corruption...
     */
 PokeFB:
    gx_rect(0,0, 1,0xf, 0x0, buf);

    (*_gx_getPixBuff)(&ibuf, &dstptr);

    for (i = 0; i <= 0xf; i++)		/* clear out image buffer */
	*(ibuf+i) = 0x0;		/* and read width word */

    stampPacket = pp = (*_gx_getPacket)();
    *pp++ = CMD_READSPANS;
    *pp++ = 0x1ffffff;
    *pp++ = 0x0;
    *pp++ = UPD_ENABLE| UMET_NOOP| SPAN| ((buf&0x3)<<3)| ((buf&0x3)<<1);
    *pp++ = dstptr;
    *pp++ = 0xf<<3;			/* width */
    *pp++ = 0x0;			/* v1 */
    *pp	  = 0x0;			/* pad */

    /* wait for things to settle down... */
    DELAY(100);

    (*_gx_sendPacket)(stampPacket);

    /* wait for frame buffer read to complete... */
    DELAY(100);

    for ( i = 0; *(ibuf+0xf) == 0 && i < 250000; i++ ) {
	DELAY(3);
    }

    if ( i < 250000 )
    {
	GX_DEBUG(GX_YAK,
		 gx_printf("gx_howManyPlanes: ib 0x%x %d dst 0x%x\n",
			   ibuf, *(ibuf+0xf), dstptr);
		 );
	/*
	 * check the 3 groups of 8 planes.  each group that's not 0x00
	 * means reduce the number of planes by 8.  this is independent
	 * of where they decide to put the valid planes...
	 */
	if (_allzero(ibuf, 0x0000ff, 0xe))
	    planes += 8;
	if (_allzero(ibuf, 0x00ff00, 0xe))
	    planes += 8;
	if (_allzero(ibuf, 0xff0000, 0xe))
	    planes += 8;
    }
    else {
	GX_DEBUG(GX_TERSE,
		 gx_puts("gx_howManyPlanes: readspans timeout\n");
		 );
	return (gx_planes[buf] = 0);
    }

    GX_DEBUG(GX_YAK,
	     if (planes && planes < 24)
	     gx_printf("gx_howManyPlanes: %d(%d)\n", planes, buf);
	     );

    return (gx_planes[buf] = (char) planes);
}
/* end gx_howManyPlanes */


/******************************************************************
 ** gx_decode_option():                                          **
 **                                                              **
 **	Routine to figure out what type of graphics device is    **
 **     present.                                                 **
 **                                                              **
 ******************************************************************/
gx_decode_option(gp)
    gxInfo *gp;
{
    sticCf *cf = (sticCf *) &_gx_modtype;

    GX_DEBUG(GX_GAB,
	     gx_printf("gx_decode_option(stic=0x%x modcl=0x%x)\n",
		       _gx_stic, _gx_modtype);
	     );

    gp->stamp_width = (cf->xconfig ? 5 : 4);
    gp->stamp_height = (1 << cf->yconfig);

    switch (cf->option)
    {
     case STIC_OPT_2DA:
	gp->nplanes = gx_planes[0] = 8; /* display buffer */
	gp->zplanes = gx_planes[2] = 0; /* no z buffer */
	gp->zzplanes = gx_planes[3] = 0;
	gp->n10_present = -2;		/* no geom accel, but stamp */
	break;

     default:
	if (gp->stamp_height > 1)	/* 5x2 stamp is high-end */
	{
	    gp->nplanes = gx_planes[0] = 24;
	    gp->zplanes = gx_planes[2] = 24;
	    gp->zzplanes = gx_planes[3] = 24;
	}
	else				/* low/mid-3D */
	{
	    gp->nplanes = gx_howManyPlanes(0); /* 8 or 24 */
	    gp->zplanes = gx_howManyPlanes(2); /* 0 or 24 */
	    gp->zzplanes = gx_planes[3] = 0;
	}
	gp->n10_present = 1;
	break;
    }

    /* double buffer: assume >=2 buffers (true for all current configs) */
    gp->dplanes = gx_planes[1] = gp->nplanes;

    return (cf->option);
}
/* end gx_decode_option() */


/******************************************************************
 ** gx_devtype():                                                **
 **	Routine to return a pointer to a string identifying      **
 **	the type of graphics option present, suitable for use    **
 **	in the DEVIOCGET ioctl().                                **
 **                                                              **
 ******************************************************************/

char *gx_devtype()
{
    int modtype = _gx_stic->modcl;
    sticCf *cf = (sticCf *) &modtype;

    return devgetstr[cf->option];
}
/* end gx_devtype() */


/******************************************************************
 **                                                              **
 ** Routine to initialize the screen.                            **
 **                                                              **
 ******************************************************************/

gx_init()
{
    gxp->row = _TEXT_ROWS;
    gxp->col = 0;

    switch (gxp->nplanes)
    {
     case 8:
	gx_planemask = (gxp->n10_present == 1) ? 0xff0000 : 0xff;
	break;
     case 24:
	gx_planemask = 0xffffff;
	break;
     default:
	panic("gx_init(nplanes)");
    }

    GX_DEBUG(GX_BLAB,
	     gx_printf("gx_init: 0x%x msk 0x%x\n", svtophy(gxp), gx_planemask);
	     );

    gx_init_stic();		     /* stic, vdac */
    gx_init_colormap();		     /* colormap, cursor */
    gx_load_defcursor();	     /* set back to known values */
    gx_pos_cur(BT459_MAXX/2, BT459_MAXY/2);

    GX_DEBUG(GX_TERSE,
	     gx_check_vdac(0);
	     );

    /*
     * Reset keyboard to default state.
     */
    if(!gx_inkbdreset)
	gx_kbdreset();

    GX_CALL(_gx_init)();		/* driver specific init */
}
/* end gx_init */


gx_init_vdac()
{
    register int i, s;
    register volatile int *vdac_data = &_gx_vdac->data;

    /*
     *  write control registers first group, then the second group
     */
    GX_DEBUG(GX_GAB,
	     gx_printf("gx_init_vdac(CR-1:vdac=0x%x,dat=0x%x",
		       _gx_vdac, &_gx_vdac->data);
	     );

    IPLTTY(s);

    BT459_SETADDR(_gx_vdac, BT459_CMD_0);

    *vdac_data = gx_bt459CR1[0]; wbflush();
    /*
     * after writing CR0, must reset the vdac in order for the chip to
     * behave properly.  of course, there is no provision for a software
     * vdac reset by the Bt459 chip.  a write to hyperspace has been hacked
     * up for this purpose.  but, whatever gets written goes into addr_lo,
     * (3da) so reset addr for next command.
     */
    if (_gx_vdacReset) {
	*_gx_vdacReset = 0x0; wbflush();
    }
    else
	panic("gx_init_vdac: _gx_vdacReset");

    BT459_SETADDR(_gx_vdac, BT459_CMD_1);
    for (i = 1; i < BT459CR1_SIZE; i++) {
        *vdac_data = gx_bt459CR1[i]; WBFLUSH();
    }

    BT459_SETADDR(_gx_vdac, BT459_CUR_CMD);

    *vdac_data = 0xc0c0c0; WBFLUSH();	/* enable plane 0+1 of cursor */
    for (i = 0; i < 12; i++) {
        *vdac_data = 0; WBFLUSH();
    }
    splx(s);

    wbflush();

    GX_DEBUG(GX_GAB,
	     gx_puts(":CR-2)\n");
	     );
}
/* end gx_init_vdac */


#ifndef GX_NODEBUG

gx_check_vdac(docorrect)
    int docorrect;
{
    register int i, s;
    register volatile int *vdac_data = &_gx_vdac->data;
    int errors = 0, x, y;
    u_long cr1 = 0, cr1data[BT459CR1_SIZE];
    u_long cr2 = 0, cr2data[13];
    u_long ccc = 0, ccdata[6];

    GX_DEBUG(GX_BLAB,
	     gx_printf("gx_check_vdac(%d)\n", docorrect);
	     );

    wbflush();

    IPLTTY(s);

    BT459_SETADDR(_gx_vdac, BT459_CMD_0);

    for ( i = 0; i < BT459CR1_SIZE; i++ )
	if ((cr1data[i] = VDAC_RDAT) != (gx_bt459CR1[i] & gx_planemask))
	    errors++, cr1 |= (0x1 << i);

    BT459_SETADDR(_gx_vdac, BT459_CUR_CMD);

    if ((cr2data[0] = VDAC_RDAT) != (0xc0c0c0 & gx_planemask))
	errors++, cr2 = 0x1;

    /* check cursor position */
    x  = (VDAC_RDAT & 0xff);
    x |= (VDAC_RDAT & 0xff) << 8;
    x -= (BT459_H+BT459_X);

    y  = (VDAC_RDAT & 0xff);
    y |= (VDAC_RDAT & 0xff) << 8;
    y -= (BT459_V+BT459_Y);

    for ( i = 5; i < 13; i++ )
        if ((cr2data[i] = VDAC_RDAT) != 0x0)
	    errors++, cr2 |= (0x1 << i);

    BT459_SETADDR(_gx_vdac, BT459_CUR_COLOR2);

    if ((ccdata[0] = VDAC_RDAT) != (_DUPBYTE2(gxp->curs_fg) & gx_planemask))
	errors++, ccc |= 0x1;
    if ((ccdata[1] = VDAC_RDAT) != (_DUPBYTE1(gxp->curs_fg) & gx_planemask))
	errors++, ccc |= 0x2;
    if ((ccdata[2] = VDAC_RDAT) != (_DUPBYTE0(gxp->curs_fg) & gx_planemask))
	errors++, ccc |= 0x4;
    if ((ccdata[3] = VDAC_RDAT) != (_DUPBYTE2(gxp->curs_bg) & gx_planemask))
	errors++, ccc |= 0x8;
    if ((ccdata[4] = VDAC_RDAT) != (_DUPBYTE1(gxp->curs_bg) & gx_planemask))
	errors++, ccc |= 0x10;
    if ((ccdata[5] = VDAC_RDAT) != (_DUPBYTE0(gxp->curs_bg) & gx_planemask))
	errors++, ccc |= 0x20;

    splx(s);

    GX_DEBUG(GX_YAK,
	     /*
	      * now print out all that's wrong with this stupid chip...
	      */
	     for ( i = 0; i < BT459CR1_SIZE; i++ ) {
		 if (cr1 & (0x1 << i)) {
		     gx_printf("VDAC CR1 #%d = 0x%x\n", i, cr1data[i]);
		 }
	     }
	     for ( i = 0; i < 13; i++ ) {
		 if (cr2 & (0x1 << i)) {
		     gx_printf("VDAC CR2 #%d = 0x%x\n", i, cr2data[i]);
		 }
	     }
	     if (docorrect && (cr1 || cr2)) {
		 gx_printf("VDAC CR: reloading\n");
		 gx_init_vdac();
	     }
#if 0
	     if (gxp->mouse.x != x || gxp->mouse.y != y) {
		 gx_printf("VDAC CR2 #1-4: %d,%d != mse %d,%d\n",
			   x, y, gxp->mouse.x, gxp->mouse.y);
	     }
#endif 0
	     for ( i = 0; i < 6; i++ ) {
		 if (ccc & (0x1 << i)) {
		     gx_printf("VDAC CC #%d = 0x%x\n", i, ccdata[i]);
		 }
	     }
	     if (ccc) {
		 if (docorrect) {
		     gx_printf("VDAC CC: fg 0x%x bg 0x%x reloading\n",
			       gxp->curs_fg, gxp->curs_bg);
		     gx_load_ccolor();
		 } else {
		     gx_printf("VDAC CC: fg 0x%x bg 0x%x\n",
			       gxp->curs_fg, gxp->curs_bg);
		 }
	     }
	     );

    return errors;
}
/* end gx_check_vdac */

#endif !gx_nodebug


gx_video_on()
{
    register volatile int *vdac_data = &_gx_vdac->data;
    register int s;

    GX_DEBUG(GX_BLAB,
	     gx_puts("gx_video_on()\n");
	     );

    IPLTTY(s);

    if ((gxp->flags & GX_F_VIDEO_ON) == 0)
    {
	BT459_SETADDR(_gx_vdac, BT459_CUR_CMD);
	*vdac_data = 0xC0C0C0; WBFLUSH();

	gxp->cmap_index = 0;
	gxp->cmap_count = (gxp->flags & GX_F_NEW_CMAP) ? 0xff : 1;
	gxp->flags |= (GX_F_NEW_CMAP| GX_F_VIDEO_ON);
	gx_load_colormap();

	BT459_SETADDR(_gx_vdac, BT459_PIX_RMASK);
	*vdac_data = 0xffffff; WBFLUSH();
    }
    splx(s);
}
/* end gx_video_on */


gx_video_off()
{
    register volatile int *vdac_data = &_gx_vdac->data;
    register int s;

    GX_DEBUG(GX_BLAB,
	     gx_puts("gx_video_off()\n");
	     );

    IPLTTY(s);

    if (gxp->flags & GX_F_VIDEO_ON)
    {
	register volatile int *vdac_cmap = &_gx_vdac->cmap;

	BT459_SETADDR(_gx_vdac, BT459_PIX_COLOR);
	*vdac_cmap = 0x0; WBFLUSH();
	*vdac_cmap = 0x0; WBFLUSH();
	*vdac_cmap = 0x0; WBFLUSH();

	BT459_SETADDR(_gx_vdac, BT459_PIX_RMASK);
	*vdac_data = 0x0; WBFLUSH();
	BT459_SETADDR(_gx_vdac, BT459_CUR_CMD);
	*vdac_data = 0x0; WBFLUSH();
	gxp->flags &= ~(GX_F_VIDEO_ON);
    }
    splx(s);
}
/* end gx_video_off */


#ifdef GX_MULTIOPEN
/*
 * Find calling process' info struct, or an unused one if not found.
 */
gx_info_get(firstEntry)
    int firstEntry;
{
    register int i;
    int anyfree = -1;

    for (i = firstEntry; i < GX_MAX_INFOS; i++) {
	if (gx_infos[i].pid == u.u_procp->p_pid)
	    return(i);
	else if (gx_infos[i].pid == -1)
	    anyfree = i;
    }

    if (anyfree == -1)
	return(gx_info_gc(firstEntry));

    return(anyfree);
}
/* end gx_info_get */


/*
 * Find a free slot.
 */
gx_info_gc(firstSlot)
    int firstSlot;
{
    register int i, pid;

    for (i = firstSlot; i < GX_MAX_INFOS; i++) {
	if (gx_info_gone(gx_infos[i].pid)) {
	    gx_infos[i].pid = -1;
	    gx_infos[i].shmat = 0;
	    return(i);
	}
    }
    return(-1);
}
/* end gx_info_gc */


gx_info_gone(pid)
    register int pid;
{
    FORALLPROC(
	       if (pid == pp->p_pid)
	       return (0);
	       );
#   if 0
    register struct proc *pp;

    for (pp = allproc; pp != NULL; pp = pp->p_nxt)
	if (pid == pp->p_pid)
	    return(0);
#   endif 0

    return (1);
}
/* end gx_info_gone */

#endif gx_multiopen


#ifndef GX_NODEBUG
/*
 * Output debugging messages to the serial-line console ONLY.
 * That leaves the graphics console undisturbed...
 */
gx_putchar(c)
    register int c;
{
    if ((gx_console & CONS_DEV) && (c & 0x7f) &&
	((printstate & PANICPRINT) == 0 || gx_level <= GX_PANIC))
	
    {
	(*slu.slu_putc)(3, c & 0x7f);
    }
}
/* end gx_putchar */


/*VARARGS1*/
gx_printf(fmt, a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s,t,v,w,x,y,z)
    char *fmt;
{
    if ((gx_console & CONS_DEV) &&
	((printstate & PANICPRINT) == 0 || gx_level <= GX_PANIC))
    {
	int old_console = gx_console;
	int old_loop = xcons_kern_loop;

	xcons_kern_loop = 0;			/* not to xcons */
	gx_console |= SLU3_DEV;			/* not to graphics console */

	cprintf(fmt, a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s,t,v,w,x,y,z);

	gx_console = old_console;
	xcons_kern_loop = old_loop;
    }
}
/* end gx_printf */


gx_puts(s)
    char *s;
{
    register char c;

    while (c = *s++) {
	gx_putchar(c);
	if (c == '\n')
	    gx_putchar('\r');
    }
}
/* end gx_puts */


gx_putb(b)
    unsigned int b;
{
    if (b < 10)
	gx_putchar('0'+b);
    else {
	switch (b) {
	 case 10:
	    gx_putchar('a'); break;
	 case 11:
	    gx_putchar('b'); break;
	 case 12:
	    gx_putchar('c'); break;
	 case 13:
	    gx_putchar('d'); break;
	 case 14:
	    gx_putchar('e'); break;
	 case 15:
	    gx_putchar('f'); break;
	 default:
	    break;
	}
    }
}
/* end gx_putb */


gx_putn(n)
    unsigned int n;
{
    gx_printf("0x%8X", n);
}
/* end gx_putn */


gx_putd(n)
    int n;
{
    int buf[16];
    int i, sign = 0;

    if (n == 0) {
	gx_putb(0);
	return;
    }

    if (n < 0) {
	sign = 1;
	n = (-n);
    }

    for (i = 0; n > 0; n /= 10, i++)
    {
	buf[i] = (n % 10);
    }

    if (sign)
	gx_putchar('-');

    for (--i ; i >= 0; i--)
    {
	gx_putb(buf[i]);
    }
}
/* end gx_putd */

gx_putevent(qep)
    pmEvent *qep;
{
    gx_puts("gxkint: ");
    switch (qep->device)
    {
     case NULL_DEVICE:
	gx_puts("NUL"); break;
     case MOUSE_DEVICE:
	gx_puts("MSE"); break;
     case KEYBOARD_DEVICE:
	gx_puts("KBD"); break;
     case TABLET_DEVICE:
	gx_puts("TAB"); break;
     case AUX_DEVICE:
	gx_puts("AUX"); break;
     case CONSOLE_DEVICE:
	gx_puts("CON"); break;
     case KNOB_DEVICE:
	gx_puts("KNB"); break;
     case JOYSTICK_DEVICE:
	gx_puts("JOY"); break;
     default:
	gx_puts("dev ");
	gx_putn(qep->device);
    }
    gx_puts(" x"); gx_putd(qep->x);
    gx_puts(" y"); gx_putd(qep->y);

    switch (qep->type)
    {
     case BUTTON_UP_TYPE:
	gx_puts(" UP"); break;
     case BUTTON_DOWN_TYPE:
	gx_puts(" DWN"); break;
     case BUTTON_RAW_TYPE:
	gx_puts(" RAW"); break;
     case MOTION_TYPE:
	gx_puts(" MOV"); break;
     default:
	gx_puts(" typ ");
	gx_putn(qep->type);
    }

    gx_puts(" key ");
    gx_putn(qep->key);
    gx_puts("\n");
}
/* end gx_putevent */

#endif !gx_nodebug


gx_panic()
{
    panic("_gx_* required linkage not initialized!");
}

int *
gx_panic2()
{
    gx_panic();
}

gx_noop()				/* save the best for last... */
{
    return 0;
}
