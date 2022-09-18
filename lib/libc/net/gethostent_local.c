#ifndef lint
static	char	*sccsid = "@(#)gethostent_local.c	4.2	(ULTRIX)	10/17/90";
#endif
/*
 * This is the local only version of hostname lookups, for use
 * by STANDALONE utilities.  It only looks at /etc/hosts.
 *
 * WARNING:
 * This code is cloned from gethostent.c, any changes made here
 * should made over there too.
 *
 *	gethostbyaddr_local()
 * 	gethostbyname_local()
 *	gethostent_local()
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#define MAXADDRS	35
#define	MAXALIASES	35

static char *getcommon_any();
static char HOSTDB[] = "/etc/hosts";
static struct hostent host;
static FILE *hostf = NULL;
static int stayopen;
static struct hostent *interpret_local();
static char hostaddr[MAXADDRS];
static char *host_aliases[MAXALIASES];
static char *host_addrs[2];
static char line[BUFSIZ+1];

sethostent_local(f)
	int f;
{
	if (hostf == NULL)
		hostf = fopen(HOSTDB, "r");
	else
		rewind(hostf);
	stayopen |= f;
}

endhostent_local()
{
	if (hostf && !stayopen) {
		fclose(hostf);
		hostf = NULL;
	}
}

struct hostent *
gethostent_local()
{
	static char line1[BUFSIZ+1];

	if (hostf == NULL && (hostf = fopen(HOSTDB, "r")) == NULL)
		return (NULL);
        if (fgets(line1, BUFSIZ, hostf) == NULL)
		return (NULL);
	return interpret_local(line1, strlen(line1));
}

struct hostent *
gethostbyaddr_local(addr, len, type)	/* needs to be global */
	char *addr;
	register int len, type;
{
	register struct hostent *p=NULL;

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
gethostbyname_local(name)	/* needs to be global */
	register char *name;
{
	register struct hostent *p=NULL;
	register char **cp;

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

/* this is cloned from interpret(), gethostent.c */
static struct hostent *
interpret_local(val, len)
	char *val;
	int len;
{
	char *p;
	register char *cp, **q;

	strncpy(line, val, len);
	p = line;
	line[len] = '\n';
	if (*p == '#')
		return (gethostent_local());
	cp = getcommon_any(p, "#\n");
	if (cp == NULL)
		return (gethostent_local());
	*cp = '\0';
	cp = getcommon_any(p, " \t");
	if (cp == NULL)
		return (gethostent_local());
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
