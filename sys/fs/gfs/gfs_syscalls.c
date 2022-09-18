#ifndef lint
static	char	*sccsid = "@(#)gfs_syscalls.c	4.13	(ULTRIX)	5/2/91";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1986, 1987, 1988 by		*
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
 * 02 May 91 -- darrell
 *	Fixed a missing ")" in copen();
 *
 * 01 May 91 -- sue
 *	Back out previous change which was to fix copen() to prevent
 *	disappearing tape drives.  On VAX, caused problems using
 *	file command on /dev/tty*.
 *
 * 10 Apr 91 -- prs
 *	The setjmp case of copen() cannot call closef(). Simply
 *	deallocate the file table entry.
 *
 * 28 Feb 91 -- prs
 *	Added support for a configurable number of open
 *	file descriptors.
 *
 * 27 Feb 91 -- chet
 *	Fix filesystem timestamping.
 *
 * 21 Jan 91 -- prs
 *	Added zeroing the f_data field to the error case in copen().
 *	Also removed some obsolete vhangup() leftovers.
 *
 * 15 Oct 90 -- dws
 *	Fixed saccess() to handle 'noexec' filesystems correctly.
 *
 * 27 Sep 90 -- prs
 *	Prevented rename(from, ".") in rename().
 *
 * 17 May 90 -- prs
 *	Fixed bug in copen() which left an unreferenced file descriptor
 *	in the uarea.
 *
 * 17 Jan 90 -- prs
 *	Added the restriction that a POSIX process cannot link to
 *	a directory.
 *
 * 12 Jan 90 -- prs
 *	Fixed saccess() to restore euid and egid before returning.
 *
 * 08 Dec 89 -- cb
 *	Fixed getmnt to exit syscall after a ^C.
 * 
 * 08 Dec 89 -- prs
 *	Checked u.u_error after a call to GUPDATE in link().
 *	If the gupdate failed, don't bother doing the namei
 *	on the target.
 *
 * 14 Nov 89 -- prs
 *	Added POSIX check to prevent unlinking of TEXT gnodes.
 *
 * 25 Jul 89 -- chet
 *	New sync() system call
 *
 * 08 Jun 89 -- lebel
 * 	Fixed BLKANDSET code in copen - open always returned fd of 0
 * 	for character special files
 *
 * 04 Apr 89 -- prs
 *	Added SMP quota locking.
 *
 * 16 Feb 89 -- prs
 *      Added XOPEN check to prevent unlinking of TEXT gnodes.
 *
 * 08 Dec 88 -- prs
 *      Added POSIX mode access time setting
 *      in getdirentries().
 *
 * 09 Jan 89 -- condylis
 *	Modified sync and getmnt to run on any processor.
 *
 * 11 Oct 88 -- prs
 *	Changed rmdir() to pass the sfs a directory gp, and properly
 *	fixed the GNOFUNC error leg.
 *
 * 14 Sep 88 -- chet
 *	Fix another bug in link() due to GUPDATE calls on a readonly
 *	remote filesystem.
 *
 * 22 Aug 88 -- prs
 *	Changed copen() to only allow root to create a file with the
 *	sticky bit set.
 *
 * 04 Aug 88 -- chet
 *      Fix problems in link() due to GUPDATE calls that destroy u.u_error.
 *
 * 28 Jul 88 -- prs
 *	SMP - All system calls except sync() and rename() are safe.
 *
 * 23 Feb 88 -- map
 *	Fix problem in chown(). In some cases it would fail but not return
 *	an errno.
 *
 * 10 Feb 88 -- map
 *	Rewrite utimes() to allow POSIX and SVID utime() behavior. The utime()
 *	library routine calls utimes(). A NULL time parameter will now default
 *	to the current time as specified in the utime() function. If the time
 *	parameter is NULL EITHER the owner(or su) OR a process with write
 *	access can update the time.  This is not a security hole as a user
 *	with write access can update the time through other means prior to
 *	this change.
 * 
 * 10 Feb 88 -- prs
 *	Modified to handle new fifo code.
 *
 * 08 Feb 88 -- prs
 *	Fixed lseek to return an error if the target offset is negative
 *	for a regular file.
 *
 * 26 Jan 88 -- chet
 *	Removed access check in gfs_getdirentries(). This
 *	check has been moved down to the specific filesystems.
 *
 * 08 Jan 88 -- prs
 *	Removed lseek change that checked for a negative offset.
 *
 * 04 Jan 88 -- prs
 *	Added a check to link() that verified link count of gnode
 *	did not exceed LINK_MAX.
 *
 * 12 Dec 87 -- prs
 *	Fixed lseek to return an error if the target offset is negative.
 *
 * 12-11-87	Robin L. and Larry C.
 *	Added new kmalloc memory allocation to system.
 *
 * 17 Nov 87 -- map
 *	Allow user to chown the group id of a file to another group
 *	to which they belong. Allows chgrp to not be suid. POSIX requirement.
 *
 * 15 Sep 87 -- prs
 *	Changed copen to validate the fp after GOPEN call.
 *
 * 09 Sep 87 -- map
 *	saccess() changed to check for invalid mode. POSIX requirement
 *
 * 19 Aug 87 -- cb
 *      getmnt() returned to original state.  readlink changed to return
 *      EINVAL if file is not a symbolic link.
 *
 * 13 Aug 87 -- cb
 *	Fixed getmnt so it will parse a block device path correctly
 *
 * 14 Jul 87 -- cb
 *	Changed mknod interface: added a dev_t.
 *
 * 11 Jun 87 -- prs
 *	Free cred if call to GOPEN fails in copen routine.
 *
 * 12 May 87 -- chet
 *	Added new getmnt() interface code.
 *
 * 03 Mar 87 -- chet
 *	Unlocked gnode before doing GSTAT so that locks won't
 *	propagate up file tree when a slow operation takes place.
 *
 * 13 Feb 87 -- prs
 *	Removed check for m_dev to be null in sync. The check prevented
 *	sync on a device with major number 0.
 *
 * 15 January 86 -- Chase
 *	added code to check for failure on truncate in copen()
 *
 * 11 Sept 86 -- koehler
 *	changes for new namei interface
 *
 * 12 Sept 86 -- koehler
 *	symlink errno change
 *
 * 2  Oct 86 -- Larry Cohen
 *	refix shared line bug in copen
 ***********************************************************************/

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/kernel.h"
#include "../h/file.h"
#include "../h/stat.h"
#include "../h/gnode.h"
#include "../h/buf.h"
#include "../h/proc.h"
#include "../h/quota.h"
#include "../h/uio.h"
#include "../h/socket.h"
#include "../h/socketvar.h"
#include "../h/mount.h"
#include "../h/ioctl.h"
#include "../h/kmalloc.h"
#include "../h/exec.h"
#include "../h/limits.h"

extern	struct gnode *fref();

