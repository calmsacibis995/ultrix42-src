/* static char *sccsid = "@(#)kn230_copy.s	4.2      (ULTRIX)        1/3/91";  */

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
/* ------------------------------------------------------------------ */
/* | Copyright Unpublished, MIPS Computer Systems, Inc.  All Rights | */
/* | Reserved.  This software contains proprietary and confidential | */
/* | information of MIPS and its suppliers.  Use, disclosure or     | */
/* | reproduction is prohibited without the prior express written   | */
/* | consent of MIPS.                                               | */
/* ------------------------------------------------------------------ */

/************************************************************************
 *
 *	Modification History: kn230_copy.s
 *
 * May-23-1990	Paul Grist 
 *	Created this file which contains specific copy routines for
 *   	use by the mipsmate scsi driver (sii). Use of these modified
 *	copy routine provides a work-around for a mipsmate hardware
 *	problem which is considered to be a high risk h/w fix. These
 *	routines were written by Eric Noya (mipsmate firmware), who modified
 *      the original bcopy routines for use by the firmware driver. In
 *	addition to working around the h/w problem, the routines have
 *      been optimized to the mipsmate (and R3000) architecture to take
 *	advantage of paged-mode I/O, this results in higher than expected
 *	scsi performance for mipsmate.
 *	
 ***********************************************************************/

#ifdef PROM
#include "machine/asm.h"
#include "machine/regdef.h"
#else
#include <asm.h>
#include <regdef.h>
#endif

#define	NBPW	4


/*
 * kn230_bzero(dst, bcount)
 * Zero block of memory
 *
 * Calculating MINZERO, assuming 50% cache-miss on non-loop code:
 * Overhead =~ 18 instructions => 63 (81) cycles
 * Byte zero =~ 16 (24) cycles/word for 08M44 (08V11)
 * Word zero =~ 3 (6) cycles/word for 08M44 (08V11)
 * If I-cache-miss nears 0, MINZERO ==> 4 bytes; otherwise, times are:
 * breakeven (MEM) = 63 / (16 - 3) =~ 5 words
 * breakeven (VME) = 81 / (24 - 6)  =~ 4.5 words
 * Since the overhead is pessimistic (worst-case alignment), and many calls
 * will be for well-aligned data, and since Word-zeroing at least leaves
 * the zero in the cache, we shade these values (18-20) down to 12
 */
#define	MINZERO	4

LEAF(kn230_bzero)
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
	addu	a0,4
	and	a0,0xfffffff8		# align on an 8 byte boundary
/*
 * zero 32 byte, aligned block
 */
blkzero:
	and	a3,a1,~31		# 32 byte chunks
	subu	a1,a3
	beq	a3,zero,wordzero
	addu	a3,a3,a3
	addu	a3,a0			# dst endpoint
1:	sw	zero,0x0(a0)
	sw	zero,0x8(a0)
	sw	zero,0x10(a0)
	sw	zero,0x18(a0)
	addu	a0,0x40
	sw	zero,-0x20(a0)
	sw	zero,-0x18(a0)
	sw	zero,-0x10(a0)
	sw	zero,-0x08(a0)
	bne	a0,a3,1b
wordzero:
	and	a3,a1,~(NBPW-1)		# word chunks
	subu	a1,a3
	beq	a3,zero,bytezero	
	addu	a3,a3
	addu	a3,a0			# dst endpoint
1:	addu	a0,NBPW*2		
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
	END(kn230_bzero)

/*
 * kn230_rbcopy(src, dst, bcount)
 *
 * NOTE: the optimal copy here is somewhat different than for the user-level
 * equivalents (kn230_rbcopy in 4.2, memcpy in V), because:
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
 *	08V11	1	35	0.71X	(8MHz, VME, 1-Deep WB, 1-way ILV)
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
/*
 * This routine is used to copy from a ram buffer that holds data
 * in alternate 32 bit words. The data in the rambuf is always
 * aligned (a characteristic of the hardware). Thus the 'normal'
 * scheme of aligning the destination really screws up the source.
 * What is done now is to leave the source aligned and
 * handle the mis-aligned destination, which is at least
 * contiguous. Burns - 12/19/90
 */

#define	MINCOPY	4
/*
 * This is really a lot simpler than regular bcopy.
 * We know only the destination can be mis-aligned.
 * Thus we have two cases; both source and dest are
 * lined up on word (32 bit) boundries and we can do
 * the easy copy, or the dest is not lined up and we
 * do the tough one. - burns
 */
