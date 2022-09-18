/*	fsmount.c -
 *		generate `affected system' info from inv list.
 *
 *			Copyright (c) 1985, 1989, 1990 by
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
 *	Design and Implementation Notes:
 *
 *			RECORD ALL CHANGES HERE
 *
 *	Function:
 *		This program determines which filesystems contain files
 *	named in inv(5) records listed on stdin. For each filesystem
 *	containing files, a mount command is written to stdout which
 *	can be used to mount all affected filesystems. Exceptions: a
 *	mount command is not written for the / filesystem.
 *
 *	000	15-aug-1990	ccb
 *		New. Culled from fitset.c.
*/

#ifndef lint
static	char *sccsid = "@(#)fsmount.c	4.1 (ULTRIX) 9/11/90";
#endif lint

#include	<sys/param.h>
#include	<sys/types.h>
#include	<sys/fs_types.h>
#include	<sys/mount.h>
#include	<sys/dir.h>
#include	<sys/stat.h>
#include	<stdio.h>
#include	<string.h>
#include	"setld.h"

extern char	*getenv();		/* environ(7) */

extern int	errno;			/* errno(2) */
extern char	*sys_errlist[];		/* errno(2) */
extern int	optind;			/* getopt(3) */


/*	file system table data structure for tracking
 *	available system space
*/
#define	FS_INUSE	0x0001
#define	FSUSED(a)	((a->fs_flags&FS_INUSE)==FS_INUSE)

typedef struct fstable {
	struct fstable	*fs_next;
	FlagsT		fs_flags;
	PathT		fs_path;
	int		fs_pathlen;
	PathT		fs_devname;
} FsT;

static FsT	*FsFind();	/* FsT list search routine */
static int	FsInit();	/* initialize from struct fs_data */
static FsT	*FsInsert();	/* FsT list insert routine */
static FsT	*FsNew();	/* allocate storage */

#define	DEBUG	0x0001
#define	CURFS	fbuf[nfsys]

static struct fs_data	fbuf[NMOUNT];	/* storage for mount points */
static int		flags = 0;	/* run flags */
static struct stat	stb;

char		*prog;


main(argc,argv)
int argc;
char *argv[];
{
	FsT		*hp = NULL;
	FsT		*np;		/* list and node pointers */
	PathT		root;		/* root path */
	int		rootlen;	/* length of root path name */
	int		c;		/* for use w/getopt(3) */
	int		loc = 0;	/* index for getmnt() */
	int		nfsys;		/* # of mounts */
	InvT		*ip;		/* Inventory Pointer */
	InvRecT		*irp;		/* inventory record pointer */

	*gt_names[GT_UNKWN] = 'u';	/* LINTHACK */

	prog = *argv;

	/* get root path from command line */
	while( (c = getopt( argc, argv, "d" )) != EOF )
	{
		switch( c )
		{
		case 'd':
			flags |= DEBUG;
			break;
		default:
			exit(2);
		}
		--argc;
	}
	argv += optind;

	*root = '\0';
	rootlen = 0;
	if( argc > 1 )
	{
		if( **argv != '/' )
		{
			(void) fprintf(stderr,
				"%s: root path must be absolute\n", prog );
			exit(2);
		}
		/* make sure rootpath exists */
		if( stat(*argv,&stb) )
		{
			(void) fprintf(stderr,"%s: cannnot stat %s (%s)\n",
				prog, *argv, sys_errlist[errno]);
			exit(2);
		}
		/* make sure rootpath is a directory */
		if( !S_ISDIR( stb.st_mode ) )
		{
			(void) fprintf(stderr,
				"%s: %s not a directory (mode %o)\n",
				prog, *argv, stb.st_mode );
			exit(2);
		}
		/* all clear, store it */
	
		if( strcmp( *argv, "/" ) )
		{
			/* non-'/' path specified, store it. If
			 *  '/' was specified, the empty string is used.
			*/
			(void) strcpy(root,*argv);
		}
		rootlen = strlen(root);
	}


	nfsys = getmountent(&loc, fbuf, NMOUNT);

	if( flags & DEBUG )
		(void) printf( "root = %s\nrootlen = %d\nnfsys = %d\n",
			root, rootlen, nfsys );

	/* cull out all of the ufs filesystems below rootpath and
	 *  queue them up into the 'fstable' list.
	*/
	while( --nfsys >= 0 )
	{
		/* is this a ufs? */
		if( CURFS.fd_fstype == GT_ULTRIX )
		{
			statfs( CURFS.fd_path, &CURFS );

			/* build an fstable entry */
			np = FsNew();
			FsInit( np, &(CURFS) );
			hp = FsInsert( np, hp );
		}
	}

	if( flags & DEBUG )
	{
		(void) printf( "Finished UFS List\n" );
		FsShow( hp, -1 );
	}

	/* run down the inventory on stdin */
	ip = InvInit( stdin );
	while( (irp = InvRead( ip )) != NULL )
	{
		if( flags & DEBUG )
			(void) printf( "s=%d\tt=%c\tp=%s\n", irp->i_size,
				irp->i_type, irp->i_path );

		/* concatenate everything in the inventory pathname after
		 *  the dot onto the end of rootpath. This is does not
		 *  effect the rootpath itself
		*/
		(void) strcpy( (root + rootlen), (irp->i_path + 1) );
		if( flags & DEBUG )
		{
			(void) printf( "Relative File:\n\t%s\n", root );
		}

		/* locate the filesystem this file lives on */
		if( (np = FsFind( root, hp )) == NULL )
		{
			/* this should never happen!?! */
			(void) fprintf(stderr,
				"%s: cannot find file system for %s\n", prog,
				irp->i_path );
			continue;
		}

		if( flags & DEBUG )
		{
			(void) printf( "Located on Filesystem Node\n" );
			FsShow( np, 1 );
		}

		/* mark file system as in use
		*/
		np->fs_flags |= FS_INUSE;

	}
	/* print mount commands
	*/
	for( np = hp; np != NULL; np = np->fs_next )
	{
		if( FSUSED(np) && (strcmp( np->fs_path, "/" ) != 0) )
			(void) printf( "/etc/mount %s %s\n", np->fs_devname,
				np->fs_path );
	}

	exit(0);
}


