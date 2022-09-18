/*
 * static	char	*sccsid = "@(#)ka60_data.c	4.1	(ULTRIX)	7/2/90";
 */

/************************************************************************
 *									*
 *			Copyright (c) 1989 by				*
 *		Digital Equipment Corporation, Maynard, MA		*
 *			All rights reserved.				*
 *									*
 *   This software is furnished under a license and may be used and	*
 *   copied  only  in accordance with the terms of such license and	*
 *   with the  inclusion  of  the  above  copyright  notice.   This	*
 *   software  or  any	other copies thereof may not be provided or	*
 *   otherwise made available to any other person.  No title to and	*
 *   ownership of the software is hereby transferred.			*
 *									*
 *   This software is  derived	from  software	received  from	the	*
 *   University    of	California,   Berkeley,   and	from   Bell	*
 *   Laboratories.  Use, duplication, or disclosure is	subject  to	*
 *   restrictions  under  license  agreements  with  University  of	*
 *   California and with AT&T.						*
 *									*
 *   The information in this software is subject to change  without	*
 *   notice  and should not be construed as a commitment by Digital	*
 *   Equipment Corporation.						*
 *									*
 *   Digital assumes no responsibility for the use  or	reliability	*
 *   of its software on equipment which is not supplied by Digital.	*
 *									*
 ************************************************************************/

/*
 * ka60_data.c
 *
 * Modification history:
 *
 * 09-05-89	Darrell A. Dunnuck
 *	Created this File.
 *
 */

#ifdef BINARY

int
ka60custconfig(mbp) { /* Stub routine */ }

#else BINARY

#ifdef vax
int
ka60custconfig(mbp)
register struct mb_node *mbp;
{

	/*
	 * Put a call to user supplied driver's probe routine here
	 */

	/*
	 * If the probe of the user supplied device was successful
	 * this routine must return a zero.
	 */
	return(0);
}
#endif vax

#endif BINARY

