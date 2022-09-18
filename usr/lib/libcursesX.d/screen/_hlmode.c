#ifdef lint
static char *sccsid = "@(#)_hlmode.c	4.1	(ULTRIX)	7/2/90";
#endif lint


#include "curses.ext"

_hlmode (on)
{
#ifdef DEBUG
	if(outf) fprintf(outf, "_hlmode(%o).\n", on);
#endif
	SP->virt_gr = on;
}
