#ifndef lint
static char	*sccsid = "@(#)batch.c	4.1	(Ultrix)	7/17/90";
#endif
/*
 * batch [file]
 *
 * Modification history
 *
 * 17-Feb-88 - Gary A. Gaudet
 *	X/OPEN
 */

#include <stdio.h>

main(argc, argv)
int argc;
char **argv;
{
	if (argc == 1) {
		execl ("/usr/bin/at", "/usr/bin/at", "midnight", (char *) 0);
	} else if (argc == 2) {
		execl ("/usr/bin/at", "/usr/bin/at","midnight", argv[1], (char *) 0);
	} else {
		(void) fprintf (stderr, "%s: arg count\nUSAGE: %s [file]\n", argv[0], argv[0]);
		exit (-1);
	}
}
