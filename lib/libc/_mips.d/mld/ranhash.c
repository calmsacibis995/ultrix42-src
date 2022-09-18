/* --------------------------------------------------------- */
/* | Copyright (c) 1986, 1989 MIPS Computer Systems, Inc.  | */
/* | All Rights Reserved.                                  | */
/* --------------------------------------------------------- */
/* $Header: ranhash.c,v 2010.2.1.3 89/11/29 14:28:24 bettina Exp $ */
/*
 * DATE:
 * MODULE:	ldhash.c
 * PURPOSE:	provide a common set of routines to access the hash table in
 *		an archive symdef file.
 *
 * INTERFACES:
 *		ranhashinit (size)	size used in future hash ops
 *		ranhash (name)		return the initial hash value
 *		ranlookup (pran, name)	lookup the name in pran or return empty
 *					bucket.
 */

#include <stdio.h>
#include "ar.h"
#define BITSPERWORD 32

static int hsize;		/* current log used in hash */
static int hlog;		/* current log used in hash */
static int rehash;		/* hash search increment from ranhash */
static struct ranlib	*phran;	/* current ranlib hash table */
static char		*phstr;	/* current string table */

int ranhashinit (pran, pstr, size)

struct ranlib	*pran;	/* current ranlib hash table */
char		*pstr;	/* current string table */
register int size;

{
	register int i;

	for (i = -1; size; i++, size >>= 1)
	    if ((size & 1) && (size & ~1))
		return (1);
	    
	hlog = i;
	hsize = (1 << i);
	phran = pran;
	phstr = pstr;
	return (0);

} /* ranhashinit */
		

ranhash(s)
register unsigned char *s;
{
	register unsigned long h;
	register unsigned long c;

	/* Convert string to a word by summing characters with a 5-bit
	   rotate.  The rotate gives maximum use of the information in
	   each character (only 5 bits of which are significant on the
	   average) by not overlaying real bits (and thus destroying
	   information) until the seventh character, while at the same
	   time being relatively prime to the word size, thus delaying
	   XY = YX problems to X and Y 32 characters apart. */
	h = *s++;
	while ((c = *s++) != '\0') {
	    h = (h >> (BITSPERWORD - 5)) + (h << 5) + c;
	}
	/* Now really hash the word by multiplicative hashing to spread
	   out the consecutive values in each 5-bit position and take
	   the high bits (which contain contributions from all lower
	   bit positions). */
	h *= 0x9dd68ab5;
	rehash = (h & (hsize-1)) | 1;
	return (h >> (BITSPERWORD - hlog));
}


struct ranlib *
ranlookup(name)

char		*name;

{
    register struct ranlib *ran, *iran;
    register struct ranlib *ranmax = phran + hsize;

    ran = phran + ranhash (name);
    iran = ran;
    do {
	    if (ran->ran_off == 0) {
		    return (ran);
	    } else if (!strcmp (name, phstr + ran->ran_un.ran_strx)) {
		    return (ran);
	    } /* if */
	    ran += rehash;
	    if (ran >= ranmax)
		    ran -= hsize;
    } while (ran != iran);

    fprintf (stderr, "hash table overflow: internal error, ihash = %d, rehash = %d, hashsize = %d.\n", iran - phran, rehash, hsize);
    return ((struct ranlib *)0);
} /* ranlookup */
