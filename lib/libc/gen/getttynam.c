#ifndef lint
static	char	*sccsid = "@(#)getttynam.c	4.1	(ULTRIX)	7/3/90";
#endif lint

/* @(#)getttynam.c	4.1 (Berkeley) 4/27/84 */

#include "ttyent.h"

struct ttyent *
getttynam(tty)
	char *tty;
{
	register struct ttyent *t;

	setttyent();
	while (t = getttyent()) {
		if (strcmp(tty, t->ty_name) == 0)
			break;
	}
	endttyent();
	return (t);
}
