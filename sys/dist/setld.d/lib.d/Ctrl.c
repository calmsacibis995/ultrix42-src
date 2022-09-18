/*	ctrl.c -
 *		setld control file routines
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
 *	mods:
 *	000	02-jun-1989	ccb
 *		New.
 *
 *	001	20-jun-1989	ccb
 *		Lint, add public ctrlerr variable & error message
 *			formatting (CtrlErrorString()).
 *		Numerous bug fixes made during depord qualification
 *
 *	002	17-oct-1989	ccb
 *		Add CtrlFileClose().
 *		Add call to CtrlFilClose() from CtrlFileRead().
 *		These changes were required to allow setld to operate
 *		on more than 59 subsets simultaneously.
*/

#ifndef lint
static	char *sccsid = "@(#)Ctrl.c	4.1	(ULTRIX)	7/2/90";
#endif lint

#include	<sys/param.h>
#include	<sys/types.h>
#include	<sys/dir.h>
#include	<stdio.h>
#include	"setld.h"
#include	"list.h"

/*LINTLIBRARY*/
static int	CtrlFileClose();
static char	*CtrlFileGetString();
static int	CtrlFileOpen();
static CtrlRecT	*CtrlFileRead();
static int	CtrlSeek();

int	ctrlerr;
StringT	ctrl_errmsg;
static StringT	ctrl_errstrings[CTRL_NCODE] =
{
	"No Error",				/* CTRL_OK */
	"No More Ctrl Records Available",	/* CTRL_EOF */
	"Cannot Access Ctrl Record",		/* CTRL_FOPEN */
	"Ctrl Record Corrupt"			/* CTRL_CPTN */
};


/*	CtrlT *CtrlCreate() -
 *		create a CtrlT with initialized path field
 *
 *	given:	PathT p - a path name to use in creation
 *	does:	allocate storage for a CtrlT, initialize c_dpath
 *	return:	a pointer to the new storage or NULL if none available
*/

CtrlT *CtrlCreate( p )
PathT p;
{
	CtrlT	*c;	/* new storage pointer */

	if( (c = CtrlNew()) == NULL )
		return( NULL );

	PathSet( c->c_dpath, p );
	return( c );
}


/*	char *CtrlErrorString() -
 *		swap error code for message string
 *
 *	given:	an error code
 *	does:	look up a string for the code
 *	return:	a pointer to the string
*/

char *CtrlErrorString( mc )
int mc;
{
	static StringT		buf;	/* buffer */

	mc &= 0xff;
	if( mc >= 0 && mc < CTRL_NCODE )
		return( ctrl_errstrings[mc] );

	(void) sprintf( buf, "unknown ctrlerr code %d", mc );
	return( buf );
}


/*	static int	CtrlFileClose() -
 *		close a control file
 *
 *	given:	Ctrl *c - a control pointer
 *	does:	close file file associated with the pointer
 *	return:	-1 if file is not open
*/

static int CtrlFileClose( c )
CtrlT *c;
{
	int	j;

	if( (j = fclose( c->c_fp )) == 0 )
		c->c_fp = NULL;

	return( j );
}


/*	static char *CtrlFileGetString() -
 *		get a string from the control file
 *
 *	given:	Ctrl *c - control pointer
 *	does:	read one string from control file referenced thru the
 *		pointer
 *	return:	NULL on EOF, addr of a StringT containing the string
 *		otherwise
*/

static char *CtrlFileGetString( c )
CtrlT *c;
{
	static StringT	buf;

	if( fgets( buf, sizeof(buf), c->c_fp ) == NULL )
		return( NULL );

	return( buf );
}



/*	static int	CtrlFileOpen()
 *		open ctrl file indicated by CtrlT descriptor
 *
 *	given:	CtrlT *c - descriptor
 *		char *mode - mode in  {"r", "w", "a"}
 *	does:	prepare the descriptor to be used in accessing data in
 *		current record
 *	return:	-1 on failure, 0 otherwise
*/

static int CtrlFileOpen( c, mode )
CtrlT *c;
char *mode;
{
	if( (c->c_fp = fopen( c->c_fpath, mode )) == NULL )
		return( -1 );
	return(0);
}



/*	DIR	*CtrlGetDir() -
 *		get a DIR pointer from a CtrlT
 *
 *	given:	CtrlT *c - CtrlT to use
 *	does:	find a DIR pointer associated with the CtrlT
 *	return:	the DIR pointer
*/

DIR *CtrlGetDir( c )
CtrlT *c;
{
	return( c->c_dir );
}


/*	char	*CtrlGetFpath()
 *		get path component from control record
 *
 *	given:	CtrlT *c - record to use
 *	does:	access the fpath associated with the record
 *	return:	a pointer to the record
*/

