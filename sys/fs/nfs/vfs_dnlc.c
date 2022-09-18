#ifndef lint
static	char	*sccsid = "@(#)vfs_dnlc.c	4.2	(ULTRIX)	11/9/90";
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

/*
 *	Portions of this software have been licensed to 
 *	Digital Equipment Company, Maynard, MA.
 *	Copyright (c) 1986 Sun Microsystems, Inc.  ALL RIGHTS RESERVED.
 */

#include "../h/types.h"
#include "../h/param.h"
#include "../h/time.h"
#include "../h/systm.h"
#include "../h/errno.h"
#include "../h/user.h"
#include "../net/rpc/types.h"
#include "../nfs/nfs.h"
#include "../nfs/vnode.h"
#include "../nfs/dnlc.h"
#include "../h/fs_types.h"

/*
 * 20 Jul 89 -- condylis
 *	Added hash2 and hash3 to speed up purges of cache entries
 *	referring to a vnode.
 *
 * 18 Jul 88 -- condylis
 *	Made directory name lookup cache operations SMP safe.  dnlc_lookup
 *	now increments ref count of gnode before returning.  This permitted
 *	me to keep all dnlc locking in this file.  
*/
/*
 * Directory name lookup cache.
 * Based on code originally done by Robert Els at Melbourne.
 *
 * Names found by directory scans are retained in a cache
 * for future referene.  It is managed LRU, so frequently
 * used names will hang around.  Cache is indexed by hash value
 * obtained from (vp, name) where the vp refers to the
 * directory containing the name.
 *
 * Hash2 and hash3 are optimized for purging the cache based on 
 * a vnode.  Hash2 is indexed by hash value obtained from the
 * address of the vnode of the directory containing the name.
 * Hash3 is indexed by a hash value obtained from the address of
 * the vnode returned from the lookup.
 *
 * For simplicity (and economy of storage), names longer than
 * some (small) maximum length are not cached, they occur
 * infrequently in any case, and are almost never of interest.
 */

#define NC_CHAIN_SIZE		12	/* the average size of a hash chain */

#define	NC_HASH1(namep, namlen, vp)	\
	((namep[0] + namep[namlen-1] + namlen + (u_int) vp) % nchashsize)
#define	NC_HASH2(vp)	\
	(((u_int)vp / (sizeof (struct gnode)))  % nchashsize)
#define	NC_HASH3(vp)	\
	(((u_int)vp / (sizeof (struct gnode)))  % nchashsize)

/*
 * Macros to insert, remove cache entries from hash, LRU lists.
 */
#define	INS_HASH1(ncp,nch)	insque(ncp, nch)
#define	RM_HASH1(ncp)		remque(ncp)
#define	INS_HASH2(ncp,nch)	insque2(ncp, nch)
#define	RM_HASH2(ncp)		remque2(ncp)
#define	INS_HASH3(ncp,nch)	insque3(ncp, nch)
#define	RM_HASH3(ncp)		remque3(ncp)

#define	INS_LRU(ncp1,ncp2)	insque4((struct ncache *) ncp1, (struct ncache *) ncp2)
#define	RM_LRU(ncp)		remque4((struct ncache *) ncp)

#define	NULL_HASH1(ncp)		(ncp)->hash1_next = (ncp)->hash1_prev = (ncp)
#define	NULL_HASH2(ncp)		(ncp)->hash2_next = (ncp)->hash2_prev = (ncp)
#define	NULL_HASH3(ncp)		(ncp)->hash3_next = (ncp)->hash3_prev = (ncp)

#define NOT_NULL1(ncp)		(ncp)->hash1_next != (ncp)

/*
 * Stats on usefulness of name cache.
 */
struct	ncstats {
	int	hits;		/* hits that we can really use */
	int	misses;		/* cache misses */
	int	enters;		/* number of enters done */
	int	dbl_enters;	/* number of enters tried when already cached */
	int	long_enter;	/* long names tried to enter */
	int	long_look;	/* long names tried to look up */
	int	lru_empty;	/* LRU list empty */
	int	hashedentries;	/* current number of hashed entries */
	int	purges;		/* number of purges of cache */
	int	purgevp;	/* number of calls to dnlc_purge_vp */
	int 	purgevpworked;	/* number of calls to dnlc_purge_vp that */
				/* purged something 			*/
	int	purge1yes;	/* number of dnlc_purge1 successes */
	int	purge1no;	/* number of dnlc_purge1 failures */
};

