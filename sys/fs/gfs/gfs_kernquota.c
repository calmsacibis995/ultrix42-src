#ifndef lint
static char *sccsid = "@(#)gfs_kernquota.c	4.2	ULTRIX	11/9/90";
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
 *	changed namei interface
 *
 *	Stephen Reilly, 09-Sept-85
 *	Modified to handle the new 4.3BSD namei code.
 *
 ***********************************************************************/

#ifdef QUOTA
/*
 * MELBOURNE QUOTAS
 *
 * Code pertaining to management of the in-core data structures.
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
#include "../h/kmalloc.h"

/*
 * Quota cache - hash chain headers.
 */
#define	NQHASH		32		/* a small prime */
#if	(NQHASH&(NQHASH-1) == 0)
#define	QHASH(uid)	((unsigned)(uid) & (NQHASH-1))
#else
#define	QHASH(uid)	((unsigned)(uid) % NQHASH)
#endif

struct	qhash	{
	struct	qhash	*qh_forw;	/* MUST be first */
	struct	qhash	*qh_back;	/* MUST be second */
};

struct	qhash	qhash[NQHASH];

/*
 * Quota free list.
 */
struct	quota	*qfreelist, **qfreetail;
typedef	struct quota *Qptr;
#ifdef GFSDEBUG
extern short GFS[];
#endif

/*
 * Dquot cache - hash chain headers.
 */
#define	NDQHASH		64		/* a smallish prime */
#if	(NDQHASH&(NDQHASH-1) == 0)
#define	DQHASH(uid, dev) \
	((unsigned)(((int)(dev) * 4) + (uid)) & (NDQHASH-1))
#else
#define	DQHASH(uid, dev) \
	((unsigned)(((int)(dev) * 4) + (uid)) % NDQHASH)
#endif

struct	dqhead	{
	struct	dqhead	*dqh_forw;	/* MUST be first */
	struct	dqhead	*dqh_back;	/* MUST be second */
};

struct	dqhead	dqhead[NDQHASH];

/*
 * Dquot free list.
 */
struct	dquot	*dqfreel, **dqback;

typedef	struct dquot *DQptr;

/*
 * Quota subsystem SMP sleep locks
 */
struct lock_t lk_quota;
struct lock_t lk_dquot;
struct lock_t lk_quotalocks;
struct lock_t lk_dquotlocks;
extern struct lock_t lk_gnode;

struct quota_active qactive[NMOUNT];
/*
 * Initialize quota caches.
 */
qtinit()
{
	register int i;
	register struct quota *q = quota;
	register struct qhash *qh = qhash;
	register struct dquot *dq = dquot;
	register struct dqhead *dh = dqhead;

	lockinit(&lk_quotalocks, &lock_quotalocks_d);
	lockinit(&lk_quota, &lock_quota_d);
	lockinit(&lk_dquotlocks, &lock_dquotlocks_d);
	lockinit(&lk_dquot, &lock_dquot_d);
	/*
	 * First the cache of structures assigned users.
	 */
	for (i = NQHASH; --i >= 0; qh++)
		qh->qh_forw = qh->qh_back = qh;
	qfreelist = q;
	qfreetail = &q->q_freef;
	q->q_freeb = &qfreelist;
	q->q_forw = q;
	q->q_back = q;
	for (i = nquota; --i > 0; ) {
		++q;
		q->q_forw = q;
		q->q_back = q;
		*qfreetail = q;
		q->q_freeb = qfreetail;
		qfreetail = &q->q_freef;
	}
	q->q_freef = NOQUOTA;
	/*
	 * Next, the cache between the in-core structures
	 * and the per-filesystem quota files on disk.
	 */
	for (i = NDQHASH; --i >= 0; dh++)
		dh->dqh_forw = dh->dqh_back = dh;
	dqfreel = dq;
	dqback = &dq->dq_freef;
	dq->dq_freeb = &dqfreel;
	dq->dq_forw = dq;
	dq->dq_back = dq;
	for (i = ndquot; --i > 0; ) {
		++dq;
		dq->dq_forw = dq;
		dq->dq_back = dq;
		*dqback = dq;
		dq->dq_freeb = dqback;
		dqback = &dq->dq_freef;
	}
	dq->dq_freef = NODQUOT;
	/*
	 * Zero out quota system active counters and flags.
	 */
	for (i = 0; i < NMOUNT; i++) {
		qactive[i].atv_flag = 0;
		qactive[i].atv_cnt = 0;
	}
}

