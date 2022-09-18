#ifndef lint
static	char	*sccsid = "@(#)bootpd.c	4.1	ULTRIX	7/2/90 - 1.1 (Stanford) 1/22/86";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1984 by				*
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
 * BOOTP (bootstrap protocol) server daemon.
 *
 * Answers BOOTP request packets from booting client machines.
 * See [SRI-NIC]<RFC>RFC951.TXT for a description of the protocol.
 */

/*
 * history
 * 01/22/86	Croft	created.
 *
 * 07/30/86     Kovar modified to work at CMU.
 * 07/24/87	Drew D. Perkins at Carnegie Mellon University
 *		    Modified to use syslog instead of Kovar's
 *		    routines.  Add debugging dumps.  Many other fixups.
 * 01/28/88	Jeffrey Mogul/DECWRL
 *			removed CMU-specific stuff but kept improvements
 *			and restored much of the Stanford code for
 *			figuring out the bootfile because the CMU code
 *			was quite broken.
 * 03/01/88	Jeffrey Mogul/DECWRL
 *			Modified for use with /etc/inetd; use
 * bootp dgram udp wait /etc/bootpd bootpd -i
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/file.h>

#include <net/if.h>
#include <netinet/in.h>

#include <signal.h>
#include <stdio.h>
#include <strings.h>
#include <errno.h>
#include <ctype.h>
#include <netdb.h>
#include <setjmp.h>
#include <syslog.h>

#include <protocols/bootp.h>

#define	DALLYTIME	60	/* seconds to wait for additional requests */
#define SYSLOG

int debug = 0;			/* Debugging flag */
int inetd = 0;			/* Running under inetd */
int s;				/* Socket file descriptor */
struct sockaddr_in sin;
struct sockaddr_in from;	/* Packet source */
u_char buf[1024];		/* Receive packet buffer */

struct ifreq ifreq[10];		/* Holds interface configuration */
struct ifconf ifconf;		/* Int. config ioctl block (pnts to ifreq) */
struct arpreq arpreq;		/* Arp request ioctl block */


/*
 * Globals below are associated with the bootp database file (bootptab).
 */
char *bootptab = "/etc/bootptab";
#ifdef DEBUG
char * bootpd_dump = "/tmp/bootpd.dump";
#endif
FILE *fp = NULL;		/* Boot FILE */
char line[256];			/* Line buffer for reading bootptab */
char *linep;			/* Pointer to 'line' */
int linenum;			/* Current line number in bootptab */
char homedir[64];		/* Bootfile homedirectory */
char defaultboot[64];		/* Default file to boot */

struct hosts {
	struct hosts *next;	/* Next in list */
	char host[31];		/* Host name (and suffix) */
	u_char htype;		/* Hardware type */
	u_char haddr[6];	/* Hardware address */
	struct in_addr iaddr;	/* Internet address */
	char bootfile[32];	/* Default boot file name */
} *hosts = NULL;

int nhosts;			/* Current number of hosts */
long modtime = 0;		/* Last modification time of bootptab */
char *haddrtoa();		/* Convert haddr to ascii */


