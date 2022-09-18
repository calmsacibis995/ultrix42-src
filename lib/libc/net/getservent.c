#ifndef lint
static  char    *sccsid = "@(#)getservent.c	4.8  (ULTRIX)        1/22/91";
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
 * 30-Oct-90    terry
 *      Added code to allow getservbyname_yp() and getservbyport_yp() to use
 *      getservent_yp() instead of yp_match if the server is running a 
 *      release prior to ULTRIX 4.2.
 *
 * 30-Aug-90    terry
 *    Changed getservbyname_yp() and getservbyport_yp() to use yp_match
 *    instead of yp_first/yp_next.
 *
 * 13-Nov-89	sue
 *	Changed svc_getservflag initial value to -2 and now perform a
 *	check in getservent to see if the setservent has been called yet.
 *
 * 24-Jul-89	logcher
 *	Removed generic setservent and endservent calls from generic
 *	getservbyname and getservbyport.  Added the specific set and end
 *	calls in the specific get routines.
 *
 * 25-May-89	logcher
 *	Changed name of any() to getcommon_any().
 *
 * 16-May-89	logcher
 *	Modularized the code to have separate local, yp, bind/hesiod
 *	routines.  Added front end to check the /etc/svc.conf file
 *	for the service ordering.
 *
 */

/* 
 * unlike gethost, getpw, etc, this doesn't implement getservbyxxx
 * directly
 */

#include <stdio.h>
#include <netdb.h>
#include <sys/svcinfo.h>
#include <sys/types.h>
#include <sys/param.h>
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

static char SERVDB[] = "/etc/services";
struct servent serv;
static char *domain;
static char buf[HES_BUFMAX];		/* Hesiod buffer */
static FILE *servf = NULL;
static char *current = NULL;	/* current entry, analogous to servf */
static int currentlen;
static int stayopen;
static struct svcinfo *svcinfo;

static struct servent *interpret();
static struct servent *servcommon();
static char *servskip();
char *getcommon_any();
char *yellowup();
int svc_getservflag = -2;
int svc_getservbind;

/*
 * Declare all service routines
 */
/* NOTE: _local versions of these are available in getservent_local.c.
 * Keep these static to avoid conflicts
 */
static int setservent_local (); 
static int endservent_local (); 
static struct servent* getservent_local ();
static struct servent* getservbyname_local ();
static struct servent* getservbyport_local (); 

int setent_bind (); 
int setservent_yp ();
int endent_bind (); 
int endservent_yp ();
struct servent* getservent_bind (); 
struct servent* getservent_yp ();
struct servent* getservbyport_bind (); 
struct servent* getservbyport_yp (); 
struct servent* getservbyname_bind ();	
struct servent* getservbyname_yp ();

static int	(*setservents []) ()={
		setservent_local,
		setservent_yp,
		setent_bind
};
static int	(*endservents []) ()={
		endservent_local,
		endservent_yp,
		endent_bind
};
static struct servent * (*getservents []) ()={
		getservent_local,
		getservent_yp,
		getservent_bind
};
static struct servent * (*getservbyports []) ()={
		getservbyport_local,
		getservbyport_yp,
		getservbyport_bind
};
static struct servent * (*getservbynames []) ()={
		getservbyname_local,
		getservbyname_yp,
		getservbyname_bind
};

/*
 * generic getserv service routines
 */

setservent (f)
	int f;
{
	register i;

	svc_getservflag = -1;
	svc_getservbind = 0;
	if ((svcinfo = getsvc()) != NULL)
		for (i=0; (svc_lastlookup = svcinfo->svcpath[SVC_SERVICES][i]) != SVC_LAST; i++)
			(*(setservents [svcinfo->svcpath[SVC_SERVICES][i]])) (f);
}

endservent ()
{
	register i;

	svc_getservflag = -1;
	svc_getservbind = 0;
	if ((svcinfo = getsvc()) != NULL)
		for (i=0; (svc_lastlookup = svcinfo->svcpath[SVC_SERVICES][i]) != SVC_LAST; i++)
			(*(endservents [svcinfo->svcpath[SVC_SERVICES][i]])) ();
}