/*
 * Hash list of name cache entries for fast lookup.
 */
struct	nc_hash {
	struct  ncache  *hash1_next, *hash1_prev;/* lookup hash chain */
	struct  ncache  *hash2_next, *hash2_prev;/* parent vnode hash chain */
	struct  ncache  *hash3_next, *hash3_prev;/* looked up vnode hash chain*/
} *nc_hash1;

/*
 * Hash list of name cache entries for fast purge based on parent gp.
 */
struct nc_hash *nc_hash2;

/*
 * Hash list of name cache entries for fast purge based on looked up gp.
 */
struct nc_hash *nc_hash3;

/*
 * LRU list of cache entries for aging.
 */
struct	nc_lru	{
        struct  ncache  *hash1_next, *hash1_prev;/* lookup hash chain */
        struct  ncache  *hash2_next, *hash2_prev;/* parent vnode hash chain */
        struct  ncache  *hash3_next, *hash3_prev;/* looked up vnode hash chain*/
        struct  ncache  *lru_next, *lru_prev;   /* LRU chain */
} nc_lru;

struct ncache *ncache;		/* the cache itself */
int ncsize = 0;			/* size of cache, computed by dnlc_init() */
int nchashsize = 0;		/* number of hash chains, function of ncsize*/

struct	ncstats ncstats;		/* cache effectiveness statistics */

struct ncache *dnlc_search();
int	dnlc_cache = 1;
struct	lock_t	lk_nfsdnlc;		/* SMP lock for directory name
					/* lookup cache and related 
					/* statistics		  */

/*
 * Initialize the directory cache.
 * Put all the entries on the LRU chain and clear out the hash links.
 */
dnlc_init()
{
	register struct ncache *ncp;
	register int i;
	extern int ngnode;

	/*
	 * Allocate cache based on system "size"
	 */
	if (!ncsize) {
		ncsize = ngnode;
#ifdef UFS
		ncsize -= ncsize/4;	/* downsize cache when UFS present */
#endif UFS
	}

	KM_ALLOC(ncache, struct ncache *,
		 (u_int)(ncsize * sizeof(struct ncache)), KM_NFS,
		 KM_CONTIG | KM_CLEAR | KM_NOWAIT);
	if (ncache == NULL) {
		printf("dnlc_init: can't km_alloc %d\n",
		       ncsize * sizeof(struct ncache));
		panic("dnlc_init");
	}

	/*
	 * number of hash chains
	 */
	nchashsize = ncsize / NC_CHAIN_SIZE;

	KM_ALLOC(nc_hash1, struct nc_hash *,
		 (u_int)(3 * nchashsize * sizeof(struct nc_hash)), KM_NFS,
		 KM_CONTIG | KM_CLEAR | KM_NOWAIT);
	if (nc_hash1 == NULL) {
		printf("dnlc_init: can't km_alloc %d\n",
		       nchashsize * sizeof(struct nc_hash));
		panic("dnlc_init");
	}
	nc_hash2 = &nc_hash1[nchashsize];
	nc_hash3 = &nc_hash2[nchashsize];

	nc_lru.lru_next = (struct ncache *) &nc_lru;
	nc_lru.lru_prev = (struct ncache *) &nc_lru;
	for (i = 0; i < ncsize; i++) {
		ncp = &ncache[i];
		INS_LRU(ncp, &nc_lru);
		NULL_HASH1(ncp);
		NULL_HASH2(ncp);
		NULL_HASH3(ncp);
		ncp->dp = ncp->vp = (struct vnode *) 0;
	}
	for (i = 0; i < nchashsize; i++) {
		ncp = (struct ncache *) &nc_hash1[i];
		NULL_HASH1(ncp);
		ncp = (struct ncache *) &nc_hash2[i];
		NULL_HASH2(ncp);
		ncp = (struct ncache *) &nc_hash3[i];
		NULL_HASH3(ncp);
	}
}

/*
 * Add a name to the directory cahce.
 */
dnlc_enter(dp, name, vp, cred)
	register struct vnode *dp;
	register char *name;
	struct vnode *vp;
	struct ucred *cred;
{
	register int namlen;
	register struct ncache *ncp;
	register int hash1, hash2, hash3;
	struct vnode *tdp;
	struct vnode *tvp;

