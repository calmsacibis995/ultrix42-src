#ifndef lint
static char *sccsid = "@(#)nfswatch.c	4.2	ULTRIX	1/25/91";
/* Based on: */
static char *RCSid = "$Header: nfswatch.c,v 3.1 91/01/23 16:56:19 mogul Exp $";
#endif

/*
 * nfswatch - NFS server packet monitoring program.
 *
 * David A. Curry				Jeffrey C. Mogul
 * SRI International				Digital Equipment Corporation
 * 333 Ravenswood Avenue			Western Research Laboratory
 * Menlo Park, CA 94025				100 Hamilton Avenue
 * davy@erg.sri.com				Palo Alto, CA 94301
 *						mogul@decwrl.dec.com
 *
 * $Log:	nfswatch.c,v $
 * Revision 3.1  91/01/23  16:56:19  mogul
 * Black magic
 * 
 * Revision 3.0  91/01/23  08:23:11  davy
 * NFSWATCH Version 3.0.
 * 
 * Revision 1.5  91/01/04  16:05:15  davy
 * Updated version number.
 * 
 * Revision 1.4  91/01/04  15:54:29  davy
 * New features from Jeff Mogul.
 * 
 * Revision 1.3  90/12/04  08:06:41  davy
 * Changed version number to 2.1.
 * 
 * Revision 1.2  90/08/17  15:47:29  davy
 * NFSWATCH Version 2.0.
 * 
 * Revision 1.1  88/11/29  11:20:40  davy
 * NFSWATCH Release 1.0
 * 
 */
#include <sys/param.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <net/if.h>
#include <signal.h>
#include <errno.h>
#include <stdio.h>

#ifdef sun
#include <net/nit_if.h>
#include <net/nit_buf.h>

#define USE_NIT

char	*devices[] = {
	"le0", "ie0", "le1", "ie1", "le2", "ie2",
	"le3", "ie3", "le4", "ie4",
	0
};
#endif /* sun */

#ifdef ultrix
#include <net/pfilt.h>

#define USE_PFILT

char	*devices[] = {
	"pf0", "pf1", "pf2", "pf3", "pf4", "pf5",
	"pf6", "pf7", "pf8", "pf9",
	0
};
#endif /* ultrix */

#include "nfswatch.h"

char		*pname;				/* program name		*/

FILE		*logfp;				/* log file pointer	*/

Counter		pkt_total = 0;			/* total packets seen	*/
Counter		pkt_drops = 0;			/* total packets dropped*/
Counter		int_pkt_total = 0;		/* packets this interval*/
Counter		int_pkt_drops = 0;		/* dropped this interval*/
Counter		dst_pkt_total = 0;		/* total pkts to host	*/
Counter		int_dst_pkt_total = 0;		/* pkts to host this int*/

int		if_fd[MAXINTERFACES];		/* nit device file desc	*/

int		srcflag = 0;			/* "-src" specified	*/
int		dstflag = 0;			/* "-dst" specified	*/
int		allflag = 0;			/* "-all" specified	*/
int		allintf = 0;			/* "-allif" specified	*/
int		logging = 0;			/* 1 when logging on	*/
int		learnfs = 0;			/* learn other servers	*/
int		do_update = 0;			/* time to update screen*/
int		showwhich = 0;			/* show filesys or files*/
int		cycletime = CYCLETIME;		/* update cycle time	*/
int		truncation = 200;		/* pkt trunc len - magic*/
int		sortbyusage = 0;		/* sort by usage counts	*/
int		nnfscounters = 0;		/* # of NFS counters	*/
int		nfilecounters = 0;		/* # of file counters	*/
int		nclientcounters = 0;		/* # of client counters */
int		screen_inited = 0;		/* 1 when in curses	*/

struct timeval	starttime;			/* time we started	*/

int		ninterfaces;			/* number of interfaces	*/

