/*	depord.c -
 *		order subsets wrt dependencies
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
 *	depord s1 s2 ... sn
 *		print a list of subset codes sorted in dependency order
 *
 *	MODS:
 *	000	15-may-1989	ccb
 *	001	24-jul-1989	ccb
 *		add -d switch and clean up during ongoing qualification
 *	002	14-aug-1989	ccb
 *		finish qualification
*/

#ifndef lint
static	char *sccsid = "@(#)depord.c	4.1	(ULTRIX)	7/2/90";
#endif lint

#include	<sys/param.h>
#include	<sys/types.h>
#include	<sys/dir.h>
#include	<stdio.h>
#include	<errno.h>
#include	"list.h"
#include	"setld.h"

extern int	errno;		/* errno(2) */
extern char	*sys_errlist[];	/* errno(2) */
extern int	optind;		/* getopt(3) */

extern int	ctrlerr;	/* libsetld(Ctrl.o) */

#define		F_DEBUG	0x0001			/* debug flag */
#define		DEBUG	(flags&F_DEBUG)		/* debug macro */

typedef struct QT
{
	struct QT	*q_next;
	NameT		q_subset;
} QT;
QT	*QAppend();
QT	*QCreate();
QT	*QMember();
void	QPrintList();

char		*prog;
unsigned	flags = 0;

main( argc, argv )
int argc;
char **argv;
{
	int		i;		/* getopt(3), misc */
	CtrlT		*c;
	CtrlRecT	*n,		/* new data pointer */
			*t;		/* temp pointer */

	ProdRecT	*prodp, *prodl;	/* bucket pointers */
	QT		*q = NULL;

	prog = *argv;

	/* check for debugging flag */
	while( (i = getopt( argc, argv, "d" )) != EOF )
	{
		switch( i )
		{
		case 'd':
			flags |= F_DEBUG;
			(void) fprintf( stderr, "%s: debug enabled\n", prog );
			break;
		default:
			exit(1);
		}
	}
	if( (argc -= optind) == 0 )
	{
		/* called with no args, exit now to avoid
		 *  the overhead incurred in processing the
		 *  database.
		*/
		exit( 0 );
	}
	argv += optind;
		
	/* read in all of the control files
	*/
	if( (c = CtrlOpen( ".", "r" )) == NULL )
	{
		(void) fprintf( stderr, "%s: cannot open '.' (%s)\n", prog,
			sys_errlist[errno] );
		exit(1);
	}

	for( i = 0, prodl = NULL; (t = CtrlRead( c )) != NULL; ++i )
	{
		if( (n = CtrlRecNew()) == NULL )
		{
			(void) fprintf( stderr,
				"%s: cannot store control info (%s)\n", prog,
				sys_errlist[errno] );
			exit(1);
		}
		(void) CtrlRecCopy( n, t );

		if( DEBUG )
		{
			/* print a copy of the control record */
			(void) fprintf( stderr, "\n%s Ctrl Rec\n",
				CtrlGetFpath(c) );
			CtrlRecPrintList( stderr, n, 0 );
		}
		prodl = ProdRecAddCtrl( prodl, n );
		if( DEBUG )
			ProdRecPrintList( stderr, prodl, 0 );
	}
	if( DEBUG )
	{
		(void) fprintf( stderr, "%s: %d ctrl records input\n",
			prog, i );
		(void) fprintf( stderr, "%s: ctrlerr 0x%x (%s)\n",
			prog, ctrlerr, CtrlErrorString( ctrlerr ) );
	}

	/* convert the arglist *argv[] to a linked list
	 *  of QT structures
	*/
	while( argc-- )
		q = QAppend( q, QCreate( *argv++ ) );

	if( DEBUG )
	{
		(void) fprintf( stderr, "Before Ordering:\n" );
		ProdRecPrintList( stderr, prodl, 0 );
	}

	/* Order the product info
	*/
	if( (prodl = ProdRecOrderByDeps( prodl, DEPS_OPEN )) == NULL )
	{
		(void) fprintf( stderr,
			"%s: dependency ordering error\n", prog );
	}
	if( DEBUG )
	{
		(void) fprintf( stderr, "After Ordering:\n" );
		ProdRecPrintList( stderr, prodl, 0 );
		(void) fprintf( stderr, "Subset to order:\n" );
		QPrintList( q );
	}

	/* traverse the buckets
	*/
	for( prodp = prodl; prodp != NULL; prodp = prodp->p_next )
	{
		for( t = prodp->p_cp; t != NULL; t = t->ct_next )
		{
			if( QMember( q, t->ct_subset ) )
				(void) printf( "%s\n", t->ct_subset );
		}
	}
}







/*	QT	*QAppend() -
 *		concatenate lists of element type QT
 *
 *	given:	QT *s - an element
 *		QT *t - another element
 *	does:	call ListAppend()
 *	return:	a pointer to the resulting list
*/

QT *QAppend( s, t )
QT *s, *t;
{
	return( (QT *) ListAppend( (ListT *) s, (ListT *) t ) );
}


/*	QT	*QCreate() -
 *		create a named QT
 *
 *	given:	char *s - a string for the subset field of the new QT
 *	does:	create a new QT, fill the subset field
 *	return:	a pointer to the new QT or NULL on error
*/

QT *QCreate( s )
char *s;
{
	QT	*q;

	if( (q = (QT *) malloc( sizeof(QT) )) == NULL )
		return( NULL );

	(void) NameSet( q->q_subset, s );
	q->q_next = NULL;
	return(q);
}



QT *QMember( q, s )
QT *q;
char *s;
{
	if( q == NULL )
		return( NULL );

	if( !strcmp( q->q_subset, s ) )
		return( q );

	return( QMember( q->q_next, s ) );
}

void QPrintList( q )
QT *q;
{
	while( q != NULL )
	{
		(void) fprintf( stderr, "%s ", q->q_subset );
		q = q->q_next;
	}
	(void) fprintf( stderr, "\n" );
}
/*END*/

