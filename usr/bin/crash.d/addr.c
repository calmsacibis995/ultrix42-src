#ifndef lint
static char *sccsid = "@(#)addr.c	4.1	(ULTRIX)	7/17/90";
#endif

#include "crash.h"

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

int
whitespace(c)
	char c;
{
	return((c == '\t') || (c == ' '));
}

int
endstr(c)
	char c;
{
	return((c == '\0') || (c == '\n'));
}

int
parse_tabaddr(str, t)
	char *str;
	struct tabloc *t;
{
	int c = 0;
	int n = 0;
	char number[10];

	/*
	 * First dig out the name of the kernel table we want.
	 */

	while(whitespace(str[c])) c++;
	while(!whitespace(str[c]) && !endstr(str[c]) && (str[c] != '['))
	{
		t->tabname[c] = str[c];
		c++;
	}
	t->tabname[c] = '\0';
	while(whitespace(str[c])) c++;
	if (str[c++] != '[') {
		return(0);
	}

	/*
	 * Now get the index into the table.
	 */

	while(whitespace(str[c])) c++;
	while(!whitespace(str[c]) && !endstr(str[c]) && (str[c] != ']'))
	{
		if (n == 9) {
			return(0);
		}
		number[n++] = str[c++];
	}
	while(whitespace(str[c])) c++;
	if (str[c++] != ']') {
		return(0);
	}
	number[n] = '\0';
	if ((t->index = atoi(number)) == -1) {
		return(0);
	}

	/*
	 * Now get the optional offset.
	 */

	while(whitespace(str[c])) c++;
	if (endstr(str[c])) {
		t->offset = 0;
		if (resolve_tabaddr(t))
			return(1);
		else {
			return(0);
		}
	}
	if (str[c++] != '+') {
		return(0);
	}
	n = 0;
	while(whitespace(str[c])) c++;
	while(!whitespace(str[c]) && !endstr(str[c]) && (str[c] != ']'))
	{
		if (n == 9)
			return(0);
		number[n++] = str[c++];
	}
	number[n] = '\0';
	if ((t->offset = atoi(number)) == -1) {
		return(0);
	}

	/*
	 * And fill out the rest of the tabloc struct.
	 */

	if (resolve_tabaddr(t))
		return(1);
	else {
		return(0);
	}
}

int
resolve_tabaddr(t)
	struct tabloc *t;
{
	int i;
	struct tabsum *ts;

	for (i = 0; i < TABSUM_MAX; i++) {
		if (strcmp(tab[i].name, t->tabname))
			continue;
		t->tab = ts = &tab[i];
		if (t->index >= ts->ents) {
			error("index too big for table");
			return(0);
		}
		if (t->offset >= ts->size) {
			error("offset too big for table");
			return(0);
		}
		t->addr = ts->first + (t->index * ts->size) + t->offset;
		return(1);
	}
	return (0);
}
