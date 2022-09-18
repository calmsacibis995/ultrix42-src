#ifndef lint
static char *sccsid = "@(#)sh.func.c	4.2  (ULTRIX)        8/13/90";
#endif
/************************************************************************
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
/* $Header: sh.func.c,v 1.6 87/04/06 20:52:18 dce Exp $ */

#include "sh.h"
#include <sys/ioctl.h>

/*
 * C shell
 *
 * Modification History
 * 003 - Bob Fontaine - Fri Jun 22 09:53:01 EDT 1990
 *	 Changed call to internal printf  routine to csh_printf to avoid
 *	 conflict with stdio library call.
 *
 * 002 - Gary A. Gaudet - Tue Jan  2 11:29:20 EST 1990
 *	added a declaration of geteuid()
 *	added a return value test for ioctl()
 *	changed environ from common to extern
 *
 * 01 Sat Aug 13 15:28:57 EDT 1988, Gary A. Gaudet
 *	merging mips & ultrix for 8 bit clean and bug fixes
 */

struct biltins *
isbfunc(t)
	struct command *t;
{
	register char *cp = t->t_dcom[0];
	register struct biltins *bp, *bp1, *bp2;
	int dolabel(), dofg1(), dobg1();
	static struct biltins label = { "", dolabel, 0, 0 };
	static struct biltins foregnd = { "%job", dofg1, 0, 0 };
	static struct biltins backgnd = { "%job &", dobg1, 0, 0 };

	if (lastchr(cp) == ':') {
		label.bname = cp;
		return (&label);
	}
	if (*cp == '%') {
		if (t->t_dflg & FAND) {
			t->t_dflg &= ~FAND;
			backgnd.bname = cp;
			return (&backgnd);
		}
		foregnd.bname = cp;
		return (&foregnd);
	}
	/*
	 * Binary search
	 * Bp1 is the beginning of the current search range.
	 * Bp2 is one past the end.
	 */
	for (bp1 = bfunc, bp2 = bfunc + nbfunc; bp1 < bp2;) {
		register i;

		bp = bp1 + (bp2 - bp1 >> 1);
		if ((i = *cp - *bp->bname) == 0 &&
		    (i = strcmp(cp, bp->bname)) == 0)
			return bp;
		if (i < 0)
			bp2 = bp;
		else
			bp1 = bp + 1;
	}
	return (0);
}

func(t, bp)
	register struct command *t;
	register struct biltins *bp;
{
	int i;

	xechoit(t->t_dcom);
	setname(bp->bname);
	i = blklen(t->t_dcom) - 1;
	if (i < bp->minargs)
		bferr("Too few arguments");
	if (i > bp->maxargs)
		bferr("Too many arguments");
	(*bp->bfunct)(t->t_dcom, t);
}

dolabel()
{

}

doonintr(v)
	char **v;
{
	register char *cp;
	register char *vv = v[1];

	if (parintr == SIG_IGN)
		return;
	if (setintr && intty)
		bferr("Can't from terminal");
	cp = gointr, gointr = 0, xfree(cp);
	if (vv == 0) {
		if (setintr)
			(void) sigblock(sigmask(SIGINT));
		else
			(void) signal(SIGINT, SIG_DFL);
		gointr = 0;
	} else if (eq((vv = strip(vv)), "-")) {
		(void) signal(SIGINT, SIG_IGN);
		gointr = "-";
	} else {
		gointr = savestr(vv);
		(void) signal(SIGINT, pintr);
	}
}

donohup()
{

	if (intty)
		bferr("Can't from terminal");
	if (setintr == 0) {
		(void) signal(SIGHUP, SIG_IGN);
#ifdef CC
		submit(getpid());
#endif
	}
}

dozip()
{

	;
}

prvars()
{

	plist(&shvhed);
}

doalias(v)
	register char **v;
{
	register struct varent *vp;
	register char *p;

	v++;
	p = *v++;
	if (p == 0)
		plist(&aliases);
	else if (*v == 0) {
		vp = adrof1(strip(p), &aliases);
		if (vp)
			blkpr(vp->vec), csh_printf("\n");	/* 003 RNF */
	} else {
		if (eq(p, "alias") || eq(p, "unalias")) {
			setname(p);
			bferr("Too dangerous to alias that");
		}
		set1(strip(p), saveblk(v), &aliases);
	}
}

