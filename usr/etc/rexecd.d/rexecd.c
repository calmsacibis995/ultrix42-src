#ifndef lint
static char sccsid[] = "@(#)rexecd.c	4.3	(ULTRIX)	12/6/90";
#endif

/************************************************************************
 *									*
 *			Copyright (c) 1985, 1990 by			*
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

/*-----------------------------------------------------------------------
 *	Modification History
 *
 *	12/5/90	dlong
 *		Numerous changes to work with enhanced security and to
 *		remove potetnial varargs problem in error() function.
 *
 *	7/09/90 jsd
 *		Add extern to environ declaration (for mips)
 *
 *	6/25/90 jsd
 *		Interchange setuid() with setgid() so latter succeeds.
 *
 *	4/5/85 -- jrs
 *		Revise to allow inetd to perform front end functions,
 *		following the Berkeley model.
 *
 *	Based on 4.2BSD labeled:
 *		rexecd.c	4.10	83/07/02
 *
 *-----------------------------------------------------------------------
 */

#include <limits.h>
#include <sys/ioctl.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <sys/wait.h>

#include <netinet/in.h>

#include <varargs.h>
#include <stdio.h>
#include <errno.h>
#include <pwd.h>
#include <signal.h>
#include <netdb.h>
#include <syslog.h>
#include <sys/syscall.h>
#include <sys/audit.h>
#include <sys/svcinfo.h>
#include <auth.h>
#ifdef	AUTHEN
#include <krb.h>
#endif	AUTHEN

#define	LEN	(SYSCALL_MASK_LEN+TRUSTED_MASK_LEN)

extern	errno;
struct	passwd *getpwnam();
char	*rindex(), *index();
extern AUTHORIZATION *_auth;
/* VARARGS 1 */
int	error();
/*
 * remote execute server:
 *	username\0
 *	password\0
 *	command\0
 *	data
 */

struct svcinfo *svcp;
static struct sockaddr_in *remhost = 0;
static struct passwd *pwd=0;
static AUTHORIZATION *auth=0;
static char user[17];

/*
 * Function to generate an audit record with all available, relevant
 * information.  Called by error() for all error cases.  Called by doit() for
 * successful case.
 */
static int auditit(message, result)
char *message;
int result;
{
	char tmask[AUD_NPARAM+1];
	char *ar[AUD_NPARAM+1];
	int i=0;

	tmask[i] = T_LOGIN;
	if(pwd)
		ar[i++] = pwd->pw_name;
	else
		ar[i++] = user;
	tmask[i] = T_SERVICE;
	ar[i++] = "rexecd";
	if(pwd) {
		tmask[i] = T_HOMEDIR;
		ar[i++] = pwd->pw_dir;
		tmask[i] = T_SHELL;
		ar[i++] = pwd->pw_shell;
	}
	if(message) {
		tmask[i] = T_CHARP;
		ar[i++] = message;
	}
	if(remhost) {
		tmask[i] = T_HOSTADDR2;
		ar[i++] = (char *) remhost->sin_addr;
	}
	if(result)
		tmask[i] = T_ERROR;
	else
		tmask[i] = T_RESULT;
	ar[i++] = (char *) result;
	tmask[i] = '\0';
	return audgen(LOGIN, tmask, ar);
}

main(argc, argv)
	int argc;
	char **argv;
{
	int fromlen, i;
	struct sockaddr_in from;

/*
  Turn off all auditing except for LOGIN and setgroups.
*/
	{ char buf[LEN];
	if ( audcntl ( SET_PROC_ACNTL, (char *)0, 0, AUDIT_AND, 0 ) == -1 ) perror ( "audcntl" );
	A_PROCMASK_SET ( buf, SYS_setgroups, 1, 1 );
	A_PROCMASK_SET ( buf, LOGIN, 1, 1 );
	if ( audcntl ( SET_PROC_AMASK, buf, LEN, 0, 0 ) == -1 ) perror ( "audcntl" );
	}
	fromlen = sizeof(from);
	if (getpeername(0, &from, &fromlen) < 0) {
		openlog(argv[0], LOG_PID);
		syslog(LOG_ERR, "getpeername: %m");
		closelog();
		exit(1);
	}
/*
 * Fire up kerberos
 */
#ifdef AUTHEN
	if((svcp = getsvc()) == NULL)
		fputs("Cannot access security type\n", stderr);
	if(svcp->svcauth.seclevel >= SEC_UPGRADE) 
		for (i = 0 ; svcp->svcpath[SVC_AUTH][i] != SVC_LAST; i++)
			if (svcp->svcpath[SVC_AUTH][i] == SVC_BIND) {
				char *ptr, namebuf[ANAME_SZ];

				if(gethostname(namebuf, sizeof(namebuf)) == -1)
					fputs("gethostname failure\n", stderr);

				if((ptr = index(namebuf, '.')) != (char *)0)
					*ptr = '\0';

				if(krb_svc_init("hesiod", namebuf, (char *)NULL, 0,
				    (char *)NULL, "/var/dss/kerberos/tkt/tkt.rexecd")
				    != RET_OK)
					fputs("Kerberos initialization failure\n", stderr);
			}
#endif AUTHEN
	(void) dup2(0, 3);
	(void) close(0);
	doit(3, &from);
}

