#ifndef lint
static  char    *sccsid = "@(#)main.c	4.1  (ULTRIX)        7/17/90";
#endif lint

/*
 * SNMP Enterprise Specific Management Daemon
 *
 * This is an example of how to write a daemon that handle your special
 * Management Information Base not defined in RFC 1066.  This daemon
 * uses the library routines in libsnmp.a to exchange management
 * information of your "special" managed objects with the 'snmpd' daemon.
 */

#include <sys/ioctl.h>
#include <sys/time.h>
#include <errno.h>
#include <nlist.h>
#include <signal.h>
#include <syslog.h>

#include "defs.h"

/*
 * Global data structures
 */
int	regcnt = -1;		/* infinite mib registration */
int	vm_mib = 0;		/* register "vm" group */
int	disk_mib = 0;		/* register "disk" group */
int	up_mib = 0;		/* register "up" group */
int	notdaemon;		/* run interactive switch */
int	trace;			/* trace switch */
int	debug;			/* debug switch */
int	getmntcache;		/* get mount cache switch */
struct nlist nl[] = {
	{ "_sum" },		/* X_SUM */
	{ "_total" },		/* X_TOTAL */
	{ "_boottime" },	/* X_BOOTTIME */
	{ "" },
};
char	*community = "public";	/* community name */

void	quit();

extern	char *sys_errlist[];	/* for reading kernel memory */
extern	int errno;
extern	int timer();


main(argc, argv)
int	argc;
char	*argv[];
{
	objident	reqoid;
	objident	reqinst;
	int		status = GENSUC;

	/*
	 * Setup run-time option
	 */
	argv++, argc--;
	while (argc > 0 && **argv == '-') {
		if (strcmp(*argv, "-I") == 0) {		/* interactive */
			notdaemon++;
			argv++, argc--;
			continue;
		}
		if (strcmp(*argv, "-t") == 0) {		/* trace */
			trace++;
			argv++, argc--;
			continue;
		}
		if (strcmp(*argv, "-d") == 0) {		/* debug */
			debug++;
			argv++, argc--;
			continue;
		}
		if (strcmp(*argv, "-vm") == 0) {	/* vm only */
			vm_mib++;
			argv++, argc--;
			continue;
		}
		if (strcmp(*argv, "-disk") == 0) {	/* disk only */
			disk_mib++;
			argv++, argc--;
			continue;
		}
		if (strcmp(*argv, "-mnt") == 0) {	/* getmnt cache */
			getmntcache++;
			argv++, argc--;
			continue;
		}
		fprintf(stderr,
			"Usage: snmpextd [-I -d -t -vm -disk -mnt]\n");
		exit(1);
	}
	if (~(vm_mib & disk_mib)) vm_mib = disk_mib = 1;
	if (~(0xfffffffe | trace | debug | notdaemon)) {
		int t;
		if (fork()) exit(0);
		for (t = 0; t < 20; t++) (void) close(t);
	}

	/*
	 * Initialize the daemon.
	 */
	openlog("snmpextd", LOG_PID , 0);
	syslog(LOG_NOTICE, "Started");
	if (trace|debug|notdaemon) printf("Started snmpextd\n");

	/*
	 * Setup cleanup signal.
	 */
	(void) signal(SIGINT, quit);
	(void) signal(SIGTERM, quit);

	/*
	 * Initialize name list.
	 */
        nlist("/vmunix", nl);
        if (nl[0].n_type == 0) {
		syslog(LOG_ERR, "no /vmunix namelist\n");
		if (debug|notdaemon) printf("snmpextd: no /vmunix namelist\n");
                exit(1);
	}
	/*
	 * Register our MIB variables with snmpd.
	 */
	if (vm_mib)
		if (register_vm() == -1) {
			syslog(LOG_ERR, "vm registration failed");
			if (debug|notdaemon) printf("main: vm registration failed\n");
			snmpextexit(1);
		}
	if (disk_mib)
		if (register_disk() == -1) {
			syslog(LOG_ERR, "disk registration failed");
			if (debug|notdaemon) printf("main: disk registration failed\n");
			snmpextexit(1);
		}
	/*
	 * Get a request from snmpd.  If none, the library routine blocks.
	 * If got one, go and process it.  When done, get another request.
	 * At this level, this daemon never exits!
	 */
	while (1) {
		status = snmpextgetreq(&reqoid, &reqinst);
		if (status != GENSUC) {
			syslog(LOG_ERR, "main: snmpextgetreq failed: %d",
			       status);
			if (notdaemon) printf("main: snmpextgetreq failed: %d\n", status);
			continue;
		}
		status = procreq(&reqoid, &reqinst);
		if (status != 0) {
			syslog(LOG_ERR, "main: procreq failed: %d", status);
			if (notdaemon) printf("main: procreq failed: %d\n",
					      status);
			continue;
		}
	}
}

/*
 * This signal handler cleanups before exit.
 */
void	quit()
{
	if (debug|notdaemon) printf("quit: killed by a signal\n");
	syslog(LOG_ERR, "main: killed by a signal\n");
	snmpextexit(0);
}


/*
 * UTILITY ROUTINES:
 */
/*
 *	This procedure constructs the object indentifer into a given structure
 *	and register it snmpd.
 */
reg_oid(oidbuf, numoid, oidtyp)
u_long	*oidbuf;
short	numoid;
int	oidtyp;
{
	static struct snmpareg	reg;

	reg.oidtype = oidtyp;
	reg.oid.ncmp = numoid;
	bcopy(oidbuf, reg.oid.cmp, numoid*sizeof(u_long));
	if (snmpextregister(&reg, community) != GENSUC) {
		if (debug) {
			printf("reg_oid: register failed: ");
			proid(&reg.oid);
		}
		return(-1);
	}
	if (debug) {
		printf("MIB registered: ");
		proid(&reg.oid);
	}
	return(0);
}

/*
 * Check for end of get-next (i.e. no more instances).  If so, call
 * snmpexterror to respond to snmpd.
 */
endgetnxt(inst)
objident	*inst;
{
	if (inst->ncmp == 0) {
		if (trace|debug) {
			printf("MIB: no object instance after inst: ");
			proid(inst);
		}
		(void) snmpexterror(NOERR);
		return(0);
	}
	return(1);
}

/*
 * Print object identifier
 */
proid(oid)
objident	*oid;
{
	int	i;

	printf("OID(ncmp:%lu cmp:", oid->ncmp);
	for (i=0; i<oid->ncmp; i++)
		printf(" %lu", oid->cmp[i]);
	printf(")\n");
}
