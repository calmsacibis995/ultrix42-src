#ifndef lint
static	char	*sccsid = "@(#)rcmd.c	4.5	(ULTRIX)	4/25/91";
#endif lint

/****************************************************************
 *								*
 *  Licensed to Digital Equipment Corporation, Maynard, MA	*
 *		Copyright 1985 Sun Microsystems, Inc.		*
 *			All rights reserved.			*
 *								*
 ****************************************************************/

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/param.h>
#include <sys/stat.h>

#include <netinet/in.h>

#include <stdio.h>
#include <netdb.h>
#include <errno.h>
#include <pwd.h>
#include <ctype.h>

extern	errno;
char	*index();

rcmd(ahost, rport, locuser, remuser, cmd, fd2p)
	char **ahost;
	int rport;
	char *locuser, *remuser, *cmd;
	int *fd2p;
{
	int s, timo = 1;
	struct sockaddr_in sin, from;
	char c;
	int lport = IPPORT_RESERVED - 1;
	struct hostent *hp;

	hp = gethostbyname(*ahost);
	if (hp == 0) {
		fprintf(stderr, "%s: unknown host\n", *ahost);
		return (-1);
	}
	*ahost = hp->h_name;
retry:
	s = rresvport(&lport);
	if (s < 0)
		return (-1);
	sin.sin_family = hp->h_addrtype;
	bcopy(hp->h_addr, (caddr_t)&sin.sin_addr, hp->h_length);
	sin.sin_port = rport;
	if (connect(s, (caddr_t)&sin, sizeof (sin), 0) < 0) {
		if (errno == EADDRINUSE) {
			close(s);
			lport--;
			goto retry;
		}
		if (errno == ECONNREFUSED && timo <= 16) {
			(void) close(s);
			sleep(timo);
			timo *= 2;
			goto retry;
		}
		perror(hp->h_name);
		(void) close(s);
		return (-1);
	}
	lport--;
	if (fd2p == 0) {
		write(s, "", 1);
		lport = 0;
	} else {
		char num[8];
		int s2 = rresvport(&lport), s3;

		if (s2 < 0) {
			(void) close(s);
			return (-1);
		}
		listen(s2, 1);
		(void) sprintf(num, "%d", lport);
		if (write(s, num, strlen(num)+1) != strlen(num)+1) {
			perror("write: setting up stderr");
			(void) close(s2);
			goto bad;
		}
		{ int len = sizeof (from);
		  s3 = accept(s2, &from, &len, 0);
		  close(s2);
		  if (s3 < 0) {
			perror("accept");
			lport = 0;
			goto bad;
		  }
		}
		*fd2p = s3;
		from.sin_port = ntohs((u_short)from.sin_port);
		if (from.sin_family != AF_INET ||
		    from.sin_port >= IPPORT_RESERVED) {
			fprintf(stderr,
			    "socket: protocol failure in circuit setup.\n");
			goto bad2;
		}
	}
	(void) write(s, locuser, strlen(locuser)+1);
	(void) write(s, remuser, strlen(remuser)+1);
	(void) write(s, cmd, strlen(cmd)+1);
	if (read(s, &c, 1) != 1) {
		perror(*ahost);
		goto bad2;
	}
	if (c != 0) {
		while (read(s, &c, 1) == 1) {
			(void) write(2, &c, 1);
			if (c == '\n')
				break;
		}
		goto bad2;
	}
	return (s);
bad2:
	if (lport)
		(void) close(*fd2p);
bad:
	(void) close(s);
	return (-1);
}

rresvport(alport)
	int *alport;
{
	struct sockaddr_in sin;
	int s;

	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = 0;
	s = socket(AF_INET, SOCK_STREAM, 0, 0);
	if (s < 0)
		return (-1);
	for (;;) {
		sin.sin_port = htons((u_short)*alport);
		if (bind(s, (caddr_t)&sin, sizeof (sin), 0) >= 0)
			return (s);
		if (errno != EADDRINUSE && errno != EADDRNOTAVAIL) {
			perror("socket");
			return (-1);
		}
		(*alport)--;
		if (*alport == IPPORT_RESERVED/2) {
			fprintf(stderr, "socket: All ports in use\n");
			return (-1);
		}
	}
}

/*
 * ruserok - returns 0 upon success
 *	     	    -1 upon failure
 */
