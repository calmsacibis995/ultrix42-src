#ifdef lint
static char *sccsid = "@(#)newpad.c	4.1	(ULTRIX)	7/2/90";
#endif lint

# include	"curses.ext"
# include	<signal.h>

char	*calloc();
char	*malloc();
extern	char	*getenv();

extern	WINDOW	*makenew();

/*
 * Like newwin, but makes a pad instead of a window.  A pad is not
 * associated with part of the screen, so it can be bigger.
 */
WINDOW *
newpad(nlines, ncols)
register int	nlines;
{
	register WINDOW	*win;
	register chtype	*sp;
	register int i;
	char *calloc();

	if ((win = makenew(nlines, ncols, 0, 0)) == NULL)
		return NULL;
	win->_flags |= _ISPAD;
	for (i = 0; i < nlines; i++)
		if ((win->_y[i] = (chtype *) calloc(ncols, sizeof (chtype))) == NULL) {
			register int j;

			for (j = 0; j < i; j++)
				free((char *)win->_y[j]);
			free((char *)win->_firstch);
			free((char *)win->_lastch);
			free((char *)win->_y);
			free((char *)win);
			return NULL;
		}
		else
			for (sp = win->_y[i]; sp < win->_y[i] + ncols; )
				*sp++ = ' ';
	return win;
}
