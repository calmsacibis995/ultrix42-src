#ifndef lint
static	char	*sccsid = "@(#)getpwent.c	4.2	(ULTRIX)	9/4/90";
#endif lint
/************************************************************************
 *									*
 *			Copyright (c) 1988, 1990 by			*
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
 *   Sun Microsystems, Inc,   and   from   Bell Laboratories.  Use, 	*
 *   duplication, or disclosure is  subject  to	restrictions  under  	*
 *   license  agreements  with  Sun Microsystems, Inc. and with AT&T.	*
 *									*
 *   The information in this software is subject to change  without	*
 *   notice  and should not be construed as a commitment by Digital	*
 *   Equipment Corporation.						*
 *									*
 *   Digital assumes no responsibility for the use  or  reliability	*
 *   of its software on equipment which is not supplied by Digital.	*
 *									*
 ************************************************************************/
/****************************************************************
 *								*
 *  Licensed to Digital Equipment Corporation, Maynard, MA	*
 *		Copyright 1985 Sun Microsystems, Inc.		*
 *			All rights reserved.			*
 *								*
 ****************************************************************/
/*
 * Modification History:
 *
 * 28-Feb-90	dlong
 *	Fixed getpwuid_local() to not use the sizeof uid_t for database
 *	accesses.  The uid is always stored as an int in the hashed
 *	passwd database.
 *
 * 17-Oct-89	sue
 *	Changed svc_getpwflag initial value to -2 and now perform a
 *	check in getpwent to see if the setpwent has been called yet.
 *
 * 24-Jul-89	logcher
 *	Removed generic setpwent and endpwent calls from generic
 *	getpwnam and getpwuid.  Added the specific set and end calls
 *	in the specific get routines.  Changed setpwent_yp to return
 *	NULL on failure, not exit 1.
 *
 * 07-Jun-89	D. Long
 *	Modified to handle 4.3BSD style hashed password data base.
 *
 * 16-May-89	logcher
 *      Modularized the code to have separate local, yp, bind/hesiod
 *      routines.  Added front end to check the /etc/svc.conf file
 *      for the service ordering.
 *
 * 19-Aug-88	Richard Hart
 *	Made many changes to force "bad" passwd entries to be skipped.
 *	(See interpret() routine )
 */


#include <sys/param.h>
#include <sys/file.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <stdio.h>
#include <pwd.h>
#include <ctype.h>
#include <ndbm.h>
/* The following line is because rcpsvc/ypclnt.h will redefine the
   typedef datum if we don't define DATUM.
*/
#define	DATUM
#include <netdb.h>
#include <sys/svcinfo.h>
#include <rpc/rpc.h>
#include <rpcsvc/ypclnt.h>

#include <rpcsvc/yp_prot.h>
#include <rpcsvc/ypv1_prot.h>
#include <strings.h>
#include <hesiod.h>
	
#if defined(lint) && !defined(DEBUG)
#define DEBUG
#endif

#define MAXINT 0x7fffffff
#define TIMEOUT 30
#define INTER_TRY 10
#define NOBODY -2

#define	PWSKIP(s)	while(*(s) && *(s) != ':' && *(s) != '\n') \
				++(s); \
			if (*(s) && *(s) == ':') colon_count++; \
			if (*(s)) *(s)++ = 0;
#define ATOI(p,pout) {\
		register char *p_atoi = p;	\
		register int n_atoi;		\
		register int f_atoi;		\
		n_atoi = 0;			\
		f_atoi = 0;			\
		for(;;p_atoi++) {		\
			switch(*p_atoi) {	\
			case ' ':		\
			case '\t':		\
				continue;	\
			case '-':		\
				f_atoi++;	\
			case '+':		\
				p_atoi++;	\
			}			\
			break;			\
		}				\
		while(*p_atoi >= '0' && *p_atoi <= '9')			\
			n_atoi = n_atoi*10 + *p_atoi++ - '0';		\
		(pout) = (f_atoi ? -n_atoi : n_atoi);			\
	}

