

#ifndef lint
static	char	*sccsid = "@(#)repl.c	4.1	(ULTRIX)	7/17/90";
#endif lint

/*
	Replace each occurrence of `old' with `new' in `str'.
	Return `str'.
*/

repl(str,old,new)
char *str;
char old,new;
{
	return(trnslat(str, &old, &new, str));
}
