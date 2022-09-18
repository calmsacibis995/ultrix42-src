/*	@(#)xmalloc.s	4.1	(ULTRIX)	7/3/90				      */
/*	stolen from libp.a....yuck rr				      */

/* --------------------------------------------------- */
/* | Copyright (c) 1986 MIPS Computer Systems, Inc.  | */
/* | All Rights Reserved.                            | */
/* --------------------------------------------------- */
/* $Header: alloc.s,v 1031.2 88/05/16 14:29:13 bettina Exp $ */

/*------------------------------------------------------------------*/
/*| Copyright Unpublished, MIPS Computer Systems, Inc.  All Rights |*/
/*| Reserved.  This software contains proprietary and confidential |*/
/*| information of MIPS and its suppliers.  Use, disclosure or     |*/
/*| reproduction is prohibited without the prior express written   |*/
/*| consent of MIPS.                                               |*/
/*------------------------------------------------------------------*/
/*$Header: alloc.s,v 1031.2 88/05/16 14:29:13 bettina Exp $*/

#include <regdef.h>

/* Move these .dsects to .h file with pascal definitions */
.struct 0
last_scb:	.word	0
next_scb:	.word	0
free_list:	.word	0

.struct 0
prev_size:	.word	0
curr_size:	.word	0
last_bcb:	.word	0
next_bcb:	.word	0

#define curr_allocated 1
#define prev_allocated 2

.comm malloc_scb,4

.text	

.globl xrealloc
.ent xrealloc
xrealloc:
	.frame	sp, 0, ra
	la	a2, malloc_scb
	j	alloc_resize
.end xrealloc

.globl xfree
.ent xfree
xfree:
	.frame	sp, 0, ra
	la	a1, malloc_scb
.end xfree
.globl	alloc_dispose
.ent	alloc_dispose
alloc_dispose:
	.frame	sp, 0, $31
 # 413	    r := ptradd(bcbp, fptr, -8);
	addu	$6, $4, -8
 # 414	    p := scbp(fheap);
	lw	$8, 0($5)
	move	$7, $8
 # 415	    if p^.last_scb <> nil then begin
	lw	$2, 0($8)
	beq	$2, 0, $67
 # 416	
 # 417	      {-----------------------------------------------------------------------}
 # 418	      { find scb that contains block, either forward from heap or backward    }
 # 419	      {-----------------------------------------------------------------------}
 # 420	      while (p <> nil) and ((ord(p) > ord(r))
	beq	$7, 0, $62
	bgt	$7, $6, $61
	lw	$3, 12($7)
	abs	$24, $3
	addu	$25, $7, $24
	bge	$25, $6, $62
$61:
 # 421	       or ((ord(p)+abs(p^.scb_size)) < ord(r))) do p := p^.next_scb;
	lw	$7, 4($7)
	beq	$7, 0, $62
	bgt	$7, $6, $61
	lw	$14, 12($7)
	abs	$15, $14
	addu	$2, $7, $15
	blt	$2, $6, $61
$62:
 # 422	      if p = nil then begin
	bne	$7, 0, $65
 # 423		p := scbp(fheap);
	move	$7, $8
 # 424		while (p <> nil) and ((ord(p) > ord(r))
	beq	$7, 0, $64
	bgt	$7, $6, $63
	lw	$3, 12($7)
	abs	$24, $3
	addu	$25, $7, $24
	bge	$25, $6, $64
$63:
 # 425		 or ((ord(p)+abs(p^.scb_size)) < ord(r))) do p := p^.last_scb;
	lw	$7, 0($7)
	beq	$7, 0, $64
	bgt	$7, $6, $63
	lw	$14, 12($7)
	abs	$15, $14
	addu	$2, $7, $15
	blt	$2, $6, $63
$64:
 # 426		if p = nil then return; 	{ could report an error 	      }
	beq	$7, 0, $75
$65:
 # 427	      end;
 # 428	
 # 429	      {-----------------------------------------------------------------------}
 # 430	      { find mark that controls storage for that scb			      }
 # 431	      {-----------------------------------------------------------------------}
 # 432	      while p^.scb_size < 0 do p := p^.last_scb;
	lw	$3, 12($7)
	bge	$3, 0, $67
$66:
	lw	$7, 0($7)
	lw	$24, 12($7)
	blt	$24, 0, $66
