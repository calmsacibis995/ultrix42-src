#ifndef lint
static	char	*sccsid = "@(#)gethostent.c	4.6	(ULTRIX)	2/12/91";
#endif lint
/************************************************************************
 *									*
 *			Copyright (c) 1984-1989 by			*
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
/****************************************************************
 *								*
 *  Licensed to Digital Equipment Corporation, Maynard, MA	*
 *		Copyright 1985 Sun Microsystems, Inc.		*
 *			All rights reserved.			*
 *								*
 ****************************************************************/

/*
 * Modification History:
 *
 * 15-Nov-90    lebel
 *	Fixed strfind.
 *
 * 07-Feb-90	sue
 *	Remove calls to remove_local_domain().  This does not conform
 *	to standard BIND semantics.  Change the name of the routine and
 *	function to local_hostname_length which returns the length of 
 *	the short hostname if the string contains the local domain 
 *	name.
 *
 * 13-Nov-89	sue
 *	Changed svc_gethostflag initial value to -2 and now perform a
 *	check in gethostent to see if the sethostent has been called yet.
 *
 * 18-Aug-89	sue
 *	Added support for gethostent_bind().  It depends on the BIND
 *	host database being built by /var/dss/namedb/bin/make_hosts.
 *	Removed call to remove_local_domain in interpret().  It does
 *	not make sense to modify the answer without modifying the
 *	request.  The user should get what he asks for.
 *
 * 24-Jul-89	logcher
 *	Removed generic sethostent and endhostent calls from generic
 *	gethostbyname and gethostbyaddr.  Added the specific set and end
 *	calls in the specific get routines.  Moved remove_local_domain
 *	call in interpret to after nulls have been placed.
 *
 * 25-May-89	logcher
 *	Changed name of any() to getcommon_any().
 *
 * 16-May-89	logcher
 *	Remove all of the "svc" specific code and put in separate
 *	file, svcent.c,  so other get*ent.c routines can use them.
 *	Removed static declarations on *_[local|yp|bind] calls so 
 *	that these routines can be called outside of this file.
 *
 * 09-May-89	logcher
 *	Added remove_local_domain to strip off local domain from name,
 *	if it exists.  Changed FSG debug lines to DEBUG.
 *
 * 15-Jul-88	logcher
 *	In init_svcorder, do an fclose on the file descriptor for
 *	the /etc/svcorder.
 *
 * 07-Mar-88	rglaser
 *	Merge Fred's changes into the latest version from Berkeley.
 *	Change debug printouts to go to stderr for better daemon debug.
 *
 * 24-Feb-88    fglover
 *	Support entries in /etc/svcorder file as case insensitive,
 *	but map to upper case for internal comparisons.  Define
 *	macro L_TO_U to do the mapping.
 *
 * 26-Jan-88	logcher
 *	Added a few more BIND 4.7.3 changes as well as changing
 *	some ifdefed debug messages to print on the console to be 
 *	useful if compiled in daemons (rshd).
 *
 */

#include <stdio.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <ctype.h>
#include <netdb.h>
#include <errno.h>
#include <rpcsvc/ypclnt.h>
#include <sys/stat.h>

#include <arpa/inet.h>
#include <arpa/nameser.h>
#include <resolv.h>
#include <sys/svcinfo.h>

#if defined(lint) && !defined(DEBUG)
#define DEBUG
#endif

/*
 * Internet version.
 */

#define	MAXALIASES	35
#define MAXADDRS	35
#define	MAXADDRSIZE	14

static char HOSTDB[] = "/etc/hosts";
static struct hostent host;
static struct in_addr host_addr;
static char *h_addr_ptrs[MAXADDRS+1];
static char *host_aliases[MAXALIASES];
static char hostbuf[BUFSIZ+1];
static FILE *hostf = NULL;
static char line[BUFSIZ+1];
static char hostaddr[MAXADDRS];
static char *host_addrs[2];
static int stayopen=0;
static char *current = NULL;	/* current entry, analogous to hostf */
static int currentlen;
static struct svcinfo *svcinfo;
static char *domain;

static struct hostent *interpret();
static struct hostent *getanswer();
static struct hostent *gethostdomain();
char *hostalias();
char *inet_ntoa();
char *strfind();
char *getcommon_any();
char *yellowup();

