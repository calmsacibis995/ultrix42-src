/*	@(#)setjmp.s	4.1 (ULTRIX)	7/3/90	*/

/*
 * C library -- setjmp, longjmp
 *
 *	longjmp(a,v)
 * will generate a "return(v)" from
 * the last call to
 *	setjmp(a)
 * by restoring registers from the stack,
 * previous signal mask, and doing a return.
 *
 * BUG: always restores onsigstack state to 0
 */

#include "DEFS.h"

ENTRY(setjmp)
	pushl	$0
	calls	$1,_sigblock		# get signal mask
	movl	r0,r1
	movl	4(ap),r0
	movl	12(fp),(r0)		# save frame pointer of caller
	movl	16(fp),4(r0)		# save pc of caller
	movl	r1,8(r0)		# save signal mask
	clrl	12(r0)			# XXX (should be onsigstack) XXX
	bisl2	$1,16(r0)		# set flag to restore signal mask
	clrl	r0
	ret

ENTRY(sigsetjmp)
	pushl	$0
	calls	$1,_sigblock		# get signal mask
	movl	r0,r1
	movl	4(ap),r0
	movl	12(fp),(r0)		# save frame pointer of caller
	movl	16(fp),4(r0)		# save pc of caller
	movl	r1,8(r0)		# save signal mask
	clrl	12(r0)			# XXX (should be onsigstack) XXX
	bicl2	$1,16(r0)		# don't restore signal mask
	tstl	8(ap)			# is signal savemask set?
	beql	1f
	bisl2	$1,16(r0)		# flag to restore signal mask
1:
	clrl	r0
	ret

ENTRY(longjmp)
	movl	8(ap),r0		# return(v)
        tstl    r0			# if val = 0 make it be 1
        bneq    1f
        movzbl  $1,r0
1:
	movl	4(ap),r1		# fetch buffer
	tstl	(r1)
	beql	botch
loop:
	bitw	$1,6(fp)		# r0 saved?
	beql	1f
	movl	r0,20(fp)
	bitw	$2,6(fp)		# was r1 saved?
	beql	2f
	movl	r1,24(fp)
	brb	2f
1:
	bitw	$2,6(fp)		# was r1 saved?
	beql	2f
	movl	r1,20(fp)
2:
	cmpl	(r1),12(fp)
	beql	done
	blssu	botch
	movl	$loop,16(fp)
	ret				# pop another frame

done:
	cmpb	*16(fp),reiins		# returning to an "rei"?
	bneq	1f
	movab	3f,16(fp)		# do return w/ psl-pc pop
	brw	2f
1:
	movab	4f,16(fp)		# do standard return
2:
	ret				# unwind stack before signals enabled
3:
	addl2	$8,sp			# compensate for PSL-PC push
4:
	bitl	$1,16(r1)		# signal savemask
	beql	5f
	pushl	sp			# old stack pointer
	pushl	8(r1)			# old signal mask
	pushl	12(r1)			# old onsigstack
	pushl	sp			# pointer to sigcontext
	chmk	$139			# restore previous signal context
5:
	jmp	*4(r1)			# done, return....

botch:
	pushl	$14		# should be the next line but it generates
				#  a phase error
/*
	pushl	$msgend-msg
*/
	pushl	$msg
	pushl	$2
	calls	$3,_write
	halt

ENTRY(siglongjmp)
	movl	8(ap),r0		# return(v)
        tstl    r0			# if val = 0 make it be 1
        bneq    1f
        movzbl  $1,r0
1:
	movl	4(ap),r1		# fetch buffer
	tstl	(r1)
	beql	sbotch
sloop:
	bitw	$1,6(fp)		# r0 saved?
	beql	1f
	movl	r0,20(fp)
	bitw	$2,6(fp)		# was r1 saved?
	beql	2f
	movl	r1,24(fp)
	brb	2f
1:
	bitw	$2,6(fp)		# was r1 saved?
	beql	2f
	movl	r1,20(fp)
2:
	cmpl	(r1),12(fp)
	beql	sdone
	blssu	sbotch
	movl	$sloop,16(fp)
	ret				# pop another frame

sdone:
	cmpb	*16(fp),reiins		# returning to an "rei"?
	bneq	1f
	movab	3f,16(fp)		# do return w/ psl-pc pop
	brw	2f
1:
	movab	4f,16(fp)		# do standard return
2:
	ret				# unwind stack before signals enabled
3:
	addl2	$8,sp			# compensate for PSL-PC push
4:
	bitl	$1,16(r1)		# signal savemask
	beql	5f
	pushl	sp			# old stack pointer
	pushl	8(r1)			# old signal mask
	pushl	12(r1)			# old onsigstack
	pushl	sp			# pointer to sigcontext
	chmk	$139			# restore previous signal context
5:
	jmp	*4(r1)			# done, return....

sbotch:
	pushl	$smsgend-smsg
	pushl	$smsg
	pushl	$2
	calls	$3,_write
	halt

	.data
/* if the length of this string is changed, changed the reference */
msg:	.ascii	"longjmp botch\n"
msgend:

smsg:	.ascii	"siglongjmp botch\n"
smsgend:

reiins:	rei
