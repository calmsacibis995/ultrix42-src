#ifndef lint
static	char	*sccsid = "@(#)cmds.c	4.1	(ULTRIX)	7/2/90";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1988 by				*
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
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *static char sccsid[] = "@(#)cmds.c	2.2 (Berkeley) 4/21/86";
 */


#include "timedc.h"
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#define TSPTYPES
#include <protocols/timed.h>
#include <sys/file.h>

int id;
int sock;
int sock_raw;
char hostname[MAXHOSTNAMELEN];
struct hostent *hp, *gethostbyname();
struct sockaddr_in server;
extern int measure_delta;
int bytenetorder(), bytehostorder();
char *strcpy();

/*
 * Clockdiff computes the difference between the time of the machine on 
 * which it is called and the time of the machines given as argument.
 * The time differences measured by clockdiff are obtained using a sequence
 * of ICMP TSTAMP messages which are returned to the sender by the IP module
 * in the remote machine.
 * In order to compare clocks of machines in different time zones, the time 
 * is transmitted (as a 32-bit value) in milliseconds since midnight UT. 
 * If a hosts uses a different time format, it should set the high order
 * bit of the 32-bit quantity it transmits.
 * However, VMS apparently transmits the time in milliseconds since midnight
 * local time (rather than GMT) without setting the high order bit. 
 * Furthermore, it does not understand daylight-saving time.  This makes 
 * clockdiff behaving inconsistently with hosts running VMS.
 *
 * In order to reduce the sensitivity to the variance of message transmission 
 * time, clockdiff sends a sequence of messages.  Yet, measures between 
 * two `distant' hosts can be affected by a small error. The error can, however,
 * be reduced by increasing the number of messages sent in each measurement.
 */

clockdiff(argc, argv)
int argc;
char *argv[];
{
	int measure_status;
	struct timeval ack;
	int measure();

	if(argc < 2)  {
		printf("Usage: clockdiff host ... \n");
		return;
	}

	id = getpid();
	(void)gethostname(hostname,sizeof(hostname));

	while (argc > 1) {
		argc--; argv++;
		hp = gethostbyname(*argv);
		if (hp == NULL) {
			printf("%s: unknown host\n", *argv);
			continue;
		}
		server.sin_family = hp->h_addrtype;
		bcopy(hp->h_addr, &(server.sin_addr.s_addr), hp->h_length); 
		ack.tv_sec = 10;
		ack.tv_usec = 0;
		if ((measure_status = measure(&ack, &server)) < 0) {
			perror("measure");
			return;
		}
		switch (measure_status) {

		case HOSTDOWN:
			printf("%s is down\n", hp->h_name);
			continue;
			break;
		case NONSTDTIME:
			printf("%s time transmitted in a non-standard format\n",						 hp->h_name);
			continue;
			break;
		case UNREACHABLE:
			printf("%s is unreachable\n", hp->h_name);
			continue;
			break;
		default:
			break;
		}

		if (measure_delta > 0)
			printf("time on %s is %d ms. ahead of time on %s\n", 
						hp->h_name, measure_delta,
						hostname);
		else
			if (measure_delta == 0)
		      		printf("%s and %s have the same time\n", 
						hp->h_name, hostname);
			else
		      	     printf("time on %s is %d ms. behind time on %s\n",
					hp->h_name, -measure_delta, hostname);
	}
	return;
}
/*
 * finds location of master timedaemon
 */

msite(argc)
int argc;
{
	int length;
	int cc;
	fd_set ready;
	struct sockaddr_in dest;
	struct timeval tout;
	struct sockaddr_in from;
	struct tsp msg;
	struct servent *srvp;

	if (argc != 1) {
		printf("Usage: msite\n");
		return;
	}

	srvp = getservbyname("timed", "udp");
	if (srvp == 0) {
		fprintf(stderr, "udp/timed: unknown service\n");
		return;
	}	
	dest.sin_port = srvp->s_port;
	dest.sin_family = AF_INET;

	(void)gethostname(hostname,sizeof(hostname));
	hp = gethostbyname(hostname);
	if (hp == NULL) {
		perror("gethostbyname");
		return;
	}
	bcopy(hp->h_addr, &dest.sin_addr.s_addr, hp->h_length);

	(void)strcpy(msg.tsp_name, hostname);
	msg.tsp_type = TSP_MSITE;
	msg.tsp_vers = TSPVERSION;
	bytenetorder(&msg);
	length = sizeof(struct sockaddr_in);
	if (sendto(sock, (char *)&msg, sizeof(struct tsp), 0, 
						&dest, length) < 0) {
		perror("sendto");
		return;
	}

	tout.tv_sec = 15;
	tout.tv_usec = 0;
	FD_ZERO(&ready);
	FD_SET(sock, &ready);
	if (select(FD_SETSIZE, &ready, (fd_set *)0, (fd_set *)0, &tout)) {
		length = sizeof(struct sockaddr_in);
		cc = recvfrom(sock, (char *)&msg, sizeof(struct tsp), 0, 
							&from, &length);
		if (cc < 0) {
			perror("recvfrom");
			return;
		}
		bytehostorder(&msg);
		if (msg.tsp_type == TSP_ACK)
			printf("master timedaemon runs on %s\n", msg.tsp_name);
		else
			printf("received wrong ack: %s\n", 
						tsptype[msg.tsp_type]);
	} else
		printf("communication error\n");
}

