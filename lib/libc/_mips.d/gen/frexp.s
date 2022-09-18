/*	@(#)frexp.s	4.1	(ULTRIX)	7/3/90				      */
/* ------------------------------------------------------------------ */
/* | Copyright Unpublished, MIPS Computer Systems, Inc.  All Rights | */
/* | Reserved.  This software contains proprietary and confidential | */
/* | information of MIPS and its suppliers.  Use, disclosure or     | */
/* | reproduction is prohibited without the prior express written   | */
/* | consent of MIPS.                                               | */
/* ------------------------------------------------------------------ */
/* $Header: frexp.s,v 1.1 87/02/16 11:17:28 dce Exp $ */

#include <mips/regdef.h>
#include <mips/asm.h>
#include <fp_class.h>
#include <mips/softfp.h>

#define	FRAME_SIZE	24
#define	LOCAL_SIZE	0
#define	F12_OFFSET	FRAME_SIZE+4*0
#define	A2_OFFSET	FRAME_SIZE+4*2
#define	RA_OFFSET	FRAME_SIZE-LOCAL_SIZE-4*1

/*
 * double frexp(value, eptr)
 * double value; int *eptr;
 *   In assembly:
 *	value -- $f12 ($f13,$f12) as a double
 *	eptr  -- a2
 *   and the return values:
 *      return value -- $f0
 *	*eptr        -- 0(a2)
 *
 * Frexp() returns a double x such that x = 0 or 0.5 <= |x| < 1.0
 * and stores an integer n such that value = x * 2 ** n indirectly
 * through eptr.
 */
NESTED(frexp, FRAME_SIZE, ra)
	.mask	0x80000000, -(LOCAL_SIZE+4)

	subu	sp,FRAME_SIZE
	s.d	$f12,F12_OFFSET(sp)
	sw	a2,A2_OFFSET(sp)
	sw	ra,RA_OFFSET(sp)

	# get the class of the floating point value and switch on it.
	jal	fp_class_d
	sll	v0,2	
	lw	v0,jmp_tbl(v0)
	j	v0

	.rdata
jmp_tbl:
	.word	nan
	.word	nan
	.word	inf
	.word	inf
	.word	norm
	.word	norm
	.word	denorm
	.word	denorm
	.word	zeroval
	.word	zeroval
	.text
	
nan:	# For NaN's return a the default quiet nan for both the return
	# value and the integer n.
	li	a0,DQUIETNAN_LESS
	li	a1,DQUIETNAN_LEAST
	mtc1	a0,$f1
	mtc1	a1,$f0
	lw	a2,A2_OFFSET(sp)
	li	a0,WQUIETNAN_LEAST
	sw	a0,0(a2)
	lw	ra,RA_OFFSET(sp)
	addu	sp,FRAME_SIZE
	j	ra

inf:	# For infinities return the infinity and return the maximum value
	# for the integer n.
	l.d	$f0,F12_OFFSET(sp)
	lw	a2,A2_OFFSET(sp)
	li	a0,WORD_MAX
	sw	a0,0(a2)
	lw	ra,RA_OFFSET(sp)
	addu	sp,FRAME_SIZE
	j	ra

norm:	# For normalized numbers return the value with the exponent changed
	# to -1.  The interger n gets the exponent of the value minus 1.
	l.d	$f0,F12_OFFSET(sp)
	mfc1	a0,$f1
	and	a1,a0,~(DEXP_MASK<<DEXP_SHIFT)
	or	a1,((DEXP_BIAS-1)<<DEXP_SHIFT)
	mtc1	a1,$f1
	lw	a2,A2_OFFSET(sp)
	srl	a0,DEXP_SHIFT
	and	a0,DEXP_MASK
	subu	a0,DEXP_BIAS-1
	sw	a0,0(a2)
	lw	ra,RA_OFFSET(sp)
	addu	sp,FRAME_SIZE
	j	ra

denorm:	# For denormalized numbers return the value with the exponent changed
	# to -1 and the value normalized.  The interger n gets the exponent of
	# the value minus 1 adjusted for the denorms value.
	l.d	$f0,F12_OFFSET(sp)
	mfc1	a2,$f1
	mfc1	a3,$f0

	# set the exponent (a1) separate the fraction (a2,a3) and sign (a0)
	and	a0,a2,SIGNBIT
	li	a1,-DEXP_BIAS+1-1
	and	a2,DFRAC_MASK

	/*
	 * Renormalize the denormalized double value.
	 */
	/*
	 * The first step in this process is to determine where the first
	 * one bit is in the fraction (a2,a3).  After this series of tests
	 * the shift count to shift the fraction left so the first 1 bit is
	 * in the high bit will be in t9.  This sequence of code uses registers
	 * v0,v1 and t9 (it could be done with two but with reorginization this
	 * is faster).
	 */
	move	v0,a2
	move	t9,zero
	bne	a2,zero,1f
	move	v0,a3
	addu	t9,32
1:
	srl	v1,v0,16
	bne	v1,zero,1f
	addu	t9,16
	sll	v0,16
1:	srl	v1,v0,24
	bne	v1,zero,2f
	addu	t9,8
	sll	v0,8
2:	srl	v1,v0,28
	bne	v1,zero,3f
	addu	t9,4
	sll	v0,4
3:	srl	v1,v0,30
	bne	v1,zero,4f
	addu	t9,2
	sll	v0,2
4:	srl	v1,v0,31
	bne	v1,zero,5f
	addu	t9,1
5:
	/*
	 * Now that the it is known where the first one bit is calculate the
	 * amount to shift the fraction to put the first one bit in the
	 * implied 1 position (also the amount to adjust the exponent by).
	 * Then adjust the exponent and shift the fraction.
	 */
	subu	t9,DFRAC_LEAD0S	# the calulated shift amount
	subu	a1,t9		# adjust the exponent
	blt	t9,32,1f
	subu	t9,32		# shift the fraction for >= 32 bit shifts
	sll	a2,a3,t9
	move	a3,zero
	b	2f
1:
	negu	v0,t9		# shift the fraction for < 32 bit shifts
	addu	v0,32
	sll	a2,t9
	srl	v1,a3,v0
	or	a2,v1
	sll	a3,t9
2:
	# Now put the normalized value together with a -1 exponent
	and	a2,~DIMP_1BIT
	or	a2,((DEXP_BIAS-1)<<DEXP_SHIFT)
	or	a2,a0
	mtc1	a2,$f1
	mtc1	a3,$f0

	# Now store the integer n
	lw	a2,A2_OFFSET(sp)
	sw	a1,0(a2)

	lw	ra,RA_OFFSET(sp)
	addu	sp,FRAME_SIZE
	j	ra

zeroval: # For zeroes return zero for both the return value and the
	 # integer n.
	l.d	$f0,F12_OFFSET(sp)
	lw	a2,A2_OFFSET(sp)
	sw	zero,0(a2)
	lw	ra,RA_OFFSET(sp)
	addu	sp,FRAME_SIZE
	j	ra

END(frexp)
