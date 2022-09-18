/*	@(#)usercopy.s	4.1	(ULTRIX)	7/2/90	*/
/************************************************************************
 *									*
 *			Copyright (c) 1988 by				*
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
/* ------------------------------------------------------------------ */
/* | Copyright Unpublished, MIPS Computer Systems, Inc.  All Rights | */
/* | Reserved.  This software contains proprietary and confidential | */
/* | information of MIPS and its suppliers.  Use, disclosure or     | */
/* | reproduction is prohibited without the prior express written   | */
/* | consent of MIPS.                                               | */
/* ------------------------------------------------------------------ */

/*
 * Modification History:
 *
 * 13-Oct-89    gmm
 *	smp changes. Access nofault etc through cpudata
 *
 * 18-July-89	kong
 *	Rewrote the routine useracc.  It now lives in machdep.c.
 *	Refer to Modification History in machdep.c for details
 *	of the changes.
 *
 * 10-July-89	burns
 *	Made the follwing cache routines cpu cpecific since DS5800's have
 *	additional requirements: clean_icache, clean_dcache, page_iflush and
 *	page_dflush.
 *
 * 16-Jan-1989	Kong
 *	Renamed flush_cache to kn01flush_cache.  This routine
 *	will probably be moved to a file specific to kn01 (pmax)
 *	if the flush_cache routines can be tuned for performance.
 */

#include "../machine/param.h"
#include "../machine/cpu.h"
#include "../machine/asm.h"
#include "../machine/reg.h"
#include "../machine/regdef.h"
#include "../h/errno.h"
#include "assym.h"

/*
 * copypage(src_ppn, dst_ppn)
 *
 * Performance:
 *	Config	C/NC	Cycles/	Speed vs VAX
 *		Reads	4K Page	
 *	08V11	NC	13,568	 1.89X
 *		C	 6,272	 4.08X
 *	08M44	NC	 6,528	 3.92X
 *		C	 2,432	10.53X
 *			
 *	
 */
LEAF(copypage)
XLEAF(copyseg)
	sll	a1,PGSHIFT		# page number to phys addr
	or	a1,K0BASE
	addu	a3,a0,NBPG		# source endpoint
1:	lw	v0,0(a0)
	lw	v1,4(a0)
	lw	t0,8(a0)
	lw	t1,12(a0)
	sw	v0,0(a1)
	sw	v1,4(a1)
	sw	t0,8(a1)
	sw	t1,12(a1)
	addu	a0,32
	lw	v0,-16(a0)
	lw	v1,-12(a0)
	lw	t0,-8(a0)
	lw	t1,-4(a0)
	sw	v0,16(a1)
	sw	v1,20(a1)
	sw	t0,24(a1)
	sw	t1,28(a1)
	addu	a1,32			# BDSLOT: incr dst address
	bne	a0,a3,1b
#ifdef EXTRA_CACHETRICKS
	/*
	 * The dcachecnt for the source page must be handled by the
	 * caller, since it's too much of a pain to do the vtop and
	 * pte issues here.
	 */
	subu	a1,32			# back to copied page
	srl	a1,PGSHIFT
	lw	v0,dcachemask
	and	a1,v0			# figure appropriate cache alias
	sll	a1,1
	lhu	v0,dcachecnt(a1)
	addu	v0,1
	sh	v0,dcachecnt(a1)
#endif EXTRA_CACHETRICKS
	j	ra
	END(copypage)

/*
 * clearseg(dst_ppn)
 *
 *	Performance
 *	Config	Cycles/	Speed vs VAX
 *		4K Page	
 *	08V11	6,144	1.09X
 *	08M44	1,229	5.46X	(could be made faster by unroll to 64)
 *                              (done April '87 per djl)
 *	since writes only occur at best 1 per two cycles(m500) and unroll
 *	shouldn't help, in fact we probably don't want many instructions
 *	so that it is easy to get into icache-- so changing back to two
 *	sw's per loop (two cycles + two cycles for loop overhead) which
 *	will keep the write buffers busy and not stall the cpu.
 */
LEAF(clearseg)
	sll	a0,PGSHIFT
	addu	a0,K0BASE		# reference via k0seg
	addu	t0,a0,NBPG-8		# dst on last pass of loop
1:	sw	zero,0(a0)
	sw	zero,4(a0)
	.set	noreorder
	bne	a0,t0,1b
	addu	a0,8			# BDSLOT: inc dst, NOTE after test
	.set	reorder
#ifdef EXTRA_CACHETRICKS
	subu	a0,8			# back to copied page
	srl	a0,PGSHIFT
	lw	v0,dcachemask
	and	a0,v0			#  figure appropriate cache alias
	sll	a0,1
	lhu	v0,dcachecnt(a0)
	addu	v0,1
	sh	v0,dcachecnt(a0)
#endif EXTRA_CACHETRICKS
	j	ra
	END(clearseg)

#ifdef USE_IDLE
/*
 * clearseg1(dst_ppn, index)
 * do a clear of one 128 byte chunk.  called from idle.
 */
LEAF(clearseg1)
	sll	a0,PGSHIFT
	addu	a0,K0BASE		# reference via k0seg
	mul	a1,a1,128		# 128 * index = offset into page
	addu	a0,a0,a1		# start at page + offset
	addu	t0,a0,120		# dst is start + (128 - 8)
1:	sw	zero,0(a0)
	sw	zero,4(a0)
	.set	noreorder
	bne	a0,t0,1b
	addu	a0,8			# BDSLOT: inc dst, NOTE after test
	.set	reorder
#ifdef EXTRA_CACHETRICKS
	subu	a0,8			# sub 8 assures correct page
	srl	a0,PGSHIFT
	lw	v0,dcachemask
	and	a0,v0			#  figure appropriate cache alias
	sll	a0,1
	lhu	v0,dcachecnt(a0)
	addu	v0,1
	sh	v0,dcachecnt(a0)
#endif EXTRA_CACHETRICKS
	j	ra
	END(clearseg1)
#endif USE_IDLE

/*
 * copyin(user_src, kernel_dst, bcount)


/*
 * copyin(user_src, kernel_dst, bcount)
 */
COPYIOFRM=	(4*4)+4			# 4 arg saves plus ra
NESTED(copyin, COPYIOFRM, zero)
	subu	sp,COPYIOFRM
	sw	ra,COPYIOFRM-4(sp)
	bltz	a0,cerror
#ifdef ASSERTIONS
	lw	v0,u+PCB_CPUPTR
	lw	v0,CPU_NOFAULT(v0)
	beq	v0,zero,8f
	PANIC("recursive nofault")
8:
#endif ASSERTIONS
	.set	noreorder
	lw	ra,u+PCB_CPUPTR
	li	v0,NF_COPYIO		# LDSLOT
	sw	v0,CPU_NOFAULT(ra)
	jal	bcopy
	nop
	lw	ra,u+PCB_CPUPTR
	nop
	sw	zero,CPU_NOFAULT(ra)
	.set	reorder
	move	v0,zero
	lw	ra,COPYIOFRM-4(sp)
	addu	sp,COPYIOFRM
	j	ra
	END(copyin)

/*
 * copyout(kernel_src, user_dst, bcount)
 */
NESTED(copyout, COPYIOFRM, zero)
	subu	sp,COPYIOFRM
	sw	ra,COPYIOFRM-4(sp)
	bltz	a1,cerror
#ifdef ASSERTIONS
	lw	v0,u+PCB_CPUPTR
	lw	v0,CPU_NOFAULT(v0)
	beq	v0,zero,8f
	PANIC("recursive nofault")
8:
#endif ASSERTIONS
	.set	noreorder
	lw	ra,u+PCB_CPUPTR
	li	v0,NF_COPYIO		# LDSLOT
	sw	v0,CPU_NOFAULT(ra)
	jal	bcopy
	nop
	lw	ra,u+PCB_CPUPTR
	nop
	sw	zero,CPU_NOFAULT(ra)
	.set	reorder
	move	v0,zero
	lw	ra,COPYIOFRM-4(sp)
	addu	sp,COPYIOFRM
	j	ra
	END(copyout)

NESTED(cerror, COPYIOFRM, zero)
	li	v0,EFAULT
	lw	ra,COPYIOFRM-4(sp)
	addu	sp,COPYIOFRM
	j	ra
	END(cerror)

