/*	Dpes.c -
 *		routines for handling DepT
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
 *	000	??-jun-1989	ccb
 *	001	24-jul-1989	ccb
 *		ongoing qualification, several new routines to
 *			simplify check-out and run-in
*/

#ifndef lint
static	char *sccsid = "@(#)Deps.c	4.1	(ULTRIX)	7/2/90";
#endif lint

#include	<sys/param.h>
#include	<sys/types.h>
#include	<sys/dir.h>
#include	<stdio.h>
#include	"setld.h"
#include	"list.h"

/*LINTLIBRARY*/

/*	DepT	*DepAppend() -
 *		append a DepT to the end of a DepT list
 *
 *	given:	DepT *l - the list
 *		DepT *n - new structure to append
 *	does:	append structure d to list e
 *	return:	a pointer to the modified list
*/

DepT *DepAppend( l, n )
DepT *l, *n;
{
	return( (DepT *) ListAppend( (ListT *) l, (ListT *) n ) );
}



/*	void	DepCopy() -
 *		copy a DepT
 *
 *	given:	DepT *s - destination pointer
 *		DepT *t - source pointer
 *	does:	copies content info from t to s
 *	return:	VOID
*/

void DepCopy( s, t )
DepT *s, *t;
{
	(void) bcopy( (char *) t, (char *) s, sizeof( DepT ) );
}



/*	DepT	*DepCreate() -
 *		allocate DepT storage and set primary field
 *
 *	given:	NameT s - name value for the new DepT
 *	does:	allocate storage for a new DepT and fill name field
 *	return:	pointer to new storage, NULL if storage is unavailable
*/

DepT *DepCreate( s )
NameT s;
{
	DepT	*d;

	if( (d = DepNew()) == NULL )
		return( NULL );

	NameSet( d->d_value, s );
	return( d );
}


/*	void	DepFree() -
 *		free DepT storage
 *
 *	given:	DepT *d - pointer to DepT to be freed
 *	does:	free the storage used for the DepT
 *	return:	VOID
*/

void DepFree( d )
DepT *d;
{
	free( (char *) d );
}


/*	DepT	*DepListCopy() -
 *		copy a dependency list
 *
 *	given:	DepT *d - list to copy
 *	does:	allocates a new list with the same values as the
 *		one given.
 *	return:	NULL on memory error, otherwise a pointer to the copy
 *	beware:	this implementation will truncate lists on memory errors
*/

DepT *DepListCopy( d )
DepT *d;
{
	DepT	*n;	/* pointer for new node */

	if( d == NULL )
		return( NULL );

	/* allocate storage
	*/
	if( (n = DepNew()) == NULL )
		return( NULL );

	DepCopy( n, d );
	n->d_next = DepListCopy( d->d_next );
	return( n );
}


/*	char	*DepListFormat() -
 *		xlate a DepT list to a string
 *
 *	given:	DepT *d - a pointer to a list of DepTs
 *	does:	format the value fields from the list into a buffer
 *	return:	a pointer to the buffer
*/

char *DepListFormat( d )
register DepT *d;
{
	static StringT	buf;	/* output buffer */
	int		i, j;	/* buffer index, count */

	for( i = 0; d != NULL; d = d->d_next )
	{
		j = strlen( d->d_value ) + 1;
		if( i + j > STRINGLEN )
			return( NULL );

		(void) sprintf( buf + i, "%s ", d->d_value );
		i += j;
	}
	buf[i] = '\0';
	return( buf );
}


/*	void	DepListFree() -
 *		destroy all DepTs accessible thru given pointer
 *
 *	given:	DepT *d - starting point for list to destroy
 *	does:	frees all malloc(3)d storage reachable thru the pointer
 *	return:	VOID
*/

void DepListFree( d )
DepT *d;
{
	if( d == NULL )
		return;

	DepListFree( d->d_next );
	DepFree( d );
}


/*	DepT	*DepListMerge() -
 *		merge dependency lists
 *
 *	given:	DepT *s - one list
 *		DepT *t - another list
 *	does:	uniquely merge the two lists
 *	return:	a pointer to the merged list
*/

DepT *DepListMerge( s, t )
DepT *s, *t;
{
	DepT	*head, *r;	/* result list */
	DepT	*p;		/* current pointer */

	/* construct the merged list from scratch,
	 *  start with the first list
	*/
	for( head = r = NULL, p = DepAppend(s, t); p != NULL; )
	{
		if( DepMember( p, head ) == NULL )
		{
			/* current node not already in output list
			*/
			if( r == NULL )
			{
				/* add current node as first and last
				 *  element in output
				*/
				head = r = p;
			}
			else
			{
				/* append current node after last node,
				 *  change last node pointer to point to
				 *  current node
				*/
				r->d_next = p;
				r = p;
			}
			/* advance p
			*/
			p = p->d_next;
			/* sever p from the list it had
			 *  belonged to
			*/
			r->d_next = NULL;
		}
		else
			p = p->d_next;
	}

	return( head );
}



/*	DepT	*DepListPromote() -
 *		replace a subset level dependency list with a product
 *		level dependency list
 *
 *	given:	DepT *d - dependency list
 *	does:	replace the value field of each node in the list, converting
 *		the value field data from subset names to product names
 *	return:	a pointer to the modified list
 *	notes:	simple recursion
*/

DepT *DepListPromote( d )
DepT *d;
{
	if( d == NULL )
	{
		return( NULL );
	}
	DepListPromote( d->d_next );
	(void) sprintf( d->d_value, "%s%s", NameGetPcode( d->d_value ),
		NameGetVcode( d->d_value ) );

	return( d );
}



/*	DepT	*DepMember() -
 *		determine if a DepT is in a list
 *
 *	given:	DepT *s - node to search for
 *		DepT *t - list to search in
 *	does:	attempts to find an instance of s in t
 *	return:	the addr of the instance in t if found, NULL otherwise
*/

DepT *DepMember( s, t )
register DepT *s, *t;
{
	if( s == NULL )
	{
		/* does not search for NULL elements
		*/
		return( NULL );
	}

	for( ; t != NULL; t = t->d_next )
	{
		if( !strcmp( s->d_value, t->d_value ) )
			return( t );
	}
	return( NULL );
}



/*	DepT	*DepNew() -
 *		allocate storage for a DepT
 *
 *	given:	nothing
 *	does:	allocate storage for a DepT
 *	return:	a pointer to storage if available, else NULL
*/
 
DepT *DepNew()
{
	DepT	*p;

	if( (p = (DepT *) malloc( sizeof(DepT) )) == NULL )
		return( NULL );

	NameSet( p->d_value, "" );

	p->d_next = NULL;
	p->d_flags = 0;
	return( p );
}


/*	DepT	*DepScan() -
 *		xlate a string into a list of DepTs
 *
 *	given:	StringT s - string representing dependencies
 *	does:	create a DepT list representing the string
 *	return:	a pointer to the list
*/

DepT *DepScan( s )
StringT s;
{
	char		*p;		/* token pointer */
	DepT		*nd,		/* new dependency */
			*dl = NULL;	/* dependency list */
	static StringT	localstring;

	StringSet( localstring, s );
	StringUnquote( localstring );
	p = StringToken( localstring, "| \t" );
	for( ;p != NULL; p = StringToken( (char *) NULL, " \t" ) )
	{
		if( !strcmp( p, "." ) )
			continue;

		if( (nd = DepCreate( p )) == NULL )
		{
			DepListFree( dl );
			return( NULL );
		}
		dl = DepAppend( dl, nd );
	}
	return( dl );
}

