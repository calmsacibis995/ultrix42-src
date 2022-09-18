#ifndef lint
static	char	*sccsid = "@(#)gfs_descrip.c	4.9	(ULTRIX)	5/2/91";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1988 by				*
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
 *
 *			Modification History
 *
 *	prs 4/2/91
 *	The F_GETLK request to fcntl() should always return the blocking
 * 	lock description relative to the beginning of the file if process
 *	is running in POSIX mode.
 *
 *	prs 2/28/91
 *	Added support for a configurable number of open
 *	file descriptors.
 *
 *	prs 1/21/91
 *	f_data is now controlled with lk_file instead of per file
 *	table entry lock. This is for vhangup().
 *
 *	scott 03/29/90
 *      add check on fp->f_data before fetching mode and number
 *
 *	prs 11/14/89
 *	Fixed EBADF detection within locking code of fcntl().
 *
 *	scott 10/16/89
 *      fix use of gnode dev # for audit
 *
 *	scott 8/24/89
 *	moved closef() in close().
 *
 *	condylis 6/23/89
 *	Removed references to unp_gc().
 *
 *	prs 06/14/89
 *	Locked gnode during call to sfs close routine in closef().
 *
 *	Scott 06/09/89
 *	Add audit support
 *
 *      McMenemy 05/04/89
 *      Add internal_dup2 call.
 *
 *	prs 03/21/89
 *	Added lockinit of LK_ACCT in finit()
 *
 *	Condylis 09/12/88
 *	Made SMP changes in flock()
 *
 *	Fred Glover, 8/29/88
 *	add POSIX check for fcntl locking with F_GETFL;
 *	change error code returned for file which does not support locking;
 *	fix return from rewhence function for unimplemented GGETVAL function.
 *
 * 	Paul Shaughnessy, 8/22/88
 *	Cleaned up F_SETFL fcntl() requests.
 *
 *	prs for chet	, 8/15/88
 *	Don't call the close routine pointed to in the file ops
 *	structure if f_data is NULL in closef().
 *
 *	prs - 7/12/88
 *	Added SMP file table entry locks around calls to dupit.
 *
 *	Paul Shaughnessy, 7/11/88
 *	Added fifo support to fsetown() and fgetown().
 *
 *	Fred Glover	6/10/88
 *	Modify error return mapping from Sys-V locking (via fcntl).
 *	Currently, EWOULDBLOCK and EDEADLK are the same errno.  Thus
 *	modify check in fcntl under locking commands: it is unnecessary 
 *	to map EWOULDBLOCK into a Sys-V errno, and in fact an error to 
 *	do so, since that would also remap EDEADLK.
 *
 *	Paul Shaughnessy, 4/27/88
 *	Changed closef to handle closing a file pointer that was
 *	"divorced" from its gnode via vhangup().
 *
 *	Paul Shaughnessy, 2/10/88
 *	Modified to handle the new fifo code.
 *
 *	Fred Glover	1/26/88
 *	Modify fcntl lock routines to support kernel and daemon based
 *	Sys-V style locking through the modified GRLOCK macro.
 *
 *	Joe Amato, 8/28/87
 *	closef() returns err from the file systems.
 *	close() sets u.u_error from closef().
 *
 *	Larry Palmer 	8/25/87
 *	Made socket fsetown/fgetown set the pgrp as a positive
 *	value. This mathces how the socket opt does it.
 *
 * 	Paul Shaughnessy, 10/02/86
 * 005- Added code to clear the close_on_exec bit when the F_DUPFD
 *	command is specified with the fcntl system call.
 *
 *	Paul Shaughnessy, 12/23/85
 * 004- Added commands to fcntl system calls to turn on/off
 * 	the syncronous write option to a file.
 *
 * 11 Nov 85 -- depp
 *	Removed all conditional compiles for System V IPC.
 *
 *	Stephen Reilly, 9/09/85
 * 002-	Modified to handle the new lockf code.				
 *
 *	Stephen Reilly,	2/19/85
 * 001- Process group or process ID was not stored correctly into the
 *	socket structure.
 *
 * 	Larry Cohen, 4/4/85
 * 002- Changes for block in use capability.
 *	Also changed calls to getf to GETF macro
 *
 * 15 Mar 85 -- funding
 *	Added named pipe support (re. System V named pipes)
 *
 * 	Larry Cohen, 4/13/85
 *	call ioctl FIOCINUSE when closing inuse desriptor
 *
 * 05-May-85 - Larry Cohen
 *	keep track of the highest number file descriptor opened
 *
 * I moved a line around as part of the inode count going negative
 * Rich
 *
 *	kern_descrip.c	6.2	83/09/25
 *
 ***********************************************************************/

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/kernel.h"
#include "../h/gnode.h"
#include "../h/proc.h"
#include "../h/conf.h"
#include "../h/file.h"
#include "../h/socket.h"
#include "../h/socketvar.h"
#include "../h/mount.h"
#include "../h/stat.h"
#include "../h/ioctl.h"
#include "../h/flock.h"
#include "../h/exec.h"
#include "../h/kmalloc.h"

