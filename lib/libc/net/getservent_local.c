#ifndef lint
static	char	*sccsid = "@(#)getservent_local.c	4.2	(ULTRIX)	10/17/90";
#endif
/* This is the local only version of service name lookups, for use
 * by STANDALONE utilities.  It only looks at /etc/services
 *
 * WARNING:
 * This code is cloned from getservent.c, any changes made here
 * should be made over there too.
 *
 *	getservbyport_local()
 *	getservbyname_local()
 *
 */

#include <stdio.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/socket.h>

#define	MAXALIASES	35

static char SERVDB[] = "/etc/services";
static struct servent *interpret_local();
static char *getcommon_any();
static FILE *servf = NULL;
static int stayopen;

setservent_local(f)
	int f;
{
	if (servf == NULL)
		servf = fopen(SERVDB, "r");
	else
		rewind(servf);
	stayopen |= f;
}

endservent_local()
{
	if (servf && !stayopen) {
		fclose(servf);
		servf = NULL;
	}
}

struct servent *
getservent_local()
{
	static char line1[BUFSIZ+1];

	if (servf == NULL && (servf = fopen(SERVDB, "r")) == NULL)
		return (NULL);
	if (fgets(line1, BUFSIZ, servf) == NULL)
		return (NULL);
	return interpret_local(line1, strlen(line1));
}

struct servent *
getservbyport_local(port, proto) /* this needs to be global */
	int port;
	char *proto;
{
	register struct servent *p=NULL;

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
getservbyname_local(name, proto) /* this needs to be global */
	register char *name, *proto;
{
	register struct servent *p;
	register char **cp;

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

static struct servent *
interpret_local(val, len)
	char *val;
	int len;
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
		return (getservent_local());
	cp = getcommon_any(p, "#\n");
	if (cp == NULL)
		return (getservent_local());
	*cp = '\0';
	serv.s_name = p;
	p = getcommon_any(p, " \t");
	if (p == NULL)
		return (getservent_local());
	*p++ = '\0';
	while (*p == ' ' || *p == '\t')
		p++;
	cp = getcommon_any(p, ",/");
	if (cp == NULL)
		return (getservent_local());
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

/* source cloned from getcommon_any() in getcommon.c: */
static char *
getcommon_any(cp, match)
	register char *cp;
	char *match;
{
	register char *mp, c;

	while (c = *cp) {
		for (mp = match; *mp; mp++)
			if (*mp == c)
				return (cp);
		cp++;
	}
	return ((char *)0);
}
