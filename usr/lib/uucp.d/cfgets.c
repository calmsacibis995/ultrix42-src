#ifndef lint
static char sccsid[] = "@(#)cfgets.c	4.1 (decvax!larry) 7/2/90";
#endif

/*********************************************************************
function:	cfgets
description:	get nonblank, non-comment, (possibly continued) line
programmer:	Alan S. Watt

history:
	11/04/81	original version
*********************************************************************/


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




#include <stdio.h>
#define COMMENT		'#'
#define CONTINUE	'\\'
#define EOLN		'\n'
#define EOS		'\0'

char *
cfgets (buf, siz, fil)
register char *buf;
int siz;
FILE *fil;
{
	register i, c, len;
	register char *s;
	char *fgets();

	for (i=0,s=buf; i = (fgets (s, siz-i, fil) != NULL); i = s - buf) {

		/* get last character of line */
		c = s[len = (strlen (s) - 1)];

		/* skip comments; make sure end of comment line seen */
		if (*s == COMMENT) {
			while (c != EOLN && c != EOF)
				c = fgetc (fil);
			*s = EOS;
		}

		/* skip blank lines */
		else if (*s != EOLN) {
			s += len;

			/* continue lines ending with CONTINUE */
			if (c != EOLN || *--s != CONTINUE)
				break;
		}
	}
	
	return (i ? buf : NULL);
}

#ifdef TEST
# include <stdio.h>
main ()
{
	char buf[512];

	while (cfgets (buf, sizeof buf, stdin))
		fputs (buf, stdout);
}
#endif TEST
