

#ifndef lint
static	char	*sccsid = "@(#)xopen.c	4.1	(ULTRIX)	7/17/90";
#endif lint

/*
	Interface to open(II) which differentiates among the various
	open errors.
	Returns file descriptor on success,
	fatal() on failure.
*/

# include "errno.h"

xopen(name,mode)
char name[];
int mode;
{
	register int fd;
	extern int errno;
	extern char Error[];

	if ((fd = open(name,mode)) < 0) {
		if(errno == EACCES) {
			if(mode == 0)
				sprintf(Error,"`%s' unreadable (ut5)",name);
			else if(mode == 1)
				sprintf(Error,"`%s' unwritable (ut6)",name);
			else
				sprintf(Error,"`%s' unreadable or unwritable (ut7)",name);
			fd = fatal(Error);
		}
		else
			fd = xmsg(name,"xopen");
	}
	return(fd);
}