char *CtrlGetFpath( c )
CtrlT *c;
{
	return( c->c_fpath );
}


/*	CtrlT	*CtrlNew() -
 *		allocate CtrlT
 *
 *	given:	nothing
 *	does:	allocate storage for CtrlT, zero all fields
 *	return:	new CtrlT, NULL on malloc() error
*/

CtrlT *CtrlNew()
{
	CtrlT	*c;

	if( (c = (CtrlT *) malloc( sizeof(CtrlT) )) == NULL )
		return( NULL );

	PathSet( c->c_dpath, "" );
	PathSet( c->c_fpath, "" );
	c->c_dir = NULL;
	c->c_fp = NULL;
	c->c_next = NULL;

	return(c);
}



/*	CtrlT	*CtrlOpen() -
 *		open a control stream
 *
 *	given:	PathT s - the name of the stream to open
 *		char *mode - mode in {"r", "w", "a"}
 *	does:	open the stream, bound to a newly allocated descriptor
 *	return:	a pointer to the new descriptor
 *	NOTE:	WRITE ACCESS NOT IMPLEMENTED
*/

CtrlT *CtrlOpen( s, mode )
PathT s;
char *mode;
{
	CtrlT	*c;
	DIR	*d;


	switch( *mode )
	{
	case 'r':
		if( (d = opendir(s)) != NULL && (c = CtrlCreate( s )) != NULL )
		{
			c->c_dir = d;
			return(c);
		}
		else
			return( NULL );
	case 'w':
	case 'a':
	default:
		/* other access modes NYI
		*/
		return( NULL );
	}
}



/*	CtrlRecT	*CtrlRead() -
 *		read a control record from a subsets database
 *
 *	given:	CtrlT *c - an active db pointer
 *	does:	finds the next ctrl record in the database and reads it
 *	return:	a pointer to the new record or NULL if no more in the
 *		database
*/

CtrlRecT *CtrlRead( c )
CtrlT *c;
{
	static CtrlRecT	cr;		/* static storage for input record */
	char		*s;		/* input line pointer */
	AssignT		*ap;		/* assign pointer */
	FlagsT		varmask;	/* stores var status bits */

	ctrlerr = 0;
	/* Position to next record position
	*/
	if( CtrlSeek( c ) )
	{
		/* no more records
		*/
		ctrlerr = CTRL_EOF;
		return( NULL );
	}

	/* read the record - this involves initializing any data we
	 *  already have, opening the 'control file' and translating any
	 *  control file information into the internal representation
	*/
	if( CtrlFileOpen( c, "r" ) != 0 )
	{
		ctrlerr = CTRL_FOPEN;
		return( NULL );
	}

	(void) CtrlRecSetSubset( &cr, PathStripExt( CtrlGetFpath( c ) ) );

	varmask = 0;
	while( (s = CtrlFileGetString( c )) != NULL )
	{
		ap = AssignScan( s );

		if( !strcmp( AssignGetName(ap), "NAME" ) )
		{
			(void) CtrlRecSetName( &cr, AssignGetVal( ap ) );
			varmask |= CVAR_NAME;
		}
		else if( !strcmp( AssignGetName( ap ), "DESC" ) )
		{
			(void) CtrlRecSetDesc( &cr, AssignGetVal( ap ) );
			varmask |= CVAR_DESC;
		}
		else if( !strcmp( AssignGetName( ap ), "NVOLS" ) )
		{
			(void) CtrlRecSetDiskInfo( &cr, AssignGetVal( ap ) );
			varmask |= CVAR_NVOLS;
		}
		else if( !strcmp( AssignGetName( ap ), "MTLOC" ) )
		{
			(void) CtrlRecSetTapeInfo( &cr, AssignGetVal( ap ) );
			varmask |= CVAR_MTLOC;
		}
		else if( !strcmp( AssignGetName( ap ), "DEPS" ) )
		{
			(void) CtrlRecSetDeps( &cr,
				DepScan( AssignGetVal(ap) ) );
			varmask |= CVAR_DEPS;
		}
		else if( !strcmp( AssignGetName( ap ), "FLAGS" ) )
		{
			(void) CtrlRecSetFlags( &cr,
				FlagsScan( AssignGetVal(ap) ) );
			varmask |= CVAR_FLAGS;
		}
	}
	(void) CtrlFileClose( c );
	if( (varmask & CVAR_ALL) != CVAR_ALL )
	{
		ctrlerr = CTRL_CPTN;
		ctrlerr |= varmask << 8;
		return( NULL );
	}
	return( &cr );
}


