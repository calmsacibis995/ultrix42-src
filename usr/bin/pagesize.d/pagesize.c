#ifndef lint
static	char	*sccsid = "@(#)pagesize.c	4.1	(ULTRIX)	7/17/90";
#endif

main()
{

	printf("%d\n", getpagesize());
}
