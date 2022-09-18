#ifndef lint
static	char	*sccsid = "@(#)nfs_mount.c	4.2	(ULTRIX)	9/7/90";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1986 by				*
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
 *	Copyright (c) 1986 Sun Microsystems, Inc.  ALL RIGHTS RESERVED.
 */

/*
 * nfs_mount
 */

#include <sys/param.h>
#include <sys/mount.h>
#include <rpc/rpc.h>
#include <sys/errno.h>
#include <sys/time.h>
#include <nfs/nfs.h>
#include <rpcsvc/mount.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netdb.h>
#include <nfs/nfs_clnt.h>
#include <nfs/vfs.h>
#include <sys/fs_types.h>
#include <nfs/nfs_gfs.h>
#include <ctype.h>
#include <stdio.h>

#define max(a,b)	((a) > (b) ? (a) : (b))

#define	MNTOPT_RO	"ro"		/* read only */
#define	MNTOPT_RW	"rw"		/* read/write */
#define	MNTOPT_SOFT	"soft"		/* soft mount */
#define	MNTOPT_HARD	"hard"		/* hard mount */
#define MNTOPT_BG	"bg"		/* background mount if no answer */
#define	MNTOPT_NOSUID	"nosuid"	/* no set uid allowed */
#define MNTOPT_NOEXEC	"noexec"	/* no execution allowed */
#define MNTOPT_NODEV	"nodev"		/* no devices access allowed */
#define MNTOPT_FORCE 	"force"		/* force the mount */
#define MNTOPT_SYNC 	"sync"		/* synchronous writes */
#define MNTOPT_NOCACHE 	"nocache"	/* don't keep in cache -- write thru */
#define MNTOPT_INT	"intr"		/* allow hard mount keyboard interrupts */
#define MNTOPT_RSIZE	"rsize="	/* read size */
#define MNTOPT_WSIZE	"wsize="	/* write size */
#define MNTOPT_NOAC     "noac"          /* no attributes caching */
#define MNTOPT_ACTIMEO  "actimeo="      /* set all 4 ac timeout values */
#define MNTOPT_ACREGMIN "acregmin="     /* min seconds to cache file attributes */
#define MNTOPT_ACREGMAX "acregmax="     /* max seconds to cache file attributes */
#define MNTOPT_ACDIRMIN "acdirmin="     /* min seconds to cache dir attributes */
#define MNTOPT_ACDIRMAX "acdirmax="     /* max seconds to cache dir attributes */
#define MNTOPT_RETRANS	"retrans="	/* # of NFS retries */
#define MNTOPT_RETRY	"retry="	/* # of mount retries */
#define MNTOPT_TIMEO	"timeo="	/* timeout interval */
#define	MNTOPT_PORT	"port="		/* NFS port # */
#define MNTOPT_PGTHRESH "pgthresh="	/* paging threshold */

#define	MINRSIZE	512
#define MAXRSIZE	8192
#define	MINWSIZE	512
#define MAXWSIZE	8192
#define MINTIMEO	2
#define MAXTIMEO	1000
#define MINRETRANS	1
#define	MAXRETRANS	10000
#define MINRETRY	1
#define MAXRETRY	10000

#define NFSAC_REGMIN    3
#define NFSAC_REGMAX    60
#define NFSAC_DIRMIN    30
#define NFSAC_DIRMAX    60
#define MAXACTIME       3600

#define PGUNITS		1024	/* to convert MINPGTHRESH to K */
#define SIXTYFOUR	64	/* default page threshhold */

struct	mntent{
	char	*mnt_fsname;		/* name of mounted file system */
	char	*mnt_dir;		/* file system path prefix */
	char	*mnt_opts;		/* MNTOPT* */
};

char * hasmntopt();

int	ro = 0;
int	verbose = 0;
int	printed = 0;
struct	nfs_gfs_mount args;
u_short port;
int	retry;		/* number of times to retry a mount request */
int	bg;		/* put this mount in background if no answer? */

#define	BGSLEEP	5	/* initial sleep time for background mount in seconds */
#define MAXSLEEP 120	/* max sleep time for background mount in seconds */

