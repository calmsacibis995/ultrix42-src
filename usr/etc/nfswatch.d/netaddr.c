#ifndef lint
static char *sccsid = "@(#)netaddr.c	4.2	(ULTRIX)	1/25/91";
#endif
/* Based on:
 * RCSid = "$Header: /sparky/a/davy/system/nfswatch/RCS/netaddr.c,v 3.0 91/01/23 08:23:06 davy Exp $";
 */

/*
 * netaddr.c - routines for working with network addresses.
 *
 * David A. Curry				Jeffrey C. Mogul
 * SRI International				Digital Equipment Corporation
 * 333 Ravenswood Avenue			Western Research Laboratory
 * Menlo Park, CA 94025				100 Hamilton Avenue
 * davy@erg.sri.com				Palo Alto, CA 94301
 *						mogul@decwrl.dec.com
 *
 * $Log:	netaddr.c,v $
 * Revision 3.0  91/01/23  08:23:06  davy
 * NFSWATCH Version 3.0.
 * 
 * Revision 1.2  90/08/17  15:47:24  davy
 * NFSWATCH Version 2.0.
 * 
 * Revision 1.1  88/11/29  11:20:35  davy
 * NFSWATCH Release 1.0
 * 
 */
#include <sys/param.h>
#include <netdb.h>
#include <stdio.h>

#include "nfswatch.h"
#include "externs.h"

/*
 * get_net_addrs - get network addresses of source and destination
 *		   hosts, along with official host names.
 */
void
get_net_addrs()
{
	register int n;
	char *inet_ntoa();
	register char **cp;
	struct hostent *hp;

	/*
	 * Look up the local host.
	 */
	if ((hp = gethostbyname(myhost)) == NULL) {
		(void) fprintf(stderr, "%s: %s: unknown host.\n", pname,
			myhost);
		finish(-1);
	}

	/*
	 * Save the official host name.
	 */
	(void) strcpy(myhost, hp->h_name);

	/*
	 * If one was specified, look up the destination host.
	 * Otherwise, we can use what we have.
	 */
	if (allflag) {
		(void) sprintf(dsthost, "all hosts");
	}
	else if (dstflag) {
		if ((hp = gethostbyname(dsthost)) == NULL) {
			(void) fprintf(stderr, "%s: %s: unknown host.\n", pname,
				dsthost);
			finish(-1);
		}

		/*
		 * Save the official host name.
		 */
		(void) strcpy(dsthost, hp->h_name);
	}
	else {
		/*
		 * Host name is the same as the local
		 * host.
		 */
		(void) strcpy(dsthost, myhost);
	}

	/*
	 * Copy destination host's network addresses.
	 */
	n = 0;
	(void) bzero((char *) dstaddrs, MAXHOSTADDR * sizeof(u_long));

	for (cp = hp->h_addr_list; *cp != NULL; cp++) {
		if (n >= MAXHOSTADDR)
			break;

		(void) bcopy(*cp, (char *) &dstaddrs[n], hp->h_length);
		n++;
	}

	/*
	 * If they didn't specify a source host,
	 * we're done.
	 */
	if (!srcflag)
		return;

	/*
	 * Look up the source host.
	 */
	if ((hp = gethostbyname(srchost)) == NULL) {
		(void) fprintf(stderr, "%s: %s: unknown host.\n", pname,
			srchost);
		finish(-1);
	}

	/*
	 * Save the official host name.
	 */
	(void) strcpy(srchost, hp->h_name);

	/*
	 * Copy source host's network addresses.
	 */
	n = 0;
	(void) bzero((char *) srcaddrs, MAXHOSTADDR * sizeof(u_long));

	for (cp = hp->h_addr_list; *cp != NULL; cp++) {
		if (n >= MAXHOSTADDR)
			break;

		(void) bcopy(*cp, (char *) &srcaddrs[n], hp->h_length);
		n++;
	}
}

/*
 * want_packet - determine if we're interested in a packet by examining
 *		 its source and destination addresses.
 */
int
want_packet(src, dst)
u_long src, dst;
{
	register int i, want;

	/*
	 * Any source or destination is okay.
	 */
	if (allflag) {
		thisdst = dst;
		return(TRUE);
	}

	want = FALSE;

	/*
	 * Check source address first.
	 */
	if (srcflag) {
		for (i = 0; (srcaddrs[i] != 0) && (i < MAXHOSTADDR); i++) {
			if (!bcmp((char *) &src, (char *) &srcaddrs[i],
			    sizeof(u_long))) {
				want = TRUE;
				break;
			}
		}

		/*
		 * If it's not from our source, we
		 * don't even need to check the destination.
		 */
		if (!want)
			return(FALSE);
	}

	want = FALSE;

	/*
	 * Check destination address.
	 */
	for (i = 0; (dstaddrs[i] != 0) && (i < MAXHOSTADDR); i++) {
		if (!bcmp((char *) &dst, (char *) &dstaddrs[i],
		    sizeof(u_long))) {
			want = TRUE;
			break;
		}
	}

	return(want);
}
