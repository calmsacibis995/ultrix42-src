#ifndef lint
static	char	*sccsid = "@(#)res_mkquery.c	4.2	(ULTRIX)	9/4/90";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1987 by				*
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
 * Copyright (c) 1985 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
static char sccsid[] = "@(#)res_mkquery.c	6.6 (Berkeley) 10/22/87";
 */

#include <stdio.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/nameser.h>
#include <resolv.h>

#if defined(lint) && !defined(DEBUG)
#define DEBUG
#endif

/*
 * Form all types of queries.
 * Returns the size of the result or -1.
 */
res_mkquery(op, dname, class, type, data, datalen, newrr, buf, buflen)
	int op;			/* opcode of query */
	char *dname;		/* domain name */
	int class, type;	/* class and type of query */
	char *data;		/* resource record data */
	int datalen;		/* length of data */
	struct rrec *newrr;	/* new rr for modify or append */
	char *buf;		/* buffer to put query */
	int buflen;		/* size of buffer */
{
	register HEADER *hp;
	register char *cp;
	register int n;
	char dnbuf[MAXDNAME];
	char *dnptrs[10], **dpp, **lastdnptr;
	extern char *index();

#ifdef DEBUG
	if (_res.options & RES_DEBUG)
		printf("res_mkquery(%d, %s, %d, %d)\n", op, dname, class, type);
#endif DEBUG
	if (!(_res.options & RES_INIT))
		if (res_init() == -1) {
			return(-1);
		}
	/*
	 * Initialize header fields.
	 */
	hp = (HEADER *) buf;
	hp->id = htons(++_res.id);
	hp->opcode = op;
	hp->qr = hp->aa = hp->tc = hp->ra = 0;
	hp->pr = (_res.options & RES_PRIMARY) != 0;
	hp->rd = (_res.options & RES_RECURSE) != 0;
	hp->rcode = NOERROR;
	hp->qdcount = 0;
	hp->ancount = 0;
	hp->nscount = 0;
	hp->arcount = 0;
	cp = buf + sizeof(HEADER);
	buflen -= sizeof(HEADER);
	dpp = dnptrs;
	*dpp++ = buf;
	*dpp++ = NULL;
	lastdnptr = dnptrs + sizeof(dnptrs)/sizeof(dnptrs[0]);
	/*
	 * If the domain name contains no dots (single label), then
	 * append the default domain name to the one given.
	 */
	if ((_res.options & RES_DEFNAMES) && dname != 0 && dname[0] != '\0' &&
	    index(dname, '.') == NULL) {
		if (!(_res.options & RES_INIT))
			if (res_init() == -1)
				return(-1);
		if (_res.defdname[0] != '\0') {
			(void)sprintf(dnbuf, "%s.%s", dname, _res.defdname);
			dname = dnbuf;
		}
	}
	/*
	 * perform opcode specific processing
	 */
	switch (op) {
	case QUERY:
		buflen -= QFIXEDSZ;
		if ((n = dn_comp(dname, cp, buflen, dnptrs, lastdnptr)) < 0)
			return (-1);
		cp += n;
		buflen -= n;
		putshort(type, cp);
		cp += sizeof(u_short);
		putshort(class, cp);
		cp += sizeof(u_short);
		hp->qdcount = htons(1);
		if (op == QUERY || data == NULL)
			break;
		/*
		 * Make an additional record for completion domain.
		 */
		buflen -= RRFIXEDSZ;
		if ((n = dn_comp(data, cp, buflen, dnptrs, lastdnptr)) < 0)
			return (-1);
		cp += n;
		buflen -= n;
		putshort(T_NULL, cp);
		cp += sizeof(u_short);
		putshort(class, cp);
		cp += sizeof(u_short);
		putlong(0, cp);
		cp += sizeof(u_long);
		putshort(0, cp);
		cp += sizeof(u_short);
		hp->arcount = htons(1);
		break;
#ifdef AUTHEN
	case AQUERY:
		buflen -= AQFIXEDSZ;
		if ((n = dn_comp(dname, cp, buflen, dnptrs, lastdnptr)) < 0)
			return (-1);
		cp += n;
		buflen -= n;
		putshort(type, cp);
		cp += sizeof(u_short);
		putshort(class, cp);
		cp += sizeof(u_short);
		putshort((u_short)_res.defauthtype, cp);
		cp += sizeof(u_short);
		putshort((u_short)_res.defauthversion, cp);
		cp += sizeof(u_short);

		hp->qdcount = htons(1);

		if (op == AQUERY || data == NULL)
			break;
		/*
		 * Make an additional record for completion domain.
		 */
		buflen -= RRFIXEDSZ;
		if ((n = dn_comp(data, cp, buflen, dnptrs, lastdnptr)) < 0)
			return (-1);
		cp += n;
		buflen -= n;
		putshort(T_NULL, cp);
		cp += sizeof(u_short);
		putshort(class, cp);
		cp += sizeof(u_short);
		putlong(0, cp);
		cp += sizeof(u_long);
		putshort(0, cp);
		cp += sizeof(u_short);
		hp->arcount = htons(1);
		break;
#endif AUTHEN
	case IQUERY:
		/*
		 * Initialize answer section
		 */
		if (buflen < 1 + RRFIXEDSZ + datalen)
			return (-1);
		*cp++ = '\0';	/* no domain name */
		putshort(type, cp);
		cp += sizeof(u_short);
		putshort(class, cp);
		cp += sizeof(u_short);
		putlong(0, cp);
		cp += sizeof(u_long);
		putshort(datalen, cp);
		cp += sizeof(u_short);
		if (datalen) {
			bcopy(data, cp, datalen);
			cp += datalen;
		}
		hp->ancount = htons(1);
		break;

#ifdef ALLOW_UPDATES
	/*
	 * For UPDATEM/UPDATEMA, do UPDATED/UPDATEDA followed by UPDATEA
	 * (Record to be modified is followed by its replacement in msg.)
	 */
	case UPDATEM:
	case UPDATEMA:

	case UPDATED:
		/*
		 * The res code for UPDATED and UPDATEDA is the same; user
		 * calls them differently: specifies data for UPDATED; server
		 * ignores data if specified for UPDATEDA.
		 */
	case UPDATEDA:
		buflen -= RRFIXEDSZ + datalen;
		if ((n = dn_comp(dname, cp, buflen, dnptrs, lastdnptr)) < 0)
			return (-1);
		cp += n;
		putshort(type, cp);
                cp += sizeof(u_short);
                putshort(class, cp);
                cp += sizeof(u_short);
		putlong(0, cp);
		cp += sizeof(u_long);
		putshort(datalen, cp);
                cp += sizeof(u_short);
		if (datalen) {
			bcopy(data, cp, datalen);
			cp += datalen;
		}
		if ( (op == UPDATED) || (op == UPDATEDA) ) {
			hp->ancount = htons(0);
			break;
		}
		/* Else UPDATEM/UPDATEMA, so drop into code for UPDATEA */

	case UPDATEA:	/* Add new resource record */
		buflen -= RRFIXEDSZ + datalen;
		if ((n = dn_comp(dname, cp, buflen, dnptrs, lastdnptr)) < 0)
			return (-1);
		cp += n;
		putshort(newrr->r_type, cp);
                cp += sizeof(u_short);
                putshort(newrr->r_class, cp);
                cp += sizeof(u_short);
		putlong(0, cp);
		cp += sizeof(u_long);
		putshort(newrr->r_size, cp);
                cp += sizeof(u_short);
		if (newrr->r_size) {
			bcopy(newrr->r_data, cp, newrr->r_size);
			cp += newrr->r_size;
		}
		hp->ancount = htons(0);
		break;

#endif ALLOW_UPDATES
	}
	return (cp - buf);
}