extern char *strcpy();
extern char *malloc();

int _pw_stayopen	= 0;
static char *domain;
static struct passwd NULLPW = {NULL, NULL, 0, 0, 0, NULL, NULL, NULL, NULL};
#define	PASSWD		"/etc/passwd"
char _pw_file[MAXPATHLEN]	= PASSWD;
static char EMPTY[] = "";
static FILE *pwf = NULL;	/* pointer into /etc/passwd */
DBM *_pw_db = NULL;	/* pointer into passwd data base */
static char *yp;		/* pointer into yellow pages */
static int yplen;
static char *oldyp = NULL;	
static int oldyplen;
static struct passwd passwd;
static struct svcinfo *svcinfo;
static char buf[HES_BUFMAX];
static struct list {
    char *name;
    struct list *nxt
} *minuslist;			/* list of - items */
int svc_getpwflag = -2;
int svc_getpwbind;

static struct passwd *interpret();
static struct passwd *interpretwithsave();
static struct passwd *save();
static struct passwd *getnamefromyellow();
static struct passwd *getuidfromyellow();
static struct passwd *pwcommon();
static char *pwskip();
char *yellowup();

/*
 *  Declare all service routines
 */

int setpwent_local ();
int setent_bind ();
int setpwent_yp ();
int endpwent_local ();
int endent_bind ();
int endpwent_yp ();
struct passwd *getpwent_local ();
struct passwd *getpwent_bind ();
struct passwd *getpwent_yp ();
struct passwd *getpwuid_local (); 
struct passwd *getpwuid_bind (); 
struct passwd *getpwuid_yp (); 
struct passwd *getpwnam_local ();
struct passwd *getpwnam_bind ();	
struct passwd *getpwnam_yp ();

static int	(*setpwents []) ()={
		setpwent_local,
		setpwent_yp,
		setent_bind
};
static int 	(*endpwents []) ()={
		endpwent_local,
		endpwent_yp,
		endent_bind
};
static struct passwd * (*getpwents []) ()={
		getpwent_local,
		getpwent_yp,
		getpwent_bind
};
static struct passwd * (*getpwuids []) ()={
		getpwuid_local,
		getpwuid_yp,
		getpwuid_bind
};
static struct passwd * (*getpwnames []) ()={
		getpwnam_local,
		getpwnam_yp,
		getpwnam_bind
};


/*
 * generic getpw service routines
 */

setpwfile(file)
char *file;
{
	endpwent_local();
	strncpy(_pw_file, file, sizeof _pw_file);
}

setpwent ()
{
	register i;

	svc_getpwflag = -1;
	svc_getpwbind = 0;
	if ((svcinfo = getsvc()) != NULL)
		for (i=0; (svc_lastlookup = svcinfo->svcpath[SVC_PASSWD][i]) != SVC_LAST; i++)
			(*(setpwents [svcinfo->svcpath[SVC_PASSWD][i]])) ();
}

endpwent ()
{
	register i;

	svc_getpwflag = -1;
	svc_getpwbind = 0;
	if ((svcinfo = getsvc()) != NULL)
		for (i=0; (svc_lastlookup = svcinfo->svcpath[SVC_PASSWD][i]) != SVC_LAST; i++)
			(*(endpwents [svcinfo->svcpath[SVC_PASSWD][i]])) ();
}

struct passwd *
getpwent()
{
	register struct passwd *p=NULL;
	register i;

	/*
	 * Check if setpwent was not made yet
	 */
	if (svc_getpwflag == -2)
		setpwent();
	/*
	 * Check if this is the first time through getpwent
	 */
	if (svc_getpwflag == -1) {
		/*
		 * If it is, init the svcinfo structure
		 */
		if ((svcinfo = getsvc()) == NULL)
			return((struct passwd *)NULL);
		i = 0;
	}
	else {
		/*
		 * If it is not, set the index to the last used one
		 */
		i = svc_getpwflag;
	}
	for (; (svc_lastlookup = svcinfo->svcpath[SVC_PASSWD][i]) != SVC_LAST; i++)
		if (p = ((*(getpwents [svcinfo->svcpath[SVC_PASSWD][i]])) () )) {
			svc_getpwflag = i;
			break;
		}
	return(p);
}

