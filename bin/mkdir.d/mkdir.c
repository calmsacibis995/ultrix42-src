#ifndef lint
static char *sccsid = "@(#)mkdir.c	4.1      ULTRIX 7/2/90";
#endif
/************************************************************************
 *									*
 *			Copyright (c) 1987 by				*
 *		Digital Equipment Corporation, Maynard, MA		*
 *			All rights reserved.				*
 *									*
 *   This software is furnished under a license and may be used and	*
 *   copied  only  in accordance with the terms of such license and	*
 *   with the  inclusion  of  the  above  copyright  notice.   This	*
 *   software  or  any	other copies thereof may not be provided or	*
 *   otherwise made available to any other person.  No title to and	*
 *   ownership of the software is hereby transferred.			*
 *									*
 *   This software is  derived	from  software	received  from	the	*
 *   University    of	California,   Berkeley,   and	from   Bell	*
 *   Laboratories.  Use, duplication, or disclosure is	subject  to	*
 *   restrictions  under  license  agreements  with  University  of	*
 *   California and with AT&T.						*
 *									*
 *   The information in this software is subject to change  without	*
 *   notice  and should not be construed as a commitment by Digital	*
 *   Equipment Corporation.						*
 *									*
 *   Digital assumes no responsibility for the use  or	reliability	*
 *   of its software on equipment which is not supplied by Digital.	*
 *									*
 ************************************************************************/
/*
 *
 * make directory - if -p option is given then make intermediate
 *                  directories as well.
 *
 * Modification History:
 *
 * May 21, 1989 - David Gray
 *
 * Modified code to exit which know values, 0 when everything
 * is ok and 1 when an error in encountered.
 *
 * April 4, 1989  - David Gray
 *
 * Added -p option to be Posix compliant, makes intermediate
 * directories if they do not exist. And added Usage().
 *
 */
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>


main(argc, argv)
char *argv[];
{
    void		Usage();
    char		*strtok();
    int			c, errors = 0;
    int			create_int = 0;
    char		tempdir[256], intermediate[256], *tfile;
    struct stat		sb;
    extern int		optind;
    extern char		*optarg;

    while (( c = getopt (argc, argv, "p")) != EOF) {
	switch (c) {
	    case 'p' :
		create_int ++;
		break;
	    case '?' :
	    default  :
		Usage ();
		exit (1);
	}
    }
    if (optind == argc) {
	fprintf(stderr, "Usage: mkdir [-p] dir1 [dir2 ...]\n");
	exit(1);
    }
    for ( ; optind < argc; optind++) {
	intermediate[0] = '\0';
	if (create_int) {
	    strcpy (tempdir, argv[optind]);
	    if (tempdir[0] == '/') strcpy (intermediate, "/");
	    for (tfile = strtok (tempdir, "/"); 
		tfile != NULL;
	 	tfile = strtok(NULL, "/")) {
		strcat (intermediate, tfile);
		if (stat (intermediate, &sb) < 0) {
		    /* try making it */
		    if (mkdir(intermediate, 0777) < 0) {
			fprintf(stderr, "mkdir: ");
			fprintf(stderr, 
			"Could not make intermediate directory %s\n", 
			intermediate);
			perror(argv[optind]);
			errors++;
			break;
		    }
		}
		strcat (intermediate, "/");
	    }
	}
	else {
	    if (mkdir(argv[optind], 0777) < 0) {
		fprintf(stderr, "mkdir: ");
		perror(argv[optind]);
		errors++;
	    }
	}
    }
    exit (errors != 0);
}

/*
 * Usage - Print usage message
 */

void
Usage ()
{
    fprintf(stderr, "Usage: mkdir [-p] dir1 [dir2 ...]\n");
}
