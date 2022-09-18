#ifndef lint
static	char	*sccsid = "@(#)flsbuf.c	4.3	(ULTRIX)	9/10/90";
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
 *	Jon Reeves, 1990-Jan-03
 * 004	Set errno on EBADF leg for POSIX conformance.
 *
 *	David L Ballenger, 01-Aug-1985
 * 003	Move fflush() and fclose() to a separate file.  Also, put in more
 *	code to make files opened for System V style append work correctly.
 *
 *	David L Ballenger, 15-Jul-1985
 * 002	Clean up buffer handling for unbuffered files.
 *
 *	David L Ballenger, 26-Jun-1985
 * 001	Clean up buffer allocation.  Also fix problems with files opened
 *	in append mode.
 *
 *	Based on:  flsbuf.c	4.6 (Berkeley) 6/30/83
 *
 ************************************************************************/

#include	<stdio.h>
#include	<errno.h>

_flsbuf(c, iop)
	register FILE	*iop;
{
	register char	*base;
	register int	size, n_written;
	char		smallbuf;

	if (iop->_flag & _IORW) {

		/* If the previous operation was a read and we are
		 * in System V append mode, then reset the buffer pointer.
		 * Otherwise we might write garbage to the file.
		 * This allows the append to work after a read without
		 * an intervening fseek().
		 */
		if ((iop->_flag & _IOAPPEND) && (iop->_flag & _IOREAD))
			iop->_ptr = iop->_base;

		/* Indicate that the last operation was a "write"
		 */
		iop->_flag |= _IOWRT;
		iop->_flag &= ~(_IOEOF|_IOREAD);
	}

	/* If not in "write" mode then we have an error.
	 */
	if ((iop->_flag&(_IOSTRG|_IOWRT|_IOREAD)) != _IOWRT) {
		iop->_flag |= _IOERR ;
		errno = EBADF;
		return(EOF);
	}
	
	/* Find the base of the file buffer.
	 */
	for (;;) {
		base = iop->_base;
		if (base != NULL)	/* If we have a buffer, great! */
			break;

		/* Note that unbuffered files are given a temporary
		 * buffer for convience sake.  But this does not
		 * change iop->_base which always remain NULL.
		 */
		if (iop->_flag & _IONBF) {
			base = iop->_ptr = &smallbuf;
			break ;
		}
		/* Don't have a buffer yet, so allocate one. If
		 * stdout is going to a tty then it will be line
		 * buffered.
		 */
		if ( iop == stdout && isatty(fileno(stdout)) ) {
			iop->_flag &= ~(_IONBF|_IOFBF);
			iop->_flag |= _IOLBF ;
		}
		/* Attempt to get a buffer.  If no buffer can be allocated,
		 * then this routine will change the file to unbuffered.
		 */
		_getstdiobuf(iop);
	}

	n_written = 0;

	if (iop->_flag & (_IONBF|_IOLBF|_IOAPPEND)) {

		/* Handle UNBUFFERED and LINE BUFFERED files as well as
		 * files opened in System V APPEND mode.  The character is
		 * first put into the buffer, then if the buffer is full 
		 * (always true for unbuffered files) or if the character is a
		 * newline and the file is line buffered, the buffer is
		 * written.  The _cnt field is ALWAYS reset to 0 to force
		 * the next putc() to call this routine again.  Note that
		 * this will also force the next getc() to call _filbuf,
		 * which is necessary fo files opened for System V append.
		 */
		*iop->_ptr++ = c;
		size = iop->_ptr - base ;
		if ((size >= iop->_bufsiz)
		    || ((c == '\n') && (iop->_flag & _IOLBF))
		   ) {
			n_written = write(fileno(iop),base,size);
			iop->_ptr = base ;
		} else
			size = 0;
		iop->_cnt = 0 ;

	} else {
		/* For fully buffered files, write anything in the
		 * buffer, then put the character in the empty buffer
		 * Note that, unlike unbuffered or line buffered files
		 * the character is not put in the buffer until after
		 * the buffer has been written.
		 */
		size = iop->_ptr - base ;
		if (size > 0) 
			n_written = write(fileno(iop),base,size);
		
		/* Allow putc() to put this many characters in buffer
		 * before calling _flsbuf();
		 */
		iop->_cnt = iop->_bufsiz - 1;
					 
		*base++ = c ;
		iop->_ptr = base ;
	}

	if (size != n_written) {
		iop->_flag |= _IOERR;
		return(EOF);
	}
	return(c);
}
