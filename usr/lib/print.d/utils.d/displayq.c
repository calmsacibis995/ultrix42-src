#ifndef lint
static char *sccsid = "@(#)displayq.c	4.1	ULTRIX	7/2/90";
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
 * displayq.c -- Routines to display the state of the queue.
 */

/* SCCS history beginning
 * ***************************************************************
 *                -- Revision History --
 * ***************************************************************
 * 
 * 1.1  01/11/83 -- sccs
 * date and time created 83/11/01 20:54:08 by sccs
 * 
 * ***************************************************************
 * 
 * 1.2  17/05/88 -- thoms
 * Now recognises remote q by presence of :rm: capability.
 * 
 * 
 * ***************************************************************
 *
 * 1.3  15/07/88 -- thoms
 * Added copyright notice, modification history, improved comments
 *
 * SCCS history end
 */


#include "lp.h"

#define JOBCOL	40		/* column for job # in -l format */
#define OWNCOL	7		/* start of Owner column in normal */
#define SIZCOL	62		/* start of Size column in normal */

/*
 * Stuff for handling job specifications
 */
extern char	*user[];	/* users to process */
extern int	users;		/* # of users in user array */
extern int	requ[];		/* job number of spool entries */
extern int	requests;	/* # of spool requests */

static int	lflag;		/* long output option */
static char	current[40];	/* current file being printed */
static int	garbage;	/* # of garbage cf files */
static int	rank;		/* order to be printed (-1=none, 0=active) */
static long	totsize;	/* total print job size in bytes */
static int	first;		/* first file in ``files'' column? */
static int	col;		/* column on screen */
static int	sendtorem;	/* are we sending to a remote? */
static char	file[132];	/* print file name */

static char	*head0 = "Rank   Owner      Job  Files";
static char	*head1 = "Total Size\n";

/*
 * displayq -- display the current state of the queue.
 *
 *	Format = 1 for long format output.
 */
displayq(format)
	int format;
{
	register struct queue *q;
	register int i, nitems, fd;
	struct queue **queue;
	struct stat statb;
	FILE *fp;

	lflag = format;
	totsize = 0;
	rank = -1;

	dlog(1, "displayq: read /etc/printcap");
	if ((i = pgetent(line, printer)) < 0)
		fatal("cannot open printer description file");
	else if (i == 0)
		fatal("unknown printer");
	if ((LP = pgetstr("lp", &bp)) == NULL)
		LP = DEFDEVLP;
	if ((RP = pgetstr("rp", &bp)) == NULL)
		RP = DEFLP;
	if ((SD = pgetstr("sd", &bp)) == NULL)
		SD = DEFSPOOL;
	if ((LO = pgetstr("lo", &bp)) == NULL)
		LO = DEFLOCK;
	if ((ST = pgetstr("st", &bp)) == NULL)
		ST = DEFSTAT;
	RM = pgetstr("rm", &bp);
	DQ = pgetstr("dq", &bp);

	/*
	 * If there is no local printer, then print the queue on
	 * the remote machine and then what's in the queue here.
	 * Note that a file in transit may not show up in either queue.
	 */
	if (RM || DQ) {
		register char *cp;
		char c;

		dlog(1, "displayq: enter remote either RM or DQ flag set");
		sendtorem++;
		(void) sprintf(line, "%c%s", format + '\3', RP);
		cp = line;
		for (i = 0; i < requests; i++) {
			cp += strlen(cp);
			(void) sprintf(cp, " %d", requ[i]);
		}
		for (i = 0; i < users; i++) {
			cp += strlen(cp);
			*cp++ = ' ';
			strcpy(cp, user[i]);
		}
		strcat(line, "\n");
		if (DQ > 0) {
			dlog(1, "displayq: call getdqport");
			fd = getdqport();
		}
		else
			fd = getport(RM);
		if (fd < 0) {
			if (from != host)
				printf("%s: ", host);
			printf("connection to %s is down\n", RM);
		} else {
			dlog(1, "displayq: send delete message");
			i = strlen(line);
			if (write(fd, line, i) != i)
				fatal("Lost connection");
			dlog(1, "displayq: wait to read response message");
			while ((i = read(fd, line, sizeof(line))) > 0)
				(void) fwrite(line, 1, i, stdout);
			dlog(1, "displayq: got response message");
			(void) close(fd);
		}
	}
	/*
	 * Find all the control files in the spooling directory
	 */
	dlog(1, "displayq: now display local DQS queue");
	if (chdir(SD) < 0)
		fatal("cannot chdir to spooling directory");
	if ((nitems = getq(&queue)) < 0)
		fatal("cannot examine spooling area\n");
	if (stat(LO, &statb) >= 0 && (statb.st_mode & 010)) {
		if (sendtorem)
			printf("\n%s: ", host);
		printf("Warning: %s queue is turned off\n", printer);
	}
	if (nitems == 0) {
		if (!sendtorem)
			printf("no entries\n");
		return(0);
	}
	fp = fopen(LO, "r");
	if (fp == NULL)
		warn();
	else {
		register char *cp;

		/* get daemon pid */
		cp = current;
		while ((*cp = getc(fp)) != EOF && *cp != '\n')
			cp++;
		*cp = '\0';
		i = atoi(current);
		if (kill(i, 0) < 0)
			warn();
		else {
			/* read current file name */
			cp = current;
			while ((*cp = getc(fp)) != EOF && *cp != '\n')
				cp++;
			*cp = '\0';
			/*
			 * Print the status file.
			 */
			if (sendtorem)
				printf("\n%s: ", host);
			fd = open(ST, O_RDONLY);
			if (fd >= 0) {
				(void) flock(fd, LOCK_SH);
				while ((i = read(fd, line, sizeof(line))) > 0)
					(void) fwrite(line, 1, i, stdout);
				(void) close(fd);	/* unlocks as well */
			} else
				putchar('\n');
		}
		(void) fclose(fp);
	}
	/*
	 * Now, examine the control files and print out the jobs to
	 * be done for each user.
	 */
	if (!lflag)
		header();
	for (i = 0; i < nitems; i++) {
		q = queue[i];
		inform(q->q_name);
		free(q);
	}
	free(queue);
	return(nitems-garbage);
}