int h_errno;
extern errno;

#if PACKETSZ > 1024
#define MAXPACKET       PACKETSZ
#else
#define MAXPACKET       1024
#endif

typedef union {
    HEADER qb1;
    char qb2[MAXPACKET];
} querybuf;

static union {
    long al;
    char ac;
} align;
int svc_gethostflag = -2;
int svc_gethostbind;

/* NOTE: _local versions of these are available in gethostent_local.c.
 * Keep these static to avoid conflicts
 */
static int sethostent_local();
static int endhostent_local();
static struct hostent *gethostent_local();
static struct hostent *gethostbyaddr_local(); 
static struct hostent *gethostbyname_local();

int setent_bind();
int sethostent_yp();
int endent_bind();
int endhostent_yp();
struct hostent *gethostent_bind();
struct hostent *gethostent_yp();
struct hostent *gethostbyaddr_bind(); 
struct hostent *gethostbyaddr_yp(); 
struct hostent *gethostbyname_bind();	
struct hostent *gethostbyname_yp();

/* 
 *	call service routines indirectly
 */

static int	(*sethostents []) ()={
		sethostent_local,
		sethostent_yp,
		setent_bind
};
static int	(*endhostents []) ()={
		endhostent_local,
		endhostent_yp,
		endent_bind
};
static struct hostent * (*gethostents []) ()={
		gethostent_local,
		gethostent_yp,
		gethostent_bind
};
static struct hostent * (*gethostaddrs []) ()={
		gethostbyaddr_local,
		gethostbyaddr_yp,
		gethostbyaddr_bind
};
static struct hostent * (*gethostnames []) ()={
		gethostbyname_local,
		gethostbyname_yp,
		gethostbyname_bind
};


/*
 * generic gethost service routines
 */

sethostent (f)
	int f;
{
	register i;

	svc_gethostflag = -1;
	svc_gethostbind = 0;
	if ((svcinfo = getsvc()) != NULL)
		for (i=0; (svc_lastlookup = svcinfo->svcpath[SVC_HOSTS][i]) != SVC_LAST; i++)
			(*(sethostents [svcinfo->svcpath[SVC_HOSTS][i]])) (f);
}

endhostent ()
{
	register i;

	svc_gethostflag = -1;
	svc_gethostbind = 0;
	if ((svcinfo = getsvc()) != NULL)
		for (i=0; (svc_lastlookup = svcinfo->svcpath[SVC_HOSTS][i]) != SVC_LAST; i++)
			(*(endhostents [svcinfo->svcpath[SVC_HOSTS][i]])) ();
}

struct hostent *
gethostent()
{
	register struct hostent *p=NULL;
	register i;

	/*
	 * Check if sethostent was not made yet
	 */
	if (svc_gethostflag == -2)
		sethostent(0);
	/*
	 * Check if this is the first time through gethostent
	 */
	if (svc_gethostflag == -1) {
		/*
		 * If it is, init the svcinfo structure
		 */
		if ((svcinfo = getsvc()) == NULL)
			return((struct hostent *)NULL);
		i = 0;
	}
	else {
		/*
		 * If it is not, set the index to the last used one
		 */
		i = svc_gethostflag;
	}
	for (; (svc_lastlookup = svcinfo->svcpath[SVC_HOSTS][i]) != SVC_LAST; i++)
		if (p = ((*(gethostents [svcinfo->svcpath[SVC_HOSTS][i]])) () )) {
			svc_gethostflag = i;
			break;
		}
	return (p);
}

struct hostent *
gethostbyname (name)
	register char *name;
{
	register struct hostent *p=NULL;
	register i;

	/* avoid null pointer de-reference on mips */
	if (name == 0)
		return(0);
	if ((svcinfo = getsvc()) != NULL)
		for (i=0; (svc_lastlookup = svcinfo->svcpath[SVC_HOSTS][i]) != SVC_LAST; i++)
			if (p = ((*(gethostnames [svcinfo->svcpath[SVC_HOSTS][i]])) (name) ))
				break;
	return (p);
}

struct hostent *
gethostbyaddr (addr, len, type)
	register char *addr;
	register int len, type;
{
	register struct hostent *p=NULL;
	register i;

	if ((svcinfo = getsvc()) != NULL)
		for (i=0; (svc_lastlookup = svcinfo->svcpath[SVC_HOSTS][i]) != SVC_LAST; i++)
			if (p = ((*(gethostaddrs [svcinfo->svcpath[SVC_HOSTS][i]])) (addr, len, type) ))
				break;
	return (p);
}

