#ifndef lint
static	char	*sccsid = "@(#)machdep.c	4.1	(ULTRIX)	7/2/90";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1984 by				*
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
 * Modification History: /sys/machine/vax/inline/machdep.c
 *
 * 10 Dec 89 -- Matt Thomas
 *	Change doreplaceon to accept either a tab or space after calls.
 *
 * 13 Dec 84 -- jrs
 *	Change doreplaceon() to check for either constant or register
 *	specification for argument count
 *	Derived from 4.2BSD, labeled:
 *		machdep.c 1.4	84/09/20
 *
 * -----------------------------------------------------------------------
 */

#include <stdio.h>
#include <ctype.h>
#include "inline.h"

/*
 * The routines and tables in this file must be rewritten
 * for each new machine that this program is ported to.
 */

#ifdef vax
/*
 * Instruction stop table.
 * All instructions that implicitly modify any of the temporary
 * registers, change control flow, or implicitly loop must be
 * listed in this table. It is used to find the end of a basic
 * block when scanning backwards through the instruction stream
 * trying to merge the inline expansion.
 */
struct inststoptbl inststoptable[] = {
	{ "jbc" }, { "jlbc" }, { "jbs" }, { "jlbs" }, { "jbcc" },
	{ "jbsc" }, { "jbcs" }, { "jbss" }, { "jbr" }, { "jcc" },
	{ "jcs" }, { "jvc" }, { "jvs" }, { "jlss" }, { "jlssu" },
	{ "jleq" }, { "jlequ" }, { "jeql" }, { "jeqlu" }, { "jneq" },
	{ "jnequ" }, { "jgeq" }, { "jgequ" }, { "jgtr" }, { "jgtru" },
	{ "chmk" }, { "chme" }, { "chms" }, { "chmu" }, { "rei" },
	{ "ldpctx" }, { "svpctx" }, { "xfc" }, { "bpt" },
	{ "bugw" }, { "bugl" }, { "halt" }, { "pushr" }, { "popr" },
	{ "polyf" }, { "polyd" }, { "polyg" }, { "polyh" },
	{ "bneq" }, { "bnequ" }, { "beql" }, { "beqlu" }, { "bgtr" },
	{ "bleq" }, { "bgeq" }, { "blss" }, { "bgtru" }, { "blequ" },
	{ "bvc" }, { "bvs" }, { "bgequ" }, { "bcc" }, { "blssu" },
	{ "bcs" }, { "brb" }, { "brw" }, { "jmp" },
	{ "bbs" }, { "bbc" }, { "bbss" }, { "bbcs" }, { "bbsc" },
	{ "bbcc" }, { "bbssi" }, { "bbcci" }, { "blbs" }, { "blbc" },
	{ "acbb" }, { "acbw" }, { "acbl" }, { "acbf" }, { "acbd" },
	{ "acbg" }, { "acbh" }, { "aoblss" }, { "aobleq" },
	{ "sobgeq" }, { "sobgtr" }, { "caseb" }, { "casew" }, { "casel" },
	{ "bsbb" }, { "bsbw" }, { "jsb" }, { "rsb" },
	{ "callg" }, { "calls" }, { "ret" },
	{ "movc3" }, { "movc5" }, { "movtc" }, { "movtuc" },
	{ "cmpc3" }, { "cmpc5" }, { "scanc" }, { "spanc" },
	{ "locc" }, { "skpc" }, { "matchc" }, { "crc" },
	{ "movp" }, { "cmpp3" }, { "cmpp4" }, { "addp4" }, { "addp6" },
	{ "subp4" }, { "subp6" }, { "mulp" }, { "divp" }, { "cvtlp" },
	{ "cvtpl" }, { "cvtpt" }, { "cvttp" }, { "cvtps" }, { "cvtsp" },
	{ "ashp" }, { "editpc" },
	{ "escd" }, { "esce" }, { "escf" },
	{ "" }
};

/*
 * Check to see if a line is a candidate for replacement.
 * If argument count is a constant, return pointer to entire
 * string, else skip register specification, which can be
 * variable and return pointer to string starting at seperator.
 */