extern int errno;

char	*index(), *rindex();
char	host[MNTMAXSTR];
char	name[MNTMAXSTR];
char	dir[MNTMAXSTR];
char	opts[MNTMAXSTR];
char	tmpopts[MNTMAXSTR];



main(argc, argv)
int argc;
char **argv;
{
	struct mntent mnt;
	char *options;
	char *colon;

	if (argc == 1) {
		usage();
		exit(0);
	}

	/*
	 * Set options
	 */
	opts[0] = '\0';
	while (argc > 1 && argv[1][0] == '-') {
		options = &argv[1][1];
		while (*options) {
			switch (*options) {
			case 'o':
				if (argc < 3) {
					usage();
				}
				if (strlen(argv[2]) >= MNTMAXSTR) {
					(void) fprintf(stderr, "nfs_mount: -o string too long\n");
					exit(1);
				}
				strcpy(opts, argv[2]);
				argv++;
				argc--;
				break;
			case 'p':
				if (argc != 2) {
					usage();
				}
				prmount();
				exit(0);				
			case 'r':
				ro++;
				break;
			case 'v':
				verbose++;
				break;
			default:
				(void) fprintf(stderr, "nfs_mount: unknown option: %c\n",
				    *options);
				usage();
			}
			options++;
		}
		argc--, argv++;
	}

/*
	if (geteuid() != 0) {
		(void) fprintf(stderr, "Must be root to use nfs_mount\n");
		exit(1);
	}
*/

	if (argc != 3) {
		usage();
	}

	if (strlen(argv[2]) >= MNTMAXSTR) {
		(void) fprintf(stderr, "nfs_mount: directory name too long\n");
		exit(1);
	}
	strcpy(dir, argv[2]);

	if (strlen(argv[1]) >= MNTMAXSTR) {
		(void) fprintf(stderr, "nfs_mount: file system name too long\n");
		exit(1);
	}
	strcpy(name, argv[1]);

	if (dir[0] != '/') {
		(void) fprintf(stderr, "nfs_mount: invalid directory name \"%s\";\n", dir);
		(void) fprintf(stderr, "\tdirectory pathname must begin with '/'.\n");
		exit(1);
	}

	mnt.mnt_fsname = name;
	mnt.mnt_dir = dir;
	mnt.mnt_opts = opts;
	getflags(&mnt);

	mountloop(&mnt);

}

mountloop(mnt)
register struct mntent *mnt;
{
	int error;
	int slptime;
	int forked;
	int retries;
	int firsttry;
	int hard_fail;

	hard_fail = 0;
	forked = 0;
	printed = 0;
	firsttry = 1;
	slptime = BGSLEEP;

	if (retry == 0)
		retries = MAXRETRY;
	else
		retries = retry;

	do {
		error = mountfs(!forked, mnt);
		if (error != ETIMEDOUT && error != ENETDOWN &&
			error != ENETUNREACH && error != ENOBUFS &&
			error != ECONNREFUSED && error != ECONNABORTED) {
				hard_fail = 1;
				break;
		}
		if (!forked && bg) {
			(void) fprintf(stderr,
				"nfs_mount: backgrounding %s\n",
				mnt->mnt_dir);
			if (fork()) {
				return;
			} else {
				forked = 1;
			}
		}
		if (!forked && firsttry && retries > 1) {
			(void) fprintf(stderr,
				"nfs_mount: retrying %s\n",
				mnt->mnt_dir);
			firsttry = 0;
		}
		if (retries > 1) {
			sleep(slptime);
			slptime = MIN(slptime << 1, MAXSLEEP);
		}
	} while (--retries);

	if (error && !hard_fail) {
		(void) fprintf(stderr,
			"nfs_mount: giving up on %s\n",
			mnt->mnt_dir);
	}
	if (forked) {
		exit(0);
	}
}

/*
 * attempt to mount file system, return errno or 0
 */
mountfs(print, mnt)
int print;
struct mntent *mnt;
{
	int error;
	int flags = 0;
	char *optp, *optend;

	error = mount_nfs(mnt);

	if (error)
		return (error);