unalias(v)
	char **v;
{

	unset1(v, &aliases);
}

dologout()
{

	islogin();
	goodbye();
}

dologin(v)
	char **v;
{

	islogin();
	rechist();
	(void) signal(SIGTERM, parterm);
	execl("/bin/login", "login", v[1], 0);
	/*
	 * Since ports may be in high demand, it is much nicer to give the
	 * user the choice rather than drop them cause the exec failed.
	 */
	csh_printf("execl of /bin/login failed, logout and re-login!\n");	/* 003 RNF */
}

#ifdef NEWGRP
donewgrp(v)
	char **v;
{

	if (chkstop == 0 && setintr)
		panystop(0);
	(void) signal(SIGTERM, parterm);
	execl("/bin/newgrp", "newgrp", v[1], 0);
	execl("/usr/bin/newgrp", "newgrp", v[1], 0);
	untty();
	exit(1);
}
#endif

islogin()
{

	if (chkstop == 0 && setintr)
		panystop(0);
	if (loginsh)
		return;
	error("Not login shell");
}

doif(v, kp)
	char **v;
	struct command *kp;
{
	register int i;
	register char **vv;

	v++;
	i = exp(&v);
	vv = v;
	if (*vv == NOSTR)
		bferr("Empty if");
	if (eq(*vv, "then")) {
		if (*++vv)
			bferr("Improper then");
		setname("then");
		/*
		 * If expression was zero, then scan to else,
		 * otherwise just fall into following code.
		 */
		if (!i)
			search(ZIF, 0);
		return;
	}
	/*
	 * Simple command attached to this if.
	 * Left shift the node in this tree, munging it
	 * so we can reexecute it.
	 */
	if (i) {
		lshift(kp->t_dcom, vv - kp->t_dcom);
		reexecute(kp);
		donefds();
	}
}

/*
 * Reexecute a command, being careful not
 * to redo i/o redirection, which is already set up.
 */
reexecute(kp)
	register struct command *kp;
{

	kp->t_dflg &= FSAVE;
	kp->t_dflg |= FREDO;
	/*
	 * If tty is still ours to arbitrate, arbitrate it;
	 * otherwise dont even set pgrp's as the jobs would
	 * then have no way to get the tty (we can't give it
	 * to them, and our parent wouldn't know their pgrp, etc.
	 */
	execute(kp, tpgrp > 0 ? tpgrp : -1);
}

doelse()
{
	search(ZELSE, 0);
}

dogoto(v)
	char **v;
{
	register struct whyle *wp;
	char *lp;

	/*
	 * While we still can, locate any unknown ends of existing loops.
	 * This obscure code is the WORST result of the fact that we
	 * don't really parse.
	 */
	for (wp = whyles; wp; wp = wp->w_next)
		if (wp->w_end == 0) {
			search(ZBREAK, 0);
			wp->w_end = btell();
		} else
			bseek(wp->w_end);
	search(ZGOTO, 0, lp = globone(v[1]));
	xfree(lp);
	/*
	 * Eliminate loops which were exited.
	 */
	wfree();
}

doswitch(v)
	register char **v;
{
	register char *cp, *lp;

	v++;
	if (!*v || *(*v++) != '(')
		goto syntax;
	cp = **v == ')' ? "" : *v++;
	if (*(*v++) != ')')
		v--;
	if (*v)
syntax:
		error("Syntax error");
	search(ZSWITCH, 0, lp = globone(cp));
	xfree(lp);
}

dobreak()
{

	if (whyles)
		toend();
	else
		bferr("Not in while/foreach");
}

doexit(v)
	char **v;
{

	if (chkstop == 0)
		panystop(0);
	/*
	 * Don't DEMAND parentheses here either.
	 */
	v++;
	if (*v) {
		set("status", putn(exp(&v)));
		if (*v)
			bferr("Expression syntax");
	}
	btoeof();
	if (intty)
		(void) close(SHIN);
}

