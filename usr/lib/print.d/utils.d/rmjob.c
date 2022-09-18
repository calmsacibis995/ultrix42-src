#ifndef lint
static char *sccsid = "@(#)rmjob.c	4.2      ULTRIX 	9/11/90";
#endif

/************************************************************************
 *									*
 *			Copyright (c) 1988 by				*
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
 * rmjob.c - remove the specified jobs from the queue.
 */

/*
 * Modification History:
 *
 * 11-jan-90 -- thoms
 *	Fixed buffer lengths for host name
 *	Added new function host_file_match() to host name match
 */

/* SCCS history beginning
 * ***************************************************************
 *                -- Revision History --
 * ***************************************************************
 * 
 * 1.1  01/11/83 -- sccs
 * date and time created 83/11/01 20:59:35 by sccs
 * 
 * ***************************************************************
 * 
 * 1.2  25/04/88 -- thoms
 * rmjob bug fixed: startdaemon now called with printer not host
 * 
 * 
 * ***************************************************************
 *
 * 1.3  19/07/88 -- thoms
 * Added copyright notice and modification history
 *
 * ****************************************************************
 *
 * 1.4  10/11/88 -- thoms
 * Fixed missing arg to getline bug
 *
 * ****************************************************************
 *
 * 1.5  20/08/90 -- atkinson
 * Hide verification files for symbolic links
 *
 * SCCS history end
 */


#include "lp.h"

/*
 * Stuff for handling lprm specifications
 */
extern char	*user[];		/* users to process */
extern int	users;			/* # of users in user array */
extern int	requ[];			/* job number of spool entries */
extern int	requests;		/* # of spool requests */
extern char	*person;		/* name of person doing lprm */

static char	root[] = "root";
static int	all = 0;		/* eliminate all files (root only) */
static int	cur_daemon;		/* daemon's pid */
static char	current[40];		/* active control file name */

static int host_file_match();	/* match a host against command file name */
static int iscf();

/* Definitions used by host_file_match() */

#define DOT		'.'
#define HUGE		9999
#define CF_PREFIX_LEN	6
#define min(a, b)	((a)<(b)?(a):(b))

rmjob()
{
	register int i, nitems;
	int assasinated = 0;
	struct direct **files;
	
	dlog(1, "rmjob: read /etc/printcap");
	if ((i = pgetent(line, printer)) < 0)
		fatal("cannot open printer description file");
	else if (i == 0)
		fatal("unknown printer");
	if ((SD = pgetstr("sd", &bp)) == NULL)
		SD = DEFSPOOL;
	if ((LO = pgetstr("lo", &bp)) == NULL)
		LO = DEFLOCK;
	if ((LP = pgetstr("lp", &bp)) == NULL)
		LP = DEFDEVLP;
	if ((RP = pgetstr("rp", &bp)) == NULL)
		RP = DEFLP;
	RM = pgetstr("rm", &bp);
	DQ = pgetstr("dq", &bp); 
	
	/*
	 * If the format was `lprm -' and the user isn't the super-user,
	 *  then fake things to look like he said `lprm user'.
	 */
	if (users < 0) {
		if (getuid() == 0)
			all = 1;	/* all files in local queue */
		else {
			user[0] = person;
			users = 1;
		}
	}
	if (!strcmp(person, "-all")) {
		if (from == host)
			fatal("The login name \"-all\" is reserved");
		all = 1;	/* all those from 'from' */
		person = root;
	}
	
	if (chdir(SD) < 0)
		fatal("cannot chdir to spool directory");
	if ((nitems = scandir(".", &files, iscf, NULL)) < 0)
		fatal("cannot access spool directory");
	
	if (nitems) {
		if(DQ > 0){
			dlog(1, "rmjob: DQ flag set");
			dlog(1, "rmjob: %d jobs in %s", nitems,SD);
		}
		/*
		 * Check for an active printer daemon (in which case we
		 *  kill it if it is reading our file) then remove stuff
		 *  (after which we have to restart the daemon).
		 * 
		 * Don't zap it if it's DQS. We catch the signal.
		 */
		if (lockchk(LO) && chk(current)) {
			dlog(1, "rmjob: send a SIGINT to daemon");
			assasinated = kill(cur_daemon, SIGINT) == 0;
			if (!assasinated)
				fatal("cannot kill printer daemon");
		}
		/*
		 * process the files
		 * delete it if it's still in the local queue.
		 */
		for (i = 0; i < nitems; i++)
			process(files[i]->d_name);
	}
	chkremote();
	/*
	 * Restart the printer daemon if it was killed
	 */
	if (assasinated && !startdaemon(printer))
		fatal("cannot restart printer daemon\n");
	exit(0);
}

/*
 * Process a lock file: collect the pid of the active
 *  daemon and the file name of the active spool entry.
 * Return boolean indicating existence of a lock file.
 */
static
	lockchk(s)
char *s;
{
	register FILE *fp;
	register int i, n;
	
	if ((fp = fopen(s, "r")) == NULL)
		if (errno == EACCES)
			fatal("can't access lock file");
		else
			return(0);
	if (!getline(fp)) {
		(void) fclose(fp);
		return(0);		/* no daemon present */
	}
	cur_daemon = atoi(line);
	if (kill(cur_daemon, 0) < 0) {
		(void) fclose(fp);
		return(0);		/* no daemon present */
	}
	for (i = 1; (n = fread(current, sizeof(char), sizeof(current), fp)) <= 0; i++) {
		if (i > 5) {
			n = 1;
			break;
		}
		sleep(i);
	}
	current[n-1] = '\0';
	(void) fclose(fp);
	return(1);
}