/*
 * specific gethost service routines
 */

static struct hostent *
gethostbyaddr_local(addr, len, type)
	char *addr;
	register int len, type;
{
	register struct hostent *p=NULL;

#ifdef DEBUG
	fprintf(stderr, "gethostbyaddr_local(%d.%d.%d.%d, %d, %d)\n", addr[0], addr[1], addr[2], addr[3], len, type);
#endif DEBUG
	sethostent_local(0);
	while (p = gethostent_local()) {
		if (p->h_addrtype != type || p->h_length != len)
			continue;
		if (bcmp(p->h_addr, addr, len) == 0)
			break;
	}
	endhostent_local();
	return (p);
}

struct hostent *
gethostbyaddr_bind(addr, len, type)
	char *addr;
	register int len, type;
{
	int n;
	querybuf buf;
	register struct hostent *hp;
	char qbuf[MAXDNAME];

#ifdef DEBUG
	fprintf(stderr, "gethostbyaddr_bind(%d.%d.%d.%d, %d, %d)\n", addr[0], addr[1], addr[2], addr[3], len, type);
#endif DEBUG
	if (type != AF_INET)
		return (NULL);
	setent_bind(0);
	(void)sprintf(qbuf, "%d.%d.%d.%d.in-addr.arpa",
		((unsigned)addr[3] & 0xff),
		((unsigned)addr[2] & 0xff),
		((unsigned)addr[1] & 0xff),
		((unsigned)addr[0] & 0xff));
	n = res_mkquery(QUERY, qbuf, C_IN, T_PTR, (char *)NULL, 0, NULL,
		(char *)&buf, sizeof(buf));
	if (n < 0) {
#ifdef DEBUG
		if (_res.options & RES_DEBUG)
			fprintf(stderr,"res_mkquery failed\n");
#endif
		endent_bind();
		return ((struct hostent *)NULL);
	}
	hp = getanswer((char *)&buf, n, 1);
	if (hp == NULL && errno == ECONNREFUSED) {
		endent_bind();
		return((struct hostent *)NULL);	
	}
	if (hp == NULL) {
		endent_bind();
		return((struct hostent *)NULL);	
	}
	hp->h_addrtype = type;
	hp->h_length = len;
	h_addr_ptrs[0] = (char *)&host_addr;
	h_addr_ptrs[1] = (char *)0;
	host_addr = *(struct in_addr *)addr;
	endent_bind();
	return(hp);
}

struct hostent *
gethostbyaddr_yp(addr, len, type)
	char *addr;
	register int len, type;
{
	register struct hostent *p=NULL;
	int reason;
	char *adrstr, *val = NULL;
	int vallen = 0;

#ifdef DEBUG
	fprintf(stderr, "gethostbyaddr_yp(%d.%d.%d.%d, %d, %d)\n", addr[0], addr[1], addr[2], addr[3], len, type);
#endif DEBUG
	sethostent_yp(0);
	if ((domain = yellowup(0)) == NULL)
		return (gethostbyaddr_local(addr, len, type));
	adrstr = inet_ntoa(*(int *)addr);
	if (reason = yp_match(domain, "hosts.byaddr", adrstr, strlen(adrstr), &val, &vallen)) {
#ifdef DEBUG
		fprintf(stderr, "reason yp_first failed is %d\n", reason);
#endif
		p = NULL;
	}
	else {
		p = interpret(val, vallen, SVC_YP);
		free(val);
	}
	endhostent_yp();
	return (p);
}

static struct hostent *
gethostbyname_local(name)
	register char *name;
{
	register struct hostent *p=NULL;
	register char **cp;

#ifdef DEBUG
	fprintf(stderr, "gethostbyname_local(%s)\n", name);
#endif DEBUG
	sethostent_local(0);
	while (p = gethostent_local()) {
		if (strcmp(p->h_name, name) == 0)
			break;
		for (cp = p->h_aliases; *cp != 0; cp++)
			if (strcmp(*cp, name) == 0)
				goto found;
	}
found:
	endhostent_local();
	return (p);
}

