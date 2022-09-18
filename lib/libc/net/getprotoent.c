#ifndef lint
static	char	*sccsid = "@(#)getprotoent.c	4.2	(ULTRIX)	9/4/90";
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
 *	Changed svc_getprotoflag initial value to -2 and now perform a
 *	check in getprotoent to see if the setprotoent has been called
 *	yet.
 *
 * 24-Jul-89	logcher
 *	Removed generic setprotoent and endprotoent calls from generic
 *	getprotobyname and getprotobynumber.  Added the specific set
 *	and end calls in the specific get routines.
 *
 * 25-May-89	logcher
 *	Changed name of any() to getcommon_any().
 *
 * 16-May-89	logcher
 *      Modularized the code to have separate local, yp, bind/hesiod
 *      routines.  Added front end to check the /etc/svc.conf file
 *      for the service ordering.
 *
 */

#include <netdb.h>
#include <stdio.h>
#include <sys/svcinfo.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <rpcsvc/ypclnt.h>
#include <hesiod.h>

#if defined(lint) && !defined(DEBUG)
#define DEBUG
#endif

/*
 * Internet version.
 */
#define	MAXALIASES	35
#define	MAXADDRSIZE	14

static char PROTODB[] = "/etc/protocols";
static char buf[HES_BUFMAX];		/* Hesiod addition */
static struct protoent proto;		/* Hesiod addition */
static char *domain;
static int stayopen;
static FILE *protof = NULL;
static char *current = NULL;	/* current entry, analogous to protof */
static int currentlen;
static struct svcinfo *svcinfo;

static struct protoent *protocommon();	/* Hesiod addition */
static char *protoskip();		/* Hesiod addition */
static struct protoent *interpret();
char *inet_ntoa();
char *getcommon_any();
char *yellowup();
int svc_getprotoflag = -2;
int svc_getprotobind;

int setprotoent_local (); 
int setent_bind (); 
int setprotoent_yp ();
int endprotoent_local (); 
int endent_bind (); 
int endprotoent_yp ();
struct protoent* getprotoent_local ();
struct protoent* getprotoent_bind (); 
struct protoent* getprotoent_yp ();
struct protoent* getprotobynumber_local (); 
struct protoent* getprotobynumber_bind (); 
struct protoent* getprotobynumber_yp (); 
struct protoent* getprotobyname_local ();
struct protoent* getprotobyname_bind ();	
struct protoent* getprotobyname_yp ();

static int	(*setprotoents []) ()={
		setprotoent_local,
		setprotoent_yp,
		setent_bind
};
static int 	(*endprotoents []) ()={
		endprotoent_local,
		endprotoent_yp,
		endent_bind
};
static struct protoent * (*getprotoents []) ()={
		getprotoent_local,
		getprotoent_yp,
		getprotoent_bind
};
static struct protoent * (*getprotobynumbers []) ()={
		getprotobynumber_local,
		getprotobynumber_yp,
		getprotobynumber_bind
};
static struct protoent * (*getprotobynames []) ()={
		getprotobyname_local,
		getprotobyname_yp,
		getprotobyname_bind
};


/*
 *	The generic getproto service routines
 */

setprotoent (f)
	int f;
{
	register i;

	svc_getprotoflag = -1;
	svc_getprotobind = 0;
	if ((svcinfo = getsvc()) != NULL)
		for (i=0; (svc_lastlookup = svcinfo->svcpath[SVC_PROTOCOLS][i]) != SVC_LAST; i++)
			(*(setprotoents [svcinfo->svcpath[SVC_PROTOCOLS][i]])) (f);
}

endprotoent ()
{
	register i;

	svc_getprotoflag = -1;
	svc_getprotobind = 0;
	if ((svcinfo = getsvc()) != NULL)
		for (i=0; (svc_lastlookup = svcinfo->svcpath[SVC_PROTOCOLS][i]) != SVC_LAST; i++)
			(*(endprotoents [svcinfo->svcpath[SVC_PROTOCOLS][i]])) ();
}

struct protoent *
getprotoent()
{
	register struct protoent *p=NULL;
	register i;

	/*
	 * Check if setprotoent was not made yet
	 */
	if (svc_getprotoflag == -2)
		setprotoent(0);
	/*
	 * Check if this is the first time through getprotoent
	 */
	if (svc_getprotoflag == -1) {
		/*
		 * If it is, init the svcinfo structure
		 */
		if ((svcinfo = getsvc()) == NULL)
			return((struct protoent *)NULL);
		i = 0;
	}
	else {
		/*
		 * If it is not, set the index to the last used one
		 */
		i = svc_getprotoflag;
	}
	for (; (svc_lastlookup = svcinfo->svcpath[SVC_PROTOCOLS][i]) != SVC_LAST; i++)
		if (p = ((*(getprotoents [svcinfo->svcpath[SVC_PROTOCOLS][i]])) () )) {
			svc_getprotoflag = i;
			break;
		}
	return (p);
}

