#ifndef lint
static char *sccsid = "@(#)acttab.c	4.2	(ULTRIX)	10/12/90";
#endif
/*
 * acttab.c
 *
 * Manage and search the action table
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
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <stdio.h>
#include "screentab.h"

extern int debug;
extern int semantic_errors;
extern int defaction;

int LastMatchRule;

/*
 * Actions are stored in an array, in order of appearance in the
 * configuration file.
 *
 */

struct ActionSpec *acttab = 0;
int acttabsize = 0;
int acttabcount = 0;

InitActionTable()
{
	acttabsize = 1;		/* XXX change this after debugging XXX */
	acttab = (struct ActionSpec *)malloc(acttabsize * sizeof(*acttab));
	if (acttab == NULL) {
	    perror("InitActionTable/malloc");
	    exit(1);
	};
}

ExpandActionTable()
{
	register struct ActionSpec *new;
	register int newsize;
	
	newsize = acttabsize * 2;
	new = (struct ActionSpec *)malloc(newsize * sizeof(*new));
	if (new == NULL) {
	    perror("ExpandActionTable/malloc");
	    exit(1);
	}
	bcopy(acttab, new, acttabsize * sizeof(*new));
	free(acttab);
	acttab = new;
	acttabsize = newsize;
}

ActionInsert(ap)
register struct ActionSpec *ap;
{
	/* Make sure that "from" and "to" have the same proto field */
	if (ap->from.pspec.proto != ap->to.pspec.proto) {
	    if (ap->from.pspec.proto && (ap->to.pspec.proto == 0))
		ap->to.pspec.proto = ap->from.pspec.proto;
	    else if ((ap->from.pspec.proto == 0) && ap->to.pspec.proto)
		ap->from.pspec.proto = ap->to.pspec.proto;
	    else {
		yyerror("from and to must have same protocol");
		semantic_errors++;
		return;
	    }
	}

	/* A little extra hackery for ICMP (blecch!) */
	if (ap->from.pspec.proto == IPPROTO_ICMP) {
	    /* make sure that at most one ICMP code is specified */
	    if ((ap->from.pspec.pval.code) && (ap->to.pspec.pval.code) &&
		((ap->from.pspec.pval.code) != (ap->to.pspec.pval.code))) {
		yyerror("cannot specify two different ICMP types");
		semantic_errors++;
		return;
	    }
	}

	if (acttabcount >= acttabsize)
	    ExpandActionTable();
	acttab[acttabcount] = *ap;
	acttabcount++;
}

DumpActionTable()
{
	register int i;
		
	printf("Action table: (%d/%d slots full)\n",
		acttabcount, acttabsize);
	for (i = 0; i < acttabcount; i++) {
	    printf("[%d] ", i);
	    PrintActionSpec(&(acttab[i]));
	}
}

MatchAction(ahp)
register struct annotated_hdrs *ahp;
{
	register int i;
	register struct ActionSpec *asp;
	register int match;
		
	asp = acttab;
	for (i = 0; i < acttabcount; i++, asp++) {
	    if (debug) {
		printf("checking [%d] ", i);
		PrintActionSpec(asp);
	    }
	    
	    /* Check "from" ObjectSpec */
	    match = MatchObject(&(ahp->hdrs.src), ahp->hdrs.proto,
			&(ahp->srcnote), &(asp->from));
	    if (!match) {
		if (debug)
		    printf("src wrong\n");
		continue;
	    }

	    /* Check "to" ObjectSpec */
	    match = MatchObject(&(ahp->hdrs.dst), ahp->hdrs.proto,
			&(ahp->dstnote), &(asp->to));
	    if (!match) {
		if (debug)
		    printf("dst wrong\n");
		continue;
	    }

	    /* it matches! */
	    if (debug) {
		printf("match\n");
	    }
	    LastMatchRule = i;
	    return(asp->action);
	}

	/* not found */
	LastMatchRule = -1;
	return(defaction);
}

/*
 * Returns true if the header matches the object;
 * may fill in the annotation fields if necessary (lazy evaluation)
 */
