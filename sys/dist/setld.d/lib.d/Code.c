/*	Code.c -
 *		CodeT routines
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
 *	000	20-jun-1989	ccb
 *		culled from inv.c
 *	001	16-aug-1989	ccb
 *		force '\0' string terminator in CodeSet
*/

#ifndef lint
static	char *sccsid = "@(#)Code.c	4.1	(ULTRIX)	7/2/90";
#endif lint

#include	<sys/param.h>
#include	<sys/types.h>
#include	<sys/dir.h>
#include	<string.h>
#include	<stdio.h>
#include	"setld.h"

/*LINTLIBRARY*/

/*	char	*CodeSet() -
 *		set a CodeT to a value
 *
 *	given:	CodeT s - code to set
 *		char *t - value to use
 *	does:	set s to value of t
 *	return:	address of s
*/

char *CodeSet( s, t )
CodeT s;
char *t;
{
	(void) strncpy( s, t, CODELEN );
	s[CODELEN] = '\0';
	return( s );
}


