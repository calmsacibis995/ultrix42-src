#ifdef lint
static char *sccsid = "@(#)has_il.c	4.1	(ULTRIX)	7/2/90";
#endif lint

#include "curses.ext"

/*
 * Queries: does the terminal have insert/delete line?
 */
has_il()
{
	return insert_line && delete_line || change_scroll_region;
}
