/*	@(#)bcopy.s	4.1	(ULTRIX)	7/3/90	*/
/* ------------------------------------------------------------------ */
/* | Copyright Unpublished, MIPS Computer Systems, Inc.  All Rights | */
/* | Reserved.  This software contains proprietary and confidential | */
/* | information of MIPS and its suppliers.  Use, disclosure or     | */
/* | reproduction is prohibited without the prior express written   | */
/* | consent of MIPS.                                               | */
/* ------------------------------------------------------------------ */
/* $Header: bcopy.s,v 1.1 87/02/16 11:17:19 dce Exp $ */

#include <mips/regdef.h>
#include <mips/asm.h>

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

LEAF(bcopy)
	slt	t1,a0,a1		# a0 < a1 ?
	addu	t2,a0,a2		# end of str in t2
	slt	t3,a1,t2		# a1 < a0 + a2
	and	t2,t1,t3		# anything on....
	beq	t2,zero,forwards	# overlapping...do backwards copy
/*	# backwards copy for overlapping strings....	*/
	addu	a0,a0,a2		# a0 is at end now
	addu	a1,a1,a2		# a1 is at end now
	ble	a2,zero,copydone	# nothing left to copy, or bad length
1:	lb	v0,-1(a0)		# get first byte
	addiu	a0,a0,-1		# move a0 back one
	sb	v0,-1(a1)		# store the byte
	addiu	a1,a1,-1		# move a1 back 1
	addiu	a2,a2,-1		# one less to copy
	bne	a2,zero,1b		# if more loop back...
	j	ra			# bye....
forwards:
	xor	v0,a0,a1		# bash src & dst for align chk; BDSLOT
	blt	a2,MINCOPY,bytecopy	# too short, just byte copy
	and	v0,NBPW-1		# low-order bits for align chk
	subu	v1,zero,a0		# -src; BDSLOT
	bne	v0,zero,unaligncopy	# src and dst not alignable
/*
 * src and dst can be simultaneously word aligned
 */
	and	v1,NBPW-1		# number of bytes til aligned
	subu	a2,v1			# bcount -= alignment; BDSLOT
	beq	v1,zero,blkcopy		# already aligned
	LWS	v0,0(a0)		# copy unaligned portion
	SWS	v0,0(a1)
	addu	a0,v1			# src += alignment
	addu	a1,v1			# dst += alignment

/*
 * 32 byte block, aligned copy loop (for big reads/writes)
 */
blkcopy:
	and	v0,a2,31		# count after by-32-byte loop done
	subu	a3,a2,v0		# total in 32 byte chunks; BDSLOT
	beq	a2,v0,wordcopy		# less than 32 bytes to copy
	move	a2,v0			# set end-of loop count
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
	and	v0,a2,(NBPW-1)		# count after by-word loop done
	subu	a3,a2,v0		# total in word chunks
	beq	a2,v0,bytecopy		# less than a word to copy
	move	a2,v0			# set end-of loop count
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
	LWS	v0,0(a0)		# get whole word
	LWB	v0,3(a0)		# for sure
	SWS	v0,0(a1)		# store left piece (1-3 bytes)
	addu	a0,a3			# src += alignment (will fill LD slot)
	addu	a1,a3			# dst += alignment

/*
 * src unaligned, dst aligned loop
 * NOTE: if MINCOPY >= 7, will always do 1 loop iteration or more
 * if we get here at all
 */
partaligncopy:
	and	v0,a2,(NBPW-1)		# count after by-word loop done
	subu	a3,a2,v0		# total in word chunks
#if MINCOPY < 7
	beq	a2,v0,bytecopy		# less than a word to copy
#endif
	move	a2,v0			# set end-of loop count
	addu	a3,a0			# source endpoint
1:
	LWS	v0,0(a0)
	LWB	v0,3(a0)
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
.end bcopy
