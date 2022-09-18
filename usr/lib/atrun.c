#ifndef lint
static	char	*sccsid = "@(#)atrun.c	4.2	(ULTRIX)	2/28/91";
#endif lint

/************************************************************************
 *									*
 *   Copyright (c) Digital Equipment Corporation, 1983, 1989, 1991	*
 *									*
 *   All Rights Reserved.  Unpublished rights  reserved  under  the	*
 *   copyright laws of the United States.				*
 *									*
 *   The software contained on this media  is  proprietary  to  and	*
 *   embodies  the  confidential  technology  of  Digital Equipment	*
 *   Corporation.  Possession, use, duplication or dissemination of	*
 *   the  software and media is authorized only pursuant to a valid	*
 *   written license from Digital Equipment Corporation.		*
 *									*
 *   RESTRICTED RIGHTS LEGEND   Use, duplication, or disclosure  by	*
 *   the U.S. Government is subject to restrictions as set forth in	*
 *   Subparagraph (c)(1)(ii)  of  DFARS  252.227-7013,  or  in  FAR	*
 *   52.227-19, as applicable.						*
 *									*
 *   This software is  derived  from  software  received  from  the	*
 *   University  of  California,  Berkeley.   Use,  duplication, or	*
 *   disclosure is subject to restrictions under license agreements	*
 *   with University of California.					*
 *									*
 ************************************************************************/
/**/

/*
 * Modification History
 *
 * 02/27/91 GWS	fix to SPRs ICA-27721, ICA-15006, ICA-18890, ICA-28716,
 *		for problem:
 *		    atrun does not get all groups uid is a member of
 *		    so jobs requiring access to secondary groups fail
 *		the fix is borrowed from BSD 4.3 atrun code:
 *		    add a new function getname(), which calls getpwuid for a
 *		      given uid, reports errors if any, else returns a user
 *		      name.
 *		    in run(), call initgroups(getname(uid),gid) for job
 *		      file's uid, to get the job's gid's.
 *
 */

/*
 * Run programs submitted by at.
 */
#include <stdio.h>
#include <sys/param.h>
#include <sys/dir.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <pwd.h>

# define ATDIR "/usr/spool/at"
# define PDIR	"past"
# define LASTF "/usr/spool/at/lasttimedone"

int	nowtime;
int	nowdate;
int	nowyear;

main(argc, argv)
char **argv;
{
	int tt, day, year, uniq;
	struct direct *dirent;
	DIR *dirp;

	chdir(ATDIR);
	makenowtime();
	if ((dirp = opendir(".")) == NULL) {
		fprintf(stderr, "Cannot read at directory\n");
		exit(1);
	}
	while ((dirent = readdir(dirp)) != NULL) {
		if (dirent->d_ino==0)
			continue;
		if (sscanf(dirent->d_name, "%2d.%3d.%4d.%2d", &year, &day, &tt, &uniq) != 4)
			continue;
		if (nowyear < year)
			continue;
		if (nowyear==year && nowdate < day)
			continue;
		if (nowyear==year && nowdate==day && nowtime < tt)
			continue;
		run(dirent->d_name);
	}
	closedir(dirp);
	updatetime(nowtime);
	exit(0);
}

makenowtime()
{
	long t;
	struct tm *localtime();
	register struct tm *tp;

	time(&t);
	tp = localtime(&t);
	nowtime = tp->tm_hour*100 + tp->tm_min;
	nowdate = tp->tm_yday;
	nowyear = tp->tm_year;
}

updatetime(t)
{
	FILE *tfile;

	tfile = fopen(LASTF, "w");
	if (tfile == NULL) {
		fprintf(stderr, "can't write lastfile\n");
		exit(1);
	}
	fprintf(tfile, "%04d\n", t);
}

run(file)
char *file;
{
	struct stat stbuf;
	register pid, i;
	char sbuf[64];
	char *getname();		/* get a uname from using a uid */

	/* printf("running %s\n", file); */
	if (fork()!=0)
		return;
	for (i=0; i<15; i++)
		close(i);
	dup(dup(open("/dev/null", 0)));
	sprintf(sbuf, "%s/%s", PDIR, file);
	link(file, sbuf);
	unlink(file);
	chdir(PDIR);
	if (stat(file, &stbuf) == -1)
		exit(1);
	if (pid = fork()) {
		if (pid == -1)
			exit(1);
		wait((int *)0);
		unlink(file);
		exit(0);
	}
	if (setgid(stbuf.st_gid) < 0) {
		fprintf(stderr, "setgid to %d failed\n", stbuf.st_gid);
		exit(1);
	}

	initgroups(getname(stbuf.st_uid),stbuf.st_gid);

	if (setuid(stbuf.st_uid) < 0) {
		fprintf(stderr, "setuid to %d failed\n", stbuf.st_uid);
		exit(1);
	}
	execl("/bin/sh", "sh", file, 0);
	execl("/usr/bin/sh", "sh", file, 0);
	fprintf(stderr, "Can't execl shell\n");
	exit(1);
}

/*
 * Get the full login name of a person using his/her user id.
 */
char *
getname(uid)
int uid;
{
	struct passwd *pwdinfo;			/* password info structure */
	

	if ((pwdinfo = getpwuid(uid)) == 0) {
		perror(uid);
		exit(1);
	}
	return(pwdinfo->pw_name);
}
