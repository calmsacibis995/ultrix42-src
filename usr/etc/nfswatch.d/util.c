#ifndef lint
static char *sccsid = "@(#)util.c	4.2	(ULTRIX)	1/25/91";
#endif
/* Based on:
 * RCSid = "$Header: /sparky/a/davy/system/nfswatch/RCS/util.c,v 3.0 91/01/23 08:23:29 davy Exp $";
 */

/*
 * util.c - miscellaneous utility routines.
 *
 * David A. Curry				Jeffrey C. Mogul
 * SRI International				Digital Equipment Corporation
 * 333 Ravenswood Avenue			Western Research Laboratory
 * Menlo Park, CA 94025				100 Hamilton Avenue
 * davy@erg.sri.com				Palo Alto, CA 94301
 *						mogul@decwrl.dec.com
 *
 * $Log:	util.c,v $
 * Revision 3.0  91/01/23  08:23:29  davy
 * NFSWATCH Version 3.0.
 * 
 * Revision 1.4  91/01/17  10:18:08  davy
 * New features from Jeff Mogul.
 * 
 * Revision 1.6  91/01/07  15:35:20  mogul
 * Must rebuild client hash table after sorting
 * 
 * Revision 1.5  91/01/07  14:09:10  mogul
 * Improved sortbyusage stuff
 * 
 * Revision 1.4  91/01/04  14:14:27  mogul
 * Support for client counters
 * 
 * Revision 1.3  91/01/03  17:42:20  mogul
 * Support for per-procedure counters
 * 
 * Revision 1.2  90/08/17  15:47:50  davy
 * NFSWATCH Version 2.0.
 * 
 * Revision 1.1  88/11/29  11:21:05  davy
 * NFSWATCH Release 1.0
 * 
 */
#include <sys/param.h>
#include <sys/stat.h>
#include <curses.h>
#include <stdio.h>
#ifdef sun
#include <mntent.h>
#include <exportent.h>
#endif /* sun */
#ifdef ultrix
#include <sys/mount.h>
#endif /* ultrix */

#include "nfswatch.h"
#include "externs.h"
#include "screen.h"

/*
 * clear_vars - set interval counters to zero.
 */
void
clear_vars()
{
	register int i;

	int_pkt_total = 0;
	int_pkt_drops = 0;
	int_dst_pkt_total = 0;

	for (i = 0; i < PKT_NCOUNTERS; i++)
		pkt_counters[i].pc_interval = 0;

	for (i = 0; i < nnfscounters; i++) {
		(void) bzero((char *) nfs_counters[i].nc_proc,
			MAXNFSPROC * sizeof(Counter));

		nfs_counters[i].nc_interval = 0;
	}

	for (i = 0; i < nfilecounters; i++) {
		(void) bzero((char *) fil_counters[i].fc_proc,
			MAXNFSPROC * sizeof(Counter));

		fil_counters[i].fc_interval = 0;
	}

	for (i = 0; i < MAXNFSPROC; i++)
		prc_counters[i].pr_interval = 0;

	for (i = 0; i < nclientcounters; i++)
		clnt_counters[i].cl_interval = 0;
}

/*
 * savestr - save a string in dynamically allocated memory.
 */
char *
savestr(s)
register char *s;
{
	char *malloc();
	register char *str;

	str = malloc(strlen(s) + 1);

	if (str == NULL) {
		(void) fprintf(stderr, "%s: out of memory.\n", pname);
		finish(-1);
	}

	(void) strcpy(str, s);
	return(str);
}

/*
 * prtime - convert a time to hh:mm:ss.
 */
char *
prtime(sec)
time_t sec;
{
	int hh, mm, ss;
	static char tbuf[16];

	hh = sec / 3600;
	sec %= 3600;

	mm = sec / 60;
	sec %= 60;

	ss = sec;

	(void) sprintf(tbuf, "%02d:%02d:%02d", hh, mm, ss);
	return(tbuf);
}

/*
 * error - print an error message preceded by the program name.
 */
