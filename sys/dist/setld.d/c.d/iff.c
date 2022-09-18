/*	iff.c
 *		inventory flag filter
 *
 *	iff -pv < x.inv
 *		filter inventory records on the flag field. -p passes
 *	records with precedence flag set, -v passes records with volitile
 *	flag set.
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
 *	000	21-feb-1989	ccb
 *		new for work on update installation
 *
 *	001	29-apr-1989	ccb
 *		lint.
 *	002	change constants (INVPREC..,etc) to match name changes in
 *		setld.h
 *	003	24-jul-1989	ccb
 *		include <sys/dir.h> for setld.h
 *		more lint
*/

#ifndef lint
static	char *sccsid = "@(#)iff.c	4.1	(ULTRIX)	7/2/90";
#endif lint

#include	<sys/param.h>
#include	<sys/types.h>
#include	<sys/dir.h>
#include	<errno.h>
#include	<stdio.h>
#include	"setld.h"

extern int	errno;			/* errno(2) */
extern char	*sys_errlist[];		/* errno(2) */

void	exit();		/* lint. */

int	 debug = 0;	/* debug flag */
char	*prog;		/* pointer for program name */

main( argc, argv )
int argc;
char *argv[];
{
	int	argmask = 0;	/* mask of bits to search for */
	int	c;		/* option char */
	InvT	*ip;		/* input pointer */
	InvT	*op;		/* output pointer */
	InvRecT	*irp;		/* current record pointer */

	prog = *argv;
	/* parse argument switches
	*/
	while( (c = getopt( argc, argv, "dpv" )) != EOF )
	{
		switch( c )
		{
		case 'd':
			++debug;
			break;
		case 'p':
			argmask |= INVPRECEDENCE;
			break;
		case 'v':
			argmask |= INVVOLITILE;
			break;
		default:
			(void) fprintf( stderr,
				"usage: %s [-dpv] < x.inv", prog );
			exit(1);
		}
	}
	if( debug )
	{
		(void) fprintf( stderr, "%s: debug: debug enabled\n", prog );
		(void) fprintf( stderr, "%s: debug: argmask = 0x%x\n",
			prog, argmask );
	}

	ip = InvInit( stdin );
	op = InvInit( stdout );
	while( (irp = InvRead( ip )) != NULL )
	{
		if( debug )
		{
			(void) fprintf( stderr, "%s: debug: ", prog );
			(void) InvWrite( stderr, irp );
		}
		/* if inventory record flags have flags specified on
		 * command line, output the record
		*/
		if( argmask & irp->i_flags )
			(void) InvWrite( op, irp );
	}
	exit(0);
}


/*END*/
