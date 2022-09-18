#ifndef lint
static char *sccsid = "@(#)sendjob.c	4.2	ULTRIX	9/11/90";
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
 * sendjob.c -- send job to remote Berkeley style spooler
 *
 * Description:
 *	This code was originally part of printjob.c
 */

/* SCCS history beginning
 * ***************************************************************
 *                -- Revision History --
 * ***************************************************************
 * 
 * 1.1  25/04/88 -- thoms
 * date and time created 88/04/25 18:57:14 by thoms
 * 
 * ***************************************************************
 *
 * 1.2  19/07/88 -- thoms
 * Added copyright notice and modification history
 *
 * ***************************************************************
 *
 * 1.3  20/08/90 -- atkinson
 * Check data files which are symbolic links.
 *
 * SCCS history end
 */


#include "lp.h"


/*
 * Send the daemon control file (cf) and any data files.
 * Return -1 if a non-recoverable error occured, 1 if a recoverable error and
 * 0 if all is well.
 */
enum job_status_e
sendit(cxp, file)
register CXP cxp;
char *file;
{
	register FILE *cfp;
	register int linelen, err = 0;
	char last[132];

	/*
	 * open control file
	 */
	if ((cfp = fopen(file, "r")) == NULL) {
		log("open failure (%d) of control file %s", errno, file);
		return(js_failed); /* used to return ok */
	}
	/*
	 *      read the control file for work to do
	 *
	 *      file format -- first character in the line is a command
	 *      rest of the line is the argument.
	 *      commands of interest are:
	 *
	 *            a-z -- "file name" name of file to print
	 *              U -- "unlink" name of file to remove
	 *                    (after we print it. (Pass 2 only)).
	 */

	/*
	 * pass 1
	 */
	while (getline(cfp)) {
	again:
		if (line[0] >= 'a' && line[0] <= 'z') {
			strcpy(last, line);
			while (linelen = getline(cfp))
				if (strcmp(last, line))
					break;
			if ((err=sendfile(cxp->cx_pr_fd, '\3', last+1)) > 0) {
				(void) fclose(cfp);
				return(js_retry);
			} else if (err)
				break;
			if (linelen)
				goto again;
			break;
		}
	}
	if (!err && sendfile(cxp->cx_pr_fd, '\2', file) > 0) {
		(void) fclose(cfp);
		return(js_retry);
	}
	/*
	 * pass 2
	 */
	fseek(cfp, 0L, 0);
	while (getline(cfp))
		if (line[0] == 'U')
			(void) unlink(line+1);
	/*
	 * clean-up incase another control file exists
	 */
	(void) fclose(cfp);
	(void) unlink(file);
	return(js_ok);
}

/*
 * Send a data file to the remote machine and spool it.
 * Return positive if we should try resending.
 */
static
sendfile(pfd, type, file)
int pfd;
char type, *file;
{
	register int f, i, amt;
	struct stat stb;
	char buf[BUFSIZ];
	int sizerr, slink;

	if (lstat(file, &stb) < 0) {
		log("lstat failed (%d) for file %s\n", errno, file);
		return -1;
	}
	slink = ((stb.st_mode & S_IFMT) == S_IFLNK);	/* Symbolic link? */

	if ((f = open(file, O_RDONLY)) < 0 || slink && fstat(f, &stb) < 0) {
		log("open failure (%d) of control file %s", errno, file);
		return(-1);
	}
	if (slink && bad_link(file, f))
		return -1;

	(void) sprintf(buf, "%c%d %s\n", type, stb.st_size, file);
	amt = strlen(buf);
	if (write(pfd, buf, amt) != amt) {
		(void) close(f);
		return(1);
	}
	if (noresponse(pfd)) {
		(void) close(f);
		return(1);
	}
	sizerr = 0;
	for (i = 0; i < stb.st_size; i += BUFSIZ) {
		amt = BUFSIZ;
		if (i + amt > stb.st_size)
			amt = stb.st_size - i;
		if (sizerr == 0 && read(f, buf, amt) != amt)
			sizerr = 1;
		if (write(pfd, buf, amt) != amt) {
			(void) close(f);
			return(1);
		}
	}
	(void) close(f);
	if (sizerr) {
		log("%s: changed size", file);
		(void) write(pfd, "\1", 1);  /* tell recvjob to ignore this file */
		return(-1);
	}
	if (write(pfd, "", 1) != 1)
		return(1);
	if (noresponse(pfd))
		return(1);
	return(0);
}

/*
 * Check to make sure there have been no errors and that both programs
 * are in sync with eachother.
 * Return non-zero if the connection was lost.
 */
int
noresponse(fd)
int fd;
{
	char resp;

	if (read(fd, &resp, 1) != 1 || resp != '\0') {
		log("lost connection or error in recvjob");
		return(1);
	}
	return(0);
}

/* Check data files which are symbolic links.
 * lpr now creates an auxiliary file with additional information alongside
 * each link.  The two names differ only in the first character.
 */

bad_link(file, fd)
char *file;
int fd;							/* fd open on file */
{
	struct stat stb;
	int dev = 0, ino = 0;
	FILE *lfp = NULL;

	if (islower(file[0]) && isascii(file[0])) {
		file[0] = _toupper(file[0]);
		lfp = fopen(file, "r");
		file[0] = _tolower(file[0]);
	}
	if (lfp != NULL) {
		if (fstat(fd, &stb) < 0) {
			logerr("fstat failed for %s", file);
			return 1;
		}
		while (getline(lfp)) {
			switch (line[0]) {
			case 'I': ino = atoi(line+1);	/* Inode number */
				break;
			case 'D': dev = atoi(line+1);	/* Device number */
				break;
			}
		}
		fclose(lfp);
		if (ino == stb.st_ino && dev == stb.st_dev)
			return 0;			/* link verified */
	}
	log("bad symbolic link %s\n", file);
	return 1;					/* bad link */
}