/*
 * quota_test_lock returns true if the quota is locked.  If the quota
 * is already locked, we return false. This routine is called when
 * the lock request cannot sleep.
 */

quota_test_lock(q)
	register struct quota *q;
{
	if (q) {
		smp_lock(&lk_quotalocks, LK_RETRY);
		if (q->q_state & Q_LOCK) {
			return(0);
		}
		q->q_state |= Q_LOCK;
		smp_unlock(&lk_quotalocks);
	}
	return(1);
}

/*
 * dquot_test_lock returns true if the dquot is locked.  If the dquot
 * is already locked, we return false. This routine is called when
 * the lock request cannot sleep.
 */

dquot_test_lock(dq)
	register struct dquot *dq;
{
	if (dq) {
		smp_lock(&lk_dquotlocks, LK_RETRY);
		if (dq->dq_state & DQ_LOCK) {
			return(0);
		}
		dq->dq_state |= DQ_LOCK;
		smp_unlock(&lk_dquotlocks);
	}
	return(1);
}

/*
 * Find an incore quota structure for a particular uid,
 * or make one.  If lookuponly is non-zero, just the lookup is performed.
 * If nodq is non-zero, the dquot structures are left uninitialized.
 */
struct quota *
getquota(uid, lookuponly, nodq)
	register int uid;
	int lookuponly, nodq;
{
	register struct quota *q;
	register struct qhash *qh;
	register struct dquot **dqq;
	register struct mount *mp;
	register struct quota *qq;
	struct gnode *rgp;

	/*
	 * Fast check to see if an existing structure
	 * can be reused with just a reference count change.
	 */
	q = u.u_quota;
	if (q != NOQUOTA && q->q_uid == uid) {
		quota_lock(q);
		goto quick;
	}
	/*
	 * Search the quota chache for a hit.
	 */
top:
	smp_lock(&lk_quota, LK_RETRY);
	qh = &qhash[QHASH(uid)];
	for (q = (Qptr)qh->qh_forw; q != (Qptr)qh; q = q->q_forw) {
		if (!quota_test_lock(q)) {
			smp_unlock(&lk_quota);
			sleep_unlock((caddr_t)q, PINOD+1, &lk_quotalocks);
			goto top;
		}
		if (q->q_uid == uid) {
			if (q->q_cnt == 0) {
				if (lookuponly) {
					quota_unlock(q);
					smp_unlock(&lk_quota);
					return (NOQUOTA);
				}
				/*
				 * Take it off the free list.
				 */
				if ((qq = q->q_freef) != NOQUOTA)
					qq->q_freeb = q->q_freeb;
				else
					qfreetail = q->q_freeb;
				*q->q_freeb = qq;
				smp_unlock(&lk_quota);

				/*
				 * Recover any lost dquot structs.
				 */
				if (!nodq)
				for (dqq = q->q_dq, mp = mount;
				    dqq < &q->q_dq[NMOUNT]; dqq++, mp++) {
					rgp = (struct gnode *)fref(mp, NULL);
					if (*dqq == LOSTDQUOT && rgp) {
						*dqq = discquota(uid,
							      mp->m_qinod);
						if (*dqq != NODQUOT) {
							(*dqq)->dq_own = q;
							dquot_unlock((*dqq));
						}
					}
					if (rgp)
						grele(rgp);
				}
			} else
				smp_unlock(&lk_quota);
  quick:
			q->q_cnt++;
			if (q->q_cnt == 1)
				q->q_flags |= Q_NEW | nodq;
			return (q);
		} else
			quota_unlock(q);
	}
	if (lookuponly) {
		smp_unlock(&lk_quota);
		return (NOQUOTA);
	}
	/*
	 * Take the quota that is at the head of the free list
	 * (the longest unused quota).
	 */
	q = qfreelist;
	if (q == NOQUOTA) {
		tablefull("quota");
/*		printf("Login limit reached\n"); */
		u.u_error = EUSERS;
		q = quota;		/* the su's slot - we must have one */
		quota_lock(q);
		q->q_cnt++;
		smp_unlock(&lk_quota);
		return (q);
	}
	/*
	 * There is one - it is free no longer.
	 */
	qq = q->q_freef;
	if (qq != NOQUOTA)
		qq->q_freeb = &qfreelist;
	qfreelist = qq;
	/*
	 * Now we are about to change this from one user to another
	 * Must take this off hash chain for old user immediately, in
	 * case some other process claims it before we are done.
	 * We must then put it on the hash chain for the new user,
	 * to make sure that we don't make two quota structs for one uid.
	 * (the quota struct will then be locked till we are done).
	 */
	remque(q);
	quota_lock(q);
	q->q_uid = uid;
	q->q_flags = 0;
	q->q_cnt++;			/* q->q_cnt = 1; */
	insque(q, qh);
	smp_unlock(&lk_quota);
	/*
	 * Next, before filling in info for the new owning user,
	 * we must get rid of any dquot structs that we own.
	 */
	for (mp = mount, dqq = q->q_dq; mp < &mount[NMOUNT]; mp++, dqq++) {
		rgp = (struct gnode *)fref(mp, NULL);
		if (*dqq != NODQUOT && *dqq != LOSTDQUOT) {
			
			dquot_lock((*dqq));
			(*dqq)->dq_own = NOQUOTA;
			putdq(mp, *dqq, 1);
			dquot_unlock((*dqq));
		}
		if (!nodq && rgp) {
			*dqq = discquota(uid, mp->m_qinod);
			if (*dqq != NODQUOT) {
				if ((*dqq)->dq_uid != uid)
					panic("got bad quota uid");
				(*dqq)->dq_own = q;
				dquot_unlock((*dqq));
			}
		} else
			*dqq = NODQUOT;
		if (rgp)
			grele(rgp);
	}
	q->q_flags = Q_NEW | nodq;
	return (q);
}

