/*	@(#)memset.s	4.2	ULTRIX	8/13/90	*/
/*
 *  Copyright 1990, Digital Equipment Corporation
 *
 *  char *memset(char *s, unsigned char c, int n)
 *  fill n bytes of memory at *s with c
 *
 *  Sys V memset.  This is optimized for extremely high performance for
 *  both for small blocks (string padding) and large blocks (memory fill).
 *  In order to reduce overhead for small cases, they are retired as quickly
 *  as possible, more case analysis is reserved for cases which will do
 *  more.  (This is based on the GEM support routine ots_fill.)
 *
 *  Warning: This is basically "microcode" and uses any and all dirty tricks
 *  to be both small and fast...
 *
 *  See also: BSD "bzero"
 *
 *  Please send copies of any changes/bugfixes to smop::glossop.
 *  KDG     26 Apr 1990
 */

#include <mips/regdef.h>
#include <mips/asm.h>

#ifdef MIPSEB
#   define sws swl
#   define swb swr
#else
#   define sws swr
#   define swb swl
#endif

#define rd $4	/* destination */
#define rp $5	/* pad */
#define rl $6	/* length */

LEAF(memset)
	.set noreorder	# delay slots are all filled manually
	beqz	rl, return		# exit if len=0
	addu	rl, -4			# 1, 2 or 3 bytes?
	bgez	rl, long		# if not - go fill longer area
	addu	t0, rl, 2		# map length 1/2/3 => -1/0/1 for simple cases
	bltz	t0, return		# exit if only 1 byte store
	sb	rp, (rd)		# write 1st byte in delay slot
	blez	t0, return		# exit if only 2 byte store
	sb	rp, 1(rd)		# write 2nd byte in delay slot
	sb	rp, 2(rd)		# write 3rd byte
return:	j	ra			# unconditional return
long:	move	v0, rd			# return value (in delay slot of "return:")
	and	rp, 0xff		# ensure clean fill byte
	sll	t0, rp, 8		# form fullword of fill bytes
	or	rp, t0
	sll	t0, rp, 16
	or	rp, t0
	and	t0, rd, 3		# low 2 bits of address ("offset")
	sws	rp, (rd)		# store 1-4 bytes initially
	addu	rl, t0			# get remaining length
	and	t1, rl, 15		# unrolled loop of interest?
	addu	rd, 4			# bump pointer to next fullword
	beq	t1, rl, word		# if equal, no unrolled interations
	xor	rd, t0			# clear low order bits of address
	xor	t1, rl			# get unrolled amount
	addu	t0, t1, rd		# unrolled loop end addr
	and	rl, 15			# set rl to new value
unrllp:	sw	rp, (rd)		# 4x unrolled loop
	sw	rp, 4(rd)
	addu	rd, 16
	sw	rp, -8(rd)
	bne	rd, t0, unrllp
	sw	rp, -4(rd)
word:	srl	t0, rl, 2		# test to see if there are words
	beqz	t0, finish		# if not, go finish off last longword
	addu	t1, rd, rl		# get final store addr in t1
	add	t0, -2	 		# map 1/2/3 to -1/0/1
	bltz	t0, finish
	sw	rp, (rd)
	blez	t0, finish
	sw	rp, 4(rd)
	sw	rp, 8(rd)
finish:	j	ra
	swb	rp, -1(t1)		# finish off the last partial word
					# may re-store last 4 bytes (aligned case)
	.set	reorder
	.end	memset
