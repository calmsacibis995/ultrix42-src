#ifdef lint
static char *sccsid = "@(#)_syncmodes.c	4.1	(ULTRIX)	7/2/90";
#endif lint


#include "curses.ext"

_syncmodes()
{
#ifdef DEBUG
	if(outf) fprintf(outf, "_syncmodes().\n");
#endif
	_sethl();
	_setmode();
	_setwind();
}
