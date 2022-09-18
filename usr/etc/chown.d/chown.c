#ifndef lint
static	char	*sccsid = "@(#)chown.c	4.1	Ultrix		7/2/90";
#endif lint
/************************************************************************
 *									*
 *			Copyright (c) 1989 by				*
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
 *     Added an extra argument to Perror to amke it more informative.	*
 * 001 Richard Hart, 9-Apr-87						*
 *     Copied from 4.3 BSD:						*
 *		chown.c      5.6 (Berkeley) 5/29/86			*
 ************************************************************************/

/*
 * chown [-fR] uid[.gid] file ...
 */

#include <stdio.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pwd.h>
#include <sys/dir.h>
#include <grp.h>
#include <strings.h>

struct	passwd *pwd;
struct	passwd *getpwnam();
struct	stat stbuf;
int	uid;
int	status;
int	fflag;
int	rflag;

main(argc, argv)
	char *argv[];
{
	register int c, gid;
	register char *cp, *group;
	struct group *grp;

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
		}
		argv++, argc--;
	}
	if (argc < 2) {
		fprintf(stderr, "usage: chown [-fR] owner[.group] file ...\n");
		exit(-1);
	}
	gid = -1;
	group = index(argv[0], '.');
	if (group != NULL) {
		*group++ = '\0';
		if (!isnumber(group)) {
			if ((grp = getgrnam(group)) == NULL)
				fatal(255, "unknown group: %s",group);
			gid = grp -> gr_gid;
			(void) endgrent();
		} else if (*group != '\0')
			gid = atoi(group);
	}
	if (!isnumber(argv[0])) {
		if ((pwd = getpwnam(argv[0])) == NULL)
			fatal(255, "unknown user id: %s",argv[0]);
		uid = pwd->pw_uid;
	} else
		uid = atoi(argv[0]);
	for (c = 1; c < argc; c++) {
		/* do stat for directory arguments */
		if (lstat(argv[c], &stbuf) < 0) {
			status += Perror("access", argv[c]);
			continue;
		}
		if (rflag && ((stbuf.st_mode&S_IFMT) == S_IFDIR)) {
			status += chownr(argv[c], uid, gid);
			continue;
		}
		if (chown(argv[c], uid, gid)) {
			status += Perror("change ownership of", argv[c]);
			continue;
		}
	}
	exit(status);
}

isnumber(s)
	char *s;
{
	register c;

	while(c = *s++)
		if (!isdigit(c))
			return (0);
	return (1);
}

chownr(dir, uid, gid)
	char *dir;
{
	register DIR *dirp;
	register struct direct *dp;
	struct stat st;
	char savedir[1024];
	int ecode;
	extern char *getwd();

	if (getwd(savedir) == (char *)0)
		fatal(255, "%s", savedir);
	/*
	 * Change what we are given before doing it's contents.
	 */
	if (chown(dir, uid, gid) < 0 && Perror("change ownership of", dir))
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
		if ((st.st_mode&S_IFMT) == S_IFDIR) {
			ecode = chownr(dp->d_name, uid, gid);
			if (ecode)
				break;
			continue;
		}
		if (chown(dp->d_name, uid, gid) < 0 &&
		    (ecode = Perror("change ownership of", dp->d_name)))
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
		fprintf(stderr, "chown: ");
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
		fprintf(stderr, "chown: can't %s ", operation);
		perror(s);
	}
	return (!fflag);
}
