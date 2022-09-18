#ifndef lint
static char *sccsid = "@(#)nfslogsum.c	4.2	(ULTRIX)	1/25/91";
/* Based on: */
static char *RCSid = "$Header: /sparky/a/davy/system/nfswatch/RCS/nfslogsum.c,v 3.0 91/01/23 08:23:08 davy Exp $";
#endif

/*
 * nfslogsum - summarize nfswatch log file
 *
 * David A. Curry				Jeffrey C. Mogul
 * SRI International				Digital Equipment Corporation
 * 333 Ravenswood Avenue			Western Research Laboratory
 * Menlo Park, CA 94025				100 Hamilton Avenue
 * davy@erg.sri.com				Palo Alto, CA 94301
 *						mogul@decwrl.dec.com
 *
 * $Log:	nfslogsum.c,v $
 * Revision 3.0  91/01/23  08:23:08  davy
 * NFSWATCH Version 3.0.
 * 
 * Revision 1.2  90/08/17  15:47:27  davy
 * NFSWATCH Version 2.0.
 * 
 * Revision 1.1  88/11/29  11:20:38  davy
 * NFSWATCH Release 1.0
 * 
 */
#include <sys/param.h>
#include <stdio.h>

#include "nfswatch.h"

char		*pname;				/* program name		*/
char		*logfile = LOGFILE;		/* log file name	*/

int		logentries;			/* # of entries		*/
int		verbose = 0;			/* print more info	*/
int		maxlogentries = -1;		/* stop at this many	*/

Counter		total_pkts;			/* total pkts this time	*/
Counter		tohost_pkts;			/* total pkts to host	*/
Counter		dropped_pkts;			/* total dropped pkts	*/

char		*startdate;			/* time started		*/
char		*enddate;			/* time finished	*/

char		srchost[MAXHOSTNAMELEN];	/* source host name	*/
char		dsthost[MAXHOSTNAMELEN];	/* destination host name*/

NFSCounter	nfs_counters[MAXEXPORT];	/* NFS pkt counters	*/
FileCounter	fil_counters[MAXEXPORT];	/* file pkt counters	*/
PacketCounter	pkt_counters[PKT_NCOUNTERS];	/* type pkt counters	*/

char	*nfs_procs[] = {
	"NULLPROC",	" GETATTR",	" SETATTR",	" GETROOT",
	"  LOOKUP",	"READLINK",	"    READ",	"  WCACHE",
	"   WRITE",	"  CREATE",	"  REMOVE",	"  RENAME",
	"    LINK",	" SYMLINK",	"   MKDIR",	"   RMDIR",
	" READDIR",	"  STATFS",	0
};

char		*index();
char		*savestr();

void		dumpit();
void		dumpit2();
void		parsenfs();
void		parsetop();
void		parsefile();
void		parsepkts();
void		parseproc();
void		clearvars();
void		parsetotals();

