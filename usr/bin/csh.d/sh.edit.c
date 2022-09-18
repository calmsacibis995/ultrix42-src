#ifdef CSHEDIT
#ifndef lint
static	char	*sccsid = "@(#)sh.edit.c	4.2	(ULTRIX)	12/20/90";
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
 * Modification History:	sh.edit.c
 *
 * Edit routines to edit a command line buffer.  The lower level routines
 * are adapted from ex/vi code.  The Top level command processors for both
 * vi style and emacs style editing are original.
 *
 * 009 - Bob Fontaine - Mon Oct 22 10:51:40 EDT 1990
 *      Prevent csh from hanging when cntrl-\ is typed in emacs editing mode.
 *      Thanks to Akira Tanaka (JRD).  Fixes QAR 5522.
 *
 * 008 - Bob Fontaine - Mon Oct 22 10:51:40 EDT 1990
 *      Fix expansion of wild card characters after command line edit mode
 *      has been entered.  Thanks to Akri Tanaka (JRD).  Fixes qar 5264.
 *
 * 007 - Gary A. Gaudet - Tue Feb 27 10:17:01 EST 1990
 *	Fixed <esc>" infinite loop problem
 *
 * 006 - Gary A. Gaudet - Thu Dec 28 17:49:45 EST 1989
 *	Removed unused variables: oglobp, holdcurs. arg, ret, i, col,
 *		cnt, escape, holdcurs, x, d, colp, l, c
 *	Modified return values.
 *	Removed unused label: egetcount, bakchar, again
 *	Added test for ioctl() return value.
 *
 * 005 - 26-Sep-88	afd
 *	Added ^P/^N to scroll up/down thru the history list.
 *
 * 004 - 22-Dec-87	afd
 *	Made the edit code "8 bit clean".  We never set nor clear the 8th bit.
 *
 * 003 - 23-Jul-87	afd
 *	Added history search capability: to vmain() and emain()
 *	command parsers and added srchback() routine to get search string.
 *	Made redraw into a function which is called in several places.
 *	Added "insearch" flag to keep track of when we are in emacs
 *	"search mode" to repeat history searches.
 *
 * 002 - 15-Jul-87	afd
 *	Fixed repeat count bug on 'r', "insert", "append" and "put" commands by 
 *		NOT using "printf" in vappend().  printf is the shell's version
 *		and it prints to a buffer which is flushed later.
 *	Set DEL and INS buffers to 0 in init_globals before editing a cmd line.
 *	Set lastcmd buffer to 0 in init_globals before editing a cmd line.
 *
 * 001 - 28-Nov-86	afd
 *	Created this file for command line edit capability in csh.
 */

#include "sh.edit.h"
#define SRCHLEN 78		/* also in sh.editglue.c */

int vdebug = 0;
#define printd if (vdebug) printf

extern short SHIN, SHOUT, SHDIAG;	 /* csh I/O file descriptors */
extern char srchcmd[];		/* cmd to search for in history list */
/*
 * The routines outchar and putchar and are actually variables,
 * and these variables point at the current definitions of the routines.
 * Outchar and Putchar will be set to vputchar, vinschar, etc.
 */
int	(*Outchar)() = vputchar;
int	(*Putchar)() = normchar;

/*
 * Main edit procedure.
 * Transfers control to the main command processing loop, vmain().
 */
editmain()
{
	register char *cp;

	/*
	 * Set up terminal environment.
	 * The tty will be used as a "dumb" terminal if TERM is not
	 *   defined in environment or if its type is not in termcap file.
	 */
	gettmode();
	if ((cp = getenv("TERM")) != 0 && *cp) {
		setterm(cp);
	}
	/*
	 * Do global initializations, call tty setup routine
	 * to get tty into raw mode, call main command parser to edit the line,
	 * then restore the tty.
	 */
	if (COLUMNS <= 0)
		COLUMNS = TUBECOLS;	/* couldn't get col size so use 80 */
	if (AM)
		COLUMNS--;
	vi_tty_setup(1);
	init_globals();
	if (emacs)
		emain();
	else
		vmain();
	CP(cmdline, linebuf);
	vi_tty_setup(0);
}

/*
 * This is the main routine for vi interface command decoding.
 * We here decode the count preceding a command
 * and interpret a few of the commands.
 * Commands which involve a target (i.e. an operator) are decoded
 * in the routine exoperate.
 */

#define	forbid(a)	{ if (a) goto fonfon; }

vmain()
{
	register int c, cnt;

	/*
	 * The line is in the line buffer linebuf,
	 * and the cursor at the position cursor.
	 */
	done_edit = 0;
	for (;;) {
		/*
		 * Decode a command.
		 * Clear state for decoding of next command.
		 */
		vglobp = 0;
		wcursor = 0;
		Xhadcnt = hadcnt = 0;
		Xcnt = cnt = 1;

		/*
		 * Gobble up counts.
		 */
		for (;;) {
looptop:
			if (isdigit(peekkey()) && peekkey() != '0') {
				hadcnt = 1;
				cnt = vgetcnt();
				forbid (cnt <= 0);
			}
			break;
		}
reread:
		/*
		 * Come to reread from below after some macro expansions.
		 * The call to map allows use of function key pads
		 * by performing a terminal dependent mapping of inputs.
		 */
		op = getkey();
		maphopcnt = 0;
		do {
			/*
			 * Keep mapping the char as long as it changes.
			 * This allows for double mappings, e.g., q to #,
			 * #1 to something else.
			 */
			c = op;
			op = map(c,arrows);
			/*
			 * Maybe the mapped to char is a count. If so, we have
			 * to go back to the "for" to interpret it.
			 */
			if (isdigit(c) && c!='0') { /* 007 - GAG */
				ungetkey(c);
				goto looptop;
			}
			if (++maphopcnt > 256)
				error("Infinite macro loop");
		} while (c != op);

		/*
		 * Begin to build an image of this cmd in the buffer workcmd.
		 */
		lastcp = workcmd;
		if (!vglobp)
			*lastcp++ = c;
		/*
		 * First level command decode.
		 */
		switch (c) {

		/*
		 * ^C		Quit: with no command line.
		 */
		case CTRL(c):
			linebuf[0] = '\0';
			destcol = 0;
			vclreol();
			return;
		/*
		 * search	Edit some prior history event
		 *		Search for a "word" delineated by white space.
		 */
		case '/':
			exputchar('\/');
			exflush();
			if (srchback()) {
				/*
				 * Got an input string from user
				 * If pattern found in history list
				 *    edit new line.
				 * else (pattern not found) restore state.
				 */
				if (reedit(1))
					goto newedit;
				else {
					beep();
					editevent++;
					redraw();
					continue;
				}
			} else
				continue;  /* user aborted command */
		/*
		 * n		repeat last history search command (/).
		 */
		case 'n':
			if (srchcmd[0] != '\0') {
				/*
				 * Have a search string
				 * If pattern found in history list
				 *    edit new line.
				 * else (pattern not found) restore state.
				 */
				editevent--;
				if (reedit(1))
					goto newedit;
				else {
					beep();
					editevent++;
					/* redraw(); */
					continue;
				}
			} else {
				beep();
				continue;  /* no search string */
			}
		/*
		 * uparrow	Edit prior history event
		 */
		case 'k':
		case CTRL(p):
			editevent--;
			if (reedit(0))
				goto newedit;
			else {	/* ran off top of history, so keep old line */
				editevent++;
				beep();
				continue;
			}
		/*
		 * downarrow	Edit next history event
		 */
		case 'j':
		case CTRL(n):
			editevent++;
			if (reedit(0) <= 0) {
				/* ran off botm of history, so keep old line */
				editevent--;
				beep();
				continue;
			}
newedit:
			/*
			 * Reset edit state for new command line
			 */
			vsetcurs(linebuf);
			init_globals();
			continue;
		/*
		 * ^L,^R	Redraw the line.
		 */
		case CTRL(l):
		case CTRL(r):
			redraw();
			continue;
		/*
		 * .		Repeat the last (modifying) command.
		 */
		case '.':
			/*
			 * Check that there was a last command, and
			 * take its count.
			 */
			forbid (lastcmd[0] == 0);
			if (hadcnt)
				lastcnt = cnt;
			cnt = lastcnt;
			hadcnt = lasthad;
			vglobp = lastcmd;
			goto reread;
		/*
		 * $		Escape just cancels the current command
		 *		with a little feedback.
		 */
		case ESCAPE:
			beep();
			continue;
		/*
		 * C		Change text to end of line, short for c$.
		 */
		case 'C':
			if (*cursor) {
				ungetkey('$'), c = 'c';
				break;
			}
			goto appnd;
		/*
		 * ~	Switch case of letter under cursor
		 */
		case '~':
			/*
			 * Save command for repeat cmd.
			 * Save old line state for undo.
			 */
			setLAST();
			CP(oldbuf, linebuf);
			oldcursor = cursor;
			while (cnt > 0) {
				cnt--;
				if (isalpha(*cursor))
					*cursor ^= ' ';	/* toggle the case */
				exputchar(*cursor);
				cursor++;
				if (*cursor == '\0') {
					cursor--;
					vsetcurs(cursor);
					break;
				}
			}
			continue;
		/*
		 * A		Append at end of line, short for $a.
		 */
		case 'A':
			exoperate('$', 1);
appnd:
			c = 'a';
			/* fall into ... */
		/*
		 * a		Appends text after cursor.  Text can continue
		 *		through arbitrary number of lines.
		 */
		case 'a':
			if (*cursor) {
				cursor++;
			}
			goto insrt;
		/*
		 * I		Insert at beginning of whitespace of line,
		 *		short for ^i.
		 */
		case 'I':
			exoperate('0', 1);
			c = 'i';
			/* fall into ... */
		/*
		 * i		Insert text to an escape in the buffer.
		 *		Text is arbitrary.
		 */
		case 'i':
insrt:
			/*
			 * Common code for all the insertion commands.
			 * Save command for repeat cmd. Save old line state
			 * for undo.  Position cursor and do append.
			 * Note that nothing is doomed.
			 * If NL or CR entered in insert mode (vgetline()),
			 * we end insert AND end entire edit with "done_edit"
			 * flag.  So flush output buffer and end edit. (flusho
			 * is normally called in "getbr()" before getting a char)
			 */
			setLAST();
			CP(oldbuf, linebuf);
			oldcursor = cursor;
			vgoto(column(cursor), 0);
			doomed = 0;
			vappend(c, cnt, 0);
			if (done_edit) {
				flusho();
				return;		/* 006 - GAG */
			}
			continue;
		/*
		 * ZZ, NL, CR	end edit so cmd can be executed.
		 */
		 case 'Z':
			forbid(getkey() != 'Z');
		 case NL:
		 case CR:
			globp = "x";
			return;		/* 006 - GAG */
		/*
		 * P		Put back text before cursor.
		 *
		 * p		Like P but after rather than before cursor.
		 */
		case 'P':
		case 'p':
			/*
			 * Save command for repeat cmd. Save old line state
			 * for undo.
			 * Use an append or insert to put it back so as to
			 * use insert mode.
			 */
			setLAST();
			CP(oldbuf, linebuf);
			oldcursor = cursor;
			if (DEL[0]) {
				forbid (DEL[0] == NULL);
				vglobp = DEL;
				ungetkey(c == 'p' ? 'a' : 'i');
				goto reread;
			}
			beep();
			continue;
		/*
		 * u		undo last change (but won't undo an undo)
		 */
		case 'u':
			CP(linebuf, oldbuf);
			/*
			 * Redraw the line
			 */
			vsetcurs(linebuf);
			normline();
			/*
			 * Clear from last char to end of line
			 */
			destcol = column(strend(linebuf)) + 1;
			vclreol();
			/*
			 * Reset cursor
			 */
			cursor = oldcursor;
			if (*cursor == '\0')
				cursor--;
			vsetcurs(cursor);
			continue;
		/*
		 * U		undo: restore current line to initial state.
		 */
		case 'U':
			CP(linebuf, cmdline);
			/*
			 * Redraw the line
			 */
			vsetcurs(linebuf);
			normline();
			/*
			 * Clear from last char to end of line
			 */
			destcol = column(strend(linebuf)) + 1;
			vclreol();
			/*
			 * Reset cursor to begining of line
			 */
			vsetcurs(linebuf);
			continue;
fonfon:
			beep();
			vmacp = 0;
			continue;
		} /* end switch (c) */

		/*
		 * Rest of commands are decoded by the exoperate
		 * routine.
		 *
		 * If "done_edit" got set in vgetline (during change)
		 * then flush output buffer and end edit. (flusho is
		 * normally called in "getbr()" before getting a char)
		 */
		exoperate(c, cnt);
		if (done_edit) {
			flusho();
			break;
		}
	}
	return;
}