	flags = (ro || (args.flags & NFSMNT_RONLY)) ? 1 : 0;
	error = mount(mnt->mnt_fsname, mnt->mnt_dir, flags, GT_NFS, &args);
	if (error) {
		if (print) {
			(void) fprintf(stderr,
				"nfs_mount: cannot mount %s on ", mnt->mnt_fsname);
			perror(mnt->mnt_dir);
		}
		return (errno);
	}

	if (verbose)
		(void) fprintf(stdout, "%s mounted on %s\n",
		    mnt->mnt_fsname, mnt->mnt_dir);

	return (0);
}

mount_nfs(mnt)
struct mntent *mnt;
{

	struct hostent *hp;
	static struct fhstatus fhs;
	static struct sockaddr_in sin;
	int err;
	char *cp;
	char *hostp = host;
	char *path;
	int s;
	struct timeval timeout;
	CLIENT *client;
	enum clnt_stat rpc_stat;

	cp = mnt->mnt_fsname;
	while ((*hostp = *cp) != ':') {
		if (*cp == '\0') {
			(void) fprintf(stderr,
			"nfs_mount: illegal file system name \"%s\";\n", mnt->mnt_fsname);
			(void) fprintf(stderr,
			"\tuse host:pathname.\n");
			exit(1);
		}
		hostp++;
		cp++;
	}
	*hostp = '\0';
	path = ++cp;
	/*
	 * Get server's address
	 */
	if ((hp = gethostbyname(host)) == NULL) {
		/*
		 * XXX
		 * Failure may be due to yellow pages, try again
		 */
		if ((hp = gethostbyname(host)) == NULL) {
			(void) fprintf(stderr,
			    "nfs_mount: %s not in hosts database.\n", host);
			exit(1);
		}
	}

	/*
	 * get fhandle of remote path from server's mountd
	 * (first check that the server's nfsd is up and talking)
	 */

	if (err = callrpc(host, NFS_PROGRAM, NFS_VERSION, NULLPROC,
	   xdr_void, 0, xdr_void, 0)) {
		if (!printed) {
			(void) fprintf(stderr, 
				"nfs_mount: %s server's nfsd not responding: ",
				mnt->mnt_fsname);
			clnt_perrno(err);
			(void) fprintf(stderr, "\n");
			printed = 1;
		}
		return (ETIMEDOUT);
	}


	bzero(&sin, sizeof(sin));
	bcopy(hp->h_addr, (char *) &sin.sin_addr, hp->h_length);
	sin.sin_family = AF_INET;
	timeout.tv_usec = 0;
	timeout.tv_sec = 20;
	s = RPC_ANYSOCK;
	if ((client = clntudp_create(&sin, MOUNTPROG, MOUNTVERS,
	    timeout, &s)) == NULL) {
		if (!printed) {
			(void) fprintf(stderr,
				"nfs_mount: %s server not responding",
				mnt->mnt_fsname);
			clnt_pcreateerror("");
			printed = 1;
		}
		return (ETIMEDOUT);
	}

	if (! local_bindresvport(s, &sin)) {
		if (geteuid() == 0)	/* suppress message if not root */
			perror("Warning: cannot bind to privileged port");
	}

	/* the next section is necessary if the server's mountd has
	 * ipaddr_check on.  If we are dealing with subnets, our address
  	 * must match the corresponding hostname in the hosts database.
	 * (not necessarily /bin/hostname)
	 */
	{
        register int len;
        register int uid;
        register int gid;
	struct hostent *machname;
        int gids[NGROUPS];
	struct sockaddr_in addr;
	struct hostent myhname;
	char hnam[MAX_MACHINE_NAME + 1];

	get_myaddr_dest(&addr,&sin);

        if ((machname = gethostbyaddr((char *) &addr.sin_addr.s_addr,4,AF_INET)) == NULL ) 
		if ( gethostname(hnam, MAX_MACHINE_NAME) < 0) {
			fprintf(stderr, "mount_nfs: gethostname failed\n");
                	abort();
		} else {
			hnam[MAX_MACHINE_NAME] = 0;
			myhname.h_name = hnam;
			machname = &myhname;
		}

        uid = geteuid();
	gid = getegid();
        if ((len = getgroups(NGROUPS, gids)) < 0){
                perror("mount_nfs: getgroups: ");
                abort();
        }
        if(len > NGRPS)  /* rpc/xdr only allows NGRPS groups */
                len = NGRPS;
        client->cl_auth = authunix_create(machname->h_name, uid, gid, len, gids);
	}

