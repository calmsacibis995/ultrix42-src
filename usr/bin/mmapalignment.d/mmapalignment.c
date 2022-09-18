#ifdef lint
static char *sccsid = "@(#)mmapalignment.c	4.1	ULTRIX	9/17/90";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1990 by				*
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
 * Modification History
 *
 * 10-Aug-90	sekhar
 *	Created this file for mmap support.  
 */

#include <sys/types.h>
#include <sys/errno.h>
#include <sys/sysinfo.h>

main()
{
	int i, j;

	if ( getsysinfo(GSI_MMAPALIGN, &i, sizeof(int), &j, 0) > 0)
		printf("%d\n",i);
	else
		printf("Unsupported command (errno  = %d)\n", errno);
}

