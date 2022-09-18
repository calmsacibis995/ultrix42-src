#ifdef lint
static char *sccsid = "@(#)setterm.c	4.1	(ULTRIX)	7/2/90";
#endif lint

# include	"curses.ext"
# include	<signal.h>

char	*calloc();
char	*malloc();
extern	char	*getenv();

extern	WINDOW	*makenew();

/*
 * Low level interface, for compatibility with old curses.
 */
setterm(type)
char *type;
{
	setupterm(type, 1, 0);
}