doforeach(v)
	register char **v;
{
	register char *cp;
	register struct whyle *nwp;

	v++;
	cp = strip(*v);
	/*
	 GT01:
	 The variable must have a valid alphanumeric form, with
	 a leading alphabetic to be accepted.
	*/
	if (!letter(*cp++))
		bferr("Invalid variable: no leading alphabetic");

	while (*cp && alnum(*cp))
		cp++;
	if (*cp)
		bferr ("Invalid variable: bad characters");

	if (strlen(*v) >= 20)
		bferr("Invalid variable: too long");

	cp = *v++;
	if (v[0][0] != '(' || v[blklen(v) - 1][0] != ')')
		bferr("Words not ()'ed");
	v++;
	gflag = 0, tglob(v);
	v = glob(v);
	if (v == 0)
		bferr("No match");
	nwp = (struct whyle *) calloc(1, sizeof *nwp);
	nwp->w_fe = nwp->w_fe0 = v; gargv = 0;
	nwp->w_start = btell();
	nwp->w_fename = savestr(cp);
	nwp->w_next = whyles;
	whyles = nwp;
	/*
	 * Pre-read the loop so as to be more
	 * comprehensible to a terminal user.
	 */
	if (intty)
		preread();
	doagain();
}

dowhile(v)
	char **v;
{
	register int status;
	register bool again = whyles != 0 && whyles->w_start == lineloc &&
	    whyles->w_fename == 0;

	v++;
	/*
	 * Implement prereading here also, taking care not to
	 * evaluate the expression before the loop has been read up
	 * from a terminal.
	 */
	if (intty && !again)
		status = !exp0(&v, 1);
	else
		status = !exp(&v);
	if (*v)
		bferr("Expression syntax");
	if (!again) {
		register struct whyle *nwp = (struct whyle *) calloc(1, sizeof (*nwp));

		nwp->w_start = lineloc;
		nwp->w_end = 0;
		nwp->w_next = whyles;
		whyles = nwp;
		if (intty) {
			/*
			 * The tty preread
			 */
			preread();
			doagain();
			return;
		}
	}
	if (status)
		/* We ain't gonna loop no more, no more! */
		toend();
}

preread()
{

	whyles->w_end = -1;
	if (setintr)
		(void) sigsetmask(sigblock(0) & ~sigmask(SIGINT));
	search(ZBREAK, 0);
	if (setintr)
		(void) sigblock(sigmask(SIGINT));
	whyles->w_end = btell();
}

doend()
{

	if (!whyles)
		bferr("Not in while/foreach");
	whyles->w_end = btell();
	doagain();
}

docontin()
{

	if (!whyles)
		bferr("Not in while/foreach");
	doagain();
}

doagain()
{

	/* Repeating a while is simple */
	if (whyles->w_fename == 0) {
		bseek(whyles->w_start);
		return;
	}
	/*
	 * The foreach variable list actually has a spurious word
	 * ")" at the end of the w_fe list.  Thus we are at the
	 * of the list if one word beyond this is 0.
	 */
	if (!whyles->w_fe[1]) {
		dobreak();
		return;
	}
	set(whyles->w_fename, savestr(*whyles->w_fe++));
	bseek(whyles->w_start);
}

dorepeat(v, kp)
	char **v;
	struct command *kp;
{
	register int i, omask;

	i = getn(v[1]);
	if (setintr)
		omask = sigblock(sigmask(SIGINT)) & ~sigmask(SIGINT);
	lshift(v, 2);
	while (i > 0) {
		if (setintr)
			(void) sigsetmask(omask);
		reexecute(kp);
		--i;
	}
	donefds();
	if (setintr)
		(void) sigsetmask(omask);
}

doswbrk()
{
	search(ZBRKSW, 0);
}

