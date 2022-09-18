#ifdef lint
static char *sccsid = "@(#)vsprintf.c	4.1	(ULTRIX)	7/2/90";
#endif lint

/*LINTLIBRARY*/
#include <stdio.h>
#include <varargs.h>
#define MAXINT 32767

extern int _doprnt();

int
vsprintf(string, format, ap)
char *string, *format;
va_list ap;
{
	register int count;
	FILE siop;

	siop._cnt = MAXINT;
	siop._file = _NFILE;
	siop._flag = _IOWRT;
	siop._base = siop._ptr = string;
	count = _doprnt(format, ap, &siop);
	*siop._ptr = '\0'; /* plant terminating null character */
	return(count);
}
