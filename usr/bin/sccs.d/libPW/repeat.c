

#ifndef lint
static	char	*sccsid = "@(#)repeat.c	4.1	(ULTRIX)	7/17/90";
#endif lint

/*
	Set `result' equal to `str' repeated `repfac' times.
	Return `result'.
*/

char *repeat(result,str,repfac)
char *result, *str;
register unsigned repfac;
{
	register char *r, *s;

	r = result;
	for (++repfac; --repfac > 0; --r)
		for (s=str; *r++ = *s++; );
	*r = '\0';
	return(result);
}
