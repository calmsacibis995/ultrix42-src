#ifndef lint
static	char	*sccsid = "@(#)getnetent.c	4.5	(ULTRIX)	10/17/90";
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
 * 13-Nov-89	sue
 *	Changed svc_getnetflag initial value to -2 and now perform a
 *	check in getnetent to see if the setnetent has been called yet.
 *
 * 24-Jul-89	logcher
 *	Removed generic setnetent and endnetent calls from generic
 *	getnetbyname and getnetbyaddr.  Added the specific set and end
 *	calls in the specific get routines.
 *
 * 25-May-89	logcher
 *	Changed name of any() to getcommon_any().
 *
 * 16-May-89	logcher
 *      Modularized the code to have separate local, yp, bind/hesiod
 *      routines.  Added front end to check the /etc/svc.conf file
 *      for the service ordering.
 *
 * 19-Jan-88	logcher
 *	Changed getnetbyaddr() to be called a second time for
 *	network numbers ending in zero (meaning current subnet),
 *	and simplified nettoa() to strip off all trailing ".0"s.
 *
 */

#include <stdio.h>
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
#include <hesiod.h>
#include <sys/svcinfo.h>

#if defined(lint) && !defined(DEBUG)
#define DEBUG
#define DEBUG_YP
#endif

/*
 * Internet version.
 */

#define	MAXALIASES	35
#define MAXADDRS	35
#define	MAXADDRSIZE	14

static char NETDB[] = "/etc/networks";
static char buf[HES_BUFMAX];	/* Hesiod addition */
static struct netent net;	/* Hesiod addition */
static char *domain;
static int stayopen;
static FILE *netf = NULL;
static char *current = NULL;    /* current entry, analogous to netf */
static int currentlen;
static struct svcinfo *svcinfo;

static struct netent *interpret();
static char *nettoa();
static struct netent *netcommon();	/* Hesiod addition */
static char *netskip();			/* Hesiod addition */
char *inet_ntoa();
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
int svc_getnetflag = -2;
int svc_getnetbind;

/*
 * Declare all service routines
 */

/* NOTE: _local versions of these are available in getnetent_local.c.
 * Keep these static to avoid conflicts
 */
static int setnetent_local ();
static int endnetent_local ();
static struct netent *getnetent_local ();
static struct netent *getnetbyaddr_local (); 
static struct netent *getnetbyname_local ();

int setent_bind ();
int setnetent_yp ();
int endent_bind ();
int endnetent_yp ();
struct netent *getnetent_bind ();
struct netent *getnetent_yp ();
struct netent *getnetbyaddr_bind (); 
struct netent *getnetbyaddr_yp (); 
struct netent *getnetbyname_bind ();	
struct netent *getnetbyname_yp ();

static int	(*setnetents []) ()={
		setnetent_local,
		setnetent_yp,
		setent_bind
};
static int	(*endnetents []) ()={
		endnetent_local,
		endnetent_yp,
		endent_bind
};
static struct netent * (*getnetents []) ()={
		getnetent_local,
		getnetent_yp,
		getnetent_bind
};
static struct netent * (*getnetaddrs []) ()={
		getnetbyaddr_local,
		getnetbyaddr_yp,
		getnetbyaddr_bind
};
static struct netent * (*getnetnames []) ()={
		getnetbyname_local,
		getnetbyname_yp,
		getnetbyname_bind
};


/*
 * generic getnet service routines
 */

setnetent (f)
	int f;
{
	register i;

	svc_getnetflag = -1;
	svc_getnetbind = 0;
	if ((svcinfo = getsvc()) != NULL)
		for (i=0; (svc_lastlookup = svcinfo->svcpath[SVC_NETWORKS][i]) != SVC_LAST; i++)
			(*(setnetents [svcinfo->svcpath[SVC_NETWORKS][i]])) (f);
}

