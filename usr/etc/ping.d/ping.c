#ifndef lint
static char sccsid[] = "@(#)ping.c	4.1	ULTRIX 7/2/90";
#endif

/************************************************************************
 *									*
 *			Copyright (c) 1985, 1988 by			*
 *		Digital Equipment Corporation, Maynard, MA		*
 *			All rights reserved.				*
 *									*
 *   This software is furnished under a license and may be used and	*
 *   copied  only  in accordance with the terms of such license and	*
 *   with the  inclusion  of  the  above  copyright  notice.   This	*
 *   software  or  any	other copies thereof may not be provided or	*
 *   otherwise made available to any other person.  No title to and	*
 *   ownership of the software is hereby transferred.			*
 *									*
 *   This software is  derived	from  software	received  from	the	*
 *   University    of	California,   Berkeley,  from   Bell 		*
 *   Laboratories, and from Sun Microsystems.  Use, duplication, or	*
 *   disclosure is	subject  to					*
 *   restrictions  under  license  agreements  with  University  of	*
 *   California, AT&T, and with Sun Microsystems.			*
 *									*
 *   The information in this software is subject to change  without	*
 *   notice  and should not be construed as a commitment by Digital	*
 *   Equipment Corporation.						*
 *									*
 *   Digital assumes no responsibility for the use  or	reliability	*
 *   of its software on equipment which is not supplied by Digital.	*
 *									*
 ************************************************************************/
/*
 * ping -l[dvr] machine [datasize [npackets]] :
 *     Using the InterNet Control Message Protocol (ICMP) "ECHO" facility,
 *     measure round-trip-delays and packet loss across network paths.
 * 
 * ping machine:  attempts to see if machine is alive by pinging it for
 *      20 seconds 
 *
 * This program has to run SUID to ROOT to access the ICMP socket.
 *
 * 06/09/88  M. Parenti		Changed signal handlers to void.
 *
 * 1/18/88   L. Gottfredsen	Merged 2 versions of ping into this one
 */

#include <stdio.h>
#include <errno.h>
#include <sys/time.h>

#include <sys/param.h>
#include <sys/socket.h>
#include <sys/file.h>

#include <netinet/in_systm.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <sys/wait.h>

#include <netdb.h>

#define	MAXWAIT		10	/* max time to wait for response, sec. */
#define	MAXPACKET	4096	/* max packet size */
#ifndef MAXHOSTNAMELEN
#define MAXHOSTNAMELEN	64
#endif
#define DEFTIMEOUT 20

int	verbose;
u_char	packet[MAXPACKET];
int	options;
extern	int errno;

int s;			/* Socket file descriptor */
struct hostent *hp;	/* Pointer to host info */
struct timezone tz;	/* leftover */

struct sockaddr whereto;/* Who to ping */
int datalen;		/* How much data */

char usage[] = "Usage:  ping [-ldrv] host [data size [npackets]]\n";

char *hostname;
char hnamebuf[MAXHOSTNAMELEN];
char *inet_ntoa();

int npackets;
int ntransmitted = 0;		/* sequence # for outbound packets = #sent */
int ident;

int nreceived = 0;		/* # of packets we got back */
int timing = 0;
int tmin = 999999999;
int tmax = 0;
int tsum = 0;			/* sum of all times, for doing average */
void finish(), catcher();
void noanswer(), die();
int pid;
short lflg=0;			/* long version, more stats */

/*
 * 			M A I N
 */
