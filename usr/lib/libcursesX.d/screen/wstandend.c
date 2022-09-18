#ifdef lint
static char *sccsid = "@(#)wstandend.c	4.1	(ULTRIX)	7/2/90";
#endif lint

/*
 * 1/26/81 (Berkeley) @(#)standout.c	1.1
 */

# include	"curses.ext"

/*
 * exit standout mode
 */
wstandend(win)
register WINDOW	*win;
{
#ifdef DEBUG
	if(outf) fprintf(outf, "WSTANDEND(%x)\n", win);
#endif

	win->_attrs = 0;
	return 1;
}
