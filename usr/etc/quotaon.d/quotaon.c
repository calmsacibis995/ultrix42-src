#ifndef lint
static char *sccsid = "@(#)quotaon.c	4.1	(ULTRIX)	7/2/90";
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

/*
 * Turn quota on/off for a filesystem.
 */
#include <sys/param.h>
#include <sys/file.h>
#include <sys/mount.h>
#include <stdio.h>
#include <fstab.h>

struct	fs_data *mountbuffer;
#define MSIZE sizeof(struct fs_data) * NMOUNT

int	vflag;		/* verbose */
int	aflag;		/* all file systems */
int	done;
int	ret;

char	*qfname = "quotas";
char	quotafile[MAXPATHLEN + 1];
char	*index(), *rindex();

main(argc, argv)
	int argc;
	char **argv;
{
	register struct fstab *fs;
	char *whoami, *rindex();
	int offmode = 0, errs = 0, i;
	int loc;
	char *malloc();
	
	whoami = rindex(*argv, '/') + 1;
	if (whoami == (char *)1)
		whoami = *argv;
	if (strcmp(whoami, "quotaoff") == 0)
		offmode++;
	else if (strcmp(whoami, "quotaon") != 0) {
		fprintf(stderr, "Name must be quotaon or quotaoff not %s\n",
			whoami);
		exit(1);
	}
again:
	argc--, argv++;
	if (argc > 0 && strcmp(*argv, "-v") == 0) {
		vflag++;
		goto again;
	}
	if (argc > 0 && strcmp(*argv, "-a") == 0) {
		aflag++;
		goto again;
	}
	if (argc <= 0 && !aflag) {
		fprintf(stderr, "Usage:\n\t%s [-v] -a\n\t%s [-v] filesys ...\n",
			whoami, whoami);
		exit(1);
	}
	if((mountbuffer = (struct fs_data *)malloc(MSIZE)) == NULL) {
		perror("malloc");
		exit(1);
	}
	/* use this so that we don't hang if server's down with nfs file sys */
	ret = getmountent(&loc, mountbuffer, NMOUNT);
	if (ret == 0) {
		perror("getmountent");
		exit(2);
	}
	setfsent();
	while ((fs = getfsent()) != NULL) {
		if (aflag &&
		    (fs->fs_type == 0 || strcmp(fs->fs_type, "rq") != 0))
			continue;
		if (!aflag &&
		    !(oneof(fs->fs_file, argv, argc) ||
		      oneof(fs->fs_spec, argv, argc)))
			continue;
		errs += quotaonoff(fs, offmode);
	}
	endfsent();
	for (i = 0; i < argc; i++)
		if ((done & (1 << i)) == 0)
			fprintf(stderr, "%s not found in /etc/fstab\n",
				argv[i]);
	exit(errs);
}

quotaonoff(fs, offmode)
	register struct fstab *fs;
	int offmode;
{

	if (strcmp(fs->fs_file, "/") && readonly(fs))
		return (1);
	if (offmode) {
		if (setquota(fs->fs_spec, NULL) < 0)
			goto bad;
		if (vflag)
			printf("%s: quotas turned off\n", fs->fs_file);
		return (0);
	}
	(void) sprintf(quotafile, "%s/%s", fs->fs_file, qfname);
	if (setquota(fs->fs_spec, quotafile) < 0)
		goto bad;
	if (vflag)
		printf("%s: quotas turned on\n", fs->fs_file);
	return (0);
bad:
	fprintf(stderr, "setquota: ");
	perror(fs->fs_spec);
	return (1);
}

oneof(target, list, n)
	char *target, *list[];
	register int n;
{
	register int i;

	for (i = 0; i < n; i++)
		if (strcmp(target, list[i]) == 0) {
			done |= 1 << i;
			return (1);
		}
	return (0);
}

/*
 * Verify file system is mounted and not readonly.
 */
readonly(fs)
	register struct fstab *fs;
{
	register struct fs_data *fs_data;
	register char *cp;

	cp = index(fs->fs_spec, '\0');
	while (*--cp == '/')
		*cp = '\0';
	cp = fs->fs_spec;
	for (fs_data = mountbuffer; fs_data < &mountbuffer[ret]; fs_data++) {
		if (strcmp(cp, fs_data->fd_devname) == 0) {
			if ((fs_data->fd_flags & M_RONLY) != 0) {
				printf("%s: mounted read-only\n", fs->fs_file);
				return (1);
			}
			return (0);
		}
	}
	printf("%s: not mounted\n", fs->fs_file);
	return (1);
}