main(argc, argv)
char *argv[];
{
	struct sockaddr_in from;
	char **av = argv;
        char *toaddr = NULL;
	struct sockaddr_in *to = (struct sockaddr_in *) &whereto;
	int on = 1;
	struct protoent *proto;
	int fromlen, size, timeout;
	union wait status;


	argc--, av++;
	while (argc > 0 && *av[0] == '-') {
		while (*++av[0]) switch (*av[0]) {
			case 'd':
				options |= SO_DEBUG;
				lflg=1;
				break;
			case 'r':
				options |= SO_DONTROUTE;
				lflg=1;
				break;
			case 'v':
				verbose++;
				lflg=1;
				break;
			case 'l':
				lflg=1;
				break;
		}
		argc--, av++;
	}
	if( argc < 1)  {
		printf(usage);
		exit(1);
	}

	bzero( (char *)&whereto, sizeof(struct sockaddr) );

	to->sin_family = AF_INET;
	to->sin_addr.s_addr = inet_addr(av[0]);
	if (to->sin_addr.s_addr != -1) {
		strcpy(hnamebuf, av[0]);
		hostname = hnamebuf;
	} else {
		hp = gethostbyname(av[0]);
		if (hp) {
			to->sin_family = hp->h_addrtype;
			bcopy(hp->h_addr, (caddr_t)&to->sin_addr, hp->h_length);
			hostname = hp->h_name;
                        toaddr = inet_ntoa(to->sin_addr.s_addr);

		} else {
			printf("%s: unknown host %s\n", argv[0], av[0]);
			exit(1);
		}
	}

	if( argc >= 2 ) {
		datalen = atoi( av[1] );
		lflg=1;
	}
	else
		datalen = 64-8;
	if (datalen > MAXPACKET) {
		fprintf(stderr, "ping: packet size too large\n");
		exit(1);
	}
	if (datalen >= sizeof(struct timeval))
		timing = 1;
	if (argc > 2) {
		npackets = atoi(av[2]);
		lflg=1;
	}

	ident = getpid() & 0xFFFF;

	if ((proto = getprotobyname("icmp")) == NULL) {
		fprintf(stderr, "icmp: unknown protocol\n");
		exit(10);
	}
	if ((s = socket(AF_INET, SOCK_RAW, proto->p_proto)) < 0) {
		perror("ping: socket");
		exit(5);
	}
	if (options & SO_DEBUG)
		setsockopt(s, SOL_SOCKET, SO_DEBUG, &on, sizeof(on));
	if (options & SO_DONTROUTE)
		setsockopt(s, SOL_SOCKET, SO_DONTROUTE, &on, sizeof(on));

	if (lflg) {		/* print out statistics */

       		printf("PING %s", hostname);
	        if (toaddr)
	               printf(" (%s)", toaddr);
	        printf(": %d data bytes\n", datalen);

		setlinebuf( stdout );

		signal( SIGINT, finish );
		signal(SIGALRM, catcher);

		catcher();	/* start things going */

		for (;;) {
			int len = sizeof (packet);
			fromlen = sizeof (from);

			if ( (size=recvfrom(s, packet, len, 0, &from, &fromlen)) < 0) {
				if( errno == EINTR )
					continue;
				perror("ping: recvfrom");
				continue;
			}
			pr_pack( packet, size, &from );
			if (npackets && nreceived >= npackets)
				finish();
		}
		/*NOTREACHED*/
	}
	else {		/* host alive or no answer from host */
                timeout = DEFTIMEOUT;
	
	        if ((pid = fork()) < 0) {
	            perror("ping: fork");
	            exit(1);
	        }
	        if (pid != 0) {         /* parent */
	                signal(SIGINT, die);
	                while (1) {
				if ( pinger() == -1) {
	                                perror("ping: sendto");
	                                kill (pid, SIGKILL);
	                                exit(1);
	                        }
	                        sleep(1);
	                        if (wait3(&status, WNOHANG, 0) == pid)
	                                if (status.w_termsig == 0)
	                                        exit(status.w_retcode);
	                                else
	                                        exit(-1);
	                }
	        }
	
	        if (pid == 0) {         /* child */
	                alarm(timeout);
	                signal(SIGALRM, noanswer);
	                while (1) {
				int len = sizeof(packet);
	                        fromlen = sizeof(from);

				if ((size = recvfrom(s, packet, len, 0, &from, &fromlen)) < 0) {
	                                perror("ping: recvfrom");
	                                continue;
	                        }

				pr_pack(packet,size,&from); 

				if (nreceived >= 1 ) {
	                        	printf("%s is alive\n", hostname);
					exit(0);
				}
	                }
        }
}

}

