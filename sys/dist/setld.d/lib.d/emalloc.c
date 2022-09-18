/*	unsigned emalloc() -
 *		allocate memory, die on error.
 *
 *	given:	number of bytes to allocate.
 *	does:	calls malloc checking malloc return value,
 *		printing error diagnostic on malloc failure and exiting.
 *	return:	pointer to allocated memory if successful.
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
 *	000	ccb	1/1/86
 *	001	29-apr-1989	ccb
 *		add declaration of exit() to silence lint.
*/

#ifndef lint
static	char *sccsid = "@(#)emalloc.c	4.1	(ULTRIX)	7/2/90";
#endif lint
#include	<stdio.h>

/*LINTLIBRARY*/

extern void	exit();		/* exit(3) */
extern char	*malloc();	/* malloc(2) */

extern char	*prog;

char *emalloc(n)
unsigned n;
{
	char *p;

	if( (p = malloc(n)) == NULL )
	{
		(void) fprintf( stderr, "%s: out of memory.\n", prog );
		exit(1);
	}
	return(p);
}

