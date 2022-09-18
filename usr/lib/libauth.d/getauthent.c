#ifndef lint
static	char	*sccsid = "@(#)getauthent.c	4.3	(ULTRIX)	11/15/90";
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
 *   The information in this software is subject to change  without	*
 *   notice  and should not be construed as a commitment by Digital	*
 *   Equipment Corporation.						*
 *									*
 *   Digital assumes no responsibility for the use  or  reliability	*
 *   of its software on equipment which is not supplied by Digital.	*
 *									*
 ************************************************************************/
/*
 * Modification History:
 *
 * 14-Nov-90 D. Long
 *	Removed all use of ASCII auth file.  This is no longer
 *	maintained by storeauthent(), nor is it used by getauthent().
 *
 * 13-Nov-89  sue
 *    Changed svc_getauthflag initial value to -2 and now perform a
 *    check in getauthent to see if the setauthent has been called yet.
 *
 * 31-Aug-89 D. Long
 *	Put in place holder functions for YP case.
 *
 * 15-Aug-89 D. Long
 *	Fixed NULL pointer problem in asciiauth().
 *
 * 11-Aug-89  sue
 *    Fixed bug in getauthent.  Need a flag to hold which service
 *    is currently being scanned.
 *
 * 20-Jul-89  logcher
 *    Added setent_bind and endent_bind to getauthuid_bind.  Changed
 *    SVC_PASSWD to SVC_AUTH in generic auth calls.
 *
 * 30-May-89	logcher
 *	Added svcinfo handling.
 *
 * 29-May-89	dlong
 *	Created.
 *
 */

#include <sys/file.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <ndbm.h>
#include "auth.h"
#include <hesiod.h>
#include <sys/svcinfo.h>

/*
  Library to support conversion of auth file entries to/from ASCII.
*/

extern DBM *_auth_db;
static char buff[1025];
static int buflen;
static struct svcinfo *svcinfo;
int svc_getauthflag = -2;
int svc_getauthbind;
extern int setent_bind();
extern int endent_bind();
static AUTHORIZATION *getauthname_bind();

static stringcat(string, i)
char *string;
int i;
{
	while(buff[buflen] = *string++)
		buflen++;
	buff[buflen++] = ':';
}

static char *itoa(i)
int i;
{
	char string[12];
	char *cp;
	int minus;

	cp = &string[11];
	*cp-- = '\0';
	if(i < 0) {
		i = -i;
		minus = 1;
	} else
		minus = 0;
	do {
		*cp-- = (i % 10) + '0';
		i /= 10;
	} while (i != 0);
	if(minus)
		*cp = '-';
	else
		cp++;
	while(buff[buflen] = *cp++)
		buflen++;
	buff[buflen++] = ':';
}

/*
  Function to convert a nibble into hex ASCII.
*/
static hex_nibble(nibble)
int nibble;
{
	if(nibble >= 0 && nibble <= 9)
		return nibble+'0';
	else if(nibble > 9 && nibble < 16)
		return nibble-10+'a';
	else
		return (int) '?';
}

/*
  Functions to separate nibbles of a byte.
*/
static hi_nibble(i)
int i;
{
	return (i >> 4)&0xf;
}

static lo_nibble(i)
int i;
{
	return (i & 0xf);
}

/*
  Function to convert a byte array into hex ASCII.
*/
static bytetohex(bytes, length)
char *bytes;
int length;
{
	int i, last;

	for(last=(length-1); last > 0 && !bytes[last]; last--)
		;
	for(i=last; i >= 0; i--) {
		buff[buflen++] = hex_nibble(hi_nibble(bytes[i]));
		buff[buflen++] = hex_nibble(lo_nibble(bytes[i]));
	}
	buff[buflen++] = ':';
}

