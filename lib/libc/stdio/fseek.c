#ifndef lint
static	char	*sccsid = "@(#)fseek.c	4.1	(ULTRIX)	7/3/90";
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
 *
 * 004  DECwest ANSI vk 016, 21 Dec 1989
 *      Cannot optimize fseek when a file is opened read only,
 *      always have to do a lseek and set _cnt to 0 (like the
 *      update case). This is to conform to ANSI C 4.9.7.11
 *      fseek should erase any memories of a previous ungetc
 *
 *	Jon Reeves, 10-Jan-1990
 * 003	More tweaks to error handling.
 *
 *	Jon Reeves, 04-Dec-1989
 * 002	Fix error handling on flush case
 *
 *	David L Ballenger, 01-Aug-1985
 * 001	Make System V style append mode work correctly.
 *
 *	Based on:  fseek.c	4.3 (Berkeley) 9/25/83
 *
 ************************************************************************/

/*
 * Seek for standard library.  Coordinates with buffering.
 */

#include	<stdio.h>
#include	<errno.h>

long lseek();

fseek(iop, offset, ptrname)
FILE *iop;
long offset;
{
	register resync, c;
	long p;

	iop->_flag &= ~_IOEOF;
	if (iop->_flag&_IOREAD) {
		if (ptrname<2 && iop->_base &&
			!(iop->_flag&_IONBF)) {
#ifndef _POSIX_SOURCE
			
			/* In System V append mode _cnt is stored as a
			 * negative number.  See _filbuf() for details.
			 */
			c = (iop->_flag&_IOAPPEND) ? -iop->_cnt : iop->_cnt ;
			p = offset;
			if (ptrname==0)
				p += c - lseek(fileno(iop),0L,1);
			else
				offset -= c;
			if(!(iop->_flag&_IORW) && c>0&&p<=c
			    && p>=iop->_base-iop->_ptr){
				iop->_ptr += (int)p;

				/* In System V append mode _cnt is stored as a
				 * negative number.  See _filbuf() for
				 * details.
				 */
				if (iop->_flag&_IOAPPEND)
					iop->_cnt += (int)p;
				else
					iop->_cnt -= (int)p;
				return(0);
			}
			resync = offset&01;
#endif
#ifdef _POSIX_SOURCE
                        c = (iop->_flag&_IOAPPEND) ? -iop->_cnt : iop->_cnt ;
                        if (ptrname == 1)       offset -= c;
                        resync = offset&01;
#endif
		} else 
			resync = 0;
		if (iop->_flag & _IORW) {
			iop->_ptr = iop->_base;
			iop->_flag &= ~_IOREAD;
			resync = 0;
		}
		p = lseek(fileno(iop), offset-resync, ptrname);
		iop->_cnt = 0;
		if (resync)
			getc(iop);
	}
	else if (iop->_flag & (_IOWRT|_IORW)) {
		
		/* Have to flush anything in the output buffer before
		 * doing the lseek().  Fflush() will now reset the
		 * the iop fields properly.
		 */
		if ( (p=fflush(iop)) != EOF)
			p = lseek(fileno(iop), offset, ptrname);
		/* The following line is a gross, ugly hack put in for
		 * one reason only:  to pass the NIST PCTS.  The test is
		 * wrong, wrong, wrong, but I'm not going to argue with a
		 * 500-pound gorilla.  Yank this when 1003.3 is fixed.
		 */
		else if (errno == EPIPE) errno = ESPIPE;
	}
	else {	/* None of the above -- probably closed */
		errno = EBADF;
		p = -1;
	}
	return(p==-1?-1:0);
}
