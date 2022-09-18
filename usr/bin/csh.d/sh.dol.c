#ifndef lint
static char *sccsid = "@(#)sh.dol.c	4.1  (ULTRIX)        7/17/90";
#endif
/***********************************************************************
 *                                                                      *
 *                      Copyright (c) 1988 by                           *
 *              Digital Equipment Corporation, Maynard, MA              *
 *                      All rights reserved.                            *
 *                                                                      *
 *   This software is furnished under a license and may be used and     *
 *   copied  only  in accordance with the terms of such license and     *
 *   with the  inclusion  of  the  above  copyright  notice.   This     *
 *   software  or  any  other copies thereof may not be provided or     *
 *   otherwise made available to any other person.  No title to and     *
 *   ownership of the software is hereby transferred.                   *
 *                                                                      *
 *   The information in this software is subject to change  without     *
 *   notice  and should not be construed as a commitment by Digital     *
 *   Equipment Corporation.                                             *
 *                                                                      *
 *   Digital assumes no responsibility for the use  or  reliability     *
 *   of its software on equipment which is not supplied by Digital.     *
 *                                                                      *
 ************************************************************************/
/* ------------------------------------------------------------------ */
/* | Copyright Unpublished, MIPS Computer Systems, Inc.  All Rights | */
/* | Reserved.  This software contains proprietary and confidential | */
/* | information of MIPS and its suppliers.  Use, disclosure or     | */
/* | reproduction is prohibited without the prior express written   | */
/* | consent of MIPS.                                               | */
/* ------------------------------------------------------------------ */
/* $Header: sh.dol.c,v 1.3 86/07/11 10:29:59 dce Exp $ */

#include "sh.h"

/*
 * C shell
 *
 * Modification History
 *
 * 003 - Gary A. Gaudet - Wed Jan 31 13:09:35 EST 1990
 *	Fixed escape of command substitution and variable expansion
 *	in heredoc rediction.
 *
 * 002 - Gary A. Gaudet - Thu Dec 28 17:43:59 EST 1989
 *	Added (castings). & TRIMming.
 *
 * 01 Sat Aug 13 15:28:57 EDT 1988, Gary A. Gaudet
 *	merging mips & ultrix for 8 bit clean and bug fixes
 */

/*
 * These routines perform variable substitution and quoting via ' and ".
 * To this point these constructs have been preserved in the divided
 * input words.  Here we expand variables and turn quoting via ' and " into
 * QUOTE bits on characters (which prevent further interpretation).
 * If the `:q' modifier was applied during history expansion, then
 * some QUOTEing may have occurred already, so we dont "trim()" here.
 */

int	Dpeekc, Dpeekrd;		/* Peeks for DgetC and Dreadc */
char	*Dcp, **Dvp;			/* Input vector for Dreadc */

#define	DEOF	-1

#define	unDgetC(c)	Dpeekc = c

#define QUOTES		(_Q|_Q1|_ESC)	/* \ ' " ` */

/*
 * The following variables give the information about the current
 * $ expansion, recording the current word position, the remaining
 * words within this expansion, the count of remaining words, and the
 * information about any : modifier which is being applied.
 */
char	*dolp;			/* Remaining chars from this word */
char	**dolnxt;		/* Further words */
int	dolcnt;			/* Count of further words */
char	dolmod;			/* : modifier character */
int	dolmcnt;		/* :gx -> 10000, else 1 */

/*
 * Fix up the $ expansions and quotations in the
 * argument list to command t.
 */
Dfix(t)
	register struct command *t;
{
	register char **pp;
	register char *p;

	if (noexec)
		return;
	/* Note that t_dcom isn't trimmed thus !...:q's aren't lost */
	for (pp = t->t_dcom; p = *pp++;)
		while (*p)
			if (cmap(*p++, _DOL|QUOTES)) {	/* $, \, ', ", ` */
				Dfix2(t->t_dcom);	/* found one */
				blkfree(t->t_dcom);
				t->t_dcom = gargv;
				gargv = 0;
				return;
			}
}

/*
 * $ substitute one word, for i/o redirection
 */