/*	CtrlRecT	*CtrlRecAppend() -
 *		append a CtrlRecT to a list of same
 *
 *	given:	CtrlRecT *l - list to append to
 *		CtrlRecT *n - node to append
 *	does:	append c to the end of d
 *	return:	a pointer to the modified list
*/

CtrlRecT *CtrlRecAppend( l, n )
CtrlRecT *l, *n;
{
	return( (CtrlRecT *) ListAppend( (ListT *) l, (ListT *) n ) );
}


/*	CtrlRecT	*CtrlRecCopy() -
 *		copy a CtrlRecT
 *
 *	given:	CtrlRecT *s - destination pointer
 *		CtrlRecT *t - source pointer
 *	does:	copies the CtrlRecT at *t to *s
 *	return:	the destination pointer
*/

CtrlRecT *CtrlRecCopy( s, t )
CtrlRecT *s, *t;
{
	bcopy( (char *) t, (char *) s, sizeof(CtrlRecT) );
	return( s );
}


/*	char *CtrlRecGetSubset() -
 *		get subset info from CtrlRecT
 *
 *	given:	a pointer to a CtrlRecT
 *	does:	find the subset name associated with the record
 *	return:	a pointer to the name in a buffer
*/

char *CtrlRecGetSubset( c )
CtrlRecT *c;
{
	return( (char *) c->ct_subset );
}


/*	CtrlRecT	*CtrlRecNew() -
 *		allocate CtrlRecT
 *
 *	given:	nothing
 *	does:	allocate space for a new CtrlRecT
 *	return:	a pointer to a newly allocated CtrlRecT or NULL on malloc()
 *		error.
*/

CtrlRecT *CtrlRecNew()
{
	return( (CtrlRecT *) malloc( sizeof(CtrlRecT) ) );
}


/*	CtrlRecT *CtrlRecOrderInsert() -
 *		insert a control record in a control record list in
 *		tape search order
 *
 *	given:	CtrlRecT *l - a pointer to a list of control records
 *		CtrlRecT *n - a pointer to a record to be inserted
 *	does:	insert the record in tape order
 *	return:	a pointer to the modified list
*/

CtrlRecT *CtrlRecOrderInsert( l, n )
CtrlRecT *l;
CtrlRecT *n;
{
	if( l == NULL )
	{
		/* starting with empty list */
		return( n );
	}

	if( n == NULL )
	{
		/* attempt to insert empty node */
		return( l );
	}

	if( n->ct_tvol > l->ct_tvol)
	{
		/* not here, keep looking */
		l->ct_next = CtrlRecOrderInsert( l->ct_next, n );
		return( l );
	}
	else if( n->ct_tvol < l->ct_tvol )
	{
		/* insert n here */
		n->ct_next = l;
		return( n );
	}
	/* same volume, same checks but on ct_tloc */
	if( n->ct_tloc > l->ct_tloc )
	{
		/* not here, keep looking */
		l->ct_next = CtrlRecOrderInsert( l->ct_next, n );
		return( l );
	}
	else if( n->ct_tloc < l->ct_tloc )
	{
		/* insert n here */
		n->ct_next = l;
		return( n );
	}
	else
	{	/* FATAL ERROR */
		return( NULL );
	}
}


/*	void	*CtrlRecPrintList() -
 *		print a list of CtrlRecTs
 *
 *	given:	FILE *f - pointer to an active output stream
 *		CtrlRecT *c - a pointer to a list of CtrlRecTs
 *		int m - width of output margin
 *	does:	output the contents of each CtrlRecT accessible from the
 *		the pointer
 *	return:	VOID
*/

void CtrlRecPrintList( f, c, m )
register FILE *f;
CtrlRecT *c;
int m;
{
	static StringT		margin;	/* print margin */
	register CtrlRecT	*cp;	/* current node pointer */
	register int		i;	/* margin index */

	i = (m < 0) ? 0 : m;
	margin[i] = '\0';
	while( i-- )
	{
		margin[i] = '\t';
	}
	for( cp = c; cp != NULL; cp = cp->ct_next )
	{
		(void) fprintf( f, "\n%sCtrlRecT at 0x%X\n", margin,
			(long) cp );
		(void) fprintf( f, "%sct_next: 0x%X\n", margin,
			(long) cp->ct_next );
		(void) fprintf( f, "%sct_subset: %s\n", margin, cp->ct_subset );
		(void) fprintf( f, "%sct_name: %s\n", margin, cp->ct_name );
		(void) fprintf( f, "%sct_desc: %s\n", margin, cp->ct_desc );
		(void) fprintf( f, "%sct_dcnt: %u\n", margin, cp->ct_dcnt );
		(void) fprintf( f, "%sct_dvol: %d\n", margin, cp->ct_dvol );
		(void) fprintf( f, "%sct_tvol: %u\n", margin, cp->ct_tvol );
		(void) fprintf( f, "%sct_tloc: %d\n", margin, cp->ct_tloc );
		(void) fprintf( f, "%sct_deps: %s\n", margin,
			(cp->ct_deps == NULL) ?
			"NULL" : DepListFormat( cp->ct_deps ) );
		(void) fprintf( f, "%sct_flags: %x\n", margin, cp->ct_flags );
	}
}



