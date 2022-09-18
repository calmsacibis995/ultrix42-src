#ifndef lint
static char *sccsid = "@(#)opendir.c	4.1	ULTRIX	7/3/90";
#endif

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
/************************************************************************
 *                       Modification History
 *
 * 24-Jun-88 - prs
 *       Added a call to getsysinfo() to get the environment of the
 *       program. Also, added call to fcntl to set the close-on-exec
 *       bit in POSIX mode.
 *
 ************************************************************************/

#include <sys/param.h>
#include <sys/stat.h>
#include <sys/dir.h>
#include <fcntl.h>
#include <sys/sysinfo.h>
#include <sys/exec.h>
#include <errno.h>

/*
 * open a directory.
 */
DIR *
opendir(name)
	char *name;
{
	register DIR *dirp;
	register int fd;
	struct stat sb;
	short progenv;
	extern int errno;
	extern char *malloc();
	extern int open(), close(), fstat();

	if (getsysinfo(GSI_PROG_ENV, &progenv, sizeof(progenv), 0, 0, 0) < 1)
	        progenv = A_BSD;
	if ((fd = open(name, 0)) == -1)
		return (NULL);
	if (progenv == A_POSIX)
	        fcntl(fd, F_SETFD, FD_CLOEXEC);
	if (fstat(fd, &sb) == -1) {
		(void) close(fd);
		return (NULL);
	}
	if ((sb.st_mode & S_IFMT) != S_IFDIR) {
		errno = ENOTDIR;
		(void) close(fd);
		return (NULL);
	}
	if (((dirp = (DIR *)malloc(sizeof(DIR))) == NULL) ||
	    ((dirp->dd_buf = malloc((unsigned)sb.st_blksize)) == NULL)) {
		if (dirp)
			free(dirp);
		(void) close(fd);
		return (NULL);
	}
	dirp->dd_bsize = sb.st_blksize;
	dirp->dd_bbase = 0;
	dirp->dd_entno = 0;
	dirp->dd_fd = fd;
	dirp->dd_loc = 0;
	return (dirp);
}
