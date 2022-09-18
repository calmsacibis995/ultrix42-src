#ifndef lint
static	char	*sccsid = "@(#)prestoctl_proc.c	4.1	(ULTRIX)	10/8/90";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1990 by			*
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
 *	Copyright (c) 1990 Legato Systems, Inc.  ALL RIGHTS RESERVED.
 */

/*
 *
 *   Modification history:
 *
 *  23 Aug 90 -- chet
 *	Added this file; it was derived from Legato sources.
 *
 */

/*
 * This file implements the bodies of the RPC's for prestoctl.
 * The remote calls are translated into ioctl's to the presto device.
 */
#include <sys/param.h>
#include <sys/types.h>
#include <sys/mount.h>
#include <sys/fs_types.h>
#include <sys/stat.h>
#include <rpc/rpc.h>
#include <sys/socket.h>
#include <sys/file.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <syslog.h>
#include <strings.h>
#include <errno.h>
#ifdef DEBUG
#include "prestoioctl.h"
#else
#include <sys/prestoioctl.h>
#endif
#include "prestoctl.h"

extern int errno;
extern char *sys_errlist[];
extern int sys_nerr;
extern void call_svc();

int pd;
int nflag = 0;
char *problem;
char errmsg[200];
char *presto_name = PRDEV;

struct fs_data *mountbuffer = 0;

long	gethostid();

/*
 * return a string to our hostname.
 */
char *
hostname()
{
	static char buf[MAXHOSTNAMELEN];

	if (gethostname(buf, sizeof buf) < 0)	/* too long to fit ? */
		(void) sprintf(buf, "[0x%lx]", 	/* so use the numeric form */
			gethostid());
	return (buf);
}

/*
 * Safely map an error number to an error string.
 */
char *
errno_to_errmsg()
{
	static char msg[200];

	if (errno > sys_nerr) {	/* being conservative--off by one maybe? XXXX */
		(void) sprintf(msg, "errno=%d", errno);
		return (msg);
	} else {
		return (sys_errlist[errno]);
	}
}

void
p_error(msg)
	char *msg;
{
	char buf[400];

	if (msg != 0 && *msg != '\0') {
		if ((strlen(msg)+2) > 200)
			*(msg+200) = '\0';
		(void) strcpy(buf, msg);
		(void) strcat(buf, ": ");
		(void) strcat(buf, errno_to_errmsg());
		syslog(LOG_ERR, buf);
	} else {
		syslog(LOG_ERR, errno_to_errmsg());
	}
}

main(argc, argv)
	int argc;
	char **argv;
{
	struct sockaddr name;
	int namelen = sizeof (name);

	openlog("prestoctl_svc", LOG_PID);

	if (argc > 2 || (argc == 2 && ! (nflag = (strcmp(argv[1], "-n") == 0))))
		syslog(LOG_ERR, "Ignoring invalid arguments.");

	pd = open(presto_name, O_RDWR|O_NDELAY, 0);
	if (pd < 0) {
		problem = errno_to_errmsg();
		p_error("Warning: cannot open /dev/pr0");
	}

	call_svc(pd);
}

