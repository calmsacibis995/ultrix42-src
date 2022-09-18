/*	inv.c -
 *		setld routines for manipulating inventory information.
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
 *	000	3-feb-1989	Chas. Bennett
 *		New.
 *
 *	001	29-apr-1989	ccb
 *		Lint.
 *	002	14-jun-1989	ccb
 *		ongoing regularization of names for subroutines
*/

#ifndef lint
static	char *sccsid = "@(#)inv.c	4.1	(ULTRIX)	7/2/90";
#endif lint
/*LINTLIBRARY*/

#include	<sys/param.h>
#include	<sys/types.h>
#include	<sys/stat.h>
#include	<sys/dir.h>
#include	<time.h>
#include	<string.h>
#include	<stdio.h>
#include	"setld.h"
#include	"list.h"

#define	INVIFMT		"%s\t%d%u%hu%hu%ho%s%s%1s%s%s%s"
#define	INVOFMT		"%x\t%d\t%05u\t%d\t%d\t%06o\t%s\t%s\t%c\t%s\t%s\t%s\n"
#define	IBUFSIZ		4096


static FILE	*InvTToFILE();
static char	ibuf[IBUFSIZ];




/*	char	InvFmtType() -
 *		translate mode bits to type information
 *
 *	given:	u_short mode - the mode bits to use
 *	does:	assign a character code for each file type in [bcdfpsu=].
 *	return:	the character code.
*/

char InvFmtType(mode)
u_short mode;
{
	char	type = 'u';	/* default type u for unknown */

	switch( mode & S_IFMT )
	{
	case S_IFBLK:
		type = 'b';	/* block special */
		break;
	case S_IFCHR:
		type = 'c';	/* character special */
		break;
	case S_IFDIR:
		type = 'd';	/* directory */
		break;
	case S_IFREG:
		type = 'f';	/* regular file */
		break;
	case S_IFIFO:
		type = 'p';	/* pipe file */
		break;
	case S_IFLNK:
		type = 's';	/* symbolic link */
		break;
	case S_IFSOCK:
		type = '=';	/* socket */
		break;
	}
	return( type );
}



/*	InvT	*InvInit() -
 *		convert FILE * to InvT *
 *
 *	given:	FILE * p - a FILE pointer
 *	does:	'converts' it to InvT
 *	return:	a pointer to the new InvT
 *
 *	note:	currently implemented as a cast
*/

InvT *InvInit(p)
FILE *p;
{
	return( (InvT *) p );
}



/*	InvT	*InvOpen() -
 *		open an inventory
 *
 *	given:	char *path - an inventory pathname
 *		char *mode - a mode in {"r","w","a"}.
 *	does:	opens the inventory with the correct mode.
 *	return:	a pointer to an InvT to be used with other
 *		inventory routines or NULL
*/

InvT *InvOpen( path, mode )
char *path, *mode;
{
	FILE	*f;

	if( (f = fopen( path, mode )) == NULL )
		return(NULL);

	return( InvInit( f ) );
}




/*	InvRecT *InvRead() -
 *		read an inventory record.
 *
 *	does:	read an inventory record from external representation
 *		into an InvRecT.
 *	given:	InvT *inp - the channel to read from.
 *	return:	A pointer to an InvRecT containing the data if availble.
 *		NULL on EOF or bad data.
 *
 *	note:	this implementation uses a temporary buffer for the first
 *		field for compatibility with previous versions of the
 *		inventory format spec. this prevents sscanf from getting
 *		confusued if the first field is not a hex int.
*/

InvRecT *InvRead(inp)
InvT *inp;
{
	static InvRecT	p;		/* read structure */
	static NameT	tmpbuf;		/* 1st field buffer */

	if( fgets( ibuf, sizeof(ibuf), InvTToFILE(inp) ) == NULL )
		return(NULL);

	(void) sscanf( ibuf, INVIFMT, tmpbuf, &(p.i_size), &(p.i_sum),
		&(p.i_uid), &(p.i_gid), &(p.i_mode), p.i_date, p.i_rev,
		&(p.i_type), p.i_path, p.i_ref, p.i_subset );

	/* extract possible int from tmpbuf into i_flags
	*/
	(void) sscanf( tmpbuf, "%x", &(p.i_flags) );

	/* if this is a device, assume the referent field contains
	 *  an ascii representation of the rdev info, scan and store.
	*/
	(void) sscanf( p.i_ref, "%d", &(p.i_rdev) );

	return(&p);
}



/*	InvRecT	*InvRecAppend() -
 *		concatenate lists of InvRecT nodes
 *
 *	given:	InvRecT *left - pointer to left InvRecT list, may be NULL
 *		InvRecT *right - pointer to right InvRecT list, may be NULL
 *	does:	appends right list to the end of the left list
 *	return:	a pointer to the head of the new list
*/

InvRecT *InvRecAppend( left, right )
InvRecT *left, *right;
{
	return( (InvRecT *) ListAppend( (ListT *) left, (ListT *) right ) );
}


/*	InvRecT	*InvRecCopy() -
 *		Copy an InvRecT
 *
 *	given:	InvRecT *dest - the destination record
 *		InvRecT *src - the source record
 *	does:	copies the source record to the destination
 *	return:	a pointer to the destination record.
*/