char *asciiauth(auth)
AUTHORIZATION *auth;
{
	buflen = 0;
	itoa(auth->a_uid);
	stringcat(auth->a_password);
	itoa(auth->a_pass_mod);
	itoa(auth->a_pw_minexp);
	itoa(auth->a_pw_maxexp);
	bytetohex(&auth->a_authmask, sizeof (AUTH_MASK));
	itoa(auth->a_fail_count);
	itoa(auth->a_audit_id);
	bytetohex(&auth->a_audit_control, sizeof (AUDIT_CONTROL));
	bytetohex(auth->a_audit_mask, sizeof (AUDIT_MASK));
	bytetohex(auth->a_privs, sizeof (PRIVILEGE_MASK));
#ifdef	B1
	itoa(auth->a_min_level);
	itoa(auth->a_max_level);
	bytetohex(auth->a_categories, sizeof (CATEGORY_MASK));
#endif	B1
	buff[buflen-1] = '\0';
	return buff;
}

static int status;

static cpystring(string, len)
char *string;
int len;
{
	int i;
	char c;

	for(i=len; (c=buff[buflen]) && c != ':' && c != '\n'; i--)
		*string++ = buff[buflen++];
}

static int asciihex(byte)
char byte;
{
	if(byte >= '0' && byte <= '9')
		return byte-'0';
	else if(byte >= 'a' && byte <='f')
		return byte-'a'+10;
	else if(byte >= 'A' && byte <= 'F')
		return byte-'A'+10;
	else
		return 0;
}

static int hextobyte(string)
char *string;
{
	return (asciihex(*string)<<4) | asciihex(string[1]);
}

static int bytestring(string, len)
char *string;
int len;
{
	int i, slen;
	char *cp;

	slen = 0;
	for(cp=(&buff[buflen]); *cp && *cp != '\n' && *cp != ':'; cp++) {
		buflen++;
		slen++;
	}
	slen /= 2;
	if(slen > len)
		slen = len;
	for(i=0; i < slen; i++) {
		cp -= 2;
		string[i] = hextobyte(cp);
	}
	cp--;
	if(slen < len) {
		string[slen++] = asciihex(*cp);
		for(i=slen; i < len; i++)
			string[i] = 0;
	}
}

static int asciiint()
{
	int i=0;
	char c;

	while((c=buff[buflen]) && c != '\n' && c != ':') {
		if(c >= '0' && c <= '9')
			i = i*10 + c-'0';
		buflen++;
	}
	return i;
}

static int check(byte)
{
	if(buff[buflen] != byte) {
		puts("error in authconv");
		status = 1;
		return status;
	} else {
		buflen++;
		return 0;
	}
}

int binauth(string, auth)
char *string;
AUTHORIZATION *auth;
{
	status = 0;
	bzero(auth, sizeof *auth);
	strncpy(buff, string, sizeof buff);
	buflen = 0;
	auth->a_uid = asciiint();
	check(':');
	cpystring(auth->a_password, 24);
	check(':');
	auth->a_pass_mod = asciiint();
	check(':');
	auth->a_pw_minexp = asciiint();
	check(':');
	auth->a_pw_maxexp = asciiint();
	check(':');
	bytestring(&auth->a_authmask, sizeof (AUTH_MASK));
	check(':');
	auth->a_fail_count = asciiint();
	check(':');
	auth->a_audit_id = asciiint();
	check(':');
	bytestring(&auth->a_audit_control, sizeof (AUDIT_CONTROL));
	check(':');
	bytestring(auth->a_audit_mask, sizeof (AUDIT_MASK));
	check(':');
	bytestring(auth->a_privs, sizeof (PRIVILEGE_MASK));
#ifdef	B1
	check(':');
	auth->a_min_level = asciiint();
	check(':');
	auth->a_max_level = asciiint();
	check(':');
	bytestring(auth->a_categories, sizeof (CATEGORY_MASK));
#endif	B1
	if(buff[buflen] != '\n' && buff[buflen] != '\0') {
		puts("Bad line terminator");
		status = 1;
	}
	return status;
}

AUTHORIZATION *_auth = (AUTHORIZATION *) NULL;
static AUTHORIZATION auth;
static int db_open_flag=0, db_mode, auth_mode;
static char *auth_file = AUTHORIZATION_DB;