/*
 * bcopy(src, dst, bcount)
 *
 * NOTE: the optimal copy here is somewhat different than for the user-level
 * equivalents (bcopy in 4.2, memcpy in V), because:
 * 1) it frequently acts on uncached data, especially since copying from
 * (uncached) disk buffers into user pgms is high runner.
 * This means one must be careful with lwl/lwr/lb - don't expect cache help.
 * 2) the distribution of usage is very different: there are a large number
 * of bcopies for small, aligned structures (like for ioctl, for example),
 * a reasonable number of randomly-sized copies for user I/O, and many
 * bcopies of large (page-size) blocks for stdio; the latter must be
 * well-tuned, hence the use of 32-byte loops.
 * 3) this is much more frequently-used code inside the kernel than outside
 *
 * Overall copy-loop speeds, by amount of loop-unrolling: assumptions:
 * a) low icache miss rate (this code gets used a bunch)
 * b) large transfers, especially, will be word-alignable.
 * c) Copying speeds (steady state, 0% I-cache-miss, 100% D-cache Miss):
 * d) 100% D-Cache Miss (but cacheable, so that lwl/lwr/lb work well)
 *	Config	Bytes/	Cycles/	Speed (VAX/780 = 1)
 *		Loop	Word
 *	08V11	1	35	0.71X	(8MHz, BUS, 1-Deep WB, 1-way ILV)
 *		4	15	1.67X
 *		8/16	13.5	1.85X
 *		32/up	13.25	1.89X
 *	08MM44	1	26	0.96X	(8MHz, MEM, 4-Deep WB, 4-way ILV)
 *		4	9	2.78X
 *		8	7.5	3.33X
 *		16	6.75	3.70X
 *		32	6.375	3.92X	(diminishing returns thereafter)
 *
 * MINCOPY is minimum number of byte that its worthwhile to try and
 * align copy into word transactions.  Calculations below are for 8 bytes:
 * Estimating MINCOPY (C = Cacheable, NC = Noncacheable):
 * Assumes 100% D-cache miss on first reference, then 0% (100%) for C (NC):
 * (Warning: these are gross numbers, and the code has changed slightly):
 *	Case		08V11			08M44
 *	MINCOPY		C	NC		C	NC
 *	9 (1 byte loop)	75	133		57	93
 *	8 (complex logic)
 *	Aligned		51	51		40	40
 *	Alignable,
 *	worst (1+4+3)	69	96		53	80
 *	Unalignable	66	93		60	72
 * MINCOPY should be lower for lower cache miss rates, lower cache miss
 * penalties, better alignment properties, or if src and dst alias in
 * cache. For this particular case, it seems very important to minimize the
 * number of lb/sb pairs: a) frequent non-cacheable references are used,
 * b) when i-cache miss rate approaches zero, even the 4-deep WB can't
 * put successive sb's together in any useful way, so few references are saved.
 * To summarize, even as low as 8 bytes, avoiding the single-byte loop seems
 * worthwhile; some assumptions are probably optimistic, so there is not quite
 * as much disadvantage.  However, the optimal number is almost certainly in
 * the range 7-12.
 *
 *	a0	src addr
 *	a1	dst addr
 *	a2	length remaining
 */
#define	MINCOPY	8

LEAF(bcopy)
#ifdef ASSERTIONS
	bgeu	a0,a1,1f		# src >= dst, no overlap error
	addu	v0,a0,a2		# src endpoint + 1
	bgeu	a1,v0,1f		# dst >= src endpoint+1, no overlap err
	PANIC("bcopy overlap")
1:
#endif ASSERTIONS
	xor	v0,a0,a1		# bash src & dst for align chk; BDSLOT
	blt	a2,MINCOPY,bytecopy	# too short, just byte copy
	and	v0,NBPW-1		# low-order bits for align chk
	subu	v1,zero,a0		# -src; BDSLOT
	bne	v0,zero,unaligncopy	# src and dst not alignable
/*
 * src and dst can be simultaneously word aligned
 */
	and	v1,NBPW-1		# number of bytes til aligned
	subu	a2,v1			# bcount -= alignment
	beq	v1,zero,blkcopy		# already aligned
#ifdef MIPSEB
	lwl	v0,0(a0)		# copy unaligned portion
	swl	v0,0(a1)
#endif
#ifdef MIPSEL
	lwr	v0,0(a0)
	swr	v0,0(a1)
#endif
	addu	a0,v1			# src += alignment
	addu	a1,v1			# dst += alignment

/*
 * 32 byte block, aligned copy loop (for big reads/writes)
 */
blkcopy:
	and	a3,a2,~31		# total space in 32 byte chunks
	subu	a2,a3			# count after by-32 byte loop done
	beq	a3,zero,wordcopy	# less than 32 bytes to copy
	addu	a3,a0			# source endpoint
1:	lw	v0,0(a0)
	lw	v1,4(a0)
	lw	t0,8(a0)
	lw	t1,12(a0)
	sw	v0,0(a1)
	sw	v1,4(a1)
	sw	t0,8(a1)
	sw	t1,12(a1)
	addu	a0,32			# src+= 32; here to ease loop end
	lw	v0,-16(a0)
	lw	v1,-12(a0)
	lw	t0,-8(a0)
	lw	t1,-4(a0)
	sw	v0,16(a1)
	sw	v1,20(a1)
	sw	t0,24(a1)
	sw	t1,28(a1)
	addu	a1,32			# dst+= 32; fills BD slot
	bne	a0,a3,1b

/*
 * word copy loop
 */
wordcopy:
	and	a3,a2,~(NBPW-1)		# word chunks
	subu	a2,a3			# count after by word loop
	beq	a3,zero,bytecopy	# less than a word to copy
	addu	a3,a0			# source endpoint
1:	lw	v0,0(a0)
	addu	a0,NBPW
	sw	v0,0(a1)
	addu	a1,NBPW			# dst += 4; BD slot
	bne	a0,a3,1b
	b	bytecopy

/*
 * deal with simultaneously unalignable copy by aligning dst
 */
unaligncopy:
	subu	a3,zero,a1		# calc byte cnt to get dst aligned
	and	a3,NBPW-1		# alignment = 0..3
	subu	a2,a3			# bcount -= alignment
	beq	a3,zero,partaligncopy	# already aligned
#ifdef MIPSEB
	lwl	v0,0(a0)		# get whole word
	lwr	v0,3(a0)		# for sure
	swl	v0,0(a1)		# store left piece (1-3 bytes)
#endif
#ifdef MIPSEL
	lwr	v0,0(a0)		# get whole word
	lwl	v0,3(a0)		# for sure
	swr	v0,0(a1)		# store right piece (1-3 bytes)
#endif
	addu	a0,a3			# src += alignment (will fill LD slot)
	addu	a1,a3			# dst += alignment

/*
 * src unaligned, dst aligned loop
 * NOTE: if MINCOPY >= 7, will always do 1 loop iteration or more
 * if we get here at all
 */
partaligncopy:
	and	a3,a2,~(NBPW-1)		# space in word chunks
	subu	a2,a3			# count after by word loop
#if MINCOPY < 7
	beq	a3,zero,bytecopy	# less than a word to copy
#endif
	addu	a3,a0			# source endpoint
1:
#ifdef MIPSEB
	lwl	v0,0(a0)
	lwr	v0,3(a0)
#endif
#ifdef MIPSEL
	lwr	v0,0(a0)
	lwl	v0,3(a0)
#endif
	addu	a0,NBPW
	sw	v0,0(a1)
	addu	a1,NBPW
	bne	a0,a3,1b


/*
 * brute force byte copy loop, for bcount < MINCOPY + tail of unaligned dst
 * note that lwl, lwr, swr CANNOT be used for tail, since the lwr might
 * cross page boundary and give spurious address exception
 */
bytecopy:
	addu	a3,a2,a0		# source endpoint; BDSLOT
	ble	a2,zero,copydone	# nothing left to copy, or bad length
1:	lb	v0,0(a0)
	addu	a0,1
	sb	v0,0(a1)
	addu	a1,1			# BDSLOT: incr dst address
	bne	a0,a3,1b
copydone:
	j	ra
	END(bcopy)

/*
 * bzero(dst, bcount)
 * Zero block of memory
 *
 * Calculating MINZERO, assuming 50% cache-miss on non-loop code:
 * Overhead =~ 18 instructions => 63 (81) cycles
 * Byte zero =~ 16 (24) cycles/word for 08M44 (08V11)
 * Word zero =~ 3 (6) cycles/word for 08M44 (08V11)
 * If I-cache-miss nears 0, MINZERO ==> 4 bytes; otherwise, times are:
 * breakeven (MEM) = 63 / (16 - 3) =~ 5 words
 * breakeven (BUS) = 81 / (24 - 6)  =~ 4.5 words
 * Since the overhead is pessimistic (worst-case alignment), and many calls
 * will be for well-aligned data, and since Word-zeroing at least leaves
 * the zero in the cache, we shade these values (18-20) down to 12
 */
#define	MINZERO	12
LEAF(bzero)
XLEAF(blkclr)
	subu	v1,zero,a0		# number of bytes til aligned
	blt	a1,MINZERO,bytezero
	and	v1,NBPW-1
	subu	a1,v1
	beq	v1,zero,blkzero		# already aligned
#ifdef MIPSEB
	swl	zero,0(a0)
#endif
#ifdef	MIPSEL
	swr	zero,0(a0)
#endif
	addu	a0,v1

/*
 * zero 32 byte, aligned block
 */
blkzero:
	and	a3,a1,~31		# 32 byte chunks
	subu	a1,a3
	beq	a3,zero,wordzero
	addu	a3,a0			# dst endpoint
