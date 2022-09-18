/* @(#)putchar.c	4.1	ULTRIX	7/3/90	*/
/* Originally derived from */
/* @(#)putchar.c	4.1 (Berkeley) 12/21/80 */
/*
 * A subroutine version of the macro putchar
 */
#include <stdio.h>

#undef putchar

putchar(c)
register c;
{
	return(putc(c, stdout));
}
