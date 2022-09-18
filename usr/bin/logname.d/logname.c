#ifndef LINT
static char *sccsid = "@(#)logname.c	4.1	(ULTRIX)	7/17/90";
#endif

#include <stdio.h>
main() {
	char *name, *cuserid();

	name = cuserid((char *)NULL);
	if (name == NULL)
		return (1);
	(void) puts (name);
	return (0);
}
