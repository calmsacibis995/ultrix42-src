
/*
 * 	@(#)ttyent.h	4.2	(ULTRIX)	10/16/90
 */

/************************************************************************
 *									*
 *			Copyright (c) 1984, 1987 by			*
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
/*	ttyent.h	4.2	85/01/30	*/

/* Modification history
 *
 * 4/22/87 - Tim Burke
 *	Added TTY_TERMIO to specify that a line is to operate in a termio-
 *	only environment.
 */

struct	ttyent { /* see getttyent(3) */
	char	*ty_name;	/* terminal device name */
	char	*ty_getty;	/* command to execute, usually getty */
	char	*ty_type;	/* terminal type for termcap (3X) */
	int	ty_status;	/* status flags (see below for defines) */
	char 	*ty_window;	/* command to start up window manager */
	char	*ty_comment;	/* usually the location of the terminal */
};

#define TTY_ON		0x1	/* enable logins (startup getty) */
#define TTY_SECURE	0x2	/* allow root to login */
#define TTY_LOCAL	0x4	/* local line that ignores modem signals */
#define TTY_SHARED	0x8	/* line used for outgoing and incoming connects */
#define TTY_TRACK	0x10	/* track this line for modem status change */
#define TTY_TERMIO	0x20	/* Open line with termio defaults & line disc */
#define TTY_SU		0x40	/* disallow su to root */

extern struct ttyent *getttyent();
extern struct ttyent *getttynam();
