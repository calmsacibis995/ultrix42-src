#ifndef lint
static char *sccsid = "@(#)kern_tpath.c	2.3    ULTRIX  6/12/89";
#endif

/************************************************************************
 *									*
 *			Copyright (c) 1986, 1988 by			*
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
 *   This software is  derived	from  software	received  from	the	*
 *   University    of	California,   Berkeley,   and	from   Bell	*
 *   Laboratories.  Use, duplication, or disclosure is	subject  to	*
 *   restrictions  under  license  agreements  with  University  of	*
 *   California and with AT&T.						*
 *									*
 *   The information in this software is subject to change  without	*
 *   notice  and should not be construed as a commitment by Digital	*
 *   Equipment Corporation.						*
 *									*
 *   Digital assumes no responsibility for the use  or	reliability	*
 *   of its software on equipment which is not supplied by Digital.	*
 *									*
 ************************************************************************/

#include "../h/param.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/proc.h"
#include "../h/gnode.h"
#include "../h/conf.h"
#include "../h/file.h"
#include "../h/tty.h"
#include "../h/sys_tpath.h"

int	tpath_handler = 0;		
extern	struct	lock_data lock_tpath_d;
extern	struct	lock_t	lk_tpath;
extern	struct	cdevsw 	cdevsw[];

#define D_TO_TTY(dev) \
	(cdevsw[major((dev))].d_ttys \
	? (struct tty *)&cdevsw[major(dev)].d_ttys[minor(dev)] \
	: (struct tty *)0)

/*
 * The tpath handler daemon (process 3).
 *
 * Wait on tpath_handler for a wakeup.  Once awake find a file in
 * the file table that is associated with a device which is requesting
 * trusted path (i.e., tp->t_tpath & TP_DOLOGIN).  When a device is found, 
 * call forceclose and kill the session leader associated with the device to
 * notify init.  If there are any outstanding wakeups (i.e., tpath_handler > 0),
 * continue processing.
 *
 */
tpath_sysproc() 
{
	register struct file 	*fp;
	register struct gnode 	*gp;
	struct	 tty		*tp;
	struct	 proc		*pp;
	dev_t			dev;	/* Device requesting trusted path */
	int			sid;	/* Session leader */
	int			s;

	lockinit(&lk_tpath, &lock_tpath_d);

again:
	TPATH_SLEEP;
top:
	smp_lock(&lk_file, LK_RETRY);
	for (fp = file; fp < fileNFILE; fp++) {
		if ((fp->f_count == 0) || (fp->f_type != DTYPE_INODE))
			continue;

		gp = (struct gnode *)fp->f_data;
		if ((gp == (struct gnode *)0) || ((gp->g_mode & GFMT) != GFCHR))
			continue;

		dev = gp->g_rdev;
		tp = D_TO_TTY(dev);
		if ((tp == (struct tty *)0) || ((tp->t_tpath & TP_DOLOGIN) == 0))
			continue; 

		TTY_LOCK(tp, s);
		tp->t_tpath &= ~TP_DOLOGIN;
		TTY_UNLOCK(tp, s);

		if ((sid = tp->t_sid) == 0)
			continue;		

		smp_unlock(&lk_file);
		forceclose(dev);
		if ((pp = proc_get(sid)) != (struct proc *)0) {
			psignal(pp, SIGKILL);
			proc_rele(pp);
		}
		goto top;
	}
	smp_unlock(&lk_file);
	goto again;
}
