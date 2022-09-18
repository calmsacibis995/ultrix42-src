/*
 *                                                                      *
 *                      Copyright (c) 1989 by                           *
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
 *  usync -
 *	Compares records of setld(8) inventory files for update
 *	installations. 
 *
 *  Arguments-
 *	Two inventory files included on command line.
 *	usync t.inv (t+1).inv
 *		merges inventory records so that the output
 *	inventory reflects the most recent state of the system
 *
 *
 *  HISTORY
 *
 *	000 - 14-Feb-1989
 *	Created by Jon Wallace
 *
 *	001	21-mar-1989	ccb
 *		add debug switch

 *	002	24-jul-1989	ccb
 *		lint

 *	003	17-oct-1989	ccb
 *		fix problems reading old style inventories with non-numbers
 *		in the flags field.
*/

#ifndef lint
static  char *sccsid = "@(#)usync.c	4.1	(ULTRIX)	7/2/90";
#endif lint

#include	<sys/param.h>
#include	<sys/file.h>
#include	<sys/dir.h>
#include	<errno.h>
#include	<string.h>
#include	<stdio.h>
#include	<setld.h>

extern void	exit();		/* exit(3) */
extern void	perror();	/* exit(3) */

extern int	errno;		/* errno(2) */
extern char	*optarg;	/* getopt(3) */
extern int	opterr;		/* getopt(3) */
extern int	optind;		/* getopt(3) */

#define	OFF	0
#define	SET	1
#define	LEFT	0
#define	RIGHT	1


int	cmp_result;			/* strcmp result */
int	debug = 0;			/* debug flag */
char    fmtstr[2][MAXPATHLEN+1];	/* format string buffer */
InvT    *fp[2];				/* input file pointers */
int	i;				/* array index */
char	*infile[2];			/* input file names */
InvT	*ipout;				/* output pointer */
InvRecT	irp[2];				/* inventory records */
char	prestr[2][MAXPATHLEN+1];	/* previous format string buffer */
char	*prog;				/* command line arguments */ 

int	toggle[] = {1,0};	/* 0 -> 1, 1 -> 0 reverse switch */
int	wrtflg[] = {1,1};	/* was buffer[] written out */

main(argc,argv)
int     argc;
char    *argv[];
{
	int	c;		/* command switch char for getopt */

	/* Make sure there are two files included in the command
	 *  line, then try to open both of them.  If successful open, go
	 *  get the first record of each of the files.
	*/

	prog = *argv;
	while( (c = getopt( argc, argv, "d" )) != EOF )
	{
		switch( c )
		{
		case 'd':
			++debug;
			break;
		default:
			(void) fprintf( stderr, "Usage: %s [-d] file1 file2\n",
				prog );
			exit(1);
		}
	}
	/* make sure the 2 filename args are available */
	if( (argc - optind) != 2 )
	{
		(void) fprintf( stderr, "Usage: %s [-d] file1 file2\n", prog );
		exit(1);
	}
	/* adjust argv wrt the results of getopt() */
	argv += optind;

	for (i=0; i < 2; ++i)
	{
		if( (fp[i] = InvOpen( argv[i], "r" )) == NULL )
		{
			perror(argv[1]);
			exit (1);
		}
		infile[i] = argv[i];
		getfile(i);
	}


	/* Compare the filename field of each record, of each file, against
	 *  each other using strcmp().  Follow the actions within the
	 *  decision making loop to determine whether or not to write the
	 *  record out to stdout.
	*/
	prestr[0][0] = prestr[1][0] = '\0';
	ipout = InvInit( stdout );
	for(;;)
	{
		if( debug )
		{
			(void) fprintf( stderr, "l:%s\tr:%s\n", fmtstr[0],
				fmtstr[1] );
		}

		/* compare the pathname fields from each file */
		if( (cmp_result = strcmp(fmtstr[LEFT], fmtstr[RIGHT])) == 0 )
		{
			/* both names match, sync by writing right hand
			 *  record and reading left and right
			*/
			(void) InvWrite( ipout, &(irp[RIGHT]) );
			getfile(LEFT);
			getfile(RIGHT);
		}
		else if (cmp_result < 0)
		{
			/* left file is less than right file, write out
			 *  left record and read new one.
			*/
			(void) InvWrite( ipout, &(irp[LEFT]) );
                	getfile(LEFT);
		}
		else
		{
			/* only one alternative: right pathname is less than
			 *  the left pathname. write out the right record and
			 *  read in a new one.
			*/
			(void) InvWrite( ipout, &(irp[RIGHT]) );
                	getfile(RIGHT);
		}
	}
}


/*	getfile() -
 *		Pulls a record from a file until EOF, then uses
 *	sscanf to select the filename field from each record.
 *
 *	Passed : Integer (i), which represents the index of variable
 *		 arrays.
 *
 *	Return : nothing.
 *
*/

getfile(j)
int	j;
{
	InvRecT		*trp;

	--wrtflg[j];
	if( (trp = InvRead( fp[j] )) == NULL )
	{
		if ((wrtflg[toggle[j]] == SET) && (cmp_result != 0))
			(void) InvWrite( ipout, &(irp[toggle[j]]) );
		finish(toggle[j]);
	}
	else
	{
		++wrtflg[j];
		(void) strcpy( fmtstr[j], trp->i_path );
		if ((cmp_result = strcmp(prestr[j], fmtstr[j])) > 0)
		{
			(void) fprintf(stderr, "%s: %s is corrupt\n",
				prog, infile[j]);
			exit (1);
		}
	}
	(void) InvRecCopy( &(irp[j]), trp );
	(void) strcpy(prestr[j], fmtstr[j]);
}


/*	finish() -
 *		Finishes writing records from one file to stdout, after 
 *	the other file has reached EOF, and comparing the strings is no
 *	longer necessary.
 *
 *	Passed : Integer (j), which represents the index for variable
 *		 arrays.
 *
 *	Return : Nothing.
 * 
*/

finish(j)
int j;
{
	InvRecT	*trp;

	while( (trp = InvRead( fp[j] )) != NULL )
		(void) InvWrite( ipout, trp );

	exit (0);
}



