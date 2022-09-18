#ifndef lint
static	char sccsid[] = "@(#)inetd.c	4.3	(ULTRIX)	9/11/90";
#endif

/************************************************************************
 *									*
 *			Copyright (c) 1985 by				*
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
 *	
 *	09/04/89 - lp
 *		Add 4.3bsd style inetd.conf to allow setting
 *		of uid/gid of executed process. This allows some
 *		internet tasks to run a something other than root
 *		(like maybe daemon or something). The compatibility
 *		shows up by looking to see if the 5th argument on
 * 		a line looks like a path. If it does then its the
 *		Ultrix style of file in use. If not then we assume its
 * 		a username and use the 6th argument as the command to 
 *		execute. FIXES a possible security problem as it 
 *		allows user started daemons to run a non-root.
 *
 *	05/06/89 -- jsd
 *		Allow invocation by super-user only
 *
 *	06/09/88 -- map
 *		Changed signal handlers to void.
 *
 *	04/29/85 -- jrs
 *		Update to handle greater than 32 fd's
 *
 *	4/10/85 -- jrs
 *		Clean up little nits in code.  Also add timer to select
 *		call so we can momentarily ignore multithreaded datagram
 *		connections in order to give the server time to pick up
 *		the initial packet before we try to listen to the socket again.
 *
 *	Based on 4.2BSD labeled:
 *		inetd.c	4.2	84/05/18
 *
 *-----------------------------------------------------------------------
 */

/*
 * Inetd - Internet super-server
 *
 * This program invokes all internet services as needed.
 * connection-oriented services are invoked each time a
 * connection is made, by creating a process.  This process
 * is passed the connection as file descriptor 0 and is
 * expected to do a getpeername to find out the source host
 * and port.
 *
 * Datagram oriented services are invoked when a datagram
 * arrives; a process is created and passed a pending message
 * on file descriptor 0.  Datagram servers may either connect
 * to their peer, freeing up the original socket for inetd
 * to receive further messages on, or ``take over the socket'',
 * processing all arriving datagrams and, eventually, timing
 * out.	 The first type of server is said to be ``multi-threaded'';
 * the second type of server ``single-threaded''. 
 *
 * Inetd uses a configuration file which is read at startup
 * and, possibly, at some later time in response to a hangup signal.
 * The configuration file is ``free format'' with fields given in the
 * order shown below.  Continuation lines for an entry must being with
 * a space or tab.  All fields must be present in each entry.
 *
 *	service name			must be in /etc/services
 *	socket type			stream/dgram/raw/rdm/seqpacket
 *	protocol			must be in /etc/protocols
 *	wait/nowait			single-threaded/multi-threaded
 *	user (optional)			user to run program as
 *	server program			full path name
 *	server program arguments	maximum of MAXARGS (5)
 *
 * Comment lines are indicated by a `#' in column 1.
 */

#include <sys/param.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/file.h>
#include <sys/time.h>
#include <sys/wait.h>

#include <netinet/in.h>
#include <arpa/inet.h>

#include <errno.h>
#include <stdio.h>
#include <signal.h>
#include <netdb.h>
#include <syslog.h>
#include <pwd.h>

#define	MAXARGV	5			/* number of arguments to subdaemon */
#define	CONFIG	"/etc/inetd.conf"	/* default configuration file name */
#define	FDWORDS	((NOFILE+(NBBY*NBPW)-1)/(NBBY*NBPW))
					/* number of words of bit mask */

extern	int errno;

void	reapchild();
char	*index();
char	*malloc();

int	debug = 0;
int	allsock[FDWORDS];
int	delaysock[FDWORDS];
int	sockndx;
struct	timeval deltime = { 1, 0 };
struct	timeval *seltimer = NULL;
int	options;
struct	servent *sp;
char	*conffile = CONFIG;
void	config();

struct	servtab {
	char	*se_service;		/* name of service */
	int	se_socktype;		/* type of socket to use */
	char	*se_proto;		/* protocol used */
	short	se_wait;		/* single threaded server */
	short	se_checked;		/* looked at during merge */
	char 	*se_user;		/* user if present */
	char	*se_server;		/* server program */
	char	*se_argv[MAXARGV+1];	/* program arguments */
	int	se_fd;			/* open descriptor */
	struct	sockaddr_in se_ctrladdr;/* bound address */
	struct	servtab *se_next;
} *servtab;