void
error(str)
register char *str;
{
	char buf[BUFSIZ];

	(void) sprintf(buf, "%s: %s", pname, str);
	(void) perror(buf);
}

/*
 * finish - clean up and exit.
 */
void
finish(code)
int code;
{
	/*
	 * Close the nit device.
	 */
	if (if_fd >= 0)
		(void) close(if_fd);

	/*
	 * End curses.
	 */
	if (screen_inited) {
#ifdef nocrmode
		(void) nocrmode();
#else
		(void) nocbreak();
#endif /* nocrmode */
		(void) echo();

		(void) move(SCR_PROMPT_Y, SCR_PROMPT_X0);
		(void) clrtoeol();
		(void) refresh();
		(void) endwin();
	}

	if (logging) {
		(void) fprintf(logfp, "#\n# endlog\n#\n");
		(void) fclose(logfp);
	}

	(void) putchar('\n');

	if (code < 0)
		(void) exit(-code);

	(void) exit(0);
}

/*
 * setup_pkt_counters - set up packet counter screen coordinates.
 */
void
setup_pkt_counters()
{
	register int i, j;

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

	/*
	 * Set screen coordinates for everything.
	 */
	for (i = 0, j = PKT_NCOUNTERS/2; i < PKT_NCOUNTERS/2; i++, j++) {
		pkt_counters[i].pc_namex = SCR_PKT_NAME_X;
		pkt_counters[j].pc_namex = SCR_PKT_NAME_X + SCR_MIDDLE;
		pkt_counters[i].pc_namey = SCR_PKT_Y + i;
		pkt_counters[j].pc_namey = SCR_PKT_Y + i;

		pkt_counters[i].pc_intx = SCR_PKT_INT_X;
		pkt_counters[j].pc_intx = SCR_PKT_INT_X + SCR_MIDDLE;
		pkt_counters[i].pc_inty = SCR_PKT_Y + i;
		pkt_counters[j].pc_inty = SCR_PKT_Y + i;

		pkt_counters[i].pc_totx = SCR_PKT_TOT_X;
		pkt_counters[j].pc_totx = SCR_PKT_TOT_X + SCR_MIDDLE;
		pkt_counters[i].pc_toty = SCR_PKT_Y + i;
		pkt_counters[j].pc_toty = SCR_PKT_Y + i;

		pkt_counters[i].pc_pctx = SCR_PKT_PCT_X;
		pkt_counters[j].pc_pctx = SCR_PKT_PCT_X + SCR_MIDDLE;
		pkt_counters[i].pc_pcty = SCR_PKT_Y + i;
		pkt_counters[j].pc_pcty = SCR_PKT_Y + i;
	}
}

/*
 * setup_nfs_counters- setup NFS counter screen coordinates, file system
 *		       names.
 */