struct servent *
getservent()
{
	register struct servent *p=NULL;
	register i;

	/*
	 * Check if setservent was not made yet
	 */
	if (svc_getservflag == -2)
		setservent(0);
	/*
	 * Check if this is the first time through getservent
	 */
	if (svc_getservflag == -1) {
		/*
		 * If it is, init the svcinfo structure
		 */
		if ((svcinfo = getsvc()) == NULL)
			return((struct servent *)NULL);
		i = 0;
	}
	else {
		/*
		 * If it is not, set the index to the last used one
		 */
		i = svc_getservflag;
	}
	for (; (svc_lastlookup = svcinfo->svcpath[SVC_SERVICES][i]) != SVC_LAST; i++)
		if (p = ((*(getservents [svcinfo->svcpath[SVC_SERVICES][i]])) () )) {
			svc_getservflag = i;
			break;
		}
	return (p);
}

struct servent *
getservbyname (name, proto)
	register char *name, *proto;
{
	register struct servent *p=NULL;
	register i;

	/* avoid null pointer de-reference on mips */
	if (name == 0)
		return(0);
	if ((svcinfo = getsvc()) != NULL)
		for (i=0; (svc_lastlookup = svcinfo->svcpath[SVC_SERVICES][i]) != SVC_LAST; i++)
			if (p = ((*(getservbynames [svcinfo->svcpath[SVC_SERVICES][i]])) (name, proto) ))
				break;
	return (p);
}

struct servent *
getservbyport (port, proto)
	register int port;
	register char *proto;
{
	register struct servent *p=NULL;
	register i;

	if ((svcinfo = getsvc()) != NULL)
		for (i=0; (svc_lastlookup = svcinfo->svcpath[SVC_SERVICES][i]) != SVC_LAST; i++)
			if (p = ((*(getservbyports [svcinfo->svcpath[SVC_SERVICES][i]])) (port, proto) ))
				break;
	return (p);
}

/*
 * specific getserv service routines
 */

static struct servent *
getservbyport_local(port, proto)
	int port;
	char *proto;
{
	register struct servent *p=NULL;

#ifdef DEBUG
	if (proto == 0)
		fprintf(stderr, "getservbyport_local(%d)\n", port);
	else
		fprintf(stderr, "getservbyport_local(%d/%s)\n", port, proto);
#endif DEBUG
	setservent_local(0);
	while (p = getservent_local()) {
		if (p->s_port != port)
			continue;
		if (proto == 0 || strcmp(p->s_proto, proto) == 0)
			break;
	}
	endservent_local();
	return (p);
}

struct servent *
getservbyport_bind(port, proto)
	int port;
	char *proto;
{
	char **pp, portbuf[10];

	if (proto == 0)
		sprintf(portbuf, "%d", ntohs((u_short)port));
	else
		sprintf(portbuf, "%d/%s", ntohs((u_short)port), proto);
#ifdef DEBUG
	fprintf(stderr, "getservbyport_bind(%d, %s)\n", port, portbuf);
#endif DEBUG
	setent_bind(0);
	pp = hes_resolve(portbuf, "services");
	endent_bind();
	return servcommon(pp);
}

struct servent *
getservbyport_yp(port, proto)
	int port;
	char *proto;
{
	register struct servent *p;
	int ok = 0;
	char *portstr, *val = NULL;
	int vallen = 0;

#ifdef DEBUG
	fprintf(stderr, "getservbyport_yp(%d/%s)\n", port, proto);
#endif DEBUG
	setservent_yp(0);
	if (proto != 0) {
	    portstr = (char *) malloc(sizeof(int) + strlen(proto) + 2);
	    sprintf(portstr, "%d/%s", ntohs((u_short)port), proto);
	    if (yp_match(domain, "services.byport", portstr, 
			 strlen(portstr),&val, &vallen) == 0) {
	           p = interpret(val, vallen, SVC_YP);
		   free(val);
		   ok = 1;
	     }
	  }
	if (!ok) {
	     /* YP database 'services.byport' is not served by the current YP
	      * server or proto = 0.  Use getservent() instead of yp_match.
	      */
	  
	     while (p = getservent_yp()) {
		   if (p->s_port != port)
			   continue;
		   if (proto == 0 || strcmp(p->s_proto, proto) == 0)
			   break;
	     }
	}
	endservent_yp();
	return (p);
}