LEAF(kn230_rbcopy)
	and	v0,a1,NBPW-1		# See if dest is aligned
	blt	a2,MINCOPY,rbytecopy	# too short, just byte copy
	subu	v1,zero,a1		# -dest; BDSLOT
	bne	v0,zero,rpartaligncopy	# dest not aligned

/*
 * 32 byte block, aligned copy loop (for big reads/writes)
 */
#ifdef PROM
rblkcopy:
        li      t1,0x9fffffff		# need mask for cashing
        la      v0,1f
        and     v0,v0,t1		# switch to kseg0
        j       v0                      # run cached
#else
rblkcopy:
#endif
1:	and	a3,a2,~31		# total space in 32 byte chunks
	subu	a2,a3			# count after by-32 byte loop done
	beq	a3,zero,rwordcopy	# less than 32 bytes to copy
	addu	a3,a3
	addu	a3,a0			# source endpoint
2:	lw	v0,0x0(a0)
	lw	v1,0x8(a0)
	lw	t0,0x10(a0)
	lw	t1,0x18(a0)
	lw	t2,0x20(a0)
	lw	t3,0x28(a0)
	lw	t4,0x30(a0)
	lw	t5,0x38(a0)
	sw	v0,0(a1)
	sw	v1,4(a1)
	sw	t0,8(a1)
	sw	t1,12(a1)
	sw	t2,16(a1)
	sw	t3,20(a1)
	addu	a0,0x40			
	sw	t4,24(a1)
	sw	t5,28(a1)
	addu	a1,32			# dst+= 32; fills BD slot
	bne	a0,a3,2b

/*
 * word copy loop
 */
rwordcopy:
	and	a3,a2,~(NBPW-1)		# word chunks
	subu	a2,a3			# count after by word loop
	beq	a3,zero,rbytecopy	# less than a word to copy
	addu	a3,a3
	addu	a3,a0			# source endpoint
1:	lw	v0,0(a0)
	addu	a0,NBPW*2
	sw	v0,0(a1)
	addu	a1,NBPW			# dst += 4; BD slot
	bne	a0,a3,1b
	b	rbytecopy

/*
 * For the specific ram buffer layout that this code is written for
 * the ram buffer is the source and it is already aligned. Plus
 * it only holds data in every other word (32 bit word, or longword).
 * Thus if we mis-align it, it is real tough to copy. Instead we deal
 * with the mis-aligned destination. Maybe the cache won't like it
 * but it will work. - burns
 */
/*
 * dst unaligned, src aligned loop
 * NOTE: if MINCOPY >= 7, will always do 1 loop iteration or more
 * if we get here at all
 */
rpartaligncopy:
	and	a3,a2,~(NBPW-1)		# space in word chunks
	subu	a2,a3			# count after 'by word' loop
#if MINCOPY < 7
	beq	a3,zero,rbytecopy	# less than a word to copy
#endif
	# due to screwy rambuf source end point is
	# twice as far away as you would expect
	addu	a3,a3			# double the word count to create
	addu	a3,a0			# the source endpoint
1:
	lw	v0,0(a0)		# get next word
#ifdef MIPSEB
	swl	v0,0(a1)		# store left half
	swr	v0,3(a1)		# store right half
#endif
#ifdef MIPSEL
	swr	v0,0(a1)		# store right half
	swl	v0,3(a1)		# store left half
#endif
	addu	a0,NBPW*2		# bump source pointer, skipping 'hole'
	addu	a1,NBPW			# bump dest pointer
	bne	a0,a3,1b		# at end of word copy?


/*
 * brute force byte copy loop, for bcount < MINCOPY + tail of unaligned src
 * note that lwl, lwr, swr CANNOT be used for tail, since the lwr might
 * cross page boundary and give spurious address exception
 */
rbytecopy:
	addu	a3,a2,a0		# source endpoint; BDSLOT
	ble	a2,zero,rcopydone	# nothing left to copy, or bad length
1:	lb	v0,0(a0)
	addu	a0,1
	sb	v0,0(a1)
	addu	a1,1			# BDSLOT: incr dst address
	bne	a0,a3,1b
rcopydone:
	j	ra
	END(kn230_rbcopy)

