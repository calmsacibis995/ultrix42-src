#ifndef lint
static char sccsid[] = "@(#)print.c	4.1 (Ultrix) 7/2/90";
/* Original ID:  "@(#)print.c	4.2 8/11/83" */
#endif

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

/*
 * Modification History
 * ~~~~~~~~~~~~~~~~~~~~
 *
 * 01	David Lindner Mon Jul 24 14:43:13 EDT 1989
 *	- Added prp2 for printing correct error message, and added
 *	- comment header.
 *
 */

#
/*
 * UNIX shell
 *
 * S. R. Bourne
 * Bell Telephone Laboratories
 *
 */

#include	"defs.h"

CHAR		numbuf[6];


/* printing and io conversion */

newline()
{	prc(NL);
}

blank()
{	prc(SP);
}

prp()
{
	IF (flags&prompt)==0 ANDF cmdadr
	THEN	prs(cmdadr); prs(colon);
	FI
}

/* DJL 01 */
prp2()
{
	IF (flags&prompt)==0 ANDF calladr 
	THEN	prs(calladr); prs(colon);
	FI
}

VOID	prs(as)
	STRING		as;
{
	REG STRING	s;

	IF s=as
	THEN	write(output,s,length(s)-1);
	FI
}

VOID	prc(c)
	CHAR		c;
{
	IF c
	THEN	write(output,&c,1);
	FI
}

prt(t)
	L_INT		t;
{
	REG INT	hr, min, sec;

	t += 30; t /= 60;
	sec=t%60; t /= 60;
	min=t%60;
	IF hr=t/60
	THEN	prn(hr); prc('h');
	FI
	prn(min); prc('m');
	prn(sec); prc('s');
}

prn(n)
	INT		n;
{
	itos(n); prs(numbuf);
}

itos(n)
{
	REG char *abuf; REG POS a, i; INT pr, d;
	abuf=numbuf; pr=FALSE; a=n;
	FOR i=10000; i!=1; i/=10
	DO	IF (pr |= (d=a/i)) THEN *abuf++=d+'0' FI
		a %= i;
	OD
	*abuf++=a+'0';
	*abuf++=0;
}

stoi(icp)
STRING	icp;
{
	REG CHAR	*cp = icp;
	REG INT		r = 0;
	REG CHAR	c;

	WHILE (c = *cp, digit(c)) ANDF c ANDF r>=0
	DO r = r*10 + c - '0'; cp++ OD
	IF r<0 ORF cp==icp
	THEN	failed(icp,badnum);
	ELSE	return(r);
	FI
}

