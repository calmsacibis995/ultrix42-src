#ifndef lint
static	char	*sccsid = "@(#)sh.glob.c	4.1	(ULTRIX)	7/17/90";
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
#include <sys/dir.h>

/*
 * C Shell
 * Modification History
 *
 * 006 - Bob Fontaine and Gary A. Gaudet - Fri May 18 1990
 *	Backed out fix in 004 below for QAR #00649 because the fix introduced
 *	a bug in that *.* also matched all . files include . and ..
 *
 * 005 - Gary A. Gaudet - Wed Jan 31 13:22:30 EST 1990
 *	Fixed escaping command substitution and variable expansion in heredoc
 *
 * 004	Mon Dec 18 15:41:53 EST 1989, Gary A. Gaudet
 *	fixed u32_qar #00664: "set a '`pwd`'; echo `a`" dumps core
 *	fixed u32_qar #00649: "echo \.*" doesn't match files
 *
 * 003	12-Nov-88, Al Delorey (afd).
 *	Use "ifdef CSHEDIT" around the cmd line edit code.
 *
 * 002	20-Sep-88, Al Delorey (afd).
 *	Added command line edit capability: call lex with 2nd
 *	   arg of 0 (meaning dont't use editword to get words).
 *
 * 001 Sat Aug 13 15:28:57 EDT 1988, Gary A. Gaudet
 *	merging mips & ultrix for 8 bit clean and bug fixes
 */

int	globcnt;

char	*globchars =	"`{[*?";

char	*gpath, *gpathp, *lastgpathp;
int	globbed;
bool	noglob;
bool	nonomatch;
char	*entp;
char	**sortbas;
int	sortscmp();

#define sort()	qsort((char *)sortbas, &gargv[gargc] - sortbas, \
		      sizeof(*sortbas), sortscmp), sortbas = &gargv[gargc]


char **
glob(v)
	register char **v;
{
	char agpath[BUFSIZ];
	char *agargv[GAVSIZ];

	gpath = agpath;
	gpathp = gpath;
	*gpathp = 0;
	lastgpathp = &gpath[sizeof agpath - 2];
	ginit(agargv);
	globcnt = 0;
#ifdef GDEBUG
	printf("glob entered: "); blkpr(v); printf("\n");
#endif
	noglob = adrof("noglob") != 0;
	nonomatch = adrof("nonomatch") != 0;
	globcnt = noglob | nonomatch;
	while (*v)
		collect(*v++);
#ifdef GDEBUG
	printf("glob done, globcnt=%d, gflag=%d: ", globcnt, gflag); blkpr(gargv); printf("\n");
#endif
	if (globcnt == 0 && (gflag&1)) {
		blkfree(gargv), gargv = 0;
		return (0);
	} else
		return (gargv = copyblk(gargv));
}

ginit(agargv)
	char **agargv;
{

	agargv[0] = 0;
	gargv = agargv;
	sortbas = agargv;
	gargc = 0;
	gnleft = NCARGS - 4;
}

collect(as)
	register char *as;
{
	register int i;

	if (any('`', as)) {
#ifdef GDEBUG
		printf("doing backp of %s\n", as);
#endif
		(void) dobackp(as, 0);
#ifdef GDEBUG
		printf("backp done, acollect'ing\n");
#endif
		for (i = 0; i < pargc; i++)
			if (noglob) {
				Gcat(pargv[i], "");
				sortbas = &gargv[gargc];
			} else
				acollect(pargv[i]);
		if (pargv)
			blkfree(pargv), pargv = 0;
#ifdef GDEBUG
		printf("acollect done\n");
#endif
	} else if (noglob || eq(as, "{") || eq(as, "{}")) {
		Gcat(as, "");
		sort();
	} else
		acollect(as);
}

acollect(as)
	register char *as;
{
	register int ogargc = gargc;

	gpathp = gpath;
	*gpathp = 0;
	globbed = 0;
	expand(as);
	if (gargc == ogargc) {
		if (nonomatch) {
			Gcat(as, "");
			sort();
		}
	} else
		sort();
}

