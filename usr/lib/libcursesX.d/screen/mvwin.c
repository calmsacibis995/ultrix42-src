#ifdef lint
static char *sccsid = "@(#)mvwin.c	4.1	(ULTRIX)	7/2/90";
#endif lint

# include	"curses.ext"

/*
 * relocate the starting position of a _window
 *
 * 1/26/81 (Berkeley) @(#)mvwin.c	1.1
 */

mvwin(win, by, bx)
reg WINDOW	*win;
reg int		by, bx; {

	if (by + win->_maxy > LINES || bx + win->_maxx > COLS)
		return ERR;
	win->_begy = by;
	win->_begx = bx;
	touchwin(win);
	return OK;
}
