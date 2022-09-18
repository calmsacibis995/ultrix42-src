#ifndef lint
static  char    *sccsid = "@(#)main.c	4.1  (ULTRIX)        7/2/90";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1984,1988 by			*
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
/*
 *	01-Jun-89       R. Bhanukitsiri
 *	Don't exit if we can't find SNMP services.
 *
 *	09-Mar-89	R. Bhanukitsiri
 *	Add SNMP support.
 *	Modify process() routine to include the protocol of the received
 *		packet.
 *	Make a copy of sp servent structure so that getservbyname() can
 *		called by someone else.  (We actually prefer to make a
 *		copy of sp->s_port, but we try to make minimum changes.)
 *
 *	15-Jan-88	lp
 *	Final 43BSD version.
 *
 */
/*
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */
/*
#ifndef lint
char copyright[] =
" Copyright (c) 1983 Regents of the University of California.\n\
 All rights reserved.\n";
#endif not lint

#ifndef lint
static char sccsid[] = "main.c	5.11 (Berkeley) 5/28/87";
#endif not lint
*/
/*
 * Routing Table Management Daemon
 */
#include "defs.h"
#include <sys/ioctl.h>
#include <sys/time.h>

#include <net/if.h>

#include <errno.h>
#include <signal.h>
#include <syslog.h>

#ifdef	SNMP
#include "snmp.h"
#endif	SNMP

#ifdef	SNMP
int	snmpdebug = 0;
extern	int snmp_socket;
#endif	SNMP
int	supplier = -1;		/* process should supply updates */
int	gateway = 0;		/* 1 if we are a gateway to parts beyond */
int	debug = 0;

struct	rip *msg = (struct rip *)packet;
int	hup(), rtdeleteall();

