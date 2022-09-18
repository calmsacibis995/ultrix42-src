#ifdef lint
static char *sccsid = "@(#)beep.c	4.1	(ULTRIX)	7/2/90";
#endif lint


#include "curses.ext"

extern	int	_outch();

beep()
{
#ifdef DEBUG
	if(outf) fprintf(outf, "beep().\n");
#endif
    if (bell)
	tputs (bell, 0, _outch);
    else
	tputs (flash_screen, 0, _outch);
    __cflush();
}
