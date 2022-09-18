/*
 * get.c
 */

#ifndef lint
static char *sccsid = "@(#)get.c	4.1	ULTRIX	7/2/90";
#endif lint
/************************************************************************
 *									*
 *			Copyright (c) 1985 by				*
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
#include "../../h/param.h"
#include "vmb.h"
#include "../../machine/vax/mtpr.h"
#include "../../machine/vax/cons.h"

static int delflg = 0;


extern (*v_getc)();
getchar()
{
	register i, c;

	if( v_getc ) {
		c = ((*v_getc)() & 0177);
		if (c == CONTROL_S) {
			while (c != CONTROL_Q)
				c = ((*v_getc)() & 0177);
			c = ((*v_getc)() & 0177);
		}
	} else {
		while((mfpr(RXCS)&RXCS_DONE) == 0);
		c = mfpr(RXDB)&0177;
		if (c == CONTROL_S) {
			while (c != CONTROL_Q) {
				while((mfpr(RXCS)&RXCS_DONE) == 0);
				c = mfpr(RXDB)&0177;
			}
			while((mfpr(RXCS)&RXCS_DONE) == 0);
			c = mfpr(RXDB)&0177;
		}
	}
	if (c=='\r')
		c = '\n';
	if (!delflg && c != '')	/* no delete flag and not a RUBOUT */
		putchar(c);
	return(c);
}

gets(buf)
	char *buf;
{
	register char *lp;
	register c;
	register cc;			/* character count */

	cc=0;
	lp = buf;
	for (;;) {
		c = getchar();
		switch(c) {
		case '\n':
		case '\r':
			if (delflg && !v_getc) {
				putchar('/');
				putchar(c);
				delflg = 0;
			}
			if (delflg && v_getc) {
				putchar(c);
				delflg = 0;
			}
			c = '\n';
			*lp++ = '\0';
			return;
		case '':			/* Delete key */
			if (lp != buf) {
				if (!delflg) {
					delflg++;
					if (!v_getc)
						putchar('\\');
				}
				if (delflg && v_getc) {
					putchar('\b');
					putchar(' ');
					putchar('\b');
				}
			}
			if (lp > buf)
				if (v_getc)
					lp--;
				else
					putchar(*--lp);
			if (lp < buf)
				lp = buf;
			continue;
		/*
		 * Backspace and pound sign are left for backward
		 * compatability.  Strange results are guaranteed if
		 * either of these are mixed with <del> on the same
		 * input line.
		 */
		case '\b':
		case '#':
			delflg = 0;
			lp--;
			if (lp < buf)
				lp = buf;
			continue;
		case '@':
		case '':			/* Control U */
			delflg = 0;
			lp = buf;
			printf("^U\n");
			continue;
		case '':			/* Control R */
			*lp = '\0';
			if (delflg && !v_getc) {
				putchar('/');
			}
			delflg = 0;
			printf("^R\n%s", buf);
			continue;
		default:
			if (delflg) {
				if (!v_getc) {
					putchar('/');
				}
				putchar(c);
				delflg = 0;
			}
			*lp++ = c;
			cc++;
			if (cc == INBUFSZ) {
				printf("\nInput line too long\n");
				lp = buf;
				*lp = '\0';
				return;
			}
		}
	}
}