struct passwd *
getpwnam (name)
	register char *name;
{
	register struct passwd *p=NULL;
	register i;

	/* avoid null pointer de-reference on mips */
	if (name == 0)
		return(0);
	if ((svcinfo = getsvc()) != NULL)
		for (i=0; (svc_lastlookup = svcinfo->svcpath[SVC_PASSWD][i]) != SVC_LAST; i++)
			if (p = ((*(getpwnames [svcinfo->svcpath[SVC_PASSWD][i]])) (name) ))
				break;
	return(p);
}

struct passwd *
getpwuid (uid)
	register uid_t uid;
{
	register struct passwd *p=NULL;
	register i;

	if ((svcinfo = getsvc()) != NULL)
		for (i=0; (svc_lastlookup = svcinfo->svcpath[SVC_PASSWD][i]) != SVC_LAST; i++)
			if (p = ((*(getpwuids [svcinfo->svcpath[SVC_PASSWD][i]])) (uid) ))
				break;
	return(p);
}

/*
 * specific getpw service routines
 */

static struct passwd *
fetchpw(key)
	datum key;
{
        register char *cp, *tp;
	static char line[BUFSIZ+1];
	int i;

	key = dbm_fetch(_pw_db, key);
	if (key.dptr == 0)
                return ((struct passwd *)NULL);
        cp = key.dptr;
	tp = line;

#define	EXPAND(e)	passwd.pw_/**/e = tp; while (*tp++ = *cp++);
	EXPAND(name);
	EXPAND(passwd);
	bcopy(cp, &i, sizeof i);
	passwd.pw_uid = i;
	cp += sizeof i;
	bcopy(cp, &i, sizeof i);
	passwd.pw_gid = i;
	cp += sizeof i;
#ifndef	SYSTEM_FIVE
	bcopy(cp, &i, sizeof i);
	passwd.pw_quota = i;
#else
	passwd.pw_age = (char *) 0;
#endif
	cp += sizeof i;
	EXPAND(comment);
	EXPAND(gecos);
	EXPAND(dir);
	EXPAND(shell);
        return (&passwd);
}

struct passwd *
getpwnam_local(name)
	char *name;
{
	struct passwd *pw;
	datum key;
	char namekey[10];

#ifdef DEBUG
	fprintf(stderr, "getpwnam_local(%s)\n", name);
#endif DEBUG

	if(_pw_db == (DBM *)0 && (_pw_db=dbm_open(_pw_file, O_RDONLY)) == (DBM *) 0) {
oldcode:
		setpwent_local();
		while (pw = getpwent_local()) {
			if (strcmp(pw->pw_name, name) == 0)
				break;
		}
	} else {
		strncpy(namekey, name, sizeof namekey);
		key.dptr = namekey;
		key.dsize = strlen(namekey);
		if (flock(dbm_dirfno(_pw_db), LOCK_SH) < 0) {
			dbm_close(_pw_db);
			_pw_db = (DBM *)0;
			goto oldcode;
		}
		pw = fetchpw(key);
		(void) flock(dbm_dirfno(_pw_db), LOCK_UN);
	}
	if(!_pw_stayopen)
		endpwent_local();
	return(pw);
}

struct passwd *
getpwnam_bind(name)
	char *name;
{
	char **pp;
	
#ifdef DEBUG
	fprintf(stderr, "getpwnam_bind(%s)\n", name);
#endif DEBUG
	setent_bind(0);
	pp = hes_resolve(name, "passwd");
	endent_bind();
	return pwcommon(pp);
}

