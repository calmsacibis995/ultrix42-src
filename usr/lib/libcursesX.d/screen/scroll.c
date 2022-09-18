#ifdef lint
static char *sccsid = "@(#)scroll.c	4.1	(ULTRIX)	7/2/90";
#endif lint

# include	"curses.ext"

/*
 *	This routine scrolls the window up a line.
 *
 * 7/8/81 (Berkeley) @(#)scroll.c	1.2
 */
scroll(win)
WINDOW *win;
{
	_tscroll(win, 1);
}
