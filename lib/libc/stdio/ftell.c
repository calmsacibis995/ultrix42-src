#ifndef lint
static	char	*sccsid = "@(#)ftell.c	4.1	(ULTRIX)	7/3/90";
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
 *      Dan Smith, 1-Mar-90
 * 004  Changed use of L_INCR to SEEK_CUR and removed #include of
 *      <sys/file.h>. This was done to allow ftell to be placed in the
 *      POSIX library. (L_INCR is not defined in POSIX mode).
 *	
 *	Jon Reeves, 10-Jan-1990
 * 003	Set EBADF when relevant.  POSIX.
 *
 *	David L Ballenger, 01-Aug-1985
 * 002	Fix more problems with System V style append mode.
 *
 *	David L Ballenger, 28-May-1985
 * 001	Fix problem with returning correct position if file was opened
 *	for append.
 *
 ************************************************************************/


/*
 * Return file offset.
 * Coordinates with buffering.
 */

#include	<stdio.h>
#include	<errno.h>

long	lseek();


long ftell(iop)
	register FILE *iop;
{
	long tres;
	register adjust;

	/* Assume no characters in the buffer.
	 */
	adjust = 0;

	if (iop->_flag & _IOREAD) 

		/* If a read has been done then we have to subtract the
		 * number of characters in the buffer from the current
		 * file position.  This means the ajustment is the _cnt
		 * in System V append mode, since it is stored as a negative
		 * number in that case.  Otherwise it is the negation of the
		 * _cnt.
		 */
		adjust = (iop->_flag & _IOAPPEND) ? iop->_cnt : (-iop->_cnt);
		
	else if (iop->_flag & (_IOWRT|_IORW)) {
		
		if (iop->_flag & _IOAPPEND)
			
			/* If in System V style append mode, we have to
			 * do a flush, because the file may have been
			 * repositioned and subsequent writes may have
			 * started writing to the buffer without
			 * actually writing to the file.  If the flush
			 * isn't done, the position returned would be
			 * wherever the file is positioned rather than where
			 * it would be if the when the buffered characters
			 * are written.  Note that if no characters are
			 * buffered the write/flush will not occur and the
			 * file position will not change.
			 */
			(void)fflush(iop);

		else if ( (iop->_flag & _IOWRT) && iop->_base)
			
			/* Otherwise if characters have been written to the
			 * buffer, then just add the number of characters
			 * to the current file position.  NOTE that _base
			 * is NULL if no reads or writes have been done, or
			 * if the file is unbuffered.
			 */
			adjust = iop->_ptr - iop->_base;
	} else {
		/* Illegal mode
		 */
		errno = EBADF;
		return(EOF);
	}

	tres = lseek(fileno(iop), 0L, SEEK_CUR);

	return( (tres<0) ? tres : tres + adjust );
}