link() 
{
        register struct gnode *source_gp, *target_pgp;
        register struct a {
                char    *source;
                char    *target;
        } *uap = (struct a *)u.u_ap;
        register struct nameidata *ndp = &u.u_nd;
        int     error = 0;

        ndp->ni_nameiop = LOOKUP | FOLLOW;
        KM_ALLOC(ndp->ni_dirp, char *, MAXPATHLEN, KM_NAMEI, KM_NOARG);
        if (ndp->ni_dirp == NULL) {
                u.u_error = EIO;
                return;
        }
        if (u.u_error = copyinstr(uap->source, ndp->ni_dirp, MAXPATHLEN,
				  (u_int *) 0))
                goto free;

        source_gp = gfs_namei(ndp);
        if (source_gp == NULL)
                goto free;
        if (source_gp->g_nlink >= LINK_MAX) {
                gput(source_gp);
                u.u_error = EMLINK;
                goto free;
        }
        if (((source_gp->g_mode & GFMT) == GFDIR) && 
	     (!suser() || (u.u_procp->p_progenv == A_POSIX))) {
                gput(source_gp);
		u.u_error = EPERM;
                goto free;
        }

        if (u.u_error = copyinstr(uap->target, ndp->ni_dirp, MAXPATHLEN,
				  (u_int *) 0)) {
                gput(source_gp);
                goto free;
        }

        /* this stuff is done here to avoid races in the fs */

        source_gp->g_nlink++;

	/*
	 * A lot of games are played here
	 * with error and u.u_error because of the GUPDATE calls
	 * that don't do anything useful except for UFS, and are guaranteed
	 * to clobber u.u_error for NFS.
	 * There is a subtle UFS case as well, where an I/O error
	 * on the inode update clobbers an already failing syscall's
	 * u.u_error with EIO.
	 *
	 * I finally decided to bite the bullet and put ISLOCAL
	 * checks around the GUPDATE calls - 9/8/88 - chet
	 */
	if (ISLOCAL(source_gp->g_mp)) {
		source_gp->g_flag |= GCHG;
		(void) GUPDATE(source_gp, timepick, timepick, 1, u.u_cred);
		if (u.u_error) {
			error = u.u_error;
			gfs_unlock(source_gp);
			goto errout;
		}
	}
        gfs_unlock(source_gp);

        ndp->ni_nameiop = CREATE;
        target_pgp = gfs_namei(ndp);

        if (u.u_error) {
                error = u.u_error;      /* save the error code */
                goto errout;
        }

        if (target_pgp != NULL) {
                gput(target_pgp);
                error = EEXIST;         /* save the error code */
                goto errout;
        }

        target_pgp = ndp->ni_pdir;

        if (target_pgp->g_mp != source_gp->g_mp) {
                gput(target_pgp);
                error = EXDEV;          /* save the error code */
                goto errout;
        }

        /*
         * LINK should take three arguments, gp of source, gp of parent
         * of source, and source component.
         */

        /* The fs specific routine does all de-referencing */
        if (GLINK(source_gp, ndp) == GNOFUNC) {
                u.u_error = EOPNOTSUPP;
                gput(target_pgp);
                grele(source_gp);
        }
        goto free;

errout:
        gfs_lock(source_gp);
        source_gp->g_nlink--;
	if (ISLOCAL(source_gp->g_mp)) {
		source_gp->g_flag |= GCHG;
		(void) GUPDATE(source_gp, timepick, timepick, 1, u.u_cred);
	}
        gput(source_gp);
        u.u_error = error; /* restore the error code after GUPDATE*/

free:
        KM_FREE(ndp->ni_dirp, KM_NAMEI);

}

unlink()
{
	register struct a {
		char	*fname;
	} *uap = (struct a *)u.u_ap;
	register struct gnode *gp, *dp;
	register struct nameidata *ndp = &u.u_nd;
	register int ret;
	
	ndp->ni_nameiop = DELETE | LOCKPARENT;

	KM_ALLOC(ndp->ni_dirp, char *, MAXPATHLEN, KM_NAMEI, KM_NOARG);
	if (ndp->ni_dirp == NULL) {
		u.u_error = EIO;
		return;
	}
 	if (u.u_error = copyinstr(uap->fname, ndp->ni_dirp, MAXPATHLEN,
				  (u_int *) 0)) {
	         goto out2;
	}
	gp = gfs_namei(ndp);

	if (gp == NULL) {
	         goto out2;
	}
	
	dp = (struct gnode *)ndp->ni_pdir;
	/* only non POSIX root can unlink a directory */

	if ((gp->g_mode&GFMT) == GFDIR) {
		if ((u.u_procp->p_progenv == A_POSIX) ||
		     !suser()) {
			if (u.u_error == 0)
				u.u_error = EPERM;
			goto out;
		}
	}
	/*
	 * Don't unlink a mounted file.
	 */
	if (gp->g_dev != dp->g_dev) {
		u.u_error = EBUSY;
		goto out;
	}
	if (gp->g_flag&GTEXT) {
		xrele(gp);	/* try once to free text */
	}
	/*
         * SysV and POSIX cannot remove the last link to a shared text file.
         */
        if ((u.u_procp->p_progenv & (A_POSIX|A_SYSV)) &&
            (gp->g_flag&GTEXT) && (gp->g_nlink == 1)) {
                u.u_error = ETXTBSY;
                goto out;
        }
	if (GUNLINK(gp, ndp) == GNOFUNC)
		u.u_error = EOPNOTSUPP;
out:
	if (dp == gp) {
		grele(gp);
		gput(gp);
	} else {
		gput(dp);
		gput(gp);
	}
out2:
        KM_FREE(ndp->ni_dirp, KM_NAMEI);
}


/*
 * Mkdir system call
 */
mkdir()
{
	register struct a {
		char	*name;
		int	dmode;
	} *uap = (struct a *) u.u_ap;
	register struct gnode *gp;
	register struct nameidata *ndp = &u.u_nd;

	uap = (struct a *)u.u_ap;
	ndp->ni_nameiop = CREATE;
	KM_ALLOC(ndp->ni_dirp, char *, MAXPATHLEN, KM_NAMEI, KM_NOARG);
	if (ndp->ni_dirp == NULL) {
		u.u_error = EIO;
		return;
	}
 	if (u.u_error = copyinstr(uap->name, ndp->ni_dirp, MAXPATHLEN,
				  (u_int *)0)) {
	        goto out;
	}
	
	gp = gfs_namei(ndp);
	if (u.u_error) {
	        goto out;
	}
	if (gp != NULL) {
		u.u_error = EEXIST;
		gput(gp);
	        goto out;
	}

	if ((gp = GMKDIR(ndp->ni_pdir, ndp->ni_dirp, uap->dmode)) == 
	    (struct gnode *) GNOFUNC)
		u.u_error = EOPNOTSUPP;
	else
		if (!u.u_error)
			grele(gp);
out:
	KM_FREE(ndp->ni_dirp, KM_NAMEI);
}