/*
 * 			C A T C H E R
 * 
 * This routine causes another PING to be transmitted, and then
 * schedules another SIGALRM for 1 second from now.
 * 
 * Bug -
 * 	Our sense of time will slowly skew (ie, packets will not be launched
 * 	exactly at 1-second intervals).  This does not affect the quality
 *	of the delay and loss statistics.
 */
void
catcher()
{
	int waittime;

	(void)pinger();
	if (npackets == 0 || ntransmitted < npackets)
		alarm(1);
	else {
		if (nreceived) {
			waittime = 2 * tmax / 1000;
			if (waittime == 0)
				waittime = 1;
		} else
			waittime = MAXWAIT;
		alarm(waittime);
		signal(SIGALRM, finish);
	}
}

/*
 * 			P I N G E R
 * 
 * Compose and transmit an ICMP ECHO REQUEST packet.  The IP packet
 * will be added on by the kernel.  The ID field is our UNIX process ID,
 * and the sequence number is an ascending integer.  The first 8 bytes
 * of the data portion are used to hold a UNIX "timeval" struct in VAX
 * byte-order, to compute the round-trip time.
 */
pinger()
{
	static u_char outpack[MAXPACKET];
	register struct icmp *icp = (struct icmp *) outpack;
	int i, cc;
	register struct timeval *tp = (struct timeval *) &outpack[8];
	register u_char *datap = &outpack[8+sizeof(struct timeval)];

	icp->icmp_type = ICMP_ECHO;
	icp->icmp_code = 0;
	icp->icmp_cksum = 0;
	icp->icmp_seq = ntransmitted++;
	icp->icmp_id = ident;		/* ID */

	cc = datalen+8;			/* skips ICMP portion */

	if (timing)
		gettimeofday( tp, &tz );

	for( i=8; i<datalen; i++)	/* skip 8 for time */
		*datap++ = i;

	/* Compute ICMP checksum here */
	icp->icmp_cksum = in_cksum( icp, cc );

	/* cc = sendto(s, msg, len, flags, to, tolen) */
	i = sendto( s, outpack, cc, 0, &whereto, sizeof(struct sockaddr) );

	if( i < 0 || i != cc )  {
		if( i<0 )  perror("sendto");
		printf("ping: wrote %s %d chars, ret=%d\n",
			hostname, cc, i );
		fflush(stdout);
		return(-1);
	}
}

/*
 * 			P R _ T Y P E
 *
 * Convert an ICMP "type" field to a printable string.
 */
char *
pr_type( t )
register int t;
{
	static char *ttab[] = {
		"Echo Reply",
		"ICMP 1",
		"ICMP 2",
		"Dest Unreachable",
		"Source Quence",
		"Redirect",
		"ICMP 6",
		"ICMP 7",
		"Echo",
		"ICMP 9",
		"ICMP 10",
		"Time Exceeded",
		"Parameter Problem",
		"Timestamp",
		"Timestamp Reply",
		"Info Request",
		"Info Reply"
	};

	if( t < 0 || t > 16 )
		return("OUT-OF-RANGE");

	return(ttab[t]);
}

/*
 *			P R _ P A C K
 *
 * Print out the packet, if it came from us.  This logic is necessary
 * because ALL readers of the ICMP socket get a copy of ALL ICMP packets
 * which arrive ('tis only fair).  This permits multiple copies of this
 * program to be run without having intermingled output (or statistics!).
 */
pr_pack( buf, cc, from )
char *buf;
int cc;
struct sockaddr_in *from;
{
	struct ip *ip;
	register struct icmp *icp;
	register long *lp = (long *) packet;
	register int i;
	struct timeval tv;
	struct timeval *tp;
	int hlen, triptime;

	from->sin_addr.s_addr = ntohl( from->sin_addr.s_addr );
	gettimeofday( &tv, &tz );