struct passwd *
getpwnam_yp(name)
	char *name;
{
	struct passwd *pw;
	char line[BUFSIZ+1];

#ifdef DEBUG
	fprintf(stderr, "getpwnam_yp(%s)\n", name);
#endif DEBUG
	setpwent_yp();
	if (!pwf)
		return(NULL);
	while (fgets(line, BUFSIZ, pwf) != NULL) {
		pw = interpret(line, strlen(line));
		if (pw != NULL)		/* skip any bad passwd lines */
			if (matchname(line, &pw, name)) {
				endpwent_yp();
				return(pw);
			}
	}
	endpwent_yp();
	return(NULL);
}

struct passwd *
getpwuid_local(uid)
	uid_t uid;
{
	static struct passwd *pw;
	datum key;
	int i;

#ifdef DEBUG
	fprintf(stderr, "getpwuid_local(%d)\n", uid);
#endif DEBUG

	if(_pw_db == (DBM *)0 && (_pw_db=dbm_open(PASSWD, O_RDONLY)) == (DBM *) 0) {
oldcode:
		setpwent_local();
		while (pw = getpwent_local()) {
			if (pw->pw_uid == uid)
				break;
		}
	} else {
		i = uid;
		key.dptr = (char *) &i;
		key.dsize = sizeof i;
		if (flock(dbm_dirfno(_pw_db), LOCK_SH) < 0) {
			dbm_close(_pw_db);
			_pw_db = (DBM *)0;
			goto oldcode;
		}
		pw = fetchpw(key);
		(void) flock(dbm_dirfno(_pw_db), LOCK_UN);
	}
	if(!_pw_stayopen)
		endpwent_local();
	return(pw);
}

struct passwd *
getpwuid_bind(uid)
	register uid_t uid;
{
	char **pp, uidbuf[6];

#ifdef DEBUG
	fprintf(stderr, "getpwuid_bind(%d)\n", uid);
#endif DEBUG
	setent_bind(0);
	sprintf(uidbuf, "%u", uid);
	pp = hes_resolve(uidbuf, "passwd");
	endent_bind();
	return pwcommon(pp);
}

struct passwd *
getpwuid_yp(uid)
	register uid_t uid;
{
	struct passwd *pw;
	char line[BUFSIZ+1];

#ifdef DEBUG
	fprintf(stderr, "getpwuid_yp(%d)\n", uid);
#endif DEBUG
	setpwent_yp();
	if (!pwf)
		return(NULL);
	while (fgets(line, BUFSIZ, pwf) != NULL) {
		pw = interpret(line, strlen(line));
		if (pw != NULL)		/* skip any bad passwd lines */
		    if (matchuid(line, &pw, uid)) {
			endpwent_yp();
			return(pw);
		}
	}
	endpwent_yp();
	return(NULL);
}

setpwent_local()
{
	if (pwf == NULL)
		pwf = fopen(_pw_file, "r");
	else
		rewind(pwf);
}

/*
 * setent_bind() is in getcommon.c
 */

setpwent_yp()
{
	setpwent_local();
	if ((domain = yellowup(1)) == NULL)
		return (NULL);
	if (yp)
		free(yp);
	yp = NULL;
	freeminuslist();
}

endpwent_local()
{
	_pw_stayopen = 0;
	if (pwf != NULL) {
		(void) fclose(pwf);
		pwf = NULL;
	}
	if (_pw_db) {
		dbm_close(_pw_db);
		_pw_db = NULL;
	}
	endnetgrent();
}

/*
 * endent_bind() is in getcommon.c
 */

endpwent_yp()
{
	endpwent_local();
	if (yp)
		free(yp);
	yp = NULL;
	freeminuslist();
}

struct passwd *
getpwent_local()
{
	char line1[BUFSIZ+1];
	static struct passwd *pw;

#ifdef DEBUG
	fprintf(stderr, "getpwent_local()\n");
#endif DEBUG
	if (pwf == NULL && (pwf = fopen(_pw_file, "r")) == NULL)
		return(NULL); 
again:
	if ((fgets(line1, BUFSIZ, pwf)) == NULL) {
		setpwent_local();
		return(NULL);
	}
	pw = interpret(line1, strlen(line1));
	switch(line1[0]) {
		case '+':
			goto again;
		case '-':
			goto again;
		default:
			return(pw);
	}
}