/*
 * String compare for qsort.  Also used by filec code in sh.file.c.
 */
sortscmp(a1, a2)
	char **a1, **a2;
{

	 return (strcmp(*a1, *a2));
}

expand(as)
	char *as;
{
	register char *cs;
	register char *sgpathp, *oldcs;
	struct stat stb;

	sgpathp = gpathp;
	cs = as;
	if (*cs == '~' && gpathp == gpath) {
		addpath('~');
		for (cs++; letter(*cs) || digit(*cs) || *cs == '-';)
			addpath(*cs++);
		if (!*cs || *cs == '/') {
			if (gpathp != gpath + 1) {
				*gpathp = 0;
				if (gethdir(gpath + 1))
					error("Unknown user: %s", gpath + 1);
				(void) strcpy(gpath, gpath + 1);
			} else
				(void) strcpy(gpath, value("home"));
			gpathp = strend(gpath);
		}
	}
	while (!any(((*cs & TRIM) == QUOTECHAR ? (*++cs & TRIM) | QUOTE : *cs), globchars)) {
		if (*cs == 0) {
			if (!globbed)
				Gcat(gpath, "");
			else if (stat(gpath, &stb) >= 0) {
				Gcat(gpath, "");
				globcnt++;
			}
			goto endit;
		}
		addpath(*cs++);
	}
	oldcs = cs;
	while (cs > as && *cs != '/') {
		if ((*cs & TRIM) != QUOTECHAR)  /* 004 - GAG */
			gpathp--;
		cs--;
	}
	if ((*cs & TRIM) == QUOTECHAR)
		gpathp++;

	if (*cs == '/')
		cs++, gpathp++;
	*gpathp = 0;
	if (*oldcs == '{') {
		(void) execbrc(cs, NOSTR);
		return;
	}
	matchdir(cs);
endit:
	gpathp = sgpathp;
	*gpathp = 0;
}

matchdir(pattern)
	char *pattern;
{
	struct stat stb;
	register struct direct *dp;
	register DIR *dirp;

	dirp = opendir(gpath);
	if (dirp == NULL) {
		if (globbed)
			return;
		goto patherr2;
	}
	if (fstat(dirp->dd_fd, &stb) < 0)
		goto patherr1;
	if (!isdir(stb)) {
		errno = ENOTDIR;
		goto patherr1;
	}
	while ((dp = readdir(dirp)) != NULL) {
		if (dp->d_ino == 0)
			continue;
		if (match(dp->d_name, pattern)) {
			Gcat(gpath, dp->d_name);
			globcnt++;
		}
	}
	closedir(dirp);
	return;

patherr1:
	closedir(dirp);
patherr2:
	Perror(gpath);
}

execbrc(p, s)
	char *p, *s;
{
	char restbuf[BUFSIZ + 2];
	register char *pe, *pm, *pl;
	int brclev = 0;
	char *lm, savec, *sgpathp;
	int flag;

	for (lm = restbuf; *p != '{'; *lm++ = *p++)
		continue;
	for (pe = ++p; *pe; pe++)
	switch (*pe) {

	case '{':
		brclev++;
		continue;

	case '}':
		if (brclev == 0)
			goto pend;
		brclev--;
		continue;

	case '[':
		for (pe++; *pe && *pe != ']'; pe++)
			continue;
		if (!*pe)
			error("Missing ]");
		continue;
	}
pend:
	if (brclev || !*pe)
		error("Missing }");
	for (pl = pm = p; pm <= pe; pm++) {
	flag = 0;
	if ((*pm  & TRIM) == QUOTECHAR) {
		pm++;
		flag = QUOTE;
	}
	switch ((*pm & TRIM) | flag) {

	case '{':
		brclev++;
		continue;

	case '}':
		if (brclev) {
			brclev--;
			continue;
		}
		goto doit;

	case ','|QUOTE:
	case ',':
		if (brclev)
			continue;
doit:
		savec = *pm;
		*pm = 0;
		(void) strcpy(lm, pl);
		(void) strcat(restbuf, pe + 1);
		*pm = savec;
		if (s == 0) {
			sgpathp = gpathp;
			expand(restbuf);
			gpathp = sgpathp;
			*gpathp = 0;
		} else if (amatch(s, restbuf))
			return (1);
		sort();
		pl = pm + 1;
		continue;

	case '[':
		for (pm++; *pm && *pm != ']'; pm++)
			continue;
		if (!*pm)
			error("Missing ]");
		continue;
	}}
	return (0);
}

