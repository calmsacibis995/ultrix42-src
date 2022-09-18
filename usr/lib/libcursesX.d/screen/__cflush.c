#ifdef lint
static char *sccsid = "@(#)__cflush.c	4.1	(ULTRIX)	7/2/90";
#endif lint

/*
 * This routine is one of the main things
 * in this level of curses that depends on the outside
 * environment.
 */
#include "curses.ext"

/*
 * Flush stdout.
 */
__cflush()
{
	fflush(SP->term_file);
}
