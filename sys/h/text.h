/* 	@(#)text.h	4.2	(ULTRIX)	1/31/91 	*/

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
/*	text.h	6.1	83/07/29	*/

/***********************************************************************
 *
 *		Modification History
 *
 * 12 Jun 89 -- gg
 * 	Changed text structure. Replaced x_daddr member by x_dmap.
 *	(x_dmap is a pointer to the text segment disk map information)
 *	Added a new flag XNOSPCE.
 *
 * 31 Aug 88 -- jmartin
 *	SMP locking of text data and links using various means.
 *
 * 03 Mar 88 - jaa
 *	Added maxtseg, variable used to configure 
 *	the number of swblk_t's for text segments
 *	which are now dynamically km_alloc'd
 *
 * 05 Feb 88 - lp
 *	Compatibilty define (x_iptr == x_gptr).
 *
 * 04 Sep 87 -- depp
 *      A number of changes, all involved with removing the xflush_free_text()
 *      algorithm, and replacing it with an array (x_hcmap) to hold the
 *      indexes of remote cmap entries that are hashed.
 *
 * 18 Aug 87 -- depp, rr
 *      Increased max text size to 12 Mb.  increased x_flag field from
 *      a char to an int.
 *
 * 15 Dec 86 -- depp
 *	added text table queue/dequeuing macros
 *
 * 11 Sep 86 -- koehler
 *	added text table management
 *
 ***********************************************************************/

/*
 * Text structure.
 * One allocated per pure
 * procedure on swap device.
 * Manipulated by text.c
 */

struct xfree {
	struct text  *xun_freef;  /* free text pointer */
	struct text  *xun_freeb;  /* free text pointer */
};

/*
 * At the right margin of the structure and bit field definitions is an
 * indication of how the datum is protected for concurrent (SMP) access.
 * (s) A process must hold the spin lock lk_text to access this datum.
 * (x) Access to this datum is controlled by setting and clearing the
 *     XLOCK bit of this text segment.
 * (p) The datum is written by the process which allocates the text
 *     segment (X_SET) and by the process which frees it (X_CLEAR).
 */

struct text
{
	struct xfree x_free;	/* linked list of free text structures    (s)*/
	struct dmap *x_dmap;	/* disk map for text segments 	          (p)*/
	struct proc *x_ownlock;	/* for recursion in xinval only 	  (x)*/ 
	size_t	x_size;		/* size (clicks)			  (x)*/
	struct proc *x_caddr;	/* ptr to linked proc, if loaded	  (x)*/
 	struct gnode *x_gptr;	/* gnode of prototype			  (p)*/
#define x_iptr	x_gptr
	size_t	x_rssize;	/* size in clicks			  (s)*/
	size_t	x_swrss;	/* swapped rss				  (x)*/
	short	x_count;	/* reference count			  (x)*/
	short	x_ccount;	/* number of loaded references		  (x)*/
	short	x_lcount;	/* number of processes locking segment	  (s)*/
	short	x_poip;		/* page out in progress count		  (s)*/
	u_int	x_flag;		/* traced, written flags	  (see below)*/
};

extern	int maxtsiz;

#define x_freef x_free.xun_freef
#define x_freeb x_free.xun_freeb

#ifdef	KERNEL
extern struct xfree freetext;
struct	text *text, *textNTEXT;
int	ntext;
struct lock_t lk_text;
#endif

#define	XTRC	0x0001		/* Text may be written, exclusive use	  (p)*/
#define	XWRIT	0x0002		/* Text written into, must swap out	  (x)*/
#define	XLOAD	0x0004		/* Currently being read from file	  (p)*/
#define	XLOCK   0x0008		/* Being swapped in or out		  (s)*/
#define	XWANT	0x0010		/* Wanted for swapping			  (s)*/
#define	XPAGI	0x0020		/* Page in on demand from inode		  (p)*/
#define XNOSW	0x0040		/* Lock segment in memory		  (s)*/
#define XFREE	0x0080		/* Text table on free list		  (s)*/
#define XBAD	0x0100		/* Bad Text entry - remove on exit (nfs)  (x)
                                 * (NFS modified text or local unlinked text)
                                 */
#define XREMOTE 0x0200          /* Remote file				  (p)*/
#define XNOSPCE 0x0400		/* Set to indicate text could not be swapped */
				/* out because of no swap space and has */
				/* to be Loaded afresh as DEMAND LOAD case */
/*
 * Macros to queue and dequeue text table entry on free list
 * Also, a macro to check whether a text entry is free
 */

#define	X_QFREE(xp)	{				\
	int s;						\
	s = splimp();					\
	smp_lock(&lk_text, LK_RETRY);			\
	if ((((xp)->x_flag ^= XFREE) & XFREE) == 0)	\
		panic("Freeing free text");		\
	insque(&(xp)->x_free, &freetext);		\
	smp_unlock(&lk_text);				\
	(void)splx(s);					\
}

#define	X_DQFREE(xp)	{				\
	int s;						\
	s = splimp();					\
	smp_lock(&lk_text, LK_RETRY);			\
	if ((((xp)->x_flag ^= XFREE) & XFREE) != 0)	\
		panic("Dequeuing non-free text");	\
	remque(&(xp)->x_free);				\
	smp_unlock(&lk_text);				\
	(void)splx(s);					\
}
	
#define X_DO_RHASH(xp)   (((xp)->x_flag & (XREMOTE|XPAGI)) == (XREMOTE|XPAGI))

/*
 *	X_FLUSH macro will flush memory hash list if required
 */
#define X_FLUSH(xp)	{			\
	if (X_DO_RHASH(xp))			\
		xflush_remote_hash(xp);		\
}

#define X_CLEAR(xp,gp)  {		\
	(gp)->g_textp=NULL; 		\
	(gp)->g_flag &= ~GTEXT; 	\
	(xp)->x_gptr=NULL; 		\
	(xp)->x_flag &= ~XREMOTE; 	\
}

#define X_SET(xp,gp) {			\
	 (gp)->g_textp=(xp); 		\
	 (xp)->x_gptr=(gp);  		\
	 if (ISLOCAL((gp)->g_mp))  	\
	   (xp)->x_flag &= ~XREMOTE; 	\
	 else 				\
	   (xp)->x_flag |= XREMOTE; 	\
}
