#ifndef lint
static	char	*sccsid = "@(#)swapon.c	4.1	(ULTRIX)	7/2/90";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1985 by				*
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

/*-----------------------------------------------------------------------
 *
 * 04-22-85 -- jrs
 *	Fixed error message for swapon -a failure.
 *
 *	Based on 4.2BSD labelled:
 *		swapon.c	4.4	10/16/80
 *
 *-----------------------------------------------------------------------*/

#include <errno.h>
#include <stdio.h>
#include <fstab.h>

#define	VSWAPON	85

extern errno;
extern char *sys_errlist[];

main(argc, argv)
	int argc;
	char *argv[];
{
	int stat = 0;

	--argc, argv++;
	if (argc == 0) {
		fprintf(stderr, "usage: swapon name...\n");
		exit(1);
	}
	if (argc == 1 && !strcmp(*argv, "-a")) {
		struct	fstab	*fsp;
		if (setfsent() == 0)
			perror(FSTAB), exit(1);
		while ( (fsp = getfsent()) != 0){
			if (strcmp(fsp->fs_type, FSTAB_SW) != 0)
				continue;
			printf("Adding %s as swap device\n",
			    fsp->fs_spec);
			if (syscall(VSWAPON, fsp->fs_spec) == -1) {
				printf("%s: %s\n", fsp->fs_spec,
				   	sys_errlist[errno]);
				stat = 1;
			}
		}
		endfsent();
		exit(stat);
	}
	do {
		if (syscall(VSWAPON, *argv++) == -1) {
			stat = 1;
			perror(argv[-1]);
		}
		argc--;
	} while (argc > 0);
	exit(stat);
}