/*	static FsT *FsFind() -
 *		searches for file system on which a file is to reside.
 *
 *	given:	PathT p - a pathname
 *		FsT *fsp - a pointer into the file system table (a list)
 *	does:	find the table entry representing the file system the
 *		pathname is located on.
 *	return:	a pointer to the entry, NULL if the entry wasn't found
*/

static FsT *FsFind( p, fsp )
PathT p;
FsT *fsp;
{
	if( fsp == NULL )	/* could not locate file system */
	{
		return( NULL );
	}

	/* does the current mount point fit inside the pathname? */
	if( !strncmp( p, fsp->fs_path, fsp->fs_pathlen ) )
	{
		/* found it */
		return( fsp );
	}

	/* look further down the list */
	return( FsFind( p, fsp->fs_next ) );
}


static FsInit( p, fs ) 
FsT *p;
struct fs_data *fs;
{
	(void) PathSet( p->fs_path, fs->fd_path );
	p->fs_pathlen = strlen(p->fs_path);
	(void) PathSet( p->fs_devname, fs->fd_devname );

	p->fs_flags = 0;
	p->fs_next = NULL;

	if( flags & DEBUG )	/* print out free info */
	{
		(void) printf( "dev %s mounted on %s\n", p->fs_devname,
			p->fs_path );
		(void) printf( "New UFS Node:\n" );
		FsShow( p, 1 );
	}
}



/*	static FsT *FsInsert() -
 *		link a new fsnode into linked list in reverse alpha order.
 *
 *	RECURSION
*/

static FsT *FsInsert( s, t )
FsT *s, *t;
{
	if( t == NULL )	/* end of list, return */
		return( s );

	if( strcmp( s->fs_path, t->fs_path ) < 0 )
	{
		/* entry belongs further down the list */
		t->fs_next = FsInsert( s, t->fs_next );
		return( t );
	}
	else
	{
		/* entry should be inserted at head of list */
		s->fs_next = t;
		return( s );
	}
}



/*	static FsT *FsNew() -
 *		allocate storage for an FsT
 *
 *	given:	void
 *	does:	allocate storage for an FsT
 *	return:	a pointer to the FsT, NULL on error
*/

static FsT *FsNew()
{
	return( (FsT *) malloc( sizeof(FsT) ) );
}


/*	FsShow() -
 *		dump out node list.
 *
 *	RECURSION
*/

FsShow( p, n )
FsT *p;
int n;
{
	if( p == NULL || n == 0 )
		return;

	(void) printf( "\tNode Name:\t%s\n\tmntpt:\t%s\n\tdevice:\t%s\n",
			p->fs_path, p->fs_devname );
	FsShow( p->fs_next, n-1 );
}


/*END*/

