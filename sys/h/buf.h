/*	@(#)buf.h	4.2	(ULTRIX)	7/17/90	*/

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
/*
 *
 *   Modification history:
 *
 *  5 Jul 90 -- chet
 *	Add B_FORCEWRITE flag
 *
 * 23 May 90 -- chet
 *	Put _BUF_H_ protection against multiple inclusion in place.
 *
 * 10 Dec 89 -- chet
 *	Replace nomatch() macro and add buffer cache stat structures
 *
 * 25 Jul 89 -- chet
 *	New buffer cache organization
 *
 * 28 Mar 89 -- tim
 *	Added definitions used to calculate disk unit numbers.  These are
 *	needed because MSCP devices (ra disks) use a range of major numbers.
 *
 * 27 Apr 88 -- chet
 *	Add SMP support.
 *
 *  2 Mar 88 -- chet
 *	Added B_NOCACHE.
 *
 * 23 Oct 86 -- chet
 *	add IO_ASYNC, IO_SYNC, and IO_APPEND constants for GRWGP
 *
 * 11 Sep 86 -- koehler
 *	gfs change  align columns change comments for accuracy
 *
 * 11 Mar 86 -- lp
 *	Added flag for n-bufferring (B_RAWASYNC).
 *
 * 25 Apr 85 -- depp
 *	Removed SHMEM ifdefs
 *
 * 01 Mar 85 -- depp
 *	Added Shared memory definition for bproc pointer.
 *
 */

/*
 * The header for buffers in the buffer pool and otherwise used
 * to describe a block i/o request is given here.  The routines
 * which manipulate these things are given in gfs_bio.c.
 *
 * Each buffer in the pool is usually doubly linked into 2 lists:
 * hashed into a chain by <dev,blkno> so it can be located in the cache,
 * and also on a queue.  These lists are circular and
 * doubly linked for easy removal.
 *
 * There are currently three queues for buffers:
 *	one for buffers which are dirty (DELWRI buffers).
 * 	one for buffers which are clean (and buffers containing
 *		``non-useful'' information, pushed onto the front).
 *	one for empty buffers which have no physical memory attached.
 *
 * The clean buffers which contain useful data constitute the ``cache'';
 * they are kept in LRU order.
 *
 * The latter two queues contain the buffers which are available for
 * reallocation. When not on one of these queues,
 * buffers are ``checked out'' to drivers which use the available list
 * pointers to keep track of them in their i/o active queues.
 */

#ifndef _BUF_H_
#define _BUF_H_

/*
 * Bufhd structures used at the head of the hashed buffer queues.
 * We only need three words for these, so this abbreviated
 * definition saves some space.
 */
struct bufhd
{
	long	b_flags;		/* see defines below */
	struct	buf *b_forw, *b_back;	/* fwd/bkwd pointer in chain */
};
struct buf
{
	long	b_flags;		/* too much goes here to describe */
	struct	buf *b_forw, *b_back;	/* hash chain (2 way street) */
	struct	buf *av_forw, *av_back;	/* position on free list if not BUSY */
#define	b_actf	av_forw			/* alternate names for driver queue */
#define	b_actl	av_back			/*    head - isn't history wonderful */
	long	b_bcount;		/* transfer count */
	long	b_bufsize;		/* size of allocated buffer */
#define	b_active b_bcount		/* driver queue head: drive active */
	short	b_error;		/* returned after I/O */
	dev_t	b_dev;			/* major+minor device name */
	union {
	    caddr_t b_addr;		/* low order core address */
	    int	*b_words;		/* words for clearing */
	    struct fs *b_fs;		/* superblocks */
	    struct csum *b_cs;		/* superblock summary information */
	    struct cg *b_cg;		/* cylinder group block */
	    struct ufs_inode *b_dino;	/* ilist */
	    daddr_t *b_daddr;		/* indirect block */
	} b_un;
	daddr_t	b_blkno;		/* block # on device */
	long	b_resid;		/* words not transferred after error */
#define	b_errcnt b_resid		/* while i/o in progress: # retries */
	struct  proc *b_proc;		/* proc doing physical or swap I/O */
	struct	gnode *b_gp;		/* who owns this bp (used remotely)*/
	int	b_gid;			/* gnode id number */
	int	(*b_iodone)();		/* function called by iodone */
	int	b_pfcent;		/* center page when swapping cluster */
	struct	buf *busy_forw, *busy_back;
	                                /* 17, 18: position on busy list */
	long	busy_time;		/* 19: time (sec.) put on busy list */
	long	state;			/* 20: buffer state flags */
					/* the "true" location for several */
					/* old b_flags bits (see comment) */
};

