#ifndef lint
static  char    *sccsid = "@(#)login.c	4.6	(ULTRIX)	2/14/91";
#endif lint

/************************************************************************
 *									*
 *		Copyright (c) 1986, 1987, 1988, 1989, 1990, 1991 by	*
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
 *	$Source: /u1/usr/src/bin/RCS/login.c,v $
 *	$Author: jg $
 *	$Locker:  $
 *	$Log:	login.c,v $
 *
 *	14-Nov-90	dlong
 * Keep prompter alive for the whole time.  Prompter will be expected to
 * exit when the close-on-exec pipe to it is closed on shell startup.  This
 * allows all login errors, including shell-not-found to be reportable
 * through prompter.  It also allows prompter to signal login for
 * keyboard generated interrupts and provides an escape from a hard,intr
 * NFS home directory or mail spool file that has gone away.
 *
 *	26-Sep-90	wessex::lisa
 * Add two additional error messages to explain when a login is refused
 * because of an invalid or missing license PAK. Also extend time that
 * message is displayed for.
 *
 *	04-Sep-90	dlong
 * Don't hang with interrupts disabled if the HOME directory of mail spool
 * file is on an unavailable NFS partition.
 *
 *	17-Jul-89	jsd
 * Workaround ruserok() returning 0 on success, when it used to return 1.
 *
 *	19-Sep-89	dlong
 * Don't update auth entry (for fail count) if it doesn't reside on the local
 * machine.
 *
 *	22-Aug-89	dlong
 * Do an endauthent() before exec'ing password (to prevent retaining
 * an old cached entry), and before handing off to the user (to avoid
 * leaving them an open file descriptor for /etc/auth*).  Also, improve
 * error reporting since ERRORNXIT does not actually display the error
 * message.  Also, remove booby trap of closing all file descriptors after
 * having done a getauthuid() and a getpwnam() which might have intentionally
 * left open file decscriptors behind.  Also, only use one of T_ERROR or
 * T_RESULT in the audit record.
 *
 *	16-Aug-89	S. Logcher
 * Remove extraneous setpwent() and endpwent() calls from dodecnetlogin()
 * routine.
 *
 *	15-Aug-89	D. Long
 * Added prompter "-e" support.  Also, handle empty password better.
 *
 *	19-Jul-89 D. Long
 * Don't issue password expiration messages until after "Login succeeded"
 * message.  Otherwise the old prompter breaks.  Also, don't try and use the
 * UID as an audit ID.  Also, recognize the "e" option and ignore it.  Also,
 * fixed up some comments.
 *
 *	28-Jun-89 D. Long
 * Fixed problem logging in remotely in BSD mode.  Also, look for auth.h
 * in usr/include, not current directory.
 *
 *	15-May-89 Giles Atkinson
 * Convert login limits to use LMF
 *
 *	7/Jun/89	D. Long
 * Added security features for 4.0
 *
 *	029 - Gary A. Gaudet. Fri Apr 21 21:57:43 EDT 1989
 *		Added unbuffered stdin, cyphered name and password,
 *		and bzero buffer, for -P.
 *
 *	028 - Gary A. Gaudet. Mon Apr 17 14:37:31 EDT 1989
 *		Added check for real root uid before using -P.
 *
 *	08/Jun/88	Mark Parenti
 * Changed signal handlers to void.
 *
 *	24/Mar/88	Tim Burke / Rich Hyde
 * Flush standard error after reporting an error.  Return success string to
 * prompter program to prevent reappearance of dialogue box on successful login.
 *
 *	29/Feb/88	Tim Burke / Rich Hyde
 * Flush output after reporting an error.
 *
 * 1.25 20/Jan/88	Mark Parenti
 * Change initialization of passwd structure to reflect changes made
 * for POSIX compliance.
 *
 *	07/Dec/87	Tim Burke / Rich Hyde
 * 1) Added a option -P <programname> that causes login to set it's
 *    standard input and output to be connected to the prompting program.
 *    This allows one to write a X based prompter that prompts for name and
 *    password and then ships that info back to login.
 *
 * 2) Added an option -C "string" to allow the system to specify a
 *    command to be run using the users shell.    It causes a (USER SHELL) -c
 *    "string" to be exec'ed.  This allows one to start a X based application
 *    but still allow normal logins, and [dr]logins to work.
 *
 *	08/Aug/87	Tim Burke
 * Look for LPASS8 in local mode word to preserve 8-bit mode as passed on
 * from getty().
 *
 * 1.22 28/Jul/87	logcher
 * Changed doremotelogin() to call ruserok() in libc.a
 *
 * 1.20 02/Jun/87	logcher
 * In doremotelogin, when opening and reading ~user/.rhosts, seteuid
 * to that user uid and seteuid back after.  This avoids problems
 * when ~user is mounted remotely.
 *
 * 1.19 3/12/87		lp
 * Three times has to make it right! Finally fixed the decnet/telnet clash.
 *
 * 1.18 3/3/87		lp
 * Fixed the problem 1.15 problem abain (as 1.17 broke decnet).
 *
 *      2/25/87		Tim Burke
 * Allow setting of TERMIODISC if "termio" was specified in /etc/ttys.
 *
 * 1.17 2/12/87 	lp
 * Fixed a silly problem introduced in rev 1.15 (which broke telnet logins).
 * 
 * Rev 1.16 1/14/86	lp
 * Increased ahosts to 128 (from 32) as hostnames of the form
 * "washington.berekley.edu username" might be longer than 32 and
 * hence might not match.
 *
 * Revision 1.15 86/12/4	Tim Burke
 * A change to the dodecnetlogin routine to set proxy to -1 so that a prompt
 * for password will appear.
 *
 * Revision 1.14 86/8/8		Tim Burke
 * Inserted changes proposed by Peter Harbo of decnet.  These changes are
 * mainly in the dodecnetlogin routine to return -1 if username, but no
 * password has been received.
 *
 * Revision 1.13 86/6/24	Tim
 * Changed so that invalid rlogins and dlogins are handled similarly.
 * Inserted copyright notice.
 *
 * Revision 1.12 86/3/11	Robin
 * Made change to count logins in the kernel.
 *
 * Revision 1.11 86/2/5    10:05:00 robin
 * fixed problem that stopped erase from working if a second try
 * at logging in was made.
 *
 * Revision 1.8  85/10/22  10:30:00  was
 * add decnet support for remote login
 *
 * Revision 1.7  84/10/29  17:24:33  jg
 * fix problem with -p option.  Wasn't allowing user name, so getty
 * wasn't happy.
 * 
 * Revision 1.6  84/10/25  14:12:23  jg
 * Undid utmp changes.  Added -p flag to preserve environment passed
 * from getty.
 * 
 * Revision 1.5  84/10/01  09:28:38  jg
 * fix allocation bug that caused login to fail.
 * 
 * Revision 1.4  84/09/09  15:50:19  jg
 * fix bug introduced by environment change; was not doing rlogin right.
 * 
 * Revision 1.3  84/09/09  14:51:31  jg
 * fixed login not to destroy the environment set up by whomever is calling it.
 * 
 * Revision 1.2  84/09/01  11:39:08  jg
 * changes required by utmp.h changes.
 * 
 * Revision 1.1  84/04/20  01:07:13  root
 * Initial revision
 * 
 */

