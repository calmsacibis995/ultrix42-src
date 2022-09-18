#ifndef lint
static	char	*sccsid = "@(#)gfs_namei.c	4.4	(ULTRIX)	3/7/91";
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


/***********************************************************************
 *
 *		Modification History
 *
 * 18 Feb 91 -- prs
 *	Added SAVE_DIRP logic which saves the name string across
 *	name translations. Since rename() calles namei several times
 *	during the operation and namei can overwrite ni_dirp when
 *	symbolic links are followed, SAVE_DIRP guarentees a consistent
 *	name string.
 *
 * 21 Jan 91 -- prs
 *	If u.u_error is set coming into this routine,
 *	return NULL. Previously, if gfs_namei() was
 *	called with u.u_error set, it would return the
 *	starting directory, instead of NULL. This caused
 *	ufs_rename() to panic.
 *
 * 16 Oct 89 -- scott
 *	fix use of gnode dev # for audit
 *
 * 09 Jun 89 -- scott
 *	Added audit support
 *
 * 09 Mar 88 -- prs
 *      Changed gfs_namei to return ENOENT if the pathname is NULL,
 *      and the process is running in POSIX or SYSTEM_V mode.
 *
 * 11 Sep 86 -- koehler
 *	changed the namei interface
 *
 ***********************************************************************/


#include "../h/param.h"
#include "../h/systm.h"
#include "../h/gnode.h"
#include "../h/mount.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/buf.h"
#include "../h/conf.h"
#include "../h/uio.h"
#include "../h/kernel.h"
#include "../h/kmalloc.h"
#include "../h/proc.h"
#include "../h/exec.h"

#ifdef GFSDEBUG
extern short GFS[];
#endif

char *
namei_strcpy(dst, src)
register char *dst, *src;
{
	while (*dst++ = *src++)
		continue;
	return (dst - 1);
}

struct gnode *
gfs_namei(ndp)
	register struct nameidata *ndp;
{
	register char *cp;		/* pointer into pathname argument */
/* these variables refer to things which must be freed or unlocked */
	register struct gnode *dp = 0;	/* the directory we are searching */
	register u_int lockparent;
	register u_int flag;
	extern struct gnode *rootdir;
	char *save_dirp = 0;

	if (u.u_error)
		return(NULL);	
	lockparent = ndp->ni_nameiop & LOCKPARENT;
	flag = ndp->ni_nameiop &~ (LOCKPARENT|NOCACHE|FOLLOW);
	if (flag == DELETE || lockparent)
		ndp->ni_nameiop &= ~NOCACHE;


	/*
	 * pathname always exist in kernel space at this point.
	 * set up the call to the SFS and go
	 */
		
	/*
	 * Get starting directory.
	 */

	if (ndp->ni_nameiop & SAVE_DIRP) {
		KM_ALLOC(save_dirp, char *, MAXPATHLEN, KM_NAMEI, KM_NOARG);
		namei_strcpy(save_dirp, ndp->ni_dirp);
		ndp->ni_nameiop &= ~SAVE_DIRP;
	}
	cp = ndp->ni_dirp;
	if (*cp == '/') {
		while (*cp == '/')
			cp++;
		if ((dp = u.u_rdir) == NULL)
			dp = rootdir;
	} else {
	        /*
		 * If running in POSIX or SYSTEM_V mode, return ENOENT
		 * when the pathname is NULL.
		 */
		if ((u.u_procp->p_progenv & (A_POSIX | A_SYSV)) && (*cp == '\0')) {
			if (save_dirp) {
				KM_FREE(save_dirp, KM_NAMEI);
				ndp->ni_nameiop |= SAVE_DIRP;
			}
			u.u_error = ENOENT;
			return(NULL);
		}
		dp = u.u_cdir;
	}

	gref(dp);
	gfs_lock(dp);
	ndp->ni_pdir = dp;
	ndp->ni_endoff = 0;
	ndp->ni_slcnt = 0;
	
	/* search though the path calling the sfs's when needed */

	ndp->ni_cp = cp;

#ifdef GFSDEBUG
	if(GFS[1])
		cprintf("gfs_namei: pdir 0x%x (%d)\n", dp, dp->g_number);
#endif
	dp->g_flag |= GINCOMPLETE;
	while (!u.u_error && (dp->g_flag & GINCOMPLETE)) {
#ifdef GFSDEBUG
		if(GFS[1])
			cprintf("gfs_namei: dp 0x%x (%d) type %d %s count %d\n",
			dp, dp->g_number, dp->g_mp->m_fstype, glocked(dp)
		        ? "locked" : "NOT LOCKED", dp->g_count);
#endif
		dp = OPS(dp)->go_namei(ndp);

		if(dp == NULL) {
#ifdef GFSDEBUG
			if(GFS[1])
				cprintf ("gfs_namei: sfs_namei return NULL, pdir 0x%x\n",
				ndp->ni_pdir);
#endif
			break;
		}
#ifdef GFSDEBUG
		if(GFS[1])
			cprintf ("gfs_namei: flags 0%o count %d name '%s'\n",
			dp->g_flag, dp->g_count, ndp->ni_cp);
#endif
	}
	/* store gnode info in u_area for audit */
	if ( DO_AUDIT(u.u_event) && u.u_gno_indx < 2 && dp != NULL ) {
		u.u_gno_dev[u.u_gno_indx] = (dp->g_mode&GFMT) == GFCHR ? dp->g_rdev : dp->g_dev;
		u.u_gno_num[u.u_gno_indx] = dp->g_number;
		u.u_gno_indx++;
	}
	if (save_dirp) {
		namei_strcpy(ndp->ni_dirp, save_dirp);
		KM_FREE(save_dirp, KM_NAMEI);
		ndp->ni_nameiop |= SAVE_DIRP;
	}
	return(dp);
}
