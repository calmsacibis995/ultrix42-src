
#ifndef lint
static	char	*sccsid = "@(#)shutdown.c	4.1	(ULTRIX)	7/2/90";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1984,86 by			*
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
 * shutdown.c
 *
 *	static	char *sccsid = "@(#)shutdown.c	4.19 (Berkeley) 83/06/17";
 *
 *	01-May-84	mah.  Restruct the sending of warning messages to
 *			prevent shutdown from hanging.
 *
 *	24-Jul-84	ma.  Correct broadcast message and put in chdir to
 *			root directory.
 *
 *	22-Oct-84	ma.  Add flag for opser initiated shutdown.
 *
 *	20-Jan-86	bjg. Log shutdown message into kernel error log
 *
 *	04-Dec-86	pmk. Added logging of shutdown msg in error log
 *
 *	12-Feb-87	jaa. Added sun's code to broadcast to any nfs clients,
 *				using /usr/etc/rwalld.
 *
 *	09-Jun-88	map. Changed signal handlers to void.
 *
 *	19-Nov-88	afd. Fixed Usage message.
 *
 */

#include <stdio.h>
#include <ctype.h>
#include <signal.h>
#include <utmp.h>
#include <setjmp.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <sys/errlog.h>
#include <sys/errno.h>
#include <rpc/rpc.h>
#include <rpc/pmap_clnt.h>
#include <nfs/nfs.h>
#include <rpcsvc/mount.h>
#include <rpcsvc/rwall.h>
#include <sys/socket.h>
#include <netdb.h>

/*
 *	/etc/shutdown when [messages]
 *
 *	allow super users to tell users and remind users
 *	of iminent shutdown of unix
 *	and shut it down automatically
 *	and even reboot or halt the machine if they desire
 */
#ifdef DEBUG
#define LOGFILE "shutdown.log"
#else
#define LOGFILE "/usr/adm/shutdownlog"
#endif
#define	REBOOT	"/etc/reboot"
#define	HALT	"/etc/halt"
#define MAXINTS 20
#define	HOURS	*3600
#define MINUTES	*60
#define SECONDS
#define NLOG		20		/* no of args possible for message */
#define	NOLOGTIME	5 MINUTES
#define IGNOREUSER	"sleeper"

char	hostname[32];

time_t	getsdt();

extern	char *ctime();
extern	struct tm *localtime();

struct	utmp utmp;
void	finish(), do_nothing();
int	sint;
int	stogo;
char	tpath[] =	"/dev/";
int	nlflag = 1;		/* nolog yet to be done */
int	killflg = 1;
int	reboot = 0;
int	halt = 0;
int	opser = 0;	/*flag to indicate if shutdown initiated from opser*/
unsigned seconds = 300;	/*sleep 5 minutes at a shot*/
char	term[sizeof tpath + sizeof utmp.ut_line];
char	tbuf[BUFSIZ];
char	buf[BUFSIZ];
char	nolog1[] = "\n\nNO LOGINS: System going down at %5.5s\n\n";
char	*nolog2[NLOG+1];
#ifdef	DEBUG
char	nologin[] = "nologin";
#else
char	nologin[] = "/etc/nologin";
#endif
time_t	nowtime;

struct interval {
	int stogo;
	int sint;
} interval[] = {
	4 HOURS,	1 HOURS,
	2 HOURS,	30 MINUTES,
	1 HOURS,	15 MINUTES,
	30 MINUTES,	10 MINUTES,
	15 MINUTES,	5 MINUTES,
	10 MINUTES,	5 MINUTES,
	5 MINUTES,	3 MINUTES,
	2 MINUTES,	1 MINUTES,
	1 MINUTES,	30 SECONDS,
	0 SECONDS,	0 SECONDS
};

struct hostlist {
    char *host;
    struct hostlist *nxt;
} *hostlist;

char	hostname[32];
char *shutter, *getlogin();

