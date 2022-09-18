#ifndef lint
static	char	*sccsid = "@(#)doscan.c	4.3	ULTRIX	2/28/91";
#endif

/************************************************************************
 *									*
 *			Copyright (c) 1985, 1988, 1989 by		*
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
 *   This software is  derived  from  software  received  from Bell	*
 *   Laboratories.  Use, duplication, or disclosure is  subject  to	*
 *   restrictions  under  license  agreements  with  AT&T.		*
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
 *	Brian Mahoney
 * 007	Change behavior of %n when eof is encountered
 *
 *	Jon Reeves, 15-Nov-1989
 * 006	Treat X like x for POSIX/X/Open.
 *
 *      Linda Wilson, 05-oct-1989
 * 005  For X/Open conformance, treat E/F/G as floats rather than doubles.
 *	When %n encountered, do not increment the assignment count.
 *
 *	Jon Reeves, 14 Jun 1989
 * 004	Added %p (same as %8x) and L modifier (treated as l) for ANSI.
 *
 *	Jon Reeves, 12 Apr 1989
 * 003	Corrected problems with short counts for %n, not counting the
 *	'e' for %#e, not scanning space in "1e 1", not counting leading
 *	0 or 0x for %#i.
 *
 *	Andy Gadsby, 27-Nov-1986
 * INTL	Modified to use external _lc_radix to detect decimal point. 
 *
 *	David L Ballenger, 7-Jun-1985
 * 001	Use modifyied version of System V doscan().  Upgraded from System
 *	V release 2 to conform to System V interface definition.  Now
 *	handles %i and %n.  Based on doscan.c 2.6.
 *
 ************************************************************************/

/*LINTLIBRARY*/
#include <stdio.h>
#include <ctype.h>
#include <varargs.h>
#include <values.h>

#define NCHARS	(1 << BITSPERBYTE)

extern double atof();
extern char *memset();
extern int ungetc();

static unsigned char *
setup(fmt, tab)
register unsigned char *fmt;
register char *tab;
{
	register int b, c, d, t = 0;

	if(*fmt == '^') {
		t++;
		fmt++;
	}
	(void)memset(tab, !t, NCHARS);
	if((c = *fmt) == ']' || c == '-') { /* first char is special */
		tab[c] = t;
		fmt++;
	}
	while((c = *fmt++) != ']') {
		if(c == '\0')
			return(NULL); /* unexpected end of format */
		if(c == '-' && (d = *fmt) != ']' && (b = fmt[-2]) < d) {
			(void)memset(&tab[b], t, d - b + 1);
			fmt++;
		} else
			tab[c] = t;
	}
	return(fmt);
}

static void
store_int(value,length,arg_list)
	long 	value;
	int	length;
	va_list	*arg_list ;
{
	switch(length) {
	case 'l': *va_arg(*arg_list, long *)  = value ;
		  break ;
	case 'h': *va_arg(*arg_list, short *) = (short)value ;
		  break ;
	default:  *va_arg(*arg_list, int *)   = (int)value ;
		  break ;
	}
}

int
_doscan(iop, fmt, va_alist)
register FILE *iop;
register unsigned char *fmt;
va_list va_alist;
{
	char tab[NCHARS];
	register int ch;
	int 	nmatch = 0,
		len,
		inchar,
		stow,
		size,
		chars_scanned = 0 ;

	/*******************************************************
	 * Main loop: reads format to determine a pattern,
	 *		and then goes to read input stream
	 *		in attempt to match the pattern.
	 *******************************************************/
	for( ; ; ) {
		if((ch = *fmt++) == '\0')
			return(nmatch); /* end of format */
		if(isspace(ch)) {
			while(isspace(inchar = getc(iop)))
				chars_scanned++;
			if(ungetc(inchar, iop) != EOF)
				continue;
			break;
		}
		if(ch != '%' || (ch = *fmt++) == '%') {
			if((inchar = getc(iop)) == ch) {
				chars_scanned++;
				continue;
			}
			if(ungetc(inchar, iop) != EOF)
				return(nmatch); /* failed to match input */
			break;
		}
		if(ch == '*') {
			stow = 0;
			ch = *fmt++;
		} else
			stow = 1;

		for(len = 0; isdigit(ch); ch = *fmt++)
			len = len * 10 + ch - '0';
		if(len == 0)
			len = MAXINT;

		if((size = ch) == 'l' || size == 'h' || size == 'L')
			ch = *fmt++;
		if(ch == '\0' ||
		    ch == '[' && (fmt = setup(fmt, tab)) == NULL)
			return(EOF); /* unexpected end of format */
		if(isupper(ch)) { /* no longer documented */
#ifdef _POSIX_SOURCE		  /* 005 */
			if(ch != 'E' && ch != 'F' && ch != 'G' && ch != 'X')
#endif
				size = 'l';
			ch = _tolower(ch);
		}
		if (ch == 'n') {
			if (stow) {
				store_int((long)chars_scanned,size,&va_alist);
			}
			continue;
		}
		if (ch == 'p') { /* pointer - convert to %8x */
			ch = 'x';
			len = 8;
		}
		if(ch != 'c' && ch != '[') {
			while(isspace(inchar = getc(iop)))
				chars_scanned++;
			if(ungetc(inchar, iop) == EOF)
				break;
		}
		if((size = (ch == 'c' || ch == 's' || ch == '[') ?
		    string(stow,ch,len,tab,iop,&chars_scanned,&va_alist) :
		    number(stow,ch,len,size,iop,&chars_scanned,&va_alist))
		   != 0)
			nmatch += stow;
		if(va_alist == NULL) /* end of input */
			break;
		if(size == 0)
			return(nmatch); /* failed to match input */
	}
	return(nmatch != 0 ? nmatch : EOF); /* end of input */
}

