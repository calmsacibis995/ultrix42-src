#ifndef lint
static char *sccsid = "@(#)xcons.c	4.1      (ULTRIX)  8/9/90";
#endif	lint

/************************************************************************
 *									*
 *			Copyright (c) 1988,89 by			*
 *		Digital Equipment Corporation, Maynard, MA		*
 *			All rights reserved.				*
 *									*
 *   This software is furnished under a license and may be used and	*
 *   copied  only  in accordance with the terms of such license and	*
 *   with the  inclusion  of  the  above  copyright  notice.   This	*
 *   software  or  any	other copies thereof may not be provided or	*
 *   otherwise made available to any other person.  No title to and	*
 *   ownership of the software is hereby transferred.			*
 *									*
 *   The information in this software is subject to change  without	*
 *   notice  and should not be construed as a commitment by Digital	*
 *   Equipment Corporation.						*
 *									*
 *   Digital assumes no responsibility for the use  or	reliability	*
 *   of its software on equipment which is not supplied by Digital.	*
 *									*
 ************************************************************************
 *
 * xcons.c
 *
 * xcons alternate console driver
 *
 * Modification history
 *
 *   4-Jul-90	Randall Brown
 *		Created file.
 */

#include "../h/types.h"
#include "../h/errno.h"
#include "../h/user.h"
#include "../h/proc.h"
#include "../h/ioctl.h"
#include "../h/tty.h"
#include "../h/conf.h"
#include "../h/file.h"
#include "../h/devio.h"
#include "../h/exec.h"
#include "../io/tc/xcons.h"

#define GRAPHIC_DEV	0x2	/* rpbfix: needs to be moved to a header file */

struct tty	xcons_tty[1];

/* rpbfix: determine how xcons_kern_loop should be set */
int	xcons_kern_loop = 1;


int 	xconsstart();

extern int consDev, printstate;

xconsopen(dev, flag)
	dev_t 	dev;
	int	flag;
{
        register struct tty *tp;
	register int unit = minor(dev);

	/* can not open xcons if graphic head is not console */
	if ((consDev != GRAPHIC_DEV) || (unit != XCONSDEV))
	    return (ENXIO);

	tp = &xcons_tty[unit];
	if (tp->t_state&TS_XCLUDE && u.u_uid!=0)
	    return (EBUSY);
	tp->t_addr = (caddr_t)tp;
	tp->t_oproc = xconsstart;

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
#ifdef O_NOCTTY
	/*
	 * Set state bit to tell tty.c not to assign this line as the 
	 * controlling terminal for the process which opens this line.
	 */
	if ((flag & O_NOCTTY) && (u.u_procp->p_progenv == A_POSIX))
	    tp->t_state |= TS_ONOCTTY;
#endif O_NOCTTY
	if ((tp->t_state&TS_ISOPEN) == 0) {
	    ttychars(tp);
	    tp->t_state = TS_ISOPEN|TS_CARR_ON;
	    tp->t_cflag = tp->t_cflag_ext = B9600;
	    tp->t_iflag_ext = 0;
	    tp->t_oflag_ext = 0;
	    tp->t_lflag_ext = 0;

	    tp->t_flags = RAW;
	    tp->t_iflag = 0;
	    tp->t_oflag = 0;
	    tp->t_cflag |= CS8|CREAD|HUPCL; 
	    tp->t_lflag = 0;
	    
	    tp->t_iflag |= IXOFF;	/* flow control for qconsole */
	}

        /*
 	 * Process line discipline specific open
 	 */
	return ((*linesw[tp->t_line].l_open)(dev, tp));
}

xconsclose(dev, flag)
	dev_t 	dev;
	int	flag;
{
        register struct tty *tp;
	register int unit = minor(dev);

	tp = &xcons_tty[unit];

	(*linesw[tp->t_line].l_close)(tp);
	ttyclose(tp);

	tty_def_close(tp);
}

xconsread(dev, uio)
	dev_t	dev;
	struct uio *uio;
{
	register int unit = minor(dev);
        register struct tty *tp = &xcons_tty[unit];

	return ((*linesw[tp->t_line].l_read)(tp, uio));
}

xconswrite(dev, uio)
	dev_t	dev;	
	struct uio *uio;
{
	register int unit = minor(dev);
        register struct tty *tp = &xcons_tty[unit];

	return ((*linesw[tp->t_line].l_write)(tp, uio));
}

xconsioctl(dev, cmd, data, flag)
	dev_t	dev;
	int	cmd;
	caddr_t	data;
	int	flag;
{
	register int error, unit = minor(dev);
        register struct tty *tp = &xcons_tty[unit];

	error = (*linesw[tp->t_line].l_ioctl)(tp, cmd, data, flag);
	if (error >= 0)
	    return (error);
	error = ttioctl(tp, cmd, data, flag);
	if (error >= 0)
	    return (error);
	switch (cmd) {
	  case DEVIOCGET:
	    break;

	  default:
	    if (u.u_procp->p_progenv == A_POSIX)
		return (EINVAL);
	    return (ENOTTY);
	    break;
	}
}

xcons_chkq()
{
        register struct tty *tp = &xcons_tty[XCONSDEV];

        if (((tp->t_state & TS_ISOPEN) == 0) || (printstate & PANICPRINT) ||
	    (xcons_kern_loop == 0)) {
	    return (XCONS_CLOSED);
	}
	
	if (tp->t_state & TS_TBLOCK) {
	    return (XCONS_BLOCKED);
	}

	return (XCONS_OK);
}
	

xconsrint(c)
	register char c;
{
        register struct tty *tp = &xcons_tty[XCONSDEV];

	(*linesw[tp->t_line].l_rint)(c, tp);
}

xconsstart(tp)
	register struct tty *tp;
{
        register struct tty *tp0;
	register int s, c;

	s = spltty();

	tp0 = &cdevsw[0].d_ttys[0];

	while (tp->t_outq.c_cc) {
	c = getc(&tp->t_outq);

	/* if START char call console's start routine */
	if (c == tp->t_cc[VSTART]) {
	    /* pass START char onto console to restart output */
	    (*linesw[tp0->t_line].l_rint)(tp0->t_cc[VSTART], tp0);
	}
	/* if STOP char call console's stop routine */
	else if (c == tp->t_cc[VSTOP]) {
	    if ((tp0->t_state&TS_TTSTOP) == 0) {
		tp0->t_state |= TS_TTSTOP;
		(*cdevsw[0].d_stop)(tp0, 0);
	    }
	}
    }
	splx(s);
}

xconsstop(tp)
	register struct tty *tp;
{
}
