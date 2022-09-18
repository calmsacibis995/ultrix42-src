/*#@(#)coroutine.c	4.1	Ultrix	7/17/90*/
# $Header: coroutine.c,v 1.5 84/05/19 11:39:41 powell Exp $
# Modula-2 coroutine implementation subroutines
# This file is named .c to avoid accidental deletion

# A "process" is represented by an area defined as follows:
#   Note:  stack grows downward, so process variable points to end.
#	Name	Offset	function	
.set	status,	-4	# indicate whether process has started
.set	entry,	-8	# entry point for process
.set	size,	-12	# size of stack area in bytes
.set	savesp,	-16	# saved sp of process
.set	stack,	-40	# start of stack area for new process

.data
# currently running process, initially the main program
.globl	_SYSTEM_currentprocess
_SYSTEM_currentprocess:
	.long	_SYSTEM_currentprocessstate
	.long	0	# savesp = ?
	.long	0x7fffffff	# size = large
	.long	0	# entry = none
	.long	1	# status = running
_SYSTEM_currentprocessstate:
.text

.globl	_SYSTEM_newprocess
_SYSTEM_newprocess:
	.word	0
	movl	8(ap),r1	# stack area for process
	ashl	$-3,12(ap),12(ap)  # convert size from bits to bytes
	subl2	12(ap),r1	# point to end of stack area
	clrl	status(r1)	# indicate process not yet started
	movl	4(ap),entry(r1)	# entry point of process
	movl	12(ap),size(r1)	# size of stack
	movl	r1,*16(ap)	# use stack record as process object
	ret

.set	ALLREGMASK,16383	# mask to save r0-fp
.globl	_SYSTEM_transfer
_SYSTEM_transfer:
	.word	0
	pushr	$ALLREGMASK
	movl	_SYSTEM_currentprocess,r1	# get current process
	movl	*8(ap),r2	# process to switch to (var parameter!)
	movl	sp,savesp(r1)	# save stack pointer for old process
	movl	r1,*4(ap)	# return pointer to old process
	movl	r2,_SYSTEM_currentprocess	# new process to run

	tstl	status(r2)	# has new process been run before?
	jeql	firsttime	# no, start it

	# run new process
	movl	savesp(r2),sp	# restore stack pointer
	popr	$ALLREGMASK
	ret
firsttime:
	# start process at procedure
	movl	$1,status(r2)	# indicate running
	movl	entry(r2),r3	# get starting address
	movab	stack(r2),fp	# set frame pointer
	movl	fp,sp		#  and stack pointer
	calls	$0,(r3)		# enter procedure
#  will return here if procedure terminates.  terminate whole program
	clrl	r0
	.globl	_exit
	calls	$0,_exit
