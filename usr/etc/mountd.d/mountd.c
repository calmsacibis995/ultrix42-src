#ifndef lint
static	char	*sccsid = "@(#)mountd.c	4.7	(ULTRIX)	4/25/91";
#endif lint
/************************************************************************
 *									*
 *			Copyright (c) 1986-1990 by			*
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
 *	Portions of this software have been licensed to 
 *	Digital Equipment Company, Maynard, MA.
 *	Copyright (c) 1987 Sun Microsystems, Inc.  ALL RIGHTS RESERVED.
 */

/***********************************************************************
 *
 *		Modification History
 *
 * 02/11/91 lebel
 *	Fixed mounting /joe/../jim security hole.  Patched spots where
 *	mountd wasn't freeing memory.  Added ip addr cache to speed up
 *	automount -hosts, modified 'imposter' (now 'getclientsname').
 *
 * 11/15/90 lebel
 *		Fixed bug with non-existant pathname parsing in
 *		set_exports.  Added smarter rmtab updating, YP netgroup
 *		caching, and duplicate requests cache.
 *
 * 07/02/90 sue
 *		Made fix to have short host names in /etc/exports.
 *
 * 11/10/89 lebel
 *		Errors now go to syslog.  Showmounts can be done over tcp 
 *		(tcp can handle arbitrarily long exports lists).  The -d
 *		option implies the -i option and the -s option implies both
 *		-i and -d.  Mountd registers itself with the portmapper 
 *		before forking to prevent the case where nfsds start up
 *		before mountd has time to register itself.  Hostname 
 *		comparisons are case insensitive.  Duplicate detection
 *		is now done by inode # comparison (to detect links).
 *
 * 07/21/89 lebel
 *		Rewrite of mtd_mount() and set_exports() to allow options at
 *		the directory level.  The -n (no filehandle) exports option
 *		is no longer employed.  update_exportfsdata() keeps the new
 *		kernel exports list in sync with the mount daemon's.
 *
 * 05/25/89	Suzanne Logcher
 *		Removed code specific to /etc/svcorder.  File is now
 *		/etc/svc.conf.  Use libc version of bindup().
 *
 * 11/16/88	Suzanne Logcher
 *		Fixed de-referencing null pointer bugs in path_check()
 *		which caused mountd on PMAX to seg fault and core dump.
 *
 * 11/10/88	Suzanne Logcher
 *		Updated with V3.0 changes.
 *
 * 07/15/88	Suzanne Logcher
 *		Fixed QAR 64 bug to continue to parse a directory entry
 *		in /etc/exports after catching an improper setting of
 *		an option.  Force a showmount request to rebuild the
 *		export structure.  Changed usage of -i with bind.  The
 *		-d flag is for after checking the IP address, see if
 *		host is in the server domain.  The -s flag does the same
 *		check but checks if the host is in the server domain or
 *		a subdomain.
 *
 * 06/09/88	Suzanne Logcher
 *		Added v2.3 fix to not cleanup the export list of
 *		unexported filesystems and use that info to determine
 *		whether the getmount info on the filesystem has changed
 *		thus giving the mountd a "kick" to rebuild the export
 *		list.  This handles the case when the filesystems have
 *		changed and the /etc/exports file has NOT been modified
 *		thus causing the mountd's export list to be out-of-date.
 *		Upon a second failure with exit with EACESS.
 *
 * 04/24/88	Fred Glover
 *		Add BIND domain specification check for ipaddr_check 
 *
 * 03/01/88	Suzanne Logcher
 *		Rewrite of code.  Use getmountent to build exports
 *		list, and then read in /etc/exports and build matrix
 *		of exports with filesystems and directories.  Change
 *		code in mtd_mount() to parse new list structure.  Add a
 *		routine, flatten_exports(), to provide full information
 *		to an EXPORT request.  Linted as best as possible.
 *
 * 05/25/87	Joe Amato
 *		When looking to match the path requested to mount
 *		loop through exports list looking for an exact match
 *		if not found, loop through exports list again, but
 *		look for a match on a subset of the requested path
 *
 * 05/19/87	Joe Amato
 *		Moved some initialization code before fork.
 *		When looking for a machine in a long group list, only 
 *		call innetgr after searching complete list to avoid 
 *		unnecessary YP timeouts.
 *
 * 05/06/87	Suzanne Logcher
 *		Changed M_RONLY to M_EXRONLY
 *
 * 03/19/87	Suzanne Logcher
 *		Added code to setopt to parse options in /etc/exports
 *
 * 02/10/87	Suzanne Logcher
 *		Added code to allow exporting of directories
 *
 ***********************************************************************/

/* NFS mount server */

#include <ctype.h>
#include <sys/param.h>
#include <ufs/fs.h>
#include <rpc/rpc.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/file.h>
#include <sys/time.h>
#include <stdio.h>
#include <syslog.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/ioctl.h>
#include <sys/errno.h>
#include <sys/dir.h>
#include <nfs/nfs.h>
#include <rpcsvc/ypclnt.h>
#include <rpcsvc/yp_prot.h>
#ifdef DEBUG
#include "mount.h"
#else
#include <rpcsvc/mount.h>
#endif DEBUG
#include <netdb.h>
#include <sys/mount.h>
#include <nfs/nfs_clnt.h>
#include <nfs/vfs.h>

#include <strings.h>
#include <arpa/nameser.h>
#include <resolv.h>

#define	EXPORTS	"/etc/exports"
#define RMTAB	"/etc/rmtab"
#define MAXRMTABLINELEN         (MAXPATHLEN + MAXHOSTNAMELEN + 2)
#define	MAXLINE	2048
#define MAX_LEVELS  10
#define MAXADDRS 8

extern int errno;

int expand_ng();
int innetgr_cache();
int check_ng();
int getkeys();
int mnt();
char *exmalloc();
struct exports *newex();
struct groups *newgroup();
char *goto_char();
char *realpath();

/*
 * mountd's version of a "struct mountlist". It is the same except
 * for the added ml_pos field.
 */
struct mountdlist {
/* same as XDR mountlist */
        char *ml_name;
        char *ml_path;
        struct mountdlist *ml_nxt;
/* private to mountd */
        long ml_pos;            /* position of mount entry in RMTAB */
};

struct ng {             /* netgroup cache */
        long ng_stamp;
        char *ng_name;
        struct hlist *ng_hosts;
        struct ng *next_ng;
        struct ng *prev_ng;
};

struct hlist {
        char *hname;
        struct hlist *next_host;
};

/*
 * Mount Reply Cache - save replies, and check the cache to catch
 * retransmitted requests.
 */

/*
 *  RPC server duplicate transaction cache flag values
 */
#define DUP_BUSY        0x1     /* transaction in progress */
#define DUP_DONE        0x2     /* transaction was completed */
#define DUP_FAIL        0x4     /* transaction failed */

struct dupreq {
        struct in_addr  rc_addr;        /* client address */
        u_short         rc_flags;       /* DUP_BUSY, DUP_DONE, DUP_FAIL */
        dev_t           rc_dev;         /* device */
        u_long          rc_ino;         /* inode number */
        u_long          rc_gen;         /* generation number */
        struct dupreq   *rc_next;       /* linked list of all entries */
        struct dupreq   *rc_chain;      /* hash chain */
	char 		*rc_ancname;    /* name of requested path's most
					   closely related exported ancestor */
};

#define KEYHASH(addr,dev,ino,gen)       ((((addr) ^ (dev)) ^ (ino) ^ (gen)) % drhashszminus1)
#define KEYTOLIST(addr,dev,ino,gen)     ((struct dupreq *)(drhashtbl[KEYHASH(addr,dev,ino,gen)]))
#define REQTOLIST(dr)   KEYTOLIST((dr)->rc_addr.s_addr,(dr)->rc_dev,(dr)->rc_ino, (dr)->rc_gen)
 
/* routine to compare dup cache entries */
#define NOTDUP(dr, addr,dev,ino,gen) (dr->rc_addr.s_addr != addr || \
                            dr->rc_dev != dev || \
                            dr->rc_ino != ino || \
                            dr->rc_gen != gen)

/*
 * dupcache_max is the number of cached items.  It is set
 * based on "system size". It should be large enough to hold
 * transaction history long enough so that a given entry is still
 * around for a few retransmissions of that transaction.
 */
#define MINDUPREQS      1024
#define MAXDUPREQS      4096
struct dupreq **drhashtbl; /* array of heads of hash lists */
int drhashsz;              /* number of hash lists */
int drhashszminus1;        /* hash modulus */
int dupcache_max;
struct dupreq *dupreqcache, *drmru;
struct dupreq *dupcache_check();

struct ng *nglist = NULL;  /* head of netgroup's cache */
char *ng_names[100];
int num_ngs;
static struct mountdlist *mountlist;
int rmtab_load();
long rmtab_insert();
 
char myname[BUFSIZ];
char mydomain[MAXNAMLEN+1];
char *pgmname;
char *exportfile = EXPORTS;
struct stat exportstat;
struct exports *exports = NULL;
struct exports *flatexports = NULL;
int nfs_portmon = 0;
int ipaddr_check = 0;
int domain_check = 0;
int subdomain_check = 0;

struct timeval rmtab_written, now;
struct timezone tz;
/*
 * resync rmtab no more often than at 30 minute intervals
 * sole purpose is to get rid of commented out (unmounted) entries
 */
int   rmtab_sync=1800;

/*
 * Cached netgroups are assumed correct for at least 15 minutes
 */
int ngtimeout = 900;
 
