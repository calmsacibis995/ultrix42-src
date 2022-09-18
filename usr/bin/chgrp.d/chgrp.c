#ifndef lint
static	char	*sccsid = "@(#)chgrp.c	4.1	(Ultrix)	7/17/90";
#endif lint
/************************************************************************
 *									*
 *			Copyright (c) 1985 by				*
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
 *			Modification History				*
 * 002 Richard Hart, 9-Apr-87						*
 *     Added extra argument to Perror for petter error messages that	*
 *     are consistant with current Ultrix ch* error messages.		*
 * 001 Richard Hart, 9-Apr-87						*
 *     Copied 4.3 BSD sources:						*
 *		chgrp.c     5.7 (Berkeley) 6/4/86			*
 ************************************************************************/

/*
 * chgrp -fR gid file ...
 */

#include <stdio.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <grp.h>
#include <pwd.h>
#include <sys/dir.h>

struct	group *gr, *getgrnam(), *getgrgid();
struct	passwd *getpwuid(), *pwd;
struct	stat stbuf;
int	gid, uid;
int	status;
int	fflag, rflag;
/* VARARGS */
int	fprintf();

main(argc, argv)
	int argc;
	char *argv[];
{
	register c, i;
	register char *cp;

	argc--, argv++;
	while (argc > 0 && argv[0][0] == '-') {
		for (cp = &argv[0][1]; *cp; cp++) switch (*cp) {

		case 'f':
			fflag++;
			break;

		case 'R':
			rflag++;
			break;

		default:
			fatal(255, "unknown option: %c", *cp);
			/*NOTREACHED*/
		}
		argv++, argc--;
	}
	if (argc < 2) {
		fprintf(stderr, "usage: chgrp [-fR] gid file ...\n");
		exit(255);
	}
	uid = getuid();
	if (isnumber(argv[0])) {
		gid = atoi(argv[0]);
		gr = getgrgid(gid);
		if (uid && gr == NULL)
			fatal(255, "%s: unknown group", argv[0]);
	} else {
		gr = getgrnam(argv[0]);
		if (gr == NULL)
			fatal(255, "%s: unknown group", argv[0]);
		gid = gr->gr_gid;
	}
	pwd = getpwuid(uid);
	if (pwd == NULL)
		fatal(255, "Who are you?");
	if (uid && pwd->pw_gid != gid) {
		for (i=0; gr->gr_mem[i]; i++)
			if (!(strcmp(pwd->pw_name, gr->gr_mem[i])))
				goto ok;
		if (fflag)
			exit(0);
		fatal(255, "You are not a member of the %s group", argv[0]);
	}
ok:
	for (c = 1; c < argc; c++) {
		/* do stat for directory arguments */
		if (lstat(argv[c], &stbuf)) {
			status += Perror("access", argv[c]);
			continue;
		}
		if (uid && uid != stbuf.st_uid) {
			status += error("You are not the owner of %s", argv[c]);
			continue;
		}
		if (rflag && ((stbuf.st_mode & S_IFMT) == S_IFDIR)) {
			status += chownr(argv[c], stbuf.st_uid, gid);
			continue;
		}
		if (chown(argv[c], -1, gid)) {
			status += Perror("change group for", argv[c]);
			continue;
		}
	}
	exit(status);
}

isnumber(s)
	char *s;
{
	register int c;

	while (c = *s++)
		if (!isdigit(c))
			return (0);
	return (1);
}

chownr(dir, uid, gid)
	char *dir;
{
	register DIR *dirp;
	register struct direct *dp;
	register struct stat st;
	char savedir[1024];
	int ecode;

	if (getwd(savedir) == 0)
		fatal(255, "%s", savedir);
	/*
	 * Change what we are given before doing its contents.
	 */
	if (chown(dir, -1, gid) < 0 && Perror("change group for", dir))
		return (1);
	if (chdir(dir) < 0) {
		Perror("chdir to", dir);
		return (1);
	}
	if ((dirp = opendir(".")) == NULL) {
		Perror("open", dir);
		return (1);
	}
	dp = readdir(dirp);
	dp = readdir(dirp); /* read "." and ".." */
	ecode = 0;
	for (dp = readdir(dirp); dp != NULL; dp = readdir(dirp)) {
		if (lstat(dp->d_name, &st) < 0) {
			ecode = Perror("access", dp->d_name);
			if (ecode)
				break;
			continue;
		}
		if (uid && uid != st.st_uid) {
			ecode = error("You are not the owner of %s",
				dp->d_name);
			continue;
		}
		if ((st.st_mode & S_IFMT) == S_IFDIR) {
			ecode = chownr(dp->d_name, st.st_uid, gid);
			if (ecode)
				break;
			continue;
		}
		if (chown(dp->d_name, -1, gid) < 0 &&
		    (ecode = Perror("change group for", dp->d_name)))
			break;
	}
	closedir(dirp);
	if (chdir(savedir) < 0)
		fatal(255, "can't change back to %s", savedir);
	return (ecode);
}

error(fmt, a)
	char *fmt, *a;
{

	if (!fflag) {
		fprintf(stderr, "chgrp: ");
		fprintf(stderr, fmt, a);
		putc('\n', stderr);
	}
	return (!fflag);
}

fatal(status, fmt, a)
	int status;
	char *fmt, *a;
{

	fflag = 0;
	(void) error(fmt, a);
	exit(status);
}

Perror(operation, s)
	char *operation, *s;
{

	if (!fflag) {
		fprintf(stderr, "chgrp: %s ", operation);
		perror(s);
	}
	return (!fflag);
}
