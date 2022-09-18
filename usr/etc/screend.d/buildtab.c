#ifndef lint
static char *sccsid = "@(#)buildtab.c	4.2	(ULTRIX)	10/12/90";
#endif
/*
 * buildtab.c
 *
 * Build screen tables from individual entries
 *
 * Modification history:
 *
 * 19 December 1988	Jeffrey Mogul/DECWRL
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
#include <netinet/in.h>
#include <stdio.h>
#include "screentab.h"

int	debug_build = 0;
extern int semantic_errors;

InitTables()
{
	InitNetmaskTable();
	InitActionTable();
}

StoreNetmask(nmp)
register struct NetmaskData *nmp;
{
	if (debug_build)
	    PrintNetmaskData(nmp);
	NetMaskInsert(nmp);
}

StoreAction(ap)
register struct ActionSpec *ap;
{
	if (debug_build)
	    PrintActionSpec(ap);
	ActionInsert(ap);
}

/*
 * Netmask stuff is stored in a hash table where the "hash" function
 * is trivial, because we don't expect to have many entries.
 */

#define	CHEAT(ia)	(ia).S_un.S_un_b
#define	HASHSIZE (1<<8)		/* 8 bits of result */
#define	HASHFUNC(ia) \
	(CHEAT(ia).s_b1 ^ CHEAT(ia).s_b2 ^ CHEAT(ia).s_b3 ^ CHEAT(ia).s_b4)

struct nm_hashentry {
	struct NetmaskData nmdata;
	struct nm_hashentry *next;	/* chain of entries */
};

struct nm_hashentry *nm_hashtable[HASHSIZE];

InitNetmaskTable()
{
	bzero(nm_hashtable, sizeof(nm_hashtable));
}

NetMaskInsert(nmp)
register struct NetmaskData *nmp;
{
	register int i;
	register struct nm_hashentry *hp;
	register struct nm_hashentry *hep;
	
	i = HASHFUNC(nmp->network);
	hp = (struct nm_hashentry *)malloc(sizeof(*hp));
	if (hp == NULL) {
	    perror("NetMaskInsert/malloc");
	    exit(1);
	}
	hp->nmdata = *nmp;
	hp->next = NULL;

	if ((hep = nm_hashtable[i]) == NULL) {	/* easy */
	    nm_hashtable[i] = hp;
	    return;
	}
	
	/* collision, must search for end of chain */
	do {
	    if (hep->nmdata.network.s_addr == nmp->network.s_addr) {
		yyerror("duplicate netmask information for network");
		semantic_errors++;
		return;
	    }
	    if (hep->next == NULL)
		break;
	    hep = hep->next;
	} while (1);
	
	hep->next = hp;
}

/*
 * Returns value by reference; returns true iff success
 */
NetMaskLookup(netp, maskp)
register struct in_addr *netp;
register struct in_addr *maskp;
{
	register int i;
	register struct nm_hashentry *hp;

	i = HASHFUNC(*netp);
	hp = nm_hashtable[i];
	
	while (hp) {
	    if (hp->nmdata.network.s_addr == netp->s_addr) {
		*maskp = hp->nmdata.mask;
		return(1);
	    }
	    hp = hp->next;
	}
	return(0);	/* not found */
}

DumpNetMaskTable()
{
	register int i;
	register struct nm_hashentry *hp;

	printf("Netmask hash table:\n");
	for (i = 0; i < HASHSIZE; i++) {
	    if ((hp = nm_hashtable[i]) == NULL)
		continue;
	    printf("[hash %d] ", i);
	    while (hp) {
		printf(" (%s, ", inet_ntoa(hp->nmdata.network));
		printf("%s)", inet_ntoa(hp->nmdata.mask));
		hp = hp->next;
	    }
	    printf("\n");
	}
}
