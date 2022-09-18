/*	@(#)atod.s	4.1	(ULTRIX)	7/3/90	*/
/* ------------------------------------------------------------------ */
/* | Copyright Unpublished, MIPS Computer Systems, Inc.  All Rights | */
/* | Reserved.  This software contains proprietary and confidential | */
/* | information of MIPS and its suppliers.  Use, disclosure or     | */
/* | reproduction is prohibited without the prior express written   | */
/* | consent of MIPS.                                               | */
/* ------------------------------------------------------------------ */
/* $Header: atod.s,v 1.1 87/02/16 11:17:15 dce Exp $ */

#include <mips/regdef.h>

.extern errno 4
/* x = _atod (buffer, ndigit, exp) */
/* Convert ndigits from buffer.  Digits in binary (0..9).
   1 <= ndigits <= 17.  -324 <= exp <= 308 */
/* Return double value. */
.globl _atod
.ent _atod
_atod:
	.frame	sp, 0, t9
	move	t9, ra

	/* Convert digits to 64-bit integer * 2 (* 2 so that multiplication
	   by 10^N leaves binary point between registers). */
	addu	a1, a0
	li	t4, 0
	li	t5, 0
	b	11f
10:
	/* Multiply by 5 */
	srl	t7, t4, 30
	sll	t1, t4, 2
	not	t6, t1
	sltu	t6, t6, t4
	addu	t0, t7, t6
	addu	t4, t1

	sll	t1, t5, 2
	addu	t5, t1
	addu	t5, t0

11:	/* Add digit */
	lbu	t2, (a0)
	addu	a0, 1
	not	t3, t4
	sltu	t3, t3, t2
	addu	t4, t2
	addu	t5, t3

	/* Multiply by 2 */
	sll	t5, 1
	srl	t7, t4, 31
	or	t5, t7
	sll	t4, 1

	bne	a0, a1, 10b

	/* Dispatch zero */
	bne	t4, 0, 12f
	beq	t5, 0, 50f
12:

	/* Multiply 64-bit integer by 10^N, leaving binary point
	   between t2/t1. */
	move	t0, a2
	jal	_tenscale

	/* Normalize */

	/* Word normalize */
	bne	t3, 0, 20f
	move	t3, t2		# hi word zero, shift up by 32
	move	t2, t1
	move	t1, t0
	li	t0, 0
	subu	v1, 32		# adjust for 32-bit shift

	/* Find normalization shift */
20:	srl	a2, t3, 16
	li	t6, 16+8
	bne	a2, 0, 21f
	subu	t6, 16
21:	srl	a2, t3, t6
	addu	t6, 4
	bne	a2, 0, 22f
	subu	t6, 8
22:	srl	a2, t3, t6
	addu	t6, 2
	bne	a2, 0, 23f
	subu	t6, 4
23:	srl	a2, t3, t6
	addu	t6, 1
	bne	a2, 0, 24f
	subu	t6, 2
24:	srl	a2, t3, t6
	xor	a2, 1
	subu	t6, a2

	addu	v1, t6
	addu	v1, 1023+32	# 1023 = bias, 32 to compensate for binary
				# point after t2 instead of t3
	bgtz	v1, 29f
	/* will produce denorm */
	subu	t6, v1		# exponent pins at 0
	addu	t6, 1
	li	v1, 1		# 1 instead of 0 to compensate for hidden bit
				# subtraction below
	blt	t6, 52, 29f
	move	t0, t1
	move	t1, t2
	move	t2, t3
	li	t3, 0
	subu	t6, 32
29:

	/* Normalize */
	subu	a3, t6, 20	# normalize so that msb is bit 20 (hidden bit)
	beq	a3, 0, 26f
	negu	t7, a3
	bltz	a3, 25f
	/* quad-word right shift */
	srl	t0, a3		# necessary only for round to even done below
	sll	a2, t1, t7	# ditto
	or	t0, a2		# ditto
	srl	t1, a3
	sll	a2, t2, t7
	or	t1, a2
	srl	t2, a3
	sll	a2, t3, t7
	or	t2, a2
	srl	t3, a3
	b	26f
25:	/* quad-word left shift */
	sll	t3, t7
	srl	a2, t2, a3
	or	t3, a2
	sll	t2, t7
	srl	a2, t1, a3
	or	t2, a2
	sll	t1, t7
	srl	a2, t0, a3
	or	t1, a2
	sll	t0, t7		# necessary only for round to even done below

26:	/* Round */
	bgez	t1, 27f
	sll	t1, 1
	bne	t1, 0, 28f
	bne	t0, 0, 28f
	and	t0, t2, 1
	beq	t0, 0, 27f
28:
	addu	t2, 1
	bne	t2, 0, 27f
	addu	t3, 1
	sll	a3, t3, 31-21	# overflow into bit 21?
	bgez	a3, 27f
	addu	v1, 1
	srl	t3, 1		# t3/t2 is 200000/00000000, so no need
				# for dw shift
27:	bgeu	v1, 2047, 40f
	subu	v1, 1		# subtract out hidden bit
	sll	v1, 20
	addu	t3, v1
	mtc1	t2, $f0
	mtc1	t3, $f1
	j	t9
	
40:	lwc1	$f1, infinity
	mtc1	$0, $f0
	li	t1, 34
	sw	t1, errno
	j	t9

50:	/* zero */
	mtc1	$0, $f0
	mtc1	$0, $f1
	j	t9
.end _atod

/*.rdata*/.sdata
infinity:
	.word 0x7ff00000
