#ifndef lint
static char *sccsid = "@(#)filbuf.c	4.3	ULTRIX	9/10/90";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1985 by				*
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
 *			Modification History
 *
 * 006  DECwest ANSI vk 017, 21 Dec 1989
 *      Do not turn off the _IOREAD flag when a read beyond EOF is
 *      attempted. This is so that ungetc followed by ftell will
 *      conform to ANSI C 4.9.7.11.
 * 005	David L Ballenger, 02-Jun-1986
 *	Do not return with an error if _IOEOF is set when _filbuf() is 
 *	called.  This restores previous behavior.  Also, set errno to the
 *	appropriate value.
 *
 * 004	David L Ballenger, 27-Feb-1986
 *	Reset _cnt and _ptr after reading EOF to show that the buffer is
 *	empty.  This fixes problem with a 'write' after 'reading' EOF on
 *	files open in update (+) mode.
 *
 *	David L Ballenger, 01-Aug-1985
 * 003	Handle files opened with System V style append mode.  This behavior
 *	is provided in the ULTRIX environment by using "A" or "A+" to open
 *	the file.
 *
 *	David L Ballenger, 15-Jul-1985
 * 002	Clean up buffer handling for unbuffered files.
 *
 *	David L Ballenger, 26-Jun-1985
 * 001	Clean up handling of buffers and fix problems with files opened
 *	for append.
 *
 *	Based on:  filbuf.c	4.6 (Berkeley) 6/30/83
 *
 ************************************************************************/

#include	<stdio.h>
#include	<errno.h>

int
_filbuf(iop)
	register FILE	*iop;
{
	register char	*base;
	register int	chars_to_read;
	register int	sysV_append_mode = (iop->_flag & _IOAPPEND);
	char		smallbuf;

	if (iop->_flag & _IORW) {

		/* If the file has been written, flush it so that
		 * System V style append mode will work correctly.
		 */
		if (sysV_append_mode && (iop->_flag & _IOWRT))
			fflush(iop);

		/* Indicate that we have done a read.
		 */
		iop->_flag &= ~_IOWRT;
		iop->_flag |= _IOREAD;
	}

	/* Make sure that this is readable
	 */
	if ((iop->_flag&(_IOSTRG|_IOWRT|_IOREAD)) != _IOREAD) {
		iop->_flag |= _IOERR ;
		errno = EBADF ;
		return(EOF);
	}

	if (sysV_append_mode) {
		/* If the file is opened for APPEND, the number of characters
		 * in the buffer is stored as negative number in the _cnt
		 * field.  This forces the calls to getc() to call _filbuf()
		 * everytime.  Since getc() decrements the _cnt field, we
		 * have to increment by two to see if there are still 
		 * characters in the buffer.  If _cnt is then <= 0 we can
		 * simply return the next character from the buffer and the
		 * absolute value of _cnt is the number of characters left.
		 */
		iop->_cnt += 2;
		if (iop->_cnt <= 0)
			return((int)((unsigned char)*iop->_ptr++));
	}

	/* Flush stdout and stderr if necessary.
	 */
	if (iop == stdin) {
		if (stdout->_flag&_IOLBF)
			fflush(stdout);
		if (stderr->_flag&_IOLBF)
			fflush(stderr);
	}
	
	/* Get a buffer if we don't have one.  Note that unbuffered
	 * files use a temporary buffer allocated on the stack in 
	 * this routine.
	 */
	for (;;) {
		base = iop->_base;
		if (base != NULL) {
			chars_to_read = iop->_bufsiz;
			break;
		}
		if (iop->_flag & _IONBF) {
			base = &smallbuf;
			chars_to_read = 1;
			break;
		}

		/* Try to get a buffer for fully or line buffered files. If
		 * this routine doesn't succeed, it sets the file to 
		 * unbuffered.
		 */
		_getstdiobuf(iop);
	}
	
	/* Reset the pointer to the base of the buffer.
	 */
	iop->_ptr = base ;

	/* Now finally do the read.
	 */
	iop->_cnt = read(fileno(iop), base, chars_to_read);
	if (--iop->_cnt < 0) {
		if (iop->_cnt == -1) {
			iop->_flag |= _IOEOF;
#ifndef _POSIX_SOURCE
			if (iop->_flag & _IORW)
				iop->_flag &= ~_IOREAD;
#endif
		} else
			iop->_flag |= _IOERR;
		/*
		 * Indicate that the buffer is empty, so that a write after
		 * reading EOF in update mode will work correctly.
		 */
		iop->_cnt = 0;
		return(EOF);

	} else if (sysV_append_mode) 

		/* The number of characters in the buffer is stored as a
		 * negative number.  This forces getc() to call _filbuf()
		 * everytime, and just as importantly will cause putc()
		 * to call _flsbuf().  This is necesary to make append mode
		 * work.
		 */
		iop->_cnt = -iop->_cnt;

	return((int)((unsigned char)*iop->_ptr++));
}
