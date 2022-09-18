#ifndef lint
static	char	*sccsid = "@(#)rdate.c	4.2	(ULTRIX)	9/7/90";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1985,86 by			*
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

/*-----------------------------------------------------------------------
 *	Modification History
 *
 *      9/06/90 -- terry
 *              Modified code to use correct network address for 
 *              finding the time on a specific network.
 *              Also added exit(1) after "unknown network" message to
 *              exit gracefully.
 *
 *	4/15/85 -- jrs
 *		Use INADDR_BROADCAST as default when none specified
 *		to get around subnet addr problems.
 *
 *	4/5/85 -- jrs
 *		Created to allow machines to set time from network.
 *		Based on a concept by Marshall Rose of UC Irvine
 *		and the internet specifications for time server.
 *
 *-----------------------------------------------------------------------
 */

/*
 *	The syntax for this client is:
 *	rdate [-sv] [network]
 *
 *	where: 
 *		-s	Set time from network median
 *		-v	Print time for each responding network host
 *		network	The network broadcast addr to poll for time
 *
 *	If no switches are set, rdate will just report the network median time
 *	If no network is specified, rdate will use the host's primary network.
 *
 *	It is intended that rdate will normally be used in the /etc/rc file
 *	with the -s switch to set the system date.  This is especially useful
 *	on machines such as MicroVax I's that have no t.o.y. clock.
 */

#include <netdb.h>
#include <stdio.h>
#include <utmp.h>
#include <sys/types.h>
#include <sys/file.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>

#define	WTMP	"/usr/adm/wtmp"
#define	RESMAX	100

struct	utmp wtmp[2] = { { "|", "", "", 0 }, { "{", "", "", 0 } };

struct	servent *getservbyname();
struct	netent *getnetbyname();
struct	hostent *gethostbyname();
struct	in_addr inet_makeaddr();
int	tcomp();

