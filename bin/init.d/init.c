#ifndef lint
#ifndef STANDALONE
static char *sccsid = "@(#)init.c	4.6    ULTRIX  2/14/91";
#else
static char *sccsid = "@(#)init.c	4.6    (STANDALONE)  2/14/91";
#endif
#endif
/*
    init

    Last step in the boot procedure.  Creates a process for each
    terminal port in multi-user mode.

- - - - - - -
Modification History
~~~~~~~~~~~~~~~~~~~~
01	19-Mar-86, Greg Tarsa
	- Added back opser code that got lost in a shuffle.

02	04-Dec-86, Robin Lewis
	- Added code to support the login limit key that lives
	  in /upgrade.

03	15-Dec-86, Robin Lewis
	- Encoded the string /upgrade so it would be harder to find
	  the point in the system that read from the /upgrade file.

04	16-Dec-86, Jeff Fries
	- Added code to remove the /etc/opseractive file prior
	  to starting up opser.

05	22-Jan-87, Larry Cohen
	- Increase the maximum command size from 128 to 400

06	20-Mar-87, Larry Cohen
	- Increase the maximum number of arguments to a command from 20 to 64

07	25-Jul-87, Jim Melvin
	- Added the O_APPEND flag to opens of /etc/utmp.

08	30-Jul-87, Jim Melvin
	- Removed the O_APPEND flags for /etc/utmp opens.

09	09-Jun-88, Mark Parenti
	- Changed signal handlers to void.

10	02-Jun-89, Giles Atkinson
	- Remove login limit initialisation

11	09-Oct-90, Joe Szczypek
	- Change handling of single/multiuser input flags for
          TURBOchannel ROMs.

12	12-Oct-90, John Dustin
	- Create STANDALONE version of init, without syslog support (smaller)
13	30-Nov-90, John Williams
	- Add wtmp fix for CLD. Fixes incorrect console terminal logging
*/

/************************************************************************
 *									*
 *			Copyright (c) 1987, 1988 by			*
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

/* static	char *sccsid = "@(#)init.c	1.10 (Berkeley) 3/19/86"; */

#include <signal.h>
#include <sys/types.h>
#include <utmp.h>
#include <setjmp.h>
#include <sys/reboot.h>
#include <errno.h>
#include <sys/file.h>
#include <limits.h>
#include <ttyent.h>
#include <syslog.h>
#include <sys/param.h>

#define	LINSIZ	sizeof(wtmp.ut_line)
#define	CMDSIZ	400	/* max string length for getty or window command*/
#define	TABSIZ	256
#define	ALL	p = &itab[0]; p < &itab[TABSIZ]; p++
#define	EVER	;;
#define SCPYN(a, b)	strncpy(a, b, sizeof(a))
#define SCMPN(a, b)	strncmp(a, b, sizeof(a))

char	shell[]	= "/bin/sh";
char	minus[]	= "-";
char	runc[]	= "/etc/rc";
char	utmp[]	= "/etc/utmp";
char	wtmpf[]	= "/usr/adm/wtmp";
char	ctty[]	= "/dev/console";

/*
  Opser information.

  opser is the name of the program that should be run
  when opseractivefile exists.
*/
char	opser[] = "/opr/opser";
char	opseractivefile[] = "/etc/opseractive";

struct utmp wtmp;
struct	tab
{
	char	line[LINSIZ];
	char	comn[CMDSIZ];
	char	xflag;
	int	pid;
	int	wpid;		/* window system pid for SIGHUP	*/
	char	wcmd[CMDSIZ];	/* command to start window system process */
	time_t	gettytime;
	int	gettycnt;
} itab[TABSIZ];

int	fi;
int	mergflag;
char	tty[20];
jmp_buf	sjbuf, shutpass;
time_t	time0;

void	reset();
void	idle(), merge();
char	*strcpy(), *strcat();
long	lseek();

struct	sigvec rvec = { reset, sigmask(SIGHUP), 0 };