/*
static char *rcsid_login_c = "$Header: login.c,v 1.7 84/10/29 17:24:33 jg Exp $";
static	char *sccsid = "@(#)login.c	4.34 (Berkeley) 84/05/07";
*/

/*
 * login [ name ]
 * login -r hostname (for rlogind)
 * login -h hostname (for telnetd, etc.)
 */

#define	NOPRIV
#include <sys/param.h>
#include <sys/quota.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/file.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/sysinfo.h>
#include <sys/utsname.h>
#include <sys/ltatty.h>
#ifndef	NOPRIV
#include <sys/priv.h>
#endif
#include <sys/svcinfo.h>

#include <fcntl.h>
#include <sgtty.h>
#include <utmp.h>
#include <signal.h>
#include <pwd.h>
#include <grp.h>
#include <stdio.h>
#include <lastlog.h>
#include <errno.h>
#include <ttyent.h>
#include <syslog.h>
#include <limits.h>
#include <lmf.h>
#include <strings.h>

#ifndef	NOAUDIT
/* sample code to audit login's */
#include <syscall.h>
#include <sys/syscall.h>
#include <sys/audit.h>
#define LEN (SYSCALL_MASK_LEN+TRUSTED_MASK_LEN)
#endif	NOAUDIT

#include <auth.h>
#include "proto.h"
#ifdef AUTHEN
#include <krb.h>
#endif AUTHEN

#define	SCMPN(a, b)	strncmp(a, b, sizeof(a))
#define	SCPYN(a, b)	strncpy(a, b, sizeof(a))

#define NMAX	sizeof(utmp.ut_name)

#define	FALSE	0
#define	TRUE	-1

#ifndef	NOAUDIT
static char buf[LEN];
static int i;
#endif	NOAUDIT

char	nolog[] =	"/etc/nologin";
char	qlog[]  =	".hushlogin";
char	maildir[40] =	"/usr/spool/mail/";
char	lastlog[] =	"/usr/adm/lastlog";
struct	passwd nouser = {"", "nope", -1, -1, -1, -1, -1, "", "", "", "" };
struct	sgttyb ttyb;
struct	utmp utmp;
struct	ltattyi ltattyi;
char	minusnam[16] = "-";
char	*envinit[] =
	{ 0 };		/* now set by setenv calls */
/*
 * This bounds the time given to login.  We initialize it here
 * so it can be patched on machines where it's too small.
 */
int	timeout = 240;

char	term[64];

struct	passwd *pwd, pwd_save;
char *auth_db = AUTHORIZATION_DB;
AUTHORIZATION *auth, *getauthuid();
char	*strcat(), *rindex(), *index();
void	timedout();
char	*ttyname();
char	*crypt16(), *crypt();
char	*getpass();
char	*stypeof();
extern	char **environ;
extern	int errno;

struct	tchars tc = {
	CINTR, CQUIT, CSTART, CSTOP, CEOT, CBRK
};
struct	ltchars ltc = {
	CSUSP, CDSUSP, CRPRNT, CFLUSH, CWERASE, CLNEXT
};

int	rflag, dflag;
int 	notty = FALSE;
char	rusername[NMAX+1], lusername[NMAX+1];
char	rpassword[NMAX+1];
char	name[NMAX+1];
char	*tty;
static 	prompt_pid;
static	eflag = 0;

int	lastsend = -1;
char	*pp;


/*
 * Signal handler to just do an exit.
 */
void exit_handler(sig, code, scp)
int sig, code;
struct sigconext *scp;
{
	exit(0);
}

/*
 * Function to send a message to the user, either through prompter
 * or the tty.
 */
static sendreq(opcode, data)
int opcode;
char *data;
{
	REQ request;
	register REQ *req = &request;
	int i, length;

	if(data)
		length = strlen(data) + 1;
	else
		length = 0;
/*
 * If using extended protocol send a packet out on stdout.
 */
	if(eflag) {
		SENDREQ(req, opcode, data, length);
/*
 * Error messages are always acknowledged to provide a synchronous
 * user interface.
 */
		if(opcode == ERROR) {
			GETREQ(req, i);
			if(req->opcode != ACKNOWLEDGE) {
				sendreq(ERRORNXIT, "Protocol error\n");
				cleanup(10, 1, "Protocol error");
			}
		}
	} else {
/*
 * If not using the extended protocol handle each message type
 * locally.
 */
		lastsend = opcode;
		switch(opcode) {
/*
 * Get login user name.
 */
		case GETNAME:
		case GETENAME:
			if(!notty) {
				getloginname(data);
				bcopy(&utmp, req->data, sizeof utmp);
				req->length = sizeof utmp;
			}
			break;
/*
 * Get users password.
 */
		case GETPWD:
		case GETEPWD:
			if(!notty) {
				pp = getpass(data);
				strncpy(req->data, pp, REQDATASIZ);
				req->length = strlen(pp) + 1;
			}
			break;
/*
 * Display an error message.
 */
		case ERROR:
		case ERRORNXIT:
			if(data && length > 0) {
				fputs(data, stdout);
				fflush(stdout);
			}
			break;
/*
 * Display a non-error message.
 */
		case VALID:
		case VALIDNXIT:
		case CHGPWD:
			if(data && length > 0) {
				fputs(data, stdout);
				fflush(stdout);
			}
			break;
/*
 * Only meaningful to extended protocol.
 */
		case INITIALIZE:
			break;
		}
	}
}

/*
 * Function to receive a message from the user, either through prompter
 * or some other route.
 */
static getreq(req)
REQ *req;
{
	int i;

/*
 * If using extended protocol just retrieve a packet from stdin.
 */
	if(eflag) {
		GETREQ(req, i);
	} else {
/*
 * If not using extended protocol handle each case locally.  Look at
 * the last sent message to determine what input should be obtained.
 */
		i = 0;
		switch(lastsend) {
/*
 * Waiting for input.
 */
		case GETNAME:
		case GETENAME:
		case GETPWD:
		case GETEPWD:
			switch(lastsend) {
/*
 * Waiting for a login name.
 */
			case GETNAME:
			case GETENAME:
				req->opcode = NAME;
				break;
/*
 * Waiting for a password.
 */
			case GETPWD:
			case GETEPWD:
				req->opcode = PASSWD;
				break;
			}
/*
 * if notty is true we are talking to an old prompter.  Otherwise
 * we are talking to a simple tty and all cases are actually handled
 * in sendreq().
 */
			if(notty) {
				i = fgets(req->data, REQDATASIZ, stdin) != NULL;
				if(i)
					req->length = strlen(req->data) + 1;
				else {
					req->length = 0;
					i = -1;
				}
			}
			break;
/*
 * The initialize case is only meaningful with the extended protocol.
 */
		case INITIALIZE:
			break;
		}
		lastsend = -1;
	}
	return i;
}