void
setup_nfs_counters()
{
	FILE *fp;
	struct stat st;
	register int i, j;
	register NFSCounter *nc;
#ifdef sun
	register struct mntent *mnt;
#endif /* sun */
#ifdef ultrix
	int dummy, nmnts;
	static struct fs_data fsData[MAXEXPORT];
#endif /* ultrix */

	(void) bzero((char *) nfs_counters,
		MAXEXPORT * sizeof(NFSCounter));

	/*
	 * If we're not watching our own host, we can't look
	 * for mounted file systems.
	 */
	if (strcmp(myhost, dsthost) != 0) {
		learnfs = 1;
		sort_nfs_counters();
		return;
	}

#ifdef sun
	/*
	 * Open the list of mounted file systems.
	 */
	if ((fp = setmntent(MOUNTED, "r")) == NULL) {
		error(MOUNTED);
		finish(-1);
	}

	nc = nfs_counters;

	/*
	 * Save the first MAXEXPORT file systems of type "4.2"
	 * which have been exported.  These are the ones which can
	 * be mounted through NFS.
	 */
	while ((mnt = getmntent(fp)) != NULL) {
		if (strcmp(mnt->mnt_type, MNTTYPE_42) != 0)
			continue;

		if (nnfscounters < MAXEXPORT) {
			if (stat(mnt->mnt_dir, &st) < 0)
				continue;

			/*
			 * Not exported; skip it.
			 */
			if (!is_exported(st.st_dev))
				continue;

			nc->nc_dev = st.st_dev;
			nc->nc_name = savestr(mnt->mnt_dir);

			nnfscounters++;
			nc++;
		}
	}

	(void) endmntent(fp);
#endif /* sun */

#ifdef ultrix
	/*
	 * Get the mounted file information.
	 */
	dummy = 0;
	nmnts = getmnt(&dummy, fsData, sizeof(fsData), NOSTAT_MANY, 0);

	if (nmnts < 0) {
		error("getmnt");
		finish(-1);
	}

	nc = nfs_counters;

	/*
	 * Save the first MAXEXPORT file systems which could have been
	 * exported.  These are what can be mounted through NFS.
	 */
	if (nmnts > MAXEXPORT)
		nmnts = MAXEXPORT;

	for (i=0; i < nmnts; i++) {
		if (fsData[i].fd_flags & M_LOCAL) {
			nc->nc_dev = fsData[i].fd_dev;
			nc->nc_name = savestr(fsData[i].fd_path);

			nnfscounters++;
			nc++;
		}
	}
#endif /* ultrix */

	sort_nfs_counters();
}

/*
 * sort_nfs_counters - sort and assign places on the screen
 */
void
sort_nfs_counters()
{
	register int i, j;

	(void) qsort(nfs_counters, nnfscounters, sizeof(NFSCounter), nfs_comp);

	/*
	 * Set screen coordinates for the ones which will be
	 * displayed.
	 */
	for (i = 0, j = NFSLINES/2; i < NFSLINES/2; i++, j++) {
		nfs_counters[i].nc_namex = SCR_NFS_NAME_X;
		nfs_counters[j].nc_namex = SCR_NFS_NAME_X + SCR_MIDDLE;
		nfs_counters[i].nc_namey = SCR_NFS_Y + i;
		nfs_counters[j].nc_namey = SCR_NFS_Y + i;

		nfs_counters[i].nc_intx = SCR_NFS_INT_X;
		nfs_counters[j].nc_intx = SCR_NFS_INT_X + SCR_MIDDLE;
		nfs_counters[i].nc_inty = SCR_NFS_Y + i;
		nfs_counters[j].nc_inty = SCR_NFS_Y + i;

		nfs_counters[i].nc_totx = SCR_NFS_TOT_X;
		nfs_counters[j].nc_totx = SCR_NFS_TOT_X + SCR_MIDDLE;
		nfs_counters[i].nc_toty = SCR_NFS_Y + i;
		nfs_counters[j].nc_toty = SCR_NFS_Y + i;

		nfs_counters[i].nc_pctx = SCR_NFS_PCT_X;
		nfs_counters[j].nc_pctx = SCR_NFS_PCT_X + SCR_MIDDLE;
		nfs_counters[i].nc_pcty = SCR_NFS_Y + i;
		nfs_counters[j].nc_pcty = SCR_NFS_Y + i;
	}
}

/*
 * setup_fil_counters- setup file counter stuff.
 */
