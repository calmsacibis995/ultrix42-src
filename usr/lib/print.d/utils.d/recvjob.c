#ifndef lint
static char *sccsid = "@(#)recvjob.c	4.1      ULTRIX 	7/2/90";
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
 * recvjob.c -- Receive printer jobs from the network, queue them and
 *	start the printer daemon.
 */

/*
 * Modification History continues from Jan 1990 in reverse order
 *
 * 11-jan-90 -- thoms
 *	Fix buffer sizes for storing hostname.
 */

/*
 * ***************************************************************
 *                -- Revision History --
 * ***************************************************************
 * 
 * 01/11/83 -- sccs
 * date and time created 83/11/01 20:59:18 by sccs
 * 
 * ***************************************************************
 * 
 * 28/06/85 -- root
 * Comments taken from: /usr/src/usr.lib/lpr/SCCS/s.recvjob.c:
 *	1.2 85/06/24 08:36:14 williams 2 1	00001/00001/00203
 * added argument to printjob to signify not initial condition.
 * 
 * ***************************************************************
 *
 * 19/07/88 -- thoms
 * Added copyright notice and modification history
 *
 * ***************************************************************
 * 
 * 14/09/89 -- Daren Seymour 
 * Fixed security hole.
 * 
 * ***************************************************************
 *
 * 17/09/89 -- Giles Atkinson
 * More security fixes - be very paranoid about the control file!
 *
 * PUT NEW MODIFICATION HISTORY IN REVERSE ORDER ABOVE
 */


#include "lp.h"

/* Default size for buffer to hold the names of the files (control and data)
 * which make up a job.   The names are needed to validate the control file
 * and clean up if there is a problem with the job or its transfer.
 */

#define FNB_SIZE 2040

/* Temporary buffers used to hold incoming file names and text of control file
 * with functions to handle them.
 */

static struct tdbuf {
	char *td_rdp;		/* Read data pointer */
	char *td_wdp;		/* Write data pointer */
	int   td_size;		/* Size of data storage */
	char  td_data[1];	/* Data here */
} *fnp, *cdp;

static int find_td();
static char *gets_td(), *puts_td();
static struct tdbuf *init_td();
static int onintr();

recvjob()
{
	struct stat stb;
	char *bp = pbuf;
	int status;

	/*
	 * Perform lookup for printer name or abbreviation
	 */
	if ((status = pgetent(line, printer)) < 0)
		fatal("cannot open printer description file");
	else if (status == 0)
		fatal("unknown printer");
	if ((LF = pgetstr("lf", &bp)) == NULL)
		LF = DEFLOGF;
	if ((SD = pgetstr("sd", &bp)) == NULL)
		SD = DEFSPOOL;
	if ((LO = pgetstr("lo", &bp)) == NULL)
		LO = DEFLOCK;

	(void) close(2);
	(void) open(LF, O_WRONLY|O_APPEND);
	if (chdir(SD) < 0)
		fatal("cannot chdir to %s", SD);
	if (stat(LO, &stb) == 0 && (stb.st_mode & 010)) {
		/* queue is disabled */
		putchar('\1');		/* return error code */
		exit(1);
	}
	signal(SIGHUP, onintr);
	signal(SIGINT, onintr);
	signal(SIGQUIT, onintr);
	signal(SIGTERM, onintr);
	signal(SIGCHLD, SIG_DFL);

	if (readjob())
		printjob(0);
}

char	*sp = "";
#define ack()	(void) write(1, sp, 1);

/*
 * Read printer jobs sent by lpd and copy them to the spooling directory.
 * Return the number of jobs successfully transfered.
 */
static
readjob() {
	register int size, nfiles;
	register char *cp;

	fnp = init_td(fnp, FNB_SIZE);		/* Init filename buffer */
	ack();
	nfiles = 0;
	for (;;) {
		/*
		 * Read a command to tell us what to do
		 */
		cp = line;
		do {
			if ((size = read(1, cp, 1)) != 1) {
				if (size < 0)
					fatal("Lost connection");
				cleanup();
				return(nfiles);
			}
		} while (*cp++ != '\n' && cp < &line[MAX_COML]);
		if (cp >= &line[MAX_COML])
			fatal("Command line too long");
		*--cp = '\0';
		cp = line;
		switch (*cp++) {
		case '\1':	/* cleanup because data sent was bad */
			cleanup();
			continue;

		case '\2':	/* read cf file */
			if (!read_control(cp))
				break;
			nfiles++;
			continue;

		case '\3':	/* read df file */
			size = 0;
			while (*cp >= '0' && *cp <= '9')
				size = size * 10 + (*cp++ - '0');
			if (*cp++ != ' ')
				break;
			(void) readfile(cp, size);
			continue;
		}
		fatal("protocol screwup");
	}
}

