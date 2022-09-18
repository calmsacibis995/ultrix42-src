#ifndef lint
static char	*sccsid = " @(#)getc_.c	1.2	(ULTRIX)	1/16/86";
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
*	Based on:	getc_.c		5.1		6/7/85
*
*************************************************************************/

/*
 * get a character from the standard input
 *
 * calling sequence:
 *	integer getc
 *	ierror = getc (char)
 * where:
 *	char will be read from the standard input, usually the terminal
 *	ierror will be 0 if successful; a system error code otherwise.
 */

#include	"../libI77/f_errno.h"
#include	"../libI77/fiodefs.h"

extern unit units[];	/* logical units table from iolib */

long getc_(c, clen)
char *c; long clen;
{
	int	i;
	unit	*lu;

	lu = &units[STDIN];
	if (!lu->ufd)
		return((long)(errno=F_ERNOPEN));
	if (lu->uwrt && ! nowreading(lu))
		return((long)errno);
	if ((i = getc (lu->ufd)) < 0)
	{
		if (feof(lu->ufd))
			return(-1L);
		i = errno;
		clearerr(lu->ufd);
		return((long)i);
	}
	*c = i;
	return(0L);
}
