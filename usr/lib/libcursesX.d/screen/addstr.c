#ifdef lint
static char *sccsid = "@(#)addstr.c	4.1	(ULTRIX)	7/2/90";
#endif lint

# include	"curses.ext"

/*
 *	This routine adds a string starting at (_cury,_curx)
 *
 * 1/26/81 (Berkeley) @(#)addstr.c	1.1
 */

#define MASK8 0xFF

waddstr(win,str)
register WINDOW	*win; 
register char	*str;
{
# ifdef DEBUG
	if(outf)
	{
		if( win == stdscr )
		{
			fprintf(outf, "WADDSTR(stdscr, ");
		}
		else
		{
			fprintf(outf, "WADDSTR(%o, ", win);
		}
		fprintf(outf, "\"%s\")\n", str);
	}
# endif	DEBUG
	while( *str )
	{
		if( waddch( win, ( chtype ) *str++ & MASK8 ) == ERR )
		{
			return ERR;
		}
	}
	return OK;
}
