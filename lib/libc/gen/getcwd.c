#ifndef lint
static	char	*sccsid = "@(#)getcwd.c	4.1	(ULTRIX)	7/3/90";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1988 by				*
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
 *			Modfication History				*
 *									*
 *	Mark A. Parenti, 25-Aug-1988					*
 * 0001 Create new version using getwd() code as basis.			*
 *									*
 ************************************************************************/

/*
 * getcwd() returns the pathname of the current working directory. On error
 * an error message is copied to pathname and null pointer is returned.
 */
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/dir.h>
#include <errno.h>

#define CURDIR		"."
#define PARENTDIR	".."
#define PATHSEP		"/"
#define ROOTDIR		"/"

char *strcpy(), *malloc();
int strlen();
static int pathsize;			/* pathname length */

char *
getcwd(pathname, size)
	char *pathname;
	int size;
{
	char pathbuf[MAXPATHLEN];		/* temporary pathname buffer */
	char *pnptr = &pathbuf[(sizeof pathbuf)-1]; /* pathname pointer */
	char *prepend();		/* prepend dirname to pathname */
	dev_t rdev;			/* root device number */
	DIR *dirp;			/* directory stream */
	ino_t rino;			/* root inode number */
	struct direct *dir;		/* directory entry struct */
	struct stat d ,dd;		/* file status struct */

	extern int errno ;		/* error status from stat() and
					 * fstat()
					 */

	pathsize = 0;
	*pnptr = '\0';

	if(size <= 0) {
		errno = EINVAL;
		return(0);
	}
	if(pathname == 0)
		if((pathname = malloc((unsigned)size)) == 0) {
			errno = ENOMEM;
			return(0);
		}
	/* Obtain info about the root and save the device and inode 
	 * information.
	 */
	if (stat(ROOTDIR, &d) != 0) {
		return(NULL) ;
	}
	rdev = d.st_dev;
	rino = d.st_ino;

	/* Climb back up the directory tree to the root, building up the
	 * pathname as we go.
	 */
	for (;;) {
		/* Get info about the current directory, and get out of
		 * the loop if we've reached the root.
		 */
		if (stat(CURDIR, &d) !=0) {
			goto fail;
		}
		if (d.st_ino == rino && d.st_dev == rdev)
			break;		/* reached root directory */

		/* Open the parent directory */
		if ((dirp = opendir(PARENTDIR)) == NULL) {
			goto fail;
		}

		/* Change the working directory to the parent */
		if (chdir(PARENTDIR) < 0) {
			goto fail;
		}

		/* See if the PARENTDIR and CURDIR are on same device.
		 */
		if (fstat(dirp->dd_fd, &dd) != 0) {
			goto fail;
		}
		if (d.st_dev == dd.st_dev) {
			/* If the parent and root have the same dev #, we
			 * may be at the root.
			 */
			if (d.st_ino == dd.st_ino) {
				/* reached root directory */
				closedir(dirp);
				break;
			}
			/* Not at the root directory, so search for the
			 * CURDIR directory entry.  
			 */
			do {
				if ((dir = readdir(dirp)) == NULL) {
					closedir(dirp);
					goto fail;
				}
			} while (dir->d_ino != d.st_ino);
		} else	/* Search for the CURDIR directory entry, but now
			 * we have to do a stat() call for each entry to
			 * check the device.
			 */
			do {
				if((dir = readdir(dirp)) == NULL) {
					closedir(dirp);
					goto fail;
				}
				if (stat(dir->d_name, &dd) != 0) {

					/* if stat failed, set impossible
					   values into structure so that
					   the test will fail and the loop
					   will continue */

					dd.st_ino = 0;
					dd.st_dev = 0;
				}
			} while(dd.st_ino != d.st_ino || dd.st_dev != d.st_dev);
		/* Put the CURDIR's name from the directory entry on the path.
		 */
		pnptr = prepend(PATHSEP, prepend(dir->d_name, pnptr));
		closedir(dirp);	/* release resources after the copy */
	}
	if (*pnptr == '\0')		/* current dir == root dir */
		strcpy(pathname, ROOTDIR);
	else {
		if((strlen(pnptr)+1) > size) {
			errno = ERANGE;
			goto fail;
		}
		strcpy(pathname, pnptr);
		if (chdir(pnptr) < 0) {
			return (NULL);
		}
	}
	return (pathname);

fail:
	chdir(prepend(CURDIR, pnptr));
	return (NULL);
}

/*
 * prepend() tacks a directory name onto the front of a pathname.
 */
static char *
prepend(dirname, pathname)
	register char *dirname;
	register char *pathname;
{
	register int i;			/* directory name size counter */

	for (i = 0; *dirname != '\0'; i++, dirname++)
		continue;
	if ((pathsize += i) < MAXPATHLEN)
		while (i-- > 0)
			*--pathname = *--dirname;
	return (pathname);
}

