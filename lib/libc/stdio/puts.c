/* @(#)puts.c	4.3	ULTRIX	9/10/90 */
/* Originally derived from: */
/* @(#)puts.c	4.1 (Berkeley) 12/21/80 */
#include	<stdio.h>

puts(s)
register char *s;
{
	register char c;

/* Put out characters until either a NUL or an error return (from _flsbuf) */
/* The body of the loop is equivalent to, but more efficient than,
 *	if (putchar(c) == EOF) return EOF;
 */
	while (c = *s++)
		if (--stdout->_cnt>=0)
			*stdout->_ptr++=c;
		else
			if (_flsbuf((unsigned char)c, stdout) == EOF)
				return EOF;
	return(putchar('\n'));
}
