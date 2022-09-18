#ifndef lint
static char *sccsid = "@(#)safesyscalls.c	4.1      ULTRIX 	3/13/91";
#endif

/************************************************************************
 *   Copyright (c) Digital Equipment Corporation 1991  All Rights Reserved.*
 ************************************************************************/

/*
 *	safesyscalls.c
 *
 *	Description:
 *
 *	This file provides `safe' system calls for use by lprsetup
 *	Hopefully a proper cleanup (or replacement) of lprsetup will
 *	make this unnecessary.
 *
 *	The system calls chmod, chown, rmdir and unlink are considered
 *	dangerous since they have the potentiality for system damage.
 *
 *	The strategy is to allow the operations only in particular sub-trees
 *	of the file system, currently, /usr/spool and /usr/adm
 *
 *	In addition, since the unlink call allows root to unlink populated
 *	directories this is explicitly protected against by preventing the
 *	unlinking of any directory.
 *
 *	So that code which checks for error returns and uses perror()
 *	produces a reasonable error message, errno is set to EROFS in
 *	the event of the call being disallowed.
 *	The associated string is "Restricted operation on a file system"
 *	which is fairly appropriate.
 *
 *	Modification History:
 *
 * 03-Mar-91 - Adrian Thoms
 *	First version.
 *
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <errno.h>

#define SPOOLAREA "/usr/spool"
#define ADMAREA "/usr/adm"

char *safe_dirs[] = {
	SPOOLAREA,
	ADMAREA,
	NULL
};

static int issafedir(path)
	char *path;
{
	register char **dirp;
	for(dirp=safe_dirs; *dirp; dirp++) {
		if (!strncmp(*dirp, path, strlen(*dirp))) {
			return 1;
		}
	}
	printf("lprsetup not allowed to ");
	return 0;
}

safechmod(path, mode)
char *path;
mode_t mode;
{
	if (issafedir(path)) {
		return chmod(path, mode);
	} else {
		printf("chmod(%s, 0%o)\n", path, mode);
		errno = EROFS;
		return -1;
	}
}

safechown(path, owner, group)
	char *path;
	uid_t owner;
	gid_t group;
{
	if (issafedir(path)) {
		return chown(path, owner, group);
	} else {
		printf("chown(%s, %d, %d)\n", path, owner, group);
		errno = EROFS;
		return -1;
	}
}

safeunlink(path)
	char *path;
{
	struct stat sb;

	if (issafedir(path)) {
		stat(path, &sb);
		if ((sb.st_mode & S_IFMT) != S_IFDIR) {
			return unlink(path);
		} else {
			printf("lprsetup not allowed to unlink directory %s\n",
			       path);
			errno = EROFS;
			return (-1);
		}
	} else {
		printf("unlink(%s)\n", path);
		errno = EROFS;
		return (-1);
	}
}

safermdir(path)
	char *path;
{
	if (issafedir(path)) {
		return rmdir(path);
	} else {
		printf("rmdir(%s)\n", path);
		errno = EROFS;
		return (-1);
	}
}