srchx(cp)
	register char *cp;
{
	register struct srch *sp, *sp1, *sp2;
	register i;

	/*
	 * Binary search
	 * Sp1 is the beginning of the current search range.
	 * Sp2 is one past the end.
	 */
	for (sp1 = srchn, sp2 = srchn + nsrchn; sp1 < sp2;) {
		sp = sp1 + (sp2 - sp1 >> 1);
		if ((i = *cp - *sp->s_name) == 0 &&
		    (i = strcmp(cp, sp->s_name)) == 0)
			return sp->s_value;
		if (i < 0)
			sp2 = sp;
		else
			sp1 = sp + 1;
	}
	return (-1);
}

char	Stype;
char	*Sgoal;

/*VARARGS2*/
search(type, level, goal)
	int type;
	register int level;
	char *goal;
{
	char wordbuf[BUFSIZ];
	register char *aword = wordbuf;
	register char *cp;

	Stype = type; Sgoal = goal;
	if (type == ZGOTO)
		bseek((off_t)0);
	do {
		if (intty && fseekp == feobp)
			csh_printf("? "), flush();		/* 003 RNF */
		aword[0] = 0;
		(void) getword(aword);
		switch (srchx(aword)) {

		case ZELSE:
			if (level == 0 && type == ZIF)
				return;
			break;

		case ZIF:
			while (getword(aword))
				continue;
			if ((type == ZIF || type == ZELSE) && eq(aword, "then"))
				level++;
			break;

		case ZENDIF:
			if (type == ZIF || type == ZELSE)
				level--;
			break;

		case ZFOREACH:
		case ZWHILE:
			if (type == ZBREAK)
				level++;
			break;

		case ZEND:
			if (type == ZBREAK)
				level--;
			break;

		case ZSWITCH:
			if (type == ZSWITCH || type == ZBRKSW)
				level++;
			break;

		case ZENDSW:
			if (type == ZSWITCH || type == ZBRKSW)
				level--;
			break;

		case ZLABEL:
			if (type == ZGOTO && getword(aword) && eq(aword, goal))
				level = -1;
			break;

		default:
			if (type != ZGOTO && (type != ZSWITCH || level != 0))
				break;
			if (lastchr(aword) != ':')
				break;
			aword[strlen(aword) - 1] = 0;
			if (type == ZGOTO && eq(aword, goal) || type == ZSWITCH && eq(aword, "default"))
				level = -1;
			break;

		case ZCASE:
			if (type != ZSWITCH || level != 0)
				break;
			(void) getword(aword);
			if (lastchr(aword) == ':')
				aword[strlen(aword) - 1] = 0;
			cp = strip(Dfix1(aword));
			if (Gmatch(goal, cp))
				level = -1;
			xfree(cp);
			break;

		case ZDEFAULT:
			if (type == ZSWITCH && level == 0)
				level = -1;
			break;
		}
		(void) getword(NOSTR);
	} while (level >= 0);
}

getword(wp)
	register char *wp;
{
	register int found = 0;
	register int c, d;

	c = readc(1);
	d = 0;
	do {
		while (c == ' ' || c == '\t' || c == '(')
			c = readc(1);
		if (c == '#')
			do
				c = readc(1);
			while (c >= 0 && c != '\n');
		if (c < 0)
			goto past;
		if (c == '\n') {
			if (wp)
				break;
			return (0);
		}
		unreadc(c);
		found = 1;
		do {
			c = readc(1);
			if (c == '\\' && (c = readc(1)) == '\n')
				c = ' ';
			if (c == '\'' || c == '"')
				if (d == 0)
					d = c;
				else if (d == c)
					d = 0;
			if (c < 0)
				goto past;
			if (wp)
				*wp++ = c;
						/* GT02: or at opening paren */
		} while ((d || c != ' ' && c != '\t' && c != '(') && c != '\n');
	} while (wp == 0);
	unreadc(c);
	if (found)
		*--wp = 0;
	return (found);

past:
	switch (Stype) {

	case ZIF:
		bferr("then/endif not found");

	case ZELSE:
		bferr("endif not found");

	case ZBRKSW:
	case ZSWITCH:
		bferr("endsw not found");

	case ZBREAK:
		bferr("end not found");

	case ZGOTO:
		setname(Sgoal);
		bferr("label not found");
	}
	/*NOTREACHED*/
}

