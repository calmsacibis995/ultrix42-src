#ifndef lint
static char	*sccsid = " @(#)rsfe.c	4.1	(ULTRIX)	7/17/90";
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
*	Based on:	rsfe.c		5.2		7/30/85
*
*************************************************************************/

/*
 * read sequential formatted external
 */

#include "fio.h"

extern int rd_ed(),rd_ned();
int x_rnew(),x_getc(),x_tab();

LOCAL char rsfe[] = "read sfe";

s_rsfe(a) cilist *a; /* start */
{	int n;
	reading = YES;
	sequential = YES;
	if(n=c_sfe(a,READ,SEQ,rsfe)) return (n);
	if(curunit->url) err(errflag,F_ERNOSIO,rsfe)
	if(curunit->uwrt && ! nowreading(curunit)) err(errflag, errno, rsfe)
	getn= x_getc;
	doed= rd_ed;
	doned= rd_ned;
	donewrec = dorevert = doend = x_rnew;
	dotab = x_tab;
	if(pars_f()) err(errflag,F_ERFMT,rsfe)
	fmt_bg();
	return(OK);
}

LOCAL
x_rnew()			/* find next record */
{	int ch;
	if(curunit->uend)
		return(EOF);
	while((ch=getc(cf))!='\n' && ch!=EOF);
	if(feof(cf))
	{	curunit->uend = YES;
		if (recpos==0) return(EOF);
	}
	cursor=recpos=reclen=0;
	return(OK);
}

LOCAL
x_getc()
{	int ch;
	if(curunit->uend) return(EOF);
	if((ch=getc(cf))!=EOF && ch!='\n')
	{	recpos++;
		return(ch);
	}
	if(ch=='\n')
	{	ungetc(ch,cf);
		return(ch);
	}
	if(feof(cf)) curunit->uend = YES;
	return(EOF);
}

e_rsfe()
{	int n;
	n=en_fio();
	fmtbuf=NULL;
	return(n);
}
