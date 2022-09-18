# include	"curses.ext"
# include	<varargs.h>

/*
 * implement the mvscanw commands.  Due to the variable number of
 * arguments, they cannot be macros.  Another sigh....
 *
 * 5/17/83 (Berkeley) @(#)mvscanw.c	1.2
 */

mvscanw(y, x, fmt, va_alist)
reg int		y, x;
char		*fmt;
va_dcl
{
	va_list ap;

	va_start(ap);
	return move(y, x) == OK ? _sscans(stdscr, fmt, ap) : ERR;
}

mvwscanw(win, y, x, fmt, va_alist)
reg WINDOW	*win;
reg int		y, x;
char		*fmt;
va_dcl
{
	va_list ap;

	va_start(ap);
	return wmove(win, y, x) == OK ? _sscans(win, fmt, ap) : ERR;
}
