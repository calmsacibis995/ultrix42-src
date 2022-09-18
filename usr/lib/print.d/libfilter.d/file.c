#ifndef lint
static char *sccsid = "@(#)file.c	4.1      ULTRIX 	10/16/90";
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
 * Modification History: reverse order
 * 25-Sep-1990 - Adrian Thoms (thoms@wessex)
 *	Added -i flag to dump the compiled magic table mtab
 *	in the form of an initialized structure so that the utility
 *	can be recompiled with the magic information built in.
 *
 *	Added -b flag, conditionally compiled which switches to using
 *	builtin magic information in place of the magic file.
 *	This is so that we can use the program to test the new file
 *	guesser for the print system.
 */

/************************************************************************
 *			Modification History				*
 * 001 Richard Hart, Oct. 21, 1987					*
 *     Copied from 4.3 BSD code:					*
 *		file.c 4.12	(Berkeley) 11/17/85			*
 * 002 Richard Hart, Oct. 21, 1987					*
 *     Added named pipes for Sys V support, and other things in the	*
 *     current Ultrix file.c						*
 * 003 Richard Hart, Oct. 22, 1987					*
 *     Added use of /etc/magic, like Sys V filecommand			*
 * 004 Richard Hart, Nov. 5, 1987					*
 *     Now uses sys/exec.h for support of a.out magic numbers		*
 * 005 Richard Hart, Nov. 16, 1987					*
 *     Separated filetype library routine from file command		*
 * 006 Richard Hart, August 17, 1988					*
 *     Switched to parse args with getopt.				*
 * 007 Jon Reeves, November 12, 1988					*
 *     Fixed declaration of optarg					*
 ************************************************************************/

#include <sys/param.h>
#include <stdio.h>
#include "filetype.h"

char *mfile = "/usr/lib/file/magic";

main(argc, argv)
char **argv;
{
	FILE *fl;
	register char *p;
	char ap[MAXPATHLEN + 1], *s;
	char *fname;
	int bflag = 0,
	    cflag = 0,
	    fflag = 0,
	    errflg = 0,
	    iflag = 0;
	extern int optind;
	extern char *optarg;
	char c;


	if (argc < 2) errflg++;

	if (!errflg)
		while((c = getopt(argc, argv, "bcf:im:")) != EOF)
			switch (c) {
			case 'b':
				bflag++;
				break;
			case 'c':
				cflag++;
				break;
			case 'f':
				fflag++;
				fname = optarg;
				break;
			case 'i':
				iflag++;
				break;
			case 'm':
				mfile = optarg;
				break;
			case '?':
				errflg++;
				break;
			default:
				fprintf(stderr, "file: illegal option - %c\n",*s);
				exit(2);
			}
	
	if (errflg) {
		fprintf(stderr, "usage: %s [-c] [-m magic-file] [-f source-file] [file ...]\n", argv[0]);
		exit(3);
	}
#ifdef HAVE_MAGIC
	if (bflag) {
		binary_mkmtab();
	} else
#endif
	mkmtab(1);	/* make the internal table now to catch errors before anything */
			/* else is begun.					       */

	if (cflag) {
		printmtab();
	}
	if (iflag) {
		init_printmtab();
	}
	if (iflag || cflag) exit(0);

	if (fflag) {
		if ((fl = fopen(fname, "r")) == NULL) {
			perror(fname);
			exit(2);
		}
		while ((p = fgets(ap, sizeof ap, fl)) != NULL) {
			int l = strlen(p);
			if (l>0)
				p[l-1] = '\0';
			printf("%s:	", p);
			filetype(p, PRINT);
		}
		exit(1);
	}
	else
		for ( ; optind < argc; optind++) {
			printf("%s:	", argv[optind]);
			filetype(argv[optind], PRINT);
			fflush(stdout);
		}
	exit(0);
}