main(argc, argv)
int argc;
char **argv;
{
	FILE *logfp;
	int printed = 0;
	char line[BUFSIZ];

	pname = *argv;

	/*
	 * Process arguments.
	 */
	while (--argc) {
		if (**++argv != '-') {
			logfile = *argv;
			continue;
		}

		switch (*++*argv) {
		case 'v':
			verbose++;
			break;
		default:
			maxlogentries = atoi(*argv);
			break;
		}
	}

	/*
	 * Open the log file.
	 */
	if ((logfp = fopen(logfile, "r")) == NULL) {
		(void) fprintf(stderr, "%s: cannot open \"%s\"\n", pname, logfile);
		(void) exit(1);
	}

	/*
	 * Read lines...
	 */
	while (fgets(line, sizeof(line), logfp) != NULL) {
		/*
		 * Skip blank comments.
		 */
		if (!strcmp(line, "#\n"))
			continue;

		/*
		 * Start of a new log session.
		 */
		if (!strcmp(line, "# startlog\n")) {
			clearvars();
			printed = 0;
			continue;
		}

		/*
		 * Start of a new log entry.
		 */
		if (!strcmp(line, "# begin\n")) {
			parsetop(logfp);
			continue;
		}

		/*
		 * End of a log entry.
		 */
		if (!strcmp(line, "# end\n")) {
			logentries++;

			/*
			 * If we've read the max, we're done.
			 */
			if ((maxlogentries > 0) &&
			    (logentries >= maxlogentries)) {
				dumpit();
				exit(0);
			}

			continue;
		}

		/*
		 * End of log session.
		 */
		if (!strcmp(line, "# endlog\n")) {
			printed++;
			dumpit();
			continue;
		}

		/*
		 * Start of NFS counter section.
		 */
		if (!strncmp(line, "# nfs counters ", 15)) {
			parsenfs(logfp);
			continue;
		}

		/*
		 * Start of file counter section.
		 */
		if (!strncmp(line, "# file counters ", 16)) {
			parsefile(logfp);
			continue;
		}

		/*
		 * Start of total packet section.
		 */
		if (!strncmp(line, "# total packets ", 16)) {
			parsetotals(logfp);
			continue;
		}

		/*
		 * Start of packet counter section.
		 */
		if (!strncmp(line, "# packet counters ", 18)) {
			parsepkts(logfp);
			continue;
		}

		/*
		 * Source host.
		 */
		if (!strncmp(line, "#    Packets from: ", 19)) {
			(void) strcpy(srchost, line + 19);
			continue;
		}

		/*
		 * Destination host.
		 */
		if (!strncmp(line, "#    Packets to:   ", 19)) {
			(void) strcpy(dsthost, line + 19);
			continue;
		}
	}

	/*
	 * In case there's no endlog (logfile in progress).
	 */
	if (!printed)
		dumpit();

	(void) fclose(logfp);
	(void) exit(0);
}

/*
 * clearvars - clear variables between log sessions.
 */
void
clearvars()
{
	register int i;

	logentries = 0;
	total_pkts = 0;
	tohost_pkts = 0;
	dropped_pkts = 0;

	if (startdate != NULL) {
		(void) free(startdate);
		startdate = NULL;
	}

	if (enddate != NULL) {
		(void) free(enddate);
		enddate = NULL;
	}

	(void) bzero((char *) pkt_counters,
		PKT_NCOUNTERS * sizeof(PacketCounter));

	/*
	 * Set up the strings.
	 */
	pkt_counters[PKT_NDREAD].pc_name = "ND Read";
	pkt_counters[PKT_NDWRITE].pc_name = "ND Write";
	pkt_counters[PKT_NFSREAD].pc_name = "NFS Read";
	pkt_counters[PKT_NFSWRITE].pc_name = "NFS Write";
	pkt_counters[PKT_NFSMOUNT].pc_name = "NFS Mount";
	pkt_counters[PKT_YELLOWPAGES].pc_name = "Yellow Pages/NIS";
	pkt_counters[PKT_RPCAUTH].pc_name = "RPC Authorization";
	pkt_counters[PKT_OTHERRPC].pc_name = "Other RPC Packets";
	pkt_counters[PKT_TCP].pc_name = "TCP Packets";
	pkt_counters[PKT_UDP].pc_name = "UDP Packets";
	pkt_counters[PKT_ICMP].pc_name = "ICMP Packets";
	pkt_counters[PKT_ROUTING].pc_name = "Routing Control";
	pkt_counters[PKT_ARP].pc_name = "Address Resolution";
	pkt_counters[PKT_RARP].pc_name = "Reverse Addr Resol";
	pkt_counters[PKT_BROADCAST].pc_name = "Ethernet Broadcast";
	pkt_counters[PKT_OTHER].pc_name = "Other Packets";

	for (i = 0; i < MAXEXPORT; i++) {
		if (nfs_counters[i].nc_name != NULL) {
			(void) free(nfs_counters[i].nc_name);
			nfs_counters[i].nc_name = NULL;
		}
	}

	(void) bzero((char *) nfs_counters,
		MAXEXPORT * sizeof(NFSCounter));

	(void) bzero((char *) fil_counters,
		MAXEXPORT * sizeof(FileCounter));
}

