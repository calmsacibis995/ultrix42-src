#ifndef lint
static char *sccsid = "@(#)def_shell.c	4.1	(ULTRIX)	7/2/90";
#endif lint


/************************************************************************
 *									*
 *			Copyright (c) 1988, 1990 by			*
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
 
/*
 * Modification History
 *
 * 02/07/90 GWS	replaced sgttyb / tty(4)-style terminal I/O with termio(4)-
 *		 style terminal I/O.  The replacement code is the formerly
 *		 conditionalized "USG" code.
 *
 */

#include "curses.h"
#include "term.h"

extern	struct term *cur_term;


/*
 * A baud rate of 0 hangs up the phone.
 * Since things are often initialized to 0, getting the phone
 * hung up on you is a common result of a bug in your program.
 * This is not very friendly, so if the baud rate is 0, we
 * assume we're doing a reset_xx_mode with no def_xx_mode, and
 * just don't do anything.
 */
#define BR(x) (cur_term->x.c_cflag&CBAUD)

def_shell_mode()
{
	ioctl(cur_term -> Filedes, TCGETA, &(cur_term->Ottyb));
	/* This is a useful default for Nttyb, too */
	if (BR(Nttyb) == 0)
		cur_term -> Nttyb = cur_term -> Ottyb;
}