main(argc,argv)
	int argc;
	char **argv;
{
	register i, ufd;
	register char **mess, *f;
	char *ts;
	time_t sdt;
	int h, m;
	int first, pid;
	FILE *msgf;
	struct hostlist *hl;

	shutter = getlogin();
	gethostname(hostname, sizeof (hostname));
	if (chdir("/"))				/*if change dir to root fails,*/
		perror("chdir");		/* say why.*/
	argc--, argv++;
	while (argc > 0 && (f = argv[0], *f++ == '-')) {
		while (i = *f++) switch (i) {
		case 'k':
			killflg = 0;
			continue;
		case 'r':
			reboot = 1;
			continue;
		case 'h':
			halt = 1;
			continue;
		case 'o':		/*flag for opser initiated shutdown*/
			opser = 1;
			continue;
		default:
			fprintf(stderr, "shutdown: '%c' - unknown flag\n", i);
			exit(1);
		}
		argc--, argv++;
	}
	if (argc < 1) {
		printf("Usage: shutdown [ -krho ] shutdowntime [ message ]\n");
		finish();
	}
	if (geteuid()) {
		fprintf(stderr, "NOT super-user\n");
		finish();
	}
	gethostlist();
	nowtime = time((time_t *)0);
	sdt = getsdt(argv[0]);
	argc--, argv++;
	i = 0;
	while (argc-- > 0)
		if (i < NLOG)
			nolog2[i++] = *argv++;
	nolog2[i] = NULL;
	m = ((stogo = sdt - nowtime) + 30)/60;
	h = m/60; 
	m %= 60;
	ts = ctime(&sdt);
	printf("Shutdown at %5.5s (in ", ts+11);
	if (h > 0)
		printf("%d hour%s ", h, h != 1 ? "s" : "");
	printf("%d minute%s) ", m, m != 1 ? "s" : "");
#ifndef DEBUG
	signal(SIGHUP, SIG_IGN);
	signal(SIGQUIT, SIG_IGN);
	signal(SIGINT, SIG_IGN);
#endif
	signal(SIGTTOU, SIG_IGN);
	signal(SIGTERM, finish);
	signal(SIGALRM, do_nothing);
	setpriority(PRIO_PROCESS, 0, PRIO_MIN);
	fflush(stdout);
#ifndef DEBUG
	if (i = fork()) {
					/*
					 *if opser initiated shutdown don't what
					 *to exit, so sit here and munch.
					 */
		while (opser)
			sleep(seconds);	/*sleeps 5 minutes at a shot*/
					/*
					 *otherwise print out pid and exit
					 */
		printf("[pid %d]\n", i);
		exit(0);
	}
#else
	putc('\n', stdout);
#endif
	sint = 1 HOURS;
	f = "";
	first = 1;
	for (;;) {
		for (i = 0; stogo <= interval[i].stogo && interval[i].sint; i++)
			sint = interval[i].sint;
		if (stogo > 0 && (stogo-sint) < interval[i].stogo)
			sint = stogo - interval[i].stogo;
		if (stogo <= NOLOGTIME && nlflag) {
			nlflag = 0;
			nolog(sdt);
		}
		if (sint >= stogo || sint == 0)
			f = "FINAL ";
		nowtime = time((time_t *) 0);
		pid = fork();
		if (pid == 0) {			/*this is the child so do msg*/
			msgf = fopen("/etc/shutdown.msg", "w");
			warn(msgf, sdt, nowtime, f);
			if (first || sdt - nowtime > 1 MINUTES) {
				if (*nolog2)
					fprintf(msgf, "\t...");
				for (mess = nolog2; *mess; mess++)
					fprintf(msgf, " %s", *mess);
			}
			fprintf(msgf,"\r\n");
			fclose(msgf);
			system("/bin/wall < /etc/shutdown.msg");

			/* now warn anybody that has us remotely mounted */
			for (hl = hostlist; hl != NULL; hl = hl->nxt) {
				rwarn(sdt, nowtime, f, hl->host);
				if (first || sdt - nowtime > 1 MINUTES) {
					buf[0] = 0;
					if (*nolog2) {
						strcat(buf, "\t...");
						for (mess = nolog2; *mess; mess++) {
							strcat(buf, " ");
							strcat(buf, *mess);
						}
						rprintf(hl->host, buf);
					}
				}
			}

			exit(0);
		}
		if (stogo <= 0) {
	printf("\n\007\007System shutdown time has arrived\007\007\n");
			log_entry(sdt);
			unlink(nologin);
			if (!killflg) {
				printf("but you'll have to do it yourself\n");
				finish();
			}
#ifndef DEBUG
			kill(-1, SIGTERM);	/* terminate everyone */
			sleep(5);		/* & wait while they die */
			if (reboot)
				execle(REBOOT, "reboot", 0, 0);
			if (halt)
				execle(HALT, "halt", 0, 0);
			kill(1, SIGTERM);	/* sync */
			kill(1, SIGTERM);	/* sync */
			sleep(20);
#else
			printf("EXTERMINATE EXTERMINATE\n");
#endif
			finish();
		}
		stogo = sdt - time((time_t *) 0);
		if (stogo > 0 && sint > 0)
			sleep(sint<stogo ? sint : stogo);
		stogo -= sint;
		first = 0;
	}
}

