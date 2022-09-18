#ifndef lint
static char	*sccsid = " @(#)douio.c	1.2	(ULTRIX)	1/16/86";
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
*	Based on:	douio.c		5.1		6/7/85
*
*************************************************************************/

/*
 * unformatted external i/o
 */

#include "fio.h"

LOCAL char *eor = "eor/uio";
LOCAL char *uio = "uio";

LOCAL
do_us(number,ptr,len) ftnint *number; ftnlen len; char *ptr;  /* sequential */
{
	if(reading)
	{
		recpos += *number * len;
		if (recpos > reclen) {
			recpos -= *number * len;
			e_rsue(); /* in case tries another read */
			err(errflag,F_EREREC,eor);
		}

		if (fread(ptr,(int)len,(int)(*number),cf) != *number)
			return(due_err(uio));
	}
	else
	{
		reclen += *number * len;
		fwrite(ptr,(int)len,(int)(*number),cf);
	}
	return(OK);
}

do_uio(number,ptr,len) ftnint *number; ftnlen len; char *ptr;
{
	if(sequential)
		return(do_us(number,ptr,len));
	else
		return(do_ud(number,ptr,len));
}

LOCAL
do_ud(number,ptr,len) ftnint *number; ftnlen len; char *ptr;  /* direct */
{
	recpos += *number * len;
	if(recpos > curunit->url && curunit->url!=1)
		err(errflag,F_EREREC,eor);
	if(reading)
	{
		if (fread(ptr, (int)len, (int)(*number), cf) != *number)
			return(due_err(uio));
	}
	else
		fwrite(ptr,(int)len,(int)(*number),cf);
	return(OK);
}

due_err(s) char *s;
{
	if(feof(cf))
		err(endflag,EOF,s)
	else
	{	clearerr(cf);
		err(errflag,errno,s)
	}
}