/*
 * quits timedc
 */

quit()
{
	exit(0);
}

#define MAXH	4	/* max no. of hosts where election can occur */

/*
 * Causes the election timer to expire on the selected hosts
 * It sends just one udp message per machine, relying on
 * reliability of communication channel.
 */

testing(argc, argv)
int argc;
char *argv[];
{
	int length;
	int nhosts;
	struct servent *srvp;
	struct sockaddr_in sin[MAXH];
	struct tsp msg;

	if(argc < 2)  {
		printf("Usage: testing host ...\n");
		return;
	}

	srvp = getservbyname("timed", "udp");
	if (srvp == 0) {
		fprintf(stderr, "udp/timed: unknown service\n");
		return;
	}	

	nhosts = 0;
	while (argc > 1) {
		argc--; argv++;
		hp = gethostbyname(*argv);
		if (hp == NULL) {
			printf("%s: unknown host %s\n", *argv);
			argc--; argv++;
			continue;
		}
		sin[nhosts].sin_port = srvp->s_port;
		sin[nhosts].sin_family = hp->h_addrtype;
		bcopy(hp->h_addr, &(sin[nhosts].sin_addr.s_addr), hp->h_length);
		if (++nhosts == MAXH)
			break;
	}

	msg.tsp_type = TSP_TEST;
	msg.tsp_vers = TSPVERSION;
	(void)gethostname(hostname, sizeof(hostname));
	(void)strcpy(msg.tsp_name, hostname);
	bytenetorder(&msg);	/* it is not really necessary here */
	while (nhosts-- > 0) {
		length = sizeof(struct sockaddr_in);
		if (sendto(sock, (char *)&msg, sizeof(struct tsp), 0, 
						&sin[nhosts], length) < 0) {
			perror("sendto");
			return;
		}
	}
}

/*
 * Enables or disables tracing on local timedaemon
 */

tracing(argc, argv)
int argc;
char *argv[];
{
	int onflag;
	int length;
	int cc;
	fd_set ready;
	struct sockaddr_in dest;
	struct timeval tout;
	struct sockaddr_in from;
	struct tsp msg;
	struct servent *srvp;

	if (argc != 2) {
		printf("Usage: tracing { on | off }\n");
		return;
	}

	srvp = getservbyname("timed", "udp");
	if (srvp == 0) {
		fprintf(stderr, "udp/timed: unknown service\n");
		return;
	}	
	dest.sin_port = srvp->s_port;
	dest.sin_family = AF_INET;

	(void)gethostname(hostname,sizeof(hostname));
	hp = gethostbyname(hostname);
	bcopy(hp->h_addr, &dest.sin_addr.s_addr, hp->h_length);

	if (strcmp(argv[1], "on") == 0) {
		msg.tsp_type = TSP_TRACEON;
		onflag = ON;
	} else {
		msg.tsp_type = TSP_TRACEOFF;
		onflag = OFF;
	}

	(void)strcpy(msg.tsp_name, hostname);
	msg.tsp_vers = TSPVERSION;
	bytenetorder(&msg);
	length = sizeof(struct sockaddr_in);
	if (sendto(sock, (char *)&msg, sizeof(struct tsp), 0, 
						&dest, length) < 0) {
		perror("sendto");
		return;
	}

	tout.tv_sec = 5;
	tout.tv_usec = 0;
	FD_ZERO(&ready);
	FD_SET(sock, &ready);
	if (select(FD_SETSIZE, &ready, (fd_set *)0, (fd_set *)0, &tout)) {
		length = sizeof(struct sockaddr_in);
		cc = recvfrom(sock, (char *)&msg, sizeof(struct tsp), 0, 
							&from, &length);
		if (cc < 0) {
			perror("recvfrom");
			return;
		}
		bytehostorder(&msg);
		if (msg.tsp_type == TSP_ACK)
			if (onflag)
				printf("timed tracing enabled\n");
			else
				printf("timed tracing disabled\n");
		else
			printf("wrong ack received: %s\n", 
						tsptype[msg.tsp_type]);
	} else
		printf("communication error\n");
}