#ifdef KLMDEBUG
extern int klm_debug;
#endif KLMDEBUG

/*
 * Descriptor management.
 */

/*
 * TODO:
 *	eliminate u.u_error side effects
 */

/*
 * Private routine forward declarations.
 */
int      alloc_fd_slots();
extern int max_nofile;

/*
 * Ensure that descriptor slot n has been allocated, returning -1 if
 * unable to do so and 0 otherwise.  Expressed as a macro to make it
 * cheap enough to call within loops iterating over all descriptors.
 * The predicate part arranges that the function to allocate descriptor
 * slots in the range [NOFILE_IN_U .. max_nofile] is called only when
 * necessary.
 */
#define       enable_fd_slot(n) \
        (((n) < (NOFILE_IN_U + u.u_of_count)) ? \
                0 : alloc_fd_slots((n)))

struct lock_t lk_file;

/*
 * System calls on descriptors.
 */
getdtablesize()
{

	u.u_r.r_val1 = max_nofile;
}

getdopt()
{

}

setdopt()
{

}

dup()
{
	register struct a {
		int	i;
	} *uap = (struct a *) u.u_ap;
	register struct file *fp;
	register int j;

	if (uap->i &~ 077) { uap->i &= 077; dup2(); return; }	/* XXX */

	GETF(fp, uap->i);
	j = ufalloc(0);
	if (j < 0)
		return;
	smp_lock(&lk_file, LK_RETRY);
	dupit(j, fp, U_POFILE(uap->i));
	smp_unlock(&lk_file);
}

dup2()
{
	register struct a {
		int	i, j;
	} *uap = (struct a *) u.u_ap;

	internal_dup2(uap->i, uap->j);
}

internal_dup2(old, new)
        int old;
        int new;
{
	register struct file *fp;
	register struct file *fp2;
	register struct gnode *gp;

	GETF(fp, old);
	if (new < 0 || new >= max_nofile) {
		u.u_error = EBADF;
		return;
	}
	u.u_r.r_val1 = new;
	if (old == new)
		return;

	/* audit info */
	if ( audswitch ) {
		if ( gp = (struct gnode *)fp->f_data ) {
			u.u_gno_dev[0] = (gp->g_mode & GFMT) == GFCHR ? gp->g_rdev : gp->g_dev;
			u.u_gno_num[0] = gp->g_number;
			u.u_gno_indx = 1;
		}
	}

	if (enable_fd_slot(new))
		goto out;

	if (U_OFILE(new)) {
		/* Release all System-V style record locks, if any */
		(void)gno_lockrelease (U_OFILE(new)); /* error? */

		/*
		 * It's a no-op, why call it?
		 *
		 * if (U_POFILE(new) & UF_MAPPED)
		 *	munmapfd(new);
		 */	

		fp2 = U_OFILE(new); 
		U_OFILE_SET(new, NULL);
		U_POFILE_SET(new, 0);
		closef(fp2);
		if (u.u_error)
			goto out;
	}
	if (new > u.u_omax)  /* track largest file pointer number */
		u.u_omax = new;
	smp_lock(&lk_file, LK_RETRY);
	dupit(new, fp, U_POFILE(old));
	smp_unlock(&lk_file);
out:
        AUDIT_CALL ( u.u_event, u.u_error, u.u_r.r_val1, AUD_HDR|AUD_PRM|AUD_RES, (int *)0, 0 );
}

dupit(fd, fp, flags)
	register int fd;
	register struct file *fp;
	register int flags;
{

	U_OFILE_SET(fd, fp);
#ifdef notdef
	flags &= ~(UF_INUSE); /* remove INUSE reference to inode   *002*/ 
#endif
	U_POFILE_SET(fd, flags);
	fp->f_count++;
}