static struct servent *
getservbyname_local(name, proto)
	register char *name, *proto;
{
	register struct servent *p;
	register char **cp;

#ifdef DEBUG
	fprintf(stderr, "getservbyname_local(%s)\n", name);
#endif DEBUG
	setservent_local(0);
	while (p = getservent_local()) {
		if (strcmp(name, p->s_name) == 0)
			goto gotname;
		for (cp = p->s_aliases; *cp; cp++)
			if (strcmp(name, *cp) == 0)
				goto gotname;
		continue;
gotname:
		if (proto == 0 || strcmp(p->s_proto, proto) == 0)
			break;
	}
	endservent_local();
	return (p);
}

struct servent *
getservbyname_bind(name, proto)
	register char *name, *proto;
{
	char **pp, namebuf[MAXHOSTNAMELEN];

	if (proto == 0)
		sprintf(namebuf, "%s", name);
	else
		sprintf(namebuf, "%s/%s", name, proto);
#ifdef DEBUG
	fprintf(stderr, "getservbyname_bind(%s)\n", namebuf);
#endif DEBUG
	setent_bind(0);
	pp = hes_resolve(namebuf, "services");
	endent_bind();
	return servcommon(pp);
}

struct servent *
getservbyname_yp(name, proto)
	register char *name, *proto;
{
	register struct servent *p;
	register char **cp;
	int reason;
	char *val = NULL;
	int vallen = 0;
	char *namestr;

#ifdef DEBUG
	fprintf(stderr, "getservbyname_yp(%s/%s)\n", name, proto);
#endif DEBUG
	setservent_yp(0);
 	namestr = (char *) malloc(strlen(name) + strlen(proto) + 2);
	strcpy(namestr, name);
 	strcat(namestr, "/");
	strcat(namestr, proto);
	if (reason = yp_match(domain, "services.byname_proto", namestr, strlen(namestr), &val, &vallen)) {

	     /* Server may not serve the database 'services.byname_proto'. 
	      * Use getservent_yp to find the service.
	      */
	     while (p = getservent_yp()) {
	           if (strcmp(name, p->s_name) == 0)
		   	   goto gotname2;
		   for (cp = p->s_aliases; *cp; cp++)
		   	   if (strcmp(name, *cp) == 0)
				   goto gotname2;
		   continue;
gotname2:
		   if (proto == 0 || strcmp(p->s_proto, proto) == 0)
			   break;
	     }
	}

	else {
	        /* ypmatch succeeded */
		p = interpret(val, vallen, SVC_YP);
               	free(val);
      	}

	endservent_yp();
	return (p);
}

static 
setservent_local(f)
	int f;
{
	if (servf == NULL)
		servf = fopen(SERVDB, "r");
	else
		rewind(servf);
	stayopen |= f;
}

/*
 * setent_bind(f) is in getcommon.c
 */

setservent_yp(f)
	int f;
{
	if ((domain = yellowup(1)) == NULL)
		return(setservent_local(f));
	if (current)
		free(current);
	current = NULL;
	stayopen |= f;
}

static
endservent_local()
{
	if (servf && !stayopen) {
		fclose(servf);
		servf = NULL;
	}
}

/*
 * endent_bind() is in getcommon.c
 */

endservent_yp()
{
	if (current && !stayopen) {
		free(current);
		current = NULL;
	}
}

static struct servent *
getservent_local()
{
	static char line1[BUFSIZ+1];

#ifdef DEBUG
	fprintf(stderr, "getservent_local()\n");
#endif DEBUG
	if (servf == NULL && (servf = fopen(SERVDB, "r")) == NULL)
		return (NULL);
	if (fgets(line1, BUFSIZ, servf) == NULL)
		return (NULL);
	return interpret(line1, strlen(line1), SVC_LOCAL);
}