/*
 * Prompter is used to solicit login identity and authentication
 * through a workstation display.
 */
void start_prompter(prompter, tty) 
char *prompter;
char *tty;
{
	int infds[2], outfds[2];
	int i;
	

	pipe(infds);
	dup2(infds[0], 10);
	close(infds[0]);
	infds[0] = 10;
	dup2(infds[1], 11);
	close(infds[1]);
	infds[1] = 11;
	pipe(outfds);
	dup2(outfds[0], 12);
	close(outfds[0]);
	outfds[0] = 12;
	dup2(outfds[1], 13);
	close(outfds[1]);
	outfds[1] = 13;

	if(eflag) {
		close(2);
		i = open("/dev/null", O_WRONLY);
		if(i != 2) {
			dup2(i, 2);
			close(i);
		}
	}
	if(prompt_pid = fork()) {
		close(infds[1]);
		close(outfds[0]);
		/*
		 * parents stdin;
                 */
		dup2(infds[0], 0);
		close(infds[0]);
		/* stdout */
		dup2(outfds[1], 1);
		if(!eflag) {
			close(2);
			dup2(outfds[1], 2);
		}
		close(outfds[1]);
	} else {
		close(infds[0]);
		close(outfds[1]);
		/* child stdin */
		dup2(outfds[0], 0);
		close(outfds[0]);
		/* stdout */
		dup2(infds[1], 1);
		close(infds[1]);
		if(eflag)
			execlp(prompter, prompter, "-e", tty, 0);
		else
			execlp(prompter, prompter, tty, 0);
		exit(0);
	}
	  
}

/*
 * audit_event() logs a LOGIN audit event.
 */
audit_event(code, message)
int code;
char *message;
{
#ifndef	NOAUDIT
	char tmask[AUD_NPARAM];
	char *ar[AUD_NPARAM];
	int i=0;
	static char name[9];
	static char host[17];
/*
  Fill in tokens.
*/
	strncpy(name, utmp.ut_name, 8);
	tmask[i] = T_LOGIN;
	ar[i++] = name;
	if(pwd) {
		tmask[i] = T_HOMEDIR;
		ar[i++] = pwd->pw_dir;
		tmask[i] = T_SHELL;
		ar[i++] = pwd->pw_shell;
	}
	if(code)
		tmask[i] = T_ERROR;
	else
		tmask[i] = T_RESULT;
	ar[i++] = (char *) code;
	if(message) {
		tmask[i] = T_CHARP;
		ar[i++] = message;
	}
	if(tty) {
		tmask[i] = T_DEVNAME;
		ar[i++] = tty;
	}
	strncpy(host, utmp.ut_host, 16);
	if(*host) {
		tmask[i] = T_CHARP;
		ar[i++] = host;
	}
	if(i < AUD_NPARAM)
		tmask[i] = '\0';
	if ( audgen ( LOGIN, tmask, ar ) == -1 ) perror ( "audgen" );
#endif	NOAUDIT
}

/*
 * Cleanup() is the general error exit routine.
 */
cleanup(time, code, message)
int time;
int code;
char *message;
{
	int pid;
	
	signal(SIGALRM, exit_handler);

	if(code && message) {
		audit_event(code, message);
	}

	alarm(60);
	fflush(stdout);
	fflush(stderr);
	close(1);
	close(2);
	if(notty) {
		for(pid=wait(0);pid != prompt_pid && pid != -1;pid = wait(0));
	} else {
		ioctl(0, TIOCHPCL, (struct sgttyb *) 0);
		sleep(time);
	}
	close(0);
	exit(code);
}

static int intcount = 0;
void no_dir(i, j, k)
int i, j, *k;
{
	intcount++;
}

/*
 * The main program.
 */
main(argc, argv)
	char *argv[];
{
	register char *namep;
	int pflag = 0;		/* preserve environment from getty */
	int t, f, c;
	int invalid, quietlog;
	int lcmask;
	FILE *nlfd;
	char *ttyn;
	int ldisc = 0, zero = 0;
	struct ttyent *t_ent;
        char *cmd = NULL;
	    char *prompter = NULL;
	    static int first = TRUE;
	REQ request;
	REQ *req = &request;
	extern int soft_exp, sec_level;
	char theKey, *pKey, *pChar;
	ver_t ultrix_v;
	struct utsname un;
        struct svcinfo *svcp;
	int i, j, status, diff;
	char lastmessage[100];
#ifdef AUTHEN
	char namebuf[ANAME_SZ];
	char *ptr;
#endif AUTHEN

#ifndef	NOAUDIT
/*
  Turn off all auditing except for LOGIN and setgroups.
*/
	if ( audcntl ( SET_PROC_ACNTL, (char *)0, 0, AUDIT_AND, 0 ) == -1 ) perror ( "audcntl" );  
	A_PROCMASK_SET ( buf, SYS_setgroups, 1, 1 );
	A_PROCMASK_SET ( buf, LOGIN, 1, 1 );
	if ( audcntl ( SET_PROC_AMASK, buf, LEN, 0, 0 ) == -1 ) perror ( "audcntl" );
#endif	NOAUDIT

	for (t = getdtablesize(); t >= 3; t--)
	  close(t);

	config_auth();

	signal(SIGALRM, timedout);
	signal(SIGQUIT, SIG_IGN);
	signal(SIGINT, SIG_IGN);
	setpriority(PRIO_PROCESS, 0, 0);
	quota(Q_SETUID, 0, 0, 0);
	/*
	 * -r is used by rlogind to cause the autologin protocol;
	 * -h is used by other servers to pass the name of the
	 *     remote host to login so that it may be placed in utmp and wtmp
	 * -p is used by getty to tell login not to destroy the environment
	 * -P is used to specify a prompting program.
	 * -C is used to specify a command to be execed by the users shell
	 *     if the user passes the authentication and auth.
	 * -e is used to specify extended prompter communications protocol.
	 *     It is only meaningful in the presence of "-P".
	 */
/*	Initialize Kerberos if possibly needed 	*/
#ifdef AUTHEN
        if((svcp = getsvc()) == NULL)
                {
                fputs(" Cannot access security type\n", stderr);
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
                               (char *)NULL, "/var/dss/kerberos/tkt/tkt.login")
                                       != RET_OK) {
                               fputs("Kerberos initialization failure\n",stderr);
                                }
			}
                }