/*
 * Some of the buffer flags (B_BUSY, B_WANTED, B_INVAL) have been
 * moved to the state field.  They may be referenced only when
 * the lk_bio lock is held.  The corresponding bit positions in the
 * b_flags word are also set as before to preserve backward compatibility
 * with device drivers.
 */

#define	BQUEUES		4		/* number of free buffer queues */

#define	BQ_LOCKED	0		/* unused */
#define	BQ_DIRTY	1		/* dirty buffers (must flush) */
#define	BQ_CLEAN	2		/* clean buffers (need not flush) */
#define	BQ_EMPTY	3		/* buffer headers with no memory */

#define BQ_LRU		BQ_CLEAN
#define BQ_AGE		BQ_DIRTY

#ifdef	KERNEL

#include "../h/time.h"
extern struct timeval time;

#define BHASHARG(dev, gp) (((gp)) ? (int)(gp) : (int)(dev))

/* NOTE: bufhsz (see bhinit() in init_main.c) MUST be a power of 2!! */
#define RND	(MAXBSIZE/DEV_BSIZE)
#define	BUFHASH(dev, dblkno, gp)	\
	((struct buf *)&bufhash[(BHASHARG((dev), (gp)) + \
				 (((int)(dblkno))/RND)) & bufhszminus1])

struct	buf *buf;		/* the buffer pool itself */

int	nbuf;			/* number of buffer headers */
char	*buffers;
int	bufpages;		/* number of memory pages in the buffer pool */

struct	bufhd *bufhash;		/* heads of hash lists */
int	bufhsz, bufhszminus1;	/* number of buffer hash lists */
                                /* computed in binit() */

struct	buf bfreelist[BQUEUES];	/* heads of available lists */
struct  buf bbusylist;		/* head of busy list */

int	nswbuf;
struct	buf *swbuf;		/* swap I/O headers */
struct	buf bswlist;		/* head of free swap header list */
struct	buf *bclnlist;		/* head of cleaned page list */

/*
 * This lock controls access to the free list, hash chains, state word of
 * individual buffers, and changes to the b_dev, b_blkno, b_gp and b_gid fields
 * of buffers (or examinations of those fields when the buffer isn't busy).
 */
extern	struct lock_t lk_bio;

struct	buf *alloc();
struct	buf *realloccg();
struct	buf *baddr();
struct	buf *getblk();
struct	buf *geteblk();
struct	buf *getnewbuf();
struct	buf *bread();
struct	buf *breada();

unsigned minphys();
#endif /* KERNEL */

/*
 * These flags are kept in b_flags.
 */
#define	B_WRITE		0x00000000	/* non-read pseudo-flag */
#define	B_READ		0x00000001	/* read when I/O occurs */
#define	B_DONE		0x00000002	/* transaction finished */
#define	B_ERROR		0x00000004	/* transaction aborted */
#define	B_BUSY		0x00000008	/* not on av_forw/back list */
#define	B_PHYS		0x00000010	/* physical IO */
#define	B_XXX		0x00000020	/* was B_MAP, alloc UNIBUS on pdp-11 */
#define	B_WANTED	0x00000040	/* issue wakeup when BUSY goes off */
#define	B_AGE		0x00000080	/* delayed write for correct aging */
#define	B_ASYNC		0x00000100	/* don't wait for I/O completion */
#define	B_DELWRI	0x00000200	/* write at exit of avail list */
#define	B_TAPE		0x00000400	/* this is a magtape (no bdwrite) */
#define	B_UAREA		0x00000800	/* add u-area to a swap operation */
#define	B_PAGET		0x00001000	/* page in/out of page table space */
#define	B_DIRTY		0x00002000	/* dirty page to be pushed out async */
#define	B_PGIN		0x00004000	/* pagein op, so swap() can count it */
#define	B_CACHE		0x00008000	/* did bread find us in the cache ? */
#define	B_INVAL		0x00010000	/* does not contain valid info  */
#define	B_LOCKED	0x00020000	/* locked in core (not reusable) */
#define	B_HEAD		0x00040000	/* a buffer header, not a buffer */
#define	B_BAD		0x00100000	/* bad block revectoring in progress */
#define	B_CALL		0x00200000	/* call b_iodone from iodone */
#define B_SMEM		0x00400000	/* b_proc is a ptr to "smem" */
#define B_RAWASYNC	0x00800000	/* buffer involved in raw async I/O */
#define B_NOCACHE	0x01000000	/* don't cache block when released */
#define B_FORCEWRITE	0x02000000	/* delayed write caused by empty clean
					   list */

/*
 * Sync, Async, and Append flags for GRWGP
 */
#define IO_ASYNC	0x0
#define IO_SYNC		0x1
#define IO_APPEND	0x2

/*
 * Insq/Remq for the buffer hash lists.
 */
