
#ifndef lint
static char *sccsid = "@(#)getttyent.c	4.2	ULTRIX	10/16/90";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1984, 1987 by			*
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

/**************************************************************************
 *			Modification History
 *
 *
 * 22-Jan-1987	-  Larry Cohen	-  Increase line size from 256 to 512
 *
 * 22-Apr-1987  -  Tim Burke	-  Look for termio in /etc/ttys.
 *
 */

/* @(#)getttyent.c	4.2 (Berkeley) 1/30/85 */

#include <stdio.h>
#include <strings.h>
#include "ttyent.h"

static char TTYFILE[] = "/etc/ttys";
static char EMPTY[] = "";
static FILE *tf = NULL;
#define LINE 512
static char line[LINE];
static struct ttyent tty;

setttyent()
{
	if (tf == NULL)
		tf = fopen(TTYFILE, "r");
	else
		rewind(tf);
}

endttyent()
{
	if (tf != NULL) {
		(void) fclose(tf);
		tf = NULL;
	}
}

#define QUOTED 1

/*
 * Skip over the current field and 
 * return a pointer to the next field.
 */
static char *
skip(p)
	register char *p;
{
	register int c;
	register int q = 0;

	for (; (c = *p) != '\0'; p++) {
		if (c == '"') {
			q ^= QUOTED;	/* obscure, but nice */
			continue;
		}
		if (q == QUOTED)
			continue;
		if (c == '#') {
			*p = '\0';
			break;
		}
		if (c == '\t' || c == ' ' || c == '\n') {
			*p++ = '\0';
			while ((c = *p) == '\t' || c == ' ' || c == '\n')
				p++;
			break;
		}
	}
	return (p);
}

static char *
value(p)
	register char *p;
{
	if ((p = index(p,'=')) == 0)
		return(NULL);
	p++;			/* get past the = sign */
	return(p);
}

/* get rid of quotes. */

static
qremove(p)
	register char *p;
{
	register char *t;

	for (t = p; *p; p++)
		if (*p != '"')
			*t++ = *p;
	*t = '\0';
}

struct ttyent *
getttyent()
{
	register char *p;
	register int c;

	if (tf == NULL) {
		if ((tf = fopen(TTYFILE, "r")) == NULL)
			return (NULL);
	}
	do {
		p = fgets(line, LINE, tf);
		if (p == NULL)
			return (NULL);
		while ((c = *p) == '\t' || c == ' ' || c == '\n')
			p++;
	} while (c == '\0' || c == '#');
	tty.ty_name = p;
	p = skip(p);
	tty.ty_getty = p;
	p = skip(p);
	tty.ty_type = p;
	p = skip(p);
	tty.ty_status = TTY_LOCAL; /* default mode is to ignore modem signals */
	tty.ty_window = EMPTY;
	for (; *p; p = skip(p)) {
		if (strncmp(p, "on", 2) == 0)
			tty.ty_status |= TTY_ON;
		else if (strncmp(p, "off", 3) == 0)
			tty.ty_status &= ~TTY_ON;
		else if (strncmp(p, "secure", 6) == 0)
			tty.ty_status |= TTY_SECURE;
		else if (strncmp(p, "su", 2) == 0)
			tty.ty_status |= TTY_SU;
		else if (strncmp(p, "window", 6) == 0) {
			if ((tty.ty_window = value(p)) == NULL)
				tty.ty_window = EMPTY;
		     }
		else if (strncmp(p, "modem", 5) == 0) {
				tty.ty_status &= ~(TTY_LOCAL);
				tty.ty_status |= TTY_TRACK;
			}
		else if (strncmp(p, "nomodem", 6) == 0) {
				tty.ty_status |= TTY_LOCAL;
				tty.ty_status |= TTY_TRACK;
			}
		else if (strncmp(p, "shared", 6) == 0) 
				tty.ty_status |= TTY_SHARED;
		else if (strncmp(p, "termio", 6) == 0) 
				tty.ty_status |= TTY_TERMIO;
		else
			break;
	}
	tty.ty_comment = p;
	if (p = index(p, '\n'))
		*p = '\0';
	qremove(tty.ty_getty);
	qremove(tty.ty_window);
	qremove(tty.ty_comment);
	return(&tty);
}