/*
 * The file control system call.
 */
fcntl()
{
	register struct file *fp;
	register struct a {
		int	fdes;
		int	cmd;
		int	arg;
	} *uap;
	register int i;
	struct flock bf;
	register struct gnode *gp;		/* 004 */
	int oldwhence;		/* whence of request */

	uap = (struct a *)u.u_ap;
	GETF(fp, uap->fdes);
	smp_lock(&fp->f_lk, LK_RETRY);

	/* 004 - Fill gnode structure */

	gp = (struct gnode *)fp->f_data;

	switch(uap->cmd) {
	case F_DUPFD:
		i = uap->arg;
		if (i < 0 || i >= max_nofile) {
			u.u_error = EINVAL;
			break;
		}
		if ((i = ufalloc(i)) >= 0) {
			smp_lock(&lk_file, LK_RETRY);
			dupit(i, fp, U_POFILE(uap->fdes) & ~UF_EXCLOSE);
			smp_unlock(&lk_file);
		}
		break;

	case F_GETFD:
		u.u_r.r_val1 = U_POFILE(uap->fdes) & UF_EXCLOSE;
		break;

	case F_SETFD:
		U_POFILE_SET(uap->fdes, (U_POFILE(uap->fdes) & ~UF_EXCLOSE) |
			     (uap->arg & UF_EXCLOSE));
		break;

	case F_GETFL:
		u.u_r.r_val1 = fp->f_flag+FOPEN;
		break;

	case F_SETFL:
		/*
		 * Clear user-settable flags
		 */
		fp->f_flag &= ~FCNTLONLYSET;
		/*
		 * Now, only and in the flags that a user can set
		 * with this request.
		 */
		fp->f_flag |= ((uap->arg-FOPEN) & FCNTLONLYSET);
		/*
		 * Setting FNDELAY may need a call to a driver, handle
		 * it seperatly.
		 */
		u.u_error = fset(fp, FNDELAY, fp->f_flag & FNDELAY);
		if (!u.u_error) {
			u.u_error = fset(fp, FASYNC, fp->f_flag & FASYNC);
			if (u.u_error)
				(void) fset(fp, FNDELAY, 0);
		}
		break;

	case F_GETOWN:
		u.u_error = fgetown(fp, &u.u_r.r_val1);
		break;

	case F_SETOWN:
		u.u_error = fsetown(fp, uap->arg);
		break;

	case F_GETLK:
	case F_SETLK:
	case F_SETLKW:

 		/* Locks supported for disk inodes only	*/

		if (fp->f_type != DTYPE_INODE) {
			u.u_error = EINVAL;
			break;
		}

		/*	Verify inode pointer	*/

		if (fp->f_data == NULL) {
			u.u_error = EFAULT;
			break;
		}

		/* get flock structure from user space */

		if (u.u_error = copyin((caddr_t)uap->arg, (caddr_t)&bf, 
		   sizeof(bf))){
			break;
		}

		/* 
		 * check access permissions:
		 * POSIX requires a valid type even for F_GETLK
		 * requests; otherwise, F_GETLK requests may point
		 * to an empty flock structure.
		 */

		if ((uap->cmd != F_GETLK) || 
		    (u.u_procp->p_progenv == A_POSIX)) {

		   switch (bf.l_type) {

			case F_RDLCK:
				if ((uap->cmd != F_GETLK) &&
				    !(fp->f_flag & FREAD)) {
					u.u_error = EBADF;
				}
				break;

			case F_WRLCK:
				if ((uap->cmd != F_GETLK) &&
				    !(fp->f_flag & FWRITE)) {
					u.u_error = EBADF;
				}
				break;

			case F_UNLCK:
				break;

			default:
				u.u_error = EINVAL;
				break;

		   }			/* End switch */
		   if (u.u_error)	/* error encountered above */
			   break;
		}
		
		/* convert offset to start of file */

		oldwhence = bf.l_whence;	/* save to renormalize later */
		if (u.u_error = rewhence(&bf, fp, 0)) {
			break;
		}

		/* convert negative lengths to positive */

		if (bf.l_len < 0) {
			bf.l_start += bf.l_len;		/* adjust start */
			bf.l_len = -(bf.l_len);		/* absolute value */
		}
	 
		/* check for validity */

		if (bf.l_start < 0) {
			u.u_error = EINVAL;
			break;
		}

		/*
		 *	Now, process the request 
		 */

		if ((uap->cmd != F_GETLK) && (bf.l_type != F_UNLCK)) {
			/* If locking is attempted, mark file locked
			 * to force unlock on close.
			 * Also, since the SVID specifies that the 
			 * FIRST close releases all locks, mark process
			 * to reduce the search overhead in 
			 * gno_lockrelease().
		 	 */
	
			U_POFILE_SET(uap->fdes, U_POFILE(uap->fdes) | UF_FDLOCK);
			u.u_procp->p_file |= SLKDONE;
		}

		/*
		 * Dispatch out to specific file system to do the actual 
		 * locking. Then, continue based upon the error returned.
		 */

		switch (u.u_error = GRLOCK (gp, &bf, uap->cmd, fp)) {

			case 0:
				break;		/* continue, if successful */

			default:	/* some other error code */
				break;

		}		/* End Switch */
		if (u.u_error)	/* error encountered above */
			break;

		/* if F_GETLK, return flock structure to user space */

		if (uap->cmd == F_GETLK) {
			/* 
			 * per SVID and P1003.1 : change only 'l_type' 
			 * field if unlocked
			 */
			if (bf.l_type == F_UNLCK) {
				u.u_error = copyout((caddr_t)&bf.l_type,
				   (caddr_t)&((struct flock*)uap->arg)->l_type,
				    sizeof (bf.l_type));
			} else {
				/* 
				 * per P1003.1 : 'l_whence' field must be
				 * relative to start of file (SEEK_SET)
				 */
				if (u.u_procp->p_progenv != A_POSIX)
					u.u_error=rewhence(&bf, fp, oldwhence);
				if (!u.u_error)
					u.u_error = copyout((caddr_t) &bf,
				       (caddr_t) uap->arg, sizeof (bf));
			}

		}	/* End if F_GETLK processing */


# ifdef KLMDEBUG
		if (klm_debug) {
			printf ("fcntl(3) ret: eno:%d cmd:%d ty:%d wh:%d st:%d ln:%d pid:%d\n",
			u.u_error,uap->cmd, bf.l_type, bf.l_whence, bf.l_start,
			bf.l_len, bf.l_pid);
		}
# endif KLMDEBUG

		break;

	case F_SETSYN:
		/* set syncronous write if regular file */
		if ((gp->g_mode&GFMT) != GFREG) {
			if ((gp->g_mode&GFMT) == GFSOCK) {
				/*
				 * 004 - If a socket was given, return
				 * appropriate error.
				 */
				u.u_error = EOPNOTSUPP;
			}
			else {
				/*	
				 * 004 - Not a regular file or socket. Return
				 * appropriate error.
				 */
				u.u_error = EINVAL;
			}
		        break;
	        }

		/*
		 * Its a regular file, now check fp flag to
		 * verify that file is open for writing. If not, return
		 * an error.
		 */
		if ((fp->f_flag&FWRITE) == 0) {
			u.u_error = EINVAL;
		}
		else {
			fp->f_flag |= FSYNCRON;
		}
		break;

	case F_CLRSYN:
		/* Clear syncronous write flag */
		/* First verify it is a regular file. */
		if ((gp->g_mode&GFMT) != GFREG) {
			if ((gp->g_mode&GFMT) == GFSOCK) {
				/*
				 * If a socket was given, return
				 * appropriate error.
				 */
				u.u_error = EOPNOTSUPP;
			}
			else {
				/*	
				 * Not a regular file or socket. Return
				 * appropriate error.
				 */
				u.u_error = EINVAL;
			}
			break;
		}

		/* Regular file ! */
		fp->f_flag &= ~FSYNCRON;
		break;

	default:
		u.u_error = EINVAL;
	}
	smp_unlock(&fp->f_lk);
}

