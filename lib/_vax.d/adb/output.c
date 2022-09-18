#ifndef lint
static	char	*sccsid = "@(#)output.c	4.1	(ULTRIX)	11/23/87";
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

/************************************************************************
 *			Modification History
 *
 * 002	David L Ballenger, 25-Oct-1985
 *	Fix handling of %r, %R, %X, %u, and %U formats.
 *	Cleanup internal printf() routine by using varargs.h.
 *
 * 001	David L Ballenger, 12-Sep-1985
 *	Change all %X to %x in [fs]printf() format strings, so that all
 *	hexadecimal numbers print in lower case.  This restores the old
 *	behavior of adb, which changed when the %X bug was fixed in the
 *	[fs]printf() routines.
 *
 *	Based on:  output.c	4.3 8/11/83
 *
 ************************************************************************/
/*
 *
 *	UNIX debugger
 *
 */

#include "defs.h"
#include <stdio.h>
#include <varargs.h>
#include <sys/types.h>

INT		mkfault;
INT		infile;
INT		outfile = 1;
L_INT		maxpos;
L_INT		maxoff;
INT		radix = 16;

CHAR		printbuf[MAXLIN];
CHAR		*printptr = printbuf;
CHAR		*digitptr;
MSG		TOODEEP;


eqstr(s1, s2)
	REG STRING	s1, s2;
{
	REG STRING	 es1;
	WHILE *s1++ == *s2
	DO IF *s2++ == 0
	   THEN return(1);
	   FI
	OD
	return(0);
}

printc(c)
	CHAR		c;
{
	CHAR		d;
	STRING		q;
	INT		posn, tabs, p;

	IF mkfault
	THEN	return;
	ELIF (*printptr=c)==EOR
	THEN tabs=0; posn=0; q=printbuf;
	     FOR p=0; p<printptr-printbuf; p++
	     DO d=printbuf[p];
		IF (p&7)==0 ANDF posn
		THEN tabs++; posn=0;
		FI
		IF d==SP
		THEN posn++;
		ELSE WHILE tabs>0 DO *q++=TB; tabs--; OD
		     WHILE posn>0 DO *q++=SP; posn--; OD
		     *q++=d;
		FI
	     OD
	     *q++=EOR;
#ifdef EDDT
		printptr=printbuf; do putchar(*printptr++); while (printptr<q);
#else
	     write(outfile,printbuf,q-printbuf);
#endif
	     printptr=printbuf;
	ELIF c==TB
	THEN *printptr++=SP;
	     WHILE (printptr-printbuf)&7 DO *printptr++=SP; OD
	ELIF c
	THEN printptr++;
	FI
	IF printptr >= &printbuf[MAXLIN-9] THEN
		write(outfile, printbuf, printptr - printbuf);
		printptr = printbuf;
	FI
}

charpos()
{	return(printptr-printbuf);
}

flushbuf()
{	IF printptr!=printbuf
	THEN printc(EOR);
	FI
}

char *
printradix(buffer,value)
	char *buffer;
	register unsigned long	value;
{
	register char	*ptr = buffer;

	if (value == 0) {
		return("0");
	} else switch (radix) {
	case 2:
		ptr = &ptr[33];
		*--ptr = '\0';
		while (value != 0) {
			*--ptr = (value & 0x1) ? '1' : '0';
			value >>= 1 ;
		}
		return(ptr);
	case 8:
		(void) sprintf(ptr,"%l#o",value);
		return(ptr);
	case 10:
		(void) sprintf(ptr,"%ld",value);
		return(ptr);
	case 16:
		(void) sprintf(ptr,"%lx",value);
		return(ptr);
	}
}

char *
print_signed_oct(buffer,o)
	STRING		buffer;
	L_INT		o;
{
	/* Print a long value in signed octal
	 */
	static char format[] = "-%l#o" ;

	if (o < 0) {
		/* Negative value, so print negated value preceded
		 * by a minus sign.
		 */
		(void) sprintf(buffer,format,-o);
		return(buffer);
	} else {
		/* Positive value, so print value as is without the
		 * minus sign.
		 */
		(void) sprintf(buffer,&format[1],o);
		return(buffer);
	}
}


