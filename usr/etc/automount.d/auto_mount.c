#ifndef lint
static char *sccsid = "@(#)auto_mount.c	4.6      (ULTRIX)        4/11/91";
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
 *	9 Mar 91 -- condylis
 *		Brought in nfssrc4.1 do_mount() code to solve 
 *		subdirectory bugs.
 *
 *	27 Dec 90 -- condylis
 *		Fixed bug in creating symbolic link to file system already 
 *		mounted.  Added storing dev number in filsys structure at
 *		mount time to prevent having to call getmnt to get it at
 *		umount time.
 *		
 *	4 Sep 90 -- condylis
 *		Changed calls to rpc client error routines to use newer
 *		interface.
 *
 *	24 May 90 -- condylis
 *		Fixed bugs in passing of option flags to mount system
 *		call.
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
#include <sys/param.h>
#include <sys/errno.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <sys/signal.h>
#include <rpc/rpc.h>
#include <rpc/pmap_clnt.h>
#include <netdb.h>
#include <errno.h>
#include "nfs_prot.h"
typedef nfs_fh fhandle_t;
#include <rpcsvc/mount.h>
#define NFSCLIENT
#include <sys/mount.h>
#include <nfs/nfs_gfs.h>
#include <nfs/nfs_clnt.h>
#include <nfs/vfs.h>
#include "automount.h"
#include "mntent.h"

#define MAXHOSTS  20

struct mapfs *find_server();
struct filsys *already_mounted();
void addtomtab();
char *inet_ntoa();
extern int trace;
extern int verbose;

