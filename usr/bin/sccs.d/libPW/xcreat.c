#ifndef lint
static	char	*sccsid = "@(#)xcreat.c	4.1	(ULTRIX)	7/17/90";
#endif lint

# include	"sys/types.h"
# include	"macros.h"

/*
	"Sensible" creat: write permission in directory is required in
	all cases, and created file is guaranteed to have specified mode
	and be owned by effective user.
	(It does this by first unlinking the file to be created.)
	Returns file descriptor on success,
	fatal() on failure.
*/

xcreat(name,mode)
char *name;
int mode;
{
	register int fd;
	register char *d;
        extern void     free();         /* DAG -- added */
        extern char     *malloc();      /* DAG -- added */

	d = (char *)malloc(size(name));
	copy(name,d);
	if (!exists(dname(d))) {
		sprintf(Error,"directory `%s' nonexistent (ut1)",d);
		free(d);
		fatal(Error);
	}
	free(d);
	unlink(name);
	if ((fd = creat(name,mode)) >= 0)
		return(fd);
	return(xmsg(name,"xcreat"));
}
