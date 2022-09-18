#ifndef lint
static	char	*sccsid = "@(#)malloc.c	4.1	(ULTRIX)	7/3/90";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1984 by				*
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
 *			Modification History				*
 *									*
 * 2.2	Joshua Friedman, 13-Nov-1989					*
 *	Added explicit behavior for XPG3: realloc() calls free if size	*
 *	is 0.  Already did call malloc() if ptr NULL.			*
 *									*
 *	NOTE: malloc(0) will allocate size = MIN_ALLOC_SIZE = 2 PAGES.	*
 *	This may not be desired behavior, but someone may depend on it.	*
 *									*
 * 002	David L Ballenger, 12-Nov-1985					*
 *	Fix problems with malloc() when given extremely large numbers	*
 *	(IPR-00070).  Also, add fix so that it will look in higher	*
 *	buckets if no more memory can be allocated.  Also, add fix so	*
 *	that very small blocks are not moved unneccesarily by realloc().*
 *									*
 *	David L. Ballenger, 07-Aug-1984					*
 * 0001	Fix problems with doing a realloc() of an already free pointer,	*
 *	when a large number of free()'s have been done before doing the	*
 *	realloc().  Also, cleanup code in morecore().			*
 *									*
 ************************************************************************/

#ifdef vax
/*
 * malloc.c (Caltech) 2/21/82
 * Chris Kingsley, kingsley@cit-20.
 *
 * This is a very fast storage allocator.  It allocates blocks of a small 
 * number of different sizes, and keeps free lists of each size.  Blocks that
 * don't exactly fit are passed up to the next larger size.  In this 
 * implementation, the available sizes are 2^n-4 (or 2^n-12) bytes long.
 * This is designed for use in a program that uses vast quantities of memory,
 * but bombs when it runs out. 
 */
#include <sys/types.h>

#define	NULL 0

/*
 * The overhead on a block is at least 4 bytes.  When free, this space
 * contains a pointer to the next free block, and the bottom two bits must
 * be zero.  When in use, the first byte is set to MAGIC, and the second
 * byte is the size index.  The remaining bytes are for alignment.
 * If range checking is enabled and the size of the block fits
 * in two bytes, then the top two bytes hold the size of the requested block
 * plus the range checking words, and the header word MINUS ONE.
 */
union	overhead {
	union	overhead *ov_next;	/* when free */
	struct {
		u_char	ovu_magic;	/* magic number */
		u_char	ovu_index;	/* bucket # */
#ifdef RCHECK
		u_short	ovu_size;	/* actual block size */
		u_int	ovu_rmagic;	/* range magic number */
#endif
	} ovu;
#define	ov_magic	ovu.ovu_magic
#define	ov_index	ovu.ovu_index
#define	ov_size		ovu.ovu_size
#define	ov_rmagic	ovu.ovu_rmagic
};

#define	MAGIC		0xff		/* magic # on accounting info */
#define RMAGIC		0x55555555	/* magic # on range info */
#ifdef RCHECK
#define	RSLOP		sizeof (u_int)
#else
#define	RSLOP		0
#endif

/* The total overhead is the amount of space taken up by the overhead
 * union information and the space at the end for range checking (if
 * that is turned on.
 */
#define TOTAL_OVERHEAD (sizeof(union overhead)+RSLOP)

/* The amount of memory that a user requests has the overhead added on
 * and is rounded up to the next power of 2 within the range of MIN_POWER_2
 * and MAX_POWER_2.  This range controls the number of hash buckets needed
 * to store information about allocated memory.
 *
 * MAX_POWER_2 controls the largest block which can be allocated.  The value
 * is 30 rather than 31, because 31 would definitely put the allocation into
 * system address space.
 *
 * MIN_POWER_2 controls the smallest block which can be allocated, and the
 * granularity / alignment in which blocks are allocated. 
 *
 * NBUCKETS controls the number of hash buckets needed and is in turn
 * controlled by MAX_POWER_2 and MIN_POWER_2.
 */
