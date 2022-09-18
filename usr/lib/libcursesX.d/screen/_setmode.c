#ifdef lint
static char *sccsid = "@(#)_setmode.c	4.1	(ULTRIX)	7/2/90";
#endif lint


#include "curses.ext"

extern	int	_outch();

_setmode ()
{
#ifdef DEBUG
	if(outf) fprintf(outf, "_setmode().\n");
#endif
	if (SP->virt_irm == SP->phys_irm)
		return;
	tputs(SP->virt_irm==1 ? enter_insert_mode : exit_insert_mode, 0, _outch);
	SP->phys_irm = SP->virt_irm;
}
