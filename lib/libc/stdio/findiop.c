#ifndef lint
static char *sccsid = "@(#)findiop.c	4.1	ULTRIX	7/3/90";
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

* * 	Lu Anne Van de Pas, 22-may-1986
 * 002  Returned proper error number in errno 
 *
 *	David L Ballenger, 26-Jul-1985
 * 003	Move _cleanup(), _fwalk(), and the pointers to dynamically 
 *	allocated FILE file structures in to the data.c module.  These
 *	are always needed when stdio is used.  This allows _findiop()
 *	to be included only when actually called.
 *
 *	David L Ballenger, 15-Jul-1985
 * 002	Add _fwalk() routine so that other components (especially franz
 *	lisp don't have to know as much about how FILE structures are 
 *	allocated.
 *
 *	David L Ballenger, 26-Jun-1985
 * 001	Reorganize _findiop() to handle dynamic allocation of FILE
 *	structures.  This routine is called by fopen() and fdopen() to
 *	get a pointer to the next free FILE structure.
 *
 ************************************************************************/

#include <errno.h>
#include <stdio.h>

extern char	*calloc();
extern int	getdtablesize();

/* Pointers to dynamically allocated array of pointers to FILE structures
 */
extern FILE	**_iob_start;
extern FILE	**_iob_end;

FILE *
_findiop()
{
	register FILE	**iov;

	/* If the vector of pointers to file structures is not allocated,
	 * attempt to allocate it.
	 */
	if (_iob_start == NULL) {
		register int	n_iobs = getdtablesize();
		register FILE	*fp;

		_iob_start = (FILE **)calloc(n_iobs,sizeof(FILE *));
		if (_iob_start == NULL) {
			/* 004 - set errno: no memory */
			errno = ENOMEM; 
			return(NULL);
		}

		_iob_end = _iob_start + n_iobs ;

		/* Have the first pointers point to the statically
		 * allocated file structures.
		 */
		iov = _iob_start ;
		fp = _iob;
		while (fp < _iob + _N_STATIC_IOBS)
			*iov++ = fp++;
	}

	/* Search for a free pointer, either NULL pointer or pointer
	 * to an unused FILE structure.
	 */
	iov = _iob_start ;
	while (*iov != NULL && (*iov)->_flag & (_IOREAD|_IOWRT|_IORW))
		if (++iov >= _iob_end) {
			/* 004 set errno: too many files opened */
			errno = EMFILE;
			return(NULL);
		}

	/* If the pointer is NULL attempt to allocate a FILE structure.
	 */
	if (*iov == NULL)
		*iov = (FILE *)calloc(1,sizeof(FILE));

	/* 004 if calloc returns Null, set errno to no memory 
	 */
	 if (*iov == NULL) {
	 	errno = ENOMEM;
		return (NULL);
	}


	return(*iov);
}
