/*	@(#)putw.c	4.1	ULTRIX	7/3/90	*/
/*
 *	reeves, 1990-Jan-15: added error handling.
 */
/* @(#)putw.c	4.1 (Berkeley) 12/21/80 */
#include <stdio.h>

putw(w, iop)
register FILE *iop;
{
	register char *p;
	register i;

	p = (char *)&w;
	for (i=sizeof(int); (--i>=0) && (putc(*p++, iop) != -1); )
		;
	return(ferror(iop));
}
