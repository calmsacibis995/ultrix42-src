/*	@(#)memcpy.s	4.1	(ULTRIX)	7/3/90				      */
/* ------------------------------------------------------------------ */
/* | Copyright Unpublished, MIPS Computer Systems, Inc.  All Rights | */
/* | Reserved.  This software contains proprietary and confidential | */
/* | information of MIPS and its suppliers.  Use, disclosure or     | */
/* | reproduction is prohibited without the prior express written   | */
/* | consent of MIPS.                                               | */
/* ------------------------------------------------------------------ */
/* $Header: memcpy.s,v 1.1 87/03/05 15:32:41 dce Exp $ */

#include <mips/regdef.h>
#include <mips/asm.h>

#define	NBPW	4

/*
 * memcpy(src, dst, bcount)
 *
 * NOTE: the optimal copy here is somewhat different than for the user-level
 * equivalents (bcopy in 4.2, memcpy in V), because:
 * 1) it frequently acts on uncached data, especially since copying from
 * (uncached) disk buffers into user pgms is high runner.
 * This means one must be careful with lwl/lwr/lb - don't expect cache help.
 * 2) the distribution of usage is very different: there are a large number
 * of memcopies for small, aligned structures (like for ioctl, for example),
 * a reasonable number of randomly-sized copies for user I/O, and many
 * memcopies of large (page-size) blocks for stdio; the latter must be
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
 * (Warning: these are gross numbers, and the code has changed a bunch):
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
 *	a0	dst addr
 *	a1	src addr
 *	a2	length remaining
 */
#define	MINCOPY	8

/* It turns out better to think of lwl/lwr and swl/swr as
   smaller-vs-bigger address rather than left-vs-right.
   Such a representation makes the code endian-independent. */

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

LEAF(memcpy)
	move	v0,a0			# return a copy of dest address;BDSLOT
	blt	a2,MINCOPY,bytecopy	# too short, just byte copy
	xor	t2,a1,a0		# bash src & dst for align chk
	and	t2,NBPW-1		# low-order bits for align chk
	subu	v1,zero,a1		# -src; BDSLOT
	bne	t2,zero,unaligncopy	# src and dst not alignable
/*
 * src and dst can be simultaneously word aligned
 */
	and	v1,NBPW-1		# number of bytes til aligned
	subu	a2,v1			# bcount -= alignment; BDSLOT
	beq	v1,zero,blkcopy		# already aligned
	LWS	t2,0(a1)		# copy unaligned portion
	SWS	t2,0(a0)
	addu	a1,v1			# src += alignment
	addu	a0,v1			# dst += alignment

/*
 * 32 byte block, aligned copy loop (for big reads/writes)
 */
blkcopy:
	and	t2,a2,31		# count after by-32-byte loop done
	subu	a3,a2,t2		# total in 32 byte chunks; BDSLOT
	beq	a2,t2,wordcopy		# less than 32 bytes to copy
	addu	a3,a1			# source endpoint
1:	lw	t7,0(a1)
	lw	v1,4(a1)
	lw	t0,8(a1)
	lw	t1,12(a1)
	lw	t3,16(a1)
	lw	t4,20(a1)
	lw	t5,24(a1)
	lw	t6,28(a1)
	sw	t7,0(a0)		# stores collected together
	sw	v1,4(a0)		# so that interleaving works better
	sw	t0,8(a0)
	sw	t1,12(a0)
	sw	t3,16(a0)
	sw	t4,20(a0)
	addu	a1,32			# src+= 32; here to ease loop end
	sw	t5,24(a0)
	sw	t6,28(a0)
	addu	a0,32			# dst+= 32; fills BD slot
	bne	a1,a3,1b
	move	a2,t2			# set end-of loop count

/*
 * word copy loop
 */
wordcopy:
	and	t2,a2,(NBPW-1)		# count after by-word loop done
	subu	a3,a2,t2		# total in word chunks
	beq	a2,t2,bytecopy		# less than a word to copy
	addu	a3,a1			# source endpoint
1:	lw	t3,0(a1)
	addu	a1,NBPW
	sw	t3,0(a0)
	addu	a0,NBPW			# dst += 4; BD slot
	bne	a1,a3,1b
	move	a2,t2			# set end-of loop count

/*
 * brute force byte copy loop, for bcount < MINCOPY + tail of unaligned dst
 * note that lwl, lwr, swr CANNOT be used for tail, since the lwr might
 * cross page boundary and give spurious address exception
 */
bytecopy:
	addu	a3,a2,a1		# source endpoint; BDSLOT
	ble	a2,zero,copydone	# nothing left to copy, or bad length
1:	lb	t2,0(a1)
	addu	a1,1
	sb	t2,0(a0)
	addu	a0,1			# BDSLOT: incr dst address
	bne	a1,a3,1b
copydone:
	j	ra

/*
 * deal with simultaneously unalignable copy by aligning dst
 */
unaligncopy:
	subu	a3,zero,a0		# calc byte cnt to get dst aligned
	and	a3,NBPW-1		# alignment = 0..3
	subu	a2,a3			# bcount -= alignment
	beq	a3,zero,partaligncopy	# already aligned
	LWS	t2,0(a1)		# get whole word
	LWB	t2,3(a1)		# for sure
	SWS	t2,0(a0)		# store low-address piece (1-3 bytes)
	addu	a1,a3			# src += alignment (will fill LD slot)
	addu	a0,a3			# dst += alignment

/*
 * src unaligned, dst aligned loop
 * NOTE: if MINCOPY >= 7, will always do 1 loop iteration or more
 * if we get here at all
 */
partaligncopy:
	and	t2,a2,(NBPW-1)		# count after by-word loop done
	subu	a3,a2,t2		# total in word chunks
#if MINCOPY < 7
	beq	a2,t2,bytecopy		# less than a word to copy
#endif
	addu	a3,a1			# source endpoint
1:
	LWS	t3,0(a1)
	LWB	t3,3(a1)
	addu	a1,NBPW
	sw	t3,0(a0)
	addu	a0,NBPW
	bne	a1,a3,1b
	move	a2,t2			# set end-of loop count
	b	bytecopy

.end memcpy
