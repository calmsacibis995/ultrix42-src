#ifndef lint
static char *sccsid = "@(#)_c_clean.c	4.1	(ULTRIX)	7/2/90";
#endif lint


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
 * Modification History
 *
 * 12//89  GWS	call _window with all -1's so Escape sequence is generated
 *		 to reset window scrolling region to maximum window size
 *		 known to terminal, or terminal emulator, instead of
 *		 restoring scrolling region to current no. of lines/columns
 */


#include "curses.ext"

extern	int	_outch();

_c_clean ()
{
#ifdef DEBUG
	if(outf) fprintf(outf, "_c_clean().\n");
#endif
	_hlmode (0);
	_kpmode(0);
	SP->virt_irm = 0;
	_window(-1, -1, -1, -1);
	_syncmodes();
	tputs(exit_ca_mode, 0, _outch);
	tputs(cursor_normal, 0, _outch);
}