nfsstat
do_mount(dir, me, rootfs, linkpath)
	struct autodir *dir;
	struct mapent *me;
	struct filsys **rootfs;
	char **linkpath;
{
	char mntpnt[MAXPATHLEN];
	static char linkbuf[MAXPATHLEN];
	enum clnt_stat pingmount();
	struct filsys *fs, *tfs;
	struct mapfs *mfs, *rmfs;
	struct mapent *m, *mapnext;
	nfsstat status = NFSERR_NOENT;
	struct in_addr prevhost;
	int imadeit;
	struct stat stbuf;
	char *roothost = "";

	*rootfs = NULL;
	*linkpath = "";
	prevhost.s_addr = (u_long)0;

        for (m = me ; m ; m = mapnext) {
                mapnext = m->map_next;

                tfs = NULL;
                mfs = find_server(m, &tfs, m == me, prevhost);
                if (mfs == NULL)
                        continue;

		if (m == me) {
                        for (fs = HEAD(struct filsys, fs_q); fs;
                                fs = NEXT(struct filsys, fs)) {
				for (rmfs = me->map_fs; rmfs; rmfs = rmfs->mfs_next) {
					(void) sprintf(mntpnt, "%s%s/%s%s%s", 
					tmpdir, dir->dir_name, rmfs->mfs_host, 
					me->map_root, m->map_mntpnt);
                                	if (strcmp(mntpnt, fs->fs_mntpnt) == 0) {
                                        	(void) sprintf(linkbuf, 
							"%s%s/%s%s%s",
                                                	tmpdir,
                                                	dir->dir_name,
							rmfs->mfs_host,
                                                	me->map_root,
                                                	me->map_fs->mfs_subdir);
                                        	if (trace > 1)
                                                	(void) fprintf(stderr,
                                                	"renew link for %s\n",
                                                	linkbuf);
                                        	*linkpath = linkbuf;
                                        	return (NFS_OK);
                                	}
				}
                        }
			roothost = mfs->mfs_host;
			(void) sprintf(mntpnt, "%s%s/%s%s%s", tmpdir,
                        dir->dir_name, mfs->mfs_host, me->map_root, m->map_mntpnt);
		} else {
			(void) sprintf(mntpnt, "%s%s/%s%s%s", tmpdir,
			dir->dir_name, roothost, me->map_root, m->map_mntpnt);
		}


                /*
                 * It may be possible to return a symlink
                 * to an existing mount point without
                 * actually having to do a mount.
                 */
                if (me->map_next == NULL && *me->map_mntpnt == '\0') {

                        /* Is it my host ? */
                        if (mfs->mfs_addr.s_addr == my_addr.s_addr) {
                                (void) strcpy(linkbuf, mfs->mfs_dir);
                                (void) strcat(linkbuf, mfs->mfs_subdir);
                                *linkpath = linkbuf;
                                if (trace > 1)
                                        (void) fprintf(stderr,
                                                "It's on my host\n");
                                return (NFS_OK);
                        }

                        /*
                         * An existing mount point ?
                         * XXX Note: this is a bit risky - the
                         * mount may belong to another automount
                         * daemon - it could unmount it anytime and
                         * this symlink would then point to an empty
                         * or non-existent mount point.
                         */
                        if (tfs != NULL) {
                                if (trace > 1)
                                        (void) fprintf(stderr,
                                        "already mounted %s:%s on %s (%s)\n",
                                        tfs->fs_host, tfs->fs_dir,
                                        tfs->fs_mntpnt, tfs->fs_opts);

                                (void) strcpy(linkbuf, tfs->fs_mntpnt);
                                (void) strcat(linkbuf, mfs->mfs_subdir);
                                *linkpath = linkbuf;
                                *rootfs = tfs;
                                return (NFS_OK);
                        }
                }

                if (nomounts)
                        return (NFSERR_PERM);

                 /* Create the mount point if necessary */

                imadeit = 0;
                if (stat(mntpnt, &stbuf) != 0) {
                        if (mkdir_r(mntpnt) == 0) {
                                imadeit = 1;
                        } else {
                                if (verbose)
                                        syslog(LOG_ERR,
                                        "Couldn't create mountpoint %s: %m",
                                        mntpnt);
                                if (trace > 1)
                                        (void) fprintf(stderr,
                                        "couldn't create mntpnt %s\n",
                                        mntpnt);
                                continue;
                        }
                }

                if (pingmount(mfs->mfs_addr) != RPC_SUCCESS) {
                        syslog(LOG_ERR, "host %s not responding", mfs->mfs_host) ;
			prevhost.s_addr = (u_long)0;
                        continue;
                }


                /*  Now do the mount */

                tfs = NULL;
                status = nfsmount(mfs->mfs_host, mfs->mfs_addr, mfs->mfs_dir,
                                mntpnt, m->map_mntopts, &tfs);
                if (status == NFS_OK) {
                        if (*rootfs == NULL) {
                                *rootfs = tfs;
                                (void) sprintf(linkbuf, "%s%s/%s%s%s",
                                        tmpdir, dir->dir_name,
                                        mfs->mfs_host, me->map_root,
                                        mfs->mfs_subdir);
                                *linkpath = linkbuf;
                        }
                        tfs->fs_rootfs = *rootfs;
                        prevhost.s_addr = mfs->mfs_addr.s_addr;
                } else {
                        if (imadeit)
                                safe_rmdir(mntpnt);
                }
        }
        if (*rootfs != NULL) {
                return (NFS_OK);
        }
        return (status);


}

struct mapfs *
find_server(me, fsp, rootmount, preferred)
	struct mapent *me;
	struct filsys **fsp;
	int rootmount;
	struct in_addr preferred;
{
	int entrycount, localcount;
	struct mapfs *mfs;
	struct hostent *hp;
	struct in_addr addrs[MAXHOSTS], addr, *addrp, trymany();


	/*
	 * get addresses & see if any are myself
	 * or were mounted from previously in a
	 * hierarchical mount.
	 */
	entrycount = localcount = 0;
	for (mfs = me->map_fs; mfs; mfs = mfs->mfs_next) {
		if (mfs->mfs_addr.s_addr == 0) {
			hp = gethostbyname(mfs->mfs_host);
			if (hp)
				bcopy(hp->h_addr, (char *)&mfs->mfs_addr,
					hp->h_length);
		}
		if (mfs->mfs_addr.s_addr) {
			entrycount++;
			if (mfs->mfs_addr.s_addr == my_addr.s_addr)
				return mfs;
			if (mfs->mfs_addr.s_addr == preferred.s_addr)
				return mfs;
		}
	}
	if (entrycount == 0)
		return NULL;

