#ifndef lint
static  char    *sccsid = "@(#)sh.parse.c	4.2  (ULTRIX)        11/13/90";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1988 by				*
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

#include "sh.h"

/*
 * C shell
 * Modification History
 *
 * 005 - Bob Fontaine Mon Oct 22 13:46:48 EDT 1990
 *	Temporary work around for infinite alias loop problem.  Fix for
 *	004 below broke valid scipts so backed out changes and put in test
 *	for specific case in 004.  Still need a general fix for infinite 
 *	alias loop condition.  Fix for QAR #5985 and CLD 143.
 *
 * 004 - Gary A. Gaudet Tue Jan  2 11:53:45 EST 1990
 *	added some (castings)
 *	fixed u32_qar #00951 "alias a \!#" and "b;a" crashing
 *
 * 03	12-Nov-88, Al Delorey (afd).
 *	Use "ifdef CSHEDIT" around the cmd line edit code.
 *
 * 02	20-Sep-88, Al Delorey (afd).
 *	Added command line edit capability: call lex with 2nd arg
 *	   of 0 (meaning don't use editword to get words).
 *
 * 01 Sat Aug 13 15:28:57 EDT 1988, Gary A. Gaudet
 *	merging mips & ultrix for 8 bit clean and bug fixes
 */

/*
 * Perform aliasing on the word list lex
 * Do a (very rudimentary) parse to separate into commands.
 * If word 0 of a command has an alias, do it.
 * Repeat a maximum of 20 times.
 */

alias(lex)
register struct wordent *lex;
{
	jmp_buf osetexit;

	getexit(osetexit);
	setexit();
	if (haderr) {
		resexit(osetexit);
		reset();
	}
	asyntax(lex->next, lex);
	resexit(osetexit);
}

asyntax(p1, p2)
register struct wordent *p1, *p2;
{

register struct wordent *p3;
	int len;


	while (p1 != p2)
		if (any(p1->word[0], ";&\n"))
			p1 = p1->next;
		else {
			asyn0(p1, p2);
			return;
		}
}

asyn0(p1, p2)
	struct wordent *p1;
	register struct wordent *p2;
{
	register struct wordent *p;
	register int l = 0;

	for (p = p1; p != p2; p = p->next)
		switch (p->word[0]) {

		case '(':
			l++;
			continue;

		case ')':
			l--;
			if (l < 0)
				error("Too many )'s");
			continue;

		case '>':
			if (p->next != p2 && eq(p->next->word, "&"))
				p = p->next;
			continue;

		case '&':
		case '|':
		case ';':
		case '\n':
			if (l != 0)
				continue;
			asyn3(p1, p);
			asyntax(p->next, p2);
			return;

		}
	if (l == 0)
		asyn3(p1, p2);
}

asyn3(p1, p2)
	struct wordent *p1;
	register struct wordent *p2;
{
	register struct varent *ap;
	struct wordent alout;
	register bool redid;

	if (p1 == p2)
		return;

	if (p1->word[0] == '(') {
		for (p2 = p2->prev; p2->word[0] != ')'; p2 = p2->prev)
			if (p2 == p1)
				return;
		if (p2 == p1->next)
			return;
		asyn0(p1->next, p2);
		return;
	}
	ap = adrof1(p1->word, &aliases);
	if (ap == 0)
		return;

	if(strcmp(strip(ap->vec[0]),"!#") == 0)			/* 005 */
	{
		csh_printf("%s will cause alias loop.\n",p1->word);
		return;
	}
	alhistp = p1->prev;
	alhistt = p2;
	alvec = ap->vec;
#ifdef CSHEDIT
	redid = lex(&alout, 0);
#else
	redid = lex(&alout);
#endif
	alhistp = alhistt = 0;
	alvec = 0;
	if (err) {
		freelex(&alout);
		error(err);
	}
	if (p1->word[0] && eq(p1->word, alout.next->word)) {
		char *cp = alout.next->word;

		/*
		 * insert a quote character into word. 
		 */
		alout.next->word = strspl(DBL_QUOTE, cp);
		XFREE(cp)
	}
	p1 = freenod(p1, redid ? p2 : p1->next);
	if (alout.next != &alout) {
		p1->next->prev = alout.prev->prev;
		alout.prev->prev->next = p1->next;
		alout.next->prev = p1;
		p1->next = alout.next;
		XFREE(alout.prev->word)
		XFREE((char *)alout.prev)
	}
	reset();		/* throw! */
}

