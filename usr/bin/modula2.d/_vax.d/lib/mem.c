/*#@(#)mem.c	4.1	Ultrix	7/17/90*/
/* $Header: mem.c,v 1.5 84/05/19 11:39:54 powell Exp $ */
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

#define ELEMENT		struct element
#define MINSBRK		4096		/* minimum sbrk = 4096 UNITs */
#define	UNIT		sizeof(ELEMENT)
#define	BYTESIZE	8		/* Number of bits in a byte */
#define	WORDSIZE	32		/* Number of bits in a word */
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
 *	Storage_ALLOCATE(p0,n) sets p0 to point to n WORDS of storage
 */

Storage_ALLOCATE(p0,n) ELEMENT **p0; int n; {
	register ELEMENT *p, *q;
	register int size;
	int increment;
	int repeat;
	if (n <= 0) {
	    fprintf(stderr,"Storage_ALLOCATE: non-positive size\n");
	    abort();
	}
	size = ((n+BYTESIZE-1)/BYTESIZE + UNIT - 1) / UNIT;
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
		if ((int)p == -1) {
		    perror("Storage_ALLOCATE: sbrk error");
		    abort();
		}
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
	*p0 = p;
}

/*
 *	Storage_DEALLOCATE(p0,n) adds the block of n bits pointed to by p0
 *		 to the free list
 */

Storage_DEALLOCATE(p0,n) ELEMENT **p0; int n; {
	register ELEMENT *p, *q, *r;

	if (n <= 0) {
	    fprintf(stderr,"Storage_DEALLOCATE: non-positive size\n");
	    abort();
	}
	n = ((n+BYTESIZE-1)/BYTESIZE + UNIT - 1) / UNIT;
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
	*p0 = (ELEMENT *)0;
}
Storage__init(){}

/* interface to C allocation routine */
modmalloc(p,s) char **p; int s; {
    *p = (char *)malloc(s);
}
MEMORY__init(){}
MEMORY_ALLOCATE(p0,n) ELEMENT **p0; int n; {
	ELEMENT *p;
	Storage_ALLOCATE(&p,n+WORDSIZE);
	*p0 = (ELEMENT*)(((int)p)+WORDSIZE/BYTESIZE);
	p->link = *p0;
}
MEMORY_DEALLOCATE(p0,n) ELEMENT **p0; int n; {
	ELEMENT *p;
	p = (ELEMENT*)(((int)*p0)-WORDSIZE/BYTESIZE);
	if (*p0 != p->link) {
	    fprintf(stderr,"MEMORY_DEALLOCATE: Invalid pointer specified\n");
	    abort();
	}
	Storage_DEALLOCATE(&p,n-WORDSIZE);
	*p0 = 0;
}