time_t
getsdt(s)
	register char *s;
{
	time_t t, t1, tim;
	register char c;
	struct tm *lt;

	if (strcmp(s, "now") == 0)		/*undocumented "time" for
						 *shutdown, ie. now.
						 */
		return(nowtime);
	if (*s == '+') {			/*handles the +xx format*/
		++s; 				/*get the 1st digit*/
		t = 0;				/*initialize t*/
		for (;;) {
			c = *s++;
			if (!isdigit(c))	/*make sure its a digit*/
				break;
			t = t * 10 + c - '0';	/*calculate the value*/
		}
		if (t <= 0)
			t = 5;			/*set default value for t*/
		t *= 60;			/*set value in seconds*/
		tim = time((time_t *) 0) + t;	/*calculate time of shutdown*/
		return(tim);
	}
	t = 0;				/*initialize t for xx:xx format*/
	while (strlen(s) > 2 && isdigit(*s))	/*calculate hour*/
		t = t * 10 + *s++ - '0';
	if (*s == ':')				/*step past :*/
		s++;
	if (t > 23)				/*check range of t*/
		goto badform;
	tim = t*60;				/*tim = min of hours*/
	t = 0;					/*initialize t for :xx*/
	while (isdigit(*s))			/*calculate minutes*/
		t = t * 10 + *s++ - '0';
	if (t > 59)				/*check range of t*/
		goto badform;
	tim += t; 				/*add mins and mins of hrs*/
	tim *= 60;				/*put into seconds*/
	t1 = time((time_t *) 0);		/*calculate time of shutdown*/
	lt = localtime(&t1);
	t = lt->tm_sec + lt->tm_min*60 + lt->tm_hour*3600;
	if (tim < t || tim >= (24*3600)) {
		/* before now or after midnight */
		printf("That must be tomorrow\nCan't you wait till then?\n");
		finish();
	}
	return (t1 + tim - t);
badform:
	printf("Bad time format\n");
	finish();
}

warn(term, sdt, now, type)
	FILE *term;
	time_t sdt, now;
	char *type;
{
	char *ts;
	register delay = sdt - now;

	fprintf(term, "\n\r\n");
	if (delay > 8)
		while (delay % 5)
			delay++;

	if (shutter)		/*specifies who is doing the shutdown*/
		fprintf(term,
	    "\007\007\t*** %sSystem shutdown message from %s@%s ***\r\n\n",
		    type, shutter, hostname);
	else			/*regular shutdown*/
		fprintf(term,
		    "\007\007\t*** %sSystem shutdown message (%s) ***\r\n\n",
		    type, hostname);

	ts = ctime(&sdt);	/*convert time to ascii string*/
	if (delay > 10 MINUTES)		/*shutdown at xx:xx*/
		fprintf(term, "System going down at %5.5s\r\n", ts+11);
	else if (delay > 95 SECONDS) {	/*shutdown in xx minutes*/
		fprintf(term, "System going down in %d minute%s\r\n",
		    (delay+30)/60, (delay+30)/60 != 1 ? "s" : "");
	} else if (delay > 0) {		/*shutdown in xx seconds*/
		fprintf(term, "System going down in %d second%s\r\n",
		    delay, delay != 1 ? "s" : "");
	} else				/*shutdown NOW*/
		fprintf(term, "System going down IMMEDIATELY\r\n");
	fprintf(term, "\r\n");
}

rwarn(sdt, now, type, host)
	time_t sdt, now;
	char *type;
	char *host;
{
	char *ts;
	register delay = sdt - now;
	char *bufp;

	if (delay > 8)
		while (delay % 5)
			delay++;

	if (shutter) {
		sprintf(buf,
	    "\007\007\t*** %sShutdown message for %s from %s@%s ***\r\n\n",
		    type, hostname, shutter, hostname);
	}
	else {
		sprintf(buf,
		    "\007\007\t*** %sShutdown message for %s ***\r\n\n",
		    type, hostname);
	}
	ts = ctime(&sdt);
	bufp = buf + strlen(buf);
	if (delay > 10 MINUTES) {
		sprintf(bufp, "%s going down at %5.5s\r\n", hostname, ts+11);
	}
	else if (delay > 95 SECONDS) {
		sprintf(bufp, "%s going down in %d minute%s\r\n",
		    hostname, (delay+30)/60, (delay+30)/60 != 1 ? "s" : "");
	} else if (delay > 0) {
		sprintf(bufp, "%s going down in %d second%s\r\n",
		    hostname, delay, delay != 1 ? "s" : "");
	} else {
		sprintf(bufp, "%s going down IMMEDIATELY\r\n", hostname);
	}
	rprintf(host, buf);
}

