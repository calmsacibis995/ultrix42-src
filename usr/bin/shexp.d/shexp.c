#ifndef lint
static	char	*sccsid = "@(#)shexp.c	4.2	(ULTRIX)	1/31/91";
#endif lint
/************************************************************************
 *									*
 *			Copyright (c) 1989, 1991 by			*
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
 * Modification history
 *
 * 3/25/90 dal	Added Kerberos stuff.
 */

#include <sys/param.h>
#include <sys/file.h>
#include <sys/stat.h>
#ifdef	AUTHEN
#include <sys/svcinfo.h>
#endif	AUTHEN
#include <stdio.h>
#include <pwd.h>
#include <auth.h>
#ifdef	AUTHEN
#include <krb.h>
struct svcinfo *svcp;
#endif	AUTHEN

usage()
{
	fputs("usage: shexp [-q] [username]\n", stderr);
}

main(argc, argv)
int argc;
char *argv[];
{
	AUTHORIZATION *auth, *getauthuid();
	UID user_id, sso_id, our_id;
	char user_name[17];
	long min_exp, max_exp, pass_mod, deadline, now, time();
	char *cp;
	struct passwd *pwd;
	struct stat stat_buf;
	int qopt=0, c, i;
	extern char *optarg;
	extern int optind;
#ifdef AUTHEN
	char namebuf[ANAME_SZ];
	char *ptr, *index();
#endif AUTHEN

/*
  Parse the options.
*/
	while((c=getopt(argc, argv, "q")) != EOF) {
		switch(c) {
		case 'q':
			qopt = 1;
			break;
		case '?':
			usage();
			exit(2);
		}
	}
	argc -= optind;
	if(argc > 1) {
		usage();
		exit(2);
	}
/*
  Fire up Kerberos
*/
#ifdef AUTHEN
        if((svcp = getsvc()) == NULL)
                {
                fputs("Cannot access security type\n", stderr);
		exit(3);
                }
        if(svcp->svcauth.seclevel >= SEC_UPGRADE)
                {
                for (i = 0 ; svcp->svcpath[SVC_AUTH][i] != SVC_LAST; i++)
                        if (svcp->svcpath[SVC_AUTH][i] == SVC_BIND) {
                                if(gethostname(namebuf, sizeof(namebuf)) == -1)
                                        {
                                        fputs("gethostname failure\n", stderr);
                                        }

			if((ptr = index(namebuf, '.')) != (char *)0)
				*ptr = '\0';

                        if(krb_svc_init("hesiod", namebuf, (char *)NULL, 0,
                                (char *)NULL, "/var/dss/kerberos/tkt/tkt.shexp")
                                        != RET_OK) {
                                fputs("Kerberos initialization failure\n", stderr);
                                }
                        }
                }
#endif AUTHEN
/*
  If a username argument was supplied use that, otherwise use the login name.
*/
	our_id = getuid();
	if(argc >= 1)
		pwd = getpwnam(argv[optind]);
	else
		pwd = getpwuid(our_id);
	if(!pwd) {
		fputs("User not found in passwd data base.\n", stderr);
		exit(2);
	}
	if(!pwd->pw_name || !*pwd->pw_name) {
		fputs("Error, no username.\n", stderr);
		exit(2);
	}
	user_id = pwd->pw_uid;
	strncpy(user_name, pwd->pw_name, sizeof user_name);
/*
  Check the auth file.
*/
	if(stat("/etc/auth.dir", &stat_buf) != 0) {
		fputs("Cannot stat auth file.\n", stderr);
		exit(3);
	}
/*
  By definition the SSO is the owner of the auth files.
*/
	sso_id = stat_buf.st_uid;
/*
  In order to display auth information for another user (UID) we must
  be privileged.
*/
	if(our_id != user_id && our_id != sso_id && our_id != 0) {
		fputs("Insufficient privilege.\n", stderr);
		exit(2);
	}
/*
  Get the auth information.
*/
	if((auth=getauthuid(user_id)) == NULL) {
		printf("Unable to retrieve auth information for %s.\n",
			user_name);
		exit(2);
	}
/*
  Get the times.
*/
	now = time((long *) 0);
	max_exp = auth->a_pw_maxexp;
	min_exp = auth->a_pw_minexp;
	pass_mod = auth->a_pass_mod;
/*
  With no options print out a readable version of the expiration times.
  If there is a q-option print out a terse version of the times.
*/
	if(!qopt) {
		if(max_exp == 0)
			puts("Never expires");
		else {
			deadline = pass_mod + max_exp;
			if(deadline < now)
				fputs("Expired ", stdout);
			else
				fputs("Expires ", stdout);
			fputs(ctime(&deadline), stdout);
		}
	} else {
		printf("%1ld %1ld %1ld\n", min_exp, max_exp, pass_mod);
	}
	endauthent();
	exit(0);
}
