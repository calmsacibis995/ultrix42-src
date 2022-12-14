#ifndef lint
static char	*sccsid = "@(#)at.c	4.1	(ULTRIX)	7/17/90";
#endif
/*
 * at time mon day
 * at time wday
 * at time wday 'week'
 * at -r job...
 * at -l [job...]
 *
 * at.c
 *
 * Modification history
 *
 * 05 Wed Jun 06 1990, Grant W. Sullivan
 *	inserted "extern" in front of declaration on "environ", to match
 *	  execl(3) and for RISC linkages
 *
 * 04 Tue Sep 13 13:18:33 EDT 1988, Gary A. Gaudet
 *	-r checks for valid job id and ownership
 *	-l checks for valid job id
 *
 * 03 Mon Aug 15 12:08:27 EDT 1988, Gary A. Gaudet
 *	put suid root back in.
 *
 * 02	09-Jun-88 - Mark Parenti
 *	Changed signal handlers to void.
 *
 * 01	17-Feb-88 - Gary A. Gaudet
 *	X/OPEN
 *	- Added -l and -r options
 *	- Added at.allow and at.deny
 *
 */
#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include <pwd.h>
#include <strings.h>
#include <signal.h>
#include <sys/file.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/dir.h>
#include <sys/stat.h>

#define HOUR 100
#define HALFDAY	(12*HOUR)
#define DAY	(24*HOUR)
#define FALSE	0
#define TRUE	1
#define THISDAY		"/usr/var/spool/at"
#define AT_ALLOW	"/usr/var/spool/at/at.allow"
#define AT_DENY		"/usr/var/spool/at/at.deny"
#define USAGE		"USAGE:\t%s time [date] [file]\n\t%s -l [job...]\n\t%s -r job...\n"

char *days[] = {
	"sunday",
	"monday",
	"tuesday",
	"wednesday",
	"thursday",
	"friday",
	"saturday",
};

struct monstr {
	char *mname; 
	int mlen;
} months[] = {
	{ "january", 31 },
	{ "february", 28 },
	{ "march", 31 },
	{ "april", 30 },
	{ "may", 31 },
	{ "june", 30 },
	{ "july", 31 },
	{ "august", 31 },
	{ "september", 30 },
	{ "october", 31 },
	{ "november", 30 },
	{ "december", 31 },
	{ 0, 0 },
};

char	fname[100];
int	utime;  /* requested time in grains */
int	now;	/* when is it */
int	uday; /* day of year to be done */
int	uyear; /* year */
int	today; /* day of year today */
FILE	*file;
FILE	*ifile;
extern	char	**environ;
char	*prefix();
char    *getenv();
int	getopt();
FILE	*popen();

extern char *sys_errlist[];	/* list of error messages...see perror(3) */