priv_resources()
{
	int port;
	struct sockaddr_in sin;

	sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock < 0) {
		perror("opening socket");
		return(-1);
	}

	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = 0;
	for (port = IPPORT_RESERVED - 1; port > IPPORT_RESERVED / 2; port--) {
		sin.sin_port = htons((u_short)port);
		if (bind(sock, (struct sockaddr *)&sin, sizeof (sin)) >= 0)
			break;
		if (errno != EADDRINUSE && errno != EADDRNOTAVAIL) {
			perror("bind");
			(void) close(sock);
			return(-1);
		}
	}
	if (port == IPPORT_RESERVED / 2) {
		fprintf(stderr, "all reserved ports in use\n");
		(void) close(sock);
		return(-1);
	}

	sock_raw = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP); 
	if (sock_raw < 0)  {
		perror("opening raw socket");
		(void) close(sock_raw);
		return(-1);
	}
	return(1);
}

#define	INCR	1
#define	DECR	0

incr(argc, argv)
int argc;
char *argv[];
{
	adjusttime(argc, argv, INCR);
}

decr(argc, argv)
int argc;
char *argv[];
{
	adjusttime(argc, argv, DECR);
}

adjusttime(argc, argv, inc)
int argc;
char *argv[];
int	inc;
{
	int dflag = 1, cflag = 0, gflag = 0;
	struct timeval	tv;
	struct timezone	tz;
	struct timeval	timeskew;

	argv++; argc--;

	if(argc > 0 && argv[0][0] == '-') {
		switch ((int)argv[0][1]) {

		case 'd':
			break;

		case 'c':
			cflag++;
			dflag--;
			break;

		case 'g':
			gflag++;
			dflag--;
			break;

		 default:
			printf("Usage: incr/decr [-dc] [minutes:][seconds][.micro_seconds]\n");
			return;
		}
		argc--;
		argv++;
	}


	if(!parse_adj(argv[0], &timeskew)) {
		printf("Usage: incr/decr [-dc] [minutes:][seconds][.micro_seconds]\n");
		return;
	}
	else {
		argc--; argv++;
	}

	if(argc > 0) {	
		printf("Usage: incr/decr [-dc] [minutes:][seconds][.micro_seconds]\n");
		return;
	}

	if(!inc) {
		timeskew.tv_sec = -timeskew.tv_sec;
		timeskew.tv_usec = -timeskew.tv_usec;	
	}

	if (dflag) {
		(void) gettimeofday(&tv, (struct timezone *)0);

		tv.tv_sec += timeskew.tv_sec;
		tv.tv_usec += timeskew.tv_usec;
	
		if (settimeofday(&tv, (struct timezone *)0) < 0) {
			perror("settimeofday");
		}
	}
	else if(gflag) {
		settime(timeskew);
	}
	else {
		if(adjtime(&timeskew, (struct timeval *)0) < 0) {
			perror("adjtime:");
		}
	}
}


parse_adj(adj_str, time_skew)
	char	*adj_str;
	struct timeval	*time_skew;
{
	char	minstr[20];
	char	secstr[20];
	char	usecstr[20];
	int	min_sec = 0;
	int	num_args = 0;

	if(adj_str == NULL)
		return(0);

	usecstr[0] = '\0';

	if((num_args = sscanf(adj_str, "%[0-9]:%[0-9].%[0-9]",
			minstr, secstr, usecstr)) < 3) {

		minstr[0] = '\0';
		secstr[0] = '\0';

		if((num_args = sscanf(adj_str, "%[0-9]:%[0-9]",
				minstr, secstr)) < 2) {

			minstr[0] = '\0';

			if((num_args = sscanf(adj_str, "%[0-9].%[0-9]",
					secstr, usecstr)) < 2) {

				secstr[0] = '\0';

				if(sscanf(adj_str, "%[0-9]", usecstr) == 0) {
					return(0);
				}
			}
		} else
			min_sec = 1;
	}
		
	if(num_args == 1)
		if(strlen(usecstr) != strlen(adj_str))
			return(0);
	else if(num_args == 2)
		if(min_sec == 1 &&
			(strlen(minstr) + strlen(secstr)) != strlen(adj_str))
				return(0);
		else if(min_sec == 0 &&
			(strlen(secstr) + strlen(usecstr)) != strlen(adj_str))
				return(0);
	else if(num_args == 3 &&
			(strlen(minstr) + strlen(secstr) + strlen(usecstr))
				!= strlen(adj_str))
		return(0);

/*	if( (strlen(minstr) + strlen(secstr) + strlen(usecstr))
	   		!= strlen(adj_str)) {
		return(0);
	}*/

	time_skew->tv_usec = atol(usecstr);
	time_skew->tv_sec = (60 * atol(minstr)) + atol(secstr);
	return(1);
}


#define WAITACK		2	/* seconds */
#define WAITDATEACK	5	/* seconds */

