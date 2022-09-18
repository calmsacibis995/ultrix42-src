/*	PermString.c -
 *		libsetld.a module for xlating mode information into
 *		rw-r---r-- style strings.
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
 *	000	??-mar-1989	ccb
 *		New
 *	001	16-jun-1989	ccb
 *		lint
*/

#ifndef lint
static	char *sccsid = "@(#)PermString.c	4.1	(ULTRIX)	7/2/90";
#endif lint

#include	<sys/types.h>
#include	<sys/stat.h>
#include	<string.h>

/*LINTLIBRARY*/

#ifdef TESTMAIN
/* Compile with TESTMAIN defined to test
 *  the subr as a free standing unit.
*/
#include	<stdio.h>

main( argc, argv )
int argc;
char *argv[];
{
	int perms;
	char buf[80];
	char *PermString();

	while( gets( buf ) != NULL )
	{
		(void) sscanf( buf, "%o", &perms );
		(void) printf( "%s\n", PermString( (u_short) perms ) );
	}
}
#endif



/*	PermString() -
 *		translate perms to a rw-rw-r-- type string
 *
 *	given:	permission bits to translate
 *	does:	assemble a permission string that represents the bits
 *	return:	a pointer to the string
*/

char *PermString( perms )
u_short perms;
{
	static char	*xtab[] =	/* perm stringlets */
			{
				"---", "--x", "-w-", "-wx",
				"r--", "r-x", "rw-", "rwx"
			};

	static char	outbuf[10];	/* "rw-rw-r--\0" */
	int		i;		/* loop control */
	u_short		p;		/* temp perms storage */

	for( p = perms, i = 6; i >= 0; i -= 3 )
	{
		(void) strncpy( outbuf+i, xtab[p & 07], 3 );
		p >>= 3;
	}
	/* doctor in the other bits */
	if( perms & S_ISUID )
		outbuf[2] = 's';	/* set-uid */

	if( perms & S_ISGID )
		outbuf[5] = 's';	/* set-gid */

	if( perms & S_ISVTX )
		outbuf[8] = 't';	/* sticky */

	return( outbuf );
}