printf(va_alist)
	va_dcl
{
	va_list		args;
	STRING		format;
	REG STRING	s;
	INT		width, prec;
	CHAR		c, adj;
	INT		decpt, n;
	CHAR		digits[64];

	va_start(args);

	format = va_arg(args, char *);

	WHILE c = *format++
	DO  IF c!='%'
	    THEN printc(c);
	    ELSE IF *format=='-' THEN adj='l'; format++; ELSE adj='r'; FI
		 width=convert(&format);
		 IF *format=='.' THEN format++; prec=convert(&format); ELSE prec = -1; FI
		 digitptr=digits;

		 s=0;
		 switch (c = *format++) {

		    case 'd':
			(void) sprintf(digits,"%d",(short)va_arg(args,int));
		        s = digits;
			break ;
		    case 'u':
			(void) sprintf(digits,"%u",(u_short)va_arg(args,int));
		        s = digits;
			break;
		    case 'o':
			(void) sprintf(digits,"%#o",(u_short)va_arg(args,int));
		        s = digits;
			break;
		    case 'q':
			s = print_signed_oct(digits,(short)va_arg(args,int));
			break;
		    case 'x':
			(void) sprintf(digits,"%x",(u_short)va_arg(args,int));
		        s = digits;
			break;
		    case 'r':
		        s = printradix(digits,
				       (u_long)((u_short)va_arg(args,int))
				      );
			break;
		    case 'R':
		        s = printradix(digits,va_arg(args,long));
			break;
		    case 'Y':
			printdate(va_arg(args,long)); 
			break;
		    case 'D':
			(void) sprintf(digits,"%ld",va_arg(args,long));
		    	s = digits;
			break;
		    case 'U':
			(void) sprintf(digits,"%lu",va_arg(args,long));
		    	s = digits;
			break;
		    case 'O':
			(void) sprintf(digits,"%l#o",va_arg(args,long));
			s = digits;
			break;
		    case 'Q':
			s = print_signed_oct(digits,va_arg(args,long));
			break;
		    case 'X':
			(void) sprintf(digits,"%lx",va_arg(args,long));
		    	s = digits;
			break;
		    case 'c':
			printc(va_arg(args,int));
			break;
		    case 's':
			s = va_arg(args,STRING);
			break;
		    case 'f':
		    case 'F':
			(void) sprintf(digits,"%+.16e",va_arg(args,double));
			s = digits;
			prec= -1;
			break;
		    case 'm':
			break;
		    case 'M':
			width = va_arg(args,int);
			break;
		    case 'T':
		        width = va_arg(args,int);
		    case 't':
			IF width
			THEN width -= charpos()%width;
			FI
			break;
		    default:
			printc(va_arg(args,int));
		}

		IF s==0
		THEN *digitptr=0; s=digits;
		FI
		n=strlen(s);
		n=(prec<n ANDF prec>=0 ? prec : n);
		width -= n;
		IF adj=='r'
		THEN WHILE width-- > 0
		     DO printc(SP); OD
		FI
		WHILE n-- DO printc(*s++); OD
		WHILE width-- > 0 DO printc(SP); OD
		digitptr=digits;
	    FI
	OD
}

printdate(tvec)
	L_INT		tvec;
{
	REG INT		i;
	REG STRING	timeptr;

	timeptr = ctime(&tvec);
	
	FOR i=20; i<24; i++ DO *digitptr++ = *(timeptr+i); OD
	FOR i=3; i<19; i++ DO *digitptr++ = *(timeptr+i); OD

} /*printdate*/

prints(s)
char *s;
{	printf("%s",s);
}

newline()
{
	printc(EOR);
}

convert(cp)
REG STRING	*cp;
{
	REG CHAR	c;
	INT		n;
	n=0;
	WHILE ((c = *(*cp)++)>='0') ANDF (c<='9') DO n=n*10+c-'0'; OD
	(*cp)--;
	return(n);
}

#define	MAXIFD	5
struct {
	int	fd;
	int	r9;
} istack[MAXIFD];
int	ifiledepth;

iclose(stack, err)
{
	IF err
	THEN	IF infile
		THEN	close(infile); infile=0;
		FI
		WHILE --ifiledepth >= 0
		DO	IF istack[ifiledepth].fd
			THEN	close(istack[ifiledepth].fd);
			FI
		OD
		ifiledepth = 0;
	ELIF stack == 0
	THEN	IF infile
		THEN	close(infile); infile=0;
		FI
	ELIF stack > 0
	THEN	IF ifiledepth >= MAXIFD
		THEN	error(TOODEEP);
		FI
		istack[ifiledepth].fd = infile;
		istack[ifiledepth].r9 = var[9];
		ifiledepth++;
		infile = 0;
	ELSE	IF infile
		THEN	close(infile); infile=0;
		FI
		IF ifiledepth > 0
		THEN	infile = istack[--ifiledepth].fd;
			var[9] = istack[ifiledepth].r9;
		FI
	FI
}

oclose()
{
	IF outfile!=1
	THEN	flushbuf(); close(outfile); outfile=1;
	FI
}

endline()
{

	if (maxpos <= charpos())
		printf("\n");
}