/*
 * Rmdir system call.
 */
rmdir()
{
	register struct a {
		char	*name;
	} *uap = (struct a *)u.u_ap;
	register struct gnode *gp;
	register struct nameidata *ndp = &u.u_nd;

	ndp->ni_nameiop = DELETE | LOCKPARENT;

	KM_ALLOC(ndp->ni_dirp, char *, MAXPATHLEN, KM_NAMEI, KM_NOARG);
	if (ndp->ni_dirp == NULL) {
		u.u_error = EIO;
		return;
	}
 	if (u.u_error = copyinstr(uap->name, ndp->ni_dirp, MAXPATHLEN,
				  (u_int *)0)) {
	        goto out;
	}

	gp = gfs_namei(ndp);
	if (gp == NULL) {
	        goto out;
	}
	
	/*
	 * GFS assures the sfs gp is a directory.
	 */
	if ((gp->g_mode & GFMT) != GFDIR) {
		u.u_error = ENOTDIR;
		goto bad;
	}
	
	if (GRMDIR(gp, ndp) == GNOFUNC) {
		u.u_error = EOPNOTSUPP;
		goto bad;
	}
	goto out;

bad:
	if (ndp->ni_pdir == gp)
		grele(ndp->ni_pdir);
	else
		gput(ndp->ni_pdir);
	gput(gp);

out:
	KM_FREE(ndp->ni_dirp, KM_NAMEI);
}


/*
 * mode mask for creation of files
 */
umask()
{
	register struct a {
		int	mask;
	} *uap = (struct a *)u.u_ap;

	u.u_r.r_val1 = u.u_cmask;
	u.u_cmask = uap->mask & 07777;
}

struct gnode *
maknode(mode, ndp)
	register int mode;
	register struct nameidata *ndp;
{
	if (GMAKNODE(mode, (dev_t) 0, ndp) == (struct gnode *) GNOFUNC)
		u.u_error = EOPNOTSUPP;
}

/*
 * Rename system call.
 * 	rename("foo", "bar");
 * is essentially
 *	unlink("bar");
 *	link("foo", "bar");
 *	unlink("foo");
 * but ``atomically''.  Can't do full commit without saving state in the
 * inode on disk which isn't feasible at this time.  Best we can do is
 * always guarantee the target exists.
 *
 * Basic algorithm is:
 *
 * 1) Bump link count on source while we're linking it to the
 *    target.  This also insure the gnode won't be deleted out
 *    from underneath us while we work (it may be truncated by
 *    a concurrent `trunc' or `open' for creation).
 * 2) Link source to destination.  If destination already exists,
 *    delete it first.
 * 3) Unlink source reference to gnode if still around.
 *    If a directory was moved and the parent of the destination
 *    is different from the source, patch the ".." entry in the
 *    directory.
 *
 * Source and destination must either both be directories, or both
 * not be directories.  If target is a directory, it must be empty.
 */
rename()
{
	register struct a {
		char	*from;
		char	*to;
	} *uap;
	register struct gnode *source_gp;
	register struct nameidata *source_ndp = &u.u_nd;
	struct nameidata target_ndp;
	
	/* get us a pointer to the from name */
	
	uap = (struct a *)u.u_ap;
	source_ndp->ni_nameiop = DELETE | LOCKPARENT;

	KM_ALLOC(source_ndp->ni_dirp, char *, MAXPATHLEN, KM_NAMEI, KM_NOARG);
	if (source_ndp->ni_dirp == NULL) {
		u.u_error = EIO;
		return;
	}
 	if (u.u_error = copyinstr(uap->from, source_ndp->ni_dirp, MAXPATHLEN,
				  (u_int *) 0)) {
	        goto out1;
	}

	KM_ALLOC(target_ndp.ni_dirp, char *, MAXPATHLEN, KM_NAMEI, KM_NOARG);
	if (target_ndp.ni_dirp == NULL) {
		u.u_error = EIO;
	        goto out1;
	}
	
 	if (u.u_error = copyinstr(uap->to, target_ndp.ni_dirp, MAXPATHLEN,
				  (u_int *) 0)) {
	        goto out2;
	}
	/*
	 * Prevent rename(from, ".")
	 */
	if (!strcmp(target_ndp.ni_dirp, ".")) {
		u.u_error = EINVAL;
		goto out2;
	}
	
	source_gp = gfs_namei(source_ndp);
	
	if (source_gp == NULL || u.u_error) {
	        goto out2;
	}
	
	/* XXX */
	/* this is UFS specific */
	if (ISLOCAL(source_gp->g_mp)) {
		if (u.u_error = copyinstr(uap->from, source_ndp->ni_dirp, 
					  MAXPATHLEN, (u_int *) 0)) {
	        goto out2;
		}
	}

	target_ndp.ni_nameiop = CREATE | LOCKPARENT | NOCACHE;
	
	if (GRENAMEG(source_gp, u.u_cdir, source_ndp, u.u_cdir, &target_ndp,
		     0)	== GNOFUNC) {
		u.u_error = EOPNOTSUPP;
		gput(source_gp);
		gput(source_ndp->ni_pdir);
	}
out2:
	KM_FREE(target_ndp.ni_dirp, KM_NAMEI);
out1:
	KM_FREE(source_ndp->ni_dirp, KM_NAMEI);
}

extern	struct fileops gnodeops;
extern	int	soo_rw(), soo_ioctl(), soo_select(), gno_close();
struct	fileops	portops =
    { soo_rw, soo_ioctl, soo_select, gno_close };
struct	file *getgnode();

/*
 * Change current working directory (``.'').
 */
chdir()
{

	chdirec(&u.u_cdir);
}

/*
 * Change notion of root (``/'') directory.
 */
chroot()
{

	if (suser())
		chdirec(&u.u_rdir);
}

/*
 * Common routine for chroot and chdir.
 */
chdirec(gpp)
	register struct gnode **gpp;
{
	register struct gnode *gp;
	register struct a {
		char	*fname;
	} *uap = (struct a *)u.u_ap;
	register struct nameidata *ndp = &u.u_nd;
	register int ret;
	
	ndp->ni_nameiop = LOOKUP | FOLLOW;

	KM_ALLOC(ndp->ni_dirp, char *, MAXPATHLEN, KM_NAMEI, KM_NOARG);
	if (ndp->ni_dirp == NULL) {
		u.u_error = EIO;
		return;
	}
 	if (u.u_error = copyinstr(uap->fname, ndp->ni_dirp, MAXPATHLEN,
				  (u_int *) 0)) {
	        goto out;
	}

	gp = gfs_namei(ndp);
	
	if (gp == NULL || u.u_error) 
	        goto out;
	if ((gp->g_mode&GFMT) != GFDIR) {
		u.u_error = ENOTDIR;
		goto bad;
	}
	if (access(gp, GEXEC))
		goto bad;
	gfs_unlock(gp);
	if (*gpp)
		grele(*gpp);
	*gpp = gp;
        goto out;

bad:
	gput(gp);
out:
	KM_FREE(ndp->ni_dirp, KM_NAMEI);
}

