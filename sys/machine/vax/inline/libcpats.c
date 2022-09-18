/* Copyright (c) 1984 Regents of the University of California */
/*
static char sccsid[] = "@(#)libcpats.c	1.2	(Berkeley)	8/18/84";
*/

#ifndef lint
static char *sccsid = "@(#)libcpats.c	4.3	(ULTRIX)	9/10/90";
#endif not lint

#include "inline.h"

/*
 * Pattern table for the C library.
 */
struct pats libc_ptab[] = {

#ifdef vax
	{ "1,_fgetc\n",
"	sobgeq	*(sp),1f\n\
	calls	$1,__filbuf\n\
	jbr     2f\n\
1:\n\
	addl3	$4,(sp)+,r1\n\
	movzbl	*(r1),r0\n\
	incl	(r1)\n\
2:\n" },

	{ "2,_fputc\n",
"	sobgeq	*4(sp),1f\n\
	calls	$2,__flsbuf\n\
	jbr	2f\n\
1:\n\
	movq	(sp)+,r0\n\
	movb	r0,*4(r1)\n\
	incl	4(r1)\n\
2:\n" },

/* make unsigned division done inline */
	{ "2,udiv\n",
"	movl	(sp)+,r0\n\
	movl	(sp)+,r2\n\
	jeql	1f\n\
	cmpl	r2,$1\n\
	jleq	2f\n\
1:\n\
	clrl	r1\n\
	ediv	r2,r0,r0,r2\n\
	jbr	4f\n\
2:	\n\
	jeql	4f\n\
	cmpl	r0,r2\n\
	jgequ	3f\n\
	clrl	r0\n\
	jbr	4f\n\
3:	\n\
	movl	$1,r0\n\
4:	\n" },

/* make unsigned division remainder done inline */
	{ "2,urem\n",
"	movl	(sp)+,r0\n\
	movl	(sp)+,r2\n\
	jeql	1f\n\
	cmpl	r2,$1\n\
	jleq	2f\n\
1:\n\
	clrl	r1\n\
	ediv	r2,r0,r2,r0\n\
	jbr	4f\n\
2:\n\
	jneq	3f\n\
	clrl	r0\n\
	jbr	4f\n\
3:\n\
	cmpl	r0,r2\n\
	jlssu	4f\n\
	subl2	r2,r0\n\
4:\n" },

	{ "1,_strlen\n",
"	movl	(sp)+,r5\n\
	movl	r5,r1\n\
1:\n\
	locc	$0,$65535,(r1)\n\
	jeql	1b\n\
	subl3	r5,r1,r0\n" },
#endif vax

#ifdef mc68000
/* someday... */
#endif mc68000

	{ "", "" }
};
