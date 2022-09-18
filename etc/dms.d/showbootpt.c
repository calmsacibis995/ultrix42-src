#ifndef lint
static	char	*sccsid = "%W	(ULTRIX)	9/09/90";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1986 by				*
 *		Digital Equipment Corporation, Maynard, MA		*
 *			All rights reserved.	/			*
 *									*
 *   This software is furnished under a license and may be used and	*
 *   copied  only  in accordance with the terms of such license and	*
 *   with the  inclusion  of  the  above  copyright  notice.   This	*
 *   software  or  any  other copies thereof may not be provided or	*
 *   otherwise made available to any other person.  No title to and	*
 *   ownership of the software is hereby transferred.			*
 *									*
 *   This software is  derived  from  software  received  from  the	*
 *   University    of   California,   Berkeley,   and   from   Bell	*
 *   Laboratories.  Use, duplication, or disclosure is  subject  to	*
 *   restrictions  under  license  agreements  with  University  of	*
 *   California and with AT&T.						*
 *									*
 *   The information in this software is subject to change  without	*
 *   notice  and should not be construed as a commitment by Digital	*
 *   Equipment Corporation.						*
 *									*
 *   Digital assumes no responsibility for the use  or  reliability	*
 *   of its software on equipment which is not supplied by Digital.	*
 *									*
 ************************************************************************/
#include <stdio.h>
#include <sys/file.h>
#include <sys/types.h>
#include "/sys/h/sysinfo.h"

#define DEBUG 0
#define BOOTDEVLEN 80

/***************************************************************************/
main()   /*showboot(i)*/
{

	FILE *fp;

	char    bootdev[BOOTDEVLEN];
	char	console[4];

	fp=fopen("/tmp/showbootmop","w");

	getsysinfo(GSI_BOOTDEV, bootdev, sizeof(bootdev));
	getsysinfo(GSI_CONSTYPE, console, sizeof(console));

	/*
	 * Determine if old or new console in use.  If new use
	 * different bootstrap command.
	 */

	if(strncmp(console,"TCF0",4)) {
	  fprintf(fp,"\t>> setenv bootpath mop(%c)\n", bootdev[4]);
	}
	else {
	  fprintf(fp,"\t>> setenv boot \"%s -a\"\n", bootdev);
	}
	fclose(fp);
}