#define MAX_POWER_2 (30)
#define MIN_POWER_2 (3)
#define NBUCKETS   ((MAX_POWER_2 - MIN_POWER_2) + 1)	


/* The maximum block which malloc() will attempt to allocate is calculated
 * by taking (2^MAX_POWER_2) subtracting off the space needed for its own
 * overhead, and then rounding down to the minimum granularity (MIN_POWER_2).
 */
#define MAX_ALLOC_SIZE	(((1<<MAX_POWER_2) - TOTAL_OVERHEAD) & ~MIN_POWER_2)

/*
 * nextf[i] is the pointer to the next free block of size 2^(i+MIN_POWER_2).
 * The smallest allocatable block is 2^MIN_POWER_2 bytes.  The overhead
 * information precedes the data area returned to the user.
 */
static	union overhead *nextf[NBUCKETS];
extern	char *sbrk();

#ifdef MSTATS
/*
 * nmalloc[i] is the difference between the number of mallocs and frees
 * for a given block size.
 */
static	u_int nmalloc[NBUCKETS];
#include <stdio.h>
#endif

#ifdef debug
#define	ASSERT(p)   if (!(p)) botch("p"); else
static
botch(s)
	char *s;
{

	printf("assertion botched: %s\n", s);
	abort();
}
#else
#define	ASSERT(p)
#endif

/* Define the page size and a mask for determining if a pointer falls on
 * a page boundary.  Also, define the minimum memory allocation size based
 * on the page size.  Note that this hardwires the page size in rather than
 * using the call to getpagesize().
 */
#define	PAGE_SIZE	1024
#define	PAGE_BOUND_MASK	(PAGE_SIZE - 1)
#define MIN_ALLOC_SIZE	(PAGE_SIZE * 2)

char *
malloc(nbytes)
	register unsigned nbytes;
{
  	register union overhead *p;
  	register int bucket;
  	register unsigned shiftr;

	/* Make sure the number of bytes requested is not larger than the
	 * maximum size that can be allocated.  This is done here, rather
	 * than attempting to just let sbrk() fail, because subsequent
	 * calculations may cause the value of nbytes to loop arround from
	 * a very high value to a very low value.
	 */
	if (nbytes > MAX_ALLOC_SIZE)
		return(NULL);

	/* Convert the number of bytes requested into the appropriate
	 * allocation block size, this includes room for overhead used
	 * by malloc() to keep track of memory (TOTAL_OVERHEAD), and 
	 * rounding to the minimum granularity / alignment (MIN_POWER_2).
	 */
  	nbytes = (nbytes + TOTAL_OVERHEAD + MIN_POWER_2) & ~MIN_POWER_2;

	/* Find the appropriate bucket for this block size.
	 */
	bucket = 0;
  	shiftr = (nbytes - 1) >> (MIN_POWER_2 - 1);
	/* apart from this loop, this is O(1) */
  	while (shiftr >>= 1)
  		bucket++;

	/* Check the appropriate hash bucket.  If there is nothing in it,
	 * then first attempt to allocate more memory from the system, and
	 * if that fails try to grab memory from higher buckets as a last
	 * resort.  This last resort if it works will result in a larger
	 * amount of memory than is needed, but can allow the calling
	 * program to continue longer.
	 */
  	if ((p = nextf[bucket]) == NULL) {
  		morecore(bucket);
		while ((p = nextf[bucket]) == NULL)
			if ( ++bucket == NBUCKETS )
				return(NULL);
	}

	/* Remove from linked list
	 */
  	nextf[bucket] = p->ov_next;
	p->ov_magic = MAGIC;
	p->ov_index= bucket;
#ifdef MSTATS
  	nmalloc[bucket]++;
#endif
#ifdef RCHECK
	/*
	 * Record allocated size of block and
	 * bound space with magic numbers.
	 */
  	if (nbytes <= 0x10000)
		p->ov_size = nbytes - 1;
	p->ov_rmagic = RMAGIC;
  	*((u_int *)((caddr_t)p + nbytes - RSLOP)) = RMAGIC;
#endif
  	return ((char *)(p + 1));
}