main(argc, argv)
char **argv;
{
	extern int optind;	/* used by getopt(3) */
	extern char *optarg;	/* used by getopt(3) */
	int	lflag;		/* user wants to list job(s) */
	int	rflag;		/* user wants to remove job(s) */
	int	errflag;	/* user wants to try again :-) */
	int	uid;		/* to check uid to job's uid */
	DIR	*dirp;		/* for reading the at spool directory */
	struct direct *dp;	/* for holding at spool entry */
	struct stat sbuf;	/* stat(2) buffer */
	extern void onintr();
	register c;
	char pwbuf[100];
	FILE *pwfil;
	int larg;
	char *tmp;

	rflag = lflag = errflag = 0;
/*
 *	straight from getopt(3) manual page
 */
	while ((c = getopt (argc, argv, "lr")) != EOF)
		switch (c) {
		case 'l':	/* user wants to list job(s) */
			if (rflag) {
				errflag++;
			} else {
				lflag++;
			}
			break;
		case 'r':	/* user wants to remove job(s) */
			if (lflag) {
				errflag++;
			} else {
				rflag++;
			}
			break;
		case '?':
			errflag++;
		}
	if (errflag) {
		(void) fprintf (stderr, USAGE, argv[0], argv[0], argv[0]);
		exit (-1);
	}
	if (lflag) {
		if (chdir (THISDAY)) {
			(void) fprintf(stderr, "%s: Cannot change directory to %s\n", argv[0], THISDAY, errno);
			exit (errno);
		} else {
			if (optind >= argc) {
				dirp = opendir(THISDAY);
				for (dp = readdir(dirp); dp != NULL; dp = readdir(dirp)) {
					if (valid_job_id (dp->d_name)) {
						stat (dp->d_name, &sbuf);
						if (getuid() == sbuf.st_uid) {
							(void) fprintf(stderr, "%s\n", dp->d_name);
						}
					}
				}
			} else {
				for ( ; optind < argc; optind++) {
					if (valid_job_id (argv[optind])) {
						if (access (argv[optind], F_OK)) {
							errflag++;
							if (errno == ENOENT) {
								(void) fprintf(stderr, "%s: %s: No such job\n", argv[0],
										argv[optind]);
							} else {
								(void) fprintf(stderr, "%s: %s: Cannot access\n", argv[0],
										argv[optind]);
							}
						} else {
							(void) fprintf(stderr, "%s\n", argv[optind]);
						}
					} else {
						errflag++;
						(void) fprintf(stderr, "%s: %s: Invalid job id\n", argv[0], argv[optind]);
					}
				}
			}
		}
	} else if (rflag) {
		if (optind < argc) {
/*
 *			cd to at spool directory
 */
			if (chdir (THISDAY)) {
				(void) fprintf(stderr, "%s: %s: Cannot cd to %s\n", argv[0], THISDAY, sys_errlist[errno]);
				exit (errno);
			} else {
/*
 *				remove loop for each job
 */
				for ( ; optind < argc; optind++) {
/*
 *					check for valid job id
 */
					if (valid_job_id (argv[optind])) {
/*
 *						get job owner
 */
						if (stat (argv[optind], &sbuf)) {
							errflag++;
							if (errno == ENOENT) {
								(void) fprintf(stderr, "%s: %s: No such job\n",
										argv[0], argv[optind]);
							} else {
								(void) fprintf(stderr, "%s: %s: Cannot stat: %s\n",
										argv[0], argv[optind],
										sys_errlist[errno]);
							}
						} else {
/*
 *							user must be root or job owner
 */
							if (!(uid = getuid()) || uid == sbuf.st_uid) {
/*
 *								remove job
 */
								if (unlink (argv[optind])) {
									errflag++;
									(void) fprintf(stderr,
											"%s: %s: Cannot unlink: %s\n",
											argv[0], argv[optind],
											sys_errlist[errno]);
								}
							} else {
								errflag++;
								(void) fprintf(stderr, "%s: %s: Not owner\n", argv[0],
										argv[optind]);
							}
						}
					} else {
						errflag++;
						(void) fprintf(stderr, "%s: %s: Invalid job id\n", argv[0], argv[optind]);
					}
				}
			}
		} else {
			(void) fprintf (stderr, USAGE, argv[0], argv[0], argv[0]);
			exit (-2);
		}
	} else if (getuid () == 0 || allow() || (access (AT_ALLOW, F_OK) && errno == ENOENT && !deny())) {
		/* argv[1] is the user's time: e.g.,  3AM */
		/* argv[2] is a month name or day of week */
		/* argv[3] is day of month or 'week' */
		/* another argument might be an input file */
		if (argc < 2) {
			(void) fprintf(stderr, "at: arg count\n");
			exit(-3);
		}
		makeutime(argv[1]);
		larg = makeuday(argc,argv)+1;
		if (uday==today && larg<=2 && utime<=now)
			uday++;
		if (uday < today)
			uyear++;
		c = uyear%4==0? 366: 365;
		if (uday >= c) {
			uday -= c;
			uyear++;
		}
		filename(THISDAY, uyear, uday, utime);
		/* Create file, then change UIDS */
		close(creat(fname,0644));
		chown(fname,getuid(),getgid());
		setuid(getuid());
		ifile = stdin;
		if (argc > larg)
			ifile = fopen(argv[larg], "r");
		if (ifile == NULL) {
			perror(argv[larg]);
			exit(errno);
		}
		if (signal(SIGINT, SIG_IGN) != SIG_IGN)
			signal(SIGINT, onintr);
		file = fopen(fname, "w");
		if (file == NULL) {
			(void) fprintf(stderr, "at: cannot open memo file\n");
			exit(errno);
		}
		if ((pwfil = popen("pwd", "r")) == NULL) {
			(void) fprintf(stderr, "at: can't execute pwd\n");
			exit(errno);
		}
		fgets(pwbuf, 100, pwfil);
		pclose(pwfil);
		(void) fprintf(file, "cd %s", pwbuf);
		c = umask(0);
		umask(c);
		(void) fprintf(file, "umask %.1o\n", c);
		if (environ) {
			char **ep = environ;
			while(*ep)
			{
				char *cp;
				for (tmp = *ep, cp = "TERMCAP"; *tmp==*cp; tmp++,cp++);
				if (*cp == 0 && *tmp== '=') {
					ep++;
					continue;
				}
				for(tmp = *ep ; *tmp != '=' ; tmp++) putc(*tmp,file);
				putc('=', file);
				putc('\'', file);
				for (tmp++; *tmp; tmp++) {
					if (*tmp == '\'')
						fputs("'\\''", file);
					else
						putc(*tmp, file);
				}
				putc('\'', file);
				(void) fprintf(file, "\nexport ");
				for(tmp = *ep ; *tmp != '=' ; tmp++) putc(*tmp,file);
				putc('\n',file);
				ep++;
			}
		}
		/*
		 * see if the SHELL variable in the current enviroment is /bin/csh
		 * and in that case, use the csh as the shell
		 */
		tmp = getenv("SHELL");
		if (strcmp(tmp+strlen(tmp)-3, "csh") == 0)
			(void) fprintf(file, "%s %s\n", tmp, "<< 'xxFUNNYxx'");
		while((c = getc(ifile)) != EOF) {
			putc(c, file);
		}
		if (strcmp(tmp+strlen(tmp)-3, "csh") == 0)
			(void) fprintf(file, "%s\n", "xxFUNNYxx");

		(void) fprintf(stderr, "%s\n", strrchr (fname, '/') + 1);	/* print the job number */

		exit(0);
	} else {
		(void) fprintf(stderr, "%s: Privilege denied\n", argv[0]);
		exit (-4);
	}
	return (errflag);	/* errflag is 0 is no errors */
}

