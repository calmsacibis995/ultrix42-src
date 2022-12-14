#ifndef lint
static	char	*sccsid = "@(#)res_query.c	4.2	(ULTRIX)	9/4/90";
#endif lint
/************************************************************************
 *									*
 *			Copyright (c) 1989 by				*
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
 * Copyright (c) 1988 Regents of the University of California.
 * All rights reserved.
 *
static char sccsid[] = "@(#)res_query.c	5.3 (Berkeley) 4/5/88";
 */

#include <sys/param.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <ctype.h>
#include <netdb.h>
#include <stdio.h>
#include <errno.h>
#include <strings.h>
#include <arpa/inet.h>
#include <arpa/nameser.h>
#include <resolv.h>

#if defined(lint) && !defined(DEBUG)
#define DEBUG
#endif

#if PACKETSZ > 1024
#define MAXPACKET	PACKETSZ
#else
#define MAXPACKET	1024
#endif

extern int errno;
int h_errno;

/*
 * Formulate a normal query, send, and await answer.
 * Returned answer is placed in supplied buffer "answer".
 * Perform preliminary check of answer, returning success only
 * if no error is indicated and the answer count is nonzero.
 * Return the size of the response on success, -1 on error.
 * Error number is left in h_errno.
 * Caller must parse answer and determine whether it answers the question.
 */
res_query(name, class, type, answer, anslen)
	char *name;		/* domain name */
	int class, type;	/* class and type of query */
	u_char *answer;		/* buffer to put answer */
	int anslen;		/* size of answer buffer */
{
	char buf[MAXPACKET];
	HEADER *hp;
	int n;

	if ((_res.options & RES_INIT) == 0 && res_init() == -1)
		return (-1);
#ifdef DEBUG
	if (_res.options & RES_DEBUG)
		printf("res_query(%s, %d, %d)\n", name, class, type);
#endif
	n = res_mkquery(QUERY, name, class, type, (char *)NULL, 0, NULL,
	    buf, sizeof(buf));

	if (n <= 0) {
#ifdef DEBUG
		if (_res.options & RES_DEBUG)
			printf("res_query: mkquery failed\n");
#endif
		h_errno = NO_RECOVERY;
		return (n);
	}
	n = res_send(buf, n, answer, anslen);
	if (n < 0) {
#ifdef DEBUG
		if (_res.options & RES_DEBUG)
			printf("res_query: send error\n");
#endif
		h_errno = TRY_AGAIN;
		return(n);
	}

	hp = (HEADER *) answer;
	if (hp->rcode != NOERROR || ntohs(hp->ancount) == 0) {
#ifdef DEBUG
		if (_res.options & RES_DEBUG)
			printf("rcode = %d, ancount=%d\n", hp->rcode,
			    ntohs(hp->ancount));
#endif
		switch (hp->rcode) {
			case NXDOMAIN:
				h_errno = HOST_NOT_FOUND;
				break;
			case SERVFAIL:
				h_errno = TRY_AGAIN;
				break;
			case NOERROR:
				h_errno = NO_DATA;
				break;
			case FORMERR:
			case NOTIMP:
			case REFUSED:
			default:
				h_errno = NO_RECOVERY;
				break;
		}
		return (-1);
	}
	return(n);
}

/*
 * Formulate a normal query, send, and retrieve answer in supplied buffer.
 * Return the size of the response on success, -1 on error.
 * If enabled, implement search rules until answer or unrecoverable failure
 * is detected.  Error number is left in h_errno.
 * Only useful for queries in the same name hierarchy as the local host
 * (not, for example, for host address-to-name lookups in domain in-addr.arpa).
 */
res_search(name, class, type, answer, anslen)
	char *name;		/* domain name */
	int class, type;	/* class and type of query */
	u_char *answer;		/* buffer to put answer */
	int anslen;		/* size of answer */
{
	register char *cp, **domain;
	int n, ret, got_nodata = 0;
	char *hostalias();

	if ((_res.options & RES_INIT) == 0 && res_init() == -1)
		return (-1);

	errno = 0;
	h_errno = HOST_NOT_FOUND;		/* default, if we never query */
	for (cp = name, n = 0; *cp; cp++)
		if (*cp == '.')
			n++;
	if (n == 0 && (cp = hostalias(name)))
		return (res_query(cp, class, type, answer, anslen));

	if ((n == 0 || *--cp != '.') && (_res.options & RES_DEFNAMES))
	    for (domain = _res.dnsrch; *domain; domain++) {
		ret = res_querydomain(name, *domain, class, type,
		    answer, anslen);
		if (ret > 0)
			return (ret);
		/*
		 * If no server present, give up.
		 * If name isn't found in this domain,
		 * keep trying higher domains in the search list
		 * (if that's enabled).
		 * On a NO_DATA error, keep trying, otherwise
		 * a wildcard entry of another type could keep us
		 * from finding this entry higher in the domain.
		 * If we get some other error (non-authoritative negative
		 * answer or server failure), then stop searching up,
		 * but try the input name below in case it's fully-qualified.
		 */
		if (errno == ECONNREFUSED) {
			h_errno = TRY_AGAIN;
			return (-1);
		}
		if (h_errno == NO_DATA)
			got_nodata++;
		if ((h_errno != HOST_NOT_FOUND && h_errno != NO_DATA) ||
		    (_res.options & RES_DNSRCH) == 0)
			break;
		h_errno = 0;
	}
	/*
	 * If the search/default failed, try the name as fully-qualified,
	 * but only if it contained at least one dot (even trailing).
	 */
	if (n)
		return (res_querydomain(name, (char *)NULL, class, type,
		    answer, anslen));
	if (got_nodata)
		h_errno = NO_DATA;
	return (-1);
}

/*
 * Perform a call on res_query on the concatenation of name and domain,
 * removing a trailing dot from name if domain is NULL.
 */
res_querydomain(name, domain, class, type, answer, anslen)
	char *name, *domain;
	int class, type;	/* class and type of query */
	u_char *answer;		/* buffer to put answer */
	int anslen;		/* size of answer */
{
	char nbuf[2*MAXDNAME+2];
	char *longname = nbuf;
	int n;

#ifdef DEBUG
	if (_res.options & RES_DEBUG)
		printf("res_querydomain(%s, %s, %d, %d)\n",
		    name, domain, class, type);
#endif
	if (domain == NULL) {
		/*
		 * Check for trailing '.';
		 * copy without '.' if present.
		 */
		n = strlen(name) - 1;
		if (name[n] == '.' && n < sizeof(nbuf) - 1) {
			bcopy(name, nbuf, n);
			nbuf[n] = '\0';
		} else
			longname = name;
	} else
		(void)sprintf(nbuf, "%.*s.%.*s",
		    MAXDNAME, name, MAXDNAME, domain);

	return (res_query(longname, class, type, answer, anslen));
}