char	username[20] = "USER=";
char	homedir[64] = "HOME=";
char	shell[64] = "SHELL=";
char	*envinit[] =
	    {homedir, shell, "PATH=:/usr/ucb:/bin:/usr/bin", username, 0};
extern char	**environ;

struct	sockaddr_in asin = { AF_INET };
static char bad_login_msg[] = "Login incorrect.";

doit(f, fromp)
	int f;
	struct sockaddr_in *fromp;
{
	char cmdbuf[NCARGS+1], *cp, *namep;
	char pass[PASS_MAX+1];
	int s, status;
	u_short port;
	int pv[2], pid, ready, readfrom, cc;
	char buf[BUFSIZ], sig;
	int one = 1;

	(void) signal(SIGINT, SIG_DFL);
	(void) signal(SIGQUIT, SIG_DFL);
	(void) signal(SIGTERM, SIG_DFL);
#ifdef DEBUG
	{ int t = open("/dev/tty", 2);
	  if (t >= 0) {
		ioctl(t, TIOCNOTTY, (char *)0);
		(void) close(t);
	  }
	}
#endif
	(void) dup2(f, 0);
	(void) dup2(f, 1);
	(void) dup2(f, 2);
	(void) alarm(60);
	port = 0;
	for (;;) {
		char c;
		if (read(f, &c, 1) != 1)
			exit(1);
		if (c == 0)
			break;
		port = port * 10 + c - '0';
	}
	(void) alarm(0);
	if (port != 0) {
		s = socket(AF_INET, SOCK_STREAM, 0);
		if (s < 0)
			exit(1);
		if (bind(s, &asin, sizeof (asin)) < 0)
			exit(1);
		(void) alarm(60);
		fromp->sin_port = htons((u_short)port);
		if (connect(s, fromp, sizeof (*fromp)) < 0)
			exit(1);
		(void) alarm(0);
	}
	remhost = fromp;
	getstr(user, sizeof(user), "username");
	getstr(pass, sizeof(pass), "password");
	getstr(cmdbuf, sizeof(cmdbuf), "command");
	(void) setpwent();
	pwd = getpwnam(user);
	if (pwd == NULL) {
		sleep(1);
		error("Invalid account.",  bad_login_msg);
		exit(1);
	}
	(void) endpwent();
	if((status=authenticate_user(pwd, pass, "")) < 0)
		switch(-status) {
		case A_ESOFTEXP:
		case A_EHARDEXP:
			error((char *)0, "Password expired.");
			exit(1);
			break;
		case A_ENOLOGIN:
			error((char *)0, "Account disabled.");
			exit(1);
			break;
		default:
			sleep(1);
			error("Invalid password.", bad_login_msg);
			exit(1);
		}
	if(svcp->svcauth.seclevel > SEC_BSD && _auth && !(auth=getauthuid(pwd->pw_uid))) {
		error("Unable to get auth record.", bad_login_msg);
		exit(1);
	}
	if (chdir(pwd->pw_dir) < 0) {
		error((char *)0, "No remote directory.");
		exit(1);
	}
	(void) write(2, "\0", 1);
	if (port) {
		(void) pipe(pv);
		pid = fork();
		if (pid == -1)  {
			error((char *)0, "Try again.");
			exit(1);
		}
		if (pid) {
			(void) close(0); (void) close(1); (void) close(2);
			(void) close(f); (void) close(pv[1]);
			readfrom = (1<<s) | (1<<pv[0]);
			ioctl(pv[1], FIONBIO, (char *)&one);
			/* should set s nbio! */
			do {
				ready = readfrom;
				(void) select(16, &ready, 0, 0, 0);
				if (ready & (1<<s)) {
					if (read(s, &sig, 1) <= 0)
						readfrom &= ~(1<<s);
					else
						(void) killpg(pid, sig);
				}
				if (ready & (1<<pv[0])) {
					cc = read(pv[0], buf, sizeof (buf));
					if (cc <= 0) {
						(void) shutdown(s, 1+1);
						readfrom &= ~(1<<pv[0]);
					} else
						(void) write(s, buf, cc);
				}
			} while (readfrom);
			exit(0);
		}
		(void) setpgrp(0, getpid());
		(void) close(s); (void)close(pv[0]);
		(void) dup2(pv[1], 2);
	}
	if (*pwd->pw_shell == '\0')
		pwd->pw_shell = "/bin/sh";
	(void) close(f);
	initgroups(pwd->pw_name, pwd->pw_gid);

