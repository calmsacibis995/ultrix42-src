#ident "$Header: libasm.s,v 1.1 87/08/18 16:23:56 mdove Exp $"
/*		4.2	libasm.s
 *
 * Copyright 1985 by MIPS Computer Systems, Inc.
 *
 * libasm.s -- standalone libc'ish assembler code
 *
 * Revision History:
 *
 * Oct 09, 1990 - Joe Szczypek
 * 	Removed unused code to make space in bootblks.
 *	Made bcopy and bzero builable in two forms, small
 *	and large, to save space in bootblks.
 */

#include "../../machine/mips/regdef.h"
#include "../../machine/mips/cpu.h"
#include "../../machine/mips/cpu_board.h"
#include "../../machine/mips/asm.h"

	BSS(_icache_size, 4)		# bytes of icache
	BSS(_dcache_size, 4)		# bytes of dcache

#define	NBPW	4

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
#define	MINCOPY	8

LEAF(bcopy)
#ifdef SECONDARY
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
#endif SECONDARY

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
 * breakeven (VME) = 81 / (24 - 6)  =~ 4.5 words
 * Since the overhead is pessimistic (worst-case alignment), and many calls
 * will be for well-aligned data, and since Word-zeroing at least leaves
 * the zero in the cache, we shade these values (18-20) down to 12
 */
#define	MINZERO	12

LEAF(bzero)
#ifdef SECONDARY

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
#endif SECONDARY

	ble	a1,zero,zerodone
	addu	a1,a0			# dst endpoint
1:	addu	a0,1
	sb	zero,-1(a0)
	bne	a0,a1,1b
zerodone:
	j	ra
	END(bzero)

/*
 * _cksum1(addr, len, prevcksum)
 *
 * Calculates a 16 bit ones-complement checksum.
 * Note that for a big-endian machine, this routine always adds even
 * address bytes to the high order 8 bits of the 16 bit checksum and
 * odd address bytes are added to the low order 8 bits of the 16 bit checksum.
 * For little-endian machines, this routine always adds even address bytes
 * to the low order 8 bits of the 16 bit checksum and the odd address bytes
 * to the high order 8 bits of the 16 bit checksum.
 */
LEAF(_cksum1)
	move	v0,a2		# copy previous checksum
	beq	a1,zero,4f	# count exhausted
	and	v1,a0,1
	beq	v1,zero,2f	# already on a halfword boundry
	lbu	t8,0(a0)
	addu	a0,1
#ifdef MIPSEL
	sll	t8,8
#endif MIPSEL
	addu	v0,t8
	subu	a1,1
	b	2f

1:	lhu	t8,0(a0)
	addu	a0,2
	addu	v0,t8
	subu	a1,2
2:	bge	a1,2,1b
	beq	a1,zero,3f	# no trailing byte
	lbu	t8,0(a0)
#ifdef MIPSEB
	sll	t8,8
#endif MIPSEB
	addu	v0,t8
3:	srl	v1,v0,16	# add in all previous wrap around carries
	and	v0,0xffff
	addu	v0,v1
	srl	v1,v0,16	# wrap-arounds could cause carry, also
	addu	v0,v1
	and	v0,0xffff
4:	j	ra
	END(_cksum1)

/*
 * nuxi_s and nuxi_l -- byte swap short and long
 */
LEAF(nuxi_s)			# a0 = ??ab
	srl	v0,a0,8		# v0 = 0??a
	and	v0,0xff		# v0 = 000a
	sll	v1,a0,8		# v1 = ?ab0
	or	v0,v1		# v0 = ?aba
	and	v0,0xffff	# v0 = 00ba
	j	ra
	END(nuxi_s)

LEAF(nuxi_l)			# a0 = abcd
	sll	v0,a0,24	# v0 = d000
	srl	v1,a0,24	# v1 = 000a
	or	v0,v0,v1	# v0 = d00a
	and	v1,a0,0xff00	# v1 = 00c0
	sll	v1,v1,8		# v1 = 0c00
	or	v0,v0,v1	# v0 = dc0a
	srl	v1,a0,8		# v1 = 0abc
	and	v1,a0,0xff00	# v1 = 00b0
	or	v0,v0,v1	# v0 = dcba
	j	ra
	END(nuxi_l)

/*
 * clearseg(dst_ppn)
 *
 *	Performance
 *	Config	Cycles/	Speed vs VAX
 *		4K Page	
 *	08V11	6,144	1.09X
 *	08M44	1,229	5.46X	(could be made faster by unroll to 64)
 */
#define	PGSHIFT	12
#define	NBPG	(4*1024)
LEAF(clearpage)
	addu	t0,a0,NBPG-32		# dst on last pass of loop
1:	sw	zero,0(a0)
	sw	zero,4(a0)
	sw	zero,8(a0)
	sw	zero,12(a0)
	sw	zero,16(a0)
	sw	zero,20(a0)
	sw	zero,24(a0)
	sw	zero,28(a0)
	.set	noreorder
	bne	a0,t0,1b
	addu	a0,32			# BDSLOT: inc dst, NOTE after test
	.set	reorder
	j	ra
	END(clearpage)

/*
 * wbflush() -- spin until write buffer empty
 */
LEAF(wbflush)
	sw	zero,wbflush_tmp
	lw	zero,wbflush_tmp
	j	ra
	END(wbflush)

	LBSS(wbflush_tmp, 4)