u_long		thisdst = 0;			/* cached IP dst of pkt	*/
u_long		srcaddrs[MAXHOSTADDR];		/* src host net addrs	*/
u_long		dstaddrs[MAXHOSTADDR];		/* dst host net addrs	*/

char		myhost[MAXHOSTNAMELEN];		/* local host name	*/
char		srchost[MAXHOSTNAMELEN];	/* source host name	*/
char		dsthost[MAXHOSTNAMELEN];	/* destination host name*/

char		*prompt = PROMPT;		/* prompt string	*/
char		*filelist = NULL;		/* list of files	*/
char		*logfile = LOGFILE;		/* log file name	*/
char		*snapshotfile = SNAPSHOTFILE;	/* snapshot file name	*/

NFSCounter	nfs_counters[MAXEXPORT];	/* NFS request counters	*/
FileCounter	fil_counters[MAXEXPORT];	/* file request counters*/
PacketCounter	pkt_counters[PKT_NCOUNTERS];	/* packet counters	*/
ProcCounter	prc_counters[MAXNFSPROC+2];	/* procedure counters	*/
				/* extra space simplifies sort_prc_counters */
int		prc_countmap[MAXNFSPROC];	/* allows sorting	*/
ClientCounter	clnt_counters[MAXCLIENTS];	/* per-client counters	*/

#ifdef ultrix
void
fpe_warn()
{
	extern void finish();

	fprintf(stderr, "nfswatch: mystery bug encountered.\n");
	finish(-1);
}
#endif /* ultrix */

