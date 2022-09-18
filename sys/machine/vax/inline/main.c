#ifndef lint
static	char	*sccsid = "@(#)main.c	4.1	(ULTRIX)	7/2/90";
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

/* ------------------------------------------------------------------------
 * Modification History: /sys/vax/inline/main.c
 *
 * 08 May 86 -- vjh
 *	Added support for .stab strings.  This mainly involved
 *	proper interpretation of ':'.  Had to make sure that a
 *	string containing a colon was not interpreted as a label.
 *
 * 13 Dec 84 -- jrs
 *	Change pattern hash to skip argument count so we can match
 *	specs optimized into registers.  Handle matching of patterns
 *	where argument count is either constant or register spec.
 *	Derived from 4.2BSD, labeled:
 *		main.c 1.6	84/09/20
 *
 * -----------------------------------------------------------------------
 */

#include <stdio.h>
#include <ctype.h>
#include "inline.h"

/*
 * These are the pattern tables to be loaded
 */
struct pats *inittables[] = {
	language_ptab,
	libc_ptab,
	machine_ptab,
	0
};

/*
 * Statistics collection
 */
struct stats {
	int	attempted;	/* number of expansion attempts */
	int	finished;	/* expansions done before end of basic block */
	int	lostmodified;	/* mergers inhibited by intervening mod */
	int	savedpush;	/* successful push/pop merger */
} stats;
int dflag;

main(argc, argv)
	int argc;
	char *argv[];
{
	register char *cp, *lp, *bquote;
	register char *bufp;
	register struct pats *pp, **php;
	Boolean instring, inlongstring;
	struct pats **tablep;
	register struct inststoptbl *itp, **ithp;
	int size;
	char *sp;
	extern char *index(), *rindex();

	if (argc > 1 && bcmp(argv[1], "-d", 3) == 0)
		dflag++, argc--, argv++;
	if (argc > 1)
		freopen(argv[1], "r", stdin);
	if (argc > 2)
		freopen(argv[2], "w", stdout);
	/*
	 * Set up the hash table for the patterns.
	 */
	for (tablep = inittables; *tablep; tablep++) {
		for (pp = *tablep; pp->name[0] != '\0'; pp++) {
			for (pp->sep = pp->name; isdigit(*pp->sep); pp->sep++)
				;
			if (*pp->sep != ',') {
				pp->sep = pp->name;
			}
			php = &patshdr[hash(pp->sep, &size)];
			pp->size = size;
			pp->next = *php;
			*php = pp;
		}
	}
	/*
	 * Set up the hash table for the instruction stop table.
	 */
	for (itp = inststoptable; itp->name[0] != '\0'; itp++) {
		ithp = &inststoptblhdr[hash(itp->name, &size)];
		itp->size = size;
		itp->next = *ithp;
		*ithp = itp;
	}
	/*
	 * Check each line and replace as appropriate.  Note the
	 * check for DOUBLEQUOTE right after LABELCHAR.  This is
	 * in case a colon occurs in a string; in that case, we
	 * do not want to interpret the string as a label.  This
	 * situation arises quite frequently when processing .stab
	 * records.
	 *
	 * Inlongstring is required because only 128 characters are
	 * maintained in an individual buffer, and the string might
	 * be much longer than that.  When it is, the opening and
	 * closing quotes are stored in different buffers even though
	 * processing only one string.
	 */
	buftail = bufhead = 0;
	bufp = line[0];
	inlongstring = false;
	while (fgets(bufp, MAXLINELEN, stdin)) {
		lp = index(bufp, LABELCHAR);
		bquote = index(bufp, DOUBLEQUOTE);
		if (bquote != NULL) {
		    if (bquote < lp || inlongstring) {
		        instring = true;
		    }
   		    if (rindex(bufp, DOUBLEQUOTE) == bquote) {
		        inlongstring = (inlongstring == true) ? false : true;
		    }
		} else {
		    instring = false;
		}
		if (lp != NULL && !(instring || inlongstring)) {
			bufp = newline();
			if (*++lp == '\n') {
				emptyqueue();
				continue;
			}
			strcpy(bufp, lp);
			*lp++ = '\n';
			*lp = '\0';
			emptyqueue();
		}
		for (cp = bufp; isspace(*cp); cp++)
			/* void */;
		if ((cp = doreplaceon(cp)) == 0) {
			bufp = newline();
			continue;
		}
		if (isdigit(*cp)) {
			for (sp = cp; isdigit(*sp); sp++)
				;
			if (*sp != ',') {
				sp = cp;
			}
			for (pp = patshdr[hash(sp, &size)]; pp; pp = pp->next) {
				if (pp->size == size &&
						bcmp(pp->name, cp, size +
							(sp - cp)) == 0) {
					expand(pp->replace);
					bufp = line[bufhead];
					break;
				}
			}
		} else {
			for (pp = patshdr[hash(cp, &size)]; pp; pp = pp->next) {
				if (pp->size == size &&
						bcmp(pp->sep, cp, size) == 0) {
					expand(pp->replace);
					bufp = line[bufhead];
					break;
				}
			}
		}
		if (!pp) {
			emptyqueue();
			fputs(bufp, stdout);
		}
	}
	emptyqueue();
	if (dflag)
		fprintf(stderr, "inline: %s %d, %s %d, %s %d, %s %d\n",
			"attempts", stats.attempted,
			"finished", stats.finished,
			"inhibited", stats.lostmodified,
			"merged", stats.savedpush);
	exit(0);
}