main(argc, argv)
	int argc;
	char *argv[];
{
	struct bootp *bp = (struct bootp *) buf;
	int n, fromlen;
	void readtab();
#ifdef DEBUG
	void dumptab();
#endif

	/*
	 * Read switches.
	 */
	for (argc--, argv++; argc > 0; argc--, argv++) {
		if (argv[0][0] == '-') {
			switch (argv[0][1]) {
			    case 'd':
				debug++;
				break;
			    case 'i':
				inetd++;
				break;
			}
		}
	}

	/*
	 * Go into background and disassociate from controlling terminal.
	 */
	if ((debug < 2) && (inetd == 0)) {
		if (fork())
			exit(0);
		for (n = 0; n < 10; n++)
			(void) close(n);
		(void) open("/", 0);
		(void) dup2(0, 1);
		(void) dup2(0, 2);
		n = open("/dev/tty", 2);
		if (n >= 0) {
			ioctl(n, TIOCNOTTY, (char *) 0);
			(void) close(n);
		}
	}

#ifdef SYSLOG
	/*
	 * Initialize logging.
	 */
#ifdef	LOG_DAEMON
	/* 4.3-style syslog */
	openlog("bootpd", LOG_PID | LOG_CONS, LOG_DAEMON);
#else
	openlog("bootpd", LOG_PID);
#endif	LOG_DAEMON
	syslog(LOG_INFO, "startup");
#endif

	if (inetd == 0) {
	    /*
	     * Get us a socket.
	     */
	    if ((s = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
#ifdef SYSLOG
		    syslog(LOG_ERR, "socket: %m");
#endif
		    perror("bootpd: socket");
		    exit(1);
	    }

	    /*
	     * Bind socket to BOOTPS port.
	     */
	    sin.sin_family = AF_INET;
	    sin.sin_addr.s_addr = INADDR_ANY;
	    sin.sin_port = htons(IPPORT_BOOTPS);
	    if (bind(s, &sin, sizeof(sin)) < 0) {
#ifdef SYSLOG
		syslog(LOG_ERR, "bind: %m");
#endif
		perror("bootpd: bind");
		exit(1);
	    }
	} else {
	    /*
	     * Running under inetd
	     */
	    s = 0;	/* inetd gives us socket as stdin */
	}	/* end "if (inetd)" */

	/*
	 * Determine network configuration.
	 */
	ifconf.ifc_len = sizeof(ifreq);
	ifconf.ifc_req = ifreq;
	if ((ioctl(s, SIOCGIFCONF, (caddr_t) &ifconf) < 0) ||
	    (ifconf.ifc_len <= 0)) {
#ifdef SYSLOG
		syslog(LOG_ERR, "ioctl: %m");
#endif
		perror("bootpd: ioctl");
		exit(1);
	}
	
	/*
	 * Read the bootptab file once immediately upon startup.
	 */
	readtab();

	/*
	 * Set up signals to read or dump the table.
	 */
	if ((int) signal(SIGHUP, readtab) == -1) {
#ifdef SYSLOG
		syslog(LOG_ERR, "signal: %m");
#endif
	    perror("bootpd: signal");
	    exit(1);
	}
#ifdef DEBUG
	if ((int) signal(SIGTERM, dumptab) == -1) {
#ifdef SYSLOG
		syslog(LOG_ERR, "signal: %m");
#endif
	    perror("bootpd: signal");
	    exit(1);
	}
#endif

	/*
	 * Process incoming requests.
	 */
	for (;;) {
		if (inetd)
		    alarm(DALLYTIME);
		fromlen = sizeof(from);
		n = recvfrom(s, buf, sizeof(buf), 0, &from, &fromlen);
		if (n <= 0)
			continue;

		if (n < sizeof(struct bootp)) {
			if (debug) {
#ifdef SYSLOG
				syslog(LOG_INFO, "received short packet");
#endif
			}
			continue;
		}

		readtab();	/* maybe re-read bootptab */
		switch (bp->bp_op) {
		    case BOOTREQUEST:
			request();
			break;

		    case BOOTREPLY:
			reply();
			break;
		}
	}
}


/*
 * Process BOOTREQUEST packet.
 *
 * (Note, this version of the bootp.c server never forwards 
 * the request to another server.  In our environment the 
 * stand-alone gateways perform that function.)
 *
 * (Also this version does not interpret the hostname field of
 * the request packet;  it COULD do a name->address lookup and
 * forward the request there.)
 */
request()
{
	register struct bootp *bp = (struct bootp *) buf;
	register struct hosts *hp;
	register n;
	char path[64], file[64];
	
	bp->bp_op = BOOTREPLY;
	if (bp->bp_ciaddr.s_addr == 0) { 
		/*
		 * client doesnt know his IP address, 
		 * search by hardware address.
		 */
		if (debug) {
#ifdef SYSLOG
			syslog(LOG_INFO,
			    "Processing boot request from hw addr %s",
			    haddrtoa(bp->bp_chaddr));
#endif
		}
		for (hp = hosts, n = 0; n < nhosts && hp; n++, hp = hp->next) {
		    if ((bp->bp_htype == hp->htype) &&
			(bcmp(bp->bp_chaddr, hp->haddr, 6) == 0))
			break;
		}
		if ((n == nhosts) || hp == NULL) {
#ifdef SYSLOG
		    syslog(LOG_NOTICE, "hw addr not found: %s",
			    haddrtoa(bp->bp_chaddr));
#endif
		    return;	/* not found */
		}
		if (debug) {
#ifdef SYSLOG
			syslog(LOG_INFO, "Found %s", inet_ntoa(hp->iaddr));
#endif
		}
		bp->bp_yiaddr = hp->iaddr;
	} else {
		/*
		 * search by IP address.
		 */
		if (debug) {
#ifdef SYSLOG
			syslog(LOG_INFO,
			    "Processing boot request from IP addr %s",
			    inet_ntoa(bp->bp_ciaddr));
#endif
		}
		for (hp = hosts, n = 0; n < nhosts && hp; n++, hp = hp->next)
		    if (bp->bp_ciaddr.s_addr == hp->iaddr.s_addr)
			break;
		if ((n == nhosts) || hp == NULL) {
#ifdef SYSLOG
		    syslog(LOG_NOTICE,
			"IP addr not found: %s", inet_ntoa(bp->bp_ciaddr));
#endif
		    return;
		}
	}

	if (strcmp(bp->bp_file, "sunboot14") == 0)	/* OBSOLETE? */
		bp->bp_file[0] = 0;	/* pretend it's null */

	if (bp->bp_file[0] == 0) { /* if client did not specify file */
		if (hp->bootfile[0] == 0)
			strcpy(file, defaultboot);
		else
			strcpy(file, hp->bootfile);
	} else {
		/* client did specify file */
		strcpy(file, bp->bp_file);
	}

	if (file[0] == '/')	/* if absolute pathname */
		strcpy(path, file);
	else {
		strcpy(path, homedir);
		strcat(path, "/");
		strcat(path, file);
	}

	/* try first to find the file with a ".host" suffix */
	n = strlen(path);
	strcat(path, ".");
	strcat(path, hp->host);
	if (access(path, R_OK) < 0) {
		path[n] = 0;	/* try it without the suffix */
		if (access(path, R_OK) < 0) {
		    if (bp->bp_file[0]) {
			/*
			 * Client wanted specific file
			 * and we didnt have it.
			 */
#ifdef SYSLOG
			syslog(LOG_NOTICE,
			    "requested file not found: %s", path);
#endif
			return;
		    }
		}
	}
	
	/* Ok, we know the filename and the file exists */
	strcpy(bp->bp_file, path);
#ifdef	SYSLOG
	if (debug)
		syslog(LOG_INFO, "bootfile is %s\n", path);
#endif	SYSLOG

	SetVendorInfo(bp, hp);

	sendreply(0);
}

SetVendorInfo(bp, hp)
register struct bootp *bp;
register struct hosts *hp;
{
	/* For now, nothing */
	bzero(bp->bp_vend, sizeof(bp->bp_vend));
}


/*
 * Process BOOTREPLY packet (something is using us as a gateway).
 */
reply()
{
	if (debug) {
#ifdef SYSLOG
		syslog(LOG_INFO, "Processing boot reply");
#endif
	}
	sendreply(1);
}


/*
 * Send a reply packet to the client.  'forward' flag is set if we are
 * not the originator of this reply packet.
 */
sendreply(forward)
{
	register struct bootp *bp = (struct bootp *) buf;
	struct in_addr dst;
	struct sockaddr_in to;

	to.sin_family = AF_INET;
	to.sin_addr.s_addr = INADDR_ANY;
	to.sin_port = htons(IPPORT_BOOTPC);

	/*
	 * If the client IP address is specified, use that
	 * else if gateway IP address is specified, use that
	 * else make a temporary arp cache entry for the client's NEW 
	 * IP/hardware address and use that.
	 */
	if (bp->bp_ciaddr.s_addr) {
		dst = bp->bp_ciaddr;
	} else if (bp->bp_giaddr.s_addr && forward == 0) {
		dst = bp->bp_giaddr;
		to.sin_port = htons(IPPORT_BOOTPS);
	} else {
		dst = bp->bp_yiaddr;
		setarp(&dst, bp->bp_chaddr, bp->bp_hlen);
	}

	if (forward == 0) {
		/*
		 * If we are originating this reply, we
		 * need to find our own interface address to
		 * put in the bp_siaddr field of the reply.
		 * If this server is multi-homed, pick the
		 * 'best' interface (the one on the same net
		 * as the client).
		 */
		int maxmatch = 0;
		int len, m;
		register struct ifreq *ifrq, *ifrmax;

		ifrmax = ifrq = &ifreq[0];
		len = ifconf.ifc_len;
		for (; len > 0; len -= sizeof(ifreq[0]), ifrq++) {
			m = nmatch(&dst, &((struct sockaddr_in *)
					  (&ifrq->ifr_addr))->sin_addr);
			if (m > maxmatch) {
				maxmatch = m;
				ifrmax = ifrq;
			}
		}
		if (bp->bp_giaddr.s_addr == 0) {
			if (maxmatch == 0) {
				return;
			}
			bp->bp_giaddr = ((struct sockaddr_in *)
				(&ifrmax->ifr_addr))->sin_addr;
		}
		bp->bp_siaddr = ((struct sockaddr_in *)
			(&ifrmax->ifr_addr))->sin_addr;
	}

	to.sin_addr = dst; 
	if (sendto(s, bp, sizeof(struct bootp), 0, &to, sizeof(to)) < 0) {
#ifdef SYSLOG
	    syslog(LOG_ERR, "sendto: %m");
#endif
	    perror("bootpd: sendto");
	}
}


/*
 * Return the number of leading bytes matching in the
 * internet addresses supplied.
 */
nmatch(ca,cb)
	register char *ca, *cb;
{
	register n,m;

	for (m = n = 0 ; n < 4 ; n++) {
		if (*ca++ != *cb++)
			return(m);
		m++;
	}
	return(m);
}


/*
 * Setup the arp cache so that IP address 'ia' will be temporarily
 * bound to hardware address 'ha' of length 'len'.
 */
setarp(ia, ha, len)
	struct in_addr *ia;
	u_char *ha;
	int len;
{
	struct sockaddr_in *si;
	
	bzero((caddr_t)&arpreq, sizeof(arpreq));
	
	arpreq.arp_pa.sa_family = AF_INET;
	si = (struct sockaddr_in *) &arpreq.arp_pa;
	si->sin_addr = *ia;

	arpreq.arp_flags = ATF_INUSE | ATF_COM;
	
	bcopy(ha, arpreq.arp_ha.sa_data, len);

	if (ioctl(s, SIOCSARP, (caddr_t)&arpreq) < 0) {
#ifdef SYSLOG
	    syslog(LOG_ERR, "ioctl(SIOCSARP): %m");
#endif
	    perror("bootpd: ioctl(SIOCSARP)");
	}
}


/*
 * Read bootptab database file.  Avoid rereading the file if the
 * write date hasn't changed since the last time we read it.
 */
void
readtab()
{
	struct stat st;
	register char *cp;
	int v;
	register int i;
	char temp[64];
	register struct hosts *hp, **hpp;
	int skiptopercent;

	/*
	 * If the file is open already check last modification time.
	 */
	if (fp != 0) {
		fstat(fileno(fp), &st);
		if (st.st_mtime == modtime && st.st_nlink)
			/*
			 * hasnt been modified or deleted yet.
			 */
			return;
		/*
		 * Close and reopen it.
		 */
		fclose(fp);
#ifdef SYSLOG
		syslog(LOG_INFO, "found new bootptab");
#endif
	}

	/*
	 * Open bootptab file.
	 */
	if ((fp = fopen(bootptab, "r")) == NULL) {
#ifdef SYSLOG
		syslog(LOG_ERR, "error opening %s: %m", bootptab);
#endif
		perror("bootpd: opening bootptab");
		exit(1);
	}

	/*
	 * Record file modification time.
	 */
	fstat(fileno(fp), &st);
	modtime = st.st_mtime;
	homedir[0] = defaultboot[0] = 0;
	nhosts = 0;
	hp = hosts;
	hpp = &hosts;
	linenum = 0;
	skiptopercent = 1;

	/* 
	 * read and parse each line in the file.
	 */
	for (;;) {
		if (fgets(line, sizeof(line), fp) == NULL)
			break;		/* done */

		if ((i = strlen(line)))
			line[i - 1] = 0; /* remove trailing newline */
		linep = line;
		linenum++;
		if (line[0] == '#' || line[0] == 0 || line[0] == ' ')
			continue;	/* skip comment lines */

		if (skiptopercent) {	/* allow for leading fields */
			if (line[0] != '%') {
			    readhdr();
			    continue;
			}
			skiptopercent = 0;
			continue;
		}

		if (hp == NULL) {
			if (!(*hpp = hp = (struct hosts *) calloc(1, sizeof(struct hosts)))) {
#ifdef SYSLOG
				syslog(LOG_ERR, "malloc failure");
#endif
				fprintf(stderr, "bootpd: malloc failure\n");
				exit(1);
			}
			hp->next = NULL;
		}

		/* fill in host table */
		getfield(hp->host, sizeof(hp->host));
		getfield(temp, sizeof(temp));
		sscanf(temp, "%d", &v);
		hp->htype = v;

		getfield(temp, sizeof(temp));
		cp = temp;

		/* parse hardware address */
		for (i = 0; i < sizeof(hp->haddr); i++) {
			if (*cp == '.' || *cp == ':' || *cp == '-')
				cp++;

			if (!isxdigit(cp[0]) || !isxdigit(cp[1]))
				goto badentry;

			if (sscanf(cp, "%2x", &v) != 1)
				goto badentry;

			cp += 2; 
			hp->haddr[i] = v;
		}
		getfield(temp, sizeof(temp));
		if ((i = inet_addr(temp)) == -1 || i == 0) {
			goto badentry;
		}
		hp->iaddr.s_addr = i;
		getfield(hp->bootfile, sizeof(hp->bootfile));

goodentry:
		++nhosts;
		hpp = &(hp->next);
		hp = hp->next;

badentry: 
		continue;
	}
#ifdef SYSLOG
	syslog(LOG_INFO, "read %d entries from bootptab", nhosts);
#endif
}


/*
 * Parse header lines of bootptab.
 */
readhdr()
{
	u_long i;
	char temp[64];

	/* fill in fixed leading fields */
	if (homedir[0] == 0) {
		getfield(homedir, sizeof(homedir));
		return;
	}
	if (defaultboot[0] == 0) {
		getfield(defaultboot, sizeof(defaultboot));
		return;
	}

}


/*
 * Get next field from 'line' buffer into 'str'.  'linep' is the 
 * pointer to current position.
 */
getfield(str, len)
	char *str;
{
	register char *cp = str;

	for ( ; *linep && (*linep == ' ' || *linep == '\t') ; linep++)
		;	/* skip spaces/tabs */
	if (*linep == 0) {
		*cp = 0;
		return;
	}
	len--;	/* save a spot for a null */
	for (; *linep && *linep != ' ' & *linep != '\t'; linep++) {
		*cp++ = *linep;
		if (--len <= 0) {
			*cp = 0;
			return;
		}
	}
	*cp = 0;
}


#ifdef DEBUG
/*
 * Dump bootptab to bootpd_dump.
 */
void
dumptab()
{
	register FILE *fp;
	register struct hosts *hp;
	register int n;
	long t;

	/*
	 * Open bootpd.dump file.
	 */
	if ((fp = fopen(bootpd_dump, "w")) == NULL) {
#ifdef SYSLOG
		syslog(LOG_ERR, "error opening %s: %m", bootpd_dump);
#endif
		perror("bootpd: opening bootpd.dump");
		exit(1);
	}

	t = time(NULL);
	fprintf(fp,
"#\n# %s: dump of bootp server database.\n#\n# dump taken %s\n#\n\
# home directory\n\%s\n\n# default bootfile\n%s\n\n",
		bootpd_dump, ctime(&t), homedir, defaultboot);
	fprintf(fp,
	    "%%%%\n# host htype haddr iaddr bootfile\n");

	for (hp = hosts, n = 0; n < nhosts && hp; n++, hp = hp->next) {
	    fprintf(fp, "%-16s %d %s %-15s %8s\n",
		hp->host, hp->htype, haddrtoa(hp->haddr),
		inet_ntoa(hp->iaddr), hp->bootfile);
	}
	fclose(fp);
#ifdef SYSLOG
	syslog(LOG_INFO, "dumped %d entries to %s", n, bootpd_dump);
#endif
}
#endif


/*
 * Convert a hardware address to an ascii string.
 */
char *haddrtoa(h)
unsigned char *h;
{
	static char haddrbuf[18];

	sprintf(haddrbuf, "%02x:%02x:%02x:%02x:%02x:%02x",
	    h[0], h[1], h[2], h[3], h[4], h[5]);
	return (haddrbuf);
}
