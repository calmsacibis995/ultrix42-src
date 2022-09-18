#ifndef lint
static char	*sccsid = " @(#)backspace.c	1.2	(ULTRIX)	1/16/86";
#endif lint

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
*
*			Modification History
*
*	David Metsky		10-Jan-86
*
* 001	Replaced old version with BSD 4.3 version as part of upgrade.
*
*	Based on:	backspace.c	5.2		7/30/85
*
*************************************************************************/

/*
 * Backspace records
 */

#include "fio.h"

static char	bksp[]	= "backspace";
char	last_char();

f_back(a)
alist	*a;
{	unit *b;
	int n,i;
	long x,y;

	lfname = NULL;
	elist = NO;
	external = YES;
	errflag = a->aerr;
	lunit = a->aunit;
	if (not_legal(lunit))
		err(errflag, F_ERUNIT, bksp)
	b= &units[lunit];
	if(!b->ufd) return(OK);
	lfname = b->ufnm;
	if(b->uend)
	{	b->uend = NO;
		clearerr(b->ufd);
		return(OK);
	}
	if((x = ftell(b->ufd)) == 0)
		return(OK);
	if(!b->useek)
		err(errflag, F_ERNOBKSP, bksp)
	if(b->uwrt && (n = t_runc(b, errflag, bksp)))	/* sets 'reading' */
		return(n);
	if(b->url)		/* direct access, purely academic */
	{	y = x%(long)b->url;
		x -= y?y:b->url;
		fseek(b->ufd,x,0);
		return(OK);
	}
	if(!b->ufmt)		/* unformatted sequential */
	{	fseek(b->ufd,-(long)sizeof(int),1);
		fread((char *)&n,sizeof(int),1,b->ufd);
		fseek(b->ufd,-(long)n-2*sizeof(int),1);
		return(OK);
	}
	if(x == 1)			/* formatted sequential */
	{	rewind(b->ufd);
		return(OK);
	}
	while (last_char(b->ufd) != '\n')	/* slow but simple */
		;
	return(OK);
}