toend()
{

	if (whyles->w_end == 0) {
		search(ZBREAK, 0);
		whyles->w_end = btell() - 1;
	} else
		bseek(whyles->w_end);
	wfree();
}

wfree()
{
	long o = btell();

	while (whyles) {
		register struct whyle *wp = whyles;
		register struct whyle *nwp = wp->w_next;

		if (o >= wp->w_start && (wp->w_end == 0 || o < wp->w_end))
			break;
		if (wp->w_fe0)
			blkfree(wp->w_fe0);
		if (wp->w_fename)
			xfree(wp->w_fename);
		xfree((char *)wp);
		whyles = nwp;
	}
}

doecho(v)
	char **v;
{

	echo(' ', v);
}

doglob(v)
	char **v;
{

	echo(0, v);
	flush();
}

echo(sep, v)
	char sep;
	register char **v;
{
	register char *cp;
	int nonl = 0;

	if (setintr)
		(void) sigsetmask(sigblock(0) & ~sigmask(SIGINT));
	v++;
	if (*v == 0)
		return;
	gflag = 0, tglob(v);
	if (gflag) {
		v = glob(v);
		if (v == 0)
			bferr("No match");
	}
	/*
	 * Don't trim quote bit in case we are repeating - rdoty@tek
	 * Repeat by "repeat 3 echo -n '/genvmu* '"
	 * 	should produce: /genvmu* /genvmu* /genvmu*
	 */
	if( sep == ' ' && *v 
	    && ( (*v)[0]&TRIM == '-' && (*v)[1]&TRIM == 'n' && (*v)[2]&TRIM == '\0' ) 
	    || *v && !strcmp(*v, "-n") )
		nonl++, v++;
	while (cp = *v++) {
		register int c;

		while (c = *cp++) {
			if ((c & TRIM) == QUOTECHAR) {
				c = *cp++;
			}
			putchar(c | QUOTE);
		}
		if (*v)
			putchar(sep | QUOTE);
	}
	if (sep && nonl == 0)
		putchar('\n');
	else
		flush();
	if (setintr)
		(void) sigblock(sigmask(SIGINT));
	if (gargv)
		blkfree(gargv), gargv = 0;
}

extern char	**environ;	/* 002 - GAG */

dosetenv(v)
	register char **v;
{
	char *vp, *lp;

	v++;
	if ((vp = *v++) == 0) {
		register char **ep;

		if (setintr)
			(void) sigsetmask(sigblock(0) & ~ sigmask(SIGINT));
		for (ep = environ; *ep; ep++)
			csh_printf("%s\n", *ep);		/* 003 RNF */
		return;
	}
	if ((lp = *v++) == 0)
		lp = "";
	setenv(vp, lp = globone(lp));
	if (eq(vp, "PATH")) {
		importpath(lp);
		dohash();
	}
	xfree(lp);
}

dounsetenv(v)
	register char **v;
{

	v++;
	do
		unsetenv(*v++);
	while (*v);
}

setenv(name, val)
	char *name, *val;
{
	register char **ep = environ;
	register char *cp, *dp;
	char *blk[2], **oep = ep;

	for (; *ep; ep++) {
		for (cp = name, dp = *ep; *cp && *cp == *dp; cp++, dp++)
			continue;
		if (*cp != 0 || *dp != '=')
			continue;
		cp = strspl("=", val);
		xfree(*ep);
		*ep = strspl(name, cp);
		xfree(cp);
		trim(ep);	/* GAG - fix "setenv a $<" */
		return;
	}
	blk[0] = strspl(name, "="); blk[1] = 0;
	environ = blkspl(environ, blk);
	xfree((char *)oep);
	setenv(name, val);
}

unsetenv(name)
	char *name;
{
	register char **ep = environ;
	register char *cp, *dp;
	char **oep = ep;

	for (; *ep; ep++) {
		for (cp = name, dp = *ep; *cp && *cp == *dp; cp++, dp++)
			continue;
		if (*cp != 0 || *dp != '=')
			continue;
		cp = *ep;
		*ep = 0;
		environ = blkspl(environ, ep+1);
		*ep = cp;
		xfree(cp);
		xfree((char *)oep);
		return;
	}
}

