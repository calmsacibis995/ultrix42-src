#ifndef lint
static	char	*sccsid = "@(#)rshd.c	4.4	(ULTRIX)	1/25/91";
#endif lint
/************************************************************************
 *									*
 *			Copyright (c) 1988-1990 by			*
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
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */
/*
 * Modification History:
 *
 * 17-Jan-91       Uttam Shikarpur Nadig
 *			Signal 3 would cause the process to die because of
 *			select being interrupted. Made select re-read in
 *			case it received a EINTR signal.
 *
 * 04-Dec-90 dlong
 *	Check for empty password using the correct database in secure
 *	mode.  Also, fixed krb ticket name.
 *
 * 12-Jun-90 dlong
 *	Added kerberos initialization and auth record verification. Also
 *	made self-auditing and removed child reaping code.
 *
 * 30-Mar-90 sue
 *	Added extern on environ.
 *
 * 07-Feb-90 sue
 *      Added Berkeley fix to check IP address after gethostbyaddr
 *      with a gethostbyname and comparison.
 *
 * 09-Jun-88	Mark Parenti
 *	Changed signal handlers to void.
 */
/*
#ifndef lint
char copyright[] =
"Copyright (c) 1983 Regents of the University of California.\n\
 All rights reserved.\n";
#endif not lint

#ifndef lint
static char sccsid[] = "rshd.c	5.10 (Berkeley) 9/4/87";
#endif not lint
*/
/*
 * remote shell server:
 *	remuser\0
 *	locuser\0
 *	command\0
 *	data
 */
#include <sys/ioctl.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <sys/file.h>
#include <sys/time.h>
#include <sys/svcinfo.h>

#include <netinet/in.h>

#include <arpa/inet.h>
#include <stdio.h>
#include <errno.h>
#include <pwd.h>
#include <signal.h>
#include <netdb.h>
#include <syslog.h>
#include <sys/wait.h>
#include <sys/syscall.h>
#include <sys/audit.h>
#include <auth.h>
#ifdef	AUTHEN
#include <krb.h>
#endif	/* AUTHEN */

#define LEN (SYSCALL_MASK_LEN+TRUSTED_MASK_LEN)

struct svcinfo *svcp;
int	errno;
char	*index(), *rindex(), *strncat();
/*VARARGS1*/
int	error();

/*ARGSUSED*/
main(argc, argv)
	int argc;
	char **argv;
{
	struct linger linger;
	int on = 1, fromlen;
	struct sockaddr_in from;
	int i;

/*
  Turn off all auditing except for LOGIN and setgroups.
*/
	{ char buf[LEN];
	if ( audcntl ( SET_PROC_ACNTL, (char *)0, 0, AUDIT_AND, 0 ) == -1 ) perror ( "audcntl" );
	A_PROCMASK_SET ( buf, SYS_setgroups, 1, 1 );
	A_PROCMASK_SET ( buf, LOGIN, 1, 1 );
	if ( audcntl ( SET_PROC_AMASK, buf, LEN, 0, 0 ) == -1 ) perror ( "audcntl" );
	}

#ifdef 43BSD
	openlog("rsh", LOG_PID | LOG_ODELAY, LOG_DAEMON);
#else
	openlog("rsh", LOG_PID);
#endif 43BS
/*
 * Fire up kerberos
 */
	if((svcp = getsvc()) == NULL)
		fputs("Cannot access security type\n", stderr);
#ifdef	AUTHEN
	if(svcp->svcauth.seclevel >= SEC_UPGRADE) 
		for (i = 0 ; svcp->svcpath[SVC_AUTH][i] != SVC_LAST; i++)
			if (svcp->svcpath[SVC_AUTH][i] == SVC_BIND) {
				char *ptr, namebuf[ANAME_SZ];

				if(gethostname(namebuf, sizeof(namebuf)) == -1)
					fputs("gethostname failure\n", stderr);

				if((ptr = index(namebuf, '.')) != (char *)0)
					*ptr = '\0';

				if(krb_svc_init("hesiod", namebuf, (char *)NULL, 0,
				    (char *)NULL, "/var/dss/kerberos/tkt/tkt.rshd")
				    != RET_OK)
					fputs("Kerberos initialization failure\n", stderr);
			}
#endif	/* AUTHEN */

	fromlen = sizeof (from);
	if (getpeername(0, &from, &fromlen) < 0) {
		fprintf(stderr, "%s: ", argv[0]);
		perror("getpeername");
		_exit(1);
	}
	if (setsockopt(0, SOL_SOCKET, SO_KEEPALIVE, (char *)&on,
	    sizeof (on)) < 0)
		syslog(LOG_WARNING, "setsockopt (SO_KEEPALIVE): %m");
	linger.l_onoff = 1;
	linger.l_linger = 60;			/* XXX */
	if (setsockopt(0, SOL_SOCKET, SO_LINGER, (char *)&linger,
	    sizeof (linger)) < 0)
		syslog(LOG_WARNING, "setsockopt (SO_LINGER): %m");
	doit(dup(0), &from);
}