/*
 * parsetop - parse the top of the log session.
 */
void
parsetop(fp)
FILE *fp;
{
	int nskip = 0;
	char line[BUFSIZ];

	/*
	 * Get lines...
	 */
	while (fgets(line, sizeof(line), fp) != NULL) {
		/*
		 * Second blank comment terminates this section.
		 */
		if (!strcmp(line, "#\n")) {
			if (++nskip == 2)
				return;

			continue;
		}

		/*
		 * Grab the date - first one is start,
		 * otherwise end.
		 */
		if (!strncmp(line, "Date: ", 6)) {
			if (startdate == NULL) {
				startdate = savestr(line + 6);
				enddate = savestr(line + 6);
			}
			else {
				(void) strcpy(enddate, line + 6);
			}

			continue;
		}
	}
}

/*
 * parsetotals - parse the totals section.
 */
void
parsetotals(fp)
FILE *fp;
{
	int x, y, z;
	int nskip = 0;
	register char *s;
	char line[BUFSIZ];

	/*
	 * Get lines...
	 */
	while (fgets(line, sizeof(line), fp) != NULL) {
		/*
		 * Seond blank comment terminates this section.
		 */
		if (!strcmp(line, "#\n")) {
			if (++nskip == 2)
				return;

			continue;
		}

		/*
		 * Grab the interval packets and add them.
		 */
		if (!strncmp(line, "Interval Packets:", 17)) {
			(void) sscanf(line + 17, "%d %d %d", &x, &y, &z);

			total_pkts += x;
			tohost_pkts += y;
			dropped_pkts += z;

			continue;
		}
	}
}

/*
 * parsepkts - parse packet counter section.
 */
void
parsepkts(fp)
FILE *fp;
{
	int x, y, z;
	int nskip = 0;
	register int i;
	register char *s;
	char line[BUFSIZ];

	/*
	 * Get lines...
	 */
	while (fgets(line, sizeof(line), fp) != NULL) {
		/*
		 * Second blank comment terminates this section.
		 */
		if (!strcmp(line, "#\n")) {
			if (++nskip == 2)
				return;

			continue;
		}

		/*
		 * Find the numbers.
		 */
		if ((s = index(line, ':')) == NULL)
			continue;

		*s++ = NULL;

		/*
		 * Find the type we need.
		 */
		for (i = 0; i < PKT_NCOUNTERS; i++) {
			if (!strcmp(line, pkt_counters[i].pc_name)) {
				(void) sscanf(s, "%d %d%% %d", &x, &y, &z);

				pkt_counters[i].pc_total += x;
				break;
			}
		}
	}
}

/*
 * parsenfs - parse the NFS counter section.
 */
void
parsenfs(fp)
FILE *fp;
{
	int x, y, z;
	int nskip = 0;
	register int i;
	register char *s;
	char line[BUFSIZ];

	/*
	 * Get lines...
	 */
	while (fgets(line, sizeof(line), fp) != NULL) {
		/*
		 * Second blank comment terminates this section.
		 */
		if (!strcmp(line, "#\n")) {
			if (++nskip == 2)
				return;

			continue;
		}

		/*
		 * Find the numbers.
		 */
		if ((s = index(line, ':')) == NULL)
			continue;

		*s++ = NULL;

		/*
		 * Find the NFS counter we want.
		 */
		if ((i = findnfs(line)) < 0)
			continue;

		(void) sscanf(s, "%d %d%% %d", &x, &y, &z);
		nfs_counters[i].nc_total += x;

		if ((s = index(s, '(')) != NULL)
			parseproc(s, nfs_counters[i].nc_proc);
	}
}

/*
 * parsefile - parse the file counter section.
 */
