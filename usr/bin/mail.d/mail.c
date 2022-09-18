#ifndef lint
static  char    *sccsid = "@(#)mail.c	4.3  (ULTRIX)        4/4/91";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1984 by				*
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
 * "binmail"   /usr/binmail
 *
 *	"@(#)mail.c	4.18 (Berkeley) 9/9/83";
 *
 *	EDIT HISTORY:
 *
 *	19-Jan-89  John Haxby
 *		Having recoded safefile, the code that ensures that the file
 *		created with the right owner and mode turned out to be
 *		somewhat dodgy.  Changed MAILMODE to be the mode that the file
 *		is created with rather than the umask and removed the
 *		associated (and somewhat redundant) calls to umask().  Removed
 *		a misleading comment about calling chown() (it wasn't) and
 *		moved the setreuid() call from immediately before fdopen()
 *		(which doesn't open the file) to immediately before the
 *		safefile() which does.  Note that this UID swapping is
 *		primarily for the sake of NFS since root doesn't have
 *		sufficient privilege across NFS -- the call to safefile() to
 *		create the dead.letter file is not changed since we don't have
 *		a specific UID to set ourselves to.  Note that the code, at
 *		present, requires the spool directory to be world writeable,
 *		we should really have fallback code to try create the maildrop
 *		as root when we can't create it as the user.
 *
 *      20-Dec-89  John Haxby/Paul Sharpe
 *              Recoded safefile() to return a file-descriptor, but only when
 *              the file is (hopefully) definitely 'safe': else race
 *              conditions may allow unauthorised mailbox access.
 *
 *	15-Jun-88  John Haxby
 *		Increased size of 'truename' to prevent SIGSEGV
 *		(which, incidentally, causes endless looping through
 *	         the signal)
 *
 *	08-Jun-88  Mark Parenti
 *		Changed signal handlers to void.
 *
 *	22-Jan-88  John Haxby
 *		Added -e flag for X/OPEN.
 *
 *	27-Feb-1987  Ray Glaser
 *		Added logic to extend the wait time on stale lock
 *		files to be a function of the system load ave.
 *
 *	12-Feb-1987  Ray Glaser 
 *		Massive revision to the file locking logic for NFS.
 *
 *	15-Dec-1986  Marc Teitelbaum  - 0001
 *		Only chown spool mailfile if we created it.
 *		Security reasons.  Also, bump timeout on
 *		waiting for lock to 60 seconds.  30 seems
 *		too low.  Flock would be preferable, but no
 *		time right now and much gastric distress.
 *
 *	aps00 10/26/83	-- added check for UID of uucp otherwise, would
 *				fail if mail is comming from off system,
 *				via uucp.
 *	02-Apr-84	mah.  Fix for gethostname for queued file.  This
 *				is to reflect 4.21 (Berkeley).
 *
 */

#include <ctype.h>
#include <stdio.h>
#include <errno.h>
#include <pwd.h>
#include <utmp.h>
#include <signal.h>
#include <syslog.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <setjmp.h>
#include <sysexits.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/param.h>
#include <nlist.h>

#define SENDMAIL	"/usr/lib/sendmail"

int OVERRIDE = 0;	/* Flag set if lock file overridden */
int FIRSTSLEEP = 1;
int LOCKSLEEP = 9;	/* Basic # seconds to sleep between checks for
			 * name.lock file existance and to ck the
			 * peak load ave.
			 */
#define OLOCKSLEEPS 19 
int LOCKSLEEPS = OLOCKSLEEPS;	/* Basic # of times to sleep & wait for 
			 	 * name.lock  file to disappear of its' 
	 			 * own accord before we blow it away.
		 		 */ 

/*copylet flags */
	/*remote mail, add rmtmsg */
#define REMOTE	1
	/* zap header and trailing empty line */
#define ZAP	3
#define ORDINARY 2
#define	FORWARD	4
#define	LSIZE	256
#define	MAXLET	300	/* maximum number of letters */
#define	MAILMODE 0600	/* mode of created mail */


struct	nlist Nl[] =
{
	{ "_avenrun" },
#define	X_AVENRUN	0
	{ 0 },
};
int load = 0;
int peak = 1;	/* Peak load ave seen */
int oload = 0;
char hostname[255];

char	line[LSIZE];
char	resp[LSIZE];

struct let {
	long	adr;
	char	change;
} let[MAXLET];
int	nlet	= 0;
char	lfil[50];
long	iop, time();
char	*getenv();
char	*index();
char	lettmp[] = "/tmp/maXXXXX";
char	maildir[] = "/usr/spool/mail/";
char	mailfile[] = "/usr/spool/mail/xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx";
char	dead[] = "dead.letter";
char	*netname = "vax";
char	forwmsg[] = " forwarded\n";
FILE	*tmpf;
FILE	*malf;
char	*my_name;
char	*getlogin();
struct	passwd	*getpwuid();
int	error;
int	changed;
int	forward;
char	from[] = "From ";
long	ftell();
void	delete();
char	*ctime();
int	flgf;
int	flgp;
int	delflg = 1;
int	hseqno;
jmp_buf	sjbuf;
int	rmail;

main(argc, argv)
char **argv;
{
	register i;
	char sobuf[BUFSIZ];

	setbuf(stdout, sobuf);
	mktemp(lettmp);
	unlink(lettmp);
	my_name = getlogin();
	if (my_name == NULL || strlen(my_name) == 0) {
		struct passwd *pwent;
		pwent = getpwuid(getuid());
		if (pwent==NULL)
			my_name = "???";
		else
			my_name = pwent->pw_name;
	}
	if(setjmp(sjbuf)) done();
	for (i=SIGHUP; i<=SIGTERM; i++)
		setsig(i, delete);
	tmpf = fopen(lettmp, "w");
	if (tmpf == NULL) {
		fprintf(stderr, "mail: cannot open %s for writing\n", lettmp);
		done();
	}
	if (argv[0][0] == 'r')
		rmail++;
	if (argv[0][0] != 'r' &&	/* no favors for rmail*/
	   (argc == 1 || argv[1][0] == '-' && !any(argv[1][1], "rhd")))
		printmail(argc, argv);
	else
		bulkmail(argc, argv);
	done();
}

setsig(i, f)
int i;
void (*f)();
{
	if(signal(i, SIG_IGN)!=SIG_IGN)
		signal(i, f);
}

any(c, str)
	register int c;
	register char *str;
{

	while (*str)
		if (c == *str++)
			return(1);
	return(0);
}

printmail(argc, argv)
char **argv;
{
	int flg, i, j, print, check = 0;
	char *p, *getarg();
	struct stat statb;

	setuid(getuid());
	cat(mailfile, maildir, my_name);
	if (stat(mailfile, &statb) >= 0
	    && (statb.st_mode & S_IFMT) == S_IFDIR) {
		strcat(mailfile, "/");
		strcat(mailfile, my_name);
	}
	for (; argc>1; argv++, argc--) {
		if (argv[1][0]=='-') {
			if (argv[1][1]=='q')
				delflg = 0;
			else if (argv[1][1]=='p') {
				flgp++;
				delflg = 0;
			} else if (argv[1][1]=='f') {
				if (argc>=3) {
					strcpy(mailfile, argv[2]);
					argv++;
					argc--;
				}
			} else if (argv[1][1]=='r') {
				forward = 1;
			} else if (argv[1][1]=='h') {
				forward = 1;
			} else if (argv[1][1]=='e') {
				check = 1;
			} else {
				fprintf(stderr, "mail: unknown option %c\n", argv[1][1]);
				done();
			}
		} else
			break;
	}
	malf = fopen(mailfile, "r");
	if (malf == NULL) {
		if (check) {
			error = 1;
			done();
		} else {
			fprintf(stdout, "No mail.\n");
			return;
		}
	}
	lock(mailfile);
	copymt(malf, tmpf);
	fclose(malf);

/* PJS: Signal an error on failing to fclose. */
	if (fclose(tmpf) == EOF) {
		perror("mail");
		done();
	}
	unlock();
	if (check) {
		error = nlet == 0;
		done();
	}
	tmpf = fopen(lettmp, "r");

	changed = 0;
	print = 1;
	for (i = 0; i < nlet; ) {
		j = forward ? i : nlet - i - 1;
		if(setjmp(sjbuf)) {
			print=0;
		} else {
			if (print)
				copylet(j, stdout, ORDINARY);
			print = 1;
		}
		if (flgp) {
			i++;
			continue;
		}
		setjmp(sjbuf);
		fprintf(stdout, "? ");
		fflush(stdout);
		if (fgets(resp, LSIZE, stdin) == NULL)
			break;
		switch (resp[0]) {

		default:
			fprintf(stderr, "usage\n");
		case '?':
			print = 0;
			fprintf(stderr, "q\tquit\n");
			fprintf(stderr, "x\texit without changing mail\n");
			fprintf(stderr, "p\tprint\n");
			fprintf(stderr, "s[file]\tsave (default mbox)\n");
			fprintf(stderr, "w[file]\tsame without header\n");
			fprintf(stderr, "-\tprint previous\n");
			fprintf(stderr, "d\tdelete\n");
			fprintf(stderr, "+\tnext (no delete)\n");
			fprintf(stderr, "m user\tmail to user\n");
			fprintf(stderr, "! cmd\texecute cmd\n");
			break;

		case '+':
		case 'n':
		case '\n':
			i++;
			break;
		case 'x':
			changed = 0;
		case 'q':
			goto donep;
		case 'p':
			break;
		case '^':
		case '-':
			if (--i < 0)
				i = 0;
			break;
		case 'y':
		case 'w':
		case 's':
			flg = 0;
			if (resp[1] != '\n' && resp[1] != ' ') {
				printf("illegal\n");
				flg++;
				print = 0;
				continue;
			}
			if (resp[1] == '\n' || resp[1] == '\0') {
				p = getenv("HOME");
				if(p != 0)
					cat(resp+1, p, "/mbox");
				else
					cat(resp+1, "", "mbox");
			}
			for (p = resp+1; (p = getarg(lfil, p)) != NULL; ) {
				malf = fopen(lfil, "a");
				if (malf == NULL) {
					fprintf(stdout, "mail: cannot append to %s\n", lfil);
					flg++;
					continue;
				}
				copylet(j, malf, resp[0]=='w'? ZAP: ORDINARY);

/* PJS: Signal an error on failing to fclose. */
				if (fclose(malf) == EOF) {
					perror("mail");
					done();
				}
			}
			if (flg)
				print = 0;
			else {
				let[j].change = 'd';
				changed++;
				i++;
			}
			break;
		case 'm':
			flg = 0;
			if (resp[1] == '\n' || resp[1] == '\0') {
				i++;
				continue;
			}
			if (resp[1] != ' ') {
				printf("invalid command\n");
				flg++;
				print = 0;
				continue;
			}
			for (p = resp+1; (p = getarg(lfil, p)) != NULL; )
				if (!sendrmt(j, lfil, "/bin/mail"))	/* couldn't send it */
					flg++;
			if (flg)
				print = 0;
			else {
				let[j].change = 'd';
				changed++;
				i++;
			}
			break;
		case '!':
			system(resp+1);
			printf("!\n");
			print = 0;
			break;
		case 'd':
			let[j].change = 'd';
			changed++;
			i++;
			if (resp[1] == 'q')
				goto donep;
			break;
		}
	}
   donep:
	if (changed)
		copyback();
}

copyback()	/* copy temp or whatever back to /usr/spool/mail */
{
	register i, n, c;
	int new = 0;
	struct stat stbuf;

	signal(SIGINT, SIG_IGN);
	signal(SIGHUP, SIG_IGN);
	signal(SIGQUIT, SIG_IGN);
	lock(mailfile);
	stat(mailfile, &stbuf);
	if (stbuf.st_size != let[nlet].adr) {	/* new mail has arrived */
		malf = fopen(mailfile, "r");
		if (malf == NULL) {
			fprintf(stdout, "mail: can't re-read %s\n", mailfile);
			done();
		}
		fseek(malf, let[nlet].adr, 0);
		fclose(tmpf);
		tmpf = fopen(lettmp, "a");
		fseek(tmpf, let[nlet].adr, 0);

/* PJS: Test the putc, in case there is no more file space... */
		while ((c = fgetc(malf)) != EOF)
			if (fputc(c, tmpf) != c) {
				perror("mail");
				done();
			}
/* PJS: Test the closing of the file, in case there is no more file space. */
		if (fclose(malf) == EOF) {
			perror("mail");
			done();
		}
		if (fclose(tmpf) == EOF) {
			perror("mail");
			done();
		}

		tmpf = fopen(lettmp, "r");
		let[++nlet].adr = stbuf.st_size;
		new = 1;
	}
	malf = fopen(mailfile, "w");
	if (malf == NULL) {
		fprintf(stderr, "mail: can't rewrite %s\n", lfil);
		done();
	}
	n = 0;
	for (i = 0; i < nlet; i++)
		if (let[i].change != 'd') {
			copylet(i, malf, ORDINARY);
			n++;
		}

/* PJS: test closing of mailfile, in case there is no more file space. */
	if (fclose(malf) == EOF) {
		fprintf(stderr,"mail: can't close copied mailfile '%s'\n",
					mailfile);
		done();
	}
	if (new)
		fprintf(stdout, "new mail arrived\n");
	unlock();
}

copymt(f1, f2)	/* copy mail (f1) to temp (f2) */
FILE *f1, *f2;
{
	long nextadr;

	nlet = nextadr = 0;
	let[0].adr = 0;
	while (fgets(line, LSIZE, f1) != NULL) {
		if (isfrom(line))
			let[nlet++].adr = nextadr;
		nextadr += strlen(line);

/* PJS: Test for output, in case there is no more file space. */
		if (fputs(line, f2) < 0) {
			perror("mail");
			done();
		}
	}
	let[nlet].adr = nextadr;	/* last plus 1 */
}

copylet(n, f, type)
	FILE *f;
{
	int ch;
	long k;

	fseek(tmpf, let[n].adr, 0);
	k = let[n+1].adr - let[n].adr;
	while(k-- > 1 && (ch=fgetc(tmpf))!='\n')
		if(type!=ZAP)
/* PJS: Test for output, in case there is no more file space. */
			if (fputc(ch,f) != ch) {
				perror("mail");
				done();
			}
	if(type==REMOTE) {
		char hostname[32];
		gethostname(hostname, sizeof (hostname));
		fprintf(f, " remote from %s\n", hostname);
	} else if (type==FORWARD)
		fprintf(f, forwmsg);
	else if(type==ORDINARY)
/* PJS: Test for output, in case there is no more file space. */
		if (fputc(ch,f) != ch) {
			perror("mail");
			done();
		}

/* PJS: Test for output, in case there is no more file space. */
	while(k-->1) {
		ch=fgetc(tmpf);
		if (fputc(ch, f) != ch) {
			perror("mail");
			done();
		}
	}

/* PJS: Test for output, in case there is no more file space. */
	if(type!=ZAP || ch!= '\n') {
		ch = fgetc(tmpf);
		if (fputc(ch, f) != ch) {
			perror("mail");
			done();
		}
	}
}

isfrom(lp)
register char *lp;
{
	register char *p;

	for (p = from; *p; )
		if (*lp++ != *p++)
			return(0);
	return(1);
}

bulkmail(argc, argv)
char **argv;
{
	char truename[1024];	/* maximum permitted by sendmail */
	int first;
	register char *cp;
	int gaver = 0;
	char *newargv[1000];
	register char **ap;
	register char **vp;
	int dflag;
	int	mald;		/* 'safe' file desc returned for mail spool */

	dflag = 0;
	if (argc < 1)
		fprintf(stderr, "puke\n");
	for (vp = argv, ap = newargv + 1; (*ap = *vp++) != 0; ap++)
	{
		if (ap[0][0] == '-' && ap[0][1] == 'd')
			dflag++;
	}
	if (!dflag)
	{
		/* give it to sendmail, rah rah! */
		unlink(lettmp);
		ap = newargv+1;
		if (rmail)
			*ap-- = "-s";
		*ap = "-sendmail";
		setuid(getuid());
		execv(SENDMAIL, ap);
		perror(SENDMAIL);
		exit(EX_UNAVAILABLE);
	}

	truename[0] = 0;
	line[0] = '\0';

	/*
	 * When we fall out of this, argv[1] should be first name,
	 * argc should be number of names + 1.
	 */

	while (argc > 1 && *argv[1] == '-') {
		cp = *++argv;
		argc--;
		switch (cp[1]) {
		case 'r':
			if (argc <= 0) {
				fprintf(stderr,
				  "r flag needs more argument\n");
				usage();
				done();
			}
			gaver++;
			strcpy(truename, argv[1]);
			fgets(line, LSIZE, stdin);
			if (strcmpn("From", line, 4) == 0)
				line[0] = '\0';
			argv++;
			argc--;
			break;

		case 'h':
			if (argc <= 0) {
				fprintf(stderr,
				  "h flag needs more arguments\n");
				usage();
				done();
			}
			hseqno = atoi(argv[1]);
			argv++;
			argc--;
			break;

		case 'd':
			break;
		
		default:
			fprintf(stderr, "Unknown flag\n");
			usage();
			done();
		}
	}
	if (argc <= 1) {
		usage();
		done();
	}
	if (gaver == 0)
		strcpy(truename, my_name);
	/*
	if (argc > 4 && strcmp(argv[1], "-r") == 0) {
		strcpy(truename, argv[2]);
		argc -= 2;
		argv += 2;
		fgets(line, LSIZE, stdin);
		if (strcmpn("From", line, 4) == 0)
			line[0] = '\0';
	} else
		strcpy(truename, my_name);
	*/
	time(&iop);
	fprintf(tmpf, "%s%s %s", from, truename, ctime(&iop));
	iop = ftell(tmpf);
	flgf = 1;
	for (first = 1;; first = 0) {
		if (first && line[0] == '\0' && fgets(line, LSIZE, stdin) == NULL)
			break;
		if (!first && fgets(line, LSIZE, stdin) == NULL)
			break;
		if (line[0] == '.' && line[1] == '\n' && isatty(fileno(stdin)))
			break;
		if (isfrom(line))
/* PJS: Signal an output error. */
			if (fputs(">", tmpf) < 0) {
				perror("mail");
				return;
			}

/* PJS: Signal an output error. */
		if (fputs(line, tmpf) < 0) {
			perror("mail");
			return;
		}
		flgf = 0;
	}

/* PJS: Signal an output error. */
	if (fputs("\n", tmpf) < 0) {
		perror("mail");
		return;
	}
	nlet = 1;
	let[0].adr = 0;
	let[1].adr = ftell(tmpf);

/* PJS: Signal an output error. */
	if (fclose(tmpf) == EOF) {
		perror("mail");
		return;
	}
	if (flgf)
		return;
	tmpf = fopen(lettmp, "r");
	if (tmpf == NULL) {
		fprintf(stderr, "mail: cannot reopen %s for reading\n", lettmp);
		return;
	}
	while (--argc > 0) {
		if (!sendmail(0, *++argv, truename))
			error++;
	}
	if (error && (mald = safefile(dead,-1)) >= 0) {
		setuid(getuid());
		malf = fdopen(mald, "w");
		if (malf == NULL) {
			fprintf(stdout, "mail: cannot open %s\n", dead);
			fclose(tmpf);
			return;
		}
		copylet(0, malf, ZAP);

/* PJS: signal an error on fclosing. */
		if (fclose(malf) == EOF) {
			perror("mail");
			return;
		}
			
		fprintf(stdout, "Mail saved in %s\n", dead);
	}
	fclose(tmpf);
}

sendrmt(n, name, rcmd)
char *name;
char *rcmd;
{
	FILE *rmf, *popen();
	register char *p;
	char rsys[64], cmd[64];
	register local, pid;
	int sts;

	local = 0;
	if (index(name, '^')) {
		while (p = index(name, '^'))
			*p = '!';
		if (strncmp(name, "researc", 7)) {
			strcpy(rsys, "research");
			if (*name != '!')
				--name;
			goto skip;
		}
	}
	if (*name=='!')
		name++;
	for(p=rsys; *name!='!'; *p++ = *name++) {
		if (p - rsys > sizeof(rsys)) {
			printf("remote system name too long\n");
			return(0);
		}
		if (*name=='\0') {
			local++;
			break;
		}
	}
	*p = '\0';
	if ((!local && *name=='\0') || (local && *rsys=='\0')) {
		fprintf(stdout, "null name\n");
		return(0);
	}
skip:
	if ((pid = fork()) == -1) {
		fprintf(stderr, "mail: can't create proc for remote\n");
		return(0);
	}
	if (pid) {
		while (wait(&sts) != pid) {
			if (wait(&sts)==-1)
				return(0);
		}
		return(!sts);
	}
	setuid(getuid());
	if (local)
		sprintf(cmd, "%s %s", rcmd, rsys);
	else {
		if (index(name+1, '!'))
			sprintf(cmd, "uux - %s!rmail \\(%s\\)", rsys, name+1);
		else
			sprintf(cmd, "uux - %s!rmail %s", rsys, name+1);
	}
	if ((rmf=popen(cmd, "w")) == NULL)
		exit(1);
	copylet(n, rmf, local ? !strcmp(rcmd, "/bin/mail") ? FORWARD : ORDINARY : REMOTE);
	exit(pclose(rmf) != 0);
}

usage()
{

	error = EX_USAGE;	/* I can't believe they forgot this */
	fprintf(stderr, "Usage: mail [ -f ] people . . .\n");
}

#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
struct sockaddr_in biffaddr;

sendmail(n, name, fromaddr)
int n;
char *name;
char *fromaddr;
{
	char file[100];
	register char *p;
	register mask;
	struct passwd *pw, *getpwnam();
	struct stat statb;
	char buf[128];
	int realuser;
	int f;
	int fd;
	struct hostent *hp = NULL;
	struct servent *sp = NULL;
	int mald;	/* 'Safe' file descriptor for mail spool file. */

	for(p=name; *p!='!'&&*p!='^' &&*p!='\0'; p++)
		;
	if (*p == '!'|| *p=='^')
		return(sendrmt(n, name, 0));
	if ((pw = getpwnam(name)) == NULL) {
		fprintf(stdout, "mail: can't send to %s\n", name);
		return(0);
	}
	cat(file, maildir, name);
	if (stat(file, &statb) >= 0 && (statb.st_mode & S_IFMT) == S_IFDIR) {
		strcat(file, "/");
		strcat(file, name);
	}
	realuser = getuid();
	setreuid(0,pw->pw_uid);
	if ((mald = safefile(file, pw->pw_uid)) < 0) {
		setuid (0);
		setreuid (realuser, 0);
		return(0);
	}
	lock(file);
	malf = fdopen(mald, "A");
	if (malf == NULL) {
		unlock();
		fprintf(stdout, "mail: cannot append to %s\n", file);
		setuid(0);
		setreuid(realuser,0);
		return(0);
	}

	/* Notify interersted parties via biff */
	{
		char hostbuf[256];
		gethostname(hostbuf, sizeof (hostbuf));
		hp = gethostbyname(hostbuf);
		sp = getservbyname("biff", "udp");
		if (hp && sp) {
			f = socket(AF_INET, SOCK_DGRAM, 0, 0);
			sprintf(buf, "%s@%d\n", name, ftell(malf)); 
		}
	}
	copylet(n, malf, ORDINARY);

/* PJS: Signal an error on failing to fclose. */
	if (fclose(malf) == EOF) {
		perror("mail");
		return(0);
	}
	setreuid(0,pw->pw_uid);
	if (hp && sp) {
		biffaddr.sin_family = hp->h_addrtype;
		bcopy(hp->h_addr, &biffaddr.sin_addr, hp->h_length);
		biffaddr.sin_port = sp->s_port;
		sendto(f, buf, strlen(buf)+1, 0, &biffaddr, sizeof (biffaddr));
		close(f);
	}
	unlock();
	setuid(0);
	setreuid(realuser,0);
	return(1);
}

void
delete(i)
{
	setsig(i, delete);
	fprintf(stderr, "\n");
	if(delflg)
		longjmp(sjbuf, 1);
	done();
}

/*
 * Lock the specified mail file by creating the file name.lock
 * (where `name' is a user login name string).
 * We must, of course, be careful to unlink the lock file by a call
 * to unlock before we stop.  The algorithm used here is to see if
 * the lock exists, and if it does we wait LOCKSLEEP time and look
 * again. If we still see the lock file, we will wait again. This loop
 * runs for approximately:
 *
 *	  seconds = (LOCKSLEEP + peak ) * (LOCKSLEEPS + peak)
 *
 * time and then we forcefully unlink the lock file.  "peak" is the
 * peak load average seen in getla(); The minimum time on an unloaded
 * system (loadave =< 1) would be about  200 seconds with LOCKSLEEP = 9
 * and OLOCKSLEEPS = 19. At loadave = 8, it would work out about -
 *
 * 9 + 8 = 17    seconds between sleeps
 * 19 + 8 = 27	 make 27 sleeps
 * 27 x 17 = 459 seconds (7 minutes 39 seconds)
 *
 * However:
 *
 * If the mail file is actually changing size, we will wait until we see
 * no change for the above amount of time in the users' mailbox
 * before we  forcefully  remove the existing lock file  (if any) and
 * proceed.
 *
 * If we MUST forcefully remove a lock file, a syslog entry is made to
 * back trace munged mail problems.
 */

char	*maillock	= ".lock";		/* Lock suffix for mailname */
char	*lockname	= "/usr/spool/mail/tmXXXXXX";
char	locktmp[30];				/* Usable lock temporary */
char	curlock[50];				/* Last used name of lock */
int	locked;					/* To note that we locked it */

lock(file)
char *file;
{
	register int f;
	struct stat sbuf;
	struct stat original;
	long curtime;
	int statfailed;

	register int n;
	off_t osize;
	off_t nsize;
	struct stat mbox;

	if (locked || flgf)
		return(0);

	strcpy(curlock, file);
	strcat(curlock, maillock);
	strcpy(locktmp, lockname);
	mktemp(locktmp);
	unlink(locktmp);
	statfailed = 0;

top:
/*
 */
	/* Get the original size of the users' mail box
	 * and save it to check for changes to the mail box whilst
	 * we are sleeping on a lock file (if any).
	 */
	if (stat(file,&mbox) < 0)
		osize = 0;
	else
		osize = mbox.st_size;

	/* Get original mod time of possible lock file to test
	 * for creation of new lock file while we were sleeping.
	 */
	if (stat(curlock, &original) < 0) {
		original.st_ctime = 0;
	}

	/* Make number of sleep cycles.
	 */
	LOCKSLEEPS = OLOCKSLEEPS + getla();

	for (n=0; n < LOCKSLEEPS; n++) {

		f = lock1(locktmp, curlock);
		if (f == 0) {
			if (OVERRIDE) {
				/*
	 			 * At this point, we would have waited 
				 * a long time for the lock file to go
				 * away. If it didn't, log a complaint.
	 			 */
				 openlog("/bin/mail",1);
				 syslog(LOG_ERR,"Overriding mail lock file for  %s  (peak load ave = %d)",file,peak);
				 closelog();
			}
			/* We have locked the file, return to caller.
			 */
			locked = 1;
			OVERRIDE = 0;
			return(0);
		}
		if (stat(curlock, &sbuf) < 0) {
			if (statfailed++ > 5)
				return(-1);

			sleep(LOCKSLEEP+peak);

			/* Take a new reading on the load.
		 	 */
			getla();
			continue;
		}
		statfailed = 0;

		/* A lock file exists. Sleep for awhile and look again.
		 */
		if (FIRSTSLEEP) {
			FIRSTSLEEP = 0;
			openlog("/bin/mail",1);
			syslog(LOG_ERR,"Waiting on mail lock file  %s  (peak load ave = %d)",curlock,peak);
			closelog();
		}
		sleep(LOCKSLEEP + peak);

		/* Take a new reading on the load.
		 */
		getla();

		/* While we were sleeping, the mail box may have grown,
		 * shrunk, -or- disappeared....
		 * Get a new size to compare to the original.
		 */
		if (stat(file,&mbox) < 0) {
			osize = nsize = 0;
		}
		else
			nsize = mbox.st_size;

		if ((nsize != osize) ||
			(original.st_ctime != sbuf.st_ctime)) {

			/* If the users' mail box changed size, reset
			 * to new size and restart the entire wait
			 * cycle over. ie. We have to see the mail box
			 * not change size for the required amount of
			 * time if there was a lock file present
			 * in the first place before we think about
			 * removing the existing lock file.
			 */
			original.st_ctime = sbuf.st_ctime;
			n = 0;
			osize = nsize;
			LOCKSLEEPS = OLOCKSLEEPS + peak;
		}
		continue;
	}
	/* If we get here, the mail lock file (name.lock) has existed
	 * for the required amount of time &  we didn't see the
	 * users' mail box change size. -or- If we saw it change size,
	 * we reset our counters and rewound the clock for another
	 * time and then waited the respectable interval before
	 * resorting to removing the lock file by force.
	 *
	 * After our last sleep, make one final attempt to gracefully
	 * create a lock file.
	 */
	f = lock1(locktmp, curlock);
	if (f == 0) {
		/*
		 * We got lucky and were able to create the lock file.
		 */
		locked = 1;
		return(0);
	}	
	/* Make one last ck to see if a new lock file has
	 * been made whilst we were asleep.
	 */
	stat(curlock, &sbuf);
	if (original.st_ctime != sbuf.st_ctime) {
		OVERRIDE = 0;
		goto top;
	}
	/* We have to remove the lock file by force.
	 */
	f = unlink(curlock);

	if (f < 0) {
		/* If we can't remove the lock file, send the mail
		 * back and record our complaint.
		 */
		if (errno != ENOENT) {
			openlog("/bin/mail",1);
			syslog(LOG_ERR,"Cannot override mail lock file  %s",curlock);
			closelog();
			error = EX_UNAVAILABLE;
			done();
		}
	}
	OVERRIDE = 1;
	goto top;	/* Rewind */

}

/*
 * Remove the mail lock, and note that we no longer
 * have it locked.
 */

unlock()
{

	unlink(curlock);
	locked = 0;
}

/*
 * Attempt to set the lock by creating the temporary file,
 * then doing a link/unlink.  If it fails, return -1 else 0
 */

lock1(tempfile, name)
	char tempfile[], name[];
{
	register int fd;

	fd = creat(tempfile, 0);
	if (fd < 0)
		return(-1);
	close(fd);
	if (link(tempfile, name) < 0) {
		unlink(tempfile);
		return(-1);
	}
	unlink(tempfile);
	return(0);
}

done()
{
	if(locked)
		unlock();
	unlink(lettmp);
	unlink(locktmp);
	exit(error);
}

cat(to, from1, from2)
char *to, *from1, *from2;
{
	int i, j;

	j = 0;
	for (i=0; from1[i]; i++)
		to[j++] = from1[i];
	for (i=0; from2[i]; i++)
		to[j++] = from2[i];
	to[j] = 0;
}

char *getarg(s, p)	/* copy p... into s, update p */
register char *s, *p;
{
	while (*p == ' ' || *p == '\t')
		p++;
	if (*p == '\n' || *p == '\0')
		return(NULL);
	while (*p != ' ' && *p != '\t' && *p != '\n' && *p != '\0')
		*s++ = *p++;
	*s = '\0';
	return(p);
}

/* JCH/PJS:
 * safefile() now returns a file descriptor, so as to try to avoid race
 * conditions: the file may acquire links (h/s) between checking for these
 * and actually creating the file. 
 */
int
safefile(f, uid)
char	*f;		/* File name to be opened. */
int	uid;		/* To check ownership of the file. */
{
int		fd;
struct stat	statb;

/* We now try to create the file for writing to, but ensuring that this
 * will only succeed if the filename does not exist at the time.
 */
    if ((fd = open(f, O_WRONLY | O_CREAT | O_EXCL, MAILMODE)) >= 0)
	return(fd);
    if (errno != EEXIST) {	/* Something 'fatal' happened! */
	fprintf(stderr,"mail: Creating %s",f);
	perror(" -");
	return(-1);
    }

/* The file name must already exist, as the 'open' with O_EXCL failed.
 * So, we can try to open it simply for O_WRONLY.
 * The file is supposed to exist at this stage, so if the open fails
 * then we may only return a FAILed status.
 */
    if ((fd = open(f, O_WRONLY, MAILMODE)) < 0) {
	fprintf(stderr,"mail: Opening %s",f);
	perror(" -");
	return(-1);
    }

/* As a last precaution, we may check the filename for certain states:
 *  - the link count must not be more than one as it is a (potential)
 *    security hole.
 *  - the file has not become a symbolic link.
 *  - the file is owned by the 'uid' argument.
 * It is possible for the file to have been removed between the open() 
 * and the lstat(). In this case, we don't need to test the above states:
 * closing the file will recreate the file.
 */
    if (lstat(f, &statb) < 0) {
	if (errno != ENOENT) {
	    fprintf(stderr,"mail: Stating %s",f);
	    perror(" -");
	    return(-1);
	}
    }
    else {
	if (statb.st_nlink != 1 || (statb.st_mode & S_IFMT) == S_IFLNK) {
	    fprintf(stderr,"mail: %s has more than one link or is a symbolic link\n",f);
	    return(-1);
	}
	if (uid > 0 && statb.st_uid != uid) {
	    fprintf(stderr,"mail: %s is not owned by you\n",f);
	    return(-1);
	} 
    }

/* We have an open file that does not appear to:
 *  - have multiple hard links,
 *  - be a symbolic link,
 *  - be owned by someone else other than the expected owner.
 * Therefore, we may safely return the file descriptor.
 */
    return (fd);
}

/*
**  GETLA -- get the current load average
**
**	This code stolen from la.c.
**
**	Parameters:
**		none.
**
**	Returns:
**		The current load average as an integer.
**
**	Side Effects:
**		none.
*/


getla()
{
	static int kmem = -1;
	double avenrun[3];
	extern off_t lseek();

	if (kmem < 0)
	{
		kmem = open("/dev/kmem", 0, 0);
		if (kmem < 0)
			exit (-1);
		(void) ioctl(kmem, (int) FIOCLEX, (char *) 0);
		nlist("/vmunix", Nl);
		if (Nl[0].n_type == 0)
			return (9);
	}
	if (lseek(kmem, (off_t) Nl[X_AVENRUN].n_value, 0) == -1 ||
	    read(kmem, (char *) avenrun, sizeof(avenrun)) < sizeof(avenrun))
	{
		return(9);
	}
	load = (int) (avenrun[0] + 0.5);
	if (load == 0)
		load = 1;

	if (load != oload) {
		oload = load;
		if (oload > peak)
			peak = oload;
		}
	return(peak);
}