/* Handle a request to read a control file */

static int
read_control(fdat)
char *fdat;
	{
	char tname[CFNAME_LEN+1], cfname[CFNAME_LEN+1];
	char lname[MAX_COML];
	FILE *cfile;
	register char *cp;
	register int size, n;

	size = 0;
	cp = fdat;
	while (*cp >= '0' && *cp <= '9')
		size = size * 10 + (*cp++ - '0');
	if (*cp++ != ' ')
		return 0;
	fdat = cp;
	if (index(fdat, '/'))
		fatal("Illegal filename %s", fdat);

	/* Allocate a buffer and read control file. */

	ack();
	cdp = init_td(cdp, size);
	cp = cdp->td_data;
	while (size) {
		n = read(1, cp, size);
		if (n <= 0)
			fatal("Lost connection");
		size -= n;
		cp += n;
	}
	cdp->td_wdp = cp;

	/* Now read it back and write a safe control file */

	strncpy(cfname, fdat, sizeof cfname - 1);
	cfname[sizeof cfname - 1] = '\0';
	strcpy(tname, cfname);

	tname[0] = (tname[0] == 't') ? 'T' : 't';
	cp = puts_td(tname, &fnp);
	if ((n = open(tname, O_WRONLY|O_CREAT|O_EXCL, FILMOD)) < 0 ||
	    (cfile = fdopen(n, "w")) == NULL) {
		if (n >= 0)
			close(n);
		else
			fnp->td_wdp = cp;       /* To protect existing file */
                fatal("cannot create %s", tname);
	}
	lname[0] = '\0';

	while (cp = gets_td(cdp)) {
		register int command_letter;

		switch(command_letter = *cp) {
		    case 'U':	/* Unlink command - dangerous, so ignore it */
			continue;
		    case 'H':	/* No spoofing allowed */
			fprintf(cfile, "H%s\n", from);
			continue;
		    default:
			break;
		}


		/* It would be nice to do something about the `M' and `E'
		 * commands which can mail back on completion - rewriting the
		 * node name would make them more reliable - do it later.
		 *
		 * In the meantime all other commands go through unchanged.
		 */

		fputs(cp, cfile);
		putc('\n', cfile);

		/* Lower case letters indicate a file to be printed.
		 * Ensure that they are in the spool directory and are
		 * part of this job.  If they pass we add a `U' line for
		 * them so that they will be deleted after printing.
		 * Only write one `U' line per file.
		 */

		if (command_letter >= 'a' && command_letter <= 'z') {
			if (strcmp(++cp, lname)) {
				strcpy(lname, cp);
				if (index(cp, '/') || !find_td(cp, fnp)) {
					fclose(cfile);
					fatal("Illegal control file data: %s",
					      --cp);
				}
				putc('U', cfile);
				fputs(cp, cfile);
				putc('\n', cfile);
			}
		}
		    
	}
	if (ferror(cfile) || fclose(cfile) == EOF)
		fatal("Error writing control file");
	if (noresponse()) {		/* file sent had bad data in it */
		return(0);
	}

	/* Use link/unlink rather than rename here for fear of destroying
	 * existing file.
	 */

	cp = puts_td(fdat, &fnp);
	if (link(tname, fdat) < 0) {
		fnp->td_wdp = cp;		/* Protect existing file */
		fatal("cannot rename %s", tname);
	}
	(void) unlink(tname);
	fnp = init_td(fnp, FNB_SIZE);		/* Job succeeded, save files */
	ack();
	return 1;
}

/*
 * Read files send by lpd and copy them to the spooling directory.
 */
