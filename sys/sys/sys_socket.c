#ifndef lint
static	char	*sccsid = "@(#)sys_socket.c	4.2	(ULTRIX)	4/30/91";
#endif lint

/***********************************************************************
 *									*
 *			Copyright (c) 1985 by				*
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
 **********************************************************************/

/************************************************************************
 *			Modification History				*
 *
 * 28-Apr-91 -- jsd, jaw
 *	Unlock socket after call to sbselqueue, not before
 *
 * 16-Apr-90 -- jaw
 *	performance fixes for single cpu.
 *
 *  9-Nov-89 -- Ursula Sinkewicz
 *	Added a setjmp to soo_close so if we sleep in soclose, and the
 *	user does a ctrl/c, we back up to the file system to release
 *	locks.  Otherwise, we exit with a file system lock held.
 *
 * 30-May-89 -- U. Sinkewicz
 *	Replaced smp_lock((&so->lk_socket, LK_RETRY) with SO_LOCK.
 *	Fixes smp problem of unlocking the socket (to accomodate the
 *	hierarchy or a sleep) then relocking the socket.  Problem without
 *	the SO_LOCK fix is that the socket lock could be on a freed socket
 *	pointer.
 *
 * 12-May-89 -- U. Sinkewicz
 *	Fortified socket locks.
 *
 * 09-May-89 -- Michael G. McMenemy
 *      Add XTI support.
 *
 * 10-Feb-88 --	prs							*
 *	Modified to support new fifo code.				*
 *									*
 * 15-Jan-88 --	lp							*
 *	Merge of final 43BSD changes. Add exception oob.		*
 *									*
 * 11 Nov 85 -- depp							*
 *	Removed all conditional compiles for System V IPC.		*
 *									*
 * 09/16/85 -- Larry Cohen						*
 * 		Add 43bsd alpha tape changes  				*
 *									*
 * 16 Apr 85 -- depp							*
 *	Added code to "soo_rw" routine that will properly handle	*
 *	FNDELAY if the file/socket is a named pipe.  This fix will	*
 *	now cause the process not to block in a normal blocking		*
 *	situation and if FNDELAY is set.				*
 *									*
 * 15 Mar 85 -- funding							*
 *	Added named pipe support (re. System V named pipes)		*
 *									*
 *	David L. Ballenger, 28-Nov-1984					*
 * 001	Add fixes so that fstat() calls on pipes will set up		*
 *	st_blksize. This will cause I/O on pipes to be buffered.	*
 *									*
 **********************************************************************/
/*	sys_socket.c	6.1	83/07/29	*/

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/file.h"
#include "../h/mbuf.h"
#include "../h/protosw.h"
#include "../h/socket.h"
#include "../h/socketvar.h"
#include "../h/ioctl.h"
#include "../h/uio.h"
#include "../h/stat.h"
#include "../h/gnode.h"

#include "../net/net/if.h"
#include "../net/net/route.h"

int	soo_rw(), soo_ioctl(), soo_select(), soo_close();
struct	fileops socketops =
    { soo_rw, soo_ioctl, soo_select, soo_close };

soo_rw(fp, rw, uio)
	struct file *fp;
	enum uio_rw rw;
	struct uio *uio;
{
	int soreceive(), sosend();

