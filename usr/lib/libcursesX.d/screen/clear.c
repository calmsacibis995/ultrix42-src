#ifdef lint
static char *sccsid = "@(#)clear.c	4.1	(ULTRIX)	7/2/90";
#endif lint

# include	"curses.ext"

/*
 *	This routine clears the _window.
 *
 * 1/26/81 (Berkeley) @(#)clear.c	1.1
 */
wclear(win)
reg WINDOW	*win; {

	if (win == curscr)
		win = stdscr;
	werase(win);
	win->_clear = TRUE;
	return OK;
}