char	username[20] = "USER=";
char	homedir[64] = "HOME=";
char	shell[64] = "SHELL=";
char	*envinit[] =
	    {homedir, shell, "PATH=:/usr/ucb:/bin:/usr/bin", username, 0};
extern char	**environ;

#ifdef	REAP
int reapdebug=0;
void
reap()
{
	int cpid;
	union wait status1;
	if(reapdebug)
	syslog(LOG_ERR,"eat children pid %x\n",pid);
	cpid = wait3(&status1, WNOHANG, 0);
	if(reapdebug)
	syslog(LOG_ERR,"eat children pid %x\n",cpid);
}
#endif	/* REAP */

static char locuser[16] = "";
static char remuser[16] = "";
static struct sockaddr_in *remhost = 0;
static struct passwd *pwd = NULL;

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
		ar[i++] = locuser;
	tmask[i] = T_SERVICE;
	ar[i++] = "rshd";
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
	if(*remuser) {
		tmask[i] = T_LOGIN2;
		ar[i++] = remuser;
	}
	if(result)
		tmask[i] = T_ERROR;
	else
		tmask[i] = T_RESULT;
	ar[i++] = (char *) result;
	tmask[i] = '\0';
	return audgen(LOGIN, tmask, ar);
}

doit(f, fromp)
	int f;
	struct sockaddr_in *fromp;
{
	char cmdbuf[NCARGS+1], *cp;
	int s;
	struct hostent *hp;
	char *hostname;
	short port;
	int pv[2], pid, ready, readfrom, cc;
	char buf[BUFSIZ], sig;
	int one = 1;
	char remotehost[2 * MAXHOSTNAMELEN + 1];
	int i;
	AUTHORIZATION *auth=NULL;

