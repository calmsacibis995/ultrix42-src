#ifndef lint
static char *sccsid = "@(#)screenit.c	4.2	(ULTRIX)	10/12/90";
#endif
/*
 * screenit.c
 *
 * Decide if a datagram shall pass.
 *
 * Modification history:
 *
 * 21 December 1988	Jeffrey Mogul/DECWRL
 *	Created.
 *
 */
/*
 *               Copyright 1989, 1990 Digital Equipment Corporation
 *                          All Rights Reserved
 * 
 * 
 * Permission to use, copy, and modify this software and its documentation
 * is hereby granted only under the following terms and conditions.  Both
 * the above copyright notice and this permission notice must appear in
 * all copies of the software, derivative works or modified versions, and
 * any portions threof, and both notices must appear in supporting
 * documentation.
 * 
 * Users of this software agree to the terms and conditions set forth herein,
 * and hereby grant back to Digital a non-exclusive, unrestricted, royalty-free
 * right and license under any changes, enhancements or extensions made to the
 * core functions of the software, including but not limited to those affording
 * compatibility with other hardware or software environments, but excluding
 * applications which incorporate this software.  Users further agree to use
 * their best efforts to return to Digital any such changes, enhancements or
 * extensions that they make and inform Digital of noteworthy uses of this
 * software.  Correspondence should be provided to Digital at:
 * 
 *                       Director of Licensing
 *                       Western Research Laboratory
 *                       Digital Equipment Corporation
 *                       100 Hamilton Avenue
 *                       Palo Alto, California  94301  
 * 
 * Comments and bug reports may also be sent using electronic mail to:
 * 			screend-reports@decwrl.dec.com
 * 
 * 	>> This software may NOT be distributed to third parties. <<
 *   
 * THE SOFTWARE IS PROVIDED "AS IS" AND DIGITAL EQUIPMENT CORP. DISCLAIMS
 * ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING ALL IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS.   IN NO EVENT SHALL DIGITAL
 * EQUIPMENT CORPORATION BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF
 * USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

#include <sys/types.h>
#include <netinet/in_systm.h>
#include <netinet/in.h>
#include <stdio.h>
#include "screentab.h"

extern int debug;
extern int LastMatchRule;
int disable_cache = 0;
int LastCacheHit = 0;		/* for logging cache hits */

/*
 * A small lookaside cache for complete matches
 */

#define	CACHESIZE	16

struct cache_entry {
	struct unpacked_hdrs	hdrs;
	int			action;
	int			age;
	int			rule;
};

struct cache_entry Cache[CACHESIZE];
		/* ASSUMPTION: initialized to all zeros */

u_long cache_time = 0;	/* pseudo-age for LRU */

ScreenIt(ahp)
register struct annotated_hdrs *ahp;
{
	register int i;
	register struct cache_entry *cep;
	register struct cache_entry *eldest;
	register int action;
	
	if (debug)
	    printf("checking cache:\n");
	cep = Cache;
	eldest = Cache;
	for (i = 0; i < CACHESIZE; i++, cep++) {
	    /* keep track of oldest entry */
	    if (cep->age < eldest->age)
		eldest = cep;

	    if (debug) {
		printf("\tage %d ", cep->age);
		PrintUnpackedHdrs(&(cep->hdrs));
		printf("\n");
	    }
	    /* check for match on all components */
	    if (cep->hdrs.src.addr.s_addr != ahp->hdrs.src.addr.s_addr)
		continue;
	    if (cep->hdrs.dst.addr.s_addr != ahp->hdrs.dst.addr.s_addr)
		continue;
	    if (cep->hdrs.proto != ahp->hdrs.proto)
		continue;
	    /* next two may not always be valid but they always must match */
	    if (cep->hdrs.src.port != ahp->hdrs.src.port)
		continue;
	    if (cep->hdrs.dst.port != ahp->hdrs.dst.port)
		continue;

	    /* got here = match */
	    if (debug) {
		printf("match\n");
	    }
	    LastCacheHit = 1;
	    cep->age = cache_time++;	/* update LRU timer */
	    LastMatchRule = cep->rule;	/* restore rule info */
	    return(cep->action);
	}
	
	if (debug) {
	    printf("no match\n");
	}
	LastCacheHit = 0;

	/* got here = not in cache */
	action = MatchAction(ahp);
	
	if (disable_cache == 0) {
	    /* replace eldest cache entry */
	    eldest->hdrs = ahp->hdrs;
	    eldest->action = action;
	    eldest->age = cache_time++;
	    eldest->rule = LastMatchRule;
	}
	
	return(action);
}