1:	sw	zero,0(a0)
	sw	zero,4(a0)
	sw	zero,8(a0)
	sw	zero,12(a0)
	addu	a0,32
	sw	zero,-16(a0)
	sw	zero,-12(a0)
	sw	zero,-8(a0)
	sw	zero,-4(a0)
	bne	a0,a3,1b

wordzero:
	and	a3,a1,~(NBPW-1)		# word chunks
	subu	a1,a3
	beq	a3,zero,bytezero
	addu	a3,a0			# dst endpoint
1:	addu	a0,NBPW
	sw	zero,-NBPW(a0)
	bne	a0,a3,1b

bytezero:
	ble	a1,zero,zerodone
	addu	a1,a0			# dst endpoint
1:	addu	a0,1
	sb	zero,-1(a0)
	bne	a0,a1,1b
zerodone:
	j	ra
	END(bzero)

/*
 * bcmp(src, dst, bcount)
 *
 * MINCMP is minimum number of byte that its worthwhile to try and
 * align cmp into word transactions
 *
 * Calculating MINCMP
 * Overhead =~ 15 instructions => 90 cycles
 * Byte cmp =~ 38 cycles/word
 * Word cmp =~ 17 cycles/word
 * Breakeven =~ 16 bytes
 */
#define	MINCMP	16

LEAF(bcmp)
	xor	v0,a0,a1
	blt	a2,MINCMP,bytecmp	# too short, just byte cmp
	and	v0,NBPW-1
	subu	t8,zero,a0		# number of bytes til aligned
	bne	v0,zero,unalgncmp	# src and dst not alignable
/*
 * src and dst can be simultaneously word aligned
 */
	and	t8,NBPW-1
	subu	a2,t8
	beq	t8,zero,wordcmp		# already aligned
 	move	a1,a0			# The FIX
#ifdef MIPSEB
	lwl	v0,0(a0)		# cmp unaligned portion
	lwl	v1,0(a1)
#endif
#ifdef MIPSEL
	lwr	v0,0(a0)
	lwr	v1,0(a1)
#endif
	addu	a0,t8
	addu	a1,t8
	bne	v0,v1,cmpne

/*
 * word cmp loop
 */
wordcmp:
	and	a3,a2,~(NBPW-1)
	subu	a2,a3
	beq	a3,zero,bytecmp
	addu	a3,a0				# src1 endpoint
1:	lw	v0,0(a0)
	lw	v1,0(a1)
	addu	a0,NBPW				# 1st BDSLOT
	addu	a1,NBPW				# 2nd BDSLOT (asm doesn't move)
	bne	v0,v1,cmpne
	bne	a0,a3,1b			# at least one more word
	b	bytecmp

/*
 * deal with simultaneously unalignable cmp by aligning one src
 */
unalgncmp:
	subu	a3,zero,a1		# calc byte cnt to get src2 aligned
	and	a3,NBPW-1
	subu	a2,a3
	beq	a3,zero,partaligncmp	# already aligned
	addu	a3,a0			# src1 endpoint
1:	lbu	v0,0(a0)
	lbu	v1,0(a1)
	addu	a0,1
	addu	a1,1
	bne	v0,v1,cmpne
	bne	a0,a3,1b

/*
 * src unaligned, dst aligned loop
 */
partaligncmp:
	and	a3,a2,~(NBPW-1)
	subu	a2,a3
	beq	a3,zero,bytecmp
	addu	a3,a0
1:
#ifdef MIPSEB
	lwl	v0,0(a0)
	lwr	v0,3(a0)
#endif
#ifdef MIPSEL
	lwr	v0,0(a0)
	lwl	v0,3(a0)
#endif
	lw	v1,0(a1)
	addu	a0,NBPW
	addu	a1,NBPW
	bne	v0,v1,cmpne
	bne	a0,a3,1b

/*
 * brute force byte cmp loop
 */
bytecmp:
	addu	a3,a2,a0			# src1 endpoint; BDSLOT
	ble	a2,zero,cmpdone
1:	lbu	v0,0(a0)
	lbu	v1,0(a1)
	addu	a0,1
	addu	a1,1
	bne	v0,v1,cmpne
	bne	a0,a3,1b
cmpdone:
	move	v0,zero	
	j	ra

cmpne:
	li	v0,1
	j	ra
	END(bcmp)

/*
 * struct u_prof offsets
 */
#define	PR_BASE		0
#define	PR_SIZE		4
#define	PR_OFF		8
#define	PR_SCALE	12

/*
 * addupc(pc, &u.u_prof, ticks)
 */
LEAF(addupc)
	lw	v1,PR_OFF(a1)		# base of profile region
	subu	a0,v1			# corrected pc
	bltz	a0,1f			# below of profile region
	lw	v0,PR_SCALE(a1)		# fixed point scale factor
	multu	v0,a0
	mflo	v0			# shift 64 bit result right 16
	srl	v0,16
	mfhi	v1
	sll	v1,16
	or	v0,v1
	addu	v0,1			# round-up to even
	and	v0,~1
	lw	v1,PR_SIZE(a1)
	bgeu	v0,v1,1f		# above profile region
	lw	v1,PR_BASE(a1)		# base of profile buckets
	addu	v0,v1
	bltz	v0,adderr		# outside kuseg

#ifdef ASSERTIONS
	lw	v1,u+PCB_CPUPTR
	lw	v1,CPU_NOFAULT(v1)
	beq	v1,zero,8f
	PANIC("recursive nofault")
8:
#endif ASSERTIONS
	.set	noreorder
	lw	a3,u+PCB_CPUPTR
	li	v1,NF_ADDUPC		# LDSLOT
	sw	v1,CPU_NOFAULT(a3)
	lh	v1,0(v0)		# add ticks to bucket
	nop				# ??? Not in mad version ???
	addu	v1,a2
	sh	v1,0(v0)
	sw	zero,CPU_NOFAULT(a3)
	.set	reorder

1:	j	ra
	END(addupc)

LEAF(adderr)
	sw	zero,PR_SCALE(a1)
	j	ra
	END(adderr)

LEAF(fubyte)
XLEAF(fuibyte)
#ifdef ASSERTIONS
	lw	v0,u+PCB_CPUPTR
	lw	v0,CPU_NOFAULT(v0)
	beq	v0,zero,8f
	PANIC("recursive nofault")
8:
#endif ASSERTIONS
	.set	noreorder
	bltz	a0,uerror
	li	v0,NF_FSUMEM		# BDSLOT
	lw	v1,u+PCB_CPUPTR
	nop
	sw	v0,CPU_NOFAULT(v1)
	lbu	v0,0(a0)
	sw	zero,CPU_NOFAULT(v1)	# LDSLOT
	.set	reorder
	j	ra
	END(fubyte)

LEAF(subyte)
XLEAF(suibyte)
#ifdef ASSERTIONS
	lw	v0,u+PCB_CPUPTR
	lw	v0,CPU_NOFAULT(v0)
	beq	v0,zero,8f
	PANIC("recursive nofault")
8:
#endif ASSERTIONS
	.set	noreorder
	bltz	a0,uerror
	li	v0,NF_FSUMEM		# BDSLOT
	lw	v1,u+PCB_CPUPTR
	nop
	sw	v0,CPU_NOFAULT(v1)
	sb	a1,0(a0)
	sw	zero,CPU_NOFAULT(v1)
	.set	reorder
	j	ra
	END(subyte)

LEAF(fuword)
XLEAF(fuiword)
#ifdef ASSERTIONS
	lw	v0,u+PCB_CPUPTR
	lw	v0,CPU_NOFAULT(v0)
	beq	v0,zero,8f
	PANIC("recursive nofault")
8:
#endif ASSERTIONS
	.set	noreorder
	bltz	a0,uerror
	li	v0,NF_FSUMEM		# BDSLOT
	lw	v1,u+PCB_CPUPTR
	nop
	sw	v0,CPU_NOFAULT(v1)
	lw	v0,0(a0)
	sw	zero,CPU_NOFAULT(v1)	# LDSLOT
	.set	reorder
	j	ra
	END(fuword)
	

LEAF(suword)
XLEAF(suiword)
#ifdef ASSERTIONS
	lw	v0,u+PCB_CPUPTR
	lw	v0,CPU_NOFAULT(v0)
	beq	v0,zero,8f
	PANIC("recursive nofault")
8:
#endif ASSERTIONS
	.set	noreorder
	bltz	a0,uerror
	li	v0,NF_FSUMEM		# BDSLOT
	lw	v1,u+PCB_CPUPTR
	nop
	sw	v0,CPU_NOFAULT(v1)
	sw	a1,0(a0)
	sw	zero,CPU_NOFAULT(v1)
	.set	reorder
	j	ra
	END(suword)

LEAF(uerror)
	li	v0,-1			# error return
	j	ra
	END(uerror)

#ifdef notdef
/* useracc now in machdep.c */
/*
 * useracc(addr, bcnt, rw)
 * verify user access to virtual addresses addr .. addr+bcnt-1
 * if rw is 0, write access is verified, if rw is 1, read access
 */
