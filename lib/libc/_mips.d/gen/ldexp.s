/*	@(#)ldexp.s	4.1	(ULTRIX)	7/3/90      */
/* ------------------------------------------------------------------ */
/* | Copyright Unpublished, MIPS Computer Systems, Inc.  All Rights | */
/* | Reserved.  This software contains proprietary and confidential | */
/* | information of MIPS and its suppliers.  Use, disclosure or     | */
/* | reproduction is prohibited without the prior express written   | */
/* | consent of MIPS.                                               | */
/* ------------------------------------------------------------------ */
/* $Header: ldexp.s,v 1.1 87/02/16 11:17:32 dce Exp $ */

/*
 * double ldexp (value, exp)
 * double value;
 * int exp;
 *
 * Ldexp returns value*2**exp, if that result is in range.
 * If underflow occurs, it returns zero.  If overflow occurs,
 * it returns a value of appropriate sign and largest
 * possible magnitude.  In case of either overflow or underflow,
 * errno is set to ERANGE.  Note that errno is not modified if
 * no error occurs.
 */

#include <mips/regdef.h>

.extern	errno 4

/*.rdata*/.sdata
infinity:
	.word 0x7ff00000

.text
.globl ldexp
.ent ldexp
ldexp:
	.frame	sp, 0, ra
.set noreorder
	mfc1	t0, $f13
	mov.d	$f0, $f12
	sll	t1, t0, 1
	srl	t1, 21
	beq	t1, 0, denorm
	 addu	t1, a2
	ble	t1, 0, 3f
	 slt	t2, t1, 2047
	beq	t2, 0, 2f
	 sll	a2, 20
	addu	t0, a2
	mtc1	t0, $f1
1:	j	ra
	 nop
2:	/* Return infinity with appropriate sign. */
	c.un.d	$f12,$f12
	li	t1, 34
	bc1f	13f
	 mov.d	$f0, $f12
	j	ra

13:	 sw	t1, errno
	lwc1	$f1, infinity
	bge	t0, 0, 1b
	 mtc1	$0, $f0
	j	ra
	 neg.d	$f0
3:	/* Return denorm. */
	ble	t1, -52, 5f
	 mfc1	t5, $f13	# t4,t5: copy of unscaled number
	li	t2, 0x80000000
	sll	t5, 11		# clear exponent
	ble	t1, -31, 4f
	 or	t5, t2		# turn on hidden bit
	srl	t5, 11
	mfc1	t4, $f12
	subu	t1, 1		# now shift by amount of underflow
6:
	sll	t3, t5, t1
	srl	t2, t4, t1
	negu	t1
	srl	t4, t1
	or	t4, t3
	bge	t2, 0, 9f
	 srl	t5, t1
	/* Round up */
	addu	t4, 1
	sltu	t6, t4, 1
	sll	t2, 1
	bne	t2, 0, 9f
	 addu	t5, t6
	and	t4, -2
9:	beq	t5, 0, 8f
	 mtc1	t5, $f1
	bge	t0, 0, 1b
	 mtc1	t4, $f0
	j	ra
	 neg.d	$f0
4:	/* Return denorm with most significant word all zero. */
	mtc1	$0, $f1
	addu	t1, 20		# now shift by amount of underflow
	sll	t2, t5, t1
	negu	t1
	bge	t2, 0, 8f
	 srl	t4, t5, t1
	/* Round up */
	addu	t4, 1
	sltu	t6, t4, 1
	sll	t2, 1
	bne	t2, 0, 8f
	 mtc1	t6, $f1
	and	t4, -2
8:	beq	t4, 0, 5f
	 nop
	bge	t0, 0, 1b
	 mtc1	t4, $f0
	j	ra
	 neg.d	$f0
5:	/* Return zero. */
	li	t1, 34
	sw	t1, errno
	mtc1	$0, $f0
	bge	t0, 0, 1b
	 mtc1	$0, $f1
	j	ra
	 neg.d	$f0
.set reorder
denorm:				# input value was denormalized or zero
	mfc1	t4, $f12
	sll	t5, t0, 1	# remove sign
	srl	t5, 1
	bne	t5, 0, 132f
	beq	t4, 0, 1b	# zero returns zero
	ble	t1, 0, 6b	# denorm result, go do it
	/* 20-bit mantissa chunk (t5) == 0 & shift > 20 */
	bge	t4, 1<<(52-32), 132f
	subu	t1, 32
	move	t5, t4		# 32-bit shift is trivial
	li	t4, 0
132:	/* normalize (join here if t5 != 0) */
	ble	t1, 0, 6b	# result still denorm, join code
	bge	t5, 1<<19, 101f
	li	t3, 1<<19
131:
	subu	t1, 1
	sll	t5, 1
	srl	t2, t4, 32-1
	sll	t4, 1
	or	t5, t2
	blt	t5, t3, 131b
101:
	bge	t1, 2047, 2b	# overflow
	ble	t1, -52, 5b	# underflow
	blt	t1, 0, 6b	# will really be denorm, go shift back
	sll	t5, 1		# high mant. bit should now be 1; strip it out
	srl	t2, t4, 32-1
	sll	t4, 1
	or	t5, t2
	and	t5, 0xfffff
	sll	t1, 20		# paste in exponent
	or	t5, t1
	j	9b		# go return
.end ldexp
