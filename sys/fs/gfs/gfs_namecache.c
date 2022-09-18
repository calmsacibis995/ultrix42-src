#ifndef lint
static	char	*sccsid = "@(#)gfs_namecache.c	4.1	(ULTRIX)	7/2/90";
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

/* ------------------------------------------------------------------------
 * Modification History: /sys/sys/ufs_nami.c - now gfs_namecache.c
 *
 * 10 Dec 89 -- chet
 *	Add binvalallgid call in cacheinvalall. 
 *
 * 19 May 88 -- prs
 *	SMP - Added a lockinit to nchinit, to initialize the name
 *	cache lock.
 *
 * 11 Sep 86 -- koehler
 *	changed a register dev_t to a dev_t
 *
 * 14 Oct 85 -- Reilly
 *	Modified a comment
 *
 * 09 Sep 85 -- Reilly
 *	Modified to handle the new 4.3BSD namei code.
 *
 * 26 Oct 84 -- jrs
 *	Add code for nami cacheing
 *	Derived from 4.2BSD, labeled:
 *		ufs_nami.c 6.11	84/07/07
 *
 * -----------------------------------------------------------------------
 */

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

struct	buf *blkatoff();
int	dirchk = 0;

/*
 * Structures associated with name cacheing.
 */
#define	NCHHASH		32	/* size of hash table */

#if	((NCHHASH)&((NCHHASH)-1)) != 0
#define	NHASH(h, i, d)	((unsigned)((h) + (i) + 13 * (int)(d)) % (NCHHASH))
#else
#define	NHASH(h, i, d)	((unsigned)((h) + (i) + 13 * (int)(d)) & ((NCHHASH)-1))
#endif

union	nchash	nchash[NCHHASH];
#define	nch_forw	nch_chain[0]
#define	nch_back	nch_chain[1]

struct	nch	*nchhead, **nchtail;	/* LRU chain pointers */
struct	nchstats nchstats;		/* cache effectiveness statistics */

struct  lock_t lk_namecache;

/*
 * Name cache initialization, from main() when we are booting
 */
nchinit()
{
	register union nchash *nchp;
	register struct nch *ncp;

	lockinit(&lk_namecache, &lock_namecache_d);

	nchhead = 0;
	nchtail = &nchhead;

	for (ncp = nch; ncp < &nch[nchsize]; ncp++) {
		ncp->nc_forw = ncp;			/* hash chain */
		ncp->nc_back = ncp;

		ncp->nc_nxt = NULL;			/* lru chain */
		*nchtail = ncp;
		ncp->nc_prev = nchtail;
		nchtail = &ncp->nc_nxt;

		/* all else is zero already */
	}

	for (nchp = nchash; nchp < &nchash[NCHHASH]; nchp++) {
		nchp->nch_head[0] = nchp;
		nchp->nch_head[1] = nchp;
	}
}

/*
 * Cache flush, called when filesys is umounted to
 * remove entries that would now be invalid
 *
 * The line "nxtcp = nchhead" near the end is to avoid potential problems
 * if the cache lru chain is modified while we are dumping the
 * inode.  This makes the algorithm O(n^2), but do you think I care?
 */
nchinval(dev)
	dev_t dev;
{
	register struct nch *ncp, *nxtcp;

	for (ncp = nchhead; ncp; ncp = nxtcp) {
		nxtcp = ncp->nc_nxt;

		if (ncp->nc_ip == NULL ||
		    (ncp->nc_idev != dev && ncp->nc_dev != dev))
			continue;

			/* free the resources we had */
		ncp->nc_idev = NODEV;
		ncp->nc_dev = NODEV;
		ncp->nc_id = NULL;
		ncp->nc_ino = 0;
		ncp->nc_ip = NULL;


			/* remove the entry from its hash chain */
		remque(ncp);
			/* and make a dummy one */
		ncp->nc_forw = ncp;
		ncp->nc_back = ncp;

			/* delete this entry from LRU chain */
		*ncp->nc_prev = nxtcp;
		if (nxtcp)
			nxtcp->nc_prev = ncp->nc_prev;
		else
			nchtail = ncp->nc_prev;

			/* cause rescan of list, it may have altered */
		nxtcp = nchhead;
			/* put the now-free entry at head of LRU */
		ncp->nc_nxt = nxtcp;
		ncp->nc_prev = &nchhead;
		nxtcp->nc_prev = &ncp->nc_nxt;
		nchhead = ncp;
	}
}

/*
 * Name cache invalidation of all entries.
 * Buffer cache invalidation of all entries using gid's
 */
cacheinvalall()
{
	register struct nch *ncp;

	for (ncp = nch; ncp < &nch[nchsize]; ncp++) {
		ncp->nc_id = 0;
	}
	binvalallgid();
}
