/*
 * @(#)if_ln_copy.s	4.1	(ULTRIX)	1/22/91
 */
/************************************************************************
 *									*
 *			Copyright (c) 1989 by				*
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
 *	Modification History: if_ln_copy.s
 *
 * 19-Jan-91	Randall Brown
 *	Created file for copy routines written in assembly for performance
 *	reasons.
 */

#include "../machine/asm.h"
#include "../machine/reg.h"
#include "../machine/regdef.h"
#include "../machine/cpu.h"
#include "assym.h"


LEAF(ln_cpyout4x4)
	.set 	noreorder

/*
 * caddr_t
 * ln_cpyout4x4(from,to,len,off)
 * caddr_t from;				# a0 = from
 * caddr_t to;					# a1 = to
 * int len, off;				# a2 = len, a3 = off
 * {
 * 	register tmp1;
 * 	register caddr_t dp = from;
 * 	register int i;
 * 	int end4word;
 * 	register caddr_t lrbptr = (to+off);
 */
 	addu	a1, a1, a3

/*
 * 	if ((u_long) lrbptr & 0xf) {
 * 
 * 		end4word = (0x10 - ((u_long)lrbptr & 0xf));
 * 		tmp1 = ((len > end4word) ? end4word : len);
 * 
 * 		len -= tmp1;
 * 		while (tmp1--)
 * 		    *lrbptr++ = *dp++;
 * 
 *              if (((u_long)lrbptr & 0xf) == 0)
 *                  lrbptr += 0x10;
 * 	}
 */
 	andi	t6,a1,0xf	# is lrbptr on 4 word boundary

 	beq	t6,zero,5f	# if yes, do 4 word copy
 	nop

 	li	t7,16		# determine number of bytes
 	subu	t7,t7,t6	# to get to 4 word boundary
 	slt	t5,t7,a2	# if number is less than len
 	beq	t5,zero,2f	# only copy len
 	move	t8,a2
 	move	t8,t7

2:	subu	a2,a2,t8	# decrement total len

 	beq	t8,zero,4f	
	nop
3: 	lb	t9,0(a0)	# copy 1 byte at a time
	addiu	t8,t8,-1	# until tmp1(t8) is zero
 	sb	t9,0(a1)	
 	addiu	a0,a0,1
 	addiu	a1,a1,1
 	bne	t8,zero,3b
	nop

4:	andi	t3,a1,0xf	# if lrbptr on 4 word boundary
	bne	t3,zero,6f	# skip 4 word hole
	sra	t8,a2,4
	addiu	a1,a1,16

/*
 * 
 * 	tmp1 = (len >> 4);
 * 
 * 	len -= (tmp1 << 4);
 */
5:	sra	t8,a2,4		# t8 = len >> 4
6:	sll	t0,t8,4		
	subu	a2,a2,t0

/*
 *	NOTE: if dp is not aligned to a word boundary, then
 *	      lwl and lwr is used to fill in the bytes.
 *
 * 	for (; tmp1; tmp1--) {
 * 
 *         	register int temp0, temp1, temp2, temp3;
 * 
 * 	        temp0 = ((u_int *)dp)[0];
 * 		temp1 = ((u_int *)dp)[1];
 * 		temp2 = ((u_int *)dp)[2];
 * 		temp3 = ((u_int *)dp)[3];
 * 		((u_int *)lrbptr)[0] = temp0;
 * 		((u_int *)lrbptr)[1] = temp1;
 * 		((u_int *)lrbptr)[2] = temp2;
 * 		((u_int *)lrbptr)[3] = temp3;
 * 
 * 		lrbptr += 32;
 * 		dp += 16;
 * 	}
 */
 	beq	t8,zero,9f	# tmp1 == 0, then get fragment
	andi	t6,a0,0x3	# is dp word aligned ?
	bne	t6,zero,1f	# no, do unaligned copy
	andi	t7,t8,0x3	# number of 4 word blocks to get 
	beq	t7,zero,8f	#     16 word aligned
	subu	t8,t8,t7	# decrement total count 
7:	lw	t0,0(a0)	# do 4 word block until we can
	lw	t1,4(a0)	#     do 16 words blocks
	lw	t2,8(a0)
	lw	t3,12(a0)
	sw	t0,0(a1)
	sw	t1,4(a1)
	sw	t2,8(a1)
	sw	t3,12(a1)
	addiu	t7,t7,-1	# decrement counter
	addiu	a0,a0,16	# increment dp by 16
	bne	t7,zero,7b
	addiu	a1,a1,32	# increment lrbptr by 32
	beq	t8,zero,9f
	nop