/*
 * warn -- Print a warning message if there is no daemon present.
 */
warn()
{
	struct stat statb;

	if (sendtorem)
		printf("\n%s: ", host);
	if (stat(LO, &statb) >= 0 && (statb.st_mode & 0100))
		printf("Warning: %s is down\n", printer);
	else
		printf("Warning: no daemon present\n");
	current[0] = '\0';
}

/*
 * header -- Print the header for the short listing format
 */
static
header()
{
	printf(head0);
	col = strlen(head0)+1;
	blankfill(SIZCOL);
	printf(head1);
}

/*
 * inform -- print out details of a queue entry
 */
static
inform(cf)
	char *cf;
{
	register int j, k;
	register char *cp;
	FILE *cfp;

	/*
	 * There's a chance the control file has gone away
	 * in the meantime; if this is the case just keep going
	 */
	if ((cfp = fopen(cf, "r")) == NULL)
		return;

	if (rank < 0)
		rank = 0;
	if (sendtorem || garbage || strcmp(cf, current))
		rank++;
	j = 0;
	while (getline(cfp)) {
		switch (line[0]) {
		case 'P': /* Was this file specified in the user's list? */
			if (!inlist(line+1, cf)) {
				fclose(cfp);
				return;
			}
			if (lflag) {
				printf("\n%s: ", line+1);
				col = strlen(line+1) + 2;
				prank(rank);
				blankfill(JOBCOL);
				printf(" [job %s]\n", cf+3);
			} else {
				col = 0;
				prank(rank);
				blankfill(OWNCOL);
				printf("%-10s %3d  ", line+1, atoi(cf+3));
				col += 16;
				first = 1;
			}
			continue;
		default: /* some format specifer and file name? */
			if (line[0] < 'a' || line[0] > 'z')
				continue;
			if (j == 0 || strcmp(file, line+1) != 0)
				strcpy(file, line+1);
			j++;
			continue;
		case 'N':
			show(line+1, file, j);
			file[0] = '\0';
			j = 0;
		}
	}
	fclose(cfp);
	if (!lflag) {
		blankfill(SIZCOL);
		printf("%D bytes\n", totsize);
		totsize = 0;
	}
}

/*
 * inlist -- check if queue job number matches one in users list
 */
static
inlist(name, file)
	char *name, *file;
{
	register int *r, n;
	register char **u, *cp;

	if (users == 0 && requests == 0)
		return(1);
	/*
	 * Check to see if it's in the user list
	 */
	for (u = user; u < &user[users]; u++)
		if (!strcmp(*u, name))
			return(1);
	/*
	 * Check the request list
	 */
	for (n = 0, cp = file+3; isdigit(*cp); )
		n = n * 10 + (*cp++ - '0');
	for (r = requ; r < &requ[requests]; r++)
		if (*r == n && !strcmp(cp, from))
			return(1);
	return(0);
}

/*
 * show -- print info for a single file
 */
static
show(nfile, file, copies)
	register char *nfile, *file;
{
	if (strcmp(nfile, " ") == 0)
		nfile = "(standard input)";
	if (lflag)
		ldump(nfile, file, copies);
	else
		dump(nfile, file, copies);
}

/*
 * blankfill -- fill the line with blanks to the specified column
 */
static
blankfill(n)
	register int n;
{
	while (col++ < n)
		putchar(' ');
}

/*
 * dump -- Give the abbreviated dump of the file names
 */
static
dump(nfile, file, copies)
	char *nfile, *file;
{
	register short n, fill;
	struct stat lbuf;

	/*
	 * Print as many files as will fit
	 *  (leaving room for the total size)
	 */
	 fill = first ? 0 : 2;	/* fill space for ``, '' */
	 if (((n = strlen(nfile)) + col + fill) >= SIZCOL-4) {
		if (col < SIZCOL) {
			printf(" ..."), col += 4;
			blankfill(SIZCOL);
		}
	} else {
		if (first)
			first = 0;
		else
			printf(", ");
		printf("%s", nfile);
		col += n+fill;
	}
	if (*file && !stat(file, &lbuf))
		totsize += copies * lbuf.st_size;
}

/*
 * ldump -- print the long info about the file
 */
static
ldump(nfile, file, copies)
	char *nfile, *file;
{
	struct stat lbuf;

	putchar('\t');
	if (copies > 1)
		printf("%-2d copies of %-19s", copies, nfile);
	else
		printf("%-32s", nfile);
	if (*file && !stat(file, &lbuf))
		printf(" %D bytes", lbuf.st_size);
	else
		printf(" ??? bytes");
	putchar('\n');
}

/*
 * prank -- print the job's rank in the queue,
 *	update col for screen management
 */
static
prank(n)
int n;
{
	char line[100];
	static char *r[] = {
		"th", "st", "nd", "rd", "th", "th", "th", "th", "th", "th"
	};

	if (n == 0) {
		printf("active");
		col += 6;
		return;
	}
	if ((n/10) == 1)
		(void) sprintf(line, "%dth", n);
	else
		(void) sprintf(line, "%d%s", n, r[n%10]);
	col += strlen(line);
	printf("%s", line);
}