struct wordent *
freenod(p1, p2)
	register struct wordent *p1, *p2;
{
	register struct wordent *retp = p1->prev;

	while (p1 != p2) {
		XFREE(p1->word)
		p1 = p1->next;
		XFREE((char *)p1->prev)
	}
	retp->next = p2;
	p2->prev = retp;
	return (retp);
}

#define	PHERE	1
#define	PIN	2
#define	POUT	4
#define	PDIAG	8

/*
 * syntax
 *	empty
 *	syn0
 */
struct command *
syntax(p1, p2, flags)
	register struct wordent *p1, *p2;
	int flags;
{

	while (p1 != p2)
		if (any(p1->word[0], ";&\n"))
			p1 = p1->next;
		else
			return (syn0(p1, p2, flags));
	return (0);
}

/*
 * syn0
 *	syn1
 *	syn1 & syntax
 */
struct command *
syn0(p1, p2, flags)
	struct wordent *p1, *p2;
	int flags;
{
	register struct wordent *p;
	register struct command *t, *t1;
	int l;

	l = 0;
	for (p = p1; p != p2; p = p->next)
		switch (p->word[0]) {

		case '(':
			l++;
			continue;

		case ')':
			l--;
			if (l < 0)
				seterr("Too many )'s");
			continue;

		case '|':
			if (p->word[1] == '|')
				continue;
			/* fall into ... */

		case '>':
			if (p->next != p2 && eq(p->next->word, "&"))
				p = p->next;
			continue;

		case '&':
			if (l != 0)
				break;
			if (p->word[1] == '&')
				continue;
			t1 = syn1(p1, p, flags);
    			if (t1->t_dtyp == TLST ||
    			    t1->t_dtyp == TAND ||
    			    t1->t_dtyp == TOR) {
				t = (struct command *) calloc((unsigned)1, (unsigned)sizeof (*t));
				t->t_dtyp = TPAR;
				t->t_dflg = FAND|FINT;
				t->t_dspr = t1;
				t1 = t;
			} else
				t1->t_dflg |= FAND|FINT;
			t = (struct command *) calloc((unsigned)1, (unsigned)sizeof (*t));
			t->t_dtyp = TLST;
			t->t_dflg = 0;
			t->t_dcar = t1;
			t->t_dcdr = syntax(p, p2, flags);
			return(t);
		}
	if (l == 0)
		return (syn1(p1, p2, flags));
	seterr("Too many ('s");
	return (0);
}

/*
 * syn1
 *	syn1a
 *	syn1a ; syntax
 */
struct command *
syn1(p1, p2, flags)
	struct wordent *p1, *p2;
	int flags;
{
	register struct wordent *p;
	register struct command *t;
	int l;

	l = 0;
	for (p = p1; p != p2; p = p->next)
		switch (p->word[0]) {

		case '(':
			l++;
			continue;

		case ')':
			l--;
			continue;

		case ';':
		case '\n':
			if (l != 0)
				break;
			t = (struct command *) calloc((unsigned)1, (unsigned)sizeof (*t));
			t->t_dtyp = TLST;
			t->t_dcar = syn1a(p1, p, flags);
			t->t_dcdr = syntax(p->next, p2, flags);
			if (t->t_dcdr == 0)
				t->t_dcdr = t->t_dcar, t->t_dcar = 0;
			return (t);
		}
	return (syn1a(p1, p2, flags));
}