LEAF(useracc)
	beq	a1,zero,3f		# nothing to check
#ifdef ASSERTIONS
	lw	v0,u+PCB_CPUPTR
	lw	v0,CPU_NOFAULT(v0)
	beq	v0,zero,8f
	PANIC("recursive nofault")
8:
#endif ASSERTIONS

	.set	noreorder
	bltz	a0,uaerror
	li	v0,NF_USERACC
	lw	v1,u+PCB_CPUPTR
	nop
	sw	v0,CPU_NOFAULT(v1)
	.set	reorder

	addu	t0,a0,a1		# end + 1
	and	a0,~PGOFSET		# back-up to start of page
	bne	a2,zero,2f		# verify read access
	/*
	 * verify write access
	 */
1:	lw	v0,0(a0)
	addu	a0,NBPG
	sw	v0,-NBPG(a0)
	bltu	a0,t0,1b

	.set	noreorder
	sw	zero,CPU_NOFAULT(v1)
	j	ra
	li	v0,1			# BDSLOT
	.set	reorder

	/*
	 * verify read access
	 */
2:	lw	v0,0(a0)
	addu	a0,NBPG
	bltu	a0,t0,2b

	.set	noreorder
3:	sw	zero,CPU_NOFAULT(v1)
	j	ra
	li	v0,1			# BDSLOT
	.set	reorder
	END(useracc)
#endif notdef

LEAF(uaerror)
	move	v0,zero
	j	ra
	END(uaerror)

/*
 * strlen - return bytes in null terminated string
 */
LEAF(strlen)
	move	v0,a0		# save beginning pointer
1:	lb	v1,0(a0)	# look at byte
	addu	a0,1		# advance current pointer
	bne	v1,zero,1b	# check for null byte
	subu	v0,a0,v0	# byte count including null byte
	subu	v0,1		# exclude null byte
	j	ra
	END(strlen)

/*
 * kn01_clean_icache(addr, len)
 * flush i cache for range of addr to addr+len-1
 * MUST NOT DESTROY a0 AND a1, SEE clean_cache ABOVE
 */
LEAF(kn01_clean_icache)
	lw	t1,icache_size
	.set noreorder
	nop
	mfc0	t3,C0_SR		# save sr
	nop
	mtc0	zero,C0_SR		# interrupts off
	nop
	.set reorder
	
	.set	noreorder
	la	v0,1f
	or	v0,K1BASE
	j	v0			# run uncached
	nop

1:	li	v0,SR_ISC|SR_SWC	# disable intr, isolate and swap
	mtc0	v0,C0_SR
	bltu	t1,a1,1f		# cache is smaller than region
	nop
	move	t1,a1
1:	addu	t1,a0			# ending address + 1
	move	t0,a0
	la	v0,1f			# run cached
	j	v0
	nop
	.set	reorder

1:	sb	zero,0(t0)
	sb	zero,4(t0)
	sb	zero,8(t0)
	sb	zero,12(t0)
	sb	zero,16(t0)
	sb	zero,20(t0)
	sb	zero,24(t0)
	addu	t0,32
	sb	zero,-4(t0)
	bltu	t0,t1,1b

	.set	noreorder
	la	v0,1f
	or	v0,K1BASE
	j	v0			# run uncached
	nop

1:	nop				# insure isolated stores out of pipe
	mtc0	zero,C0_SR		# unisolate, unswap
	nop				# keep pipeline clean
	nop				# keep pipeline clean
	nop				# keep pipeline clean
	mtc0	t3,C0_SR		# enable interrupts
	j	ra			# return and run cached
	nop
	.set	reorder
	END(kn01_clean_icache)

LEAF(kn01_clean_dcache)
	lw	t2,dcache_size
	.set noreorder
	nop
	mfc0	t3,C0_SR		# save sr
	nop
	.set reorder

	.set	noreorder
	li	v0,SR_ISC		# disable interrupts, isolate caches
	mtc0	v0,C0_SR
	bltu	t2,a1,1f		# cache is smaller than region
	nop
	move	t2,a1
1:	addu	t2,a0			# ending address + 1
	move	t0,a0
	nop
	nop				# cache must be isolated by now
	.set	reorder

1:	sb	zero,0(t0)
	sb	zero,4(t0)
	sb	zero,8(t0)
	sb	zero,12(t0)
	sb	zero,16(t0)
	sb	zero,20(t0)
	sb	zero,24(t0)
	addu	t0,32
	sb	zero,-4(t0)
	bltu	t0,t2,1b

	.set	noreorder
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
	END(kn01_clean_dcache)

/*
 * kn01flush_cache()
 * flush entire cache
 */
LEAF(kn01flush_cache)
	lw	t1,icache_size		# must load before isolating
	lw	t2,dcache_size		# must load before isolating
	.set noreorder
	nop
	mfc0	t3,C0_SR		# save SR
	nop
	mtc0	zero,C0_SR		# interrupts off
	nop
	.set reorder

	.set	noreorder
	la	v0,1f
	or	v0,K1BASE
	j	v0			# run uncached
	nop

	/*
	 * flush text cache
	 */
1:	li	v0,SR_ISC|SR_SWC	# disable intr, isolate and swap
	mtc0	v0,C0_SR
	li	t0,K1BASE
	subu	t0,t1
	li	t1,K1BASE
	la	v0,1f			# run cached
	j	v0
	nop
	.set	reorder

1:	sb	zero,0(t0)
	sb	zero,4(t0)
	sb	zero,8(t0)
	sb	zero,12(t0)
	sb	zero,16(t0)
	sb	zero,20(t0)
	sb	zero,24(t0)
	addu	t0,32
	sb	zero,-4(t0)
	bne	t0,t1,1b

	.set	noreorder
	la	v0,1f
	or	v0,K1BASE
	j	v0			# run uncached
	nop

	/*
	 * flush data cache
	 */
1:	li	v0,SR_ISC		# isolate and swap back caches
	mtc0	v0,C0_SR
	li	t0,K1BASE
	subu	t0,t2
	la	v0,1f
	j	v0			# back to cached mode
	nop
	.set	reorder

1:	sb	zero,0(t0)
	sb	zero,4(t0)
	sb	zero,8(t0)
	sb	zero,12(t0)
	sb	zero,16(t0)
	sb	zero,20(t0)
	sb	zero,24(t0)
	addu	t0,32
	sb	zero,-4(t0)
	bne	t0,t1,1b

	.set	noreorder
	nop				# insure isolated stores out of pipe
	nop
	nop
	mtc0	t3,C0_SR		# un-isolate, enable interrupts
	nop				# insure cache unisolate
	nop
	nop
	nop
	.set	reorder
#ifdef CACHETRICKS
	lw	v0,icachemask		# index of last entry in icachecnt
	li	v1,0
	sll	v0,1			# offset to last entry
1:	lhu	t0,icachecnt(v1)
	addu	t0,1
	sh	t0,icachecnt(v1)
	addu	v1,2
	ble	v1,v0,1b		# more cachecnt's to bump

	lw	v0,dcachemask		# index of last entry in dcachecnt
	li	v1,0
	sll	v0,1			# offset to last entry
1:	lhu	t0,dcachecnt(v1)
	addu	t0,1
	sh	t0,dcachecnt(v1)
	addu	v1,2
	ble	v1,v0,1b		# more cachecnt's to bump
#endif CACHETRICKS

	j	ra
	END(kn01flush_cache)

/*
 * kn01_page_iflush(addr)
 * flush one page of i cache, addr is assumed to be in K0SEG
 */
LEAF(kn01_page_iflush)
	lw	t1,icache_size
	.set noreorder
	nop
	mfc0	t3,C0_SR		# save sr
	nop
	mtc0	zero,C0_SR		# interrupts off
	nop
	.set reorder
	.set	noreorder
	la	v0,1f
	or	v0,K1BASE
	j	v0			# run uncached
	nop				# BDSLOT

	/*
	 * flush text cache
	 */
1:	li	v0,SR_ISC|SR_SWC	# disable intr, isolate and swap
	mtc0	v0,C0_SR
	bltu	t1,NBPG,1f		# cache is smaller than region
	nop				# BDSLOT
	li	t1,NBPG
1:	addu	t1,a0			# ending address + 1
	move	t0,a0
	la	v0,1f			# run cached
	j	v0
	nop				# cache must be isolated by now
	.set	reorder

1:	sb	zero,0(t0)
	sb	zero,4(t0)
	sb	zero,8(t0)
	sb	zero,12(t0)
	sb	zero,16(t0)
	sb	zero,20(t0)
	sb	zero,24(t0)
	addu	t0,32
	sb	zero,-4(t0)
	bltu	t0,t1,1b

	.set	noreorder
	la	v0,1f
	or	v0,K1BASE
	j	v0			# run uncached
	nop