/*
 * ALLOW - is the user allowed to use at
 */
allow()
{
	struct passwd *user;
	FILE *fd;
	char bufr[9];

	if ((fd = fopen (AT_ALLOW, "r")) != NULL) {
		user = getpwuid (getuid ());
		while (fscanf (fd, "%8s", bufr) != EOF) {
			if (strncmp (bufr, user->pw_name, 8) == 0) {
				fclose (fd);
				return (1);
			}
		}
		fclose (fd);
	}
	return (0);
}

/*
 * DENY - is the user denied use of at
 */
deny()
{
	struct passwd *user;
	FILE *fd;
	char bufr[9];

	if ((fd = fopen (AT_DENY, "r")) != NULL) {
		user = getpwuid (getuid ());
		while (fscanf (fd, "%8s", bufr) != EOF) {
			if (strncmp (bufr, user->pw_name, 8) == 0) {
				fclose (fd);
				return (1);
			}
		}
		fclose (fd);
		return (0);
	}
	return (1);
}

makeutime(pp)
char *pp; 
{
	register val;
	register char *p;

	/* p points to a user time */
	p = pp;
	val = 0;
	while(isdigit(*p)) {
		val = val*10+(*p++ -'0');
	}
	if (p-pp < 3)
		val *= HOUR;

	for (;;) {
		switch(*p) {

		case ':':
			++p;
			if (isdigit(*p)) {
				if (isdigit(p[1])) {
					val +=(10* *p + p[1] - 11*'0');
					p += 2;
					continue;
				}
			}
			(void) fprintf(stderr, "at: bad time format:\n");
			exit(-5);

		case 'A':
		case 'a':
			if (val >= HALFDAY+HOUR)
				val = DAY+1;  /* illegal */
			if (val >= HALFDAY && val <(HALFDAY+HOUR))
				val -= HALFDAY;
			break;

		case 'P':
		case 'p':
			if (val >= HALFDAY+HOUR)
				val = DAY+1;  /* illegal */
			if (val < HALFDAY)
				val += HALFDAY;
			break;

		case 'n':
		case 'N':
			val = HALFDAY;
			break;

		case 'M':
		case 'm':
			val = 0;
			break;


		case '\0':
		case ' ':
			/* 24 hour time */
			if (val == DAY)
				val -= DAY;
			break;

		default:
			(void) fprintf(stderr, "at: bad time format\n");
			exit(-6);

		}
		break;
	}
	if (val < 0 || val >= DAY) {
		(void) fprintf(stderr, "at: time out of range\n");
		exit(-7);
	}
	if (val%HOUR >= 60) {
		(void) fprintf(stderr, "at: illegal minute field\n");
		exit(-8);
	}
	utime = val;
}