endnetent ()
{
	register i;

	svc_getnetflag = -1;
	svc_getnetbind = 0;
	if ((svcinfo = getsvc()) != NULL)
		for (i=0; (svc_lastlookup = svcinfo->svcpath[SVC_NETWORKS][i]) != SVC_LAST; i++)
			(*(endnetents [svcinfo->svcpath[SVC_NETWORKS][i]])) ();
}

struct netent *
getnetent()
{
	register struct netent *p=NULL;
	register i;

	/*
	 * Check if setnetent was not made yet
	 */
	if (svc_getnetflag == -2)
		setnetent(0);
	/*
	 * Check if this is the first time through getnetent
	 */
	if (svc_getnetflag == -1) {
		/*
		 * If it is, init the svcinfo structure
		 */
		if ((svcinfo = getsvc()) == NULL)
			return((struct netent *)NULL);
		i = 0;
	}
	else {
		/*
		 * If it is not, set the index to the last used one
		 */
		i = svc_getnetflag;
	}
	for (; (svc_lastlookup = svcinfo->svcpath[SVC_NETWORKS][i]) != SVC_LAST; i++)
		if (p = ((*(getnetents [svcinfo->svcpath[SVC_NETWORKS][i]])) () )) {
			svc_getnetflag = i;
			break;
		}
	return (p);
}

struct netent *
getnetbyname (name)
	register char *name;
{
	register struct netent *p=NULL;
	register i;

	/* avoid null pointer de-reference on mips */
	if (name == 0)
		return(0);
	if ((svcinfo = getsvc()) != NULL)
		for (i=0; (svc_lastlookup = svcinfo->svcpath[SVC_NETWORKS][i]) != SVC_LAST; i++)
			if (p = ((*(getnetnames [svcinfo->svcpath[SVC_NETWORKS][i]])) (name) ))
				break;
	return (p);
}

struct netent *
getnetbyaddr (net, type)
	register int net, type;
{
	register struct netent *p=NULL;
	register i;

	if ((svcinfo = getsvc()) != NULL)
		for (i=0; (svc_lastlookup = svcinfo->svcpath[SVC_NETWORKS][i]) != SVC_LAST; i++)
			if (p = ((*(getnetaddrs [svcinfo->svcpath[SVC_NETWORKS][i]])) (net, type) ))
				break;
	return (p);
}

/*
 * specific getnet service routines
 */

static struct netent *
getnetbyaddr_local(net, type)
{
	register struct netent *p;

#ifdef DEBUG
	fprintf(stderr, "getnetbyaddr_local(%d, %d)\n", net, type);
#endif DEBUG
	setnetent_local(0);
	while (p = getnetent_local()) {
		if (p->n_addrtype == type && p->n_net == net)
			break;
	}
	endnetent_local();
	return (p);
}

struct netent *
getnetbyaddr_bind(net, type)
{
	register char **pp;
	char *adrstr;

	if (type != AF_INET)
		return (NULL);
	setent_bind(0);
	adrstr = nettoa(net);
#ifdef DEBUG
	fprintf(stderr, "getnetbyaddr_bind(%s, %d)\n", adrstr, type);
#endif DEBUG
	pp = hes_resolve(adrstr, "networks");
	endent_bind();
	return netcommon(pp);
}

struct netent *
getnetbyaddr_yp(net, type)
{
	register struct netent *p;
	int reason;
	char *adrstr, *val = NULL;
	int vallen = 0;
	int found = 0;
	int first = 0;

	setnetent_yp(0);
	if ((domain = yellowup(0)) == NULL)
                return (getnetbyaddr_local(net, type));
	adrstr = nettoa(net);
#ifdef DEBUG
	fprintf(stderr, "getnetbyaddr_yp(%s, %d)\n", adrstr, type);
#endif DEBUG
	while (!found) {
		if (reason = yp_match(domain, "networks.byaddr", adrstr, strlen(adrstr), &val, &vallen)) {
#ifdef DEBUG_YP
			fprintf(stderr, "reason yp_first failed is %d\n", reason);
#endif DEBUG_YP
			p = NULL;
			/*
			 * Try one more time in case #.#.0 
			 * meaning current subnet
			 */
			if (!first) {
				strcat(adrstr, ".0\0");
				first = 1;
			}
			else
				found = 1;
		}
		else {
			p = interpret(val, vallen, SVC_YP);
			free(val);
			found = 1;
		}
	}
	endnetent_yp();
	return (p);
}

