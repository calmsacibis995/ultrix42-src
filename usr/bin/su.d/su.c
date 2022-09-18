#ifndef lint
static  char    *sccsid = "@(#)su.c	4.2	(ULTRIX)	10/16/90";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1988, 1989 by			*
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
 * Modification history:
 *
 * 21 Sep 1989	D. Long
 *	Fixed security hole for password check in ENHANCED mode.
 *
 * 15 Aug 1989	D. Long
 *	Changed SEC_TRANS to SEC_UPGRADE.
 *	Also fixed problem handling the password in UPGRADE mode.
 *
 * 7 Jun 1989 - D. Long
 *	Added security features for V4.0.
 */
/*
	"@(#)su.c	4.6 (Berkeley) 7/6/83";
*/

#define	NOPRIV
#include <stdio.h>
#include <pwd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/resource.h>
#ifndef	NOPRIV
#include <sys/priv.h>
#endif
#include <syslog.h>
#include <ttyent.h>
#include <sys/file.h>
#include <sys/svcinfo.h>
#include <auth.h>
#ifdef AUTHEN
#include <krb.h>
#endif AUTHEN

char	userbuf[16]	= "USER=";
char	homebuf[128]	= "HOME=";
char	shellbuf[128]	= "SHELL=";
char	pathbuf[128]	= "PATH=:/usr/ucb:/bin:/usr/bin";
char	*cleanenv[] = { userbuf, homebuf, shellbuf, pathbuf, 0, 0 };
char	*user = "root";
char	*shell = "/bin/sh";
int	fulllogin;
int	fastlogin;

extern char	**environ;
struct	passwd *pwd,*getpwnam();
char	*crypt16(), *crypt();
char	*getpass();
char	*getenv();

main(argc,argv)
	int argc;
	char *argv[];
{
	char *password;
	int i;
	struct timeval tp;
	struct timezone tzp;
	FILE *sulog;
	AUTHORIZATION *auth, *getauthuid();
	CRYPT_PASSWORD crypt_pass;
	extern int sec_level;
        struct svcinfo *svcp;
#ifdef AUTHEN
	char namebuf[ANAME_SZ];
	char *ptr;
	extern char *index();
#endif AUTHEN

	while(1)
		if (argc > 1 && strcmp(argv[1], "-f") == 0) {
			fastlogin++;
			argc--, argv++;
		} else if (argc > 1 && strcmp(argv[1], "-") == 0) {
			fulllogin++;
			argc--, argv++;
		} else
			break;

	if (argc > 1 && argv[1][0] != '-') {
		user = argv[1];
		argc--, argv++;
	}
	config_auth();

#ifdef AUTHEN
        if((svcp = getsvc()) == NULL)
                {
                fprintf(stderr," Cannot access security type\n");
                exit(0);
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
                                (char *)NULL, "/var/dss/kerberos/tkt/tkt.su")
                                        != RET_OK) {
                                fputs("Kerberos initialization failure\n", stderr);
                                }
                        }
                }
#endif AUTHEN

	if (strcmp(user, "root") == 0)
		setpriority(PRIO_PROCESS, 0, -2);
	if ((pwd = getpwnam(user)) == NULL) {
		fprintf(stderr, "Unknown login: %s\n", user);
		exit(1);
	}
/*
 * Make sure terminal is secure before attempting to become a
 * privileged user.
 */
	if(sec_level >= SEC_UPGRADE && pwd->pw_uid == 0 && getuid() != 0) {
		char *ttyname();
		char *tty=ttyname(2);
		struct ttyent *tt;

		if(!tty || strncmp(tty, "/dev/", 5) ||
		  !(tt=getttynam(&tty[5])) || !(tt->ty_status & (TTY_SECURE|TTY_SU))) {
			syslog(LOG_INFO, "SU TO ROOT REFUSED %s", tty);
			fputs("Disallowed on this terminal\n", stderr);
			exit(2);
		}
	}
/*
 * Get the auth information for the UID.
 */
	if(sec_level >= SEC_UPGRADE) {
		if((auth=getauthuid(pwd->pw_uid)) == NULL) {
			fputs("Unable to find auth record for user ", stderr);
			fputs(user, stderr);
			fputs(".\n", stderr);
			exit(1);
		}
	}
