#ifndef lint
static char *sccsid = "@(#)nfsstat.c	4.2	ULTRIX	10/15/90";
#endif lint

/****************************************************************
 *								*
 *  Licensed to Digital Equipment Corporation, Maynard, MA	*
 *		Copyright 1985 Sun Microsystems, Inc.		*
 *			All rights reserved.			*
 *								*
 ****************************************************************/

/************************************************************************
 *			Modification History
 *
 * 15 Oct 90  -- dws
 *	Fixed to correctly handle crash dumps.
 *
 ************************************************************************/

/* 
 * nfsstat: Network File System statistics
 *
 */
#define KERNELBASE 0x80000000
#include <stdio.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/vmmac.h>
#include <machine/pte.h>
#include <machine/cpu.h>
#include <net/if.h>
#include <netinet/in.h>
#include <netinet/if_ether.h>
#ifdef sun
#include <sun/ndio.h>
#endif sun
#include <nlist.h>

struct nlist nl[] = {
#define	X_RCSTAT	0
	{ "_rcstat" },
#define	X_CLSTAT	1
	{ "_clstat" },
#define	X_RSSTAT	2
	{ "_rsstat" },
#define	X_SVSTAT	3
	{ "_svstat" },
#define X_SYSMAP	4
	{ "_Sysmap" },
#define X_SYSSIZ	5
	{ "_Syssize" },
#ifdef sun
#define	X_NDSTAT	6
	{ "_ndstat" },
#endif sun
	"",
};

#define coreadj(x)	((int)x - KERNELBASE)

int kflag = 0;			/* set if using core instead of kmem */
int kmem;			/* file descriptor for /dev/kmem */
char *vmunix = "/vmunix";	/* name for /vmunix */
char *core = "/dev/kmem";	/* name for /dev/kmem */

/*
 * client side rpc statistics
 */
struct {
        int     rccalls;
        int     rcbadcalls;
        int     rcretrans;
        int     rcbadxids;
        int     rctimeouts;
        int     rcwaits;
        int     rcnewcreds;
} rcstat;

/*
 * client side nfs statistics
 */
struct {
        int     nclsleeps;              /* client handle waits */
        int     nclgets;                /* client handle gets */
        int     ncalls;                 /* client requests */
        int     nbadcalls;              /* rpc failures */
        int     reqs[32];               /* count of each request */
} clstat;

/*
 * Server side rpc statistics
 */
struct {
        int     rscalls;
        int     rsbadcalls;
        int     rsnullrecv;
        int     rsbadlen;
        int     rsxdrcall;
} rsstat;

/*
 * server side nfs statistics
 */
struct {
        int     ncalls;         /* number of calls received */
        int     nbadcalls;      /* calls that failed */
        int     reqs[32];       /* count for each request */
} svstat;

#ifdef sun
struct ndstat ndstat;
#endif sun


main(argc, argv)
	char *argv[];
{
	char *options;
	int	cflag = 0;		/* client stats */
	int	dflag = 0;		/* network disk stats */
	int	nflag = 0;		/* nfs stats */
	int	rflag = 0;		/* rpc stats */
	int	sflag = 0;		/* server stats */
	int	zflag = 0;		/* zero stats after printing */


	if (argc >= 2 && *argv[1] == '-') {
		options = &argv[1][1];
		while (*options) {
			switch (*options) {
			case 'c':
				cflag++;
				break;
#ifdef sun
			case 'd':
				dflag++;
				break;
#endif sun
			case 'n':
				nflag++;
				break;
			case 'r':
				rflag++;
				break;
			case 's':
				sflag++;
				break;
			case 'z':
				if (getuid()) {
					fprintf(stderr,
					    "Must be root for z flag\n");
					exit(1);
				}
				zflag++;
				break;
			default:
				usage();
			}
			options++;
		}
		argv++;
		argc--;
	}
	if (argc >= 2) {
		vmunix = argv[1];
		argv++;
		argc--;
		if (argc == 2) {
			kflag++;
			core = argv[1];
			argv++;
			argc--;
		}
	}
	if (argc != 1) {
		usage();
	}


	setup(zflag);
	getstats();
#ifdef sun
	if (dflag || (dflag + cflag + sflag + nflag + rflag) == 0) {
		d_print(zflag);
	}
#endif sun
	if (dflag && (sflag + cflag + rflag + nflag) == 0) {
		if (zflag) {
			putstats();
		}
		exit(0);
	}
	if (sflag || (!sflag && !cflag)) {
		if (rflag || (!rflag && !nflag)) {
			sr_print(zflag);
		}
		if (nflag || (!rflag && !nflag)) {
			sn_print(zflag);
		}
	}
	if (cflag || (!sflag && !cflag)) {
		if (rflag || (!rflag && !nflag)) {
			cr_print(zflag);
		}
		if (nflag || (!rflag && !nflag)) {
			cn_print(zflag);
		}
	}
	if (zflag) {
		putstats();
	}
}