	(void) signal(SIGINT, SIG_DFL);
	(void) signal(SIGQUIT, SIG_DFL);
	(void) signal(SIGTERM, SIG_DFL);
#ifdef	REAP
	(void) signal(SIGCHLD, reap);
#endif	/* REAP */
#ifdef DEBUG
	{ int t = open("/dev/tty", 2);
	  if (t >= 0) {
		ioctl(t, TIOCNOTTY, (char *)0);
		(void) close(t);
	  }
	}
#endif
	fromp->sin_port = ntohs((u_short)fromp->sin_port);
	if (fromp->sin_family != AF_INET) {
		syslog(LOG_ERR, "malformed from address\n");
		exit(1);
	}
	remhost = fromp;
	if (fromp->sin_port >= IPPORT_RESERVED ||
	    fromp->sin_port < IPPORT_RESERVED/2) {
		syslog(LOG_NOTICE, "connection from bad port\n");
		exit(1);
	}
	(void) alarm(60);
	port = 0;
	for (;;) {
		char c;
		if ((cc = read(f, &c, 1)) != 1) {
			if (cc < 0)
				syslog(LOG_NOTICE, "read: %m");
			shutdown(f, 1+1);
			exit(1);
		}
		if (c == 0)
			break;
		port = port * 10 + c - '0';
	}
	(void) alarm(0);
	if (port != 0) {
		int lport = IPPORT_RESERVED - 1;
		s = rresvport(&lport);
		if (s < 0) {
			syslog(LOG_ERR, "can't get stderr port: %m");
			exit(1);
		}
		if (port >= IPPORT_RESERVED) {
			syslog(LOG_ERR, "2nd port not reserved\n");
			exit(1);
		}
		fromp->sin_port = htons((u_short)port);
		if (connect(s, fromp, sizeof (*fromp)) < 0) {
			syslog(LOG_INFO, "connect second port: %m");
			exit(1);
		}
	}
	dup2(f, 0);
	dup2(f, 1);
	dup2(f, 2);
	hp = gethostbyaddr((char *)&fromp->sin_addr, sizeof (struct in_addr),
		fromp->sin_family);
	if (hp) {
		/*
		 * Attempt to verify that we haven't been fooled by
		 * someone on the net; look up the name and check
		 * that this address corresponds to the name.
		 */
		if (svc_lastlookup == SVC_BIND) {
			strncpy(remotehost, hp->h_name, sizeof(remotehost) - 1);
			remotehost[sizeof(remotehost) - 1] = 0;
			hp = gethostbyname(remotehost);
			if (hp == NULL) {
				syslog(LOG_INFO, "Could not look up address for %s", remotehost);
				error("Could not look up address for your host.  Permission denied.", "Host not in database");
				exit(1);
			} else
				for (i = 0; ; i++) {
					if (hp->h_addr_list[i] == NULL) {
						syslog(LOG_CRIT, "Host addr %s not listed for host %s", inet_ntoa(fromp->sin_addr), hp->h_name);
						error("Host address mismatch.  Permission denied.", "Host address mismatch");
						exit(1);
					}
					if (!bcmp(hp->h_addr_list[i], (caddr_t)&fromp->sin_addr, sizeof(fromp->sin_addr)))
						break;
				}
		}
		hostname = hp->h_name;
	}
	else
		hostname = inet_ntoa(fromp->sin_addr);
	getstr(remuser, sizeof(remuser), "remuser");
	getstr(locuser, sizeof(locuser), "locuser");
	getstr(cmdbuf, sizeof(cmdbuf), "command");
	pwd = getpwnam(locuser);
	if (pwd == NULL) {
		error("Login incorrect.", "No such user");
		exit(1);
	}
	if (chdir(pwd->pw_dir) < 0) {
		(void) chdir("/");
#ifdef notdef
		error("No remote directory.", NULL);
		exit(1);
#endif
	}
	if(svcp->svcauth.seclevel >= SEC_UPGRADE) {
		auth = getauthuid(pwd->pw_uid);
		if(auth == NULL) {
			error("Login incorrect.", "No auth record");
			exit(1);
		}
	}
/*
 * Find the right password to use based on security level.
 */
	if(svcp->svcauth.seclevel < SEC_UPGRADE)
		cp = pwd->pw_passwd;
	else if(svcp->svcauth.seclevel == SEC_UPGRADE)
		if(!pwd->pw_passwd || strcmp(pwd->pw_passwd, "*"))
			cp = pwd->pw_passwd;
		else
			cp = auth->a_password;
	else
		cp = auth->a_password;
/*
 * If account has empty password, let anybody in.
 */
	if (cp != 0 && *cp != '\0' &&
	    ruserok(hostname, pwd->pw_uid == 0, remuser, locuser) < 0) {
		error("Permission denied.", NULL);
		exit(1);
	}
	if(svcp->svcauth.seclevel >= SEC_UPGRADE) {
		if(!(auth->a_authmask&A_LOGIN)) {
			error("Account disabled.", NULL);
			exit(1);
		}
		if(auth->a_pw_maxexp && (auth->a_pass_mod+auth->a_pw_maxexp) < time((long *)0)) {
			error("Password expired.", NULL);
			exit(1);
		}
	}
	if (!access("/etc/nologin", F_OK)) {
		error("Logins currently disabled.", NULL);
		exit(1);
	}
	(void) write(2, "\0", 1);
	if (port) {
		if (pipe(pv) < 0) {
			error("Can't make pipe.", NULL);
			exit(1);
		}
		pid = fork();
		if (pid == -1)  {
			error("Try again.", "Can't fork");
			exit(1);
		}
		if (pid) {
			(void) close(0); (void) close(1); (void) close(2);
			(void) close(f); (void) close(pv[1]);
			readfrom = (1<<s) | (1<<pv[0]);
			ioctl(pv[0], FIONBIO, (char *)&one);
			/* should set s nbio! */
			do {
again:				ready = readfrom;
				if (select(16, &ready, (fd_set *)0,
				    (fd_set *)0, (struct timeval *)0) < 0) {
					if (errno == EINTR)
						goto again;
					break;
				}
				if (ready & (1<<s)) {
					if (read(s, &sig, 1) <= 0)
						readfrom &= ~(1<<s);
					else
						killpg(pid, sig);
				}
				if (ready & (1<<pv[0])) {
					errno = 0;
					cc = read(pv[0], buf, sizeof (buf));
					if (cc <= 0) {
				/* syslog(LOG_ERR,"shutdown: %x\n", ready); */
						shutdown(s, 1+1);
						readfrom &= ~(1<<pv[0]);
					} else
						(void) write(s, buf, cc);
				}
			} while (readfrom);
			exit(0);
		}
		setpgrp(0, getpid());
		(void) close(s); (void) close(pv[0]);
		(void) dup2(pv[1], 2);
	}
	if (*pwd->pw_shell == '\0')
		pwd->pw_shell = "/bin/sh";
	(void) close(f);
	if (setgid(pwd->pw_gid) < 0) {
		syslog(LOG_ERR, "setgid: %m");
		exit(1);
	}
	initgroups(pwd->pw_name, pwd->pw_gid);
	if(auth) {
		if(audcntl(SET_PROC_ACNTL, (char *) 0, 0, auth->a_audit_control, 0) < 0) {
			syslog(LOG_ERR, "audcntl error.");
			exit(1);
		}
		if(audcntl(SET_PROC_AMASK, auth->a_audit_mask, SYSCALL_MASK_LEN+TRUSTED_MASK_LEN, 0) < 0) {
			syslog(LOG_ERR, "audit mask error.");
			exit(1);
		}
		if(audcntl(SETPAID, (char *) 0, 0, 0, auth->a_audit_id) < 0) {
			syslog(LOG_ERR, "error setting audit ID.");
			exit(1);
		}
	} else {
		audcntl(SET_PROC_ACNTL, (char *)0, 0, AUDIT_OR, 0);
		A_PROCMASK_SET(buf, SYS_setgroups, 0, 0);
		A_PROCMASK_SET(buf, LOGIN, 0, 0);
		audcntl(SET_PROC_AMASK, buf, LEN, 0, 0);
	}
	auditit("rsh succeeded", 0);
	if (setuid(pwd->pw_uid) < 0) {
		syslog(LOG_ERR, "setuid: %m");
		exit(1);
	}
	environ = envinit;
	strncat(homedir, pwd->pw_dir, sizeof(homedir)-6);
	strncat(shell, pwd->pw_shell, sizeof(shell)-7);
	strncat(username, pwd->pw_name, sizeof(username)-6);
	cp = rindex(pwd->pw_shell, '/');
	if (cp)
		cp++;
	else
		cp = pwd->pw_shell;
	execl(pwd->pw_shell, cp, "-c", cmdbuf, 0);
	perror(pwd->pw_shell);
	exit(1);
}

/*
 * Error function.  Writes error message to standard error and generates
 * audit record.  If the second argument is non-NULL it will be used for
 * the informational message in the audit record, otherwise the first
 * argument will be used both for the error message and the audit message.
 */
error(usermsg, auditmsg)
char *usermsg, *auditmsg;
{
	char buf[BUFSIZ];

	if(!auditmsg)
		auditmsg = usermsg;
	auditit(auditmsg, 1);
	buf[0] = 1;
	strncpy(&buf[1], usermsg, BUFSIZ-3);
	strcat(buf, "\n");
	(void) write(2, buf, strlen(buf));
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
		if (--cnt == 0) {
			char mesg[64];

			strcpy(mesg, err);
			strcat(mesg, " too long");
			error(mesg, NULL);
			exit(1);
		}
	} while (c != 0);
}