/*
 * Code for local case
 */
static int open_auth_db(mode)
int mode;
{
	_auth = (AUTHORIZATION *) NULL;
	if(db_open_flag && db_mode != mode) {
		close_auth();
		db_open_flag = 0;
	}
	if(!db_open_flag)
		if(open_auth(auth_file, mode))
			return -1;
		else {
			db_mode = mode;
			db_open_flag = 1;
		}
	return 0;
}

static AUTHORIZATION *getauthent_local()
{
	int first_time;
	UID user;

	_auth = (AUTHORIZATION *) NULL;
	if(db_open_flag)
		first_time = 0;
	else
		first_time = 1;
	if(open_auth_db(O_RDONLY) < 0)
		return _auth;
	if (flock(dbm_dirfno(_auth_db), LOCK_SH) < 0)
		return _auth;
	if(first_time) {
		if(first_auth_user(&user) >= 0)
			if(get_auth(user, &auth) >= 0)
				_auth = &auth;
	} else {
		if(next_auth_user(&user) >= 0)
			if(get_auth(user, &auth) >= 0)
				_auth = &auth;
	}
	(void) flock(dbm_dirfno(_auth_db), LOCK_UN);
	return _auth;
}

static AUTHORIZATION *getauthent_yp()
{
	return (AUTHORIZATION *) NULL;
}

static AUTHORIZATION *getauthent_bind()
{
	char bindbuf[64];
	AUTHORIZATION *auth = NULL;

	sprintf(bindbuf, "auth-%d", svc_getauthbind);
	if ((auth = getauthname_bind(bindbuf)) == NULL)
		return((AUTHORIZATION *)NULL);
	svc_getauthbind++;
	return(auth);
}

static setauthent_local()
{
	endauthent_local();
}

static setauthent_yp()
{}

/*
 * setent_bind() is in libc getcommon.c
 */

void setauthfile(file)
char *file;
{
	endauthent();
	auth_file = file;
}

static endauthent_local()
{
	_auth = (AUTHORIZATION *) NULL;
	if(db_open_flag) {
		close_auth();
		db_open_flag = 0;
	}
}

static endauthent_yp()
{}

/*
 * endent_bind() is in libc getcommon.c
 */

static AUTHORIZATION *getauthuid_local(uid)
int uid;
{
	_auth = (AUTHORIZATION *) NULL;
	if(open_auth_db(O_RDONLY) == 0) {
		if (flock(dbm_dirfno(_auth_db), LOCK_SH) < 0)
			return _auth;
		if(get_auth(uid, &auth) >= 0)
			_auth = &auth;
		(void) flock(dbm_dirfno(_auth_db), LOCK_UN);
	}
	return _auth;
}

static AUTHORIZATION *getauthuid_yp(uid)
int uid;
{
	return (AUTHORIZATION *) NULL;
}

static AUTHORIZATION *getauthuid_bind(uid)
int uid;
{
	char uidbuf[10], **pp;

	setent_bind(0);
	_auth = (AUTHORIZATION *) NULL;
	sprintf(uidbuf, "%u", uid);
	pp = hes_auth_resolve(uidbuf, "auth");
	endent_bind();
	if(pp != NULL)
		if(*pp) {
			binauth(*pp, &auth);
			while(*pp)
				free(*pp++);
			_auth = &auth;
		}
	else
		return(NULL);
	return _auth;
}

static AUTHORIZATION *
getauthname_bind(name)
	char *name;
{
	char **pp;

	setent_bind(0);
	_auth = (AUTHORIZATION *) NULL;
	pp = hes_auth_resolve(name, "auth");
	endent_bind();
	if(pp != NULL)
		if(*pp) {
			binauth(*pp, &auth);
			while(*pp)
				free(*pp++);
			_auth = &auth;
		}
	else
		return(NULL);
	return _auth;
}

