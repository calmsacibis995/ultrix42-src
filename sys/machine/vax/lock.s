/*	@(#)lock.s	4.1	(ULTRIX)	7/2/90	*/

/************************************************************************
 *									*
 *			Copyright (c) 1983,86,87 by			*
 *		Digital Equipment Corporation, Maynard, MA		*
 *			All rights reserved.				*
 *									*
 *   This software is furnished under a license and may be used and	*
 *   copied  only  in accordance with the terms of such license and	*
 *   with the  inclusion  of  the  above  copyright  notice.   This	*
 *   software  or  any	other copies thereof may not be provided or	*
 *   otherwise made available to any other person.  No title to and	*
 *   ownership of the software is hereby transferred.			*
 *									*
 *   This software is  derived	from  software	received  from	the	*
 *   University    of	California,   Berkeley,   and	from   Bell	*
 *   Laboratories.  Use, duplication, or disclosure is	subject  to	*
 *   restrictions  under  license  agreements  with  University  of	*
 *   California and with AT&T.						*
 *									*
 *   The information in this software is subject to change  without	*
 *   notice  and should not be construed as a commitment by Digital	*
 *   Equipment Corporation.						*
 *									*
 *   Digital assumes no responsibility for the use  or	reliability	*
 *   of its software on equipment which is not supplied by Digital.	*
 *									*
 ************************************************************************/

/************************************************************************
 *
 * Modification history: /sys/vax/locore.s
 *
 * 03-Mar-90 jaw
 *	primitive change to optimize mips.
 *
 * 09-Nov-89 jaw
 * 	allow spin locks to be compiled out.
 *
 * 20-Jul-89 jaw
 *	rearrange lock structure.
 *
 *************************************************************************/
#include "../machine/mtpr.h"

#include "../h/smp_lock.h"
.text

/*
 *	smp_lock(l,LK_RETRY)
 *
 *	Call parameters:
 *		R0	holds lock to obtain.
 *
 *	Registers used:
 *		R0	holds lock to obtain.
 *		R1	used to hold current per cpu data structure.
 *		R2 	temp space.
 *
 *	Returns:
 *		nothing.
 */
	.globl	_Smp_lock_retry
_Smp_lock_retry:
#ifndef SMP
        tstb	(r0)
        bneq    smplock_retry_cont
        movl    $LK_WON,r0
        rsb
smplock_retry_cont:
#endif
#ifdef SMP_DEBUG
	tstl	_smp_debug
	bneq	smplock_long_retry
#endif SMP_DEBUG
#ifdef SMP
	bbssi	$31,(r0),smplock_long_retry
#else
	bbss	$31,(r0),smplock_long_retry
#endif
/*
 *	add lock to lock chain.
 */
	mfpr	$ESP,r1			# get cpudata pointer
	movl	CPU_HLOCK(r1),r2	# save off old head of lock list.
	movl	r0,CPU_HLOCK(r1)	# put new lock at head.
	movl	r2,L_PLOCK(r0)		# attach old list not new lock.
	movl	(sp),L_PC(r0)		# save pc lock asserted at.
	incl	L_WON(r0)		# bump lock won count
	rsb

smplock_long_retry:
	pushl	$LK_RETRY
	pushl	r0
	calls	$3,_smp_lock_long  	# note pc pushed by jsb!!
	halt				#  we will never return because
					# of a magic ASM instruction at the
					# front of the smp_lock_long routine
					# that changes the return point.

/*
 *	smp_lock(l,LK_ONCE)
 *
 *	Call parameters:
 *		R0	holds lock to obtain.
 *
 *	Registers used:
 *		R0	holds lock to obtain.
 *		R1	used to hold current per cpu data structure.
 *		R2 	temp space.
 *
 *	Returns:
 *		R0 =	LK_WON 	if lock obtained.
 *			LK_LOST if lock not obtained.
 */
	.globl	_Smp_lock_once
_Smp_lock_once:
#ifndef SMP
        bitl    $0x1000000,(r0)
        beql    smplock_once_cont
        movl    $LK_WON,r0
        rsb
smplock_once_cont:
#endif
#ifdef SMP_DEBUG
	tstl	_smp_debug
	bneq	smplock_long_once
#endif SMP_DEBUG
#ifdef SMP
	bbssi	$31,(r0),smplock_once_fail
#else
	bbss	$31,(r0),smplock_once_fail