/*
 * Allocate more memory to the indicated bucket.
 */
static
morecore(bucket)
	register int bucket;
{
  	register union overhead *op;
	register unsigned allocation_size ;/* amount of memmory to allocate */
	register unsigned block_size ;	   /* size of blocks to allocate    */
	register int n_blocks ;		   /* # of blocks to allocate       */
	register int power_of_2 ;

	/* Since the minimum block size which can be allocated is 
	 * (1 << MIN_POWER_2), the actual power of 2 which a bucket
	 * represents is MIN_POWER_2 greater than the bucket number.
	 */
	power_of_2 = bucket + MIN_POWER_2 ;
	block_size = 1 << power_of_2 ;

	/* Assume the minimum allocation size but if the block size is
	 * greater, use it.  Then see how many blocks we will allocate.
	 */
	allocation_size = MIN_ALLOC_SIZE ;
	if (block_size > allocation_size)
		allocation_size = block_size ;
	n_blocks = allocation_size >> power_of_2 ;

	/* Before doing the allocation, see if the memory being allocated
	 * will be aligned on a page boundary.  If not allocate enough
	 * memory to boost it up to a page boundary.
	 */
	if (op = (union overhead *)((int)sbrk(0) & PAGE_BOUND_MASK))
		sbrk(PAGE_SIZE - (int)op);

	/* Now allocate the needed memory and if the allocation didn't
	 * succeed just return.
	 */
	if ( (int)(op = (union overhead *)sbrk(allocation_size)) == -1)
  		return;

	/*
	 * Put new memory allocated  on free list for this hash bucket.
	 */
  	nextf[bucket] = op;
  	while (--n_blocks > 0) {
		op->ov_next = (union overhead *)((int)op + block_size);
		op = (union overhead *)((int)op + block_size);
  	}

	/* Make sure final block points to NULL
	 */
	op->ov_next = NULL ;
}

free(cp)
	char *cp;
{   
  	register int size;
	register union overhead *op;

  	if (cp == NULL)
  		return;
	op = (union overhead *)cp - 1;	/* Backup pointer to the overhead */
#ifdef debug
  	ASSERT(op->ov_magic == MAGIC);		/* make sure it was in use */
#else
	if (op->ov_magic != MAGIC)
		return;				/* sanity */
#endif
#ifdef RCHECK
  	ASSERT(op->ov_rmagic == RMAGIC);
	if (op->ov_index <= 13)
		ASSERT(*(u_int *)((caddr_t)op + op->ov_size + 1 - RSLOP) == RMAGIC);
#endif
  	ASSERT(op->ov_index < NBUCKETS);
  	size = op->ov_index;
	op->ov_next = nextf[size];
  	nextf[size] = op;
#ifdef MSTATS
  	nmalloc[size]--;
#endif
}

/*
 * When a program attempts "storage compaction" as mentioned in the
 * old malloc man page, it realloc's an already freed block.  Usually
 * this is the last block it freed; occasionally it might be farther
 * back.  We have to search all the free lists for the block in order
 * to determine its bucket: 1st we make one pass thru the lists
 * checking only the first block in each; if that fails we search
 * ``realloc_srchlen'' blocks in each list for a match (the variable
 * is extern so the caller can modify it).  If that fails we just copy
 * however many bytes was given to realloc() and hope it's not huge.
 */
int realloc_srchlen = -1;	/* -1 will cause the whole list to be
				 * searched.  Small integer values can
				 * be used to limit the search at the
				 * risk of not finding the pointer to
				 * be realloc()'d.		0001
				 */

