#ifndef lint
#static char	*sccsid = "@(#)d_cbrt.s	4.1	(ULTRIX)	7/17/90";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1986 by				*
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
*
*			Modification History
*
*	David Metsky, 12/18/86
* 001	Adapted from cbrt.s 1.1 (Berkeley) 5/23/85
*
*************************************************************************/

# @(#)cbrt.s	1.1 (Berkeley) 5/23/85

# double cbrt(double arg)
# W. Kahan, 10/13/80. revised 1/13/84 for keeping sign symmetry
# error check by E LeBlanc, 8/18/82
# Revised and tested by K.C. Ng, 5/2/85  
# Max error less than 0.667 ulps (unit in the last places)
	.globl	_cbrt
	.globl	_d_cbrt
	.globl  _dcbrt_
	.text
	.align	1

_cbrt:
_d_cbrt:
	.word	0x00fc		# save r2 to r7
	movq	4(ap),r0	# r0 = argument x
	jmp 	dcbrt2
_dcbrt_:
	.word	0x00fc		# save r2 to r7
	movq	*4(ap),r0	# r0 = argument x

dcbrt2:	bicw3	$0x807f,r0,r2	# biased exponent of x
	jeql	return		# dcbrt(0)=0  dcbrt(res)=res. operand
	bicw3	$0x7fff,r0,ap	# ap has sign(x)
	xorw2	ap,r0		# r0 is abs(x)
	movl	r0,r2		# r2 has abs(x)
	rotl	$16,r2,r2	# r2 = |x| with bits unscrambled
	divl2	$3,r2		# rough dcbrt with bias/3
	addl2	B,r2		# restore bias, diminish fraction
	rotl	$16,r2,r2	# r2=|q|=|dcbrt| to 5 bits
	mulf3	r2,r2,r3	# r3 =qq
	divf2	r0,r3		# r3 = qq/x
	mulf2	r2,r3
	addf2	C,r3		# r3 = s = C + qqq/x
	divf3	r3,D,r4		# r4 = D/s
	addf2	E,r4
	addf2	r4,r3		# r3 = s + E + D/s
	divf3	r3,F,r3		# r3 = F / (s + E + D/s)
	addf2	G,r3		# r3 = G + F / (s + E + D/s)
	mulf2	r3,r2		# r2 = qr3 = new q to 23 bits
	clrl	r3		# r2:r3 = q as double float
	muld3	r2,r2,r4	# r4:r5 = qq exactly
	divd2	r4,r0		# r0:r1 = x/(q*q) rounded
	subd3	r2,r0,r6	# r6:r7 = x/(q*q) - q exactly
	movq    r2,r4		# r4:r5 = q
	addw2	$0x80,r4	# r4:r5 = 2 * q
	addd2	r0,r4		# r4:r5 = 2*q + x/(q*q)
	divd2	r4,r6		# r6:r7 = (x/(q*q)-q)/(2*q+x/(q*q))
	muld2	r2,r6		# r6:r7 = q*(x/(q*q)-q)/(2*q+x/(q*q))
	addd3	r6,r2,r0	# r0:r1 = q + r6:r7
	bisw2	ap,r0		# restore the sign bit
return:
	ret			# error less than 0.667 ulps

.data
.align	2
B :	.long		 721142941		# (86-0.03306235651)*(2^23)
C :	.float		0f0.5428571429		# 19/35
D :	.float		0f-0.7053061224		# -864/1225
E :	.float		0f1.414285714		# 99/70
F :	.float		0f1.607142857		# 45/28
G :	.float		0f0.3571428571		# 5/14

