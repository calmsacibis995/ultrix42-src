#ifdef lint
static char *sccsid = "@(#)_clearhl.c	4.1	(ULTRIX)	7/2/90";
#endif lint


#include "curses.ext"

_clearhl ()
{
#ifdef DEBUG
	if(outf) fprintf(outf, "_clearhl().\n");
#endif
	if (SP->phys_gr) {
		register oldes = SP->virt_gr;
		SP->virt_gr = 0;
		_sethl ();
		SP->virt_gr = oldes;
	}
}