char *
realloc(cp, nbytes)
	char *cp; 
	unsigned nbytes;
{   
  	register u_int onb;
	union overhead *op;
  	char *res;
	register int i;
	int was_alloced = 0;

  	if (cp == NULL)
  		return (malloc(nbytes));

  	if (nbytes == 0)
	{
  		free(cp);
  		return;
	}

	/* Backup pointer to the overhead
	 */
	op = (union overhead *)cp -1;

	/* See if allocated
	 */
	if (op->ov_magic == MAGIC) {
		was_alloced++;
		i = op->ov_index;
	} else {
		/*
		 * Already free, doing "compaction".
		 *
		 * Search for the old block of memory on the
		 * free list.  First, check the most common
		 * case (last element free'd), then (this failing)
		 * the last ``realloc_srchlen'' items free'd.
		 * If all lookups fail, then assume the size of
		 * the memory block being realloc'd is the
		 * smallest possible.
		 */
		if ((i = findbucket(op, 1)) < 0 &&
		    (i = findbucket(op, realloc_srchlen)) < 0)
			i = 0;
	}
	onb = (1 << (i + MIN_POWER_2)) - TOTAL_OVERHEAD;
	/* avoid the copy if same size block */
	if ( was_alloced &&
	     (nbytes <= onb) && 
	     ((i == 0) || ( nbytes > ((onb >> 1) - TOTAL_OVERHEAD)))
	   )
		return(cp);
  	if ((res = malloc(nbytes)) == NULL)
  		return (NULL);
  	if (cp != res)			/* common optimization */
		bcopy(cp, res, (nbytes < onb) ? nbytes : onb);
  	if (was_alloced)
		free(cp);
  	return (res);
}

/*
 * Search ``srchlen'' elements of each free list for a block whose
 * header starts at ``freep''.  If srchlen is -1 search the whole list.
 * Return bucket number, or -1 if not found.
 */
static
findbucket(freep, srchlen)
	union overhead *freep;
	int srchlen;
{
	register union overhead *p;
	register int i, j;

	for (i = 0; i < NBUCKETS; i++) {
		j = 0;
		for (p = nextf[i]; p && j != srchlen; p = p->ov_next) {
			if (p == freep)
				return (i);
			j++;
		}
	}
	return (-1);
}

#ifdef MSTATS
/*
 * mstats - print out statistics about malloc
 * 
 * Prints two lines of numbers, one showing the length of the free list
 * for each size category, the second showing the number of mallocs -
 * frees for each size category.
 */
mstats(s)
	char *s;
{
  	register int i, j;
  	register union overhead *p;
  	int totfree = 0,
  	totused = 0;

  	fprintf(stderr, "Memory allocation statistics %s\nfree:\t", s);
  	for (i = 0; i < NBUCKETS; i++) {
  		for (j = 0, p = nextf[i]; p; p = p->ov_next, j++)
  			;
  		fprintf(stderr, " %d", j);
  		totfree += j * (1 << (i + 3));
  	}
  	fprintf(stderr, "\nused:\t");
  	for (i = 0; i < NBUCKETS; i++) {
  		fprintf(stderr, " %d", nmalloc[i]);
  		totused += nmalloc[i] * (1 << (i + 3));
  	}
  	fprintf(stderr, "\n\tTotal in use: %d, total free: %d\n",
	    totused, totfree);
}
#endif
#endif vax
#ifdef mips
/* ------------------------------------------------------------------ */
/* | Copyright Unpublished, MIPS Computer Systems, Inc.  All Rights | */
/* | Reserved.  This software contains proprietary and confidential | */
/* | information of MIPS and its suppliers.  Use, disclosure or     | */
/* | reproduction is prohibited without the prior express written   | */
/* | consent of MIPS.                                               | */
/* ------------------------------------------------------------------ */
/* $Header: malloc.c,v 1.1 87/02/16 11:16:26 dce Exp $ */