/*
 * Delete a quota, wakeup anyone waiting.
 */
delquota(q)
	register struct quota *q;
{
	register struct dquot **dqq;
	register struct mount *mp;
	register struct gnode *rgp;

#ifdef SMP_DEBUG
	if ((q->q_state & Q_LOCK) == 0)
		panic("delquota: Quota should be locked");
#endif

	if (q->q_cnt != 1) {
		q->q_cnt--;
		return;
	}

	/*
	 * If we own dquot structs, sync them to disc, but don't release
	 * them - we might be recalled from the LRU chain.
	 * As we will sit on the free list while we are waiting for that,
	 * if dquot structs run out, ours will be taken away.
	 */
	if ((q->q_flags & Q_NDQ) == 0) {
		mp = mount;
		for (dqq = q->q_dq; dqq < &q->q_dq[NMOUNT]; dqq++, mp++) {
			rgp = (struct gnode *)fref(mp, NULL);
			if (rgp) {
				dquot_lock((*dqq));
				putdq(mp, *dqq, 0);
				dquot_unlock((*dqq));
				grele(rgp);
			}
		}
	}

	smp_lock(&lk_quota, LK_RETRY);

	q->q_cnt--;
	if (qfreelist != NOQUOTA) {
		*qfreetail = q;
		q->q_freeb = qfreetail;
	} else {
		qfreelist = q;
		q->q_freeb = &qfreelist;
	}
	q->q_freef = NOQUOTA;
	qfreetail = &q->q_freef;
	q->q_flags = 0;
	smp_unlock(&lk_quota); 
}

/*
 * Obtain the user's on-disk quota limit
 * from the file specified.
 */