main(argc, argv)
int	argc;
char	**argv;
{
	int	set = 0;
	int	verbose = 0;
	int	on = 1;
	char	*net = NULL;
	struct	servent	*tserv;
	struct	netent	*tnet;
	struct	hostent	*thost;
	struct	sockaddr_in netaddr;
	struct	timeval baset, nowt, timeout;
	struct	timezone basez, nowz;
	char	hostnam[32], resbuf[16], *swtp;
	int	argp, wtmpfd, tsock, readsel, writesel, selmax, selvalue;
	int	rescount, median, addrsiz;
	unsigned long reslist[RESMAX], resvalue;

	for (argp = 1; argp < argc; argp++) {
		if (*argv[argp] == '-') {
			for (swtp = &argv[argp][1]; *swtp != '\0'; swtp++) {
				switch (*swtp) {

				case 's':	/* set time */
					set = 1;
					break;

				case 'v':	/* verbose report */
					verbose = 1;
					break;

				default:
					fprintf(stderr,
						"%s: Unknown switch - %c\n",
						argv[0], *swtp);
					exit(1);
				}
			}
		} else {
			net = argv[argp];
		}
	}

	/* research network and service information */

	if ((tserv = getservbyname("time", "udp")) == NULL) {
		fprintf(stderr, "%s: Time service unknown\n", argv[0]);
		exit(1);
	}
	netaddr.sin_family = AF_INET;
	netaddr.sin_port = tserv->s_port;
	if (net == NULL) {
		netaddr.sin_addr.s_addr = INADDR_BROADCAST;
	} else {
		if ((tnet = getnetbyname(net)) == NULL) {
			fprintf(stderr, "%s: Unknown network - %s\n",
					argv[0], net);
			exit(1);
		}
		netaddr.sin_addr = inet_makeaddr(tnet->n_net, INADDR_ANY);
	}

	/* set up the socket and define the base time */

	if ((tsock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		fprintf(stderr, "%s: socket create failure\n", argv[0]);
		exit(1);
	}
	if (setsockopt(tsock, SOL_SOCKET, SO_BROADCAST, &on, sizeof(on)) < 0) {
		fprintf(stderr, "%s: set broadcast failure\n", argv[0]);
		exit(1);
	}
	(void) gettimeofday(&baset, &basez);

	/* set up for select, then yell for someone to tell us the time */

	timeout.tv_sec = 2;
	timeout.tv_usec = 0;
	readsel = 1 << tsock;
	writesel = 0;
	selmax = tsock + 1;
	rescount = 0;

	if (sendto(tsock, resbuf, sizeof(resbuf), 0, &netaddr,
				sizeof(netaddr)) < 0) {
		fprintf(stderr, "%s: socket send failure\n", argv[0]);
		exit(1);
	}

	/* loop for incoming packets.  We will break out on error
	   or timeout period expiration */

	while ((selvalue = select(selmax, &readsel, &writesel, &writesel,
				&timeout)) > 0) {

		/* reset for next select */

		timeout.tv_sec = 2;
		timeout.tv_usec = 0;
		readsel = 1 << tsock;
		writesel = 0;
		selmax = tsock + 1;
		
		/* try to pick up packet */

		addrsiz = sizeof(netaddr);
		if (recvfrom(tsock, resbuf, sizeof(resbuf), 0, &netaddr,
				&addrsiz) != sizeof(resvalue)) {
			continue;
		}
		
		/* this little piece of code is to insure that all
		   incoming times are stamped from same base time */

		(void) gettimeofday(&nowt, &nowz);
		resvalue = ntohl(*(unsigned long *)resbuf) - 2208988800l;
		reslist[rescount++] = resvalue - (nowt.tv_sec - baset.tv_sec);

		/* if we are verbose, explain what we just got */

		if (verbose != 0) {
			thost = gethostbyaddr(&netaddr.sin_addr,
					sizeof(netaddr.sin_addr), AF_INET);
			printf("%s: %s", (thost == NULL)? "*Unknown*":
					thost->h_name, ctime(&resvalue));
		}

		/* if list is full, we are done */

		if (rescount >= RESMAX) {
			selvalue = 0;
			break;
		}
	}

	/* make sure we did not end abnormally */

	if (selvalue != 0) {
		fprintf(stderr, "%s: select failure\n", argv[0]);
		exit(1);
	}

	/* cheap exit if time list is empty */

	if (rescount == 0) {
		printf("Network time indeterminate\n");
		exit(0);
	}

	/* sort the time list and pick median */

	qsort(reslist, rescount, sizeof(resvalue), tcomp);
	median = (rescount - 1) / 2;

	/* adjust selected value from base time to present */

	(void) gettimeofday(&nowt, &nowz);
	resvalue = reslist[median] + (nowt.tv_sec - baset.tv_sec);

	/* if setting, do it, otherwise just print conclusions */

	if (set == 0) {
		fprintf(stderr, "Network time is %s", ctime(&resvalue));
	} else {
		wtmp[0].ut_time = nowt.tv_sec;
		wtmp[1].ut_time = resvalue;
		nowt.tv_sec = resvalue;
		if (settimeofday(&nowt, &nowz) != 0) {
			fprintf(stderr, "%s: Time set failed\n", argv[0]);
			exit(1);
		}
		if ((wtmpfd = open(WTMP, O_WRONLY|O_APPEND)) >= 0) {
			(void) write(wtmpfd, wtmp, sizeof(wtmp));
			(void) close(wtmpfd);
		}
		printf("Time set to %s", ctime(&resvalue));
	}
}

/*
 *	This function aids the sort in the main routine.
 *	It compares two unsigned longs and returns accordingly.
 */

tcomp(first, second)
unsigned long *first, *second;
{
	if (*first < *second) {
		return(-1);
	} else if (*first > *second) {
		return(1);
	} else {
		return(0);
	}
}