	ip = (struct ip *) buf;
	hlen = ip->ip_hl << 2;
	if (cc < hlen + ICMP_MINLEN) {
		if (verbose)
			printf("packet too short (%d bytes) from %s\n", cc,
				inet_ntoa(ntohl(from->sin_addr.s_addr)));
		return;
	}
	cc -= hlen;
	icp = (struct icmp *)(buf + hlen);
	if( icp->icmp_type != ICMP_ECHOREPLY )  {
		if (verbose) {
			printf("%d bytes from %s: ", cc,
				inet_ntoa(ntohl(from->sin_addr.s_addr)));
			printf("icmp_type=%d (%s)\n",
				icp->icmp_type, pr_type(icp->icmp_type) );
			for( i=0; i<12; i++)
			    printf("x%2.2x: x%8.8x\n", i*sizeof(long), *lp++ );
			printf("icmp_code=%d\n", icp->icmp_code );
		}
		if (!lflg) 
			fprintf(stderr,"icp->icmp_type  (%d)  !=  ICMP_ECHOREPLY (%d)\n",icp->icmp_type, ICMP_ECHOREPLY);
		return;
	}
	if( icp->icmp_id != ident )
		return;			/* 'Twas not our ECHO */

	tp = (struct timeval *)&icp->icmp_data[0];
	if (lflg) {
		printf("%d bytes from %s: ", cc, inet_ntoa(ntohl(from->sin_addr.s_addr)));
		printf("icmp_seq=%d. ", icp->icmp_seq );
	}
	if (timing) {
		tvsub( &tv, tp );
		triptime = tv.tv_sec*1000+(tv.tv_usec/1000);
		if (lflg) printf("time=%d. ms\n", triptime );
		tsum += triptime;
		if( triptime < tmin )
			tmin = triptime;
		if( triptime > tmax )
			tmax = triptime;
	} else
		putchar('\n');
	nreceived++;
}


/*
 *			I N _ C K S U M
 *
 * Checksum routine for Internet Protocol family headers (C Version)
 *
 */
in_cksum(addr, len)
u_short *addr;
int len;
{
	register int nleft = len;
	register u_short *w = addr;
	register u_short answer;
	register int sum = 0;
 	u_short odd_byte = 0;

	/*
	 *  Our algorithm is simple, using a 32 bit accumulator (sum),
	 *  we add sequential 16 bit words to it, and at the end, fold
	 *  back all the carry bits from the top 16 bits into the lower
	 *  16 bits.
	 */
	while( nleft > 1 )  {
		sum += *w++;
		nleft -= 2;
	}

	/* mop up an odd byte, if necessary */
	if( nleft == 1 ) {
               *(u_char *)(&odd_byte) = *(u_char *)w;
               sum += odd_byte;
	}


	/*
	 * add back carry outs from top 16 bits to low 16 bits
	 */
	sum = (sum >> 16) + (sum & 0xffff);	/* add hi 16 to low 16 */
	sum += (sum >> 16);			/* add carry */
	answer = ~sum;				/* truncate to 16 bits */
	return (answer);
}

/*
 * 			T V S U B
 * 
 * Subtract 2 timeval structs:  out = out - in.
 * 
 * Out is assumed to be >= in.
 */
tvsub( out, in )
register struct timeval *out, *in;
{
	if( (out->tv_usec -= in->tv_usec) < 0 )   {
		out->tv_sec--;
		out->tv_usec += 1000000;
	}
	out->tv_sec -= in->tv_sec;
}

/*
 *			F I N I S H
 *
 * Print out statistics, and give up.
 * Heavily buffered STDIO is used here, so that all the statistics
 * will be written with 1 sys-write call.  This is nice when more
 * than one copy of the program is running on a terminal;  it prevents
 * the statistics output from becomming intermingled.
 */
void
finish()
{
	printf("\n----%s PING Statistics----\n", hostname );
	printf("%d packets transmitted, ", ntransmitted );
	printf("%d packets received, ", nreceived );
	if (ntransmitted)
	    printf("%d%% packet loss",
		(int) (((ntransmitted-nreceived)*100) / ntransmitted ) );
	printf("\n");
	if (nreceived && timing)
	    printf("round-trip (ms)  min/avg/max = %d/%d/%d\n",
		tmin,
		tsum / nreceived,
		tmax );
	fflush(stdout);
	exit(0);
}
void
noanswer()
{
        printf("no answer from %s\n", hostname);
        exit(1);
}
void
die()
{
        kill (pid, SIGKILL);
        exit(1);
}


