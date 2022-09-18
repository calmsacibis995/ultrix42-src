#ifdef CSHEDIT
#ifndef lint
static  char    *sccsid = "@(#)sh.editglue.c	4.3  (ULTRIX)        12/20/90";
#endif lint
/************************************************************************
 *									*
 *			Copyright (c) 1986 by				*
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
 * Modification History:	sh.editglue.c
 *
 *  This file contains the "glue" between the csh and the edit facility.
 *  In editcmd() the global "cmdline" buffer is filled with the
 *     command that "word0" (from csh proper) points to.  Then editcmd calls
 *     editmain() in sh.edit.c where the real edit code is.
 *   In editword() words are obtained from "cmdline" buffer,
 *      rather than from tty input.
 *
 * 008 - Bob Fontaine - Mon Oct 22 10:51:40 EDT 1990
 *       Remove # as a metacharacter because if it is entered from command
 *       line edit mode on a line by itself it will cause the c shell
 *       to grow and consume all memory available.  Thanks to Akira Tanaka
 *       (JRD).  Fixes QAR #5472.
 *
 * 007 - Bob Fontaine - Fri Jun 22 09:53:01 EDT 1990
 *	 Changed call of internal printf routine to csh_printf to avoid
 *	 conflict with stdio routine.
 *
 * 006 - Gary A. Gaudet - Tue Mar 13 17:27:37 EST 1990
 *	suspend now works after using interactive history edit
 * 005 - Gary A. Gaudet - Tue Jan  2 10:57:32 EST 1990
 *	Removed unused variable dolflg
 *	Removed unused label casebksl
 * 004 11-Nov-88	afd
 *	Use VAX version of editword() (as opposed to modified version of mips
 *	word routine).  This fixes "&" and "|" bug.
 *
 * 003 24-Sep-88	afd
 *	Ported to PMAX:
 *	sigignore(SIGxxx) => omask = sigblock(sigmask(SIGxxx)
 *	sigirelse(SIGxxx) => sigsetmask(omask)
 *	editword() replaced so it matches mips word() routine.
 *
 * 002 23-Jul-87	afd
 *	Added history search capability:
 *	Added a flag paramter to reedit() which tells if we want to search
 *	(instead of scroll one) in the history list.
 *	Added a little code to reedit() to search and compare in history list.
 *
 * 001 28-Nov-86	afd
 *	Created this file for command line edit capability in csh.
 *	This file contains the routines necessary to start up the command line
 *	edit.  The code here interfaces between csh and edit code in sh.edit.c.
 *
 *	The edit mode is invoked on a history command by using
 *	the ":v" modifier.
 */

#include "sh.h"
#include <sys/file.h>

#define SRCHLEN 78		/* also in sh.edit.c */

char *getenv();
char *strrchr();
char *cmdptr;			/* ptr into new cmd line */
char cmdline[BUFSIZ];		/* string for command line */
char srchcmd[SRCHLEN];		/* cmd to search for in history list */
int  emacs;			/* set true if emacs environ wanted */
int  editevent;			/* event number to edit */

editcmd (word0)
    	struct wordent *word0;
{
	register struct wordent *wp;
	char *editor;		/* EDITOR from environment */
	char *base;		/* base name of EDITOR */
	int omask;		/* old signal mask */

	/*
	 * Pick up all of the words of the command line
	 * & put them into our command line buffer
	 */
	wp = word0->next;
	cmdline[0] = '\0';
	for (;;) {
		strcat(cmdline, wp->word);
		wp = wp->next;
		if (wp == word0 || (!strcmp (wp->word, "\n")))
			break;
		strcat(cmdline, " ");
	}

	/*
	 * Get user's editor from environment (default to vi)
	 */
	if ((editor = getenv("CSHEDIT")) != NULL)
		if (eq(editor, "emacs") || eq(editor, "EMACS"))
			emacs = 1;
		else	/* could explicitely check for vi */
			emacs = 0;
	else	/* no CSHEDIT given so try "EDITOR" */
		if ((editor = getenv("EDITOR")) == NULL)
			emacs = 0;
		else	{
			base = strrchr(editor,'/');
			if (base != 0)
				base++;
			else
				base = editor;
			if (eq(base, "vi") ||
			    eq(base, "ex") ||
			    eq(base, "ed") ||
			    eq(base, "edit"))
				emacs = 0;
			else
				emacs = 1;
		}
	/*
	 * Ignore signals: interrupt, quit, tty stop
	 * 006 - GAG
	 */
	omask = sigblock(sigmask(SIGINT) |sigmask (SIGQUIT) |sigmask (SIGTSTP));

	/*
	 * Now free old resources, set "editevent" to last event in
	 * the history list & call the main edit routine.
	 */
	freelex(&paraml);
	if (emacs)
		strcat(cmdline, " ");
	editevent = lastev;
	editmain();

	if (emacs && cmdline[strlen(cmdline)-1] == ' ') {
		cmdline[strlen(cmdline)-1] = '\0';	/* remove blank */
	}
	/*
	 * Restore signals
	 */
	(void) sigsetmask(omask);