/* ARGSUSED */
presto_get_fs_status *
prestoctl_get_fs_status_3(filesystem)
	char **filesystem;
{
	register char *fs = *filesystem;
	register char *path, *zap;
	static presto_get_fs_status get_fs_status;
	char buf[MAXPATHLEN];
	presto_status ps;
	presto_fs_status *pfs;
	struct stat stats;
	struct uprtab uprtab;
	int rtn;

	pfs = &get_fs_status.presto_get_fs_status_u.status;
	if (strlen(fs) > sizeof (buf)) {
		errno = 0;
		(void) sprintf(errmsg, "get_fs_status: fs name too long");
		goto error;
	}
	(void) strcpy(buf, fs);
	path = buf;

	/*
	 * Check our ioctl file descripter
	 */
	if (pd < 0) {
		/* bad news */
		(void) sprintf(errmsg, "prestoctl_svc: cannot open %s: %s.",
			presto_name, problem);
		goto error;
	}

	if (ioctl(pd, PRGETSTATUS, &ps) < 0) {
		/* more bad news */
		p_error("PRGETSTATUS");
		goto error;
	}
	pfs->pfs_state = ps.pr_state;

loop:
	/*
	 * find device name for this fs from mount table
	 */
	if (!mountbuffer) {
		mountbuffer = (struct fs_data *)
			malloc(sizeof(struct fs_data));
		if (mountbuffer == NULL) {
			(void) sprintf(errmsg,
			"prestoctl_svc: cannot allocate buffer for  %s\n",
				       path);
			goto error;
		}
	}
	if ((rtn = getmnt(0, mountbuffer, 0, NOSTAT_ONE, path)) == -1) {
		/* more bad news */
		(void) sprintf(errmsg,
	        "prestoctl_svc: cannot get mount info on %s: %s\n",
			       path, errno_to_errmsg());
		goto error;
	}
	else if (rtn != 0) {  
		/* the file system is mounted */
		if (mountbuffer->fd_fstype != GT_ULTRIX) {
			errno = 0;
			(void) sprintf(errmsg,
			"prestoctl_svc: %s is not a local filesystem on %s\n",
			    fs, hostname());
			goto error;
		}

		/*
		 * map this device name to a block device number
		 */
		if (stat(mountbuffer->fd_devname, &stats) < 0) {
			/* and yet more bad news */
			(void) sprintf(errmsg,
			    "prestoctl_svc: cannot stat %s on %s, %s\n",
			    mountbuffer->fd_devname, hostname(), errno_to_errmsg());
			goto error;
		}
		if ((stats.st_mode & S_IFMT) != S_IFBLK) {
			(void) sprintf(errmsg,
			    "prestoctl_svc: %s is not a block device on %s\n",
			    mountbuffer->fd_devname, hostname());
			p_error(errmsg);
			errno = 0;
			goto error;
		}
		/*
		 * query enabled status for this device from presto driver
		 */
		uprtab.upt_bmajordev = major(stats.st_rdev);
		if (ioctl(pd, PRGETUPRTAB, &uprtab) < 0) {
			(void) sprintf(errmsg,
		"prestoctl_svc: ioctl PRGETUPRTAB failed on host %s : %s.",
			    hostname(), errno_to_errmsg());
			goto error;
		}
		get_fs_status.succeeded = TRUE;
		if (uprtab.upt_bmajordev != NODEV) {
			int min_d = minor(stats.st_rdev);

			pfs->pfs_prestoized = TRUE;
			pfs->pfs_enabled = isset(uprtab.upt_enabled.bits,
			    min_d);
			pfs->pfs_bounceio = isset(uprtab.upt_bounceio.bits,
			    min_d);
		} else {
			pfs->pfs_prestoized = FALSE;
			pfs->pfs_enabled = FALSE;
			pfs->pfs_bounceio = FALSE;
		}
		return (&get_fs_status);
	}
	/*
	 * Can we shorten the path and try again?
	 * Since subdirectories of filesystems can be mounted by clients
	 * we have to find the maximal matching path.
	 */
	if ((zap = rindex(path, '/')) != 0) {
		if (zap != path) {		/* not left with root */
			*zap = '\0';
			goto loop;		/* try the shorter path */
		} else if (*(++zap) != '\0') {	/* was it already root ? */
			*zap = '\0';		/* nope */
			goto loop;		/* try the shorter path */
		}
		/* they loose */
	}

	(void) sprintf(errmsg, "prestoctl_svc: %s does not exist on %s\n",
		       fs, hostname);
	errno = 0;
error:
	get_fs_status.succeeded = FALSE;
	get_fs_status.presto_get_fs_status_u.errmsg = errmsg;
	if (errno)
		syslog(LOG_ERR, errmsg);
	return (&get_fs_status);
}

/* ARGSUSED */
presto_modstat *
prestoctl_getstate_3(a, r)
	void *a;
	struct svc_req *r;
{
	static struct presto_modstat ps;  /* results of this call */

	if (pd < 0) {
		/* bad news */
		ps.ps_status = FALSE;
		(void) sprintf(errmsg, "prestoctl_svc: cannot open %s: %s.",
			presto_name, problem);
		ps.presto_modstat_u.ps_errmsg = errmsg;
		return (&ps);
	}
	ps.ps_status = TRUE;
	if (ioctl(pd, PRGETSTATUS, &ps.presto_modstat_u.ps_new) < 0) {
		/* more bad news */
		ps.ps_status = FALSE;
		ps.presto_modstat_u.ps_errmsg =
			"prestoctl_svc PRGETSTATUS ioctl failed.";
		return (&ps);
	}
	return (&ps);
}

/* ARGSUSED */
presto_modstat *
prestoctl_setbytes_3(bytes, r)
	u_int *bytes;  /* bytes of NV RAM to use */
	struct svc_req *r;
{
	if (nflag) {
		(void) ioctl(pd, PRSETMEMSZ, bytes);
		return (prestoctl_getstate_3((void *)NULL, r));
	} else {
		static struct presto_modstat ps;  /* results of this call */

		ps.ps_status = FALSE;
		ps.presto_modstat_u.ps_errmsg =
		    "presto modifications not allowed!";
		return (&ps);
	}
}

/* ARGSUSED */
presto_modstat *
prestoctl_toggle_3(on, r)
	bool_t *on;
	struct svc_req *r;
{
	static struct presto_modstat ps;  /* results of this call */

	if (nflag) {
		if ((pd >= 0) && ioctl(pd, PRSETSTATE, on) < 0) {
			ps.ps_status = FALSE;
			problem = sys_errlist[errno];
			(void) sprintf(errmsg,
			    "prestoctl_svc toggle: bad ioctl: %s.", problem);
			ps.presto_modstat_u.ps_errmsg = errmsg;
			return (&ps);
		}
		return (prestoctl_getstate_3((void *) 0, r));
	} else {
		ps.ps_status = FALSE;
		ps.presto_modstat_u.ps_errmsg =
		    "presto modifications not allowed!";
		return (&ps);
	}
}