struct hostent *
gethostbyname_bind(name)
	register char *name;
{
	register char *cp, **domain;
	int n;
	struct hostent *hp, *gethostdomain();

#ifdef DEBUG
	fprintf(stderr, "gethostbyname_bind(%s)\n", name);
#endif DEBUG
	if (!(_res.options & RES_INIT) && res_init() == -1)
		return (NULL);
	setent_bind(0);
	/*
	 * disallow names consisting only of digits/dots, unless
	 * they end in a dot.
	 */
	if (isdigit(name[0]))
		for (cp = name;; ++cp) {
			if (!*cp) {
				if (*--cp == '.')
					break;
				h_errno = HOST_NOT_FOUND;
				endent_bind();
				return ((struct hostent *) NULL);
			}
			if (!isdigit(*cp) && *cp != '.') 
				break;
		}
	errno = 0;
	for (cp = name, n = 0; *cp; cp++)
		if (*cp == '.')
			n++;

	if (n == 0 && (cp = hostalias(name))) {
		endent_bind();
		return (gethostdomain(cp, (char *)NULL));
	}
	if ((n == 0 || *--cp != '.') && (_res.options & RES_DEFNAMES))
	    for (domain = _res.dnsrch; *domain; domain++) {
		hp = gethostdomain(name, *domain);
		if (hp) {
			endent_bind();
			return (hp);
		}
		/*
		 * If no server present, use host table.
		 * If host isn't found in this domain,
		 * keep trying higher domains in the search list
		 * (if that's enabled).
		 * On a NO_ADDRESS error, keep trying, otherwise
		 * a wildcard MX entry could keep us from finding
		 * host entries higher in the domain.
		 * If we get some other error (non-authoritative negative
		 * answer or server failure), then stop searching up,
		 * but try the input name below in case it's fully-qualified.
		 */
		if (errno == ECONNREFUSED) {
			endent_bind();
			return ((struct hostent *)NULL);
		}
		if ((h_errno != HOST_NOT_FOUND && h_errno != NO_ADDRESS) ||
		    (_res.options & RES_DNSRCH) == 0)
			break;
		h_errno = 0;
	}
	/*
	 * If the search/default failed, try the name as fully-qualified,
	 * but only if it contained at least one dot (even trailing).
	 */
	if (n) {
		endent_bind();
		return (gethostdomain(name, (char *)NULL));
	}
	endent_bind();
	return ((struct hostent *) NULL);
}

struct hostent *
gethostbyname_yp(name)
	register char *name;
{
	register struct hostent *p=NULL;
	int reason;
	char *val = NULL;
	int vallen = 0;

#ifdef DEBUG
	fprintf(stderr, "gethostbyname_yp(%s)\n", name);
#endif DEBUG
	sethostent_yp(0);
	if ((domain = yellowup(0)) == NULL)
		return (gethostbyname_local(name));
	if (reason = yp_match(domain, "hosts.byname", name, strlen(name), &val, &vallen)) {
#ifdef DEBUG
		fprintf(stderr, "reason yp_first failed is %d\n", reason);
#endif
		p = NULL;
	}
	else {
		p = interpret(val, vallen, SVC_YP);
		free(val);
	}
	endhostent_yp();
	return (p);
}

static
sethostent_local(f)
	int f;
{
	if (hostf == NULL)
		hostf = fopen(HOSTDB, "r");
	else
		rewind(hostf);
	stayopen |= f;
}

/*
 * setent_bind(stayopen) in getcommon.c
 */

sethostent_yp(f)
	int f;
{
	if ((domain = yellowup(1)) == NULL)
		return (sethostent_local (f));
	if (current)
		free(current);
	current = NULL;
	stayopen |= f;
}
static 
endhostent_local()
{
	if (hostf && !stayopen) {
		fclose(hostf);
		hostf = NULL;
	}
}

/*
 * endent_bind(stayopen) in getcommon.c
 */

endhostent_yp()
{
	if ((domain = yellowup(0)) == NULL)
		return (endhostent_local ());
	if (current && !stayopen) {
		free(current);
		current = NULL;
	}
}

