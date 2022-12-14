#ifdef lint
static char *sccsid = "@(#)line_alloc.c	4.1	(ULTRIX)	7/2/90";
#endif lint


#include "curses.ext"

/*
 * _line_alloc returns a pointer to a new line structure.
 */
struct line *
_line_alloc ()
{
	register struct line   *rv = SP->freelist;
	char *calloc();

#ifdef DEBUG
	if(outf) fprintf(outf, "mem: _line_alloc (), prev SP->freelist %x\n", SP->freelist);
#endif
	if (rv) {
		SP->freelist = rv -> next;
	} else {
#ifdef NONSTANDARD
		_ec_quit("No lines available in line_alloc", "");
#else
		rv = (struct line *) calloc (1, sizeof *rv);
		rv -> body = (chtype *) calloc (columns, sizeof (chtype));
#endif
	}
	rv -> length = 0;
	rv -> hash = 0;
	return rv;
}
