#ifndef lint
static	char	*sccsid = "@(#)fwrite.c	4.3	(ULTRIX)	9/10/90";
#endif

/************************************************************************
 *									*
 *			Copyright (c) 1984 - 1989 by			*
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
 * 007	Jon Reeves, 10-Jan-1990						*
 *	Set errno on EBADF leg.						*
 *									*
 * 006	Jon Reeves, 05-Dec-1989						*
 *	XVS pointed out that we weren't flushing a buffer when it got	*
 *	exactly full; this is needed for multiple-handle file access.	*
 *									*
 * 005  Linda Wilson, 06-oct-1989					*
 *	For unbuffered files, return the number of items written in     *
 *	the case of write errors.  Remove performance enhancement (003) *
 *	since buffering obscures the number of items written.		*
 *	Implemented for standards conformance.				*
 *	Set _IOWRT flag so that buffer will be flushed when file is	*
 *	closed.								*
 *									*
 * 004	Jon Reeves, 1989 June 08					*
 *	Change return type to ANSI-mandated size_t.			*
 *									*
 * 003	David L Ballenger, 08-Nov-1985					*
 *	Use a temporary buffer for unbuffered files to cut down on the	*
 *	write(2) system call overhead.					*
 *									*
 *	David L Ballenger, 29-May-1985					*
 * 002	Fix problems with System V emulation and add some performance	*
 *	enhancements.							*
 *									*
 *	David L Ballenger, 29-Mar-1985					*
 * 0001 Put fread() and fwrite() into separate modules.			*
 *									*
 ************************************************************************/

#include	<stdio.h>
#include	<errno.h>

#ifndef SYSTEM_FIVE

/* In the BSD and POSIX environments the numeric arguments are unsigned
 * values and the number of bytes to write are the product of those
 * arguments (ie. size and count).
 */
#define NUMERIC_ARG size_t
#define BYTES_TO_WRITE(x,y) ((x) * (y))

#else	/* SYSTEM_FIVE */

/* In the System V environment the numeric arguments are ints.  If 
 * either is negative then the number of bytes to write is 0 otherwise
 * it is the product of the numeric arguments (size, count).
 */
#define NUMERIC_ARG int
#define BYTES_TO_WRITE(x,y) ( ((x) < 0 || (y) < 0) ? 0 : ((x) * (y)) )
#endif	/* SYSTEM_FIVE */

size_t
fwrite(ptr, size, count, iop)
	register char	*ptr;
	NUMERIC_ARG	size, count;
	register FILE	*iop;
{
	register int s;

	s = BYTES_TO_WRITE(size,count);

	if (s == 0) return(0);
	if(!(iop->_flag & (_IORW | _IOWRT))) {	/* 005 */
		iop->_flag |= _IOERR;
		errno = EBADF;
		return(0);
	}
	iop->_flag |= _IOWRT;		 	/* 005 */
	iop->_flag &= ~(_IOEOF|_IOREAD); 	/* 005 */

	if (iop->_flag & _IOLBF)
		while (s > 0) {
			if (--iop->_cnt > -iop->_bufsiz && *ptr != '\n')
				*iop->_ptr++ = *ptr++;
			else if (_flsbuf(*(unsigned char *)ptr++, iop) == EOF)
				break;
			s--;
		}
	else {
		int nitems;

		if(iop->_flag & _IONBF) {	 /* 005 */
			for(nitems=0; nitems<count; nitems++) {
				if(write(fileno(iop),ptr,size) < 0) {
					iop->_flag |= _IOERR;
					break;
				}
				ptr += size;
			}
			return(nitems);
		}
		else {
			while (s > 0) {
				if (iop->_cnt < s) {
					if (iop->_cnt > 0) {
						bcopy(ptr, iop->_ptr, iop->_cnt);
						ptr += iop->_cnt;
						iop->_ptr += iop->_cnt;
						s -= iop->_cnt;
					}
					if (_flsbuf(*(unsigned char *)ptr++, iop) == EOF)
						break;
					s--;
				}
				if (iop->_cnt >= s) {
					bcopy(ptr, iop->_ptr, s);
					iop->_ptr += s;
					iop->_cnt -= s;
					s = 0;
					if (iop->_cnt == 0)
						if (fflush(iop) == EOF)
							return 0;
				}
			}
		}
	}
	
	if (s == 0)
		return(count) ;
	else
		return (count - ((s + size - 1) / size));
}