static struct netent *
getnetbyname_local(name)
	register char *name;
{
	register struct netent *p;
	register char **cp;

#ifdef DEBUG
	fprintf(stderr, "getnetbyname_local(%s)\n", name);
#endif DEBUG
	setnetent_local(0);
	while (p = getnetent_local()) {
		if (strcmp(p->n_name, name) == 0)
			break;
		for (cp = p->n_aliases; *cp != 0; cp++)
			if (strcmp(*cp, name) == 0)
				goto found;
	}
found:
	endnetent_local();
	return (p);
}

struct netent *
getnetbyname_bind(name)
	register char *name;
{
	register char **pp;

#ifdef DEBUG
	fprintf(stderr, "getnetbyname_bind called for (%s)\n", name);
#endif DEBUG
	setent_bind(0);
	pp = hes_resolve(name, "networks");
	endent_bind();
	return netcommon(pp);
}

struct netent *
getnetbyname_yp(name)
	register char *name;
{
	register struct netent *p;
	int reason;
	char *val = NULL;
	int vallen = 0;

#ifdef DEBUG
	fprintf(stderr, "getnetbyname_yp(%s)\n", name);
#endif DEBUG
	setnetent_yp(0);
	if ((domain = yellowup(0)) == NULL)
                return (getnetbyname_local(name));
	if (reason = yp_match(domain, "networks.byname", name, strlen(name), &val, &vallen)) {
#ifdef DEBUG_YP
		fprintf(stderr, "reason yp_first failed is %d\n", reason);
#endif DEBUG_YP
		p = NULL;
	}
	else {
		p = interpret(val, vallen, SVC_YP);
		free(val);
	}
	endnetent_yp();
	return (p);
}
static 
setnetent_local(f)
	int f;
{
	if (netf == NULL)
		netf = fopen(NETDB, "r");
	else
		rewind(netf);
	stayopen |= f;
}

/*
 * setent_bind(stayopen) is in getcommon.c
 */
setnetent_yp(f)
	int f;
{
	if ((domain = yellowup(1)) == NULL)
		return (setnetent_local (f));
	if (current)
		free(current);
	current = NULL;
	stayopen |= f;
}

static 
endnetent_local()
{
	if (netf && !stayopen) {
		fclose(netf);
		netf = NULL;
	}
}

/*
 * endent_bind(stayopen) is in getcommon.c
 */

endnetent_yp()
{
	if ((domain = yellowup(0)) == NULL)
		return (endnetent_local ());
	if (current && !stayopen) {
		free(current);
		current = NULL;
	}
}

static struct netent *
getnetent_local()
{
	static char line1[BUFSIZ+1];

#ifdef DEBUG
	fprintf (stderr,"getnetent_local\n");
#endif DEBUG
	if (netf == NULL && (netf = fopen(NETDB, "r")) == NULL)
		return (NULL);
        if (fgets(line1, BUFSIZ, netf) == NULL)
		return (NULL);
	return interpret(line1, strlen(line1), SVC_LOCAL);
}

struct netent *
getnetent_bind()
{
	char bindbuf[64];
	struct netent *np = NULL;

	sprintf(bindbuf, "networks-%d", svc_getnetbind);
#ifdef DEBUG
	fprintf(stderr, "getnetent_bind(%s)\n", bindbuf);
#endif DEBUG
	if ((np = getnetbyname_bind(bindbuf)) == NULL)
		return(NULL);
	svc_getnetbind++;
	return(np);
}

