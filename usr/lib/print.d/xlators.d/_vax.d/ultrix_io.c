#ifndef lint
static char sccsid[] = "@(#)ultrix_io.c	4.1	ULTRIX	7/2/90";
#endif

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
 *   The information in this software is subject to change  without	*
 *   notice  and should not be construed as a commitment by Digital	*
 *   Equipment Corporation.						*
 *									*
 *   Digital assumes no responsibility for the use  or  reliability	*
 *   of its software on equipment which is not supplied by Digital.	*
 *									*
 ************************************************************************/


/*
 * ultrix_io.c -- I/O functions for regis and tek xlators
 */

/* SCCS history beginning
 * ***************************************************************
 *                -- Revision History --
 * ***************************************************************
 * 
 * 1.1  31/05/88 -- thoms
 * date and time created 88/05/31 19:55:12 by thoms
 * 
 * ***************************************************************
 * 1.2  19/07/88 -- thoms
 * Added copyright notice and modification history
 *
 * SCCS history end
 */


#define	OBUFSIZE	8192
#define	IBUFSIZE	2048


/*
 * put_xlbuf -- write a buffer to file
 *
 * NOTE:
 *	BLISS calls this via a stub in call_stubs.s
 */
int
put_xlbuf (length, buf, user_arg_p)
short int	*length;
unsigned char	**buf;
int		user_arg_p;
{
	static unsigned char	putbuf [OBUFSIZE];
	int	status;

	if (*length) {
	    if ((status = write (user_arg_p, *buf, *length)) < 0) {
			perror("xlate.put_xlbuf");
			*length = 0;
		}
	}

	*buf = putbuf;
	*length = OBUFSIZE;

	return 1;		/* SS$_NORMAL */
}

/*
 * gett_xlbuf -- get a buffer full from file
 *
 * NOTE:
 *	BLISS calls this via a stub in call_stubs.s
 */
int
get_xlbuf (length, buf, user_arg_g)
short int	*length;
unsigned char	**buf;
int		user_arg_g;
{
	static unsigned char	ibuf [IBUFSIZE];
	int	status;

	if ((status = read (user_arg_g, ibuf, IBUFSIZE)) < 0) {
	    perror("xlate");
	    *length = 0;
	}
	else
	    *length = status;

	*buf = ibuf;

	return 1;		/* SS$_NORMAL */
}