/*	DepT	*CtrlRecSetDeps() -
 *		associate a dependency list with a CtrlRecT
 *
 *	given:	CtrlRecT *c - CtrlRecT to modify
 *		DepT *d - dependency to use
 *	does:	stuff the DepT pointer into the appropriate field
 *	return:	the DepT pointer
*/

DepT *CtrlRecSetDeps( c, d )
CtrlRecT *c;
DepT *d;
{
	return( c->ct_deps = d );	/* assign */
}


/*	char	*CtrlRecSetDesc() -
 *		bind description to CtrlRecT
 *
 *	given:	CtrlRecT *c - record
 *		StringT d - description
 *	does:	associate description value with record
 *	return:	pointer to the description
 *!	SHOULD BE VOID
*/

char *CtrlRecSetDesc( c, d )
CtrlRecT *c;
StringT d;
{
	(void) StringSet( c->ct_desc, d );
	return( c->ct_desc );
}



/*	char	*CtrlRecSetDiskInfo() -
 *		set disk info fields (ct_dvol, ct_dcnt)
 *
 *	given:	CtrlRecT *c - record to set
 *		StringT i - a string (x:y) repreentation of the disk info
 *	does:	scan the disk info into the apropriate fields on the
 *		record
 *	return:	VOID
*/

char *CtrlRecSetDiskInfo( c, i )
CtrlRecT *c;
StringT i;
{
	(void) sscanf( i, "%d:%d", &(c->ct_dvol), &(c->ct_dcnt) );
	return( i );
}



/*	FlagsT	*CtrlRecSetFlags() -
 *		set flags field
 *
 *	given:	CtrlRecT *c - record to modify
 *		FlagsT f - flags value
 *	does:	place the flags value in the record
 *	return:	the flags value (SHOULD BE VOID)
*/

FlagsT CtrlRecSetFlags( c, f )
CtrlRecT *c;
FlagsT f;
{
	c->ct_flags = f;
	return( f );
}



/*	char	*CtrlRecSetName() -
 *		bind name to CtrlRecT
 *
 *	given:	CtrlRecT *c - CtrlRecT
 *		StringT n - name value
 *	does:	associate name value with record
 *!	return:	pointer to the record (SHOULD BE VOID)
*/

char *CtrlRecSetName( c, n )
CtrlRecT *c;
StringT n;
{
	(void) StringSet( c->ct_name, n );
	return( c->ct_name );
}


/*	CtrlRecT	*CtrlRecSetSubset() -
 *		set subset value for CtrlRecT
 *
 *	given:	CtrlRecT *c - record to use
 *		NameT s - subset name
 *	does:	hangs the subset info on the record
 *	return:	the record pointer
*/

char *CtrlRecSetSubset( c, s )
CtrlRecT *c;
NameT s;
{
	return( NameSet( c->ct_subset, s ) );
}


char *CtrlRecSetTapeInfo( c, i )
CtrlRecT *c;
StringT i;
{
	(void) sscanf( i, "%d:%d", &(c->ct_tvol), &(c->ct_tloc) );
	return( i );
}



/*	static int *CtrlSeek() -
 *		find the next control record in the subset database
 *
 *	given:	CtrlT *c - The initialized db pointer
 *	does:	scan the database for the next control record
 *	return:	-1 if no more records, 0 if successful.
 *	effect:	changes c->fpath in the db structure
*/

static int CtrlSeek( c )
CtrlT *c;
{
	struct direct	*entry;

	while( (entry = readdir( CtrlGetDir(c) )) != NULL )
	{
		if( PathMatch( entry->d_name, "*.ctrl" ) )
		{
			(void) CtrlSetFpath( c, entry->d_name );
			return( 0 );
		}
	}
	return( -1 );
}



/*	char	*CtrlSetFpath() -
 *		set c_fpath field of a CtrlT
 *
 *	given:	CtrlT *c - a pointer to the CtrlT to use
 *		PathT p - a path data to use
 *	does:	sets the fpath of the CtrlT to the value given
 *	return:	a pointer to the fpath
 *!		SHOULD BE VOID
*/

char *CtrlSetFpath( c, p )
CtrlT *c;
PathT p;
{
	(void) PathSet( c->c_fpath, p );
	return(  c->c_fpath );
}

/*END*/