getstats()
{
	int size;

	if (klseek(kmem, (long)nl[X_RCSTAT].n_value, 0) == -1) {
		fprintf(stderr, "can't seek in kmem\n");
		exit(1);
	}
	if (read(kmem, &rcstat, sizeof rcstat) != sizeof rcstat) {
		fprintf(stderr, "can't read rcstat from kmem\n");
		exit(1);
	}

	if (klseek(kmem, (long)nl[X_CLSTAT].n_value, 0) == -1) {
		fprintf(stderr, "can't seek in kmem\n");
		exit(1);
	}
 	if (read(kmem, &clstat, sizeof(clstat)) != sizeof (clstat)) {
		fprintf(stderr, "can't read clstat from kmem\n");
		exit(1);
	}

	if (klseek(kmem, (long)nl[X_RSSTAT].n_value, 0) == -1) {
		fprintf(stderr, "can't seek in kmem\n");
		exit(1);
	}
 	if (read(kmem, &rsstat, sizeof(rsstat)) != sizeof (rsstat)) {
		fprintf(stderr, "can't read rsstat from kmem\n");
		exit(1);
	}

	if (klseek(kmem, (long)nl[X_SVSTAT].n_value, 0) == -1) {
		fprintf(stderr, "can't seek in kmem\n");
		exit(1);
	}
 	if (read(kmem, &svstat, sizeof(svstat)) != sizeof (svstat)) {
		fprintf(stderr, "can't read svstat from kmem\n");
		exit(1);
	}

#ifdef sun
	if (klseek(kmem, (long)nl[X_NDSTAT].n_value, 0) == -1) {
		fprintf(stderr, "can't seek in kmem\n");
		exit(1);
	}
 	if (read(kmem, &ndstat, sizeof(ndstat)) != sizeof (ndstat)) {
		fprintf(stderr, "can't read ndstat from kmem\n");
		exit(1);
	}
#endif sun
}

putstats()
{
	if (klseek(kmem, (long)nl[X_RCSTAT].n_value, 0) == -1) {
		fprintf(stderr, "can't seek in kmem\n");
		exit(1);
	}
	if (write(kmem, &rcstat, sizeof rcstat) != sizeof rcstat) {
		fprintf(stderr, "can't write rcstat to kmem\n");
		exit(1);
	}

	if (klseek(kmem, (long)nl[X_CLSTAT].n_value, 0) == -1) {
		fprintf(stderr, "can't seek in kmem\n");
		exit(1);
	}
 	if (write(kmem, &clstat, sizeof(clstat)) != sizeof (clstat)) {
		fprintf(stderr, "can't write clstat to kmem\n");
		exit(1);
	}

	if (klseek(kmem, (long)nl[X_RSSTAT].n_value, 0) == -1) {
		fprintf(stderr, "can't seek in kmem\n");
		exit(1);
	}
 	if (write(kmem, &rsstat, sizeof(rsstat)) != sizeof (rsstat)) {
		fprintf(stderr, "can't write rsstat to kmem\n");
		exit(1);
	}

	if (klseek(kmem, (long)nl[X_SVSTAT].n_value, 0) == -1) {
		fprintf(stderr, "can't seek in kmem\n");
		exit(1);
	}
 	if (write(kmem, &svstat, sizeof(svstat)) != sizeof (svstat)) {
		fprintf(stderr, "can't write svstat to kmem\n");
		exit(1);
	}

#ifdef sun
	if (klseek(kmem, (long)nl[X_NDSTAT].n_value, 0) == -1) {
		fprintf(stderr, "can't seek in kmem\n");
		exit(1);
	}
 	if (write(kmem, &ndstat, sizeof(ndstat)) != sizeof (ndstat)) {
		fprintf(stderr, "can't write ndstat to kmem\n");
		exit(1);
	}
#endif sun
}