#define	bremhash(bp) { \
	(bp)->b_back->b_forw = (bp)->b_forw; \
	(bp)->b_forw->b_back = (bp)->b_back; \
}
#define	binshash(bp, dp) { \
	(bp)->b_forw = (dp)->b_forw; \
	(bp)->b_back = (dp); \
	(dp)->b_forw->b_back = (bp); \
	(dp)->b_forw = (bp); \
}

/*
 * Insq/Remq for the buffer free and busy lists.
 */
#define	bremfree(bp) { \
	(bp)->av_back->av_forw = (bp)->av_forw; \
	(bp)->av_forw->av_back = (bp)->av_back; \
}
#define	brembusy(bp) { \
        if (!((bp)->state & B_BUSY)) \
		panic("brembusy"); \
	(bp)->busy_back->busy_forw = (bp)->busy_forw; \
	(bp)->busy_forw->busy_back = (bp)->busy_back; \
}
#define	binsheadfree(bp, dp) { \
	(dp)->av_forw->av_back = (bp); \
	(bp)->av_forw = (dp)->av_forw; \
	(dp)->av_forw = (bp); \
	(bp)->av_back = (dp); \
}
#define	binstailfree(bp, dp) { \
	(dp)->av_back->av_forw = (bp); \
	(bp)->av_back = (dp)->av_back; \
	(dp)->av_back = (bp); \
	(bp)->av_forw = (dp); \
}
#define	binstailbusy(bp) { \
	if (!((bp)->state & B_BUSY)) \
		panic("binstailbusy"); \
        bbusylist.busy_back->busy_forw = (bp); \
	(bp)->busy_back = bbusylist.busy_back; \
	bbusylist.busy_back = (bp); \
	(bp)->busy_forw = &bbusylist; \
	(bp)->busy_time = time.tv_sec; \
}

/*
 * Take a buffer off the free list it's on and
 * mark it as being used (B_BUSY) by a device.
 *
 * We also set the old B_BUSY bit for backward
 * compatibility with device drivers.
 *
 * NOTE: must be called at splbio().  Caller must
 * own the lk_bio lock.
 */
#define	notavail(bp) { \
	bremfree(bp); \
	(bp)->state |= B_BUSY; \
	(bp)->b_flags |= B_BUSY; \
	binstailbusy(bp); \
}

#define	iodone	biodone
#define	iowait	biowait

#define bufavail(bp) \
	((bp >= &buf[0]) && (bp < &buf[nbuf]) && ((bp->state&B_BUSY) == 0))

#define bassert(bp) \
	if (bufavail(bp)) panic("buffer should be busy");

/*
 * Zero out a buffer's data portion.
 */
#define	clrbuf(bp) { \
	blkclr(bp->b_un.b_addr, bp->b_bcount); \
	bp->b_resid = 0; \
}

/*
 * Routines to match gnode pointers and buffers:
 *
 * N.B. UFS passes a NULL gnode pointer.
 *      matchgp() will match (return TRUE) any UFS gnode and
 *	          any UFS buffer.
 *	matchgp() will match an NFS gnode to a buffer that
 *		  has been invalidated for that gnode; matchgid() must
 *		  also be used to check for this case.
 *      matchgid() MUST return TRUE so that UFS buffers match UFS gnodes. 
 */
#define matchgp(bp, gp) ((gp) == (bp)->b_gp) 
#define matchgid(bp, gp) (!(gp) || (bp)->b_gid == (gp)->g_id) 

#define STRATEGY(gp, dev, bp, saveaffinity) { \
	if (gp) { \
		(*GIOSTRATEGY(gp))(bp);	\
	} else { \
		CALL_TO_NONSMP_DRIVER(bdevsw[major(dev)],saveaffinity); \
		(*bdevsw[major(dev)].d_strategy)(bp);	\
		RETURN_FROM_NONSMP_DRIVER(bdevsw[major(dev)],saveaffinity); \
	}							\
}

/*
 * Release the buffer, start I/O on it, but don't wait for completion.
 */
#define bawrite(bp) { \
	bp->b_flags |= B_ASYNC; \
	bwrite(bp); \
}

/*
 * Disk unit number calculations.
 *
 * MSCP devices use a range of major numbers.  For this reason, unit number
 * calculation involves determining if an MSCP disk is in use.  If so determine
 * the offset from the base major number and multiply by the number of units
 * that each major number is used to represent.
 *
 * 2 sets of macros are needed because the major number for block and
 * character devices are not the same.
 */
#define MSCP_B_MIN 23			/* First major number for block ra */
#define MSCP_B_MAX 30			/* Last  major number for block ra */
#define MSCP_C_MIN 60			/* First major number for char ra */
#define MSCP_C_MAX 67			/* Last  major number for char ra */

/*
 * The maximum ra unit number is  the number of units per major number
 * multiplied by the number of major numbers in use.  The number of major 
 * numbers used by the block and character devices must be the same.
 */
