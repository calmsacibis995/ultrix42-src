#ifndef lint
static	char	*sccsid = "@(#)getrpcent.c	4.2	(ULTRIX)	9/4/90";
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
 *	Changed svc_getrpcflag initial value to -2 and now perform a
 *	check in getrpcent to see if the setrpcent has been called yet.
 *
 * 24-Jul-89	logcher
 *	Removed generic setrpcent and endrpcent calls from generic
 *	getrpcbyname and getrpcbynumber.  Added the specific set and end
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
 */

#include <stdio.h>
#include <netdb.h>
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

static char RPCDB[] = "/etc/rpc";
static char buf[HES_BUFMAX];   /* Hesiod addition */
static struct rpcent rpc;	/* Hesiod addition */
static char *domain;
static FILE *rpcf = NULL;
static char *current = NULL;	/* current entry, analogous to hostf */
static int currentlen;
static int stayopen;
static struct svcinfo *svcinfo;

static struct rpcent *interpret();
static struct rpcent *rpccommon();
static char *rpcskip();
char *getcommon_any();
char *yellowup();
int svc_getrpcflag = -2;
int svc_getrpcbind;

/*
 * Declare all service routines
 */
int setrpcent_local (); 
int setent_bind (); 
int setrpcent_yp ();
int endrpcent_local (); 
int endent_bind (); 
int endrpcent_yp ();
struct rpcent* getrpcent_local ();
struct rpcent* getrpcent_bind (); 
struct rpcent* getrpcent_yp ();
struct rpcent* getrpcbynumber_local (); 
struct rpcent* getrpcbynumber_bind (); 
struct rpcent* getrpcbynumber_yp (); 
struct rpcent* getrpcbyname_local ();
struct rpcent* getrpcbyname_bind ();	
struct rpcent* getrpcbyname_yp ();

static int	(*setrpcents []) ()={
		setrpcent_local,
		setrpcent_yp,
		setent_bind
};
static int 	(*endrpcents []) ()={
		endrpcent_local,
		endrpcent_yp,
		endent_bind
};
static struct rpcent * (*getrpcents []) ()={
		getrpcent_local,
		getrpcent_yp,
		getrpcent_bind
};
static struct rpcent * (*getrpcbynumbers []) ()={
		getrpcbynumber_local,
		getrpcbynumber_yp,
		getrpcbynumber_bind
};
static struct rpcent * (*getrpcbynames []) ()={
		getrpcbyname_local,
		getrpcbyname_yp,
		getrpcbyname_bind
};


/*
 * generic getrpc service routines
 */

setrpcent (f)
	int f;
{
	register i;

	svc_getrpcflag = -1;
	svc_getrpcbind = 0;
	if ((svcinfo = getsvc()) != NULL)
		for (i=0; (svc_lastlookup = svcinfo->svcpath[SVC_RPC][i]) != SVC_LAST; i++)
			(*(setrpcents [svcinfo->svcpath[SVC_RPC][i]])) (f);
}

endrpcent ()
{
	register i;

	svc_getrpcflag = -1;
	svc_getrpcbind = 0;
	if ((svcinfo = getsvc()) != NULL)
		for (i=0; (svc_lastlookup = svcinfo->svcpath[SVC_RPC][i]) != SVC_LAST; i++)
			(*(endrpcents [svcinfo->svcpath[SVC_RPC][i]])) ();
}

struct rpcent *
getrpcent()
{
	register struct rpcent *p=NULL;
	register i;

	/*
	 * Check if setrpcent was not made yet
	 */
	if (svc_getrpcflag == -2)
		setrpcent(0);
	/*
	 * Check if this is the first time through getrpcent
	 */
	if (svc_getrpcflag == -1) {
		/*
		 * If it is, init the svcinfo structure
		 */
		if ((svcinfo = getsvc()) == NULL)
			return((struct rpcent *)NULL);
		i = 0;
	}
	else {
		/*
		 * If it is not, set the index to the last used one
		 */
		i = svc_getrpcflag;
	}
	for (; (svc_lastlookup = svcinfo->svcpath[SVC_RPC][i]) != SVC_LAST; i++)
		if (p = ((*(getrpcents [svcinfo->svcpath[SVC_RPC][i]])) () )) {
			svc_getrpcflag = i;
			break;
		}
	return (p);
}