#ifdef vax
#define clear(x)        ((int)x & 0x7fffffff)
struct pte *Sysmap = 0;
#endif vax

klseek(fd, loc, off)
	int fd;
	long loc;
	int off;
{
#ifdef sun
	if (kflag) {
		loc = coreadj(loc);
	}
	(void) lseek(fd, (long)loc, off);
#endif sun
#ifdef vax
	static int	sizeSysmap;
	if (kflag && Sysmap == 0) {
		sizeSysmap = nl[X_SYSSIZ].n_value * sizeof(struct pte);
		Sysmap = (struct pte *)calloc((unsigned)sizeSysmap, 1);

		lseek(kmem, clear(nl[X_SYSMAP].n_value), 0);
		if (read(kmem, Sysmap, sizeSysmap) != sizeSysmap)
			return(-1);
	}
	if (kflag && (loc&0x80000000)) { 
		struct pte *ptep;

		loc &= 0x7fffffff;
		ptep = &Sysmap[btop(loc)];
		if ((char *)ptep - (char *)Sysmap > sizeSysmap)
			return(-1);
		if (ptep->pg_v == 0)
			return(-1);
		loc = (off_t)((loc&PGOFSET) + ptob(ptep->pg_pfnum));
	}
	(void) lseek(fd, (long)loc, off);
#endif vax
#ifdef mips
	if (kflag && (loc & 0x80000000)) {
		loc = mkphys(loc);
	}
	(void) lseek(fd, (long)loc, off);
#endif mips
}

setup(zflag)
	int zflag;
{
	register struct nlist *nlp;

	nlist(vmunix, nl);
	if (nl[0].n_value == 0) {
		fprintf (stderr, "Variables missing from namelist\n");
		exit (1);
	}
	if (kflag) {
		for (nlp = nl; nlp < &nl[sizeof (nl)/sizeof (nl[0])]; nlp++)
#ifdef sun
			nlp->n_value = coreadj(nlp->n_value);
#endif sun
#ifdef vax
			nlp->n_value = clear(nlp->n_value);
#endif vax
#ifdef mips
			nlp->n_value = nlp->n_value;
#endif mips
	}
	if ((kmem = open(core, zflag ? 2 : 0)) < 0) {
		perror(core);
		exit(1);
	}
}

cr_print(zflag)
	int zflag;
{
	fprintf(stdout, "\nClient rpc:\n");
	fprintf(stdout,
	 "calls      badcalls   retrans    badxid     timeout    wait       newcred\n");
	fprintf(stdout,
	    "%-11d%-11d%-11d%-11d%-11d%-11d%-11d\n",
	    rcstat.rccalls,
            rcstat.rcbadcalls,
            rcstat.rcretrans,
            rcstat.rcbadxids,
            rcstat.rctimeouts,
            rcstat.rcwaits,
            rcstat.rcnewcreds);
	if (zflag) {
		bzero(&rcstat, sizeof rcstat);
	}
}

sr_print(zflag)
	int zflag;
{
	fprintf(stdout, "\nServer rpc:\n");
	fprintf(stdout,
	    "calls      badcalls   nullrecv   badlen     xdrcall\n");
	fprintf(stdout,
	    "%-11d%-11d%-11d%-11d%-11d\n",
           rsstat.rscalls,
           rsstat.rsbadcalls,
           rsstat.rsnullrecv,
           rsstat.rsbadlen,
           rsstat.rsxdrcall);
	if (zflag) {
		bzero(&rsstat, sizeof rsstat);
	}
}

#define RFS_NPROC       18
char *nfsstr[RFS_NPROC] = {
	"null",
	"getattr",
	"setattr",
	"root",
	"lookup",
	"readlink",
	"read",
	"wrcache",
	"write",
	"create",
	"remove",
	"rename",
	"link",
	"symlink",
	"mkdir",
	"rmdir",
	"readdir",
	"fsstat" };

cn_print(zflag)
	int zflag;
{
	int i;

	fprintf(stdout, "\nClient nfs:\n");
	fprintf(stdout,
	    "calls      badcalls   nclget     nclsleep\n");
	fprintf(stdout,
	    "%-11d%-11d%-11d%-11d\n",
            clstat.ncalls,
            clstat.nbadcalls,
            clstat.nclgets,
            clstat.nclsleeps);
	req_print((int *)clstat.reqs, clstat.ncalls);
	if (zflag) {
		bzero(&clstat, sizeof clstat);
	}
}

