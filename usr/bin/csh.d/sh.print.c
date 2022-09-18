#ifndef lint
static char *sccsid = "@(#)sh.print.c	4.3  (ULTRIX)        12/20/90";
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
/* $Header: sh.print.c,v 1.3 86/07/11 10:46:12 dce Exp $ */

#include "sh.h"
#include <sys/ioctl.h>

/*
 * C Shell
 *
 * Modification History
 *
 * 002 - Bob Fontaine - Mon Oct 22 10:51:40 EDT 1990
 *	 If a charcter is quoted, strip the quote before printing and don't
 *	 print the quote character.  Thanks to Akira Tanaka (JRD).  Fixes
 *	 QAR #5853 and #6041.
 *
 * 001 - Bob Fontaine	- Fri Jun 22 09:53:01 EDT 1990
 *	 Chnaged call to internal printf routine to csh_printf to avoid 
 *	 confusion with stdio library routine.
 */

psecs(l)
	long l;
{
	register int i;

	i = l / 3600;
	if (i) {
		csh_printf("%d:", i);			/* 001 RNF */
		i = l % 3600;
		p2dig(i / 60);
		goto minsec;
	}
	i = l;
	csh_printf("%d", i / 60);			/* 001 RNF */
minsec:
	i %= 60;
	csh_printf(":");				/* 001 RNF */
	p2dig(i);
}

p2dig(i)
	register int i;
{

	csh_printf("%d%d", i / 10, i % 10);			/* 001 RNF */
}

char	linbuf[128];
char	*linp = linbuf;

putchar(c)
	register int c;
{

	static int quoted;

	if(c & QUOTE)                   /* 002 RNF */
		quoted++;
	c &= TRIM;
	if(c == QUOTECHAR) {
		quoted++;
		return;
	}
	if(!quoted && (c == 0177 || c < ' ' && c != '\t' && c != '\n' && c != '\b')) {
		putchar('^');
		if (c == 0177)
			c = '?';
		else
			c |= 'A' - 1;
	}
	*linp++ = c;
	quoted = 0;
	if (c == '\n' || linp >= &linbuf[sizeof linbuf - 2])
		flush();
}

draino()
{

	linp = linbuf;
}

flush()
{
	register int unit;
	int lmode;

	if (linp == linbuf)
		return;
	if (haderr)
		unit = didfds ? 2 : SHDIAG;
	else
		unit = didfds ? 1 : SHOUT;
#ifdef TIOCLGET
	if (didfds == 0 && ioctl(unit, TIOCLGET, (char *)&lmode) == 0 &&
	    lmode&LFLUSHO) {
		lmode = LFLUSHO;
		(void) ioctl(unit, TIOCLBIC, (char *)&lmode);
		(void) write(unit, "\n", 1);
	}
#endif
	(void) write(unit, linbuf, linp - linbuf);
	linp = linbuf;
}
