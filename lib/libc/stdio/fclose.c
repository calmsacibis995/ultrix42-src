#ifndef lint
static	char	*sccsid = "@(#)fclose.c	4.1	(ULTRIX)	7/3/90";
#endif

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
 * 010	Jon Reeves, 12-Jan-1990
 *	Move much of the code from 006 from fclose() to fflush(), since
 *	that's where it really belongs for POSIX; fclose() calls
 *	fflush(), anyhow.  Side effect: flushing a read-only stream
 *	is now a legal action.
 *	Also moved obviously misplaced line from 008.
 *
 * 009  David J. Gray, 15-Nov-1989
 *      Set iop->_file to -1 (file descriptor) instead of 0 when 
 *      the file is closed.
 *
 * 008  David J. Gray, 01-Nov-1989
 *      Check to see if the file pointer is null, return eof if it is.
 *
 * 007	Jon Reeves, 31-oct-89
 *	As specified in ANSI, make fflush(NULL) flush all output streams.
 *
 * 006  Linda Wilson, 06-oct-1989
 *	For X/Open conformance, position fully buffered files to 
 *	character following last character read or written by the
 *	application.
 *
 * 005	David L Ballenger, 05-Sep-1985
 *	Have fflush() reset all information in the iob to look as if
 *	there is nothing to write, before doing the write().  This
 *	prevents a signal handler from getting into an infinite loop
 *	if it attempts to fclose() / fflush() the file and the write 
 *	to the file causes the signal which the handler is catching.
 *
 *	David L Ballenger, 08-Aug-1985
 * 004	Have fclose() make sure the file is open for output before
 *	attempting to flush it.
 *
 *	David L Ballenger, 01-Aug-1985
 * 003	Move fflush() and fclose() into a separate file so that _flsbuf()
 *	is not included unneccesarily.  Also add code to fflush() to make
 *	System V style append mode work correctly.
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
#include	<sys/types.h>
#include	<sys/stat.h>

#define activeout(iop) ((iop)->_flag & _IOWRT)

fflush(iop)
	register FILE *iop;
{
	register char 	*base ;
	register int	n_chars ;
#ifdef _POSIX_SOURCE
	off_t seekpos;
	struct stat statbuf;
#endif /* _POSIX_SOURCE */

	/* Special case mandated by ANSI: flush all output streams.
	 * Can't use _fwalk, as that would flush input streams too.  Sigh.
	 */
	if (iop == NULL) {
		extern FILE	**_iob_start;
		extern FILE	**_iob_end;

		if (_iob_start == NULL) {
			for (iop = _iob; iop < &_iob[_N_STATIC_IOBS]; iop++)
				if (activeout(iop))
					if (fflush(iop) == EOF)
						return(EOF);
		} else {
			register FILE **iov;

			for (iov = _iob_start; iov < _iob_end; iov++)
				if (*iov != NULL && activeout(*iov))
					if (fflush(*iov) == EOF)
						return(EOF);
		}
		return(0);
	}

#if	!defined(_POSIX_SOURCE)
	/* Make sure this isn't readonly
	 */
	if ((iop->_flag & (_IOREAD|_IORW)) == _IOREAD) {
		iop->_flag |= _IOERR;
		return(EOF);
	}
#endif	/* !defined(_POSIX_SOURCE) */

	/* See if there is anything to flush.
	 */
	if ( iop->_flag & _IOWRT && (base=iop->_base) != NULL )
		n_chars = iop->_ptr - base ;
	else 
		n_chars = 0 ;

#ifdef _POSIX_SOURCE
	/* If file is buffered, not write-only, not at eof, and buffer 
	 * isn't empty, calculate seek position for underlying fd.
	 * Seek position is stored as a negative value since the
	 * seek will be backwards from the current location.
	 * If _cnt is < 0, use it unchanged.
	 */
	if(!n_chars &&
	   !(iop->_flag & _IONBF) &&
	   (iop->_flag & (_IOWRT|_IORW)) !=_IOWRT &&
	   !feof(iop) &&
	   (iop->_ptr != iop->_base)) {
		if(iop->_cnt == 0)
			seekpos = -(iop->_bufsiz -(iop->_ptr - iop->_base));
		else {
			if(iop->_cnt > 0)
				seekpos = -(iop->_cnt);
			else
				seekpos = iop->_cnt;
		}
	}
	else
		seekpos = 0;
#endif /* _POSIX_SOURCE */
	/* Reset the FILE info to indicate that the buffers
	 * have been flushed.  For {line,un}buffered, update, or
	 * System V append mode files _cnt is set to 0 so that
	 * the next putc() / getc() will call _flsbuf() / _filbuf().
	 * In update mode the write flag is turned off to indicate
	 * that there are no characters in the file to be written.
	 */
	iop->_ptr = iop->_base;
	iop->_cnt = (iop->_flag & (_IOLBF|_IONBF|_IORW|_IOAPPEND)) 
	          ? 0 : iop->_bufsiz ;
	if (iop->_flag & _IORW) 
		iop->_flag &= ~_IOWRT ;

	/* Now flush the buffer if there is actually anything to write.
	 * This is done after all the flags are reset, so that an
	 * interrupted write won't cause an infinite loop, if the signal
	 * handler tries to flush/close this file again.  This can happen
	 * when a pclose() is done as a result of a SIGPIPE signal, without
	 * the calling program setting the action for SIGPIPE to SIG_IGN.
	 */
	if (n_chars > 0) {
		if (write(fileno(iop),base,n_chars) != n_chars){
			iop->_flag |= _IOERR;
				return(EOF) ;
		}
	}
#ifdef _POSIX_SOURCE
	/* If seek needs to be done, check that file can seek.
	 * Seek to location following last char read/written.
	 * Lseek errors are avoided and, ultimately, ignored.
	 */
	else if (seekpos) {
		if (!fstat(fileno(iop),&statbuf)) {
			if (!(statbuf.st_mode & S_IFIFO) && 
			    statbuf.st_nlink && !isatty(fileno(iop)))
				lseek(fileno(iop),seekpos,SEEK_CUR);
		}
	}
#endif /* _POSIX_SOURCE */
	
	return(0);
}

fclose(iop)
	register FILE *iop;
{
	register int r;
	r = EOF;
	if (iop==NULL) return(r);  /* 008 */
	if (iop->_flag&(_IOREAD|_IOWRT|_IORW) && (iop->_flag&_IOSTRG)==0) {

		/* Flush the file if it is open for output, then close it
		 * and free any buffers allocated by stdio.
		 */
#if	!defined(_POSIX_SOURCE)
		if (iop->_flag & (_IOWRT|_IORW))
#endif	/* !defined(_POSIX_SOURCE) */
			r = fflush(iop);
#if	!defined(_POSIX_SOURCE)
		else
			r = 0;
#endif	/* !defined(_POSIX_SOURCE) */
		if (close(fileno(iop)) < 0)
			r = EOF;
		if (iop->_flag&_IOMYBUF)
			free(iop->_base);
	}

	iop->_cnt = 0;
	iop->_base = (char *)NULL;
	iop->_ptr = (char *)NULL;
	iop->_bufsiz = 0;
	iop->_flag = 0;
	iop->_file = -1;  /* 009 */
	return(r);
}
