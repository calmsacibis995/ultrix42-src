#ifndef lint
static char *sccsid = "@(#)gfs_quota.c	4.2	ULTRIX	9/7/90";
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
 *	lebel  17-Aug-90
 *	Added reset of warnings if below disk quota in chkdq.
 *
 *	prs 06 Apr 89
 *	Added SMP quota.
 *
 *	koehler 11 Sep 86
 *	registerized a few things
 *
 *	Stephen Reilly, 09-Sept-85
 *	Modified to handle the new 4.3BSD namei code.
 *
 ***********************************************************************/
#ifdef QUOTA
/*
 * MELBOURNE QUOTAS
 *
 * Routines used in checking limits on file system usage.
 */
#include "../h/param.h"
#include "../h/systm.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/proc.h"
#include "../h/gnode.h"
#include "../h/quota.h"
#include "../h/buf.h"
#include "../h/mount.h"
#include "../h/uio.h"

#ifdef GFSDEBUG
extern short GFS[];
#endif

/*
 * Find the dquot structure that should
 * be used in checking i/o on gnode gp.
 */
struct dquot *
inoquota(gp)
	register struct gnode *gp;
{
	register struct quota *q;
	register struct dquot **dqq;
	register struct mount *mp;
	register int index;
	register struct dquot *dq;
	struct gnode *rgp;
	
 top:
	q = qfind(gp->g_uid);
	if (q == NOQUOTA) {
#ifdef GFSDEBUG
		if(GFS[8])
			cprintf("inoquota: no quotas, gid %d m_qinod 0x%x\n",
			gp->g_uid, gp->g_mp->m_qinod);
#endif
		dq = discquota(gp->g_uid, gp->g_mp->m_qinod);
		dquot_unlock((dq));
		return(dq);
	}

	/*
	 * We have a quota struct in core (most likely our own) that
	 * belongs to the same user as the gnode
	 */
	if (q->q_flags & Q_NDQ) {
		quota_unlock(q);
		return (NODQUOT);
	}
	
	GETMP(mp, gp->g_dev);
	rgp = (struct gnode *)fref(mp, gp->g_dev);
	if(rgp == NULL) {
		quota_unlock(q);
		return (NODQUOT);
	}
	index = mp - mount;
	dqq = &q->q_dq[index];
	dquot_lock((*dqq));
	if (*dqq == LOSTDQUOT) {
		*dqq = discquota(q->q_uid, mount[index].m_qinod);
		if (*dqq != NODQUOT)
			(*dqq)->dq_own = q;
	}
	if (*dqq != NODQUOT) {
		(*dqq)->dq_cnt++;
		dquot_unlock((*dqq));
	}
	quota_unlock(q);
	if (rgp)
		grele(rgp);
	return (*dqq);
}

/*
 * Update disc usage, and take corrective action.
 */
chkdq(gp, change, force)
	register struct gnode *gp;
	register long change;
	register int force;
{
	register struct dquot *dq;

	if (change == 0)
		return (0);
	dq = gp->g_dquot;
	if (dq == NODQUOT)
		return (0);
	if (dq->dq_bsoftlimit == 0)
		return (0);
	dquot_lock(dq);
	dq->dq_flags |= DQ_MOD;

	/*
	 * reset warnings if below disk quota.
	 */
	if (dq->dq_bwarn == 0 && dq->dq_bsoftlimit &&
            (dq->dq_curblocks + change) < dq->dq_bsoftlimit) {
         dq->dq_bwarn = MAX_DQ_WARN;
         if (dq->dq_own == u.u_quota) {
                uprintf("\nUNDER DISC QUOTA: (%s) by %d Kbytes\n",
			gp->g_mp->m_fs_data->fd_path,
                        dbtob(dq->dq_bsoftlimit -
                        (dq->dq_curblocks + change)) / 1024);
	 }
	}
	if (change < 0) {
		if ((int)dq->dq_curblocks + change >= 0)
			dq->dq_curblocks += change;
		else
			dq->dq_curblocks = 0;
		dq->dq_flags &= ~DQ_BLKS;
		dquot_unlock(dq);
		return (0);
	}

	/*
	 * If user is over quota, or has run out of warnings, then
	 * disallow space allocation (except su's are never stopped).
	 */
	if (u.u_uid == 0)
		force = 1;
	if (!force && dq->dq_bwarn == 0) {
		if ((dq->dq_flags & DQ_BLKS) == 0 && dq->dq_own == u.u_quota) {
		     uprintf("\nOVER DISC QUOTA: (%s) NO MORE DISC SPACE\n",
			gp->g_mp->m_fs_data->fd_path);
		     dq->dq_flags |= DQ_BLKS;
		}
		dquot_unlock(dq);
		return (EDQUOT);
	}
	if (dq->dq_curblocks < dq->dq_bsoftlimit) {
		dq->dq_curblocks += change;
		if (dq->dq_curblocks < dq->dq_bsoftlimit) {
			dquot_unlock(dq);
			return (0);
		}
		if (dq->dq_own == u.u_quota)
			uprintf("\nWARNING: disc quota (%s) exceeded\n",
			gp->g_mp->m_fs_data->fd_path);
		dquot_unlock(dq);
		return (0);
	}
	if (!force && dq->dq_bhardlimit &&
	    dq->dq_curblocks + change >= dq->dq_bhardlimit) {
		if ((dq->dq_flags & DQ_BLKS) == 0 && dq->dq_own == u.u_quota) {
			uprintf("\nDISC LIMIT REACHED (%s) - WRITE FAILED\n",
			gp->g_mp->m_fs_data->fd_path);
			dq->dq_flags |= DQ_BLKS;
		}
		dquot_unlock(dq);
		return (EDQUOT);
	}
	/*
	 * User is over quota, but not over limit
	 * or is over limit, but we have been told
	 * there is nothing we can do.
	 */
	dq->dq_curblocks += change;
	dquot_unlock(dq);
	return (0);
}

