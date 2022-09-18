#ifndef lint
static	char	*sccsid = "@(#)getnetent_local.c	4.2	(ULTRIX)	10/17/90";
#endif
/* This is the local only version of network name lookups, for use
 * by STANDALONE utilities.  It only looks at /etc/networks
 *
 * WARNING:
 * This code is cloned from getnetent.c, any changes made here
 * should made over there too.
 *
 *	getnetbyaddr_local()
 *	getnetbyname_local()
 *
 */
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#define	MAXALIASES	35

static char NETDB[] = "/etc/networks";
static int stayopen;
static FILE *netf = NULL;
static struct netent *interpret_local();
struct netent *getnetent_local();
static char *getcommon_any();

struct netent *
getnetbyaddr_local(net, type)	/* needs to be global */
{
	register struct netent *p;

	setnetent_local(0);
	while (p = getnetent_local()) {
		if (p->n_addrtype == type && p->n_net == net)
			break;
	}
	endnetent_local();
	return (p);
}

struct netent *
getnetbyname_local(name)	/* needs to be global */
	register char *name;
{
	register struct netent *p;
	register char **cp;

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

setnetent_local(f)
	int f;
{
	if (netf == NULL)
		netf = fopen(NETDB, "r");
	else
		rewind(netf);
	stayopen |= f;
}

endnetent_local()
{
	if (netf && !stayopen) {
		fclose(netf);
		netf = NULL;
	}
}

struct netent *
getnetent_local()
{
	static char line1[BUFSIZ+1];

	if (netf == NULL && (netf = fopen(NETDB, "r")) == NULL)
		return (NULL);
        if (fgets(line1, BUFSIZ, netf) == NULL)
		return (NULL);
	return interpret_local(line1, strlen(line1));
}

/* cloned from interpret() in getnetent.c, but with SVC_LOCAL hard-coded */
static struct netent *
interpret_local(val, len)
	char *val;
	int len;
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
		return (getnetent_local());
	cp = getcommon_any(p, "#\n");
	if (cp == NULL)
		return (getnetent_local());
	*cp = '\0';
	net.n_name = p;
	cp = getcommon_any(p, " \t");
	if (cp == NULL)
		return (getnetent_local());
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

/* source cloned from getcommon_any() in getcommon.c */
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