/*
 * Open system call.
 */
open()
{
	register struct a {
		char	*fname;
		int	mode;
		int	crtmode;
	} *uap = (struct a *) u.u_ap;

	copen(uap->mode-FOPEN, uap->crtmode, uap->fname);
}

/*
 * Creat system call.
 */
creat()
{
	register struct a {
		char	*fname;
		int	fmode;
	} *uap = (struct a *)u.u_ap;

	copen(FWRITE|FCREAT|FTRUNC, uap->fmode, uap->fname);
}

/*
 * Common code for open and creat.
 * Check permissions, allocate an open file structure,
 * and call the device open routine if any.
 */
copen(mode, arg, fname)
	register int mode;
	int arg;
	caddr_t fname;
{
	register struct gnode *gp;
	register struct file *fp;
	register struct nameidata *ndp = &u.u_nd;
	register int i;
	caddr_t value;
	int ret;
	
	if ((mode&(FREAD|FWRITE)) == 0) {
		u.u_error = EINVAL;
		return;
	}

	KM_ALLOC(ndp->ni_dirp, char *, MAXPATHLEN, KM_NAMEI, KM_NOARG);
	if (ndp->ni_dirp == NULL) {
		u.u_error = EIO;
		return;
	}
 	if (u.u_error = copyinstr(fname, ndp->ni_dirp, MAXPATHLEN,
				  (u_int *)0)) {
	        goto out;
	}

	if (mode&FCREAT) {
		if (mode & FEXCL)
			ndp->ni_nameiop = CREATE;
		else
			ndp->ni_nameiop = CREATE | FOLLOW;
		gp = gfs_namei(ndp);
		if (gp == NULL) {
			if (u.u_error) {
			        goto out;
			}
			/*
			 * Don't let a non super user create a regular file
			 * with the sticky bit set.
			 */
			if (u.u_uid)
				arg &= ~GSVTX;
			if ((gp = GMAKNODE((arg & 07777) | GFREG, (dev_t) 0,
					   ndp))
			== (struct gnode *) GNOFUNC) {
				u.u_error = EOPNOTSUPP;
			        goto out;
			}
			if (gp == NULL) {
			        goto out;
			}
			mode &= ~FTRUNC;
		} else {
			if (mode&FEXCL) {
				u.u_error = EEXIST;
				gput(gp);
			        goto out;
			}
			mode &= ~FCREAT;
		}
	} else {
		ndp->ni_nameiop = LOOKUP | FOLLOW;
		gp = gfs_namei(ndp);
		if (gp == NULL) {
		        goto out;
		}
		
	}
	if ((gp->g_mode & GFMT) == GFSOCK) {
		u.u_error = EOPNOTSUPP;
		goto free_gp;
	}
	if ((mode&FCREAT) == 0) {
		if (mode&FREAD)
			if (access(gp, GREAD))
				goto free_gp;
		if (mode&(FWRITE|FTRUNC)) {
			if (access(gp, GWRITE))
				goto free_gp;
			if ((gp->g_mode&GFMT) == GFDIR) {
				u.u_error = EISDIR;
				goto free_gp;
			}
		}
	}
waitinuse:
	while ((mode & FBLKINUSE) && (gp->g_flag & GINUSE))  { /*002*/
		if (mode & FNDELAY) {
			u.u_error = EWOULDBLOCK;
			goto free_gp;
		}

		sleep_unlock((caddr_t)&gp->g_flag, PLOCK, &gp->g_lk);
		gfs_lock(gp);
		
	}

	if ((gp->g_mode & GFMT) == GFPORT)
		mode &= ~FTRUNC;

	if (mode&FTRUNC) {
		if (GTRUNC(gp, (u_long)0, u.u_cred) == GNOFUNC)
			u.u_error = EOPNOTSUPP;
		if (u.u_error)
			goto free_gp;
	}
	/*
	 * This should be done first to verify this resource exists.
	 */
	fp = falloc();
	if (fp == NULL)
		goto free_gp;
		
	gfs_unlock(gp);
	fp->f_flag = mode&FMASK;
	if ((gp->g_mode & GFMT) == GFPORT) {
		/*
		 * For named-pipes, the FNDELAY flag must propagate to
		 * the rdwr layer.  Also, FAPPEND must always be set so
		 * that fp->f_offset is correctly maintained.
		 */
		fp->f_offset = 0;
		fp->f_flag |= FAPPEND | (mode & FNDELAY);
		fp->f_type = DTYPE_PORT;
	}
	else
		fp->f_type = DTYPE_INODE;
	fp->f_ops = &gnodeops;
	fp->f_data = (caddr_t)gp;
	i = u.u_r.r_val1;
	if (setjmp(&u.u_qsave)) {
		if (u.u_error == 0)
			u.u_error = EINTR;
		goto free_fp;
	}

	u.u_error = GOPEN(gp, mode);

	if (u.u_error == 0) {
		if ((mode&FBLKANDSET)==FBLKANDSET) { /*002*/
			gp->g_flag |= GINUSE;
			U_POFILE_SET(i, U_POFILE(i) | UF_INUSE);
			(*fp->f_ops->fo_ioctl)(fp, FIOSINUSE, value, u.u_cred);
			u.u_r.r_val1 = i;
		}
		goto out;
	}
	/*
	 * The call to the sfs open routine failed or was interrupted. Clean
	 * up the u-area and deallocated the file table entry so it can be
	 * used by someone else.
	 */
free_fp:
	U_OFILE_SET(i,NULL);
	U_POFILE_SET(i,0);
	crfree(fp->f_cred);
	smp_lock(&lk_file, LK_RETRY);
	fp->f_data = (caddr_t)0;
	fp->f_count = 0;
	smp_unlock(&lk_file);
	fp = NULL;
	if (u.u_error==EALREADY && (gp->g_flag&GINUSE)) { 
				   /*  gnode was grabbed while we were */
		u.u_error = 0;	   /*  blocked.  wait to free up again.*/
		gfs_lock(gp);		/* need to lock again */
		goto waitinuse;	    
	}
free_gp:
	if (!smp_owner(&gp->g_lk))
		grele(gp);
	else
		gput(gp);
out:
	KM_FREE(ndp->ni_dirp, KM_NAMEI);
}


/*
 * Mknod system call
 */
