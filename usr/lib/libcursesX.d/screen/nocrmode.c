#ifdef lint
static char *sccsid = "@(#)nocrmode.c	4.1	(ULTRIX)	7/2/90";
#endif lint

#include "curses.ext"

nocrmode()
{
	nocbreak();
}