#define	blank()		isspace(wcursor[0])
#undef	forbid
#define	forbid(a)	if (a) goto errlab;

/*
 * Decode an operator/operand type command.
 * The work here is setting up a function variable to point
 * to the routine we want, and manipulation of the variable
 * wcursor, which marks the other end of the affected
 * area.
 */
exoperate(c, cnt)
	register int c, cnt;
{
	register int i;
	int (*moveop)(), (*deleteop)();
	register int (*opf)();
	bool subop = 0;
	static char lastFKND, lastFCHR;

	moveop = vmove, deleteop = vdelete;
	wcursor = cursor;
	dir = 1;
	switch (c) {

	/*
	 * d		delete operator.
	 */
	case 'd':
		CP(oldbuf, linebuf);
		oldcursor = cursor;
		moveop = vdelete;
		deleteop = beep;
		break;

	/*
	 * c		Change operator.
	 */
	case 'c':
		CP(oldbuf, linebuf);
		oldcursor = cursor;
		if (c == 'c' && workcmd[0] == 'C' || workcmd[0] == 'S')
			subop++;
		moveop = vchange;
		deleteop = beep;
		break;

	/*
	 * r		Replace character under cursor with single following
	 *		character.
	 */
	case 'r':
		CP(oldbuf, linebuf);
		oldcursor = cursor;
		vrep(cnt);
		return;

	default:
		goto nocount;
	}
	/*
	 * Had an operator, so accept another count.
	 * Multiply counts together.
	 */
	if (isdigit(peekkey()) && peekkey() != '0') {
		cnt *= vgetcnt();
		Xcnt = cnt;
		forbid (cnt <= 0);
	}

	/*
	 * Get next character, mapping it and saving it.
	 */
	c = map(getesc(),arrows);
	if (c == 0)
		return;
	if (!subop)
		*lastcp++ = c;
nocount:
	opf = moveop;
	switch (c) {

	/*
	 * b		Back up a word.
	 * B		Back up a word, liberal definition.
	 */
	case 'b':
	case 'B':
		dir = -1;
		/* fall into ... */
	/*
	 * w		Forward a word.
	 * W		Forward a word, liberal definition.
	 */
	case 'W':
	case 'w':
		wdkind = c & ' ';
		forbid(lfind(2, cnt, opf) < 0);
		break;
	/*
	 * E		to end of following blank/nonblank word
	 */
	case 'E':
		wdkind = 0;
		goto ein;
	/*
	 * e		To end of following word.
	 */
	case 'e':
		wdkind = 1;
ein:
		forbid(lfind(3, cnt - 1, opf) < 0);
		break;
	/*
	 * %		To matching (), [], or {}.  If not at one scan for
	 *		first such after cursor on this line.
	 */
	case '%':
		i = lmatchp();
		forbid(!i);
		if (opf != vmove)
			if (dir > 0)
				wcursor++;
			else
				cursor++;
		break;
	/*
	 * 0		To beginning of real line.
	 */
	case '0':
		wcursor = linebuf;
		break;
	/*
	 * F		Find single character before cursor in current line.
	 * T		Like F, but stops before character.
	 */
	case 'F':	/* inverted find */
	case 'T':
		dir = -1;
		/* fall into ... */
	/*
	 * f		Find single character following cursor in current line.
	 * t		Like f, but stop before character.
	 */
	case 'f':	/* find */
	case 't':
		if (!subop) {
			i = getesc();
			if (i == 0)
				return;
			*lastcp++ = i;
		}
		if (vglobp == 0)
			lastFKND = c, lastFCHR = i;
		for (; cnt > 0; cnt--)
			forbid (find(i) == 0);
		switch (c) {

		case 'T':
			wcursor++;
			break;

		case 't':
			wcursor--;
		case 'f':
fixup:
			if (moveop != vmove)
				wcursor++;
			break;
		}
		break;
	/*
	 * $		To end of line.
	 */
	case '$':
		if (cnt > 1)
			cnt = 1;
		if (linebuf[0]) {
			wcursor = strend(linebuf) - 1;
			goto fixup;
		}
		wcursor = linebuf;
		break;
	/*
	 * h		Back a character.
	 * ^H		Back a character.
	 */
	case 'h':
	case CTRL(h):
		dir = -1;
		/* fall into ... */
	/*
	 * space	Forward a character.
	 */
	case 'l':
	case ' ':
		forbid (margin() || opf == vmove && edge());
		while (cnt > 0 && !margin())
			wcursor += dir, cnt--;
		if (margin() && opf == vmove || wcursor < linebuf)
			wcursor -= dir;
		break;
	/*
	 * D		Delete to end of line, short for d$.
	 */
	case 'D':
		cnt = INF;
		goto deleteit;
	/*
	 * X		Delete character before cursor.
	 */
	case 'X':
		dir = -1;
		/* fall into ... */
deleteit:
	/*
	 * x		Delete character at cursor, leaving cursor where it is.
	 */
	case 'x':
		CP(oldbuf, linebuf);
		oldcursor = cursor;
		if (margin())
			goto errlab;
		while (cnt > 0 && !margin())
			wcursor += dir, cnt--;
		opf = deleteop;
		break;

	default:
		if (opf == vmove || c != workcmd[0]) {
errlab:
			beep();
			return;
		}
	}
	/*
	 * Apply.
	 */
	(*opf)(c);
}