/***************************************************************
 * Functions to read the input stream in an attempt to match incoming
 * data to the current pattern from the main loop of _doscan().
 ***************************************************************/
static int
number(stow, type, len, size, iop, chars_scanned, listp)
int stow, type, len, size;
register FILE *iop;
register int *chars_scanned ;
va_list *listp;
{
	char numbuf[64];
	register char *np = numbuf;
	register int c, base;
	int	digitseen = 0,
		dotseen = 0,
		expseen = 0,
		floater = 0,
		negflg = 0;
	long lcval = 0;
	extern char _lc_radix;	/* INTL. the international radix character */

	switch(type) {
	case 'e':
	case 'f':
	case 'g':
		floater++;
		/* FALLTHROUGH */
	case 'd':
	case 'u':
		base = 10;
		break;
	case 'o':
		base = 8;
		break;
	case 'x':
		base = 16;
		break;
	case 'i':
		base = 0;	/* General integer */
		break ;
	default:
		return(0); /* unrecognized conversion character */
	}

	switch(c = getc(iop)) {
	case '-':
		negflg++;
	case '+': /* fall-through */
		len--;
		c = getc(iop);
		(*chars_scanned)++ ;
	}
	if (base == 0) {	/* general integer, test format */
		if (c == '0') {
			c = getc(iop) ;
			if (c == 'x' || c == 'X') {
				/* 0x or 0X mean hex, get next digit and
				 * mark the 0[Xx] as scanned.
				 */
				base = 16 ;
				len -= 2;
				c = getc(iop);
				*chars_scanned += 2 ;
			} else if (isdigit(c)) {
				/* 0 followed by a digit is octal,
				 * mark 0 as scanned.
				 */
				base = 8 ;
				len--;
				(*chars_scanned)++ ;
			} else {
				/* Decimal 0, restart scan at 0.
				 */
				ungetc(c,iop); 
				c = '0';
				base = 10 ;
			}
		} else
			base = 10 ;
	} 
	    for( ; --len >= 0 && np < &numbuf[63]; *np++ = c, c = getc(iop)) {
		if(isdigit(c) || base == 16 && isxdigit(c)) {
			int digit = c - (isdigit(c) ? '0' :
			    isupper(c) ? 'A' - 10 : 'a' - 10);
			if(digit >= base)
				break;
			if(stow && !floater)
				lcval = base * lcval + digit;
			digitseen++;
			(*chars_scanned)++ ;
			continue;
		}
		if(!floater)
			break;
		if(c == _lc_radix && !dotseen++) {	/* INTL */
			(*chars_scanned)++ ;
			continue;
		}
		if((c == 'e' || c == 'E') && digitseen && !expseen++) {
			(*chars_scanned)++ ;
			if (--len < 0 || np >= &numbuf[63])
				break;
			*np++ = c;
			c = getc(iop);
			if(isdigit(c) || c == '+' || c == '-' || isspace(c)) {
				(*chars_scanned)++ ;
				continue;
			}
		}
		break;
	    }
	if(stow && digitseen)
		if(floater) {
			register double dval;
	
			*np = '\0';
			dval = atof(numbuf);
			if(negflg)
				dval = -dval;
			if(size == 'l' || size == 'L')
				*va_arg(*listp, double *) = dval;
			else
				*va_arg(*listp, float *) = (float)dval;
		} else {
			/* suppress possible overflow on 2's-comp negation */
			if(negflg && lcval != HIBITL)
				lcval = -lcval;
			store_int(lcval,size,listp);
		}
	ungetc(c,iop);
	return(digitseen); /* successful match if non-zero */
}

static int
string(stow, type, len, tab, iop, chars_scanned, listp)
register int stow, type, len;
register char *tab;
register FILE *iop;
int *chars_scanned;
va_list *listp;
{
	register int ch;
	register char *ptr;
	char *start;

	start = ptr = stow ? va_arg(*listp, char *) : NULL;
	if(type == 'c' && len == MAXINT)
		len = 1;
	while((ch = getc(iop)) != EOF &&
	    !(type == 's' && isspace(ch) || type == '[' && tab[ch])) {
		if(stow)
			*ptr = ch;
		ptr++;
		(*chars_scanned)++;
		if(--len <= 0)
			break;
	}
	if(ch == EOF || len > 0 && ungetc(ch, iop) == EOF)
		*listp = NULL; /* end of input */
	if(ptr == start)
		return(0); /* no match */
	if(stow && type != 'c')
		*ptr = '\0';
	return(1); /* successful match */
}
