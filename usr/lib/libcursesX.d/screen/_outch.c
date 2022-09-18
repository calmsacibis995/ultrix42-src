#ifdef lint
static char *sccsid = "@(#)_outch.c	4.1	(ULTRIX)	7/2/90";
#endif lint

/*
 * This routine is one of the main things
 * in this level of curses that depends on the outside
 * environment.
 */
#include "curses.ext"

int outchcount;

/*
 * Write out one character to the tty.
 */
_outch (c)
chtype c;
{
#ifdef DEBUG
# ifndef LONGDEBUG
	if (outf)
		if (c < ' ')
			fprintf(outf, "^%c", (c+'@')&A_CHARTEXT);
		else
			fprintf(outf, "%c", c&A_CHARTEXT);
# else LONGDEBUG
	if(outf) fprintf(outf, "_outch: char '%s' term %x file %x=%d\n",
		unctrl(c&A_CHARTEXT), SP, SP->term_file, fileno(SP->term_file));
# endif LONGDEBUG
#endif DEBUG

	outchcount++;
	if (SP && SP->term_file)
		putc (c&A_CHARTEXT, SP->term_file);
	else
		putc (c&A_CHARTEXT, stdout);
}