main(argc, argv)
int argc;
char **argv;
{
	register int i;
	struct sigvec sv;
	char *device = NULL;
	extern void finish(), nfswatch();

	pname = *argv;

	/*
	 * Get our host name.  The default destination
	 * host is the one we're running on.
	 */
	if (gethostname(myhost, sizeof(myhost)) < 0) {
		error("gethostname");
		finish(-1);
	}

	(void) strcpy(dsthost, myhost);

	/*
	 * Process arguments.
	 */
	while (--argc) {
		if (**++argv != '-')
			usage();

		/*
		 * Set destination host.
		 */
		if (!strcmp(*argv, "-dst")) {
			if (--argc <= 0)
				usage();

			(void) strcpy(dsthost, *++argv);
			dstflag++;
			continue;
		}

		/*
		 * Set source host.
		 */
		if (!strcmp(*argv, "-src")) {
			if (--argc <= 0)
				usage();

			(void) strcpy(srchost, *++argv);
			srcflag++;
			continue;
		}

		/*
		 * Device to use.
		 */
		if (!strcmp(*argv, "-dev")) {
			if (--argc <= 0)
				usage();

			device = *++argv;
			continue;
		}

		/*
		 * Log file name.
		 */
		if (!strcmp(*argv, "-lf")) {
			if (--argc <= 0)
				usage();

			logfile = *++argv;
			continue;
		}

		/*
		 * Snapshot file name.
		 */
		if (!strcmp(*argv, "-sf")) {
			if (--argc <= 0)
				usage();

			snapshotfile = *++argv;
			continue;
		}

		/*
		 * List of files.
		 */
		if (!strcmp(*argv, "-f")) {
			if (--argc <= 0)
				usage();

			if (showwhich == 0)
				showwhich = SHOWINDVFILES;

			filelist = *++argv;
			continue;
		}

		/*
		 * Change cycle time.
		 */
		if (!strcmp(*argv, "-t")) {
			if (--argc <= 0)
				usage();

			cycletime = atoi(*++argv);
			continue;
		}

		/*
		 * Show file systems.
		 */
		if (!strcmp(*argv, "-fs")) {
			showwhich = SHOWFILESYSTEM;
			continue;
		}

		/*
		 * Show individual files.
		 */
		if (!strcmp(*argv, "-if")) {
			showwhich = SHOWINDVFILES;
			continue;
		}

		/*
		 * Show NFS procedures
		 */
		if (!strcmp(*argv, "-procs")) {
			showwhich = SHOWNFSPROCS;
			continue;
		}

		/*
		 * Show NFS clients
		 */
		if (!strcmp(*argv, "-clients")) {
			showwhich = SHOWCLIENTS;
			continue;
		}

		/*
		 * Turn on logging.
		 */
		if (!strcmp(*argv, "-l")) {
			logging++;
			continue;
		}

		/*
		 * Watch all traffic.
		 */
		if (!strcmp(*argv, "-all")) {
			allflag++;
			continue;
		}

		/*
		 * Use all interfaces.
		 */
		if (!strcmp(*argv, "-allif")) {
			allintf++;
			continue;
		}

		/*
		 * Sort file systems by usage, not name.
		 */
		if (!strcmp(*argv, "-usage")) {
			sortbyusage++;
			continue;
		}

		usage();
	}

	/*
	 * Check what we're showing.
	 */
	switch (showwhich) {
	case 0:			/* default */
		showwhich = SHOWFILESYSTEM;
		break;
	case SHOWINDVFILES:
		if (filelist == NULL) {
			(void) fprintf(stderr, "%s: must specify file list with -fi.\n", pname);
			finish(-1);
		}

		break;
	}

	/*
	 * Trap signals so we can clean up.
	 */
	sv.sv_handler = finish;
	sv.sv_mask = sv.sv_flags = 0;

	(void) sigvec(SIGINT, &sv, (struct sigvec *) 0);
	(void) sigvec(SIGQUIT, &sv, (struct sigvec *) 0);

#ifdef ultrix
	sv.sv_handler = fpe_warn;
	(void) sigvec(SIGFPE, &sv, (struct sigvec *) 0);
#endif /* ultrix */

#ifdef USE_NIT
	/*
	 * Set up the network interface tap right away,
	 * since we probably need super-user permission.
	 */
	if (allintf) {
		ninterfaces = 0;
		for (i=0; devices[i] != NULL; i++) {
			if_fd[ninterfaces] = setup_nit_dev(&devices[i]);

			if (if_fd[ninterfaces] >= 0)
				ninterfaces++;
		}
	}
	else {
		if_fd[0] = setup_nit_dev(&device);

		if (if_fd[0] < 0) {
			perror(device);
			finish(-1);
		}

		ninterfaces = 1;
	}
#endif /* USE_NIT */

#ifdef USE_PFILT
	/*
	 * Set up the packet filter interface now,
	 * although we don't need super-user permission.
	 */
	if (allintf) {
		ninterfaces = 0;
		for (i=0; devices[i] != NULL; i++) {
			if_fd[ninterfaces] = setup_pfilt_dev(&devices[i]);

			if (if_fd[ninterfaces] >= 0)
				ninterfaces++;
		}
	}
	else {
		if_fd[0] = setup_pfilt_dev(&device);

		if (if_fd[0] < 0) {
			perror(device);
			finish(-1);
		}

		ninterfaces = 1;
	}
#endif /* USE_PFILT */

	if (ninterfaces < 1) {
		fprintf(stderr, "%s: no valid interfaces.\n", pname);
		finish(-1);
	}

	/*
	 * Now lose super-user permission, since we
	 * don't need it for anything else.
	 */
	(void) setreuid(getuid(), getuid());

	/*
	 * Look up the network addresses of the source and
	 * destination hosts.
	 */
	get_net_addrs();

	/*
	 * Tell the user what's going on.
	 */
	(void) printf("NFSWATCH Version %s\n", VERSION);
	(void) printf("Watch packets from %s to %s on ",
		(srcflag ? srchost : "all hosts"), dsthost);

	if (allintf)
		(void) printf("all interfaces;\n");
	else
		(void) printf("interface %s;\n", device);

	(void) printf("log to \"%s\" (logging %s);\n", logfile,
		(logging ? "on" : "off"));
	(void) printf("snapshots to \"%s\";\n", snapshotfile);
	(void) printf("cycle time %d seconds...", cycletime);
	(void) fflush(stdout);

	/*
	 * Set up a pseudo RPC server.
	 */
	setup_rpcxdr();

	/*
	 * Set up the screen.
	 */
	setup_screen(device);

	/*
	 * Set up the packet counters.  This must be done after
	 * setup_screen because they use the LINES variable.
	 */
	setup_pkt_counters();
	setup_nfs_counters();
	setup_proc_counters();

	if (filelist)
		setup_fil_counters();

	/*
	 * Now label the screen.
	 */
	label_screen();

	/*
	 * Open log file if logging is on.
	 */
	if (logging) {
		if ((logfp = fopen(logfile, "a")) == NULL) {
			error(logfile);
			finish(-1);
		}

		(void) fprintf(logfp, "#\n# startlog\n#\n");
		(void) fprintf(logfp, "# NFSwatch log file\n");
		(void) fprintf(logfp, "#    Packets from: %s\n",
			(srcflag ? srchost : "all hosts"));
		(void) fprintf(logfp, "#    Packets to:   %s\n#\n",
			dsthost);
	}

	/*
	 * Go watch packets.  Never returns.
	 */
	nfswatch();
}

