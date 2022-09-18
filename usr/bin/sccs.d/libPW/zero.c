

#ifndef lint
static	char	*sccsid = "@(#)zero.c	4.1	(ULTRIX)	7/17/90";
#endif lint

/*
	Zero `cnt' bytes starting at the address `ptr'.
	Return `ptr'.
*/

char	*zero(p,n)
register char *p;
register int n;
{
	char *op = p;
	while (--n >= 0)
		*p++ = 0;
	return(op);
}
