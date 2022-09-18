/*	@(#)fp_class.s	4.1	(ULTRIX)	7/3/90				      */
/* ------------------------------------------------------------------ */
/* | Copyright Unpublished, MIPS Computer Systems, Inc.  All Rights | */
/* | Reserved.  This software contains proprietary and confidential | */
/* | information of MIPS and its suppliers.  Use, disclosure or     | */
/* | reproduction is prohibited without the prior express written   | */
/* | consent of MIPS.                                               | */
/* ------------------------------------------------------------------ */
/* $Header: fp_class.s,v 1.1 87/02/16 11:17:27 dce Exp $ */

#include <mips/regdef.h>
#include <mips/asm.h>
#include <fp_class.h>
#include <mips/softfp.h>

/*
 * fp_class_d(d)	
 * double d;
 */
LEAF(fp_class_d)
	# Get the double from f12
	mfc1	a2,$f13
	mfc1	a3,$f12

	# Break down the double into sign, exponent and fraction
	srl	a1,a2,DEXP_SHIFT
	move	a0,a1
	and	a1,DEXP_MASK
	and	a0,SIGNBIT>>DEXP_SHIFT
	and	a2,DFRAC_MASK

	# Check for infinities and Nan's
	bne	a1,DEXP_INF,4f
	bne	a2,zero,2f
	bne	a3,zero,2f
	bne	a0,zero,1f
	li	v0,FP_POS_INF
	j	ra
1:
	li	v0,FP_NEG_INF
	j	ra
2:	# Check to see if this is a signaling NaN
	and	a0,a2,DSNANBIT_MASK
	beq	a0,zero,3f
	li	v0,FP_SNAN
	j	ra
3:	li	v0,FP_QNAN
	j	ra
4:
	# Check for zeroes and denorms
	bne	a1,zero,8f
	bne	a2,zero,6f
	bne	a3,zero,6f
	bne	a0,zero,5f
	li	v0,FP_POS_ZERO
	j	ra
5:	li	v0,FP_NEG_ZERO
	j	ra
6:	bne	a0,zero,7f
	li	v0,FP_POS_DENORM
	j	ra
7:	li	v0,FP_NEG_DENORM
	j	ra
8:
	# It is just a normal number
	bne	a0,zero,9f
	li	v0,FP_POS_NORM
	j	ra
9:	li	v0,FP_NEG_NORM
	j	ra
END(fp_class_d)

/*
 * fp_class_f(f)	
 * float d;
 */
LEAF(fp_class_f)
	# Get the float from f12
	mfc1	a2,$f12

	# Break down the float into sign, exponent and fraction
	srl	a1,a2,SEXP_SHIFT
	move	a0,a1
	and	a1,SEXP_MASK
	and	a0,SIGNBIT>>SEXP_SHIFT
	and	a2,SFRAC_MASK

	# Check for infinities and Nan's
	bne	a1,SEXP_INF,4f
	bne	a2,zero,2f
	bne	a0,zero,1f
	li	v0,FP_POS_INF
	j	ra
1:
	li	v0,FP_NEG_INF
	j	ra
2:	# Check to see if this is a signaling NaN
	and	a0,a2,SSNANBIT_MASK
	beq	a0,zero,3f
	li	v0,FP_SNAN
	j	ra
3:	li	v0,FP_QNAN
	j	ra
4:
	# Check for zeroes and denorms
	bne	a1,zero,8f
	bne	a2,zero,6f
	bne	a0,zero,5f
	li	v0,FP_POS_ZERO
	j	ra
5:	li	v0,FP_NEG_ZERO
	j	ra
6:	bne	a0,zero,7f
	li	v0,FP_POS_DENORM
	j	ra
7:	li	v0,FP_NEG_DENORM
	j	ra
8:
	# It is just a normal number
	bne	a0,zero,9f
	li	v0,FP_POS_NORM
	j	ra
9:	li	v0,FP_NEG_NORM
	j	ra
END(fp_class_f)
