/*	String.c -
 *		routines for manipulating StringT
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
*/

#ifndef lint
static	char *sccsid = "@(#)String.c	4.1	(ULTRIX)	7/2/90";
#endif lint

#include	<sys/param.h>
#include	<sys/types.h>
#include	<sys/dir.h>
#include	<stdio.h>
#include	<string.h>
#include	"setld.h"

/*LINTLIBRARY*/


char *StringSet( s, p )
StringT s;
char *p;
{
	(void) strncpy( s, p, STRINGLEN );
	s[STRINGLEN] = '\0';
	return( s );
}


char *StringUnquote( s )
register StringT s;
{
	register char	*t;
	char		*u;

	for( u = t = s; *s != '\0'; s++ )
	{
		if( *s == '"' )
			continue;
		*t++ = *s;
	}
	*t = '\0';
	return( u );
}

char *StringToken( s, t )
char *s, *t;
{
	return( strtok( s, t ) );
}