makeuday(argc,argv)
char **argv;
{
	/* the presumption is that argv[2], argv[3] are either
	   month day OR weekday [week].  Returns either 2 or 3 as last
	   argument used */
	/* first of all, what's today */
	long tm;
	int found = -1;
	char **ps;
	struct tm *detail, *localtime();
	struct monstr *pt;

	time(&tm);
	detail = localtime(&tm);
	uday = today = detail->tm_yday;
	uyear = detail->tm_year;
	now = detail->tm_hour*100+detail->tm_min;
	if (argc<=2)
		return(1);
	/* is the next argument a month name ? */
	for (pt=months; pt->mname; pt++) {
		if (prefix(argv[2], pt->mname)) {
			if (found<0)
				found = pt-months;
			else {
				(void) fprintf(stderr, "at: ambiguous month\n");
				exit(-9);
			}
		}
	}
	if (found>=0) {
		if (argc<=3)
			return(2);
		uday = atoi(argv[3]) - 1;
		if (uday<0) {
			(void) fprintf(stderr, "at: illegal day\n");
			exit(-10);
		}
		while(--found>=0)
			uday += months[found].mlen;
		if (detail->tm_year%4==0 && uday>59)
			uday += 1;
		return(3);
	}
	/* not a month, try day of week */
	found = -1;
	for (ps=days; ps<days+7; ps++) {
		if (prefix(argv[2], *ps)) {
			if (found<0)
				found = ps-days;
			else {
				(void) fprintf(stderr, "at: ambiguous day of week\n");
				exit(-11);
			}
		}
	}
	if (found<0)
		return(1);
	/* find next day of this sort */
	uday = found - detail->tm_wday;
	if (uday<=0)
		uday += 7;
	uday += today;
	if (argc>3 && strcmp("week", argv[3])==0) {
		uday += 7;
		return(3);
	}
	return(2);
}

char *
prefix(begin, full)
char *begin, *full;
{
	int c;
	while (c = *begin++) {
		if (isupper(c))
			c = tolower(c);
		if (*full != c)
			return(0);
		else
			full++;
	}
	return(full);
}

filename(dir, y, d, t)
char *dir;
{
	register i;

	for (i=0; ; i += 53) {
		(void) sprintf(fname, "%s/%02d.%03d.%04d.%02d", dir, y, d, t,
		   (getpid()+i)%100);
		if (access(fname, 0) == -1)
			return;
	}
}

void
onintr()
{
	unlink(fname);
	exit(-12);
}

/*
 * VALID_JOB_ID - returns true if the string is "%02d.%03d.%04d.%02d"
 */
int
valid_job_id (s)
char *s;
{
	if (isdigit (*s) && isdigit (*++s) && *++s == '.' && isdigit (*++s) && isdigit (*++s) && isdigit (*++s)
			&& *++s == '.' && isdigit (*++s) && isdigit (*++s) && isdigit (*++s) && isdigit (*++s)
			&& *++s == '.' && isdigit (*++s) && isdigit (*++s) && *++s == NULL) {
		return (TRUE);
	}
	return (FALSE);
}