	timeout.tv_usec = 0;
	timeout.tv_sec = 20;
	rpc_stat = clnt_call(client, MOUNTPROC_MNT, xdr_path, &path,
	    xdr_fhstatus, &fhs, timeout);
	errno = 0;
	if (rpc_stat != RPC_SUCCESS) {
		if (!printed) {
			(void) fprintf(stderr,
				"nfs_mount: %s server not responding",
				mnt->mnt_fsname);
			clnt_perror(client, "");
			printed = 1;
		}
		switch (rpc_stat) {
		case RPC_TIMEDOUT:
		case RPC_PMAPFAILURE:
		case RPC_PROGNOTREGISTERED:
			errno = ETIMEDOUT;
			break;
		case RPC_AUTHERROR:
			errno = EACCES;
			break;
		default:
			errno = 0;
			break;
		}
	}
	clnt_destroy(client);

	if (errno || (errno = fhs.fhs_status)) {
		if (errno == EACCES) {
			(void) fprintf(stderr,
				"nfs_mount: access denied for %s:%s\n",
			 	host, path);
		} else {
			(void) fprintf(stderr, "nfs_mount: cannot mount ");
			perror(mnt->mnt_fsname);
		}
		return (errno);
	}
	if (printed) {
		(void) fprintf(stderr,
			"nfs_mount: %s server ok\n", mnt->mnt_fsname);
		printed = 0;
	}

	/* set remaining mount args */
	args.fh = &fhs.fhs_fh;
	args.hostname = host;
	args.flags |= NFSMNT_HOSTNAME;
	args.optstr = mnt->mnt_opts;
	if (port) {
		sin.sin_port = htons(port);
	} else {
		sin.sin_port = htons(NFS_PORT);	/* XXX should use portmapper */
	}
	args.addr = &sin;

	return (0);
}

usage()
{
	(void) fprintf(stderr,
	"Usage: mount -t nfs [-r] [-v] [-o option,...] host:fsname dir\n");
	exit(1);
}

static char *
mntopt(p)
char **p;
{
	char *cp = *p;
	char *retstr;
	while (*cp && isspace(*cp))
		cp++;
	retstr = cp;
	while (*cp && *cp != ',')
		cp++;
	if (*cp) {
		*cp = '\0';
		cp++;
	}
	*p = cp;
	return (retstr);
}

char *
hasmntopt(mnt, opt)
register struct mntent *mnt;
register char *opt;
{
	char *f, *o;

	strcpy(tmpopts, mnt->mnt_opts);
	o = tmpopts;
	f = mntopt(&o);
	for (; *f; f = mntopt(&o)) {
		if (strncmp(opt, f, strlen(opt)) == 0)
			return (f - tmpopts + mnt->mnt_opts);
	} 
	return (NULL);
}

removemntopt(mnt,opt)
register struct mntent *mnt;
register char *opt;
{
	char *optp, *optend;

	if ((optp = hasmntopt(mnt, opt)) != NULL) {
		optend = index(optp, ',');
		if (optp != mnt->mnt_opts) {
			optp--;
			if (optend == NULL)
				*optp = '\0';
		}
		else {
			if (optend == NULL)
				*optp = '\0';
			else
				optend++;
		}
		if (optend != NULL)			
			while (*optp++ = *optend++)
				;
		return (1);
	}
	else
		return (0);
}