static struct hostent *
gethostent_local()
{
	static char line1[BUFSIZ+1];

#ifdef DEBUG
	fprintf(stderr,"gethostent_local\n");
#endif DEBUG
	if (hostf == NULL && (hostf = fopen(HOSTDB, "r")) == NULL)
		return (NULL);
        if (fgets(line1, BUFSIZ, hostf) == NULL)
		return (NULL);
	return interpret(line1, strlen(line1), SVC_LOCAL);
}

struct hostent *
gethostent_bind()
{
	char bindbuf[64];
	struct hostent *hp = NULL;

	sprintf(bindbuf, "hosts-%d", svc_gethostbind);
#ifdef DEBUG
	fprintf(stderr, "gethostent_bind(%s)\n", bindbuf);
#endif DEBUG
	if ((hp = gethostbyname_bind(bindbuf)) == NULL)
		return(NULL);
	svc_gethostbind++;
	return(hp);
}

struct hostent *
gethostent_yp()
{
	struct hostent *hp;
	int reason;
	char *key = NULL;
	char *val = NULL;
	int keylen = 0;
	int vallen = 0;

#ifdef DEBUG
	fprintf(stderr,"gethostent_yp\n");
#endif DEBUG
	if ((domain = yellowup(0)) == NULL)
		return (gethostent_local ());
	if (current == NULL) {
		if (reason =  yp_first(domain, "hosts.byaddr", &key, &keylen, &val, &vallen)) {
#ifdef DEBUG
			fprintf(stderr, "reason yp_first failed is %d\n", reason);
#endif
			return NULL;
		}
	}
	else {
		if (reason = yp_next(domain, "hosts.byaddr", current, currentlen, &key, &keylen, &val, &vallen)) {
#ifdef DEBUG
			fprintf(stderr, "reason yp_next failed is %d\n", reason);
#endif
			return NULL;
		}
	}
	if (current)
		free(current);
	current = key;
	currentlen = keylen;
	hp = interpret(val, vallen, SVC_YP);
	free(val);
	return (hp);
}

static struct hostent *
interpret(val, len, svc)
	char *val;
	int len, svc;
{
	char *p;
	register char *cp, **q;

	strncpy(line, val, len);
	p = line;
	line[len] = '\n';
	if (*p == '#')
		switch (svc) {
			case SVC_LOCAL:
				return (gethostent_local());
			case SVC_YP:
				return (gethostent_yp());
		}
	cp = getcommon_any(p, "#\n");
	if (cp == NULL)
		switch (svc) {
			case SVC_LOCAL:
				return (gethostent_local());
			case SVC_YP:
				return (gethostent_yp());
		}
	*cp = '\0';
	cp = getcommon_any(p, " \t");
	if (cp == NULL)
		switch (svc) {
			case SVC_LOCAL:
				return (gethostent_local());
			case SVC_YP:
				return (gethostent_yp());
		}
	*cp++ = '\0';
	/* THIS STUFF IS INTERNET SPECIFIC */

	/* 
	 *	The hostent structure was modified in 4.3BSD to return
	 *	a list of host addresses, with the last address in the list
	 *	NULL.  Some 4.3 utilities expect the list, so set it up, 
	 *	but provide backward compatibility for local /etc/hosts
	 *	and YP by defining host.h_addr as the first address in 
	 *	the list.
	 */

	host.h_addr_list    = host_addrs;	/* point to the list */
	host.h_addr_list[0] = hostaddr;		/* point to the host addrs */
	host.h_addr_list[1] = (char *)0;	/* and null terminate */
	*((u_long *)host.h_addr) = inet_addr(p);
	
	host.h_length = sizeof (u_long);
	host.h_addrtype = AF_INET;
	while (*cp == ' ' || *cp == '\t')
		cp++;
	host.h_name = cp;
	q = host.h_aliases = host_aliases;
	cp = getcommon_any(cp, " \t");
	if (cp != NULL) 
		*cp++ = '\0';
	while (cp && *cp) {
		if (*cp == ' ' || *cp == '\t') {
			cp++;
			continue;
		}
		if (q < &host_aliases[MAXALIASES - 1])
			*q++ = cp;
		cp = getcommon_any(cp, " \t");
		if (cp != NULL)
			*cp++ = '\0';
	}
	*q = NULL;
	return (&host);
}

