/* @(#)isatty.c	4.1	(Berkeley) 7/3/90 */
/*
 * Returns 1 iff file is a tty
 */

#include <sys/termios.h>

isatty(f)
{
	struct termios ttyb;

	if (ioctl(f,TCGETP,&ttyb) < 0)
		return(0);
	return(1);
}