main(argc, argv)
	int argc;
	char *argv[];
{
	int cc;
	int selectbits = 0;
	struct sockaddr from;
	struct servent  *srvp;
	u_char retry;
	
	argv0 = argv;
#ifdef 43BSD
	openlog("routed", LOG_PID | LOG_ODELAY, LOG_DAEMON);
	setlogmask(LOG_UPTO(LOG_WARNING));
#else 43BSD
	openlog("routed", LOG_PID , 0);
#endif 43BSD
	srvp = getservbyname("router", "udp");
	if (srvp == NULL) {
		fprintf(stderr, "routed: router/udp: unknown service\n");
		exit(1);
	}
	sp = (struct servent *)malloc(sizeof(struct servent));
	bcopy(srvp, sp, sizeof(struct servent));
	addr.sin_family = AF_INET;
	addr.sin_port = sp->s_port;
	s = getsocket(AF_INET, SOCK_DGRAM, &addr);
	if (s < 0)
		exit(1);
#ifdef SNMP
	/*
	 * Get a socket to communicated with SNMPD
	 */
	if ((snmp_socket = snmp_init()) < 0) {
		syslog(LOG_NOTICE, "main: continuing without SNMP\n");
		snmp_socket = -1;
	}
#endif SNMP
	argv++, argc--;
	while (argc > 0 && **argv == '-') {
		if (strcmp(*argv, "-s") == 0) {
			supplier = 1;
			argv++, argc--;
			continue;
		}
		if (strcmp(*argv, "-q") == 0) {
			supplier = 0;
			argv++, argc--;
			continue;
		}
		if (strcmp(*argv, "-t") == 0) {
			tracepackets++;
#ifdef 43BSD
			setlogmask(LOG_UPTO(LOG_DEBUG));
#endif 43BSD
			argv++, argc--;
			continue;
		}
		if (strcmp(*argv, "-d") == 0) {
			debug++;
#ifdef 43BSD
			setlogmask(LOG_UPTO(LOG_DEBUG));
#endif 43BSD
			argv++, argc--;
			continue;
		}
#ifdef SNMP
		if (strcmp(*argv, "-snmpdebug") == 0) {
			snmpdebug++;
			argv++, argc--;
			continue;
		}
#endif SNMP
		if (strcmp(*argv, "-g") == 0) {
			gateway = 1;
			argv++, argc--;
			continue;
		}
		fprintf(stderr,
			"usage: routed [ -s ] [ -q ] [ -t ] [ -g ]\n");
		exit(1);
	}
	if (tracepackets == 0 && debug == 0 && snmpdebug == 0) {
		int t;

		if (fork())
			exit(0);
		for (t = 0; t < 20; t++)
			if (t != s && t != snmp_socket)
				(void) close(t);
		(void) open("/", 0);
		(void) dup2(0, 1);
		(void) dup2(0, 2);
		t = open("/dev/tty", 2);
		if (t >= 0) {
			ioctl(t, TIOCNOTTY, (char *)0);
			(void) close(t);
		}
	}
	/*
	 * Any extra argument is considered
	 * a tracing log file.
	 */
	if (argc > 0)
		traceon(*argv);
	/*
	 * Collect an initial view of the world by
	 * checking the interface configuration and the gateway kludge
	 * file.  Then, send a request packet on all
	 * directly connected networks to find out what
	 * everyone else thinks.
	 */
	rtinit();
	ifinit();
	gwkludge();
	if (gateway > 0)
		rtdefault();
	if (supplier < 0)
		supplier = 0;
	msg->rip_cmd = RIPCMD_REQUEST;
	msg->rip_vers = RIPVERSION;
	msg->rip_nets[0].rip_dst.sa_family = AF_UNSPEC;
	msg->rip_nets[0].rip_metric = HOPCNT_INFINITY;
	msg->rip_nets[0].rip_dst.sa_family = htons(AF_UNSPEC);
	msg->rip_nets[0].rip_metric = htonl(HOPCNT_INFINITY);
	toall(sendmsg);
	signal(SIGALRM, timer);
	signal(SIGHUP, hup);
	signal(SIGTERM, hup);
	signal(SIGINT, rtdeleteall);
	timer();

	/*
	 * Construct the all of the socket descriptor we will be listening on.
	 */
	selectbits |= 1 << s;			/* RIP socket */
#ifdef SNMP
	if (snmp_socket != -1)
		selectbits |= 1 << snmp_socket;		/* SNMPD socket */
#endif SNMP

	for (;;) {
		int ibits;
		register int n;

		ibits = selectbits;
		n = select(20, &ibits, 0, 0, 0);
		if (n < 0)
			continue;
		if (ibits & (1 << s))
			process(s, IPPROTO_RIP);
#ifdef SNMP
		if (snmp_socket != -1)
			if (ibits & (1 << snmp_socket))
				process(snmp_socket, IPPROTO_SNMP);
#endif SNMP
		/* handle ICMP redirects */
	}
}

process(fd, protocol)
	int fd;
	int protocol;
{
	struct sockaddr from;
	int fromlen = sizeof (from), cc, omask;

	cc = recvfrom(fd, packet, sizeof (packet), 0, &from, &fromlen);
	if (cc <= 0) {
		if (cc < 0 && errno != EINTR)
			perror("recvfrom");
		return;
	}
	if (fromlen != sizeof (struct sockaddr_in))
		return;
	omask = sigblock(sigmask(SIGALRM));

	switch (protocol) {
	  case IPPROTO_RIP:
		rip_input(&from, cc);
		break;
#ifdef SNMP
	  case IPPROTO_SNMP:
		snmpin(&from, cc, packet);
		break;
#endif SNMP
	}
	sigsetmask(omask);
}

getsocket(domain, type, sin)
	int domain, type;
	struct sockaddr_in *sin;
{
	int s, on = 1;

	if ((s = socket(domain, type, 0)) < 0) {
		perror("socket");
		syslog(LOG_ERR, "socket: %m");
		return (-1);
	}
	if (setsockopt(s, SOL_SOCKET, SO_BROADCAST, &on, sizeof (on)) < 0) {
		syslog(LOG_ERR, "setsockopt SO_BROADCAST: %m");
		close(s);
		return (-1);
	}
	on = 48*1024;
	if (setsockopt(s, SOL_SOCKET, SO_RCVBUF, &on, sizeof (on)) < 0)
		syslog(LOG_ERR, "setsockopt SO_RCVBUF: %m");
	if (bind(s, sin, sizeof (*sin), 0) < 0) {
		perror("bind");
		syslog(LOG_ERR, "bind: %m");
		close(s);
		return (-1);
	}
	return (s);
}
