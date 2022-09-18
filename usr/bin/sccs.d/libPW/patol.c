

#ifndef lint
static	char	*sccsid = "@(#)patol.c	4.1	(ULTRIX)	7/17/90";
#endif lint

/*
	Function to convert ascii string to long.  Converts
	positive numbers only.  Returns -1 if non-numeric
	character encountered.
*/

long
patol(s)
register char *s;
{
	long i;

	i = 0;
	while (*s >= '0' && *s <= '9')
		i = 10*i + *s++ - '0';

	if (*s)
		return(-1);
	return(i);
}