mknod()
{
	register struct gnode *gp;
	register struct a {
		char	*fname;
		int	fmode;
		int	dev;
	} *uap;
	register struct nameidata *ndp = &u.u_nd;
	register int ret;
	
	uap = (struct a *)u.u_ap;
	if (!suser() && ((uap->fmode & GFMT) != GFPORT))
		return;
			/* if a non-privileged user is making	*/
			/* a port node then negate the error	*/
			/* posted by SUSER.			*/
	u.u_error = 0;
	ndp->ni_nameiop = CREATE;

	KM_ALLOC(ndp->ni_dirp, char *, MAXPATHLEN, KM_NAMEI, KM_NOARG);
	if (ndp->ni_dirp == NULL) {
		u.u_error = EIO;
		return;
	}
 	if (u.u_error = copyinstr(uap->fname, ndp->ni_dirp, MAXPATHLEN,
				  (u_int *) 0)) {
	        goto out2;
	}

	gp = gfs_namei(ndp);
	if (gp != NULL) {
		u.u_error = EEXIST;
		goto out;
	}
	if (u.u_error) {
	        goto out2;
	}
	
	if ((gp = GMAKNODE(uap->fmode, uap->dev, ndp)) == (struct gnode *) GNOFUNC) {
		u.u_error = EOPNOTSUPP;
	        goto out2;
	}
	if (gp == NULL) {
	        goto out2;
	}
	
out:
	gput(gp);
out2:
	KM_FREE(ndp->ni_dirp, KM_NAMEI);
}


/*
 * Synch an open file.
 */
fsync()
{
	register struct a {
		int	fd;
	} *uap = (struct a *)u.u_ap;
	register struct gnode *gp;
	register struct file *fp;
	
	fp = getgnode(uap->fd);
	if (fp == NULL)
		return;
	if ((gp = (struct gnode *)fp->f_data) == 0) {
		u.u_error = EBADF;
		return;
	}

	gfs_lock(gp);
	if (GSYNCG(gp, fp->f_cred) == GNOFUNC)
		u.u_error = EOPNOTSUPP;
	gfs_unlock(gp);
}



/*
 * Change mode of a file given path name.
 */
chmod()
{
	register struct gnode *gp;
	register struct a {
		char	*fname;
		int	fmode;
	} *uap = (struct a *)u.u_ap;

	/* where should owner be? */
	
	if ((gp = owner(uap->fname, FOLLOW)) == NULL)
		return;
	u.u_error = chmod1(gp, uap->fmode, u.u_cred);
	gput(gp);
}

/*
 * Change mode of a file given a file descriptor.
 */
fchmod()
{
	register struct a {
		int	fd;
		int	fmode;
	} *uap;
	register struct gnode *gp;
	register struct file *fp;
	
	uap = (struct a *)u.u_ap;
	fp = getgnode(uap->fd);
	if (fp == NULL)
		return;
	if ((gp = (struct gnode *)fp->f_data) == 0) {
		u.u_error = EBADF;
		return;
	}
	if (u.u_uid != gp->g_uid && !suser())
		return;

	gfs_lock(gp);
	u.u_error = chmod1(gp, uap->fmode, fp->f_cred);
	gfs_unlock(gp);
}

/*
 * Change the mode on a file.
 * Gnode must be locked before calling.
 */
chmod1(gp, mode, cred)
	register struct gnode *gp;
	register int mode;
	struct	ucred	*cred;
{
        register int sticky = gp->g_mode & GSVTX;
	register int ret;

	if (ISREADONLY(gp->g_mp))
		return(EROFS);
		
	if (u.u_uid) {
		if ((gp->g_mode & GFMT) != GFDIR)
			mode &= ~GSVTX;
		if (!groupmember(gp->g_gid))
			mode &= ~GSGID;
	}

        /* for the new text table sticky changes blow away unref-ed text */
        if ((gp->g_flag&GTEXT) && (mode&GSVTX) && sticky == 0) {
                xrele(gp);
	}
        gp->g_mode &= ~07777;
	gp->g_mode |= mode&07777;
	gp->g_flag |= (GCHG | GCMODE);
	if (!ISLOCAL(gp->g_mp)) {
		if (ret = GUPDATE(gp, 0, 0, 0, cred)) {
			return(ret);
		}
	}
	if (gp->g_flag&GTEXT && (gp->g_mode&GSVTX) == 0 && sticky)
		xrele(gp);
	return(0);
}

/*
 * Set ownership given a path name.
 */
chown()
{
	register struct gnode *gp;
	register struct a {
		char	*fname;
		int	uid;
		int	gid;
	} *uap;

	uap = (struct a *)u.u_ap;
	/*
	 * owner() is not going to perform additional access checks
	 * over chown1(). Leave it for now, but this should change
	 * eventually.
	 */
	if ((gp = owner(uap->fname, NOFOLLOW)) == NULL)
		return;
	u.u_error = chown1(gp, uap->uid, uap->gid, u.u_cred);
	gput(gp);
}

/*
 * Set ownership given a file descriptor.
 */
fchown()
{
	register struct a {
		int	fd;
		int	uid;
		int	gid;
	} *uap;
	register struct gnode *gp;
	register struct file *fp;
	
	uap = (struct a *)u.u_ap;
	fp = getgnode(uap->fd);
	if (fp == NULL)
		return;
	if ((gp = (struct gnode *)fp->f_data) == 0) {
		u.u_error = EBADF;
		return;
	}
	if (!suser())
		return;

	gfs_lock(gp);
	u.u_error = chown1(gp, uap->uid, uap->gid, fp->f_cred);
	gfs_unlock(gp);
}

/*
 * Perform chown operation on gnode gp;
 * gnode must be locked prior to call.
 */
chown1(gp, uid, gid, cred)
	register struct gnode *gp;
	struct	ucred	*cred;
	register int uid, gid;
{
	register int ret = 0;
#ifdef QUOTA
	register long change;
#endif

	if (ISREADONLY(gp->g_mp))
		return(EROFS);
	if (uid == -1)
		uid = gp->g_uid;
	if (gid == -1)
		gid = gp->g_gid;
	if (uid != gp->g_uid && !suser())
		return(u.u_error);
	if (gid != gp->g_gid && !groupmember(gid) && !suser())
		return(u.u_error);
#ifdef QUOTA
	if (gp->g_uid == uid)		/* this just speeds things a little */
		change = 0;
	else
		change = gp->g_blocks;
	(void) chkdq(gp, -change, 1);
	(void) chkiq(gp->g_dev, gp, gp->g_uid, 1);
	dquot_lock(gp->g_dquot);
	dqrele(gp->g_dquot);
	dquot_unlock(gp->g_dquot);
#endif
	gp->g_uid = uid;
	gp->g_gid = gid;
	gp->g_flag |= (GCHG | GCID);
		
	if (u.u_ruid != 0)
		gp->g_mode &= ~(GSUID|GSGID);
	
	if (!ISLOCAL(gp->g_mp)) {
		if (ret = GUPDATE(gp, 0, 0, 0, cred)) {
			return(ret);
		}
	}
		
#ifdef QUOTA
	gp->g_dquot = inoquota(gp);
	(void) chkdq(gp, change, 1);
	(void) chkiq(gp->g_dev, (struct gnode *)NULL, uid, 1);
	return (u.u_error);	
#else
	return (ret);
#endif
}