/*
 * syn1a
 *	syn1b
 *	syn1b || syn1a
 */
struct command *
syn1a(p1, p2, flags)
	struct wordent *p1, *p2;
	int flags;
{
	register struct wordent *p;
	register struct command *t;
	register int l = 0;

	for (p = p1; p != p2; p = p->next)
		switch (p->word[0]) {

		case '(':
			l++;
			continue;

		case ')':
			l--;
			continue;

		case '|':
			if (p->word[1] != '|')
				continue;
			if (l == 0) {
				t = (struct command *) calloc((unsigned)1, (unsigned)sizeof (*t));
				t->t_dtyp = TOR;
				t->t_dcar = syn1b(p1, p, flags);
				t->t_dcdr = syn1a(p->next, p2, flags);
				t->t_dflg = 0;
				return (t);
			}
			continue;
		}
	return (syn1b(p1, p2, flags));
}

/*
 * syn1b
 *	syn2
 *	syn2 && syn1b
 */
struct command *
syn1b(p1, p2, flags)
	struct wordent *p1, *p2;
	int flags;
{
	register struct wordent *p;
	register struct command *t;
	register int l = 0;

	l = 0;
	for (p = p1; p != p2; p = p->next)
		switch (p->word[0]) {

		case '(':
			l++;
			continue;

		case ')':
			l--;
			continue;

		case '&':
			if (p->word[1] == '&' && l == 0) {
				t = (struct command *) calloc((unsigned)1, (unsigned)sizeof (*t));
				t->t_dtyp = TAND;
				t->t_dcar = syn2(p1, p, flags);
				t->t_dcdr = syn1b(p->next, p2, flags);
				t->t_dflg = 0;
				return (t);
			}
			continue;
		}
	return (syn2(p1, p2, flags));
}

/*
 * syn2
 *	syn3
 *	syn3 | syn2
 *	syn3 |& syn2
 */
struct command *
syn2(p1, p2, flags)
	struct wordent *p1, *p2;
	int flags;
{
	register struct wordent *p, *pn;
	register struct command *t;
	register int l = 0;
	int f;

	for (p = p1; p != p2; p = p->next)
		switch (p->word[0]) {

		case '(':
			l++;
			continue;

		case ')':
			l--;
			continue;

		case '|':
			if (l != 0)
				continue;
			t = (struct command *) calloc((unsigned)1, (unsigned)sizeof (*t));
			f = flags | POUT;
			pn = p->next;
			if (pn != p2 && pn->word[0] == '&') {
				f |= PDIAG;
				t->t_dflg |= FDIAG;
			}
			t->t_dtyp = TFIL;
			t->t_dcar = syn3(p1, p, f);
			if (pn != p2 && pn->word[0] == '&')
				p = pn;
			t->t_dcdr = syn2(p->next, p2, flags | PIN);
			return (t);
		}
	return (syn3(p1, p2, flags));
}

char	*RELPAR =	"<>()";

/*
 * syn3
 *	( syn0 ) [ < in  ] [ > out ]
 *	word word* [ < in ] [ > out ]
 *	KEYWORD ( word* ) word* [ < in ] [ > out ]
 *
 *	KEYWORD = (@ exit foreach if set switch test while)
 */