fset(fp, bit, value)
	register struct file *fp;
	register int bit;
	int  value;
{

	if (value)
		fp->f_flag |= bit;
	else
		fp->f_flag &= ~bit;
	return (fioctl(fp, (int)(bit == FNDELAY ? FIONBIO : FIOASYNC),
	    (caddr_t)&value));
}

fgetown(fp, valuep)
	register struct file *fp;
	register int *valuep;
{
	register int error;

	switch (fp->f_type) {

	case DTYPE_SOCKET:
		*valuep = ((struct socket *)fp->f_data)->so_pgrp; /*001*/
		return (0);

	case DTYPE_PIPE:
	case DTYPE_PORT:
		error = fioctl(fp, (int)FIOGETOWN, (caddr_t)valuep);
		return (error);
	default:
		error = fioctl(fp, (int)TIOCGPGRP, (caddr_t)valuep);
		*valuep = -*valuep;
		return (error);
	}
}

fsetown(fp, value)
	register struct file *fp;
	int value;
{

	if (fp->f_type == DTYPE_SOCKET) {
		((struct socket *)fp->f_data)->so_pgrp = value; /*001*/
		return (0);
	}
	if ((fp->f_type == DTYPE_PIPE) || (fp->f_type == DTYPE_PORT))
	        return (fioctl(fp, (int)FIOSETOWN, (caddr_t)&value));
	if (value > 0) {
		struct proc *p = pfind(value);
		if (p == 0)
			return (EINVAL);
		value = p->p_pgrp;
	} else
		value = -value;
	return (fioctl(fp, (int)TIOCSPGRP, (caddr_t)&value));
}