struct servent *
getservent_bind()
{
	char bindbuf[64];
	struct servent *sp = NULL;

	sprintf(bindbuf, "services-%d", svc_getservbind);
#ifdef DEBUG
	fprintf(stderr, "getservent_bind(%s)\n", bindbuf);
#endif DEBUG
	if ((sp = getservbyname_bind(bindbuf, 0)) == NULL)
		return(NULL);
	svc_getservbind++;
	return(sp);
}

struct servent *
getservent_yp()
{
	int reason;
	char *key = NULL;
	char *val = NULL;
	int keylen = 0;
	int vallen = 0;
	struct servent *sp;

#ifdef DEBUG
	fprintf(stderr, "getservent_yp()\n");
#endif DEBUG
	if ((domain = yellowup(0)) == NULL)
		return(getservent_local());
	if (current == NULL) {
		if (reason =  yp_first(domain, "services.byname", &key, &keylen, &val, &vallen)) {
#ifdef DEBUG
			fprintf(stderr, "reason yp_first failed is %d\n", reason);
#endif
			return NULL;
		}
	}
	else {
		if (reason = yp_next(domain, "services.byname", current, currentlen, &key, &keylen, &val, &vallen)) {
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
	sp = interpret(val, vallen, SVC_YP);
	return (sp);
}

static struct servent *
interpret(val, len, svc)
	char *val;
	int len, svc;
{
	static char *serv_aliases[MAXALIASES];
	static struct servent serv;
	static char line[BUFSIZ+1];
	char *p;
	register char *cp, **q;

	strncpy(line, val, len);
	p = line;
	line[len] = '\n';
	if (*p == '#')
		switch (svc) {
			case SVC_LOCAL:
				return (getservent_local());
			case SVC_YP:
				return (getservent_yp());
		}
	cp = getcommon_any(p, "#\n");
	if (cp == NULL)
		switch (svc) {
			case SVC_LOCAL:
				return (getservent_local());
			case SVC_YP:
				return (getservent_yp());
		}
	*cp = '\0';
	serv.s_name = p;
	p = getcommon_any(p, " \t");
	if (p == NULL)
		switch (svc) {
			case SVC_LOCAL:
				return (getservent_local());
			case SVC_YP:
				return (getservent_yp());
		}
	*p++ = '\0';
	while (*p == ' ' || *p == '\t')
		p++;
	cp = getcommon_any(p, ",/");
	if (cp == NULL)
		switch (svc) {
			case SVC_LOCAL:
				return (getservent_local());
			case SVC_YP:
				return (getservent_yp());
		}
	*cp++ = '\0';
	serv.s_port = htons((u_short)atoi(p));
	serv.s_proto = cp;
	q = serv.s_aliases = serv_aliases;
	cp = getcommon_any(cp, " \t");
	if (cp != NULL)
		*cp++ = '\0';
	while (cp && *cp) {
		if (*cp == ' ' || *cp == '\t') {
			cp++;
			continue;
		}
		if (q < &serv_aliases[MAXALIASES - 1])
			*q++ = cp;
		cp = getcommon_any(cp, " \t");
		if (cp != NULL)
			*cp++ = '\0';
	}
	*q = NULL;
	return (&serv);
}

static struct servent *
servcommon(pp)
char **pp;
{
	static char *serv_aliases[MAXALIASES];
	register char *p, **q;

        if (pp == NULL)
                return(NULL);
        /* choose only the first response (only 1 expected) */
        strcpy(buf, pp[0]);
        while(*pp) free(*pp++); /* necessary to avoid leaks */
        p = buf;
        serv.s_name = p;
        p = servskip(p);
	serv.s_port = htons((u_short)atoi(p));
        p = servskip(p);
        serv.s_proto = p;
	q = serv.s_aliases = serv_aliases;
        while (*p && (p = servskip(p)) != 0) {
		if (q < &serv_aliases[MAXALIASES - 1])
			*q++ = p;
	}
        *p = '\0';
        return(&serv);
}

static char *
servskip(p)
register char *p;
{
        while (*p && *p != ':' && *p != '\n')
                ++p;
        if (*p)
                *p++ = 0;
        return(p);
}