#ifdef vax
main()
{
	register int r11;		/* passed thru from boot */
#else
main(argc, argv)
	char **argv;
{
	int i;
#endif
	int howto, oldhowto;

	time0 = time(0);
#ifdef vax
	howto = r11;
#else
	howto = RB_SINGLE;
	for (i = 1 ; i < argc ; i++) {
		if (argv[i][0] == '-') {
			char *cp;

			cp = &argv[i][1];
			while (*cp) switch (*cp++) {
			case 'a':
				howto &= ~RB_SINGLE;
				howto |= RB_ASKNAME;
				break;
			case 's':
				howto |= RB_SINGLE;
				break;
			}
		}
	}
#endif
/*
	openlog("init", LOG_CONS|LOG_ODELAY, 0);
*/
	if (geteuid()) {
		printf("NOT super-user\n");
		exit(0);
		}
	sigvec(SIGTERM, &rvec, (struct sigvec *)0);
	signal(SIGTSTP, idle);
	signal(SIGSTOP, SIG_IGN);
	signal(SIGTTIN, SIG_IGN);
	signal(SIGTTOU, SIG_IGN);
	(void) setjmp(sjbuf);
	for (EVER) {
		oldhowto = howto;
		howto = RB_SINGLE;
		if (setjmp(shutpass) == 0)
			shutdown();
		if (oldhowto & RB_SINGLE)
			single();
		if (runcom(oldhowto) == 0) 
			continue;
		merge();
		multiple();
	}
}

void	shutreset();

shutdown()
{
	register i;
	register struct tab *p;

	close(creat(utmp, 0644));
	signal(SIGHUP, SIG_IGN);
	for (ALL) {
		term(p);
		p->line[0] = 0;
	}
	signal(SIGALRM, shutreset);
	alarm(30);
	for (i = 0; i < 5; i++)
		kill(-1, SIGKILL);
	while (wait((int *)0) != -1)
		;
	alarm(0);
	shutend();
}

char shutfailm[] = "WARNING: Something is hung (wont die); ps axl advised\n";

void
shutreset()
{

	if (fork() == 0) {
		int ct = open(ctty, 1);
		write(ct, shutfailm, sizeof (shutfailm));
		sleep(5);
		exit(1);
	}
	sleep(5);
	shutend();
	longjmp(shutpass, 1);
}

shutend()
{
	register i, f;

	acct(0);
	signal(SIGALRM, SIG_DFL);
	for (i = 0; i < 10; i++)
		close(i);
	f = open(wtmpf, O_WRONLY|O_APPEND);
	if (f >= 0) {
		SCPYN(wtmp.ut_line, "~");
		SCPYN(wtmp.ut_name, "shutdown");
		SCPYN(wtmp.ut_host, "");
		time(&wtmp.ut_time);
		write(f, (char *)&wtmp, sizeof(wtmp));
		close(f);
	}
	return (1);
}

single()
{
	register pid;
	register xpid;
	extern	errno;

	do {
		pid = fork();
		if (pid == 0) {
			signal(SIGTERM, SIG_DFL);
			signal(SIGHUP, SIG_DFL);
			signal(SIGALRM, SIG_DFL);
			signal(SIGTSTP, SIG_IGN);
			(void) open(ctty, O_RDWR);
			dup2(0, 1);
			dup2(0, 2);

			/* If file exists restart opser, not shell */
			if (access(opseractivefile,0) == 0) {
				unlink(opseractivefile);
				sync();
				sync();
				execl(opser, (char *)0);
				exit(0);
			}
			else {
				execl(shell, minus, (char *)0);
				exit(0);
			}
		}
		while ((xpid = wait((int *)0)) != pid)
			if (xpid == -1 && errno == ECHILD)
				break;
	} while (xpid == -1);
}

runcom(oldhowto)
	int oldhowto;
{
	register pid, f;
	int status;

	pid = fork();
	if (pid == 0) {
		(void) open("/", O_RDONLY);
		dup2(0, 1);
		dup2(0, 2);
		if (oldhowto & RB_SINGLE)
			execl(shell, shell, runc, (char *)0);
		else
			execl(shell, shell, runc, "autoboot", (char *)0);
		exit(1);
	}
	while (wait(&status) != pid)
		;
	if (status)
		return (0);
	f = open(wtmpf, O_WRONLY|O_APPEND);
	if (f >= 0) {
		SCPYN(wtmp.ut_line, "~");
		SCPYN(wtmp.ut_name, "reboot");
		SCPYN(wtmp.ut_host, "");
		if (time0) {
			wtmp.ut_time = time0;
			time0 = 0;
		} else
			time(&wtmp.ut_time);
		write(f, (char *)&wtmp, sizeof(wtmp));
		close(f);
	}
	return (1);
}

struct	sigvec	mvec = { merge, sigmask(SIGTERM), 0 };

/*
 * Multi-user.  Listen for users leaving, SIGHUP's
 * which indicate ttys has changed, and SIGTERM's which
 * are used to shutdown the system.
 */
multiple()
{
	register struct tab *p;
	register pid;

	sigvec(SIGHUP, &mvec, (struct sigvec *)0);
	for (EVER) {
		pid = wait((int *)0);
		if (pid == -1)
			return;
		for (ALL) {
			/* must restart window system BEFORE emulator */
			if (p->wpid == pid || p->wpid == -1)
				wstart(p);
			if (p->pid == pid || p->pid == -1) {
				/* disown the window system */
				if (p->wpid)
					kill(p->wpid, SIGHUP);
				rmut(p);
				dfork(p);
			}
		}
	}
}

/*
 * Merge current contents of ttys file
 * into in-core table of configured tty lines.
 * Entered as signal handler for SIGHUP.
 */
#define	FOUND	1
#define	CHANGE	2
#define WCHANGE 4

void
merge()
{
	register struct tab *p;
	register struct ttyent *t;

	for (ALL)
		p->xflag = 0;
	setttyent();
	while (t = getttyent()) {
		if ((t->ty_status & TTY_ON) == 0)
			continue;
		for (ALL) {
			if (SCMPN(p->line, t->ty_name))
				continue;
			p->xflag |= FOUND;
			if (SCMPN(p->comn, t->ty_getty)) {
				p->xflag |= CHANGE;
				SCPYN(p->comn, t->ty_getty);
			}
			if (SCMPN(p->wcmd, t->ty_window)) {
				p->xflag |= WCHANGE|CHANGE;
				SCPYN(p->wcmd, t->ty_window);
			}
			goto contin1;
		}

		for (ALL) {
			if (p->line[0] != 0)
				continue;
			SCPYN(p->line, t->ty_name);
			p->xflag |= FOUND|CHANGE;
			SCPYN(p->comn, t->ty_getty);
			if (strcmp(t->ty_window, "") != 0) {
				p->xflag |= WCHANGE;
				SCPYN(p->wcmd, t->ty_window);
			}
			goto contin1;
		}
	contin1:
		;
	}
	endttyent();
	for (ALL) {
		if ((p->xflag&FOUND) == 0) {
			term(p);
			p->line[0] = 0;
			wterm(p);
		}
		/* window system should be started first */
		if (p->xflag&WCHANGE) {
			wterm(p);
			wstart(p);
		}
		if (p->xflag&CHANGE) {
			term(p);
			dfork(p);
		}
	}
}

term(p)
	register struct tab *p;
{

	if (p->pid != 0) {
		rmut(p);
		kill(p->pid, SIGKILL);
	}
	p->pid = 0;
	/* send SIGHUP to get rid of connections */
	if (p->wpid > 0)
		kill(p->wpid, SIGHUP);
}

#include <sys/ioctl.h>

dfork(p)
	struct tab *p;
{
	register pid;
	time_t t;
	int dowait = 0;

	time(&t);
	p->gettycnt++;
	if ((t - p->gettytime) >= 60) {
		p->gettytime = t;
		p->gettycnt = 1;
	} else if (p->gettycnt >= 5) {
		dowait = 1;
		p->gettytime = t;
		p->gettycnt = 1;
	}
	pid = fork();
	if (pid == 0) {
		signal(SIGTERM, SIG_DFL);
		signal(SIGHUP, SIG_IGN);
		if (dowait) {
#ifndef STANDALONE
			syslog(LOG_ERR, "init: '%s %s' failing, sleeping", p->comn, p->line);
			closelog();
#endif
			sleep(30);
		}
		execit(p->comn, p->line);
		exit(0);
	}
	p->pid = pid;
}

/*
 * Remove utmp entry.
 */
rmut(p)
	register struct tab *p;
{
	register f;
	int found = 0;
	char pline[LINSIZ];

	f = open(utmp, O_RDWR);
	if (f >= 0) {
                /*
                 * Look for console if logging off console head
                 */
                if (!SCMPN(p->line, ":0"))
                        SCPYN(pline, "console");
                else
                        SCPYN(pline, p->line);
                while (read(f, (char *)&wtmp, sizeof(wtmp)) == sizeof(wtmp)) {
                        if (SCMPN(wtmp.ut_line, pline))
                                continue;
                        lseek(f, -(long)sizeof(wtmp), 1);
                        SCPYN(wtmp.ut_name, "");
                        SCPYN(wtmp.ut_host, "");
                        time(&wtmp.ut_time);
                        write(f, (char *)&wtmp, sizeof(wtmp));
                        found++;
                }
                close(f);
                /*
                 * Close wtmp logging for second head
                 */
                if (!found && !SCMPN(p->line, ":1"))
                        found++;
        }
	if (found) {
		f = open(wtmpf, O_WRONLY|O_APPEND);
		if (f >= 0) {
			SCPYN(wtmp.ut_line, p->line);
			SCPYN(wtmp.ut_name, "");
			SCPYN(wtmp.ut_host, "");
			time(&wtmp.ut_time);
			write(f, (char *)&wtmp, sizeof(wtmp));
			close(f);
		}
		/*
		 * After a proper login force reset
		 * of error detection code in dfork.
		 */
		p->gettytime = 0;
	}
}

void
reset()
{

	longjmp(sjbuf, 1);
}

jmp_buf	idlebuf;

void
idlehup()
{

	longjmp(idlebuf, 1);
}

void
idle()
{
	register struct tab *p;
	register pid;

	signal(SIGHUP, idlehup);
	for (EVER) {
		if (setjmp(idlebuf))
			return;
		pid = wait((int *) 0);
		if (pid == -1) {
			sigpause(0);
			continue;
		}
		for (ALL) {
			/* if window system dies, mark it for restart */
			if (p->wpid == pid)
				p->wpid = -1;
			if (p->pid == pid) {
				rmut(p);
				p->pid = -1;
			}
		}
	}
}

wterm(p)
	register struct tab *p;
{
	if (p->wpid != 0) {
		kill(p->wpid, SIGKILL);
	}
	p->wpid = 0;
}

wstart(p)
	register struct tab *p;
{
	int npid = fork();

	if (npid == 0) {
/*
		signal(SIGTERM, SIG_DFL);
		signal(SIGHUP,  SIG_DFL);
		signal(SIGALRM, SIG_DFL);
		signal(SIGTSTP, SIG_IGN);
*/
		execit(p->wcmd, p->line);
		exit(0);
	}
	p->wpid = npid;
}

#define NARGS	64	/* must be at lease 4 */
#define ARGLEN	512	/* total size for all the argument strings */

execit(s, arg)
	char *s;
	char *arg;	/* last argument on line */
{
	char *argv[NARGS], args[ARGLEN], *envp[1];
	register char *sp = s;
	register char *ap = args;
	register char c;
	register int i, j;
	char buf[ARGLEN+40];

	/*
	 * First we have to set up the argument vector.
	 * "prog arg1 arg2" maps to exec("prog", "-", "arg1", "arg2"). 
	 */
	for (i = 1; i < NARGS - 2; i++) {
		argv[i] = ap;
		for (EVER) {
			if ((c = *sp++) == '\0' || ap >= &args[ARGLEN-1]) {
				*ap = '\0';
				goto done;
			}
			if (c == ' ') {
				*ap++ = '\0';
				while (*sp == ' ')
					sp++;
				if (*sp == '\0')
					goto done;
				break;
			}
			*ap++ = c;
		}
	}
done:
	argv[0] = argv[1];
	argv[1] = "-";
	argv[i+1] = arg;
	argv[i+2] = 0;
	envp[0] = 0;
	sigsetmask(0);
	execve(argv[0], &argv[1], envp);
	strcat(buf, "init: exec failed: cmd= ");
	strcat(buf, argv[0]);
	strcat(buf, ", ");
	strcat(buf, argv[1]);
	/* report failure of exec */
	for (j=1; j<i+1; j++) {
		strcat(buf, ", ");
		strcat(buf, argv[j+1]);
	}
#ifndef STANDALONE
	syslog(LOG_ERR, "%s: %m", buf);
	closelog();
#endif
	sleep(10);	/* prevent failures from eating machine */
}