match(s, p)
	char *s, *p;
{
	register int c;
	register char *sentp;
	char sglobbed = globbed;

	if (*s == '.' && *p != '.')
		return (0);
	sentp = entp;
	entp = s;
	c = amatch(s, p);
	entp = sentp;
	globbed = sglobbed;
	return (c);
}

amatch(s, p)
	register char *s, *p;
{
	register int scc;
	int ok, lc;
	char *sgpathp;
	struct stat stb;
	int c, cc;

	globbed = 1;
	for (;;) {
		if ((scc = (*s++ & TRIM)) == QUOTECHAR)
			scc = *s++ & TRIM;
		switch (c = (*p++ & TRIM)) {

		case '{':
			return (execbrc(p - 1, s - 1));

		case '[':
			ok = 0;
			lc = 077777;
			while (cc = *p++) {
				if (cc == ']') {
					if (ok)
						break;
					return (0);
				}
				if (cc == '-') {
					if (lc <= scc && scc <= *p++)
						ok++;
				} else
					if (scc == (lc = cc))
						ok++;
			}
			if (cc == 0)
				error("Missing ]");
			continue;

		case '*':
			if (!*p)
				return (1);
			if (*p == '/') {
				p++;
				goto slash;
			}
			for (s--; *s; s++)
				if (amatch(s, p))
					return (1);
			return (0);

		case 0:
			return (scc == 0);

		default:
			if ((c & TRIM) != scc)
				return (0);
			continue;

		case '?':
			if (scc == 0)
				return (0);
			continue;

		case '/':
			if (scc)
				return (0);
slash:
			s = entp;
			sgpathp = gpathp;
			while (*s)
				addpath(*s++);
			addpath('/');
			if (stat(gpath, &stb) == 0 && isdir(stb))
				if (*p == 0) {
					Gcat(gpath, "");
					globcnt++;
				} else
					expand(p);
			gpathp = sgpathp;
			*gpathp = 0;
			return (0);
		}
	}
}

Gmatch(s, p)
	register char *s, *p;
{
	register int scc;
	int ok, lc;
	int c, cc;

	for (;;) {
		if ((scc = (*s++ & TRIM)) == QUOTECHAR)
			scc = *s++;
		if ((c = (*p++ & TRIM)) == QUOTECHAR)
			c = (*p++ & TRIM) | QUOTE;
		switch (c) {

		case '[':
			ok = 0;
			lc = 077777;
			while (cc = *p++) {
				if (cc == ']') {
					if (ok)
						break;
					return (0);
				}
				if (cc == '-') {
					if (lc <= scc && scc <= *p++)
						ok++;
				} else
					if (scc == (lc = cc))
						ok++;
			}
			if (cc == 0)
				bferr("Missing ]");
			continue;

		case '*':
			if (!*p)
				return (1);
			for (s--; *s; s++)
				if (Gmatch(s, p))
					return (1);
			return (0);

		case 0:
			return (scc == 0);

		default:
			if ((c & TRIM) != scc)
				return (0);
			continue;

		case '?':
			if (scc == 0)
				return (0);
			continue;

		}
	}
}

Gcat(s1, s2)
	char *s1, *s2;
{
	register char *p, *q;
	int n;

	for (p = s1; *p++;)
		;
	for (q = s2; *q++;)
		;
	gnleft -= (n = (p - s1) + (q - s2) - 1);
	if (gnleft <= 0 || ++gargc >= GAVSIZ)
		error("Arguments too long");
	gargv[gargc] = 0;
	p = gargv[gargc - 1] = xalloc((unsigned)n);
	for (q = s1; *p++ = *q++;)
		;
	for (p--, q = s2; *p++ = *q++;)
		;
}