utimes()
{
	register struct a {
		char	*fname;
		struct	timeval *tptr;
	} *uap = (struct a *)u.u_ap;
	register struct gnode *gp;
	struct timeval tv[2];
 	register struct nameidata *ndp = &u.u_nd;
	register int ret;
	
 	ndp->ni_nameiop = LOOKUP | FOLLOW;
	KM_ALLOC(ndp->ni_dirp, char *, MAXPATHLEN, KM_NAMEI, KM_NOARG);
	if (ndp->ni_dirp == NULL) {
		u.u_error = EIO;
		return;
	}
 	if (u.u_error = copyinstr(uap->fname, ndp->ni_dirp, MAXPATHLEN,
				  (u_int *)0))
	{
		KM_FREE(ndp->ni_dirp, KM_NAMEI);
		return;
	}

 	gp = GNAMEI(ndp);

	KM_FREE(ndp->ni_dirp, KM_NAMEI);
	if (gp == NULL) {
		return;
	}

	if (ISREADONLY(gp->g_mp)) {
		u.u_error = EROFS;
		gput(gp);
		return;
	}
	ret = GGETVAL(gp);
	if (uap->tptr != NULL) {
		u.u_error = copyin((caddr_t)uap->tptr, (caddr_t)tv,
				   sizeof (tv));
		}
	else {
		tv[0].tv_sec = tv[1].tv_sec = timepick->tv_sec;
		tv[0].tv_usec = tv[1].tv_usec = timepick->tv_usec;
	}
	/* If not the owner or su and tptr is NULL, check for write access. */
	/* If tptr is not NULL then EPERM error condition.		    */
	if ((u.u_error == 0) && (u.u_uid != gp->g_uid) && u.u_uid != 0) {
		if (uap->tptr != NULL) 
			u.u_error = EPERM;
		else
			access(gp,GWRITE);

	}	
	if (u.u_error == 0) {
		if (tv[0].tv_sec < 0 || tv[0].tv_usec < 0 ||
		    tv[1].tv_sec < 0 || tv[1].tv_usec < 0 ||
		    tv[0].tv_usec >= 1000000 || tv[1].tv_usec >= 1000000)
			u.u_error = EINVAL;
		else {
			gp->g_flag |= GMOD|GACC|GUPD|GCHG;
			if (GUPDATE(gp, &tv[0], &tv[1], 0, u.u_cred)
			    == GNOFUNC)
				u.u_error = EOPNOTSUPP;
		}
	}			

	gput(gp);
}

/*
 * Flush any pending I/O.
 */
sync()
{
	update();
	gfs_gupdat(NODEV);
}

/*
 * Truncate a file given its path name.
 */
truncate()
{
	register struct a {
		char	*fname;
		u_long	length;
	} *uap = (struct a *)u.u_ap;
	register struct gnode *gp;
	register struct nameidata *ndp = &u.u_nd;

	ndp->ni_nameiop = LOOKUP | FOLLOW;

	KM_ALLOC(ndp->ni_dirp, char *, MAXPATHLEN, KM_NAMEI, KM_NOARG);
	if (ndp->ni_dirp == NULL) {
		u.u_error = EIO;
		return;
	}
 	if (u.u_error = copyinstr(uap->fname, ndp->ni_dirp, MAXPATHLEN,
				  (u_int *) 0)) {
	        goto bad2;
	}

	gp = gfs_namei(ndp);
	
	if (gp == NULL)
	        goto bad2;
	if (access(gp, GWRITE))
		goto bad;
	if ((gp->g_mode&GFMT) == GFDIR) {
		u.u_error = EISDIR;
		goto bad;
	}
	if (GTRUNC(gp, uap->length, u.u_cred) == GNOFUNC)
		u.u_error = EOPNOTSUPP;
bad:
	gput(gp);
bad2:
	KM_FREE(ndp->ni_dirp, KM_NAMEI);
}

/*
 * Truncate a file given a file descriptor.
 */
ftruncate()
{
	register struct a {
		int	fd;
		u_long	length;
	} *uap = (struct a *)u.u_ap;
	register struct gnode *gp;
	register struct file *fp;

	fp = getgnode(uap->fd);
	if (fp == NULL)
		return;
	if ((fp->f_flag&FWRITE) == 0) {
		u.u_error = EINVAL;
		return;
	}

	if ((gp = (struct gnode *)fp->f_data) == 0) {
		u.u_error = EBADF;
		return;
	}
	if (ISREADONLY(gp->g_mp)) {
		u.u_error = EROFS;
		return;
	}
	
	gfs_lock(gp);
	if (GTRUNC(gp, uap->length, fp->f_cred) == GNOFUNC)
		u.u_error = EOPNOTSUPP;
	gfs_unlock(gp);
	return;
}


/*
 * symlink -- make a symbolic link
 */
symlink()
{
	register struct a {
		char	*target;
		char	*linkname;
	} *uap;
	register struct gnode *gp;
	register struct nameidata *ndp = &u.u_nd;
	register char *target_cp;
	register char *source_cp;
	
	uap = (struct a *)u.u_ap;
	
	/* get us the target name */
	
	KM_ALLOC(target_cp, char *, MAXPATHLEN, KM_NAMEI, KM_NOARG);
	if (target_cp == NULL) {
		u.u_error = EIO;
		return;
	}

	if (u.u_error = copyinstr(uap->target, target_cp, MAXPATHLEN,
				  (u_int *) 0)) {
	        goto out1;
	}
	
	/* get us the source name */
	
	KM_ALLOC(source_cp, char *, MAXPATHLEN, KM_NAMEI, KM_NOARG);
	if (source_cp == NULL) {
		u.u_error = EIO;
	        goto out1;
	}

	if (u.u_error = copyinstr(uap->linkname, source_cp, MAXPATHLEN,
				  (u_int *) 0)) {
	        goto out2;
	}
	
	/* we may not create a symlink when the source exists */
	
	ndp->ni_nameiop = CREATE;
	ndp->ni_dirp = source_cp;
	gp = gfs_namei(ndp);
	if (gp) {
		gput(gp);
		u.u_error = EEXIST;
	        goto out2;
	}
	if (u.u_error) {
	        goto out2;
	}

	/* create the special file type for a symlink */
	if (GSYMLINK(ndp, target_cp) == GNOFUNC)
		u.u_error = EOPNOTSUPP;
out2:
	KM_FREE(source_cp, KM_NAMEI);
out1:
	KM_FREE(target_cp, KM_NAMEI);
}

/*
 * Seek system call
 */