static
readfile(file, size)
	char *file;
	int size;
{
	register char *cp;
	char buf[BUFSIZ];
	register int i, j, amt;
	int fd, err;

	if (index(file, '/') !=0) {
		fatal("%s illegal filename", file);
	}
	cp = puts_td(file, &fnp);
	if ((fd = open(file, O_WRONLY|O_CREAT|O_EXCL, FILMOD)) < 0) {
		fnp->td_wdp = cp;		/* Protect existing file */
		fatal("cannot create %s", file);
	}
	ack();
	err = 0;
	for (i = 0; i < size; i += BUFSIZ) {
		amt = BUFSIZ;
		cp = buf;
		if (i + amt > size)
			amt = size - i;
		do {
			j = read(1, cp, amt);
			if (j <= 0)
				fatal("Lost connection");
			amt -= j;
			cp += j;
		} while (amt > 0);
		amt = BUFSIZ;
		if (i + amt > size)
			amt = size - i;
		if (write(fd, buf, amt) != amt) {
			err++;
			break;
 	    }
	}
	(void) close(fd);
	if (err)
		fatal("%s: write error", file);
	if (noresponse()) {		/* file sent had bad data in it */
		(void) unlink(file);
		return(0);
	}
	ack();
	return(1);
}

static
noresponse()
{
	char resp;

	if (read(1, &resp, 1) != 1)
		fatal("Lost connection");
	if (resp == '\0')
		return(0);
	return(1);
}

/*
 * Remove all the files associated with the current job being transfered.
 */
static
cleanup()
{
	register char *cp;

	fnp->td_rdp = fnp->td_data;		/* Rewind pseudo-file */
	while (cp = gets_td(fnp))
		unlink(cp);
}

static
fatal(msg, a1)
	char *msg;
{
	cleanup();
	log(msg, a1);
	putchar('\1');		/* return error code */
	exit(1);
}

/* Functions to handle temporary text storage. */

static struct tdbuf *
init_td(tdp, size)
register struct tdbuf *tdp;
int size;
	{
	if (tdp && tdp->td_size < size) {
		free(tdp);
		tdp = 0;
	}
	if (!tdp) {
		tdp = (struct tdbuf *) malloc(sizeof (struct tdbuf) + size);
		tdp->td_size = size;
	}
	tdp->td_rdp = tdp->td_wdp = tdp->td_data;
	return tdp;
}

/* Put a null-terminated string into a buffer.
 * Notice that the second argument is a pointer to the buffer pointer -
 * this allows us to reallocate the buffer on overflow.
 * A pointer to the stored data is returned, this allows it to be erased
 * if necesary by restoring the write pointer.
 */

static char *
puts_td(cp, tdpp)
char *cp;
register struct tdbuf **tdpp;
	{
	register struct tdbuf *tdp;
	register char *ip, *op, *ep;

	tdp = *tdpp;				/* Point at buffer */
	ip = cp;
	op = tdp->td_wdp;
again:
	ep = tdp->td_data + tdp->td_size;
	while (*ip && op < ep) {
		*op++ = *ip++;
	}
	if (ip == cp && !*ip)			/* Nothing there */
		return;
	if (op == ep) {

		/* Buffer overflowed */

		register struct tdbuf *nbfp;

		nbfp = (struct tdbuf *) realloc(tdp,
				sizeof (struct tdbuf) + 2 * tdp->td_size);
		nbfp->td_size = 2 * tdp->td_size;
		nbfp->td_wdp += nbfp - tdp;
		nbfp->td_rdp += nbfp - tdp;
		op += nbfp - tdp;
		*tdpp = tdp = nbfp;
		goto again;
	}
	*op++ = '\0';
	ep = tdp->td_wdp;
	tdp->td_wdp = op;
	return ep;
}

/* Read the next string out of a temporary buffer */

static char *
gets_td(tdp)
register struct tdbuf *tdp;
	{
	register char *cp, *dp;

	cp = dp = tdp->td_rdp;
	if (dp < tdp->td_wdp) {
		while (*cp && *cp != '\n' && cp < tdp->td_wdp)
			++cp;
		*cp++ = '\0';
		
		tdp->td_rdp = cp;
		return dp;
	} else
		return 0;
}

/* Find if a string is stored in a temporary data buffer. */

static int
find_td(xp, tdp)
register char *xp;
register struct tdbuf *tdp;
	{
	register char *sp, *cp;

	sp = tdp->td_rdp;			/* Save initial position */
	while (cp = gets_td(tdp))
		if (!strcmp(cp, xp))
			return 1;
	if (sp != tdp->td_data) {		/* Rewind pseudo-file */
		tdp->td_rdp = tdp->td_data;
		while (tdp->td_rdp < sp && (cp = gets_td(tdp)))
        	        if (!strcmp(cp, xp))
	                        return 1;
	}
	return 0;
}

static int
onintr()
{
	fatal("Terminated by signal\n");
}