fioctl(fp, cmd, value)
	register struct file *fp;
	register int cmd;
	register caddr_t value;
{

	return ((*fp->f_ops->fo_ioctl)(fp, cmd, value, u.u_cred));
}

close()
{
	register struct a {
		int	i;
	} *uap = (struct a *)u.u_ap;
	register struct file *fp;
	register struct gnode *gp;
	register int j;
	register caddr_t value;

	GETF(fp, uap->i);

	/* Release all System-V style record locks, if any */
	(void) gno_lockrelease (fp);   /* error? */

	gp = (struct gnode *)fp->f_data;
	if ((U_POFILE(uap->i) & UF_INUSE) && gp) {
		U_POFILE_SET(uap->i, U_POFILE(uap->i) & ~UF_INUSE);
		/* clear inuse only when last inuse open file */
		for (j=0; j <= u.u_omax; j++)
			if ((U_POFILE(j) & UF_INUSE) &&
			    U_OFILE(j) == fp)
				goto stillinuse;
		(*fp->f_ops->fo_ioctl)(fp, FIOCINUSE, value, u.u_cred);
		gfs_lock(gp);
		gp->g_flag &= ~(GINUSE);
		gfs_unlock(gp);
		wakeup((caddr_t)&gp->g_flag);
	}
stillinuse:

	U_OFILE_SET(uap->i, NULL);
	U_POFILE_SET(uap->i, 0);

	/* audit info */
	if ( audswitch && gp ) {
		u.u_gno_dev[0] = (gp->g_mode&GFMT) == GFCHR ? gp->g_rdev : gp->g_dev;
		u.u_gno_num[0] = gp->g_number;
		u.u_gno_indx = 1;
	}

	u.u_error = closef(fp);
	AUDIT_CALL ( u.u_event, u.u_error, u.u_r.r_val1, AUD_HDR|AUD_PRM|AUD_RES, (int *)0, 0 );

	/*
	 * It's a no-op, why call it?
	 *
	 * if (*pf & UF_MAPPED)
	 *	munmapfd(uap->i);
	 */	

}
	

fstat()
{
	register struct file *fp;
	register struct a {
		int	fdes;
		struct	stat *sb;
	} *uap;
	struct stat ub;

	uap = (struct a *)u.u_ap;
	GETF(fp, uap->fdes);
	switch (fp->f_type) {

	case DTYPE_PORT:
	case DTYPE_INODE:
	case DTYPE_PIPE:
		if (fp->f_data)
			u.u_error = gno_stat((struct gnode *)fp->f_data, &ub);
		else
			u.u_error = EBADF;
		break;

	case DTYPE_SOCKET:
		u.u_error = soo_stat((struct socket *)fp->f_data, &ub);
		break;

	default:
		panic("fstat");
		/*NOTREACHED*/
	}
	if (u.u_error == 0)
		u.u_error = copyout((caddr_t)&ub,(caddr_t)uap->sb,sizeof (ub));
}

/*
 * Allocate descriptor slots in the range [0 .. max_nofile], assuming that
 * the range [0 .. NOFILE_IN_U] is currently allocated.  Intended to be
 * called only from the enable_fd_slots macro.
 *
 * Return 0 on success, -1 on failure.
 */
