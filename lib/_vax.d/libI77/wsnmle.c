#ifndef lint
static char	*sccsid = " @(#)wsnmle.c	4.1	(ULTRIX)	7/17/90";
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
*	Based on:	wsnmle.c	5.2		8/2/85
*
*************************************************************************/

/*
 *		name-list write
 */

#include "fio.h"
#include "lio.h"
#include "nmlio.h"
#include <strings.h>

int l_write(), t_putc();
LOCAL char nml_wrt[] = "namelist write";
char namelistkey_ = '&';

s_wsne(a) namelist_arglist *a;
{
	int n, first;
	struct namelistentry *entries;
	int *dimptr, *spans, ndim, nelem, offset, vlen, vtype, number;
	char *nmlist_nm, *cptr;

	nmlist_nm = a->namelist->namelistname;
	reading = NO;
	formatted = NAMELIST;
	fmtbuf = "ext namelist io";
	if(n=c_le(a,WRITE)) return(n);
	putn = t_putc;
	line_len = LINE-1;	/* so we can always add a comma */
	curunit->uend = NO;
	leof = NO;
	if(!curunit->uwrt && ! nowwriting(curunit)) err(errflag, errno, nml_wrt)

	/* begin line with " &namelistname " */
	if(recpos != 0)
		PUT('\n');  /* PUT() adds blank */
	PUT(namelistkey_);
	while(*nmlist_nm != '\0') PUT(*nmlist_nm++);
	PUT(' ');

	/* now loop through entries writing them out */
	entries = a->namelist->names;
	first = 1;
	while( entries->varname[0] != 0 )
	{
		/* write out variable name and '=' */
		cptr = entries->varname;
		chk_len( strlen(cptr) + 3);
		if(first++ != 1) PUT(',');
		PUT(' ');
		while( *cptr != '\0') PUT(*cptr++);
		PUT('=');

		/* how many value are there? */
		if( (dimptr = entries->dimp) == NULL ) number = 1;
		else number = dimptr[1];
		/* what is element length? */
		vlen = entries->typelen;
		/* get type */
		vtype = entries->type;
		
		if(n=l_write( &number, entries->varaddr, vlen, vtype ))
				err(errflag,n,nml_wrt);
		entries++;
	}
	PUT('\n');
	PUT(namelistkey_);
	cptr = "end\n";
	while(*cptr != '\0') PUT(*cptr++);
	return(OK);
}

LOCAL
t_putc(c) char c;
{
	if(c=='\n') { 
		recpos=0;
	} else if(recpos == 0) {
		putc(' ',cf);		/* for namelist,	   */
		recpos = 2;		/* never print in column 1 */
	} else {
		recpos++;
	}
	putc(c,cf);
	return(OK);
}
