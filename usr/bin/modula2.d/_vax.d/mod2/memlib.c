/*#@(#)memlib.c	4.1	Ultrix	7/17/90*/
/****************************************************************************
 *									    *
 *  Copyright (c) 1984 by						    *
 *  DIGITAL EQUIPMENT CORPORATION, Maynard, Massachusetts.		    *
 *  All rights reserved.						    *
 * 									    *
 *  This software is furnished under a license and may be used and copied   *
 *  only in  accordance with  the  terms  of  such  license  and with the   *
 *  inclusion of the above copyright notice. This software or  any  other   *
 *  copies thereof may not be provided or otherwise made available to any   *
 *  other person.  No title to and ownership of  the  software is  hereby   *
 *  transferred.							    *
 * 									    *
 *  The information in this software is  subject to change without notice   *
 *  and  should  not  be  construed as  a commitment by DIGITAL EQUIPMENT   *
 *  CORPORATION.							    *
 * 									    *
 *  DIGITAL assumes no responsibility for the use  or  reliability of its   *
 *  software on equipment which is not supplied by DIGITAL.		    *
 * 									    *
$Header: memlib.c,v 1.3 84/05/19 11:41:55 powell Exp $
 ****************************************************************************/
/******************
MODIFIED TO LOOK LIKE Pascal NEW/DISPOSE
******************/
#include <stdio.h>
/*
 *	Next-fit storage allocation mechanism
 *
 *	Algorithm and variable names stolen from Knuth V1, p 437,
 *		with suggested modifications from Exercise 2.5.6.
 *
 *	Storage is manipulated in terms of UNITs, the amount needed
 *		to store a pointer and length.  Requests are always
 *		rounded up to multiples of UNITs.
 */
extern int _minptr, _maxptr;
#define ELEMENT		struct element
#define MINSBRK		4096		/* minimum sbrk = 4096 UNITs */
#define	UNIT		sizeof(ELEMENT)
#define	WORDSIZE	1		/* request is in this size words */
ELEMENT {
	ELEMENT *link;
	int size;
};

/*
 *	free list is kept in address order
 *	avail is a dummy header that points to first free element
 *	rover is pointer into free list
 */

static ELEMENT avail;
static ELEMENT *rover = &avail;

/*
 *	runtime__allocate(p0,n) sets p0 to point to n WORDS of storage
 */

NEW(p0,n) ELEMENT **p0; int n; {
	register ELEMENT *p, *q;
	register int size;
	int increment;
	int repeat;
	size = (n * WORDSIZE + UNIT - 1) / UNIT;
	if (rover == NULL) rover = &avail;
	q = rover;
	p = q->link;
	/* outer loop executed at most twice */
	repeat = 0;
	do {
		/* search for a block large enough */
		while ((p != NULL) && (p->size < size)) {
			/* keep looking */
			q = p;
			p = p->link;
		}
		if ((p == NULL) && !repeat) {
			/* if first time, one more chance */
			q = &avail;
			p = q->link;
			repeat = 1;
		} else {
			repeat = 0;
		}
	} while (repeat);
	if (p == NULL) {
		/* out of memory, get some more */
		increment = (size<MINSBRK) ? MINSBRK : size;
		p = (ELEMENT *)sbrk(increment*UNIT);
		/* pretend like we found it */
		p->link = NULL;
		p->size = increment;
		q->link = p;
	}
	if (p->size == size) {
		/* found one of right size. remove it */
		q->link = p->link;
	} else if (p->size > size) {
		/* found one too big. take part of it */
		p->size -= size;
		p += p->size;
	}
	rover = q->link;
	if (((int)p) < _minptr) _minptr = ((int)p);
	if (((int)p)+n > _maxptr) _maxptr = ((int)p)+n;
	*p0 = p;
}

/*
 *	runtime__deallocate(p0,n) adds the block of n words pointed to by p0
 *		 to the free list
 */

DISPOSE(p0,n) ELEMENT **p0; int n; {
	register ELEMENT *p, *q, *r;

	n = (n * WORDSIZE + UNIT - 1) / UNIT;
	q = &avail;
	r = *p0;
	p = q->link;
	/* search for the right place */
	while ((p != NULL) && (p < r)) {
		/* not the place, keep searching */
		q = p;
		p = p->link;
	}
	/* this is where it should go */
	/* note: since NULL = 0, if p = NULL, p != r + n */
	if (p == r + n) {
		/* new block abuts p, consolidate */
		n += p->size;
		r->link = p->link;
	} else {
		/* does not abut, just connect */
		r->link = p;
	}
	if (r == q + q->size) {
		/* new block abuts q, consolidate */
		q->size += n;
		q->link = r->link;
	} else {
		/* does not abut, just connect */
		q->link = r;
		r->size = n;
	}
	rover = q;	/* start searching here next time */
	*p0 = (ELEMENT *)0xf0f0f0f0;
}