/*
 * malloc.c (Caltech) 2/21/82
 * Chris Kingsley, kingsley@cit-20.
 *
 * This is a very fast storage allocator.  It allocates blocks of a small 
 * number of different sizes, and keeps free lists of each size.  Blocks that
 * don't exactly fit are passed up to the next larger size.  In this 
 * implementation, the available sizes are 2^n-4 (or 2^n-10) bytes long.
 * This is designed for use in a virtual memory environment.
 */

#include <sys/types.h>

#define	NULL 0

/*
 * The overhead on a block is at least 4 bytes.  When free, this space
 * contains a pointer to the next free block, and the bottom two bits must
 * be zero.  When in use, the first byte is set to MAGIC, and the second
 * byte is the size index.  The remaining bytes are for alignment.
 * If range checking is enabled then a second word holds the size of the
 * requested block, less 1, rounded up to a multiple of sizeof(RMAGIC).
 * The order of elements is critical: ov_magic must overlay the low order
 * bits of ov_next, and ov_magic can not be a valid ov_next bit pattern.
 */
union	overhead {
	union	overhead *ov_next;	/* when free */
	struct {
		u_char	ovu_magic;	/* magic number */
		u_char	ovu_index;	/* bucket # */
#ifdef RCHECK
		u_short	ovu_rmagic;	/* range magic number */
		u_int	ovu_size;	/* actual block size */
#endif
	} ovu;
#define	ov_magic	ovu.ovu_magic
#define	ov_index	ovu.ovu_index
#define	ov_rmagic	ovu.ovu_rmagic
#define	ov_size		ovu.ovu_size
};

#define	MAGIC		0xef		/* magic # on accounting info */
#define RMAGIC		0x5555		/* magic # on range info */

#ifdef RCHECK
#define	RSLOP		sizeof (u_short)
#else
#define	RSLOP		0
#endif

/*
 * nextf[i] is the pointer to the next free block of size 2^(i+3).  The
 * smallest allocatable block is 8 bytes.  The overhead information
 * precedes the data area returned to the user.
 */
#define	NBUCKETS 30
static	union overhead *nextf[NBUCKETS];
extern	char *sbrk();

static	int pagesz;			/* page size */
static	int pagebucket;			/* page size bucket */

#ifdef MSTATS
/*
 * nmalloc[i] is the difference between the number of mallocs and frees
 * for a given block size.
 */
static	u_int nmalloc[NBUCKETS];
#include <stdio.h>
#endif

#if defined(DEBUG) || defined(RCHECK)
#define	ASSERT(p)   if (!(p)) botch("p")
#include <stdio.h>
static
botch(s)
	char *s;
{
	fprintf(stderr, "\r\nassertion botched: %s\r\n", s);
 	(void) fflush(stderr);		/* just in case user buffered it */
	abort();
}
#else
#define	ASSERT(p)
#endif