char *
hostalias(name)
	register char *name;
{
	register char *C1, *C2;
	FILE *fp;
	char *file, *getenv();
	char buf[BUFSIZ];
	static char abuf[MAXDNAME];

	file = getenv("HOSTALIASES");
	if (file == NULL || (fp = fopen(file, "r")) == NULL)
		return (NULL);
	buf[sizeof(buf) - 1] = '\0';
	while (fgets(buf, sizeof(buf), fp)) {
		for (C1 = buf; *C1 && !isspace(*C1); ++C1);
		if (!*C1)
			break;
		*C1 = '\0';
		if (!strcasecmp(buf, name)) {
			while (isspace(*++C1));
			if (!*C1)
				break;
			for (C2 = C1 + 1; *C2 && !isspace(*C2); ++C2);
			abuf[sizeof(abuf) - 1] = *C2 = '\0';
			(void)strncpy(abuf, C1, sizeof(abuf) - 1);
			fclose(fp);
			return (abuf);
		}
	}
	fclose(fp);
	return (NULL);
}

static struct hostent *
gethostdomain(name, domain)
	char *name, *domain;
{
	querybuf buf;
	char nbuf[2*MAXDNAME+2];
	char *longname = nbuf;
	int n;

	if (domain == NULL || strfind(name,domain)) {
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
	}
	else 
		(void)sprintf(nbuf, "%.*s.%.*s", MAXDNAME, name, MAXDNAME, domain);

	n = res_mkquery(QUERY, longname, C_IN, T_A, (char *)NULL, 0, NULL, (char *)&buf, sizeof(buf));
	if (n < 0) {
#ifdef DEBUG
		if (_res.options & RES_DEBUG)
			fprintf(stderr,"res_mkquery failed\n");
#endif
		return ((struct hostent *)NULL);
	}
	return (getanswer((char *)&buf, n, 0));
}

