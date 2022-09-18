#ifdef lint
static char *sccsid = "@(#)baudrate.c	4.1	(ULTRIX)	7/2/90";
#endif lint

#include "curses.ext"

int
baudrate()
{
	return SP->baud;
}
