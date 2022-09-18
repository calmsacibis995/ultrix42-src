#ifdef lint
static char *sccsid = "@(#)curses.c	4.1	(ULTRIX)	7/2/90";
#endif lint

/*
 * Define global variables
 *
 * 3/5/81 (Berkeley) @(#)curses.c	1.2
 */
# include	"curses.ext"

char	*Def_term	= "unknown";	/* default terminal type	*/
WINDOW *stdscr, *curscr;
int	LINES, COLS;
struct screen *SP;

char *curses_version = "Packaged for USG UNIX 6.0, 3/6/83";

# ifdef DEBUG
FILE	*outf;			/* debug output file			*/
# endif

struct	term _first_term;
struct	term *cur_term = &_first_term;

WINDOW *lwin;

int _endwin = FALSE;

int	tputs();
