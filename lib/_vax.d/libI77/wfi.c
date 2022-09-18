#ifndef lint
static char	*sccsid = " @(#)wfi.c	4.1	(ULTRIX)	7/17/90";
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
* 001	Added from BSD 4.3 version as part of upgrade.
*
*	Based on:	wfi.c		5.1		6/7/85
*
*************************************************************************/

/*
 * internal (character array) i/o: write formatted, sequential and direct
 */

#include "fio.h"

extern int w_ed(),w_ned();
extern int z_wnew(), z_putc(), z_tab();

LOCAL
c_wfi()
{
	reading = NO;
	doed=w_ed;
	doned=w_ned;
	putn=z_putc;
	doend = donewrec = z_wnew;
	dorevert = z_wnew;
	dotab = z_tab;
}

s_wsfi(a) icilist *a;
{
	int n;

	c_wfi();
	if( n = c_si(a) ) return (n);
	if(pars_f()) err(errflag,F_ERFMT,"wsfio")
	fmt_bg();
	return( OK );
}

s_wdfi(a) icilist *a;
{
	int n;

	c_wfi();
	if( n = c_di(a) ) return (n) ;
	if(pars_f()) err(errflag,F_ERFMT,"wdfio")
	fmt_bg();
	return( OK );
}

e_wsfi()
{
	int n;
	n = en_fio();
	fmtbuf = NULL;
	return(n);
}


e_wdfi()
{
	return(e_wsfi());
}