#endif
/*
 *	add lock to lock chain.
 */
	mfpr	$ESP,r1			# get cpudata pointer
	movl	CPU_HLOCK(r1),r2	# save off old head of lock list.
	movl	r0,CPU_HLOCK(r1)	# put new lock at head.
	movl	r2,L_PLOCK(r0)		# attach old list not new lock.
	incl	L_WON(r0)		# bump lock won count
	movl	(sp),L_PC(r0)		# save pc lock asserted at.
	movl	$LK_WON,r0		# Got the lock!
	rsb

smplock_once_fail:
	movl 	$LK_LOST,r0		# failed to get lock.
	rsb

smplock_long_once:
	pushl	$LK_ONCE
	pushl	r0
	calls	$3,_smp_lock_long  	# note pc pushed by jsb!!
	halt				#  we will never return because
					# of a magic ASM instruction at the
					# front of the smp_lock_long routine
					# that changes the return point.

	

#ifdef SMP	
/*
 *	smp_unlock(l)
 *
 *	Call parameters:
 *		R0	holds lock to release.
 *
 *	Registers used:
 *		R0	holds lock to release.
 *		R1	used to hold current per cpu data structure.
 *		R2 	temp space.
 *
 *	Returns:
 *		nothing.
 *
 *	Panics:
 * 		"smp_unlock: lock not held"  --- lock to release is not on
 *						 the processor lock list.
 *		"smp_unlock: no process woke" --- no process was woken on
 *						  a sleep lock with the wanted
 *						  field non-zero.
 */
	.globl	_Smp_unlock_short

_Smp_unlock_short:
		
#ifdef SMP_DEBUG
	tstl	_smp_debug
	bneq	smpunlock_long
#endif SMP_DEBUG
/*
 *	first the lock must be deleted from the lock list.
 */
	mfpr	$ESP,r1			# get per-cpu pointer.
	movl	CPU_HLOCK(r1),r2	# put head of list in r2.
	beql	smpunlock_notheld	# if list is zero then we don't
					# hold the lock.  Time to panic.

	cmpl	r2,r0			# check to see if lock to delete is
					# at head of list.
	bneq	smpunlock_test		# branch if not....
	movl	L_PLOCK(r0),CPU_HLOCK(r1) # delete lock at head of list.

smpunlock_release:
	clrl	L_PLOCK(r0)		# clear the locks next pointer.
	bbcci	$31,(r0),smpunlock_notheld	# release the lock.

	bitl	$LK_WAIT,3(r0)
	beql	smpunlock_done	

	tstw	L_WANTED(r0)		# test wanted...wake someone if
	bneq	smpunlock_wakeone	# non-zero.

smpunlock_done:
	rsb				# done.

smpunlock_test:
	cmpl	r0,L_PLOCK(r2)		# check next lock on list..
	bneq	smpunlock_next		# branch if not lock to delete.

	movl	L_PLOCK(r0),L_PLOCK(r2)	# got it... delete lock from list.
	brb	smpunlock_release	# join the normal flow above.

smpunlock_next:
	movl	L_PLOCK(r2),r2		# get next lock to check.
	bneq	smpunlock_test		# branch if more locks to check

smpunlock_notheld:			# get here if lock not in lock list. We
	pushal	smpunlock_msg		# are in trouble if this happens...
	calls 	$1,_panic

smpunlock_msg: .asciz "Smp_unlock: lock not held"

smpunlock_long:
	pushl	r0
	calls	$2,_smp_unlock_long  # note pc pushed by jsb!!
	halt				#  we will never return because
					# of a magic ASM instruction at the
					# front of the smp_unlock_long routine
					# that changes the return point.

smpunlock_wakeone:
	pushr 	$0xf00			# save of r8-r11 for scratch space.
	movl	r1,r11			# save of cpudata
	movl	$0,r10			# release lk_rq flag
	movl	r0,r8			# save lock 

	#
	#  Next do a smp_owner on lk_rq.
	#
	movl	CPU_HLOCK(r1),r2	# save off lock list.
	beql	smpunlock_wake_get_lk_rq # if NULL then we are not hold lk_rq.

smpunlock_checknext:
	cmpl	r2,$_lk_rq		# see lock is lk_rq.
	beql	smpunlock_have_lk_rq	# if so then don't need to get it.

	movl	L_PLOCK(r2),r2		# get the next lock
	bneq	smpunlock_checknext	# if NULL then we are not holding lk_rq