struct dquot *
discquota(uid, gp)
	register struct gnode *gp;
{
	register struct dquot *dq;
	register struct dqhead *dh;
	register struct dquot *dp;
	register int fail;

#ifdef GFSDEBUG
	if(GFS[8])
		cprintf ("discquota: uid %d, gp 0x%x\n", uid, gp);
#endif
	if (gp == NULL)
		return (NODQUOT);
#ifdef SMP_DEBUG
	if (gp->g_count <= 0) {
		printf("discquota: gp 0x%x count %d \n", gp, gp->g_count);
		panic("discquota: gp needs to have a non zero ref count");
	}
	if (smp_owner(&lk_quota) || smp_owner(&lk_dquot))
		panic("discquota: cannot hold table locks");
#endif
	dq = dqalloc(uid, gp->g_dev);
	if (dq == NODQUOT)
		return (dq);
	if (dq->dq_flags & DQ_HASH) {
		dq->dq_flags &= ~DQ_HASH;
		/*
		 * We do this test after the reclaim so that
		 * the dquot will be moved to the end of the free
		 * list - frequently accessed ones ought to hang around.
		 */
		if (dq->dq_isoftlimit == 0 && dq->dq_bsoftlimit == 0) {
			dqrele(dq);
			dquot_unlock(dq);
			return (NODQUOT);
		}
		return(dq);
	}
	/*
         * Not in cache, initialize dq_dqb
         * by reading the entry in the quotas file for uid.
         */
	gfs_lock(gp);
	fail = rdwri(UIO_READ, gp, (caddr_t)&dq->dq_dqb, sizeof (struct dqblk),
	    uid * sizeof (struct dqblk), 1, (int *)0);
	gfs_unlock(gp);
	/*
	 * I/O error in reading quota file, release
	 * quota structure and reflect problem to caller.
	 */
	if (fail) {
		smp_lock(&lk_dquot, LK_RETRY);
		remque(dq);
		dq->dq_forw = dq;	/* on a private, unfindable hash list */
		dq->dq_back = dq;
		/* dqrele() (just below) will put dquot back on free list */
		smp_unlock(&lk_dquot);
	}
	/* no quota exists */
	if (fail || dq->dq_isoftlimit == 0 && dq->dq_bsoftlimit == 0) {
		dqrele(dq);
		dquot_unlock(dq);
		return (NODQUOT);
	}
	return (dq);
}

/*
 * Allocate a dquot structure.  If there are
 * no free slots in the cache, flush LRU entry from
 * the cache to the appropriate quota file on disk.
 */
struct dquot *
dqalloc(uid, dev)
	int uid;
	dev_t dev;
{
	register struct dquot *dq;
	register struct dqhead *dh;
	register struct dquot *dp;
	register struct quota *q;
	register struct mount *mp;
	static struct dqblk zdqb = { 0 };
	struct gnode *rgp;
	struct gnode *trgp;

#ifdef SMP_DEBUG
	if (smp_owner(&lk_quota) || smp_owner(&lk_dquot))
		panic("dqalloc: cannot hold table locks");
#endif
 	/*
	 * Locate inode of quotas file for
	 * indicated file system, return
	 * if file doesn't exist.
	 */
	GETMP(mp, dev);
	rgp = (struct gnode *)fref(mp, dev);
	if (mp->m_qinod == NULL) {
		if (rgp)
			grele(rgp);
		u.u_error = EINVAL;
		return (NODQUOT);
	}
	
	/*
	 * Check the cache first.
	 */
search:
	smp_lock(&lk_quota, LK_RETRY);
	smp_lock(&lk_dquot, LK_RETRY);
	dh = &dqhead[DQHASH(uid, dev)];
	for (dq = (DQptr)dh->dqh_forw; dq != (DQptr)dh; dq = dq->dq_forw) {
		if (!dquot_test_lock(dq)) {
			smp_unlock(&lk_dquot);
			smp_unlock(&lk_quota);
			sleep_unlock((caddr_t)dq, PINOD+1, &lk_dquotlocks);
			goto search;
		}
		if (dq->dq_uid != uid || dq->dq_dev != dev) {
			dquot_unlock(dq);
			continue;
		}
		/*
		 * Cache hit with no references.  Take
		 * the structure off the free list.
		 */
		if (dq->dq_cnt++ == 0) {
			dp = dq->dq_freef;
			if (dp != NODQUOT)
				dp->dq_freeb = dq->dq_freeb;
			else
				dqback = dq->dq_freeb;
			*dq->dq_freeb = dp;
			dq->dq_own = NOQUOTA;
		}
		smp_unlock(&lk_quota);
		smp_unlock(&lk_dquot);
		dq->dq_flags |= DQ_HASH;
		if (rgp)
			grele(rgp);
		return (dq);
	}
	/*
	 * Check free list.  If table is full, pull entries
	 * off the quota free list and flush any associated
	 * dquot references until something frees up on the
	 * dquot free list.
	 */
	if ((dq = dqfreel) == NODQUOT && (q = qfreelist) != NOQUOTA) {

		do {
			register struct dquot **dqq;
			register struct mount *mountp = mount;

			dqq = q->q_dq;
			quota_lock(q);
			while (dqq < &q->q_dq[NMOUNT]) {
				trgp = (struct gnode *)fref(mountp, NULL);
				if (!dquot_test_lock((*dqq))) {
					smp_unlock(&lk_dquotlocks);
					mountp++;
					dqq++;
					if (trgp)
						grele(trgp);
					continue;
				}
				if ((dq = *dqq) != NODQUOT &&
				    dq != LOSTDQUOT) {
					/*
					 * Mark entry as "lost" due to
					 * scavenging operation.
					 */
					if (dq->dq_cnt == 1) {
						*dqq = LOSTDQUOT;
						smp_unlock(&lk_quota);
						smp_unlock(&lk_dquot);
						putdq(mountp, dq, 1);
						dquot_unlock(dq);
						quota_unlock(q);
						if (trgp)
							grele(trgp);
						goto search;
					}
				}
				mountp++;
				dquot_unlock((*dqq));
				dqq++;
				if (trgp)
					grele(trgp);
			}
			quota_unlock(q);
			q = q->q_freef;
		} while ((dq = dqfreel) == NODQUOT && q != NOQUOTA);
	}
	smp_unlock(&lk_quota);
	if (dq == NODQUOT) {
		tablefull("dquot");
		u.u_error = EUSERS;
		smp_unlock(&lk_dquot);
		if (rgp)
			grele(rgp);
		return (dq);
	}
	/*
	 * This shouldn't happen, as we sync
	 * dquot before freeing it up.
	 */
	if (dq->dq_flags & DQ_MOD)
		panic("discquota");

	/*
	 * Now take the dquot off the free list,
	 */
	dp = dq->dq_freef;
	if (dp != NODQUOT)
		dp->dq_freeb = &dqfreel;
	dqfreel = dp;
	/*
	 * and off the hash chain it was on, & onto the new one.
	 */
	dh = &dqhead[DQHASH(uid, dev)];
	remque(dq);
	dquot_lock(dq);
	dq->dq_cnt = 1;
	dq->dq_flags = 0;
	dq->dq_uid = uid;
	dq->dq_dev = dev;
	dq->dq_dqb = zdqb;
	dq->dq_own = NOQUOTA;
	insque(dq, dh);
	smp_unlock(&lk_dquot);
	if (rgp)
		grele(rgp);
	return (dq);
}

