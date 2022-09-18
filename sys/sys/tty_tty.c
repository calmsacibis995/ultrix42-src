
#ifndef lint
static char *sccsid = "@(#)tty_tty.c	4.1	ULTRIX	7/2/90";
#endif
 
/************************************************************************
 *									*
 *			Copyright (c) 1987 by				*
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
 
/************************************************************************
 *			Modification History				*
 *									*
 * 12-28-87	Tim Burke
 *  	Moved u.u_ttyp to u.u_procp->p_ttyp.
 */

/*
 * Indirect driver for controlling tty.
 */
#include "../h/param.h"
#include "../h/systm.h"
#include "../h/conf.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/ioctl.h"
#include "../h/tty.h"
#include "../h/proc.h"
#include "../h/uio.h"
#include "../h/cpudata.h"

/*ARGSUSED*/
syopen(dev, flag)
	dev_t dev;
	int flag;
{
	int error,saveaffinity;

	if (u.u_procp->p_ttyp == NULL)
		return (ENXIO);

	CALL_TO_NONSMP_DRIVER(cdevsw[major(u.u_ttyd)],saveaffinity);
	error = (*cdevsw[major(u.u_ttyd)].d_open)(u.u_ttyd, flag);
	RETURN_FROM_NONSMP_DRIVER(cdevsw[major(u.u_ttyd)],saveaffinity);

	return (error);
}

/*ARGSUSED*/
syread(dev, uio)
	dev_t dev;
	struct uio *uio;
{
	int error,saveaffinity;

	if (u.u_procp->p_ttyp == NULL)
		return (ENXIO);
	CALL_TO_NONSMP_DRIVER(cdevsw[major(u.u_ttyd)],saveaffinity);
	error =  (*cdevsw[major(u.u_ttyd)].d_read)(u.u_ttyd, uio);
	RETURN_FROM_NONSMP_DRIVER(cdevsw[major(u.u_ttyd)],saveaffinity);
	return(error);
}

/*ARGSUSED*/
sywrite(dev, uio)
	dev_t dev;
	struct uio *uio;
{
	int error,saveaffinity;

	if (u.u_procp->p_ttyp == NULL)
		return (ENXIO);
	CALL_TO_NONSMP_DRIVER(cdevsw[major(u.u_ttyd)],saveaffinity);
	error = (*cdevsw[major(u.u_ttyd)].d_write)(u.u_ttyd, uio);
	RETURN_FROM_NONSMP_DRIVER(cdevsw[major(u.u_ttyd)],saveaffinity);
	return (error);
}

/*ARGSUSED*/
syioctl(dev, cmd, addr, flag)
	dev_t dev;
	int cmd;
	caddr_t addr;
	int flag;
{

	int error,saveaffinity;

	if (cmd == TIOCNOTTY) {
		u.u_procp->p_ttyp = 0;
		u.u_ttyd = 0;
		u.u_procp->p_pgrp = 0;
		return (0);
	}
	if (u.u_procp->p_ttyp == NULL)
		return (ENXIO);
	CALL_TO_NONSMP_DRIVER(cdevsw[major(u.u_ttyd)],saveaffinity);
	error = (*cdevsw[major(u.u_ttyd)].d_ioctl)(u.u_ttyd, cmd, addr, flag);
	RETURN_FROM_NONSMP_DRIVER(cdevsw[major(u.u_ttyd)],saveaffinity);
	return (error);
}

/*ARGSUSED*/
syselect(dev, flag)
	dev_t dev;
	int flag;
{
	int error,saveaffinity;

	if (u.u_procp->p_ttyp == NULL) {
		u.u_error = ENXIO;
		return (0);
	}
	CALL_TO_NONSMP_DRIVER(cdevsw[major(u.u_ttyd)],saveaffinity);
	error = (*cdevsw[major(u.u_ttyd)].d_select)(u.u_ttyd, flag);
	RETURN_FROM_NONSMP_DRIVER(cdevsw[major(u.u_ttyd)],saveaffinity);
	return (error);
}
