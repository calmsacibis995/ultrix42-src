

#ifndef lint
static	char	*sccsid = "@(#)xunlink.c	4.1	(ULTRIX)	7/17/90";
#endif lint

/*
	Interface to unlink(II) which handles all error conditions.
	Returns 0 on success,
	fatal() on failure.
*/

xunlink(f)
{
	if (unlink(f))
		return(xmsg(f,"xunlink"));
	return(0);
}