smpunlock_wake_get_lk_rq:
	mfpr	$IPL,r9			# save of old IPL
	mtpr	$0x18,$IPL		# set IPL to spl6.
	
	moval	_lk_rq,r0		
	jsb	_Smp_lock_retry		# smp_lock(&lk_rq,LK_RETRY)
	movl	$1,r10			# set release lk_rq flag. 

smpunlock_have_lk_rq:
	tstw	L_WANTED(r8)		# test l->l_wanted.
	beql	smpunlock_unlock_lk_rq	# if zero then no one to wake.

	decw	L_WANTED(r8)		# l->l_wanted--

	pushl	$WAKE_ONE		
	pushl	r8
	calls	$2,_wakeup_type		# wakeup_type(l,WAKE_ONE)

	tstl	r0
	beql	smpunlock_no_one_woke	# if no one woken then panic!

smpunlock_unlock_lk_rq:
	tstl	r10			# need to release lk_rq if gotton
	beql	smpunlock_exit		# above.

	moval _lk_rq,r0
	jsb	_Smp_unlock_short		# smp_unlock(&lk_rq)
	mtpr	r9,$IPL			# restore IPL.

smpunlock_exit:
	popr	$0xf00			# restore saved r8-r11
	rsb

smpunlock_no_one_woke:
	pushal	smpunlock_msg1
	calls	$1,_panic
smpunlock_msg1:
	.asciz "Smp_unlock: no process woken"


#else SMP
/*
 *	smp_unlock(l)
 *
 *	Call parameters:
 *		R0	holds lock to release.
 *
 *	Registers used:
 *		R0	holds lock to release.
 *		R1	used to hold current per cpu data structure.
 *		R2 	temp space.
 *
 *	Returns:
 *		nothing.
 *
 *	Panics:
 * 		"smp_unlock: lock not held"  --- lock to release is not on
 *						 the processor lock list.
 *		"smp_unlock: no process woke" --- no process was woken on
 *						  a sleep lock with the wanted
 *						  field non-zero.
 */
	.globl	_Smp_unlock_short

_Smp_unlock_short:	
/*
 *	first the lock must be deleted from the lock list.
 */
	tstb	(r0)
	bneq	smpunlock_cont	
	rsb
smpunlock_cont:
	mfpr	$ESP,r1			# get per-cpu pointer.
	movl	CPU_HLOCK(r1),r2	# put head of list in r2.
	beql	smpunlock_notheld	# if list is zero then we don't
					# hold the lock.  Time to panic.

	cmpl	r2,r0			# check to see if lock to delete is
					# at head of list.
	bneq	smpunlock_test		# branch if not....
	movl	L_PLOCK(r0),CPU_HLOCK(r1) # delete lock at head of list.

smpunlock_release:
	clrl	L_PLOCK(r0)		# clear the locks next pointer.
	bbcc	$31,(r0),smpunlock_notheld
	tstw	L_WANTED(r0)		# test wanted...wake someone if
	bneq	smpunlock_wakeone	# non-zero.
	rsb				# done.

smpunlock_test:
	cmpl	r0,L_PLOCK(r2)		# check next lock on list..
	bneq	smpunlock_next		# branch if not lock to delete.

	movl	L_PLOCK(r0),L_PLOCK(r2)	# got it... delete lock from list.
	brb	smpunlock_release	# join the normal flow above.

smpunlock_next:
	movl	L_PLOCK(r2),r2		# get next lock to check.
	bneq	smpunlock_test		# branch if more locks to check

smpunlock_notheld:			# get here if lock not in lock list. We
	pushal	smpunlock_msg		# are in trouble if this happens...
	calls 	$1,_panic

smpunlock_msg: .asciz "Smp_unlock: lock not held"

smpunlock_long:
	pushl	r0
	calls	$2,_smp_unlock_long  # note pc pushed by jsb!!
	halt				#  we will never return because
					# of a magic ASM instruction at the
					# front of the smp_unlock_long routine
					# that changes the return point.

smpunlock_wakeone:
	decw	L_WANTED(r0)		# l->l_wanted--
	pushl	$WAKE_ONE		
	pushl	r0
	calls	$2,_wakeup_type		# wakeup_type(l,WAKE_ONE)
smpunlock_unlock_lk_rq:
	rsb



#endif SMP
