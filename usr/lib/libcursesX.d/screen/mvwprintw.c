#ifdef lint
static char *sccsid = "@(#)mvwprintw.c	4.1	(ULTRIX)	7/2/90";
#endif lint

# include	"curses.ext"

/* VARARGS */
mvwprintw(win, y, x, fmt, args)
reg WINDOW	*win;
reg int		y, x;
char		*fmt;
int		args;
{

	return wmove(win, y, x) == OK ? _sprintw(win, fmt, &args) : ERR;
}