/*
 * This is the main routine for emacs interface command decoding.
 * We here decode the count preceding a command and interpret the commands.
 */

#define	eforbid(a)	{ if (a) goto efonfon; }

emain()
{
	register int c, cnt;
	register int i;
	register int (*opf)();
	char *markcursor;
	int (*moveop)(), (*deleteop)();
	static char lastFKND, lastFCHR;
	char holdchar;
	int insearch;			/* state of emacs search mode */

	/*
	 * The line is in the line buffer linebuf,
	 * and the cursor at the position cursor.
	 */
	insearch = 0;
	for (;;) {
		/*
		 * Decode a command.
		 * Clear state for decoding of next command.
		 */
		vglobp = 0;
		Xhadcnt = hadcnt = 0;
		Xcnt = cnt = 1;

		moveop = vmove, deleteop = vdelete;
		opf = moveop;
		wcursor = cursor;
		dir = 1;
ereread:
		/*
		 * Come to reread from below after some macro expansions.
		 * The call to map allows use of function key pads
		 * by performing a terminal dependent mapping of inputs.
		op = getkey();
		 */
		op = getbr();
		maphopcnt = 0;
		do {
			/*
			 * Keep mapping the char as long as it changes.
			 * This allows for double mappings, e.g., q to #,
			 * #1 to something else.
			 */
			c = op;
			op = map(c,arrows);
			if (++maphopcnt > 256)
				error("Infinite macro loop");
		} while (c != op);

		/*
		 * Begin to build an image of this cmd in the buffer workcmd.
		 */
		lastcp = workcmd;
		if (!vglobp)
			*lastcp++ = c;
		/*
		 * First level command decode.
		 */
		switch (c) {

		/*
		 * ^C		Quit: with no command line.
		 */
		case CTRL(c):
			linebuf[0] = '\0';
			destcol = 0;
			vclreol();
			return;
		/*
		 * NL, CR	end edit so cmd can be executed.
		 */
		 case NL:
		 case CR:
			globp = "x";
			return;		/* 006 - GAG */
		/*
		 * ^U		Repeat count for command.
		 */
		 case CTRL(u):
			if (isdigit(peekkey()) && peekkey() != '0') {
				hadcnt = 1;
				cnt = vgetcnt();
				eforbid (cnt <= 0);
			} else {
				hadcnt = 1;
				cnt = 4;
				Xhadcnt = 1;
				Xcnt = 4;
			};
			goto ereread;
		/*
		 * ^T		Exchange the 2 characters BEFORE the cursor
		 */
		case CTRL(t):
			insearch = 0;
			if (cursor-2 < linebuf) {
				beep();
				continue;
			}
			wcursor = cursor-2;
			holdchar = *wcursor;
			*wcursor++ = *(cursor-1);
			*wcursor = holdchar;
			/* fall into... to redisplay the line */
		/*
		 * ^L		Redraw the line.
		 */
		case CTRL(l):
			redraw();
			continue;
		/*
		 * ^A		To beginning of real line.
		 */
		case CTRL(a):
			insearch = 0;
			wcursor = linebuf;
			(*opf)(c);
			continue;
		/*
		 * ^E		To end of line.
		 */
		case CTRL(e):
			insearch = 0;
			if (cnt > 1)
				cnt = 1;
			if (linebuf[0]) {
				wcursor = strend(linebuf) - 1;
				if (moveop != vmove)
					wcursor++;
				(*opf)(c);
				continue;
			}
			wcursor = linebuf;
			(*opf)(c);
			continue;
		/*
		 * uparrow	Edit prior history event
		 */
		case 'k':
		case CTRL(p):
			insearch = 0;
			if (maphopcnt < 2)
				goto einsrt;	/* no map, its an insert char */
			editevent--;
			if (reedit(0))
				goto enewedit;
			else {	/* ran off top of history, so keep old line */
				editevent++;
				beep();
				continue;
			}
		/*
		 * downarrow	Edit next history event
		 */
		case 'j':
		case CTRL(n):
			insearch = 0;
			if (maphopcnt < 2)
				goto einsrt;	/* no map, its an insert char */
			editevent++;
			if (reedit(0) <= 0) {
				/* ran off botm of history, so keep old line */
				editevent--;
				beep();
				continue;
			}
enewedit:
			/*
			 * Reset edit state for new command line
			 */
			vsetcurs(linebuf);
			init_globals();
			continue;
		/*
		 * <-		Back a character (arrow key mapped to 'h').
		 * ^B		Back a character.
		 */
		case 'h':
			if (maphopcnt < 2)
				goto einsrt;	/* no map, its an insert char */
			/* fall into ... */
		case CTRL(b):
			dir = -1;
			/* fall into ... */
		/*
		 * ->		Frwd a character (arrow key mapped to 'l').
		 * ^F		Forward a character.
		 */
		case 'l':
			if ((maphopcnt < 2) && c != CTRL(b))
				goto einsrt;	/* no map, its an insert char */
			/* fall into ... */
		case CTRL(f):
			insearch = 0;
			eforbid (margin() || opf == vmove && edge());
			while (cnt > 0 && !margin())
				wcursor += dir, cnt--;
			if (margin() && opf == vmove || wcursor < linebuf)
				wcursor -= dir;
			(*opf)(c);
			continue;
		/*
		 * ^K		Delete to end of line, (vi: d$).
		 */
		case CTRL(k):
			insearch = 0;
			cnt = INF;
			goto edeleteit;
		/*
		 * ^H,DEL	Delete character before cursor.
		 */
		case CTRL(h):
		case DELETE:
			dir = -1;
			/* fall into ... */
edeleteit:
		/*
		 * ^D		Delete char at cursor, leaving cursor where it is.
		 */
		case CTRL(d):
			insearch = 0;
			CP(oldbuf, linebuf);
			oldcursor = cursor;
			if (margin())
				goto efonfon;
			while (cnt > 0 && !margin())
				wcursor += dir, cnt--;
			if (wcursor == strend(linebuf))
				wcursor--;    /* keep blank at eol for emacs */
			opf = deleteop;
			(*opf)(c);
			continue;
		/*
		 * ^R	Find single character backward in current line.
		 */
		case CTRL(r):	/* inverted find */
			dir = -1;
			/* fall into ... */
		/*
		 * ^S	Find single character forward in current line.
		 */
		case CTRL(s):	/* find */
			insearch = 0;
			i = getesc();
			if (i == 0)
				continue;
			*lastcp++ = i;
			if (vglobp == 0)
				lastFKND = c, lastFCHR = i;
			for (; cnt > 0; cnt--)
				eforbid (find(i) == 0);
			(*opf)(c);
			continue;
		/*
		 * ^Y		Put back text before cursor.
		 */
		case CTRL(y):
			/*
			 * Save command for repeat cmd. Save old line state
			 * for undo.
			 * Use an append or insert to put it back so as to
			 * use insert mode.
			 */
			insearch = 0;
			setLAST();
			CP(oldbuf, linebuf);
			oldcursor = cursor;
			if (DEL[0]) {
				eforbid (DEL[0] == NULL);
				vglobp = DEL;
				goto einsrt;
			}
			beep();
			continue;
		/*
		 * ^space		Set mark.
		 */
		case 00:
			markcursor = cursor;
			continue;
		/*
		 * ^W		Delete to mark
		 */
		case CTRL(w):
			insearch = 0;
			if (markcursor < linebuf || markcursor > strend(linebuf)) {
				beep();
				continue;
			}
			wcursor = markcursor;
			opf = deleteop;
			(*opf)(c);
			continue;
		/*
		 * 2 character commands: ESCAPE + something.
		 */
		case ESCAPE:
			insearch = 0;
			/*
			 * Gobble up counts.
			 */
			if (isdigit(peekkey()) && peekkey() != '0') {
				hadcnt = 1;
				cnt = vgetcnt();
				eforbid (cnt <= 0);
				goto ereread;
			}
			c = getkey();
			*lastcp++ = c;
			switch (c) {

			/*
			 * ESC^C	Exit and execute command.
			 */
			case CTRL(c):
				return;
			/*
			 * ESC B	Back word.
			 */
			case 'B':
			case 'b':
				dir = -1;
				/* fall into */
			/*
			 * ESC F	Forward word.
			 */
			case 'F':
			case 'f':
				wdkind = c & ' ';
				eforbid(lfind(2,cnt,opf) < 0);
				(*opf)(c);
				continue;
			/*
			 * ESC H	delete prev word (vi: db)
			 * ESC DEL	delete prev word (vi: db)
			 */
			case 'H':
			case 'h':
			case DELETE:
				dir = -1;
				/* fall into */
			/*
			 * ESC D	delete next word (vi: dw)
			 */
			case 'D':
			case 'd':
				CP(oldbuf, linebuf);
				oldcursor = cursor;
				moveop = vdelete;
				deleteop = beep;
				opf = moveop;
				wdkind = c & ' ';
				eforbid(lfind(2, cnt, opf) < 0);
				if (wcursor == strend(linebuf))
					wcursor--; /* keep blank at eol for emacs */
				(*opf)(c);
				continue;
			/*
			 * 	Abort command (with a little feedback).
			 */
			default:
				beep();
				vmacp = 0;
				continue;
			} /* end switch on char after ESC */
		/*
		 * 2 character commands: ^X + something.
		 */
		case CTRL(x):
			c = getkey();
			*lastcp++ = c;
			switch (c) {

			/*
			 * ^X^C		Exit and execute command.
			 */
			case CTRL(c):
				return;
			/*
			 * search	Edit some prior history event
			 *		Search for a "word" delineated by white space.
			 */
			case CTRL(s):
			    if (insearch) {
				/*
				 * Repeat last history search command.
				 */
				if (srchcmd[0] != '\0') {
					/*
					 * Have a search string
					 * If pattern found in history list
					 *    edit new line.
					 * else (pattern not found) restore state.
					 */
					editevent--;
					if (reedit(1))
						goto enewedit;
					else {
						beep();
						editevent++;
						continue;
					}
				} else {
					insearch = 0;
					beep();
					continue;  /* no search string */
				}
			    } else {
				/*
				 * Get new string for history search command.
				 */
				if (srchback()) {
					/*
					 * Got an input string from user
					 * If pattern found in history list
					 *    edit new line.
					 * else (pattern not found) restore state.
					 */
					insearch = 1;
					if (reedit(1))
						goto enewedit;
					else {
						beep();
						editevent++;
						redraw();
						continue;
					}
				} else
					continue;  /* user aborted command */
			    }
			/*
			 * ^Xu	undo last change (but won't undo an undo)
			 */
			case 'u':
				insearch = 0;
				CP(linebuf, oldbuf);
				/*
				 * Redraw the line
				 */
				vsetcurs(linebuf);
				normline();
				/*
				 * Clear from last char to end of line
				 */
				destcol = column(strend(linebuf)) + 1;
				vclreol();
				/*
				 * Reset cursor
				 */
				cursor = oldcursor;
				if (*cursor == '\0')
					cursor--;
				vsetcurs(cursor);
				continue;
			/*
			 * ^XU	undo: restore current line to initial state.
			 */
			case 'U':
				insearch = 0;
				CP(linebuf, cmdline);
				/*
				 * Redraw the line
				 */
				vsetcurs(linebuf);
				normline();
				/*
				 * Clear from last char to end of line
				 */
				destcol = column(strend(linebuf)) + 1;
				vclreol();
				/*
				 * Reset cursor to begining of line
				 */
				vsetcurs(linebuf);
				continue;
			/*
			 * ^X~	Switch case of letter under cursor
			 */
			case '~':
				/*
				 * Save command for repeat cmd.
				 * Save old line state for undo.
				 */
				insearch = 0;
				setLAST();
				CP(oldbuf, linebuf);
				oldcursor = cursor;
				while (cnt > 0) {
					cnt--;
					if (isalpha(*cursor))
						*cursor ^= ' ';	/* toggle the case */
					exputchar(*cursor);
					cursor++;
					if (*cursor == '\0') {
						cursor--;
						vsetcurs(cursor);
						break;
					}
				}
				continue;
#ifdef notdef
			/*
			 * ^X-ESC	Repeat the last (modifying) command.
			 * This is degenerate in Emacs since there is no
			 *    change or replace mode, and only the last
			 *    single char inserted is saved (no insert mode).
			 */
			case ESCAPE:
				/*
				 * Check that there was a last command, and
				 * take its count.
				 */
				insearch = 0;
				eforbid (lastcmd[0] == 0);
				if (hadcnt)
					lastcnt = cnt;
				cnt = lastcnt;
				hadcnt = lasthad;
				vglobp = lastcmd;
				goto ereread;
#endif
			/*
			 * 	Abort command (with a little feedback).
			 */
			default:
efonfon:
				insearch = 0;
				beep();
				vmacp = 0;
				continue;
			} /* end switch on char after ^X */
		/*
		 * default	Insert one char in the buffer.
		 */
		default:
einsrt:
			/*
			 * Save command for repeat cmd. Save old line state
			 * for undo.  Position cursor and do append.
			 * vappend() detects emacs mode & calls vgetline to get
			 * a single char, similar to input for vi 'r' cmd.
			 */
			setLAST();
			CP(oldbuf, linebuf);
			oldcursor = cursor;
			vgoto(column(cursor), 0);
			doomed = 0;
			if (!vglobp)
				ungetkey(c);	/* the char to insert */
			vappend('i', cnt, 0);
			continue;
		} /* end switch (c) */
	} /* end for (;;) */
}

