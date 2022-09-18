#ifndef lint
static  char    *sccsid = "@(#)mktemp.c	4.1     (ULTRIX) 7/17/90";
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
* mktemp - Make a name for a temporary file. 
*
* This command is Posix, 1003.2-Draft 8 compliant.
**********************************************************************/


/**********************************************************************
* Modification History:
*
**********************************************************************/

/* Includes */
#include <stdio.h>
#include <limits.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>


/* Defines */
#define OPTIONS		"d:p:c"

/* Global Variables */
extern int	errno;
extern int	optind;
extern char	*optarg;
extern char	*getlogin(), *getenv();

main (argc, argv)

int	argc;
char	*argv[];

{
    /* Declarations */
    int			c, pid, notUnique = 1;
    int			cflag =0, dflag = 0, pflag = 0;
    char		dirName[PATH_MAX];
    char		tempName[PATH_MAX];
    char		filename[NAME_MAX];
    char		prefix[NAME_MAX];
    char		ll, *tmpDir, *tmpPrefix;
    void		Usage();
    struct stat		fileStatus;

    while ((c = getopt (argc, argv, OPTIONS)) != EOF) {
	switch (c) {
	    case 'c':
		cflag++;
		break;
	    case 'd':
		if (strlen (optarg) >= PATH_MAX) {
		    fprintf (stderr, "%s: directory name exceeds ", argv[0]);
		    fprintf (stderr, "%d characters\n", PATH_MAX);
		    exit (1);
		}
		dflag++;
		if (Writable(optarg))
		    strcpy (dirName, optarg);
		else {
		    fprintf (stderr, "%s: directory not writable", argv[0]);
		    fprintf (stderr, " or does not exist, using /tmp\n");
		    strcpy (dirName, "/tmp");
		}
		break;
	    case 'p':
		pflag ++;
		strcpy (prefix, optarg);
		if (strlen(prefix) > (NAME_MAX - 6))
		    prefix[NAME_MAX-6] = '\0';
		break;
	    case '?':
	    default:
		Usage (argv[0]);
		exit (1);
	}
    }

    if (!dflag) {
	if ((tmpDir = getenv ("TMPDIR")) != NULL) {
	    if (!Writable(tmpDir)) {
		fprintf (stderr, "%s: directory not writable", argv[0]);
		fprintf (stderr, " or does not exist, using /tmp\n");
		tmpDir = "/tmp";
	    }
	}
	else
	    tmpDir = "/tmp";

	strcpy (dirName, tmpDir);
    }

    if (!pflag) {
	if ((tmpPrefix = getenv ("LOGNAME")) == NULL) {
	    if ((tmpPrefix = getlogin()) == NULL) {
		fprintf (stderr, "%s: could not get file name prefix\n",
			argv[0]);
		exit (1);
	    }
	}
	strcpy (prefix, tmpPrefix);
    }

    pid = getpid();
    while (pid > 9999) pid = pid / 10;

    /* Assemble temp name and make sure its unique */

    ll = 'a';
    while (notUnique) {
	(void) sprintf (tempName, "%s/%s.%04d%c", dirName, prefix, pid, ll);
	if (stat (tempName, &fileStatus) != 0) {
	    if (errno == ENOENT)
		break;
	    else {
		fprintf (stderr, "%s: error occurred while examining",argv[0]);
		fprintf (stderr, " file path %s : errno %d\n", dirName, errno);
		exit (1);
	    }
	}
	/* sequentially generate names until one is found to be unique */
	if (ll < 'z')
	    ll++;
	else {
	    ll = 'a';
	    pid = (pid < 9999) ? pid+1 : 100;
	}
    }

    if (cflag) {
	if (creat (tempName, 0666) == -1) 
	    fprintf (stderr, "%s: could not create temp file\n", argv[0]);
    }

    printf ("%s\n", tempName);
}

/**********************************************************************
* Usage - print usage message before exiting
**********************************************************************/

void
Usage (name)

char 	*name;

{
    fprintf (stderr, "%s: [-c] [-d dir_name] [-p prefix]\n", name);
}

/**********************************************************************
* Writable - returns true is the directory chosen is writable, false
*            otherwise.
**********************************************************************/

#define U_PERM		6
#define G_PERM		3
#define O_PERM		0
#define RWX		7
#define U_MASK		00007
#define G_MASK		000007
#define O_MASK		0000007

int
Writable (directory)

char	*directory;

{
    struct stat		statbuf;
    int			uid, gid;

    if (stat (directory, &statbuf) != 0) {
	return (0);
    }

    uid = getuid();
    gid = getgid();

    if (uid == statbuf.st_uid) {
	if (((statbuf.st_mode >> U_PERM) & U_MASK) == RWX)
	    return (1);
    }
    if (gid == statbuf.st_gid) {
	if (((statbuf.st_mode >> G_PERM) & G_MASK) == RWX)
	    return (1);
    }
    if (((statbuf.st_mode >> O_PERM) & O_MASK) == RWX)
	return (1);
    else
	return (0);

}
	