MatchObject(uhp, proto, anp, osp)
register struct unpacked_hdr *uhp;
int proto;
register struct annotation *anp;
register struct ObjectSpec *osp;
{
	register int osp_port;
	register int matchval;
	
	switch (osp->aspec.addrtype) {
	case ASAT_ANY:
		matchval = 1;
		break;

	case ASAT_HOST:
		matchval = (osp->aspec.aval.host.s_addr == uhp->addr.s_addr);
		break;

	case ASAT_NET:
		if (anp->net.s_addr == INADDR_ANY) {
		    /* initialize this annotation */
		    netextract(&(uhp->addr), &(anp->net));
		}
		matchval = (osp->aspec.aval.network.s_addr == anp->net.s_addr);
		break;

	case ASAT_SUBNET:
		if (anp->net.s_addr == INADDR_ANY) {
		    /* must initialize this annotation before subnet */
		    netextract(&(uhp->addr), &(anp->net));
		}
		if (anp->subnet.s_addr == INADDR_ANY) {
		    /* now initialize this annotation */
		    subnetextract(&(uhp->addr), &(anp->net), &(anp->subnet));
		}
		matchval =
			(osp->aspec.aval.subnet.s_addr == anp->subnet.s_addr);
		break;

	default:
	    fprintf(stderr,
		"internal error: MatchObject/unknown addrtype %d\n",
			osp->aspec.addrtype);
	    exit(1);
	}
	
	/* So far, we know if the address part matches */
	if (osp->flags & OSF_NOTADDR) {	/* if looking for non-match addr */
	    if (matchval)	/* but it does match */
		return(0);	/* so return false */
	}
	else {				/* looking for matching addr */
	    if (matchval == 0)	/* but it doesn't match */
		return(0);	/* so return false */
	}
	
	if (osp->pspec.proto) {
	    /* proto is specified, check it */
	    matchval = (osp->pspec.proto == proto);
	}
	else
	    matchval = 1;	/* any protocol matches */

	/* Now we know if the protocol matches */
	if (osp->flags & OSF_NOTPROTO) { /* if looking for non-match proto */
	    if (matchval)	/* but it does match */
		return(0);	/* so return false */
	}
	else {				/* looking for matching proto */
	    if (matchval == 0)	/* but it doesn't match */
		return(0);	/* so return false */
	}

	/*
	 * Assumption here:
	 *	if this is ICMP then the code is either zero or
	 *		we should match it
	 *	if this is none of (ICMP, UDP, TCP) then port.descrim is
	 *		PORTV_ANY for both source and dest
	 * Thus, we can safely use the following code no matter what
	 *	the protocol.
	 */
	if (proto == IPPROTO_ICMP) {
	    osp_port = osp->pspec.pval.code;
	    if (osp_port == 0) {
		matchval = 1;		/* any ICMP matches */
	    }
	    else if (osp_port == -1) {
		/* is it an "infotype"? */
		matchval = ICMP_INFOTYPE(uhp->port);
	    }
	    else
		matchval = (uhp->port == osp_port);
	}
	else {
	    osp_port = osp->pspec.pval.port.value;
	    switch (osp->pspec.pval.port.discrim) {
	    case PORTV_EXACT:
		/* is it the specified port? */
		matchval = (uhp->port == osp_port);
		break;

	    case PORTV_RESERVED:
		/* is it "reserved"? */
		matchval = ((uhp->port >= 0) && (uhp->port < IPPORT_RESERVED));
		break;

	    case PORTV_XSERVER:
		/* Range comparisons must be done in host byte-order */
		osp_port = ntohs(uhp->port);
		/* is it in the range used by X-servers? */
		matchval = ((osp_port >= XSERVERPORT_MIN) &&
			     (osp_port < XSERVERPORT_MAX));
		break;

	    case PORTV_ANY:
	    default:
		matchval = 1;       /* any port in a storm */
		break;
	    }
	}

	/* Now we know if the port matches */
	if (osp->flags & OSF_NOTPORT) {	/* if looking for non-match port */
	    return(matchval == 0);	/* true if no match */
	}
	else {				/* looking for matching port */
	    return(matchval);		/* true if match */
	}
}

netextract(iap, netp)
register struct in_addr *iap;
register struct in_addr *netp;
{
	register u_long i = ntohl(iap->s_addr);
	
	if (IN_CLASSA(i))
	    netp->s_addr = htonl(i&IN_CLASSA_NET);
	else if (IN_CLASSB(i))
	    netp->s_addr = htonl(i&IN_CLASSB_NET);
	else if (IN_CLASSC(i))
	    netp->s_addr = htonl(i&IN_CLASSC_NET);
	else
	    netp->s_addr = i;	/* XXX wrong for Multicast? XXX */
}

subnetextract(iap, netp, subnetp)
register struct in_addr *iap;
register struct in_addr *netp;
register struct in_addr *subnetp;
{
	static struct in_addr mask;
	
	if (NetMaskLookup(netp, &mask)) {
	    subnetp->s_addr = iap->s_addr & mask.s_addr;
	}
	/* otherwise don't set the subnet value */
}
