/*	@(#)exit.c	4.2	ULTRIX	9/4/90	*/
/*	Based on:	*/
/*	exit.c	1.1	83/06/23	*/
/************************************************************************
 *									*
 *			Copyright (c) 1989 by				*
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
 *	Modification History
 *
 * 3	Jon Reeves, 1990 August 16
 *	Fixed atexitp checking to not go beyond end of assigned space.
 *	Not necessary to check contents of atexitlist; address is enough.
 *
 * 2	Peter Hack, 1989 November 9
 *	Made exit() uninterruptible during calls to atexit routines.
 *	This is for XPG3.  The changes should be removed for XPG4 compliance.
 *
 * 1	Jon Reeves, 1989 June 14
 *	Added ANSI-mandated atexit() handling.
 */

#define	ATEXITMAX	32
void	(*_atexitlist[ATEXITMAX]) ();
void	(**_atexitp)() = &_atexitlist[ATEXITMAX];

void
exit(code)
	int code;
{
	int condition, oldsig;
	static int oldsig_set = 0;
	void _cleanup(), _exit();

/* Uncomment the following two lines out for XPG4
 	while (_atexitp++ < &_atexitlist[ATEXITMAX])
		( *(_atexitp-1) )();
 * down to here
 */

/* Delete the following lines for XPG4 when XPG3 is no longer a requirement */
	condition = _atexitp < &_atexitlist[ATEXITMAX];
 	while (condition) {
		if (!oldsig_set)	/* in case exit() called from atexit routine */
			oldsig = sigsetmask( -1 );	/* uninterruptable */
		oldsig_set = 1;
		_atexitp++;
		( *(_atexitp-1) )();
		condition = _atexitp < &_atexitlist[ATEXITMAX];
		oldsig_set = 0;
		sigsetmask( oldsig );
	}
/* End of lines to be deleted for XPG4 */

	_cleanup();
	_exit(code);
}
