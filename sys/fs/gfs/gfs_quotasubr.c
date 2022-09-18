#ifndef lint
static char *sccsid = "@(#)gfs_quotasubr.c	4.2	ULTRIX	11/9/90";
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
 *	prs 06 Apr 89
 *	Added SMP quota.
 *
 *	koehler 11 Sep 86
 *	dev_t can't be a register
 *
 *	Stephen Reilly, 09-Sept-85
 *	Modified to handle the new 4.3BSD namei code.
 *
 ***********************************************************************/
#ifdef QUOTA
/*
 * MELBOURNE QUOTAS
 *
 * Miscellaneous subroutines.
 */
#include "../h/param.h"
#include "../h/systm.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/proc.h"
#include "../h/gnode.h"
#include "../h/quota.h"
#include "../h/mount.h"
#include "../h/uio.h"

struct dquot *
dqp(q, dev)
	register struct quota *q;
	dev_t dev;
{
	register struct dquot **dqq;
	register int i;
	register struct mount *mp;
	struct gnode *rgp;
	
	if (q == NOQUOTA || q->q_flags & Q_NDQ)
		return (NODQUOT);
#ifdef SMP_DEBUG
	if (q->q_state & Q_LOCK == 0)
		panic("dqp: Quota should be locked");
#endif
	GETMP(mp, dev);
	rgp = (struct gnode *)fref(mp, dev);
	if (rgp == NULL)
		return (NODQUOT);
	i = mp - mount;
	dqq = &q->q_dq[i];
	dquot_lock((*dqq));
	if (*dqq == LOSTDQUOT) {
		*dqq = discquota(q->q_uid, mount[i].m_qinod);
		if (*dqq != NODQUOT)
			(*dqq)->dq_own = q;
	}
	if (*dqq != NODQUOT)
		(*dqq)->dq_cnt++;
	if (rgp)
		grele(rgp);
	return (*dqq);
}

/*
 * Quota cleanup at process exit, or when
 * switching to another user.
 */
qclean()
{
	register struct proc *p = u.u_procp;
	register struct quota *q = p->p_quota;

	if (q == NOQUOTA)
		return;
	quota_lock(q);
	/*
	 * Before we rid ourselves of this quota, we must be sure that
	 * we no longer reference it (otherwise clock might do nasties).
	 * But we have to have some quota (or clock will get upset).
	 * (Who is this clock anyway ??). So we will give ourselves
	 * root's quota for a short while, without counting this as
	 * a reference in the ref count (as either this proc is just
	 * about to die, in which case it refers to nothing, or it is
	 * about to be given a new quota, which will just overwrite this
	 * one).
	 */
	p->p_quota = quota;
	u.u_quota = quota;
	delquota(q);
	quota_unlock(q);
}

qstart(q)
	register struct quota *q;
{

#ifdef SMP_DEBUG
	if (q->q_state & Q_LOCK == 0)
		panic("qstart: q 0x%x not locked\n", q);
#endif
	u.u_quota = q;
	u.u_procp->p_quota = q;
	quota_unlock(q);
}

qwarn(dq)
	register struct dquot *dq;
{
	register char *name;
	register struct mount *mp;
	
	GETMP(mp, dq->dq_dev);
	name = mp->m_fs_data->fd_path;

#ifdef SMP_DEBUG
	if ((dq->dq_state & DQ_LOCK) == 0)
		panic("qwarn: dquot should be locked");
#endif

	if (dq->dq_isoftlimit && dq->dq_curinodes >= dq->dq_isoftlimit) {
		dq->dq_flags |= DQ_MOD;
		if (dq->dq_iwarn && --dq->dq_iwarn)
			uprintf(
			    "Warning: too many files on %s, %d warning%s left\n"
			    , name
			    , dq->dq_iwarn
			    , dq->dq_iwarn > 1 ? "s" : ""
			);
		else
			uprintf(
			    "WARNING: too many files on %s, NO MORE!!\n", name);
	} else
		dq->dq_iwarn = MAX_IQ_WARN;

	if (dq->dq_bsoftlimit && dq->dq_curblocks >= dq->dq_bsoftlimit) {
		dq->dq_flags |= DQ_MOD;
		if (dq->dq_bwarn && --dq->dq_bwarn)
			uprintf(
		    "Warning: too much disc space on %s, %d warning%s left\n"
			    , name
			    , dq->dq_bwarn
			    , dq->dq_bwarn > 1 ? "s" : ""
			);
		else
			uprintf(
		    "WARNING: too much disc space on %s, NO MORE!!\n"
			    , name
			);
	} else
		dq->dq_bwarn = MAX_DQ_WARN;
}
#endif