char *
Dfix1(cp)
	register char *cp;
{
	char *Dv[2];

	if (noexec)
		return (0);
	Dv[0] = cp; Dv[1] = NOSTR;
	Dfix2(Dv);
	if (gargc != 1) {
		setname(cp);
		bferr("Ambiguous");
	}
	cp = savestr(gargv[0]);
	blkfree(gargv), gargv = 0;
	return (cp);
}

/*
 * Subroutine to do actual fixing after state initialization.
 */
Dfix2(v)
	char **v;
{
	char *agargv[GAVSIZ];

	ginit(agargv);			/* Initialize glob's area pointers */
	Dvp = v; Dcp = "";		/* Setup input vector for Dreadc */
	unDgetC(0); unDredc(0);		/* Clear out any old peeks (at error) */
	dolp = 0; dolcnt = 0;		/* Clear out residual $ expands (...) */
	while (Dword())
		continue;
	gargv = copyblk(gargv);
}

/*
 * Get a word.  This routine is analogous to the routine
 * word() in sh.lex.c for the main lexical input.  One difference
 * here is that we don't get a newline to terminate our expansion.
 * Rather, DgetC will return a DEOF when we hit the end-of-input.
 */
Dword()
{
	register int c, c1;
	char wbuf[BUFSIZ*2];
	register char *wp = wbuf;
	register int i = BUFSIZ*2 - 4*2;
	register bool dolflg;
	bool sofar = 0;

loop:
	c = DgetC(DODOL);
	switch (c) {

	case DEOF:
deof:
		if (sofar == 0)
			return (0);
		/* finish this word and catch the code above the next time */
		unDredc(c);
		/* fall into ... */

	case '\n':
		*wp = 0;
		goto ret;

	case ' ':
	case '\t':
		goto loop;

	case '`':
		/* We preserve ` quotations which are done yet later */
		*wp++ = c, --i;
	case '\'':
	case '"':
		/*
		 * Note that DgetC never returns a QUOTES character
		 * from an expansion, so only true input quotes will
		 * get us here or out.
		 */
		c1 = c;
		dolflg = c1 == '"' ? DODOL : 0;
		for (;;) {
			c = DgetC(dolflg);
			if (c == c1)
				break;
			if (c == '\n' || c == DEOF)
				error("Unmatched %c", c1);
			if ((c & (QUOTE|TRIM)) == ('\n' | QUOTE)) 
				/*
				 * backout the previous character and its
				 * QUOTECHAR
				 */
				wp -= 2, i +=2;
			if (--i <= 0)
				goto toochars;
			switch (c1) {

			case '"':
				/*
				 * Leave any `s alone for later.
				 * Other chars are all quoted, thus `...`
				 * can tell it was within "...".
				 */
				if (c != '`') {
					*wp++ = QUOTECHAR;
					i--;
				}
				*wp++ = c;
				break;

			case '\'':
				/* Prevent all further interpretation */
				*wp++ = (char) QUOTECHAR;	/* 002 - GAG */
				i--;
				*wp++ = c;
				break;

			case '`':
				/* Leave all text alone for later */
				*wp++ = c;
				break;
			}
		}
		if (c1 == '`')
			*wp++ = '`', --i;
		goto pack;		/* continue the word */

	case '\\':
		c = DgetC(0);		/* No $ subst! */
		if (c == '\n' || c == DEOF)
			goto loop;
		c |= QUOTE;
		break;
	}
	unDgetC(c);
pack:
	sofar = 1;
	/* pack up more characters in this word */
	for (;;) {
		c = DgetC(DODOL);
		if (c == '\\') {
			c = DgetC(0);
			if (c == DEOF)
				goto deof;
			if (c == '\n')
				c = ' ';
			else
				c |= QUOTE;
		}
		if (c == DEOF)
			goto deof;
		if (any(c," \t\n'\"`")) {		/* sp \t\n'"` */
			unDgetC(c);
			if (any(c, "\\'\"`"))
				goto loop;
			*wp++ = 0;
			goto ret;
		}
		if (--i <= 0)
toochars:
			error("Word too long");
		if (c & QUOTE) {
			*(unsigned char *)wp++ = QUOTECHAR;	/* 002 - GAG */
			i--;
		}
		*wp++ = c & TRIM;	/* 002 - GAG */
	}
ret:
	Gcat("", wbuf);
	return (1);
}

/*
 * Get a character, performing $ substitution unless flag is 0.
 * Any QUOTES character which is returned from a $ expansion is
 * QUOTEd so that it will not be recognized above.
 */
