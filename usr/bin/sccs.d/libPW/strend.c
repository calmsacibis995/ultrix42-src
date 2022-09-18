

#ifndef lint
static	char	*sccsid = "@(#)strend.c	4.1	(ULTRIX)	7/17/90";
#endif lint

char *strend(p)
register char *p;
{
	while (*p++)
		;
	return(--p);
}
