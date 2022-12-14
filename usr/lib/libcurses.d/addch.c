#ifndef lint
static char *sccsid = "@(#)addch.c	4.1	(ULTRIX)	7/2/90";
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
 *   University of  California,  Berkeley.   Use,  duplication,  or	*
 *   disclosure is subject to restrictions under license agreements	*
 *   with University of California.					*
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
 * 03/07/90 GWS	removed call to wrefresh() when y >= maxy and scrolling is
 *		 enabled, to avoid cursor-up problems on vt100/vt200/vt300
 *		 (Esc[A either being ignored or going down instead of up), and
 *		 to improve output performance
 *
 */

# include	"curses.ext"

/*
 *	This routine adds the character to the current position
 *
 * @(#)addch.c	1.5 (Berkeley) 5/19/83
 */
waddch(win, c)
reg WINDOW	*win;
char		c;
{
	reg int		x, y;
	reg WINDOW	*wp;

	x = win->_curx;
	y = win->_cury;
# ifdef FULLDEBUG
	fprintf(outf, "ADDCH('%c') at (%d, %d)\n", c, y, x);
# endif
	if (y >= win->_maxy || x >= win->_maxx || y < 0 || x < 0)
		return ERR;
	switch (c) {
	  case '\t':
	  {
		reg int		newx;

		for (newx = x + (8 - (x & 07)); x < newx; x++)
			if (waddch(win, ' ') == ERR)
				return ERR;
		return OK;
	  }

	  default:
# ifdef FULLDEBUG
		fprintf(outf, "ADDCH: 1: y = %d, x = %d, firstch = %d, lastch = %d\n", y, x, win->_firstch[y], win->_lastch[y]);
# endif
		if (win->_flags & _STANDOUT)
			c |= _STANDOUT;
		set_ch(win, y, x, c, NULL);
		for (wp = win->_nextp; wp != win; wp = wp->_nextp)
			set_ch(wp, y, x, c, win);
		win->_y[y][x++] = c;
		if (x >= win->_maxx) {
			x = 0;
newline:
			if (++y >= win->_maxy)
				if (win->_scroll) {
					scroll(win);
					--y;
				}
				else
					return ERR;
		}
# ifdef FULLDEBUG
		fprintf(outf, "ADDCH: 2: y = %d, x = %d, firstch = %d, lastch = %d\n", y, x, win->_firstch[y], win->_lastch[y]);
# endif
		break;
	  case '\n':
		wclrtoeol(win);
		if (!NONL)
			x = 0;
		goto newline;
	  case '\r':
		x = 0;
		break;
	  case '\b':
		if (--x < 0)
			x = 0;
		break;
	}
	win->_curx = x;
	win->_cury = y;
	return OK;
}

/*
 * set_ch:
 *	Set the first and last change flags for this window.
 */
static
set_ch(win, y, x, ch, orig)
reg WINDOW	*win;
int		y, x;
WINDOW		*orig; {

	if (orig != NULL) {
		y -= win->_begy - orig->_begy;
		x -= win->_begx - orig->_begx;
	}
	if (y < 0 || y >= win->_maxy || x < 0 || x >= win->_maxx)
		return;
	if (win->_y[y][x] != ch) {
		if (win->_firstch[y] == _NOCHANGE)
			win->_firstch[y] = win->_lastch[y] = x;
		else if (x < win->_firstch[y])
			win->_firstch[y] = x;
		else if (x > win->_lastch[y])
			win->_lastch[y] = x;
	}
}