1:	nop				# insure isolated stores out of pipe
	mtc0	zero,C0_SR		# unisolate, unswap
	nop
	nop
	nop
	nop
	mtc0	t3,C0_SR		# enable interrupts
	la	v0,1f			# run cached
	j	v0
	nop

	.set	reorder
1:
#ifdef CACHETRICKS
	lw	v0,icachemask
	srl	t1,a0,PGSHIFT
	and	t1,v0
	sll	t1,1			# cachecnt index
	lhu	t0,icachecnt(t1)
	addu	t0,1
	sh	t0,icachecnt(t1)
#endif CACHETRICKS
	j	ra
	END(kn01_page_iflush)

/*
 * kn01_page_dflush(addr)
 * flush one page of i cache, addr is assumed to be in K0SEG
 */
LEAF(kn01_page_dflush)
	lw	t2,dcache_size
	.set noreorder
	nop
	mfc0	t3,C0_SR		# save sr
	nop
	.set reorder

	.set	noreorder
	li	v0,SR_ISC		# interrupts off, isolate caches
	mtc0	v0,C0_SR
	bltu	t2,NBPG,1f		# cache is smaller than region
	nop
	li	t2,NBPG
1:	addu	t2,a0			# ending address + 1
	move	t0,a0			# cache must be isolated by now
	.set	reorder

1:	sb	zero,0(t0)
	sb	zero,4(t0)
	sb	zero,8(t0)
	sb	zero,12(t0)
	sb	zero,16(t0)
	sb	zero,20(t0)
	sb	zero,24(t0)
	addu	t0,32
	sb	zero,-4(t0)
	bltu	t0,t2,1b

	.set	noreorder
	nop				# insure isolated stores out of pipe
	nop
	nop
	mtc0	t3,C0_SR		# un-isolate, enable interrupts
	nop				# insure cache unisolated
	nop
	nop
	.set	reorder
#ifdef CACHETRICKS
	lw	v0,dcachemask
	srl	t1,a0,PGSHIFT
	and	t1,v0
	sll	t1,1			# cachecnt index
	lhu	t0,dcachecnt(t1)
	addu	t0,1
	sh	t0,dcachecnt(t1)
#endif CACHETRICKS
	j	ra
	END(kn01_page_dflush)

/*
 * Config_cache() -- determine sizes of i and d caches
 * Sizes stored in globals dcache_size and icache_size
 */
CONFIGFRM=	(4*4)+4+4		# 4 arg saves, ra, and a saved register
NESTED(config_cache, CONFIGFRM, zero)
	subu	sp,CONFIGFRM
	sw	ra,CONFIGFRM-4(sp)
	sw	s0,CONFIGFRM-8(sp)	# save s0 on stack
	.set noreorder
	nop
	mfc0	s0,C0_SR		# save SR
	nop
	mtc0	zero,C0_SR		# disable interrupts
	nop
	.set reorder
	.set	noreorder
	la	v0,1f
	or	v0,K1BASE
	j	v0			# run uncached
	nop

1:	jal	size_cache
	nop
	sw	v0,dcache_size
	nop				# make sure sw out of pipe
	nop
	nop
	nop
	li	v0,SR_SWC		# swap caches
	mtc0	v0,C0_SR
	nop				# insure caches stable
	nop
	nop
	nop
	jal	size_cache
	nop
	sw	v0,icache_size
	nop				# make sure sw out of pipe
	nop
	nop
	nop
	mtc0	zero,C0_SR		# swap back caches
	nop
	nop
	nop
	nop
	la	t0,1f
	j	t0			# back to cached mode
	nop

1:	mtc0	s0,C0_SR		# restore SR
	nop
	lw	s0,CONFIGFRM-8(sp)	# restore old s0
	lw	ra,CONFIGFRM-4(sp)
	addu	sp,CONFIGFRM
	j	ra
	nop
	.set	reorder
	END(config_cache)

/*
 * size_cache()
 * return size of current data cache
 */
LEAF(size_cache)
	.set	noreorder
	mfc0	t0,C0_SR		# save current sr
	nop				# LDSLOT
	or	v0,t0,SR_ISC		# isolate cache
	nop				# make sure no stores in pipe
	mtc0	v0,C0_SR
	nop				# make sure isolated
	nop
	nop
	/*
	 * Clear cache size boundries to known state.
	 */
	li	v0,MINCACHE
1:
	sw	zero,K0BASE(v0)
	sll	v0,1
	ble	v0,+MAXCACHE,1b
	nop				# BDSLOT

	li	v0,-1
	sw	v0,K0BASE(zero)		# store marker in cache
	li	v0,MINCACHE		# MIN cache size

2:	lw	v1,K0BASE(v0)		# Look for marker
	nop				# LDSLOT
	bne	v1,zero,3f		# found marker
	nop				# BDSLOT

	sll	v0,1			# cache size * 2
	ble	v0,+MAXCACHE,2b		# keep looking
	nop
	move	v0,zero			# must be no cache
	.set	reorder

	.set noreorder
	nop
3:	mtc0	t0,C0_SR		# restore sr
	nop
	.set reorder
	.set noreorder
	nop

	nop				# make sure unisolated
	nop
	nop
	nop
	nop
	.set reorder
	j	ra
	END(size_cache)

#ifdef MIPSEB
#    define LWS lwl
#    define LWB lwr
#    define SWS swl
#    define SWB swr
#else
#    define LWS lwr
#    define LWB lwl
#    define SWS swr
#    define SWB swl
#endif

/*
 * Copy a null terminated string from the user address space into
 * the kernel address space.
 *
 * copyinstr(user_src, kernel_dest, maxlength, &lencopied)
 *	returns:
 *		0		- success
 *		EFAULT		- user_src not accessable
 *		ENAMETOOLONG	- string exceeded maxlength
 */
LEAF(copyinstr)
#ifdef ASSERTIONS
	lw	v0,u+PCB_CPUPTR
	lw	v0,CPU_NOFAULT(v0)
	beq	v0,zero,8f
	PANIC("recursive nofault")
8:
#endif ASSERTIONS
	bgez	a0,_cpyiostr	# user_src must be in kuseg
	b	cstrerror
	END(copyinstr)

/*
 * Copy a null terminated string from the kernel address space into
 * user address space.
 *
 * copyoutstr(kernel_src, user_dest, maxlength, &lencopied)
 *	returns:
 *		0		- success
 *		EFAULT		- user_dest not accessable
 *		ENAMETOOLONG	- string exceeded maxlength
 */
LEAF(copyoutstr)
#ifdef ASSERTIONS
	lw	v0,u+PCB_CPUPTR
	lw	v0,CPU_NOFAULT(v0)
	beq	v0,zero,8f
	PANIC("recursive nofault")
8:
#endif ASSERTIONS
	bgez	a1,_cpyiostr	# user_dest must be in kuseg
	b	cstrerror
	END(copyoutstr)

/*
 * Copy a null terminated string from one point to another in 
 * kernel address space.
 *
 * copystr(src, dest, maxlength, &lencopied)
 *	returns:
 *		0		- success
 *		EFAULT		- address not accessable (bogus length)
 *		ENAMETOOLONG	- string exceeded maxlength
 */
LEAF(copystr)
	bgez	a2,_cpystr
	b	cstrerror

XLEAF(_cpyiostr)
	.set	noreorder
	li	v0,NF_COPYSTR
	lw	v1,u+PCB_CPUPTR
	nop
	sw	v0,CPU_NOFAULT(v1)	# prepare for the worst
	.set	reorder

XLEAF(_cpystr)
	/*
	 * start up first word
	 * adjust pointers so that a0 points to next word
	 * t7 = a1 adjusted by same amount minus one
	 * t0,t1,t2,t3 are filled with 4 consecutive bytes
	 * t4 is filled with the same 4 bytes in a single word
	 */
	.set noreorder
	ble	a2,4,$dumbcpy	# not enough for a word
	move	v0,a2		# BDSLOT save copy of maxlength
	lb	t0,0(a0)
	nop			# LDSLOT
	beq	t0,zero,$cpy1ch
	or	t5,a1,3		# LDSLOT get an early start
	lb	t1,1(a0)
	subu	t6,t5,a1	# LDSLOT number of char in 1st word of dst - 1
	beq	t1,zero,$cpy2ch
	addu	t7,a0,t6	# BDSLOT offset starting pt for source string
	lb	t2,2(a0)
	nop			# LDSLOT
	beq	t2,zero,$cpy3ch
	LWS	t4,0(a0)	# BDSLOT safe: always in same word as 0(a0)
	lb	t3,3(a0)	# LDSLOT
	LWB	t4,3(a0)	# LDSLOT fill out word
	beq	t3,zero,$cpy4ch	# LDSLOT
	addu	t6,1		# BDSLOT chars stored by SWS
	blt	a2,t6,$cpy4ch	# out of space
	addu	a0,t6		# adjust source pointer
	SWS	t4,0(a1)	# store entire or part word
	subu	a1,t5,3		# adjust destination ptr
	subu	a2,t6		# decr maxlength

	/*
	 * inner loop
	 * at this point the destination is word aligned and t7
	 * points 1 byte before the corresponding source location
	 */
