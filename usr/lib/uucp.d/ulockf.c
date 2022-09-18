#ifndef lint
static char sccsid[] = "@(#)ulockf.c	4.1 (decvax!larry) 7/2/90";
#endif

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


/*******************
 *  Mods:
 *	decvax!larry - decrement count of lock files when remove lock
 *			and make associated changes that make this possible
 *			
 *		     - use full path name of lock (does not assume in
 *			spool directory like before)
 *******************/





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




#include "uucp.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <errno.h>
#include <sys/file.h>


extern int errno;
extern time_t	time();

/* File mode for lock files */
#define	LCKMODE	0644


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
	chmod(file, LCKMODE);
	if (flock(fd, LOCK_EX | LOCK_NB) <  0) {
		DEBUG(4,"could not get shared lock, %s\n", file);
		close(fd);
		return(FAIL);
	}
	sprintf(tempfile,"%d %s %s\n",pid, Progname, Login);
	write(fd,tempfile, strlen(tempfile));
	stlock(file, fd);
#else
	if (pid < 0) {
		pid = getpid();
		sprintf(tempfile, "%s/LTMP.%d", SPOOL, pid);
	}
	if (onelock(pid, tempfile, file) == -1) {
		/* lock file exists */
		/* get status to check age of the lock file */
		ret = stat(file, &stbuf);
		if (ret != -1) {
			time(&ptime);
			if ((ptime - stbuf.st_ctime) < atime) {
				/* file not old enough to delete */
				DEBUG(8,"lock not old enough to delete: %s\n",
					file);
				return(FAIL);
			}
		}
		if ((ret = unlink(file))) 
			DEBUG(4, "ulockf, could not unlink, errno=%d\n", errno);
		ret = onelock(pid, tempfile, file);
		if (ret != 0) { 
			/* unlinked old lock file but could not create new one*/
			DEBUG(8,"could not create new lock %s\n", file);
			return(FAIL);
		}
	}
	stlock(file, -1);
#endif
	return(0);
}


#define MAXLOCKS 10	/* maximum number of lock files */
#ifdef FLOCK
int openfd[MAXLOCKS];
#endif
char *Lockfile[MAXLOCKS];
int Nlocks = 0;

/***
 *	stlock(name, fd)	put name in list of lock files
 *	char *name;
 *	int fd;	  open file descriptor
 *
 *	return codes:  none
 */

stlock(name, fd)
char *name;
int fd;
{
	char *p;
	int i;

	for (i = 0; i < MAXLOCKS; i++) {
		if (Lockfile[i] == NULL)
			break;
	}
	ASSERT(++Nlocks < MAXLOCKS, "TOO MANY LOCKS", "", Nlocks);
	p = calloc((unsigned)(strlen(name)+1), sizeof (char));
	ASSERT(p != NULL, "CAN NOT ALLOCATE FOR", name, 0);
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

rmlock(name)
char *name;
{
	register int i;
	int ret;

	for (i = 0; i < MAXLOCKS; i++) {
		if (Lockfile[i] == NULL)
			continue;
		if (name == NULL
		|| strcmp(name, Lockfile[i]) == SAME) {
			Nlocks--;
#ifdef FLOCK
			close(openfd[i]);
#endif
			unlink(Lockfile[i]);
			free(Lockfile[i]);
			Lockfile[i] = NULL;
		}
	}
	return;
}


/*  this stuff from pjw  */
/*  /usr/pjw/bin/recover - check pids to remove unnecessary locks */
/*	isalock(name) returns 0 if the name is a lock */
/*	unlock(name)  unlocks name if it is a lock*/
/*	onelock(pid,tempfile,name) makes lock a name
	on behalf of pid.  Tempfile must be in the same
	file system as name. */
/*	lock(pid,tempfile,names) either locks all the
	names or none of them */
isalock(name) char *name;
{
	struct stat xstat;
	if(stat(name,&xstat)<0) return(0);
	if(xstat.st_size!=sizeof(int)) return(0);
	return(1);
}
unlock(name) char *name;
{
	if(isalock(name)) return(unlink(name));
	else return(-1);
}
onelock(pid,tempfile,name) char *tempfile,*name;
{	int fd;
	fd=creat(tempfile,LCKMODE);
	if(fd<0) {
		return(-1);	
	}
	write(fd,(char *) &pid,sizeof(int));
	close(fd);
	if(link(tempfile,name)<0)
	{
		DEBUG(9, "in onelock, errno=%d\n",errno);
		unlink(tempfile);
		return(-1);
	}
	unlink(tempfile);
	chmod(name, 0644);
	return(0);
}
lock(pid,tempfile,names) char *tempfile,**names;
{	int i,j;
	for(i=0;names[i]!=0;i++)
	{	if(onelock(pid,tempfile,names[i])==0) continue;
		for(j=0;j<i;j++) unlink(names[j]);
		return(-1);
	}
	return(0);
}

#define LOCKPRE "LCK."

/***
 *	delock(s)	remove a lock file
 *	char *s;
 *
 *	return codes:  0  |  FAIL
 */

delock(s)
char *s;
{
	char ln[70];

	sprintf(ln, "%s/%s.%s", SPOOL, LOCKPRE, s);
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
	char lname[70];
	sprintf(lname, "%s/%s.%s", SPOOL, LOCKPRE, sys);
	return(ulockf(lname, (time_t) SLCKTIME ) < 0 ? FAIL : 0);
}



/***
 *	ultouch()	update 'change' time for lock files
 *
 *	-- mod by rti!trt --
 *	Only update ctime, not mtime or atime.
 *	The 'chmod' method permits cu(I)-like programs
 *	to determine how long uucp has been on the line.
 *	The old "change access, mod, and change time" method
 *	can be had by defining OLDTOUCH
 *
 *	return code - none
 */

ultouch()
{
#ifndef FLOCK
	time_t time();
	static time_t lasttouch = 0;
	register int i;
	struct ut {
		time_t actime;
		time_t modtime;
	} ut;

	ut.actime = time(&ut.modtime);
	/* Do not waste time touching locking files too often */
	if ((ut.actime - lasttouch) < 60)
		return;
	lasttouch = ut.actime;
	DEBUG(4, "ultouch\n", 0);

	for (i = 0; i < Nlocks; i++) {
		if (Lockfile[i] == NULL)
			continue;
#ifdef	OLDTOUCH
		utime(Lockfile[i], &ut);
#else
		chmod(Lockfile[i], LCKMODE);
#endif
	}
	return;
#endif;
}