doumask(v)
	register char **v;
{
	register char *cp = v[1];
	register int i;

	if (cp == 0) {
		i = umask(0);
		(void) umask(i);
		csh_printf("%o\n", i);				/* 003 RNF  */
		return;
	}
	i = 0;
	while (digit(*cp) && *cp != '8' && *cp != '9')
		i = i * 8 + *cp++ - '0';
	if (*cp || i < 0 || i > 0777)
		bferr("Improper mask");
	(void) umask(i);
}


struct limits {
	int	limconst;
	char	*limname;
	int	limdiv;
	char	*limscale;
} limits[] = {
	RLIMIT_CPU,	"cputime",	1,	"seconds",
	RLIMIT_FSIZE,	"filesize",	1024,	"kbytes",
	RLIMIT_DATA,	"datasize",	1024,	"kbytes",
	RLIMIT_STACK,	"stacksize",	1024,	"kbytes",
	RLIMIT_CORE,	"coredumpsize",	1024,	"kbytes",
	RLIMIT_RSS,	"memoryuse",	1024,	"kbytes",
	-1,		0,
};

struct limits *
findlim(cp)
	char *cp;
{
	register struct limits *lp, *res;

	res = 0;
	for (lp = limits; lp->limconst >= 0; lp++)
		if (prefix(cp, lp->limname)) {
			if (res)
				bferr("Ambiguous");
			res = lp;
		}
	if (res)
		return (res);
	bferr("No such limit");
	/*NOTREACHED*/
}

dolimit(v)
	register char **v;
{
	register struct limits *lp;
	register int limit;
	char hard = 0;

	v++;
	if (*v && eq(*v, "-h")) {
		hard = 1;
		v++;
	}
	if (*v == 0) {
		for (lp = limits; lp->limconst >= 0; lp++)
			plim(lp, hard);
		return;
	}
	lp = findlim(v[0]);
	if (v[1] == 0) {
		plim(lp,  hard);
		return;
	}
	limit = getval(lp, v+1);
	if (setlim(lp, hard, limit) < 0)
		error(NOSTR);
}

getval(lp, v)
	register struct limits *lp;
	char **v;
{
	register float f;
	double atof();
	char *cp = *v++;

	f = atof(cp);
	while (digit(*cp) || *cp == '.' || *cp == 'e' || *cp == 'E')
		cp++;
	if (*cp == 0) {
		if (*v == 0)
			return ((int)(f+0.5) * lp->limdiv);
		cp = *v;
	}
	switch (*cp) {

	case ':':
		if (lp->limconst != RLIMIT_CPU)
			goto badscal;
		return ((int)(f * 60.0 + atof(cp+1)));

	case 'h':
		if (lp->limconst != RLIMIT_CPU)
			goto badscal;
		limtail(cp, "hours");
		f *= 3600.;
		break;

	case 'm':
		if (lp->limconst == RLIMIT_CPU) {
			limtail(cp, "minutes");
			f *= 60.;
			break;
		}
	case 'M':
		if (lp->limconst == RLIMIT_CPU)
			goto badscal;
		*cp = 'm';
		limtail(cp, "megabytes");
		f *= 1024.*1024.;
		break;

	case 's':
		if (lp->limconst != RLIMIT_CPU)
			goto badscal;
		limtail(cp, "seconds");
		break;

	case 'k':
		if (lp->limconst == RLIMIT_CPU)
			goto badscal;
		limtail(cp, "kbytes");
		f *= 1024;
		break;

	case 'u':
		limtail(cp, "unlimited");
		return (RLIM_INFINITY);

	default:
badscal:
		bferr("Improper or unknown scale factor");
	}
	return ((int)(f+0.5));
}

limtail(cp, str0)
	char *cp, *str0;
{
	register char *str = str0;

	while (*cp && *cp == *str)
		cp++, str++;
	if (*cp)
		error("Bad scaling; did you mean ``%s''?", str0);
}

