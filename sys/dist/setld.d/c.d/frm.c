/*	frm.c -
 *
 *	frm [-c] < inventory
 *		all files listed in input inventory are unlinked from the
 *		system. directories are left untouched. used by setld during
 *		deletes and updates. If -c is specified, the checksum of the
 *		file must match the checksum in the inventory for the file
 *		to be removed.
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
 *	Modification History:
 *
 *	000	19-apr-1989	ccb
 *		First pool integration - X4.0-2
 *	001	24-jul-1989	ccb
 *		Include <sys/dir.h>, required for setld.h
 *		Lint.
*/

#ifndef lint
static	char *sccsid = "@(#)frm.c	4.1	(ULTRIX)	7/2/90";
#endif lint

#include	<sys/param.h>
#include	<sys/types.h>
#include	<sys/stat.h>
#include	<sys/dir.h>
#include	<errno.h>
#include	<stdio.h>
#include	<string.h>
#include	"setld.h"

#define	YES	1	/* affirmative */
#define	NO	0	/* negative */

int	checksums = 0;	/* checksum flag */
int	debug = 0;	/* debug flag */
char	*prog;		/* program name */

main(argc,argv)
int argc;
char *argv[];
{
	InvT		*ip;	/* input inventory pointer */
	InvRecT		*irp;	/* input record pointer */
	int		c;	/* getopt(3) */

	prog = *argv;
	while( (c = getopt( argc, argv, "cd" )) != EOF )
	{
		switch(c)
		{
		case 'c':
			++checksums;
			break;
		case 'd':
			++debug;
			break;
		default:
			(void) fprintf( stderr, "usage: %s [-c] < x.inv\n",
				prog );
			exit(1);
		}
	}

	if( debug )
	{
		(void) fprintf( stderr, "%s: debug: debug enabled\n", prog );
		(void) fprintf( stderr, "%s: debug: checksums %sabled\n", prog,
			(checksums) ? "en" : "dis" );
	}

	ip = InvInit( stdin );
	while( (irp = InvRead( ip )) != NULL )
	{
		if( debug )
		{
			(void) fprintf( stderr, "%s: debug: input: ", prog );
			(void) InvWrite( stderr, irp );
		}
		if( CanDelete( irp ) )
		{
			if( debug )
			{
				(void) fprintf( stderr,
					"%s: debug: unlinking %s\n", prog,
					irp->i_path );
			}
			(void) unlink( irp->i_path );
		}
	}
	exit(0);
}


/*	CanDelete() -
 *		determine if a file can be deleted.
 *
 *	given:	InvRecT *p - a pointer to an inventory record for the file
 *	does:	assure the file exists (non-existent files cannot be deleted)
 *		assure that the file is not a directory
 *		if the checksums flag is set, assures that the checksum in
 *			the inventory matches the checksum in the inventory.
 *	return:	0 - file cannot be deleted
 *		1 - file can be deleted
*/

CanDelete( p )
InvRecT *p;
{
	struct stat	stb;	/* stat info buffer */

	if( lstat( p->i_path, &stb ) )
	{
		/* do not try to delete non-existent files
		*/
		return( NO );
	}

	if( S_ISDIR( stb.st_mode ) || p->i_type == 'd' )
	{
		/* a directory, leave it alone
		*/
		return( NO );
	}
	if( S_ISBLK( stb.st_mode ) ||
		S_ISCHR( stb.st_mode ) || index( "Dbc", p->i_type ) != NULL )
	{
		/* device file - checksum checks do not make
		 *  sense when applied to devices, the file can
		 *  be deleted
		*/
		return( YES );
	}

	if( checksums )
	{
		/* verify that the file being deleted is the one we installed
		 *  check the size and mtime first before springing for the
		 *  expense of a checksum
		*/
		if( stb.st_size != p->i_size )
		{
			/* different size
			*/
			return( NO );
		}
		
		if( strcmp( p->i_date, DateFormat( stb.st_mtime ) ) )
		{
			/* different modification time
			*/
			return( NO );
		}

		/* $$$
		*/
		if( CheckSum( p->i_path ) != p->i_sum )
		{
			/* different checksum
			*/
			return( NO );
		}
	}
	/* either it's the same file we installed or we just don't
	 *  care, remove it
	*/
	return( YES );
}

/*END*/