/*
 * Initialize global variables that need to be set for the start
 * of an edit line.  This get done by editmain() and when we
 * scroll to another command line from the history list.
 */
init_globals()
{
	doomed = 0;
	Peekkey = 0;
	Outchar = vputchar;
	vmacp = 0;
	strip(cmdline);		/* 008 RNF */
	CP(linebuf, cmdline);
	CP(oldbuf, cmdline);
	outcol = 0;
	destcol = 0;
	outline = 0;
	destline = 0;
	normline();			/* print out cmd line */
	destcol = column(strend(linebuf)) + 1;
	vclreol();			/* sets markline and markeol */
	vsetcurs(linebuf);		/* get destcol and outcol set */
	oldcursor = linebuf;

	DEL[0] = 0;
	INS[0] = 0;
	lastcmd[0] = 0;
}

/*************************************************************
 * TTY setup code is in this section.
 *************************************************************/

#include <sys/time.h>

ttymode
vi_tty_setup(onoff)
	int onoff;		/* turn special attributes on or off */
{
	struct tchars  tchars;	/* INT, QUIT, XON, XOFF, EOF, BRK */
	struct sgttyb sgtty;

	if (onoff) {
		/*
		 * Setup tty characteristics
		 */
		if (ioctl(SHIN, TIOCGETP, &sgtty) == -1) {
			Perror ("ioctl");	/* 006 - GAG */
		}
		hold_tty = sgtty;
		sgtty.sg_flags &= ~(ECHO|XTABS|CRMOD);
		sgtty.sg_flags |= RAW;
		ioctl(SHIN, TIOCSETP, &sgtty);

		/* Get and save current break character. */
		ioctl (SHIN, TIOCGETC, &tchars);
		hold_tchars = tchars;

		/* Clear the ESC, if necessary */
		if (tchars.t_brkc == ESCAPE)
		    tchars.t_brkc = -1;
		/*
		 * Turn off start/stop char's.
		 * Also turn off quit since some ttys send quit for right arrow.
		 */
		tchars.t_quitc = '\377';
		tchars.t_startc = '\377';
		tchars.t_stopc = '\377';
		ioctl (SHIN, TIOCSETC, &tchars);
		putpad(KS);		/* Keypad transmit mode */
	} else {
		struct timeval timeout;
		/*
		 * Restore tty modes.
		 * On a slow network line the KE sequence doesn't get sent
		 *    out fast enough.  So the select is an easey way to pause.
		 */
		putpad(KE);		/* end keypad transmit mode */
		flusho();
		timeout.tv_sec = 0;
		timeout.tv_usec = 100000;	/* 100,000 is 1/10 sec */
		(void) select(0, 0, 0, 0, &timeout);
		ioctl(SHIN, TIOCSETP, &hold_tty);
		ioctl (SHIN, TIOCSETC, &hold_tchars);
	}
}

char	exttytype[ONMSZ] =
	{ 'd', 'u', 'm', 'b' };
/*
 * Terminal type initialization routines,
 * and calculation of flags at entry.
 */

gettmode()
{
	struct sgttyb tty;
	ioctl(SHIN, TIOCGETP, &tty);
	UPPERCASE = (tty.sg_flags & LCASE) != 0;
	NONL = (tty.sg_flags & CRMOD) == 0;
}

char *xPC;
char **sstrs[] = {
	&BC, &CE, &KD, &KE, &KH, &KL, &KR, &KS, &KU, &ND, &xPC, &TA, &UP
};
bool *sflags[] = {
	&AM, &BS, &HC, &HZ, &NC, &XB
};

char ltcbuf[TCBUFSIZE];