void
parsefile(fp)
FILE *fp;
{
	int x, y, z;
	int nskip = 0;
	register int i;
	register char *s;
	char line[BUFSIZ];

	/*
	 * Get lines...
	 */
	while (fgets(line, sizeof(line), fp) != NULL) {
		/*
		 * Second blank comment terminates this section.
		 */
		if (!strcmp(line, "#\n")) {
			if (++nskip == 2)
				return;

			continue;
		}

		/*
		 * Find the numbers.
		 */
		if ((s = index(line, ':')) == NULL)
			continue;

		*s++ = NULL;

		/*
		 * Find the file counter we want.
		 */
		if ((i = findfile(line)) < 0)
			continue;

		(void) sscanf(s, "%d %d%% %d", &x, &y, &z);
		fil_counters[i].fc_total += x;

		if ((s = index(s, '(')) != NULL)
			parseproc(s, fil_counters[i].fc_proc);
	}
}

/*
 * parseproc - parse the NFS procedure counters.
 */
void
parseproc(line, ctrs)
Counter *ctrs;
char *line;
{
	register int i;
	register char *s;

	s = line + 1;

	for (i = 0; i < MAXNFSPROC; i++) {
		ctrs[i] += atoi(s);

		if ((s = index(s, '/')) == NULL)
			return;

		s++;
	}
}

/*
 * dumpit - print out the information.
 */
void
dumpit()
{
	float percent;
	char buf[BUFSIZ];
	register int i, nfstotal;

	(void) printf("NFSwatch logfile summary:\n");
	(void) printf("    Log time:     %.24s to %.24s\n", startdate, enddate);
	(void) printf("    Log entries:  %d\n", logentries);
	(void) printf("    Packets from: %s", srchost);
	(void) printf("    Packets to:   %s\n", dsthost);

	(void) printf("Total packets:\n");
	(void) printf("    %8d (network) %8d (to host) %8d (dropped)\n\n",
		total_pkts, tohost_pkts, dropped_pkts);

	(void) printf("Packet counters:\n");

	/*
	 * Print the packet counters.  Percentage is calculated as
	 * this counter over total packets.
	 */
	for (i = 0; i < PKT_NCOUNTERS; i++) {
		if (total_pkts) {
			percent = ((float) pkt_counters[i].pc_total /
				  (float) tohost_pkts) * 100.0;
		}
		else {
			percent = 0.0;
		}

		(void) sprintf(buf, "%s:", pkt_counters[i].pc_name);
		(void) printf("    %-25s %8d %7.0f%%\n",
			buf, pkt_counters[i].pc_total, percent);

		if (i == (PKT_NCOUNTERS/2 - 1))
			(void) putchar('\n');
	}

	/*
	 * Calculate the total number of NFS packets.
	 */
	nfstotal = pkt_counters[PKT_NFSWRITE].pc_total +
		   pkt_counters[PKT_NFSREAD].pc_total;

	if (nfs_counters[0].nc_name != NULL)
		(void) printf("\nNFS counters:\n");

	/*
	 * Print the NFS counters.  Percentage is calculated as
	 * packets this file system over total NFS packets.
	 */
	for (i = 0; i < MAXEXPORT; i++) {
		if (nfs_counters[i].nc_name == NULL)
			continue;

		if (nfstotal) {
			percent = ((float) nfs_counters[i].nc_total /
				  (float) nfstotal) * 100.0;
		}
		else {
			percent = 0.0;
		}

		(void) sprintf(buf, "%s:", nfs_counters[i].nc_name);
		(void) printf("    %-25s %8d %7.0f%%\n",
			buf, nfs_counters[i].nc_total, percent);
	}

	if (fil_counters[0].fc_name != NULL)
		(void) printf("\nFile counters:\n");

	/*
	 * Print the file counters.  Percentage is calculated as
	 * packets this file over total NFS packets.
	 */
	for (i = 0; i < MAXEXPORT; i++) {
		if (fil_counters[i].fc_name == NULL)
			continue;

		if (nfstotal) {
			percent = ((float) fil_counters[i].fc_total /
				  (float) nfstotal) * 100.0;
		}
		else {
			percent = 0.0;
		}

		(void) sprintf(buf, "%s:", fil_counters[i].fc_name);
		(void) printf("    %-25s %8d %7.0f%%\n",
			buf, fil_counters[i].fc_total, percent);
	}

	(void) putchar('\014');

	/*
	 * If needed, put out the extra info.
	 */
	if (verbose)
		dumpit2();
}