plim(lp, hard)
	register struct limits *lp;
	char hard;
{
	struct rlimit rlim;
	int limit;

	csh_printf("%s \t", lp->limname);			/* 003 RNF */
	(void) getrlimit(lp->limconst, &rlim);
	limit = hard ? rlim.rlim_max : rlim.rlim_cur;
	if (limit == RLIM_INFINITY)
		csh_printf("unlimited");			/* 003 RNF */
	else if (lp->limconst == RLIMIT_CPU)
		psecs((long)limit);
	else
		csh_printf("%d %s", limit / lp->limdiv, lp->limscale);/* 003 RNF */
	csh_printf("\n");					/* 003 RNF */
}

dounlimit(v)
	register char **v;
{
	register struct limits *lp;
	int err = 0;
	char hard = 0;

	v++;
	if (*v && eq(*v, "-h")) {
		hard = 1;
		v++;
	}
	if (*v == 0) {
		for (lp = limits; lp->limconst >= 0; lp++)
			if (setlim(lp, hard, (int)RLIM_INFINITY) < 0)
				err++;
		if (err)
			error(NOSTR);
		return;
	}
	while (*v) {
		lp = findlim(*v++);
		if (setlim(lp, hard, (int)RLIM_INFINITY) < 0)
			error(NOSTR);
	}
}

setlim(lp, hard, limit)
	register struct limits *lp;
	char hard;
{
	extern	uid_t geteuid();	/* 002 - GAG */
	struct rlimit rlim;

	(void) getrlimit(lp->limconst, &rlim);
	if (hard)
		rlim.rlim_max = limit;
  	else if (limit == RLIM_INFINITY && (int) geteuid() != 0)	/* 002 - GAG */
 		rlim.rlim_cur = rlim.rlim_max;
 	else
 		rlim.rlim_cur = limit;
	if (setrlimit(lp->limconst, &rlim) < 0) {		/* 003 RNF */
		csh_printf("%s: %s: Can't %s%s limit\n", bname, lp->limname,
		    limit == RLIM_INFINITY ? "remove" : "set",
		    hard ? " hard" : "");
		return (-1);
	}
	return (0);
}

dosuspend()
{
	int ldisc, ctpgrp;
	void (*old)();

	if (loginsh)
		error("Can't suspend a login shell (yet)");
	untty();
	old = signal(SIGTSTP, SIG_DFL);
	(void) kill(0, SIGTSTP);
	/* the shell stops here */
	(void) signal(SIGTSTP, old);
	if (tpgrp != -1) {
retry:
		if (ioctl(FSHTTY, TIOCGPGRP, (char *)&ctpgrp)) {	/* 002 - GAG */
			Perror ("ioctl");
		}
		if (ctpgrp != opgrp) {
			old = signal(SIGTTIN, SIG_DFL);
			(void) kill(0, SIGTTIN);
			(void) signal(SIGTTIN, old);
			goto retry;
		}
		(void) ioctl(FSHTTY, TIOCSPGRP, (char *)&shpgrp);
		(void) setpgrp(0, shpgrp);
	}
	(void) ioctl(FSHTTY, TIOCGETD, (char *)&oldisc);
	if (oldisc != NTTYDISC) {
		csh_printf("Switching to new tty driver...\n");   /* 003 RNF */
		ldisc = NTTYDISC;
		(void) ioctl(FSHTTY, TIOCSETD, (char *)&ldisc);
	}
}

doeval(v)
	char **v;
{
	char **oevalvec = evalvec;
	char *oevalp = evalp;
	jmp_buf osetexit;
	int reenter;
	char **gv = 0;

	v++;
	if (*v == 0)
		return;
	gflag = 0, tglob(v);
	if (gflag) {
		gv = v = glob(v);
		gargv = 0;
		if (v == 0)
			error("No match");
		v = copyblk(v);
	} else
		trim(v);
	getexit(osetexit);
	reenter = 0;
	setexit();
	reenter++;
	if (reenter == 1) {
		evalvec = v;
		evalp = 0;
		process(0);
	}
	evalvec = oevalvec;
	evalp = oevalp;
	doneinp = 0;
	if (gv)
		blkfree(gv);
	resexit(osetexit);
	if (reenter >= 2)
		error(NOSTR);
}


