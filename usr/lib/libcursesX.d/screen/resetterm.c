#ifdef lint
static char *sccsid = "@(#)resetterm.c	4.1	(ULTRIX)	7/2/90";
#endif lint


#include "curses.ext"
#include "../local/uparm.h"

extern	struct term *cur_term;

resetterm()
{
	reset_shell_mode();
}