setterm(type)
	char *type;
{
	/*
	 * Get termcap entry
	 */
	if (tgetent(ltcbuf, type) != 1) {
		return;		/* 006 - GAG */
	}
	COLUMNS = tgetnum("co");
	aoftspace = tspace;
	zap();
	/*
	 * Initialize keypad arrow keys. (up, down, home are done to prevent
	 * them from causing strange edit behavior.)
	 */
	arrows[0].cap = KU; arrows[0].mapto = "k"; arrows[0].descr = "up";
	arrows[1].cap = KD; arrows[1].mapto = "j"; arrows[1].descr = "down";
	arrows[2].cap = KL; arrows[2].mapto = "h"; arrows[2].descr = "left";
	arrows[3].cap = KR; arrows[3].mapto = "l"; arrows[3].descr = "right";
	arrows[4].cap = KH; arrows[4].mapto = "H"; arrows[4].descr = "home";

	aoftspace = tspace;
	CP(exttytype, longname(ltcbuf, type));
}

/*
 * Get terminal capabilities (flag & string values).
 */
zap()
{
	register char *namp;
	register bool **fp;
	register char ***sp;

	namp = "ambshchzncxb";		/* boolean termcap attributes */
	fp = sflags;
	do {
		*(*fp++) = tgetflag(namp);
		namp += 2;
	} while (*namp);
	namp = "bccekdkekhklkrkskundpctaup";	/* string termcap attributes */
	sp = sstrs;
	do {
		*(*sp++) = tgetstr(namp, &aoftspace);
		namp += 2;
	} while (*namp);
	PC = xPC ? xPC[0] : 0;
}

int
srchback()
{
	register int idx, c, i;
	char *backptr;		/* for deletes during search string entry */

	editevent--;
	srchptr = srchcmd;
	*srchptr = 0;
	for (idx = 1; idx < SRCHLEN; idx++) {
		i = getkey();
		if (i == hold_tty.sg_erase)
			i = CTRL(h);
		else if (i == hold_tty.sg_kill)
			i = -1;
		switch (i) {

		/*
		 * ^C
		 * ESCAPE
		 *
		 * 	Abort command.
		 * 	(Maybe we should look up
		 *	 intr char in hold_tchars.t_intrc)
		 */
		case CTRL(c):
		case ESCAPE:
			beep();
			srchcmd[0] = '\0';
			editevent++;
			redraw();
			return(0);
		/*
		 * 	Execute search.
		 */
		case NL:
		case CR:
			*srchptr = 0;
			return(1);
		/*
		 * ^H	Backs up a character in the input.
		 */
		case CTRL(h):
			backptr = srchptr - 1;
			if (backptr < srchcmd) {
				beep();
				continue; /* get another char */
			}
			goto srchbackup;
		/*
		 * ^W		Back up a white/non-white word.
		 */
		case CTRL(w):
			wdkind = 1;
			for (backptr = srchptr; backptr > srchcmd && isspace(backptr[-1]); backptr--)
				continue;
			for (c = wordch(backptr - 1);
			    backptr > srchcmd && wordof(c, backptr - 1); backptr--)
				continue;
			goto srchbackup;
		/*
		 * users kill	Kill input on this line, back to
		 * (^U)		start of insert.
		 */
		case -1:
			backptr = srchcmd;
srchbackup:
			if (backptr == srchptr) {
				beep();
				continue;    /* get new char */
			}
			*backptr = '\0';
			vgoto(outcol - (srchptr-backptr),outline);
			srchptr = backptr;
			continue;	    /* get new char */
		}	/* end switch (i) */
		/*
		 * Have a normal character, echo it and save it
		 */
		*srchptr++ = i;
		exputchar(i);
		exflush();
	}
	/*
	 * Over-ran srchcmd buffer, terminate search string
	 */
	beep();
	srchcmd[0] = '\0';
	editevent++;
	redraw();
	return(0);
}

/*
 * Redraw the line
 */
redraw()
{
	char *holdcurs;

	holdcurs = cursor;
	vsetcurs(linebuf);
	normline();
	/*
	 * Clear from last char to end of line
	 */
	destcol = column(strend(linebuf)) + 1;
	vclreol();
	/*
	 * Reset cursor
	 */
	cursor = holdcurs;
	vsetcurs(cursor);
}

char *
longname(bp, def)
	register char *bp;
	char *def;
{
	register char *cp;

	while (*bp && *bp != ':' && *bp != '|')
		bp++;
	if (*bp == '|') {
		bp++;
		cp = bp;
		while (*cp && *cp != ':' && *cp != '|')
			cp++;
		*cp = 0;
		return (bp);
	}
	return (def);
}

/*
 * Ring terminal bell.
 */
beep()
{
	vputc(CTRL(g));
}

/*
 * Mapping for special keys on the terminal only.
 * Map the command input character c,
 * for keypads and labelled keys which do cursor
 * motions.  I.e. on an adm3a we might map ^K to ^P.
 * DM1520 for example has a lot of mappable characters.
 */

map(c,maps)
	register int c;
	register struct maps *maps;
{
	register int d;
	register char *p, *q;
	char b[10];	/* Assumption: no keypad sends string longer than 10 */

	/*
	 * If c==0, the char came from getesc typing escape.  Pass it through
	 * unchanged.  0 messes up the following code anyway.
	 */
	if (c==0)
		return(0);
	b[0] = c;
	b[1] = 0;
	for (d=0; maps[d].mapto; d++) {
		if (p = maps[d].cap) {
			for (q=b; *p; p++, q++) {
				if (*q==0) {
					/*
					 * Is there another char waiting?
					 *
					 * This test is oversimplified, but
					 * should work mostly. It handles the
					 * case where we get an ESCAPE that
					 * wasn't part of a keypad string.
					 */
					if ((c=='#' ? peekkey() : fastpeekkey()) == 0) {
						/*
						 * Nothing waiting.  Push back
						 * what we peeked at & return
						 * failure (c).
						 */
						macpush(&b[1]);
						return(c);
					}
					*q = getkey();
					q[1] = 0;
				}
				if (*p != *q)
					goto contin;
			}
			macpush(maps[d].mapto);
			c = getkey();
			return(c);	/* first char of map string */
			contin:;
		}
	}
	macpush(&b[1]);
	return(c);
}

/*
 * Push st onto the front of vmacp. This is tricky because we have to
 * worry about where vmacp was previously pointing. We also have to
 * check for overflow (which is typically from a recursive macro)
 */
macpush(st)
	char *st;
{
	char tmpbuf[BUFSIZ];

	if (st==0 || *st==0)
		return;
	if ((vmacp ? strlen(vmacp) : 0) + strlen(st) > BUFSIZ)
		error("Macro too long@ - maybe recursive?");
	if (vmacp) {
		strcpy(tmpbuf, vmacp);
	}
	strcpy(vmacbuf, st);
	if (vmacp)
		strcat(vmacbuf, tmpbuf);
	vmacp = vmacbuf;
}

/*************************************************************
 * The following routines deal with carying out the command actions
 *************************************************************/

/*
 * Find single character c, in direction dir from cursor.
 */
find(c)
	char c;
{
	for(;;) {
		if (edge())
			return (0);
		wcursor += dir;
		if (*wcursor == c)
			return (1);
	}
}

/*
 * Do a word motion with operator op, and cnt more words
 * to go after this.
 */
exword(op, cnt)
	register int (*op)();
	int cnt;
{
	register int which;
	register char *iwc;

	if (dir == 1) {
		iwc = wcursor;
		which = wordch(wcursor);
		while (wordof(which, wcursor)) {
			if (cnt == 1 && op != vmove && wcursor[1] == 0) {
				wcursor++;
				break;
			}
			if (!lnext())
				return (0);
			if (wcursor == linebuf)
				break;
		}
		/* Unless last segment of a change skip blanks */
		if (op != vchange || cnt > 1)
			while (!margin() && blank())
				wcursor++;
		else
			if (wcursor == iwc && *iwc)
				wcursor++;
		if (op == vmove && margin())
			wcursor--;
	} else {
		if (!lnext())
			return (0);
		while (blank())
			if (!lnext())
				return (0);
		if (!margin()) {
			which = wordch(wcursor);
			while (!margin() && wordof(which, wcursor))
				wcursor--;
		}
		if (wcursor < linebuf || !wordof(which, wcursor))
			wcursor++;
	}
	return (1);
}

lnext()
{
	if (dir > 0) {
		if (*wcursor)
			wcursor++;
		if (*wcursor)
			return (1);
		else
			if (wcursor > linebuf)
				wcursor--;
			return (0);
	} else {
		--wcursor;
		if (wcursor >= linebuf)
			return (1);
		else   	{
			wcursor++; /* 11/16 fixes 'b' past start of line bug */
			return (0);
		}
	}
}

/*
 * To end of word, with operator op and cnt more motions
 * remaining after this.
 */
