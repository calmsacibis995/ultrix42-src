#ifdef lint
static char *sccsid = "@(#)idlok.c	4.1	(ULTRIX)	7/2/90";
#endif lint

#include "curses.ext"

/*
 * TRUE => OK to use insert/delete line.
 */
idlok(win,bf)
WINDOW *win;
int bf;
{
	win->_use_idl = bf;
}
