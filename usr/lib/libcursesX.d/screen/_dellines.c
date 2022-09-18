#ifndef lint
static char *sccsid = "@(#)_dellines.c	4.1	(ULTRIX)	7/2/90";
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
 * 02/28/90 GWS	replaced "*delete_line==0" NULL pointer dereferences with
 *		 "delete_line==NULL || *delete_line==NULL"
 *		 so this code will be machine-independent.  This prevents core
 *		 dumps on RISC machines when term=vt100.
 *
 */

#include "curses.ext"

char *tparm();

extern	int	_outch();

_dellines (n)
{
	register int i;

#ifdef DEBUG
	if(outf) fprintf(outf, "_dellines(%d).\n", n);
#endif
	if (lines - SP->phys_y <= n && (clr_eol && n == 1 || clr_eos)) {
		tputs(clr_eos, n, _outch);
	} else
	if (scroll_forward && SP->phys_y == SP->des_top_mgn /* &&costSF<costDL */) {
		/*
		 * Use forward scroll mode of the terminal, at
		 * the bottom of the window.  Linefeed works
		 * too, since we only use it from the bottom line.
		 */
		_setwind();
		for (i = n; i > 0; i--) {
			_pos(SP->des_bot_mgn, 0);
			tputs(scroll_forward, 1, _outch);
			SP->ml_above++;
		}
		if (SP->ml_above + lines > lines_of_memory)
			SP->ml_above = lines_of_memory - lines;
	} else if (parm_delete_line && (n>1 || (delete_line==NULL || *delete_line==NULL))) {
		tputs(tparm(parm_delete_line, n, SP->phys_y), lines-SP->phys_y, _outch);
	}
	else if (change_scroll_region && (delete_line==NULL || *delete_line==NULL)) {
		/* vt100: fake delete_line by changing scrolling region */
		/* Save since change_scroll_region homes cursor */
		tputs(save_cursor, 1, _outch);
		tputs(tparm(change_scroll_region,
			SP->phys_y, SP->des_bot_mgn), 1, _outch);
		/* go to bottom left corner.. */
		tputs(tparm(cursor_address, SP->des_bot_mgn, 0), 1, _outch);
		for (i=0; i<n; i++)	/* .. and scroll n times */
			tputs(scroll_forward, 1, _outch);
		/* restore scrolling region */
		tputs(tparm(change_scroll_region,
			SP->des_top_mgn, SP->des_bot_mgn), 1, _outch);
		tputs(restore_cursor, 1, _outch);	/* put SP->curptr back */
		SP->phys_top_mgn = SP->des_top_mgn;
		SP->phys_bot_mgn = SP->des_bot_mgn;
	}
	else {
		for (i = 0; i < n; i++)
			tputs(delete_line, lines-SP->phys_y, _outch);
	}
}
