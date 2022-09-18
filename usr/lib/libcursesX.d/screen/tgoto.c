#ifdef lint
static char *sccsid = "@(#)tgoto.c	4.1	(ULTRIX)	7/2/90";
#endif lint

/*
 * tgoto: function included only for upward compatibility with old termcap
 * library.  Assumes exactly two parameters in the wrong order.
 */
char *
tgoto(cap, col, row)
char *cap;
int col, row;
{
	char *cp;
	char *tparm();

	cp = tparm(cap, row, col);
	return cp;
}