lseek()
{
	register struct file *fp;
	register struct a {
		int	fd;
		off_t	off;
		int	sbase;
	} *uap;
	register struct gnode *gp;
	register long where;
	register int ret = 0;
	
	uap = (struct a *)u.u_ap;
	GETF(fp, uap->fd);
	if (fp->f_type != DTYPE_INODE) {
		u.u_error = ESPIPE;
		return;
	}
	if (fp->f_data == (caddr_t)0) {
		u.u_error = EBADF;
		return;
	}
	
	/* should seeks just arbitrarily be done?, should we check
	 * with the correct sfs to see if the operation can be done?
	 */
	
	gp = (struct gnode *)fp->f_data;

	switch (uap->sbase) {

	case L_INCR:
		where = fp->f_offset + uap->off;
		break;

	case L_XTND:
		where = uap->off + ((struct gnode *)fp->f_data)->g_size;
		break;

	case L_SET:
		where = uap->off;
		break;

	default:
		u.u_error = EINVAL;
		return;
	}

	/*
	 * if gnode type is a regular file, and resulting offset
	 * would be negative, return an error.
	 */

	if ((where < 0) && ((gp->g_mode & GFMT) == GFREG)) {
 		u.u_error = EINVAL;
 		return;
 	}

	/*
	 * Call sfs's seek routine
	 */

	ret = GSEEK(gp, where);
	
	/* 
	 * there needs to be a documentation change here, if
   	 * the seek is not successful (which is not possible for
	 * ufs), fp->offset does not change
	 */

	if (u.u_error) {
		u.u_error = EINVAL;
		where = -1;
	 } else {
		if (ret) {
			u.u_error = EOPNOTSUPP;
			return;
		}
		smp_lock(&fp->f_lk, LK_RETRY);
		fp->f_offset = where;
		smp_unlock(&fp->f_lk);
	}
	u.u_r.r_off = where;
}

/*
 * Access system call
 */
saccess()
{
	register int svuid, svgid;
	register struct gnode *gp;
	register struct a {
		char	*fname;
		int	fmode;
	} *uap;
	register struct nameidata *ndp = &u.u_nd;
	register long	mode_mask = ~(X_OK | R_OK | W_OK);

	uap = (struct a *)u.u_ap;

	if (uap->fmode & mode_mask) {
		u.u_error = EINVAL;
		return;
	}

	KM_ALLOC(ndp->ni_dirp, char *, MAXPATHLEN, KM_NAMEI, KM_NOARG);
	if (ndp->ni_dirp == NULL) {
		u.u_error = EIO;
		return;
	}

 	if (u.u_error = copyinstr(uap->fname, ndp->ni_dirp, MAXPATHLEN,
				  (u_int *) 0)) {
		KM_FREE(ndp->ni_dirp, KM_NAMEI);
	        return;
	}

	svuid = u.u_uid;
	svgid = u.u_gid;
	u.u_uid = u.u_ruid;
	u.u_gid = u.u_rgid;

	ndp->ni_nameiop = LOOKUP | FOLLOW;
	gp = gfs_namei(ndp);
	if (gp != NULL) {
		
		/*
		 * make checks for M_NODEV, M_NOSUID, and M_NOEXEC
		 * flags 
		 */
		
		if ((uap->fmode&R_OK) && access(gp, GREAD))
			goto done;
		if ((uap->fmode&W_OK) && access(gp, GWRITE))
			goto done;

		/*
		 * convoluted as it seems, the following test
		 * needs to be checked:
		 *	if a regular file is being checked for exec
		 *		and the filesystem is NOSUID
		 *		and the file is SUID or SGID
		 *		and we are not the super user
		 *	or if a regular file is being checked for exec
		 *		and the filesystem is NOEXEC
		 *		and we are not the super user
		 * we may not permit access to the file
		 */
		if (uap->fmode&X_OK) {
			if ((gp->g_mp->m_flags & M_NOSUID) &&
			    (gp->g_mode & (GSUID | GSGID)) &&
			    (gp->g_mode & GFREG) &&
			    u.u_uid) {
				u.u_error = EROFS;
				goto done;
			}
			if ((gp->g_mp->m_flags & M_NOEXEC) &&
			    (gp->g_mode & GEXEC) &&
			    (gp->g_mode & GFREG) &&
			    u.u_uid) {
				u.u_error = EROFS;
				goto done;
			}
			if (access(gp, GEXEC))
				goto done;
		}
done:
		gput(gp);
	}
	u.u_uid = svuid;
	u.u_gid = svgid;
	KM_FREE(ndp->ni_dirp, KM_NAMEI);
}


/*
 * Stat system call.  This version follows links.
 */

stat()
{

	stat1(FOLLOW);
}


/*
 * Lstat system call.  This version does not follow links.
 */

lstat()
{

	stat1(NOFOLLOW);
}


stat1(follow)
	register int follow;
{
	register struct gnode *gp;
	register struct a {
		char	*fname;
		struct stat *ub;
	} *uap;
	struct stat sb;
	register struct nameidata *ndp = &u.u_nd;

	uap = (struct a *)u.u_ap;
	ndp->ni_nameiop = LOOKUP | follow;

	KM_ALLOC(ndp->ni_dirp, char *, MAXPATHLEN, KM_NAMEI, KM_NOARG);
	if (ndp->ni_dirp == NULL) {
		u.u_error = EIO;
		return;
	}
 	if (u.u_error = copyinstr(uap->fname, ndp->ni_dirp, MAXPATHLEN,
				 (u_int *) 0)) {
	        goto out;
	}

	gp = gfs_namei(ndp);

	if (gp == NULL)
	        goto out;
	gfs_unlock(gp);
	(void) GSTAT(gp, &sb);
	grele(gp);
	if (!u.u_error)
	 	u.u_error = copyout((caddr_t)&sb, (caddr_t)uap->ub, sizeof (sb));
out:
	KM_FREE(ndp->ni_dirp, KM_NAMEI);
}


/*
 * Return target name of a symbolic link
 */

readlink()
{
	register struct gnode *gp;
	register struct a {
		char	*name;
		char	*buf;
		int	count;
	} *uap = (struct a *)u.u_ap;
	register struct nameidata *ndp = &u.u_nd;
	struct uio _auio;
	register struct uio *auio = &_auio;
	struct iovec _aiov;
	register struct iovec *aiov = &_aiov;
	
	ndp->ni_nameiop = LOOKUP;

	KM_ALLOC(ndp->ni_dirp, char *, MAXPATHLEN, KM_NAMEI, KM_NOARG);
	if (ndp->ni_dirp == NULL) {
		u.u_error = EIO;
		return;
	}
 	if (u.u_error = copyinstr(uap->name, ndp->ni_dirp, MAXPATHLEN,
				  (u_int *)0)) {
	        goto out1;
	}

	gp = gfs_namei(ndp);
	
	if (gp == NULL)
	        goto out1;

	/* if the sfs doesn't allow links everything is cool */
		
	if ((gp->g_mode&GFMT) != GFLNK) {
		u.u_error = EINVAL;
		goto out;
	}
	auio->uio_iov = aiov;
	auio->uio_iovcnt = 1;
	aiov->iov_base = uap->buf;
	aiov->iov_len = auio->uio_resid = uap->count;
	auio->uio_segflg = auio->uio_offset = 0;
	if (GREADLINK(gp, auio) == GNOFUNC)
		u.u_error = EOPNOTSUPP;
out:
	gput(gp);
	if (u.u_error == NULL)
		u.u_r.r_val1 = uap->count - auio->uio_resid;
out1:
	KM_FREE(ndp->ni_dirp, KM_NAMEI);
}

