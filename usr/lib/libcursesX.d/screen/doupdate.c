#ifndef lint
static	char	*sccsid = "@(#)doupdate.c	4.2	(ULTRIX)	11/15/90";
#endif lint

/************************************************************************
 *									*
 *       Copyright (c) Digital Equipment Corporation, 1988, 1990	*
 *									*
 *   All Rights Reserved.  Unpublished rights  reserved  under  the	*
 *   copyright laws of the United States.				*
 *									*
 *   The software contained on this media  is  proprietary  to  and	*
 *   embodies  the  confidential  technology  of  Digital Equipment	*
 *   Corporation.  Possession, use, duplication or dissemination of	*
 *   the  software and media is authorized only pursuant to a valid	*
 *   written license from Digital Equipment Corporation.		*
 *									*
 *   RESTRICTED RIGHTS LEGEND   Use, duplication, or disclosure  by	*
 *   the U.S. Government is subject to restrictions as set forth in	*
 *   Subparagraph (c)(1)(ii)  of  DFARS  252.227-7013,  or  in  FAR	*
 *   52.227-19, as applicable.						*
 *									*
 *   This software is  derived  from  software  received  from  the	*
 *   University    of   California,   Berkeley,   and   from   Bell	*
 *   Laboratories.  Use, duplication, or disclosure is  subject  to	*
 *   restrictions  under  license  agreements  with  University  of	*
 *   California and with AT&T.						*
 *									*
 ************************************************************************/
/**/

/*
 * make the current screen look like "win" over the area covered by
 * win.
 *
 * 7/9/81 (Berkeley) @(#)refresh.c	1.6
 */

/*	@(#) doupdate.c: 1.1 10/15/83	(1.14	3/6/83)	*/

/*
 * Modification History
 *
 * 10/02/90 GWS  move cursor if `leave' set, even if a pad window
 *		 refresh cursor position only if this is not a pad
 */

#include	"curses.ext"

extern	WINDOW *lwin;

/* Update screen */
doupdate()
{
	int rc;
	extern int _endwin;
	int _outch();

#ifdef	DEBUG
	if(outf) fprintf( outf, "doupdate()\n" );
#endif	DEBUG

	if( lwin == NULL )
	{
		return ERR;
	}

	if( _endwin )
	{
		/*
		 * We've called endwin since last refresh.  Undo the
		 * effects of this call.
		 */

		_fixdelay(FALSE, SP->fl_nodelay);
		if (stdscr->_use_meta)
			tputs(meta_on, 1, _outch);
		_endwin = FALSE;
		SP->doclear = TRUE;
		reset_prog_mode();
	}

	/* Tell the back end where to leave the cursor */
	if( lwin->_leave )
	{
#ifdef	DEBUG
		if(outf) fprintf( outf, "'_ll_move(-1, -1)' being done.\n" );
#endif	DEBUG
		_ll_move(-1, -1);
	}
	else
	{
		if ( ! ( lwin->_flags&_ISPAD ) )
		{
#ifdef	DEBUG
			if(outf) fprintf( outf,
			   "'lwin->_cury+lwin->_begy, lwin->_curx+lwin->_begx' being done.\n" );
#endif	DEBUG
			_ll_move( lwin->_cury+lwin->_begy, lwin->_curx+lwin->_begx );
		}
	}
#ifdef	DEBUG
	if(outf) fprintf( outf, "doing 'rc = _ll_refresh(lwin->_use_idl)'.\n" );
#endif	DEBUG
	rc = _ll_refresh(lwin->_use_idl);
#ifdef	DEBUG
	_dumpwin(lwin);
#endif	DEBUG
	return rc;
}
