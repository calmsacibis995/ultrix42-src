#ifndef lint
static	char	*sccsid = "@(#)cdfs_syscalls.c	4.1	(ULTRIX)	11/9/90";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1986 by				*
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
 *			Modification History
 *	fs/cdfs/cdfs_syscalls.c
 *
 *  9-Nov-90 -- prs
 *	Initial creation.
 *
 ***********************************************************************/
#include "../h/param.h"
#include "../h/systm.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/kernel.h"
#include "../h/file.h"
#include "../h/stat.h"
#include "../h/gnode.h"
#include "../fs/cdfs/cdfs_fs.h"
#include "../h/buf.h"
#include "../h/proc.h"
#include "../h/quota.h"
#include "../h/uio.h"
#include "../h/socket.h"
#include "../h/socketvar.h"
#include "../h/mount.h"
#include "../h/ioctl.h"
#include "../h/limits.h"
#include "../h/text.h"

cdfs_link(source_gp, target_ndp)
	register struct	gnode *source_gp;
	register struct nameidata *target_ndp;
{
	return(u.u_error = EROFS);
}

cdfs_unlink(gp, ndp)
	register struct gnode *gp;
	register struct nameidata *ndp;
{

	return(u.u_error = EROFS);

}

cdfs_rename(gp, ssd, source_ndp, tsd, target_ndp, flag)
	register struct gnode *gp;
	struct gnode *ssd, *tsd;
	register struct nameidata *source_ndp;
	register struct nameidata *target_ndp;
	int flag;
{
	return(u.u_error = EROFS);
}

struct gnode *
cdfs_maknode(mode, dev, ndp)
	register int mode;
        register dev_t dev;
	register struct nameidata *ndp;
{
	register struct gnode *gp;
	register struct gnode *pdir = ndp->ni_pdir;
	register gno_t gpref;
	int type;

	u.u_error = EROFS;
	return((struct gnode *)NULL);
}

struct gnode *
cdfs_mkdir(dp, name, mode)
	register struct gnode *dp;
	register int mode;
	register char *name;
{
	u.u_error = EROFS;
	return((struct gnode *)NULL);
}


cdfs_rmdir(gp, ndp)
	register struct gnode *gp;
	register struct nameidata *ndp;
{
	return(u.u_error = EROFS);
}

cdfs_seek(gp, where)
	struct gnode *gp;
	int where;
{
	return(0);
}
