/*	@(#)spell.h	1.2		(ULTRIX)	8/13/85		*/
/************************************************************************
 *									*
 *			Copyright (c) 1985 by				*
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
 *
 *			Modification History
 *
 *	Stephen Reilly, 13-Aug-85
 * 	Changed the "ri" in fopen to "r" because the "i" option does
 *	not exist.
 *
 *	@(#)spell.h	4.1	12/18/82
 ***********************************************************************/

#include <stdio.h>
#include <ctype.h>

#ifndef unix
#define SHIFT	5
#define TABSIZE (int)(400000/(1<<SHIFT))
int	*tab;	/*honeywell loader deficiency*/
#else
#define Tolower(c)	(isupper(c)?tolower(c):c) /* ugh!!! */
#define SHIFT	4
#define TABSIZE 25000	/*(int)(400000/(1<<shift))--pdp11 compiler deficiency*/
short	tab[TABSIZE];
#endif
long	p[] = {
	399871,
	399887,
	399899,
	399911,
	399913,
	399937,
	399941,
	399953,
	399979,
	399983,
	399989,
};
#define	NP	(sizeof(p)/sizeof(p[0]))
#define	NW	30

/*
* Hash table for spelling checker has n bits.
* Each word w is hashed by k different (modular) hash functions, hi.
* The bits hi(w), i=1..k, are set for words in the dictionary.
* Assuming independence, the probability that no word of a d-word
* dictionary sets a particular bit is given by the Poisson formula
* P = exp(-y)*y**0/0!, where y=d*k/n.
* The probability that a random string is recognized as a word is then
* (1-P)**k.  For given n and d this is minimum when y=log(2), P=1/2,
* whence one finds, for example, that a 25000-word dictionary in a
* 400000-bit table works best with k=11.
*/

long	pow2[NP][NW];

prime(argc, argv) register char **argv;
{
	int i, j;
	long h;
	register long *lp;

#ifndef unix
	if ((tab = (int *)calloc(sizeof(*tab), TABSIZE)) == NULL)
		return(0);
#endif
	if (argc > 1) {
		FILE *f;
		if ((f = fopen(argv[1], "r")) == NULL)
			return(0);
		if (fread((char *)tab, sizeof(*tab), TABSIZE, f) != TABSIZE)
			return(0);
		fclose(f);
	}
	for (i=0; i<NP; i++) {
		h = *(lp = pow2[i]) = 1<<14;
		for (j=1; j<NW; j++)
			h = *++lp = (h<<7) % p[i];
	}
	return(1);
}

#define get(h)	(tab[h>>SHIFT]&(1<<((int)h&((1<<SHIFT)-1))))
#define set(h)	tab[h>>SHIFT] |= 1<<((int)h&((1<<SHIFT)-1))
