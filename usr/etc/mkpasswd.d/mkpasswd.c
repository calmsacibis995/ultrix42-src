#ifndef lint
static  char    *sccsid = "@(#)mkpasswd.c	4.2	(ULTRIX)	11/15/90";
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
/*
 * Modification history:
 *
 * 14-Nov-90 D. Long
 *	Don't issue warning messages unless -v option supplied.
 *
 * 19-Jul-89 D. Long
 *	Modified to rebuild the passwd data base even if it already exists
 *	unless the data base files are newer than the input file.  Also,
 *	added the -u option to prevent creation of a data base if one doesn't
 *	already exist.  Also, fixed to use getpwent_local().
 */
/*
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifndef lint
char copyright[] =
"@(#) Copyright (c) 1980 Regents of the University of California.\n\
 All rights reserved.\n";
#endif not lint

/* @(#)mkpasswd.c	5.1 (Berkeley) 5/28/85 */

#include <sys/param.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <stdio.h>
#include <pwd.h>
#include <ndbm.h>

char	buf[BUFSIZ];

struct	passwd *getpwent_local();

main(argc, argv)
	char *argv[];
{
	DBM *dp;
	datum key, content;
	register char *cp, *tp;
	register struct passwd *pwd;
	struct stat statbuf;
	time_t mtime;
	int i, verbose = 0, entries = 0, maxlen = 0;
	int update = 0, errflag = 0;
	char c;
	extern int optind;

	while((c = getopt(argc, argv, "vu")) != EOF)
		switch(c) {
		case 'v':
			verbose = 1;
			break;
		case 'u':
			update = 1;
			break;
		case '?':
			errflag++;
		}
	if(errflag || optind >= argc) {
		fputs("usage: mkpasswd [ -uv ] file\n", stderr);
		exit(1);
	}
	if (access(argv[optind], R_OK) < 0) {
		fputs("mkpasswd: ", stderr);
		perror(argv[optind]);
		exit(1);
	}
	strcpy(buf, argv[optind]);
	strcat(buf, ".pag");
	if(stat(buf, &statbuf) == 0) {
		mtime = statbuf.st_mtime;
		if(stat(argv[optind], &statbuf) < 0) {
			perror(argv[optind]);
			exit(2);
		}
		if(mtime > statbuf.st_mtime) {
			if(verbose)
				puts("The database is already up to date.");
			exit(0);
		} else {
			unlink(buf);
			strcpy(buf, argv[optind]);
			strcat(buf, ".dir");
			unlink(buf);
		}
	} else {
		if(update) {
			if(verbose)
				puts("Hashed password database not in use, database not created.");
			exit(0);
		}
	}
	umask(022);
	dp = dbm_open(argv[optind], O_WRONLY|O_CREAT|O_EXCL, 0644);
	if (dp == NULL) {
		fputs("mkpasswd: ", stderr);
		perror(argv[optind]);
		exit(1);
	}
	setpwfile(argv[optind]);
	setpwent_local();
	while (pwd = getpwent_local()) {
		cp = buf;
#define	COMPACT(e)	tp = pwd->pw_/**/e; while (*cp++ = *tp++);
		COMPACT(name);
		COMPACT(passwd);
		i = pwd->pw_uid;
		bcopy(&i, cp, sizeof i);
		cp += sizeof i;
		i = pwd->pw_gid;
		bcopy(&i, cp, sizeof i);
		cp += sizeof i;
		i = pwd->pw_quota;
		bcopy(&i, cp, sizeof i);
		cp += sizeof i;
		COMPACT(comment);
		COMPACT(gecos);
		COMPACT(dir);
		COMPACT(shell);
		content.dptr = buf;
		content.dsize = cp - buf;
		if (verbose)
			printf("store %s, uid %d\n", pwd->pw_name, pwd->pw_uid);
		key.dptr = pwd->pw_name;
		key.dsize = strlen(pwd->pw_name);
		if (dbm_store(dp, key, content, DBM_INSERT) < 0) {
			fputs("mkpasswd: ", stderr);
			perror("dbm_store failed");
			exit(1);
		}
		i = pwd->pw_uid;
		key.dptr = (char *)&i;
		key.dsize = sizeof i;
		if (dbm_store(dp, key, content, DBM_INSERT) < 0) {
			fputs("mkpasswd: ", stderr);
			perror("dbm_store failed");
			exit(1);
		}
		entries++;
		if (cp - buf > maxlen)
			maxlen = cp - buf;
	}
	endpwent_local();
	dbm_close(dp);
	printf("%d password entries, maximum length %d\n", entries, maxlen);
	exit(0);
}
