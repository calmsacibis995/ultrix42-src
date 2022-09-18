/*	@(#)quota.h	4.1	ULTRIX	7/2/90 */

/************************************************************************
 *									*
 *			Copyright (c) 1989 by				*
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
 * Modification History 
 *
 * 14-Jun-89 - Giles Atkinson
 *	Removed commands for pre-LMF login limit scheme.
 *
 * 06-Apr-89 - prs
 *	Added SMP quota.
 *
 ************************************************************************/

#ifdef	KERNEL
#include "../h/smp_lock.h"
#else /*	KERNEL */
#include <sys/smp_lock.h>
#endif /*	KERNEL */

/*
 * MELBOURNE DISC QUOTAS
 *
 * Various junk to do with various quotas (etc) imposed upon
 * the average user (big brother finally hits UNIX).
 *
 * The following structure exists in core for each logged on user.
 * It contains global junk relevant to that user's quotas.
 *
 * The u_quota field of each user struct contains a pointer to
 * the quota struct relevant to the current process, this is changed
 * by 'setuid' sys call, &/or by the Q_SETUID quota() call.
 */
struct quota {
	struct	quota *q_forw, *q_back;	/* hash chain, MUST be first */
	short	q_cnt;			/* ref count (# processes) */
	short	q_uid;			/* real uid of owner */
	int	q_flags;		/* struct management flags */
	int	q_state;		/* Q_LOCK */
#define	Q_LOCK	0x01		/* quota struct locked (for disc i/o) */
#define	Q_WANT	0x02		/* issue a wakeup when lock goes off */
#define	Q_NEW	0x04		/* new quota - no proc1 msg sent yet */
#define	Q_NDQ	0x08		/* account has NO disc quota */
	struct	quota *q_freef, **q_freeb;
	struct	dquot *q_dq[NMOUNT];	/* disc quotas for mounted filesys's */
};

#ifdef KERNEL
/*
 * Global quota structure lock, used for locking free and hashed linked
 * lists of quota and dquot structures.
 */
extern struct lock_t lk_quota;
extern struct lock_t lk_dquot;
extern struct lock_t lk_quotalocks;
extern struct lock_t lk_dquotlocks;

/*
 * quota_lock and quota_unlock defines were added to lock and unlock
 * quota structures. The old mechanism didn't lock quota structures
 * after "determining" they were unlocked. It was assumed that noone
 * else could be running at the same time to clobber anything. However,
 * this is no longer the case. We will use the quota_lock and quota_unlock
 * macros to aquire a smp sleep lock when freeing and allocating quota
 * structures.
 */

#define quota_lock(q) {	\
		if (q != NOQUOTA) { \
	              smp_lock(&lk_quotalocks, LK_RETRY); \
	              while (q->q_state & Q_LOCK) { \
                           sleep_unlock((caddr_t)q, PINOD+1, &lk_quotalocks); \
		           smp_lock(&lk_quotalocks, LK_RETRY); \
	              } \
		      q->q_state |= Q_LOCK; \
		      smp_unlock(&lk_quotalocks); \
		} \
	}

#define quota_unlock(q) { \
		if (q != NOQUOTA) { \
		      smp_lock(&lk_quotalocks, LK_RETRY); \
		      q->q_state &= ~Q_LOCK; \
	              smp_unlock(&lk_quotalocks); \
		      wakeup((caddr_t)q); \
		} \
	}

#define dquot_lock(dq) { \
	        if ((dq != LOSTDQUOT) && (dq != NODQUOT)) { \
		        smp_lock(&lk_dquotlocks, LK_RETRY); \
		        while (dq->dq_state & DQ_LOCK) { \
                	   sleep_unlock((caddr_t)dq, PINOD+1, &lk_dquotlocks); \
			   smp_lock(&lk_dquotlocks, LK_RETRY); \
		        } \
			dq->dq_state |= DQ_LOCK; \
			smp_unlock(&lk_dquotlocks); \
	        } \
	}

#define dquot_unlock(dq) { \
	        if ((dq != LOSTDQUOT) && (dq != NODQUOT)) { \
			smp_lock(&lk_dquotlocks, LK_RETRY); \
			dq->dq_state &= ~DQ_LOCK; \
		        smp_unlock(&lk_dquotlocks); \
			wakeup((caddr_t)dq); \
	        } \
	}

struct quota_active {
	int atv_flag;
#define	QMP_UNSAFE	0x1b366
	int atv_cnt;
};
#endif /* KERNEL */
#define	NOQUOTA	((struct quota *) 0)

#if defined(KERNEL) && defined(QUOTA)
struct	quota *quota, *quotaNQUOTA;
int	nquota;
struct	quota *getquota(), *qfind();
#endif