struct passwd *
getpwent_bind()
{
	char bindbuf[64];
	struct passwd *pw = NULL;

	sprintf(bindbuf, "passwd-%d", svc_getpwbind);
#ifdef DEBUG
	fprintf(stderr, "getpwent_bind(%s)\n", bindbuf);
#endif DEBUG
	if ((pw = getpwnam_bind(bindbuf)) == NULL)
		return(NULL);
	svc_getpwbind++;
	return(pw);
}

struct passwd *
getpwent_yp()
{
	char line1[BUFSIZ+1];
	static struct passwd *savepw;
	struct passwd *pw;
	char *user = NULL; 
	char *mach = NULL;
	char *dom = NULL;

#ifdef DEBUG
	fprintf(stderr, "getpwent_yp()\n");
#endif DEBUG
	if (pwf == NULL)
		if((pwf=fopen(_pw_file, "r")) == NULL)
			return NULL;
	if((domain=yellowup(1)) == NULL)
		return NULL;
	for (;;) {
		if (yp) {
			pw = interpretwithsave(yp, yplen, savepw); 
			free(yp);
			getnextfromyellow();
			if (pw != NULL && !onminuslist(pw)) {
				return(pw);
			}
		} else if (getnetgrent(&mach,&user,&dom)) {
			if (user) {
				pw = getnamefromyellow(user, savepw);
				if (pw != NULL && !onminuslist(pw)) {
					return(pw);
				}
			}
		} else {
			endnetgrent();
			do {
				if (fgets(line1, BUFSIZ, pwf) == NULL)  {
					return(NULL);
				}
				pw = interpret(line1, strlen(line1));
				}
			while(pw == NULL);
			switch(line1[0]) {
			case '+':
				if (strcmp(pw->pw_name, "+") == 0) {
					getfirstfromyellow();
					savepw = save(pw);
				} else if (line1[1] == '@') {
					savepw = save(pw);
					if (innetgr(pw->pw_name+2,(char *) NULL,
					"*",domain)) {
					/* include the whole yp database */
						getfirstfromyellow();
					} else {
						setnetgrent(pw->pw_name+2);
					}
				} else {
					/* 
					 * else look up this entry
					 * in yellow pages
				 	 */
					savepw = save(pw);
					pw = getnamefromyellow(pw->pw_name+1,
									savepw);
					if (pw != NULL && !onminuslist(pw)) {
						return(pw);
					}
				}
				break;
			case '-':
				if (line1[1] == '@') {
					if (innetgr(pw->pw_name+2,(char *) NULL,
					"*",domain)) {
						/* everybody was subtracted */
						return(NULL);
					}
					setnetgrent(pw->pw_name+2);
					while (getnetgrent(&mach,&user,&dom)) {
						if (user) {
							addtominuslist(user);
						}
					}
					endnetgrent();
				} else {
					addtominuslist(pw->pw_name+1);
				}
				break;
			default:
				if (!onminuslist(pw)) {
					return(pw);
				}
				break;
			}
		}
	}
}

static
matchname(line1, pwp, name)
	register char *line1;
	register struct passwd **pwp;
	register char *name;
{
	register struct passwd *savepw;
	register struct passwd *pw = *pwp;

	switch(line1[0]) {
		case '+':
			if (strcmp(pw->pw_name, "+") == 0) {
				savepw = save(pw);
				pw = getnamefromyellow(name, savepw);
				if (pw) {
					*pwp = pw;
					return(1);
				}
				else
					return(0);
			}
			if (line1[1] == '@') {
				if (innetgr(pw->pw_name+2,(char *) NULL,
				name,domain)) {
					savepw = save(pw);
					pw = getnamefromyellow(name,savepw);
					if (pw) {
						*pwp = pw;
						return(1);
					}
				}
				return(0);
			}
			if (strcmp(pw->pw_name+1, name) == 0) {
				savepw = save(pw);
				pw = getnamefromyellow(pw->pw_name+1, savepw);
				if (pw) {
					*pwp = pw;
					return(1);
				}
				else
					return(0);
			}
			break;
		case '-':
			if (line1[1] == '@') {
				if (innetgr(pw->pw_name+2,(char *) NULL,
				name,domain)) {
					*pwp = NULL;
					return(1);
				}
			}
			else if (strcmp(pw->pw_name+1, name) == 0) {
				*pwp = NULL;
				return(1);
			}
			break;
		default:
			if (pw->pw_name[0] == name[0])	/* avoid calls */
				if (strcmp(pw->pw_name, name) == 0)
					return(1);
	}
	return(0);
}

