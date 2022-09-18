#ifdef lint
static char *sccsid = "@(#)wrefresh.c	4.1	(ULTRIX)	7/2/90";
#endif lint

/*
 * make the current screen look like "win" over the area covered by
 * win.
 *
 * 7/9/81 (Berkeley) @(#)refresh.c	1.6
 */

# include	"curses.ext"

/* Put out window and update screen */
wrefresh(win)
WINDOW	*win;
{
	wnoutrefresh(win);
	return doupdate();
}