	if (!dnlc_cache) {
		return;
	}
	namlen = strlen(name);
	if (namlen > NC_NAMLEN) {
		/* SMP lock name cache statistics during access */
		smp_lock(&lk_nfsdnlc, LK_RETRY);
		ncstats.long_enter++;
		smp_unlock(&lk_nfsdnlc);
		return;
	}
	hash1 = NC_HASH1(name, namlen, dp);
	/* SMP lock access to directory name lookup cache */
	smp_lock(&lk_nfsdnlc, LK_RETRY);
	ncp = dnlc_search(dp, name, namlen, hash1, cred);
	if (ncp != (struct ncache *) 0) {
		ncstats.dbl_enters++;
		smp_unlock(&lk_nfsdnlc);
		return;
	}
	/*
	 * Take least recently used cache struct.
	 */
	ncp = nc_lru.lru_next;
	if (ncp == (struct ncache *) &nc_lru) {	/* LRU queue empty */
		ncstats.lru_empty++;
		smp_unlock(&lk_nfsdnlc);
		return;
	}
	/*
	 * Remove from LRU, hash chains.
	 */
	RM_LRU(ncp);
	if (NOT_NULL1(ncp)) {
		ncstats.hashedentries--;
		RM_HASH1(ncp);
		RM_HASH2(ncp);
		RM_HASH3(ncp);
	}

	/*
	 * Save references to vnodes.  We'll release them after unlocking
	 * dnlc.  Must follow lock ordering rules.
	 */
	tdp = ncp->dp;
	tvp = ncp->vp;


	/* make sure the cred associated with the vnode gets freed */
	if (ncp->cred != NOCRED) {   
		crfree(ncp->cred);
	}

	/*
	 * Hold the vnodes we are entering and
	 * fill in cache info.
	 */
	ncp->dp = dp;
	VN_HOLD(dp);
	ncp->vp = vp;
	VN_HOLD(vp);
	ncp->namlen = namlen;
	bcopy(name, ncp->name, (unsigned)namlen);
	ncp->cred = cred;
	if (cred != NOCRED) {
		crhold(cred);
	}
	/*
	 * Insert in LRU, hash chains.
	 */
	hash2 = NC_HASH2(dp);
	hash3 = NC_HASH3(vp);
	INS_LRU(ncp, nc_lru.lru_prev);
	ncstats.hashedentries++;
	INS_HASH1(ncp, &nc_hash1[hash1]);
	INS_HASH2(ncp, &nc_hash2[hash2]);
	INS_HASH3(ncp, &nc_hash3[hash3]);
	ncstats.enters++;
	smp_unlock(&lk_nfsdnlc);
	/*
	 * Drop hold on vnodes (if we had any).
	 */
	if (tdp != (struct vnode *) 0) {
		VN_RELE(tdp);
	}
	if (tvp != (struct vnode *) 0) {
		VN_RELE(tvp);
	}
}

/*
 * Look up a name in the directory name cache.
 */
/*
 * This routine has been changed to handle locking of 
 * the directory name lookup cache and to bump the ref count
 * the vnode found during the lookup before it is returned to the
 * caller.  This lets us keep all the dnlc smp locking in this file.
 */
struct vnode *
dnlc_lookup(dp, name, cred)
	struct vnode *dp;
	register char *name;
	struct ucred *cred;
{
	register int namlen;
	register int hash;
	register struct ncache *ncp;
	register struct tvp;
	struct vnode *tvp;

	if (!dnlc_cache) {
		return ((struct vnode *) 0);
	}
	namlen = strlen(name);
	if (namlen > NC_NAMLEN) {
		/* SMP lock name cache statistics during access */
		smp_lock(&lk_nfsdnlc, LK_RETRY);
		ncstats.long_look++;
		smp_unlock(&lk_nfsdnlc);
		return ((struct vnode *) 0);
	}
	hash = NC_HASH1(name, namlen, dp);
	/* SMP lock directory name cache during access	*/
	smp_lock(&lk_nfsdnlc, LK_RETRY);
	ncp = dnlc_search(dp, name, namlen, hash, cred);
	if (ncp == (struct ncache *) 0) {
		ncstats.misses++;
		smp_unlock(&lk_nfsdnlc);
		return ((struct vnode *) 0);
	}
	ncstats.hits++;
	/*
	 * Move this slot to the end of LRU
	 * chain.
	 */
	RM_LRU(ncp);
	INS_LRU(ncp, nc_lru.lru_prev);
	/*
	 * If not at the head of the hash chain,
	 * move forward so will be found
	 * earlier if looked up again.
	 */
	if (ncp->hash1_prev != (struct ncache *) &nc_hash1[hash]) {
		RM_HASH1(ncp);
		INS_HASH1(ncp, ncp->hash1_prev->hash1_prev);
	}
	tvp = ncp->vp;
	VN_HOLD(tvp);
	smp_unlock(&lk_nfsdnlc);
	return (tvp);
}