/*
 * Check the gnode limit, applying corrective action.
 */
chkiq(dev, gp, uid, force)
	dev_t dev;
	register struct gnode *gp;
	register int uid;
	register int force;
{
	register struct dquot *dq;
	register struct quota *q;
	register struct mount *mp;
	
	if (gp == NULL)	{		/* allocation */
		q = qfind(uid);
		if (q != NOQUOTA) {
			dq = dqp(q, dev);
			quota_unlock(q);
		} else {
			GETMP(mp, dev);
			if(mp == NULL)
				panic("chkiq");
			dq = discquota(uid, mp->m_qinod);
		}
	} else {			/* free */
		dq = gp->g_dquot;
		if (dq != NODQUOT) {
			dquot_lock(dq);
			dq->dq_cnt++;
		}
	}
	if (dq == NODQUOT)
		return (0);
	if (dq->dq_isoftlimit == 0) {
		dqrele(dq);
		dquot_unlock(dq);
		return (0);
	}
	dq->dq_flags |= DQ_MOD;
	if (gp) {			/* a free */
		if (dq->dq_curinodes)
			dq->dq_curinodes--;
		dq->dq_flags &= ~DQ_INODS;
		dqrele(dq);
		dquot_unlock(dq);
		return (0);
	}

	/*
	 * The following shouldn't be necessary, as if u.u_uid == 0
	 * then dq == NODQUOT & we wouldn't get here at all, but
	 * then again, its not going to harm anything ...
	 */
	if (u.u_uid == 0)		/* su's musn't be stopped */
		force = 1;
	if (!force && dq->dq_iwarn == 0) {
		if ((dq->dq_flags & DQ_INODS) == 0 && dq->dq_own == u.u_quota) {
			GETMP(mp, dq->dq_dev);
			uprintf("\nOVER FILE QUOTA - NO MORE FILES (%s)\n",
			mp->m_fs_data->fd_path);
			dq->dq_flags |= DQ_INODS;
		}
		dqrele(dq);
		dquot_unlock(dq);
		return (EDQUOT);
	}
	if (dq->dq_curinodes < dq->dq_isoftlimit) {
		if (++dq->dq_curinodes >= dq->dq_isoftlimit &&
		    dq->dq_own == u.u_quota) {
			GETMP(mp, dq->dq_dev);
			uprintf("\nWARNING - too many files (%s)\n",
			mp->m_fs_data->fd_path);
		}
		dqrele(dq);
		dquot_unlock(dq);
		return (0);
	}
	if (!force && dq->dq_ihardlimit &&
	    dq->dq_curinodes + 1 >= dq->dq_ihardlimit) {
		if ((dq->dq_flags & DQ_INODS) == 0 && dq->dq_own == u.u_quota) {
		     GETMP(mp, dq->dq_dev);
		     uprintf("\nFILE LIMIT REACHED - CREATE FAILED (%s)\n",
		     mp->m_fs_data->fd_path);
		     dq->dq_flags |= DQ_INODS;
		}
		dqrele(dq);
		dquot_unlock(dq);
		return (EDQUOT);
	}
	/*
	 * Over quota but not at limit;
	 * or over limit, but we aren't
	 * allowed to stop it.
	 */
	dq->dq_curinodes++;
	dqrele(dq);
	dquot_unlock(dq);
	return (0);
}
#endif
