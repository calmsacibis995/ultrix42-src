
#ifndef lint
static  char    *sccsid = "@(#)mkfifo.c	4.1     (ULTRIX) 7/17/90";
#endif lint

/*********************************************************************
*                                                                    *
*                      Copyright (c) 1989 by                         *
*              Digital Equipment Corporation, Maynard, MA            *
*                                                                    *
*   This software is furnished under a license and may be used and   *
*   copied  only  in accordance with the terms of such license and   *
*   with the  inclusion  of  the  above  copyright  notice.   This   *
*   software  or  any  other copies thereof may not be provided or   *
*   otherwise made available to any other person.  No title to and   *
*   ownership of the software is hereby transferred.                 *
*                                                                    *
*   The information in this software is subject to change  without   *
*   notice  and should not be construed as a commitment by Digital   *
*   Equipment Corporation.                                           *
*                                                                    *
*   Digital assumes no responsibility for the use  or  reliability   *
*   of its software on equipment which is not supplied by Digital.   *
*                                                                    *
*********************************************************************/

/**********************************************************************
* mkfifo - Make FIFO special files
*
* This command is Posix, 1003.2-Draft 8 compliant.
**********************************************************************/

/**********************************************************************
* Modification History
*
**********************************************************************/

/* Includes */
#include <stdio.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

/* Defines */
#define	FIFO		0010000
#define ACCESS		0666

/* Global Variables */
extern int		errno;

main (argc, argv)

int 	argc;
char	*argv[];

{
    /* declarations */
    int			i, errors = 0;
    char		filename[PATH_MAX];
    struct stat		statbuf;

    if (argc == 1) {
	fprintf (stderr, "Usage: %s filename ... \n", argv[0]);
	errors++;
    }
    else {
	for (i = 1; i < argc; i++) {
	    strcpy (filename, argv[i]);
	    if (stat (filename, &statbuf) != 0) {
		if (errno == ENOENT) {
		    if (mknod (filename, (FIFO | ACCESS), 0) != 0) {
			fprintf (stderr, "%s: error making %s, errno %d\n",
				argv[0], filename, errno);
			errors++;
		    }
		}
		else {
		    fprintf (stderr, "%s: error in %s, errno %d\n",
			argv[0], filename, errno);
		    errors ++;
		}
	    }
	    else {
		fprintf (stderr, "%s: file %s already exists\n",
			argv[0], filename);
		errors++;
	    }
	}
    }
    exit(errors != 0);
}

