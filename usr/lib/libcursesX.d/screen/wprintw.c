#ifdef lint
static char *sccsid = "@(#)wprintw.c	4.1	(ULTRIX)	7/2/90";
#endif lint


# include	"curses.ext"
# include	<varargs.h>

/*
 *	This routine implements a printf on the given window.
 */
/* VARARGS */
wprintw(win, fmt, va_alist)
WINDOW	*win;
char	*fmt;
va_dcl
{
	va_list ap;

	va_start(ap);
	return _sprintw(win, fmt, ap);
}