static
matchuid(line1, pwp, uid)
	register char *line1;
	register struct passwd **pwp;
	register int uid;
{
	register struct passwd *savepw;
	register struct passwd *pw = *pwp;
	char group[256];

	switch(line1[0]) {
		case '+':
			if (strcmp(pw->pw_name, "+") == 0) {
				savepw = save(pw);
				pw = getuidfromyellow(uid, savepw);
				if (pw) {
					*pwp = pw;
					return(1);
				} else {
					return(0);
				}
			}
			if (line1[1] == '@') {
				(void) strcpy(group,pw->pw_name+2);
				savepw = save(pw);
				pw = getuidfromyellow(uid,savepw);
				if (pw && innetgr(group,(char *) NULL,
				pw->pw_name,domain)) {
					*pwp = pw;
					return(1);
				} else {
					return(0);
				}
			}
			savepw = save(pw);
			pw = getnamefromyellow(pw->pw_name+1, savepw);
			if (pw && pw->pw_uid == uid) {
				*pwp = pw;
				return(1);
			} else
				return(0);
		case '-':
			if (line1[1] == '@') {
				(void) strcpy(group,pw->pw_name+2);
				pw = getuidfromyellow(uid,&NULLPW);
				if (pw && innetgr(group,(char *) NULL,
				pw->pw_name,domain)) {
					*pwp = NULL;
					return(1);
				}
			} else if (uid == uidof(pw->pw_name+1)) {
				*pwp = NULL;
				return(1);
			}
			break;
		default:
			if (pw->pw_uid == uid)
				return(1);
	}
	return(0);
}

static
uidof(name)
	char *name;
{
	struct passwd *pw;
	struct passwd nullpw;
	
	nullpw = NULLPW;
	pw = getnamefromyellow(name, &nullpw);
	if (pw)
		return(pw->pw_uid);
	else
		return(MAXINT);
}

static
getnextfromyellow()
{
	int reason = 0;
	char *key = NULL;
	int keylen = 0;

	reason = yp_next(domain, "passwd.byname",oldyp, oldyplen, &key,
	&keylen,&yp,&yplen);
	if (reason) {
#ifdef DEBUG
		fprintf(stderr, "reason yp_next failed is %d\n", reason);
#endif DEBUG
		yp = NULL;
	}
	if (oldyp)
		free(oldyp);
	oldyp = key;
	oldyplen = keylen;
}

static
getfirstfromyellow()
{
	int reason = 0;
	char *key = NULL;
	int keylen = 0;
	
	reason =  yp_first(domain, "passwd.byname", &key, &keylen, &yp, &yplen);
	if (reason) {
#ifdef DEBUG
		fprintf(stderr, "reason yp_first failed is %d\n", reason);
#endif DEBUG
		yp = NULL;
	}
	if (oldyp)
		free(oldyp);
	oldyp = key;
	oldyplen = keylen;
}

static struct passwd *
getnamefromyellow(name, savepw)
	char *name;
	struct passwd *savepw;
{
	struct passwd *pw;
	int reason = 0;
	char *val = NULL;
	int vallen = 0;
	
