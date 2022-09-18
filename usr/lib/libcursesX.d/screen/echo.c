#ifdef lint
static char *sccsid = "@(#)echo.c	4.1	(ULTRIX)	7/2/90";
#endif lint

#include "curses.ext"

echo()	
{
	SP->fl_echoit = TRUE;
}