1:	ble	a2,4,$dumbcpy
	addu	a1,4		# BDSLOT
	lb	t0,1(t7)
	addu	t7,4		# LDSLOT
	beq	t0,zero,$cpy1ch
	nop			# BDSLOT
	lb	t1,1+1-4(t7)
	nop			# LDSLOT
	beq	t1,zero,$cpy2ch
	nop			# BDSLOT
	lb	t2,2+1-4(t7)
	addu	a0,4		# LDSLOT adjust source pointer
	beq	t2,zero,$cpy3ch
	LWS	t4,0+1-4(t7)	# BDSLOT
	subu	a2,4		# LDSLOT
	bltz	a2,$nsp4ch	# no room for 4
	lb	t3,3+1-4(t7)	# BDSLOT
	LWB	t4,3+1-4(t7)	# LDSLOT
	bne	t3,zero,1b	# LDSLOT
	sw	t4,0(a1)	# BDSLOT
	b	$cpyok
	nop			# BDSLOT

$cpy4ch:
	/*
	 * 4 bytes left to store
	 */
	subu	a2,4
	bgez	a2,$do4ch	# room left
	nop			# BDSLOT
	b	$nsp3ch		# try 3 characters
	addu	a2,1		# BDSLOT

$do4ch:
	SWS	t4,0(a1)
	b	$cpyok
	SWB	t4,3(a1)	# BDSLOT

$cpy3ch:	
	/*
	 * 3 bytes left to store
	 */
	subu	a2,3
	bgez	a2,$do3ch	# room left
	nop			# BDSLOT
	b	$nsp2ch		# no space for 3, see if 2 will fit
	addu	a2,1		# BDSLOT

$cpy2ch:
	/*
	 * 2 bytes left to store
	 */
	subu	a2,2
	bgez	a2,$do2ch	# room left
	nop			# BDSLOT
	b	$nsp1ch		# no space for 2, see if 1 will fit
	addu	a2,1		# BDSLOT

$cpy1ch:
	/*
	 * 1 last byte to store
	 */
	subu	a2,1
	bgez	a2,$do1ch	# room left
	nop			# BDSLOT
	b	$nospace	# no space at all
	addu	a2,1		# BDSLOT

$do3ch: sb	t2,2(a1)
$do2ch:	sb	t1,1(a1)
$do1ch:	sb	t0,0(a1)

$cpyok:
	/*
	 * copy complete, calculate length copied if necessary and
	 * return
	 */
	subu	a2,v0,a2		# bytes copied = maxlength - rem
	move	v0,zero			# success return code

$cpyexit:
	.set	reorder
	lw	v1,u+PCB_CPUPTR
	sw	zero,CPU_NOFAULT(v1)
	beq	a3,zero,1f		# &lencopied == 0 ?
	sw	a2,0(a3)		# no, return lencopied
1:	j	ra
	.set	noreorder

/*
 * not enough room to move one word, do stupid byte copy
 */
$dumbcpy:
	beq	a2,zero,$nospace	# no room
	nop				# BDSLOT
	lbu	t0,0(a0)
	subu	a2,1			# LDSLOT decr count
	addu	a0,1			# bump source ptr
	sb	t0,0(a1)
	bne	t0,zero,$dumbcpy	# not null terminator
	addu	a1,1			# BDSLOT bump dest ptr
	b	$cpyok
	nop				# BDSLOT

/*
 * ran out of space, copy as many characters as possible
 */
$nsp4ch:
	addu	a2,1
$nsp3ch:
	bgez	a2,$ndo3ch		# room for 3
	addu	a2,1			# BDSLOT
$nsp2ch:
	bgez	a2,$ndo2ch		# room for 2
	addu	a2,1			# BDSLOT
$nsp1ch:
	bgez	a2,$ndo1ch		# room for 1
	nop				# BDSLOT
	b	$nospace
	nop				# BDSLOT

$ndo3ch:sb	t2,2(a1)
$ndo2ch:sb	t1,1(a1)
$ndo1ch:sb	t0,0(a1)
$nospace:
	/*
	 * Ran out of space, length copied is always maxlength
	 */
	move	a2,v0			# copied max length
	b	$cpyexit
	li	v0,ENAMETOOLONG		# BDSLOT string too big
	.set	reorder
	END(cpystr)

/*
 * handle address fault for copy*str routines
 */
LEAF(cstrerror)
	lw	v0,u+PCB_CPUPTR
	sw	zero,CPU_NOFAULT(v0)
	li	v0,EFAULT
	j	ra
	END(cstrerror)

#ifdef oldmips
/*
 *	hwcpout is going from 32 bit bus to 16 bit bus, hwcpin is opposite.
 *	a0 is the source. a1 is the destination. a2 is the byte-count.
 *	a3 is (usually) the value a0 should have when the current move'
 *	loop is done.  v0,v1,t0,t1 scratch regs, used for alignment, and
 *	moves.  Does not save any regs.  No return value.
 *
 *	Basic Algorithm:
 *		First check if the count passed is < HWMINCOPY.  If yes,
 *		jump to byte copy routine, it isn't worth hassling.
 *		Then, try to align the 32 bit side on a word bit boundary.
 *		If you can't, just byte copy.  This happens very rarely, the
 *		typical case is both sides word aligned.  Then do as many
 *		16 byte copy loops as you can, then do as many 2 byte copy
 *		iterations as you can, then pick up the dregs with byte copies.
 *		This assumes typical case is full aligned, 20-112 bytes.
 */

/*
 *	HWMINCOPY is the minimum copy size on which we try to align and
 *	use half-word rather than byte copy loop.  Mash thought 8-12
 *	was a good number for bcopy(), seems to me that the outside of
 *	that range would suite here.  Why? We are assuming cache hit rate
 *	of 0%.  Our half-word loop is less efficient in its use of the
 *	WB and of cycles than his full word loop. MORE THOUGHT HERE PLEASE.
 */

#define	HWMINCOPY	12

LEAF(hwcpout)
/*
 *	first, check for alignment possibilities
 */
	.sdata
tmp:
	.word	0		# used for WB flush
	.text
	xor	v0, a0, a1	# bash src & dst for align chk
	blt	a2, HWMINCOPY, hbytecopy	# too short, just byte copy
	and	v0, 1		# low-order bit for align chk
	subu	v1, zero, a0	# -src; BDSLOT
	bne	v0, zero, hbytecopy	# src and dst not alignable

/*
 * src and dst can be simultaneously word aligned.
 */
	and	v1, 3		# number of bytes til aligned
	subu	a2, v1		# bcount -= alignment
	addu	a3,v1,a0	# end of align move
	beq	v1, zero, hblkcopy	# already aligned

/*
 * This is the easy way, could maybe be done better.  The problem
 * is that lwl/r and swl/r will not work on the 16 bit side.  Since
 * worst case is three times through, the math to do the possible
 * half-word copy does not seem worth it, nor does the shifting to
 * use the lwl/r from the 32 bit side.
 */
1:				# tight loop
	lb	v0, 0(a0)
	addu	a0, 1
	sb	v0, 0(a1)
	addu	a1, 1
	sw	zero, tmp	# ensure no WB gather
	bne	a0, a3, 1b
	
/*
 * 16 byte block, aligned copy loop (for big reads/writes)
 * We must out fox the WB on 16 bit stores, else the card will
 * punt the data.  This explains the somewhat esoteric ordering of
 * the stores.  If we write consecutive half-words to the same
 * word address, we loose.
 */
hblkcopy:
	and	a3, a2, ~15	# total space in 16 byte chunks
	subu	a2, a3		# count after by-16 byte loop done
	beq	a3, zero, hwordcopy	# less than 16 bytes to copy
	addu	a3, a0		# source endpoint
	.set noreorder
1:	lw	v0, 0(a0)
	addu	a0, 16		# src += 16 ; no other delay slot...
	lw	t0, -12(a0)
	sh	v0, 2(a1)
	srl	v0,  16
	lw	t1, -8(a0)
	sh	t0, 6(a1)
	sh	v0, 0(a1) 
	srl	t0,  16
	lw	t2, -4(a0)
	sh	t1,  10(a1)
	sh	t0,  4(a1)
	srl	t1,  16
	sh	t2,  14(a1)
	sh	t1,  8(a1)
	srl	t2,  16
	sh	t2,  12(a1)
	bne	a0, a3, 1b
	addu	a1, 16		# dst += 16
	.set reorder

/*
 * copy what ever is left,  but is aligned,  in half-words
 */
hwordcopy:
	addu	a3, a2, a0	# source endpoint;
	ble	a2, 1, hbytecopy
/*
 * This could maybe be done better?
 */
	and	t0, a3, ~1	# catch tail
	subu	a2, a3, t0
	move	a3, t0
1:				# tight loop
	.set 	noreorder
	lh	v0, 0(a0)
	addu	a0, 2		#LDSLOT
	sh	v0, 0(a1)
	addu	a1, 2
	bne	a0, a3, 1b
	sw	zero, tmp	# ensure no WB gather
	.set	reorder
	
