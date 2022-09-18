/*	list.c -
 *		singly linked list manipulation routines
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
 *	000	8904??	ccb	New
 *	001	890608	ccb	Add ListFree, free all nodes in a list
*/

#ifndef lint
static	char *sccsid = "@(#)list.c	4.1	(ULTRIX)	7/2/90";
#endif lint

#include	"list.h"
/*LINTLIBRARY*/

/*	ListT	*ListAppend()
 *		concatenate lists
 *
 *	given:	ListT *p - list to append to
 *		ListT *q - list to append
 *	does:	append list q to list p
 *	return:	a pointer to the head of the merged list
*/

ListT *ListAppend( p, q )
ListT *p;
ListT *q;
{
	register ListT	*t;	/* temporary pointer */

	if( q == (ListT *) 0 )		/* nothing to append */
		return( p );

	if( p == (ListT *) 0 )		/* append to nothing */
		return( q );

	/* find the last element in p */
	for( t = p; t->l_next != (ListT *) 0; t = t->l_next )
		;

	/* cause the last element of p to point to q */
	t->l_next = q;

	/* done */
	return( p );
}


/*	void	ListFree() -
 *		free a list
 *
 *	given:	ListT *l - a pointer to a list
 *	does:	free all of the elements of the list
 *	return:	nothing
*/

void ListFree( l )
ListT *l;
{
	if( l == (ListT *) 0 )
		return;

	ListFree( l->l_next );
	free( (char *) l );
	return;
}


/*	int	ListLen() -
 *		return number of elements in a list
 *
 *	given:	ListT *l - beginning of a list
 *	does:	count list elements
 *	return:	0 for NULL list, or 1 greater than the length of the
 *		the same list without its first element.
 *
 *	NOTE:	will not return if given a circular list
*/

int ListLen( l )
ListT *l;
{
	if( l == (ListT *) 0 )
		return( 0 );

	return( ListLen( l->l_next ) + 1 );
}

