/*	Flags.c
 *		routines for manipulating FlagsT
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
 *		forgot to set this up before. Flags.c is about 2 weeks
 *			old now.
 *	001	24-jul-1989	ccb
 *		lint
*/

#ifndef lint
static	char *sccsid = "@(#)Flags.c	4.1	(ULTRIX)	7/2/90";
#endif lint

#include	<sys/param.h>
#include	<sys/types.h>
#include	<sys/dir.h>
#include	<stdio.h>
#include	"setld.h"

/*LINTLIBRARY*/

/*	FlagsT	FlagsScan() -
 *		xlate string to flag
 *
 *	given:	StringT s
 *	does:	xlate value in s to a FlagsT
 *	return:	the FlagsT value
*/
 
FlagsT FlagsScan( s )
StringT s;
{
	FlagsT	f;

	(void) sscanf( s, "%x", &f );
	return( f );
}