struct rpcent *
getrpcbyname (name)
	register char *name;
{
	register struct rpcent *p=NULL;
	register i;

	/* avoid null pointer de-reference on mips */
	if (name == 0)
		return(0);
	if ((svcinfo = getsvc()) != NULL)
		for (i=0; (svc_lastlookup = svcinfo->svcpath[SVC_RPC][i]) != SVC_LAST; i++)
			if (p = ((*(getrpcbynames [svcinfo->svcpath[SVC_RPC][i]])) (name) ))
				break;
	return (p);
}

struct rpcent *
getrpcbynumber (number)
	register int number;
{
	register struct rpcent *p=NULL;
	register i;

	if ((svcinfo = getsvc()) != NULL)
		for (i=0; (svc_lastlookup = svcinfo->svcpath[SVC_RPC][i]) != SVC_LAST; i++)
			if (p = ((*(getrpcbynumbers [svcinfo->svcpath[SVC_RPC][i]])) (number) ))
				break;
	return (p);
}

/*
 * specific getrpc service routines
 */

struct rpcent *
getrpcbynumber_local(number)
	register int number;
{
	register struct rpcent *p;

#ifdef DEBUG
	fprintf (stderr,"getrpcbynumber_local(%d)\n", number);
#endif DEBUG
	setrpcent_local(0);
	while (p = getrpcent_local()) {
		if (p->r_number == number)
			break;
	}
	endrpcent_local();
	return (p);
}

struct rpcent *
getrpcbynumber_bind(number)
	register int number;
{
	char **pp, numbuf[12];

#ifdef DEBUG
	fprintf (stderr,"getrpcbynumber_bind(%d)\n", number);
#endif DEBUG
	setent_bind(0);
	sprintf(numbuf, "%d", number);
	pp = hes_resolve(numbuf, "rpc");
	endent_bind();
	return rpccommon(pp);
}

struct rpcent *
getrpcbynumber_yp(number)
	register int number;
{
	register struct rpcent *p;
	int reason;
	char adrstr[10], *val = NULL;
	int vallen = 0;

#ifdef DEBUG
	fprintf (stderr,"getrpcbynumber_yp(%d)\n", number);
#endif DEBUG
	setrpcent_yp(0);
	sprintf(adrstr, "%d", number);
	if (reason = yp_match(domain, "rpc.bynumber", adrstr, strlen(adrstr), &val, &vallen)) {
#ifdef DEBUG
		fprintf(stderr, "reason yp_first failed is %d\n", reason);
#endif
		p = NULL;
	}
	else {
		p = interpret(val, vallen, SVC_YP);
		free(val);
	}
	endrpcent_yp();
	return (p);
}

struct rpcent *
getrpcbyname_local(name)
	char *name;
{
	register struct rpcent *rpc=NULL;
	register char **rp;

#ifdef DEBUG
	fprintf (stderr,"getrpcbyname_local(%s)\n", name);
#endif DEBUG
	setrpcent_local(0);
	while (rpc = getrpcent_local()) {
		if (strcmp(rpc->r_name, name) == 0)
			break;
		for (rp = rpc->r_aliases; *rp != 0; rp++)
			if (strcmp(*rp, name) == 0)
				goto found;
	}
found:
	endrpcent_local();
	return (rpc);
}

struct rpcent *
getrpcbyname_bind(name)
	char *name;
{
	char **pp;

#ifdef DEBUG
	fprintf (stderr,"getrpcbyname_bind(%s)\n", name);
#endif DEBUG
	setent_bind(0);
	pp = hes_resolve(name, "rpc");
	endent_bind();
	return rpccommon(pp);
}

struct rpcent *
getrpcbyname_yp(name)
	char *name;
{
	register struct rpcent *rpc=NULL;
	register char **rp;

#ifdef DEBUG
	fprintf (stderr,"getrpcbyname_yp(%s)\n", name);
#endif DEBUG
	setrpcent_yp(0);
	while (rpc = getrpcent_yp()) {
		if (strcmp(rpc->r_name, name) == 0)
			break;
		for (rp = rpc->r_aliases; *rp != 0; rp++)
			if (strcmp(*rp, name) == 0)
				goto found2;
	}
found2:
	endrpcent_yp();
	return (rpc);
}

setrpcent_local(f)
	int f;
{
	if (rpcf == NULL)
		rpcf = fopen(RPCDB, "r");
	else
		rewind(rpcf);
	stayopen |= f;
}

/*
 * setent_bind(f) is in getcommon.c
 */

setrpcent_yp(f)
{
	if ((domain = yellowup(1)) == NULL)
		return(setrpcent_local(f));
	if (current)
		free(current);
	current = NULL;
}

endrpcent_local()
{
	if (rpcf && !stayopen) {
		fclose(rpcf);
		rpcf = NULL;
	}
}