static int storeauthent_local(authin)
AUTHORIZATION *authin;
{
	if(open_auth_db(O_RDWR))
		return -1;
	bcopy(authin, &auth, sizeof auth);
	if (flock(dbm_dirfno(_auth_db), LOCK_EX) < 0)
		return -1;
	if(set_auth(auth.a_uid, auth)) {
		(void) flock(dbm_dirfno(_auth_db), LOCK_UN);
		return -1;
	}
	endauthent_local();
	(void) flock(dbm_dirfno(_auth_db), LOCK_UN);
	return 0;
}

static int storeauthent_yp(authin)
AUTHORIZATION *authin;
{
	return -1;
}

static int storeauthent_bind(authin)
AUTHORIZATION *authin;
{
	return -1;
}

/*
 * Function arrays used by generic routines.
 */
static AUTHORIZATION *(*getauthents[])() = {
	getauthent_local, getauthent_yp, getauthent_bind
};

static AUTHORIZATION *(*getauthuids[])() = {
	getauthuid_local, getauthuid_yp, getauthuid_bind
};

static int (*setauthents[])() = {
	setauthent_local, setauthent_yp, setent_bind
};

static int (*endauthents[])() = {
	endauthent_local, endauthent_yp, endent_bind
};

static int (*storeauthents[])() = {
	storeauthent_local, storeauthent_yp, storeauthent_bind
};

/*
 * Code for generic case
 */
AUTHORIZATION *getauthent()
{
	AUTHORIZATION *authout=NULL;
	register i;

	/*
	 * Check if setauthent was not made yet
	 */
	if (svc_getauthflag == -2)
		setauthent();
	/*
	 * Check if this is the first time through getauthent
	 */
	if (svc_getauthflag == -1) {
		/*
		 * If it is, init the svcinfo structure
		 */
		if ((svcinfo = getsvc()) == NULL)
			return((AUTHORIZATION *)NULL);
		i = 0;
	}
	else {
		/*
		 * If it is not, set the index to the last used one
		 */
		i = svc_getauthflag;
	}
	for (; (svc_lastlookup = svcinfo->svcpath[SVC_AUTH][i]) != SVC_LAST; i++)
		if (authout = ((*(getauthents [svcinfo->svcpath[SVC_AUTH][i]])) () )) {
			svc_getauthflag = i;
			break;
		}
	return(authout);
}

AUTHORIZATION *getauthuid(uid)
int uid;
{
	AUTHORIZATION *authout=NULL;
	register i;

	if ((svcinfo = getsvc()) != NULL)
		for (i=0; (svc_lastlookup = svcinfo->svcpath[SVC_AUTH][i]) != SVC_LAST; i++)
			if (authout = ((*(getauthuids [svcinfo->svcpath[SVC_AUTH][i]])) (uid) ))
				break;
	if (authout == NULL)
		return(_auth);
	return(authout);
}

setauthent()
{
	register i;

	svc_getauthflag = -1;
	svc_getauthbind = 0;
	if ((svcinfo = getsvc()) != NULL)
		for (i=0; (svc_lastlookup = svcinfo->svcpath[SVC_AUTH][i]) != SVC_LAST; i++)
			(*(setauthents [svcinfo->svcpath[SVC_AUTH][i]])) ();
}

endauthent()
{
	register i;

	svc_getauthflag = -1;
	svc_getauthbind = 0;
	if ((svcinfo = getsvc()) != NULL)
		for (i=0; (svc_lastlookup = svcinfo->svcpath[SVC_AUTH][i]) != SVC_LAST; i++)
			(*(endauthents [svcinfo->svcpath[SVC_AUTH][i]])) ();
}

storeauthent(authin)
AUTHORIZATION *authin;
{
	register i, status;

	if ((svcinfo = getsvc()) != NULL)
		for (i=0; (svc_lastlookup = svcinfo->svcpath[SVC_AUTH][i]) != SVC_LAST; i++)
			if((status=(*(storeauthents [svcinfo->svcpath[SVC_AUTH][i]])) (authin)) == 0) return status;
	return status;
}
