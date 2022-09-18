#ifndef lint
static	char	*sccsid = "@(#)checkpass.c	4.1	(ULTRIX)	7/2/90";
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
 * Modificaition history:
 *
 * 11-Aug-89	D. Long
 *	Fixed names of SEC_* defines.  Fixed final passwd comparison.
 *
 * 19-Jul-89 - D. Long
 *	Changed to get SEC_* defines from sys/svcinfo.h.
 */

/*
 * Checkpass - a routine to verify a user's password.
 */
#include <pwd.h>
#include <sys/svcinfo.h>
#include "auth.h"

extern int sec_level;

int checkpass(uid, passwd)
int uid;
char *passwd;
{
	struct passwd *pwd=NULL;
	AUTHORIZATION *auth=NULL, *getauthuid();
	extern char *crypt(), *crypt16();
	extern long time();
	char *pp, *(*fp)();

	config_auth();
	switch(sec_level) {
	case SEC_BSD:
		if((pwd=getpwuid(uid)) == NULL)
			return 0;
		pp = pwd->pw_passwd;
		fp = crypt;
		break;
	case SEC_UPGRADE:
		if((pwd=getpwuid(uid)) == NULL)
			return 0;
		if((auth=getauthuid(uid)) == NULL)
			return 0;
		if(!strcmp(pwd->pw_passwd, "*")) {
			pp = auth->a_password;
			fp = crypt16;
		} else {
			pp = pwd->pw_passwd;
			fp = crypt;
		}
		break;
	case SEC_ENHANCED:
		if((auth=getauthuid(uid)) == NULL)
			return 0;
		pp = auth->a_password;
		fp = crypt16;
		break;
	}
	if(strcmp((*fp)(passwd, pp), pp))
		return 0;
	if(auth) {
		if(auth->a_pw_maxexp == 0)
			return 1;
		if(time(0) > auth->a_pass_mod+auth->a_pw_maxexp)
			return 0;
	}
	return 1;
}