sn_print(zflag)
	int zflag;
{
	fprintf(stdout, "\nServer nfs:\n");
	fprintf(stdout, "calls      badcalls\n");
	fprintf(stdout, "%-11d%-11d\n", svstat.ncalls, svstat.nbadcalls);
	req_print((int *)svstat.reqs, svstat.ncalls);
	if (zflag) {
		bzero(&svstat, sizeof svstat);
	}
}

#ifdef sun
d_print(zflag)
	int zflag;
{
	fprintf(stdout, "\nNetwork Disk:\n");
	fprintf(stdout, "rcv %d  snd %d  retrans %d  (%.2f%%)\n",
	    ndstat.ns_rpacks,ndstat.ns_xpacks,ndstat.ns_rexmits,
	    (double)(ndstat.ns_rexmits*100)/ndstat.ns_xpacks);
	fprintf(stdout,
	    "notuser %d  noumatch %d  nobuf %d  lbusy %d  operrs %d\n",
	    ndstat.ns_notuser, ndstat.ns_noumatch, ndstat.ns_nobufs,
	    ndstat.ns_lbusy, ndstat.ns_lbusy);
	fprintf(stdout,
	    "rseq %d  wseq %d  badreq %d  stimo %d  utimo %d  iseq %d\n",
	    ndstat.ns_rseq, ndstat.ns_wseq, ndstat.ns_badreq, ndstat.ns_stimo,
	    ndstat.ns_utimo, ndstat.ns_iseq);
	if (zflag) {
		bzero(&ndstat, sizeof ndstat);
	}
}
#endif sun


req_print(req, tot)
	int	*req;
	int	tot;
{
	int	i, j;
	char	fixlen[128];

	for (i=0; i<=RFS_NPROC / 7; i++) {
		for (j=i*7; j<min(i*7+7, RFS_NPROC); j++) {
			fprintf(stdout, "%-11s", nfsstr[j]);
		}
		fprintf(stdout, "\n");
		for (j=i*7; j<min(i*7+7, RFS_NPROC); j++) {
			if (tot) {
				sprintf(fixlen,
				    "%d %2d%% ", req[j], (req[j]*100)/tot);
			} else {
				sprintf(fixlen, "%d 0%% ", req[j]);
			}
			fprintf(stdout, "%-11s", fixlen);
		}
		fprintf(stdout, "\n");
	}
}

usage()
{
#ifdef sun
	fprintf(stderr, "nfsstat [-cdnrsz] [vmunix] [core]\n");
#endif sun
#ifdef vax
	fprintf(stderr, "nfsstat [-cnrsz] [vmunix] [core]\n");
#endif vax
#ifdef mips
	fprintf(stderr, "nfsstat [-cnrsz] [vmunix] [core]\n");
#endif mips
	exit(1);
}

min(a,b)
	int a,b;
{
	if (a<b) {
		return(a);
	}
	return(b);
}

#ifdef mips
/*
 * The following routines are borrowed for ps(1).
 */
/*
 * "addr"  is a kern virt addr and does not correspond
 * To a phys addr after zipping out the high bit..
 * since it was valloc'd in the kernel.
 *
 * We return the phys addr by simulating kernel vm (/dev/kmem)
 * when we are reading a crash dump.
 */
#include <sys/fixpoint.h>

off_t
mkphys(addr)
	unsigned addr;
{
	register off_t o;
	unsigned addr2=addr;

	if (IS_KSEG0(addr)) {
		return(K0_TO_PHYS(addr));
	}
	if (IS_KSEG1(addr)) {
		return(K1_TO_PHYS(addr));
	}
	o = addr & PGOFSET;
	addr = 4 * btop(addr2 - K2BASE);
	addr = getsys((unsigned)nl[X_SYSMAP].n_value + addr);
	addr = (addr & PG_PFNUM) << (PGSHIFT - PTE_PFNSHIFT) | o;
	return(addr);
}

getsys(loc)
	register unsigned loc;
{
	register unsigned seekloc = 0;
	int word = 0;

	if (IS_KSEG0(loc)) {
		seekloc = K0_TO_PHYS(loc);
	}
	if (IS_KSEG1(loc)) {
		seekloc = K1_TO_PHYS(loc);
	}
	lseek(kmem, seekloc, 0);
	read(kmem, &word, sizeof (word));
	return (word);
}
#endif mips