prmount()
{
	struct v_fs_data fsdata;
	struct mntent mnt;
	char optbuf[MNTMAXSTR];

	mnt.mnt_opts = optbuf;

	while (read(0, &fsdata, sizeof(struct v_fs_data)) == 
		sizeof(struct v_fs_data)) {

	opts[0] = '\0';
	strcpy(mnt.mnt_opts, fsdata.fd_un.gvfs.mi.mi_optstr);
	while (removemntopt(&mnt, MNTOPT_RO)) ;
	while (removemntopt(&mnt, MNTOPT_RW)) ;
	while (removemntopt(&mnt, MNTOPT_HARD)) ;
	while (removemntopt(&mnt, MNTOPT_SOFT)) ;

	if (fsdata.fd_flags & M_RONLY)
		strcat(opts, MNTOPT_RO);
 	else
		strcat(opts, MNTOPT_RW);
	strcat(opts, ",");

	if (fsdata.fd_un.gvfs.mi.mi_hard)
		strcat(opts, MNTOPT_HARD);
	else
		strcat(opts, MNTOPT_SOFT);

	if (mnt.mnt_opts[0] != '\0')
		strcat(opts, ",");
	strcat(opts, mnt.mnt_opts);

	(void) fprintf(stdout, "%s on %s type nfs (%s)\n",
		fsdata.fd_devname, fsdata.fd_path, opts);

	}
	return(0);
}

/*
 * Returns true if s1 is a pathname substring of s2.
 */
substr(s1, s2)
char *s1;
char *s2;
{
	while (*s1 == *s2) {
		s1++;
		s2++;
	}
	if (*s1 == '\0' && *s2 == '/') {
		return (1);
	}
	return (0);
}

local_bindresvport(sd, tsin)
int sd;
struct sockaddr_in *tsin;
{
 
	u_short port;
	struct sockaddr_in sin;
	int err = -1;

#	define MAX_PRIV (IPPORT_RESERVED-1)
#	define MIN_PRIV (IPPORT_RESERVED/2)

	bzero(&sin, sizeof(struct sockaddr_in));
	get_myaddr_dest(&sin, tsin);
	sin.sin_family = AF_INET;
	for (port = MAX_PRIV; err && port >= MIN_PRIV; port--) {
		sin.sin_port = htons(port);
		err = bind(sd,&sin,sizeof(sin));
	}
	return (err == 0);
}

getflags(mnt)
struct mntent *mnt;
{

	char optbuf[MNTMAXSTR];
	register char *p = optbuf, *q = tmpopts, *r;
	register int len; 
	char *tmp;
	/* flags set if default attr cache values changed */
	int actimeo=0, acmod=0;

	/*
	 * set default mount opts
	 */
	args.flags = args.gfs_flags = 0;
	args.flags |= NFSMNT_INT;
	args.pg_thresh = SIXTYFOUR;
	args.acregmin = NFSAC_REGMIN;
	args.acregmax = NFSAC_REGMAX;
	args.acdirmin = NFSAC_DIRMIN;
	args.acdirmax = NFSAC_DIRMAX;
	port = 0;
	retry = 0;
	bg = 0;

