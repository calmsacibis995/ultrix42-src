#ifndef lint
static char *sccsid = "@(#)chpw_c2.c	4.1      ULTRIX  7/2/90";
#endif lint

/************************************************************************
 *                                                                      *
 *                      Copyright (c) 1989 by                           *
 *              Digital Equipment Corporation, Maynard, MA              *
 *                      All rights reserved.                            *
 *                                                                      *
 *   This software is furnished under a license and may be used and     *
 *   copied  only  in accordance with the terms of such license and     *
 *   with the  inclusion  of  the  above  copyright  notice.   This     *
 *   software  or  any  other copies thereof may not be provided or     *
 *   otherwise made available to any other person.  No title to and     *
 *   ownership of the software is hereby transferred.                   *
 *                                                                      *
 *   The information in this software is subject to change  without     *
 *   notice  and should not be construed as a commitment by Digital     *
 *   Equipment Corporation.                                             *
 *                                                                      *
 *   Digital assumes no responsibility for the use  or  reliability     *
 *   of its software on equipment which is not supplied by Digital.     *
 *                                                                      *
 ************************************************************************/

/************************************************************************
 *                                                                      *
 *	chpw_c2 - Accepts as arguments the following:			*
 *                                                                      *
 *	uid_t	ouid;	uid of user requesting a password change 	*
 *	char	*opwd;	Old passowrd of user(uid)			*
 *	char	*ncrypt;New encrypted password of user(uid)		*
 *                                                                      *
 *	retval=0 => An error occured    				*
 *	       1 => All is well						*
 *                                                                      *
 *	Chpw_c2 checks the received password to assure its validity  	*
 *	with respect to the associated uid. If all is well the new	*
 *	encrypted password string will replace the old encrypted        *
 *	password string in the homebase/passwd file. Hesiod make_passwd *
 *	is then called to update the hesiod.db. Once a new hesiod.db is *
 *	created the local "MASTER" named is restarted which starts the  *
 * 	distribution of the new password database.			*
 *									*
 ************************************************************************/
     

#include <sys/param.h>
#include <sys/types.h>
#include <sys/fs.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <errno.h>
#include <syslog.h>
#include <signal.h>
#include <stdio.h>
#include <pwd.h>
#include <ctype.h>
#include "auth.h"


extern FILE *heslog; 	/* hesupd log file opened by now 		*/
extern char *homebase; 	/* hesupd home directory chdired by now 	*/

char *index();

int
chpw_c2(ouid, opwd, ncrypt)
uid_t	ouid;		/* uid of user requesting a password change 	*/
char	*opwd;		/* Old passowrd of user(uid)			*/
char	*ncrypt;	/* New encrypted password of user(uid)		*/
{	
        FILE *afp=NULL, *oafp;
	long now, time();
        int afd, i, len;
	int retval=0;
        void (*f1)(), (*f2)(), (*f3)();
        char buf[2048], *p, *pwptr;
	CRYPT_PASSWORD newpass;
        char cmdbuf[BUFSIZ];
	struct passwd *pwent, *getpwuid_bind();
	AUTHORIZATION *authent, *getauthuid_hesiod();

	pwent = getpwuid_bind(ouid);
	if(pwent == NULL)  {
		fprintf(heslog,"No password entry for uid %d\n",ouid);
		return(retval);
	}
	authent = getauthuid_hesiod(ouid);
	if(authent == NULL) {
		fprintf(heslog, "No auth entry for uid %d\n", ouid);
		return retval;
	}
	pwptr = authent->a_password;
	if(strcmp(crypt16(opwd, pwptr), pwptr) != 0) {
		fprintf(heslog,"Bad password attempted for uid %d\n",ouid);
		return(retval);
	}
	if(!(authent->a_authmask & A_CHANGE_PASSWORD)) {
		fprintf(heslog, "Password for uid %n not user settable\n", ouid);
		return(retval);
	}
	now = time(0);
	if(now < authent->a_pass_mod+authent->a_pw_minexp) {
		fprintf(heslog, "Minimum password modification time has not expired for uid %d\n", ouid);
		return retval;
	}
/***								 ***/
/*** We now assume that the ouid is valid and set the new passwd ***/
/***								 ***/
	        (void) umask(0);

        f1 = signal(SIGHUP, SIG_IGN);
        f2 = signal(SIGINT, SIG_IGN);
        f3 = signal(SIGQUIT, SIG_IGN);
/*
 * Lock the auth data base.
 */
        afd = open("hesupd_auth", O_WRONLY|O_CREAT|O_EXCL, 0600);
        if (afd < 0) {
		fputs("auth file busy - try again.\n", stderr);
                goto cleanup;
	}
	afp = fdopen(afd, "w");
	if(afp == NULL) {
		fputs("hesupd: fdopen failed?\n", heslog);
		goto cleanup;
	}
	if ((oafp = fopen("auth", "r")) == NULL) {
		fputs("hesupd: Unable to open auth file?\n", stderr);
		goto cleanup;
	}
        signal(SIGTSTP, SIG_IGN);
/*
 * Make a local copy of the new password checking to make sure it's
 * not garbage.
 */
	len = strlen(ncrypt);
	if(len >= sizeof newpass) {
		fputs("hesupd: New password for uid %d too long\n", ouid);
		goto cleanup;
	}
	pwptr = newpass;
	p = ncrypt;
	for(p=ncrypt; (*p != '\0'); p++) {
                if ((*p == ':') || !(isprint(*p)))
                        *pwptr++ = '$';       /* you lose buckwheat */
		else
			*pwptr++ = *p;
	}
	*pwptr = '\0';
	bcopy(newpass,authent->a_password, CRYPT_PASSWORD_LENGTH);
	authent->a_pass_mod = now;
/*
 * Copy and modify the auth file
 */
	while (fgets(buf, sizeof(buf), oafp)) {
		p = index(buf, ':');
		if (p && atoi(buf) == authent->a_uid) {
			fputs(asciiauth(authent), afp);
			putc('\n', afp);
		} else
			fputs(buf, afp);
	}
	fclose(oafp);
	fclose(afp);
	afp = NULL;
/*
 * Copy in new auth file
 */
	(void) umask(077);
	strcpy(cmdbuf,"cp ");
	strcat(cmdbuf,"hesupd_auth");
	strcat(cmdbuf," ");
	strcat(cmdbuf,"auth");
	system(cmdbuf);
	unlink("hesupd_auth");
	(void) umask(022);
/*
 * Create the new hesiod.db 
 */
	strcpy(cmdbuf, "/var/dss/namedb/bin/make_auth ");
	strcat(cmdbuf, homebase);
	strcat(cmdbuf, "/auth ");
	strcat(cmdbuf, "/var/dss/namedb/auth.db");
	system(cmdbuf);
/*
 * Distrbute the new hesiod.db
 */
	strcpy(cmdbuf,"/var/dss/namedb/bin/restart_named");
	system(cmdbuf);
	sleep(10);
	retval=1;
cleanup:
	if(afp)
		fclose(afp);
        signal(SIGHUP, f1);
        signal(SIGINT, f2);
        signal(SIGQUIT, f3);
	return(retval);
}