eend(op)
	register int (*op)();
{
	register int which;

	if (!lnext())
		return;
	while (blank())
		if (!lnext())
			return;
	which = wordch(wcursor);
	while (wordof(which, wcursor)) {
		if (wcursor[1] == 0) {
			wcursor++;
			break;
		}
		if (!lnext())
			return;
	}
	if (op != vchange && op != vdelete && wcursor > linebuf)
		wcursor--;
}

/*
 * Wordof tells whether the character at *wc is in a word of
 * kind which (blank/nonblank words are 0, conservative words 1).
 */
wordof(which, wc)
	char which;
	register char *wc;
{
	if (isspace(*wc))
		return (0);
	return (!wdkind || wordch(wc) == which);
}

/*
 * Wordch tells whether character at *wc is a word character
 * i.e. an alfa, digit, or underscore.
 */
wordch(wc)
	char *wc;
{
	register int c;

	c = wc[0];
	return (isalpha(c) || isdigit(c) || c == '_');
}

/*
 * Edge tells when we hit the last character in the current line.
 */
edge()
{
	if (linebuf[0] == 0)
		return (1);
	if (dir == 1)
		return (wcursor[1] == 0);
	else
		return (wcursor == linebuf);
}

/*
 * Margin tells us when we have fallen off the end of the line.
 */
margin()
{
	return (wcursor < linebuf || wcursor[0] == 0);
}


/*
 * Find words, repeated count times.
 * 'f' is the operation to be performed eventually.  
 */
lfind(pastatom, cnt, f)
	bool pastatom;			/* == 2 for word; == 3 for end */
	int cnt, (*f)();
{
	int rc = 0;			/* return code */

	wcursor = cursor;

	while (cnt > 0 && exword(f, cnt))
		cnt--;
	if (pastatom == 3)
		eend(f);
	if (cursor == wcursor)
		rc = -1;
	return (rc);
}

/*
 * Delete operator.
 */
vdelete(c)
	char c;
{
	register char *cp;

	if (wcursor < linebuf)
		wcursor = linebuf;
	if (cursor == wcursor) {
		beep();
		return;
	}
	(void) vdcMID();
	cp = cursor;
	setBUF(DEL);			/* save del text for put */
	CP(cp, wcursor);
	if (cp > linebuf && (cp[0] == 0 || c == '#'))
		cp--;
	/*
	 * Redraw the line now that we deleted some
	 */
	vsetcurs(linebuf);
	normline();
	/*
	 * Clear from last char to end of line
	 */
	destcol = column(strend(linebuf)) + 1;
	vclreol();
	/*
	 * Reset cursor
	 */
	vsetcurs(cp);
}

/*
 * Common code for middle part of delete
 * and change operating on parts of lines.
 */
vdcMID()
{
	register char *cp;

	setLAST();
	if (wcursor < cursor)
		cp = wcursor, wcursor = cursor, cursor = cp;
	return (column(wcursor - 1));
}

/*
 * Change operator.
 * We mark the end of the changed area with '$'.
 * Delete the text in the linebuf then do an insert.
 */
vchange(c)
	char c;
{
	register char *cp;
	register int i;		/* 006 - GAG */

	if (wcursor < linebuf)
		wcursor = linebuf;
	if (cursor == wcursor) {
		beep();
		return;
	}
	i = vdcMID();
	cp = cursor;
	vsetcurs(cursor);
	/*
	 * Mark the end of the change with $.
	 */
	vgoto(i,0);
	exputchar('$');
	/*
	 * Copy text over the deleted portion,
	 * then execute the input portion of the change.
	 */
	cursor = cp;
	CP(cursor, wcursor);
	vgoto(column(cursor),0);
	doomed = i;
	vappend('c', 1, 0);
}


/*
 * Replace a single character with the next input character.
 * A funny kind of insert.
 */
vrep(cnt)
	register int cnt;
{
	register int i, c;

	if (cnt > strlen(cursor)) {
		beep();
		return;
	}
	i = column(cursor + cnt - 1);
	vgoto(column(cursor),0);
	doomed = i;
	if (!vglobp) {
		c = getesc();
		if (c == 0) {
			vsetcurs(cursor);
			return;
		}
		ungetkey(c);
	}
	wcursor = cursor + cnt;
	CP(cursor, wcursor);
	vappend('r', cnt, 0);
	*lastcp++ = INS[0];
	setLAST();
}

char	*ogcursor;
/*
 * Append command (append, insert, change)
 * Called for insert (i), append (a), replace (r), and change (c)
 */
vappend(ch, cnt, indent)
	int ch;			/* type of text addition: i, a, r, c */
	int cnt, indent;	/* repeat count on command */
{
	register char *gcursor;
	int repcnt;
	short oldcol;		/* hold destcol while updating line */
	short oldline;		/* hold destline while updating line */

	/*
	 * Handle replace character by (eventually)
	 * limiting the number of input characters allowed
	 * in the vgetline routine.
	 */
	if (ch == 'r' || (emacs && (!vglobp)))
		repcnt = 2;
	else
		repcnt = 0;
	gcursor = genbuf;
	*gcursor = 0;
	/*
	 * If we are in a repeated command then
	 * use the previous inserted text (in INS buffer).
	 * If there is none or it was too long to be saved,
	 * then beep() and also arrange to undo any damage done
	 * so far (e.g. if we are a change.)
	 */
	if (vglobp && *vglobp == 0) {
		if (INS[0] == NULL) {
			beep();
			doomed = 0;
			return;
		}
		/*
		 * Unread input from INS.
		 * An escape will be generated at end of string.
		 * Hold off n^^2 type update on dumb terminals.
		 */
		vglobp = INS;
	} else if (vglobp == 0)
		/*
		 * Not a repeated command, get
		 * a new inserted text for repeat.
		 */
		INS[0] = 0;
	/*
	 * Text gathering.
	 * New text goes into genbuf starting at gcursor.
	 * cursor preserves place in linebuf where text will eventually go.
	 */
	if (ch != 'r' || repcnt != 0) {	/* 006 - GAG */
		gcursor = vgetline(repcnt, gcursor, ch);
		addtext(ogcursor);
	}
	repcnt = 0;
	/*
	 * Limit the repetition count based on maximum
	 * possible line length; do output implied
	 * by further count (> 1) and cons up the new line
	 * in linebuf (cursor pointer).
	 */
	cnt = vmaxrep(ch, cnt);
	CP(gcursor + 1, cursor);
	do {
		CP(cursor, genbuf);
		if (cnt > 1) {
			Outchar = vinschar;
			oldcol = destcol;
			oldline = destline;
			updateline(cursor);
			destcol = oldcol;
			destline = oldline;
			insmode = 1;
			vgoto(destcol,destline);
			insmode = 0;
			exflush();
			vgoto(destcol,destline);
			Outchar = vputchar;
		}
		cursor += gcursor - genbuf;
	} while (--cnt > 0);
	CP(cursor, gcursor + 1);
	/*
	 * If doomed characters remain, clobber them,
	 * and update the line on the screen.
	 */
	if (doomed > 0) {
		doomed = 0;
	}
	/*
	 * Now that insert is done:
	 *    for emacs mode: increment the cursor.
	 *    for vi mode:    redraw the line.
	 */
	if ((!emacs) || (hadcnt != 0) || vglobp) {
		redraw();
		/*
		 * All done with insertion, position the cursor.
		 */
		if ((cursor > linebuf) && (!emacs))
			cursor--;
		doomed = 0;
	}
	wcursor = cursor;
	vsetcurs(cursor);
}

/*
 * Get a line into genbuf after gcursor.
 * Cnt limits the number of input characters
 * accepted and is used for handling the replace
 * single character command.
 *
 * We do erase-kill type processing here.
 * commch is the command character involved.
 *
 * Input mode mappings are done here also.
 */
