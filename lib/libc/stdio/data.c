#ifndef lint
static	char	*sccsid = "@(#)data.c	4.1	(ULTRIX)	7/3/90";
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
 *	David L Ballenger, 26-Jul-1985
 * 002	Move the _cleanup() routine for stdio here so that it will be 
 *	included whenever stdio is.  _fwalk() and the pointers for the
 *	dynamiclly allocated FILE structures are also located here, so
 *	that _findiop() won't be included in a program unless it is
 *	actually called.
 *
 *	David L Ballenger, 26-Jun-1985
 * 001	Only allocate enough FILE structures for stdin, stdout, and
 *	stderr.  The others will be allocated as needed by _findiop().
 *	Also move _sibuf and _sobuf to seperate files, so they aren't
 *	included if not needed.
 *
 *	Based on:  data.c	4.2 (Berkeley) 10/5/82
 *
 ************************************************************************/

#include <stdio.h>

FILE _iob[_N_STATIC_IOBS] ={
	{ 0, NULL, NULL, NULL, _IOREAD, 0},		/* stdin  */
	{ 0, NULL, NULL, NULL, _IOWRT, 1},		/* stdout */
	{ 0, NULL, NULL, NULL, _IOWRT|_IONBF, 2},	/* stderr */
};

/* Pointers to dynamically allocated array of pointers to FILE structures
 */
FILE	**_iob_start = (FILE **)0;
FILE	**_iob_end;


#define active(iop) ((iop)->_flag & (_IOREAD|_IOWRT|_IORW))

void
_fwalk(function)
	register int (*function)();
{
	if (_iob_start == NULL) {

		/* No FILE structures alloacted at runtime, so step
		 * through the static ones.
		 */
		register FILE *fp;
		for (fp = _iob; fp < &_iob[_N_STATIC_IOBS]; fp++)
			if (active(fp))
				(*function)(fp);
	} else {
		/* Step through all the FILE structures.
		 */
		register FILE **iov;
		for (iov = _iob_start; iov < _iob_end; iov++)
			if (*iov != NULL && active(*iov))
				(*function)(*iov);
	}
}

void
_cleanup()
{
	extern int fclose();
	_fwalk(fclose);
}
