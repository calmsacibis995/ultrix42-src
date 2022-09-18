/*	ils.c -
 *		list a named file in inventory format.
 *
 *	usage: ils file [file...]
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
 *	000	29-apr-1989	ccb
 *		about a month old already. Running Lint
 *	001	24-jul-1989	ccb
 *		Include <sys/dir.h> for setld.h
 *		Merge changes by Tungning Cherng recognizing -f file.
 *			'file' here contains a list of files to expand into
 *			inventory records.
 *		More Lint
 *		Continue transition to fully mediated structure references
 *
*/

#ifndef lint
static	char *sccsid = "@(#)ils.c	4.1	(ULTRIX)	7/2/90";
#endif lint

#include	<sys/param.h>
#include	<sys/types.h>
#include	<sys/stat.h>
#include	<sys/dir.h>
#include	<errno.h>
#include	<string.h>
#include	<stdio.h>
#include	"setld.h"

extern int	errno;			/* errno(2) */
extern char	*sys_errlist[];		/* errno(2) */
extern int	optind;			/* getopt(3) */

void		exit();		/* exit(3) */

int	debug = 0;	/* debug flag */
int	fflg = 0;	/* a file contains files names */
char	*prog;		/* program name */

InvRecT		*i;	/* inventory record pointer */
InvT		*ip;	/* inventory pointer */
struct stat	stb;	/* stat(2) buffer */

main(argc, argv)
int argc;
char *argv[];
{
	int		c;		/* for use with getopt(3) */
	FILE		*fp;		/* for use with -f */
	char		line[132];	/* for use with -f */
	int		j;		/* for use with -f */

	prog = *argv;
	while( (c = getopt( argc, argv, "d:f" )) != EOF )
	{
		switch( c )
		{
		case 'd':
			++debug;
			break;
		case 'f':
			++fflg;
			break;
		default:
			(void) fprintf( stderr, "---usage: %s [-d] file...\n", prog );
			exit(1);
		}
	}

	ip = (InvT *) stdout;
	while( ++argv, --argc  )
	{
		if( fflg == 0 )
			getfile( *argv );	
		else
		{
			++argv; --argc;
			if( (fp = fopen(*argv, "r")) == NULL )
			{
				(void) fprintf( stderr,
					"%s: cannot open %s (%s)\n", prog,
					*argv, sys_errlist[errno] );
				exit(1);
			}
			while( fgets(line, sizeof(line),fp) != NULL )
			{
				for( j=0;
					line[j] != ' ' &&
					line[j] != '\t' &&
					line[j] !='\n'; j++)	
					;	
				line[j] = '\0';
				getfile(line);
			}
		}
	}
}

getfile(fn)
char *fn;
{
	if( lstat( fn, &stb ) )
	{
		(void) fprintf( stderr, "%s: cannot stat %s (%s)\n", prog,
			fn, sys_errlist[errno] );
		errno = 0;
		return;
	}
	if( debug )
	{
		(void) fprintf( stderr, "%s: debug: stb.st_size = %d\n", prog,
			stb.st_size );
	}

	/* generate an inventory record.
	 *  NOTE: this implemetation does not correctly handle hard
	 *   links. All hard links appear on the output as individual
	 *   file instances.
	*/
	i = StatToInv( &stb );
	if( debug )
	{
		(void) fprintf( stderr, "%s: debug: i->i_size = %d\n", prog,
			i->i_size );
	}
	i->i_flags = 0;
	(void) InvRecSetRev( i, "010" );
	(void) InvRecSetPath( i, fn );
	(void) InvRecSetSubset( i, "-" );
	(void) InvRecSetRef( i, "unknown" );

	/* check file type before generating checksum
	*/
	if( i->i_type == 'f' )
		i->i_sum = CheckSum( i->i_path );
	else
		i->i_sum = 0;

	(void) InvWrite( ip, i );
}