#endif AUTHEN
	if (argc > 1) {
		if (strcmp(argv[1], "-r") == 0) {
			rflag = doremotelogin(argv[2]);
			SCPYN(utmp.ut_host, argv[2]);
			argc = 0;
		}
		if (strcmp(argv[1], "-h") == 0 && getuid() == 0) {
			dflag = dodecnetlogin();
			SCPYN(utmp.ut_host, argv[2]);
			argc = 0;
		}
		if (argc > 1 && strcmp(argv[1], "-p") == 0) {
			argv++;
			pflag = 1;
			argc -= 1;
		}
		if (argc > 2 && strcmp(argv[1], "-P") == 0) {
			if(getuid() == 0) {
				notty = TRUE;
				prompter = argv[2];
			}
			argv += 2;
			argc -= 2;
		}
		if (argc > 2 && strcmp(argv[1], "-C") == 0) {
			if(getuid() == 0) {
				quietlog = TRUE;
				cmd = argv[2];
			}
			argv += 2;
			argc -= 2;
		}
		if (argc > 1 && strcmp(argv[1], "-e") == 0 && getuid() == 0) {
			notty = TRUE;
			eflag = 1;
			argv++;
			argc--;
		}
	}
        if( ! notty ) {
		ioctl(0, TIOCLGET, &lcmask);
		lcmask &= LPASS8;
		ioctl(0, TIOCLSET, &zero);
		lcmask |= (LPRTERA | LCRTBS | LCRTERA);
		ioctl(0, TIOCLSET, &lcmask);
		ioctl(0, TIOCNXCL, 0);
		ioctl(0, FIONBIO, &zero);
		ioctl(0, FIOASYNC, &zero);
		ioctl(0, TIOCGETP, &ttyb);
		/* If on a LAT terminal, store the LAT server's name
		 * into the host field of the utmp record.
		 */
		if (ioctl(0, LIOCTTYI, &ltattyi) == 0) {
		    SCPYN(utmp.ut_host, ltattyi.lta_server_name);
		}
		/*
		 * If talking to an rlogin process,
		 * propagate the terminal type and
		 * baud rate across the network.
		 */
		if (rflag)
			doremoteterm(term, &ttyb);
		ioctl(0, TIOCSLTC, &ltc);
		ioctl(0, TIOCSETC, &tc);
		ioctl(0, TIOCSETP, &ttyb);
		ttyn = ttyname(0);
		if (ttyn == (char *)0)
			ttyn = "/dev/tty??";
		tty = rindex(ttyn, '/');
		if (tty == NULL)
			tty = ttyn;
		else
			tty++;
	} else {
		if(argc > 1)
		  tty = argv[1];
		argc--;
		argv++;
	}
	if(prompter != NULL) {
		start_prompter(prompter, tty);
	} else
	  alarm(timeout);
	
	openlog("login", 0);

	if(eflag) {
/*
 * If extended login protocol get rid of stderr to prevent spurious data
 * from getting into the prompter conversation.  All errors are reported
 * through the protocol using sendreq().
 */
		close(2);
/*
 * Process the initial message from prompter.
 */
		if(getreq(req) < 0)
			cleanup(5, 1, "Prompter died");
		if(req->opcode == INITIALIZE)
			sendreq(INITIALIZE, VERSION);
		else {
			sendreq(ERRORNXIT, "Protocol error\n");
			cleanup(10, 1, "Protocol error");
		}
	}
	t = 0;
	do {
                if(!notty) {
                        ldisc = 0;
                        ioctl(0, TIOCSETD, &ldisc);
		}
		invalid = FALSE;
		SCPYN(utmp.ut_name, "");
		/*
		 * Name specified, take it.
		 */
		if (argc > 1) {
			SCPYN(utmp.ut_name, argv[1]);
			argc = 0;
		} else if (notty) {
/*
 * Talking to prompter.
 */
			/*
			 * 029 - GAG
			 * get the string and decypher
			 */
			sendreq(GETENAME, (char *)0);
			if(getreq(req) < 0)
				cleanup(5, 1, "Prompter died");
			if(req->opcode != NAME) {
				sendreq(ERRORNXIT, "Protocol error\n");
				cleanup(10, 1, "Protocol error");
			}
/*
 * unsalt the string.
 */
			pChar = req->data;
			pKey = pChar++;
			for (i = req->length; i && (*pChar) && (*pChar != '\n'); i--) {
				*pKey ^= *pChar;
				pKey = pChar++;
			}
			theKey = *pKey; /* save the Key */
			*pKey = NULL;

			if(!eflag) {
				if(sscanf(req->data, "name: %s", utmp.ut_name) != 1)
					utmp.ut_name[0] = '\0';
			} else {
				if(req->length > sizeof utmp.ut_name)
					req->length = sizeof utmp.ut_name;
				bzero(utmp.ut_name, sizeof utmp.ut_name);
				bcopy(req->data, utmp.ut_name, req->length);
			}

			if(first) {
				alarm(timeout);
				first = FALSE;
			}
		}
		
		/*
		 * If remote login take given name,
		 * otherwise prompt user for something.
		 */
		if (rflag || dflag) {
			SCPYN(utmp.ut_name, lusername);
			/* autologin failed, prompt for passwd */
			if (rflag == -1)
				rflag = 0;
			if (dflag == -1)
				dflag = 0;
		} else
			getloginname(&utmp);
                if (!notty && !strcmp(pwd->pw_shell, "/bin/csh")) {
			ldisc = NTTYDISC;
			ioctl(0, TIOCSETD, &ldisc);
		}
		/*
		 * If "termio" is specified in /etc/ttys then use the TERMIODISC
		 * line discipline.
		 */
		if ((t_ent = getttynam(tty)) != NULL)
			if (t_ent->ty_status & TTY_TERMIO){
				ldisc = TERMIODISC;
				ioctl(0, TIOCSETD, &ldisc);
			}
		/*
		 * If no remote login authentication and
		 * a password exists for this user, prompt
		 * for one and verify it.
		 */
		if(sec_level >= SEC_UPGRADE && (auth == NULL || auth->a_uid != pwd->pw_uid))
			auth = getauthuid(pwd->pw_uid);
		if(!rflag && !dflag) {

			setpriority(PRIO_PROCESS, 0, -4);
			if( !notty ) {
				if((sec_level < SEC_UPGRADE) ||
				    sec_level == SEC_UPGRADE && strcmp(pwd->pw_passwd, "*")) {
					if(*pwd->pw_passwd)
						pp = getpass("Password:");
					else
						pp = "";
				} else {
					if(!auth || *auth->a_password)
						pp = getpass("Password:");
					else
						pp = "";
				}
			} else {
/*
 * Talking to prompter.
 */
				int tmp = alarm(10);
				void (*handler)();

				/*
				 * 029 - GAG
				 * unbuffer stdin, line[0] is the Key,
				 * line[1...] is the Cypher,
				 */
				handler = signal(SIGALRM, exit_handler);
				if(!eflag)
					setbuf(stdin, (char *) NULL);
				sendreq(GETEPWD, (char *)0);
				if(getreq(req) < 0)
					cleanup(5, 1, "Prompter died");
				if(req->opcode != PASSWD) {
					sendreq(ERRORNXIT, "Protocol error\n");
					cleanup(10, 1, "Protocol error");
				}
				if(eflag) {
					LISTOFPASSWORDS *lop = (LISTOFPASSWORDS *)req->data;
					pp = pChar = lop->passwords[0].data;
					pKey = pChar++;
					i = lop->passwords[0].length-1;
				} else {
					for(i=req->length+1; i; i--)
						req->data[i] = req->data[i-1];
					pChar = req->data;
					pKey = pChar++;
					*pKey = theKey;
					i = req->length;
				}
				alarm(tmp);
				signal(SIGALRM, handler);
				/*
				 * 029 - GAG
				 * decypher line,
				 * line[0...] becomes plaintext
				 */
				for (; i && (*pChar != '\n'); i--) {
					*pKey ^= *pChar;
					pKey = pChar++;
				}
				*pKey = NULL;
	
				if(!eflag) {
 					if(sscanf(req->data,"password: %[^\n]\n", req->data) != 1)
						req->data[0] = '\0';
					pp = req->data;
				}
			}
			if(sec_level > SEC_UPGRADE || (sec_level == SEC_UPGRADE && !strcmp(pwd->pw_passwd, "*"))) {
				if(auth && !(!*auth->a_password && *pp)) {
					namep = crypt16(pp, auth->a_password);
					pp = auth->a_password;
				} else {
					pp = "Nologin";
					namep = "";
				}
			} else if(!(!*pwd->pw_passwd && *pp)) {
				namep = crypt(pp, pwd->pw_passwd);
				pp = pwd->pw_passwd;
			} else {
				pp = "Nologin";
				namep = "";
			}
			setpriority(PRIO_PROCESS, 0, 0);
			if (strcmp(namep, pp)) {
				bzero(req->data, REQDATASIZ);
				invalid = TRUE;
				if(sec_level >= SEC_UPGRADE) {
					if(auth=getauthuid(pwd->pw_uid))
						if(svc_lastlookup == SVC_LOCAL) {
							auth->a_fail_count++;
							storeauthent(auth);
						}
				}
			}
			bzero(req->data, REQDATASIZ);
		}
		/*
		 * If user not super-user, check for logins disabled.
		 */
		if (!invalid && pwd->pw_uid != 0 && (nlfd = fopen(nolog, "r")) > 0) {
			char error[REQDATASIZ];
			int i;

			for(i=0; (c=getc(nlfd)) != EOF && i < REQDATASIZ-1; i++)
				error[i] = c;
			error[i++] = '\0';
			sendreq(ERROR, error);
                        cleanup(5, 1, "Logins disabled");
		}
		/*
		 * If valid so far and root is logging in,
		 * see if root logins on this terminal are permitted.
		 */
		if (!invalid && pwd->pw_uid == 0 && !rootterm(tty)) {
			sendreq(ERROR, "Requires secure terminal\n");
			syslog(LOG_INFO, "ROOT LOGIN REFUSED %s", tty);
			invalid = TRUE;
			cleanup(5, 1, "Unsecure terminal");
		}
		if(!invalid && sec_level >= SEC_UPGRADE) {
			if(!(auth->a_authmask&A_LOGIN)) {
				sendreq(ERROR, "This account has been disabled\n");
				sendreq(ERRORNXIT, (char *)0);
				cleanup(5, 1, "Account disabled");
			}
		}
		if(!invalid && sec_level >= SEC_UPGRADE && auth->a_pw_maxexp) {
/*
 * Check for password expiration.
 */
			diff = auth->a_pass_mod
				+ auth->a_pw_maxexp - time((long *) 0);
			if(diff < 0) {
				if(-diff > soft_exp || !(auth->a_authmask&A_CHANGE_PASSWORD)) {
					sendreq(ERROR, "Your password has expired\n");
					sendreq(ERRORNXIT, (char *)0);
					invalid = TRUE;
					cleanup(5, 1, "Password expired");
				} else {
					sendreq(CHGPWD, "Your password has expired, please change it\n");
					endauthent();
					if((i=vfork()) < 0) {
						invalid = TRUE;
						sendreq(ERRORNXIT, "Unable to fork\n");
						cleanup(5, 1, "Unable to fork");
					}
					if(i == 0) {
						chdir("/");
 
/*
 * Fire up passwd(1) to force user to set their password.
 */
						if(eflag) 
							execl("/bin/passwd", "/bin/passwd", "-ea", pwd->pw_name, (char *)0);
						else {
							setuid(pwd->pw_uid);
							setgid(pwd->pw_gid);
							execl("/bin/passwd", "passwd", pwd->pw_name, (char *) 0);
						}
						sendreq(ERROR, "Unable to exec passwd\n");
						exit(1);
					} else {
						while((j=wait(&status)) != i && j >= 0) ;
						if(status) {
							invalid = TRUE;
							sendreq(ERRORNXIT, "Failed to set new password\n");
							cleanup(5, 1, "Failed to set new password");
						} else {
							if(svc_lastlookup == SVC_LOCAL) {
								if(!(auth=getauthuid(pwd->pw_uid))) {
									sendreq(ERRORNXIT, "Failed to retrieve auth record\n");
									cleanup(5, 2, "No auth record");
								}
							} else
								auth->a_pass_mod = time((long *) 0);
						}
					}
				}
			}
		}
		if (invalid) {
			if (++t >= 5) {
				sendreq(ERRORNXIT, "Login incorrect\n");
				syslog(LOG_INFO,
				    "REPEATED LOGIN FAILURES %s, %s",
					tty, utmp.ut_name);
                                cleanup(5, 1, "Repeated login failures");
			} else {
				if(pwd == &nouser)
					audit_event(1, "Invalid account");
				else
					audit_event(1, "Failed authentication");
				sendreq(ERROR, "Login incorrect\n");
			}
		} else {
			if(sec_level >= SEC_UPGRADE) {
				if(auth->a_fail_count) {
					char line[BUFSIZ];
					int i;

					i = auth->a_fail_count;
					auth->a_fail_count = 0;
					if(svc_lastlookup == SVC_LOCAL)
						storeauthent(auth);
					sprintf(line, "There have been %d unsuccessful login attempts on your account\n", i);
					sendreq(ERROR, line);
					if(!notty)
						fputs("", stdout);
				}
				diff = auth->a_pass_mod
					+ auth->a_pw_maxexp - time((long *) 0);
				if(auth->a_pw_maxexp && diff < 5*24*60*60) {
					char line[BUFSIZ];

					if(diff < 24*60*60)
						strcpy(line, "Your password will expire very soon\n");
					else
						sprintf(line, "Your password will expire in %d days\n",
							diff/(24*60*60));
					sendreq(ERROR, line);
				}
			}
			if(notty && !eflag) {
				sendreq(VALIDNXIT, "Login succeeded\n");
			}
		}
		if (*pwd->pw_shell == '\0')
			pwd->pw_shell = "/bin/sh";
		/*
		 * Remote login invalid must have been because
		 * of a restriction of some sort, no extra chances.
		 */
		if ((rflag|dflag) && invalid)
			cleanup(0, 1, "Failed remote login");
	} while (invalid);
