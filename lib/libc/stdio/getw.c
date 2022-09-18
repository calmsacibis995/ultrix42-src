/*	@(#)getw.c	4.1	ULTRIX	7/3/90	*/
/*
 *	reeves, 1990-Jan-15: added error handling.
 */
/* @(#)getw.c	4.1 (Berkeley) 12/21/80 */
#include <stdio.h>

getw(iop)
register FILE *iop;
{
	register i;
	register int x;
	register char *p;
	int w;

	p = (char *)&w;
	for (i=sizeof(int); --i>=0;)
		if ((x = getc(iop)) != EOF)
			*p++ = x;
		else
			break;
	if (feof(iop) || ferror(iop))
		return(EOF);
	return(w);
}
