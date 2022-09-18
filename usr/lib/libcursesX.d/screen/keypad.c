#ifdef lint
static char *sccsid = "@(#)keypad.c	4.1	(ULTRIX)	7/2/90";
#endif lint

#include "curses.ext"

/*
 * TRUE => special keys should be passed as a single character by getch.
 */
keypad(win,bf)
WINDOW *win; int bf;
{
	win->_use_keypad = bf;
}