8:	lw	t0,0(a0)	# do copy 16 words at a time
	lw	t1,4(a0)
	lw	t2,8(a0)
	lw	t3,12(a0)
	lw	t4,16(a0)	# no hole to skip
	lw	t5,20(a0)
	lw	t6,24(a0)
	lw	t7,28(a0)
	sw	t0,0(a1)
	sw	t1,4(a1)
	sw	t2,8(a1)
	sw	t3,12(a1)
	sw	t4,32(a1)	# skip 4 word hole
	sw	t5,36(a1)
	sw	t6,40(a1)
	sw	t7,44(a1)
	lw	t0,32(a0)
	lw	t1,36(a0)
	lw	t2,40(a0)
	lw	t3,44(a0)
	lw	t4,48(a0)
	lw	t5,52(a0)
	lw	t6,56(a0)
	lw	t7,60(a0)
	sw	t0,64(a1)
	sw	t1,68(a1)
	sw	t2,72(a1)
	sw	t3,76(a1)
	sw	t4,96(a1)
	sw	t5,100(a1)
	sw	t6,104(a1)
	sw	t7,108(a1)
	addiu	t8,t8,-4	# decrement tmp1 by 4
	addiu	a0,a0,64	# add 64 to dp
	bne	t8,zero,8b	# repeat if tmp != 0
	addiu	a1,a1,128	# add 128 to lrbptr
	b	9f
	nop
1:	lwr	t0,0(a0)
	lwl	t0,3(a0)
	lwr	t1,4(a0)
	lwl	t1,7(a0)
	lwr	t2,8(a0)
	lwl	t2,11(a0)
	lwr	t3,12(a0)
	lwl	t3,15(a0)
	sw	t0,0(a1)
	sw	t1,4(a1)
	sw	t2,8(a1)
	sw	t3,12(a1)
	addiu	a0,a0,16
	addiu	t8,t8,-1
	bne	t8,zero,1b
	addiu	a1,a1,32

/*
 * 
 *
 * 	if (len) {
 * 	        tmp1 = len;		
 * 		while (tmp1--)
 * 		    *lrbptr++ = *dp++;	
 * 	}
 * 	return(lrbptr);
 */
9:	beq	a2,zero,2f
	nop
1:	lb	t0,0(a0)
	addiu	a2,a2,-1
	sb	t0,0(a1)
	addiu	a0,a0,1
	addiu	a1,a1,1
	bne	a2,zero,1b
	nop

2:	move	v0,a1
	j	ra
	nop

	.set	reorder
	END(ln_cpyout4x4)

/*
 * ln_clean_dache4x4(addr, len)
 *	a0 = addr
 *	a1 = len
 */
LEAF(ln_clean_dcache4x4)
	.set noreorder
	nop
	mfc0	t3,C0_SR		# save sr
	nop

	li	v0,SR_ISC		# disable interrupts, isolate caches
	mtc0	v0,C0_SR

	nop
	nop
	nop
	nop				# cache must be isolated by now

1:	sb	zero,0(a0)
	sb	zero,4(a0)
	sb	zero,8(a0)
	sb	zero,12(a0)
	addiu	a1,a1,-1
	bne	a1,zero,1b
	addiu	a0,a0,32		# skip 4 word hole

	nop				# insure isolated stores out of pipe
	nop
	nop
	mtc0	t3,C0_SR		# un-isolate, enable interrupts
	nop				# insure cache unisolated
	nop				# insure cache unisolated
	nop				# insure cache unisolated
	j	ra
	nop
	.set	reorder
	END(ln_clean_dcache)

LEAF(ln_cpyin4x4s)
	.set 	noreorder

/*
 * caddr_t
 * ln_cpyin4x4s(lrbptr,dp,len)
 * caddr_t lrbptr;				# a0 = from
 * caddr_t dp;					# a1 = to
 * int len;					# a2 = len
 * {
 * 	register tmp1;
 * 	int end4word;
 */

/*
 * 	if ((u_long) lrbptr & 0xf) {
 * 
 * 		end4word = (0x10 - ((u_long)lrbptr & 0xf));
 * 		tmp1 = ((len > end4word) ? end4word : len);
 * 
 * 		len -= tmp1;
 * 		while (tmp1--)
 * 		    *dp++ = *lrbptr++;
 * 
 *              if (((u_long)lrbptr & 0xf) == 0)
 *                  lrbptr += 0x10;
 * 	}
 */
 	andi	t6,a0,0xf	# is lrbptr on 4 word boundary

 	beq	t6,zero,5f	# if yes, do 4 word copy
 	nop

 	li	t7,16		# determine number of bytes
 	subu	t7,t7,t6	# to get to 4 word boundary
 	slt	t5,t7,a2	# if number is less than len
 	beq	t5,zero,2f	# only copy len
 	move	t8,a2
 	move	t8,t7

2:	subu	a2,a2,t8	# decrement total len

 	beq	t8,zero,4f	
	nop
3: 	lb	t9,0(a0)	# copy 1 byte at a time
	addiu	t8,t8,-1	# until tmp1(t8) is zero
 	sb	t9,0(a1)	
 	addiu	a0,a0,1
 	addiu	a1,a1,1
 	bne	t8,zero,3b
	nop

