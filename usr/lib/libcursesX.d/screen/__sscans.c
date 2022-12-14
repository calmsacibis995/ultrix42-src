#ifdef lint
static char *sccsid = "@(#)__sscans.c	4.1	(ULTRIX)	7/2/90";
#endif lint

/*
 * 1/26/81 (Berkeley) @(#)scanw.c	1.1
 */

# include	"curses.ext"
# include	<varargs.h>

/*
 *	This routine actually executes the scanf from the window.
 *
 *	This code calls vsscanf, which is like sscanf except
 * 	that it takes a va_list as an argument pointer instead
 *	of the argument list itself.  We provide one until
 *	such a routine becomes available.
 */

__sscans(win, fmt, ap)
WINDOW	*win;
char	*fmt;
va_list	ap;
{
	char	buf[256];

	if (wgetstr(win, buf) == ERR)
		return ERR;

	return vsscanf(buf, fmt, ap);
}
