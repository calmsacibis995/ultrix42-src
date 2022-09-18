#ifdef lint
static char *sccsid = "@(#)printw.c	4.1	(ULTRIX)	7/2/90";
#endif lint

/*
 * printw and friends
 *
 * 1/26/81 (Berkeley) @(#)printw.c	1.1
 */

# include	"curses.ext"
# include	<varargs.h>

/*
 *	This routine implements a printf on the standard screen.
 */
/* VARARGS */
printw(fmt, va_alist)
char	*fmt;
va_dcl
{
	va_list ap;

	va_start(ap);
	return _sprintw(stdscr, fmt, ap);
}
