#ifdef lint
static char *sccsid = "@(#)has_ic.c	4.1	(ULTRIX)	7/2/90";
#endif lint

#include "curses.ext"

/*
 * Does it have insert/delete char?
 */
has_ic()
{
	return insert_character || enter_insert_mode;
}
