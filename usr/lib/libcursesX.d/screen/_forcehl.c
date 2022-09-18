#ifdef lint
static char *sccsid = "@(#)_forcehl.c	4.1	(ULTRIX)	7/2/90";
#endif lint


#include "curses.ext"

/*
 * Output the string to get us in the right highlight mode,
 * no matter what mode we are currently in.
 */
_forcehl()
{
#ifdef DEBUG
	if(outf) fprintf(outf, "_forcehl().\n");
#endif
	SP->phys_gr = -1;
	_sethl();
}
