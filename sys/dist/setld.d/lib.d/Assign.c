/*	Assign.c -
 *		mainipulate AssignT
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
 *	000	890606	ccb	New
 *	001	24-jul-1989	ccb
 *		Include <sys/dir.h> for setld.h
 *		Rename AssignParse() to AssignScan() to conform to libsetld
 *			conventions
 *		Fix AssignScan() bug, value fields we comming back with
 *			unwanted trailing newlines.
 *
*/

#ifndef lint
static	char *sccsid = "@(#)Assign.c	4.1	(ULTRIX)	7/2/90";
#endif lint

#include	<sys/param.h>
#include	<sys/types.h>
#include	<sys/dir.h>
#include	<stdio.h>
#include	"setld.h"


/*LINTLIBRARY*/


/*	char	*AssignGetName() -
 *		return name portion of a AssignT
 *
 *	given:	AssignT *a - assignment
 *	does:	get the name portion
 *	return:	a pointer to the name portion
*/

char *AssignGetName( a )
AssignT *a;
{
	return( a->a_name );
}


/*	char	*AssignGetVal() -
 *		get AssignT( value )
 *
 *	given:	AssignT *a - assign to use
 *	does:	finds the value associated with the assign
 *	return:	a pointer to the value (a string)
*/

char *AssignGetVal( a )
AssignT *a;
{
	return( a->a_val );
}


/*	AssignT	*AssignScan() -
 *		xlate string data into an assign
 *
 *	given:	char *s - string to format
 *	does:	breaks the name=value string into an assign.
 *	return:	a pointer to a static AssignT representing the assign. Will
 *		return NULL for syntax error ( =foo, baz )
*/
 
AssignT *AssignScan( s )
char *s;
{
	static AssignT	a;
	char		*t;

	/* two part operation -
	 *  copy name.
	*/
	for( t = a.a_name; *s && *s != '=' ; ++s, ++t )
	{
		*t = *s;
	}
	*t = '\0';

	if( t == a.a_name || *s != '=' )
	{
		/* (t == a.a_name) - assign to nothing
		 * (*s != '=') - no equal sign
		*/
		return( NULL );
	}

	/* copy value
	*/
	++s;

	for( t = a.a_val; *s && *s != '\n'; ++s, ++t )
	{
		*t = *s;
	}
	*t = '\0';

	return( &a );
}