/*
 * dumpit2 - dump even more information.
 */
void
dumpit2()
{
	register int i, j, k;
	
	/*
	 * Print NFS procs in groups of 6.
	 */
	for (i = 0; i < MAXNFSPROC; i += 6) {
		/*
		 * Print header.
		 */
		(void) printf("NFSwatch logfile summary:\n");
		(void) printf("    Log time:     %.24s to %.24s\n",
			startdate, enddate);
		(void) printf("    Log entries:  %d\n", logentries);
		(void) printf("    Packets from: %s", srchost);
		(void) printf("    Packets to:   %s\n", dsthost);
		(void) printf("NFS Procedure Call Summary (part %d):\n",
			((i / 6) + 1));
		(void) printf("                         ");

		for (j = 0; j < 6; j++)
			(void) printf("%s ", nfs_procs[i+j]);

		(void) putchar('\n');

		/*
		 * Print NFS counters.
		 */
		for (j = 0; j < MAXEXPORT; j++) {
			if (nfs_counters[j].nc_name == NULL)
				continue;

			(void) printf("%-25s", nfs_counters[j].nc_name);

			for (k = 0; k < 6; k++) {
				(void) printf("%8d ",
					nfs_counters[j].nc_proc[i+k]);
			}

			(void) putchar('\n');
		}

		if (fil_counters[0].fc_name != NULL)
			(void) putchar('\n');

		/*
		 * Print file counters.
		 */
		for (j = 0; j < MAXEXPORT; j++) {
			if (fil_counters[j].fc_name == NULL)
				continue;

			(void) printf("%-25s", fil_counters[j].fc_name);

			for (k = 0; k < 6; k++) {
				(void) printf("%8d ",
					fil_counters[j].fc_proc[i+k]);
			}

			(void) putchar('\n');
		}

		(void) putchar('\014');
	}
}

/*
 * findnfs - find the counter associated with name.
 */
int
findnfs(name)
char *name;
{
	register int i;

	/*
	 * Look for it.
	 */
	for (i = 0; i < MAXEXPORT; i++) {
		/*
		 * At end of list.
		 */
		if (nfs_counters[i].nc_name == NULL)
			break;

		/*
		 * Found it.
		 */
		if (!strcmp(name, nfs_counters[i].nc_name))
			return(i);
	}

	/*
	 * If there's room, add it to the list.
	 */
	if (i < MAXEXPORT) {
		nfs_counters[i].nc_name = savestr(name);
		return(i);
	}

	return(-1);
}

/*
 * findfile - find the counter assopciated with name.
 */
int
findfile(name)
char *name;
{
	register int i;

	/*
	 * Look for it.
	 */
	for (i = 0; i < MAXEXPORT; i++) {
		/*
		 * At end of list.
		 */
		if (fil_counters[i].fc_name == NULL)
			break;

		/*
		 * Found it.
		 */
		if (!strcmp(name, fil_counters[i].fc_name))
			return(i);
	}

	/*
	 * If there's room, add it to the list.
	 */
	if (i < MAXEXPORT) {
		fil_counters[i].fc_name = savestr(name);
		return(i);
	}

	return(-1);
}

/*
 * savestr - save string in dynamic memory.
 */
char *
savestr(s)
char *s;
{
	char *t;
	char *malloc();

	if ((t = malloc(strlen(s) + 1)) == NULL) {
		(void) fprintf(stderr, "%s: out of memory.\n", pname);
		(void) exit(1);
	}

	(void) strcpy(t, s);

	return(t);
}
