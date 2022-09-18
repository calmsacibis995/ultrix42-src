/*	@(#)atexit.c	4.1	ULTRIX	7/3/90	*/
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
 * 1	Jon Reeves, 1989 June 14
 *	Created ANSI-mandated atexit() function.
 *	See also exit.c.
 */

extern	void	(*_atexitlist[]) ();
extern	void	(**_atexitp)();

int
atexit(func)
	void	(*func)();
{
/*	Indicate error if input function pointer null or table full	*/
	if ( !func || (_atexitp == _atexitlist) ) return (-1);
	*--_atexitp = func;
	return (0);
}