	reason = yp_match(domain, "passwd.byname", name, strlen(name)
		, &val, &vallen);
	if (reason) {
#ifdef DEBUG
		fprintf(stderr, "reason yp_next failed is %d\n", reason);
#endif DEBUG
		return(NULL);
	} else {
		pw = interpret(val, vallen);
		if (pw == NULL)
			return(NULL);
		free(val);
		if (savepw->pw_passwd && *savepw->pw_passwd)
			pw->pw_passwd =  savepw->pw_passwd;
		if (savepw->pw_gecos && *savepw->pw_gecos)
			pw->pw_gecos = savepw->pw_gecos;
		if (savepw->pw_dir && *savepw->pw_dir)
			pw->pw_dir = savepw->pw_dir;
		if (savepw->pw_shell && *savepw->pw_shell)
			pw->pw_shell = savepw->pw_shell;
		return(pw);
	}
}

static struct passwd *
getuidfromyellow(uid, savepw)
	int uid;
	struct passwd *savepw;
{
	struct passwd *pw;
	int reason = 0;
	char *val = NULL;
	int vallen = 0;
	char uidstr[20];
	
	(void) sprintf(uidstr, "%d", uid);
	reason = yp_match(domain, "passwd.byuid", uidstr, strlen(uidstr)
		, &val, &vallen);
	if (reason) {
#ifdef DEBUG
		fprintf(stderr, "reason yp_next failed is %d\n", reason);
#endif DEBUG
		return(NULL);
	} else {
		pw = interpret(val, vallen);
		if (pw == NULL)
			return(NULL);
		free(val);
		if (savepw->pw_passwd && *savepw->pw_passwd)
			pw->pw_passwd =  savepw->pw_passwd;
		if (savepw->pw_gecos && *savepw->pw_gecos)
			pw->pw_gecos = savepw->pw_gecos;
		if (savepw->pw_dir && *savepw->pw_dir)
			pw->pw_dir = savepw->pw_dir;
		if (savepw->pw_shell && *savepw->pw_shell)
			pw->pw_shell = savepw->pw_shell;
		return(pw);
	}
}

static struct passwd *
interpretwithsave(val, len, savepw)
	register char *val;
	register struct passwd *savepw;
{
	register struct passwd *pw;
	
	pw = interpret(val, len);
	if (pw == NULL)
		return(NULL);
	if (savepw->pw_passwd && *savepw->pw_passwd)
		pw->pw_passwd =  savepw->pw_passwd;
	if (savepw->pw_gecos && *savepw->pw_gecos)
		pw->pw_gecos = savepw->pw_gecos;
	if (savepw->pw_dir && *savepw->pw_dir)
		pw->pw_dir = savepw->pw_dir;
	if (savepw->pw_shell && *savepw->pw_shell)
		pw->pw_shell = savepw->pw_shell;
	return(pw);
}
/*
 * Build a passwd structure from the passwd file entry line in val.
 * A pointer to the created passwd structure will be returned, unless the 
 * passwd entry in val is "bad".  A bad entry is defined to be one
 * that has less than 4 colons and that does not start with a '+' or
 * a '-' (for yp).
 */
static struct passwd *
interpret(val, len)
	char *val;
	int len;
{
	register char *p, *v;
	register int i;
	int colon_count = 0;
	static struct passwd passwd;
	static char line[BUFSIZ+1];
	register struct passwd *pw = &passwd;

	p = line;
	v = val;
	for (i = 0; i < len; i++) {
		if ((*p++ = *v++) == '\0') {
			while (++i < len)
				*p++ = '\0';
			break;
		}
	}
	*p++ = '\n';
	*p = 0;
	p = line;

	pw->pw_name = p;
	PWSKIP(p);
	pw->pw_passwd = p;
	PWSKIP(p);
	if (colon_count < 2 && (pw->pw_name[0] != '+' && pw->pw_name[0] != '-'))
		pw->pw_uid = NOBODY;
	else ATOI(p,pw->pw_uid);
	PWSKIP(p);
	if (colon_count < 3 && (pw->pw_name[0] != '+' && pw->pw_name[0] != '-'))
		pw->pw_gid = NOBODY;
	else
		ATOI(p,pw->pw_gid);

	/* The only difference in the pwd structure between ULTRIX
	 * and System V is in the definition of the following field.
	 * However, both systems ignore it because it is not present
	 * in the actual password file.
	 */
#ifndef SYSTEM_FIVE
	pw->pw_quota = 0;
#else	SYSTEM_FIVE
	pw->pw_age = (char *) 0;
#endif	SYSTEM_FIVE

	pw->pw_comment = EMPTY;
	PWSKIP(p);
	pw->pw_gecos = p;
	PWSKIP(p);
	pw->pw_dir = p;
	PWSKIP(p);
	pw->pw_shell = p;
	while(*p && *p != '\n') p++;
	*p = '\0';
	if (colon_count < 4 && (pw->pw_name[0] != '+' && pw->pw_name[0] != '-'))
		return(NULL);
	else 	return(pw);
}

