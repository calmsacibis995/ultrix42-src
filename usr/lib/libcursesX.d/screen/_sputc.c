#ifdef lint
static char *sccsid = "@(#)_sputc.c	4.1	(ULTRIX)	7/2/90";
#endif lint

/*
 * 1/26/81 (Berkeley) @(#)standout.c	1.1
 */

# include	"curses.ext"

int _fix1;      /* curses fix to stop ar complaining */
#ifdef DEBUG
_sputc(c, f)
chtype c;
FILE *f;
{
	int so;

	so = c & A_ATTRIBUTES;
	c &= A_CHARTEXT;
	if (so) {
		putc('<', f);
		fprintf(f, "%o,", so);
	}
	putc(c, f);
	if (so)
		putc('>', f);
}
#endif