/*
 * Brute force byte copy loop, pick up the dregs.  Also pick up copies
 * that are unalignable, doing the math to be smarter under the
 * 16 bit constraints turns out to lose.
 */

hbytecopy:
	addu	a3, a2, a0	# source endpoint; BDSLOT
	ble	a2, zero, hcopydone	# nothing left to copy,  or bad length
1:				# tight loop
	.set	noreorder
	lb	v0, 0(a0)
	addu	a0, 1		# incr src address
	sb	v0, 0(a1)
	addu	a1, 1		# incr dst address
	bne	a0, a3, 1b
	sw	zero, tmp	# ensure no WB gather
	.set	reorder
hcopydone:
	j	ra
	END(hwcpout)

LEAF(hwcpin)
/*
 *	first, check for alignment possibilities
 */
	xor	v0, a0, a1	# bash src & dst for align chk
	blt	a2, HWMINCOPY, hbytecopy	# too short, just byte copy
	and	v0, 1		# low-order bit for align chk
	subu	v1, zero, a1	# -src
	bne	v0, zero, hbytecopy	# src and dst not alignable
	addu	t5, a1, a2	# firewall panic on overrun

/*
 * src and dst can be simultaneously word aligned.
 */
	and	v1, 3		# number of bytes til aligned
	subu	a2, v1		# bcount -= alignment
	addu	a3,v1,a0	# end of align move
	beq	v1, zero, blkcpin	# already aligned

/*
 * This is the easy way, could maybe be done better.  The problem
 * is that lwl/r and swl/r will not work on the 16 bit side.  Since
 * worst case is three times through, the math to do the possible
 * half-word copy does not seem worth it, nor does the shifting to
 * use the lwl/r from the 32 bit side.
 */
1:
	lb	v0, 0(a0)
	addu	a0, 1
	sb	v0, 0(a1)
	addu	a1, 1		# should go in the BDSLOT
	bne	a0, a3, 1b
	
/* 16 byte block copy */

blkcpin:
	and	a3, a2, ~15	# total space in 16 byte chunks
	subu	a2, a3		# count after by-16 byte loop done
	beq	a3, zero, hwordcopy	# less than 16 bytes to copy
	addu	a3, a0		# source endpoint
	.set noreorder
1:	
	lhu	v0, 0(a0)
	addu	a0, 16
	lhu	v1, -14(a0)
	sll	v0, 16
	lhu	t0, -12(a0)
	or	v0, v1
	sw	v0, 0(a1)
	lhu	t1, -10(a0)
	sll	t0, 16
	or	t0, t1
	lhu	v0, -8(a0)
	sw	t0, 4(a1)
	lhu	v1, -6(a0)
	sll	v0, 16
	or	v0, v1
	lhu	t0, -4(a0)
	sw	v0, 8(a1)
	lhu	t1, -2(a0)
	sll	t0, 16
	or	t0, t1
	sw	t0, 12(a1)
	bne	a0, a3, 1b
	addu	a1, 16			# dst+= 16; fills BD slot
	.set reorder
	bgt	a1, t5, bad
	b	hwordcopy
panic_lab:
	.data
	.asciiz "hwcpin"
	.text
bad:
	lw	a0, panic_lab
	jal	panic
	
	END(hwcpin)
#endif oldmips

/*
 * The following routines uload_word(), uload_half(), uloaduhalf(),
 * ustore_word() and ustore_half() load and store unaligned items.
 * The "addr" parameter is the address at which the reference is to be
 * made.  For load routines the value is returned indirectly through
 * the "pword" parameter.  For store routines the "value" pramameter
 * is stored.  All routines indicate an error by returning a non-zero
 * value.  If no error occurs a zero is returned.
 */

/*
 * int uload_word(addr, pword)
 * u_int addr, *pword;
 */
LEAF(uload_word)
#ifdef ASSERTIONS
	lw	v0,u+PCB_CPUPTR
	lw	v0,CPU_NOFAULT(v0)
	beq	v0,zero,8f
	PANIC("recursive nofault")
8:
#endif ASSERTIONS
	.set	noreorder
	lw	v0,u+PCB_CPUPTR
	li	v1,NF_FIXADE		# LDSLOT
	sw	v1,CPU_NOFAULT(v0)
	ulw	v1,0(a0)
	sw	zero,CPU_NOFAULT(v0)
	.set	reorder
	sw	v1,0(a1)
	move	v0,zero
	j	ra
	END(uload_word)

/*
 * int uload_half(addr, pword)
 * u_int addr, *pword;
 */
LEAF(uload_half)
#ifdef ASSERTIONS
	lw	v0,u+PCB_CPUPTR
	lw	v0,CPU_NOFAULT(v0)
	beq	v0,zero,8f
	PANIC("recursive nofault")
8:
#endif ASSERTIONS
	.set	noreorder
	lw	v0,u+PCB_CPUPTR
	li	v1,NF_FIXADE		#LDSLOT
	sw	v1,CPU_NOFAULT(v0)
	.set	reorder
	ulh	v1,0(a0)
	.set	noreorder
	sw	zero,CPU_NOFAULT(v0)
	.set	reorder
	sw	v1,0(a1)
	move	v0,zero
	j	ra
	END(uload_half)

/*
 * int uload_uhalf(addr, pword)
 * u_int addr, *pword;
 */
LEAF(uload_uhalf)
#ifdef ASSERTIONS
	lw	v0,u+PCB_CPUPTR
	lw	v0,CPU_NOFAULT(v0)
	beq	v0,zero,8f
	PANIC("recursive nofault")
8:
#endif ASSERTIONS
	lw	v0,u+PCB_CPUPTR
	li	v1,NF_FIXADE		#LDSLOT
	.set	noreorder
	sw	v1,CPU_NOFAULT(v0)
	.set	reorder
	ulhu	v1,0(a0)
	.set	noreorder
	sw	zero,CPU_NOFAULT(v0)
	.set	reorder
	sw	v1,0(a1)
	move	v0,zero
	j	ra
	END(uload_uhalf)

/*
 * ustore_word(addr, value)
 * u_int addr, value;
 */
LEAF(ustore_word)
#ifdef ASSERTIONS
	lw	v0,u+PCB_CPUPTR
	lw	v0,CPU_NOFAULT(v0)
	beq	v0,zero,8f
	PANIC("recursive nofault")
8:
#endif ASSERTIONS
	lw	v1,u+PCB_CPUPTR
	li	v0,NF_FIXADE		# LDSLOT
	.set	noreorder
	sw	v0,CPU_NOFAULT(v1)
	usw	a1,0(a0)
	sw	zero,CPU_NOFAULT(v1)
	.set	reorder
	move	v0,zero
	j	ra
	END(ustore_word)

/*
 * ustore_half(addr, value)
 * u_int addr, value;
 */
LEAF(ustore_half)
#ifdef ASSERTIONS
	lw	v0,u+PCB_CPUPTR
	lw	v0,CPU_NOFAULT(v0)
	beq	v0,zero,8f
	PANIC("recursive nofault")
8:
#endif ASSERTIONS
	lw	v1,u+PCB_CPUPTR
	li	v0,NF_FIXADE		#LDSLOT
	.set	noreorder
	sw	v0,CPU_NOFAULT(v1)
	ush	a1,0(a0)
	sw	zero,CPU_NOFAULT(v1)
	.set	reorder
	move	v0,zero
	j	ra
	END(ustore_half)

LEAF(fixade_error)
	move	v0,gp
	j	ra
	END(fixade_error)

/* isis */

/*
 * kn5800_cln_icache(addr, len, wbfladdr)
 * flush i cache for range of addr to addr+len-1
 * MUST NOT DESTROY a0 AND a1, SEE clean_cache ABOVE
 * wbfladdr is the address to read to cause a write buffer flush
 */
LEAF(kn5800_cln_icache)
	lw	t1,icache_size
	.set noreorder
	nop
	mfc0	t3,C0_SR		# save sr
	nop
	mtc0	zero,C0_SR		# interrupts off
	nop
	.set reorder

	.set	noreorder
	la	v0,1f
	or	v0,K1BASE
	j	v0			# run uncached
	nop
1:	lw	v0,0(a2)		# isis hack to flush write buffer
	nop
	li	v0,SR_ISC|SR_SWC	# disable intr, isolate and swap
	mtc0	v0,C0_SR
	bltu	t1,a1,1f		# cache is smaller than region
	nop
	move	t1,a1
1:	addu	t1,a0			# ending address + 1
	move	t0,a0
	la	v0,1f			# run cached
	j	v0
	nop
	.set	reorder

1:	sb	zero,0(t0)
	sb	zero,4(t0)
	sb	zero,8(t0)
	sb	zero,12(t0)
	sb	zero,16(t0)
	sb	zero,20(t0)
	sb	zero,24(t0)
	addu	t0,32
	sb	zero,-4(t0)
	bltu	t0,t1,1b

	.set	noreorder
	la	v0,1f
	or	v0,K1BASE
	j	v0			# run uncached
	nop

