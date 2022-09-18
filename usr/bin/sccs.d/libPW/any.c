

#ifndef lint
static	char	*sccsid = "@(#)any.c	4.1	(ULTRIX)	7/17/90";
#endif lint

/*
	If any character of `s' is `c', return 1
	else return 0.
*/

any(c,s)
register char c, *s;
{
	while (*s)
		if (*s++ == c)
			return(1);
	return(0);
}
