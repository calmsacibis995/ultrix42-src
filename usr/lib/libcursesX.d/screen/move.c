#ifdef lint
static char *sccsid = "@(#)move.c	4.1	(ULTRIX)	7/2/90";
#endif lint

# include	"curses.ext"

/*
 *	This routine moves the cursor to the given point
 *
 * 1/26/81 (Berkeley) @(#)move.c	1.1
 */
wmove(win, y, x)
reg WINDOW	*win;
reg int		y, x;
{

# ifdef DEBUG
	if(outf) fprintf(outf, "MOVE to win ");
	if( win == stdscr )
	{
		if(outf) fprintf(outf, "stdscr ");
	}
	else
	{
		if(outf) fprintf(outf, "%o ", win);
	}
	if(outf) fprintf(outf, "(%d, %d)\n", y, x);
# endif
	if( x >= win->_maxx || y >= win->_maxy )
	{
		return ERR;
	}
	win->_curx = x;
	win->_cury = y;
	return OK;
}