addpath(c)
	char c;
{

	if (gpathp >= lastgpathp)
		error("Pathname too long");
	*gpathp++ = c & TRIM;
	*gpathp = 0;
}

rscan(t, f)
	register char **t;
	int (*f)();
{
	register char *p;
	register CHTYPE c;

	while (p = *t++) {
		while (c = *p++) {
			if ((c & TRIM) == QUOTECHAR)
				c = (*p++ & TRIM) | QUOTE;
			(*f)(c);
		}
	}
}

trim(t)
	register char **t;
{
	register char *p;

	while (p = *t++)
		strip (p);
}

/*
 * tests for file expansion characters
 */
tglob(t)
	register char **t;
{
	register char *p;
	register CHTYPE c;

#ifdef TGLOBDEBUG
	printf ("TGLOB: entered; *t = %s\n", *t);
#endif
	while (p = *t++) {
		if (*p == '~')
			gflag |= 2;
		else if (*p == '{' && (p[1] == '\0' || p[1] == '}' && p[2] == '\0'))
			continue;
		while (c = (unsigned char) *p++) {
			if ((c & TRIM) == QUOTECHAR)
				c = (*p++ & TRIM) | QUOTE;
			else if (any(c, globchars))
				gflag |= c == '{' ? 2 : 1;
		}
	}
#ifdef TGLOBDEBUG
	printf ("TGLOB: exiting; *t = %s\n", *t);
#endif
}

char *
globone(str)
	register char *str;
{
	char *gv[2];
	register char **gvp;
	register char *cp;

	gv[0] = str;
	gv[1] = 0;
	gflag = 0;
	tglob(gv);
	if (gflag) {
		gvp = glob(gv);
		if (gvp == 0) {
			setname(str);
			bferr("No match");
		}
		cp = *gvp++;
		if (cp == 0)
			cp = "";
		else if (*gvp) {
			setname(str);
			bferr("Ambiguous");
		} else
			cp = strip(cp);
/*
		if (cp == 0 || *gvp) {
			setname(str);
			bferr(cp ? "Ambiguous" : "No output");
		}
*/
		xfree((char *)gargv); gargv = 0;
	} else {
		trim(gv);
		cp = savestr(gv[0]);
	}
	return (cp);
}

/*
 * Command substitute cp.  If literal, then this is
 * a substitution from a << redirection, and so we should
 * not crunch blanks and tabs, separating words only at newlines.
 */
char **
dobackp(cp, literal)
	char *cp;
	bool literal;
{
	register char *lp, *rp;
	char *ep;
	char word[BUFSIZ];
	char *apargv[GAVSIZ + 2];

	if (pargv) {
		blkfree(pargv);
	}
	pargv = apargv;
	pargv[0] = NOSTR;
	pargcp = pargs = word;
	pargc = 0;
	pnleft = BUFSIZ - 4;
	for (;;) {
		for (lp = cp; *lp != '`' || (*(lp-1)&TRIM) == QUOTECHAR; lp++) {	/* 005 - GAG */
			if (*lp == 0) {
				if (pargcp != pargs)
					pword();
#ifdef GDEBUG
				printf("leaving dobackp\n");
#endif
				return (pargv = copyblk(pargv));
			}
			psave(*lp & TRIM);
		}
		lp++;
		for (rp = lp; *rp && *rp != '`' || (*(rp-1)&TRIM) == QUOTECHAR; rp++)	/* 005 - GAG */
			if (*rp == '\\') {
				rp++;
				if (!*rp)
					goto oops;
			}
		if (!*rp)
oops:
			error("Unmatched `");
		ep = savestr(lp);
		ep[rp - lp] = 0;
		backeval(ep, literal);
#ifdef GDEBUG
		printf("back from backeval\n");
#endif
		cp = rp + 1;
	}
}

