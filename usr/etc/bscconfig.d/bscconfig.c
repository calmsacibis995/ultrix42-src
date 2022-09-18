#ifndef lint
static	char	sccsid[] = "@(#)bscconfig.c	4.1 (ULTRIX)		7/2/90";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1985 by				*
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

/************************************************************************
 *			Modification History				*
 *									*
 *	U. Sinkewicz - Based on ifconfig.c.  Written 3/10/85		*
 *									*
 ************************************************************************/

/*
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifndef lint
char copyright[] =
"@(#) Copyright (c) 1983 Regents of the University of California.\n\
 All rights reserved.\n";
#endif not lint


#include <sys/types.h>
#include <sys/socket.h>
#include <net/if.h>

#include <sys/ioctl.h>
#include <netbsc/bsc.h>
#include <netinet/in.h>

#include <stdio.h>
#include <errno.h>
#include <ctype.h>
#include <netdb.h>

extern int errno;
struct	ifreq ifr;

struct sockaddr_bsc sin = {AF_BSC};
struct sockaddr_bsc netmask = {AF_BSC};
char	name[30];
char	addr[14];
int	flags;
int	setaddr;
int	setmask;
int	s;
extern	int errno;
int	setifaddr();

#define	NEXTARG		0xffffff
int	bsc_getaddr();

/* Known address families */
struct afswtch {
	char *af_name;
	short af_af;
	int (*af_status)();
	int (*af_getaddr)();
} afs[] = {
	{ "bsc",	AF_BSC,		0,		bsc_getaddr }
};

struct afswtch *afp;	/*the address family being set or asked about*/

main(argc, argv)
	int argc;
	char *argv[];
{
	int af = AF_BSC;
	if (argc < 2) {
	fprintf(stderr, "usage: bscconfig interface protocol address\n");
		exit(1);
	}
	argc--, argv++;   /* device */
	strncpy(name, *argv, sizeof(name - 1));
	name[sizeof name - 1] = 0;
	strncpy(ifr.ifr_name, name, sizeof(ifr.ifr_name));
	argc--, argv++;  /* af */
	if (argc > 0) {
		struct afswtch *myafp;
		
		for (myafp = afp = afs; myafp->af_name; myafp++)
			if (strcmp(myafp->af_name, *argv) == 0) {
				afp = myafp; argc--; argv++;
				break;
			}
		/* argv now is address # */
		af = ifr.ifr_addr.sa_family = afp->af_af;
	}

	s = socket(af, SOCK_DGRAM, 0);
	if (s < 0) {
		perror("bscconfig: socket");
		exit(1);
	}

	strncpy(ifr.ifr_name, name, sizeof ifr.ifr_name);
	setifaddr(*argv); 

	ifr.ifr_addr = *(struct sockaddr *)&sin;

	if (ioctl(s, SIOCSIFNETMASK, (caddr_t)&ifr) < 0)
		Perror("ioctl (SIOCSIFNETMASK)");

	strncpy(ifr.ifr_name, name, sizeof ifr.ifr_name);
	ifr.ifr_addr = *(struct sockaddr *)&sin;

	if (ioctl(s, SIOCSIFADDR, (caddr_t)&ifr) < 0)
		Perror("ioctl (SIOCSIFADDR)");

	exit(0);
}

/*ARGSUSED*/
setifaddr(addr, param)
	char *addr;
	short param;
{

	/*
	 * Delay the ioctl to set the interface addr until flags are all set.
	 * The address interpretation may depend on the flags,
	 * and the flags may change when the address is set.
	 */

	bsc_getaddr(addr, &sin);
 }

bsc_getaddr(s, saddr)
	char *s;
	struct sockaddr *saddr;
{
	register struct sockaddr_bsc *sin = (struct sockaddr_bsc *)saddr;

	sin->sin_family = AF_BSC;
/*	sin->sin_addr = *(struct sockaddr_bsc *)s; */
	return;
}


/*
 * Print the status of the interface.  If an address family was
 * specified, show it and it only; otherwise, show them all.
 */
status()
{
	register struct afswtch *p = afp;
	short af = ifr.ifr_addr.sa_family;

	if ((p = afp) != NULL) {
		(*p->af_status)();
		return;
	}
	for (p = afs; p->af_name; p++) {
		ifr.ifr_addr.sa_family = p->af_af;
		(*p->af_status)();
	}
}

#define	IFFBITS \
"\020\1UP\2BROADCAST\3DEBUG\4ROUTE\5POINTOPOINT\6NOTRAILERS\7RUNNING\10NOARP\
\11LOCAL"

bsc_status()
{
	struct sockaddr_bsc *sin;

	if (ioctl(s, SIOCGIFADDR, (caddr_t)&ifr) < 0) {
		if (errno == EADDRNOTAVAIL)
			bzero((char *)&ifr.ifr_addr, sizeof(ifr.ifr_addr));
		else
			Perror("ioctl (SIOCGIFADDR)");
	}
	strncpy(ifr.ifr_name, name, sizeof (ifr.ifr_name));
	sin = (struct sockaddr_bsc *)&ifr.ifr_addr;
	if (flags & IFF_POINTOPOINT) {
		if (ioctl(s, SIOCGIFDSTADDR, (caddr_t)&ifr) < 0) {
			if (errno == EADDRNOTAVAIL)
			    bzero((char *)&ifr.ifr_addr, sizeof(ifr.ifr_addr));
			else
			    Perror("ioctl (SIOCGIFDSTADDR)");
		}
		strncpy(ifr.ifr_name, name, sizeof (ifr.ifr_name));
		sin = (struct sockaddr_bsc *)&ifr.ifr_dstaddr;
	}
	if (flags & IFF_BROADCAST) {
		if (ioctl(s, SIOCGIFBRDADDR, (caddr_t)&ifr) < 0) {
			if (errno == EADDRNOTAVAIL)
				return;
			Perror("ioctl (SIOCGIFADDR)");
		}
		strncpy(ifr.ifr_name, name, sizeof (ifr.ifr_name));
		sin = (struct sockaddr_bsc *)&ifr.ifr_addr;
	}
}

Perror(cmd)
	char *cmd;
{
	extern int errno;

	fprintf(stderr, "bscconfig: ");
	switch (errno) {

	case ENXIO:
		fprintf(stderr, "%s: no such interface\n", cmd);
		break;

	case EPERM:
		fprintf(stderr, "%s: permission denied\n", cmd);
		break;

	default:
		perror(cmd);
	}
	exit(1);
}


/*
 * Print a value a la the %b format of the kernel's printf
 */
printb(s, v, bits)
	char *s;
	register char *bits;
	register unsigned short v;
{
	register int i, any = 0;
	register char c;

	if (bits && *bits == 8)
		printf("%s=%o", s, v);
	else
		printf("%s=%x", s, v);
	bits++;
	if (bits) {
		putchar('<');
		while (i = *bits++) {
			if (v & (1 << (i-1))) {
				if (any)
					putchar(',');
				any = 1;
				for (; (c = *bits) > 32; bits++)
					putchar(c);
			} else
				for (; *bits > 32; bits++)
					;
		}
		putchar('>');
	}
}