/*
 * nfswatch - main packet reading loop.
 */
void
nfswatch()
{
	int i, cc;
	char *buf;
	char *malloc();
	fd_set readfds;
	struct sigvec sv;
	struct timeval tv;
	extern void wakeup();
	struct itimerval itv;
	register char *bp, *cp, *bufstop;
#ifdef USE_NIT
	int tdrops[MAXINTERFACES];
	struct nit_bufhdr *hdrp;
	struct nit_ifdrops *ndp;
#endif /* USE_NIT */

#ifdef USE_PFILT
	struct enstamp stamp;
	int datalen;
#endif /* USE_PFILT */

	/*
	 * Allocate a buffer so it's properly aligned for
	 * casting to structure types.
	 */
	if ((buf = malloc(NIT_CHUNKSIZE)) == NULL) {
		(void) fprintf(stderr, "%s: out of memory.\n", pname);
		finish(-1);
	}

	/*
	 * Set up the alarm handler.
	 */
	sv.sv_handler = wakeup;
	sv.sv_mask = sv.sv_flags = 0;

	(void) sigvec(SIGALRM, &sv, (struct sigvec *) 0);

	/*
	 * Set up the alarm clock.
	 */
	(void) bzero((char *) &itv, sizeof(struct itimerval));

	itv.it_interval.tv_sec = cycletime;
	itv.it_interval.tv_usec = 0;

	itv.it_value = itv.it_interval;

	(void) setitimer(ITIMER_REAL, &itv, (struct itimerval *) 0);

	/*
	 * Set the start time.
	 */
	(void) gettimeofday(&starttime, (struct timezone *) 0);

#ifdef USE_NIT
	/*
	 * Flush the read queue of any packets that accumulated
	 * during setup time.
	 */
	for (i=0; i < ninterfaces; i++) {
		flush_nit(if_fd[i]);
		tdrops[i] = 0;
	}

	for (;;) {
		FD_ZERO(&readfds);

		for (i=0; i < ninterfaces; i++)
			FD_SET(if_fd[i], &readfds);

		/*
		 * See which nets have packets to read.
		 */
		cc = select(NFDBITS, &readfds, (fd_set *) 0, (fd_set *) 0, 0);

		if ((cc < 0) && (errno != EINTR)) {
			error("select");
			finish(-1);
		}
		if (cc == 0) {
			continue;
		}
		
		/*
		 * For each interface...
		 */
		for (i=0; i < ninterfaces; i++) {
			/*
			 * Nothing to read.
			 */
			if (!FD_ISSET(if_fd[i], &readfds))
				continue;

			/*
			 * Now read packets from the nit device.
			 */
			if ((cc = read(if_fd[i], buf, NIT_CHUNKSIZE)) <= 0)
				continue;

			bufstop = buf + cc;
			bp = buf;

			/*
			 * Loop through the chunk, extracting packets.
			 */
			while (bp < bufstop) {
				cp = bp;

				/*
				 * Get the nit header.
				 */
				hdrp = (struct nit_bufhdr *) cp;
				cp += sizeof(struct nit_bufhdr);

				/*
				 * Get the number of dropped packets.
				 */
				ndp = (struct nit_ifdrops *) cp;
				cp += sizeof(struct nit_ifdrops);

				int_pkt_drops += ndp->nh_drops - tdrops[i];
				pkt_drops += ndp->nh_drops - tdrops[i];
				tdrops[i] = ndp->nh_drops;

				/*
				 * Filter the packet.
				 */
				pkt_filter(cp, hdrp->nhb_msglen);

				/*
				 * Skip over this packet.
				 */
				bp += hdrp->nhb_totlen;
			}
		}
#endif /* USE_NIT */

#ifdef USE_PFILT
	/*
	 * Flush the read queue of any packets that accumulated
	 * during setup time.
	 */
	for (i=0; i < ninterfaces; i++)
		flush_pfilt(if_fd[i]);

	for (;;) {
		FD_ZERO(&readfds);

		for (i=0; i < ninterfaces; i++)
			FD_SET(if_fd[i], &readfds);

		/*
		 * See which interfaces have any packets to read.
		 */
		cc = select(NFDBITS, &readfds, (fd_set *) 0, (fd_set *) 0, 0);

		if ((cc < 0) && (errno != EINTR)) {
			error("select");
			finish(-1);
		}
		if (cc == 0) {
			continue;
		}

		/*
		 * Now read packets from the packet filter device.
		 */
		for (i=0; i < ninterfaces; i++) {
			if (!FD_ISSET(if_fd[i], &readfds))
				continue;
			
			if ((cc = read(if_fd[i], buf, NIT_CHUNKSIZE)) < 0) {
				lseek(if_fd[i], 0L, 0);

				/*
				 * Might have read MAXINT bytes.  Try again.
				 */
				if ((cc = read(if_fd[i], buf, NIT_CHUNKSIZE)) < 0) {
					error("pfilt read");
					finish(-1);
				}
			}
		
			bp = buf;

			/*
			 * Loop through buffer, extracting packets.
			 */
			while (cc > 0) {
				/*
				 * Avoid alignment issues.
				 */
				(void) bcopy(bp, &stamp, sizeof(stamp));

				/*
				 * Treat entire buffer as garbage.
				 */
				if (stamp.ens_stamplen != sizeof(stamp))
					break;

				/*
				 * Get the number of dropped packets.
				 */
				int_pkt_drops += stamp.ens_dropped;
				pkt_drops += stamp.ens_dropped;

				/*
				 * Filter the packet.
				 */
				datalen = stamp.ens_count;

				if (datalen > truncation)
					datalen = truncation;

				pkt_filter(&(bp[sizeof(stamp)]), datalen);

				/*
				 * Skip over this packet.
				 */
				if (cc == (datalen + sizeof(stamp)))
					break;

				datalen = ENALIGN(datalen);
				datalen += sizeof(stamp);
				cc -= datalen;
				bp += datalen;
			}
		}

#endif /* USE_PFILT */

		tv.tv_sec = 0;
		tv.tv_usec = 0;
		FD_ZERO(&readfds);
		FD_SET(0, &readfds);

		/*
		 * See if a command has been typed.
		 */
		cc = select(NFDBITS, &readfds, (fd_set *) 0, (fd_set *) 0, &tv);

		if ((cc > 0) && FD_ISSET(0, &readfds))
			command();
	}
}
