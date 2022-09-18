#ifndef lint
static	char	*sccsid = "@(#)makehosts.c	4.2	(ULTRIX)	4/25/91";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1988-1990 by			*
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
 * Modification History:
 *
 * 08-Mar-88	logcher
 *	Creation date - converted from a shell script to c to make
 *	proper use of the new gethostent interface.  Note:
 *	gethostent() only exists for local and yp - no bind yet.
 *
 */

#include <sys/types.h>
#include <sys/dir.h>
#include <sys/param.h>
#include <errno.h>
#include <netdb.h>
#include <stdio.h>

extern int errno;
char *myname;
extern void exit();
extern void perror();

main(argc, argv)
	int argc;
	char *argv[];
{
	char *UHOSTS = "/usr/hosts";
	char *path;
	char buf[MAXPATHLEN+1];
	struct hostent *hp;
	DIR *dirp;
	struct direct *dp;
	int length, link, done;

	myname = argv[0];
	if (argc > 2)
		usage();
	if (argc == 2) {
		argv++;
		path = (char *)malloc(strlen(*argv)+1);
		if (!path)
			exit(-1);
		(void) strcpy(path, *argv);
	}
	else {
		path = (char *)malloc(strlen(UHOSTS)+1);
		if (!path)
			exit(-1);
		(void) strcpy(path, UHOSTS);
	}
	/*
	 * Set up path
	 */
	if (chdir(path) < 0) {
		if (mkdir(path, 0755) == -1) {
			perror(path);
			exit(-1);
		}
	}
	else {
		/*
		 * Remove any entries in path
		 */
		if ((dirp = opendir(".")) == NULL) {
			perror(path);
			exit(-1);
		}
		while ((dp = readdir(dirp)) != NULL) {
			if ((strcmp(dp->d_name, ".")) == 0)
				continue;
			if ((strcmp(dp->d_name, "..")) == 0)
				continue;
			if ((strcmp(dp->d_name, "MAKEHOSTS")) == 0)
				continue;
			if ((unlink(dp->d_name)) < 0) {
				(void) fprintf(stderr, "%s: unlink error(%d)\n", myname, errno);
				exit(-1);
			}
		}
		
		closedir(dirp);
	}
	/*
	 * Get hostents by gethostent and make symbolic links
	 */
	sethostent(1);
	while ((hp = gethostent()) != NULL) {
		if ((strcmp(hp->h_name, "localhost")) == 0)
			continue;
		if (((length = local_hostname_length(hp->h_name)) != NULL) &&  
				length == 9  && 
				(strncmp(hp->h_name, "localhost", length))== 0)
			continue;
		(void)sprintf(buf, "%s/%s", path, hp->h_name);
		link = 0, done = 0;
		while (!done) {
			if (symlink("/usr/ucb/rsh", buf) == -1) {
				if (errno == EROFS) {
					(void) fprintf(stderr, "%s: write to a readonly fs\n", myname);
					exit(-1);
				}
				if (errno == ENOSPC) {
					(void) fprintf(stderr, "%s: out of space on fs\n", myname);
					exit(-1);
				}
				if (errno == EEXIST)
					link++;
			}
			else
				link++;
			if (((length = local_hostname_length(hp->h_name)) == NULL) || (link == 2))
				done = 1;
			else {
				(void)sprintf(buf, "%s/", path);
				strncat(buf, hp->h_name, length);
			}
		}
	}
	endhostent();
	exit(0);
}

usage()
{
	(void) fprintf(stderr, "Usage: %s [name]\n", myname);
	exit(-1);
}