/* committed to login turn off timeout */
	alarm(timeout);

	if (quota(Q_SETUID, pwd->pw_uid, 0, 0) < 0) {
		if (errno == EUSERS) {
			sendreq(ERROR, "Too many users logged on already.\nTry again later.\n");
			sendreq(ERRORNXIT, (char *) 0);
			cleanup(5, 1, "Too many users");
		} else if (errno == EPROCLIM) {
			sendreq(ERROR, "You have too many processes running.\n");
			sendreq(ERRORNXIT, (char *) 0);
			cleanup(5, 1, "Too many processes for user");
		} else {
			sendreq(ERROR, "Error in quota call\n");
			sendreq(ERRORNXIT, (char *) 0);
			cleanup(5, 2, "Internal error in quota call");
		}
	}

	/* Do the LMF check.
         * The Ultrix version numbers are obtained from the kernel
         * by uname().   The release date is defined on the command
         * line which compiles this program, or by the pre-processor
	 * lines below.
	 */

	uname(&un);
	i = sscanf(un.release, "%hd.%hd", &ultrix_v.v_major, &ultrix_v.v_minor);
	if (i==0) {
		/* Try again in case it looks like `X4.6' */
		i = sscanf(un.release, "%*[^0-9]%hd.%hd", &ultrix_v.v_major,
			   &ultrix_v.v_minor);
	}
	if (i<2)
		ultrix_v.v_minor = 0;
	if (i<1)
		ultrix_v.v_major = 0;

