#ifdef lint
static char *sccsid = "@(#)longname.c	4.1	(ULTRIX)	7/2/90";
#endif lint


/*
 *	This routine returns the long name of the terminal.
 */
char *
longname()
{
	register char	*cp;
	extern char ttytype[];

	for (cp=ttytype; *cp++; )		/* Go to end of string */
		;
	while (*--cp != '|' && cp>=ttytype)	/* Back up to | or beginning */
		;
	return ++cp;
}
