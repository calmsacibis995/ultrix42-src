#ifdef lint
static char *sccsid = "@(#)_insmode.c	4.1	(ULTRIX)	7/2/90";
#endif lint


#include "curses.ext"

/*
 * Set the virtual insert/replacement mode to new.
 */
_insmode (new)
int new;
{
#ifdef DEBUG
	if(outf) fprintf(outf, "_insmode(%d).\n", new);
#endif
	SP->virt_irm = new;
}