int
alloc_fd_slots(fdno)
	int fdno;  /* fd of interest */
{
	struct file **ofilep;
	char *pofilep;
	int count;

	/*
	 * The first overflow buffer will contain 64 file descriptors.
	 * Subsequent allocations will double buffer size.  
	 */
	if (u.u_of_count == 0)
		count = NOFILE_IN_U;
	else
		count = 2 * u.u_of_count;
	/*
	 * If the buffer size calculated above is not big enough to be
	 * able to reference the fd of interest, we figure out how big
	 * the buffer needs to be (making it a multiple of NOFILE_IN_U).
	 * This case can happen as a result of a dup2.
	 */
	if (fdno >= (count + NOFILE_IN_U))
		count = (fdno / NOFILE_IN_U) * NOFILE_IN_U;

        /*
         * Allocate new overflow structures. KM_ALLOC may sleep
	 * waiting to allocate virtual memory, but will never
	 * fail.
         */
	KM_ALLOC(ofilep, struct file **, count * sizeof (struct file *), 
		 KM_NOFILE, KM_CLEAR);
	KM_ALLOC(pofilep, char *, count, KM_NOFILE, KM_CLEAR);

	/*
	 * Copy current overflow buffer if it exists.
	 */
	if (u.u_of_count) {
		bcopy(u.u_ofile_of, ofilep, 
		      u.u_of_count * sizeof (struct file *));
		bcopy(u.u_pofile_of, pofilep, u.u_of_count);
		/*
		 * Release previous memory back to pool.
		 */
		KM_FREE(u.u_ofile_of, KM_NOFILE);
		KM_FREE(u.u_pofile_of, KM_NOFILE);
	}

	/*
	 * Update pointers and current counts.
	 */
	u.u_ofile_of = ofilep;
	u.u_pofile_of = pofilep;
	u.u_of_count = count;

	return(u.u_error);
}

/*
 * Allocate a user file descriptor.
 */
ufalloc(i)
	register int i;
{

	for (; i < max_nofile; i++) {
                /*
                 * Make sure that slot i exists before referring to it.
                 */
                if (enable_fd_slot(i))
                        return (-1);

		if (U_OFILE(i) == NULL) {
			u.u_r.r_val1 = i;
			U_POFILE_SET(i, 0);
			if (i > u.u_omax)
				u.u_omax = i;
			return (i);
		}
	}
	u.u_error = EMFILE;
	return (-1);
}

ufavail()
{
        register int i;
	register int maxfd;
        register int avail;

        /*
         * If the process hasn't yet had occasion to allocate its
         * overflow descriptor array out to the maximum extent, confine the
         * search to the preallocated slots and credit the descriptors
         * obtainable by reallocating to the maximum extent.
         */

	if (u.u_omax >= 0)
		maxfd = u.u_omax;
	else
		return(max_nofile);
	avail = (max_nofile - maxfd) - 1;
        for (i = 0; i < maxfd; i++)
                if (U_OFILE(i) == NULL)
                        avail++;
        return (avail);
}

struct	file *lastf;
/*
 * Allocate a user file descriptor
 * and a file structure.
 * Initialize the descriptor
 * to point at the file structure.
 */
struct file *
falloc()
{
	register struct file *fp;
	register int i;

	i = ufalloc(0);
	if (i < 0)
		return (NULL);
	smp_lock(&lk_file, LK_RETRY);
	if (lastf == 0)
		lastf = file;
	for (fp = lastf; fp < fileNFILE; fp++)
		if (fp->f_count == 0)
			goto slot;
	for (fp = file; fp < lastf; fp++)
		if (fp->f_count == 0)
			goto slot;
	smp_unlock(&lk_file);
	tablefull("file");
	u.u_error = ENFILE;
	return (NULL);
slot:
	fp->f_data = 0;
	fp->f_count = 1;
	lastf = fp + 1;
	smp_unlock(&lk_file);

	U_OFILE_SET(i, fp);
	fp->f_offset = 0;
	crhold(u.u_cred);
	fp->f_cred = u.u_cred;
	return (fp);
}

/*
 * Convert a user supplied file descriptor into a pointer
 * to a file structure.  Only task is to check range of the descriptor.
 * Critical paths should use the GETF macro.
 */
struct file *
getf(f)
	register int f;
{
	register struct file *fp;

	if (f < 0 || (unsigned)f > u.u_omax ||
	    (fp = U_OFILE(f)) == NULL) {
		u.u_error = EBADF;
		return (NULL);
	}
	return (fp);
}

/*
 * Internal form of close.
 * Decrement reference count on file structure.
 */