/*
 * kn230_wbcopy(src, dst, bcount)
 *
 * NOTE: the optimal copy here is somewhat different than for the user-level
 * equivalents (kn230_wbcopy in 4.2, memcpy in V), because:
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
 *	08V11	1	35	0.71X	(8MHz, VME, 1-Deep WB, 1-way ILV)
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
#define	MINCOPY	4

LEAF(kn230_wbcopy)
	xor	v0,a0,a1		# bash src & dst for align chk; BDSLOT
	blt	a2,MINCOPY,wbytecopy	# too short, just byte copy
	and	v0,NBPW-1		# low-order bits for align chk
	subu	v1,zero,a0		# -src; BDSLOT
	bne	v0,zero,wunaligncopy	# src and dst not alignable
/*
 * src and dst can be simultaneously word aligned
 */
	and	v1,NBPW-1		# number of bytes til aligned
	subu	a2,v1			# bcount -= alignment
	beq	v1,zero,wblkcopy		# already aligned
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
	add	a1,4
	and	a1,0xfffffff8		# align on an 8 byte boundary

/*
 * 32 byte block, aligned copy loop (for big reads/writes)
 */

#ifdef PROM
wblkcopy:
        li      t1,0x9fffffff		# need mask for cashing
        la      v0,1f
        and     v0,v0,t1		# switch to kseg0
        j       v0                      # run cached
#else
wblkcopy:
#endif
1:	and	a3,a2,~31		# total space in 32 byte chunks
	subu	a2,a3			# count after by-32 byte loop done
	beq	a3,zero,wwordcopy	# less than 32 bytes to copy
	addu	a3,a0			# source endpoint
2:	lw	v0,0(a0)
	lw	v1,4(a0)
	lw	t0,8(a0)
	lw	t1,12(a0)
	sw	v0,0(a1)
	sw	v1,0x8(a1)
	sw	t0,0x10(a1)
	sw	t1,0x18(a1)
	addu	a0,32			# src+= 32; here to ease loop end
	lw	v0,-16(a0)
	lw	v1,-12(a0)
	lw	t0,-8(a0)
	lw	t1,-4(a0)
	sw	v0,0x20(a1)
	sw	v1,0x28(a1)
	sw	t0,0x30(a1)
	sw	t1,0x38(a1)
	addu	a1,0x40			# dst+= 32; fills BD slot
	bne	a0,a3,2b

/*
 * word copy loop
 */
wwordcopy:
	and	a3,a2,~(NBPW-1)		# word chunks
	subu	a2,a3			# count after by word loop
	beq	a3,zero,wbytecopy	# less than a word to copy
	addu	a3,a0			# source endpoint
1:	lw	v0,0(a0)
	addu	a0,NBPW
	sw	v0,0(a1)
	addu	a1,NBPW*2		# dst += 4; BD slot
	bne	a0,a3,1b
	b	wbytecopy

/*
 * deal with simultaneously unalignable copy by aligning dst
 */
wunaligncopy:
	subu	a3,zero,a1		# calc byte cnt to get dst aligned
	and	a3,NBPW-1		# alignment = 0..3
	subu	a2,a3			# bcount -= alignment
	beq	a3,zero,wpartaligncopy	# already aligned
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
	add	a1,4
	and	a1,0xfffffff8	        # JN trmendus hack

/*
 * src unaligned, dst aligned loop
 * NOTE: if MINCOPY >= 7, will always do 1 loop iteration or more
 * if we get here at all
 */
wpartaligncopy:
	and	a3,a2,~(NBPW-1)		# space in word chunks
	subu	a2,a3			# count after by word loop
#if MINCOPY < 7
	beq	a3,zero,wbytecopy	# less than a word to copy
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
	addu	a1,NBPW*2
	bne	a0,a3,1b


/*
 * brute force byte copy loop, for bcount < MINCOPY + tail of unaligned dst
 * note that lwl, lwr, swr CANNOT be used for tail, since the lwr might
 * cross page boundary and give spurious address exception
 */
wbytecopy:
	addu	a3,a2,a0		# source endpoint; BDSLOT
	ble	a2,zero,wcopydone	# nothing left to copy, or bad length
1:	lb	v0,0(a0)
	addu	a0,1
	sb	v0,0(a1)
	addu	a1,1			# BDSLOT: incr dst address
	bne	a0,a3,1b
wcopydone:
	j	ra
	END(kn230_wbcopy)