main(argc, argv)
	int argc;
	char *argv[];
{
	register struct servtab *sep;
	register struct passwd *pwd;
	char *cp, buf[50];
	int pid, i;

	if (getuid()) {
		fprintf(stderr, "%s: not super user\n", argv[0]);
		exit(1);
	}

	argc--, argv++;
	while (argc > 0 && *argv[0] == '-') {
		for (cp = &argv[0][1]; *cp; cp++) switch (*cp) {

		case 'd':
			debug = 1;
			options |= SO_DEBUG;
			break;

		default:
			fprintf(stderr,
			    "inetd: Unknown flag -%c ignored.\n", *cp);
			break;
		}
		argc--, argv++;
	}
	if (argc > 0) {
		conffile = argv[0];
	}
#ifndef DEBUG
	if (fork())
		exit(0);
	{ int s;
	for (s = 0; s < 10; s++)
		(void) close(s);
	}
	(void) open("/", O_RDONLY);
	(void) dup2(0, 1);
	(void) dup2(0, 2);
	{ int tt = open("/dev/tty", O_RDWR);
	  if (tt > 0) {
		(void) ioctl(tt, TIOCNOTTY, 0);
		(void) close(tt);
	  }
	}
#endif
	for (sockndx = 0; sockndx < FDWORDS; sockndx++) {
		delaysock[sockndx] = 0;
	}
	openlog("inetd", LOG_PID, 0);
	config();
	(void) signal(SIGHUP, config);
	(void) signal(SIGCHLD, reapchild);
	for (;;) {
		int readable[FDWORDS], s, ctrl;

		while (anybit(allsock, FDWORDS) == 0) {
			sigpause(0);
		}
		for (sockndx = 0; sockndx < FDWORDS; sockndx++) {
			readable[sockndx] = allsock[sockndx];
		}
		if (select(NOFILE, readable, 0, 0, seltimer) <= 0) {
			if (anybit(readable, FDWORDS) == 0 && seltimer != NULL) {
				for (sockndx = 0; sockndx < FDWORDS; sockndx++) {
					allsock[sockndx] |= delaysock[sockndx];
					delaysock[sockndx] = 0;
				}
				seltimer = NULL;
			}
			continue;
		}
		for (sockndx = 0; sockndx < FDWORDS; sockndx++) {
			s = ffs(readable[sockndx])-1;
			if (s >= 0) {
				s += NBBY * NBPW * sockndx;
				break;
			}
		}
		if (s < 0)
			continue;
		for (sep = servtab; sep; sep = sep->se_next)
			if (s == sep->se_fd)
				goto found;
		abort();
	found:
		if (debug)
			fprintf(stderr, "someone wants %s\n", sep->se_service);
		if (sep->se_socktype == SOCK_STREAM) {
			ctrl = accept(s, 0, 0);
			if (debug)
				fprintf(stderr, "accept, ctrl %d\n", ctrl);
			if (ctrl < 0) {
				if (errno == EINTR)
					continue;
				syslog(LOG_WARNING, "accept: %m");
				continue;
			}
		} else
			ctrl = sep->se_fd;
#define mask(sig)	(1 << (sig - 1))
		(void) sigblock(mask(SIGCHLD)|mask(SIGHUP));
		pid = fork();
		if (pid < 0) {
			if (sep->se_socktype == SOCK_STREAM)
				(void) close(ctrl);
			sleep(1);
			continue;
		}
		if (sep->se_wait) {
			sep->se_wait = pid;
			clearbit(allsock, s);
		} else {
			if (sep->se_socktype == SOCK_DGRAM) {
				pickbit(delaysock, s);
				clearbit(allsock, s);
				seltimer = &deltime;
			}
		}
		(void) sigsetmask(0);
		if (pid == 0) {
#ifdef	DEBUG
			int tt = open("/dev/tty", O_RDWR);
			if (tt > 0) {
				(void) ioctl(tt, TIOCNOTTY, 0);
				(void) close(tt);
			}
#endif
			(void) dup2(ctrl, 0);
			(void) close(ctrl);
			(void) dup2(0, 1);
			for (i = getdtablesize(); --i > 2; )
				(void) close(i);
			if (debug)
				fprintf(stderr, "%d execl %s\n",
				    getpid(), sep->se_server);

			if(sep->se_user != NULL) {
				if ((pwd = getpwnam(sep->se_user)) == NULL) {
					syslog(LOG_ERR, "Invalid user name to run command -  %s: No such user", sep->se_user);
					if(sep->se_socktype != SOCK_STREAM)
						recv(0, buf, sizeof(buf), 0);
					_exit(1);
				}
				if(pwd->pw_uid) {
					(void) setgid((gid_t)pwd->pw_gid);
					initgroups(pwd->pw_name, pwd->pw_gid);
					(void) setuid((uid_t)pwd->pw_uid);
				}
			}
			execv(sep->se_server, sep->se_argv);
			if (sep->se_socktype != SOCK_STREAM)
				(void) recv(0, buf, sizeof (buf), 0);
			syslog(LOG_ERR, "execv %s: %m", sep->se_server);
			_exit(1);
		}
		if (sep->se_socktype == SOCK_STREAM)
			(void) close(ctrl);
	}
}

