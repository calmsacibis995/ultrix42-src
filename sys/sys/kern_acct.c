#ifndef lint
static  char    *sccsid = "@(#)kern_acct.c	4.3  ULTRIX  4/25/91";
#endif

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
 *
 *			Modification History
 *
 *	Ray Glaser  11 Apr 91  (rjg)
 *	  Correct mips handling of ac_etime to be comp_t type
 *
 *	Paul Shaughnessy 06 Apr 89
 *	  Added SMP lock and logic for lk_acct.
 *
 *	Al Delorey (for rr) 18-Nov-88
 *      Add system accounting hooks.
 *
 *	Charlie Briggs 19 May 88
 * 	  Modified GFS interface.
 *
 *	Paul Shaughnessy 27-Apr-88
 *	  Enhanced acctp handling in acct() and sysacct().
 *
 *	Chet Juszczak 10-Feb-88
 *	  Added gnode locks in acct() and sysacct().
 *
 *	Tim Burke, 28-Dec-87
 *	  Moved u.u_ttyp to u.u_procp->p_ttyp.
 *
 *	Joe Amato, 14-Dec-87
 *	   Added new KM_ALLOC/KM_FREE macros
 *
 *	Suzanne Logcher, 02-Jun-87
 *	  Added acctcred to hold a valid cred structure.  Need to crhold
 *	  and crfree it.  GSYNCG needed in order to flush out buffers
 *	  marked delayed write before GRELE which will disassociate 
 *	  acct buffer from cred in gnode.
 *
 *	Koehler 11 Sep 86
 *	  gfs namei interface change
 *
 *	Stephen Reilly, 09-Sept-85
 *	  Modified to handle the new 4.3BSD namei code.
 *
 *
 ***********************************************************************/
#include "../h/param.h"
#include "../h/systm.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/gnode.h"
#include "../h/mount.h"
#include "../h/buf.h"
#include "../h/kernel.h"
#include "../h/acct.h"
#include "../h/uio.h"
#include "../h/kmalloc.h"
#include "../h/proc.h"

/*
 * SHOULD REPLACE THIS WITH A DRIVER THAT CAN BE READ TO SIMPLIFY.
 */
struct	gnode *acctp = NULL;
struct	gnode *savacctp = NULL;
struct	ucred *acctcred = NULL;

struct lock_t lk_acct;

/*
 * Perform process accounting functions.
 */
sysacct()
{
	register struct gnode *gp;
	register struct a {
		char	*fname;
	} *uap = (struct a *)u.u_ap;
	register struct nameidata *ndp = &u.u_nd;

	if (suser()) {
		smp_lock(&lk_acct, LK_RETRY);
		if (acctcred == NULL) {
			crhold(u.u_cred);
			acctcred = u.u_cred;
		}
		if (savacctp) {
			acctp = savacctp;
			savacctp = NULL;
		}
		if (uap->fname==NULL) {
			if (gp = acctp) {
				gfs_lock(gp);
				if (acctp != NULL) {
					acctp = NULL;
					GSYNCG(gp, acctcred);
					crfree(acctcred);
					acctcred = NULL;
					gput(gp);
				} else
				       gfs_unlock(gp);
			}
			smp_unlock(&lk_acct);
			return;
		}
 		ndp->ni_nameiop = LOOKUP | FOLLOW;

		KM_ALLOC(ndp->ni_dirp, char *, MAXPATHLEN, KM_NAMEI, KM_NOARG); 
		if(ndp->ni_dirp == NULL)
			goto out;
 		if(u.u_error = copyinstr(uap->fname, ndp->ni_dirp, MAXPATHLEN,
		(u_int *) 0)) {
			KM_FREE(ndp->ni_dirp, KM_NAMEI);
			goto out;
		}

 		gp = gfs_namei(ndp);

		KM_FREE(ndp->ni_dirp, KM_NAMEI);
		if(gp == NULL)
			goto out;
		if((gp->g_mode & GFMT) != GFREG) {
			u.u_error = EACCES;
			gput(gp);
			goto out;
		}
		if (ISREADONLY(gp->g_mp)) {
			u.u_error = EROFS;
			gput(gp);
			goto out;
		}
		if (acctp && (acctp->g_number != gp->g_number ||
		    acctp->g_dev != gp->g_dev)) {
			GSYNCG(acctp, acctcred);
			grele(acctp);
		}
		acctp = gp;
		gfs_unlock(gp);
out:
		smp_unlock(&lk_acct);
	}
}

#ifdef mips
/*
 * Produce a pseudo-floating point representation
 * with 3 bits base-8 exponent, 13 bits fraction.
 */
static
compress(t, ut)
	register long t;
	long ut;
{
	register int exp = 0, round = 0;

	t = t * AHZ;  /* compiler will convert only this format to a shift */
	if (ut)
		t += ut / (1000000 / AHZ);
	while (t >= 8192) {
		exp++;
		round = t&04;
		t >>= 3;
	}
	if (round) {
		t++;
		if (t >= 8192) {
			t >>= 3;
			exp++;
		}
	}
	return ((exp<<13) + t);
}
#endif mips

int	acctsuspend = 2;	/* stop accounting when < 2% free space left */
int	acctresume = 4;		/* resume when free space risen to > 4% */
struct	acct acctbuf;
/*
 * On exit, write a record on the accounting file.
 */