/*
 * The following structure defines the format of the disc quota file
 * (as it appears on disc) - the file is an array of these structures
 * indexed by user number.  The setquota sys call establishes the inode
 * for each quota file (a pointer is retained in the mount structure).
 *
 * The following constants define the number of warnings given a user
 * before the soft limits are treated as hard limits (usually resulting
 * in an allocation failure).  The warnings are normally manipulated
 * each time a user logs in through the Q_DOWARN quota call.  If
 * the user logs in and is under the soft limit the warning count
 * is reset to MAX_*_WARN, otherwise a message is printed and the
 * warning count is decremented.  This makes MAX_*_WARN equivalent to
 * the number of logins before soft limits are treated as hard limits.
 */
#define	MAX_IQ_WARN	3
#define	MAX_DQ_WARN	3

struct	dqblk {
	u_long	dqb_bhardlimit;	/* absolute limit on disc blks alloc */
	u_long	dqb_bsoftlimit;	/* preferred limit on disc blks */
	u_long	dqb_curblocks;	/* current block count */
	u_short	dqb_ihardlimit;	/* maximum # allocated inodes + 1 */
	u_short	dqb_isoftlimit;	/* preferred inode limit */
	u_short	dqb_curinodes;	/* current # allocated inodes */
	u_char	dqb_bwarn;	/* # warnings left about excessive disc use */
	u_char	dqb_iwarn;	/* # warnings left about excessive inodes */
};

/*
 * The following structure records disc usage for a user on a filesystem.
 * There is one allocated for each quota that exists on any filesystem
 * for the current user. A cache is kept of other recently used entries.
 */
struct	dquot {
	struct	dquot *dq_forw, *dq_back;/* MUST be first entry */
	union	{
		struct	quota *Dq_own;	/* the quota that points to this */
		struct {		/* free list */
			struct	dquot *Dq_freef, **Dq_freeb;
		} dq_f;
	} dq_u;
	int	dq_state;		/* DQ_LOCK */
#define	DQ_LOCK		0x01		/* this quota locked (no MODS) */
	short	dq_flags;

#define DQ_HASH		0x01		/* Found on hash chain */
#define	DQ_WANT		0x02		/* wakeup on unlock */
#define	DQ_MOD		0x04		/* this quota modified since read */
#define	DQ_FAKE		0x08		/* no limits here, just usage */
#define	DQ_BLKS		0x10		/* has been warned about blk limit */
#define	DQ_INODS	0x20		/* has been warned about inode limit */
	short	dq_cnt;			/* count of active references */
	short	dq_uid;			/* user this applies to */
	dev_t	dq_dev;			/* filesystem this relates to */
	struct dqblk dq_dqb;		/* actual usage & quotas */
};

#define	dq_own		dq_u.Dq_own
#define	dq_freef	dq_u.dq_f.Dq_freef
#define	dq_freeb	dq_u.dq_f.Dq_freeb
#define	dq_bhardlimit	dq_dqb.dqb_bhardlimit
#define	dq_bsoftlimit	dq_dqb.dqb_bsoftlimit
#define	dq_curblocks	dq_dqb.dqb_curblocks
#define	dq_ihardlimit	dq_dqb.dqb_ihardlimit
#define	dq_isoftlimit	dq_dqb.dqb_isoftlimit
#define	dq_curinodes	dq_dqb.dqb_curinodes
#define	dq_bwarn	dq_dqb.dqb_bwarn
#define	dq_iwarn	dq_dqb.dqb_iwarn

#define	NODQUOT		((struct dquot *) 0)
#define	LOSTDQUOT	((struct dquot *) 1)

#if defined(KERNEL) && defined(QUOTA)
struct	dquot *dquot, *dquotNDQUOT;
int	ndquot;
struct	dquot *discquota(), *inoquota(), *dqalloc(), *dqp();
#endif

/*
 * Definitions for the 'quota' system call.
 */
#define	Q_SETDLIM	1	/* set disc limits & usage */
#define	Q_GETDLIM	2	/* get disc limits & usage */
#define	Q_SETDUSE	3	/* set disc usage only */
#define	Q_SYNC		4	/* update disc copy of quota usages */
#define	Q_SETUID	16	/* change proc to use quotas for uid */
#define	Q_SETWARN	25	/* alter inode/block warning counts */
#define	Q_DOWARN	26	/* warn user about excessive space/inodes */

/*
 * Used in Q_SETDUSE.
 */
struct	dqusage {
	u_short	du_curinodes;
	u_long	du_curblocks;
};

/*
 * Used in Q_SETWARN.
 */
struct	dqwarn {
	u_char	dw_bwarn;
	u_char	dw_iwarn;
};
