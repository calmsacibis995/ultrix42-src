#ifndef lint
static	char	*sccsid = "@(#)fprintf.c	4.1	(ULTRIX)	7/3/90";
#endif
/* ------------------------------------------------------------------ */
/* | Copyright Unpublished, MIPS Computer Systems, Inc.  All Rights | */
/* | Reserved.  This software contains proprietary and confidential | */
/* | information of MIPS and its suppliers.  Use, disclosure or     | */
/* | reproduction is prohibited without the prior express written   | */
/* | consent of MIPS.                                               | */
/* ------------------------------------------------------------------ */
/* $Header: fprintf.c,v 1.1 87/02/16 11:19:19 dce Exp $ */

/************************************************************************
 *									*
 *			Copyright (c) 1988 by				*
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
 * 002  Linda Wilson, 06-oct-1989
 *	Return number of characters transmitted for X/Open conformance.
 *	Use System V additions made in 001.
 *
 * 001	David L Ballenger, 08-Nov-1985
 *	Add temporary buffering for unbuffered files to reduce the
 *	write(2) system call overhead for unbuffered files.
 *	Also, add fix for return values for the System V environment.
 *
 *	Based on:  fprintf.c	4.1 (Berkeley) 12/21/80
 *
 ************************************************************************/

#include	<stdio.h>
#ifdef mips
#include	<varargs.h>
#endif /* mips */

#ifdef vax
fprintf(iop, fmt, args)
	register FILE *iop;
	char *fmt;
#endif /* vax */
#ifdef mips
fprintf(iop, fmt, va_alist)
register FILE *iop;
char *fmt;
va_dcl
#endif /* mips */
{
	int unbuffered;
	char temp_buf[BUFSIZ];
#if defined(SYSTEM_FIVE) || defined(_POSIX_SOURCE)	/* 002 */
	int n_chars;		/* characters transmitted by _doprnt() */
#endif /* defined(SYSTEM_FIVE) || defined(_POSIX_SOURCE) */
#ifdef mips
	va_list ap;
	
	va_start(ap);
#endif /* mips */
/* the following allows the source to look the same for arg processing */
#ifdef vax
#define ARGS	&args
#endif /* vax */
#ifdef mips
#define ARGS	ap
#endif /* mips */

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
	 * In the System V environment, keep track of the # of characters
	 * transmitted by _doprnt().
	 */
#if defined(SYSTEM_FIVE) || defined(_POSIX_SOURCE)	/* 002 */
	n_chars = _doprnt(fmt, ARGS, iop);
#else
	(void)_doprnt(fmt, ARGS, iop);
#endif /* defined(SYSTEM_FIVE) || defined(_POSIX_SOURCE) */

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

	/* In the System V environment return the number of characters
	 * transmitted on success, and in the ULTRIX environment return
	 * 0 on success
	 */
#if defined(SYSTEM_FIVE) || defined(_POSIX_SOURCE)	/* 002 */
	return(ferror(iop)? EOF: n_chars);
#else
	return(ferror(iop)? EOF: 0);
#endif /* defined(SYSTEM_FIVE) || defined(_POSIX_SOURCE) */
}