/*
 * If original user is not the superuser solicit and verify the password.
 */
	if(getuid() != 0) {
	if((sec_level == SEC_BSD && pwd->pw_passwd[0])
	    || (sec_level == SEC_UPGRADE && pwd->pw_passwd[0] && strcmp(pwd->pw_passwd, "*"))
	    || (sec_level == SEC_UPGRADE && !strcmp(pwd->pw_passwd, "*") && auth->a_password)
	    || (sec_level > SEC_UPGRADE && auth->a_password)) {
		char *cp;

		password = getpass("Password:");
		if(sec_level > SEC_UPGRADE ||
		    (sec_level == SEC_UPGRADE && !strcmp(pwd->pw_passwd, "*"))) {
			cp = crypt16(password, auth->a_password);
			pwd->pw_passwd = auth->a_password;
		} else
			cp = crypt(password, pwd->pw_passwd);
		if (strcmp(pwd->pw_passwd, cp) != 0) {
			fputs("Sorry\n", stderr);
			if (pwd->pw_uid == 0) {
				FILE *console = fopen("/dev/console", "w");
				if (console != NULL) {
					fprintf(console, "BADSU: %s %s\r\n",
						getlogin(), ttyname(2));
					fclose(console); 
				}
				sulog = fopen("/usr/adm/sulog", "a");
				if (sulog != NULL){
					if(gettimeofday(&tp,&tzp) != 0)
						tp.tv_sec = 0;
					fprintf(sulog,"BADSU: %s %s %s",
						getlogin(),ttyname(2),ctime(&tp.tv_sec));
					fclose(sulog);
				}
			}
			exit(2);
		}
		}
/*
 * Verify password still valid.
 */
		if(sec_level >= SEC_UPGRADE)
			if(auth->a_pw_maxexp)
				if((auth->a_pw_maxexp+auth->a_pass_mod)
				    < time((long *)0)) {
					fputs(user, stderr);
					fputs("'s password has expired\n", stderr);
					exit(2);
				}
/*
 * Make sure account is enabled.
 */
		if(sec_level >= SEC_UPGRADE)
			if(!(auth->a_authmask&A_LOGIN)) {
				fputs("This account is disabled\n", stderr);
				exit(2);
			}
	}

	endpwent();
	if(sec_level >= SEC_UPGRADE)
		endauthent();
/*
 * If auth is being granted for superuser access log the fact
 * on the console and in the log file.
 */
	if (pwd->pw_uid == 0) {
/*
 * Log to the console.
 */
		FILE *console = fopen("/dev/console", "w");
		if (console != NULL) {
			fprintf(console, "SU: %s %s\r\n",
				getlogin(), ttyname(2));
			fclose(console);
		}
/*
 * Log to the log file.
 */
		sulog = fopen("/usr/adm/sulog", "a");
		if (sulog != NULL){
			if(gettimeofday(&tp,&tzp) != 0)
				tp.tv_sec = 0;
			fprintf(sulog,"SU: %s %s %s",
				getlogin(),ttyname(2),ctime(&tp.tv_sec));
			fclose(sulog);
		}
	}
/*
 * Set the group access list from the password entry.
 */
	if (setgid(pwd->pw_gid) < 0) {
		perror("su: setgid");
		exit(3);
	}
	if (initgroups(user, pwd->pw_gid)) {
		fputs("su: initgroups failed\n", stderr);
		exit(4);
	}
#ifndef	NOPRIV
	if(sec_level >= SEC_UPGRADE)
		if(setpriv(P_SET_PROC|P_SET_INHL, auth->a_privs) < 0) {
			fputs("su: setpriv failed\n", stderr);
			exit(5);
		}
#endif
/*
 * Change the uid.
 */
	if (setuid(pwd->pw_uid) < 0) {
		perror("su: setuid");
		exit(5);
	}
/*
 * Find the shell specified in the password file entry.
 */
	if (pwd->pw_shell && *pwd->pw_shell)
		shell = pwd->pw_shell;
/*
 * Set up the environment.
 */
	if (fulllogin) {
		cleanenv[4] = getenv("TERM");
		environ = cleanenv;
	}
	if (strcmp(user, "root"))
		setenv("USER", pwd->pw_name, userbuf);
	setenv("SHELL", shell, shellbuf);
	setenv("HOME", pwd->pw_dir, homebuf);
	setpriority(PRIO_PROCESS, 0, 0);
	if (fastlogin) {
		*argv-- = "-f";
		*argv = "su";
	} else if (fulllogin) {
/*
 * If full login change to the home directory.
 */
		if (chdir(pwd->pw_dir) < 0) {
			fputs("No directory\n", stderr);
			exit(6);
		}
		*argv = "-su";
	} else
		*argv = "su";
/*
 * Launch the new shell.
 */
	execv(shell, argv);
	fputs("No shell\n", stderr);
	exit(7);
}

/*
 * The function "setenv" looks up the specified environment variable
 * in the environment and resets it to the specified value.
 */
setenv(ename, eval, buf)
	char *ename, *eval, *buf;
{
	register char *cp, *dp;
	register char **ep = environ;

	/*
	 * this assumes an environment variable "ename" already exists
	 */
	while (dp = *ep++) {
		for (cp = ename; *cp == *dp && *cp; cp++, dp++)
			continue;
		if (*cp == 0 && (*dp == '=' || *dp == 0)) {
			strcat(buf, eval);
			*--ep = buf;
			return;
		}
	}
}

/*
 * The function "getenv" looks up the specified environment variable in
 * the environment and returns its value.
 */
char *getenv(ename)
	char *ename;
{
	register char *cp, *dp;
	register char **ep = environ;

	while (dp = *ep++) {
		for (cp = ename; *cp == *dp && *cp; cp++, dp++)
			continue;
		if (*cp == 0 && (*dp == '=' || *dp == 0))
			return (*--ep);
	}
	return ((char *)0);
}