	return (
	    (*(rw==UIO_READ?soreceive:sosend))
	      ((struct socket *)fp->f_data, 0, uio, 0, 0));
}
soo_ioctl(fp, cmd, data)
	struct file *fp;
	int cmd;
	register caddr_t data;
{
	register struct socket *so = (struct socket *)fp->f_data;
	int error = 0;
	int s; /* SMP */

	s = splnet(); /* SMP */
	SO_LOCK(so);

#ifdef XTI
	if (so->so_xticb.xti_epvalid && fp->f_flag & FNBLOCK) {
	  if (!(so->so_state & SS_NBIO))
	    so->so_state |= SS_NBIO;
	}
#endif XTI
	switch (cmd) {
	case FIONBIO:
		if (*(int *)data)
			so->so_state |= SS_NBIO;
		else
			so->so_state &= ~SS_NBIO;
		smp_unlock(&so->lk_socket);
		splx(s); /* SMP */
		return(0);

	case FIOASYNC:
		if (*(int *)data)
			so->so_state |= SS_ASYNC;
		else
			so->so_state &= ~SS_ASYNC;
		smp_unlock(&so->lk_socket);
		splx(s); /* SMP */
		return(0);

	case FIONREAD:
		*(int *)data = so->so_rcv.sb_cc;
		smp_unlock(&so->lk_socket);
		splx(s); /* SMP */
		return(0);

	case SIOCSPGRP:
		so->so_pgrp = *(int *)data;
		smp_unlock(&so->lk_socket);
		splx(s); /* SMP */
		return(0);

	case SIOCGPGRP:
		*(int *)data = so->so_pgrp;
		smp_unlock(&so->lk_socket);
		splx(s); /* SMP */
		return(0);

	case SIOCATMARK:
		*(int *)data = (so->so_state&SS_RCVATMARK) != 0;
		smp_unlock(&so->lk_socket);
		splx(s); /* SMP */
		return(0);
	}
	/*
	 * Interface/routing/protocol specific ioctls:
	 * interface and routing ioctls should have a
	 * different entry since a socket's unnecessary
	 */
#define	cmdbyte(x)	(((x) >> 8) & 0xff)
	if (cmdbyte(cmd) == 'i') {
		error = (ifioctl(so, cmd, data));
		goto release;
	}
	if (cmdbyte(cmd) == 'r') {
		error = (rtioctl(cmd, data));
		goto release;
	}
	error = ((*so->so_proto->pr_usrreq)(so, PRU_CONTROL, 
	    (struct mbuf *)cmd, (struct mbuf *)data, (struct mbuf *)0));

release: /* SMP */
	smp_unlock(&so->lk_socket);
	splx(s);
	return(error);
}

soo_select(fp, which)
	struct file *fp;
	int which;
{
	register struct socket *so = (struct socket *)fp->f_data;
	
	 splnet();

	switch (which) {

	case FREAD:
		if (smp) {
			SO_LOCK(so);
			if (soreadable(so)) {
				smp_unlock(&so->lk_socket);
				spl0();
				return (1);
			}
			sbselqueue(&so->so_rcv);
			smp_unlock(&so->lk_socket);
		} else {
			if (soreadable(so)) {
				spl0();
				return (1);
			}
			sbselqueue(&so->so_rcv);
		}
		break;

	case FWRITE:
		if (smp) {
			SO_LOCK(so);
			if (sowriteable(so)) {
				smp_unlock(&so->lk_socket);
				spl0();
				return (1);
			}
			sbselqueue(&so->so_snd);
			smp_unlock(&so->lk_socket);
		} else {
			if (sowriteable(so)) {
				spl0();
				return (1);
			}
			sbselqueue(&so->so_snd);
		}
		break;
	case 0: /* Exception handling */
		if (smp) {
			SO_LOCK(so);
			if (so->so_oobmark ||
			    (so->so_state & SS_RCVATMARK)) {
				smp_unlock(&so->lk_socket);
				spl0();
				return (1);
			}
			sbselqueue(&so->so_rcv);
			smp_unlock(&so->lk_socket);
		} else {
			if (so->so_oobmark ||
			    (so->so_state & SS_RCVATMARK)) {
				spl0();
				return (1);
			}
			sbselqueue(&so->so_rcv);
			}
		break;
	}
	spl0();
	return (0);
}

/*ARGSUSED*/
soo_stat(so, ub)
	register struct socket *so;
	register struct stat *ub;
{

#ifdef lint
	so = so;
#endif
	bzero((caddr_t)ub, sizeof (*ub));
	return ((*so->so_proto->pr_usrreq)(so, PRU_SENSE,	/*DLB001*/
	    (struct mbuf *)ub, (struct mbuf *)0, 
	    (struct mbuf *)0));
}

soo_close(fp)
	struct file *fp;
{
	int error = 0;
	
	if (setjmp(&u.u_qsave)) {
		if (u.u_error == 0)
			u.u_error = EINTR;
		return(u.u_error);
	}
	if (fp->f_data)
		error = soclose((struct socket *)fp->f_data);
	fp->f_data = 0;
	return (error);
}


