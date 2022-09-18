#ifdef lint
static char *sccsid = "@(#)prefresh.c	4.1	(ULTRIX)	7/2/90";
#endif lint

/*
 * make the current screen look like "win" over the area covered by
 * win.
 *
 */

/*	@(#) prefresh.c: 1.1 10/15/83	(1.14	3/6/83)	*/
#include	"curses.ext"

/* Like wrefresh but refreshing from a pad. */
prefresh(pad, pminrow, pmincol, sminrow, smincol, smaxrow, smaxcol)
WINDOW	*pad;
int pminrow, pmincol, sminrow, smincol, smaxrow, smaxcol;
{
	pnoutrefresh(pad, pminrow, pmincol, sminrow, smincol, smaxrow, smaxcol);
	return doupdate();
}