#ifndef ULTRIX_RELEASE_DATE
#define ULTRIX_RELEASE_DATE 0x7fffffff
#endif

	if (lmf_probe_license("ULTRIX", 0, &ultrix_v,
			      ULTRIX_RELEASE_DATE, 0) != 0 &&
	    pwd->pw_uid != 0)			/* Always let root in */
			switch(errno) {
			case EDQUOT:
				sendreq(ERROR, "Too many users logged on already.\nTry again later.\n");
                        	sendreq(ERRORNXIT, (char *) 0);
                        	cleanup(10, 1, "Too many users");
				break;
			case ERANGE:
				sendreq(ERROR, "License not valid for this version of ULTRIX.\n");
                        	sendreq(ERRORNXIT, (char *) 0);
                        	cleanup(10, 1, "License version not valid");
				break;
			default:
				sendreq(ERROR, "No valid license found for ULTRIX.\n");
				sendreq(ERRORNXIT, (char *) 0);
				cleanup(10, 1, "No valid license");
				break;
			}
	alarm(0);
	/* Mark this as a login process in the kernel. */

	setsysinfo(SSI_LOGIN, 0, 0, 0, 0);

	time(&utmp.ut_time);
	if(!notty) { 
		t = ttyslot();
	} else {
	  	struct ttyent *tent;
	  	setttyent();
		for(t = 1; (tent = getttyent()) != NULL; t++ ) {
			if (strcmp(tent->ty_name, tty) == 0) {
				endttyent();
				break;
			}
	       }
	}
	if (t > 0 && (f = open("/etc/utmp", O_WRONLY)) >= 0) {
		lseek(f, (long)(t*sizeof(utmp)), 0);
		SCPYN(utmp.ut_line, tty);
		write(f, (char *)&utmp, sizeof(utmp));
		close(f);
	}
	if ((f = open("/usr/adm/wtmp", O_WRONLY|O_APPEND)) >= 0) {
		write(f, (char *)&utmp, sizeof(utmp));
		close(f);
	}
	lastmessage[0] = '\0';
	if ((f = open(lastlog, O_RDWR)) >= 0) {
		struct lastlog ll;
		char *cp=lastmessage;

		if(!notty) {
		lseek(f, (long)pwd->pw_uid * sizeof (struct lastlog), 0);
		if (read(f, (char *) &ll, sizeof ll) == sizeof ll &&
		    ll.ll_time != 0) {
			sprintf(cp, "Last login: %.*s ",
			    24-5, (char *)ctime(&ll.ll_time));
			cp += strlen(cp);
			if (*ll.ll_host != '\0')
				sprintf(cp, "from %.*s",
				    sizeof (ll.ll_host), ll.ll_host);
			else
				sprintf(cp, "on %.*s",
				    sizeof (ll.ll_line), ll.ll_line);
		}
		}
		lseek(f, (long)pwd->pw_uid * sizeof (struct lastlog), 0);
		time(&ll.ll_time);
		SCPYN(ll.ll_line, tty);
		SCPYN(ll.ll_host, utmp.ut_host);
		write(f, (char *) &ll, sizeof ll);
		close(f);
	}
	if(!notty) {
		struct group *gp;

		gp = getgrnam("tty");
		chown(ttyn, pwd->pw_uid, gp?gp->gr_gid:pwd->pw_gid);
		chmod(ttyn, 0620);
	}
	if(setgid(pwd->pw_gid) < 0) {
		char line[40];

		sprintf(line, "Unable to set gid to %d\n", pwd->pw_gid);
		sendreq(ERROR, line);
		sendreq(ERRORNXIT, (char *)0);
		cleanup(5, 1, "Unable to set gid");
	}
	strncpy(name, utmp.ut_name, NMAX);
	name[NMAX] = '\0';
	initgroups(name, pwd->pw_gid);
	if(!notty)
		quota(Q_DOWARN, pwd->pw_uid, (dev_t)-1, 0);
#ifndef	NOAUDIT
/*
 * Set audit infomation for user.
 */
	if(auth) {
		if(audcntl(SET_PROC_ACNTL, (char *) 0, 0, auth->a_audit_control, 0) < 0) {
			sendreq(ERROR, "audit control error.\n");
			sendreq(ERRORNXIT, (char *)0);
			cleanup(5, 1, "Error in audit control");
		}
		if(audcntl(SET_PROC_AMASK, auth->a_audit_mask,
		    SYSCALL_MASK_LEN+TRUSTED_MASK_LEN, 0, 0) < 0) {
			sendreq(ERROR, "audit mask error.\n");
			sendreq(ERRORNXIT, (char *)0);
			cleanup(5, 1, "Error in audit mask");
		}
		if(audcntl(SETPAID, (char *) 0, 0, 0,
		    auth->a_audit_id) < 0) {
			sendreq(ERROR, "Error setting audit ID.\n");
			sendreq(ERRORNXIT, (char *)0);
			cleanup(5, 1, "Error in audit ID");
		}
	} else {
		audcntl(SET_PROC_ACNTL, (char *)0, 0, AUDIT_OR, 0);
		A_PROCMASK_SET(buf, SYS_setgroups, 0, 0);
		A_PROCMASK_SET(buf, LOGIN, 0, 0);
		audcntl(SET_PROC_AMASK, buf, LEN, 0, 0);
	}
