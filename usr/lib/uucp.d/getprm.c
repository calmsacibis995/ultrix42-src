#ifndef lint
static char sccsid[] = "@(#)getprm.c	4.1 (decvax!larry) 7/2/90";
#endif

#include <stdio.h>

#define LQUOTE	'('
#define RQUOTE ')'
#define NOSYSPART	0
#define HASSYSPART	1


/*******
 *	char *
 *	getprm(s, prm)	get next parameter from s
 *	char *s, *prm;
 *
 *	return - pointer to next character in s
 */




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




char *
getprm(s, prm)
char *s, *prm;
{
	char *c, *index();

	/* skip over white space */
	while (*s == ' ' || *s == '\t' || *s == '\n')
		s++;

	*prm = '\0';
	if (*s == '\0')
		return(NULL);

	/* return any meta characters */
	if (*s == '>' || *s == '<' || *s == '|'
	  || *s == ';' || *s == '&') {
		*prm++ = *s++;
		*prm = '\0';
		return(s);
	}

	/* look for quoted argument - any string between () */
	if (*s == LQUOTE) {
		if ((c = index(s + 1, RQUOTE)) != NULL) {
			c++;
			while (c != s)
				*prm++ = *s++;
			*prm = '\0';
			return(s);
		}
	}

	/* look for `  ` string */
	if (*s == '`') {
		if ((c = index(s + 1, '`')) != NULL) {
			c++;
			while (c != s)
				*prm++ = *s++;
			*prm = '\0';
			return(s);
		}
	}

	/* return a regular parameter - word seperated by white space */
	while (*s != ' ' && *s != '\t' && *s != '<'
	&& *s != '>' && *s != '|' && *s != '\0'
	&& *s != '&' && *s != ';' && *s != '\n')
		*prm++ = *s++;
	*prm = '\0';

	return(s);
}

/***
 *	split(name, sys, rest)	split into system and file part
 *	char *name, *sys, *rest;
 *
 *	return codes:
 *		NOSYSPART
 *		HASSYSPART
 */

split(name, sys, rest)
char *name, *sys, *rest;
{
	char *c, *index(), *strcpy();
	int i;

	if (*name == LQUOTE) {
		if ((c = index(name + 1, RQUOTE)) != NULL) {
		/* strip off quotes */
			name++;
			while (c != name)
				*rest++ = *name++;
			*rest = '\0';
			*sys = '\0';
			return(NOSYSPART);
		}
	}

	if ((c = index(name, '!')) == NULL) {
		strcpy(rest, name);
		*sys = '\0';
		return(NOSYSPART);
	}

	*c = '\0';
	for (i = 0; i < 7; i++)
		if ((*sys++ = *name++) == '\0')
			break;

	strcpy(rest, ++c);
	return(HASSYSPART);
}
