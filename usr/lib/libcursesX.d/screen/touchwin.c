#ifdef lint
static char *sccsid = "@(#)touchwin.c	4.1	(ULTRIX)	7/2/90";
#endif lint

# include	"curses.ext"

/*
 * make it look like the whole window has been changed.
 *
 * 1/26/81 (Berkeley) @(#)touchwin.c	1.1
 */
touchwin(win)
reg WINDOW	*win;
{
	reg int		y, maxy, maxx;

#ifdef DEBUG
	if (outf) fprintf(outf, "touchwin(%x)\n", win);
#endif
	maxy = win->_maxy;
	maxx = win->_maxx - 1;
	for (y = 0; y < maxy; y++) {
		win->_firstch[y] = 0;
		win->_lastch[y] = maxx;
	}
}
