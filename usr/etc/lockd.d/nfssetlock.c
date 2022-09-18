#ifndef lint
static char *sccsid = "@(#)nfssetlock.c	4.1	ULTRIX	7/2/90";
#endif lint


/************************************************************************
 *									*
 *			Copyright (c) 1986 by				*
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
 
/***********************************************************************
 *
 *		Modification History
 *
 * 13 Mar 88 -- chet
 *	Changed to use setsysinfo() call
 *
 *
 ***********************************************************************/


#include <stdio.h>
#include <sys/sysinfo.h>

main(argc, argv)
int	argc;
char	*argv[];
{

	/*
	 * must be superuser to run 
	 */

	if (geteuid() != 0){
		(void) fprintf(stderr, "nfssetlock:  must be super user\n");
		(void) fflush(stderr);
		exit(1);
	}

	**++argv;

	if (argc != 2 || (*argv)[0] != 'o') {
		(void) fprintf(stderr,"usage: nfssetlock {on, off}\n");
		exit (1);
	}

	switch ((*argv)[1]) {

		case 'n':
			nfslockon ();
			break;

		case 'f':
			nfslockoff ();
			break;

		default:
			(void) fprintf(stderr,"usage: nfssetlock {on, off}\n");
			exit(1);
			break;
	}
}

int arg[2] = {SSIN_NFSSETLOCK, 0};

nfslockon ()
{
	arg[1] = 1;
	if (setsysinfo(SSI_NVPAIRS, arg, 1, 0, 0))
		perror("nfssetlock - setsysinfo failed");
}

nfslockoff ()
{
	if (setsysinfo(SSI_NVPAIRS, arg, 1, 0, 0))
		perror("nfssetlock - setsysinfo failed");
}
