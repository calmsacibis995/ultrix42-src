# include	"curses.ext"
# include	<varargs.h>

/*
 * implement the mvprintw commands.  Due to the variable number of
 * arguments, they cannot be macros.  Sigh....
 *
 * 1/26/81 (Berkeley) @(#)mvprintw.c	1.1
 */

mvprintw(y, x, fmt, va_alist)
reg int		y, x;
char		*fmt;
va_dcl
{
	va_list ap;

	va_start(ap);
	return move(y, x) == OK ? _sprintw(stdscr, fmt, ap) : ERR;
}

mvwprintw(win, y, x, fmt, va_alist)
reg WINDOW	*win;
reg int		y, x;
char		*fmt;
va_dcl
{
	va_list ap;

	va_start(ap);
	return wmove(win, y, x) == OK ? _sprintw(win, fmt, ap) : ERR;
}