/*
 * endent_bind() is in getcommon.c
 */

endrpcent_yp()
{
	if (current && !stayopen) {
		free(current);
		current = NULL;
	}
}

struct rpcent *
getrpcent_local()
{
	static char line1[BUFSIZ+1];

	if (rpcf == NULL && (rpcf = fopen(RPCDB, "r")) == NULL)
		return (NULL);
        if (fgets(line1, BUFSIZ, rpcf) == NULL)
		return (NULL);
	return interpret(line1, strlen(line1), SVC_LOCAL);
}

struct rpcent *
getrpcent_bind()
{
	char bindbuf[64];
	struct rpcent *hp = NULL;

	sprintf(bindbuf, "rpc-%d", svc_getrpcbind);
#ifdef DEBUG
	fprintf(stderr, "getrpcent_bind(%s)\n", bindbuf);
#endif DEBUG
	if ((hp = getrpcbyname_bind(bindbuf)) == NULL)
		return(NULL);
	svc_getrpcbind++;
	return(hp);
}

struct rpcent *
getrpcent_yp()
{
	struct rpcent *hp;
	int reason;
	char *key = NULL;
	char *val = NULL;
	int keylen = 0;
	int vallen = 0;

	if ((domain = yellowup(0)) == NULL)
		return(getrpcent_local());
	if (current == NULL) {
		if (reason =  yp_first(domain, "rpc.bynumber", &key, &keylen, &val, &vallen)) {
#ifdef DEBUG
			fprintf(stderr, "reason yp_first failed is %d\n", reason);
#endif
			return NULL;
		}
	}
	else {
		if (reason = yp_next(domain, "rpc.bynumber", current, currentlen, &key, &keylen, &val, &vallen)) {
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

static struct rpcent *
interpret(val, len, svc)
	char *val;
	int len, svc;
{
	static char *rpc_aliases[MAXALIASES];
	static struct rpcent rpc;
	static char line[BUFSIZ+1];
	char *p;
	register char *cp, **q;

	strncpy(line, val, len);
	p = line;
	line[len] = '\n';
	if (*p == '#')
		switch (svc) {
			case SVC_LOCAL:
				return (getrpcent_local());
			case SVC_YP:
				return (getrpcent_yp());
		}
	cp = getcommon_any(p, "#\n");
	if (cp == NULL)
		switch (svc) {
			case SVC_LOCAL:
				return (getrpcent_local());
			case SVC_YP:
				return (getrpcent_yp());
		}
	*cp = '\0';
	cp = getcommon_any(p, " \t");
	if (cp == NULL)
		switch (svc) {
			case SVC_LOCAL:
				return (getrpcent_local());
			case SVC_YP:
				return (getrpcent_yp());
		}
	*cp++ = '\0';
	/* THIS STUFF IS INTERNET SPECIFIC */
	rpc.r_name = line;
	while (*cp == ' ' || *cp == '\t')
		cp++;
	rpc.r_number = atoi(cp);
	q = rpc.r_aliases = rpc_aliases;
	cp = getcommon_any(cp, " \t");
	if (cp != NULL) 
		*cp++ = '\0';
	while (cp && *cp) {
		if (*cp == ' ' || *cp == '\t') {
			cp++;
			continue;
		}
		if (q < &rpc_aliases[MAXALIASES - 1])
			*q++ = cp;
		cp = getcommon_any(cp, " \t");
		if (cp != NULL)
			*cp++ = '\0';
	}
	*q = NULL;
	return (&rpc);
}

static
struct rpcent *
rpccommon(pp)
char **pp;
{
	static char *rpc_aliases[MAXALIASES];
        register char *p, **q;

        if (pp == NULL)
                return(NULL);
        /* choose only the first response (only 1 expected) */
        strcpy(buf, pp[0]);
        while(*pp) free(*pp++); /* necessary to avoid leaks */
        p = buf;
        rpc.r_name = p;
        p = rpcskip(p);
        rpc.r_number = atoi(p);
	q = rpc.r_aliases = rpc_aliases;
	while (*p && (p = rpcskip(p)) != 0) {
		if (q < &rpc_aliases[MAXALIASES - 1])
			*q++ = p;
	}
        while (*p && *p != '\n')
                p++;
        *p = '\0';
        return(&rpc);
}

static
char *
rpcskip(p)
register char *p;
{
        while (*p && *p != ':' && *p != '\n')
                ++p;
        if (*p)
                *p++ = 0;
        return(p);
}