InvRecT *InvRecCopy( dest, src )
InvRecT *dest, *src;
{
	bcopy( (char *) src, (char *) dest, sizeof( InvRecT ) );
	return( dest );
}



/*	InvRecT	*InvRecInsertAlpha() -
 *		Insert an InvRec in alpha order on i_path field
 *
 *	given:	InvRecT *s - list to insert record into
 *		InvRecT *t - record to insert
 *	does:	destructively insert element t into list s
 *	return:	a pointer to the modified list
*/

InvRecT *InvRecInsertAlpha( s, t )
InvRecT *s;
InvRecT *t;
{
	if( t == NULL )
		return( s );

	if( s == NULL )
		return( t );

	if( strcmp( s->i_path, t->i_path ) >= 0 )
	{
		t->i_next = s;
		return( t );
	}
	else
	{
		s->i_next = InvRecInsertAlpha( s->i_next, t );
		return( s );
	}
}


/*	InvRecT	*InvRecNew() -
 *		allocate storage
 *
 *	given:	nothing
 *	does:	allocate storage for an InvRecT
 *	return:	a pointer to the appropriate storage if available, else NULL
*/

InvRecT *InvRecNew()
{
	return( (InvRecT *) malloc( sizeof(InvRecT) ) );
}



/*	char	*InvRecSetPath() -
 *		set path, SHOULD BE VOID!
 *
 *	given:	InvRecT *p - pointer to Record to set
 *		PathT path - path value to use
 *	does:	transcribe the given path value into the i_path field of
 *		the record
 *!	return:	the addr of the i_path field (SHOULD BE VOID)
*/

char *InvRecSetPath( p, path )
InvRecT *p;
PathT path;
{
	(void) PathSet( p->i_path, path );
	return( p->i_path );
}


/*	char	*InvRecSetRef() -
 *		set referent field
 *
 *	given:	InvRecT *p - pointer to record to set
 *		PathT ref - referent field value
 *	does:	set the referent field
 *!	return;	a pointer to the referent field within the record
 *		(SHOULD BE VOID)
*/

char *InvRecSetRef( p, ref )
InvRecT *p;
PathT ref;
{
	(void) PathSet( p->i_ref, ref );
	return( p->i_ref );
}



/*	char	*InvRecSetRev() -
 *		set revision field
 *
 *	given:	InvRecT *p - record to modify
 *		CodeT v - version code to use
 *	does:	transcribe the version code into the rev field of the record
 *	return:	a pointer to the referent field within the record (SOULD
 *!		BE VOID)
*/

char *InvRecSetRev( p, v )
InvRecT *p;
CodeT v;
{
	(void) CodeSet( p->i_rev, v );
	return(  p->i_rev );
}



/*	char	*InvRecSetSubset()
 *		set subset field
 *
 *	given:	InvRecT *p - pointer to record to modify
 *		NameT sub - subset name value
 *	does:	transcribe the name value into the record
 *	return:	a pointer to the subset field in the record (SOULD
 *!		BE VOID)
*/

char *InvRecSetSubset( p, sub )
InvRecT *p;
NameT sub;
{
	(void) NameSet( p->i_subset, sub );
	return(  p->i_subset );
}


/*	FILE	*InvTToFILE
 *		Return a file pointer from an InvT
 *
 *	given:	InvT *p - the InvT to use.
 *	does:	find the associated FILE pointer
 *	return:	the pointer found
*/

static FILE *InvTToFILE(p)
InvT *p;
{
	return( (FILE *) p );
}



/*	InvWrite() -
 *		output an inventory record
 *
 *	given:	InvT *p - an inventory channel pointer.
 *		InvRecT *q - a pointer to a record.
 *
 *	does:	Translate the information in record into standard
 *		on-disk inventory representation.
 *
 *	return:	-1 on failure, 0 otherwise.
*/

InvWrite( p, x )
InvT *p;
InvRecT *x;
{
	/* There is some data specific processing. If the record being
	 *  written represents a device, the rdev information is xlated
	 *  to ascii and placed in the ref field.
	*/
	if( x->i_type == 'c' || x->i_type == 'b' )
		(void) sprintf( x->i_ref, "%d", x->i_rdev );

	(void) fprintf( InvTToFILE(p), INVOFMT, x->i_flags, x->i_size, x->i_sum,
		x->i_uid, x->i_gid, x->i_mode, x->i_date, x->i_rev, x->i_type,
		x->i_path, x->i_ref, x->i_subset );
	return(0);
}



/*	InvRecT	*StatToInv()
 *		Format stat info into an InvRecT
 *
 *	given:	struct stat *s - stat info to use.
 *	does:	copies relevant stat info to a static
 *		InvRecT
 *	return:	a pointer to the static InvRecT.
*/

InvRecT *StatToInv(s)
struct stat *s;
{
	static InvRecT	i;

	i.i_dev = s->st_dev;
	i.i_gid = s->st_gid;
	i.i_ino = s->st_ino;
	i.i_mode = s->st_mode;
	i.i_nlink = s->st_nlink;
	i.i_rdev = s->st_rdev;
	i.i_size = s->st_size;
	i.i_uid = s->st_uid;
	(void) strcpy( i.i_date, DateFormat( s->st_mtime ) );
	i.i_type = InvFmtType( s->st_mode );
	
	return( &i );
}



