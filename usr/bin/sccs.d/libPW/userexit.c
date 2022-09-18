

#ifndef lint
static	char	*sccsid = "@(#)userexit.c	4.1	(ULTRIX)	7/17/90";
#endif lint

/*
	Default userexit routine for fatal and setsig.
	User supplied userexit routines can be used for logging.
*/

userexit(code)
{
	return(code);
}