/*
 * After much disucussion, it was decided to have this fast clean internal
 * version of 'which' produce the same ugly output as it slower counterpart
 * /usr/ucb/which.  A -u (useful) flag was added to allow a user to alias
 * which to which -u for a cleaner more useful form of the command.
 * tonyb@tek
 */
dowhich(vec)
	char **vec;
{
	register struct biltins *bp;
	register char *word, **pp;	/* GAG - don't need func */
	struct varent *vp, *pth;
	char dir[MAXPATHLEN+1];
	int cmp, hit, outty, uflag;


	/*
	 * Glob it up
	 */
	uflag = gflag = 0;
	tglob(++vec);
	if(gflag) { 			/* has globbing chars */
		vec = glob(vec);	/* glob it */
		if(vec == 0)
			bferr("No match");
	} else
		trim( vec );

	pth = adrof("path");

	/*
	 * Find out if we're writting to a tty.  If not, use "command type"
	 * output, else use "user" type output.
	 * command type output is used to do things like 'echo `which foobar`'
	 */
	outty = isatty(1);

	/*
	 * If no -u, go into useless mode, else provide useable,
	 * coherent output.
	 */
	if(eq(*vec,"-u")) {
		uflag++;
		vec++;
	}

	while(word = *vec++) {

		/*
		 * check aliases first.  
		 */
		vp = adrof1(word, &aliases);
		if(vp) {
			if(!uflag) {
				csh_printf("%s: \t aliased to '", word); /* 003 RNF */
				blkpr(vp->vec);csh_printf("'\n"); /* 003 RNF */
			} else if(outty) {
				csh_printf("alias/%s '", word); /* 003 RNF */
				blkpr(vp->vec); csh_printf("'\n"); /* 003 RNF */
			} else
				csh_printf("%s\n",word); /* 003 RNF */
			continue;
		}

		/*
		 * If a hard path, just return it if it is executable, else
		 * return nothing.
		 */
		if(index(word, '/') ) {
			if(executable(word)) {
				csh_printf("%s\n",word);	/* 003 RNF */
				continue;
			}
			else if( !uflag ) {
				csh_printf("%s not found\n", word); /* 003 RNF */
				continue;
			}
		}

		/*
		 * Check builtins
		 * "standard" /usr/ucb/which does not support the concept
		 * of builtin commands. YUK!!!
		 */
		if(uflag) {
			hit = 0;
/*
 *			GAG - the list of bfunc->bname is not null terminated
 */
			for (bp = bfunc + nbfunc - 1; bp >= bfunc; bp--) {
				if ((cmp = strcmp (bp->bname, word)) == 0) {
					if (outty)
						csh_printf ("builtin/%s\n",word); /* 003 RNF */
					else
						csh_printf ("%s\n",word); /* 003 RNF */
					hit++;
				} else if (cmp < 0)
					break;
			}
			if(hit) 
				continue;
		}


		/*
		 * Check the paths
		 * No need to read 'em, just check if it is executable.
		 */
		if(pth) {
			register char *ep;

			hit = 0;
			for(pp = pth->vec; pp && *pp && **pp; pp++) {
				ep = strcpy(dir,*pp);
				while(*ep)
					ep++;
				*ep++ = '/';
				strcpy(ep,word);
				if(executable(dir)) {
					csh_printf("%s\n",dir); /* 003 RNF */
					hit++;
					break;
				}
			}
		}

		/*
		 * If not found and in useless mode
		 */
		if(!hit && !uflag) {
			csh_printf("no %s in ", word);  /* 003 RNF */
			blkpr(pth->vec);
			csh_printf("\n");		/* 003 RNF */
		}
	} /* end while */
}


executable(path)
	register char *path;
{
	struct stat st;

	if(access(path,1) || stat(path, &st) )
		st.st_mode = 0;
	return( (st.st_mode & S_IFMT) == S_IFREG);
}