char *
doreplaceon(cp)
	char *cp;
{

	if (bcmp(cp, "calls", 5) != 0 || (cp[5] != ' ' && cp[5] != '\t')) {
		return(0);
	}
	switch (*(cp + 6)) {

	case '$':		/* constant spec */
		return(cp + 7);

	case 'r':		/* register spec */
		for (cp += 7; isdigit(*cp); cp++)
			;
		if (*cp == ',') {
			return(cp);
		} else {
			return(0);
		}

	default:		/* can't handle anything else */
		return(0);

	}
}

/*
 * Find the next argument to the function being expanded.
 */
nextarg(argc, argv)
	int argc;
	char *argv[];
{
	register char *lastarg = argv[2];

	if (argc == 3 &&
	    bcmp(argv[0], "mov", 3) == 0 &&
	    bcmp(argv[1], "(sp)+", 6) == 0 &&
	    lastarg[0] == 'r' && isdigit(lastarg[1]) && lastarg[2] == '\0')
		return (lastarg[1] - '0');
	return (-1);
}

/*
 * Determine whether the current line pushes an argument.
 */
 ispusharg(argc, argv)
	int argc;
	char *argv[];
{

	if (argc < 2)
		return (0);
	if (argc == 2 && bcmp(argv[0], "push", 4) == 0)
		return (1);
	if (bcmp(argv[argc - 1], "-(sp)", 6) == 0)
		return (1);
	return (0);
}

/*
 * Determine which (if any) registers are modified
 * Return register number that is modified, -1 if none are modified.
 */
modifies(argc, argv)
	int argc;
	char *argv[];
{
	/*
	 * For the VAX all we care about are r0 to r5
	 */
	register char *lastarg = argv[argc - 1];

	if (lastarg[0] == 'r' && isdigit(lastarg[1]) && lastarg[2] == '\0')
		return (lastarg[1] - '0');
	return (-1);
}

/*
 * Rewrite the instruction in (argc, argv) to store its
 * contents into arg instead of onto the stack. The new
 * instruction is placed in the buffer that is provided.
 */
rewrite(instbuf, argc, argv, target)
	char *instbuf;
	int argc;
	char *argv[];
	int target;
{

	switch (argc) {
	case 0:
		instbuf[0] = '\0';
		fprintf("blank line to rewrite?\n");
		return;
	case 1:
		sprintf(instbuf, "\t%s\n", argv[0]);
		fprintf(stderr, "rewrite?-> %s", instbuf);
		return;
	case 2:
		if (bcmp(argv[0], "push", 4) == 0) {
			sprintf(instbuf, "\tmov%s\t%s,r%d\n",
				&argv[0][4], argv[1], target);
			return;
		}
		sprintf(instbuf, "\t%s\tr%d\n", argv[0], target);
		return;
	case 3:
		sprintf(instbuf, "\t%s\t%s,r%d\n", argv[0], argv[1], target);
		return;
	case 4:
		sprintf(instbuf, "\t%s\t%s,%s,r%d\n",
			argv[0], argv[1], argv[2], target);
		return;
	case 5:
		sprintf(instbuf, "\t%s\t%s,%s,%s,r%d\n",
			argv[0], argv[1], argv[2], argv[3], target);
		return;
	default:
		sprintf(instbuf, "\t%s\t%s", argv[0], argv[1]);
		argc -= 2, argv += 2;
		while (argc-- > 0) {
			strcat(instbuf, ",");
			strcat(instbuf, *argv++);
		}
		strcat(instbuf, "\n");
		fprintf(stderr, "rewrite?-> %s", instbuf);
		return;
	}
}

/*
 * Do any necessary post expansion cleanup.
 */
cleanup(numargs)
	int numargs;
{

	return;
}
#endif vax

#ifdef mc68000
/*
 * Instruction stop table.
 * All instructions that implicitly modify any of the temporary
 * registers, change control flow, or implicitly loop must be
 * listed in this table. It is used to find the end of a basic
 * block when scanning backwards through the instruction stream
 * trying to merge the inline expansion.
 */
struct inststoptbl inststoptable[] = {
	{ "" }
};

/*
 * Check to see if a line is a candidate for replacement.
 * Return pointer to name to be looked up in pattern table.
 */