acct()
{
	register int i;
	register struct gnode *gp;
	register struct fs_data *fs_data;
	off_t siz;
	register struct acct *ap = &acctbuf;
#ifdef vax
	double itime, million = 1000000.0;
#endif vax
#ifdef mips
	struct rusage *ru = &u.u_ru;
	struct timeval t;
#endif mips
	struct uio _auio;
	register struct uio *auio = &_auio;
	struct iovec _aiov;
	register struct iovec *aiov = &_aiov;

	smp_lock(&lk_acct, LK_RETRY);
	if (savacctp) {
		fs_data = GGETFSDATA(savacctp->g_mp);
		if(acctresume * fs_data->fd_btot / 100 <= fs_data->fd_bfreen) {
			acctp = savacctp;
			savacctp = NULL;
			printf("Accounting resumed\n");
		}
	}
	if ((gp = acctp) == NULL) {
		smp_unlock(&lk_acct);
		return;
	}
	gfs_lock(gp);
	/*
	 * XXX - Unfortunatly, during shutdown, one of the last system
	 * calls init makes turns accounting off. If a process is exiting
	 * while the shutdown occurs, then there is a good possibility
	 * that the accounting file will have been closed, by the time we
	 * get the lock. The only way to prevent this race condition is to
	 * check that acctp is non null, before continuing.
	 */
	if (acctp == NULL) {	/* Accounting was turned off while waiting */
		smp_unlock(&lk_acct);
		gfs_unlock(gp);
		return;
	}
	fs_data = GGETFSDATA(acctp->g_mp);
	if(acctsuspend * fs_data->fd_btot / 100 > fs_data->fd_bfreen) {
		savacctp = acctp;
		acctp = NULL;
		smp_unlock(&lk_acct);
		gfs_unlock(gp);
		printf("Accounting suspended\n");
		return;
	}
	for (i = 0; i < sizeof (ap->ac_comm); i++)
		ap->ac_comm[i] = u.u_comm[i];

	/* RR - 001
	 * Changed fields are ac_utime, ac_stime, ac_etime, ac_mem, ac_io
	 * The decision here is to keep the accounting records small (44 bytes)
	 * yet maintain accuracy. The first three times (user, sys, elapsed)
	 * need to be accurate fractions. The mem usage and io are also
	 * floats for accuracy, i.e., we won't overflow, we get fractional usage
	 * of memory and we save bytes over doubles. Note that we also threw
	 * out the routine compress for the 13 bit pseudo-fraction and
	 * 3 bit exponent
	 */
#ifdef vax
#define tm_to_dbl(tmbuf) (double)tmbuf.tv_sec+(tmbuf.tv_usec/million)
	ap->ac_utime = tm_to_dbl(u.u_ru.ru_utime);
	ap->ac_stime = tm_to_dbl(u.u_ru.ru_stime);
/*..rjg..*/
	ap->ac_etime = time.tv_sec - u.u_start.tv_sec;
#endif vax
#ifdef mips
	ap->ac_utime = compress(u.u_ru.ru_utime);
	ap->ac_stime = compress(u.u_ru.ru_stime);
/*..rjg..*/
	ap->ac_etime = compress((time.tv_sec - u.u_start.tv_sec),0L);
#endif mips
	ap->ac_btime = u.u_start.tv_sec;
	ap->ac_uid = u.u_ruid;
	ap->ac_gid = u.u_rgid;
	ap->ac_mem = 0;
#ifdef vax
	itime = tm_to_dbl(u.u_ru.ru_utime) + tm_to_dbl(u.u_ru.ru_stime);
	if (itime > 0.01) {
		ap->ac_mem = (u.u_ru.ru_ixrss  + u.u_ru.ru_idrss +
				u.u_ru.ru_ismrss + u.u_ru.ru_isrss) / itime;
	}
	ap->ac_io = u.u_ru.ru_inblock + u.u_ru.ru_oublock;
#endif vax
#ifdef mips
	t = ru->ru_stime;
	timevaladd(&t, &ru->ru_utime);
        if (i = t.tv_sec * hz + t.tv_usec / tick)
                ap->ac_mem = (ru->ru_ixrss + ru->ru_idrss + 
				ru->ru_isrss + ru->ru_ismrss) / i;
	ap->ac_mem >>= CLSIZELOG2;
        ap->ac_io = compress(ru->ru_inblock + ru->ru_oublock, (long)0);
#endif mips
	if (u.u_procp->p_ttyp)
		ap->ac_tty = u.u_ttyd;
	else
		ap->ac_tty = NODEV;
	ap->ac_flag = u.u_acflag;
	siz = gp->g_size;
	u.u_error = 0;				/* XXX */

	auio->uio_iov = aiov;
	auio->uio_iovcnt = 1;
	aiov->iov_base = (caddr_t)ap;
	aiov->iov_len = sizeof (acctbuf);
	auio->uio_resid = sizeof (acctbuf);
	auio->uio_offset = siz;
	auio->uio_segflg = 1;
	u.u_error = rwgp(gp, auio, UIO_WRITE, IO_ASYNC, acctcred);
	if (auio->uio_resid)
		u.u_error = EIO;
	if (u.u_error)
		GTRUNC(gp, (u_long)siz, acctcred);
	smp_unlock(&lk_acct);
	gfs_unlock(gp);
}
