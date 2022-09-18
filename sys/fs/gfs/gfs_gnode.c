#ifndef lint
static	char	*sccsid = "@(#)gfs_gnode.c	4.2	(ULTRIX)	11/9/90";
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


/***********************************************************************
 *
 *		Modification History
 *
 * 10 May 90 -- prs for cb
 *	Added wantid argument to ggrab(). ufs_namei() calls ggrab with
 *	the expected g_id, so ggrab() can perform an advisory check
 *	before locking gnode.
 *
 *  2 Feb 90 -- chet
 *	Change getegnode to use cacheinval()
 *
 * 12-Jan-90 -- prs
 *	Changed km_free to use KM_FREE macro to keep malloc
 *	counters in sync with actual number of free's.
 * 
 * 09-Nov-89 -- jaw
 *	remove smp_owner check on gnode lock.
 *
 * 24 Jul 89 -- prs
 *	Added unggrab() primitive.
 *
 * 06 Apr 89 -- prs
 *	Added SMP quota locks. Cleaned up gflush().
 *
 * 9 Mar 89 -- chet
 *	Put code in gget() to ask NFS dnlc to give up gnode
 *	references when out of free gnodes.
 *
 * 05 jan 89 -- condylis
 *	Added check for state of mount table entry to ggrab.
 *	Restructured gflush for SMP umount, mount and sync.  Added fref
 *	primitive.
 *
 * 06 Sep 88 -- prs
 *      SMP - Changed grele() to dealloc space associated with a
 *      named pipe since fifo_rele() is not called.
 *
 * 19 May 88 -- cb 
 *      Added SMP lock mechanisms.  Changed GFS interface.
 *
 * 11 Sep 86 -- koehler 
 *	changed gflush
 *
 * 16 Oct 86 -- koehler
 *	ggrab needs to make sure that the last free gnode is not being
 *	consumed.
 *
 ***********************************************************************/


#include "../h/param.h"
#include "../h/systm.h"
#include "../h/mount.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/gnode.h"
#include "../h/conf.h"
#include "../h/buf.h"
#ifdef QUOTA
#include "../h/quota.h"
#include "../h/kmalloc.h"
#endif
#include "../h/kernel.h"

union ghead ghead[GNOHSZ];
 
struct gnode *gfreeh, **gfreet;
struct lock_t lk_gnode;

struct gstats {
        int greuse;
        int greclaims;
        int gfound;
        int grefs;
        int greles;
        int gfrees;
};

struct gstats gstats;

/*
 * Initialize hash links for gnodes
 * and build gnode free list.
 */
ghinit()
{
	register int i;
	register struct gnode *gp = gnode;
	register union  ghead *gh = ghead;


	for (i = GNOHSZ; --i >= 0; gh++) {
		gh->gh_head[0] = gh;
		gh->gh_head[1] = gh;
	}
	gfreeh = gp;
	gfreet = &gp->g_freef;
	gp->g_freeb = &gfreeh;
	gp->g_forw = gp;
	gp->g_back = gp;
	lockinit(&gp->g_lk, &lock_eachgnode_d);
	for (i = ngnode; --i > 0; ) {
		++gp;
		gp->g_forw = gp;
		gp->g_back = gp;
		*gfreet = gp;
		gp->g_freeb = gfreet;
		gfreet = &gp->g_freef;
		lockinit(&gp->g_lk, &lock_eachgnode_d);
	}
	gp->g_freef = NULL;
	gstats.greuse = 0;
	gstats.greclaims = 0;
	gstats.gfound = 0;
	gstats.grefs = 0;
	gstats.greles = 0;
	gstats.gfrees = 0;
	finit();   /* initialize the file table */
}

/*
 * Convert a pointer to an gnode into a reference to an gnode.
 *
 * This is basically the internal piece of gget (after the
 * gnode pointer is located) but without the test for mounted
 * filesystems.  It is caller's responsibility to check that
 * the gnode pointer is valid. ufs_namei() is currently the
 * only invoker. Please note the unggrab() must be used
 * to deallocate the gnode if the gnode pointer is not valid.
 */