/*
 * dqrele - layman's interface to putdq.
 */
dqrele(dq)
	register struct dquot *dq;
{
	register struct mount *mp;
	register struct gnode *rgp;

	if (dq == NODQUOT || dq == LOSTDQUOT)
		return;

#ifdef SMP_DEBUG
	if ((dq->dq_state & DQ_LOCK) == 0)
		panic("dqrele: Dquot should be locked");
#endif

	if (dq->dq_cnt > 1) {
		dq->dq_cnt--;
		return;
	}
	/*
	 * I/O required, find appropriate file system
	 * to sync the quota information to.
	 */
	GETMP(mp, dq->dq_dev);
	rgp = (struct gnode *)fref(mp, NULL);
	putdq(mp, dq, 1);
	if (rgp)
		grele(rgp);
}

/*
 * Update the disc quota in the quota file.
 */
putdq(mp, dq, free)
	register struct mount *mp;
	register struct dquot *dq;
{
	register struct gnode *gp;

	if (dq == NODQUOT || dq == LOSTDQUOT)
		return;

#ifdef SMP_DEBUG
	if ((dq->dq_state & DQ_LOCK) == 0) 
		panic("putdq: dquot must be locked");
	if (smp_owner(&lk_quota) || smp_owner(&lk_dquot))
		panic("putdq: cannot hold table locks");
#endif

	if (free && dq->dq_cnt > 1) {
		dq->dq_cnt--;
		return;
	}
	/*
	 * Disk quota not modified, just discard
	 * or return (having adjusted the reference
	 * count), as indicated by the "free" param.
	 */
	if ((dq->dq_flags & DQ_MOD) == 0) {
		if (free) {
			dq->dq_cnt = 0;
 release:
			smp_lock(&lk_dquot, LK_RETRY);
			if (dqfreel != NODQUOT) {
				*dqback = dq;
				dq->dq_freeb = dqback;
			} else {
				dqfreel = dq;
				dq->dq_freeb = &dqfreel;
			}
			dq->dq_freef = NODQUOT;
			dqback = &dq->dq_freef;
			smp_unlock(&lk_dquot);
		}
		return;
	}
	/*
	 * Quota modified, write back to disk.
	 */
	if ((gp = mp->m_qinod) == NULL)
		panic("lost quota file");
	gfs_lock(gp);
	(void) rdwri(UIO_WRITE, gp, (caddr_t)&dq->dq_dqb, sizeof (struct dqblk),
	    dq->dq_uid * sizeof (struct dqblk), 1, (int *)0);
	gfs_unlock(gp);
	dq->dq_flags &= ~DQ_MOD;
	if (free && --dq->dq_cnt == 0)
		goto release;
}

