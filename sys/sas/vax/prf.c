/*
 * prf.c
 */

#ifndef lint
static char *sccsid = "@(#)prf.c	4.1	(ULTRIX)	7/2/90";
#endif lint
/************************************************************************
 *									*
 *			Copyright (c) 1985,87,88 by				*
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
/*	prf.c	6.1	83/07/29	*/

/*
 * MODIFICATION HISTORY
 *
 * 15-Feb-88 - fred (Fred Canter)
 *	Changes to recognize VAX420 (CVAXstar/PVAX) CPU.
 *
 * 05-May-87 - afd
 *	Added support for Mayfair console.
 *
 * tresvik 9-sep-84
 *
 *	Removed the keyboard side of this module (gets and getchar)
 *	and created a separate module called get.c
 *	
 */

#include "../../h/param.h"
#include "vmb.h"
#include "../../machine/vax/mtpr.h"
#include "../../machine/vax/cons.h"
#include "../../machine/vax/cpu.h"
#include "../../machine/common/cpuconf.h"

static char *string = NULL;
extern qv_init(), qd_init(), cpu, cpuext;
extern c640getc(), c640putc();
extern c650getc(), c650putc();
extern c60getc(), c60putc(), c60init();
int	cons_rdy=0;


int (*vcons_init[])() = {
	qd_init,
	qv_init,
	0
};
int (*v_getc)()=0, (*v_putc)()=0;

/*
 * configure the console for MVAX, VAXstar, and Mayfair
 */
cons_init()
{
	int i;
	if ((cpu == CVAX_CPU) && (cpuext == SB_KA650)) {
		/*
		 * Mayfair: We use the console "call-back" routines
		 *	in low console ROM.
		 */
		v_getc = c650getc;
		v_putc = c650putc;
	}
	else if ((cpu == CVAX_CPU) && (cpuext == ST_KA60)) {
		/*
		 * Firefox: We use the console routines pointed to
		 *	by the get_character and put_character 
		 *	pointers in the CTSIA.  First we read the
		 *	address of the CTSIA from NVR, and put it
		 *	into the variable ctsia.
		 */
		c60init();
		v_getc = c60getc;
		v_putc = c60putc;
	}
	else if (cpuext == ST_VAXSTAR) {
		/*
		 * VAXstar & CVAXstar
		 */
		v_getc = c640getc;
		v_putc = c640putc;
	} else {
		/*
		 * MVAX QVSS or QDSS
		 */
		for( i = 0 ; vcons_init[i] && (*vcons_init[i])() != 1 ; i++ );
	}
	cons_rdy=1;

}
/*
 * Scaled down version of C Library printf.
 * Used to print diagnostic information directly on console tty.
 * Since it is not interrupt driven, all system activities are
 * suspended.  Printf should not be used for chit-chat.
 *
 * One additional format: %b is supported to decode error registers.
 * Usage is:
 *	printf("reg=%b\n", regval, "<base><arg>*");
 * Where <base> is the output base expressed as a control character,
 * e.g. \10 gives octal; \20 gives hex.  Each arg is a sequence of
 * characters, the first of which gives the bit number to be inspected
 * (origin 1), and the next characters (up to a control character, i.e.
 * a character <= 32), give the name of the register.  Thus
 *	printf("reg=%b\n", 3, "\10\2BITTWO\1BITONE\n");
 * would produce output:
 *	reg=2<BITTWO,BITONE>
 */
/*VARARGS1*/
printf(fmt, x1)
	char *fmt;
	unsigned x1;
{
	if(!cons_rdy && (cpu == MVAX_I || cpu == MVAX_II ||
	    (cpu == CVAX_CPU && cpuext == ST_VAXSTAR) ||
	    (cpu == CVAX_CPU && cpuext == SB_KA650)||
	    (cpu == CVAX_CPU && cpuext == ST_KA60)))
		cons_init();
	string = NULL;
	prf(fmt, &x1);
}

/*VARARGS1*/
sprintf(cptr, fmt, x1)
	char *cptr;
	char *fmt;
	unsigned x1;
{

	string = cptr;
	prf(fmt, &x1);
}

prf(fmt, adx)
	register char *fmt;
	register u_int *adx;
{
	register int b, c, i;
	char *s;
	int any;

loop:
	while ((c = *fmt++) != '%') {
		if(c == '\0')
			return;
		putchar(c);
	}
again:
	c = *fmt++;
	/* THIS CODE IS VAX DEPENDENT IN HANDLING %l? AND %c */
	switch (c) {

	case 'l':
		goto again;
	case 'x': case 'X':
		b = 16;
		goto number;
	case 'd': case 'D':
	case 'u':		/* what a joke */
		b = 10;
		goto number;
	case 'o': case 'O':
		b = 8;
number:
		printn((u_long)*adx, b);
		break;
	case 'c':
		b = *adx;
		for (i = 24; i >= 0; i -= 8)
			if (c = (b >> i) & 0x7f)
				putchar(c);
		break;
	case 'b':
		b = *adx++;
		s = (char *)*adx;
		printn((u_long)b, *s++);
		any = 0;
		if (b) {
			putchar('<');
			while (i = *s++) {
				if (b & (1 << (i-1))) {
					if (any)
						putchar(',');
					any = 1;
					for (; (c = *s) > 32; s++)
						putchar(c);
				} else
					for (; *s > 32; s++)
						;
			}
			putchar('>');
		}
		break;

	case 's':
		s = (char *)*adx;
		while (c = *s++)
			putchar(c);
		break;
	}
	adx++;
	goto loop;
}

/*
 * Printn prints a number n in base b.
 * We don't use recursion to avoid deep kernel stacks.
 */
printn(n, b)
	u_long n;
{
	char prbuf[11];
	register char *cp;

	if (b == 10 && (int)n < 0) {
		putchar('-');
		n = (unsigned)(-(int)n);
	}
	cp = prbuf;
	do {
		*cp++ = "0123456789abcdef"[n%b];
		n /= b;
	} while (n);
	do
		putchar(*--cp);
	while (cp > prbuf);
}

extern (*v_putc)();
/*
 * Print a character on console.
 * Attempts to save and restore device
 * status.
 *
 * Whether or not printing is inhibited,
 * the last MSGBUFS characters
 * are saved in msgbuf for inspection later.
 */
putchar(c)
	register c;
{
	register i, s, timo;

	timo = 30000;
	/*
	 * Try waiting for the console tty to come ready,
	 * otherwise give up after a reasonable time.
	 */
	if( string ) {
		*string++ = c;
		*string = '\0';
		return;
	}
	if( v_putc ) {
		(*v_putc)( c );
		if( c == '\n' )
			(*v_putc)( '\r' );
	} else {
	 	/*
	 	 * If a character is waiting on the input side, see
	 	 * if it is a CONTROL_S.  If so, spin until the
	 	 * CONTROL_Q comes along.
	 	 */
 	 	if (mfpr(RXCS)&RXCS_DONE)
 			if ((i = mfpr(RXDB)&0177) == CONTROL_S)
 				while (i != CONTROL_Q) {	
 					while ((mfpr(RXCS)&RXCS_DONE) == 0)
 						;
	 				i = mfpr(RXDB)&0177;
 				}
		while((mfpr(TXCS)&TXCS_RDY) == 0)
			if(--timo == 0)
				break;
		if(c == 0)
			return;
		s = mfpr(TXCS);
		mtpr(TXCS,0);
		mtpr(TXDB, c&0xff);
		if(c == '\n')
			putchar('\r');
		putchar(0);
		mtpr(TXCS, s);
	}
}