ruserok(rhost, superuser, ruser, luser)
	char *rhost;
	int superuser;
	char *ruser, *luser;
{
	FILE *hostf;
	char domain[256];
	int suid;
	int sgid;
	int first = 1;

	if (getdomainname(domain, sizeof(domain)) < 0) {
		fprintf(stderr, "rcmd: getdomainname system call missing\n");
		return(-1);
	}
	hostf = superuser ? (FILE *)0 : fopen("/etc/hosts.equiv", "r");
again:
	if (hostf) {
		if ((parser(hostf, rhost, luser, ruser, domain)) == 0) {
			(void) fclose(hostf);
			if (first == 0) {
				(void) seteuid(suid);
				(void) setegid(sgid);
				(void) setgroups(1, &sgid);
			}
			return(0);
		}
		(void) fclose(hostf);
	}
	if (first == 1) {
		struct stat sbuf;
		struct passwd *pwd;
		char pbuf[MAXPATHLEN];

		first = 0;
		suid = geteuid();
		sgid = getegid();
		if ((pwd = getpwnam(luser)) == NULL)
			return(-1);
		(void) setegid(pwd->pw_gid);
		(void) initgroups(luser, pwd->pw_gid);
		(void) seteuid(pwd->pw_uid);
		(void)strcpy(pbuf, pwd->pw_dir);
		(void)strcat(pbuf, "/.rhosts");
		if ((hostf = fopen(pbuf, "r")) == NULL)
			goto bad;
		(void)fstat(fileno(hostf), &sbuf);
 		if (sbuf.st_uid && sbuf.st_uid != pwd->pw_uid) {
			fclose(hostf);
			goto bad;
		}
		goto again;
	}
bad:
	if (first == 0) {
		(void) seteuid(suid);
	        (void) setegid(sgid);
		(void) setgroups(1, &sgid);
	}
	return (-1);
}

/*
 * parser - reads in all entries from file descriptor, hostf
 *	and compares entries with the one to match
 */
parser(hostf, rhost, luser, ruser, domain)
	FILE *hostf;
	char *rhost, *luser, *ruser, *domain;
{
	char *user;
	char ahost[128];
	register char *p, *ahostp;

	for (p = rhost; *p; p++)
		*p = isupper(*p) ? tolower(*p) : *p;
	while (fgets(ahost, sizeof (ahost), hostf)) {
		int hostmatch, usermatch;

		/*
		 * Skip beginning white space
		 */
		p = ahost;
		while (*p == ' ' || *p == '\t')
			p++;
		ahostp = p;
		while (*p != '\n' && *p != ' ' && *p != '\t' && *p != '\0') {
			*p = isupper(*p) ? tolower(*p) : *p;
			p++;
		}
		if (*p == ' ' || *p == '\t') {
			*p++ = '\0';
			/*
		 	 * Skip middle white space
		 	 */
			while (*p == ' ' || *p == '\t')
				p++;
			user = p;
			while (*p != '\n' && *p != ' ' && *p != '\t' && *p != '\0')
				p++;
			/*
		 	 * Stop before trailing white space
		 	 */
		} else
			user = p;
		*p = '\0';
		if ((hostmatch = special_case(rhost, ahostp, NULL, domain)) < 0)
			hostmatch = !strcmp(rhost, ahostp);
		if (*user) {
			if ((usermatch = special_case(NULL, user, ruser, domain)) < 0)
				usermatch = !strcmp(ruser, user);
		}
		else
			usermatch = !strcmp(ruser, luser);

		if (hostmatch && usermatch)
			return (0);
	}
	return (-1);
}

/*
 * special_case - parses special cases
 *		  returns 1 if connection allowed
 *			  0 if connection not allowed
 *			 -1 if no match
 */

special_case(rhost, name, ruser, domain)
	char *rhost, *name, *ruser, *domain;
{
	int match = -1;
	int length;

	/*
	 * Check if host in local domain
	 */
	if (((length = local_hostname_length(rhost)) != NULL) &&
		    strlen(name) == length && (!strncmp(rhost, name, length)))
		match = 1;
	/*
	 * Check to allow all hosts
	 */
	else if (name[0] == '+' && name[1] == 0)
		match = 1;
	/*
	 * Check for two yp group cases
	 */
	else if (name[0] == '+' && name[1] == '@' && domain != NULL)
		match = innetgr(name + 2, rhost, ruser, domain);
	else if (name[0] == '-' && name[1] == '@' && domain != NULL) {
		if (innetgr(name + 2, rhost, ruser, domain))
			match = 0;
	}
	/*
	 * Check to disallow this host
	 */
 	else if (name[0] == '-') {
 		if (!strcmp(rhost, name+1))
 			match = 0;
 	}
	return(match);
}
