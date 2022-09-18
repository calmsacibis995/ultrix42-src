#ifndef lint
static	char	*sccsid = "@(#)cat.c	4.1	(ULTRIX)	7/17/90";
#endif lint

/*
	Concatenate strings.
 
	cat(destination,source1,source2,...,sourcen,0);
 
	returns destination.
*/

#include <varargs.h>

char *cat(dest,va_alist)
char *dest;
va_dcl
{

	va_list ap;
	char *src;
	char *dp;

	va_start(ap);

	src = va_arg(ap, char *);
	dp = dest;
	while (src) {
		while (*src) {
			*dp = *src;
			dp++;
			src++;
		}
		src = va_arg(ap, char *);
	}
	*dp = '\0';

	return(dest);
}