$67:
	li	$12, -4
 # 433	    end;
 # 434	
 # 435	    lsize := r^.curr_size;
	lw	$4, 4($6)
 # 436	    rsize := bitand(lsize, bcb_mask);
	and	$5, $4, $12
 # 437	    s := ptradd(bcbp, r, rsize);
	addu	$9, $6, $5
	move	$8, $9
 # 438	    msize := s^.curr_size;
	lw	$10, 4($8)
 # 439	    ssize := bitand(msize, bcb_mask);
	and	$11, $10, $12
 # 446	    if bitand(lsize, prev_allocated) = 0 then begin
	and	$25, $4, 2
	bne	$25, 0, $71
 # 447	      tsize := r^.prev_size;
	lw	$9, 0($6)
	move	$4, $9
 # 448	      t := ptradd(bcbp, r, -tsize);
	subu	$12, $6, $4
 # 449	      if not odd(msize) then begin	{ merge all three blocks	      }
	and	$14, $10, 1
	bne	$14, $0, $69
 # 450		u := p^.free_list;
	lw	$10, 8($7)
 # 451		tsize := tsize+rsize+ssize;
	addu	$15, $4, $5
	addu	$4, $15, $11
 # 452		if (tsize >= min_fragment) and (r^.prev_size < min_fragment) then
	blt	$4, 256, $68
	bge	$9, 256, $68
 # 453		  begin
 # 454	/**/
 # 455		  x := u^.last_bcb;
	lw	$13, 8($10)
 # 456		  t^.next_bcb := u;
	sw	$10, 12($12)
 # 457		  t^.last_bcb := x;
	sw	$13, 8($12)
 # 458		  x^.next_bcb := t;
	sw	$12, 12($13)
 # 459		  u^.last_bcb := t;
	sw	$12, 8($10)
$68:
 # 460		end;
 # 461		t^.curr_size := tsize+prev_allocated;
	addu	$2, $4, 2
	sw	$2, 4($12)
 # 462		z := ptradd(bcbp, t, tsize);
	addu	$5, $12, $4
 # 463		z^.prev_size := tsize;
	sw	$4, 0($5)
 # 464		if ssize < min_fragment then return;
	blt	$11, 256, $75
 # 465		x := s^.next_bcb; 
	lw	$13, 12($8)
 # 466		y := s^.last_bcb;
	lw	$4, 8($8)
 # 467		y^.next_bcb := x; 
	sw	$13, 12($4)
 # 468		x^.last_bcb := y;
	sw	$4, 8($13)
 # 469		if u = s then p^.free_list := x;
	bne	$10, $8, $75
	sw	$13, 8($7)
 # 470		return;
	b	$75
$69:
 # 471	      end else begin			{ merge upper and middle blocks       }
 # 472		tsize := tsize+rsize;
	addu	$4, $4, $5
 # 473		if (tsize >= min_fragment) and (r^.prev_size < min_fragment) then
	blt	$4, 256, $70
	bge	$9, 256, $70
 # 474		  begin
 # 475		  u := p^.free_list;
	lw	$10, 8($7)
 # 476	/**/
 # 477		  x := u^.last_bcb;
	lw	$13, 8($10)
 # 478		  t^.next_bcb := u;
	sw	$10, 12($12)
 # 479		  t^.last_bcb := x;
	sw	$13, 8($12)
 # 480		  x^.next_bcb := t;
	sw	$12, 12($13)
 # 481		  u^.last_bcb := t;
	sw	$12, 8($10)
$70:
 # 482		end;
 # 483		t^.curr_size := tsize+prev_allocated;
	addu	$3, $4, 2
	sw	$3, 4($12)
 # 484		s^.curr_size := ssize+curr_allocated;
	addu	$24, $11, 1
	sw	$24, 4($8)
 # 485		s := ptradd(bcbp, t, tsize);
	addu	$8, $12, $4
 # 486		s^.prev_size := tsize;
	sw	$4, 0($8)
 # 487		return;
	b	$75
$71:
 # 488	      end;
 # 489	    end else begin
 # 490	      if not odd(msize) then begin	{ merge middle and lower blocks       }
	and	$25, $10, 1
	bne	$25, $0, $73
 # 491		rsize := rsize+ssize;
	addu	$5, $5, $11
 # 492		u := p^.free_list;
	lw	$10, 8($7)
 # 493		if (rsize >= min_fragment) then begin
	blt	$5, 256, $72
 # 494	/**/
 # 495		  x := u^.last_bcb;
	lw	$13, 8($10)
 # 496		  r^.next_bcb := u;
	sw	$10, 12($6)
 # 497		  r^.last_bcb := x;
	sw	$13, 8($6)
 # 498		  x^.next_bcb := r;
	sw	$6, 12($13)
 # 499		  u^.last_bcb := r;
	sw	$6, 8($10)
$72:
 # 500		end;
 # 501		t := ptradd(bcbp, r, rsize);
	addu	$12, $6, $5
 # 502		t^.prev_size := rsize;
	sw	$5, 0($12)
 # 503		r^.curr_size := rsize+prev_allocated;
	addu	$14, $5, 2
	sw	$14, 4($6)
 # 504		if ssize < min_fragment then return;
	blt	$11, 256, $75
 # 505		x := s^.next_bcb; 
	lw	$13, 12($8)
 # 506		y := s^.last_bcb;
	lw	$4, 8($8)
 # 507		y^.next_bcb := x; 
	sw	$13, 12($4)
 # 508		x^.last_bcb := y;
	sw	$4, 8($13)
 # 509		if u = s then p^.free_list := x;
	bne	$10, $8, $75
	sw	$13, 8($7)
 # 510		return;
	b	$75