	/* see if any already mounted */
	if (rootmount)
	    for (mfs = me->map_fs; mfs; mfs = mfs->mfs_next) {
		    if (*fsp = already_mounted(mfs->mfs_host, mfs->mfs_dir,
			me->map_mntopts, mfs->mfs_addr)) 
			    return mfs;
	    }

	if (entrycount == 1) 	/* no replication involved */
		return me->map_fs;

	/* try local net */
	addrp = addrs;
	for (mfs = me->map_fs; mfs; mfs = mfs->mfs_next) {
		if (inet_netof(mfs->mfs_addr) == inet_netof(my_addr)) {
			localcount++;
			*addrp++ = mfs->mfs_addr;
		}
	}
	if (localcount > 0) {	/* got some */
		(*addrp).s_addr = 0;	/* terminate list */
		addr = trymany(addrs, mount_timeout / 2);
		if (addr.s_addr) {	/* got one */
			for (mfs = me->map_fs; mfs; mfs = mfs->mfs_next)
			if (addr.s_addr == mfs->mfs_addr.s_addr)
				return mfs;
		}
	}
	if (entrycount == localcount)
		return NULL;

	/* now try them all */
	addrp = addrs;
	for (mfs = me->map_fs; mfs; mfs = mfs->mfs_next)
		*addrp++ = mfs->mfs_addr;
	(*addrp).s_addr = 0;	/* terminate list */
	addr = trymany(addrs, mount_timeout / 2);
	if (addr.s_addr) {	/* got one */
		for (mfs = me->map_fs; mfs; mfs = mfs->mfs_next)
		if (addr.s_addr == mfs->mfs_addr.s_addr)
				return mfs;
	}
	return NULL;
}

/*
 *  If mount table has changed update internal fs info
 */
void
check_mtab()
{
	struct stat stbuf;
	struct filsys *fs, *fsnext;
	char *index();
	int found, has1, has2;
	struct hostent *hp;
	struct v_fs_data tfs;
	struct v_fs_data *buffer = &tfs;
	int rtn, start=0;

	/* reset the present flags */
	for (fs = HEAD(struct filsys, fs_q); fs;
	    fs = NEXT(struct filsys, fs))
		    fs->fs_present = 0;

	/* now see what's been mounted */
	while ((rtn = getmnt(&start, buffer, sizeof(struct fs_data),
NOSTAT_MANY, NULL)) > 0) {
		char *tmphost, *tmppath, *p, tmpc;

/* XXX will fix later */
		if (buffer->fd_fstype != 0x05)
			continue;
		p = index(buffer->fd_devname, ':');
		if (p == NULL)
			continue;
		tmpc = *p;
		*p = '\0';
		tmphost = buffer->fd_devname;
		tmppath = p+1;
		if (tmppath[0] != '/')
			continue;
		found = 0;
/* lebel - compare other opts too */
		for (fs = HEAD(struct filsys, fs_q); fs;
			fs = NEXT(struct filsys, fs)) {
			if (strcmp(buffer->fd_path, fs->fs_mntpnt) == 0 &&
			    strcmp(tmphost, fs->fs_host) == 0 &&
			    strcmp(tmppath, fs->fs_dir) == 0 &&
			    same_opts(buffer->fd_un.gvfs.mi.mi_optstr, fs->fs_opts)) { 
				fs->fs_present = 1;
				found++;
				break;
			}
		}
		if (!found) {
			fs = alloc_fs(tmphost, tmppath,
				buffer->fd_path, buffer->fd_un.gvfs.mi.mi_optstr);
			if (fs == NULL)
				return;
			fs->fs_present = 1;
			hp = gethostbyname(tmphost);
			if (hp != NULL) {
				bcopy(hp->h_addr, &fs->fs_addr.sin_addr, hp->h_length);
			}
		}
		*p = tmpc;
	}

	/* free fs's that are no longer present */
	for (fs = HEAD(struct filsys, fs_q); fs; fs = fsnext) {
		fsnext = NEXT(struct filsys, fs);
		if (!fs->fs_present) {
			flush_links(fs);
			free_filsys(fs);
		}
	}
}

/*
 * Search the mount table to see if the given file system is already
 * mounted. 
 */
