/*	umerge.c -
 *		propogate flags information from one inventory to
 *		another.
 *
 *	umerge inv1 inv2
 *		the flags information for records in inv2 is copied into
 *		the corresponding records of inv1. The merged records are
 *		written to stdout. Records in inv2 not present in inv1
 *		are dropped.
 *
 *			Copyright (c) 1989 by
 *		Digital Equipment Corporation, Maynard, MA
 *			All rights reserved.
 *								
 *	This software is furnished under a license and may be used and
 *	copied  only  in accordance with the terms of such license and
 *	with the  inclusion  of  the  above  copyright  notice.   This
 *	software  or  any  other copies thereof may not be provided or
 *	otherwise made available to any other person.  No title to and
 *	ownership of the software is hereby transferred.		
 *								
 *	The information in this software is subject to change  without
 *	notice  and should not be construed as a commitment by Digital
 *	Equipment Corporation.					
 *								
 *	Digital assumes no responsibility for the use  or  reliability
 *	of its software on equipment which is not supplied by Digital.
 *
 *	MODIFICATIONS:
 *
 *	000	10-mar-1989	ccb
 *	New for update installation
 *
 *	001	22-mar-1989	ccb
 *	Add debug switch. Debug and qualify.
 *
 *	002	29-apr-1989	ccb
 *	Lint.
 *	003	14-jun-1989	ccb
 *	004	24-jul-1989	ccb
 *		Lint
 *		include <sys/dir.h> for setld.h
*/

#ifndef lint
static	char *sccsid = "@(#)umerge.c	4.1	(ULTRIX)	7/2/90";
#endif lint

#include	<sys/param.h>
#include	<sys/types.h>
#include	<sys/dir.h>
#include	<errno.h>
#include	<stdio.h>
#include	<string.h>
#include	"setld.h"

extern void	exit();			/* exit(3) */
extern int	errno;			/* errno(2) */
extern char	*sys_errlist[];		/* errno(2) */
extern int	optind;			/* getopt(3) */

#define	LEFT	0	/* readable subscript */
#define	RIGHT	1	/* readable subscript */

int		debug = 0;	/* debug flag */
char		*fname[2];	/* file name pointers */
InvT		*inv[2];	/* input inventory pointers */
InvRecT		ir[2];		/* inventory record storage */
InvT		*out;		/* output inventory pointer */
char		*prog;		/* program name */

main(argc,argv)
int argc;
char *argv[];
{
	int	i;		/* index */
	int	c;		/* for use with getopt(3) */	

	prog = *argv;

	while( (c = getopt( argc, argv, "d" )) != EOF )
	{
		switch( c )
		{
		case 'd':
			++debug;
			break;
		default:
			(void) fprintf( stderr,
				"Usage: %s [-d] f1.inv f2.inv\n", prog );
			exit(1);
		}
	}
	if( debug )
	{
		(void) fprintf( stderr, "%s: debug: enabled\n", prog );
		(void) fprintf( stderr, "%s: optind %d\n", prog );
	}
	
	if( (argc - optind) != 2 )
	{
		(void) fprintf( stderr, "Usage: %s [-d] f1.inv f2.inv\n",
			prog );
		exit(1);
	}

	argv += optind;
	for( i = 0; i < 2; ++i )
	{
		/* store argv[i]
		*/
		fname[i] = argv[i];
		if( (inv[i] = InvOpen( argv[i], "r" )) == NULL )
		{
			(void) fprintf( stderr, "%s: cannot open %s (%s)\n",
				prog, argv[i], sys_errlist[errno] );
			exit(1);
		}
		/* read initial record from i
		*/
		if( debug )
		{
			(void) fprintf( stderr, "%s: debug: File %d: %s\n",
				prog, i, fname[i] );
		}
		GetRecord( i );
	}

	out = InvInit( stdout );

	for( ;; )	/* Loop exit thru GetRecord() */
	{
		/* compare path fields
		*/
		if( (i = strcmp( ir[LEFT].i_path, ir[RIGHT].i_path )) == 0 )
		{
			/* pathname appears in both inventories
			 *  copy flags field from RIGHT to LEFT
			 *  write updated left record
			 *  get new LEFT record
			 *  get new RIGHT record
			*/
			ir[LEFT].i_flags = ir[RIGHT].i_flags;
			(void) InvWrite( out, &ir[LEFT] );
			GetRecord( LEFT );
			GetRecord( RIGHT );
		}
		else if( i < 0 )
		{
			/* in LEFT, absent from RIGHT.
			 *  Write LEFT record
			 *  read new LEFT record
			*/
			(void) InvWrite( out, &ir[LEFT] );
			GetRecord( LEFT );
		}
		else
		{
			/* in RIGHT, absent from LEFT.
			 *  Read new RIGHT record (discards old record)
			*/
			GetRecord( RIGHT );
		}
	}
}



/*	GetRecord() -
 *		Fetch a record from one of the input files
 *
 *	given:	int i - the index of the file to read from
 *	does:	fills global ir[i] with the information gotten from
 *		file inv[i].
 *	return:	nothing
*/


GetRecord(i)
int i;
{
	static PathT	sprev[2] =
	{
		{ '\0' },
		{ '\0' }
	};	/* storage for path field in record .-1 */

	InvRecT	*p;	/* pointer to static data in InvRead() */

	if( (p = InvRead( inv[i] )) == NULL ) /* end of file */
	{
		/* file read has encountered end-of-file.
		 *  call finish() to process the rest of the
		 *  other file.
		*/
		Finish( !i );	/* does not return */
	}

	/* copy static InvRecT into local InvRecT */
	(void) InvRecCopy( &ir[i], p );
	if( strcmp( sprev[i], ir[i].i_path ) > 0 )
	{
		/* input file not in ascending sort
		 *  on path name. This is a fatal error.
		*/
		(void) fprintf( stderr, "%s: %s corrupt at %s\n",
			prog, fname[i], ir[i].i_path );
		exit(1);
	}
	/* store current path as previous path
	*/
	(void) PathSet( sprev[i], ir[i].i_path );
}



/*	Finish() -
 *		write out remainder of input inventory
 *
 *	given:	int i - index of file which needs to be finished
 *	does:	transfers indicated file contents to stdout iff the
 *		indicated file is file 0.
 *	return:	Never Returns
*/

Finish(i)
int i;
{
	InvRecT		*p;	/* inventory record pointer */

	if( i == RIGHT )
	{
		/* this indicates that no more LEFT records
		 *  are available. Exit immediately.
		*/
		exit( 0 );
	}

	p = &ir[i];
	do
	{
		/* Write out current LEFT record, continue
		 *  to read and write LEFT records until
		 *  end of LEFT file...
		*/
		(void) InvWrite( out, p );
	}
	while( (p = InvRead( inv[i] )) != NULL );
	/* ...and exit.
	*/
	exit(0);
}



/*END*/
