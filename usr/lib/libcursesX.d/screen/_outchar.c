#ifdef lint
static char *sccsid = "@(#)_outchar.c	4.1	(ULTRIX)	7/2/90";
#endif lint

/*
 * Handy functions to put out a string with padding.
 * These make two assumptions:
 *	(1) Output is via stdio to stdout through putchar.
 *	(2) There is no count of affected lines.  Thus, this
 *	    routine is only valid for certain capabilities,
 *	    i.e. those that don't have *'s in the documentation.
 */
#include <stdio.h>

/*
 * Routine to act like putchar for passing to tputs.
 */
_outchar(ch)
char ch;
{
	putchar(ch);
}