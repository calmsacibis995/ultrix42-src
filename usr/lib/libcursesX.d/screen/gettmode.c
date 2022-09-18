#ifdef lint
static char *sccsid = "@(#)gettmode.c	4.1	(ULTRIX)	7/2/90";
#endif lint

# include	"curses.ext"
# include	<signal.h>

char	*calloc();
char	*malloc();
extern	char	*getenv();

extern	WINDOW	*makenew();

gettmode()
{
	/* No-op included only for upward compatibility. */
}