	strcpy(p, mnt->mnt_opts);
	if (*p == NULL) return (0);
	/*
	 * go through the string keeping only characters in the
	 * range a-z 0-9 = or , and eliminating double commas and double =
	 */
	r = p + strlen(p);
	while (p < r && q < (tmpopts + MNTMAXSTR - 1)) {
		if ((*p >= 'a' && *p <= 'z') || *p == ',' || *p == '=' ||
			(*p >= '0' && *p <= '9')) {
			if ((*p == ',' || *p == '=') &&
				*(q-1) != ',' && *(q-1) != '=') *q++ = *p;
			else if (*p != ',' && *p != '=') *q++ = *p;
		}
		p++;
	}
	*q = NULL;
	p = tmpopts;
	r = tmpopts + strlen(tmpopts);
	strcpy(mnt->mnt_opts, tmpopts);

top:
	for (q=p; *q != ',' && q < r; q++);
	*q = NULL;
	q++;
	switch (*p) {
	case 'a':
		if (*(p+1) != 'c')
			opterr(p);
		switch(*(p+2)) {
		case 'd':	/* acdirmin= or acdirmax= */
			len = strlen(MNTOPT_ACDIRMIN);
			if (strncmp(p, MNTOPT_ACDIRMIN, len) == 0) {
				args.acdirmin = atoi(&p[len]);
				if (args.acdirmin < 0)
					opterr(p);
				acmod++; 
				break;
			}
			else if (strncmp(p, MNTOPT_ACDIRMAX, len) == 0) {
				args.acdirmax = atoi(&p[len]);
				if (args.acdirmax > MAXACTIME)
					opterr(p);
				acmod++; 
				break;
			}
			else 
				opterr(p);
			break;
		case 'r':	/* acregmin= or acregmax= */
			len = strlen(MNTOPT_ACREGMIN);
			if (strncmp(p, MNTOPT_ACREGMIN, len) == 0) {
				args.acregmin = atoi(&p[len]);
				if (args.acregmin < 0)
					opterr(p);
				acmod++; 
				break;
			}
			else if (strncmp(p, MNTOPT_ACREGMAX, len) == 0) {
				args.acregmax = atoi(&p[len]);
				if (args.acregmax > MAXACTIME)
					opterr(p);
				acmod++; 
				break;
			}
			else 
				opterr(p);
			break;
		case 't':	/* actimeo= */
			len = strlen(MNTOPT_ACTIMEO);
			if (strncmp(p, MNTOPT_ACTIMEO, len))
				opterr(p);
			args.acregmin = atoi(&p[len]);
			if ((args.acregmin < 0) || (args.acregmin > MAXACTIME))
				opterr(p);
			actimeo++;
			args.acregmax = args.acregmin;
			args.acdirmin = args.acregmin;
			args.acdirmax = args.acregmin;
			break;
		default:
			opterr(p);
		}
		break;
	case 'b':	/* bg */
		if (strcmp(p, MNTOPT_BG))
			opterr(p);
		bg = 1;
		break;
	case 'f':	/* force */
		if (strcmp(p, MNTOPT_FORCE))
			opterr(p);
		args.gfs_flags |= M_FORCE;
		break;
	case 'h':	/* hard */
		if (strcmp(p, MNTOPT_HARD))
			opterr(p);
		args.flags &= ~NFSMNT_SOFT;
		break;
	case 'i':	/* intr */
		if (strcmp(p, MNTOPT_INT))
			opterr(p);
		args.flags |= NFSMNT_INT;
		break;
	case 'n':
		if (*(p+1) != 'o')
			opterr(p);
		switch(*(p+2)) {
		case 'a':   /* noac */
			if (strcmp(p, MNTOPT_NOAC))
				opterr(p);
			args.flags |= NFSMNT_NOAC;
			break;
		case 'e':	/* noexec */
			if (strcmp(p, MNTOPT_NOEXEC))
				opterr(p);
			args.gfs_flags |= M_NOEXEC;
			break;
		case 's':	/* nosuid */
			if (strcmp(p, MNTOPT_NOSUID))
				opterr(p);
			args.gfs_flags |= M_NOSUID;
			break;
		case 'd':	/* nodev */
			if (strcmp(p, MNTOPT_NODEV))
				opterr(p);
			args.gfs_flags |= M_NODEV;
			break;
		case 'c':	/* nocache */
			if (strcmp(p, MNTOPT_NOCACHE))
				opterr(p);
			args.gfs_flags |= M_NOCACHE;
			break;
		default:
			opterr(p);
		}
		break;
	case 'p':
		switch(*(p+1)) {
		case 'g':	/* pgthresh= */
			len = strlen(MNTOPT_PGTHRESH);
			if (strncmp(p, MNTOPT_PGTHRESH, len))
				opterr(p);
			args.pg_thresh = max(atoi(&p[len]),MINPGTHRESH/PGUNITS);
			args.flags |= NFSMNT_PGTHRESH;
			break;
		case 'o':	/* port= */
			len = strlen(MNTOPT_PORT);
			if (strncmp(p, MNTOPT_PORT, len))
				opterr(p);
			port = atoi(&p[len]);
			break;
		default:
			opterr(p);
		}
		break;
	case 'r':
		switch(*(p+1)) {
		case 'o':	/* ro */
			if (strcmp(p, MNTOPT_RO))
				opterr(p);
			args.flags |= NFSMNT_RONLY;
			args.gfs_flags |= M_RONLY;
			break;
		case 'w':	/* rw */
			if (strcmp(p, MNTOPT_RW))
				opterr(p);
			args.flags &= ~NFSMNT_RONLY;
			args.gfs_flags &= ~M_RONLY;
			break;
		case 's':	/* rsize= */
			len = strlen(MNTOPT_RSIZE);
			if (strncmp(p, MNTOPT_RSIZE, len))
				opterr(p);
			args.rsize = atoi(&p[len]);
			/*
			 * has to be a multiple of 512
			 */
			if (args.rsize < MINRSIZE || args.rsize > MAXRSIZE || 
			    args.rsize % MINRSIZE)
				opterr(p);
			args.flags |= NFSMNT_RSIZE;
			break;
		case 'e':
			if (*(p+2) != 't' || *(p+3) != 'r')
				opterr(p);
			switch (*(p+4)) {
			case 'a':	/* retrans= */
				len = strlen(MNTOPT_RETRANS);
				if (strncmp(p, MNTOPT_RETRANS, len))
					opterr(p);
				args.retrans = atoi(&p[len]);
				if (args.retrans < MINRETRANS ||
					args.retrans > MAXRETRANS)
					opterr(p);
				args.flags |= NFSMNT_RETRANS;
				break;
			case 'y':	/* retry= */
				len = strlen(MNTOPT_RETRY);
				if (strncmp(p, MNTOPT_RETRY, len))
					opterr(p);
				retry = atoi(&p[len]);
				if (retry < MINRETRY || retry > MAXRETRY)
					opterr(p);
				break;
			default:
				opterr (p);
			}
			break;
		default:
			opterr(p);
		}
		break;
	case 's':
		switch (*(p+1)) {
		case 'o':	/* soft */
			if (strcmp(p, MNTOPT_SOFT))
				opterr(p);
			args.flags |= NFSMNT_SOFT;
			break;
		case 'y':	/* sync */
			if (strcmp(p, MNTOPT_SYNC))
				opterr(p);
			args.gfs_flags |= M_SYNC;
			break;
		default:
			opterr(p);
		}
		break;
	case 't':	/* timeo= */
		len = strlen(MNTOPT_TIMEO);
		if (strncmp(p, MNTOPT_TIMEO, len))
			opterr(p);
		args.timeo = atoi(&p[len]);
		if (args.timeo < MINTIMEO || args.timeo > MAXTIMEO)
			opterr(p);
		args.flags |= NFSMNT_TIMEO;
		break;
	case 'w':	/* wsize= */
		len = strlen(MNTOPT_WSIZE);
		if (strncmp(p, MNTOPT_WSIZE, len))
			opterr(p);
		args.wsize = atoi(&p[len]);
		/*
		 * has to be a multiple of 512
		 */
		if (args.wsize < MINWSIZE || args.wsize> MAXWSIZE ||
		    args.wsize % MINWSIZE)
			opterr(p);
		args.flags |= NFSMNT_WSIZE;
		break;
	default:
		opterr(p);
	}

	if (q < r) {
		p = q;
		goto top;
	}

	/*
	 * Check that attributes caching values make sense
	 *	noac and actimeo should not appear together and 
	 *	if any of the 4 min/max values are specified, then
	 *	neither noac or actimeo would make sense.
	 */
	if (((args.flags & NFSMNT_NOAC) == NFSMNT_NOAC) && actimeo) {
		(void) fprintf(stderr,
		 	"nfs_mount: contradicting attr cache options\n");
		exit(1);
	}
	if (acmod && (actimeo || ((args.flags & NFSMNT_NOAC) == NFSMNT_NOAC))) {
		(void) fprintf(stderr,
		 	"nfs_mount: contradicting attr cache options\n");
		exit(1);
	}
	if ((args.acregmax < args.acregmin) || 
	      (args.acdirmax < args.acdirmin)) {
		(void) fprintf(stderr,
		 	"nfs_mount: recheck attributes cache values\n");
		exit(1);
	}
	return(0);
}

opterr(opt)
char *opt;
{
	(void) fprintf(stderr,
		"nfs_mount: invalid -o option \"%s\"\n", opt);
	exit(1);
}