struct file *
getgnode(fdes)
	register int fdes;
{
	register struct file *fp;

	if ((unsigned)fdes > u.u_omax || (fp = U_OFILE(fdes)) == NULL) {
		u.u_error = EBADF;
		return ((struct file *)0);
	}
	if (fp->f_type != DTYPE_INODE && fp->f_type != DTYPE_PORT) {
		u.u_error = EINVAL;
		return ((struct file *)0);
	}
	return (fp);
}

getmnt()
{
	register struct a {
		u_int	*cookie;
		struct 	fs_data	*buf;
		u_int	nbytes;
		int	mode;
		char	*path;
	} *uap = (struct a *) u.u_ap;

	register u_int number;
	register struct mount *mp;
	int cookie, i;	
	register struct fs_data *fs_data;
	register int count;
	register struct gnode *gp;
	register struct nameidata *ndp = &u.u_nd;
	register struct gnode *rgp;

	switch(uap->mode) {

	case STAT_ONE:
	case NOSTAT_ONE:

	        KM_ALLOC(ndp->ni_dirp, char *, MAXPATHLEN, KM_NAMEI, KM_NOARG);
		if (ndp->ni_dirp == NULL) {
			u.u_error = EIO;
			return;
		}

		if (u.u_error = copyinstr(uap->path, ndp->ni_dirp, MAXPATHLEN,
					 (u_int *) 0)) {
		        goto out;
		}

		ndp->ni_nameiop = LOOKUP | FOLLOW;
		gp = gfs_namei(ndp);
		if (gp == NULL)
		        goto out;
		gfs_unlock(gp);  

		mp = gp->g_mp;

		if (uap->mode == STAT_ONE) {
			gref(mp->m_rootgp);
			fs_data = GGETFSDATA(mp); 
			grele(mp->m_rootgp);
		} else
			fs_data = mp->m_fs_data;

		if (! u.u_error)
			u.u_error = copyout((caddr_t) fs_data,
			(caddr_t) uap->buf, sizeof(struct fs_data)); 
		u.u_r.r_val1 = 1;
		grele(gp);
out:
		KM_FREE(ndp->ni_dirp, KM_NAMEI);
		break;

	case STAT_MANY:
	case NOSTAT_MANY: 

		/* insure we can get all the stuff out. */
		if ((count = number = 
		    uap->nbytes / sizeof(struct fs_data)) < 1) {
			u.u_error = EINVAL;
	                break;
		}

		if (u.u_error = copyin(uap->cookie, &cookie, 
				       sizeof(cookie)))
	                break;
			
		if (cookie < 0 || cookie > NMOUNT) {
			u.u_error = EINVAL;
	                break;
		}

		i = cookie;             /* save in case none found */
		for (mp = &mount[cookie]; mp < &mount[NMOUNT] && number; 
		    mp++, cookie++) {
			/*
			 * Check if mp is mounted on and if so get a
			 * ref on its file system.
			 */
			if (rgp = fref(mp, (dev_t)0)) {
				number--;
				if (uap->mode == STAT_MANY) {
					gref(mp->m_rootgp);
					fs_data = GGETFSDATA(mp);
					grele(mp->m_rootgp);
					if (u.u_error == EINTR)  {
						grele(rgp);
						break;
					}
				} else
					fs_data = mp->m_fs_data;
				if (!u.u_error)
					if (u.u_error = copyout((caddr_t)fs_data,
								(caddr_t) uap->buf, 
								sizeof(struct fs_data))) {
						grele(rgp);
						break;
					}
				/*
				 * Release ref on file system.
				 */
				grele(rgp);
				/* save slot # of last one found */
				/* we wouldn't have to do this, if this was */
				/* a linked list */
				i = cookie;
				uap->buf++;
			}
		}
		( ((u.u_r.r_val1 = count - number) > 0) ? (cookie = ++i) : 
		(cookie = i) );
		copyout(&cookie, uap->cookie, sizeof(cookie));
		break;
	
	default:
		u.u_error = EINVAL;
		break;
	}
}


getdirentries()
{
	register struct a {
		int fd;
		struct gen_dir *buf;
		u_int nbytes;
		u_int *cookie;
	} *uap = (struct a *) u.u_ap;
	register struct file *fp;
	register struct gnode *gp;
	struct uio _auio;
	register struct uio *auio = &_auio;
	struct iovec _aiov;
	register struct iovec *aiov = &_aiov;
	register int ret;
	
	if ((fp = getgnode(uap->fd)) == NULL) {   /* bad file descriptor */
		/* 
		 * getgnode returns EINVAL if not an INODE or PORT
		 * we just interpret that to mean it isn't a directory
		 * either
		 */ 

		if (u.u_error == EINVAL)
			u.u_error = ENOTDIR;
		return;
	}

	if (fp->f_type != DTYPE_INODE) {	/* this must be a gnode */
		u.u_error = EOPNOTSUPP;	
		return;
	}
	
	gp = (struct gnode *) fp->f_data;
	if ((gp->g_mode & GFMT) != GFDIR) {	/* this must be a directory */
		u.u_error = ENOTDIR;
		return;
	}
	
	/* check for valid buffer */
	if (uap->nbytes < DIRBLKSIZ || (uap->nbytes & (DIRBLKSIZ-1))
		|| !useracc(uap->buf,uap->nbytes,B_WRITE)) {
		u.u_error = EINVAL;
		return;
	}

	aiov->iov_base = (caddr_t) uap->buf;
	aiov->iov_len = uap->nbytes;
	auio->uio_iov = aiov;
	auio->uio_iovcnt = 1;
	auio->uio_segflg = UIO_USERSPACE;
	auio->uio_offset = fp->f_offset;
	auio->uio_resid = uap->nbytes;

	ret = GGETDIRENTS(gp, auio, fp->f_cred);

	if (u.u_error)
		return;

	/* POSIX says update access time */
        if (u.u_procp->p_progenv == A_POSIX)
                gp->g_flag |= GACC;
	u.u_error = copyout((caddr_t)&fp->f_offset,
				(caddr_t)uap->cookie,sizeof(long));
	u.u_r.r_val1 = uap->nbytes - auio->uio_resid;
	smp_lock(&fp->f_lk, LK_RETRY);
	fp->f_offset = auio->uio_offset;	/* for lseek and next read */
	smp_unlock(&fp->f_lk);
}
