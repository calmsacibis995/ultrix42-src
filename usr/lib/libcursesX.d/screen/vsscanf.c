#ifdef lint
static char *sccsid = "@(#)vsscanf.c	4.1	(ULTRIX)	7/2/90";
#endif lint

#include <stdio.h>
#include <varargs.h>

/*
 *	This routine implements vsscanf (nonportably) until such time
 *	as one is available in the system (if ever).
 */

vsscanf(buf, fmt, ap)
char	*buf;
char	*fmt;
va_list	ap;
{
	FILE	junk;

	junk._flag = _IOREAD;
	junk._file = -1;
	junk._base = junk._ptr = buf;
	junk._cnt = strlen(buf);
	return _doscan(&junk, fmt, ap);
}