char *
vgetline(cnt, gcursor, commch)
	int cnt;
	register char *gcursor;
	char commch;
{
	register int c, ch;
	register char *cp;
	char *iglobp;
	int (*OO)() = Outchar;
	short oldcol;		/* hold destcol while updating line */
	short oldline;		/* hold destline while updating line */
	short farcol;		/* farthest point of advance before delete */

	/*
	 * Clear the output state and counters.
	 */
	ogcursor = gcursor;
	flusho();
	iglobp = vglobp;
	farcol = 0;

	Outchar = vinschar;
	for (;;) {
		if (cnt != 0) {
			cnt--;
			if (cnt == 0)
				goto vadone;
		}
		c = getkey();
		if (c != ATTN)
			c &= (QUOTE|TRIM);
		ch = c;
		maphopcnt = 0;
		if (vglobp == 0 && Peekkey == 0 && commch != 'r')
			while ((ch = map(c, immacs)) != c) {
				c = ch;
				if (++maphopcnt > 256)
					error("Infinite macro loop");
			}
		if (!iglobp) {
			/*
			 * Erase-kill type processing.
			 * Only happens if we were not reading
			 * from untyped input when we started.
			 * Map users erase to ^H, kill to -1 for switch.
			 */
			if (c == hold_tty.sg_erase)
				c = CTRL(h);
			else if (c == hold_tty.sg_kill)
				c = -1;
			switch (c) {

			/*
			 * ^?		Interrupt
			 *
			 * ^\		Quit
			 */
			case ATTN:
				ungetkey(c);
			case QUIT:			/* 009 RNF */
				goto vadone;
			/*
			 * ^H		Backs up a character in the input.
			 */
			case CTRL(h):
				cp = gcursor - 1;
				if (cp < ogcursor) {
					beep();
					continue;
				}
				goto vbackup;
			/*
			 * ^W		Back up a white/non-white word.
			 */
			case CTRL(w):
				wdkind = 1;
				for (cp = gcursor; cp > ogcursor && isspace(cp[-1]); cp--)
					continue;
				for (c = wordch(cp - 1);
				    cp > ogcursor && wordof(c, cp - 1); cp--)
					continue;
				goto vbackup;
			/*
			 * users kill	Kill input on this line, back to
			 * (^U)		start of insert.
			 */
			case -1:
				cp = ogcursor;
vbackup:
				if (cp == gcursor) {
					beep();
					continue;
				}
				*cp = 0;
				c = 0;
				vgoto(outcol - (gcursor-cp),outline);
				if (doomed >= 0)
					doomed += c;
				gcursor = cp;
				continue;
			}	/* end switch (c) */
		}

		switch (c) {

		/*
		 * ^M		Except in repeat maps to \n.
		 */
		case CR:
			if (vglobp)
				goto def;
			c = '\n';
			/* presto chango ... */
		/*
		 * \n		End insert & set flag for top level cmd loop.
		 */
		case NL:
			done_edit = 1;
			goto vadone;
		/*
		 * escape	End insert.
		 */
		case ESCAPE:
			if (lastvgk)
				goto def;
			goto vadone;

		default:
			/*
			 * Possibly discard control inputs.
			 */
			if (!vglobp && junk(c)) {
				beep();
				continue;
			}
def:
			/*
			 * Put out the new char.
			 * If insert, append, or change beyond '$', and
			 * not getting chars from delete buffer (iglobp set),
			 * and not on multiple lines (UP) without ND
			 * then reprint the rest of the line
			 * to give the illusion of shifting over.
			 */
			exputchar(c);
			if (destcol > farcol)
				farcol = destcol;
			if (!(commch == 'r' || (commch == 'c' && destcol < column(wcursor)) || (destcol < farcol) || iglobp || (UP && !(ND)))) {
				oldcol = destcol;
				oldline = destline;
				updateline(cursor);
				destcol = oldcol;
				destline = oldline;
				insmode = 1;
				vgoto(destcol,destline);
				insmode = 0;
			}
			exflush();
			if (gcursor > &genbuf[LBSIZE - 2])
				error("Line too long");
			*gcursor++ = c & (QUOTE|TRIM);
			vgoto(destcol,destline);
			continue;
		}
	}
vadone:
	*gcursor = 0;
	Outchar = OO;
	return (gcursor);
}

/*
 * Update line.  Output the line from cp to eol.
 */
updateline(cp)
	register char *cp;
{
	for (; *cp; )
		exputchar(*cp++);
}

junk(c)
	register int c;
{
	if (c >= ' ' && c != DELETE)
		return (0);
	switch (c) {

	case '\t':
	case '\n':
	case '\f':
		return (0);
	default:
		return (1);
	}
}

/*
 * Vmaxrep determines the maximum repetitition factor
 * allowed that will yield total line length less than
 * LBSIZE characters.
 */
vmaxrep(ch, cnt)
	char ch;
	register int cnt;
{
	register int len, replen;

	if (cnt > LBSIZE - 2)
		cnt = LBSIZE - 2;
	replen = strlen(genbuf);
	len = strlen(linebuf);
	if (len + cnt * replen <= LBSIZE - 2)
		return (cnt);
	cnt = (LBSIZE - 2 - len) / replen;
	if (cnt == 0) {
		error("Line too long");
	}
	return (cnt);
}

/*
 * Clear to the end of the current physical line
 */
vclreol()
{
	register int i;
	int col;

	if (CE) {
		col = destcol;
		vgoto(destcol,0);
		vputp(CE, 1);
		if (UP) {
			for (i = col/COLUMNS +1; i <= markline; i++) {
				vgoto(0,i);
				vputp(CE, 1);
			}
			markline = strlen(linebuf)/COLUMNS;
		}
		return;
	}
	/*
	 * No clear to end of line capability, so
	 * calculate the number of blanks needed and put them out.
	 */
	vgoto(destcol,0);
	for (i = markeol - strend(linebuf); i > 0; i--) {
		exputchar(' ');
	}
	markeol = strend(linebuf);
}

/*
 * Gather up some more text from an insert.
 * If the insertion buffer oveflows, then destroy
 * the repeatability of the insert.
 */
addtext(cp)
	char *cp;
{
	if (vglobp)
		return;
	addto(INS, cp);
	if (INS[0] == NULL)
		lastcmd[0] = 0;
}

/*
 * Put text from cursor upto wcursor in BUF.
 * Used to put back deletes.
 */
setBUF(BUF)
	register char *BUF;
{
	register int c;
	register char *wp = wcursor;

	c = *wp;
	*wp = 0;
	BUF[0] = 0;
	addto(BUF, cursor);
	*wp = c;
}

addto(buf, str)
	register char *buf, *str;
{
	if (strlen(buf) + strlen(str) + 1 >= VBSIZE) {
		buf[0] = NULL;
		return;
	}
	(void)strcat(buf, str);
}

/*
 * A complete command has been defined for
 * the purposes of repeat, so copy it from
 * the working to the previous command buffer.
 */
setLAST()
{

	if (vglobp || vmacp)
		return;
	lasthad = Xhadcnt;
	lastcnt = Xcnt;
	*lastcp = 0;
	CP(lastcmd, workcmd);
}

Copy(to, from, size)
	register char *from, *to;
	register int size;
{
	if (size > 0)
		do
			*to++ = *from++;
		while (--size > 0);
}

/*
 * Find matching paren-like items: ({[ ]}).
 */
lmatchp()
{
	register int i;
	register char *parens, *cp;

	for (cp = cursor; !any(*cp, "({[)}]");)
		if (*cp++ == 0)
			return (0);
	parens = any(*cp, "()") ? "()" : any(*cp, "[]") ? "[]" : "{}";
	if (*cp == parens[1]) {
		dir = -1;
	} else {
		dir = 1;
	}
	wcursor = cp;
	i = lskipbal(parens);
	return (i);
}


/*
 * Find the matching paren-like object
 */
lskipbal(parens)
	register char *parens;
{
	register int level = dir;
	register int c;

	do {
		if (!lnext()) {
			return (0);
		}
		c = *wcursor;
		if (c == parens[1])
			level--;
		else if (c == parens[0])
			level++;
	} while (level);
	return (1);
}

/*************************************************************
 * Input routines.
 *************************************************************/

/*
 * Return the key.
 */
ungetkey(c)
	int c;
{
	if (Peekkey != ATTN)
		Peekkey = c;
}

/*
 * Return a keystroke, but never a ^@.
 */
getkey()
{
	register int c;

	do {
		c = getbr();
		if (c==0)
			beep();
	} while (c == 0);
	return (c);
}

jmp_buf	readbuf;
int	doingread = 0;

/*
 * Get a keystroke, including a ^@.
 * If a key was returned with ungetkey, that
 * comes back first.  Next comes unread input (e.g.
 * from repeating commands with .), and finally new
 * keystrokes.
 */
getbr()
{
	char ch;
	register int c;
#define BEEHIVE
#ifdef BEEHIVE
	static char Peek2key;
#endif

getATTN:
	if (Peekkey) {
		c = Peekkey;
		Peekkey = 0;
		return (c);
	}
#ifdef BEEHIVE
	if (Peek2key) {
		c = Peek2key;
		Peek2key = 0;
		return (c);
	}
#endif
	if (vglobp) {
		if (*vglobp)
			return (lastvgk = *vglobp++);
		lastvgk = 0;
		return (ESCAPE);
	}
	if (vmacp) {
		if (*vmacp)
			return(*vmacp++);
		/* End of a macro or set of nested macros */
		vmacp = 0;
	}
	flusho();
	if (setjmp(readbuf))
		goto getATTN;
	doingread = 1;
	c = read(SHIN, &ch, 1);
	doingread = 0;
	if (c != 1) {
		if (errno == EINTR)
			goto getATTN;
		error("Input read error");
	}
	c = ch & (QUOTE|TRIM);
#ifdef BEEHIVE
	if (XB && c == ESCAPE) {
		if (read(SHIN, &Peek2key, 1) != 1)
			goto getATTN;
		Peek2key &= (QUOTE|TRIM);
		switch (Peek2key) {
		case 'C':	/* SPOW mode sometimes sends \EC for space */
			c = ' ';
			Peek2key = 0;
			break;
		case 'q':	/* f2 -> ^C */
			c = CTRL(c);
			Peek2key = 0;
			break;
		case 'p':	/* f1 -> esc */
			Peek2key = 0;
			break;
		}
	}
#endif
	lastvgk = 0;
	return (c);
}

