#ifndef lint
static	char	*sccsid = "@(#)ns_sort.c	4.1	(ULTRIX)	7/2/90";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1984-1988 by			*
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
/*
 * Copyright (c) 1986 Regents of the University of California
 *	All Rights Reserved
 * static char sccsid[] = "@(#)ns_sort.c	4.3 (Berkeley) 2/17/88";
 */

/*
 * Modification History:
 *
 * 18-Jan-88	logcher
 *	Added BIND 4.7.2.
 *
 * 26-Jan-88	logcher
 *	Added BIND 4.7.3.
 *
 * 17-May-89	logcher
 *	Added BIND 4.8.
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/time.h>
#ifdef ULTRIXFUNC
#include <sys/stat.h>
#endif ULTRIXFUNC
#include <sys/socket.h>
#include <sys/file.h>
#include <netinet/in.h>
#include <syslog.h>
#include <arpa/nameser.h>
#include "ns.h"
#include "db.h"

extern	char *p_type(), *p_class();

extern	int	debug;
extern  FILE	*ddt;

struct netinfo* 
local(from)
	struct sockaddr_in *from;
{
	extern struct netinfo *nettab, netloop;
	struct netinfo *ntp;

	if (from->sin_addr.s_addr == netloop.my_addr.s_addr)
		return( &netloop);
	for (ntp = nettab; ntp != NULL; ntp = ntp->next) {
		if (ntp->net == (from->sin_addr.s_addr & ntp->mask))
			return(ntp);
	}
	return(NULL);
}


sort_response(cp, ancount, lp, eom)
	register char *cp;
	register int ancount;
	struct netinfo *lp;
	u_char *eom;
{
	register struct netinfo *ntp;
	extern struct netinfo *nettab;

#ifdef DEBUG
	if (debug > 2)
	    fprintf(ddt,"sort_response(%d)\n", ancount);
#endif DEBUG
	if (ancount > 1) {
		if (sort_rr(cp, ancount, lp, eom))
			return;
		for (ntp = nettab; ntp != NULL; ntp = ntp->next) {
			if ((ntp->net == lp->net) && (ntp->mask == lp->mask))
				continue;
			if (sort_rr(cp, ancount, ntp, eom))
				break;
		}
	}
}

int
sort_rr(cp, count, ntp, eom)
	register u_char *cp;
	int count;
	register struct netinfo *ntp;
	u_char *eom;
{
	int type, class, dlen, n, c;
	struct in_addr inaddr;
	u_char *rr1;

#ifdef DEBUG
	if (debug > 2) {
	    inaddr.s_addr = ntp->net;
	    fprintf(ddt,"sort_rr( x%x, %d, %s)\n",cp, count,
		inet_ntoa(inaddr));
	}
#endif DEBUG
	rr1 = NULL;
	for (c = count; c > 0; --c) {
	    n = dn_skipname(cp, eom);
	    if (n < 0)
		return (1);		/* bogus, stop processing */
	    cp += n;
	    if (cp + QFIXEDSZ > eom)
		return (1);
	    GETSHORT(type, cp);
	    GETSHORT(class, cp);
	    cp += sizeof(u_long);
	    GETSHORT(dlen, cp);
	    if (dlen > eom - cp)
		return (1);		/* bogus, stop processing */
	    switch (type) {
	    case T_A:
	    	switch (class) {
	    	case C_IN:
	    		bcopy(cp, (char *)&inaddr, sizeof(inaddr));
			if (rr1 == NULL)
				rr1 = cp;
			if ((ntp->mask & inaddr.s_addr) == ntp->net) {
#ifdef DEBUG
			    if (debug > 1) {
	    			fprintf(ddt,"net %s best choice\n",
					inet_ntoa(inaddr));
			    }
#endif DEBUG
			    if (rr1 != cp) {
	    		        bcopy(rr1, cp, sizeof(inaddr));
	    		        bcopy((char *)&inaddr, rr1, sizeof(inaddr));
			    }
			    return(1);
			}
	    		break;
	    	}
	    	break;
	    }
	    cp += dlen;
	}
	return(0);
}

