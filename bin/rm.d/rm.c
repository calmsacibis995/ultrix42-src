#ifndef lint
static	char	*sccsid = "@(#)rm.c	4.1	(ULTRIX)	7/2/90";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1984 by				*
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
 * rm.c
 *
 *	02-Nov-83	mah1: Suppress all questions and messages if -f
 *			      option has been indicated.  Exceptions are
 *			      "unusually" messges.
 *
 *	17-Feb-84	mah. Change sccsid format to match
 *			ueg's sccsid format.
 *
 */

int	errcode;

#include <stdio.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/dir.h>

main(argc, argv)
char *argv[];
{
	register char *arg;
	int fflg, iflg, rflg;

	fflg = 0;
	if (isatty(0) == 0)
		fflg++;
	iflg = 0;
	rflg = 0;
	while(argc>1 && argv[1][0]=='-') {
		arg = *++argv;
		argc--;

		/*
		 *  all files following a null option are considered file names
		 */
		if (*(arg+1) == '\0') break;

		while(*++arg != '\0')
			switch(*arg) {
			case 'f':
				fflg++;
				break;
			case 'i':
				iflg++;
				break;
			case 'r':
				rflg++;
				break;
			default:
				printf("rm: unknown option %s\n", *argv);
				exit(1);
			}
	}
	while(--argc > 0) {
		if(!strcmp(*++argv, "..")) {
			if (!fflg)		/* mah1 - suppress msg*/
				fprintf(stderr, "rm: cannot remove `..'\n");
			continue;
		}
		rm(*argv, fflg, rflg, iflg, 0);
	}

	exit(errcode);
}

rm(arg, fflg, rflg, iflg, level)
char arg[];
{
	struct stat buf;
	struct direct *dp;
	DIR *dirp;
	char name[BUFSIZ];
	int d;

	if(lstat(arg, &buf)) {
		if (fflg==0) {
			printf("rm: %s nonexistent\n", arg);
			++errcode;
		}
		return;
	}
	if ((buf.st_mode&S_IFMT) == S_IFDIR) {
		if(rflg) {
			if (access(arg, 02) < 0) {
				if (fflg==0)
					printf("%s not changed\n", arg);
				errcode++;
				return;
			}
			if(iflg && level!=0 && !fflg) {	/* mah1 - suppress msg*/
				printf("remove directory %s? ", arg);
				if(!yes())
					return;
			}
			if((dirp = opendir(arg)) == NULL) {
				if (!fflg)		/* mah1 - suppress msg*/
					printf("rm: cannot read %s?\n", arg);
				exit(1);
			}
			while((dp = readdir(dirp)) != NULL) {
				if(dp->d_ino != 0 && !dotname(dp->d_name)) {
					sprintf(name, "%s/%s", arg, dp->d_name);
					rm(name, fflg, rflg, iflg, level+1);
				}
			}
			closedir(dirp);
			if (dotname(arg))
				return;
			if (iflg && !fflg) {		/* mah1 - suppress msg*/
				printf("rm: remove %s? ", arg);
				if (!yes())
					return;
			}
			if (rmdir(arg) < 0) {
				fprintf(stderr, "rm: ");
				perror(arg);
				errcode++;
			}
			return;
		}
		printf("rm: %s directory\n", arg);
		++errcode;
		return;
	}

	if(iflg && !fflg) {				/* mah1 - suppress msg*/
		printf("rm: remove %s? ", arg);
		if(!yes())
			return;
	}
	else if(!fflg) {
		if ((buf.st_mode&S_IFMT) != S_IFLNK && access(arg, 02) < 0) {
			printf("rm: override protection %o for %s? ", buf.st_mode&0777, arg);
			if(!yes())
				return;
		}
	}
	if(unlink(arg)){				/*mah1 - ~suppress msg*/
		printf("rm: %s not removed\n", arg);
		++errcode;
	}
}

dotname(s)
char *s;
{
	if(s[0] == '.')
		if(s[1] == '.')
			if(s[2] == '\0')
				return(1);
			else
				return(0);
		else if(s[1] == '\0')
			return(1);
	return(0);
}

yes()
{
	int i, b;

	i = b = getchar();
	while(b != '\n' && b != EOF)
		b = getchar();
	return(i == 'y');
}
