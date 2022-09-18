/*	verify.c
 *		library routines used for subset verification
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
 *	000	2-feb-1989	Chas Bennett
 *		new.
 *
 *	001	1-may-1989	Chas Bennett
 *		modify FVerify so that files of type 'l' will match
 *		any type.
*/

#ifndef lint
static	char *sccsid = "@(#)verify.c	4.1	(ULTRIX)	7/2/90";
#endif lint

#include	<sys/param.h>
#include	<sys/types.h>
#include	<sys/stat.h>
#include	<sys/dir.h>
#include	<stdio.h>
#include	"setld.h"


/*LINTLIBRARY*/

/*	InvRecT *FVerify() -
 *		Compares the stat information from a file against an inventory
 *	record.
 *
 *	given: 	InvRecT *p - a pointer to the record to be verified.
 *	does:	looks up the file in the filesystem. For each attribute
 *		requested that doesn't match the inventory record, a bit
 *		is set in a return mask which is included in p->i_flags
 *	return:	a pointer to the static InvRecT used in the verify
*/

InvRecT *FVerify( p )
InvRecT *p;
{
	unsigned	mask;	/* output mask */
	char		*path;	/* path info from InvRecT */
	InvRecT		*real;	/* inventory record for actual file */
	struct stat	stb;	/* stat buffer */
	char		type;	/* type info from InvRecT */

	path = p->i_path;
	if( lstat( path, &stb ) )
	{
		p->i_vflags |= I_PATH;
		return( NULL );
	}
	real = StatToInv( &stb );

	mask = 0;
	type = p->i_type;

	if( type == 'f' && p->i_size != real->i_size )
		mask |= I_SIZE;

	if( type == 'f' && p->i_sum != (real->i_sum = CheckSum(path)) )
		mask |= I_SUM;

	if( p->i_uid != real->i_uid )	/* user id */
		mask |= I_UID;

	if( p->i_gid != real->i_gid )	/* group id */
		mask |= I_GID;

	if( PERM( p->i_mode ) != PERM( real->i_mode ) )	/* permissions */
		mask |= I_PERM;

	if( strcmp( p->i_date, real->i_date  ) )
		mask |= I_DATE;

	if( type != real->i_type && type != 'l' )
		mask |= I_TYPE;

	/* referent checking is not done here, calling functions
	 *  which need it can implement checking there
	*/

	p->i_vflags = mask;
	return( real );
}

