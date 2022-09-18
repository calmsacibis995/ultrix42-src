#ifndef lint
static char *sccsid = "@(#)sh.misc.c	4.2  (ULTRIX)        8/13/90";
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
/* $Header: sh.misc.c,v 1.4 86/07/11 10:45:16 dce Exp $ */

#include "sh.h"

/*
 * C Shell
 * Modification History
 *
 * 003	- Bob Fontaine  - Fri Jun 22 09:53:01 EDT 1990
 *	 Changed call to internal printf function to csh_printf to avoid
 *	 confusion with stdio library routine.
 *
 * 002 - Gary A. Gaudet - Tue Jan  2 11:51:47 EST 1990
 *	added some (castings)
 *
 * 01 Sat Aug 13 15:28:57 EDT 1988, Gary A. Gaudet
 *	merging mips & ultrix for 8 bit clean and bug fixes
 */

any(c, s)
	register int c;
	register char *s;
{

	if (c & QUOTE)
		return (0);
	while (*s)
		if (*s++ == c)
			return(1);
	return(0);
}

onlyread(cp)
	char *cp;
{
	extern char end[];

	return (cp < end);
}

xfree(cp)
	char *cp;
{
	extern char end[];

	if (cp >= end && cp < (char *) &cp)
		free(cp);
}

char *
savestr(s)
	register char *s;
{
	char *n;
	register char *p;

	if (s == 0)
		s = "";
	for (p = s; *p++;)
		;
	n = p = xalloc((unsigned) (p - s));
	while (*p++ = *s++)
		;
	return (n);
}

char *
calloc(i, j)
	register unsigned i;
	unsigned j;
{
	char *cp;

	i *= j;
	cp = xalloc(i);
	bzero(cp, (int)i);
	return (cp);
}

nomem(i)
	unsigned i;
{
#ifdef debug
	static char *av[2] = {0, 0};
#endif

	child++;
#ifndef debug
	error("Out of memory");
#ifdef lint
	i = i;
#endif
#else
	showall(av);
	csh_printf("i=%d: Out of memory\n", i);			/* 003 RNF */
	chdir("/usr/bill/cshcore");
	abort();
#endif
	return 0;		/* fool lint */
}

char **
blkend(up)
	register char **up;
{

	while (*up)
		up++;
	return (up);
}
 
blkpr(av)
	register char **av;
{

	for (; *av; av++) {
		csh_printf("%s", *av);			/* 003 RNF */
		if (av[1])
			csh_printf(" ");		/* 003 RNF */
	}
}

blklen(av)
	register char **av;
{
	register int i = 0;

	while (*av++)
		i++;
	return (i);
}

char **
blkcpy(oav, bv)
	char **oav;
	register char **bv;
{
	register char **av = oav;

	while (*av++ = *bv++)
		continue;
	return (oav);
}

char **
blkcat(up, vp)
	char **up, **vp;
{

	(void) blkcpy(blkend(up), vp);
	return (up);
}

blkfree(av0)
	char **av0;
{
	register char **av = av0;

	for (; *av; av++)
		XFREE(*av)
	XFREE((char *)av0)
}

char **
saveblk(v)
	register char **v;
{
	register char **newv =
		(char **) calloc((unsigned) (blklen(v) + 1), (unsigned)sizeof (char **));
	char **onewv = newv;

	while (*v)
		*newv++ = savestr(*v++);
	return (onewv);
}

char *
strspl(cp, dp)
	char *cp, *dp;
{
	char *ep;
	register char *p, *q;

	for (p = cp; *p++;)
		;
	for (q = dp; *q++;)
		;
	ep = xalloc((unsigned) ((p - cp) + (q - dp) - 1));
	for (p = ep, q = cp; *p++ = *q++;)
		;
	for (p--, q = dp; *p++ = *q++;)
		;
	return (ep);
}

char **
blkspl(up, vp)
	register char **up, **vp;
{
	register char **wp =
		(char **) calloc((unsigned) (blklen(up) + blklen(vp) + 1),
			(unsigned)sizeof (char **));

	(void) blkcpy(wp, up);
	return (blkcat(wp, vp));
}

lastchr(cp)
	register char *cp;
{

	if (!*cp)
		return (0);
	while (cp[1])
		cp++;
	return (*cp);
}

/*
 * This routine is called after an error to close up
 * any units which may have been left open accidentally.
 */
closem()
{
	register int f;

	for (f = 0; f < NOFILE; f++)
		if (f != SHIN && f != SHOUT && f != SHDIAG && f != OLDSTD &&
		    f != FSHTTY)
			(void) close(f);
}

donefds()
{

	(void) close(0);
	(void) close(1);
	(void) close(2);
	didfds = 0;
}

/*
 * Move descriptor i to j.
 * If j is -1 then we just want to get i to a safe place,
 * i.e. to a unit > 2.  This also happens in dcopy.
 */
dmove(i, j)
	register int i, j;
{
	if (i == j || i < 0)
		return (i);
	if (j >= 0) {
		(void) dup2(i, j);
		return (j);
	}
	j = dcopy(i, j);
	if (j != i)
		(void) close(i);
	return (j);
}

dcopy(i, j)
	register int i, j;
{
	int x;

	if (i == j || i < 0 || j < 0 && i > 2)
		return (i);
	if (j >= 0) {
		(void) dup2(i, j);
		return (j);
	}
	if(j >= 0)
		(void) close(j); 
	x = renum(i,j);
	return(x);

/*	return (renum(i, j)); */
}

renum(i, j)
	register int i, j;
{
	register int k = dup(i);

	if (k < 0)
		return (-1);
	if (j == -1 && k > 2)
		return (k);
	if (k != j) {
		j = renum(k, j);
		(void) close(k);
		return (j);
	}
	return (k);
}

#ifndef copy
copy(to, from, size)
	register char *to, *from;
	register int size;
{

	if (size)
		do
			*to++ = *from++;
		while (--size != 0);
}
#endif

/*
 * Left shift a command argument list, discarding
 * the first c arguments.  Used in "shift" commands
 * as well as by commands like "repeat".
 */
lshift(v, c)
	register char **v;
	register int c;
{
	register char **u = v;

	while (*u && --c >= 0)
		xfree(*u++);
	(void) blkcpy(v, u);
}

number(cp)
	char *cp;
{

	if (*cp == '-') {
		cp++;
		if (!digit(*cp++))
			return (0);
	}
	while (*cp && digit(*cp))
		cp++;
	return (*cp == 0);
}

char **
copyblk(v)
	register char **v;
{
	register char **nv =
		(char **) calloc((unsigned) (blklen(v) + 1), (unsigned)sizeof (char **));

	return (blkcpy(nv, v));
}

char *
strend(cp)
	register char *cp;
{

	while (*cp)
		cp++;
	return (cp);
}

char *
strip(cp)
	char *cp;
{
	register char *dp = cp;		/* destination pointer */
	register char *sp = cp;		/* source pointer */

        /*
         * only strip if string is not empty
         */

	if (*sp) {
		while (*sp) {
			if ((*sp & TRIM) == QUOTECHAR)
				sp++;
			*dp++ = *sp++;
		}
		*dp++ = '\0';
	}
	return (cp);
}

udvar(name)
	char *name;
{

	setname(name);
	bferr("Undefined variable");
}

prefix(sub, str)
	register char *sub, *str;
{

	for (;;) {
		if (*sub == 0)
			return (1);
		if (*str == 0)
			return (0);
		if (*sub++ != *str++)
			return (0);
	}
}