1:	nop				# insure isolated stores out of pipe
	mtc0	zero,C0_SR		# unisolate, unswap
	nop				# keep pipeline clean
	nop				# keep pipeline clean
	nop				# keep pipeline clean
	mtc0	t3,C0_SR		# enable interrupts
	j	ra			# return and run cached
	nop
	.set	reorder
	END(kn5800_cln_icache)

LEAF(kn5800_cln_dcache)
	lw	t2,dcache_size
	.set noreorder
	nop
	mfc0	t3,C0_SR		# save sr
	nop
	.set reorder

	.set	noreorder
	lw	v0,0(a2)		# isis hack - to flush write buffer
	li	v0,SR_ISC		# disable interrupts, isolate caches
	mtc0	v0,C0_SR
	bltu	t2,a1,1f		# cache is smaller than region
	nop
	move	t2,a1
1:	addu	t2,a0			# ending address + 1
	move	t0,a0
	nop
	nop				# cache must be isolated by now
	.set	reorder

1:	sb	zero,0(t0)
	sb	zero,4(t0)
	sb	zero,8(t0)
	sb	zero,12(t0)
	sb	zero,16(t0)
	sb	zero,20(t0)
	sb	zero,24(t0)
	addu	t0,32
	sb	zero,-4(t0)
	bltu	t0,t2,1b

	.set	noreorder
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
	END(kn5800_cln_dcache)

/*
 * kn5800_flsh_cache()
 * flush entire cache
 */
LEAF(kn5800_flsh_cache)
	lw	t1,icache_size		# must load before isolating
	lw	t2,dcache_size		# must load before isolating
	.set noreorder
	nop
	mfc0	t3,C0_SR		# save SR
	nop
	mtc0	zero,C0_SR		# interrupts off
	nop
	.set reorder

	.set	noreorder
	la	v0,1f
	or	v0,K1BASE
	j	v0			# run uncached
	nop

	/*
	 * flush text cache - but for isis we must muck with the rinval
	 * bit in csr1 to stop invalidates.
	 */
	/* code added for isis (except for the label "1:" begins here. */
1:	lui	v1, 0x1			# put RINVAL bit in v1
	lw	t7, 0(a0)		# get csr1 contents
	nop
	or	t8, t7, v1		# set rinval bit in t8
	sw	t8, 0(a0)		# set RINVAL in csr1
	nop
2:	lw	t7, 0(a0)		# read csr1
	nop
	and	t8, t7, v1		# check rinval
	beq	t8,zero,2b		# loop till set
	nop				# branch delay slot
	/* end code added for isis */
	li	v0,SR_ISC|SR_SWC	# disable intr, isolate and swap
	mtc0	v0,C0_SR
	li	t0,K1BASE
	subu	t0,t1
	li	t1,K1BASE
	la	v0,1f			# run cached
	j	v0
	nop
	.set	reorder

1:	sb	zero,0(t0)
	sb	zero,4(t0)
	sb	zero,8(t0)
	sb	zero,12(t0)
	sb	zero,16(t0)
	sb	zero,20(t0)
	sb	zero,24(t0)
	addu	t0,32
	sb	zero,-4(t0)
	bne	t0,t1,1b

	.set	noreorder
	la	v0,1f
	or	v0,K1BASE
	j	v0			# run uncached
	nop

	/*
	 * flush data cache
	 */
1:	li	v0,SR_ISC		# isolate and swap back caches
	mtc0	v0,C0_SR
	li	t0,K1BASE
	subu	t0,t2
	la	v0,1f
	j	v0			# back to cached mode
	nop
	.set	reorder

1:	sb	zero,0(t0)
	sb	zero,4(t0)
	sb	zero,8(t0)
	sb	zero,12(t0)
	sb	zero,16(t0)
	sb	zero,20(t0)
	sb	zero,24(t0)
	addu	t0,32
	sb	zero,-4(t0)
	bne	t0,t1,1b

	.set	noreorder
	nop				# insure isolated stores out of pipe
	nop
	nop
	mtc0	t3,C0_SR		# un-isolate, enable interrupts
	nop				# insure cache unisolate
	nop
	nop
	nop
	.set	reorder
#ifdef CACHETRICKS
	lw	v0,icachemask		# index of last entry in icachecnt
	li	v1,0
	sll	v0,1			# offset to last entry
1:	lhu	t0,icachecnt(v1)
	addu	t0,1
	sh	t0,icachecnt(v1)
	addu	v1,2
	ble	v1,v0,1b		# more cachecnt's to bump

	lw	v0,dcachemask		# index of last entry in dcachecnt
	li	v1,0
	sll	v0,1			# offset to last entry
1:	lhu	t0,dcachecnt(v1)
	addu	t0,1
	sh	t0,dcachecnt(v1)
	addu	v1,2
	ble	v1,v0,1b		# more cachecnt's to bump
#endif CACHETRICKS

	j	ra
	END(kkn5800_flsh_cache)

/*
 * kn5800_page_iflush(addr, wbfladdr)
 * flush one page of i cache, addr is assumed to be in K0SEG
 * wbfladdr is the address to read to cause a write buffer flush
 */
LEAF(kn5800_pg_iflush)
	lw	t1,icache_size
	.set noreorder
	nop
	mfc0	t3,C0_SR		# save sr
	nop
	mtc0	zero,C0_SR		# interrupts off
	nop
	.set reorder
	.set	noreorder
	la	v0,1f
	or	v0,K1BASE
	j	v0			# run uncached
	nop				# BDSLOT

	/*
	 * flush text cache
	 */
1:	lw	v0,0(a1)		# isis hack to flush write buffer
	li	v0,SR_ISC|SR_SWC	# disable intr, isolate and swap
	mtc0	v0,C0_SR
	bltu	t1,NBPG,1f		# cache is smaller than region
	nop				# BDSLOT
	li	t1,NBPG
1:	addu	t1,a0			# ending address + 1
	move	t0,a0
	la	v0,1f			# run cached
	j	v0
	nop				# cache must be isolated by now
	.set	reorder

1:	sb	zero,0(t0)
	sb	zero,4(t0)
	sb	zero,8(t0)
	sb	zero,12(t0)
	sb	zero,16(t0)
	sb	zero,20(t0)
	sb	zero,24(t0)
	addu	t0,32
	sb	zero,-4(t0)
	bltu	t0,t1,1b

	.set	noreorder
	la	v0,1f
	or	v0,K1BASE
	j	v0			# run uncached
	nop

1:	nop				# insure isolated stores out of pipe
	mtc0	zero,C0_SR		# unisolate, unswap
	nop
	nop
	nop
	nop
	mtc0	t3,C0_SR		# enable interrupts
	la	v0,1f			# run cached
	j	v0
	nop

	.set	reorder
1:
#ifdef CACHETRICKS
	lw	v0,icachemask
	srl	t1,a0,PGSHIFT
	and	t1,v0
	sll	t1,1			# cachecnt index
	lhu	t0,icachecnt(t1)
	addu	t0,1
	sh	t0,icachecnt(t1)
#endif CACHETRICKS
	j	ra
	END(kn5800_pg_iflush)

/*
 * kn5800_page_dflush(addr, wbfladdr)
 * flush one page of i cache, addr is assumed to be in K0SEG
 * wbfladdr is the address to read to cause a write buffer flush
 */
LEAF(kn5800_pg_dflush)
	lw	t2,dcache_size
	.set noreorder
	nop
	mfc0	t3,C0_SR		# save sr
	nop
	.set reorder

	.set	noreorder
	lw	v0,0(a1)		# isis hack to flush write buffer
	li	v0,SR_ISC		# interrupts off, isolate caches
	mtc0	v0,C0_SR
	bltu	t2,NBPG,1f		# cache is smaller than region
	nop
	li	t2,NBPG
1:	addu	t2,a0			# ending address + 1
	move	t0,a0			# cache must be isolated by now
	.set	reorder

1:	sb	zero,0(t0)
	sb	zero,4(t0)
	sb	zero,8(t0)
	sb	zero,12(t0)
	sb	zero,16(t0)
	sb	zero,20(t0)
	sb	zero,24(t0)
	addu	t0,32
	sb	zero,-4(t0)
	bltu	t0,t2,1b

	.set	noreorder
	nop				# insure isolated stores out of pipe
	nop
	nop
	mtc0	t3,C0_SR		# un-isolate, enable interrupts
	nop				# insure cache unisolated
	nop
	nop
	.set	reorder
#ifdef CACHETRICKS
	lw	v0,dcachemask
	srl	t1,a0,PGSHIFT
	and	t1,v0
	sll	t1,1			# cachecnt index
	lhu	t0,dcachecnt(t1)
	addu	t0,1
	sh	t0,dcachecnt(t1)
#endif CACHETRICKS
	j	ra
	END(kkn5800_pg_dflush)