#endif	NOAUDIT
/*
  Generate audit record for successful login.
*/
	audit_event(0, "Login succeeded");
#ifndef	NOPRIV
/*
 * Set up privileges
 */
	if(setpriv(P_SET_PROC|P_SET_INHL, auth->a_privs) < 0) {
		sendreq(ERROR, "privilege mask error.\n");
		sendreq(ERRORNXIT, (char *)0);
		cleanup(5, 1, "Error in privilege mask");
	}
#endif
	if(sec_level >= SEC_UPGRADE)
		endauthent();
	endpwent();
	if(setuid(pwd->pw_uid) < 0) {
		char line[40];

		sprintf(line, "Unable to setuid to %d\n", pwd->pw_uid);
		sendreq(ERROR, line);
		sendreq(ERRORNXIT, (char *)0);
		cleanup(5, 1, "Error setting UID");
	}
/*
 * Change to home directory.  Don't hang if it's an unavailable NFS partition.
 */
	signal(SIGALRM, SIG_DFL);
	signal(SIGTSTP, SIG_IGN);
	signal(SIGQUIT, no_dir);
	signal(SIGINT, no_dir);
	siginterrupt(SIGQUIT, 1);
	siginterrupt(SIGINT, 1);
	if (chdir(pwd->pw_dir) < 0) {
		if(strcmp(pwd->pw_name, "root") && !shell_supported(pwd->pw_shell)) {
			sendreq(ERROR, "No directory!\n");
			sendreq(ERRORNXIT, (char *)0);
			cleanup(5, 1, "No directory");
		}
		sendreq(ERROR, "No directory!  Logging in with home=/\n");
		pwd->pw_dir = "/";
		if(chdir(pwd->pw_dir) < 0)
			sendreq(ERROR, "No directory!\n");
	}
	/* Finally, we can see if this is to be a quiet login */
	if(!notty )
		quietlog = (access(qlog, F_OK) == 0);

	/* destroy environment unless user has asked to preserve it */
	if (pflag == 0) environ = envinit;
	
	/* set up environment, this time without destruction */
	/* copy the environment before setenving */
	{
	int i = 0;

	char **envnew;
	while (environ [i] != NULL) i++;

	envnew = (char **) malloc (sizeof (char *) * (i + 1));
	for (; i >= 0; i--) envnew [i] = environ [i];
	environ = envnew;
	}

	setenv("HOME=",pwd->pw_dir);
	setenv("SHELL=",pwd->pw_shell);
	if (term[0] == '\0') strncpy (term,stypeof(tty), sizeof(term));
	setenv("TERM=",term);
	setenv("USER=",pwd->pw_name);
	setenv("PATH=",":/usr/ucb:/bin:/usr/bin");

	if ((namep = rindex(pwd->pw_shell, '/')) == NULL)
		namep = pwd->pw_shell;
	else
		namep++;
	strcat(minusnam, namep);
	umask(022);
        if (!notty && tty[sizeof("tty")-1] == 'd')
		syslog(LOG_NOTICE, "DIALUP %s %s", tty, pwd->pw_name);
	if (!notty && !quietlog) {		/*  */
		struct stat statb;
		int status;

		puts(lastmessage);
		showmotd();
		strcat(maildir, pwd->pw_name);
		if((status=stat(maildir, &statb)) >= 0) {
			if((statb.st_mode & S_IFMT) == S_IFDIR) {
				strcat(maildir, "/");
				strcat(maildir, pwd->pw_name);
				status = stat(maildir, &statb);
			}
			if (status >= 0 && access(maildir, R_OK) == 0)
				if (statb.st_size)
					puts("You have mail.");
		}
	}
	signal(SIGALRM, SIG_DFL);
	signal(SIGQUIT, SIG_DFL);
	signal(SIGINT, SIG_DFL);
	signal(SIGTSTP, SIG_IGN);
	closelog();
	if(notty) {
		fcntl(0, F_SETFD, 1);
		fcntl(1, F_SETFD, 1);
	}
	if(cmd != NULL) {
		char buf[4096];
		sprintf(buf, "exec %s %s", cmd, tty);
		execlp(pwd->pw_shell, minusnam, "-c", buf, 0);
	} else {
		execlp(pwd->pw_shell, minusnam, 0);
	}
	/*
	 * Emergency clause in case the password file gets totaled or other
	 * such extremes.  Look for username of root to prevent other users
	 * (such as operator users) who would have a uid of 0 from getting
	 * /bin/sh.
 	 */
	if ((pwd->pw_uid == 0) &&
		((strncmp(pwd->pw_name,"root",strlen(pwd->pw_name))) == 0))
	{
		static char binshell[] = "/bin/sh";

		sendreq(ERROR, "Bad shell, root will use /bin/sh\n");
		if(cmd != NULL) {
			char buf[4096];

			sprintf(buf, "exec %s %s", cmd, tty);
			execl(binshell, "-sh", "-c", buf, 0);
		} else {
			execl(binshell, "-sh", 0);
		}
	}
	if(!notty)
		perror(pwd->pw_shell);
	sendreq(ERROR, "No shell\n");
	sendreq(ERRORNXIT, (char *)0);
        cleanup(0, 1, (char *) 0);
}

#define	SHELLS	"/etc/shells"
/*
 * Function to match a shell name against a pathname.
 */
static int path_match(path, shell)
char *path, *shell;
{
	char *cp;

	cp = strchr(path, '\n');
	if(cp)
		*cp = '\0';
	if(*shell != '/') {
		cp = strrchr(path, '/');
		if(!cp)
			cp = path;
		else
			cp++;
	} else
		cp = path;
	if(!strcmp(shell, cp)) {
		return 1;
	}
	return 0;
}

static int shell_supported(shell)
char *shell;
{
	FILE *fp;
	int i;

	if(!(fp=fopen(SHELLS, "r"))) {
		static char *shells[] = { "/bin/sh", "/bin/csh", "/usr/bin/sh5",
			"/usr/bin/ksh", (char *)0 };

		for(i=0; shells[i]; i++)
			if(path_match(shells[i], shell))
				return 1;
	} else {
		char line[BUFSIZ];

		while(fgets(line, sizeof line, fp))
			if(path_match(line, shell)) {
				close(fp);
				return 1;
			}
		close(fp);
	}
	return 0;
}

getloginname(up)
	register struct utmp *up;
{
	register char *namep;
	char c;
	static CRYPT_PASSWORD password;

	while (up->ut_name[0] == '\0') {
		namep = up->ut_name;
		fputs("login: ", stdout);
		while ((c = getchar()) != '\n') {
			if (c == ' ')
				c = '_';
			if (c == EOF)
			        cleanup(0, 0, (char *) 0);
			if (namep < up->ut_name+NMAX)
				*namep++ = c;
		}
	}
	strncpy(lusername, up->ut_name, NMAX);
	lusername[NMAX] = 0;
	if ((pwd = getpwnam(lusername)) == NULL) 
		pwd = &nouser;
	else {
		bcopy(pwd, &pwd_save, sizeof pwd_save);
		pwd = &pwd_save;
		if(sec_level >= SEC_UPGRADE) {
			auth = getauthuid(pwd->pw_uid);
			if(sec_level > SEC_UPGRADE && !auth)
				pwd = &nouser;
		}
/*
		else {
			if(!strcmp(pwd->pw_passwd, "*")) {
				strncpy(password, auth->a_password, sizeof password);
				pwd->pw_passwd = password;
			}
		}
*/
	}
}

