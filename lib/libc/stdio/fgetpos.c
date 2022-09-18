/*	@(#)fgetpos.c	4.1	ULTRIX	7/3/90	*/
/************************************************************************
 *									*
 *			Copyright (c) 1989 by				*
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
 *   The information in this software is subject to change  without	*
 *   notice  and should not be construed as a commitment by Digital	*
 *   Equipment Corporation.						*
 *									*
 *   Digital assumes no responsibility for the use  or  reliability	*
 *   of its software on equipment which is not supplied by Digital.	*
 *									*
 ************************************************************************/
/*
	ftell, with arbitrary argument; since we have no plans to expand
	beyond a long in ULTRIX, this is just an ftell front end.
	Mandated by ANSI X3J11.

	--Jon Reeves, June 1989.

*/

#include <stdio.h>

int
fgetpos(stream, pos)
	FILE	*stream;
	fpos_t	*pos;
{
	return ( ((*pos)=ftell(stream)) == -1L ? -1 : 0 );
}