struct protoent *
getprotobyname (name)
	register char *name;
{
	register struct protoent *p=NULL;
	register i;

	/* avoid null pointer de-reference on mips */
	if (name == 0)
		return(0);
	if ((svcinfo = getsvc()) != NULL)
		for (i=0; (svc_lastlookup = svcinfo->svcpath[SVC_PROTOCOLS][i]) != SVC_LAST; i++)
			if (p = ((*(getprotobynames [svcinfo->svcpath[SVC_PROTOCOLS][i]])) (name) ))
				break;
	return (p);
}

struct protoent *
getprotobynumber (proto)
	register int proto;
{
	register struct protoent *p=NULL;
	register i;

	if ((svcinfo = getsvc()) != NULL)
		for (i=0; (svc_lastlookup = svcinfo->svcpath[SVC_PROTOCOLS][i]) != SVC_LAST; i++)
			if (p = ((*(getprotobynumbers [svcinfo->svcpath[SVC_PROTOCOLS][i]])) (proto) ))
				break;
	return (p);
}

/*
 * specific getproto service routines
 */

struct protoent *
getprotobynumber_local(proto)
{
	register struct protoent *p;

#ifdef DEBUG
	fprintf(stderr, "getprotobynumber_local(%d)\n", proto);
#endif DEBUG
	setprotoent_local(0);
	while (p = getprotoent_local()) {
		if (p->p_proto == proto)
			break;
	}
	endprotoent_local();
	return (p);
}

struct protoent *
getprotobynumber_bind(proto)
	int proto;
{
	char **pp, protobuf[12];

#ifdef DEBUG
	fprintf(stderr, "getprotobynumber_bind(%d)\n", proto);
#endif DEBUG
	setent_bind(0);
	sprintf(protobuf, "%d", proto);
	pp = hes_resolve(protobuf, "protocols");
	endent_bind();
	return protocommon(pp);
}

struct protoent *
getprotobynumber_yp(proto)
{
	register struct protoent *p;
	int reason;
	char adrstr[12], *val = NULL;
	int vallen = 0;

#ifdef DEBUG
	fprintf(stderr, "getprotobynumber_yp(%d)\n", proto);
#endif DEBUG
	setprotoent_yp(0);
	if ((domain = yellowup(0)) == NULL)
		return (getprotobynumber_local(proto));
	sprintf(adrstr, "%d", proto);
	if (reason = yp_match(domain, "protocols.bynumber", adrstr, strlen(adrstr), &val, &vallen)) {
#ifdef DEBUG
		fprintf(stderr, "reason yp_first failed is %d\n", reason);
#endif
		p = NULL;
	}
	else {
		p = interpret(val, vallen, SVC_YP);
		free(val);
	}
	endprotoent_yp();
	return (p);
}

struct protoent *
getprotobyname_local(name)
	register char *name;
{
	register struct protoent *p;
	register char **cp;

#ifdef DEBUG
	fprintf(stderr, "getprotobyname_local(%s)\n", name);
#endif DEBUG
	setprotoent_local(0);
	while (p = getprotoent_local()) {
		if (strcmp(p->p_name, name) == 0)
			break;
		for (cp = p->p_aliases; *cp != 0; cp++)
			if (strcmp(*cp, name) == 0)
				goto found;
	}
found:
	endprotoent_local();
	return (p);
}

struct protoent *
getprotobyname_bind(name)
	register char *name;
{
	char **pp; 

#ifdef DEBUG
	fprintf(stderr, "getprotobyname_bind(%s)\n", name);
#endif DEBUG
	setent_bind(0);
	pp = hes_resolve(name, "protocols");
	endent_bind();
	return protocommon(pp);
}

struct protoent *
getprotobyname_yp(name)
	register char *name;
{
	register struct protoent *p;
	int reason;
	char *val = NULL;
	int vallen = 0;

#ifdef DEBUG
	fprintf(stderr, "getprotobyname_yp(%s)\n", name);
#endif DEBUG
	setprotoent_yp(0);
	if ((domain = yellowup(0)) == NULL)
		return (getprotobyname_local(name));
	if (reason = yp_match(domain, "protocols.byname", name, strlen(name), &val, &vallen)) {
#ifdef DEBUG
		fprintf(stderr, "reason yp_first failed is %d\n", reason);
#endif
		p = NULL;
	}
	else {
		p = interpret(val, vallen, SVC_YP);
		free(val);
	}
	endprotoent_yp();
	return (p);
}

setprotoent_local(f)
	int f;
{
	if (protof == NULL)
		protof = fopen(PROTODB, "r");
	else
		rewind(protof);
	stayopen |= f;
}

/*
 * setent_bind(f) is in getcommon.c
 */

