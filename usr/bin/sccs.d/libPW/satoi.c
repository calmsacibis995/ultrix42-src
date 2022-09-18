
#ifndef lint
static	char	*sccsid = "@(#)satoi.c	4.1	(ULTRIX)	7/17/90";
#endif lint

# include	"sys/types.h"
# include	"macros.h"

char *satoi(p,ip)
register char *p;
register int *ip;
{
	register int sum;

	sum = 0;
	while (numeric(*p))
		sum = sum * 10 + (*p++ - '0');
	*ip = sum;
	return(p);
}
