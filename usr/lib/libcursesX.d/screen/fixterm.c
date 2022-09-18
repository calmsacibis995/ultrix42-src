#ifdef lint
static char *sccsid = "@(#)fixterm.c	4.1	(ULTRIX)	7/2/90";
#endif lint

#include "curses.ext"
#include "../local/uparm.h"

extern	struct term *cur_term;

fixterm()
{
	reset_prog_mode();
}
