#ifndef lint
static	char	*sccsid = "@(#)fread.c	4.3	(ULTRIX)	9/10/90";
#endif

/************************************************************************
 *									*
 *			Copyright (c) 1985 - 1989 by			*
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
 *			Modification History				*
 *									*
 * 006	Jon Reeves, 05-Dec-1989						*
 *	XVS pointed out that we aren't flushing before a read if the	*
 *	last operation was a write; we should (we do, now).		*
 *									*
 * 005  Linda Wilson, 10-Oct-1989					*
 *	For X/Open conformance in fclose, negate _cnt after returning   *
 *	from _filbuf, as is done for System V.  This is necessary for   *
 *	correct operation of files opened with "a+" mode.		*
 *									*
 * 004	Jon Reeves, 1989 June 8						*
 *	Change return type to ANSI-mandated size_t.			*
 *									*
 * 	David Metsky, 20-Nov-1986					*
 * 003  Make sure that after call to _filbuf, iop->_cnt contains a	* 
 *	positive integer.  This is necessary when working on an		*
 *	append in System V environment.					*
 *									*
 *	David L Ballenger, 29-May-1985					*
 * 002	Fix problems with System V emulation and add some performance	*
 *	enhancements.							*
 *									*
 *	David L Ballenger, 29-Mar-1985					*
 * 001  Put fread() and fwrite() into separate modules.			*
 *									*
 ************************************************************************/


#include	<stdio.h>

#ifndef SYSTEM_FIVE

/* In the BSD and POSIX environments the numeric arguments are unsigned
 * values and the number of bytes to read are the product of those
 * arguments (ie. size and count).
 */
#define NUMERIC_ARG size_t
#define BYTES_TO_READ(x,y) ((x) * (y))

#else	/* SYSTEM_FIVE */

/* In the System V environment the numeric arguments are ints.  If 
 * either is negative then the number of bytes to read is 0 otherwise
 * it is the product of the numeric arguments (size, count).
 */
#define NUMERIC_ARG int
#define BYTES_TO_READ(x,y) ( ((x) < 0 || (y) < 0) ? 0 : ((x) * (y)) )
#endif	/* SYSTEM_FIVE */

size_t
fread(ptr, size, count, iop)
	register 	char *ptr;
	NUMERIC_ARG	size, count;
	register FILE 	*iop;
{
	register unsigned s;
	int c;

	s = BYTES_TO_READ(size,count);

	if (s == 0) return(0);

	if ((iop->_flag & (_IORW | _IOWRT | _IOREAD)) == (_IORW | _IOWRT))
		if (fflush(iop) == EOF)
			return 0;

	while (s > 0) {
		if (iop->_cnt < s) {
			if (iop->_cnt > 0) {
				bcopy(iop->_ptr, ptr, iop->_cnt);
				ptr += iop->_cnt;
				s -= iop->_cnt;
			}
			/*
			 * filbuf clobbers _cnt & _ptr,
			 * so don't waste time setting them.
			 */
			if ((c = _filbuf(iop)) == EOF)
				break;
/* 
 * _filbuf uses negative numbers if in system V append mode.  Must
 * convert back to positive integer to work correctly.
 * 
 */
#if defined(SYSTEM_FIVE) || defined(_POSIX_SOURCE)	/* 005 */
			if (iop->_cnt < 0){ iop->_cnt *= (-1);}
#endif


			*ptr++ = c;
			s--;
		}
		if (iop->_cnt >= s) {
			bcopy(iop->_ptr, ptr, s);
			iop->_ptr += s;
			iop->_cnt -= s;
			return (count);
		}
	}
	return (count - ((s + size - 1) / size));
}
