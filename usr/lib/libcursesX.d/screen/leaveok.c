#ifdef lint
static char *sccsid = "@(#)leaveok.c	4.1	(ULTRIX)	7/2/90";
#endif lint

#include "curses.ext"

/*
 * TRUE => OK to leave cursor where it happens to fall after refresh.
 */
leaveok(win,bf)
WINDOW *win; int bf;
{
	win->_leave = bf;
}
