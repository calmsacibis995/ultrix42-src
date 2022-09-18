#ifdef lint
static char *sccsid = "@(#)_comphash.c	4.1	(ULTRIX)	7/2/90";
#endif lint


#include "curses.ext"

_comphash (p)
register struct line   *p;
{
	register chtype  *c, *l;
	register int rc;

	if (p == NULL)
		return;
	if (p -> hash)
		return;
	rc = 1;
	c = p -> body;
	l = &p -> body[p -> length];
	while (--l > c && *l == ' ')
		;
	while (c <= l && *c == ' ')
		c++;
	p -> bodylen = l - c + 1;
	while (c <= l)
		rc = (rc<<1) + *c++;
	p -> hash = rc;
	if (p->hash == 0)
		p->hash = 123;
}