4:	andi	t3,a0,0xf	# if lrbptr on 4 word boundary
	bne	t3,zero,6f	# skip 4 word hole
	sra	t8,a2,4
	addiu	a0,a0,16

/*
 * 
 * 	tmp1 = (len >> 4);
 * 
 * 	len -= (tmp1 << 4);
 */
5:	sra	t8,a2,4		# t8 = len >> 4
6:	sll	t0,t8,4		
	subu	a2,a2,t0

/*
 *	NOTE: if dp is not aligned to a word boundary, then
 *	      swl and swr is used to fill in the bytes.
 *
 * 	for (; tmp1; tmp1--) {
 * 
 *         	register int temp0, temp1, temp2, temp3;
 * 
 * 	        temp0 = ((u_int *)dp)[0];
 * 		temp1 = ((u_int *)dp)[1];
 * 		temp2 = ((u_int *)dp)[2];
 * 		temp3 = ((u_int *)dp)[3];
 * 		((u_int *)lrbptr)[0] = temp0;
 * 		((u_int *)lrbptr)[1] = temp1;
 * 		((u_int *)lrbptr)[2] = temp2;
 * 		((u_int *)lrbptr)[3] = temp3;
 * 
 * 		lrbptr += 32;
 * 		dp += 16;
 * 	}
 */
 	beq	t8,zero,9f	# tmp1 == 0, then get fragment
	andi	t6,a1,0x3	# is dp word aligned ?
	bne	t6,zero,1f	# no, do unaligned copy
	andi	t7,t8,0x3	# number of 4 word blocks to get 
	beq	t7,zero,8f	#     16 word aligned
	subu	t8,t8,t7	# decrement total count 
7:	lw	t0,0(a0)	# do 4 word block until we can
	lw	t1,4(a0)	#     do 16 words blocks
	lw	t2,8(a0)
	lw	t3,12(a0)
	sw	t0,0(a1)
	sw	t1,4(a1)
	sw	t2,8(a1)
	sw	t3,12(a1)
	addiu	t7,t7,-1	# decrement counter
	addiu	a1,a1,16	# increment dp by 16
	bne	t7,zero,7b
	addiu	a0,a0,32	# increment lrbptr by 32
	beq	t8,zero,9f
	nop
8:	lw	t0,0(a0)	# do copy 16 words at a time
	lw	t1,4(a0)
	lw	t2,8(a0)
	lw	t3,12(a0)
	lw	t4,32(a0)	# no hole to skip
	lw	t5,36(a0)
	lw	t6,40(a0)
	lw	t7,44(a0)
	sw	t0,0(a1)
	sw	t1,4(a1)
	sw	t2,8(a1)
	sw	t3,12(a1)
	sw	t4,16(a1)	# skip 4 word hole
	sw	t5,20(a1)
	sw	t6,24(a1)
	sw	t7,28(a1)
	lw	t0,64(a0)
	lw	t1,68(a0)
	lw	t2,72(a0)
	lw	t3,76(a0)
	lw	t4,96(a0)
	lw	t5,100(a0)
	lw	t6,104(a0)
	lw	t7,108(a0)
	sw	t0,32(a1)
	sw	t1,36(a1)
	sw	t2,40(a1)
	sw	t3,44(a1)
	sw	t4,48(a1)
	sw	t5,52(a1)
	sw	t6,56(a1)
	sw	t7,60(a1)
	addiu	t8,t8,-4	# decrement tmp1 by 4
	addiu	a1,a1,64	# add 64 to dp
	bne	t8,zero,8b	# repeat if tmp != 0
	addiu	a0,a0,128	# add 128 to lrbptr
	b	9f
	nop
1:	lw	t0,0(a0)
	lw	t1,4(a0)
	lw	t2,8(a0)
	lw	t3,12(a0)
	swr	t0,0(a1)
	swl	t0,3(a1)
	swr	t1,4(a1)
	swl	t1,7(a1)
	swr	t2,8(a1)
	swl	t2,11(a1)
	swr	t3,12(a1)
	swl	t3,15(a1)
	addiu	a1,a1,16
	addiu	t8,t8,-1
	bne	t8,zero,1b
	addiu	a0,a0,32

/*
 * 
 *
 * 	if (len) {
 * 	        tmp1 = len;		
 * 		while (tmp1--)
 * 		    *dp++ = *lrbptr++;	
 * 	}
 * 	return(lrbptr);
 */
9:	beq	a2,zero,2f
	nop
1:	lb	t0,0(a0)
	addiu	a2,a2,-1
	sb	t0,0(a1)
	addiu	a0,a0,1
	addiu	a1,a1,1
	bne	a2,zero,1b
	nop

2:	move	v0,a0
	j	ra
	nop

	.set	reorder
	END(ln_cpyin4x4s)

