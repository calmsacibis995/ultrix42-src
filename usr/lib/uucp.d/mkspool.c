#ifndef lint
static char sccsid[] = "@(#)mkspool.c	4.1 (decvax!larry) 7/2/90";
#endif

/*********************************
 * mkspool.c
 *
 *	Program to make all spool subdirectories for 
 *	the specified systems.
 *
 *					
 *********************************/




/************************************************************************
 *									*
 *			Copyright (c) 1984 by				*
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



#include "uucp.h"

main(argc,argv)
char **argv; int argc;
{
int orig_uid = getuid();
char tempname[NAMESIZE];

	setgid(getegid());
	setuid(geteuid());

	uucpname(tempname); /* init. subdir stuff */
	while(argc>1) {
		argc--; 
		mkspooldirs(*++argv);
		printf("made spool directories for: %s\n", *argv);
	}
}

cleanup(code)
int code; 
{
	exit(code);
}