/*
 * Remove an entry in the directory name cache.
 */
dnlc_remove(dp, name)
	struct vnode *dp;
	register char *name;
{
	register int namlen;
	register struct ncache *ncp;
	int hash;

	namlen = strlen(name);
	if (namlen > NC_NAMLEN) {
		return;
	}
	hash = NC_HASH1(name, namlen, dp);
	/* SMP lock access to directory name lookup cache */
	smp_lock(&lk_nfsdnlc, LK_RETRY);
	while (ncp = dnlc_search(dp, name, namlen, hash, ANYCRED)) {
		dnlc_rm(ncp);
	}
	smp_unlock(&lk_nfsdnlc);
}

/*
 * Purge the entire cache.
 */
dnlc_purge()
{
	register struct nc_hash *nch;
	register struct ncache *ncp;

	/* SMP lock access to directory name lookup cache */
	smp_lock(&lk_nfsdnlc, LK_RETRY);
	ncstats.purges++;
start:
	for (nch = nc_hash1; nch < &nc_hash1[nchashsize]; nch++) {
		ncp = nch->hash1_next;
		while (ncp != (struct ncache *) nch) {
			if (ncp->dp == 0 || ncp->vp == 0) {
				panic("dnlc_purge: zero vp");
			}
			dnlc_rm(ncp);
			goto start;
		}
	}
	smp_unlock(&lk_nfsdnlc);
}

/*
 * Purge any cache entries referencing a vnode.
 */
dnlc_purge_vp(vp)
	register struct vnode *vp;
{
	register int moretodo, hash;
	register struct nc_hash *nhp;
	register struct ncache *ncp;
	register int hitcnt = 0;

	/* SMP lock access to directory name lookup cache */
	smp_lock(&lk_nfsdnlc, LK_RETRY);
	ncstats.purgevp++;
	do {
		moretodo = 0;
		hash = NC_HASH2(vp);
		(struct nc_hash *) ncp = nhp = &nc_hash2[hash];
		while ((ncp = ncp->hash2_next) != (struct ncache *) nhp) {
			if (ncp->dp == vp) {
				dnlc_rm(ncp);
				moretodo = 1;
				hitcnt++;
				break;
			}
		}
		hash = NC_HASH3(vp);
		(struct nc_hash *) ncp = nhp = &nc_hash3[hash];
		while ((ncp = ncp->hash3_next) != (struct ncache *) nhp) {
			if (ncp->vp == vp) {
				dnlc_rm(ncp);
				moretodo = 1;
				hitcnt++;
				break;
			}
		}
	} while (moretodo);
	if (hitcnt)
		ncstats.purgevpworked++;
	smp_unlock(&lk_nfsdnlc);
}

/*
 * Purge any cache entry.
 * Called by iget when inode freelist is empty.
 */
dnlc_purge1()
{
	register struct ncache *ncp;

	smp_lock(&lk_nfsdnlc, LK_RETRY);
	for (ncp = nc_lru.lru_next; ncp != (struct ncache *) &nc_lru;
	     ncp = ncp->lru_next) {
		if (ncp->dp &&
		    (((struct gnode *)ncp->vp)->g_count == 1 ||
		    (ncp->vp && ((struct gnode *)ncp->dp)->g_count == 1))) {
			dnlc_rm(ncp);
			ncstats.purge1yes++;
			smp_unlock(&lk_nfsdnlc);
			return (1);
		}
	}
	ncstats.purge1no++;
	smp_unlock(&lk_nfsdnlc);
	return (0);
}

/*
 * Obliterate a cache entry.
 */
/*
 * Now assumes lk_nfsdnlc is held.  This is unlocked long enough
 * to grele the gnodes associated with the cache entry.
 */
