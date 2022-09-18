#ifndef lint
static char *sccsid = "@(#)gfs_fio.c	4.2	ULTRIX	11/9/90";
#endif

/************************************************************************
 *									*
 *			Copyright (c) 1986,87 by			*
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
 *		Modification History
 *
 * 12-11-87	Robin L. and Larry C.
 *	Added new kmalloc memory allocation to system.
 *
 * 11 Sep 86 -- koehler
 *	made changes for M_NOEXEC & M_NODEV also changed namei interface
 *
 ***********************************************************************/


#include "../machine/reg.h"

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/file.h"
#include "../h/conf.h"
#include "../h/gnode.h"
#include "../h/buf.h"
#include "../h/acct.h"
#include "../h/mount.h"
#include "../h/socket.h"
#include "../h/socketvar.h"
#include "../h/proc.h"
#include "../h/sysproc.h"
#include "../h/kmalloc.h"

#ifdef GFSDEBUG
extern short GFS[];
#endif

/*
 * Test if the current user is the
 * super user.
 */
suser()
{

	if (u.u_uid == 0) {
		u.u_acflag |= ASU;
		return (1);
	}
	u.u_error = EPERM;
	return (0);
}


/*
 * Check mode permission on gnode pointer.
 * Mode is READ, WRITE or EXEC.
 * In the case of WRITE, the
 * read-only status of the file
 * system is checked.
 * Also in WRITE, prototype text
 * segments cannot be written.
 * The mode is shifted to select
 * the owner/group/other fields.
 * The super user is granted all
 * permissions.
 */
access(gp, mode)
	register struct gnode *gp;
	register int mode;
{
	register int m;
	register int *groups;
	
	/*
	 * Refresh gnode attributes.
	 */	
	GGETVAL(gp);
	if (u.u_error)
		return(1);

	m = mode;
#ifdef GFSDEBUG
	if(GFS[1])
		cprintf("access: gp 0x%x (%d) mode 0%o uid %d\n",
			gp, gp->g_number, mode,u.u_uid);
#endif
	if (m == GWRITE) {
		/*
		 * Disallow write attempts on read-only
		 * file systems; unless the file is a block
		 * or character device resident on the
		 * file system.
		 */
		if(ISREADONLY(gp->g_mp)) {
			if ((gp->g_mode & GFMT) != GFCHR &&
			    (gp->g_mode & GFMT) != GFBLK) {
				u.u_error = EROFS;
				return (1);
			}
		}
		/*
		 * If there's shared text associated with
		 * the inode, try to free it up once.  If
		 * we fail, we can't allow writing.
		 */
		if (gp->g_flag&GTEXT)
			xrele(gp);
		if (gp->g_flag & GTEXT) {
#ifdef GFSDEBUG
			if(GFS[1])
				cprintf("access: text busy\n");
#endif
			u.u_error = ETXTBSY;
			return (1);
		}
	}
	/*
	 * If you're the super-user,
	 * you always get access.
	 */
	if (u.u_uid == 0)
		return (0);
	
	/*
	 * NO access allowed to block or character devices when MNODEV
	 * is set
	 */
	
	if((((gp->g_mode & GFMT) == GFBLK) || ((gp->g_mode & GFMT) ==
	GFCHR)) && (gp->g_mp->m_flags & M_NODEV)) {
		u.u_error = EROFS;
		return(1);
	}
	
	/*
	 * Access check is based on only
	 * one of owner, group, public.
	 * If not owner, then check group.
	 * If not a member of the group, then
	 * check public access.
	 */
	if (u.u_uid != gp->g_uid) {
		m >>= 3;
		if (u.u_gid == gp->g_gid)
			goto found;
		groups = u.u_groups;
		for (; groups < &u.u_groups[NGROUPS] && *groups != NOGROUP;
		groups++)
			if (gp->g_gid == *groups)
				goto found;
		m >>= 3;
found:
		;
	}
#ifdef GFSDEBUG
	if(GFS[1])
		cprintf("access: mode 0x%x m 0x%x\n", gp->g_mode, m);
#endif
	if ((gp->g_mode&m) != 0)
		return (0);
	u.u_error = EACCES;
	return (1);
}

/*
 * Look up a pathname and test if
 * the resultant inode is owned by the
 * current user.
 * If not, try for super-user.
 * If permission is granted,
 * return inode pointer.
 */
struct gnode *
owner(fname, follow)
	register caddr_t fname;
	register int follow;
{
	register struct gnode *gp;
 	register struct nameidata *ndp = &u.u_nd;
	register int ret;
	
 	ndp->ni_nameiop = LOOKUP | follow;
	KM_ALLOC(ndp->ni_dirp, char *, MAXPATHLEN, KM_NAMEI, KM_NOARG);
	if(ndp->ni_dirp == NULL) {
		u.u_error = EIO;
		return(NULL);
	}
 	if(u.u_error = copyinstr(fname, ndp->ni_dirp, MAXPATHLEN, (u_int *)
	0)) {
		KM_FREE(ndp->ni_dirp, KM_NAMEI);
		return(NULL);
	}

 	gp = GNAMEI(ndp);

	KM_FREE(ndp->ni_dirp, KM_NAMEI);
	
	/* allow any file systems to update the gp */

	if (gp == NULL)
		return (NULL);
	ret = GGETVAL(gp);
	if (u.u_uid == gp->g_uid)
		return (gp);
	if (suser())
		return (gp);
	gput(gp);
	return (NULL);
}