struct netent *
getnetent_yp()
{
	int reason;
	char *key = NULL;
	char *val = NULL;
	int keylen = 0;
	int vallen = 0;
	struct netent *np;

#ifdef DEBUG
	fprintf (stderr,"getnetent_yp\n");
#endif DEBUG
	if ((domain = yellowup(0)) == NULL)
		return (getnetent_local ());
	if (current == NULL) {
		if (reason =  yp_first(domain, "networks.byaddr", &key, &keylen, &val, &vallen)) {
#ifdef DEBUG_YP
			fprintf(stderr, "reason yp_first failed is %d\n", reason);
#endif DEBUG_YP
			return NULL;
		}
	}
	else {
		if (reason = yp_next(domain, "networks.byaddr", current, currentlen, &key, &keylen, &val, &vallen)) {
#ifdef DEBUG_YP
			fprintf(stderr, "reason yp_next failed is %d\n", reason);
#endif DEBUG_YP
			return NULL;
		}
	}
	if (current)
		free(current);
	current = key;
	currentlen = keylen;
	np = interpret(val, vallen, SVC_YP);
	free(val);
	return (np);
}

static struct netent *
interpret(val, len, svc)
	char *val;
	int len, svc;
{
	static char *net_aliases[MAXALIASES];
	static struct netent net;
	static char line[BUFSIZ+1];
	char *p;
	register char *cp, **q;

	strncpy(line, val, len);
	p = line;
	line[len] = '\n';
	if (*p == '#')
		switch (svc) {
			case SVC_LOCAL:
				return (getnetent_local());
			case SVC_YP:
				return (getnetent_yp());
		}
	cp = getcommon_any(p, "#\n");
	if (cp == NULL)
		switch (svc) {
			case SVC_LOCAL:
				return (getnetent_local());
			case SVC_YP:
				return (getnetent_yp());
		}
	*cp = '\0';
	net.n_name = p;
	cp = getcommon_any(p, " \t");
	if (cp == NULL)
		switch (svc) {
			case SVC_LOCAL:
				return (getnetent_local());
			case SVC_YP:
				return (getnetent_yp());
		}
	*cp++ = '\0';
	while (*cp == ' ' || *cp == '\t')
		cp++;
	p = getcommon_any(cp, " \t");
	if (p != NULL)
		*p++ = '\0';
	net.n_net = inet_network(cp);
	net.n_addrtype = AF_INET;
	q = net.n_aliases = net_aliases;
	if (p != NULL) 
		cp = p;
	while (cp && *cp) {
		if (*cp == ' ' || *cp == '\t') {
			cp++;
			continue;
		}
		if (q < &net_aliases[MAXALIASES - 1])
			*q++ = cp;
		cp = getcommon_any(cp, " \t");
		if (cp != NULL)
			*cp++ = '\0';
	}
	*q = NULL;
	return (&net);
}

/*
 * Strip off any trailing ".0" after inet_makeaddr
 */

static char *
nettoa(net)
	unsigned net;
{
	static char buf[16];
	char *p, *rindex();
	struct in_addr in;

	in = inet_makeaddr(net, INADDR_ANY);
	strcpy(buf, inet_ntoa(in));
	while (p = rindex(buf, '.'))
		if (!strcmp(p+1, "0"))
			*p = '\0';
		else
			break;
	return(buf);
}

static struct netent *
netcommon(pp)
char **pp;
{
	static char *net_aliases[MAXALIASES];
        register char *p, **q;

        if (pp == NULL)
                return(NULL);
        strcpy(buf, pp[0]);
        while(*pp)
		free(*pp++); /* necessary to avoid leaks */
        p = buf;
        net.n_name = p;
        p = netskip(p);
        net.n_net = inet_network(p);
        net.n_addrtype = AF_INET;
	q = net.n_aliases = net_aliases;
	while (*p && (p = netskip(p)) != 0) {
		if (q < &net_aliases[MAXALIASES - 1])
			*q++ = p;
	}
        while (*p && *p != '\n')
                p++;
        *p = '\0';
        return(&net);
}

static char *
netskip(p)
register char *p;
{
        while (*p && *p != ':' && *p != '\n')
                ++p;
        if (*p)
                *p++ = 0;
        return(p);
}
