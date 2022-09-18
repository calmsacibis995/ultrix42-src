#ifndef lint
static char *sccsid = "@(#)nmf.c	4.1	ULTRIX	7/17/90";
#endif

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

/************************************************************************
 *	Modification History
 *      --------------------
 *
 *	14-Nov-88		Tim N
 *			Took vax source and added support for the
 *			mips nm(1) output.  Also inserted the DEC header,
 *			this comment block and added the real sccs
 *			identifiers since this file had the hard coded
 *			id in it which was:
 *				@(#)nmf.c	1.3
 ************************************************************************/


#include "stdio.h"

#ifdef vax
#define SYMSTART 11
#define SYMCLASS 9
#endif

#ifdef mips
#define SYMSTART 11
#define SYMCLASS 9
#endif

#ifdef u3b
#define SYMSTART 11
#define SYMCLASS 9
#endif

#ifdef pdp11
#define SYMSTART 9
#define SYMCLASS 7
#endif

main(argc, argv)
char	*argv[];
{
	char	name[15], buf[64];
	char	*fname = NULL;
	char	*p;

	strcpy(name, argc > 1? argv[1] : "");
	if (argc > 2)
		fname = argv[2];
	while (gets(buf)) {
		p = &buf[SYMSTART];
		if (*p == '_')
			++p;
		switch (buf[SYMCLASS]) {
		case 'U':
#ifdef mips
		case 'V':
#endif
			printf("%s : %s\n", name, p);
			continue;
		case 'T':
			printf("%s = text", p);
			strcpy(name, p);
			break;
		case 'D':
#ifdef mips
		case 'G':
#endif
			printf("%s = data", p);
			if (strcmp(name, "") == 0)
				strcpy(name, p);
			break;
		case 'B':
#ifdef mips
		case 'S':
#endif
			printf("%s = bss", p);
			break;
		case 'A':
			printf("%s = abs", p);
			break;
		default:
			continue;
		}
		if (fname != NULL)
			printf(", %s", fname);
		printf("\n");
	}
	exit(0);
}
