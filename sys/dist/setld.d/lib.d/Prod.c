/*	Prod.c -
 *		routine for processing product-level objects for setld
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
 *	000	02-jun-1989	ccb	New
*/

#ifndef lint
static	char *sccsid = "@(#)Prod.c	4.1	(ULTRIX)	7/2/90";
#endif lint

#include	<sys/param.h>
#include	<sys/types.h>
#include	<sys/dir.h>
#include	<string.h>
#include	<stdio.h>
#include	"list.h"
#include	"setld.h"

/*LINTLIBRARY*/

typedef struct ProdLinkT
{
	struct ProdLinkT	*pl_next;	/* link */
	ProdRecT		*pl_data;	/* record pointer */
} ProdLinkT;


/*	int	ProdDepsExist() -
 *		return true if a product's dependency list can be
 *		theoretically satisfied in a set of products
 *
 *	given:	ProdRecT *p - product to check against
 *		ProdRecT *x - list to check in
 *	does:	assert a products prospect for dependency resolution
 *	return:	true if a product exists in the set which will satisfy
 *		a particular product's dependencies
*/

int ProdDepsExist( p, x )
ProdRecT *p;
ProdRecT *x;
{
	DepT		*dp;	/* current dependency pointer */
	ProdRecT	*l;	/* current product pointer */

	for( dp = p->p_deps; dp != NULL; dp = dp->d_next )
	{
		/* check all of the dependencies for a product
		 *  against the products in a given list
		*/
		for( l = x; l != NULL; l = l->p_next )
		{
			if( !strcmp( dp->d_value, ProdRecGetName( l ) ) )
				break;
		}
		if( l == NULL )
			return( 0 );	/* dependency cannot match */
	}
	return( 1 );
}



/*	int	ProdDepsOk() -
 *		return true if product dependencies have been resolved
 *
 *	given:	ProdRecT *p - product to resolve
 *		ProdLinkT *resl - list to check
 *
 *	does:	determine if product p can be resolved against list resl.
 *		if it cannot be resolved, checks to make sure the products
 *		needed can be found in the product list being resolved
 *
 *	return:	1 (true) iff: (required product(s) exist(s) resolved -or-
 *			does not exist)
 *		0 (false) iff: (required product exists unresolved)
*/

ProdDepsOk( p, resl, all, closure )
ProdRecT *p;
ProdLinkT *resl;
ProdRecT *all;
int closure;
{
	if( ProdDepsResolve( p, resl ) )
		return( 1 );

	else if( closure == DEPS_CLOSED )
		return( 0 );

	else
		return( !ProdDepsExist( p, all ) );
}



/*	int	ProdDepsResolve() -
 *		dependencies resolved predicate
 *
 *	given:	ProdRecT *p - Product to resolve
 *		ProdLinkT *l - list to resolve against
 *	does:	determine if all required products have been resolved
 *	return:	1 (true) if resolution is successful, false otherwise
*/

ProdDepsResolve( p, l )
ProdRecT *p;
ProdLinkT *l;
{
	register DepT		*dp;	/* dependency pointer */
	register ProdLinkT	*lp;	/* current ProdLink pointer */

	/* iterate thru dependencies, scaning the resolution list
	 *  for each one, returning immediately if resolution cannot be
	 *  be made. If loop thru dependency list exits normally, resolution
	 *  was successful.
	*/
	for( dp = p->p_deps; dp != NULL; dp = dp->d_next )
	{
		/* first check to see if this is a product which has
		 *  an internal dependency
		*/
		if( !strcmp( dp->d_value, ProdRecGetName( p ) ) )
			continue;

		for( lp = l; lp != NULL; lp = lp->pl_next )
		{
			/* compare dependency against current subset name
			*/
			if( !strcmp( dp->d_value,
				ProdRecGetName( lp->pl_data ) ) )
			{
				break;	/* got it */
			}

		}
		if( lp == NULL )
			return( 0 );	/* resolve failed */
	}
	return( 1 );
}



/*	ProdLinkT *ProdLinkAppend() -
 *		append a ProdLinkT list to another
 *
 *	given:	ProdLinkT *l - list to append to
 *		ProdLinkT *n - list to append
 *	does:	Append n to l
 *	return:	a pointer to the merged lists
*/

ProdLinkT *ProdLinkAppend( l, n )
ProdLinkT *l;
ProdLinkT *n;
{
	return( (ProdLinkT *) ListAppend( (ListT *) l, (ListT *) n ) );
}


/*	void	ProdLinkFree() -
 *		free a list of ProdLinkT
 *
 *	given:	ProdLinkT *p - a pointer to a list of ProdLinkT
 *	does:	Free all ProdLinkTs accessible thru the pointer
 *	return:	VOID
*/

void ProdLinkFree( p )
ProdLinkT *p;
{
	(void) ListFree( p );
}



/*	ProdLinkT *ProdLinkNew() -
 *		allocate prodlink storage
 *
 *	given:	nil
 *	does:	allocate ProdLinkT storage, scrubbed
 *	return:	pointer to suitable storage, NULL if unavailable
*/