struct gnode *
ggrab(gp, wantid)
        register struct gnode *gp;
	int wantid;
{
        caddr_t ptr = (caddr_t) NULL;    /* XXX */

	smp_lock(&lk_gnode, LK_RETRY);

	/*
	 * Has the file system been unmounted
	 */
	if (!(gp->g_mp->m_flgs & MTE_DONE)) {
		smp_unlock(&lk_gnode);
		return(NULL);
	}

	/*
	 * get an advisory opinion, that's binding if
	 * it says the gnode has been reused, the
	 * caller must check again (while holding 
	 * the lock) if it looks like the right gnode.
	 */	

	if (gp->g_id != wantid) {
		smp_unlock(&lk_gnode);
		return(NULL);
	}

	gp->g_count++;

	if (gp->g_count != 1) 
	        gstats.grefs++;
	else {
		gstats.greclaims++;
		gremque(gp);
		if (gisready(gp))
		        panic("ggrab: active unreferenced gnode");
	}
	smp_unlock(&lk_gnode);

	gfs_lock(gp);
	if (!gisready(gp)) {
		if (!ginitialize(gp, ptr)) {
	        	gclobber(gp);
			gfs_unlock(gp);
			grele(gp);
			return(NULL);
		}
	}
	return(gp);
}

/*
 * ufs_namei() needs to un ggrab a gnode. The ggrab() routine
 * pays no attention to nlink. The case where nlink is equal to
 * zero, and we hold the only reference, we must set the g_init
 * field to the reclaim state, or the sfs inactive routine
 * will attempt to deallocate a previously deallocated gnode.
 * All other cases will go through proper channels.
 */
unggrab(gp)
	register struct gnode *gp;
{
	gassert(gp);
	if ((gp->g_count == 1) && (gp->g_nlink == 0))
		gp->g_init = RECLAIM_GNODE;
	gfs_unlock(gp);
	grele(gp);
}


/*
 * Decrement reference count of
 * an gnode structure.
 * On the last reference,
 * write the gnode out and if necessary,
 * truncate and deallocate the file.
 */
gput(gp)
	register struct gnode *gp;
{
	gactive(gp);
	if(gp->g_count == 1 && (gp->g_mode& GFMT)  != GFPIPE) {
	        GINACTIVE(gp);
		gp->g_init = RECLAIM_GNODE;
	}
	
	gfs_unlock(gp);

	grele(gp);
}

/*
 * remove any gnodes in the gnode cache belonging to dev
 *
 * There should not be any active ones, return error if any are found
 * (nb: this is a user error, not a system err)
 *
 * Also, count the references to dev by block devices - this really
 * has nothing to do with the object of the procedure, but as we have
 * to scan the gnode table here anyway, we might as well get the
 * extra benefit.
 *
 */
#ifdef QUOTA
gflush(dev, gq, mgp)
	dev_t dev;
	register struct gnode *gq;
	struct gnode *mgp;
#else
gflush(dev, mgp)
	dev_t dev;
	struct gnode *mgp;
