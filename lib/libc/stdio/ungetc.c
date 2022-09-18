#ifndef lint
static char sccsid[] = "@(#)ungetc.c	4.1     (ULTRIX)        7/3/90";
#endif
  
/* Includes changes from   "ungetc.c	5.3 (Berkeley) 3/26/86";	*/

/*
 *			Copyright (c) 1985, 1989 by
 *		Digital Equipment Corporation, Maynard, MA
 *			All rights reserved.
 *								
 *	This software is furnished under a license and may be used and
 *	copied  only  in accordance with the terms of such license and
 *	with the  inclusion  of  the  above  copyright  notice.   This
 *	software  or  any  other copies thereof may not be provided or
 *	otherwise made available to any other person.  No title to and
 *	ownership of the software is hereby transferred.		
 *								
 *	This software is  derived  from  software  received  from  the
 *	University    of   California,   Berkeley,   and   from   Bell
 *	Laboratories.  Use, duplication, or disclosure is  subject  to
 *	restrictions  under  license  agreements  with  University  of
 *	California and with AT&T.					
 *								
 *	The information in this software is subject to change  without
 *	notice  and should not be construed as a commitment by Digital
 *	Equipment Corporation.					
 *								
 *	Digital assumes no responsibility for the use  or  reliability
 *	of its software on equipment which is not supplied by Digital.
*/

/*
 *	Modification History.
 *
 * 001	Jon Reeves, 1989-Nov-01
 *	For X/Open and other standards, use the _bufsiz field as a
 *	pushback buffer when there is no buffer in use.  Also, in POSIX
 *	mode only, clear EOF.  Also, truncate the return value to
 *	unsigned char for the normal return case.
 */

#include <stdio.h>

ungetc(c, iop)
	register FILE *iop;
{
	if (c == EOF || (iop->_flag & (_IOREAD|_IORW)) == 0 )
		return (EOF);

/* Steal underused field if there's no real buffer yet */
	if (iop->_ptr == NULL)
		iop->_ptr = (char *)&(iop->_bufsiz) + sizeof(iop->_bufsiz);

/* Check to see if temporary storage area is full */
	if (iop->_ptr == (char *)&(iop->_bufsiz))
		return (EOF);

	if (iop->_ptr == iop->_base)
		if (iop->_cnt == 0)
			iop->_ptr++;
		else
			return (EOF);

	iop->_cnt++;
	if (*--iop->_ptr != c)
		*iop->_ptr = c;

#ifdef	_POSIX_SOURCE
	iop->_flag &= ~_IOEOF;
#endif

	return ((int)(unsigned char)c);
}
