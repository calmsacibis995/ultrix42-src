#ifdef lint
static char *sccsid = "@(#)_clearline.c	4.1	(ULTRIX)	7/2/90";
#endif lint


#include "curses.ext"

/*
 * '_clearline' positions the cursor at the beginning of the
 * indicated line and clears the line (in the image)
 */
_clearline (row)
{
	_ll_move (row, 0);
	SP->std_body[row+1] -> length = 0;
}
