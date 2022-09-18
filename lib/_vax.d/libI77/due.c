#ifndef lint
static char	*sccsid = " @(#)due.c	1.2	(ULTRIX)	1/16/86";
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
*	Based on:	due.c		5.1		6/7/85
*
*************************************************************************/

/*
 * direct unformatted external i/o
 */

#include "fio.h"

LOCAL char rdue[] = "read due";
LOCAL char wdue[] = "write due";

s_rdue(a) cilist *a;
{
	int n;
	reading = YES;
	if(n=c_due(a,READ)) return(n);
	if(curunit->uwrt && ! nowreading(curunit)) err(errflag, errno, rdue);
	return(OK);
}

s_wdue(a) cilist *a;
{
	int n;
	reading = NO;
	if(n=c_due(a,WRITE)) return(n);
	curunit->uend = NO;
	if(!curunit->uwrt && ! nowwriting(curunit)) err(errflag, errno, wdue)
	return(OK);
}

LOCAL
c_due(a,flag) cilist *a;
{	int n;
	lfname = NULL;
	elist = NO;
	sequential=formatted=NO;
	recpos = reclen = 0;
	external = YES;
	errflag = a->cierr;
	endflag = a->ciend;
	lunit = a->ciunit;
	if(not_legal(lunit)) err(errflag,F_ERUNIT,rdue+5);
	curunit = &units[lunit];
	if (!curunit->ufd && (n=fk_open(flag,DIR,UNF,(ftnint)lunit)) )
		err(errflag,n,rdue+5)
	cf = curunit->ufd;
	elist = YES;
	lfname = curunit->ufnm;
	if (curunit->ufmt) err(errflag,F_ERNOUIO,rdue+5)
	if (!curunit->useek || !curunit->url) err(errflag,F_ERNODIO,rdue+5)
	if (fseek(cf, (long)((a->cirec-1)*curunit->url), 0) < 0)
		return(due_err(rdue+5));
	else
		return(OK);
}

e_rdue()
{
	return(OK);
}

e_wdue()
{/*	This is to ensure full records. It is really necessary. */
	int n = 0;
	if (curunit->url!=1 && recpos!=curunit->url &&
	    (fseek(cf, (long)(curunit->url-recpos-1), 1) < 0
		|| fwrite(&n, 1, 1, cf) != 1))
			return(due_err(rdue+5));
	return(OK);
}