void
reapchild()
{
	union wait status;
	int pid;
	register struct servtab *sep;

	for (;;) {
		pid = wait3(&status, WNOHANG, 0);
		if (pid <= 0)
			break;
		if (debug)
			fprintf(stderr, "%d reaped\n", pid);
		for (sep = servtab; sep; sep = sep->se_next)
			if (sep->se_wait == pid) {
				if (status.w_status)
					syslog(LOG_WARNING,
					    "%s: exit status 0x%x",
					    sep->se_server, status);
				if (debug)
					fprintf(stderr, "restored %s, fd %d\n",
					    sep->se_service, sep->se_fd);
				pickbit(allsock, sep->se_fd);
				sep->se_wait = 1;
			}
	}
}

void
config()
{
	register struct servtab *sep, *cp, **sepp;
	struct servtab *getconfigent(), *enter();
	int omask;
	int on = 1;

	if (!setconfig()) {
		syslog(LOG_ERR, "%s: %m", conffile);
		return;
	}
	for (sep = servtab; sep; sep = sep->se_next)
		sep->se_checked = 0;
	while (cp = getconfigent()) {
		for (sep = servtab; sep; sep = sep->se_next)
			if (strcmp(sep->se_service, cp->se_service) == 0 &&
			    strcmp(sep->se_proto, cp->se_proto) == 0)
				break;
		if (sep != 0) {
			int i;

			omask = sigblock(mask(SIGCHLD));
			sep->se_wait = cp->se_wait;
#define SWAP(a, b) { char *c = a; a = b; b = c; }
			if (cp->se_server)
				SWAP(sep->se_server, cp->se_server);
			for (i = 0; i < MAXARGV; i++)
				SWAP(sep->se_argv[i], cp->se_argv[i]);
			(void) sigsetmask(omask);
			freeconfig(cp);
		} else
			sep = enter(cp);
		sep->se_checked = 1;
		if (sep->se_fd != -1)
			continue;
		sp = getservbyname(sep->se_service, sep->se_proto);
		if (sp == 0) {
			syslog(LOG_ERR, "%s/%s: unknown service",
			    sep->se_service, sep->se_proto);
			continue;
		}
		sep->se_ctrladdr.sin_port = sp->s_port;
		if ((sep->se_fd = socket(AF_INET, sep->se_socktype, 0)) < 0) {
			syslog(LOG_ERR, "%s/%s: socket: %m",
			    sep->se_service, sep->se_proto);
			continue;
		}
#define turnon(fd, opt) \
setsockopt(fd, SOL_SOCKET, opt, (char *)&on, sizeof(on))
		if (strcmp(sep->se_proto, "tcp") == 0 && (options & SO_DEBUG) &&
		    turnon(sep->se_fd, SO_DEBUG) < 0)
			syslog(LOG_ERR, "setsockopt (SO_DEBUG): %m");
		if (turnon(sep->se_fd, SO_REUSEADDR) < 0)
			syslog(LOG_ERR, "setsockopt (SO_REUSEADDR): %s %m",sep->se_service);
#undef turnon
		if (bind(sep->se_fd, &sep->se_ctrladdr,
		    sizeof (sep->se_ctrladdr), 0) < 0) {
			syslog(LOG_ERR, "%s/%s: bind: %m",
			    sep->se_service, sep->se_proto);
			continue;
		}
		if (sep->se_socktype == SOCK_STREAM)
			(void) listen(sep->se_fd, 10);
		pickbit(allsock, sep->se_fd);
	}
	endconfig();
	/*
	 * Purge anything not looked at above.
	 */
	omask = sigblock(mask(SIGCHLD));
	sepp = &servtab;
	while (sep = *sepp) {
		if (sep->se_checked) {
			sepp = &sep->se_next;
			continue;
		}
		*sepp = sep->se_next;
		if (sep->se_fd != -1) {
			clearbit(allsock, sep->se_fd);
			clearbit(delaysock, sep->se_fd);
			if (anybit(delaysock, FDWORDS) == 0) {
				seltimer = NULL;
			}
			(void) close(sep->se_fd);
		}
		freeconfig(sep);
		free((char *)sep);
	}
	(void) sigsetmask(omask);
}

struct servtab *
enter(cp)
	struct servtab *cp;
{
	register struct servtab *sep;
	int omask;
	char *strdup();