char *
malloc(nbytes)
	unsigned nbytes;
{
  	register union overhead *op;
  	register int bucket;
	register unsigned amt, n;

	/*
	 * First time malloc is called, setup page size and
	 * align break pointer so all data will be page aligned.
	 */
	if (pagesz == 0) {
		pagesz = n = getpagesize();
		op = (union overhead *)sbrk(0);
  		n = n - sizeof (*op) - ((int)op & (n - 1));
		if (n < 0)
			n += pagesz;
  		if (n) {
  			if (sbrk(n) == (char *)-1)
				return (NULL);
		}
		bucket = 0;
		amt = 8;
		while (pagesz > amt) {
			amt <<= 1;
			bucket++;
		}
		pagebucket = bucket;
	}
	/*
	 * Convert amount of memory requested into closest block size
	 * stored in hash buckets which satisfies request.
	 * Account for space used per block for accounting.
	 */
	if (nbytes <= (n = pagesz - sizeof (*op) - RSLOP)) {
#ifndef RCHECK
		amt = 8;	/* size of first bucket */
		bucket = 0;
#else
		amt = 16;	/* size of first bucket */
		bucket = 1;
#endif
		n = -(sizeof (*op) + RSLOP);
	} else {
		amt = pagesz;
		bucket = pagebucket;
	}
	while (nbytes > amt + n) {
		amt <<= 1;
		if (amt == 0)
			return (NULL);
		bucket++;
	}
	/*
	 * If nothing in hash bucket right now,
	 * request more memory from the system.
	 */
  	if ((op = nextf[bucket]) == NULL) {
  		morecore(bucket);
  		if ((op = nextf[bucket]) == NULL)
  			return (NULL);
	}
	/* remove from linked list */
  	nextf[bucket] = op->ov_next;
	op->ov_magic = MAGIC;
	op->ov_index = bucket;
#ifdef MSTATS
  	nmalloc[bucket]++;
#endif
#ifdef RCHECK
	/*
	 * Record allocated size of block and
	 * bound space with magic numbers.
	 */
	op->ov_size = (nbytes + RSLOP - 1) & ~(RSLOP - 1);
	op->ov_rmagic = RMAGIC;
  	*(u_short *)((caddr_t)(op + 1) + op->ov_size) = RMAGIC;
#endif
  	return ((char *)(op + 1));
}

/*
 * Allocate more memory to the indicated bucket.
 */
morecore(bucket)
	int bucket;
{
  	register union overhead *op;
	register int sz;		/* size of desired block */
  	int amt;			/* amount to allocate */
  	int nblks;			/* how many blocks we get */

	/*
	 * sbrk_size <= 0 only for big, FLUFFY, requests (about
	 * 2^30 bytes on a VAX, I think) or for a negative arg.
	 */
	sz = 1 << (bucket + 3);
#ifdef DEBUG
	ASSERT(sz > 0);
#else
	if (sz <= 0)
		return;
#endif
	if (sz < pagesz) {
		amt = pagesz;
  		nblks = amt / sz;
	} else {
		amt = sz + pagesz;
		nblks = 1;
	}
	op = (union overhead *)sbrk(amt);
	/* no more room! */
  	if ((int)op == -1)
  		return;
	/*
	 * Add new memory allocated to that on
	 * free list for this hash bucket.
	 */
  	nextf[bucket] = op;
  	while (--nblks > 0) {
		op->ov_next = (union overhead *)((caddr_t)op + sz);
		op = (union overhead *)((caddr_t)op + sz);
  	}
}

free(cp)
	char *cp;
{   
  	register int size;
	register union overhead *op;

  	if (cp == NULL)
  		return;
	op = (union overhead *)((caddr_t)cp - sizeof (union overhead));
#ifdef DEBUG
  	ASSERT(op->ov_magic == MAGIC);		/* make sure it was in use */
#else
	if (op->ov_magic != MAGIC)
		return;				/* sanity */
#endif
#ifdef RCHECK
  	ASSERT(op->ov_rmagic == RMAGIC);
	ASSERT(*(u_short *)((caddr_t)(op + 1) + op->ov_size) == RMAGIC);
#endif
  	size = op->ov_index;
  	ASSERT(size < NBUCKETS);
	op->ov_next = nextf[size];	/* also clobbers ov_magic */
  	nextf[size] = op;
#ifdef MSTATS
  	nmalloc[size]--;
#endif
}

/*
 * When a program attempts "storage compaction" as mentioned in the
 * old malloc man page, it realloc's an already freed block.  Usually
 * this is the last block it freed; occasionally it might be farther
 * back.  We have to search all the free lists for the block in order
 * to determine its bucket: 1st we make one pass thru the lists
 * checking only the first block in each; if that fails we search
 * ``realloc_srchlen'' blocks in each list for a match (the variable
 * is extern so the caller can modify it).  If that fails we just copy
 * however many bytes was given to realloc() and hope it's not huge.
 */
int realloc_srchlen = 4;	/* 4 should be plenty, -1 =>'s whole list */

