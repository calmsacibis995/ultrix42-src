#ifndef lint
static char *sccsid = "@(#)license.c	4.1 (ULTRIX) 7/2/90";
#endif
/************************************************************************
 *									*
 *			Copyright (c) 1986 by				*
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

/* This program will open the /upgrade file and print to the standard
 * ouput the current system limit.  This will be used in the /etc/rc
 * file to report the user limit when the system goes multi-user.
 */

#include <stdio.h>
#include <sys/file.h>
#include <limits.h>
main()
{
	int fd, cc;
	int byte;
	if((fd = open("/upgrade",O_RDONLY)) < 0)
	{
		printf("System supports 2 users (the default).\n");
		(void) exit();
	}
	cc = read(fd,&byte,1);
	close(fd);
	if ( cc != 1)
	{
		printf("??");
		(void) exit();
	}
	switch (byte)
	{
		case 0:
			printf("System supports more than 64 users.\n");
			break;
		default:
			printf("System supports %d users.\n",byte);
			break;
	}
}