int closef_nulls = 0;
int gc_count = 0;
closef(fp)
	register struct file *fp;
{
	register int othernbuf = 0;
	int err;

	if (fp == NULL) {
		closef_nulls++;
		return(0);
	}

	smp_lock(&fp->f_lk, LK_RETRY);

	cleanlocks(fp);			/* 002  */

	if(fp->f_flag&FNBUF) {
		struct gnode *gp = (struct gnode *)fp->f_data;
		othernbuf = asyncclose(gp->g_rdev, fp->f_flag);
	}

	smp_lock(&lk_file, LK_RETRY);
	if (fp->f_count > 1) {
		fp->f_count--;
		smp_unlock(&lk_file);
		if (fp->f_count == fp->f_msgcount)
			gc_count += fp->f_msgcount;
		smp_unlock(&fp->f_lk);
		return(0);
	}
	smp_unlock(&lk_file);
	if(fp->f_flag&FNBUF) {
		struct gnode *gp = (struct gnode *)fp->f_data;
		fp->f_flag &= ~FNBUF;
		if(!othernbuf) {
		        gfs_lock(gp);
			gp->g_flag &= ~GSYNC;
			gfs_unlock(gp);
		}
	}

	err = (*fp->f_ops->fo_close)(fp);
	crfree(fp->f_cred);
	smp_lock(&lk_file, LK_RETRY);
	fp->f_data = (caddr_t)0;
	fp->f_count = 0;
	smp_unlock(&lk_file);
	smp_unlock(&fp->f_lk);
	return(err);
}

/*
 * Apply an advisory lock on a file descriptor.
 */
flock()
{
	register struct a {
		int	fd;
		int	how;
	} *uap = (struct a *)u.u_ap;
	register struct file *fp;

	GETF(fp, uap->fd);
	if (fp->f_type != DTYPE_INODE && fp->f_type != DTYPE_PORT ) {
		u.u_error = EOPNOTSUPP;
		return;
	}
 	if ((uap->how & (LOCK_UN|LOCK_EX|LOCK_SH)) == 0){
 		u.u_error = EINVAL;			
 		return;
 	}
	if (uap->how & LOCK_UN) {
		smp_lock(&fp->f_lk, LK_RETRY);
		gno_unlock(fp, FSHLOCK|FEXLOCK);
		smp_unlock(&fp->f_lk);
		return;
	}
	/* SMP lock file table entry while locking file descriptor */
	smp_lock(&fp->f_lk, LK_RETRY);
	/* avoid work... */
	if ((fp->f_flag & FEXLOCK) && (uap->how & LOCK_EX) ||
	    (fp->f_flag & FSHLOCK) && (uap->how & LOCK_SH)) {
		smp_unlock(&fp->f_lk);
		return;
	}
	if (fp->f_data)
		u.u_error = gno_lock(fp, uap->how);
	else
		u.u_error = EBADF;

	smp_unlock(&fp->f_lk);
}

/*
 *	Normalize System V-style record locks
 */

rewhence(ld, fp, newwhence)
	struct flock *ld;
	struct file *fp;
	int newwhence;
{
	register int error;
	struct gnode *gp = (struct gnode *)fp->f_data;

	/* 
	 * If reference is to end-of-file, then get current attributes.  
	 * Three return values are possible:  unimplemented (GNOFUNC), 
	 * error (u.u_error),  and normal return (0).  
	 */

	if ((ld->l_whence == 2) || (newwhence == 2)) {
		if ((error = GGETVAL(gp)) > 0 ) {
			return (error);
		}
	}

	/* normalize to start of file */

	switch (ld->l_whence) {
	case 0:
		break;
	case 1:
		ld->l_start += fp->f_offset;
		break;
	case 2:
		ld->l_start += gp->g_size;
		break;
	default:
		return(EINVAL);
	}

	/* renormalize to given start point */

	switch (ld->l_whence = newwhence) {
		case 0:
			break;
		case 1:
			ld->l_start -= fp->f_offset;
			break;
		case 2:
			ld->l_start -= gp->g_size;
			break;
		default:
			return (EINVAL);
	}

	return(0);
}

/*
 * Initialize the file table locks.
 */
finit()
{
        struct file *fp;
	extern struct lk_acct lk_acct;

	lockinit(&lk_file, &lock_file_d);
	lockinit(&lk_acct, &lock_acct_d);
	for (fp = file; fp < fileNFILE; fp++) {
	        lockinit(&fp->f_lk, &lock_eachfile_d);
	}
}