DgetC(flag)
	register int flag;
{
	register int c;

top:
	if (c = Dpeekc) {
		Dpeekc = 0;
		return (c);
	}
	if (lap) {
		if (((c = *lap++) & TRIM) == QUOTECHAR)
			c = (*lap++ & TRIM) | QUOTE; /* GAG - fix "set a=$<" */
		if (c == 0) {
			lap = 0;
			goto top;
		}
quotspec:
		if (any(c, "\\'\"`"))
			return (c | QUOTE);
		return (c);
	}
	if (dolp) {
		if (((c = *dolp++) & TRIM) == QUOTECHAR)
			c = (*dolp++ & TRIM) | QUOTE; /* GAG - fix "set a=$<" */
		if (c & (QUOTE|TRIM))
			goto quotspec;
		if (dolcnt > 0) {
			setDolp(*dolnxt++);
			--dolcnt;
			return (' ');
		}
		dolp = 0;
	}
	if (dolcnt > 0) {
		setDolp(*dolnxt++);
		--dolcnt;
		goto top;
	}
	c = Dredc();
	if (c == '$' && flag) {
		Dgetdol();
		goto top;
	}
	return (c);
}

char	*nulvec[] = { 0 };
struct	varent nulargv = { nulvec, "argv", 0 };

/*
 * Handle the multitudinous $ expansion forms.
 * Ugh.
 */
Dgetdol()
{
	register char *np;
	register struct varent *vp;
	char name[20];
	int c, sc;
	int subscr = 0, lwb = 1, upb = 0;
	bool dimen = 0, bitset = 0;
	char wbuf[BUFSIZ];

	dolmod = dolmcnt = 0;
	c = sc = DgetC(0);
	if (c == '{')
		c = DgetC(0);		/* sc is { to take } later */
	if ((c & TRIM) == '#')
		dimen++, c = DgetC(0);		/* $# takes dimension */
	else if (c == '?')
		bitset++, c = DgetC(0);		/* $? tests existence */
	switch (c) {
	
	case '$':
		if (dimen || bitset)
			goto syntax;		/* No $?$, $#$ */
		setDolp(doldol);
		goto eatbrac;

	case '<'|QUOTE:
		if (dimen || bitset)
			goto syntax;		/* No $?<, $#< */
		for (np = wbuf; read(OLDSTD, np, 1) == 1; np++) {
			if (np >= &wbuf[BUFSIZ-1])
				error("$< line too long");
			if (*np <= 0 || *np == '\n')
				break;
		}
		*np = 0;
		/*
		 * KLUDGE: dolmod is set here because it will
		 * cause setDolp to call domod and thus to copy wbuf.
		 * Otherwise setDolp would use it directly. If we saved
		 * it ourselves, no one would know when to free it.
		 * The actual function of the 'q' causes filename
		 * expansion not to be done on the interpolated value.
		 */
		dolmod = 'q';
		dolmcnt = 10000;
		setDolp(wbuf);
		goto eatbrac;

	case DEOF:
	case '\n':
		goto syntax;

	case '*':
		(void) strcpy(name, "argv");
		vp = adrof("argv");
		subscr = -1;			/* Prevent eating [...] */
		break;

	default:
		np = name;
		if (digit(c)) {
			if (dimen)
				goto syntax;	/* No $#1, e.g. */
			subscr = 0;
			do {
				subscr = subscr * 10 + c - '0';
				c = DgetC(0);
			} while (digit(c));
			unDredc(c);
			if (subscr < 0)
				goto oob;
			if (subscr == 0) {
				if (bitset) {
					dolp = file ? "1" : "0";
					goto eatbrac;
				}
				if (file == 0)
					error("No file for $0");
				setDolp(file);
				goto eatbrac;
			}
			if (bitset)
				goto syntax;
			vp = adrof("argv");
			if (vp == 0) {
				vp = &nulargv;
				goto eatmod;
			}
			break;
		}
		/* GT01: must start with an alphabetic */
		if (!letter(c))
			goto syntax;
		for (;;) {
			*np++ = c;
			c = DgetC(0);
			if (!alnum(c))
				break;
			if (np >= &name[sizeof name - 2])
syntax:
				error("Variable syntax");
		}
		*np++ = 0;
		unDredc(c);
		vp = adrof(name);
	}
	if (bitset) {
		dolp = (vp || getenv(name)) ? "1" : "0";
		goto eatbrac;
	}
	if (vp == 0) {
		np = getenv(name);
		if (np) {
			addla(np);
			goto eatbrac;
		}
		udvar(name);
		/*NOTREACHED*/
	}
	c = DgetC(0);
	upb = blklen(vp->vec);
	if (dimen == 0 && subscr == 0 && c == '[') {
		np = name;
		for (;;) {
			c = DgetC(DODOL);	/* Allow $ expand within [ ] */
			if (c == ']')
				break;
			if (c == '\n' || c == DEOF)
				goto syntax;
			if (np >= &name[sizeof name - 2])
				goto syntax;
			*np++ = c;
		}
		*np = 0, np = name;
		if (dolp || dolcnt)		/* $ exp must end before ] */
			goto syntax;
		if (!*np)
			goto syntax;
		if (digit(*np)) {
			register int i = 0;

			while (digit(*np))
				i = i * 10 + *np++ - '0';
			if ((i < 0 || i > upb) && !any(*np, "-*")) {
oob:
				setname(vp->v_name);
				error("Subscript out of range");
			}
			lwb = i;
			if (!*np)
				upb = lwb, np = "*";
		}
		if (*np == '*')
			np++;
		else if (*np != '-')
			goto syntax;
		else {
			register int i = upb;

			np++;
			if (digit(*np)) {
				i = 0;
				while (digit(*np))
					i = i * 10 + *np++ - '0';
				if (i < 0 || i > upb)
					goto oob;
			}
			if (i < lwb)
				upb = lwb - 1;
			else
				upb = i;
		}
		if (lwb == 0) {
			if (upb != 0)
				goto oob;
			upb = -1;
		}
		if (*np)
			goto syntax;
	} else {
		if (subscr > 0)
			if (subscr > upb)
				lwb = 1, upb = 0;
			else
				lwb = upb = subscr;
		unDredc(c);
	}
	if (dimen) {
		char *cp = putn(upb - lwb + 1);

		addla(cp);
		xfree(cp);
	} else {
eatmod:
		c = DgetC(0);
		if (c == ':') {
			c = DgetC(0), dolmcnt = 1;
			if (c == 'g')
				c = DgetC(0), dolmcnt = 10000;
			if (!any(c, "htrqxe"))
				error("Bad : mod in $");
			dolmod = c;
			if (c == 'q')
				dolmcnt = 10000;
		} else
			unDredc(c);
		dolnxt = &vp->vec[lwb - 1];
		dolcnt = upb - lwb + 1;
	}
eatbrac:
	if (sc == '{') {
		c = Dredc();
		if (c != '}')
			goto syntax;
	}
}

