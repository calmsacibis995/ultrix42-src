#ifndef lint
static	char	*sccsid = "@(#)setvbuf.c	4.1	(ULTRIX)	7/3/90";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1984 by				*
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
 *			Modification History				*
 *									*
 *	David L Ballenger, 15-Jul-1985					*
 * 003	Clean up buffer handling for unbuffered files.			*
 *									*
 *	David L Ballenger, 26-Jun-1985					*
 * 002	Clean up some problems with handling _IONBF, and also dynamicly	*
 *	allocated buffers.						*
 *									*
 *	David L Ballenger, 28-Mar-1985					*
 * 0001	Add setvbuf() from BRL System V emulation package.  Fix a	*
 *	couple of bugs to make it behave as documented.			*
 *									*
 *	Based on:  setvbuf.c (System V)	1.2				*
 *									*
 ************************************************************************/


/*LINTLIBRARY*/
#include <stdio.h>

extern void free();
extern char *malloc();

int
setvbuf(iop, buf, type, size)
	register FILE	*iop;
	char		*buf;
	int		type,
			size;
{
	if(iop->_base != NULL && iop->_flag & _IOMYBUF)
		free((char*)iop->_base);
	iop->_flag &= ~(_IOMYBUF | _IONBF | _IOLBF);
	switch (type)  {

	    /* Note that the flags are the same as the possible values for
	     * type.
	     */

	    case _IONBF:
		iop->_base = NULL;
		iop->_bufsiz = 0;
		break;

	    case _IOLBF:
	    case _IOFBF:
	    	
		/* Fully buffered files and line buffered files are handled
		 * the same way.  Check the size, then allocate the buffer
		 * if the user didn't specify one.  If we allocate the buffer
		 * mark it as ours.
		 */
	    	if ((iop->_bufsiz = size) == 0)	/* illegal size */
			return(-1);
		if ((iop->_base = buf) == NULL) {
			iop->_base = malloc(size);
			if (iop->_base == NULL)
				return(-1);
			iop->_flag |= _IOMYBUF ;
		}
		break;
	    default:			/* illegal type */
		return(-1);
	}

	/* Set up the rest of the buffer information in the FILE structure
	 */
	iop->_flag &= ~(_IONBF|_IOLBF|_IOFBF);
	iop->_flag |= type;
	iop->_ptr = iop->_base;
	iop->_cnt = 0;
	return 0;
}