	/* do setgid before setuid, else we lose our root-uid process */
	if (setgid(pwd->pw_gid) < 0) {
		openlog("rexecd", LOG_PID);
		syslog(LOG_ERR, "setgid: %m");
		closelog();
		exit(1);
	}
	if(auth) {
		if(audcntl(SET_PROC_ACNTL, (char *) 0, 0, auth->a_audit_control, 0) < 0) {
			openlog("rexecd", LOG_PID);
			syslog(LOG_ERR, "audcntl error.");
			exit(1);
		}
		if(audcntl(SET_PROC_AMASK, auth->a_audit_mask, SYSCALL_MASK_LEN+TRUSTED_MASK_LEN, 0) < 0) {
			openlog("rexecd", LOG_PID);
			syslog(LOG_ERR, "audit mask error.");
			exit(1);
		}
		if(audcntl(SETPAID, (char *) 0, 0, 0, auth->a_audit_id) < 0) {
			openlog("rexecd", LOG_PID);
			syslog(LOG_ERR, "error setting audit ID.");
			exit(1);
		}
	} else {
		audcntl(SET_PROC_ACNTL, (char *)0, 0, AUDIT_OR, 0);
		A_PROCMASK_SET(buf, SYS_setgroups, 0, 0);
		A_PROCMASK_SET(buf, LOGIN, 0, 0);
		audcntl(SET_PROC_AMASK, buf, LEN, 0, 0);
	}
	auditit("rexec succeeded", 0);
	if (setuid(pwd->pw_uid) < 0) {
		openlog("rexecd", LOG_PID);
		syslog(LOG_ERR, "setuid: %m");
		closelog();
		exit(1);
	}
	environ = envinit;
	(void) strncat(homedir, pwd->pw_dir, sizeof(homedir)-6);
	(void) strncat(shell, pwd->pw_shell, sizeof(shell)-7);
	(void) strncat(username, pwd->pw_name, sizeof(username)-6);
	cp = rindex(pwd->pw_shell, '/');
	if (cp)
		cp++;
	else
		cp = pwd->pw_shell;
	execl(pwd->pw_shell, cp, "-c", cmdbuf, 0);
	perror(pwd->pw_shell);
	exit(1);
}

/* VARARGS 1 */
/*
 * Function to report an error.  This function will also cause an audit
 * record to be generated.  The first argument is a message to place in
 * the audit record.  If it is the NULL pointer the message placed in the
 * audit record will be the same as that printed out.  The second
 * argument is the format for a sprintf.  It may not be NULL.  Any
 * remaining arguments are used as arguments in the call to sprintf.
 */
error(va_alist)
va_dcl
{
	va_list ap;
	char buf[BUFSIZ];
	char *fmt, *audit;

	va_start(ap);
	audit = va_arg(ap, char *);
	fmt = va_arg(ap, char *);
	buf[0] = 1;
	(void) vsprintf(buf+1, fmt, ap);
	if(!audit)
		audit = buf+1;
	auditit(audit, 1);
	strcat(buf, "\n");
	(void) write(2, buf, strlen(buf));
	va_end(ap);
}

getstr(buf, cnt, err)
	char *buf;
	int cnt;
	char *err;
{
	char c;

	do {
		if (read(0, &c, 1) != 1)
			exit(1);
		*buf++ = c;
		if (cnt-- <= 0) {
			error((char *)0, "%s too long", err);
			exit(1);
		}
	} while (c != 0);
}
