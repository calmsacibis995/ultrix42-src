/*	invcutter.c --
 *		generate full inventory from MI records and filesystem data.
 *
 *			Copyright (c) 1985, 1989 by
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
 *	Overall algorithm:
 *		reads master inventory records (MiRecT) from stdin.
 *		with each record, gather all file statistics and fill
 *		out an inventory record (InvRecT). For files with
 *		link counts greater than 1, call LinkResolve() to
 *		resolve hard links.
 *
 *	Mods:
 *		2.0,1	ccb	14-jan-1987
 *		Unify directory type to compensate for tar change leaving
 *			empty dirs off tape.
 *
 *	001	14-feb-1989	ccb
 *		25 months later....
 *		Clean Up to use underlying routines done for update install.
 *		Guarantee perfect sort order for use by update routines.
 *
 *	002	29-apr-1989	ccb
 *		lint.
 *
 *	003	14-jun-1989	ccb
 *		remove a few minor system dependencies
 *	004	24-jul-1989	ccb
 *		Include <sys/dir.h> for setld.h
 *		More Lint.
 *		Minor changes for libsetld.a conformance
*/

#ifndef lint
static char *sccsid = "@(#)invcutter.c	4.1	(ULTRIX)	7/2/90";
#endif lint

#include	<sys/param.h>
#include	<sys/stat.h>
#include	<sys/dir.h>
#include	<errno.h>
#include	<string.h>
#include	<stdio.h>
#include	"setld.h"

extern char	*sys_errlist[];	/* errno(2) */
extern int	errno;		/* errno(2) */
extern char	*optarg;	/* getopt(3) */
extern int	optind, opterr;	/* getopt(3) */

extern void	exit();		/* exit(3) */

/*	Command Flags
*/
#define	VERSION	0x0001
#define	MNTPATH	0x0002
#define	DEBUGON	0x0004

#define	Usage()	(void) fprintf(stderr,"Usage: %s [-v vcode][-f mntpath]\n",prog)
#define	DEBUG()	(flags & DEBUGON)

typedef struct ResQT {
	struct ResQT	*rq_next;
	char		rq_path[MAXPATHLEN+1];
	dev_t		rq_dev;
	ino_t		rq_ino;
	short		rq_nlink;
} ResQT;

static ResQT	*LinkSearch();
static ResQT	*ResQCreate();
static ResQT	*ResQNew();

unsigned	flags;		/* program command flags */
char 		*prog;		/* program name */
char		*version;	/* version code */
ResQT		*links = NULL;	/* hard link resolution queue */


main(argc,argv)
int argc;
char *argv[];
{
	InvT		*i;		/* output inventory */
	InvRecT		*ip;		/* record under contruction */
	PathT		lastpath;	/* last path seen */
	MiT		*mi;		/* input master inventory */
	MiRecT		*mip;		/* pointer to current record */
	int		opt;		/* for use w/getopt() */
	char		*path;		/* tmp path pointer */
	short		rn;		/* input record number */
	struct stat	stb;		/* struct for stat ops */

	prog = *argv;

	version = "010";
	while( (opt = getopt( argc, argv, "df:v:" )) != EOF )
	{
		switch( opt )
		{
		case 'd':
			flags |= DEBUGON;
			break;
		case 'f':
			if( chdir( optarg ) )
			{
				(void) fprintf(stderr,
					"%s: cannot chdir to %s (%s)\n", prog,
					optarg, sys_errlist[errno] );
				exit(1);
			}
			break;
		case 'v':
			version = optarg;
			flags |= VERSION;
			break;
		default:
			Usage();
			exit(1);
		}
	}
	if( optind != argc )	/* pick up unflagged mountpath */
	{
		if( chdir( argv[optind] ) )
		{
			(void) fprintf(stderr, "%s: cannot chdir to %s (%s)\n",
				prog, optarg, sys_errlist[errno] );
			exit(1);
		}
	}

	/* set up input and output files */
	mi = MiInit( stdin );
	i = InvInit( stdout );

	/* for all the lines in the input file */
	for( *lastpath = '\0', rn = 1; (mip = MiRead( mi )) != NULL; ++rn)
	{
		path = mip->mi_path;
		if( strcmp( path, lastpath ) < 0 )
		{
			(void) fprintf( stderr, "%s: sort error, record #%d\n",
				prog, rn );
			exit(1);
		}
		(void) PathSet( lastpath, path );

		if( lstat( path, &stb ) ) 
		{	
			(void) fprintf(stderr,"%s: cannot stat %s (%s)\n",
				prog, path, sys_errlist[errno]);
			exit(1);
		}

		if( (stb.st_mode & S_IFMT) == S_IFSOCK )
		{
			/* error, no sockets permitted */

			(void) fprintf(stderr,
				"%s: %s: illegal file type code %o\n", prog,
				path, stb.st_mode & S_IFMT);
			exit(1);
		}
		ip = StatToInv( &stb );
		ip->i_flags =  mip->mi_flags;
		(void) InvRecSetRev( ip, version );
		(void) InvRecSetPath( ip, path );
		(void) InvRecSetSubset( ip, mip->mi_subset );
		(void) InvRecSetRef( ip, "none" );

		LinkResolve( ip );

		switch( ip->i_type )
		{
		case 'b':	/* block device */
		case 'c':	/* char device */
		case 'd':	/* directory */
			break;
		case 'f':	/* straight file */
			ip->i_sum = CheckSum( path );
		case 'l':	/* link */
		case 'p':	/* fifo */
			break;
		case 's':	/* symbolic link */
			SymLink( ip );
			break;
		default:
			(void) fprintf(stderr,
				"%s: unknown type '%c', file %s\n", prog,
				ip->i_type, path );
		}
		/* Write finished record to stdout */
		(void) InvWrite( i, ip );	
	}
	LinkClear();

}