/*
 * Integrate an expansion into the assembly stream
 */
expand(replace)
	char *replace;
{
	register int curptr;
	char *nextreplace, *argv[MAXARGS];
	int argc, argreg, foundarg, mod = 0, args = 0;
	char parsebuf[BUFSIZ];

	stats.attempted++;
	for (curptr = bufhead; ; ) {
		nextreplace = copyline(replace, line[bufhead]);
		argc = parseline(line[bufhead], argv, parsebuf);
		argreg = nextarg(argc, argv);
		if (argreg == -1)
			break;
		args++;
		for (foundarg = 0; curptr != buftail; ) {
			curptr = PRED(curptr);
			argc = parseline(line[curptr], argv, parsebuf);
			if (isendofblock(argc, argv))
				break;
			if (foundarg = ispusharg(argc, argv))
				break;
			mod |= 1 << modifies(argc, argv);
		}
		if (!foundarg)
			break;
		replace = nextreplace;
		if (mod & (1 << argreg)) {
			stats.lostmodified++;
			if (curptr == buftail) {
				(void)newline();
				break;
			}
			(void)newline();
		} else {
			stats.savedpush++;
			rewrite(line[curptr], argc, argv, argreg);
			mod |= 1 << argreg;
		}
	}
	if (argreg == -1)
		stats.finished++;
	emptyqueue();
	fputs(replace, stdout);
	cleanup(args);
}

/*
 * Parse a line of assembly language into opcode and arguments.
 */
parseline(linep, argv, linebuf)
	char *linep;
	char *argv[];
	char *linebuf;
{
	register char *bufp = linebuf, *cp = linep;
	register int argc = 0;

	for (;;) {
		/*
		 * skip over white space
		 */
		while (isspace(*cp))
			cp++;
		if (*cp == '\0')
			return (argc);
		/*
		 * copy argument
		 */
		if (argc == MAXARGS - 1) {
			fprintf(stderr, "instruction too long->%s", linep);
			return (argc);
		}
		argv[argc++] = bufp;
		while (!isspace(*cp) && *cp != ARGSEPCHAR && *cp != COMMENTCHAR)
			*bufp++ = *cp++;
		*bufp++ = '\0';
		if (*cp == COMMENTCHAR)
			return (argc);
		if (*cp == ARGSEPCHAR)
			cp++;
	}
}

/*
 * Check for instructions that end a basic block.
 */
isendofblock(argc, argv)
	int argc;
	char *argv[];
{
	register struct inststoptbl *itp;
	int size;

	if (argc == 0)
		return (0);
	for (itp = inststoptblhdr[hash(argv[0], &size)]; itp; itp = itp->next)
		if (itp->size == size && bcmp(argv[0], itp->name, size) == 0)
			return (1);
	return (0);
}

/*
 * Copy a newline terminated string.
 * Return pointer to character following last character copied.
 */
char *
copyline(from, to)
	register char *from, *to;
{

	while (*from != '\n')
		*to++ = *from++;
	*to++ = *from++;
	*to = '\0';
	return (from);
}

/*
 * open space for next line in the queue
 */
char *
newline()
{
	bufhead = SUCC(bufhead);
	if (bufhead == buftail) {
		fputs(line[buftail], stdout);
		buftail = SUCC(buftail);
	}
	return (line[bufhead]);
}

/*
 * empty the queue by printing out all its lines.
 */
emptyqueue()
{
	while (buftail != bufhead) {
		fputs(line[buftail], stdout);
		buftail = SUCC(buftail);
	}
}

/*
 * Compute the hash of a string.
 * Return the hash and the size of the item hashed
 */
hash(cp, size)
	char *cp;
	int *size;
{
	register char *cp1 = cp;
	register int hash = 0;

	while (*cp1 && *cp1 != '\n')
		hash += (int)*cp1++;
	*size = cp1 - cp + 1;
	hash &= HSHSIZ - 1;
	return (hash);
}