struct filsys *
already_mounted(host, fsname, opts, hostaddr)
	char *host;
	char *fsname;
	char *opts;
	struct in_addr hostaddr;
{
	struct filsys *fs;
	struct mntent m1, m2;
	int has1, has2;
	struct autodir *dir;
	int mydir;

	check_mtab();
	m1.mnt_opts = opts;
	for (fs = HEAD(struct filsys, fs_q); fs; fs = NEXT(struct filsys, fs)){
		if (strcmp(fsname, fs->fs_dir) != 0)
			continue;
		if (hostaddr.s_addr != fs->fs_addr.sin_addr.s_addr)
			continue;

		/*
		 * Check it's not on one of my mount points.
		 * I might be mounted on top of a previous 
		 * mount of the same file system.
		 */

		for (mydir = 0, dir = HEAD(struct autodir, dir_q); dir;
			dir = NEXT(struct autodir, dir)) {
			if (strcmp(dir->dir_name, fs->fs_mntpnt) == 0) {
				mydir = 1;
				if (verbose)
					syslog(LOG_ERR,
					"%s:%s already mounted on %s",
					host, fsname, fs->fs_mntpnt);
				break;
			}
		}
		if (mydir)
			continue;

/* lebel - should be comparing other opts too */
		m2.mnt_opts = fs->fs_opts;
		has1 = hasmntopt(&m1, MNTOPT_RO) != NULL;
		has2 = hasmntopt(&m2, MNTOPT_RO) != NULL;
		if (has1 != has2)
			continue;
		has1 = hasmntopt(&m1, MNTOPT_NOSUID) != NULL;
		has2 = hasmntopt(&m2, MNTOPT_NOSUID) != NULL;
		if (has1 != has2)
			continue;
		has1 = hasmntopt(&m1, MNTOPT_SOFT) != NULL;
		has2 = hasmntopt(&m2, MNTOPT_SOFT) != NULL;
		if (has1 != has2)
			continue;
		has1 = hasmntopt(&m1, MNTOPT_INT) != NULL;
		has2 = hasmntopt(&m2, MNTOPT_INT) != NULL;
		if (has1 != has2)
			continue;

		return (fs);
	}
	return(0);
}

nfsunmount(fs)
	struct filsys *fs;
{
	struct sockaddr_in sin;
	struct timeval timeout;
	CLIENT *client;
	enum clnt_stat rpc_stat;
	int s;

	sin = fs->fs_addr;
	/*
	 * Port number of "fs->fs_addr" is NFS port number; make port
	 * number 0, so "clntudp_create" finds port number of mount
	 * service.
	 */
	sin.sin_port = 0;
	s = RPC_ANYSOCK;
	timeout.tv_usec = 0;
	timeout.tv_sec = 10;
	if ((client = clntudp_create(&sin, MOUNTPROG, MOUNTVERS,
	    timeout, &s)) == NULL) {
		syslog(LOG_WARNING, "%s:%s %s",
		    fs->fs_host, fs->fs_dir,
		    clnt_spcreateerror("server not responding"));
		return;
	}
#ifdef OLDMOUNT
	if (bindresvport(s, NULL))
		syslog(LOG_ERR, "Warning: cannot do local bind");
#endif
	client->cl_auth = authunix_create_default();
	timeout.tv_usec = 0;
	timeout.tv_sec = 25;
	rpc_stat = clnt_call(client, MOUNTPROC_UMNT, xdr_path, &fs->fs_dir,
	    xdr_void, (char *)NULL, timeout);
	(void) close(s);
	AUTH_DESTROY(client->cl_auth);
	clnt_destroy(client);
	if (rpc_stat != RPC_SUCCESS)
		syslog(LOG_WARNING, "%s", clnt_sperror(client, "unmount"));
}

enum clnt_stat
pingmount(hostaddr)
	struct in_addr hostaddr;
{
	struct timeval tottime;
	struct sockaddr_in server_addr;
	enum clnt_stat clnt_stat;
	u_long port;
	static struct in_addr goodhost, deadhost;
	static time_t goodtime, deadtime;
	int cache_time = 60;  /* sec */

	if (goodtime > time_now && hostaddr.s_addr == goodhost.s_addr)
			return RPC_SUCCESS;
	if (deadtime > time_now && hostaddr.s_addr == deadhost.s_addr)
			return RPC_TIMEDOUT;