static
freeminuslist()
{
	struct list *ls;
	
	for (ls = minuslist; ls != NULL; ls = ls->nxt) {
		free(ls->name);
		free((char *) ls);
	}
	minuslist = NULL;
}

static
addtominuslist(name)
	char *name;
{
	struct list *ls;
	char *buf;
	
	ls = (struct list *) malloc(sizeof(struct list));
	buf = malloc((unsigned) strlen(name) + 1);
	(void) strcpy(buf, name);
	ls->name = buf;
	ls->nxt = minuslist;
	minuslist = ls;
}

/* 
 * save away psswd, gecos, dir and shell fields, which are the only
 * ones which can be specified in a local + entry to override the
 * value in the yellow pages
 */
static struct passwd *
save(pw)
	struct passwd *pw;
{
	static struct passwd *sv;

	/* free up stuff from last call */
	if (sv) {
		free(sv->pw_passwd);
		free(sv->pw_gecos);
		free(sv->pw_dir);
		free(sv->pw_shell);
		free((char *) sv);
	}
	sv = (struct passwd *) malloc(sizeof(struct passwd));

	sv->pw_passwd = malloc((unsigned) strlen(pw->pw_passwd) + 1);
	(void) strcpy(sv->pw_passwd, pw->pw_passwd);

	sv->pw_gecos = malloc((unsigned) strlen(pw->pw_gecos) + 1);
	(void) strcpy(sv->pw_gecos, pw->pw_gecos);

	sv->pw_dir = malloc((unsigned) strlen(pw->pw_dir) + 1);
	(void) strcpy(sv->pw_dir, pw->pw_dir);

	sv->pw_shell = malloc((unsigned) strlen(pw->pw_shell) + 1);
	(void) strcpy(sv->pw_shell, pw->pw_shell);

	return(sv);
}

static
onminuslist(pw)
	struct passwd *pw;
{
	struct list *ls;
	register char *nm;

	nm = pw->pw_name;
	for (ls = minuslist; ls != NULL; ls = ls->nxt) {
		if (strcmp(ls->name,nm) == 0) {
			return(1);
		}
	}
	return(0);
}
	
static
struct passwd *
pwcommon(pp)
char **pp;
{
	register char *p;

	if (pp == NULL)
		return(NULL);
	/* choose only the first response (only 1 expected) */
	strcpy(buf, pp[0]);
	while(*pp) free(*pp++); /* necessary to avoid leaks */
	p = buf;
	passwd.pw_name = p;
	p = pwskip(p);
	passwd.pw_passwd = p;
	p = pwskip(p);
	passwd.pw_uid = atoi(p);
	p = pwskip(p);
	passwd.pw_gid = atoi(p);
#ifndef	SYSTEM_FIVE
	passwd.pw_quota = 0;
#else
	passwd.pw_age = (char *) 0;
#endif
	passwd.pw_comment = EMPTY;
	p = pwskip(p);
	passwd.pw_gecos = p;
	p = pwskip(p);
	passwd.pw_dir = p;
	p = pwskip(p);
	passwd.pw_shell = p;
	while (*p && *p != '\n')
		p++;
	*p = '\0';
	return(&passwd);
}

static char *
pwskip(p)
register char *p;
{
	while (*p && *p != ':' && *p != '\n')
		++p;
	if (*p)
		*p++ = 0;
	return(p);
}
