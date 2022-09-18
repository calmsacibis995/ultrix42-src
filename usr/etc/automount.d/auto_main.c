#ifndef lint
static char *sccsid = "@(#)auto_main.c	4.3      (ULTRIX)        2/1/91";
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
 *	Copyright (c) 1987 Sun Microsystems, Inc.  ALL RIGHTS RESERVED.
 */
/*
 *	Modification History:
 * 
 *	17 Apr 90 -- condylis
 *		Fixed handling of SIGTERM in catch().  Added changing
 *		of modification time to insure umount succeeds.
 *
 *      10 Nov 89 -- lebel
 *              Incorporated direct maps, bugfixes, metacharacter handling 
 *              and other fun stuff from the reference tape.
 * 	14 Jun 89 -- condylis
 *		Added copyright header.
 *
 */

#include <stdio.h>
#include <ctype.h>
#include <syslog.h>
#include <errno.h>
#include <string.h>
#include <rpc/rpc.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include "nfs_prot.h"
#include <netinet/in.h>
#include <netdb.h>
#include <sys/signal.h>
#include <sys/file.h>
#include <rpcsvc/yp_prot.h>
#include <rpcsvc/ypclnt.h>
#include <nfs/nfs_clnt.h>

#define NFSCLIENT
typedef nfs_fh fhandle_t;
#include <sys/param.h>
#include <sys/mount.h>
#include <sys/fs_types.h>
#include <nfs/nfs_gfs.h>
#include "automount.h"

extern errno;

void catch();
void reap();
void set_timeout();
void loadmaster_file();
void loadmaster_yp();

#define	MAXDIRS	10

#define	MASTER_MAPNAME	"auto.master"
#define    INADDR_LOOPBACK         (u_long)0x7F000001

int maxwait = 60;
int mount_timeout = 30;
int max_link_time = 5*60;
int verbose;

u_short myport;

