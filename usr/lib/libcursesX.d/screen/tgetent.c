#ifdef lint
static char *sccsid = "@(#)tgetent.c	4.1	(ULTRIX)	7/2/90";
#endif lint

/*
 * Simulation of termcap using terminfo.
 */

#include "curses.ext"


int
tgetent(bp, name)
char *bp, *name;
{
	int rv;

	if (setupterm(name, 1, &rv) >= 0)
		/* Leave things as they were (for compatibility) */
		reset_shell_mode();
	return rv;
}
