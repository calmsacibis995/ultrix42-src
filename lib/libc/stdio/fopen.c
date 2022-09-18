#ifndef lint
static char *sccsid = "@(#)fopen.c	4.1	ULTRIX	7/3/90";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1985 by				*
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
/************************************************************************
 *			Modification History
 * 	Lu Anne Van de Pas, 22-may-1986
 * 002  returned proper error number in errno if error didn't occur in
 *	open system call.
 *
 *	David L Ballenger, 28-May-1985
 * 001	Call common open routine for fopen() and freopen().  This 
 *	routine correctly opens files for append mode.
 *
 ************************************************************************/

#include	<stdio.h>

FILE *
fopen(file, mode)
	char 	*file,
		*mode;

{
	extern	FILE *_doopen();
	extern	FILE *_findiop();
	FILE *fp;

	fp = _findiop();
	if (fp == NULL) 
		/* errno is set by _findiop so we wouldn't worry about
		 * it here - 002 
		 */ 
		return (NULL);

	return(_doopen(file,mode,fp));

}