static
dnlc_rm(ncp)
	register struct ncache *ncp;
{
	register struct vnode *tdp;
	register struct vnode *tvp;
	/*
	 * Remove from LRU, hash chains.
	 */
	RM_LRU(ncp);
	if (NOT_NULL1(ncp)) {
		ncstats.hashedentries--;
		RM_HASH1(ncp);
		RM_HASH2(ncp);
		RM_HASH3(ncp);
	}
	/*
	 * Release ref on vnodes.
	 */
/*	VN_RELE(ncp->dp); */
	tdp = ncp->dp;
	ncp->dp = (struct vnode *) 0;
/*	VN_RELE(ncp->vp); */
	tvp = ncp->vp;
	ncp->vp = (struct vnode *) 0;
	if (ncp->cred != NOCRED) {
		crfree(ncp->cred);
		ncp->cred = NOCRED;
	}
	/*
	 * Insert at head of LRU list (first to grab).
	 */
	INS_LRU(ncp, &nc_lru);
	/*
	 * And make a dummy hash chain.
	 */
	NULL_HASH1(ncp);
	NULL_HASH2(ncp);
	NULL_HASH3(ncp);
	smp_unlock(&lk_nfsdnlc);
	VN_RELE(tdp);
	VN_RELE(tvp);
	smp_lock(&lk_nfsdnlc, LK_RETRY);
}

/*
 * Utility routine to search for a cache entry.
 */
static struct ncache *
dnlc_search(dp, name, namlen, hash, cred)
	register struct vnode *dp;
	register char *name;
	register int namlen;
	int hash;
	struct ucred *cred;
{
	register struct nc_hash *nhp;
	register struct ncache *ncp;

	nhp = &nc_hash1[hash];
	for (ncp = nhp->hash1_next; ncp != (struct ncache *) nhp;
	    ncp = ncp->hash1_next) {
		if (ncp->dp == dp && ncp->namlen == namlen &&
		    *ncp->name == *name &&	/* fast chk 1st chr */
		    bcmp(ncp->name, name, namlen) == 0 &&
		    (cred == ANYCRED || ncp->cred == cred ||
		     (cred->cr_uid == ncp->cred->cr_uid &&
		      cred->cr_gid == ncp->cred->cr_gid &&
		      bcmp((caddr_t)cred->cr_groups, 
			   (caddr_t)ncp->cred->cr_groups,
			   NGROUPS * sizeof(cred->cr_groups[0])) == 0))) {
			return (ncp);
		}
	}
	return ((struct ncache *) 0);
}

/*
 * Insert into queue, where the queue pointers are
 * not in the first two longwords.
 * Should be in assembler like insque.
 */
static
insque2(ncp2, ncp1)
	register struct ncache *ncp2, *ncp1;
{
	register struct ncache *ncp3;

	ncp3 = ncp1->hash2_next;
	ncp1->hash2_next = ncp2;
	ncp2->hash2_next = ncp3;
	ncp3->hash2_prev = ncp2;
	ncp2->hash2_prev = ncp1;
}
static
insque3(ncp2, ncp1)
	register struct ncache *ncp2, *ncp1;
{
	register struct ncache *ncp3;

	ncp3 = ncp1->hash3_next;
	ncp1->hash3_next = ncp2;
	ncp2->hash3_next = ncp3;
	ncp3->hash3_prev = ncp2;
	ncp2->hash3_prev = ncp1;
}
static
insque4(ncp2, ncp1)
	register struct ncache *ncp2, *ncp1;
{
	register struct ncache *ncp3;

	ncp3 = ncp1->lru_next;
	ncp1->lru_next = ncp2;
	ncp2->lru_next = ncp3;
	ncp3->lru_prev = ncp2;
	ncp2->lru_prev = ncp1;
}

/*
 * Remove from queue, like insque2, insque3, insque4.
 */
static
remque2(ncp)
	register struct ncache *ncp;
{
	ncp->hash2_prev->hash2_next = ncp->hash2_next;
	ncp->hash2_next->hash2_prev = ncp->hash2_prev;
}
static
remque3(ncp)
	register struct ncache *ncp;
{
	ncp->hash3_prev->hash3_next = ncp->hash3_next;
	ncp->hash3_next->hash3_prev = ncp->hash3_prev;
}
static
remque4(ncp)
	register struct ncache *ncp;
{
	ncp->lru_prev->lru_next = ncp->lru_next;
	ncp->lru_next->lru_prev = ncp->lru_prev;
}
