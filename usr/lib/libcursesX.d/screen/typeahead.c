#ifdef lint
static char *sccsid = "@(#)typeahead.c	4.1	(ULTRIX)	7/2/90";
#endif lint

#include "curses.ext"

/*
 * Set the file descriptor for typeahead checks to fd.  fd can be -1
 * to disable the checking.
 */
typeahead(fd)
int fd;
{
	SP->check_fd = fd;
}
