#ifdef lint
static char *sccsid = "@(#)wattrset.c	4.1	(ULTRIX)	7/2/90";
#endif lint

/*
 * 1/26/81 (Berkeley) @(#)standout.c	1.1
 */

# include	"curses.ext"

/*
 * Set selected attributes.
 */
wattrset(win, attrs)
register WINDOW	*win;
int attrs;
{
#ifdef DEBUG
	if(outf) fprintf(outf, "WATTRON(%x, %o)\n", win, attrs);
#endif

	win->_attrs = attrs;
	return 1;
}