static struct hostent *
getanswer(msg, msglen, iquery)
	char *msg;
	int msglen, iquery;
{
	register HEADER *hp;
	register char *cp;
	register int n;
	querybuf answer;
	char *eom, *bp, **ap;
	int type, class, buflen, ancount, qdcount;
	int haveanswer, getclass = C_ANY;
	char **hap;

	n = res_send(msg, msglen, (char *)&answer, sizeof(answer));
	if (n < 0) {
#ifdef DEBUG
		int terrno;
		terrno = errno;
		if (_res.options & RES_DEBUG)
			fprintf(stderr,"res_send failed\n");
		errno = terrno;
#endif
		h_errno = TRY_AGAIN;
		return ((struct hostent *)NULL);
	}
	eom = (char *)&answer + n;
	/*
	 * find first satisfactory answer
	 */
	hp = (HEADER *) &answer;
	ancount = ntohs(hp->ancount);
	qdcount = ntohs(hp->qdcount);
	if (hp->rcode != NOERROR || ancount == 0) {
#ifdef DEBUG
		if (_res.options & RES_DEBUG)
			fprintf(stderr,"rcode = %d, ancount=%d\n", hp->rcode, ancount);
#endif
		switch (hp->rcode) {
			case NXDOMAIN:
				/* Check if it's an authoritive answer */
				if (hp->aa)
					h_errno = HOST_NOT_FOUND;
				else
					h_errno = TRY_AGAIN;
				break;
			case SERVFAIL:
				h_errno = TRY_AGAIN;
				break;
			case NOERROR:
				if (hp->aa)
					h_errno = NO_ADDRESS;
				else
					h_errno = TRY_AGAIN;
				break;
			case FORMERR:
			case NOTIMP:
			case REFUSED:
				h_errno = NO_RECOVERY;
		}
		return ((struct hostent *)NULL);
	}
	bp = hostbuf;
	buflen = sizeof(hostbuf);
	cp = (char *)&answer + sizeof(HEADER);
	if (qdcount) {
		if (iquery) {
			if ((n = dn_expand((char *)&answer, eom,
			     cp, bp, buflen)) < 0) {
				h_errno = NO_RECOVERY;
				return ((struct hostent *)NULL);
			}
			cp += n + QFIXEDSZ;
			host.h_name = bp;
			n = strlen(bp) + 1;
			bp += n;
			buflen -= n;
		} else
			cp += dn_skipname(cp, eom) + QFIXEDSZ;
		while (--qdcount > 0)
			cp += dn_skipname(cp, eom) + QFIXEDSZ;
	} else if (iquery) {
		if (hp->aa)
			h_errno = HOST_NOT_FOUND;
		else
			h_errno = TRY_AGAIN;
		return ((struct hostent *)NULL);
	}
	ap = host_aliases;
	host.h_aliases = host_aliases;
	hap = h_addr_ptrs;
#ifdef h_addr
	host.h_addr_list = h_addr_ptrs;
#endif
	haveanswer = 0;
	while (--ancount >= 0 && cp < eom) {
		if ((n = dn_expand((char *)&answer, eom, cp, bp, buflen)) < 0)
			break;
		cp += n;
		type = _getshort(cp);
 		cp += sizeof(u_short);
		class = _getshort(cp);
 		cp += sizeof(u_short) + sizeof(u_long);
		n = _getshort(cp);
		cp += sizeof(u_short);
		if (type == T_CNAME) {
			cp += n;
			if (ap >= &host_aliases[MAXALIASES-1])
				continue;
			*ap++ = bp;
			n = strlen(bp) + 1;
			bp += n;
			buflen -= n;
			continue;
		}
		if (type == T_PTR) {
			if ((n = dn_expand((char *)&answer, eom,
			    cp, bp, buflen)) < 0) {
				cp += n;
				continue;
			}
			cp += n;
			host.h_name = bp;
			return(&host);
		}
		if (type != T_A)  {
#ifdef DEBUG
			if (_res.options & RES_DEBUG)
				fprintf(stderr,"unexpected answer type %d, size %d\n",
					type, n);
#endif
			cp += n;
			continue;
		}
		if (haveanswer) {
			if (n != host.h_length) {
				cp += n;
				continue;
			}
			if (class != getclass) {
				cp += n;
				continue;
			}
		} else {
			host.h_length = n;
			getclass = class;
			host.h_addrtype = (class == C_IN) ? AF_INET : AF_UNSPEC;
			if (!iquery) {
				host.h_name = bp;
				bp += strlen(bp) + 1;
			}
		}

		bp += sizeof(align) - ((u_long)bp % sizeof(align));

		if (bp + n >= &hostbuf[sizeof(hostbuf)]) {
#ifdef DEBUG
			if (_res.options & RES_DEBUG)
				fprintf(stderr,"size (%d) too big\n", n);
#endif
			break;
		}
		bcopy(cp, *hap++ = bp, n);
		bp +=n;
		cp += n;
		haveanswer++;
	}
	if (haveanswer) {
		*ap = NULL;
#ifdef h_addr
		*hap = NULL;
#else
		host.h_addr = h_addr_ptrs[0];
#endif
		return (&host);
	} else {
		h_errno = TRY_AGAIN;
		return ((struct hostent *)NULL);
	}
}

/*
 * If
 *	1.  There is a dot in hname, and
 *	2.  the resolver is inited, and
 *	3.  the default domain name is not null, and
 *	4.  the end of hname contains the local domain name
 * return the length of the local host name of hname.
 */
local_hostname_length(hname)
	char *hname;
{
	char *cp;
	int i,j = 0;

	if (hname == NULL)
		return(NULL);
	i = strlen(hname);
	for (cp = hname; cp && *cp != NULL && *cp != '.'; cp++, j++)
		;
	if ((j < i) && (res_init() != -1) && (_res.defdname[0] != '\0') && ((strcasecmp(cp+1, _res.defdname)) == NULL))
		return(j);
	return(NULL);
}

/* Function:
 *
 *	strfind
 *
 * Function Description:
 *
 *	Searches for  substr  at the END of text. ie.
 *	See if one string is terminated by the occurance of another string.
 *	
 * Arguments:
 *
 *	char	*text		Text to search
 *	char	*substr		String to locate
 *
 * Return values:
 *
 *	Pointer to  substr  starting position in text if found.
 *	NULL if not found.
 *
 * Side Effects:
 *
 *	
 */
char *
strfind(text,substr)
		char *text, *substr;
{
	int	substrlen;
	int	textlen;

textlen = strlen(text);
substrlen = strlen(substr);

if (textlen > substrlen) 
	text = (text + textlen) - substrlen;

else
	return(NULL);

/* Search end of text for match.
 */
if (strncasecmp(text, substr, substrlen) == NULL)
        return(text);
else
        return(NULL);
}/*E strfind() */