backeval(cp, literal)
	char *cp;
	bool literal;
{
	int pvec[2];
	int quoted = (literal || ((cp[0] & TRIM) == QUOTECHAR)) ? QUOTE : 0;
	char ibuf[BUFSIZ];
	register int icnt = 0, c;
	register char *ip;
	bool hadnl = 0;
	char *fakecom[2];
	struct command faket;

	faket.t_dtyp = TCOM;
	faket.t_dflg = 0;
	faket.t_dlef = 0;
	faket.t_drit = 0;
	faket.t_dspr = 0;
	faket.t_dcom = fakecom;
	fakecom[0] = "` ... `";
	fakecom[1] = 0;
	/*
	 * We do the psave job to temporarily change the current job
	 * so that the following fork is considered a separate job.
	 * This is so that when backquotes are used in a
	 * builtin function that calls glob the "current job" is not corrupted.
	 * We only need one level of pushed jobs as long as we are sure to
	 * fork here.
	 */
	psavejob();
	/*
	 * It would be nicer if we could integrate this redirection more
	 * with the routines in sh.sem.c by doing a fake execute on a builtin
	 * function that was piped out.
	 */
	mypipe(pvec);
	if (pfork(&faket, -1) == 0) {
		struct wordent paraml;
		struct command *t;

		(void) close(pvec[0]);
		(void) dmove(pvec[1], 1);
		(void) dmove(SHDIAG, 2);
		initdesc();
		arginp = cp;
		strip(cp);
#ifdef CSHEDIT
		(void) lex(&paraml, 0);
#else
		(void) lex(&paraml);
#endif
		if (err)
			error(err);
		alias(&paraml);
		t = syntax(paraml.next, &paraml, 0);
		if (err)
			error(err);
		if (t)
			t->t_dflg |= FPAR;
		(void) signal(SIGTSTP, SIG_IGN);
		(void) signal(SIGTTIN, SIG_IGN);
		(void) signal(SIGTTOU, SIG_IGN);
		execute(t, -1);
		exitstat();
	}
	xfree(cp);
	(void) close(pvec[1]);
	do {
		int cnt = 0;
		for (;;) {
			if (icnt == 0) {
				ip = ibuf;
				icnt = read(pvec[0], ip, BUFSIZ);
				if (icnt <= 0) {
					c = -1;
					break;
				}
			}
			if (hadnl)
				break;
			--icnt;
			c = (*ip++ & TRIM);
			if (c == 0)
				break;
			if (c == '\n') {
				/*
				 * Continue around the loop one
				 * more time, so that we can eat
				 * the last newline without terminating
				 * this word.
				 */
				hadnl = 1;
				continue;
			}
			if (!quoted && (c == ' ' || c == '\t'))
				break;
			cnt++;
			psave(c | quoted);
		}
		/*
		 * Unless at end-of-file, we will form a new word
		 * here if there were characters in the word, or in
		 * any case when we take text literally.  If
		 * we didn't make empty words here when literal was
		 * set then we would lose blank lines.
		 */
		if (c != -1 && (cnt || literal))
			pword();
		hadnl = 0;
	} while (c >= 0);
#ifdef GDEBUG
	printf("done in backeval, pvec: %d %d\n", pvec[0], pvec[1]);
	printf("also c = %c <%o>\n", c, c);
#endif
	(void) close(pvec[0]);
	pwait();
	prestjob();
}

psave(c)
	int c;
{

	if (--pnleft <= 0)
		error("Word too long");
	if (c & QUOTE) { 			/* 004 - GAG */
		*pargcp++ = QUOTECHAR;
		if (--pnleft <= 0)
			error("Word too long");
	}

	*pargcp++ = c & TRIM;
}

pword()
{

	psave(0);
	if (pargc == GAVSIZ)
		error("Too many words from ``");
	pargv[pargc++] = savestr(pargs);
	pargv[pargc] = NOSTR;
#ifdef GDEBUG
	printf("got word %s\n", pargv[pargc-1]);
#endif
	pargcp = pargs;
	pnleft = BUFSIZ - 4;
}
