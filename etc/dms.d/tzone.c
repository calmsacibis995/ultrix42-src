#ifndef lint
static char *sccsid = "@(#)tzone.c	4.1	ULTRIX	7/2/90";
#endif lint
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
 * Modification history:						*
 * 000 - 4/19/89 	TungNing Cherng Created				* 
 *									*
 ************************************************************************/
#include <sys/time.h>
struct timeval tv;
static struct timezone tz;
main()
{
	gettimeofday(&tv, &tz);
	if (tz.tz_dsttime==0)
		printf("%d\n",tz.tz_minuteswest/60);
	else
		printf("%d dst %d\n",tz.tz_minuteswest/60,tz.tz_dsttime);
}
