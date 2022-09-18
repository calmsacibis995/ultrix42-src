#ifndef lint
static char *sccsid = "@(#)chpw_bsd.c	4.2      ULTRIX  7/17/90";
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
 *	chpw_bsd - Accepts as arguments the following:			*
 *                                                                      *
 *	uid_t	ouid;	uid of user requesting a password change 	*
 *	char	*opwd;	Old passowrd of user(uid)			*
 *	char	*ncrypt;New encrypted password of user(uid)		*
 *                                                                      *
 *	retval=0 => An error occured    				*
 *	       1 => All is well						*
 *                                                                      *
 *	Chpw_bsd checks the received password to assure its validity    *
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


extern FILE *heslog; 	/* hesupd log file opened by now 		*/
extern char *homebase; 	/* hesupd home directory chdired by now 	*/
char *index();

int
chpw_bsd(ouid,opwd,ncrypt)
uid_t	ouid;		/* uid of user requesting a password change 	*/
char	*opwd;		/* Old passowrd of user(uid)			*/
char	*ncrypt;	/* New encrypted password of user(uid)		*/
{	
        FILE *tempfp, *filefp, *fp;
        int tempfd, i, len;
	int retval=0;
        void (*f1)(), (*f2)(), (*f3)();
        char buf[256], *p;
	char *pwptr;
        char cmdbuf[1024];
	char pwdbuf[64];
	struct passwd *pwent, *getpwuid_bind();
	pwent = getpwuid_bind(ouid);
	fflush(heslog);
	if(pwent == NULL) 
		{
		fprintf(heslog,"No password entry for uid %d\n",ouid);
		return(retval);
		}
	else if(strcmp(crypt(opwd,pwent->pw_passwd),pwent->pw_passwd) != 0 )
		{
		fprintf(heslog,"Bad password attempted for uid %d\n",ouid);
		return(retval);
		}
	/***								 ***/
	/*** We now assume that the ouid is valid and set the new passwd ***/
	/***								 ***/
	        (void) umask(0);

        f1 = signal(SIGHUP, SIG_IGN);
        f2 = signal(SIGINT, SIG_IGN);
        f3 = signal(SIGQUIT, SIG_IGN);
        tempfd = open("hesupd_passwd", O_WRONLY|O_CREAT|O_EXCL, 0644);
        if (tempfd < 0) {
                        fprintf(heslog, "password file busy - try again.\n");
                	goto cleanup_noclose;
			}
        signal(SIGTSTP, SIG_IGN);
        if ((tempfp = fdopen(tempfd, "w")) == NULL) {
                fprintf(heslog, "hesupd: fdopen failed?\n");
                goto cleanup;
        }
	
	/*
	 * Prepare to make new passwd file copy
	 * with new password.
	 */
	if ((filefp = fopen("passwd", "r")) == NULL) {
		fprintf(heslog, "hesupd: fopen of passwd failed?\n");
		goto cleanup;
	}
	/*				*/
	/* copy and check new password  */
 	/* into pwent struct		*/
	/*				*/
	bzero(pwdbuf, sizeof(pwdbuf));
        for (p = ncrypt, pwptr = pwdbuf; (*p != '\0'); p++,pwptr++)
		{
                if ((*p == ':') || !(isprint(*p)))
                        *pwptr = '$';       /* you lose buckwheat */
		else *pwptr = *p;	    /* the SUN way of doing it */
		}
	/*				*/
	/* copy and modify passwd file  */
	/*				*/
	len = strlen(pwent->pw_name);
	while (fgets(buf, sizeof(buf), filefp)) {
		p = index(buf, ':');
		if (p && p - buf == len
		    && strncmp(pwent->pw_name, buf, p - buf) == 0) {
			fprintf(tempfp,"%s:%s:%d:%d:%s:%s:%s\n",
			    pwent->pw_name,
			    pwdbuf,
			    pwent->pw_uid,
			    pwent->pw_gid,
			    pwent->pw_gecos,
			    pwent->pw_dir,
			    pwent->pw_shell);
		}
		else
			fputs(buf, tempfp);
	}
	bzero(pwdbuf, sizeof(pwdbuf));
	fclose(filefp);
	fclose(tempfp);
	/*				*/
	/* copy in new passwd file	*/
	/*				*/
	(void) umask(022);
	strcpy(cmdbuf,"cp ");
	strcat(cmdbuf,"hesupd_passwd");
	strcat(cmdbuf," ");
	strcat(cmdbuf,"passwd");
	system(cmdbuf);
	unlink("hesupd_passwd");
	/*				*/
	/* create the new hesiod.db 	*/
	/*				*/
	bzero(cmdbuf,sizeof(cmdbuf));
	strcpy(cmdbuf,"/var/dss/namedb/bin/make_passwd ");
	strcat(cmdbuf,"/var/dss/namedb/src");
	strcat(cmdbuf,"/passwd ");
	strcat(cmdbuf,"/var/dss/namedb/passwd.db");
	system(cmdbuf);
	/*				*/
	/* distribute the new hesiod.db */
	/*				*/
	bzero(cmdbuf,sizeof(cmdbuf));
	strcpy(cmdbuf,"/var/dss/namedb/bin/restart_named");
	system(cmdbuf);
	sleep(10);
	retval=1;
    cleanup:
	fclose(tempfp);
    cleanup_noclose:
        signal(SIGHUP, f1);
        signal(SIGINT, f2);
        signal(SIGQUIT, f3);
        fflush(heslog);
	return(retval);
}
