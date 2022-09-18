#ifndef lint
static  char    *sccsid = "@(#)zeero.c	4.1 (ULTRIX) 7/2/90";
#endif lint

/* *********************************************************************
*                                                                      *
*                      Copyright (c) 1986 by                           *
*              Digital Equipment Corporation, Maynard, MA              *
*                      All rights reserved.                            *
*                                                                      *
*   This software is furnished under a license and may be used and     *
*   copied  only  in accordance with the terms of such license and     *
*   with the  inclusion  of  the  above  copyright  notice.   This     *
*   software  or  any  other copies thereof may not be provided or     *
*   otherwise made available to any other person.  No title to and     *
*   ownership of the software is hereby transferred.                   *
*                                                                      *
*   The information in this software is subject to change  without     *
*   notice  and should not be construed as a commitment by Digital     *
*   Equipment Corporation.                                             *
*                                                                      *
*   Digital assumes no responsibility for the use  or  reliability     *
*   of its software on equipment which is not supplied by Digital.     *
*                                                                      *
********************************************************************** */

/* **************************************************************
*  HISTORY							*
*								*
*	01-Feb-1988						*
*	Created by Jon Wallace					*
*								*
*************************************************************** */

/* ******************************************* 
ZEERO.C
Program to zero out disks prior to duplication
********************************************** */

#include	<errno.h>
#include	<stdio.h>
#include	<string.h>
#include	<sys/file.h>
#include	<sys/types.h>
#include	<sys/stat.h>

#define		ON		1
#define		OFF		0
#define		BYTESIZE	8192	

extern		int		errno;




/* **********************
Program ZEERO starts here
************************* */

main(argc,argv)

	int 	argc;
	char	*argv[];
{




/* ************** 
Declare variables 
***************** */


	static char	*err = "ZEERO_ERROR ";
	char		buffer[BYTESIZE];
	int		ans;
	int 		fp1;
	int		xstat;
	int		fflag = ON;
	struct		stat	status;




/* ***************************************************************
Check to see if ZEERO is being used interactively, or being called
from another program.  If interactive, turn off the force flag and
leave command arguments as they are,  else leave force flag on but
make command argument[2], become command argument[1].
****************************************************************** */

	if ((strcmp(argv[1],"-f")) != 0)
		{
		fflag = OFF;
		}
	else
		++argv;




/* ******* 
Initialize 
********** */

	bzero(buffer,BYTESIZE);
	fp1 = open (argv[1], O_WRONLY, 0 );




/* ***************************************************************** 
Check status of file input by user to make sure it is a SPECIAL FILE
******************************************************************** */

	fstat(fp1, &status);

	if (errno != 0)
		{
		perror(err);
		exit(1);
		}
	else
		{
		xstat = (status.st_mode & S_IFMT);
		if ((xstat != S_IFCHR) && (xstat != S_IFBLK))
			{
			printf ("\n\n");
			printf ("ERROR: %s is not a valid ", argv[1]);
			printf ("Device Special File \n\n\n");
			exit(1);
			}
		}




/* *****************************************************
Check to see if the Force (-f) flag is on or off.  If it
is off, this program is being used interactively, so ask
the user to confirm that they want to zero out the disk.
******************************************************** */

	if (fflag == OFF)
		{
		printf ("\n\n******** W A R N I N G ******** \n\n");
		printf ("This program will completely erase all data ");
		printf ("on %s \n\n", argv[1]);
		printf ("Please confirm intent to ZEERO ");
		printf ("%s  (y/n) : ", argv[1]);
		ans = getchar();
		printf (" \n\n");
		if ((ans == 'y') || (ans == 'Y'))
			{
			;
			}
		else
			exit(0);
		}




/* **************************************************** 
File must be ok, so start writing zero's to it until we
get an error, then check the error to make sure it is a
ENOSPC 28 from errno.h, which means the disk ran out of
space.
******************************************************* */

	printf ("Cleaning %s.....", argv[1]);
	fflush (stdout);

	while ( write (fp1, buffer, BYTESIZE) > 0 )
		{
		;
		}

	if (errno == ENOSPC)
		{
		printf ("done. \n");
		exit(0);
		}
	else
		perror(err);

	close (fp1);
}