	if (trace > 1)
		fprintf(stderr, "ping %s ", inet_ntoa(hostaddr));

	tottime.tv_sec = 10;
	tottime.tv_usec = 0;

	/* 
	 * We use the pmap_rmtcall interface because the normal
	 * portmapper has a wired-in 60 second timeout
	 */
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr = hostaddr;
	server_addr.sin_port = 0;
	clnt_stat = pmap_rmtcall(&server_addr, MOUNTPROG, MOUNTVERS, NULLPROC,
			xdr_void, 0, xdr_void, 0, tottime, &port);

	if (clnt_stat == RPC_SUCCESS) {
		goodhost = hostaddr;
		goodtime = time_now + cache_time;
	} else {
		deadhost = hostaddr;
		deadtime = time_now + cache_time;
	}

	if (trace > 1)
		fprintf(stderr, "%s\n", clnt_stat == RPC_SUCCESS ?
			"OK" : "NO RESPONSE");

	return (clnt_stat);
}

struct in_addr gotaddr;

/* ARGSUSED */
catchfirst(ignore, raddr)
	struct sockaddr_in *raddr;
{
	gotaddr = raddr->sin_addr;
	return (1);	/* first one ends search */
}

/*
 * ping a bunch of hosts at once and find out who
 * responds first
 */
struct in_addr
trymany(addrs, timeout)
	struct in_addr *addrs;
	int timeout;
{
	enum clnt_stat many_cast();
	enum clnt_stat clnt_stat;

	if (trace > 1) {
		register struct in_addr *a;

		fprintf(stderr, "many_cast: ");
		for (a = addrs ; a->s_addr ; a++)
			fprintf(stderr, "%s ", inet_ntoa(*a));
		fprintf(stderr, "\n");
	}
		
	gotaddr.s_addr = 0;
	clnt_stat = many_cast(addrs, MOUNTPROG, MOUNTVERS, NULLPROC,
			xdr_void, 0, xdr_void, 0, catchfirst, timeout);
	if (trace > 1) {
		fprintf(stderr, "many_cast: got %s\n",
			(int) clnt_stat ? "no response" : inet_ntoa(gotaddr));
	}
	if (clnt_stat)
		syslog(LOG_ERR, "trymany: servers not responding: %s",
			clnt_sperrno(clnt_stat));
	return (gotaddr);
}

nfsstat
nfsmount(host, hostaddr, dir, mntpnt, opts, fsp)
	char *host, *dir, *mntpnt, *opts;
	struct in_addr hostaddr;
	struct filsys **fsp;
{
	struct filsys *fs;
	char remname[MAXPATHLEN];
	struct mntent m;
	struct nfs_gfs_mount args;
	int flags;
	struct sockaddr_in sin;
	static struct fhstatus fhs;
	struct timeval timeout;
	CLIENT *client;
	enum clnt_stat rpc_stat;
	enum clnt_stat pingmount();
	int s;
	u_short port;
	nfsstat status;
	struct stat stbuf;
	struct fs_data fsdata;

	if (lstat(mntpnt, &stbuf) < 0) {
		syslog(LOG_ERR, "Couldn't stat %s: %m", mntpnt);
		return (NFSERR_NOENT);
	}
	if ((stbuf.st_mode & S_IFMT) == S_IFLNK) {
		if (readlink(mntpnt, remname, sizeof remname) < 0)
			return (NFSERR_NOENT);
		if (remname[0] == '/') {
			syslog(LOG_ERR,
				"%s -> %s from %s: absolute symbolic link",
				mntpnt, remname, host);
			return (NFSERR_NOENT);
		}
	}