rprintf(host, buf)
	char *host, *buf;
{
	int err;
	
#ifdef DEBUG
		fprintf(stderr, "about to call %s\n", host);
#endif
	if (err = callrpcfast(host, WALLPROG, WALLVERS, WALLPROC_WALL,
	    xdr_path, &buf, xdr_void, NULL)) {
#ifdef DEBUG
		fprintf(stderr, "couldn't make rpc call ");
		clnt_perrno(err);
		fprintf(stderr, "\n");
#endif
	    }
}

gethostlist()
{
 	int port, s, err;
	char host[256];
	struct mountlist *ml;
	struct hostlist *hl;
	struct sockaddr_in addr;
    
	/* 
	 * check for portmapper
	 */
	get_myaddress(&addr);
	s = socket(AF_INET, SOCK_STREAM, 0);
	if (s < 0)
		return;
	if (connect(s, &addr, sizeof(addr)) < 0)
		return;
	close(s);

	gethostname(host, sizeof(host));
	ml = NULL;
	if (err = callrpc(host, MOUNTPROG, MOUNTVERS, MOUNTPROC_DUMP,
	    xdr_void, 0, xdr_mountlist, &ml)) {
		if (err != (int) RPC_PROGNOTREGISTERED) {
			fprintf(stderr, "shutdown warning: callrpc ");
			clnt_perrno(err);
			fprintf(stderr, "\n");
		}
		return;
	}
	for (; ml != NULL; ml = ml->ml_nxt) {
		for (hl = hostlist; hl != NULL; hl = hl->nxt)
			if (strcmp(ml->ml_name, hl->host) == 0)
				goto again;
		hl = (struct hostlist *)malloc(sizeof(struct hostlist));
		hl->host = ml->ml_name;
		hl->nxt = hostlist;
		hostlist = hl;
	   again:;
	}
}

nolog(sdt)
	time_t sdt;
{
	FILE *nologf;
	register char **mess;

	unlink(nologin);			/* in case linked to std file */
	if ((nologf = fopen(nologin, "w")) != NULL) {
		fprintf(nologf, nolog1, (ctime(&sdt)) + 11);
		putc('\t', nologf);
		for (mess = nolog2; *mess; mess++)
			fprintf(nologf, " %s", *mess);
		putc('\n', nologf);
		fclose(nologf);
	}
}

void
finish()
{
	signal(SIGTERM, SIG_IGN);
	unlink(nologin);
	exit(0);
}

void
do_nothing()
{

	signal(SIGALRM, do_nothing);
}

/*
 * make an entry in the shutdown log
 */

char *days[] = {
	"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
};

char *months[] = {
	"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep",
	"Oct", "Nov", "Dec"
};

log_entry(now)
	time_t now;
{
	register FILE *fp;
	register char **mess;
	struct tm *tm, *localtime();
	char shtdmsg[512];
	char *msgp = shtdmsg;

	if (*nolog2) {
		for (mess = nolog2; *mess; mess++) {
			sprintf(msgp, " %s", *mess);
			while (*msgp) msgp++;
		}
	}
	else {
		shtdmsg[0] = NULL;
	}
	(void)logerr(ELMSGT_SD, shtdmsg);
	tm = localtime(&now);
	fp = fopen(LOGFILE, "a");
	if (fp == NULL) {
		printf("Shutdown: log entry failed\n");
		return;
	}
	fseek(fp, 0L, 2);
	fprintf(fp, "%02d:%02d  %s %s %2d, %4d.  Shutdown:", tm->tm_hour,
		tm->tm_min, days[tm->tm_wday], months[tm->tm_mon],
		tm->tm_mday, tm->tm_year + 1900);
	for (mess = nolog2; *mess; mess++)
		fprintf(fp, " %s", *mess);
	if (shutter)
		fprintf(fp, " (by %s!%s)", hostname, shutter);
	fputc('\n', fp);
	fclose(fp);
}

/* 
 * Don't want to wait for usual portmapper timeout you get with
 * callrpc or clnt_call, so use rmtcall instead.  Use timeout
 * of 8 secs, based on the per try timeout of 3 secs for rmtcall 
 */
callrpcfast(host, prognum, versnum, procnum, inproc, in, outproc, out)
	char *host;
	xdrproc_t inproc, outproc;
	char *in, *out;
{
	struct sockaddr_in server_addr;
	struct hostent *hp;
	struct timeval timeout;
	int port;

	if ((hp = gethostbyname(host)) == NULL)
		return ((int) RPC_UNKNOWNHOST);
	bcopy(hp->h_addr, &server_addr.sin_addr, hp->h_length);
	server_addr.sin_family = AF_INET;
	server_addr.sin_port =  0;
	timeout.tv_sec = 8;
	timeout.tv_usec = 0;
        pmap_rmtcall(&server_addr, prognum, versnum, procnum,
            inproc, in, outproc, out, timeout, &port);
}
