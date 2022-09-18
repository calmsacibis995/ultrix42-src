#ifndef lint
static char sccsid[] = "@(#)lastpart.c	4.1 (decvax!larry) 7/2/90";
#endif

/*******
 *	char *
 *	lastpart(file)	find last part of file name
 *	char *file;
 *
 *	return - pointer to last part
 */

char *
lastpart(file)
char *file;
{
	char *c;

	c = file + strlen(file);
	while (c >= file)
		if (*(--c) == '/')
			break;
	return(++c);
}
