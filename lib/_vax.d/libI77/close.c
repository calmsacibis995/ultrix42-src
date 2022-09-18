#ifndef lint
static char	*sccsid = " @(#)close.c	1.2	(ULTRIX)	1/16/86";
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
*	Based on:	close.c		5.2		7/30/85
*
*************************************************************************/

/*
 * f_clos(): f77 file close
 * t_runc(): truncation
 * f_exit(): I/O library exit routines
 */

#include "fio.h"

static char FROM_OPEN[] =	"\2";
static char clse[]	=	"close";

f_clos(a) cllist *a;
{	unit *b;
	int n;

	lfname = NULL;
	elist = NO;
	external = YES;
	errflag = a->cerr;
	lunit = a->cunit;
	if(not_legal(lunit)) return(OK);
	if(lunit==STDERR && (!a->csta || *a->csta != FROM_OPEN[0]))
		err(errflag,F_ERUNIT,"can't close stderr");
	b= &units[lunit];
	if(!b->ufd) return(OK);
	if(a->csta && *a->csta != FROM_OPEN[0])
		switch(lcase(*a->csta))
		{
	delete:
		case 'd':
			fclose(b->ufd);
			if(b->ufnm) unlink(b->ufnm); /*SYSDEP*/
			break;
		default:
	keep:
		case 'k':
			if(b->uwrt && (n=t_runc(b,errflag,clse))) return(n);
			fclose(b->ufd);
			break;
		}
	else if(b->uscrtch) goto delete;
	else goto keep;
	if(b->ufnm) free(b->ufnm);
	b->ufnm=NULL;
	b->ufd=NULL;
	return(OK);
}

f_exit()
{
	ftnint lu, dofirst = YES;
	cllist xx;
	xx.cerr=1;
	xx.csta=FROM_OPEN;
	for(lu=STDOUT; (dofirst || lu!=STDOUT); lu = ++lu % MXUNIT)
	{
		xx.cunit=lu;
		f_clos(&xx);
		dofirst = NO;
	}
}

t_runc (b, flag, str)
unit	*b;
ioflag	flag;
char	*str;
{
	long	loc;

	if (b->uwrt)
		fflush (b->ufd);
	if (b->url || !b->useek || !b->ufnm)
		return (OK);	/* don't truncate direct access files, etc. */
	loc = ftell (b->ufd);
	if (truncate (b->ufnm, loc) != 0)
		err (flag, errno, str)
	if (b->uwrt && ! nowreading(b))
		err (flag, errno, str)
	return (OK);
}