void
setup_fil_counters()
{
	FILE *fp;
	struct stat st;
	register int i, j;
	char fname[MAXPATHLEN];
	register FileCounter *fc;

	(void) bzero((char *) fil_counters,
		MAXEXPORT * sizeof(FileCounter));

	/*
	 * If we're not watching our own host, we can't look
	 * for individual files.
	 */
	if (strcmp(myhost, dsthost) != 0)
		return;

	/*
	 * Open the list of files to watch.
	 */
	if ((fp = fopen(filelist, "r")) == NULL) {
		error(filelist);
		finish(-1);
	}

	fc = fil_counters;

	/*
	 * Save the first MAXEXPORT file systems of type "4.2"
	 * which have been exported.  These are the ones which can
	 * be mounted through NFS.
	 */
	while (fgets(fname, sizeof(fname), fp) != NULL) {
		fname[strlen(fname)-1] = '\0';

		if (nfilecounters < MAXEXPORT) {
			if (stat(fname, &st) < 0)
				continue;

			/*
			 * Not on an exported file system; skip it.
			 */
			if (!is_exported(st.st_dev)) {
				(void) fprintf(stderr, "warning: \"%s\" is not on an exported file system.\n", fname);
				continue;
			}

			fc->fc_dev = st.st_dev;
			fc->fc_ino = st.st_ino;
			fc->fc_name = savestr(fname);

			nfilecounters++;
			fc++;
		}
	}

	(void) fclose(fp);

	(void) qsort(fil_counters, nfilecounters, sizeof(FileCounter), fil_comp);

	/*
	 * Set screen coordinates for the ones which will be
	 * displayed.
	 */
	for (i = 0, j = NFSLINES/2; i < NFSLINES/2; i++, j++) {
		fil_counters[i].fc_namex = SCR_NFS_NAME_X;
		fil_counters[j].fc_namex = SCR_NFS_NAME_X + SCR_MIDDLE;
		fil_counters[i].fc_namey = SCR_NFS_Y + i;
		fil_counters[j].fc_namey = SCR_NFS_Y + i;

		fil_counters[i].fc_intx = SCR_NFS_INT_X;
		fil_counters[j].fc_intx = SCR_NFS_INT_X + SCR_MIDDLE;
		fil_counters[i].fc_inty = SCR_NFS_Y + i;
		fil_counters[j].fc_inty = SCR_NFS_Y + i;

		fil_counters[i].fc_totx = SCR_NFS_TOT_X;
		fil_counters[j].fc_totx = SCR_NFS_TOT_X + SCR_MIDDLE;
		fil_counters[i].fc_toty = SCR_NFS_Y + i;
		fil_counters[j].fc_toty = SCR_NFS_Y + i;

		fil_counters[i].fc_pctx = SCR_NFS_PCT_X;
		fil_counters[j].fc_pctx = SCR_NFS_PCT_X + SCR_MIDDLE;
		fil_counters[i].fc_pcty = SCR_NFS_Y + i;
		fil_counters[j].fc_pcty = SCR_NFS_Y + i;
	}
}

/*
 * Names of NFS procedures (MUST be kept in the right order)
 *	two leading spaces make things look nicer
 */
char	*nfs_procs[] = {
	"  NULLPROC",	"  GETATTR",	"  SETATTR",	"  GETROOT",
	"  LOOKUP",	"  READLINK",	"  READ",	"  WCACHE",
	"  WRITE",	"  CREATE",	"  REMOVE",	"  RENAME",
	"  LINK",	"  SYMLINK",	"  MKDIR",	"  RMDIR",
	"  READDIR",	"  STATFS",	0
};

/*
 * setup_proc_counters- setup procedure counter stuff.
 */
void
setup_proc_counters()
{
	register int i;
	register ProcCounter *pc;

	(void) bzero((char *) prc_counters,
		MAXNFSPROC * sizeof(ProcCounter));

	for (i = 0; i < MAXNFSPROC; i++) {
	    prc_counters[i].pr_type = i;
	    prc_counters[i].pr_name = nfs_procs[i];
	}

	sort_prc_counters();
}

/*
 * sort_prc_counters - sort and assign places on the screen
 */
