#ifdef lint
static char *sccsid = "@(#)delwin.c	4.1	(ULTRIX)	7/2/90";
#endif lint

# include	"curses.ext"

/*
 *	This routine deletes a _window and releases it back to the system.
 *
 * 1/26/81 (Berkeley) @(#)delwin.c	1.1
 */
delwin(win)
reg WINDOW	*win; {

	reg int	i;

	if (!(win->_flags & _SUBWIN))
		for (i = 0; i < win->_maxy && win->_y[i]; i++)
			free((char *) win->_y[i]);
	free((char *) win->_y);
	free((char *) win);
}
