#ifndef lint
static	char	*sccsid = "@(#)vfprintf.c	4.1	(ULTRIX)	7/3/90";
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
 * 001	David L Ballenger, 08-Nov-1985
 *	Add temporary buffering for unbuffered files to reduce the
 *	write(2) system call overhead for unbuffered files.
 *
 *
 ************************************************************************/

/*LINTLIBRARY*/
#include <stdio.h>
#include <varargs.h>

extern int _doprnt();

/*VARARGS2*/
int
vfprintf(iop, format, ap)
FILE *iop;
char *format;
va_list ap;
{
	register int count;
	int unbuffered;
	char temp_buf[BUFSIZ];

	/* If the file is unbuffered, then use the temporary buffer, and
	 * make the file look like it is buffered.  This prevents a
	 * write() system call from being done for every character.
	 */
	unbuffered = iop->_flag & _IONBF;
	if (unbuffered) {
		iop->_flag &= ~_IONBF;
		iop->_ptr = iop->_base = temp_buf;
		iop->_bufsiz = BUFSIZ;
	}

	/* Call _doprnt() to do the real work of formating and printing.
	 */
	count = _doprnt(format, ap, iop);

	/* If the file is unbuffered, then flush it to make sure that
	 * anything in the temporary buffer is written, then make it
	 * into an unbuffered file again.
	 */
	if (unbuffered) {
		(void)fflush(iop);
		iop->_flag |= _IONBF;
		iop->_base = NULL;
		iop->_bufsiz = 0;
		iop->_cnt = 0;
	}

	return(ferror(iop)? EOF: count);
}