/*
 * Process a control file.
 */
static
process(file)
char *file;
{
	FILE *cfp;
	
	if (!chk(file))
		return;
	dlog(1, "process: open control file %s",file);
	if ((cfp = fopen(file, "r")) == NULL)
		fatal("cannot open %s", file);
	while (getline(cfp)) {
		switch (line[0]) {
		case 'U':  /* unlink associated files */
			if (isascii(line[1]) && isupper(line[1])) {
				/* This is almost certainly a verification
				 * file acompanying a symbolic link to a
				 * data file.   Delete it silently as there
				 * is no point in drawing attention to it!
				 */
				unlink(line+1);
			} else {
				if (from != host)
					printf("%s: ", host);
				dlog(1, "process: unlink %s",line+1);
				printf(unlink(line+1) ? "cannot dequeue %s\n" :
				       "%s dequeued\n", line+1);
			}
		}
	}
	(void) fclose(cfp);
	if (from != host)
		printf("%s: ", host);
	printf(unlink(file) ? "cannot dequeue %s\n" : "%s dequeued\n", file);
}

/*
 * Do the dirty work in checking
 */
static
chk(file)
	char *file;
{
	register int *r, n;
	register char **u, *cp;
	FILE *cfp;

	if (all && (from == host || host_file_match(from, file)))
		return(1);

	/*
	 * get the owner's name from the control file.
	 */
	if ((cfp = fopen(file, "r")) == NULL)
		return(0);
	while (getline(cfp)) {
		if (line[0] == 'P')
			break;
	}
	(void) fclose(cfp);
	if (line[0] != 'P')
		return(0);

	if (users == 0 && requests == 0)
		return(!strcmp(file, current) && isowner(line+1, file));
	/*
	 * Check the request list
	 */
	for (n = 0, cp = file+3; isdigit(*cp); )
		n = n * 10 + (*cp++ - '0');
	for (r = requ; r < &requ[requests]; r++)
		if (*r == n && isowner(line+1, file))
			return(1);
	/*
	 * Check to see if it's in the user list
	 */
	for (u = user; u < &user[users]; u++)
		if (!strcmp(*u, line+1) && isowner(line+1, file))
			return(1);
	return(0);
}

/*
 * If root is removing a file on the local machine, allow it.
 * If root is removing a file from a remote machine, only allow
 * files sent from the remote machine to be removed.
 * Normal users can only remove the file from where it was sent.
 */
static
isowner(owner, file)
	char *owner, *file;
{
	if (!strcmp(person, root) && (from == host || host_file_match(from, file)))
		return(1);
	if (!strcmp(person, owner) && host_file_match(from, file))
		return(1);
	if (from != host)
		printf("%s: ", host);
	printf("%s: Permission denied\n", file);
	return(0);
}

/*
 * Check to see if we are sending files to a remote machine. If we are,
 * then try removing files on the remote machine.
 */
static
chkremote()	/* BOB BURTON - changes line - */
{
	register char *cp;
	register int i, rem;
	char buf[BUFSIZ];

	if (DQ == NULL && (*LP || RM == NULL))
		return;	/* not sending to a remote machine */

	dlog(1, "chkremote: enter");
	/*
	 * Flush stdout so the user can see what has been deleted
	 * while we wait (possibly) for the connection.
	 */
	fflush(stdout);

	sprintf(buf, "\5%s %s", RP, all ? "-all" : person);
	cp = buf;
	for (i = 0; i < users; i++) {
		cp += strlen(cp);
		*cp++ = ' ';
		strcpy(cp, user[i]);
	}
	for (i = 0; i < requests; i++) {
		cp += strlen(cp);
		(void) sprintf(cp, " %d", requ[i]);
	}
	strcat(cp, "\n");
	if(DQ > 0){  /* BOB BURTON */ 
		dlog(1, "chkremote: call getdqport");
		rem = getdqport();
	}
	else
		rem = getport(RM);
	if (rem < 0) {
		if (from != host)
			printf("%s: ", host);
		printf("connection to %s is down\n", RM);
	} else {
		i = strlen(buf);
		dlog(1, "chkremote: send delete message");
		if (write(rem, buf, i) != i)
			fatal("Lost connection");
		dlog(1, "chkremote: wait to read response message");
		while ((i = read(rem, buf, sizeof(buf))) > 0)
			(void) fwrite(buf, 1, i, stdout);
		dlog(1, "chkremote: got response message");
		(void) close(rem);
	}
}

/*
 * Return 1 if the filename begins with 'cf'
 */
static int
iscf(d)
	struct direct *d;
{
	return(d->d_name[0] == 'c' && d->d_name[1] == 'f');
}

static int
host_file_match(host, cfname)
char *host, *cfname;
{
	register char *dot_position, *cfhost;
	register int matchlen=HUGE;

	if (strlen(cfname) < CF_PREFIX_LEN) {
		return 0;
	}
	cfhost = cfname + CF_PREFIX_LEN;

	if ((dot_position=index(host, DOT)) != NULL) {
		matchlen = dot_position - host;
	}
	if ((dot_position=index(cfhost, DOT)) != NULL) {
		matchlen = min(matchlen, dot_position - cfhost);
	}
	
	return !strncmp(host, cfhost, matchlen);
}