main(argc, argv)
	int argc;
	char *argv[];
{
	SVCXPRT *transp;
	extern void nfs_program_2();
	extern void check_mtab();
	static struct sockaddr_in sin;	/* static causes zero init */
	struct nfs_gfs_mount args;
	struct autodir *dir, *dir_next;
	int pid;
	int bad;
	int master_yp = 1;
	char *master_file;
	char *myname = argv[0];
	struct hostent *hp;
	extern int trace;
	char pidbuf[64];


	if (geteuid() != 0) {
		fprintf(stderr, "Must be root to use automount\n");
		exit(1);
	}

	argc--;
	argv++;

	openlog("automount", LOG_PID);

	/* Unbuffered I/O */
	(void) setbuf(stdout, (char *)NULL);
	(void) gethostname(self, sizeof self);
	hp = gethostbyname(self);
	if (hp == NULL) {
		syslog(LOG_ERR, "Can't get my address");
		exit(1);
	}
	bcopy(hp->h_addr, (char *)&my_addr, hp->h_length);
	(void) getdomainname(mydomain, sizeof mydomain);
	if (mydomain[0] == NULL)
		master_yp = 0;
	else if (bad = yp_bind(mydomain)) {
		syslog(LOG_ERR, "YP bind failed: %s", yperr_string(bad));
		master_yp = 0;
	}

	(void) strcpy(tmpdir, "/tmp_mnt");
	master_file = NULL;

	while (argc && argv[0][0] == '-') switch (argv[0][1]) {
	case 'n':
		nomounts++;
		argc--;
		argv++;
		break;
	case 'm':
		master_yp = 0;
		argc--;
		argv++;
		break;
	case 'f':
		master_file = argv[1];
		argc -= 2;
		argv += 2;
		break;
	case 'M':
		(void) strcpy(tmpdir, argv[1]);
		argc -= 2;
		argv += 2;
		break;
	case 't':			/* timeouts */
		if (argc < 2) {
			(void) fprintf(stderr, "Bad timeout value\n");
			usage();
		}
		if (argv[0][2]) {
			set_timeout(argv[0][2], atoi(argv[1]));
		} else {
			char *s;

			for (s = strtok(argv[1], ","); s ;
				s = strtok(NULL, ",")) {
				if (*(s+1) != '=') {
					(void) fprintf(stderr,
						"Bad timeout value\n");
					usage();
				}
				set_timeout(*s, atoi(s+2));
			}
		}
		argc -= 2;
		argv += 2;
		break;

	case 'T':
/* temporary */
		trace++;
		argc--;
		argv++;
		break;

	case 'D':
		if (argv[0][2])
			(void) putenv(&argv[0][2]);
		else {
			(void) putenv(argv[1]);
			argc--;
			argv++;
		}
		argc--;
		argv++;
		break;

	case 'v':
		verbose++;
		argc--;
		argv++;
		break;

	default:
		usage();
	}

	if (verbose && argc == 0 && master_yp == 0 && master_file == NULL) {
		syslog(LOG_ERR, "no mount maps specified");
		usage();
	}

	check_mtab();
	/*
	 * Get mountpoints and maps off the command line
	 */
	while (argc >= 2) {
		if (argc >= 3 && argv[2][0] == '-') {
			dirinit(argv[0], argv[1], argv[2]+1, 0);
			argc -= 3;
			argv += 3;
		} else {
			dirinit(argv[0], argv[1], "rw", 0);
			argc -= 2;
			argv += 2;
		}
	}
	if (argc)
		usage();

#ifdef SET_ARCH     /* non-SunOS systems may not have 'arch' script */
	if (getenv("ARCH") == NULL) {
		char buf[16], str[32];
		int len;
		FILE *f;

		f = popen("arch", "r");
		(void) fgets(buf, 16, f);
		(void) pclose(f);
		if (len = strlen(buf))
			buf[len - 1] = '\0';
		(void) sprintf(str, "ARCH=%s", buf);
		(void) putenv(str);
	}
#endif /* def SET_ARCH */
	
	if (master_file) {
		loadmaster_file(master_file);
	}
	if (master_yp) {
		loadmaster_yp(MASTER_MAPNAME);
	}

	/*
	 * Remove -null map entries
	 */
	for (dir = HEAD(struct autodir, dir_q); dir; dir = dir_next) {
	    	dir_next = NEXT(struct autodir, dir);
		if (strcmp(dir->dir_map, "-null") == 0) {
			REMQUE(dir_q, dir);
		}
	}
	if (HEAD(struct autodir, dir_q) == NULL)   /* any maps ? */
		exit(1);

	transp = svcudp_create(RPC_ANYSOCK);
	if (transp == NULL) {
		syslog(LOG_ERR, "Cannot create UDP service");
		exit(1);
	}
	if (!svc_register(transp, NFS_PROGRAM, NFS_VERSION, nfs_program_2, 0)) {
		syslog(LOG_ERR, "svc_register failed");
		exit(1);
	}

#ifdef DEBUG
	pid = getpid();
	if (fork()) {
		/* parent */
		signal(SIGTERM, catch);
		signal(SIGHUP,  check_mtab);
		signal(SIGCHLD, reap);
		auto_run();
		syslog(LOG_ERR, "svc_run returned");
		exit(1);
	}
#else NODEBUG
	switch (pid = fork()) {
	case -1:
		syslog(LOG_ERR, "Cannot fork: %m");
		exit(1);
	case 0:
		/* child */
		{ int tt = open("/dev/tty", O_RDWR);
		  if (tt > 0) {
			(void) ioctl(tt, TIOCNOTTY, (char *)0);
			(void) close(tt);
		  }
		}
		signal(SIGTERM, catch);
		signal(SIGHUP, check_mtab);
		signal(SIGCHLD, reap);
		auto_run();
		syslog(LOG_ERR, "svc_run returned");
		exit(1);
	}
#endif

	/* parent */
	sin.sin_family = AF_INET;
	sin.sin_port = htons(transp->xp_port);
	myport = transp->xp_port;
	sin.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	bzero(sin.sin_zero, sizeof(sin.sin_zero));
	args.addr = &sin;
	args.flags = NFSMNT_INT + NFSMNT_TIMEO +
		     NFSMNT_HOSTNAME + NFSMNT_RETRANS;
	args.timeo = (mount_timeout + 5) * 10;
	args.retrans = 5;
	bad = 1;

	/*
	 * Mount the daemon at it's mount points.
	 * Start at the end of the list because that's
	 * the first on the command line.
	 */
	for (dir = TAIL(struct autodir, dir_q); dir;
	    dir = PREV(struct autodir, dir)) {
		(void) sprintf(pidbuf, "(pid%d@%s)", pid, dir->dir_name);
		if (strlen(pidbuf) >= HOSTNAMESZ-1)
			(void) strcpy(&pidbuf[HOSTNAMESZ-3], "..");
		args.hostname = pidbuf;
		args.fh = (fhandle_t *)&dir->dir_vnode.vn_fh; 
#ifdef OLDMOUNT
#define MOUNT_NFS 1
		if (mount(args.hostname, dir->dir_name, M_RONLY, GT_NFS, &args)) {
#else
		if (mount(args.hostname, dir->dir_name, M_RONLY, GT_NFS, &args)) {
#endif
			syslog(LOG_ERR, "Can't mount %s: %m", dir->dir_name);
			bad++;
		} else {
			/* domntent(pid, dir); */
			bad = 0;
		}
	}
	if (bad)
		(void) kill(pid, SIGKILL);
	exit(bad);
	/*NOTREACHED*/
}

void
set_timeout(letter, t)
	char letter ; int t;
{
	if (t <= 0) {
		(void) fprintf(stderr, "Bad timeout value\n");
		usage();
	}
	switch (letter) {
	case 'm':
		mount_timeout = t;
		break;
	case 'l':
		max_link_time = t;
		break;
	case 'w':
		maxwait = t;
		break;
	default:
		(void) fprintf(stderr, "automount: bad timeout switch\n");
		usage();
		break;
	}
}


void
catch()
{
	struct autodir *dir;
	int child;
	struct filsys *fs, *fs_next;
	struct fs_data fsdata;
	struct stat stbuf;
	struct fattr *fa;

	/*
	 *  The automounter has received a SIGTERM.
	 *  Here it forks a child to carry on servicing
	 *  its mount points in order that those
	 *  mount points can be unmounted.  The child
	 *  checks for symlink mount points and changes them
	 *  into directories to prevent the unmount system
	 *  call from following them.
	 */
	signal(SIGTERM, SIG_IGN);
	if ((child = fork()) == 0) {
		for (dir = HEAD(struct autodir, dir_q); dir;
		    dir = NEXT(struct autodir, dir)) {
			if (dir->dir_vnode.vn_type != VN_LINK)
				continue;

			dir->dir_vnode.vn_type = VN_DIR;
			fa = &dir->dir_vnode.vn_fattr;
			fa->type = NFDIR;
			fa->mode = NFSMODE_DIR + 0555;
			fa->mtime.seconds++;
		}
		return;
	}

	for (dir = HEAD(struct autodir, dir_q); dir;
	    dir = NEXT(struct autodir, dir)) {

		/*  This lstat is a kludge to force the kernel
		 *  to flush the attr cache.  If it was a direct
		 *  mount point (symlink) the kernel needs to
		 *  do a getattr to find out that it has changed
		 *  back into a directory.
		 */
		if (lstat(dir->dir_name, &stbuf) < 0) {
			syslog(LOG_ERR, "lstat %s: %m", dir->dir_name);
		}

               if (getmnt(0, &fsdata, 0, NOSTAT_ONE, dir->dir_name) < 1) {
                       syslog(LOG_ERR, "getmnt %s: %m", dir->dir_name);
                       continue;
                }
   
		if (umount(fsdata.fd_req.dev) < 0) {
			if (errno != EBUSY)
				syslog(LOG_ERR, "umount %s: %m", dir->dir_name);
		} else {
			if (dir->dir_remove)
				(void) rmdir(dir->dir_name);
		}
	}
	(void) kill (child, SIGKILL);

	/*
	 *  Unmount any mounts done by the automounter
	 */
	for (fs = HEAD(struct filsys, fs_q); fs; fs = fs_next) {
		fs_next = NEXT(struct filsys, fs);
		if (fs->fs_mine && fs == fs->fs_rootfs) {
			if (do_unmount(fs))
				fs_next = HEAD(struct filsys, fs_q);
		}
	}

	syslog(LOG_ERR, "exiting");
	exit(0);
}

void
reap()
{
	while (wait3((union wait *)0, WNOHANG, (struct rusage *)0) > 0)
		;
}

auto_run()
{
	int read_fds, n;
	time_t time();
	long last;
	struct timeval tv;

	last = time((long *)0);
	tv.tv_sec = maxwait;
	tv.tv_usec = 0;
	for (;;) {
		read_fds = svc_fds;
		n = select(32, &read_fds, (int *)0, (int *)0, &tv);
		time_now = time((long *)0);
		if (n)
			svc_getreq(read_fds);
		if (time_now >= last + maxwait) {
			last = time_now;
			do_timeouts();
		}
	}
}

usage()
{
	fprintf(stderr, "Usage: automount\n%s%s%s%s%s%s%s%s%s%s%s",
		"\t[-n]\t\t(no mounts)\n",
		"\t[-m]\t\t(no auto.master)\n",
		"\t[-T]\t\t(trace nfs requests)\n",
		"\t[-v]\t\t(verbose error msgs)\n",
		"\t[-tl n]\t\t(mount duration)\n",
		"\t[-tm n]\t\t(attempt interval)\n",
		"\t[-tw n]\t\t(unmount interval)\n",
		"\t[-M dir]\t(temporary mount dir)\n",
		"\t[-D n=s]\t(define env variable)\n",
		"\t[-f file]\t(get mntpnts from file)\n",
		"\t[ dir map [-mntopts] ] ...\n");
	exit(1);
}

void
loadmaster_yp(mapname)
	char *mapname;
{
	int first, err;
	char *key, *nkey, *val;
	int kl, nkl, vl;
	char dir[100], map[100];
	char *p, *opts;


	first = 1;
	key  = NULL; kl  = 0;
	nkey = NULL; nkl = 0;
	val  = NULL; vl  = 0;

	for (;;) {
		if (first) {
			first = 0;
			err = yp_first(mydomain, mapname, &nkey, &nkl, &val, &vl);
		} else {
			err = yp_next(mydomain, mapname, key, kl, &nkey, &nkl,
				&val, &vl);
		}
		if (err) {
			if (err != YPERR_NOMORE && err != YPERR_MAP)
				syslog(LOG_ERR, "%s: %s",
					mapname, yperr_string(err));
			return;
		}
		if (key)
			free(key);
		key = nkey;
		kl = nkl;

		if (kl >= 100 || vl >= 100)
			return;
		if (kl < 2 || vl < 1)
			return;
		if (isspace(*key) || *key == '#')
			return;
		(void) strncpy(dir, key, kl);
		dir[kl] = '\0';
		(void) strncpy(map, val, vl);
		map[vl] = '\0';
		p = map;
		while (*p && !isspace(*p))
			p++;
		opts = "rw";
		if (*p) {
			*p++ = '\0';
			while (*p && isspace(*p))
				p++;
			if (*p == '-')
				opts = p+1;
		}

		dirinit(dir, map, opts, 0);

		free(val);
	}
}

void
loadmaster_file(masterfile)
	char *masterfile;
{
	FILE *fp;
	char *line, *dir, *map, *opts;
	extern char *get_line();
	char linebuf[1024];

	if ((fp = fopen(masterfile, "r")) == NULL) {
		syslog(LOG_ERR, "%s:%m", masterfile);
		return;
	}

	while ((line = get_line(fp, linebuf, sizeof linebuf)) != NULL) {
		dir = line;
		while (*dir && isspace(*dir)) dir++;
		if (*dir == '\0')
			continue;
		map = dir;
		while (*map && !isspace(*map)) map++;
		if (*map)
			*map++ = '\0';
		if (*dir == '+') {
			loadmaster_yp(dir+1);
		} else {
			while (*map && isspace(*map)) map++;
			if (*map == '\0')
				continue;
			opts = map;
			while (*opts && !isspace(*opts)) opts++;
			if (*opts) {
				*opts++ = '\0';
				while (*opts && isspace(*opts)) opts++;
			}
			if (*opts != '-')
				opts = "-rw";
			
			dirinit(dir, map, opts+1, 0);
		}
	}
	(void) fclose(fp);
}
