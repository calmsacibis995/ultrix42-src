#ifndef lint
static	char	*sccsid = "@(#)chroot.c	4.1  (Ultrix)   7/17/90";
#endif lint
/************************************************************************
 *									*
 *			Copyright (c) 1989 by				*
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
 *   This software is  derived  from  software  received  from Bell	*
 *   Laboratories.  Use, duplication, or disclosure is  subject  to	*
 *   restrictions  under  license  agreements  with AT&T.		*
 *									*
 *   The information in this software is subject to change  without	*
 *   notice  and should not be construed as a commitment by Digital	*
 *   Equipment Corporation.						*
 *									*
 *   Digital assumes no responsibility for the use  or  reliability	*
 *   of its software on equipment which is not supplied by Digital.	*
 *									*
 ************************************************************************/
/************************************************************************
 *			Modification History				*
 * 001 Richard Hart, Apr. 13, 1987					*
 *     Copied from SysvR2 code:						*
 *	@(#)chroot.c	1.1						*
 ************************************************************************/

# include <stdio.h>
/* chroot */

main(argc, argv)
char **argv;
{
	if(argc < 3) {
		printf("usage: chroot rootdir command arg ...\n");
		exit(1);
	}
 	argv[argc] = 0; 
	/* the following lines were in the SysV source.  They don't seem  */
	/* useful or necessary in Ultrix, so they are commented out until */
	/* we can discover why they are there (note the useful SysV       */
	/* comment!) or to be sure Ultrix doesn't need them.		  */

/*	if(argv[argc-1] == (char *) -1) */ /* don't ask why */
/*		argv[argc-1] = (char *) -2; */
	if (chroot(argv[1]) < 0) {
		perror(argv[1]);
		exit(1);
	}
	if (chdir("/") < 0) {
		printf("Can't chdir to new root\n");
		exit(1);
	}
	execv(argv[2], &argv[2]);
	close(2);
	open("/dev/tty", 1);
	printf("%s: not found\n",argv[2]);
	exit(1);
}