setprotoent_yp(f)
	int f;
{
	if ((domain = yellowup(1)) == NULL)
		return (setprotoent_local(f));
	if (current)
		free(current);
	current = NULL;
}

endprotoent_local()
{
	if (protof && !stayopen) {
		fclose(protof);
		protof = NULL;
	}
}

/*
 * endent_bind() is in getcommon.c
 */

endprotoent_yp()
{
	if ((domain = yellowup(0)) == NULL)
		return (endprotoent_local());
	if (current && !stayopen) {
		free(current);
		current = NULL;
	}
}

struct protoent *
getprotoent_local()
{
	static char line1[BUFSIZ+1];

	if (protof == NULL && (protof = fopen(PROTODB, "r")) == NULL)
		return (NULL);
        if (fgets(line1, BUFSIZ, protof) == NULL)
		return (NULL);
	return interpret(line1, strlen(line1), SVC_LOCAL);
}

struct protoent *
getprotoent_bind()
{
	char bindbuf[64];
	struct protoent *pp = NULL;

	sprintf(bindbuf, "protocols-%d", svc_getprotobind);
#ifdef DEBUG
	fprintf(stderr, "getprotoent_bind(%s)\n", bindbuf);
#endif DEBUG
	if ((pp = getprotobyname_bind(bindbuf)) == NULL)
		return(NULL);
	svc_getprotobind++;
	return(pp);
}

struct protoent *
getprotoent_yp()
{
	int reason;
	char *key = NULL;
	char *val = NULL;
	int keylen = 0;
	int vallen = 0;
	struct protoent *pp;

	if ((domain = yellowup(0)) == NULL)
		return (getprotoent_local());
	if (current == NULL) {
		if (reason =  yp_first(domain, "protocols.bynumber", &key, &keylen, &val, &vallen)) {
#ifdef DEBUG
			fprintf(stderr, "reason yp_first failed is %d\n", reason);
#endif
			return NULL;
		}
	}
	else {
		if (reason = yp_next(domain, "protocols.bynumber", current, currentlen, &key, &keylen, &val, &vallen)) {
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
	pp = interpret(val, vallen, SVC_YP);
	free(val);
	return (pp);
}

static struct protoent *
interpret(val, len, svc)
	char *val;
	int len, svc;
{
	static char *proto_aliases[MAXALIASES];
	static struct protoent proto;
	static char line[BUFSIZ+1];
	char *p;
	register char *cp, **q;

	strncpy(line, val, len);
	p = line;
	line[len] = '\n';
	if (*p == '#')
		switch (svc) {
			case SVC_LOCAL:
				return (getprotoent_local());
			case SVC_YP:
				return (getprotoent_yp());
		}
	cp = getcommon_any(p, "#\n");
	if (cp == NULL)
		switch (svc) {
			case SVC_LOCAL:
				return (getprotoent_local());
			case SVC_YP:
				return (getprotoent_yp());
		}
	*cp = '\0';
	proto.p_name = p;
	cp = getcommon_any(p, " \t");
	if (cp == NULL)
		switch (svc) {
			case SVC_LOCAL:
				return (getprotoent_local());
			case SVC_YP:
				return (getprotoent_yp());
		}
	*cp++ = '\0';
	while (*cp == ' ' || *cp == '\t')
		cp++;
	p = getcommon_any(cp, " \t");
	if (p != NULL)
		*p++ = '\0';
	proto.p_proto = atoi(cp);
	q = proto.p_aliases = proto_aliases;
	if (p != NULL) {
		cp = p;
		while (cp && *cp) {
			if (*cp == ' ' || *cp == '\t') {
				cp++;
				continue;
			}
			if (q < &proto_aliases[MAXALIASES - 1])
				*q++ = cp;
			cp = getcommon_any(cp, " \t");
			if (cp != NULL)
				*cp++ = '\0';
		}
	}
	*q = NULL;
	return (&proto);
}

static
struct protoent *
protocommon(pp)
char **pp;
{
	static char *proto_aliases[MAXALIASES];
        register char *p, **q;

        if (pp == NULL)
                return(NULL);
        /* choose only the first response (only 1 expected) */
        strcpy(buf, pp[0]);
        while(*pp) free(*pp++); /* necessary to avoid leaks */
        p = buf;
        proto.p_name = p;
        p = protoskip(p);
        proto.p_proto = atoi(p);
	q = proto.p_aliases = proto_aliases;
	while (*p && (p = protoskip(p)) != 0) {
		if (q < &proto_aliases[MAXALIASES - 1])
			*q++ = p;
	}
        while (*p && *p != '\n')
                p++;
        *p = '\0';
        return(&proto);
}

static
char *
protoskip(p)
register char *p;
{
        while (*p && *p != ':' && *p != '\n')
                ++p;
        if (*p)
                *p++ = 0;
        return(p);
}
