#ifndef lint
static	char	*sccsid = "@(#)getstdiobuf.c	4.1	(ULTRIX)	7/3/90";
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
 *	David L Ballenger, 29-Aug-1985
 * 002	Mark allocated buffers as _IOMYBUF
 *
 *	David L Ballenger, 15-Jul-1985
 * 001	Clean up buffer handling for unbuffered files.
 *
 ************************************************************************/

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>

extern char *malloc();

/* getstdiobuf()
 *
 * Routine called by _filbuf() and flsbuf() to allocate buffers.
 *
 */
void
_getstdiobuf(iop)
	register FILE	*iop;
{
	struct stat	stat;

	/* For line buffered or fully buffered files, attempt
	 * to find the optimal buffer size.
	 */
	if (fstat(fileno(iop),&stat) == -1)
		stat.st_blksize = BUFSIZ;
	else if (stat.st_blksize <= 0)
		stat.st_blksize = BUFSIZ;

	iop->_base = malloc(stat.st_blksize);
	if (iop->_base == NULL) {

		/* If the buffer allocataion fails then do unbuffered io.
		 */
		iop->_flag &= ~(_IOFBF|_IOLBF);
		iop->_flag |= _IONBF;
	} else {
		/* Set up the buffer information in the iob.
		 */
		iop->_bufsiz = stat.st_blksize;
		iop->_ptr = iop->_base ;
		iop->_flag |= _IOMYBUF ;
	}
}