main(argc, argv)
char	*argv[];
{
	SVCXPRT *transp;
	char *strchr ();
	int fd; /* open fd of rmtab */

	pgmname = argv[0];

	/*
	 * must be superuser to run 
	 */
	if (geteuid() != 0){
		(void) fprintf(stderr, "%s:  must be super user\n", pgmname);
		(void) fflush(stderr);
		exit(1);
	}
	while (--argc > 0 ) {
		*++argv;
		if ((*argv)[0] != '-' || strlen(*argv) != 2) {
			usage();
		}

		switch ((*argv)[1]) {
		case 'i':
			ipaddr_check++;
			break;
		case 'd':
			ipaddr_check++;
			domain_check++;
			break;
		case 's':
			ipaddr_check++;
			subdomain_check++;
			break;
		default:
			usage();
		}
	}
	if (openlog("mountd", LOG_PID) < 0)
		fprintf(stderr, "mountd: openlog failed\n");
	syslog(LOG_ERR, "startup");

	/*
	 * Initalize the world
	 */
	if(gethostname(myname, sizeof(myname)) < 0){            /* jaa */
		syslog(LOG_ERR, "gethostname: %m");
		exit(1);
	}
	if(getdomainname(mydomain, sizeof(mydomain)) < 0){      /* jaa */
		syslog(LOG_ERR, "getdomainname: %m");
		exit(1);
	}

	/*
	 * Read rmtab and exports files, build netgroups cache
	 */
	fd = rmtab_load();
	if (mydomain[0] != NULL)
        	build_ngcache();
#ifdef DEBUG
        print_ngcache();
#endif DEBUG
	set_exports();
#ifdef DEBUG
	(void) fprintf(stderr, "*** finished loading export list ***\n");
#endif DEBUG
	gettimeofday(&rmtab_written, &tz);

	/*
	 * Remove this chunk of code if we ever run under inetd.
         * (Make sure rmtab - fd - does not get closed)
	 * Also remove the mtd_abort() routine if running under inetd.
	 */
	{
		int s, t;
		struct sockaddr_in addr;
		int len = sizeof(struct sockaddr_in);
		int pid;

#ifndef DEBUG
		for (t = 0; t < 20; t++)
			if (t != fd)
				(void) close(t);
	 	open("/", 0);
	 	dup2(0, 1);
	 	dup2(0, 2);
#endif DEBUG
		if ((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
			syslog(LOG_ERR, "socket: %m");
			exit(1);
		}
		if (bind(s, &addr, sizeof(addr)) < 0) {
			syslog(LOG_ERR, "bind: %m");
			exit(1);
		}
		if (getsockname(s, &addr, &len) != 0) {
			syslog(LOG_ERR, "getsockname: %m");
			(void) close(s);
			exit(1);
		}
		pmap_unset(MOUNTPROG, MOUNTVERS);
		/* 
		 * register with portmapper if not started from inetd
		 */
		pmap_set(MOUNTPROG, MOUNTVERS, IPPROTO_UDP,
		    ntohs(addr.sin_port));
		if (dup2(s, 0) < 0) {
			syslog(LOG_ERR, "dup2: %m");
			exit(1);
		}
#ifndef DEBUG
		pid = fork();
		if (pid < 0) {
			syslog(LOG_ERR, "Cannot fork: %m");
			exit(1);
		}
		if (pid != 0)
			exit(0);
#endif DEBUG
	}

	/* End chunk to remove if running under inetd. */

	/*
	 * Create UDP service
	 */
	if ((transp = svcudp_create(0)) == NULL) {
		syslog(LOG_ERR, "couldn't create UDP transport");
		exit(1);
	}
	if (!svc_register(transp, MOUNTPROG, MOUNTVERS, mnt, 0)) {
		syslog(LOG_ERR, "couldn't register MOUNTPROG");
		exit(1);
	}

	/*
	 * Create TCP service
	 */
	if ((transp = svctcp_create(RPC_ANYSOCK, 0, 0)) == NULL) {
		syslog(LOG_ERR, "couldn't create TCP transport");
		exit(1);
	}
	if (!svc_register(transp, MOUNTPROG, MOUNTVERS, mnt, IPPROTO_TCP)) {
		syslog(LOG_ERR, "couldn't register MOUNTPROG");
		exit(1);
	}

	dupcache_init();  /* dup req cache */
	/*
	 * Start serving
	 */

	while(1) {
		svc_run();
		syslog(LOG_ERR, "Error: svc_run shouldn't have returned");
		mtd_abort();
	}
}

/*
 * Server procedure switch routine
 */
mnt(rqstp, transp)
	struct svc_req *rqstp;
	SVCXPRT *transp;
{
#ifdef DEBUG
	char *machine;

	machine = ((struct authunix_parms *) rqstp->rq_clntcred)->aup_machname;
#endif DEBUG
	switch(rqstp->rq_proc) {
		case NULLPROC:
			if (!svc_sendreply(transp, xdr_void, NULL)) {
				syslog(LOG_ERR, "couldn't reply to NULL rpc call");
				mtd_abort();
			}
			return;
		case MOUNTPROC_MNT:
#ifdef DEBUG
			(void) fprintf(stderr, "about to do a mount from %s\n", machine);
#endif
			set_exports();
			(void) mtd_mount(rqstp, transp);
			return;
		case MOUNTPROC_DUMP:
#ifdef DEBUG
			(void) fprintf(stderr, "about to do a dump from %s\n", machine);
#endif
			if (!svc_sendreply(transp, xdr_mountlist, &mountlist)) {
				syslog(LOG_ERR, "couldn't reply to DUMP rpc call");
				mtd_abort();
			}
			return;
		case MOUNTPROC_UMNT:
#ifdef DEBUG
			(void) fprintf(stderr, "about to do an unmount from %s\n", machine);
#endif
			(void) mtd_umount(rqstp, transp);
			return;
		case MOUNTPROC_UMNTALL:
#ifdef DEBUG
			(void) fprintf(stderr, "about to do an unmountall from %s\n", machine);
#endif
			mtd_umountall(rqstp, transp);
			return;
		case MOUNTPROC_EXPORT:
		case MOUNTPROC_EXPORTALL:
#ifdef DEBUG
			(void) fprintf(stderr, "about to do an export from %s\n", machine);
#endif
			/*
			 * Make saved /etc/exports time older to force
			 * a re-build of the in core structure.
			 */
			exportstat.st_mtime--;
			set_exports();
			mtd_export(transp);
			return;
		default:
			svcerr_noproc(transp);
			return;
	}
}

struct hostent *
getclientsname(rqstp,transp)
	struct svc_req *rqstp;
	SVCXPRT *transp;
{
	struct in_addr claim;
	struct sockaddr_in actual;
	struct hostent *hp, *chp;
	static struct hostent h;
	static struct in_addr prev;
	static char *null_alias;
	char *machine;
	char *cp;
	char *strchr ();
	int i;
		
	if (rqstp->rq_cred.oa_flavor != AUTH_UNIX) {
		return(NULL);
	}
	actual = *svc_getcaller(transp);
	if (nfs_portmon) {
		if (ntohs(actual.sin_port) >= IPPORT_RESERVED) {
			return(NULL);
		}
	}

        /*
         * Don't use the unix credentials to get the machine name,
         * instead use the source IP address.  Use cached hostent
         * if previous request was from same client.
         */
        if (bcmp(&actual.sin_addr, &prev, sizeof(struct in_addr)) == 0) {
#ifdef DEBUG
                (void)fprintf(stderr, "Found addr in hostent cache! \n");
#endif DEBUG
                return (&h);
	}

        hp = gethostbyaddr((char *) &actual.sin_addr, sizeof(actual.sin_addr),
                           AF_INET);
        if (hp == NULL) {
		if (ipaddr_check) {
                	svcerr_auth(transp, AUTH_BADCRED);
                	return(NULL);
		} /* else dummy one up */
		h.h_name = (char *)inet_ntoa(actual.sin_addr);
#ifdef DEBUG
                (void)fprintf(stderr, "Dummy hostent name %s\n", h.h_name);
#endif DEBUG
                h.h_aliases = &null_alias;
                h.h_addrtype = AF_INET;
                h.h_length = sizeof (u_long);
                hp = &h;
        } else {
        	bcopy(hp, &h, sizeof(struct hostent));
	}

        prev = actual.sin_addr;

#ifdef DEBUG
	(void)fprintf(stderr, "getclientsname %s\n", hp->h_name);
#endif DEBUG

	/*
	 *	If domain_check or subdomain_check is set and
	 *	if BIND enabled, check the requester's domain spec
	 *	to be sure it is local.  Check if:
	 *		1) domain spec matches that of local host or
	 *		2) host name is unqualified (local)
	 */
	if ((domain_check || subdomain_check) && (bindup()) != NULL) {
                machine = ((struct authunix_parms *)
                        rqstp->rq_clntcred)->aup_machname;
                chp = gethostbyname(machine);
                if (chp == NULL) {
                        svcerr_auth(transp,AUTH_BADCRED);
                        return(NULL);
                }
		for (i=0; i<MAXADDRS; i++) {
			if (chp->h_addr_list[i] == NULL)
				return(NULL);
                	bcopy(chp->h_addr_list[i], &claim, sizeof(struct in_addr));
                	if (actual.sin_addr.s_addr == claim.s_addr)
				break;  /* not an imposter */
			if (i == (MAXADDRS - 1))
				return(NULL);
                }

		if ((cp = strchr (hp->h_name, '.')) != NULL) {

			/* qualified name */
			if (strcasecmp (cp+1, _res.defdname) != NULL) {
				/*
				 * Not in local domain, check  
				 * if in subdomain
				 */
				if (subdomain_check) {
#ifdef DEBUG
					(void) fprintf (stderr, "mountd: client, %s not local domain member, check if %s is a subdomain of %s\n", hp->h_name, cp+1, _res.defdname);
#endif DEBUG
					if ((strfind (cp+1, _res.defdname)) == NULL) {
						syslog(LOG_ERR, "warning: (u)mount attempt from client %s, not a local subdomain member", hp->h_name);
						return (NULL);
					}
				}
				else {
					syslog(LOG_ERR, "warning: (u)mount attempt from client %s, not a local domain member", hp->h_name);
					return(NULL);
				}
			}
		}
	}
#ifdef DEBUG
	(void) fprintf(stderr, "%s: end ipaddr_check successful\n", pgmname);
#endif DEBUG
	return(hp);
}

/*
 * Check mount requests, add to mounted list if ok
 */
mtd_mount(rqstp, transp)
	struct svc_req *rqstp;
	SVCXPRT *transp;
{
	fhandle_t fh;
	struct fhstatus fhs;
	char *path;
	int fd, anc_fd;
	struct mountdlist *ml;
	struct stat statbuf;
	struct exports *ex, *ex2, *ancestors[MAX_LEVELS];
	struct exports *saveex = NULL;
	int found=0, success=0, bestposs=1, index, i;
	int first = 1;
	struct dupreq *dr;
	char *anc_name;
	char rpath[MAXPATHLEN];
	struct hostent *client;
	struct sockaddr_in addr;

        if ((client = getclientsname(rqstp,transp))== NULL) {
                svcerr_weakauth(transp);
                return;
        }
	path = NULL;
	if (!svc_getargs(transp, xdr_path, &path)) {
		svcerr_decode(transp);
		mtd_abort();
		return;
	}
#ifdef DEBUG
	(void) fprintf(stderr, "path is %s\n", path);
#endif

	if ((fd = open(path, O_RDONLY, 0)) < 0) {
		fhs.fhs_status = errno;
		perror("mountd: open");
		goto done;
	}
	if (fstat(fd, &statbuf) < 0) {
		fhs.fhs_status = errno;
		perror("mountd: stat");
		(void) close(fd);
		goto done;
	}

        /*
         * Now, before we spend a lot of time looking through the
         * exports list, lets check to see if we have already handed out
         * a filehandle to this requestor. . .
         */
	addr = *svc_getcaller(transp);
        if (dr = dupcache_check(addr.sin_addr, statbuf.st_dev, statbuf.st_ino,
                        statbuf.st_gennum)) {
		if ((anc_fd = open(dr->rc_ancname, O_RDONLY, 0)) < 0) {
			fhs.fhs_status = errno;
			perror("mountd: open");
			goto done;
		}
		if (nfs_getfh(fd, anc_fd, &fh) < 0) {
			fhs.fhs_status = errno;
			syslog(LOG_ERR, "nfs_getfh: %m");
			goto done;
		}
		fhs.fhs_status = 0;
                fhs.fhs_fh = fh;
		(void)close(anc_fd);
                goto done;
        }

	/*
	 * Expand path into its canonical form, resolving symbolic link
	 * and ../ components. 
	 */
	if (realpath(path, rpath) == NULL) {
#ifdef DEBUG
		perror("mountd");
		(void)fprintf(stderr, "Realpath failed on %s\n", path);
#endif DEBUG
		fhs.fhs_status = EACCES;
		goto done;
	}
	/*
	 * Search exports list for a directory with an entry with the
	 * same dev_t number.  Search down that dev's list looking for 
	 * an exact match.  If none found, look for entries that are
	 * an ancestor of the requested path.  Check the access on the
	 * closest match.  If access is not allowed, check the next
	 * closest, ... until either no more exported ancestors exist
	 * or an ancestor is found with allowable access for the client.
	 * The export options used will be those on the ancestor found.
	 */
loop_search:
	for (ex = exports; ex != NULL; ex = ex->ex_devptr) {
#ifdef DEBUG
		(void) fprintf(stderr, "checking %o for %o\n", ex->ex_dev, statbuf.st_dev);
#endif
		if (ex->ex_dev == statbuf.st_dev) {
			for (ex2=ex; ex2 != NULL; ex2 = ex2->ex_next) {
#ifdef DEBUG
				(void) fprintf(stderr, "checking %s with %s for exact match dir\n", ex2->ex_name, rpath);
#endif
				if (strcmp(ex2->ex_name, rpath) == NULL) {
#ifdef DEBUG
					(void) fprintf(stderr, "got exact match dir\n");
#endif
					if (ex2->ex_groups == NULL) {
						success = 1;
						break;
					}
					else {
						if ((check_groups(ex2->ex_groups, client->h_name))!=NULL) {
							success = 1;
							break;
						}
						else {
							/*
							 * Exact match found but requester
							 * is not an allowable host.  Break
							 * to search for an ancestor match.
							 */
							saveex = ex;
							break;
						}
					}
				}
			}
			if (success) {
				if ((anc_fd = open(ex2->ex_name, O_RDONLY, 0)) < 0) {
					fhs.fhs_status = errno;
					perror("mountd: open");
					goto done;
				}
				anc_name = ex2->ex_name;
				goto hit;
			}
			if (saveex != NULL)
				break;
			if (ex2 == NULL) {
				/*
				 * Exact matches failed.  
				 * Break to best match search.
				 */
#ifdef DEBUG
				(void) fprintf(stderr,"exact match for %s failed\n", rpath);
#endif
				saveex = ex;
				break;
			}
		}
	}
	if (!saveex) {
		if (first) {
			first = 0;
			exportstat.st_mtime--;
			set_exports();
			goto loop_search;
		}
		else {
#ifdef DEBUG
			(void) fprintf(stderr, "%s: filesystem %s not found\n", pgmname, rpath);
#endif DEBUG
			fhs.fhs_status = EACCES;
			goto done;
		}
	}

	/*
	 * Using pointer to directory exports record with same
	 * dev_t number, try to find best match.
	 */
	ex = saveex;
	if (ex->ex_dev == statbuf.st_dev) {
		/*
		 *  initialize ancestors
		 */
		for (i=0; i< MAX_LEVELS; i++)
			ancestors[i] = NULL;
		for (ex2 = ex; ex2 != NULL; ex2 = ex2->ex_next) {
			if ((strcmp(ex2->ex_name, rpath) != NULL) && (index = path_check(ex2->ex_name, rpath)) > bestposs) {
#ifdef DEBUG
				(void) fprintf(stderr, "ancestor: %s found, level:%d is > bestposs\n", ex2->ex_name, index);
#endif
				found = 1;
				ancestors[index-2] = ex2;
			}
			else if (index == bestposs) {
#ifdef DEBUG
				(void) fprintf(stderr, "best match %s found, checking access\n", ex2->ex_name);
#endif
				if (ex2->ex_groups == NULL) {
					success = 1;
					break;
				}
				else {
					if ((check_groups(ex2->ex_groups, client->h_name)) != NULL) {
						success = 1;
						break;
					}
					else
						++bestposs;
				}
			}
		}
		if (success) {
			if ((anc_fd = open(ex2->ex_name, O_RDONLY, 0)) < 0) {
				fhs.fhs_status = errno;
				perror("mountd: open");
				goto done;
			}
			anc_name = ex2->ex_name;
			goto hit;
		}
		if (found) {
			for (i= ((bestposs==1)? 0:bestposs-2); i< MAX_LEVELS-1; i++)
				if (ancestors[i] != NULL) {
#ifdef DEBUG
					(void) fprintf(stderr, "Checking ancestor: %s for allowable access\n", ancestors[i]->ex_name); 
#endif DEBUG
					if (ancestors[i]->ex_groups == NULL) {
						success = 1;
						break;
					}
					else {
						if ((check_groups(ancestors[i]->ex_groups, client->h_name)) != NULL) {
							success = 1;
							break;
						}
					}
				}
			if (success) {
				if ((anc_fd = open(ancestors[i]->ex_name, O_RDONLY, 0)) < 0) {
					fhs.fhs_status = errno;
					perror("mountd: open");
					goto done;
				}
				anc_name = ancestors[i]->ex_name;
				goto hit;
			}
		}
	}
	fhs.fhs_status = EACCES;
	goto done;
  hit:
	if (nfs_getfh(fd, anc_fd, &fh) < 0) {
		fhs.fhs_status = errno;
		syslog(LOG_ERR, "nfs_getfh: %m");
		(void)close(anc_fd);
		goto done;
	}
	fhs.fhs_status = 0;
	fhs.fhs_fh = fh;
	(void)close(anc_fd);
        /*
         * Now that we have a "New" mount request, lets tuck it into the
         * dupreq cache so that if the client retransmits, we can reply
	 * with the filehandle immediately.
         */
        dupcache_enter(addr.sin_addr, statbuf.st_dev, statbuf.st_ino,
                    statbuf.st_gennum, anc_name);
done:
#ifdef DEBUG
        (void) fprintf(stderr, "*** ng cache and export list ***\n");
        print_ngcache();
        print_exports(exports);
#endif DEBUG
	(void) close(fd);

	if (!svc_sendreply(transp, xdr_fhstatus, &fhs)) {
		syslog(LOG_ERR, "couldn't reply to MOUNT rpc call");
		mtd_abort();
	} 
	else  {
		for (ml = mountlist; ml != NULL; ml = ml->ml_nxt) {
			if (strcmp(ml->ml_path, rpath) == 0 &&
		    	    strcmp(ml->ml_name, client->h_name) == 0)
				break;
		}
		if (ml == NULL) {
			ml = (struct mountdlist *)exmalloc(sizeof(struct mountdlist));
			ml->ml_path = (char *)exmalloc(strlen(rpath) + 1);
			(void) strcpy(ml->ml_path, rpath);
			ml->ml_name = (char *)exmalloc(strlen(client->h_name) + 1);
			(void) strcpy(ml->ml_name, client->h_name);
			ml->ml_nxt = mountlist;
			mountlist = ml;
			ml->ml_pos = rmtab_insert(client->h_name, rpath);
		}
	}
	svc_freeargs(transp, xdr_path, &path);
}

/*
 * Check if machine is in groups structure.  If not, call YP.
 */
check_groups(headgl, mach)
	struct groups *headgl;
	char *mach;
{
	struct groups *gl;
	int length;

	/* 
	 * first check for exact match on machine name
	 */
	for (gl = headgl; gl != NULL; gl = gl->g_next){ 
#ifdef DEBUG
		(void) fprintf(stderr, "checking machines: %s for %s\n", gl->g_name, mach);
#endif
		if (strcasecmp(gl->g_name, mach) == 0) {
			gl->g_hbit = 1;
			return(TRUE);
		}
		else if (((length = local_hostname_length(mach)) != NULL) && 
			  (strlen(gl->g_name) == length) &&
			  (strncasecmp(gl->g_name, mach, length) == 0)) {
			gl->g_hbit = 1;
			return(TRUE);
		}
	}
	/*
	 * now check netgroups
	 */
	for (gl = headgl; gl != NULL; gl = gl->g_next){ 
#ifdef DEBUG
		(void) fprintf(stderr, "checking innetgr: %s for %s\n", gl->g_name, mach);
#endif
		if (gl->g_hbit)
			continue;
		if (innetgr_cache (gl->g_name, mach, &gl->g_hbit)) {
			return(TRUE);
		}
	}
	return(FALSE);
}

/*
 * Check the pathnames of the export name and the desired mount
 * pathname.
 */
path_check(list_name, name)
 	char *list_name, *name;
{
 	char *ch1, *ch2;
 	char *ch11, *ch22;
 	char s1, s2;
 	int done, level=0;
 	char *index(); 
 
#ifdef DEBUG
	(void) fprintf(stderr, "path_check %s with %s\n", list_name, name);
#endif
 	if (!strcmp(list_name, "/"))
 		return(TRUE);
 	ch1 = list_name;
 	ch2 = name;
 	done = 0;
 	while (!done) {
 		if ((ch1 = goto_char(ch1)) != NULL)
 			ch11 = index(ch1, '/');
 		if ((ch2 = goto_char(ch2)) != NULL)
 			ch22 = index(ch2, '/');
 		if (ch11 && *ch11 != '\0') {
 			s1 = *ch11;
 			*ch11 = '\0';
 			ch11++;
 		}
 		if (ch22 && *ch22 != '\0') {
 			s2 = *ch22;
 			*ch22 = '\0';
 			ch22++;
 		}
 		if ((ch1 && ch2) && !strcmp(ch1, ch2)) {
 			if (ch11 && *ch11 != '\0') {
 				*--ch11 = s1;
 				ch11++;
 			}
 			if (ch22 && *ch22 != '\0') {
 				*--ch22 = s2;
 				ch22++;
 			}
 			ch1 = ch11;
 			ch2 = ch22;
 			if ((ch11 && *ch11 == '\0') || !ch11) {
				/*
				 * count number of slashes remaining in ancestor
				 */
				if (ch2) {
					for (--ch2; ch2 && *ch2!='\0'; ch2++)
						if (*ch2 == '/') level++;
					if (*--ch2 == '/')
						level--;
				}
 				return(level);
			}
 			if ((ch11 && *ch11 != '\0') && ((ch22 && *ch22 == '\0') || !ch22))
 				done = 1;
 		}
 		else {
 			if (ch11 && *ch11 != '\0')
 				*--ch11 = s1;
 			if (ch22 && *ch22 != '\0')
 				*--ch22 = s2;
 			done = 1;
 		}
 	}
	return(level);
}

/*
 * Skip over slashes (/) and go to first character
 */
char *
goto_char(ch)
 	char *ch;
{
 	for (;ch && *ch == '/'; ch++)
 		;
	if (ch)
 		return(ch);
	else
		return(NULL);
}
 
/*
 * Remove an entry from mounted list
 */
mtd_umount(rqstp, transp)
	struct svc_req *rqstp;
	SVCXPRT *transp;
{
	char *path;
	struct mountdlist *ml, *oldml;
	struct hostent *client;
	long pos = -1;

        if ((client = getclientsname(rqstp,transp))== NULL) {
                svcerr_weakauth(transp);
                return;
        }
	path = NULL;
	if (!svc_getargs(transp, xdr_path, &path)) {
		svcerr_decode(transp);
		mtd_abort();
		return;
	}
#ifdef DEBUG
	(void) fprintf(stderr, "umounting %s for %s\n", path, client->h_name);
#endif
	oldml = mountlist;
	for (ml = mountlist; ml != NULL;
	    oldml = ml, ml = ml->ml_nxt) {
		if (strcmp(ml->ml_path, path) == 0 &&
		    strcmp(ml->ml_name, client->h_name) == 0) {
			if (ml == mountlist)
				mountlist = ml->ml_nxt;
			else
				oldml->ml_nxt = ml->ml_nxt;
#ifdef DEBUG
			(void) fprintf(stderr, "freeing %s\n", path);
#endif
			pos = ml->ml_pos;
			free(ml->ml_path);
			free(ml->ml_name);
			free((char *)ml);
			break;
		    }
	}
	if (!svc_sendreply(transp,xdr_void, NULL)) {
		syslog(LOG_ERR, "couldn't reply to UMOUNT rpc call");
		mtd_abort();
	} else {
		gettimeofday(&now, &tz);
                if ((now.tv_sec - rmtab_written.tv_sec) > rmtab_sync) {
                        dumptofile();
                        rmtab_written = now;
                }
                else if (pos >= 0) {
                        rmtab_delete(pos);
                }
	}
	svc_freeargs(transp, xdr_path, &path);
}

/*
 * Remove all entries for one machine from mounted list
 */
mtd_umountall(rqstp, transp)
	struct svc_req *rqstp;
	SVCXPRT *transp;
{
	char *machine;
	struct mountdlist *ml, *oldml;

	if (!svc_getargs(transp, xdr_void, NULL)) {
		svcerr_decode(transp);
		return;
	}
	/*
	 * We assume that this call is asynchronous and made via the 
	 * portmapper callit routine.  Therefore return control immediately.
	 * The error causes the portmapper to remain silent, as opposed to
	 * every machine on the net blasting the requester with a response.
	 */
	svcerr_systemerr(transp);
        if (rqstp->rq_cred.oa_flavor == AUTH_UNIX) {
                machine =
                   ((struct authunix_parms *)rqstp->rq_clntcred)->aup_machname;
        }
        else
                return;

	oldml = mountlist;
	for (ml = mountlist; ml != NULL; ml = ml->ml_nxt) {
		if (strncmp(ml->ml_name, machine, sizeof(machine)) == 0) {
#ifdef DEBUG
			(void) fprintf(stderr, "got a hit\n");
#endif
			if (ml == mountlist) {
				mountlist = ml->ml_nxt;
				oldml = mountlist;
			}
			else
				oldml->ml_nxt = ml->ml_nxt;
			rmtab_delete(ml->ml_pos);
			free(ml->ml_path);
			free(ml->ml_name);
			free((char *)ml);
		}
		else
			oldml = ml;
	}
	svc_freeargs(transp, xdr_void, NULL);
}

FILE *f;
/*
 * Save current mount state info so we
 * can attempt to recover in case of a crash.
 */
dumptofile()
{
	static char *t1 = "/etc/zzXXXXXX";
	static char *t2 = "/etc/zzXXXXXX";
	FILE *fp;
	struct mountdlist *ml;
	char *mktemp();
	int mf;
	
	(void) fclose(f);
	(void) strcpy(t2, t1);
	t2 = mktemp(t2);
	if ((mf = creat(t2, 0644)) < 0) {
		syslog(LOG_ERR, "creat: %m, cannot dump mountlist to %s", RMTAB);
		return;
	}
	if ((fp = fdopen(mf, "w")) == NULL) {
		syslog(LOG_ERR, "fdopen: %m, cannot dump mountlist to %s", RMTAB);
		return;
	}
	for (ml = mountlist; ml != NULL; ml = ml->ml_nxt) {
		ml->ml_pos = ftell(fp);
		(void) fprintf(fp, "%s:%s\n", ml->ml_name, ml->ml_path);
	}
	if (rename(t2, RMTAB) < 0) 
		syslog(LOG_ERR, "rename: %m, cannot dump mountlist to %s", RMTAB);
	(void) fclose(fp);

	 f = fopen(RMTAB, "r+");
}

/*
 * send current export list
 */
mtd_export(transp)
	SVCXPRT *transp;
{
	struct exports *ex;

	if (!svc_getargs(transp, xdr_void, NULL)) {
		svcerr_decode(transp);
		mtd_abort();
	} else {
		/*
		 * Send exported request the list of flattened exports
		 * to conform with other os's.  Otherwise, all 
		 * exported directories would not be seen.
		 */
		ex = flatexports;
		if (!svc_sendreply(transp, xdr_exports, &ex)) {
			syslog(LOG_ERR, "couldn't reply to EXPORT rpc call");
			mtd_abort();
		}
	}
}

/*
 * Parse exports file.  If this is the first call or the file exportfile
 * has changed, it is opened and parsed to create an exports list.
 * File should look like:
 *
 * pathname [-r=#] [-o] [name1 name2 ... namen]
 *   or
 * #anything
 *
 * pathname:	name of a mounted local file system
 *		name of a directory of a mounted local filesystem
 * optional options
 *	 -r=#	for entire filesystem, root maps to #
 *	 -o	entire filesystem exported readonly
 * name:	netgroup or host name or a list of whitespace separated
 *			names (optional, no names implies everyone)
 *
 * A '#' anywhere in the line marks a comment to the end of that line
 *
 * NOTE: a non-white character in column 1 indicates a new export
 *	specification.
 */
set_exports()
{
	int bol;	/* begining of line */
	int eof;	/* end of file */
	int opt;	/* beginning of option */
	struct exports *ex, *ex2;
	int newdev();
	char ch;
	char *str;
	char *l;
	char line[MAXLINE];	/* current line */
	struct stat statb;
	FILE *fp;
	int isdev;
	int found;
	int bad_entry=0;  /* true if current entry does not exist or is a duplicate */

	if (stat(exportfile, &statb) < 0) {
#ifdef DEBUG
		(void) fprintf(stderr, "%s: stat failed", pgmname);
		perror("mountd: exportfile");
#endif
		freeex(exports);
		exports = NULL;
		freeex(flatexports);
		flatexports = NULL;
		return;
	}
	if (exportstat.st_mtime == statb.st_mtime) {
		return;
	}
	exportstat = statb;

	if ((fp = fopen(exportfile, "r")) == NULL) {
		syslog(LOG_ERR, "fopen: %m");
		freeex(exports);
		exports = NULL;
		freeex(flatexports);
		flatexports = NULL;
		return;
	}

	dupcache_inval();               /* invalidate the dup req cache */
	freeex(exports);
	exports = NULL;
	freeex(flatexports);
	flatexports = NULL;
	eof = 0;

	l = line;
	*l = '\0';
	while (!eof) {
		switch (*l) {
		case '\0':
		case '\n':
			/*
			 * End of line, read next line and set state vars
			 */
			if (fgets(line, MAXLINE, fp) == NULL) {
				eof = 1;
			} else {
				bol = 1;
				opt = 0;
				l = line;
			}
			break;
		case ' ':
		case '	':
			/*
			 * If this is the continuation of a bad entry, skip the line
			 */
			if (bad_entry) {
				*l = '\0';
				break;
			}
			/*
			 * White space, skip to first non-white
			 */
			while (*l == ' ' || *l == '	') {
				l++;
			}
			bol = 0;
			break;
		case '#':
			/*
			 * Comment, skip to end of line.
			 */
			*l = '\0';
			break;
		case '-':
			/*
			 * option of the form: -option=value or -option
			 */
			if (bol) {
				syslog(LOG_ERR, "Cannot parse '%s'", l);
				*l = '\0';
				break;
			}
			opt = 1;
			l++;
			break;
		default:
			/*
			 * normal character: if col one get dir else name or opt
			 */
			str = l;
			while (*l != ' ' && *l != '	' &&
			     *l != '\0' && *l != '\n') {
				l++;
			}
			ch = *l;
			*l = '\0';
			if (bol) {
				/*
				 * Logic for building export list:
				 *  The list is organized by dev horizontally,
				 *  all entries of the same dev hang off vertically.
				 *  The top dev entry is just the first one read.
				 *
				 *  Try to find an exports list entry with the
				 *  same dev number.  If not found, this entry
				 *  represents a new top entry.  Connect its
				 *  devptr to the front of the exports list.
				 *  If a dev match was found, tack this entry on
				 *  to the "next" pointer of the top entry for this 
				 *  dev.  Always check if this is a duplicate entry.
				 */
#ifdef DEBUG_FULL
				(void) fprintf(stderr, "--- next /etc/exports entry to add is %s ---\n", str);
#endif DEBUG_FULL
				bad_entry = 0;  /* this is a new export entry */
				if (stat(str, &statb) < 0) {
					syslog(LOG_ERR, "stat: %m, Cannot stat %s", str);
					bad_entry = 1;
					break;
				}
				for (ex = exports; ex != NULL; ex = ex->ex_devptr) 
					if (ex->ex_dev == statb.st_dev) break;

				if (ex == NULL) {
					isdev = TRUE;
#ifdef DEBUG_FULL
					(void) fprintf(stderr, "adding new export %s\n", str);
#endif DEBUG_FULL
					newdev(str, statb.st_dev, statb.st_ino, statb.st_gennum);
					ex = exports;
				}
				else {
					found = FALSE;
					for (ex2=ex; ex2 != NULL; ex2 = ex2->ex_next)
						if (ex2->ex_ino == statb.st_ino) {
							syslog(LOG_ERR, "Duplicate directory entry for %s - duplicate ignored", str); 
							found = TRUE;
							bad_entry = 1;
							break;
						}
					if (!found) {
						isdev = FALSE;
						/*
					  	   initialize rootmap to nobody and flags to 0
						*/
#ifdef DEBUG_FULL
						(void) fprintf(stderr, "adding new export %s\n", str);
#endif DEBUG_FULL
						ex->ex_next = newex(str, ex->ex_next, statb.st_dev, statb.st_ino, statb.st_gennum, -2, 0);
					}
					else 
						break;  /* skip rest of entry */
				}
			}
			else {
				if (opt) {
					opt = 0;
					if (isdev) 
						setopt(str, ex);
					else 
						setopt(str, ex->ex_next);
				}
				else {
					if (!isdev) {
#ifdef DEBUG_FULL
						(void) fprintf(stderr, "adding new groups %s to %s\n", str, ex->ex_next->ex_name);
#endif DEBUG_FULL
						ex->ex_next->ex_groups = newgroup(str, ex->ex_next->ex_groups);
					}
					else {
#ifdef DEBUG_FULL
						(void) fprintf(stderr, "adding new groups %s to %s\n", str, ex->ex_name);
#endif DEBUG_FULL
						ex->ex_groups = newgroup(str, ex->ex_groups);
					}
				}
			}
			*l = ch;
			bol = 0;
			break;
		}
	}
	(void) fclose(fp);
#ifdef DEBUG
	(void) fprintf(stderr, "*** export list final results ***\n");
	print_exports(exports);
#endif DEBUG

	/*
	 * Update the kernel's exportfsdata list so that it is in sync with
	 * the exports file and the mountd's list.
	 */

	update_exportfsdata();
	flatten_exports();
#ifdef DEBUG_FULL
	(void) fprintf(stderr, "*** export list final results flattened ***\n");
	print_exports(flatexports);
#endif DEBUG_FULL
	return;
}

/*
 * Make exportfs calls necessary to update kernel's view of exports.
 */
update_exportfsdata()
{
	struct exports *ex, *ex2, *k_ex=NULL, *k_ptr;
	struct exportfsdata kbuf;
	u_int cookie=0;

	/*
	 * Build a list equivalent to kernel's list.
	 */
	for (;;) {
		if ((exportfs (EXPORTFS_READ, &cookie, &kbuf)) < 0) {
			if (errno == ENOENT) break;
			syslog(LOG_ERR, "exportfs READ: %m");
			exit (1);
		}
		k_ex = newex(kbuf.e_path, k_ex, kbuf.e_fsid, kbuf.e_gnum, kbuf.e_gen, kbuf.e_rootmap, kbuf.e_flags);
		if (kbuf.e_more == 0) break;
	}
#ifdef DEBUG_FULL 
	(void) fprintf (stderr, "*** Kernel exportfsdata before changes ***\n");
	print_exports(k_ex);
#endif DEBUG_FULL  

	/*
	 * Loop through mountd's export list.  Find matching entry in
	 * local view of kernel's exportfsdata list.  If one is not found,
	 * add this export to kernel's list.  If they are different,
	 * update the kernel's data.  Mark the k_ex entry that has 
	 * been looked at so that we know what hasn't been checked yet.
	 */

	 for (ex=exports; ex!=NULL; ex=ex->ex_devptr) {
		for (ex2=ex; ex2!=NULL; ex2=ex2->ex_next) {

			/*
			   find matching kernel entry
			*/
			for (k_ptr=k_ex; k_ptr!=NULL; k_ptr=k_ptr->ex_next)
				if ((strcmp(k_ptr->ex_name, ex2->ex_name))==0) break;

			/*
			   if export is not in kernel's exportfsdata, add it 
			*/
			if (k_ptr == NULL) {
#ifdef DEBUG_FULL
				(void) fprintf(stderr, "Adding new export %s to kernel\n", ex2->ex_name);
#endif DEBUG_FULL 
				extokex(ex2, &kbuf);
				if ((exportfs(EXPORTFS_CREATE, NULL, &kbuf)) < 0) {
					syslog(LOG_ERR, "exportfs CREATE of %s: %m", kbuf.e_path);
					exit (1);
				}
			}
			/*
			 * if kernel's exportfsdata struct is not uptodate, replace it 
			 */
			else if (!(ex_same(k_ptr, ex2))) {
#ifdef DEBUG_FULL
				(void) fprintf(stderr, "Fixing kernel export %s\n", ex2->ex_name);
#endif DEBUG_FULL 
				extokex(k_ptr, &kbuf);
				if ((exportfs (EXPORTFS_REMOVE, NULL, &kbuf))< 0) {
					syslog(LOG_ERR, "exportfs REMOVE of %s: %m", kbuf.e_path);
					exit (1);
				}
				extokex(ex2, &kbuf);
				if ((exportfs(EXPORTFS_CREATE, NULL, &kbuf)) < 0) {
					syslog(LOG_ERR, "exportfs CREATE(2) of %s: %m", kbuf.e_path);
					exit (1);
				}
				k_ptr->ex_name= "CHECKED";
			}
			else k_ptr->ex_name= "CHECKED";
		}
	}
	/*
	 * Loop through k_ex list looking for entries that need to be 
	 * removed from kernel's exportfsdata list.
	 */

	for (k_ptr=k_ex; k_ptr!=NULL; k_ptr=k_ptr->ex_next) {
		if (strcmp(k_ptr->ex_name, "CHECKED") != 0) {
#ifdef DEBUG_FULL
			(void)fprintf(stderr,"Removing kernel export %s\n", k_ptr->ex_name);
#endif DEBUG_FULL 
			extokex(k_ptr, &kbuf);
			if ((exportfs (EXPORTFS_REMOVE, NULL, &kbuf)) < 0) {
				syslog(LOG_ERR, "exportfs REMOVE(2) of %s: %m", kbuf.e_path);
				exit (1);
			}
		}
	}
	freeex(k_ex);

#ifdef DEBUG_FULL 
	(void) fprintf (stderr, "*** Kernel exportfsdata after changes ***\n");
	cookie = 0;
	for (;;) {
		if ((exportfs (EXPORTFS_READ, &cookie, &kbuf)) < 0) {
			syslog(LOG_ERR, "exportfs READ(2): %m");
			exit (1);
		}
		(void)fprintf(stderr,"export name %s", kbuf.e_path);
		(void)fprintf(stderr,"-r=%d  dev: %d  ino: %d", kbuf.e_rootmap, kbuf.e_fsid, kbuf.e_gnum);
		if (kbuf.e_flags & M_EXRONLY)
			(void)fprintf(stderr," (M_EXRONLY)");
		(void)fprintf(stderr, "\n");
		if (kbuf.e_more == 0) break;
	}
	(void)fprintf(stderr, "\n");
#endif DEBUG_FULL
}

/*
 * Compare kernel's export entry to mountd's
 */

ex_same (k_ex, ex)
	struct exports *k_ex, *ex;
{
	if ((k_ex->ex_rootmap!=ex->ex_rootmap) || (k_ex->ex_flags!=ex->ex_flags) ||
		(k_ex->ex_ino != ex->ex_ino) || (k_ex->ex_gen != ex->ex_gen) || 
		(k_ex->ex_dev != ex->ex_dev))
		return (FALSE);
	return (TRUE);
}


newdev (name, dev, ino, gen)
	char *name;
	dev_t dev;
	ino_t ino;
	u_long gen;
{
	struct exports *new;
	char *newname;

	new = (struct exports *)exmalloc(sizeof(*new));
	newname = (char *)exmalloc(strlen(name) + 1);
	(void) strcpy(newname, name);

	new->ex_name = newname;
	new->ex_dev = dev;
	new->ex_ino = ino;
	new->ex_gen = gen;
	new->ex_next = NULL;
	new->ex_rootmap = -2;		/* Initialize to nobody */
	new->ex_flags = 0;
	new->ex_groups = NULL;		/* Groups ptr */
	new->ex_devptr = exports;
	exports = new;
}

struct exports *
newex(name, next, dev, ino, gen, rootmap, flags)
	char *name;
	struct exports *next;
	dev_t dev;
	ino_t ino;
	u_long gen;
	short rootmap;
	u_int flags;
{
	struct exports *new;
	char *newname;

	new = (struct exports *)exmalloc(sizeof(*new));
	newname = (char *)exmalloc(strlen(name) + 1);
	(void) strcpy(newname, name);

	new->ex_name = newname;
	new->ex_dev = dev;
	new->ex_ino = ino;
	new->ex_gen = gen;
	new->ex_rootmap = rootmap;
	new->ex_flags = flags;
	new->ex_next = next;
	new->ex_groups = NULL;		/* Groups ptr */
	new->ex_devptr = NULL;
	return (new);
}

extokex(ex, kex)
	struct exports *ex;
	struct exportfsdata *kex;
{
	(void)strcpy (kex->e_path, ex->ex_name);
	kex->e_fsid = ex->ex_dev;
	kex->e_gnum = ex->ex_ino;
	kex->e_gen = ex->ex_gen;
	kex->e_rootmap = ex->ex_rootmap;
	kex->e_flags = ex->ex_flags;
	kex->e_more = 0;
}

struct groups *
newgroup(name, next)
	char *name;
	struct groups *next;
{
	struct groups *new;
	char *newname;

	new = (struct groups *)exmalloc(sizeof(*new));
	newname = (char *)exmalloc(strlen(name) + 1);
	(void) strcpy(newname, name);

	new->g_name = newname;
	new->g_next = next;
	new->g_hbit = 0;
	return (new);
}

setopt(str, ex)
	char *str;
	struct exports *ex;
{
	char *m, *n;

	/*
	 * Parse options
	 */

	switch (str[0]) {
	case 'r':
		/*
		 * If r, then set root mapping field
		 */
		m = str;
		m++;
		if (*m != '=') {
			syslog(LOG_ERR, "%s bad export option format in %s", str, EXPORTS);
			return;
		}
		else {
			m++;
			if (*m == '-')
				n = ++m;
			else
				n = m;
			for (; !n ; n++)
				if (!isdigit(*n)) {
					/*
		 		 	 * Bad string
		 		 	 */
					syslog(LOG_ERR, "%c unknown root mapping integer in %s", *m, EXPORTS);
					return;
				}
			ex->ex_rootmap = atoi(m);
		}
		break;
	case 'n':
		/*
		 * if n, ignore (no filehandle obsolete)
		 */
		break;
	case 'o':
		/*
		 * if o, set flags field for readonly
		 */
		ex->ex_flags |= M_EXRONLY;
		break;
	default:
		/*
		 * Bad option
		 */
		syslog(LOG_ERR, "%c unknown export option in %s", str[0], EXPORTS);
		break;
	}
	return;
}


/*
 * Flatten exports list to show directory entries.
 */
flatten_exports()
{
	struct exports *ex, *ex2;
	struct exports *exnew = NULL;
	struct groups *groups;
	struct groups *groupsnew = NULL;

#ifdef DEBUG_FULL
	(void) fprintf(stderr, "\n******* Flattening exports: *******\n");
#endif DEBUG_FULL
	for (ex = exports; ex != NULL; ex = ex->ex_devptr) {
		for (ex2 = ex; ex2 != NULL; ex2 = ex2->ex_next) {
#ifdef DEBUG_FULL
			(void) fprintf(stderr, "making newex for %s\n", ex2->ex_name);
#endif DEBUG_FULL
			exnew = newex(ex2->ex_name, exnew, NULL, NULL, NULL, NULL, NULL);
			for (groups = ex2->ex_groups, groupsnew = NULL; groups != NULL; groups = groups->g_next) {
#ifdef DEBUG_FULL
				(void) fprintf(stderr, "making newgroup for %s\n", groups->g_name);
#endif DEBUG_FULL
				groupsnew = newgroup(groups->g_name, groupsnew);
			}
			exnew->ex_groups = groupsnew;
		}
	}
	flatexports = exnew;
}

char *
exmalloc(size)
	int size;
{
	char *ret;

	if ((ret = (char *)malloc((unsigned) size)) == 0) {
		syslog(LOG_ERR, "Memory allocation failed in exmalloc: %m");
		exit(1);
	}
	return (ret);
}

/*
 * Free entire ex list
 */
freeex(ex)
	struct exports *ex;
{
	struct exports *next_ex, *next_dev;
	struct groups *group_ptr, *next_group;

	while (ex) {
		next_dev = ex->ex_devptr;
		while (ex) {
			next_ex = ex->ex_next;
			group_ptr = ex->ex_groups;
			while (group_ptr) {
				next_group = group_ptr->g_next;
				free (group_ptr->g_name);
				free ((char *)group_ptr);
				group_ptr = next_group;
			}
			free (ex->ex_name);
			free ((char *)ex);
			ex = next_ex;
		}
		ex = next_dev;
	}
}


#ifdef DEBUG
print_exports(ex)
	struct exports *ex;
{
	struct groups *groups;
	struct exports *ex2;

	(void) fprintf(stderr, "\nexport list begin:\n");
	for (; ex != NULL; ex = ex->ex_devptr) {
		for (ex2 = ex; ex2 != NULL; ex2 = ex2->ex_next) {
			(void) fprintf(stderr, "fs or dir: %s dev: %x", 
				ex2->ex_name, ex2->ex_dev);
			(void) fprintf(stderr, " -r=%d ", ex2->ex_rootmap);
			if (ex2->ex_flags & M_EXRONLY)
				(void) fprintf(stderr, " ( M_EXRONLY )");
			(void) fprintf(stderr, "\n\tdev: %d, ino: %d", ex2->ex_dev, ex2->ex_ino);
			for (groups= ex2->ex_groups; groups!= NULL; groups= groups->g_next)
				(void) fprintf(stderr, " %s, hbit: %d", 
					groups->g_name, groups->g_hbit);
			(void) fprintf(stderr, "\n");
		}
		(void) fprintf(stderr, "finish exports for this dev\n");
	}
	(void) fprintf(stderr, "export list end:\n\n");
}
#endif DEBUG

usage()
{
	(void) fprintf(stderr,"usage: mountd [-i -d -s]\n");
	(void) fflush(stderr);
	exit(1);
}

mtd_abort()
{
	/*
	 * This routine is a do-nothing to replace the libc abort, to prevent
	 * mountd from crashing all the time.  It is safe to remove this stub
	 * if running under inetd, since inetd will just restart us when we
	 * crash and the core dumps may be useful.
	 */
}


/*
 * Restore saved mount state; rewrite file without commented out
 * entries
 */
rmtab_load()
{
        char *path;
        char *name;
        char *end;
        struct mountdlist *ml;
        char line[MAXRMTABLINELEN];
	int fd;

        f = fopen(RMTAB, "r");
        if (f != NULL) {
                while (fgets(line, sizeof(line), f) != NULL) {
                        name = line;
                        path = strchr(name, ':');
                        if (*name != '#' && path != NULL) {
                                *path = 0;
                                path++;
                                end = strchr(path, '\n');
                                if (end != NULL) {
                                        *end = 0;
                                }
                                ml = (struct mountdlist *)
                                        exmalloc(sizeof(struct mountdlist));
                                ml->ml_path = (char *)
                                        exmalloc(strlen(path) + 1);
                                (void)strcpy(ml->ml_path, path);
                                 ml->ml_name = (char *)
                                        exmalloc(strlen(name) + 1);
                                (void)strcpy(ml->ml_name, name);
                                ml->ml_nxt = mountlist;
                                mountlist = ml;
                        }
                }
                (void)fclose(f);
        }
	fd = open(RMTAB, O_CREAT|O_TRUNC|O_RDWR, 0644);
	f = fdopen(fd, "r+");
        if (f != NULL) {
                for (ml = mountlist; ml != NULL; ml = ml->ml_nxt) {
                        ml->ml_pos = rmtab_insert(ml->ml_name, ml->ml_path);
                }
		return(fd);
        }
	return(-1);
}

long
rmtab_insert(name, path)
        char *name;
        char *path;
{
        long pos;

        if (f == NULL || fseek(f, 0L, 2) == -1) {
                return (-1);
        }
        pos = ftell(f);
        if (fprintf(f, "%s:%s\n", name, path) == EOF) {
                return (-1);
        }
	fflush(f);
        return (pos);
}

rmtab_delete(pos)
        long pos;
{
        if (f != NULL && pos != -1 && fseek(f, pos, 0) == 0) {
                (void) fprintf(f, "#");
                fflush(f);
	}
}


build_ngcache()
{
	int err, i;
	struct ypall_callback cbinfo;

	cbinfo.foreach = getkeys;
	cbinfo.data = NULL;

	err = yp_all(mydomain, "netgroup", &cbinfo);
	if (err) {
		if (err != YPERR_MAP)
			syslog(LOG_ERR, "YP error building netgroup cache: %s",
				yperr_string(err));
		return;
	}
	for (i=0; i< num_ngs; i++) {
		(void)expand_ng(ng_names[i]);
		free(ng_names[i]);
	}	
}

/*
 * Called for each netgroup in yp database.  Returns 0 to have yp_all
 * call it again to process the next netgroup.
 */
static int
getkeys(status, key, kl, val, vl, data)
	int status;
	char *key;
	int kl;
	char *val;
	int vl;
	char *data;
{
	int size;

	if (status == YP_TRUE) {
		size = kl + 1;
		ng_names[num_ngs] = (char *)malloc((unsigned) size);
		strncpy(ng_names[num_ngs], key, kl);
		ng_names[num_ngs++][kl] = '\0';
		/*
		 * initial cache size is limited to 100 netgroups
		 */
		if (num_ngs == 100)  
			return(1);
		return(0);
	}
	if (status != YP_NOMORE)
		syslog(LOG_ERR, "YP error expanding netgroups: %s",
			yperr_string(ypprot_err(status)));
	return(1);
}


int
expand_ng(ngname)
	char *ngname;
{
	char *machp, *userp, *domainp;
	int is_a_ng=0;

	setnetgrent(ngname);
	while (getnetgrent(&machp, &userp, &domainp)) {
		if (is_a_ng++ == 0)  
			new_ng(ngname);
		if ((domainp == NULL) || (strcmp(domainp, mydomain) == 0)) {
			if (machp == NULL) {
				new_ng_host("everyone");
				break;
			}
			else if (isalnum(*machp) || (*machp == '_'))
				new_ng_host(machp);
		}
	}
	endnetgrent();
#ifdef DEBUG
	(void)fprintf(stderr,"Expand ng of %s returning %d\n", ngname, is_a_ng);
#endif DEBUG
	return(is_a_ng);
}


/* Is "export_gname" a netgroup and if so is client "mach" in it ??
 * Searches netgroup cache, rebuilds the ng entry if it is timed out,
 * adds a new ng entry if need be.  Sets ng_or_host to 0 if export_gname
 * is a netgroup, 1 if it is a hostname.
 */
int 
innetgr_cache(export_gname, mach, ng_or_host)
	char *export_gname, *mach;
	int *ng_or_host;
{
	struct ng *ngp;
	int success, match;
	static int variance;

#ifdef DEBUG
	(void)fprintf(stderr, "innetgr_cache called with gname: %s, mach: %s\n",
		export_gname, mach);
#endif DEBUG

	/* Try to find netgroup in cache */
	for (ngp = nglist; ngp != NULL; ngp = ngp->next_ng) {
		if (strcmp(export_gname, ngp->ng_name) == 0) {
			/*
			 * In cache, is it timed out?  Netgroups time out
			 * after 15-30 minutes.
			 */
			gettimeofday(&now, &tz);
			if (variance > 900) 
				variance=0;
			if (now.tv_sec - ngp->ng_stamp > (ngtimeout+variance)) {
			/*
			 * Timed out, re-expand it
			 */
#ifdef DEBUG
				(void)fprintf(stderr, 
				    "timed out: variance is %d\n", variance);
#endif DEBUG
				success = expand_ng(ngp->ng_name);
				if (success) {
					free_ng(ngp);
					ngp = nglist;
					/* add 1 minute to variance */
					variance += 60;
				}
				else {
					syslog(LOG_ERR, 
					  "Could not expand netgroup %s", 
					   ngp->ng_name);
					*ng_or_host = 1;
					return(FALSE);
				}
			}
			/* Is requesting host in this ng? */
			match = check_ng(ngp, mach);
			return(match);
		}
	}
	/* 
	 * export_gname not found in cache, try to expand it
	 */
	success = expand_ng(export_gname);
	if (! success) {
		*ng_or_host = 1;  /* its a hostname */
		return(FALSE);
	}
	match = check_ng(nglist, mach);
	return(match);
}
	

/*
 * returns true if mach is in the cached netgroup pointed to by ngp
 */
int
check_ng(ngp, mach)
	struct ng *ngp;
	char *mach;
{
	struct hlist *hptr;

	for (hptr = ngp->ng_hosts; hptr != NULL; hptr = hptr->next_host) {
		if ((strcmp(hptr->hname, mach) == 0) || 
		    (strcmp(hptr->hname, "everyone") == 0))
			return(TRUE);
	}
#ifdef DEBUG
	(void)fprintf(stderr, "Check ng returning FALSE\n");
#endif DEBUG
	return(FALSE);
}

new_ng(ngname)
	char *ngname;
{
	struct ng *new;
	char *newname;

	new = (struct ng *)exmalloc(sizeof(*new));
	newname = (char *)exmalloc(strlen(ngname) + 1);
	(void)strcpy(newname, ngname);

	new->ng_name = newname;
	new->ng_hosts = NULL;
	gettimeofday(&now, &tz);
	new->ng_stamp = now.tv_sec;
	new->next_ng = nglist;
	new->prev_ng = NULL;
	if (nglist != NULL)
		nglist->prev_ng = new;
	nglist = new;
}

new_ng_host(hname)
	char *hname;
{
	struct hlist *new;
	char *newname;

	new = (struct hlist *)exmalloc(sizeof(*new));
	newname = (char *)exmalloc(strlen(hname) + 1);
	(void)strcpy(newname, hname);

	new->hname = newname;
	new->next_host = nglist->ng_hosts;
	nglist->ng_hosts = new;
}

free_ng(ngp)
	struct ng *ngp;
{
	struct hlist *hptr, *tmphptr;

	hptr = ngp->ng_hosts;
	while (hptr) {
		tmphptr = hptr->next_host;
		free(hptr->hname);
		free(hptr);
		hptr = tmphptr;
	}
	if (nglist == ngp) {
		nglist = ngp->next_ng;
		ngp->next_ng->prev_ng = NULL;
	}
	else {
		ngp->prev_ng->next_ng = ngp->next_ng;
		if (ngp->next_ng != NULL)
			ngp->next_ng->prev_ng = ngp->prev_ng;
	}
	free(ngp->ng_name);
	free(ngp);
}

#ifdef DEBUG
print_ngcache()
{
	struct ng *ngp;
	struct hlist *hptr;

	(void) fprintf(stderr, "NETGROUPS CACHE begin: \n");
	for (ngp = nglist; ngp != NULL; ngp = ngp->next_ng) {
		(void) fprintf(stderr, "\t ng_name: %s  stamp: %d\n",
			ngp->ng_name, ngp->ng_stamp);
		(void) fprintf(stderr, "\t HOSTS:\n");
		for (hptr=ngp->ng_hosts; hptr!=NULL; hptr=hptr->next_host) {
			(void)fprintf(stderr, "\t\t %s\n", hptr->hname);
		}
	}
	(void) fprintf(stderr, "NETGROUPS CACHE end.\n");
}
#endif DEBUG

/*
 * Initialize the duplicate reply cache. . .
 */
dupcache_init() 
{
	register struct dupreq *dr;
	register struct dupreq **dt;
	int i;

	dupcache_max = 1024;
	drhashsz = dupcache_max / 16;
	drhashszminus1 = drhashsz - 1;

	dr= (struct dupreq *) malloc(sizeof(*dr) * dupcache_max);

	dt= (struct dupreq **) malloc(sizeof(struct dupreq *) * drhashsz);

	for (i = 0; i < dupcache_max; i++)
		dr[i].rc_next = &(dr[i + 1]);
	dr[dupcache_max - 1].rc_next = dr;
	dupreqcache = dr;
	drmru = dr;

	for (i = 0; i < drhashsz; i++)
		dt[i] = NULL;
	drhashtbl = dt;
}


dupcache_inval()
{
	int i;
#ifdef DEBUG
	(void) fprintf(stderr, "Invalidating duplicate requests cache\n");
#endif DEBUG
	for (i = 0; i < dupcache_max; i++)
		unhash(&dupreqcache[i]);
}


dupcache_enter(addr,dev,ino,gen, anc_name)
	struct in_addr addr;
	dev_t  dev;
	u_long	ino;	
	u_long	gen;	
	char *anc_name;
{
	register struct dupreq *dr;
	char *newname;

#ifdef DEBUG
	(void) fprintf(stderr,"Entering addr:%x dev:%x ino:%d %d anc:%s\n",
		addr.s_addr,dev,ino,gen, anc_name);
#endif DEBUG
	dr = drmru->rc_next;
	unhash(dr);
	drmru = dr;

	newname = (char *)exmalloc(strlen(anc_name) + 1);
	(void) strcpy(newname, anc_name);

	dr->rc_addr = addr;
	dr->rc_dev = dev;
	dr->rc_ino = ino;
	dr->rc_gen = gen;
	dr->rc_ancname = newname;

	dr->rc_chain = drhashtbl[((addr.s_addr ^ (int) dev ^ (int) ino ^ (int) gen) % drhashszminus1)];
	drhashtbl[((addr.s_addr ^ (int)dev ^ (int)ino ^ (int) gen) % drhashszminus1)] = dr;
#ifdef DEBUG
	print_dupcache();
#endif DEBUG
}


/*
 * returns a pointer to the dup req cache entry if it exists
 */
struct dupreq *
dupcache_check(addr,dev,ino,gen)
	struct in_addr addr;
	dev_t  dev;
	u_long	ino;	
	u_long	gen;	
{
	register struct dupreq *dr;
#ifdef DEBUG
	(void) fprintf(stderr,"CHECK DUP CACHE for addr:%x dev:%x ino:%d %d\n",
			addr.s_addr,dev,ino,gen);
#endif DEBUG
	dr = KEYTOLIST(addr.s_addr,dev,ino,gen); 
	while (dr != NULL) { 
		if (NOTDUP(dr,addr.s_addr,dev,ino,gen)) {
			dr = dr->rc_chain;
			continue;
		}
#ifdef DEBUG
		(void)fprintf(stderr,"\t Got it!\n");
#endif DEBUG
		return(dr);
	}
#ifdef DEBUG
	(void)fprintf(stderr,"\t Nope\n");
#endif DEBUG
	return((struct dupreq *) 0);
}

static
unhash(dr)
	struct dupreq *dr;
{
	struct dupreq *drt;
	struct dupreq *drtprev = NULL;
	 
	drt = REQTOLIST(dr); 
	while (drt != NULL) { 
		if (drt == dr) { 
			if (drtprev == NULL) {
				REQTOLIST(dr) = drt->rc_chain;
			} else {
				drtprev->rc_chain = drt->rc_chain;
			}
			free(dr->rc_ancname);
			return; 
		}	
		drtprev = drt;
		drt = drt->rc_chain;
	}	
}

#ifdef DEBUG
print_dupcache()
{
	struct dupreq *dr, **dt;
	int i;

	(void) fprintf(stderr, "DUP REQS CACHE begin: \n");
	dt = drhashtbl;
	for (i=0; i < drhashsz; i++) {
		(void)fprintf(stderr, "hash list[%d]\n", i);
		for (dr = dt[i]; dr != NULL; dr = dr->rc_chain) {
		    (void)fprintf(stderr,"\t addr:%x dev:%x ino:%d %d anc:%s\n",
			dr->rc_addr, dr->rc_dev, dr->rc_ino, dr->rc_gen,
			dr->rc_ancname);
		}
	}
	(void) fprintf(stderr, "DUP REQS CACHE end.\n");
}
#endif DEBUG

/*
 * Input name in raw, canonicalized pathname output to canon.  If dosymlinks
 * is nonzero, resolves all symbolic links encountered during canonicalization
 * into an equivalent symlink-free form.  Returns 0 on success, -1 on failure.
 *
 * Sets errno on failure.
 */
int
pathcanon(raw, canon, dosymlinks)
    char	*raw,
		*canon;
    int		dosymlinks;
{
    register char	*s,
			*d;
    register char	*limit = canon + MAXPATHLEN;
    char		*modcanon;
    int			nlink = 0;

    /*
     * Make sure that none of the operations overflow the corresponding buffers.
     * The code below does the copy operations by hand so that it can easily
     * keep track of whether overflow is about to occur.
     */
    s = raw;
    d = canon;
    modcanon = canon;

    while (d < limit && *s)
	*d++ = *s++;

    /* Add a trailing slash to simplify the code below. */
    s = "/";
    while (d < limit && (*d++ = *s++))
	continue;
	
    /*
     * Canonicalize the path.  The strategy is to update in place, with
     * d pointing to the end of the canonicalized portion and s to the
     * current spot from which we're copying.  This works because
     * canonicalization doesn't increase path length, except as discussed
     * below.  Note also that the path has had a slash added at its end.
     * This greatly simplifies the treatment of boundary conditions.
     */
    d = s = modcanon;
    while (d < limit && *s) {
	if ((*d++ = *s++) == '/' && d > canon + 1) {
	    register char  *t = d - 2;

	    switch (*t) {
	    case '/':
		/* Found // in the name. */
		d--;
		continue;
	    case '.': 
		switch (*--t) {
		case '/':
		    /* Found /./ in the name. */
		    d -= 2;
		    continue;
		case '.': 
		    if (*--t == '/') {
			/* Found /../ in the name. */
			while (t > canon && *--t != '/')
			    continue;
			d = t + 1;
		    }
		    continue;
		default:
		    break;
		}
		break;
	    default:
		break;
	    }
	    /*
	     * We're at the end of a component.  If dosymlinks is set
	     * see whether the component is a symbolic link.  If so,
	     * replace it by its contents.
	     */
	    if (dosymlinks) {
		char		link[MAXPATHLEN + 1];
		register int	llen;

		/*
		 * See whether it's a symlink by trying to read it.
		 *
		 * Start by isolating it.
		 */
		*(d - 1) = '\0';
		if ((llen = readlink(canon, link, sizeof link)) >= 0) {
		    /* Make sure that there are no circular links. */
		    nlink++;
		    if (nlink > MAXSYMLINKS) {
			errno = ELOOP;
			return (-1);
		    }
		    /*
		     * The component is a symlink.  Since its value can be
		     * of arbitrary size, we can't continue copying in place.
		     * Instead, form the new path suffix in the link buffer
		     * and then copy it back to its proper spot in canon.
		     */
		    t = link + llen;
		    *t++ = '/';
		    /*
		     * Copy the remaining unresolved portion to the end
		     * of the symlink. If the sum of the unresolved part and
		     * the readlink exceeds MAXPATHLEN, the extra bytes
		     * will be dropped off. Too bad!
		     */
		    (void) strncpy(t, s, sizeof link - llen - 1);
		    link[sizeof link - 1] = '\0';
		    /*
		     * If the link's contents are absolute, copy it back
		     * to the start of canon, otherwise to the beginning of
		     * the link's position in the path.
		     */
		    if (link[0] == '/') {
			/* Absolute. */
			(void) strcpy(canon, link);
			d = s = canon;
		    }
		    else {
			/*
			 * Relative: find beginning of component and copy.
			 */
			--d;
			while (d > canon && *--d != '/')
			    continue;
			s = ++d;
			/*
			 * If the sum of the resolved part, the readlink
			 * and the remaining unresolved part exceeds
			 * MAXPATHLEN, the extra bytes will be dropped off.
			*/
			if (strlen(link) >= (limit - s)) {
				(void) strncpy(s, link, limit - s);
				*(limit - 1) = '\0';
			} else {
				(void) strcpy(s, link);
			}
		    }
		    continue;
		} else {
		   /*
		    * readlink call failed. It can be because it was
		    * not a link (i.e. a file, dir etc.) or because the
		    * the call actually failed.
		    */
		    if (errno != EINVAL)
			return (-1);
		    *(d - 1) = '/';	/* Restore it */
		}
	    } /* if (dosymlinks) */
	}
    } /* while */

    /* Remove the trailing slash that was added above. */
    if (*(d - 1) == '/' && d > canon + 1)
	    d--;
    *d = '\0';
    return (0);
}

/*
 * Canonicalize the path given in raw, resolving away all symbolic link
 * components.  Store the result into the buffer named by canon, which
 * must be long enough (MAXPATHLEN bytes will suffice).  Returns NULL
 * on failure and canon on success.
 *
 * The routine indirectly invokes the readlink() system call 
 * so it inherits the possibility of hanging due to inaccessible file 
 * system resources.
 */
char *
realpath(raw, canon)
    char	*raw;
    char	*canon;
{
    return (pathcanon(raw, canon, 1) < 0 ? NULL : canon);
}
