/*	Path.c -
 *		routine for manipulating PathT
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
 *	000	05-jun-1989	ccb
 *		New
*/

#ifndef lint
static	char *sccsid = "@(#)Path.c	4.1	(ULTRIX)	7/2/90";
#endif lint

#include	<sys/param.h>
#include	<sys/types.h>
#include	<sys/dir.h>
#include	<string.h>
#include	<stdio.h>
#include	"setld.h"

/*LINTLIBRARY*/

/*	PathMatch() -
 *		determine if a path matches a pattern
 *
 *	given:	PathT p - a path
 *		char *s - a pattern (uses shell *, ?, [...])
 *	does:	match the pattern to the path
 *	return:	0 if the pattern does not match, 1 if it does.
*/

PathMatch( p, s )
PathT p;
char *s;
{
	int	found;
	char	*t;

	for( ; *p && *s; ++s, ++p )
	{
		switch( *s )
		{
		case '?':
			break;
		case '[':
			found = 0;
			while( *++s != ']' )
			{
				if( !found && *p == *s )
					found = 1;
			}
			if( !found )
				return( 0 );
			break;
		case '*':
			for( t = p; *t; ++t )
			{
				if( PathMatch( t, s + 1 ) )
					return( 1 );
			}
			return( 0 );
		default:
			/* non-meta, must match exactly */
			if( *s != *p )
				return( 0 );
		}
	}
	return( *s == *p );	/* both should be '\0' */
}



/*	char *	PathSet() -
 *		set a path object to a value
 *
 *	given:	PathT p - the path object to set
 *		char *s - the value to use
 *	does:	set the PathT to the value
 *	return:	a pointer to the initial value
*/

char *PathSet( p, s )
PathT p;
char *s;
{
	(void) strncpy( p, s, MAXPATHLEN );
	p[MAXPATHLEN] = '\0';
	return( (char *) p );
}


char *PathStripExt( p )
char *p;
{
	static PathT	s;
	char		*t;

	t = s;
	while( *p && *p != '.' )
	{
		*t++ = *p++;
	}
	*t = '\0';
	return((char *) s);
}

