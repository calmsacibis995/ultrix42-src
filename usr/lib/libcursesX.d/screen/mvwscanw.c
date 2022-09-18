#ifdef lint
static char *sccsid = "@(#)mvwscanw.c	4.1	(ULTRIX)	7/2/90";
#endif lint

# include	"curses.ext"

/* VARARGS */
mvwscanw(win, y, x, fmt, args)
reg WINDOW	*win;
reg int		y, x;
char		*fmt;
int		args; {

	return wmove(win, y, x) == OK ? _sscans(win, fmt, &args) : ERR;
}