ProdLinkT *ProdLinkNew()
{
	register ProdLinkT	*l;	/* pointer to allocated storage */

	if( (l = (ProdLinkT *) malloc( sizeof(ProdLinkT) )) == NULL )
		return( NULL );

	l->pl_next = NULL;
	l->pl_data = NULL;
	return(l);
}



/*	ProdRecT *ProdRecAddCtrl() -
 *		add control information to product list
 *
 *	given:	ProdRecT *p - a product list pointer
 *		CtrlRecT *c - a control record pointer
 *	does:	assign the control record to a product, placing the record
 *		in order in the product's control record list
 *	return:	a product list pointer, pointing to the modified product list
*/

ProdRecT *ProdRecAddCtrl( p, c )
ProdRecT *p;
CtrlRecT *c;
{
	ProdRecT	*t;	/* temporary bucket pointer */

	/* find product that this control file belongs to
	*/
	t = ProdRecFindBySubset( p, CtrlRecGetSubset( c ) );

	if( t == NULL )
	{
		/* need a new bucket
		*/
		if( (t = ProdRecNew()) == NULL )
		{
			/* memory allocation error
			*/
			return( NULL );
		}

		/* set the pcode, vcode fields
		*/
		CodeSet( t->p_pcode, NameGetPcode( c->ct_subset ) );
		CodeSet( t->p_vcode, NameGetVcode( c->ct_subset ) );

		/* stuff it onto the front of the list
		*/
		t->p_next = p;
		p = t;
	}

	/* t is now points at the product structure we want to use
	 *  insert the control record in order in the
	 *  control list for this bucket
	*/
	t->p_cp = CtrlRecOrderInsert( t->p_cp, c );
	
	/* merge depends from this control record with
	 *  existing depends if any. this is done by creating
	 *  a copy of the dependencies listed on the control record,
	 *  promoting the copy to represent product dependencies
	 *  and finally merging the promoted dependencies with the
	 *  existing ones.
	*/
	t->p_deps = DepListMerge( t->p_deps,
		DepListPromote( DepListCopy( c->ct_deps ) ) );

	return( p );
}


ProdRecDepCycles( p )
ProdRecT *p;
{
	/* cycle detection NYI, assume no cycles possible
	*/
	return( 0 );
}


/*	ProdRecT	*ProdRecFindBySubset() -
 *		find a ProdRecT in a list
 *
 *	given:	ProdRecT *p - list pointer for list to search
 *		NameT s - name of subset to use as a key
 *	does:	search for the named subset
 *	return:	a pointer to a ProdRecT containg the subset
*/

ProdRecT *ProdRecFindBySubset( p, s )
register ProdRecT *p;
NameT s;
{
	CodeT	pc, vc;		/* local product, version code info */

	CodeSet( pc, NameGetPcode( s ) );
	CodeSet( vc, NameGetVcode( s ) );

	for( ; p != NULL; p = p->p_next )
	{
		if( !strcmp( pc, p->p_pcode ) && !strcmp( vc, p->p_vcode ) )
		{
			/* this is the product block we're looking for
			*/
			return(p);
		}
	}
	return( NULL );	/* Not Found */
}



/*	char	*ProdRecGetName() -
 *		get a product-name from a ProdRecT
 *
 *	given:	ProdRecT *p - the ProdRecT to look at
 *	does:	place a product name in a static buffer
 *	return:	a pointer the the product name (pcode, vcode) in a static
 *		buffer
*/

char *ProdRecGetName( p )
ProdRecT *p;
{
	static NameT	b;

	(void) strcpy( b, p->p_pcode );
	(void) strcpy( b + CODELEN, p->p_vcode );
	b[CODELEN+CODELEN] = '\0';
	return( b );
}


/*	ProdRecT	*ProdRecLinkOrder() -
 *		propogate ProdLinkT ordering to subordinate ProdRecTs
 *
 *	given:	ProdLinkT *p - pointer to a list of ProdLinkTs
 *	does:	impose the link order from the LinkTs onto the RecTs
 *		indicated by the data fields
 *	return:	a pointer to the head of the list generated by the
 *		re-ordering.
 *
 *	RECURSION.
*/

ProdRecT *ProdRecLinkOrder( p )
ProdLinkT *p;
{
	if( p == NULL )
		return( NULL );

	p->pl_data->p_next = ProdRecLinkOrder( p->pl_next );
	return( p->pl_data );
}



/*	int ProdRecListLen() -
 *		return length (# nodes) in a list of ProdRecTs
 *
 *	given:	ProdRecT *p - a pointer to al list of ProdRecTs
 *	does:	count the number of nodes
 *	return:	the count as an integer
*/

ProdRecListLen( p )
ProdRecT *p;
{
	return( ListLen( (ListT *) p ) );
}



/*	ProdRecT	*ProdRecNew()
 *		allocate storage for new ProdRecT
 *
 *	given:	nothing
 *	does:	allocate storage for new ProdRecT
 *	return:	pointer to "cleared" fresh storage if available, otherwise
 *		NULL.
*/