#endif
{
	register struct gnode *gp;
	register int open = 0;
#ifdef QUOTA
	register struct dquot *dq;
	struct dquot **dqq;
	struct dquot *dquot_array;

	KM_ALLOC(dquot_array, struct dquot *, (ngnode + 1) * 4, 
		 KM_TEMP, KM_CLEAR);
	dqq = (struct dquot **)dquot_array;
#endif

smp_lock(&lk_gnode, LK_RETRY);

	for (gp = gnode; gp < gnodeNGNODE; gp++) {
#ifdef QUOTA
		if (gp != gq && gp->g_dev == dev && gp != gp->g_mp->m_gnodp) {
#else
		if ((gp->g_dev == dev) && (gp != gp->g_mp->m_gnodp)) {
#endif
			if((gp == mgp) && (mgp->g_count == 1))
				continue;
			if (gp->g_count) {
				smp_unlock(&lk_gnode);
				open = -1;
				goto out;
			}
			else {
				remque(gp);
				gp->g_forw = gp;
				gp->g_back = gp;
				/*
				 * as g_count == 0, the gnode was on the free
				 * list already, just leave it there, it will
				 * fall off the bottom eventually. We could
				 * perhaps move it to the head of the free
				 * list, but as umounts are done so
				 * infrequently, we would gain very little,
				 * while making the code bigger.
				 */
#ifdef QUOTA
				if (gp->g_dquot != NODQUOT) {
					*dqq = gp->g_dquot;
					dqq++;
				}
				gp->g_dquot = NODQUOT;
#endif
			}
		}
		else if (gp->g_count && (gp->g_mode&GFMT)==GFBLK &&
		   gp->g_rdev == dev) /* hack? */
			open++;
	}

	if (mgp != NULL) {
		smp_lock(&mgp->g_mp->m_lk, LK_RETRY);
		mgp->g_mp->m_flgs &= ~MTE_DONE;	/* mount table entry is being 
					/* unmounted.  (Can't get a ref
					/* on gnode in file system and
					/* mount table entry cannot be
					/* reclaimed)
					 */
		smp_unlock(&mgp->g_mp->m_lk);
	}
	smp_unlock(&lk_gnode);

out:
#ifdef QUOTA
	/* Now free up quota related stuff */

	for (dqq = (struct dquot **)dquot_array; *dqq; dqq++) {
		dquot_lock((*dqq));
		dqrele((*dqq));
		dquot_unlock((*dqq));
	}
	KM_FREE(dquot_array, KM_TEMP);
#endif

	return (open);
}

int getegnode_ufs, getegnode_nfs;
int getegnode_purge_worked, getegnode_purge1_worked;


/*
 * Look up a gnode by device, number. If it is in core (in the gnode
 * table), return it locked. If it is not in core, get an empty gnode
 * and call a filesystem-specific routine to initialize it.  If the gnode
 * is mounted on, jump over the mount point.  In all cases a pointer to
 * a locked gnode is returned.
 */
struct gnode *
gget(mp, gno, nocross, ptr)
	register struct mount *mp;
	register gno_t gno;
	int nocross;
	caddr_t ptr;
{
	register struct gnode *gp = (struct gnode *)0;
	register union  ghead *gh;
	struct gnode *rootgp;
	register dev_t dev = mp->m_dev;
	int	purge_state = 0;



start:
	gh = &ghead[GNOHASH(mp->m_dev, gno)];
	smp_lock(&lk_gnode, LK_RETRY);
	for (gp = gh->gh_chain[0]; gp != (struct gnode *)gh; gp = gp->g_forw) {
		if (gno == gp->g_number && dev == gp->g_dev && GMATCH(gp, ptr)) {
			gp->g_count++;
			if ((gp->g_flag&GMOUNT) && (nocross == 0)) {
				mp = gp->g_mpp;
				rootgp = mp->m_rootgp;
				gp->g_count--;
				rootgp->g_count++;
				smp_unlock(&lk_gnode);
				gfs_lock(rootgp);
				return(rootgp);				
			}
			if (gp->g_count == 1) {
				gremque(gp);
			}
			smp_unlock(&lk_gnode);

			gfs_lock(gp);

			if (gisready(gp))
				gstats.gfound++;
			else {
				gstats.greclaims++;
				gp->g_mp = mp;
				if (!ginitialize(gp, ptr)) {
					gclobber(gp);
					gfs_unlock(gp);
					grele(gp);
					return(NULL);
				}
			}

/*	XXX		gp->g_blocks = G_TO_I(gp)->di_blocks;		XXX */
			return(gp);
		}
	}
	/*
	 * Get a free gnode.
	 * If there aren't any, then ask NFS to give one up from
	 * its name lookup cache.
	 * If that doesn't work, ask NFS to free all of its lookup
	 * cache gnode references.
	 * If that doesn't work, bail out.
	 */
	if((gp = getegnode(GNOHASH(dev, gno), mp, gno)) == NULL) {
		smp_unlock(&lk_gnode);
		switch (purge_state) {
			case 0:
				if (major(dev) < nblkdev + nchrdev)
					getegnode_ufs++;
				else
					getegnode_nfs++;
				dnlc_purge1();
				purge_state++;
				goto start;
				break;
			case 1:
				dnlc_purge();
				purge_state++;
				goto start;
				break;
			default:
				tablefull("gnode");
				u.u_error = ENFILE;
				return(NULL);
				break;
		}
	} else if (purge_state) {
		switch (purge_state) {
			case 1:
				getegnode_purge1_worked++;
				break;
			default:
				getegnode_purge_worked++;
				break;
		}
	}
	gp->g_count++;
	gstats.greuse++;
	smp_unlock(&lk_gnode);
	gfs_lock(gp);

#ifdef QUOTA
	dquot_lock(gp->g_dquot);
	dqrele(gp->g_dquot);
	dquot_unlock(gp->g_dquot);
#endif
	if (!gisready(gp)) {
		if (!ginitialize(gp, ptr)) {
			gclobber(gp);
			gfs_unlock(gp);
			grele(gp);
			return(NULL);
		}
	}

#ifdef QUOTA
	if (gp->g_mode == 0)
		gp->g_dquot = NODQUOT;
	else 
		gp->g_dquot = inoquota(gp);
#endif

	return (gp);
}


/*
int
gfs_lock(gp)
	register struct gnode *gp;
{
	if (gp->g_count < 1)
	        panic("gfs_lock: locking unrefed gnode");
	smp_lock(&gp->g_lk, LK_RETRY);
}


int
gfs_unlock(gp)
	register struct gnode *gp;
{
	if (gp->g_count < 1)
	        panic("gfs_unlock: unlocking unrefed gnode");

	if (!glocked(gp))
	        panic("gfs_unlock: unlocking unlocked gnode");

	smp_unlock(&gp->g_lk);
}
*/

/*
 * Pull an empty gnode off of the free list and prepare to reuse it for
 * another file.  Link it onto its new hash chain.  Caller must hold
 * the gnode table spin lock.
 */
struct gnode *
getegnode(hash, mp, gno)
	register int	hash;
	struct	 mount  *mp;
	register gno_t	gno;
{
	register struct gnode *gp;
	register struct gnode *gq;
	register struct gnode *xp;
	dev_t	dev = mp->m_dev;
	
	if ((gp = gfreeh) == NULL) {
		return(NULL);
	}
	if (gp->g_count) {
		cprintf("getegnode: gp 0x%x (%d)\n", gp, gp->g_number);
		panic("getegnode: free gnode isn't");
	}

	/*
	 * Free resources hanging off the gnode from its previous life.
	 * XXX What if the filesystem has already been unmounted?  This
	 * code is unused by UFS and NFS and it causes problems.  It should
	 * probably be moved up to gget() since we can't release the spin
	 * lock here.
	 *
	if ((gp->g_mp) && (gp->g_mp->m_dev != NODEV))
		(void)GFREEGN(gp);
	 */

	/*
	 * Take the gnode off of the free list.  We could call gremque
	 * but we can do it faster here.
	 */

	if (gq = gp->g_freef)
		gq->g_freeb = &gfreeh;
	gfreeh = gq;
	gp->g_freef = NULL;
	gp->g_freeb = NULL;
		

	/*
	 * Now to take gnode off the hash chain it was on (initially, or
	 * after a gflush, it is on a "hash chain" consisting entirely of
	 * itself, and pointed to by no-one, but that doesn't matter), and
	 * put it on the chain for its new (gno, dev) pair.
	 */
		
	remque(gp);
	insque(gp, &ghead[hash]);

	/*
	 * Initialize some of the easy stuff.  We leave the rest for the
	 * caller to do, but these we to set up here for convenience.
	 */

	gp->g_dev = dev;
	gp->g_mp = mp;
	gp->g_number = gno;
	gp->g_lastr = 0;
	gp->g_textp = NULL;
	gp->g_rdev = 0;
	gp->g_blocks = 0;
	gp->g_flag = 0;
	gp->g_fifo = 0;
	gp->g_init = NEW_GNODE;
	bzero((caddr_t)gp->g_in.pad, sizeof(gp->g_in));
	cacheinval(gp);

	if (glocked(gp)) {
		printf("getegnode: free gnode locked 0x%x\n", gp);
		panic("getegnode: locked gnode on freelist");
	}

	return(gp);
}


/*
 * Put a gnode on the end of the free list.  Caller must hold gnode table
 * spin lock.  Historical comment:
 *
 *  "Possibly in some cases it would be better to put the inode at the head
 *   of the free list, eg: where g_mode == 0 || g_number == 0).  The g_number
 *   field is rarely 0 - only after an i/o error in gget, where g_mode == 0,
 *   the gnode will probably be wanted again soon for an ialloc, so possibly
 *   we should keep it."     - kre
 */

freegnode(gp)
	register struct gnode *gp;
{
	if (gp < gnode || gp > gnodeNGNODE)
		panic("freegnode: not a gnode");
	if(gp->g_count != 0) {
		cprintf("freegnode: gp 0x%x (%d)\n", gp, gp->g_number);
		panic("freegnode: freeing active gnode");
	}
	if(glocked(gp)) {
		cprintf("freegnode: gp 0x%x (%d)\n", gp, gp->g_number);
		panic("freegnode: freeing locked gnode");
	}
	if (gfreeh) {
		*gfreet = gp;
		gp->g_freeb = gfreet;
	} else {
		gfreeh = gp;
		gp->g_freeb = &gfreeh;
		wakeup((caddr_t)&gfreeh);
	}
	gp->g_freef = NULL;
	gfreet = &gp->g_freef;
}

/*
 * Call the correct filesystem-specific initialization routine
 * to initialize a gnode, and return success or failure.
 */
int
ginitialize(gp, ptr)
	register struct gnode *gp;
	caddr_t ptr;
{
	int status = 0;
	switch (gp->g_init) {

	case NEW_GNODE:
		status = GINIT(gp, gp->g_init, ptr);
		break;

	case RECLAIM_GNODE:
		status = GRECLAIM(gp, gp->g_init, ptr);
		break;

	default:
		panic("initializing ready gnode");
	}

	if (status == 1)
		gp->g_init = READY_GNODE;
	return(status);
}


/*
 * Called when an error occurred while initializing a gnode.
 * The gnode doesn't contain anything useful, so it would be
 * misleading to leave it on its hash chain.
 */
gclobber(gp)
	struct gnode *gp;
{
	gassert(gp);
	smp_lock(&lk_gnode, LK_RETRY);
	remque(gp);
	gp->g_forw = gp;
	gp->g_back = gp;
	gp->g_number = 0;
#ifdef QUOTA
	gp->g_dquot = NODQUOT;
#endif
	smp_unlock(&lk_gnode);
}

/*
 * Insist that a gnode be active.  We panic if the gnode has
 * a zero reference count or is marked as uninitialized.
 * Caller (must, should, can) hold the gnode table spin lock.
 */
gactive(gp)
	struct gnode *gp;
{
	if (gp->g_count <= 0)
		panic("gnode is inactive");
	if (gp->g_init != READY_GNODE)
		panic("uninitialized gnode");
}


/*
 * There are several examples in gput() and grele() of the gnode count
 * being examined without holding the spin lock.  Some are safe because
 * the result is used only as a "hint" on whether or not we are releasing
 * the last reference...we grab the spin lock and check again before
 * deciding to make the gnode inactive.  Others are safe because we hold
 * a lock on the gnode, so we know that although a new reference can be
 * created, it is safe to make the gnode inactive because a new LOCKED
 * reference cannot be created.
 *
 * Also, in the while loop in grele, we look at the g_init field without
 * holding the gnode sleep lock.  This is safe because we know that we
 * have the only reference to the gnode, and we hold the gnode table spin
 * lock so we know that no new references can be created.
 *
 * The while loop in grele is necessary because there is a window between
 * making the gnode inactive and placing it on the free list when we have
 * released both the gnode sleep lock and the gnode table spin lock.  Another
 * process(or) could conceivably come in, create a reference to the gnode,
 * reinitialize it, and destroy their reference before we get the spin lock
 * back again, leaving us with the last reference to a gnode which needs to
 * be made inactive again.  In practice this will never happen, but the code
 * needs to protect against it.
 *
 * The routine inactive_hack() is the internal part of that while loop.  It
 * tries to lock the gnode (failure ==> not last reference, so give up),
 * checks to make sure that no new references have been created, and calls
 * the fs-specific inactive routine.  We check the count without holding the
 * spin lock: this is safe because we hold the gnode sleep lock so no new
 * LOCKED references to this gnode may be created while we're making it
 * inactive (see above).  Unlocked references are OK.
 */


void
inactive_hack(gp)
        struct gnode *gp;
{
        if (smp_lock(&gp->g_lk, 0) == LK_WON) { /* XXX hack? */
                if (gp->g_count == 1) {
                        GINACTIVE(gp);
                        gp->g_init = RECLAIM_GNODE;
                }
                gfs_unlock(gp);
        } else {
#ifdef GFSDEBUG
                smp_lock(&lk_gnode, LK_RETRY);
                if ((gp->g_count == 1) && glocked(gp))
                        panic("grele: inactive gnode is locked");
                smp_unlock(&lk_gnode);
#endif GFSDEBUG
        }
}

void
grele(gp)
	register struct gnode *gp;
{
	int loopcount = 0;
	register int ret;

	if ((gp->g_mode & GFMT) == GFPIPE) {
		fifo_rele(gp);
		return;
	}
#ifdef notdef
	if (gp < gnode || gp > (gnode + ngnode * sizeof (struct gnode)))
		panic("grele: not a gnode");
#endif
        smp_lock(&lk_gnode, LK_RETRY);
        gstats.greles++;
        while ((gp->g_count == 1) && gisready(gp)) {
                smp_unlock(&lk_gnode);
#ifdef GFSDEBUG
                if (loopcount++ > 1)
                        panic("grele: round and round and round we go");
#endif GFSDEBUG

                inactive_hack(gp);
                smp_lock(&lk_gnode, LK_RETRY);
        }
        if (gp->g_count == 1) {
#ifdef GFSDEBUG
                if (gisready(gp))
                        panic("grele: freeing initialized gnode");
#endif GFSDEBUG
                gstats.gfrees++;
                gp->g_flag = 0;
                gp->g_count--;
                freegnode(gp);
                smp_unlock(&lk_gnode);
                return;
        }

        if((gp->g_count--) < 1)
                panic("grele: gp count bad");
        smp_unlock(&lk_gnode);
}


/*
 * Create a new reference to a previously referenced gnode.
 */
gref(gp)
        register struct gnode *gp;
{
        smp_lock(&lk_gnode, LK_RETRY);
        gactive(gp);
        gstats.grefs++;
        gp->g_count++;
        smp_unlock(&lk_gnode);
}

/*
 * Create a ref on a file system to prevent it from
 * being umounted.  Ref is destroyed by calling grele
 * with returned gnode pointer.  dev is optional.
 */
struct gnode *
fref(mp, dev)
	register struct mount *mp;
	register dev_t dev;
{

	smp_lock(&lk_gnode, LK_RETRY);
	if ((mp == NULL) || (mp == (struct mount *) MSWAPX) || !(mp->m_flgs & MTE_DONE) || (dev && (mp->m_dev != dev))) {
		smp_unlock(&lk_gnode);
		return(NULL);
	}
	gactive(mp->m_rootgp);
	mp->m_rootgp->g_count++;
	smp_unlock(&lk_gnode);
	return(mp->m_rootgp);
}