/*
 * See if there is a quota struct in core for user 'uid'.
 */
struct quota *
qfind(uid)
	register int uid;
{
	register struct quota *q;
	register struct qhash *qh;

#ifdef SMP_DEBUG
	if (smp_owner(&lk_quota))
		panic("qfind: Cannot hold quota table lock");
#endif
	/* 
	 * Check common cases first: asking for own quota,
	 * or that of the super user (has reserved slot 0
	 * in the table).
	 */
	q = u.u_quota;
	if (q != NOQUOTA && q->q_uid == uid) {
		quota_lock(q);
		return (q);
	}
	if (uid == 0) {		/* the second most likely case */
		quota_lock(quota);
		return (quota);
	}
	/*
	 * Search cache.
	 */
top:
	smp_lock(&lk_quota, LK_RETRY);
	qh = &qhash[QHASH(uid)];
	for (q = (Qptr)qh->qh_forw; q != (Qptr)qh; q = q->q_forw) {
		if (!quota_test_lock(q)) {
			smp_unlock(&lk_quota);
			sleep_unlock((caddr_t)q, PINOD+1, &lk_quotalocks);
			goto top;
		}
		if (q->q_uid == uid) {
			smp_unlock(&lk_quota);
			return (q);
		}
		quota_unlock(q);
	}
	smp_unlock(&lk_quota);
	return (NOQUOTA);
}

/*
 * Set the quota file up for a particular file system.
 * Called as the result of a setquota system call.
 */
opendq(mp, fname)
	register struct mount *mp;
	caddr_t fname;
{
	register struct gnode *gp;
	register struct quota *q;
 	register struct nameidata *ndp = &u.u_nd;
	register struct dquot *dq;
	register int i = mp - mount;

again:
	smp_lock(&lk_gnode, LK_RETRY);
	if (qactive[i].atv_cnt) {
		sleep_unlock((caddr_t)qactive[i].atv_cnt, 
		     PINOD+1, &lk_gnode);
		goto again;
	}
	if (qactive[i].atv_flag & QMP_UNSAFE) {
		sleep_unlock((caddr_t)qactive[i].atv_flag, 
		     PINOD+1, &lk_gnode);
		goto again;
	}
	qactive[i].atv_flag |= QMP_UNSAFE;
	smp_unlock(&lk_gnode);

	if (mp->m_qinod)
		closedq(mp, 1);
 	ndp->ni_nameiop = LOOKUP | FOLLOW;
	ndp->ni_dirp = fname;

 	gp = gfs_namei(ndp);
	
	if (gp == NULL)
		goto out;
	gfs_unlock(gp);
	if (gp->g_dev != mp->m_dev) {
		u.u_error = EACCES;
		goto out;
	}
	if ((gp->g_mode & GFMT) != GFREG) {
		u.u_error = EACCES;
		goto out;
	}
	/*
	 * Flush in-core references to any previous
	 * quota file for this file system.
	 */
	mp->m_qinod = gp;
	i = mp - mount;
	for (q = quota; q < quotaNQUOTA; q++) {
		quota_lock(q);
		if ((q->q_flags & Q_NDQ) == 0) {
			if (q->q_cnt == 0)
				q->q_dq[i] = LOSTDQUOT;
			else {
				q->q_cnt++;	/* cannot be released */
				dq = discquota(q->q_uid, gp);
				q->q_dq[i] = dq;
				if (dq != NODQUOT) {
					dq->dq_own = q;
					dquot_unlock(dq);
				}
				delquota(q);
			}
		}
		quota_unlock(q);
	}
out:
	smp_lock(&lk_gnode, LK_RETRY);
	qactive[i].atv_flag = qactive[i].atv_cnt = 0;
	smp_unlock(&lk_gnode);
	wakeup(qactive[i].atv_flag);
	wakeup(qactive[i].atv_cnt);
}