	/*
	 * Need a line feed at end of command for csh.
	 * Plus we need to echo a newline now.
	 */
	strcat(cmdline, "\n");
	cmdptr = cmdline;
	csh_printf("\n");			/* 007 RNF */
}

/*
 * Get new command line to edit from history list
 * when up-arrow or down-arrow is typed.
 */
reedit(srch)
	int srch;	/* true if search cmd, else uparrow/down arrow */
{
	register struct wordent *word0, *wp;
	register struct Hist *hp;
	register int tmpevent;
	int foundit;
	/*
	 * Get history info
	 */
	for (hp = Histlist.Hnext; hp; hp = hp->Hnext)
		if (hp->Hnum == editevent) {
			word0 = &hp->Hlex;
			break;
		}
	if (srch) {
		tmpevent = editevent;
		for (foundit = 0; hp; hp = hp->Hnext, tmpevent--) {
			word0 = &hp->Hlex;
			for (wp = word0->next; ;) {
				if (!strcmp (wp->word, srchcmd)) {
					foundit = 1;
					break;
				}
				wp = wp->next;
				if (wp == word0 || (!strcmp (wp->word, "\n")))
					break;
			}
			if (foundit == 1)
				break;
		}
		if (foundit == 0)
			return(0);
		else {
			editevent = tmpevent;
			word0 = &hp->Hlex;
		}
	}
	/*
	 * Pick up all of the words of the command line
	 * & put them into our command line buffer (if we found the line).
	 */
	if (hp) {
		wp = word0->next;
		cmdline[0] = '\0';
		for (;;) {
			strcat(cmdline, wp->word);
			wp = wp->next;
			if (wp == word0 || (!strcmp (wp->word, "\n")))
				break;
			strcat(cmdline, " ");
		}
		if (emacs)
			strcat(cmdline, " ");
		return(1);
	}
	return(0);
}

char *WORDMETA = " '`\"\t;&<>()|\n";		/* 008 RNF */

/*
 * Like word() (in sh.lex.c) but gets input from "cmdline" (cmdptr),
 * the edited command line.
 *
 * Note: This is the editword routine is derived from the VAX csh version 
 * 	of word(), not the mips csh version of word().  The editword derived
 *	from the mips version of word() had a couple of bugs in it.
 *	1. editing a cmd line and adding an '&', caused the '&' to expand
 *	   into environment junk (like the termcap entry).
 *	2. editing a cmd line and adding "|something" expanded to "|s omething"
 */
char *
editword()
{
	register char c, c1;
	register char *wp;
	char wbuf[BUFSIZ];
	register int i;

	wp = wbuf;
	i = BUFSIZ - 4;
loop:
	c = *cmdptr++;
	switch (c) {

	case ' ':
	case '\t':
		goto loop;

	case '`':
	case '\'':
	case '"':
		*wp++ = c, --i, c1 = c;
		/*dolflg = */c == '"' ? DOALL : DOEXCL;
		for (;;) {
			c = *cmdptr++;
			if (c == c1)
				break;
			if (c == '\n') {
				seterrc("Unmatched ", c1);
				cmdptr--;
				goto ret;
			}
			if (c == '\\') {
				c = *cmdptr++;
				if (c == HIST)
					c |= QUOTE;
				else {
					if (c == '\n' && c1 != '`')
						c |= QUOTE;
					cmdptr--, c = '\\';
				}
			}
			if (--i <= 0)
				goto toochars;
			*wp++ = c;
		}
		*wp++ = c, --i;
		goto pack;

	case '&':
	case '|':
	case '<':
	case '>':
		*wp++ = c;
		c1 = *cmdptr++;
		if (c1 == c)
			*wp++ = c1;
		else
			cmdptr--;
		goto ret;

	case '#':
		if (intty)
			break;
		if (wp != wbuf) {
			cmdptr--;
			goto ret;
		}
		c = 0;
		do {
			c1 = c;
			c = *cmdptr++;
		} while (c != '\n');
		if (c1 == '\\')
			goto loop;
		/* fall into ... */

	case ';':
	case '(':
	case ')':
	case '\n':
		*wp++ = c;
		goto ret;

	case '\\':
		c = *cmdptr++;
		if (c == '\n') {
			if (onelflg == 1)
				onelflg = 2;
			goto loop;
		}
		if (c != HIST)
			*wp++ = '\\', --i;
		c |= QUOTE;
		break;
	}
	cmdptr--;
pack:
	for (;;) {
		c = *cmdptr++;
		if (c == '\\') {
			c = *cmdptr++;
			if (c == '\n') {
				if (onelflg == 1)
					onelflg = 2;
				goto ret;
			}
			if (c != HIST)
				*wp++ = '\\', --i;
			c |= QUOTE;
		}
		if (any(c, WORDMETA) && intty) {
			cmdptr--;
			if (any(c, "\"'`"))
				goto loop;
			goto ret;
		}
		if (--i <= 0)
			goto toochars;
		*wp++ = c;
	}
toochars:
	seterr("Word too long");
	wp = &wbuf[1];
ret:
	*wp = 0;
	return (savestr(wbuf));
}
#endif