#define MSCP_MAJORS (MSCP_B_MAX - MSCP_B_MIN + 1) 
#define MSCP_UNITS_MAJOR 32		
#define MSCP_MAXDISK ((MSCP_MAJORS * MSCP_UNITS_MAJOR) - 1)

/*
 * MSCP_B_DEV is true if the dev is an ra block device.
 * MSCP_C_DEV is true if the dev is an ra character device.
 *
 * The UNIT_OFFSET macros are used to calculate how far away from the base
 * major number the particular unit number is.
 */
#define MSCP_B_DEV(Dev) ((major(Dev) >= MSCP_B_MIN)&&(major(Dev) <= MSCP_B_MAX))
#define MSCP_B_UNIT_OFFSET(Dev) ((major(Dev) - MSCP_B_MIN) * 32)

#define MSCP_C_DEV(Dev) ((major(Dev) >= MSCP_C_MIN)&&(major(Dev) <= MSCP_C_MAX))
#define MSCP_C_UNIT_OFFSET(Dev) ((major(Dev) - MSCP_C_MIN) * 32)

/*
 * Structures used for kernel buffer cache stats
 */
struct bflush_dirty {
	unsigned call;		/* 1: number of calls  */
	unsigned look;		/* 2: dirty buffers inspected */
	unsigned flush;		/* 3: dirty buffers flushed */
	unsigned loop;		/* 4: long loops detected */
};

struct bflush_busy {
	unsigned look;		/* 1: busy buffers inspected */
	unsigned sleep;		/* 2: busy buffers slept on */
	unsigned more;		/* 3: more sleeps than flushes */
	unsigned loop;		/* 4: long loops detected */
};

struct bufstats {	/* cache hits and misses */
	unsigned readhit;	/* 1: bread cache hits */
	unsigned readmiss;	/* 2: bread cache misses */
	unsigned readahit;	/* 3: breada cache hits */
	unsigned readamiss;	/* 4: breada cache misses */

	/* write operations and I/O errors */ 
	unsigned sync_write;	/* 5: sync writes */
	unsigned async_write;	/* 6: async writes (incl. delayed writes) */
	unsigned delwrite;	/* 7: writes that had been delayed */
	unsigned biodone_errs;	/* 8: error buffers to biodone */

	/* new buffer and memory lending operations */
	unsigned newbuf;	/* 9: get new buffer calls */
	unsigned realloc;	/* 10: buffer reallocations */
	unsigned brelse;	/* 11: brelse calls */
	unsigned brelsenotbusy;	/* 12: brelse of un-busy buffers */

	/* blkflush() operations */
	unsigned blkflush_call;	 /* 13: blkflush() calls */
	unsigned blkflush_look;	 /* 14: blkflush() buffers inspected */
	unsigned blkflush_flush; /* 15: blkflush() buffers flushed */
	unsigned blkflush_sleep; /* 16: blkflush() sleeps */

	/* binval() operations */
	unsigned binval_call;	/* 17: binval() calls */
	unsigned binval_look;	/* 18: binval() buffers inspected */
	unsigned binval_inval;	/* 19: binval() invals */
	unsigned blkflushgp;	/* 20: blkflush() called with gp */

	/* binvalfree() operations */
	unsigned binvalfree_call;  /* 21: binvalfree() calls */
	unsigned binvalfree_look;  /* 22: binvalfree() buffers inspected */
	unsigned binvalfree_inval; /* 23: binvalfree() invals */
	unsigned geteblk;	   /* 24: geteblk() calls */		  
	
	/* miscellaneous operations */
	unsigned binvalallgid_call; /* 25: binvalallgid() calls */
	unsigned forcewrite;        /* 26: writes forced by empty clean list */
	unsigned unused2; 	    /* 27: */
	unsigned unused3;	    /* 28: */		  
	
	/* bflush() `gp', e.g. NFS file syncs */
	struct bflush_dirty gp_async;	/* 29 - 32 */
	struct bflush_dirty gp_sync;	/* 33 - 36 */
	struct bflush_busy gp_busy;	/* 37 - 40 */

	/* bflush() `dev', e.g. close of block device, unmount, getxfile(), */
	/* UFS file syncs */
	struct bflush_dirty dev_async;	/* 41 - 44 */
	struct bflush_dirty dev_sync;	/* 45 - 48 */
	struct bflush_busy dev_busy;	/* 49 - 52 */

	/* bflush() `all' , e.g. update() (sync(2)) */
	struct bflush_dirty all_async;	/* 53 - 56 */
	struct bflush_dirty all_sync;	/* 57 - 60 */
	struct bflush_busy all_busy;	/* 61 - 64 */

} ;

#endif /* _BUF_H_ */

