

#ifndef lint
static	char	*sccsid = "@(#)logname.c	4.1	(ULTRIX)	7/17/90";
#endif lint

char *
logname()
{
	return((char *)getenv("LOGNAME"));
}
