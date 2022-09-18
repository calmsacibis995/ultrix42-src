#ifdef lint
static char *sccsid = "@(#)wstandout.c	4.1	(ULTRIX)	7/2/90";
#endif lint

/*
 * 1/26/81 (Berkeley) @(#)standout.c	1.1
 */

# include	"curses.ext"

/*
 * enter standout mode
 */
wstandout(win)
register WINDOW	*win;
{
#ifdef DEBUG
	if(outf) fprintf(outf, "WSTANDOUT(%x)\n", win);
#endif

	win->_attrs |= A_STANDOUT;
	return 1;
}
