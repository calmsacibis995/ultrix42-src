

#ifndef lint
static	char	*sccsid = "@(#)imatch.c	4.1	(ULTRIX)	7/17/90";
#endif lint

/*
	initial match
	if `prefix' is a prefix of `string' return 1
	else return 0
*/

imatch(prefix,string)
register char *prefix, *string;
{
	while (*prefix++ == *string++)
		if (*prefix == 0)
			return(1);
	return(0);
}
