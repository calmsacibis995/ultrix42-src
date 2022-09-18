/*	@(#)getc.c	4.3	ULTRIX	9/10/90	*/
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

	Duplicate of stdio.h function getc for ANSI/POSIX
	compliance.

	--Jon Reeves, November 1989.

*/

#include <stdio.h>

#undef	getc

int
getc(p)
	FILE	*p;
{
	return ( --(p)->_cnt>=0? *(p)->_ptr++&0377:_filbuf(p) );
}
