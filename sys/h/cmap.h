/* 	@(#)cmap.h	4.2	(ULTRIX)	9/4/90 	*/

/************************************************************************
 *									*
 *			Copyright (c) 1986, 1988 by			*
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
/*	cmap.h	6.1	83/07/29	*/
/*
 *	Modification History
 *
 * 06 Mar 90 -- afd
 *	Define SHAREDPG macro in this file so its not in several kn???.c files.
 *
 * 19 Jan 89 -- jmartin
 *	Define CMAPSZ, CMAP_FREE, and CMAP_INTRANS in genassym.
 *
 * 25 Jul 88 -- jmartin
 *	Added declaration of SMP locks lk_cmap, lk_cmap_bio.
 *
 * 04 Sep 87 -- depp
 *      Added c_remote flag to indicate that this cmap is hash on a remote
 *      a.out blkno.
 *
 * 27 Jul 87 -- depp
 *      fixed CMHASH() algorithm to use the entire cmhash array rather than
 *      every other entry
 *
 * 14 Jan 87 -- rr
 *	performance macros
 *
 * 08 Jan 87 -- depp
 *	Up'ed physical memory limit to 1/2 Gbyte
 *
 * 18 Mar 86 -- depp
 *	Added "u" area free list {ucmap, eucmap, nucmap}
 *
 * 25 Apr 85 -- depp
 *	Removed SHMEM ifdefs
 *
 * 12 Mar 85 -- depp
 *	Expanded c_type field to handle shared memory segment type
 *
 */

/*
 * core map entry
 *
 * Limits imposed by this structure
 *
 *		limit		cur.size	fields
 *
 *	physical memory		512 Mb		c_next, c_prev, c_hlink
 *	mounted file systems	255		c_mdev
 *	size of proc segment	1 Gb		c_page
 *	filesystem size		8 Gb		c_blkno	
 *	proc, text table size	4096		c_ndx
 */
#ifdef KERNEL
#include "../h/ansi_compat.h"
#else
#include <ansi_compat.h>
#endif

#define CMLNKSZ	19
#ifndef LOCORE
struct cmap
{
unsigned int 	c_next:CMLNKSZ,	/* index of next free list entry */
		:(23-CMLNKSZ),		/* expansion for c_next */
                c_remote:1,     /* cmap entry hashed to remote a.out */
		c_free:1,	/* on the free list */
		c_intrans:1,	/* intransit bit */
		c_gone:1,	/* associated page has been released */
		c_want:1,	/* wanted */
		c_lock:1,	/* locked for raw i/o or pagein */
		c_type:3,	/* type CSYS, CTEXT, CSTACK, CDATA or CSMEM */

		c_prev:CMLNKSZ,	/* index of previous free list entry */
		:(24-CMLNKSZ),	/* expansion for c_prev */
		c_mdev:8,	/* which mounted dev this is from */

		c_hlink:CMLNKSZ,/* hash link for <blkno,mdev> */
		:(20-CMLNKSZ),	/* expansion for c_hlink */
		c_ndx:12,	/* index of owner proc or text */

		c_blkno:24,	/* disk block this is a copy of */
		:8,		/* fill to longword boundary */

		c_page:21,	/* virtual page number in segment */
		:3,		/* expansion for c_page */
		c_refcnt:8;	/* ref count for future use */
};
#else /* LOCORE */
#include "../h/param.h"
#define MAX_MEM	(1<<(CMLNKSZ+CLSHIFT))
#ifdef __mips
#define SZ_CMAP		CMAPSZ
#define C_FREE		CMAP_FREE
#define C_INTRANS	CMAP_INTRANS
#endif /* __mips */
#endif /* LOCORE */

#define	CMHEAD	0

/*
 * Shared text pages are not totally abandoned when a process
 * exits, but are remembered while in the free list hashed by <mdev,blkno>
 * off the cmhash structure so that they can be reattached
 * if another instance of the program runs again soon.
 */
#define	CMHSIZ	512		/* SHOULD BE DYNAMIC - MUST BE POWER OF 2 */
#define	CMHASH(bn)	(((bn) >> 1) &(CMHSIZ-1))

#ifndef LOCORE
#ifdef	KERNEL
struct	cmap *cmap;
struct	cmap *ecmap;
int	ncmap;
struct	cmap *mfind();
int	firstfree, maxfree;
int	ecmx;			/* cmap index of ecmap */
int	cmhash[CMHSIZ];
#ifdef __mips
extern unsigned int icachemask;	/* mask with pfnum to get icache index */
extern unsigned int dcachemask;	/* mask with pfnum to get dcache index */
#endif /* __mips */

/* temp "u" area free list */
int	ucmap;			/* head */
int	eucmap;			/* tail */
int	nucmap;			/* number of clusters in list */

/* SMP locks for virtual memory subsystem */
struct lock_t lk_cmap_bio;
struct lock_t lk_cmap;

/* an array of smp locks indexed by c_type */
extern struct lock_t *lock_by_type[];
#endif

/* bits defined in c_type */

#define	CSYS		0		/* none of below */
#define	CTEXT		1		/* belongs to shared text segment */
#define	CDATA		2		/* belongs to data segment */
#define	CSTACK		3		/* belongs to stack segment */
#define CSMEM		4		/* shared memory segment */

#define SHAREDPG(pa) ((cmap[pgtocm(btop(pa))].c_type)==CSMEM \
		   || (cmap[pgtocm(btop(pa))].c_type)==CTEXT)

/* a fancier cache like I and D cache on mips needs more info in memall */
#define V_NOOP	0x0
#define V_CACHE 0x1

/* special fast macros for pgtocm -- shifts avoid an ediv instruction */
#if CLSIZE==1
#define	pgtocm(x)	(((x)-firstfree) + 1)
#define	cmtopg(x)	(((x)-1) + firstfree)
#endif
#if CLSIZE==2 || CLSIZE==4
#define	pgtocm(x)	((((x)-firstfree) >> CLSIZELOG2 ) + 1)
#define	cmtopg(x)	((((x)-1) << CLSIZELOG2 ) + firstfree)
#endif
#if CLSIZE!=1 && CLSIZE!=2 && CLSIZE!=4
#define	pgtocm(x)	((((int)(x)-firstfree) / CLSIZE) + 1)
#define	cmtopg(x)	((((x)-1) * CLSIZE) + firstfree)
#endif

extern	int	ucmap;
extern	int	eucmap;
extern	int	nucmap;
#endif /* LOCORE */