/*
 * Close off disc quotas for a file system.
 */
closedq(mp, bypass)
	register struct mount *mp;
	int bypass;
{
	register struct dquot *dq;
	register int i = mp - mount;
	register struct quota *q;
	register struct gnode *gp;

	if (mp->m_qinod == NULL)
		return;
try:
	if (bypass == 0) {
		smp_lock(&lk_gnode, LK_RETRY);
		if (qactive[i].atv_cnt) {
			sleep_unlock((caddr_t)qactive[i].atv_cnt, 
				     PINOD+1, &lk_gnode);
			goto try;
		}
		if (qactive[i].atv_flag & QMP_UNSAFE) {
			sleep_unlock((caddr_t)qactive[i].atv_flag, 
				     PINOD+1, &lk_gnode);
			goto try;
		}
		qactive[i].atv_flag |= QMP_UNSAFE;
		smp_unlock(&lk_gnode);
	}

	
	/*
	 * Search inode table, delete any references
	 * to quota file being closed.
	 */
       
top:
	smp_lock(&lk_gnode, LK_RETRY);
	for (gp = gnode; gp < gnodeNGNODE; gp++) {
		if (gp->g_dev == mp->m_dev) {
			if (gp->g_dquot == NODQUOT)
				continue;
			dq = gp->g_dquot;
			gp->g_dquot = NODQUOT;
			smp_unlock(&lk_gnode);
			dquot_lock(dq);
			putdq(mp, dq, 1);
			dquot_unlock(dq);
			goto top;
		}
	}
	smp_unlock(&lk_gnode);
	/*
	 * Search quota table, flush any pending
	 * quota info to disk and also delete
	 * references to closing quota file.
	 */
again:
	smp_lock(&lk_quota, LK_RETRY);
	for (q = quota; q < quotaNQUOTA; q++) {
		if (!quota_test_lock(q)) {
			smp_unlock(&lk_quota);
			sleep_unlock((caddr_t)q, PINOD+1, &lk_quotalocks);
			goto again;
		}
		if (q->q_dq[i] == NODQUOT) {
			quota_unlock(q);
			continue;
		}
		if ((q->q_flags & Q_NDQ) == 0) {
			if (q->q_cnt) {
				q->q_cnt++;
				smp_unlock(&lk_quota);
				dquot_lock(q->q_dq[i]);
				putdq(mp, q->q_dq[i], 1);
				dquot_unlock(q->q_dq[i]);
				delquota(q);
			} else {
				smp_unlock(&lk_quota);
				dquot_lock(q->q_dq[i]);
				putdq(mp, q->q_dq[i], 1);
				dquot_unlock(q->q_dq[i]);
			}
		}
		q->q_dq[i] = NODQUOT;
		quota_unlock(q);
		goto again;
	}
	smp_unlock(&lk_quota);
	/*
	 * Move all dquot's that used to refer to this quota
	 * file of into the never-never (they will eventually
	 * fall off the head of the free list and be re-used).
	 */
	smp_lock(&lk_dquot, LK_RETRY);
	for (dq = dquot; dq < dquotNDQUOT; dq++) {
		if (dq->dq_dev == mp->m_dev) {
			if (!dquot_test_lock(dq))
				panic("closedq: dq should not be locked");
			if (dq->dq_cnt)
				panic("closedq: stray dquot");
			remque(dq);
			dq->dq_forw = dq;
			dq->dq_back = dq;
			dq->dq_dev = NODEV;
			dquot_unlock(dq);
		}
	}
	smp_unlock(&lk_dquot);
	grele(mp->m_qinod);
	mp->m_qinod = NULL;
	if (bypass)	/* We were called by opendq */
		return;
	smp_lock(&lk_gnode, LK_RETRY);
	qactive[i].atv_flag = qactive[i].atv_cnt = 0;
	smp_unlock(&lk_gnode);
	wakeup((caddr_t)qactive[i].atv_flag);
	wakeup((caddr_t)qactive[i].atv_cnt);
}
#endif
