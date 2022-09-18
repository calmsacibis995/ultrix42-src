#ifdef lint
static char *sccsid = "@(#)nodelay.c	4.1	(ULTRIX)	7/2/90";
#endif lint

#include "curses.ext"

/*
 * TRUE => don't wait for input, but return -1 instead.
 */
nodelay(win,bf)
WINDOW *win; int bf;
{
	_fixdelay(win->_nodelay, bf);
	win->_nodelay = bf;
}
