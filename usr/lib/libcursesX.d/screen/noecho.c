#ifdef lint
static char *sccsid = "@(#)noecho.c	4.1	(ULTRIX)	7/2/90";
#endif lint

#include "curses.ext"

noecho()
{
	SP->fl_echoit = FALSE;
}
