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
printw(fmt, va_alist)
char	*fmt;
va_dcl
{
	va_list ap;

        va_start(ap);
        return _sprintw(stdscr, fmt, ap);
}

/*
 *	This routine implements a printf on the given window.
 */
wprintw(win, fmt, va_alist)
WINDOW	*win;
char	*fmt;
va_dcl
{
	va_list ap;

        va_start(ap);
	return _sprintw(win, fmt, ap);
}
/*
 *	This routine actually executes the printf and adds it to the window
 *
 *	This is really a modified version of "sprintf".  As such,
 * it assumes that sprintf interfaces with the other printf functions
 * in a certain way.  If this is not how your system works, you
 * will have to modify this routine to use the interface that your
 * "sprintf" uses.
 */
_sprintw(win, fmt, args)
WINDOW	*win;
char	*fmt;
int	*args; {

	FILE	junk;
	char	buf[512];

	junk._flag = _IOWRT + _IOSTRG;
	junk._ptr = buf;
	junk._cnt = 32767;
	_doprnt(fmt, args, &junk);
	putc('\0', &junk);
	return waddstr(win, buf);
}