/*
 * Get a key, but if a delete, quit or attention
 * is typed return 0 so we will abort a partial command.
 */
getesc()
{
	register int c;

	c = getkey();
	switch (c) {

	case CTRL(v):
	case CTRL(q):
		c = getkey();
		return (c);

	case ATTN:
	case QUIT:
		ungetkey(c);
		return (0);

	case ESCAPE:
		return (0);
	}
	return (c);
}

/*
 * Peek at the next keystroke.
 */
peekkey()
{
	Peekkey = getkey();
	return (Peekkey);
}

/*
 * Get a count from the keyed input stream.
 * A zero count is indistinguishable from no count.
 */
vgetcnt()
{
	register int c, cnt;

	cnt = 0;
	for (;;) {
		c = getkey();
		if (!isdigit(c))
			break;
		cnt *= 10, cnt += c - '0';
	}
	ungetkey(c);
	Xhadcnt = 1;
	Xcnt = cnt;
	return(cnt);
}

/*
 * fastpeekkey is just like peekkey but insists the character come in
 * fast (within 1 second). This will succeed if it is the 2nd char of
 * a machine generated sequence (such as a function pad from an escape
 * flavor terminal) but fail for a human hitting escape then waiting.
 */
fastpeekkey()
{
	int trapalarm();
	int (*Oint)();
	register int c;

	/*
	 * We force this to die in 1 second.
	 * This is pretty reliable (VMUNIX rounds it to .5 - 1.5 secs,
	 * but due to system delays
	 * there are times when arrow keys or very fast typing get counted
	 * as separate.
	 */
	Oint = (int (*)())signal(SIGINT, trapalarm);
	signal(SIGALRM, trapalarm);
	alarm(1);
	CATCH
		c = peekkey();
		alarm(0);
	ONERR
		c = 0;
	ENDCATCH
	signal(SIGINT,Oint);
	return(c);
}

trapalarm() {
	alarm(0);
	if (vcatch)
		longjmp(vreslab,1);
}

/*************************************************************
 * Terminal output and line formatting routines.
 *************************************************************/

/*
 * Format c for printing.  Handle funnies of upper case terminals
 * and crocky hazeltines which don't have ~.
 */
normchar(c)
	register short c;
{
	register char *colp;

	c &= (QUOTE|TRIM);
	if (c == '~' && HZ) {
		normchar('\\');
		c = '^';
	}
	if (c < ' ' && (c != '\b') || c == DELETE)
		c = ' ';
	else if (UPPERCASE)
		if (isupper(c)) {
			outchar('\\');
			c = tolower(c);
		} else {
			colp = "({)}!|^~'`";
			while (*colp++)
				if (c == *colp++) {
					outchar('\\');
					c = colp[-2];
					break;
				}
		}
	outchar(c);
}

/*
 * Normal line output, no numbering.
 */
normline()
{
	register char *cp;

	for (cp = linebuf; *cp;)
		exputchar(*cp++);
}

/*
 * The output buffer is initialized with a useful error
 * message so we don't have to keep it in data space.
 */
static	char linb[66];
char *exlinp = linb;

/*
 * Indirect to current definition of putchar.
 */
exputchar(c)
	int c;
{
	(*Putchar)(c);
}

exflush()
{
	flush1();
	flush2();
}

/*
 * Flush from small line buffer into output buffer.
 * Work here is destroying motion into positions, and then
 * letting fgoto do the optimized motion.
 */
flush1()
{
	register char *lp;
	register short c;
	*exlinp = 0;
	lp = linb;
	while (*lp)
		switch (c = *lp++) {

		case '\r':
			destcol = 0;
			continue;

		case '\b':
			if (destcol)
				destcol--;
			continue;

		case '\t':
		case ' ':
			destcol++;
			continue;
		case '\n':
			destcol = 0;
			continue;

		default:
			fgoto();
			for (;;) {
				if (AM == 0 && outcol == COLUMNS)
					fgoto();
				c &= (QUOTE|TRIM);
				putch(c);
				if (c == '\b') {
					outcol--;
					destcol--;
				} else if (c >= ' ' && c != DELETE) {
					outcol++;
					destcol++;
					if (outcol % COLUMNS == 0)
						putch('\r'), putch('\n');
				}
				c = *lp++;
				if (c <= ' ')
					break;
			}
			--lp;
			continue;
		}
	exlinp = linb;
}

flush2()
{
	fgoto();
	flusho();
}

char	*obp = obuf;

flusho()
{
	if (obp != obuf) {
		write(SHOUT, obuf, obp - obuf);
		obp = obuf;
	}
}

putch(c)
	int c;
{
	*obp++ = c & (QUOTE|TRIM);
	if (obp >= &obuf[sizeof obuf])
		flusho();
}

/*
 * Put with padding
 */
putpad(cp)
	char *cp;
{
	exflush();
	tputs(cp, 0, putch);
}

/*
 * Insert character c at current cursor position.
 */
vinschar(c)
	int c;
{
	vputchar(c);
}

/*
 * Put the character c on the screen at the current cursor position.
 */
vputchar(c)
	register int c;
{
	c &= (QUOTE|TRIM);
	if (destcol >= COLUMNS) {
		if (UP) {
			destline += destcol / COLUMNS;
			destcol %= COLUMNS;
		}
	else
		destcol = COLUMNS - 1;
	}
	switch (c) {

	case '\t':
		c = ' ';
		/* fall into ... */

	default:
		if (outcol != destcol)
			vgoto(destcol,destline);
		vputc(c);
		destcol++, outcol++;
	}
}

/*
 * Put a character
 */
vputch(c)
	int c;
{
	vputc(c);
}

/*************************************************************
 * Cursor Motion Routines
 *************************************************************/

vmove()
{
	vsetcurs(wcursor);
}

/*
 * Compute the column position implied by the cursor at ``nc'',
 * and move the cursor there.
 */
vsetcurs(nc)
	register char *nc;
{
	register int col;

	col = column(nc);
	vgoto(col,0);
	cursor = nc;
}

column(cp)
	char *cp;
{
	return(cp - linebuf);
}


/*
 * Move cursor to column x.
 */
vgoto(x,y)
	register int x,y;
{
	/*
	 * Fold the possibly too large value of x.
	 */
	if (x >= COLUMNS) {
		if (UP) {
			y += x / COLUMNS;
			x %= COLUMNS;
		}
		else
			x = COLUMNS - 1;
	}
	destcol = x;
	destline = y;
	fgoto();
}

/*
 * Sync the position of the output cursor.
 */
fgoto()
{

	if (destcol >= COLUMNS) {
		if (UP) {
			destline += destcol /COLUMNS;
			destcol %= COLUMNS;
		}
		else
			destcol = COLUMNS - 1;
	}
	plod(0);
	outcol = destcol;
	outline = destline;
}

/*
 * Move (slowly) to destination.
 * Just use character printing (move right), and backspace (move left).
 */

static int plodcnt, plodflg;

plodput(c)
{
	if (plodflg)
		plodcnt--;
	else
		putch(c);
}

plod(cnt)
{
	register int c;
	register int index;

	plodcnt = plodflg = cnt;
	/*
	 * Move down, if necessary, to desired line
	 */
	if ((outline < destline) && UP) {
		plodput('\r');
		outcol = 0;
		while (outline < destline) {
			plodput('\n');
			outline++;
		}
	}
	/*
	 * Move up, if necessary, to desired line
	 */
	while (outline > destline) {
		outline--;
		tputs(UP,0,plodput);
	}
	/*
	 * Move left, if necessary, to desired column
	 */
	while (outcol > destcol) {
		if (plodcnt < 0)
			goto out;
		outcol--;
		if (BC)
			tputs(BC, 0, plodput);
		else
			plodput('\b');
	}
	/*
	 * Now move to the right, if necessary.
	 */
	index = outcol + outline * COLUMNS;
	while (outcol < destcol) {
		/*
		 * Move one char to the right.  We only use ND space
		 * if repositioning the cursor during insert and we can
		 * have more than 80 chars (tty has upline (UP) capability).
		 * Otherwise it's faster to just print the char we are
		 * moving over.  There are various exceptions, however.
		 * If the char is a null or a tab we want to print a space.
		 * Other random chars we use space for instead, too.
		 */
		if ( (c=linebuf[index]) < ' ')
			c = ' ';
		if (insmode && UP && ND)
			tputs(ND, 0, plodput);
		else
			plodput(c);
		index++;
		outcol++;
		if (plodcnt < 0)
			break;
	}
out:
	return(plodcnt);
}
#endif