char *
doreplaceon(cp)
	char *cp;
{

	if (bcmp(cp, "jbsr\t", 5) == 0)
		return (cp + 5);
	return (0);
}

/*
 * Find the next argument to the function being expanded.
 */
nextarg(argc, argv)
	int argc;
	char *argv[];
{
	register char *lastarg = argv[2];

	if (argc == 3 &&
	    bcmp(argv[0], "movl", 5) == 0 &&
	    bcmp(argv[1], "sp@+", 5) == 0 &&
	    (lastarg[1] == '0' || lastarg[1] == '1') &&
	    lastarg[2] == '\0') {
		if (lastarg[0] == 'd')
			return (lastarg[1] - '0');
		return (lastarg[1] - '0' + 8);
	}
	return (-1);
}

/*
 * Determine whether the current line pushes an argument.
 */
 ispusharg(argc, argv)
	int argc;
	char *argv[];
{

	if (argc < 2)
		return (0);
	if (argc == 2 && bcmp(argv[0], "pea", 4) == 0)
		return (1);
	if (bcmp(argv[argc - 1], "sp@-", 5) == 0)
		return (1);
	return (0);
}

/*
 * Determine which (if any) registers are modified
 * Return register number that is modified, -1 if none are modified.
 */
modifies(argc, argv)
	int argc;
	char *argv[];
{
	/*
	 * For the MC68000 all we care about are d0, d1, a0, and a1.
	 */
	register char *lastarg = argv[argc - 1];

	if (lastarg[0] == 'd' && isdigit(lastarg[1]) && lastarg[2] == '\0')
		return (lastarg[1] - '0');
	if (lastarg[0] == 'a' && isdigit(lastarg[1]) && lastarg[2] == '\0')
		return (lastarg[1] - '0' + 8);
	return (-1);
}

/*
 * Rewrite the instruction in (argc, argv) to store its
 * contents into arg instead of onto the stack. The new
 * instruction is placed in the buffer that is provided.
 */
rewrite(instbuf, argc, argv, target)
	char *instbuf;
	int argc;
	char *argv[];
	int target;
{
	int regno;
	char regtype;

	if (target < 8) {
		regtype = 'd';
		regno = target;
	} else {
		regtype = 'a';
		regno = target - 8;
	}
	switch (argc) {
	case 0:
		instbuf[0] = '\0';
		fprintf("blank line to rewrite?\n");
		return;
	case 1:
		sprintf(instbuf, "\t%s\n", argv[0]);
		fprintf(stderr, "rewrite?-> %s", instbuf);
		return;
	case 2:
		if (bcmp(argv[0], "pea", 4) == 0) {
			if (regtype == 'a') {
				sprintf(instbuf, "\tlea\t%s,%c%d\n",
					argv[1], regtype, regno);
				return;
			}
			if (argv[1][0] == '_' || isdigit(argv[1][0])) {
				sprintf(instbuf, "\tmovl\t#%s,%c%d\n",
					argv[1], regtype, regno);
				return;
			}
			sprintf(instbuf,
				"\texg\ta0,d%d\n\tlea\t%s,a0\n\texg\ta0,d%d\n",
				regno, argv[1], regno);
			return;
		}
		sprintf(instbuf, "\t%s\t%c%d\n", argv[0], regtype, regno);
		return;
	case 3:
		sprintf(instbuf, "\t%s\t%s,%c%d\n",
			argv[0], argv[1], regtype, regno);
		return;
	default:
		sprintf(instbuf, "\t%s\t%s", argv[0], argv[1]);
		argc -= 2, argv += 2;
		while (argc-- > 0) {
			strcat(instbuf, ",");
			strcat(instbuf, *argv++);
		}
		strcat(instbuf, "\n");
		fprintf(stderr, "rewrite?-> %s", instbuf);
		return;
	}
}

/*
 * Do any necessary post expansion cleanup.
 */
cleanup(numargs)
	int numargs;
{
	
	if (numargs == 0)
		return;
	/*
	 * delete instruction to pop arguments.
	 * TODO:
	 *	CHECK FOR LABEL
	 *	CHECK THAT INSTRUCTION IS A POP
	 */
	fgets(line[bufhead], MAXLINELEN, stdin);
}
#endif mc68000