/*
 * Set the date in the machines controlled by timedaemons
 * by communicating the new date to the local timedaemon. 
 * If the timedaemon is in the master state, it performs the
 * correction on all slaves.  If it is in the slave state, it
 * notifies the master that a correction is needed.
 * Returns 1 on success, 0 on failure.
 */
settime(timeskew)
	struct timeval timeskew;
{
	struct timeval tv;
	int s, length, port, timed_ack, found, err;
	long waittime;
	fd_set ready;
	char hostname[MAXHOSTNAMELEN];
	struct timeval tout;
	struct servent *sp;
	struct tsp msg;
	struct sockaddr_in sin, dest, from;

	sp = getservbyname("timed", "udp");
	if (sp == 0) {
		fprintf(stderr, "udp/timed: unknown service\n");
		return (0);
	}	
	dest.sin_port = sp->s_port;
	dest.sin_family = AF_INET;
	dest.sin_addr.s_addr = htonl((u_long)INADDR_ANY);
	s = socket(AF_INET, SOCK_DGRAM, 0);
	if (s < 0) {
		if (errno != EPROTONOSUPPORT)
			perror("date: socket");
		goto bad;
	}
	bzero((char *)&sin, sizeof (sin));
	sin.sin_family = AF_INET;
	for (port = IPPORT_RESERVED - 1; port > IPPORT_RESERVED / 2; port--) {
		sin.sin_port = htons((u_short)port);
		if (bind(s, (struct sockaddr *)&sin, sizeof (sin)) >= 0)
			break;
		if (errno != EADDRINUSE) {
			if (errno != EADDRNOTAVAIL)
				perror("date: bind");
			goto bad;
		}
	}
	if (port == IPPORT_RESERVED / 2) {
		fprintf(stderr, "date: All ports in use\n");
		goto bad;
	}
	msg.tsp_type = TSP_SETDATE;
	msg.tsp_vers = TSPVERSION;
	(void) gethostname(hostname, sizeof (hostname));
	(void) strncpy(msg.tsp_name, hostname, sizeof (hostname));
	msg.tsp_seq = htons((u_short)0);

/*	msg.tsp_time.tv_sec = htonl((u_long)tv.tv_sec);
	msg.tsp_time.tv_usec = htonl((u_long)tv.tv_usec);*/

	length = sizeof (struct sockaddr_in);
	if (connect(s, &dest, length) < 0) {
		perror("date: connect");
		goto bad;
	}

	(void) gettimeofday(&tv, (struct timezone *)0);

	tv.tv_sec += timeskew.tv_sec;
	tv.tv_usec += timeskew.tv_usec;

	msg.tsp_time.tv_sec = htonl((u_long)tv.tv_sec);
	msg.tsp_time.tv_usec = htonl((u_long)tv.tv_usec);

	if (send(s, (char *)&msg, sizeof (struct tsp), 0) < 0) {
		if (errno != ECONNREFUSED)
			perror("date: send");
		goto bad;
	}
	timed_ack = -1;
	waittime = WAITACK;
loop:
	tout.tv_sec = waittime;
	tout.tv_usec = 0;
	FD_ZERO(&ready);
	FD_SET(s, &ready);
	found = select(FD_SETSIZE, &ready, (fd_set *)0, (fd_set *)0, &tout);
	length = sizeof(err);
	if (getsockopt(s, SOL_SOCKET, SO_DEBUG, (char *)&err, &length) == 0
	    && err) {
		errno = err;
		if (errno != ECONNREFUSED)
			perror("date: send (delayed error)");
		goto bad;
	}
	if (found > 0 && FD_ISSET(s, &ready)) {
		length = sizeof (struct sockaddr_in);
		if (recvfrom(s, (char *)&msg, sizeof (struct tsp), 0, &from,
		    &length) < 0) {
			if (errno != ECONNREFUSED)
				perror("date: recvfrom");
			goto bad;
		}
		msg.tsp_seq = ntohs(msg.tsp_seq);
		msg.tsp_time.tv_sec = ntohl(msg.tsp_time.tv_sec);
		msg.tsp_time.tv_usec = ntohl(msg.tsp_time.tv_usec);
		switch (msg.tsp_type) {

		case TSP_ACK:
			timed_ack = TSP_ACK;
			waittime = WAITDATEACK;
			goto loop;

		case TSP_DATEACK:
			(void)close(s);
			return (1);

		default:
			fprintf(stderr,
			    "date: Wrong ack received from timed: %s\n", 
			    tsptype[msg.tsp_type]);
			timed_ack = -1;
			break;
		}
	}
	if (timed_ack == -1)
		fprintf(stderr,
		    "date: Can't reach time daemon, time set locally.\n");
bad:
	(void)close(s);
	return (0);
}