ProdRecT *ProdRecNew()
{
	ProdRecT	*p;

	if( (p = (ProdRecT *) malloc( sizeof(ProdRecT) )) == NULL )
		return( NULL );

	CodeSet( p->p_pcode, "" );
	CodeSet( p->p_vcode, "" );
	p->p_next = NULL;
	p->p_cp = NULL;
	p->p_deps = NULL;
	p->p_flags = 0;

	return( p );
}



/*	ProdRecT *ProdRecOrderByDeps() -
 *		place a set of product records in dependency order
 *
 *	given:	ProdRecT *p - pointer to a list of product records
 *		int closure - set to 1 if all dependencies must
 *			be ordered for success.
 *	does:	topologically orders the records by dependency using
 *		a series of linear searches. Algorithm courtesy of
 *		Tungning "Donnie" Cherng.
 *	return:	a pointer to the re-ordered list. NULL will be returned
 *		if cycles are detected.
*/

ProdRecT *ProdRecOrderByDeps( p, closure )
ProdRecT *p;
int closure;
{
	int		resolved;	/* dependency resolved */
	int		todo;		/* dependencies requiring resolution */
	ProdLinkT	*ordered;	/* partially ordered list */
	ProdLinkT	*link;		/* current list pointer */
	ProdRecT	*prod;		/* product record pointer */


	/* how? - this is fairly straightforward. We start with a count of
	 *  how many products need to be resolved and an empty 'resolved'
	 *  list. We pass thru the list of nodes to be resolved repeatedly
	 *  resolving what we can until either none remain to be resolved
	 *  or a pass thru the list resolves nothing.
	*/

	todo = ProdRecListLen( p );
	for( ordered = NULL, resolved = 1; todo > 0 && resolved > 0; )
	{
		resolved = 0;
		for( prod = p; prod != NULL; prod = prod->p_next )
		{
			if( prod->p_flags & PROD_DEPORD )
			{
				/* product node already processed
				*/
				continue;
			}

			if( ProdDepsOk( prod, ordered, p, closure ) )
			{
				/* node can be resolved, either because
				 *  it's prerquisites are met in the order
				 *  or it's resolution cannot be made with
				 *  the current set of products and closure
				 *  is not required.
				*/
				/*
				 * Allocate storage for link
				*/
				if( (link = ProdLinkNew()) == NULL )
				{
					/* memory error
					*/
					return( NULL );
				}

				/* Initialize link
				*/
				link->pl_next = NULL;
				link->pl_data = prod;
				/* append to list of ordered products
				*/
				ordered = ProdLinkAppend( ordered, link );
				/* mark product as ordered
				*/
				prod->p_flags |= PROD_DEPORD;
				++resolved;
				--todo;
			}
		}
	}
	if( todo != 0 )
	{
		/* there were items that were not resolved, either
		 *  closure was required and subsets were missing
		 *  OR there are cyclic dependencies.
		*/
		if( ProdRecDepCycles( p ) )
		{
			/* NYI - cycle detection may be desired
			*/
			return( NULL );
		}
		else if( closure == DEPS_CLOSED )
		{
			/* closure required or Cyclic Dependencies detected
			*/
			return( NULL );
		}
	}

	/* propogate ordering information from the ProdLinkT level down
	 *  to the ProdRec level
	*/
	p = ProdRecLinkOrder( ordered );

	/* free the link storage as it is no longer needed
	*/
	(void) ProdLinkFree( ordered );	/* put back some storage */
	return( p );
}



/*	void	ProdRecPrintList() -
 *		format and print ProdRecT list
 *
 *	given:	ProdRecT *p - pointer to head of list
 *		FILE *f - file pointer to print to
 *		int m - size of margin to leave at beginning of output line
 *	does:	print each record in the list
 *	return:	VOID
*/

void ProdRecPrintList( f, p, m )
register FILE *f;
ProdRecT *p;
int m;
{
	static StringT		margin;	/* margin padding */
	register ProdRecT	*cp;	/* current pointer */
	register int		i;	/* margin index */

	/* set up margin for printing */
	i = (m < 0) ? 0 : m;
	margin[i] = '\0';
	while( i-- )
	{
		margin[i] = '\t';
	}
	for( cp = p; cp != NULL; cp = cp->p_next )
	{
		(void) fprintf( f, "\n%sProdRecT at 0x%X\n", margin, cp );
		(void) fprintf( f, "%sp_next: 0x%X\n", margin,
			(long) cp->p_next );
		(void) fprintf( f, "%sp_cp:\n", margin );
		if( cp->p_cp == NULL )
		{
			(void) fprintf( f, "%sNULL\n", margin );
		}
		else
		{
			CtrlRecPrintList( f, cp->p_cp, m + 1 );
		}
		(void) fprintf( f, "%sp_pcode: %s\n", margin, cp->p_pcode );
		(void) fprintf( f, "%sp_vcode: %s\n", margin, cp->p_vcode );
		(void) fprintf( f, "%sp_flags: 0x%x\n", margin,
			(int) cp->p_flags );
		(void) fprintf( f, "%sp_deps: %s\n", margin,
			(cp->p_deps == NULL) ? "NULL" :
			DepListFormat( cp->p_deps ) );
	}
}

