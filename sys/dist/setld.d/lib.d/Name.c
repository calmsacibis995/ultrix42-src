/*	Name.c -
 *		libsetld. Routine for manipulating NameT
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
 *	001	890605	ccb	New.
 *	002	24-jul-1989	ccb
 *		wrote obligatory comment headers for all routines
 *		lint
*/

#ifndef lint
static	char *sccsid = "@(#)Name.c	4.1	(ULTRIX)	7/2/90";
#endif lint

#include	<sys/param.h>
#include	<sys/types.h>
#include	<sys/dir.h>
#include	<string.h>
#include	<stdio.h>
#include	"setld.h"

/*LINTLIBRARY*/

/*	char	*NameSet() -
 *		set NameT value
 *
 *	given:	NameT n - NameT to set
 *		char *s - value to use
 *	does:	sets value of n to s with all type specific checking
 *	return:	the value as set (may have been truncated)
*/

char *NameSet( n, s )
NameT n;
char *s;
{
	(void) strncpy( n, s, NAMELEN );
	n[NAMELEN] = '\0';
	return( n );
}


/*	char	*NameGetPcode() -
 *		get product code from a name
 *
 *	given:	NameT n - the name to use
 *	does:	extract the pcode section into a static buffer
 *	return:	a pointer to the static buffer
*/

char *NameGetPcode( n )
NameT n;
{
	static CodeT	c;

	(void) strncpy( c, n, CODELEN );
	c[CODELEN] = '\0';
	return( c );
}


/*	char	*NameGetVcode() -
 *		get version code from a name
 *
 *	given:	NameT n - the name to use
 *	does:	extract the vcode section of the name into a static buffer
 *	return:	a pointer to the static buffer
*/

char *NameGetVcode( n )
NameT n;
{
	static CodeT	c;

	(void) strncpy( c, n + strlen(n) - CODELEN, CODELEN );
	c[CODELEN] = '\0';
	return( c );
}

