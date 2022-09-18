#ifndef lint
static char	*sccsid = " @(#)malloc_.c	4.1	(ULTRIX)	7/17/90";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1986 by				*
 *		Digital Equipment Corporation, Maynard, MA		*
 *			All rights reserved.				*
 *									*
 *   This software is furnished under a license and may be used and	*
 *   copied  only  in accordance with the terms of such license and	*
 *   with the  inclusion  of  the  above  copyright  notice.   This	*
 *   software  or  any  other copies thereof may not be provided or	*
 *   otherwise made available to any other person.  No title to and	*
 *   ownership of the software is hereby transferred.			*
 *									*
 *   This software is  derived  from  software  received  from  the	*
 *   University    of   California,   Berkeley,   and   from   Bell	*
 *   Laboratories.  Use, duplication, or disclosure is  subject  to	*
 *   restrictions  under  license  agreements  with  University  of	*
 *   California and with AT&T.						*
 *									*
 *   The information in this software is subject to change  without	*
 *   notice  and should not be construed as a commitment by Digital	*
 *   Equipment Corporation.						*
 *									*
 *   Digital assumes no responsibility for the use  or  reliability	*
 *   of its software on equipment which is not supplied by Digital.	*
 *									*
 ************************************************************************/

/************************************************************************
*
*			Modification History
*
*	David Metsky		10-Jan-86
*
* 001	Added from BSD 4.3 version as part of upgrade.
*
*	Based on:	malloc_.c	5.1		6/7/85
*
*************************************************************************/

/*
 *	allows f77 programs to dynamicly allocate space
 *	three routines:
 *		call malloc(need, addr)
 *		integer need, addr
 *
 *		call free(addr)
 *		integer addr
 *
 *		call falloc( nelem, elsize, clean, basevec, addr, offset )
 *		integer nelem, elsize, clean, addr, offset
 *		dimension basevec(1)
 *
 *	malloc() & falloc() alloc space and put address in 'addr', 0 if can't
 *	do it.  free() frees a block.  malloc() gets a block of at least
 *	'need' bytes; falloc() gets at least nelem*elsize bytes, zeros
 *	the block if clean=1, and returns an offset so that the block
 *	can be referenced as basevec(offset+1)...basevec(offset+nelem)
 *	in the calling program.  falloc() gets an extra element so that
 *	all the elements will be in the block even if address arithmetic
 *	involves truncation.
 */

char *calloc(), *malloc();

malloc_( need, addr )
int *need; char **addr;
{
	*addr = malloc( *need );
}

free_( addr )
char **addr;
{
	free( *addr );
}

falloc_( nelem, elsize, clean, basevec, addr, offset )
int *nelem, *elsize, *clean, *offset;
char **addr, *basevec;
{
	if( *clean == 1 )
		*addr = calloc( *nelem + 1, *elsize );
	else
		*addr = malloc( (*nelem + 1) * *elsize );
		
	if( *addr != 0 )
		*offset = ((*addr - basevec) / *elsize) + 1;
	else
		*offset = 0;

}
