#ifndef lint
static	char	*sccsid = "@(#)setbuf.c	4.1	(ULTRIX)	7/3/90";
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
 *	David L Ballenger, 25-Jun-1985					*
 * 002	Call setvbuf to setup the buffering information.  This allows	*
 *	buffer handling and setup to be localized.			*
 *									*
 *	David L Ballenger, 13-Mar-1985					*
 * 0001	Change declaration of "setbuf()" to "void setbuf()", since it	*
 *	does not return a value.  This also brings it in line with the	*
 *	System V definition.						*
 *									*
 ************************************************************************/

/* @(#)setbuf.c	4.2 (Berkeley) 10/5/82 */
#include	<stdio.h>

void
setbuf(iop, buf)						/*0001*/
	FILE *iop;
	char *buf;
{
	setvbuf(iop,buf,(buf == NULL) ? _IONBF : _IOFBF ,BUFSIZ);
}
