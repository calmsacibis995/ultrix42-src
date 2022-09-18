#ifndef lint
static char *sccsid = "@(#)newterm.c	4.1	(ULTRIX)	7/2/90";
#endif lint


/************************************************************************
 *									*
 *			Copyright (c) 1988, 1990 by			*
 *		Digital Equipment Corporation, Maynard, MA		*
 *			All rights reserved.				*
 *									*
 *   This software is furnished under a license and may be used and	*
 *   copied  only  in accordance with the terms of such license and	*
 *   with the  inclusion  of  the  above  copyright  notice.   This	*
 *   software  or  any  other copies thereof may not be provided or	*
 *   otherwise made available to any other person.  No title to and	*
 *   ownership of the software is hereby transferred.			*
 *									*
 *   This software is  derived  from  software  received  from  the	*
 *   University    of   California,   Berkeley,   and   from   Bell	*
 *   Laboratories.  Use, duplication, or disclosure is  subject  to	*
 *   restrictions  under  license  agreements  with  University  of	*
 *   California and with AT&T.						*
 *									*
 *   The information in this software is subject to change  without	*
 *   notice  and should not be construed as a commitment by Digital	*
 *   Equipment Corporation.						*
 *									*
 *   Digital assumes no responsibility for the use  or  reliability	*
 *   of its software on equipment which is not supplied by Digital.	*
 *									*
 ************************************************************************/
 
/*
 * Modification History
 *
 * 02/07/90 GWS	replaced sgttyb / tty(4)-style terminal I/O with termio(4)-
 *		 style terminal I/O.  The replacement code is the formerly
 *		 conditionalized "USG" code.
 *
 */

# include	"curses.ext"
# include	<signal.h>

char	*calloc();
char	*malloc();
extern	char	*getenv();

extern	WINDOW	*makenew();

struct screen *
newterm(type, outfd, infd)
char *type;
FILE *outfd, *infd;
{
	int		_tstp();
	struct screen *scp;
	struct screen *_new_tty();
	extern int _endwin;

#ifdef DEBUG
	if(outf) fprintf(outf, "NEWTERM() isatty(2) %d, getenv %s\n",
		isatty(2), getenv("TERM"));
# endif
	SP = (struct screen *) calloc(1, sizeof (struct screen));
	SP->term_file = outfd;
	SP->input_file = infd;
	/*
	 * The default is echo, for upward compatibility, but we do
	 * all echoing in curses to avoid problems with the tty driver
	 * echoing things during critical sections.
	 */
	SP->fl_echoit = 1;
	savetty();
	scp = _new_tty(type, outfd);
	if (scp == NULL)
		return NULL;
	(cur_term->Nttyb).c_lflag &= ~ECHO;
	reset_prog_mode();
# ifdef SIGTSTP
	signal(SIGTSTP, _tstp);
# endif
	if (curscr != NULL) {
# ifdef DEBUG
		if(outf) fprintf(outf, "INITSCR: non null curscr = 0%o\n", curscr);
# endif
	}
# ifdef DEBUG
	if(outf) fprintf(outf, "LINES = %d, COLS = %d\n", LINES, COLS);
# endif
	LINES =	lines;
	COLS =	columns;
	curscr = makenew(LINES, COLS, 0, 0);
	stdscr = newwin(LINES, COLS, 0, 0);
# ifdef DEBUG
	if(outf) fprintf(outf, "SP %x, stdscr %x, curscr %x\n", SP, stdscr, curscr);
# endif
	SP->std_scr = stdscr;
	SP->cur_scr = curscr;
	/* Maybe should use makewin and glue _y's to DesiredScreen. */
	_endwin = FALSE;
	return scp;
}
