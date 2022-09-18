#ifndef lint
static char *sccsid = "@(#)auto_all.c	4.2      (ULTRIX)        1/3/91";
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
 *      27 Dec 90 -- condylis
 *              Do umount now with fs_dev member of filsys structure rather
 *              than getting fs_dev at umount time (if possible) via getmnt.
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
#include <sys/types.h>
#include <sys/errno.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <rpc/rpc.h>
#include <rpc/pmap_clnt.h>
#include <netdb.h>
#include <errno.h>
#include "nfs_prot.h"
#define NFSCLIENT
typedef nfs_fh fhandle_t;
#include <rpcsvc/mount.h>
#include <sys/mount.h>
#include <nfs/nfs_gfs.h>
#include "automount.h"

do_unmount(fsys)
	struct filsys *fsys;
{
	struct queue tmpq;
        struct fs_data fsdata;
	struct filsys *fs, *nextfs;
	nfsstat remount();
	extern int trace;

	tmpq.q_head = tmpq.q_tail = NULL;
	for (fs = HEAD(struct filsys, fs_q); fs; fs = nextfs) {
		nextfs = NEXT(struct filsys, fs);
		if (fs->fs_rootfs == fsys) {
			REMQUE(fs_q, fs);
			INSQUE(tmpq, fs);
		}
	}
	/* walk backwards trying to unmount */
	for (fs = TAIL(struct filsys, tmpq); fs; fs = nextfs) {
		nextfs = PREV(struct filsys, fs);
		if (trace > 1) {
			fprintf(stderr, "unmount %s ", fs->fs_mntpnt);
			fflush(stderr);
		}
		/* if couldn't get dev number at mount time 		*/
		if (fs->fs_dev == -1) {
			/* If can't get it now either			*/
			if (getmnt(0, &fsdata, 0, NOSTAT_ONE, fs->fs_mntpnt) < 1)
				syslog(LOG_ERR, "do_unmount: getmnt failed on %s\n", fs->fs_mntpnt);
			else
				fs->fs_dev = fsdata.fd_req.dev;
		}
		
		/* if still don't have dev number or umount fails	*/
		if ((fs->fs_dev == -1) || (umount(fs->fs_dev) < 0)) {
			if (trace > 1)
				fprintf(stderr, "BUSY\n");
			/* remount previous unmounted ones */
			for (fs = NEXT(struct filsys, fs); fs; fs = nextfs) {
				nextfs = NEXT(struct filsys, fs);
				(void) remount(fs);
			}
			goto inuse;
		}
		if (trace > 1)
			fprintf(stderr, "OK\n");
	}
	/* all ok - walk backwards removing directories */

	/* clean_mtab(0, HEAD(struct filsys, tmpq)); */
	for (fs = TAIL(struct filsys, tmpq); fs; fs = nextfs) {
		nextfs = PREV(struct filsys, fs);
		nfsunmount(fs);
		safe_rmdir(fs->fs_mntpnt);
		REMQUE(tmpq, fs);
		INSQUE(fs_q, fs);
		free_filsys(fs);
	}
	/* success */
	return (1);

inuse:	/* put things back on the correct list */
	for (fs = HEAD(struct filsys, tmpq); fs; fs = nextfs) {
		nextfs = NEXT(struct filsys, fs);
		REMQUE(tmpq, fs);
		INSQUE(fs_q, fs);
	}
	return (0);
}

freeex(ex)
	struct exports *ex;
{
	struct groups *groups, *tmpgroups;
	struct exports *tmpex;

	while (ex) {
		free(ex->ex_name);
		groups = ex->ex_groups;
		while (groups) {
			tmpgroups = groups->g_next;
			free((char *)groups);
			groups = tmpgroups;
		}
		tmpex = ex->ex_next;
		free((char *)ex);
		ex = tmpex;
	}
}

mkdir_r(dir)
	char *dir;
{
	int err;
	char *slash;
	char *rindex();

	if (mkdir(dir, 0555) == 0)
		return (0);
	if (errno != ENOENT)
		return (-1);
	slash = rindex(dir, '/');
	if (slash == NULL)
		return (-1);
	*slash = '\0';
	err = mkdir_r(dir);
	*slash++ = '/';
	if (err || !*slash)
		return (err);
	return mkdir(dir, 0555);
}

safe_rmdir(dir)
	char *dir;
{
	dev_t tmpdev;
	struct stat stbuf;

	if (stat(tmpdir, &stbuf))
		return;
	tmpdev = stbuf.st_dev;

	if (stat(dir, &stbuf))
		return;
	if (tmpdev == stbuf.st_dev)
		(void) rmdir(dir);
}