sort_prc_counters()
{
	register int i, j;
	register int numlines;

	(void) qsort(prc_counters, MAXNFSPROC, sizeof(ProcCounter), prc_comp);

	/* Create indirection index */
	for (i = 0; i < MAXNFSPROC; i++)
	    prc_countmap[prc_counters[i].pr_type] = i;

	/*
	 * Set screen coordinates for the ones which will be
	 * displayed.
	 */
	numlines = MAXNFSPROC;
	if (MAXNFSPROC & 1)	/* round up to even number; extra space */
	    numlines++;		/* has been provided (see nfswatch.c)   */
	if (NFSLINES < numlines)
	    numlines = NFSLINES;

	for (i = 0, j = numlines/2; i < numlines/2; i++, j++) {
		prc_counters[i].pr_namex = SCR_NFS_NAME_X;
		prc_counters[j].pr_namex = SCR_NFS_NAME_X + SCR_MIDDLE;
		prc_counters[i].pr_namey = SCR_NFS_Y + i;
		prc_counters[j].pr_namey = SCR_NFS_Y + i;

		prc_counters[i].pr_intx = SCR_NFS_INT_X;
		prc_counters[j].pr_intx = SCR_NFS_INT_X + SCR_MIDDLE;
		prc_counters[i].pr_inty = SCR_NFS_Y + i;
		prc_counters[j].pr_inty = SCR_NFS_Y + i;

		prc_counters[i].pr_totx = SCR_NFS_TOT_X;
		prc_counters[j].pr_totx = SCR_NFS_TOT_X + SCR_MIDDLE;
		prc_counters[i].pr_toty = SCR_NFS_Y + i;
		prc_counters[j].pr_toty = SCR_NFS_Y + i;

		prc_counters[i].pr_pctx = SCR_NFS_PCT_X;
		prc_counters[j].pr_pctx = SCR_NFS_PCT_X + SCR_MIDDLE;
		prc_counters[i].pr_pcty = SCR_NFS_Y + i;
		prc_counters[j].pr_pcty = SCR_NFS_Y + i;
	}
}

/*
 * setup_clnt_counters- setup client counter stuff.
 */
void
setup_clnt_counters()
{
	register int i;

	(void) bzero((char *) clnt_counters,
		MAXCLIENTS * sizeof(ProcCounter));

	sort_clnt_counters();
}

/*
 * sort_clnt_counters - sort and assign places on the screen
 */
sort_clnt_counters()
{
	register int i, j;
	register int numlines;

	(void) qsort(clnt_counters, nclientcounters, sizeof(ClientCounter),
			clnt_comp);
	ClientHashRebuild();

	/*
	 * Set screen coordinates for the ones which will be
	 * displayed.
	 */
	numlines = nclientcounters;
	if (numlines & 1)	/* round up to even number; nfswatch.h	*/
	    numlines++;		/* must set MAXCLIENTS to even number	*/
	if (NFSLINES < numlines)
	    numlines = NFSLINES;

	for (i = 0, j = numlines/2; i < numlines/2; i++, j++) {
		clnt_counters[i].cl_namex = SCR_NFS_NAME_X;
		clnt_counters[j].cl_namex = SCR_NFS_NAME_X + SCR_MIDDLE;
		clnt_counters[i].cl_namey = SCR_NFS_Y + i;
		clnt_counters[j].cl_namey = SCR_NFS_Y + i;

		clnt_counters[i].cl_intx = SCR_NFS_INT_X;
		clnt_counters[j].cl_intx = SCR_NFS_INT_X + SCR_MIDDLE;
		clnt_counters[i].cl_inty = SCR_NFS_Y + i;
		clnt_counters[j].cl_inty = SCR_NFS_Y + i;

		clnt_counters[i].cl_totx = SCR_NFS_TOT_X;
		clnt_counters[j].cl_totx = SCR_NFS_TOT_X + SCR_MIDDLE;
		clnt_counters[i].cl_toty = SCR_NFS_Y + i;
		clnt_counters[j].cl_toty = SCR_NFS_Y + i;

		clnt_counters[i].cl_pctx = SCR_NFS_PCT_X;
		clnt_counters[j].cl_pctx = SCR_NFS_PCT_X + SCR_MIDDLE;
		clnt_counters[i].cl_pcty = SCR_NFS_Y + i;
		clnt_counters[j].cl_pcty = SCR_NFS_Y + i;
	}
}

/*
 * is_exported - return whether or not a file system is exported.
 */
