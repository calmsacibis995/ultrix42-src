#ifndef lint
static char sccsid[] = "@(#)setbrk.c	4.1 (Ultrix) 7/2/90";
/* Original ID:  "@(#)setbrk.c	4.2 8/11/83" */
#endif

#
/*
 *	UNIX shell
 *
 *	S. R. Bourne
 *	Bell Telephone Laboratories
 *
 */

#include	"defs.h"

setbrk(incr)
{
	REG BYTPTR	a=sbrk(incr);
	brkend=a+incr;
	return(a);
}