$73:
 # 511	      end else begin			{ just put block onto chain	      }
 # 512		if (rsize >= min_fragment) then begin
	blt	$5, 256, $74
 # 513		  u := p^.free_list;
	lw	$10, 8($7)
 # 514	/**/
 # 515		  x := u^.last_bcb;
	lw	$13, 8($10)
 # 516		  r^.next_bcb := u;
	sw	$10, 12($6)
 # 517		  r^.last_bcb := x;
	sw	$13, 8($6)
 # 518		  x^.next_bcb := r;
	sw	$6, 12($13)
 # 519		  u^.last_bcb := r;
	sw	$6, 8($10)
$74:
 # 520		end;
 # 521		t := ptradd(bcbp, r, rsize);
	move	$12, $9
 # 522		t^.prev_size := rsize;
	sw	$5, 0($12)
 # 523		r^.curr_size := rsize+prev_allocated;
	addu	$15, $5, 2
	sw	$15, 4($6)
 # 524		s^.curr_size := ssize+curr_allocated;
	addu	$2, $11, 1
	sw	$2, 4($8)
 # 525		return;
	j	$31
$75:
	j	$31
	.end	alloc_dispose
.globl xmalloc
.ent xmalloc
.set noreorder
xmalloc:
	.frame	sp, 0, ra
	la	a1, malloc_scb
	/* fall through to alloc_new */
.end xmalloc
.globl alloc_new
.ent alloc_new
.set noreorder
alloc_new:
	.frame	sp, 0, $31
	lw	t0, 0(a1)	/* t0: scb */
	addu	a0, 4+7		/* add in 4 bytes overhead, 7 for roundup */
				/* to dw alignment */
	beq	t0, 0, l7	/* if no scb yet, got get one */
	 li	t5, -8		/* constant for masking off low 3 bits */
l1:
	lw	t1, free_list(t0) /* t1: 1st block on free list */
	and	a0, t5		/* complete dw alignment of size */
	lw	t2, curr_size(t1) /* t2: size of 1st block on free list */
	bge	a0, 16, l2	/* minimum allocated block size in 16 bytes */
	 nop
	li	a0, 16
l2:
	bge	t2, a0, l4	/* if 1st block is big enough, we're done */
	 and	t2, t5		/* mask out low 3 bits of size word */
	move	t3, t1
l3:	lw	t1, next_bcb(t1)  /* t1: next block */
	nop
	lw	t2, curr_size(t1) /* t2: size of block */
	beq	t1, t3, l6	/* if wrapped, then no block big enough */
	 slt	t4, t2, a0
	bne	t4, 0, l3	/* if not big enough, go try next */
	 and	t2, t5
l4:	/* Found a block big enough */
	addu	t3, t2, -256
	bge	a0, t3, l5
	/* Leftover is big enough to put back */
	addu	t3, t1, t2		/* t3: block after one found */
	lw	t4, curr_size(t3)
	sw	a0, prev_size(t3)
	or	t4, prev_allocated
	sw	t4, curr_size(t3)
	subu	t3, a0			/* t3: block we'll return */
	or	t4, a0, curr_allocated
	sw	t4, curr_size(t3)
	subu	t2, a0			/* t2: size of leftover */
	sw	t2, prev_size(t3)
	or	t2, prev_allocated
	sw	t2, curr_size(t1)
	j	ra
	 addu	v0, t3, 8
l5:	/* Leftover isn't big enough to put back */
	lw	t3, last_bcb(t1)
	lw	t4, next_bcb(t1)
	lw	t5, curr_size(t1)
	sw	t4, next_bcb(t3)
	sw	t3, last_bcb(t4)
	sw	t3, free_list(t0)
	or	t5, curr_allocated
	addu	t3, t1, t2
	lw	t4, curr_size(t3)
	sw	t5, curr_size(t1)
	or	t4, prev_allocated
	sw	t4, curr_size(t3)
	j	ra
	 addu	v0, t1, 8
l6:	/* No block big enough.  Get more space. */
	sw	a0, 4(sp)
	sw	t0, 8(sp)
	sw	ra, 12(sp)
	subu	a0, 4
	jal	alloc_next_scb
	 subu	sp, 16
	addu	sp, 16
	lw	t0, 8(sp)
	lw	ra, 12(sp)
	beqz	v0, l8
	 lw	a0, 4(sp)
	lw	t1, free_list(t0)
	li	t5, -8
	lw	t2, curr_size(t1)
	b	l4
	 and	t2, t5
	/* can't allocate space  */
l8:	j	ra
	 nop
l7:	/* Not yet initialized: do a mark. */
	sd	a0, 0(sp)	/* save a0, a1 */
	sw	ra, 12(sp)
	move	a0, a1
	jal	alloc_mark
	 subu	sp, 16		/* parameter area for alloc_mark */
	addu	sp, 16
	ld	a0, 0(sp)
	lw	ra, 12(sp)
	beqz	v0, l8
	 lw	t0, 0(a1)
	b	l1
	 li	t5, -8		/* constant for masking off low 3 bits */
	.end	alloc_new