void
timedout(sig, code, scp)
int sig, code;
struct sigcontext *scp;
{

	printf("Login timed out after %d seconds\n", timeout);
        cleanup(0, 0, (char *) 0);
}

rootterm(tty)
	char *tty;
{
	register struct ttyent *t;

	if ((t = getttynam(tty)) != NULL) {
		if (t->ty_status & TTY_SECURE)
			return (1);
	}
	return (0);
}

showmotd()
{
	FILE *mf;
	register c;

	intcount = 0;
	if ((mf = fopen("/etc/motd", "r")) != NULL) {
		while ((c = getc(mf)) != EOF && intcount == 0)
			putchar(c);
		fclose(mf);
	}
}

#undef	UNKNOWN
#define UNKNOWN "su"

char *
stypeof(ttyid)
	char *ttyid;
{
	register struct ttyent *t;

	if (ttyid == NULL || (t = getttynam(ttyid)) == NULL)
		return (UNKNOWN);
	return (t->ty_type);
}

dodecnetlogin()
{
	char *getenv();
	char *cp;
	int proxy = 1;
	static CRYPT_PASSWORD password;

	/*
	 * check environment variables are present, and defined for DECnet
	 *	if "NETWORK" != "DECnet", force login
	 *	if "ACCESS" == "DEFAULT", force login
	 *	if "USER" is not defined, or is too long, force
	 *	login
	 */
	if (((cp = getenv("NETWORK")) == 0) || (strcmp(cp, "DECnet") != 0))
		proxy = 0; 	/* Else we break telnet */
	/*
	 * if terminal type is defined, use it for local terminal
	 */
	if (cp = getenv("TERM")) {
		if (strcmp(cp, "none"))
			strncat(term, cp, sizeof(term)-6);
	}
	/*
	 * don't login if using default access
	 */
	if (((cp = getenv("ACCESS")) == 0) || (strcmp(cp, "DEFAULT") == 0))
		proxy = 0;
	/*
	 * if local name is too long, can't log user in
	 */
	if (((cp = getenv("USER")) == 0) || (strlen(cp) > NMAX)) {
		if (getenv("USERNAME") == NULL)
			return(0);
		else
			proxy = 0;
	} else
		strcpy(lusername, cp);
	/*
	 * save the connecting host name
	 */
	if (cp = getenv("REMNODE"))
		SCPYN(utmp.ut_host, cp);
	/*
	 * Get username from environment if this is not a proxy line.
	 */
	if (*lusername == '\000') {
		if (cp = getenv("USERNAME"))
			strcpy(lusername,cp);
		proxy = -1;
	}
	pwd = getpwnam(lusername);
	if (pwd == NULL) {
		pwd = &nouser;
		proxy = -1;
	} else {
		bcopy(pwd, &pwd_save, sizeof pwd_save);
		pwd = &pwd_save;
		if(sec_level >= SEC_UPGRADE)
		if((auth=getauthuid(pwd->pw_uid)) == NULL) {
			pwd = &nouser;
			proxy = -1;
		}
/*
		else {
			strncpy(password, auth->a_password, sizeof password);
			pwd->pw_passwd = password;
		}
*/
	}
	return(proxy);
}

doremotelogin(host)
	char *host;
{
	static CRYPT_PASSWORD password;

	getstr(rusername, sizeof (rusername), "remuser");
	getstr(lusername, sizeof (lusername), "locuser");
	getstr(term, sizeof(term), "Terminal type");
	if (getuid()) {
		pwd = &nouser;
		return(-1);
	}
	pwd = getpwnam(lusername);
	if (pwd == NULL) {
		pwd = &nouser;
		return(-1);
	} else {
		bcopy(pwd, &pwd_save, sizeof pwd_save);
		pwd = &pwd_save;
		if(sec_level >= SEC_UPGRADE)
		if((auth=getauthuid(pwd->pw_uid)) == NULL) {
			pwd = &nouser;
			return -1;
		}
/*
		else {
			strncpy(password, auth->a_password, sizeof password);
			pwd->pw_passwd = password;
		}
*/
	}
	/* ruserok returns -1 on error.  Force it to return a 1 on success */
	return(ruserok(host, (pwd->pw_uid == 0), rusername, lusername) < 0 ? -1 : 1);
}

getstr(buf, cnt, err)
	char *buf;
	int cnt;
	char *err;
{
	char c;

	do {
		if (read(0, &c, 1) != 1)
			cleanup(0, 1, "EOF reading input");
		if (--cnt < 0) {
			printf("%s too long\r\n", err);
			cleanup(0, 1, "Input line too long");
		}
		*buf++ = c;
	} while (c != 0);
}

char	*speeds[] =
    { "0", "50", "75", "110", "134", "150", "200", "300",
      "600", "1200", "1800", "2400", "4800", "9600", "19200", "38400" };
#define	NSPEEDS	(sizeof (speeds) / sizeof (speeds[0]))

doremoteterm(term, tp)
	char *term;
	struct sgttyb *tp;
{
	char *cp = index(term, '/');
	register int i;

	if (cp) {
		*cp++ = 0;
		for (i = 0; i < NSPEEDS; i++)
			if (!strcmp(speeds[i], cp)) {
				tp->sg_ispeed = tp->sg_ospeed = i;
				break;
			}
	}
	tp->sg_flags = ECHO|CRMOD|ANYP|XTABS;
}

setenv (var, value)
/*
   sets the value of var to be arg in the Unix 4.2 BSD environment env.
   Var should end with '='.
   (bindings are of the form "var=value")
   This procedure assumes the memory for the first level of environ
   was allocated using malloc.
 */
char *var, *value;
{
	extern char **environ;
	int index = 0;

	while (environ [index] != NULL)
	{
	    if (strncmp (environ [index], var, strlen (var)) == 0)
	    {
		/* found it */
		environ [index] = (char *) malloc (strlen (var) + strlen (value) + 1);
		strcpy (environ [index], var);
		strcat (environ [index], value);
		return;
	    }
	    index ++;
	}

	if ((environ = (char **) realloc (environ, sizeof (char *) * (index + 2))) == NULL)
	{
	    fputs("Setenv: malloc out of memory\n", stderr);
	    cleanup(0, 1, "Unable to allocate memory for environment variables");
	}

	environ [index] = (char *) malloc (strlen (var) + strlen (value) + 1);
	strcpy (environ [index], var);
	strcat (environ [index], value);
	environ [++index] = NULL;
}

