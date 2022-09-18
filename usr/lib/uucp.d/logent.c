#ifndef lint
static char sccsid[] = "@(#)logent.c	4.1 (decvax!larry) 7/2/90";
#endif

#include "uucp.h"
#include <sys/types.h>
#include <sys/time.h>
#include <sys/file.h>

extern	time_t	time();

/* This logfile stuff was awful -- it did output to an
 * unbuffered stream.
 *
 * This new version just open the single logfile and writes
 * the record in the stdio buffer.  Once that's done, it
 * positions itself at the end of the file (lseek), and
 * writes the buffer out.  This could mangle things but
 * it isn't likely. -- ittvax!swatt
 *
 * If the files could be opened with "guaranteed append to end",
 * the lseeks could be removed.
 * Using fseek would be slightly cleaner,
 * but would mangle things slightly more often.
 *
 * 2-11-88  Lea Gottfredsen -- changed length of buf in mlogent();
 * 	    If buf is too small when all that info is crammed into it,
 *	    the cmd will be logged in LOGFILE as being executed but
 *	    but logent will fail (Illegal Instruction).  The queue
 *	    will clog as uuxqt tries to execute to same big command
 * 	    over and over again.  Better bounds checking could be done.
 */


#ifdef ULTRIX
int Lp = NULL;
int Sp = NULL;
#else
FILE *Lp = NULL;
FILE *Sp = NULL;
#endif
static Ltried = 0;
static Stried = 0;

/*******
 *	logent(text, status)	make log entry
 *	char *text, *status;
 *
 *	return code - none
 */

logent(text, status)
char *text, *status;
{
	/* Open the log file if necessary */
	if (Lp == NULL) {
		if (!Ltried) {
			int savemask;
#ifdef ULTRIX
			Lp = open(LOGFILE, O_RDWR | O_APPEND | O_CREAT, 0644);
#else
			savemask = umask(LOGMASK);
			Lp = fopen (LOGFILE, "a");
			umask(savemask);
#endif
		}
		Ltried = 1;
#ifdef ULTRIX
		if (Lp < 0) {
			Lp = NULL;
			return;
		}
		fioclex(Lp);
#else
		if (Lp == NULL)
			return;
		fioclex(fileno(Lp));
#endif
	}

	/*  make entry in log file  */
	mlogent(Lp, status, text);
}

/***
 *	mlogent(fp, status, text)  - make a log entry
 */

mlogent(fp, status, text)
char *text, *status;
#ifdef ULTRIX
int fp;
#else
FILE *fp;
#endif
{
	static pid = 0;
	struct tm *tp;
	extern struct tm *localtime();
	extern int strlen();
	time_t clock;
#ifdef ULTRIX
	char buf[BUFSIZ];  	/* should be big enough */
#endif

	if (!pid)
		pid = getpid();
	time(&clock);
	tp = localtime(&clock);
#ifdef ULTRIX
	sprintf(buf, "%s %s (%d/%d-%d:%02d-%d) %s (%s)\n",  User, Rmtname,
		tp->tm_mon + 1, tp->tm_mday, tp->tm_hour, tp->tm_min, pid,
		status, text);
	write(fp, buf, strlen(buf));
#else
	fprintf(fp, "%s %s ", User, Rmtname);
	fprintf(fp, "(%d/%d-%d:%02d-%d) ", tp->tm_mon + 1,
		tp->tm_mday, tp->tm_hour, tp->tm_min, pid);
	fprintf(fp, "%s (%s)\n", status, text);

	/* Since it's buffered */
	lseek (fileno(fp), (long)0, 2);
	fflush (fp);
#endif
	return;
}

/***
 *	logcls()	close log file
 *
 *	return codes:  none
 */

logcls()
{
	if (Lp != NULL)
#ifdef ULTRIX
		close(Lp);
#else
		fclose(Lp);
#endif
	Lp = NULL;
	Ltried = 0;

	if (Sp != NULL)
#ifdef ULTRIX
		close(Sp);
#else
		fclose (Sp);
#endif
	Sp = NULL;
	Stried = 0;
	return;
}


/***
 *	syslog(text)	make system log entry
 *	char *text;
 *
 *	return codes - none
 */

syslog(text)
char *text;
{
	struct tm *tp;
	extern struct tm *localtime();
	time_t clock;
#ifdef ULTRIX
	char buf[256];
#endif

	if (Sp == NULL) {
		if (!Stried) {
			int savemask;
#ifdef ULTRIX
			Sp = open(SYSLOG, O_RDWR | O_APPEND | O_CREAT, 0644);
#else
			savemask = umask(LOGMASK);
			Sp = fopen(SYSLOG, "a");
			umask(savemask);
#endif
		}
		Stried = 1;
#ifdef ULTRIX
		if (Sp < 0) {
			Sp = NULL;
			return;
		}
		fioclex(Sp);
#else
		if (Sp == NULL)
			return;
		fioclex(fileno(Sp));
#endif
	}
	time(&clock);
	tp = localtime(&clock);

#ifdef ULTRIX
	sprintf(buf, "%s %s (%d/%d-%d:%02d) (%ld) %s\n",  User, Rmtname,
		tp->tm_mon + 1, tp->tm_mday, tp->tm_hour, tp->tm_min, 
		clock, text);
	write(Sp, buf, strlen(buf));
#else
	fprintf(Sp, "%s %s ", User, Rmtname);
	fprintf(Sp, "(%d/%d-%d:%02d) ", tp->tm_mon + 1,
		tp->tm_mday, tp->tm_hour, tp->tm_min);
	fprintf(Sp, "(%ld) %s\n", clock, text);

	/* Position at end and flush */
	lseek (fileno(Sp), (long)0, 2);
	fflush (Sp);
#endif
	return;
}
/*
 * Arrange to close fd on exec(II).
 * Otherwise unwanted file descriptors are inherited
 * by other programs.  And that may be a security hole.
 */
#ifdef SYSIII
#include <fcntl.h>
#endif
#ifndef	SYSIII
#include <sgtty.h>
#endif

fioclex(fd)
int fd;
{
	register int ret;

#ifdef	SYSIII
	ret = fcntl(fd, F_SETFD, 1);	/* Steve Bellovin says this does it */
#endif
#ifndef	SYSIII
	ret = ioctl(fd, FIOCLEX, STBNULL);
#endif
	if (ret)
		DEBUG(2, "CAN'T FIOCLEX %d\n", fd);
}