char *
realloc(cp, nbytes)
	char *cp; 
	unsigned nbytes;
{   
  	register u_int onb, i;
	union overhead *op;
  	char *res;
	int was_alloced = 0;

  	if (cp == NULL)
  		return (malloc(nbytes));

  	if (nbytes == 0)
	{
  		free(cp);
  		return;
	}

	op = (union overhead *)((caddr_t)cp - sizeof (union overhead));
	if (op->ov_magic == MAGIC) {
		was_alloced++;
		i = op->ov_index;
	} else {
		/*
		 * Already free, doing "compaction".
		 *
		 * Search for the old block of memory on the
		 * free list.  First, check the most common
		 * case (last element free'd), then (this failing)
		 * the last ``realloc_srchlen'' items free'd.
		 * If all lookups fail, then assume the size of
		 * the memory block being realloc'd is the
		 * largest possible (so that all "nbytes" of new
		 * memory are copied into).  Note that this could cause
		 * a memory fault if the old area was tiny, and the moon
		 * is gibbous.  However, that is very unlikely.
		 */
		if ((i = findbucket(op, 1)) < 0 &&
		    (i = findbucket(op, realloc_srchlen)) < 0)
			i = NBUCKETS;
	}
	onb = 1 << (i + 3);
	if (onb < pagesz)
		onb -= sizeof (*op) + RSLOP;
	else
		onb += pagesz - sizeof (*op) - RSLOP;
	/* avoid the copy if same size block */
	if (was_alloced) {
		if (i) {
			i = 1 << (i + 2);
			if (i < pagesz)
				i -= sizeof (*op) + RSLOP;
			else
				i += pagesz - sizeof (*op) - RSLOP;
		}
		if (nbytes <= onb && nbytes > i) {
#ifdef RCHECK
			op->ov_size = (nbytes + RSLOP - 1) & ~(RSLOP - 1);
			*(u_short *)((caddr_t)(op + 1) + op->ov_size) = RMAGIC;
#endif
			return(cp);
		} else
			free(cp);
	}
  	if ((res = malloc(nbytes)) == NULL)
  		return (NULL);
  	if (cp != res)		/* common optimization if "compacting" */
		bcopy(cp, res, (nbytes < onb) ? nbytes : onb);
  	return (res);
}

/*
 * Search ``srchlen'' elements of each free list for a block whose
 * header starts at ``freep''.  If srchlen is -1 search the whole list.
 * Return bucket number, or -1 if not found.
 */
static
findbucket(freep, srchlen)
	union overhead *freep;
	int srchlen;
{
	register union overhead *p;
	register int i, j;

	for (i = 0; i < NBUCKETS; i++) {
		j = 0;
		for (p = nextf[i]; p && j != srchlen; p = p->ov_next) {
			if (p == freep)
				return (i);
			j++;
		}
	}
	return (-1);
}

#ifdef MSTATS
/*
 * mstats - print out statistics about malloc
 * 
 * Prints two lines of numbers, one showing the length of the free list
 * for each size category, the second showing the number of mallocs -
 * frees for each size category.
 */
mstats(s)
	char *s;
{
  	register int i, j;
  	register union overhead *p;
  	int totfree = 0,
  	totused = 0;

  	fprintf(stderr, "Memory allocation statistics %s\nfree:\t", s);
  	for (i = 0; i < NBUCKETS; i++) {
  		for (j = 0, p = nextf[i]; p; p = p->ov_next, j++)
  			;
  		fprintf(stderr, " %d", j);
  		totfree += j * (1 << (i + 3));
  	}
  	fprintf(stderr, "\nused:\t");
  	for (i = 0; i < NBUCKETS; i++) {
  		fprintf(stderr, " %d", nmalloc[i]);
  		totused += nmalloc[i] * (1 << (i + 3));
  	}
  	fprintf(stderr, "\n\tTotal in use: %d, total free: %d\n",
	    totused, totfree);
}
#endif
#endif mips
