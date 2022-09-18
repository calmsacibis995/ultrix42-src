#ifndef lint
static	char	*sccsid = "@(#)uucplock.c	4.1	(ULTRIX)	7/17/90";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1984 by				*
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
 * defs that come from uucp.h
 */
#define NAMESIZE 15
#define FAIL -1
#define SAME 0
#define SLCKTIME 28800	/* system/device timeout (LCK.. files) in seconds (8 hours) */
#define ASSERT(e, f, v) if (!(e)) {\
	fprintf(stderr, "AERROR - (%s) ", "e");\
	fprintf(stderr, f, v);\
	finish(FAIL);\
}

#define LOCKPRE "/usr/spool/uucp/LCK."
#define LCKMODE 0644

/*
 * This code is taken almost directly from uucp and follows the same
 * conventions.  This is important since uucp and tip should
 * respect each others locks.
 */

	/*  ulockf 3.2  10/26/79  11:40:29  */
/* #include "uucp.h" */
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <sys/file.h>

/*******
 *	ulockf(file, atime)
 *	char *file;
 *	time_t atime;
 *
 *	ulockf  -  this routine will create a lock file (file).
 *	If one already exists, the create time is checked for
 *	older than the age time (atime).
 *	If it is older, an attempt will be made to unlink it
 *	and create a new one.
 *
 *	return codes:  0  |  FAIL
 */

#define MAXFULLNAME 255

static
ulockf(file, atime)
	char *file;
	time_t atime;
{
	struct stat stbuf;
	time_t ptime;
	int ret;
	static int pid = -1;
	static char *Login = NULL;
	static char tempfile[MAXFULLNAME];
	int fd;

#ifdef FLOCK
	if (pid < 0)
		pid = getpid();
	if (Login == NULL)
		Login = (char *)getlogin();
	if ((fd = open(file, O_RDWR | O_CREAT, LCKMODE)) < 0)
		return(FAIL);
	chmod(file,LCKMODE);
	if (flock(fd, LOCK_EX | LOCK_NB) <  0) {
		close(fd);
		return(FAIL);
	}
	sprintf(tempfile,"%d %s %s\n",pid, "tip", Login);
	write(fd,tempfile, strlen(tempfile));
	stlock(file, fd);
#else

	if (pid < 0) {
		pid = getpid();
		sprintf(tempfile, "/usr/spool/uucp/LTMP.%d", pid);
	}
	if (onelock(pid, tempfile, file) == -1) {
		/* lock file exists */
		/* get status to check age of the lock file */
		ret = stat(file, &stbuf);
		if (ret != -1) {
			time(&ptime);
			if ((ptime - stbuf.st_ctime) < atime) {
				/* file not old enough to delete */
				return (FAIL);
			}
		}
		ret = unlink(file);
		ret = onelock(pid, tempfile, file);
		if (ret != 0)
			return (FAIL);
	}
	stlock(file, -1);
#endif
	return (0);
}

#define MAXLOCKS 10	/* maximum number of lock files */
#ifdef FLOCK
int openfd[MAXLOCKS];
#endif
char *Lockfile[MAXLOCKS];
int Nlocks = 0;

/***
 *	stlock(name)	put name in list of lock files
 *	char *name;
 *
 *	return codes:  none
 */

static
stlock(name, fd)
	char *name;
	int fd;
{
	char *p;
	extern char *calloc();
	int i;

	for (i = 0; i < Nlocks; i++) {
		if (Lockfile[i] == NULL)
			break;
	}
	ASSERT(i < MAXLOCKS, "TOO MANY LOCKS %d", i);
	if (i >= Nlocks)
		i = Nlocks++;
	p = calloc(strlen(name) + 1, sizeof (char));
	ASSERT(p != NULL, "CAN NOT ALLOCATE FOR %s", name);
	strcpy(p, name);
	Lockfile[i] = p;
#ifdef FLOCK
	openfd[i] =fd;
#endif
	return;
}

/***
 *	rmlock(name)	remove all lock files in list
 *	char *name;	or name
 *
 *	return codes: none
 */

static
rmlock(name)
	char *name;
{
	int i;

	for (i = 0; i < Nlocks; i++) {
		if (Lockfile[i] == NULL)
			continue;
		if (name == NULL || strcmp(name, Lockfile[i]) == SAME) {
#ifdef FLOCK
			flock(openfd[i], LOCK_UN);
			close(openfd[i]);
#endif
			unlink(Lockfile[i]);
			free(Lockfile[i]);
			Lockfile[i] = NULL;
		}
	}
}

/*
 * this stuff from pjw 
 *  /usr/pjw/bin/recover - check pids to remove unnecessary locks
 *
 *	isalock(name) returns 0 if the name is a lock
 *
 *	onelock(pid,tempfile,name) makes lock a name
 *	on behalf of pid.  Tempfile must be in the same
 *	file system as name.
 *
 *	lock(pid,tempfile,names) either locks all the
 *	names or none of them
 */
static
isalock(name)
	char *name;
{
	struct stat xstat;

	if (stat(name, &xstat) < 0)
		return (0);
	if (xstat.st_size != sizeof(int))
		return (0);
	return (1);
}

static
onelock(pid, tempfile, name)
	char *tempfile, *name;
{
	int fd;

	fd = creat(tempfile, 0444);
	if (fd < 0)
		return (-1);
	write(fd,(char *)&pid, sizeof(int));
	close(fd);
	if (link(tempfile, name) < 0) {
		unlink(tempfile);
		return (-1);
	}
	unlink(tempfile);
	return (0);
}

/***
 *	delock(s)	remove a lock file
 *	char *s;
 *
 *	return codes:  0  |  FAIL
 */

delock(s)
	char *s;
{
	char ln[30];

	sprintf(ln, "%s.%s", LOCKPRE, s);
	rmlock(ln);
}

/***
 *	mlock(sys)	create system lock
 *	char *sys;
 *
 *	return codes:  0  |  FAIL
 */

mlock(sys)
	char *sys;
{
	char lname[30];
	sprintf(lname, "%s.%s", LOCKPRE, sys);
	return (ulockf(lname, (time_t) SLCKTIME ) < 0 ? FAIL : 0);
}