/*	static LinkClear() -
 *		Assert than no unresolved hard links exist.
 *
 *	given:	uses global (ResQT *links)
 *	does:	searches list for unresolved links
 *	return:	only is there are no unresolved links.
*/

static LinkClear()
{
	ResQT	*t;
	int	n = 0;

	for( t = links; t != NULL; t = t->rq_next )
	{
		if( t->rq_nlink != 0 )
		{
			(void) fprintf( stderr, "%s: unresolved nlink %d: %s\n",
				prog, t->rq_nlink, t->rq_path );
			++n;
		}
	}
	if( n )
	{
		(void) fprintf( stderr, "%s: %d unresolved hard links\n",
			prog, n );
		exit(1);
	}
}



/*	static LinkResolve() -
 *		Resolve Hard Links
 *
 *	given:	InvRecT *p - pointer to an InvRecT to resolve
 *	does:	verifies that the file is not a directory.
 *		if the link count on the file is > 1, checks to
 *		see if the file satisfies any pending links. if
 *		so it is marked type 'l' and the pending link is
 *		decremented or cleared as appropriate. If the file
 *		does not satisfy any pending links, it is new! and
 *		gets queue for resolution.
 *	return:	nothing
 *	side:	may modify i_type field of the incomming record.
 *		may modify the referent field of the incomming record.
*/

static LinkResolve(p)
InvRecT *p;
{
	ResQT	*t;

	if( p->i_type == 'd' || p->i_nlink == 1 )
		return;

	if( (t = LinkSearch( p, links )) == NULL )
	{
		t = ResQCreate( p );
		t->rq_next = links;
		links = t;
		return;
	}
	--(t->rq_nlink);
	(void) InvRecSetRef( p, t->rq_path );
	p->i_type = 'l';
}



/*	static ResQT *LinkSearch() -
 *		Attempt to locate a file in the resolution queue
 *
 *	given:	InvRecT *p - a pointer to a file being resolved
 *		ResQT *l - a pointer to the resolution queue
 *	does:	searches the resolution queue for a dev/ino match
 *		on the incomming InvRecT
 *	return: a pointer to the resolution node if found, else NULL
 *
*/

static ResQT *LinkSearch(p, l)
InvRecT *p;
ResQT *l;
{
	register ResQT	*t;

	for( t = l; t != NULL; t = t->rq_next )
	{
		if( p->i_dev == t->rq_dev && p->i_ino == t->rq_ino )
			break;
	}
	return(t);
}



/*	ResQT *ResQCreate() -
 *		Queue up a hard link resolution block
 *
 *	given:	InvRecT *p - an InvRecT with the data to be queued
 *	does:	allocates space for a ResQT and fills in the blanks
 *	return:	a pointer to the new ResQT
*/

static ResQT *ResQCreate( p )
InvRecT *p;
{
	register ResQT	*t;

	if( (t = ResQNew()) == NULL )
		return(NULL);

	/* copy in: pathname, dev, ino, linkcount-1.
	*/
	(void) strcpy( t->rq_path, p->i_path );
	t->rq_dev = p->i_dev;
	t->rq_ino = p->i_ino;
	t->rq_nlink = p->i_nlink - 1;
	return(t);
}



	
/*	SymLink() -
 *		Look up symbolic link information for referrent
 *		field of an InvRecT.
 *
 *	given:	InvRecT *p - an inventory record to work with.
 *	does:	fill referent field from readlink() info.
 *	return:	Nothing.
*/

static SymLink(p)
InvRecT *p;
{
	int	n;
	PathT	buf;

	if( (n = readlink( p->i_path, buf, sizeof(buf) )) == -1 )
	{
		(void) fprintf(stderr, "%s: cannot readlink %s (%s)\n", prog,
			p->i_path, sys_errlist[errno]);
		exit(-1);
	}
	/* readlink doesnt null terminate result buffer */
	buf[n] = '\0';
	(void) InvRecSetRef( p, buf );
	p->i_sum = 0;
}

static ResQT *ResQNew()
{
	return( (ResQT *) malloc( sizeof(ResQT) ) );
}