#ifdef notdef
dump_namebuf(np)
	register struct namebuf *np;
{
	register struct databuf *dp;
	long n;
	u_long addr;
	u_short i;
	int j;
	char *cp;
	char *proto;
	FILE *fp;
	extern char *inet_ntoa(), *p_protocal(), *p_service();
	int found_data;

	gettime(&tt);
	if ((fp = fopen("/usr/var/tmp/namebuf", "a")) == NULL)
		return;
	found_data = 0;
	for (dp = np->n_data; dp != NULL; dp = dp->d_next) {
		if (dp->d_ttl <= tt.tv_sec)
			continue;	/* Stale */
		if (!found_data) {
		    fprintf(fp, "%s\t", np->n_dname);
		    if (strlen(np->n_dname) < 8)
			(void) putc('\t', fp);
		    found_data++;
		} else
			fprintf(fp, "\t\t");
		if (dp->d_zone == 0)
			fprintf(fp, "%d\t", dp->d_ttl - tt.tv_sec);
		else if (dp->d_ttl > zones[dp->d_zone].z_minimum)
			fprintf(fp, "%d\t", dp->d_ttl);
		fprintf(fp, "%s\t%s\t", p_class(dp->d_class),
			p_type(dp->d_type));
		cp = dp->d_data;
		/*
		 * Print type specific data
		 */
		switch (dp->d_type) {
		case T_A:
			switch (dp->d_class) {
			case C_IN:
				n = htonl(_getlong(cp));
				fprintf(fp, "%s\n",
				   inet_ntoa(*(struct in_addr *)&n));
				break;
			}
			break;
		case T_CNAME:
		case T_MB:
		case T_MG:
		case T_MR:
		case T_PTR:
			if (cp[0] == '\0')
				fprintf(fp, ".\n");
			else
				fprintf(fp, "%s.\n", cp);
			break;

		case T_NS:
			cp = dp->d_data;
			if (cp[0] == '\0')
				fprintf(fp, ".\t");
			else
				fprintf(fp, "%s.", cp);
			if (dp->d_nstime)
				fprintf(fp, "\t; %d", dp->d_nstime);
			fprintf(fp, "\n");
			break;

		case T_HINFO:
			if (n = *cp++) {
				fprintf(fp, "\"%.*s\"", n, cp);
				cp += n;
			} else
				fprintf(fp, "\"\"");
			if (n = *cp++)
				fprintf(fp, " \"%.*s\"", n, cp);
			else
				fprintf(fp, "\"\"");
			(void) putc('\n', fp);
			break;

		case T_SOA:
			fprintf(fp, "%s.", cp);
			cp += strlen(cp) + 1;
			fprintf(fp, " %s. (\n", cp);
			cp += strlen(cp) + 1;
			fprintf(fp, "\t\t%d", _getlong(cp));
			cp += sizeof(u_long);
			fprintf(fp, " %d", _getlong(cp));
			cp += sizeof(u_long);
			fprintf(fp, " %d", _getlong(cp));
			cp += sizeof(u_long);
			fprintf(fp, " %d", _getlong(cp));
			cp += sizeof(u_long);
			fprintf(fp, " %d )\n", _getlong(cp));
			break;

		case T_MX:
			fprintf(fp,"%d", _getshort(cp));
			cp += sizeof(u_short);
			fprintf(fp," %s.\n", cp);
			break;


		case T_UINFO:
			fprintf(fp, "\"%s\"\n", cp);
			break;

		case T_UID:
		case T_GID:
			if (dp->d_size == sizeof(u_long)) {
				fprintf(fp, "%d\n", _getlong(cp));
				cp += sizeof(u_long);
			}
			break;

		case T_WKS:
			addr = htonl(_getlong(cp));	
			fprintf(fp,"%s ",
			    inet_ntoa(*(struct in_addr *)&addr));
			cp += sizeof(u_long);
			proto = p_protocal(*cp); /* protocal */
			cp += sizeof(char); 
			fprintf(fp, "%s ", proto);
			i = 0;
			while(cp < dp->d_data + dp->d_size) {
				j = *cp++;
				do {
					if(j & 0200)
						fprintf(fp," %s",
						   p_service(i, proto));
					j <<= 1;
				} while(++i & 07);
			} 
			fprintf(fp,"\n");
			break;

		default:
			fprintf(fp, "???\n");
		}
	}
	(void) fclose(fp);
}
#endif notdef
