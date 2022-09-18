#ifndef lint
static	char	*sccsid = "@(#)authenticate.c	4.2	(ULTRIX)	11/15/90";
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
 * Modification history:
 *
 * 14-Nov-90 D. Long
 *	Fixed erroneous use of access(2).  Access is now determined
 *	by dbm_open(3).
 */

/*
 * authenticate_user - a routine to verify a user's password.
 */
#include <unistd.h>
#include <sys/errno.h>
#include <pwd.h>
#include <sys/svcinfo.h>
#include <ttyent.h>
#include "auth.h"

static AUTHORIZATION *auth;
static char authorization_db[] = AUTHORIZATION_DB;
static char empty[] = "";

/*
 * Function to determine if an authenticated user actually has login access
 * on the specified tty device.  If the "tty" argument is NULL then only
 * general login capability will be tested for.  Function returns zero on
 * success and the negative autherr on failure.  Errno will be set with the
 * coarse error status on error.
 */
static int term_access(pwd, tty)
register struct passwd *pwd;
char *tty;
{
/*
 * If auth record for user verify they have login capability in authmask.
 */
	if(auth)
		if(!(auth->a_authmask & A_LOGIN)) {
			errno = EPERM;
			return -A_ENOLOGIN;
		}
/*
 * If checking tty information and user is root make sure they are specifying
 * a secure tty.
 */
	if(tty && pwd->pw_uid == 0) {
		struct ttyent *t;
		char *ttyn;
		extern char *rindex();

/*
 * Get basename of tty device.
 */
		if((ttyn=rindex(tty, '/')))
			ttyn++;
		else
			ttyn = tty;
/*
 * Get ttys entry for tty device.
 */
		if(!(t=getttynam(ttyn))) {
			errno = EPERM;
			return -A_EOPENLINE;
		}
/*
 * Make sure it supports privileged login.
 */
		if(!(t->ty_status&TTY_SECURE)) {
			errno = EPERM;
			return -A_EOPENLINE;
		}
	}
	return 0;
}

/*
 * Library function to authenticate a user.  The supplied password is
 * verified against the applicable user password base on the current
 * I&A security level.  Password expiration information is also examined
 * if it is available.  If the tty argument is supplied the user is also
 * tested for login access to the system in general and on the specified
 * line in particular.  On success the function returns the number of failed
 * authentication attempts since the last successful one (or zero if this
 * feature is not enabled).  On failure it returns minus autherr and sets
 * errno:
 *
 *	Reason				errno		return
 *
 *	Passwd is wrong			EPERM		A_EBADPASS
 *	SEC_LEVEL > BSD and no auth
 *	     record was found		EINVAL		-
 *	Password expired		EPERM		A_ESOFTEXP/A_EHARDEXP
 *	Unable to initialize auth info	ENOSYS		-
 *	Unable to read auth data base	EACCES		-
 *
 * The following errors could also occur when a non-NULL tty argument is
 * supplied:
 *
 *	Reason				errno		return
 *
 *	Account is disabled in ADB	EPERM		A_ENOLOGIN
 *	Privileged account and tty was
 *	     not found in /etc/ttys	EPERM		A_EOPENLINE
 *	Privileged account and tty is not
 *	     marked secure in /etc/ttys	EPERM		A_EOPENLINE
 */
int authenticate_user(pwd, passwd, tty)
register struct passwd *pwd;
char *passwd, *tty;
{
	extern AUTHORIZATION *getauthuid();
	extern char *crypt(), *crypt16();
	extern long time();
	struct svcinfo *svcinfo;
	int sec_level, i;
	char *pp, *(*fp)();

	auth = (AUTHORIZATION *) 0;
	if(!pwd) {
		errno = EINVAL;
		return -EINVAL;
	}
/*
 * Get the service and security configuration.
 */
	if(!(svcinfo=getsvc())) {
		errno = ENOSYS;
		return -ENOSYS;
	}
	sec_level = svcinfo->svcauth.seclevel;
/*
 * Get the password based on the I&A security mode.
 */
	switch(sec_level) {
	case SEC_BSD:
/*
 * Password is always in /etc/passwd (compatability).
 */
		pp = pwd->pw_passwd;
		fp = crypt;
		break;
	case SEC_UPGRADE:
		if(!(auth=getauthuid(pwd->pw_uid))) {
			if(errno == 0)
				errno = EINVAL;
			else if(errno == ENOENT)
				errno = ENOSYS;
			return -errno;
		}
/*
 * If the passwd file has an "*" in the password field then the password has
 * already been moved into the auth data base.
 */
		if(!strcmp(pwd->pw_passwd, "*")) {
			pp = auth->a_password;
			fp = crypt16;
		} else {
/*
 * Otherwise the password is still in /etc/passwd.
 */
			pp = pwd->pw_passwd;
			fp = crypt;
		}
		break;
	case SEC_ENHANCED:
/*
 * The password is always in the auth data base.
 */
		if(!(auth=getauthuid(pwd->pw_uid))) {
			if(errno == 0)
				errno = EINVAL;
			else if(errno == ENOENT)
				errno = ENOSYS;
			return -errno;
		}
		pp = auth->a_password;
		fp = crypt16;
		break;
	default:
/*
 * Unsupported security level.
 */
		errno = ENOSYS;
		return -ENOSYS;
	}
/*
 * Assume NULL pointer means NULL string.
 */
	if(!pp)
		pp = empty;
	if(!passwd)
		passwd = empty;
/*
 * If non-empty password supplied when there is no password for the account
 * detect it here.  This is a special case because crypt(3) always returns
 * an empty string when the salt is an empty string leading to erroneous
 * authentications.
 */
	if(!*pp && *passwd) {
/*
 * Increment failed authentication counter (if not served remotely).
 */
		if(auth && svc_lastlookup == SVC_LOCAL) {
			auth->a_fail_count++;
			storeauthent(auth);
			endauthent();
		}
		errno = EPERM;
		return -A_EBADPASS;
	}
/*
 * Compare passwords.
 */
	if(strcmp((*fp)(passwd, pp), pp)) {
/*
 * Increment failed authentication counter (if not served remotely).
 */
		if(auth && svc_lastlookup == SVC_LOCAL) {
			auth->a_fail_count++;
			storeauthent(auth);
			endauthent();
		}
		errno = EPERM;
		return -A_EBADPASS;
	}
/*
 * If tty argument supplied check login access on the specified line.
 */
	if(tty) {
		int i;

		if(!*tty)
			tty = NULL;
		if((i=term_access(pwd, tty)) < 0)
			return i;
	}
	i = 0;
/*
 * If auth information present check the password expiration times.
 */
	if(auth) {
		extern long time();
		long now, expiration;

		if(auth->a_pw_maxexp) {
			now = time((long *) 0);
			expiration = auth->a_pass_mod + auth->a_pw_maxexp;
			if(now > expiration) {
				endauthent();
/*
 * Set hard or soft expiration error code as approriate.
 */
				errno = EPERM;
				if(now > expiration + svcinfo->svcauth.softexp)
					return -A_EHARDEXP;
				else
					return -A_ESOFTEXP;
			}
		}
/*
 * Not expired.
 */
		i = auth->a_fail_count;
/*
 * Clear the authentication fail count.
 */
		if(i && svc_lastlookup == SVC_LOCAL) {
			auth->a_fail_count = 0;
			storeauthent(auth);
		}
		endauthent();
	}
/*
 * Return the number of failed attempts since last successful authentication
 * (or zero if feature not enabled).
 */
	return i;
}