	/*
	 * get fhandle of remote path from server's mountd
	 */
	(void) sprintf(remname, "%s:%s", host, dir);
	bzero((char *)&sin, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = hostaddr.s_addr;
	timeout.tv_usec = 0;
	timeout.tv_sec = mount_timeout;
	s = RPC_ANYSOCK;
	if ((client = clntudp_create(&sin, MOUNTPROG, MOUNTVERS,
	    timeout, &s)) == NULL) {
		syslog(LOG_ERR, "%s %s", remname,
		    clnt_spcreateerror("server not responding"));
		return (NFSERR_NOENT);
	}
#ifdef OLDMOUNT
	if (bindresvport(s, NULL))
		syslog(LOG_ERR, "Warning: cannot do local bind");
#endif
	client->cl_auth = authunix_create_default();
	timeout.tv_usec = 0;
	timeout.tv_sec = mount_timeout;
	rpc_stat = clnt_call(client, MOUNTPROC_MNT, xdr_path, &dir,
	    xdr_fhstatus, &fhs, timeout);
	if (rpc_stat != RPC_SUCCESS) {
		/*
		 * Given the way "clnt_sperror" works, the "%s" immediately
		 * following the "not responding" is correct.
		 */
		syslog(LOG_ERR, "%s server not responding%s", remname,
		    clnt_sperror(client, ""));
		(void) close(s);
		clnt_destroy(client);
		return (NFSERR_NOENT);
	}
	(void) close(s);
	AUTH_DESTROY(client->cl_auth);
	clnt_destroy(client);

	if (errno = fhs.fhs_status)  {
                if (errno == EACCES) {
			status = NFSERR_ACCES;
                } else {
			syslog(LOG_ERR, "%s: %m", remname);
			status = NFSERR_IO;
                }
                return (status);
        }        

	/*
	 * set mount args
	 */
	args.fh = (fhandle_t *)&fhs.fhs_fh;
	args.hostname = host;
	args.flags = 0;
	args.flags |= (NFSMNT_HOSTNAME | NFSMNT_INT);
	args.gfs_flags = 0;
	args.addr = &sin;
	m.mnt_opts = opts;
	args.optstr = m.mnt_opts;
	args.pg_thresh = SIXTYFOUR;
        args.acregmin = NFSAC_REGMIN;
        args.acregmax = NFSAC_REGMAX;
        args.acdirmin = NFSAC_DIRMIN;
        args.acdirmax = NFSAC_DIRMAX;

	/* ignore sun secure mount option */
	if (hasmntopt(&m, MNTOPT_SECURE) != NULL) {
		syslog(LOG_ERR, "Mount of %s on %s; secure option specified: %m", remname, mntpnt);
		return (NFSERR_IO);
	}


	if (hasmntopt(&m, MNTOPT_SOFT) != NULL) {
		args.flags |= NFSMNT_SOFT;
	}
	if (hasmntopt(&m, MNTOPT_INT) != NULL) {
		args.flags |= NFSMNT_INT;
	}
	if (hasmntopt(&m, MNTOPT_NOEXEC) != NULL) {
		args.gfs_flags |= M_NOEXEC;
	}
	if (hasmntopt(&m, MNTOPT_NOSUID) != NULL) {
		args.gfs_flags |= M_NOSUID;
	}
	if (hasmntopt(&m, MNTOPT_NODEV) != NULL) {
		args.gfs_flags |= M_NODEV;
	}
	if (hasmntopt(&m, MNTOPT_NOAC) != NULL) {
		args.flags |= NFSMNT_NOAC;
	}
	if (port = nopt(&m, "port")) {
		sin.sin_port = htons(port);
	} else {
		sin.sin_port = htons(NFS_PORT);	/* XXX should use portmapper */
	}

	if (args.rsize = nopt(&m, "rsize")) {
		args.flags |= NFSMNT_RSIZE;
	}
	if (args.wsize = nopt(&m, "wsize")) {
		args.flags |= NFSMNT_WSIZE;
	}
	if (args.timeo = nopt(&m, "timeo")) {
		args.flags |= NFSMNT_TIMEO;
	}
	if (args.retrans = nopt(&m, "retrans")) {
		args.flags |= NFSMNT_RETRANS;
	}

	flags = 0;
	flags |= (hasmntopt(&m, MNTOPT_RO) == NULL) ? 0 : M_RONLY;

	if (trace > 1) {
		fprintf(stderr, "mount %s %s (%s)\n",
			remname, mntpnt, opts);
	}
#ifdef OLDMOUNT
#define MOUNT_NFS 1
	if (mount(remname, mntpnt, flags, 0x05, &args)) {
#else
	if (mount(remname, mntpnt, flags, 0x05, &args)) {
#endif
		syslog(LOG_ERR, "Mount of %s on %s: %m", remname, mntpnt);
		return (NFSERR_IO);
	}
	if (trace > 1) {
		fprintf(stderr, "mount %s OK\n", remname);
	}
	if (*fsp)
		fs = *fsp;
	else {
		fs = alloc_fs(host, dir, mntpnt, opts);
		if (fs == NULL)
			return (NFSERR_NOSPC);
	}
	if (getmnt(0, &fsdata, 0, NOSTAT_ONE, mntpnt) < 1) {
		/* Couldn't get dev number; we'll try later */
		fs->fs_dev = -1;
	}
	else {
		/* Got it; save it away for the umount	    */
		fs->fs_dev = fsdata.fd_req.dev;
	}
	fs->fs_type = MNTTYPE_NFS;
	fs->fs_mine = 1;
	fs->fs_nfsargs = args;
	fs->fs_mflags = flags;
	fs->fs_nfsargs.hostname = fs->fs_host;
	fs->fs_nfsargs.addr = &fs->fs_addr;
	fs->fs_nfsargs.fh = (fhandle_t *)&fs->fs_rootfh;
	fs->fs_addr = sin;
	bcopy(&fhs.fhs_fh, &fs->fs_rootfh, sizeof fs->fs_rootfh);
	*fsp = fs;
	return (NFS_OK);
}

nfsstat
remount(fs)
	struct filsys *fs;
{
	char remname[1024];

	if (fs->fs_nfsargs.fh == 0) 
		return nfsmount(fs->fs_host, fs->fs_addr.sin_addr, fs->fs_dir,
				fs->fs_mntpnt, fs->fs_opts, &fs);
	(void) sprintf(remname, "%s:%s", fs->fs_host, fs->fs_dir);
	if (trace > 1) {
		fprintf(stderr, "remount %s %s (%s)\n",
			remname, fs->fs_mntpnt, fs->fs_opts);
	}
#ifdef OLDMOUNT
	if (mount(remname, fs->fs_mntpnt, fs->fs_mflags, 0x05, &fs->fs_nfsargs)) {
#else
	if (mount(remname, fs->fs_mntpnt, fs->fs_mflags, 0x05, &fs->fs_nfsargs)) {
#endif
		syslog(LOG_ERR, "Remount of %s on %s: %m", remname,
		    fs->fs_mntpnt);
		return (NFSERR_IO);
	}
	if (trace > 1) {
		fprintf(stderr, "remount %s OK\n", remname);
	}
	return (NFS_OK);
}


/*
 * Return the value of a numeric option of the form foo=x, if
 * option is not found or is malformed, return 0.
 */
nopt(mnt, opt)
	struct mntent *mnt;
	char *opt;
{
	int val = 0;
	char *equal;
	char *str;

	if (str = (char *)hasmntopt(mnt, opt)) {
		if (equal = index(str, '=')) {
			val = atoi(&equal[1]);
		} else {
			syslog(LOG_ERR, "Bad numeric option '%s'", str);
		}
	}
	return (val);
}

same_opts(opts1, opts2)
	char *opts1, *opts2;
{
	int has1, has2;
	struct mntent m1, m2;

	m1.mnt_opts = opts1;
	m2.mnt_opts = opts2;
	has1 = hasmntopt(&m1, MNTOPT_RO) != NULL;
	has2 = hasmntopt(&m2, MNTOPT_RO) != NULL;
	if (has1 != has2) return(0);
	has1 = hasmntopt(&m1, MNTOPT_NOSUID) != NULL;
	has2 = hasmntopt(&m2, MNTOPT_NOSUID) != NULL;
	if (has1 != has2) return(0);
	has1 = hasmntopt(&m1, MNTOPT_SOFT) != NULL;
	has2 = hasmntopt(&m2, MNTOPT_SOFT) != NULL;
	if (has1 != has2) return(0);
	has1 = hasmntopt(&m1, MNTOPT_INT) != NULL;
	has2 = hasmntopt(&m2, MNTOPT_INT) != NULL;
	if (has1 != has2) return(0);
	return(1);
}