setDolp(cp)
	register char *cp;
{
	register char *dp;

	if (dolmod == 0 || dolmcnt == 0) {
		dolp = cp;
		return;
	}
	dp = domod(cp, dolmod);
	if (dp) {
		dolmcnt--;
		addla(dp);
		xfree(dp);
	} else
		addla(cp);
	dolp = "";
}

unDredc(c)
	int c;
{

	Dpeekrd = c;
}

Dredc()
{
	register int c;

	if (c = Dpeekrd) {
		Dpeekrd = 0;
		return (c);
	}
	if (Dcp && (c = *Dcp++)) {
		if ((c  & TRIM) == QUOTECHAR)
			c = (*Dcp++ & TRIM) | QUOTE;
		return (c&(QUOTE|TRIM));
	}
	if (*Dvp == 0) {
		Dcp = 0;
		return (DEOF);
	}
	Dcp = *Dvp++;
	return (' ');
}

Dtestq(c)
	register int c;
{

	if (any(c, "\\'\"`"))
		gflag = 1;
}

/*
 * Form a shell temporary file (in unit 0) from the words
 * of the shell input up to a line the same as "term".
 * Unit 0 should have been closed before this call.
 */
heredoc(term)
	char *term;
{
	register int c;
	char *Dv[2];
	char obuf[BUFSIZ], lbuf[BUFSIZ], mbuf[BUFSIZ];
	int ocnt, lcnt, mcnt;
	register char *lbp, *obp, *mbp;
	char **vp;
	bool quoted;

	if (creat(shtemp, 0600) < 0)
		Perror(shtemp);
	(void) close(0);
	if (open(shtemp, 2) < 0) {
		int oerrno = errno;

		(void) unlink(shtemp);
		errno = oerrno;
		Perror(shtemp);
	}
	(void) unlink(shtemp);			/* 0 0 inode! */
	Dv[0] = term; Dv[1] = NOSTR; gflag = 0;
	trim(Dv); rscan(Dv, Dtestq); quoted = gflag;
	ocnt = BUFSIZ; obp = obuf;
	for (;;) {
		/*
		 * Read up a line
		 */
		lbp = lbuf; lcnt = BUFSIZ - 4;
		for (;;) {
			c = readc(1);		/* 1 -> Want EOF returns */
			if (c == (-1)) {	/* ARG - compare correctly */
				setname(term);
				bferr("<< terminator not found");
			}
			if (c == '\n')
				break;
			if (c &= TRIM) {
				*lbp++ = c;
				if (--lcnt < 0) {
					setname("<<");
					error("Line overflow");
				} 
			}
		}
		*lbp = 0;

		/*
		 * Compare to terminator -- before expansion
		 */
		if (eq(lbuf, term)) {
			(void) write(0, obuf, BUFSIZ - ocnt);
			(void) lseek(0, (off_t)0, 0);
			return;
		}

		/*
		 * If term was quoted or -n just pass it on
		 */
		if (quoted || noexec) {
			*lbp++ = '\n'; *lbp = 0;
			for (lbp = lbuf; c = *lbp++;) {
				*obp++ = c;
				if (--ocnt == 0) {
					(void) write(0, obuf, BUFSIZ);
					obp = obuf; ocnt = BUFSIZ;
				}
			}
			continue;
		}

		/*
		 * Term wasn't quoted so variable and then command
		 * expand the input line
		 */
		Dcp = lbuf; Dvp = Dv + 1; mbp = mbuf; mcnt = BUFSIZ - 4;
		for (;;) {
			c = DgetC(DODOL);
			if (c == DEOF)
				break;
			if ((c &= TRIM) == 0)
				continue;
			/* \ quotes \ $ ` here */
			if (c =='\\') {
				c = DgetC(0);
				if (!any(c, "$\\`"))
					unDgetC(c | QUOTE), c = '\\';
				else
					*mbp++ = QUOTECHAR, c |= QUOTE;	/* 003 - GAG */
			}
			*mbp++ = c & TRIM;	/* 003 - GAG */
			if (--mcnt == 0) {
				setname("<<");
				bferr("Line overflow");
			}
		}
		*mbp = 0;	/* 003 - GAG */

		/*
		 * If any ` in line do command substitution
		 */
		for (mbp = mbuf; *mbp; mbp++) {
			if ((*mbp & TRIM) == QUOTECHAR)
				mbp++;
			else if (any(*mbp, "`")) {
				/*
				 * 1 arg to dobackp causes substitution to be literal.
				 * Words are broken only at newlines so that all blanks
				 * and tabs are preserved.  Blank lines (null words)
				 * are not discarded.
				 */
				vp = dobackp(mbuf, 1);
				break;
			}
		}
		if (!*mbp) {	/* no ` in string; mbp points to NULL */
			mbp = mbuf;
			/* Setup trivial vector similar to return of dobackp */
			Dv[0] = mbp, Dv[1] = NOSTR, vp = Dv;
		}
		/*
		 * Resurrect the words from the command substitution
		 * each separated by a newline.  Note that the last
		 * newline of a command substitution will have been
		 * discarded, but we put a newline after the last word
		 * because this represents the newline after the last
		 * input line!
		 */
		trim (vp);	/* 003 - GAG */
		for (; *vp; vp++) {
			for (mbp = *vp; *mbp; mbp++) {
				*obp++ = *mbp & TRIM;
				if (--ocnt == 0) {
					(void) write(0, obuf, BUFSIZ);
					obp = obuf; ocnt = BUFSIZ;
				}
			}
			*obp++ = '\n';
			if (--ocnt == 0) {
				(void) write(0, obuf, BUFSIZ);
				obp = obuf; ocnt = BUFSIZ;
			}
		}
		if (pargv)
			blkfree(pargv), pargv = 0;
	}
}
