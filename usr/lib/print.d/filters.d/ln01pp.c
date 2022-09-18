#ifndef lint
static	char	*sccsid = "@(#)ln01pp.c	4.1		7/2/90";
#endif lint

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

/************************************************************************/
/*	ln01pr - print filter for print funtion "pr"			*/
/*		 adjusts page size and calls pr				*/
/************************************************************************/
#include <stdio.h>
#define PR "/bin/pr"
main(argc, argv)
int	argc;
char **argv;
{
	fprintf(stdout,"\033\1333300t");	/* set full size page	*/
	fprintf(stdout,"\033\1331;3300");/* set margins within page*/
	execl(PR, argv[0], argv[1], argv[2], argv[3], argv[4], 0);
			fprintf(stderr,"cannot execl %s", PR);
			exit(2);
}