	sep = (struct servtab *)malloc(sizeof (*sep));
	if (sep == (struct servtab *)0) {
		syslog(LOG_ERR, "Out of memory.");
		exit(-1);
	}
	*sep = *cp;
	sep->se_fd = -1;
	omask = sigblock(mask(SIGCHLD));
	sep->se_next = servtab;
	servtab = sep;
	(void) sigsetmask(omask);
	return (sep);
}

FILE	*fconfig = NULL;
struct	servtab serv;
char	line[256];
char	*skip(), *nextline();

setconfig()
{

	if (fconfig != NULL) {
		(void) fseek(fconfig, 0, L_SET);
		return (1);
	}
	fconfig = fopen(conffile, "r");
	return (fconfig != NULL);
}

endconfig()
{

	if (fconfig == NULL)
		return;
	(void) fclose(fconfig);
	fconfig = NULL;
}

struct servtab *
getconfigent()
{
	register struct servtab *sep = &serv;
	char *cp, *arg;
	int argc;

	while ((cp = nextline(fconfig)) && ((*cp == '#') || (strlen(cp) == 0)))
		;
	if (cp == NULL)
		return ((struct servtab *)0);
	sep->se_service = strdup(skip(&cp));
	arg = skip(&cp);
	if (strcmp(arg, "stream") == 0)
		sep->se_socktype = SOCK_STREAM;
	else if (strcmp(arg, "dgram") == 0)
		sep->se_socktype = SOCK_DGRAM;
	else if (strcmp(arg, "rdm") == 0)
		sep->se_socktype = SOCK_RDM;
	else if (strcmp(arg, "seqpacket") == 0)
		sep->se_socktype = SOCK_SEQPACKET;
	else if (strcmp(arg, "raw") == 0)
		sep->se_socktype = SOCK_RAW;
	else
		sep->se_socktype = -1;
	sep->se_proto = strdup(skip(&cp));
	arg = skip(&cp);
	sep->se_wait = strcmp(arg, "wait") == 0;
	/*
	 * Check next arg to see if its a path. If it is not it must
	 * be a user name and we're in 4.3bsd compatibility mode.
	 */
	arg = skip(&cp);

	if(*arg != '/') {
		sep->se_user = strdup(arg);
		arg = skip(&cp);
	} else
		sep->se_user = NULL;

	sep->se_server = strdup(arg);
	argc = 0;
	for (arg = skip(&cp); cp; arg = skip(&cp))
		if (argc < MAXARGV)
			sep->se_argv[argc++] = strdup(arg);
	while (argc <= MAXARGV)
		sep->se_argv[argc++] = NULL;
	return (sep);
}

freeconfig(cp)
	register struct servtab *cp;
{
	int i;

	if (cp->se_service)
		free(cp->se_service);
	if (cp->se_proto)
		free(cp->se_proto);
	if (cp->se_server)
		free(cp->se_server);
	for (i = 0; i < MAXARGV; i++)
		if (cp->se_argv[i])
			free(cp->se_argv[i]);
}

char *
skip(cpp)
	char **cpp;
{
	register char *cp = *cpp;
	char *start;

again:
	while (*cp == ' ' || *cp == '\t')
		cp++;
	if (*cp == '\0') {
		char c;

		c = getc(fconfig);
		(void) ungetc(c, fconfig);
		if (c == ' ' || c == '\t')
			if (cp = nextline(fconfig))
				goto again;
		*cpp = (char *)0;
		return ((char *)0);
	}
	start = cp;
	while (*cp && *cp != ' ' && *cp != '\t')
		cp++;
	if (*cp != '\0')
		*cp++ = '\0';
	*cpp = cp;
	return (start);
}

char *
nextline(fd)
	FILE *fd;
{
	char *cp;

	if (fgets(line, sizeof (line), fd) == NULL)
		return ((char *)0);
	cp = index(line, '\n');
	if (cp)
		*cp = '\0';
	return (line);
}

char *
strdup(cp)
	char *cp;
{
	char *new;

	if (cp == NULL)
		cp = "";
	new = malloc(strlen(cp) + 1);
	if (new == (char *)0) {
		syslog(LOG_ERR, "Out of memory.");
		exit(-1);
	}
	(void) strcpy(new, cp);
	return (new);
}

anybit(field, count)
int *field;
int count;
{
	int index;

	for (index = 0; index < count; index++) {
		if (field[index] != 0) {
			return(-1);
		}
	}
	return(0);
}

pickbit(field, sbit)
int *field;
int sbit;
{
	field[sbit / (NBBY * NBPW)] |= 1 << (sbit % (NBBY * NBPW));
}

clearbit(field, cbit)
int *field;
int cbit;
{
	field[cbit / (NBBY * NBPW)] &= ~(1 << (cbit % (NBBY * NBPW)));
}
