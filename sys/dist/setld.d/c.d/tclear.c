/*
 *	tclear.c -
 *		prepare a system for subset extraction with tar.
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
 *	000	31-aug-1989	ccb
 *		New
*/

#ifndef lint
static	char *sccsid = "@(#)tclear.c	4.1	(ULTRIX)	7/2/90";
#endif lint

/*	tclear < x.inv
 *		clears system of files which have the same names as files
 *	about to be extracted from distribution but have undesirable
 *	file types. These types are s, l, c, and b and are undesirable
 *	because tar cannot overwrite them to produce files which represent
 *	the archive on the distribution.
 *
 *	The clearing is done by reading the pathnames from the inventory
 *	records, lstat(2)ing the path and checking the file type ON DISK.
 *	S_IFLNK, S_IFCHR, and S_IFBLK are removed immediately. Any S_IFREG
 *	with a link count in excess of 1 is also removed.
*/

#include	<sys/param.h>
#include	<sys/dir.h>
#include	<sys/stat.h>
#include	<errno.h>
#include	<stdio.h>
#include	<setld.h>

extern int	errno;		/* errno(2) */
extern char	*sys_errlist[];	/* errno(2) */

/*! these definitions should be replaced when the appropriate routines
 *  become available in the library
*/
#define	InvRecGetPath(p)	((p)->i_path)
#define	InvRecGetType(p)	((p)->i_type)

#define	S_ISLNK(mode)	(((mode) & S_GFMT) == S_IFLNK)
#define M		stb.st_mode
#define	NLINK		stb.st_nlink

char *prog;

main( argc, argv )
int argc;
char **argv;
{
	InvT		*isp;	/* inventory stream pointer */
	InvRecT		*irp;	/* inventory record pointer */
	struct stat	stb;	/* stat buffer */
	int		errs;	/* error counter */

	prog = *argv;
	isp = InvInit( stdin );
	errs = 0;
	while( (irp = InvRead( isp )) != NULL )
	{
		/*! should use mediated references !*/
		if( lstat( InvRecGetPath( irp ), &stb ) != 0 )
		{
			/* file does not exist, no conflict is possible
			*/
			continue;
		}

		if( S_ISBLK(M) || S_ISCHR(M) || S_ISFIFO(M) || S_ISLNK(M) || 
			(!S_ISDIR(M) && NLINK > 1 ) )
		{
			/* file is one of b, c, p, s OR is linked to
			 *  something. Remove the file.
			*/
			if( unlink( InvRecGetPath( irp ) ) != 0 )
			{
				fprintf( stderr, "%s: cannot unlink %s (%s)\n",
					prog, irp->i_path, sys_errlist[errno] );
				++errs;
			}
		}
		else if( S_ISDIR(M) && InvRecGetType( irp ) != 'd' )
		{
			/* there is a directory on the disk in a place where
			 *  we wish to install something else
			*/
			fprintf( stderr,
				"%s: cannot install %s (%c) on a directory\n",
				prog, InvRecGetPath( irp ),
				InvRecGetType( irp ) );
			exit(1);
		}
	}
	exit( errs );
}