#ifdef sun
int
is_exported(dev)
register dev_t dev;
{
	FILE *fp;
	struct stat st;
	register dev_t *exp;
	static int nexported = -1;
	register struct exportent *xent;
	static dev_t exported[MAXEXPORT];

	/*
	 * First time through, read the export table and
	 * save all the device numbers.
	 */
	if (nexported < 0) {
		/*
		 * If there's no export file, it must
		 * not be exported.
		 */
		if ((fp = setexportent(TABFILE)) == NULL)
			return(FALSE);

		nexported = 0;

		while ((xent = getexportent(fp)) != NULL) {
			if (stat(xent->xent_dirname, &st) < 0)
				continue;

			if (nexported < MAXEXPORT)
				exported[nexported++] = st.st_dev;
		}

		(void) endexportent(fp);
	}

	/*
	 * Search the exported device numbers for this device number.
	 */
	for (exp = exported; exp < &exported[nexported]; exp++) {
		if (dev == *exp)
			return(TRUE);
	}

	return(FALSE);
}
#endif /* sun */

#ifdef ultrix
int
is_exported(dev)
register dev_t dev;
{
	return(TRUE);
}
#endif /* ultrix */

/*
 * nfs_comp - compare NFS counters for qsort.
 */
int
nfs_comp(a, b)
register NFSCounter *a, *b;
{
	if (sortbyusage) {
	    if (((long)b->nc_interval) == ((long)a->nc_interval))
		return(((long)b->nc_total) - ((long)a->nc_total));
	    else
		return(((long)b->nc_interval) - ((long)a->nc_interval));
	}
	else
		return(strcmp(a->nc_name, b->nc_name));
}

/*
 * fil_comp = compare file counters for qsort.
 */
int
fil_comp(a, b)
register FileCounter *a, *b;
{
	if (sortbyusage) {
	    if (((long)b->fc_interval) == ((long)a->fc_interval))
		return(((long)b->fc_total) - ((long)a->fc_total));
	    else
		return(((long)b->fc_interval) - ((long)a->fc_interval));
	}
	else
		return(strcmp(a->fc_name, b->fc_name));
}

/*
 * prc_comp - compare procedure counters for qsort.
 */
int
prc_comp(a, b)
register ProcCounter *a, *b;
{
	if (sortbyusage) {
	    if (((long)b->pr_interval) == ((long)a->pr_interval))
		return(((long)b->pr_total) - ((long)a->pr_total));
	    else
		return(((long)b->pr_interval) - ((long)a->pr_interval));
	}
	else
		return(strcmp(a->pr_name, b->pr_name));
}

/*
 * clnt_comp - compare client counters for qsort.
 */
int
clnt_comp(a, b)
register ClientCounter *a, *b;
{
	if (sortbyusage) {
	    if (((long)b->cl_interval) == ((long)a->cl_interval))
		return(((long)b->cl_total) - ((long)a->cl_total));
	    else
		return(((long)b->cl_interval) - ((long)a->cl_interval));
	}
	else
		return(strcmp(a->cl_name, b->cl_name));
}

/*
 * usage - print a usage message and exit.
 */
void
usage()
{
	fprintf(stderr, "Usage: %s [-dst host] [-src host] [-all]", pname);
	fprintf(stderr, " [-dev device] [-allif]\n");
        fprintf(stderr, "       [-f filelist] [-lf logfile] [-sf snapfile]");
	fprintf(stderr, "[-t timeout]\n");
        fprintf(stderr, "       [-usage] [-fs] [-if] [-procs] [-clients]");
	fprintf(stderr, "[-l]\n");
	finish(-1);
}

/*
 * wakeup - wake up for a screen update.
 */
void
wakeup()
{
	/*
	 * Re-sort and re-do the labels.
	 */
	if (sortbyusage) {
		sort_nfs_counters();
		sort_prc_counters();
		sort_clnt_counters();
	}

	label_screen();
	update_screen();

	if (logging)
		update_logfile();

	clear_vars();
}
