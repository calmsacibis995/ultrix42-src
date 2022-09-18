#ifndef lint
static	char	*sccsid = "@(#)storepwent.c	4.1	(ULTRIX)	7/2/90";
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
 * Modification history
 *
 * 6-Sep-89 D. A. Long
 *	Replace getpwent_local() with straight read of /etc/passwd (or file
 *	set if setpwfile() call). Using getpwent causes YP entries to be lost.
 *
 * 25-Jul-89 D. A. Long
 *	Fixed to make a copy of incoming passwd struct which would otherwise
 *	probably get clobbered by the first call to getpw*().  Notice when
 *	this function returns the incoming pwd will probably point to
 *	something else.  Also fixed error returns.  Also, use local versions
 *	of getpwent routines.
 *
 * 18-Jul-89 D. A. Long
 *	Ignore the data base if it is not present.  Update just the ASCII file.
 */

#include <sys/errno.h>
#include <sys/param.h>
#include <sys/file.h>
#include <stdio.h>
#include <pwd.h>
#include <ndbm.h>

extern char _pw_file[];
extern DBM *_pw_db;
extern struct passwd *getpwent_local();
extern char *malloc();

/*
 * Define to allocate and copy a string.
 */
#define	scopy(in,out)	{ if(in) { out=malloc(strlen(in)+1); strcpy(out, in);} \
	else out=(char *)0; }

/*
 * Internal function to make a copy of a passwd structure.
 */
static pcopy(in, out)
struct passwd *in, *out;
{
	scopy(in->pw_name, out->pw_name);
	scopy(in->pw_passwd, out->pw_passwd);
	out->pw_uid = in->pw_uid;
	out->pw_gid = in->pw_gid;
#ifndef	SYSTEM_FIVE
	out->pw_quota = in->pw_quota;
#else
	scopy(in->pw_age, out->pw_age);
#endif	SYSTEM_FIVE
	scopy(in->pw_comment, out->pw_comment);
	scopy(in->pw_gecos, out->pw_gecos);
	scopy(in->pw_dir, out->pw_dir);
	scopy(in->pw_shell, out->pw_shell);
}

/*
 * Internal function to safely get a line from the named stream.  If
 * the line is too long to fit into the buffer it is thrown away and a
 * new line is fetched until successful or end-of-file.
 */
static char *getline(string, len, file)
char *string;
int len;
FILE *file;
{
	register char *s;
	int c;

	while(s=fgets(string, len, file))
		if(strchr(string, '\n'))
			break;
		else
			while((c=getc(file)) != EOF && c != '\n') ;
	return s;
}

/*
 * Returns -1 for fail and 0 for success.
 */
int storepwent(pwd)
struct passwd *pwd;
{
	datum key, content;
	struct passwd *pass, pwd_save;
	char *cp, *tp, line[BUFSIZ+1], *tmpfile = "/etc/ptmp";
	char string[BUFSIZ+1];
	FILE *tfp, *pfp;
	int i, tfd, acctlen;

	pcopy(pwd, &pwd_save);
	pwd = &pwd_save;
	if(!(pfp=fopen(_pw_file, "r")))
		return -1;
	for(i=1; i <= 5; i++)
		if((tfd=open(tmpfile, O_RDWR|O_EXCL|O_CREAT, 0644)) < 0){
			if(errno == EEXIST) {
				sleep(i);
				continue;
			} else
				break;
		} else
			break;
	if(tfd < 0) {
		fclose(pfp);
		return -1;
	}
	if(!(tfp=fdopen(tfd, "w"))) {
		close(tfd);
		unlink(tmpfile);
		fclose(pfp);
		return -1;
	}
	if(_pw_db) {
		dbm_close(_pw_db);
		_pw_db = (DBM *) NULL;
	}
	if((_pw_db=dbm_open(_pw_file, O_RDWR, 0))) {
		if (flock(dbm_dirfno(_pw_db), LOCK_EX) < 0) {
			fclose(tfp);
			unlink(tmpfile);
			dbm_close(_pw_db);
			_pw_db = (DBM *)0;
			fclose(pfp);
			return -1;
		}
	}
	acctlen = strlen(pwd->pw_name);
	while(getline(string, sizeof string, pfp)) {
		i = strcspn(string, ":\n");
		if(i == acctlen && !strncmp(pwd->pw_name, string, i)) {
			fprintf(tfp, "%s:%s:%d:%d:%s:%s:%s\n",
				pwd->pw_name, pwd->pw_passwd, pwd->pw_uid,
				pwd->pw_gid, pwd->pw_gecos, pwd->pw_dir,
				pwd->pw_shell);
/*
	There's a bug in putpwent.  It treats negative UIDs as unsigned.
			putpwent(pwd, tfp);
*/
			if(_pw_db && *string != '+' && *string != '-') {
			cp = line;
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
			content.dptr = line;
			content.dsize = cp - line;
			key.dptr = pwd->pw_name;
			key.dsize = strlen(pwd->pw_name);
			if (dbm_store(_pw_db, key, content, DBM_REPLACE) < 0) {
				fclose(tfp);
				unlink(tmpfile);
				(void) flock(dbm_dirfno(_pw_db), LOCK_UN);
				dbm_close(_pw_db);
				_pw_db = (DBM *) 0;
				fclose(pfp);
				return -1;
			}
			i = pwd->pw_uid;
			key.dptr = (char *)&i;
			key.dsize = sizeof i;
			if (dbm_store(_pw_db, key, content, DBM_REPLACE) < 0) {
				fclose(tfp);
				unlink(tmpfile);
				(void) flock(dbm_dirfno(_pw_db), LOCK_UN);
				dbm_close(_pw_db);
				_pw_db = (DBM *) 0;
				fclose(pfp);
				return -1;
			}
			}
		} else
			fputs(string, tfp);
	}
	if(ferror(tfp)) {
		fclose(tfp);
		unlink(tmpfile);
		if(_pw_db) {
			(void) flock(dbm_dirfno(_pw_db), LOCK_UN);
			dbm_close(_pw_db);
			_pw_db = (DBM *) 0;
		}
		fclose(pfp);
		return -1;
	}
	if(fclose(tfp)) {
		unlink(tmpfile);
		if(_pw_db) {
			(void) flock(dbm_dirfno(_pw_db), LOCK_UN);
			dbm_close(_pw_db);
			_pw_db = (DBM *) 0;
		}
		fclose(pfp);
		return -1;
	}
	if(rename(tmpfile, _pw_file) < 0) {
		unlink(tmpfile);
		if(_pw_db) {
			(void) flock(dbm_dirfno(_pw_db), LOCK_UN);
			dbm_close(_pw_db);
			_pw_db = (DBM *) 0;
		}
		fclose(pfp);
		return -1;
	}
	if(_pw_db) {
		(void) flock(dbm_dirfno(_pw_db), LOCK_UN);
		dbm_close(_pw_db);
		_pw_db = (DBM *) 0;
	}
	fclose(pfp);
	return 0;
}