struct command *
syn3(p1, p2, flags)
	struct wordent *p1, *p2;
	int flags;
{
	register struct wordent *p;
	struct wordent *lp, *rp;
	register struct command *t;
	register int l;
	char **av;
	int n, c;
	bool specp = 0;

	if (p1 != p2) {
		p = p1;
again:
		switch (srchx(p->word)) {

		case ZELSE:
			p = p->next;
			if (p != p2)
				goto again;
			break;

		case ZEXIT:
		case ZFOREACH:
		case ZIF:
		case ZLET:
		case ZSET:
		case ZSWITCH:
		case ZWHILE:
			specp = 1;
			break;
		}
	}
	n = 0;
	l = 0;
	for (p = p1; p != p2; p = p->next)
		switch (p->word[0]) {

		case '(':
			if (specp)
				n++;
			l++;
			continue;

		case ')':
			if (specp)
				n++;
			l--;
			continue;

		case '>':
		case '<':
			if (l != 0) {
				if (specp)
					n++;
				continue;
			}
			if (p->next == p2)
				continue;
			if (any(p->next->word[0], RELPAR))
				continue;
			n--;
			continue;

		default:
			if (!specp && l != 0)
				continue;
			n++;
			continue;
		}
	if (n < 0)
		n = 0;
	t = (struct command *) calloc((unsigned)1, (unsigned)sizeof (*t));
	av = (char **) calloc((unsigned) (n + 1), (unsigned)sizeof (char **));
	t->t_dcom = av;
	n = 0;
	if (p2->word[0] == ')')
		t->t_dflg = FPAR;
	lp = 0;
	rp = 0;
	l = 0;
	for (p = p1; p != p2; p = p->next) {
		c = p->word[0];
		switch (c) {

		case '(':
			if (l == 0) {
				if (lp != 0 && !specp)
					seterr("Badly placed (");
				lp = p->next;
			}
			l++;
			goto savep;

		case ')':
			l--;
			if (l == 0)
				rp = p;
			goto savep;

		case '>':
			if (l != 0)
				goto savep;
			if (p->word[1] == '>')
				t->t_dflg |= FCAT;
			if (p->next != p2 && eq(p->next->word, "&")) {
				t->t_dflg |= FDIAG, p = p->next;
				if (flags & (POUT|PDIAG))
					goto badout;
			}
			if (p->next != p2 && eq(p->next->word, "!"))
				t->t_dflg |= FANY, p = p->next;
			if (p->next == p2) {
missfile:
				seterr("Missing name for redirect");
				continue;
			}
			p = p->next;
			if (any(p->word[0], RELPAR))
				goto missfile;
			if ((flags & POUT) && (flags & PDIAG) == 0 || t->t_drit)
badout:
				seterr("Ambiguous output redirect");
			else
				t->t_drit = savestr(p->word);
			continue;

		case '<':
			if (l != 0)
				goto savep;
			if (p->word[1] == '<')
				t->t_dflg |= FHERE;
			if (p->next == p2)
				goto missfile;
			p = p->next;
			if (any(p->word[0], RELPAR))
				goto missfile;
			if ((flags & PHERE) && (t->t_dflg & FHERE))
				seterr("Can't << within ()'s");
			else if ((flags & PIN) || t->t_dlef)
				seterr("Ambiguous input redirect");
			else
				t->t_dlef = savestr(p->word);
			continue;

savep:
			if (!specp)
				continue;
		default:
			if (l != 0 && !specp)
				continue;
			if (err == 0)
				av[n] = savestr(p->word);
			n++;
			continue;
		}
	}
	if (lp != 0 && !specp) {
		if (n != 0)
			seterr("Badly placed ()'s");
		t->t_dtyp = TPAR;
		t->t_dspr = syn0(lp, rp, PHERE);
	} else {
		if (n == 0)
			seterr("Invalid null command");
		t->t_dtyp = TCOM;
	}
	return (t);
}

freesyn(t)
	register struct command *t;
{
	register char **v;

	if (t == 0)
		return;
	switch (t->t_dtyp) {

	case TCOM:
		for (v = t->t_dcom; *v; v++)
			XFREE(*v)
		XFREE((char *)t->t_dcom)
		goto lr;

	case TPAR:
		freesyn(t->t_dspr);
		/* fall into ... */

lr:
		XFREE(t->t_dlef)
		XFREE(t->t_drit)
		break;

	case TAND:
	case TOR:
	case TFIL:
	case TLST:
		freesyn(t->t_dcar), freesyn(t->t_dcdr);
		break;
	}
	XFREE((char *)t)
}
