/*	@(#)fp_intr.s	4.2	(ULTRIX)	10/9/90	*/
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
 * fp_intr.s -- floating pointer interrupt handler
 */

/*
 * Modification history:
 *
 * 27-Sep-1990 jaw
 *	change to rescheduling signals to be more general
 *
 * 29-Mar-1990  gmm/woodward
 *	Change the way signal sent to a process. Instead of calling
 *	psignal(), schedule one at low ipl through softnet.
 *
 * 13-Oct-1989  gmm
 *	smp changes. Made fpowner a per cpu variable in cpudata.
 *      Removed the code to allow hardclock interrupt in fpuintr since
 *	it was interfering with lock strategy.
 *
 * 26-Jan-1989	Kong
 *	In routine fpuintr, changed the code that allows hardclock
 *	interrupt from hardwired mask to using the mask in "splm"
 */

#include "../machine/cpu.h"
#include "../machine/cpu_board.h"
#include "../machine/fpu.h"
#include "../machine/reg.h"
#include "../machine/regdef.h"
#include "../machine/asm.h"
#include "../machine/pcb.h"
#include "../machine/debug.h"
#include "assym.h"

#include "../machine/softfp.h"
#include "../h/signal.h"
#include "../h/proc.h"

IMPORT(splm,SPLMSIZE*4)		/* extern splm[SPLMSIZE] */


/*
 * Register setup before calling an interrupt handler (from intr() in 
 * mips/trap.c through the table of routines c0vec_tbl[]):
 *	a0 -- exception frame pointer
 *
 * This routine is called from a 'C' routine thus the registers are
 * handled accordingly.
 *
 * To get to here there must be some form of floating-point coprocessor
 * hardware to generate the interrupt.
 *
 * The pointer to the proc structure which currently owns the floating point
 * unit is in fpowner.  And the current values of the floating-point registers
 * are in the floating-point registers in the coprocessor.
 *
 * To exit this routine any modified value of a floating-point register
 * is just left in that register and then a return to the caller is done.
 */

/*
 * Floating-point coprocessor interrupt handler
 */
#define	FRAME_SIZE	24
#define	LOCAL_SIZE	8
#define	A0_OFFSET	FRAME_SIZE+4*0
#define	A1_OFFSET	FRAME_SIZE+4*1
#define	A2_OFFSET	FRAME_SIZE+4*2
#define	A3_OFFSET	FRAME_SIZE+4*3
#define	RA_OFFSET	FRAME_SIZE-LOCAL_SIZE+4*0
#define	V0_OFFSET	FRAME_SIZE-LOCAL_SIZE+4*1

NESTED(fpuintr, FRAME_SIZE, ra)
	subu	sp,FRAME_SIZE
	sw	ra,RA_OFFSET(sp)
	sw	a0,A0_OFFSET(sp)	# save exception frame pointer
	.mask	0x80000002, -LOCAL_SIZE

	lw	a3,u+PCB_CPUPTR		# get cpudata pointer
	lw	a3,CPU_FPOWNER(a3)	# current coproc 1 (fp) owner
	beq	a3,zero,strayfp_intr	# coproc 1 not currently owned

	/*
	 * If the p_fp is P_FP_SIGINTR1 then a SIGFPE is genorated on every
	 * floating-point interrupt before each instruction and is then set to
	 * P_FP_SIGINTR2.  If p_fp is P_FP_SIGINTR2 then no SIGFPE is genorated
	 * but p_fp is set to P_FP_SIGINTR1.
	 */
	lw	a1,P_FP(a3)
	beq	a1,zero,2f
	bne	a1,P_FP_SIGINTR1,1f
	# We are in state 1, so change to state2 and genorate a SIGFPE
	li	a1,P_FP_SIGINTR2
	sw	a1,P_FP(a3)
	b	12f
1:	# We are in state 2, so change to state1 and don't genorate a SIGFPE
 	li	a1,P_FP_SIGINTR1
	sw	a1,P_FP(a3)
2:

	.set noreorder
	nop
	mfc0	a1,C0_SR	# enable coproc 1 for the kernel
	nop
	or	a1,SR_CU1		
	nop
	mtc0	a1,C0_SR
	nop
	.set reorder

	lw	a3,EF_EPC*4(a0)		# load the epc into a3

	# Check the fp implementation so to know were to get the instruction
	# and then load it into register a1 where softfp() expects it.
	lw	a2,fptype_word
	bne	a2,IRR_IMP_R2360,2f

	# For board implementations the instruction is in the fpc_eir
	# floating-point control register.
	sw	a3,V0_OFFSET(sp)	# save the resulting pc (board case)

#ifdef XPRBUG
	lw	a0,xpr_flags
	and	a0,XPR_FPINTR
	beq	a0,zero,1f
	subu	sp,10*4
	la	a0,9f
	move	a1,a3
	cfc1	a2,fpc_eir
	cfc1	a3,fpc_csr
	jal	xprintf
	MSG("epc = 0x%x eir = 0x%x csr = 0x%x\n")
	addu	sp,10*4
	lw	a0,A0_OFFSET(sp)
	lw	a2,fptype_word
1:
#endif XPRBUG

	cfc1	a1,fpc_eir
	b	4f
2:
	# For chip implementations the floating-point instruction that caused
	# the interrupt is at the address of the epc as modified by the branch
	# delay bit in the cause register.

	lw	a1,0(a3)		# load the instr at the epc into a1
	lw	v0,EF_CAUSE*4(a0)	# load the cause register into v0
	bltz	v0,3f			# check the branch delay bit
	
	# This is not in a branch delay slot (branch delay bit not set) so
	# calculate the resulting pc (epc+4) into v0 and continue to softfp().
	addu	v0,a3,4
	sw	v0,V0_OFFSET(sp)	# save the resulting pc
	b	4f
3:
	# This is in a branch delay slot so the branch will have to be emulated
	# to get the resulting pc (done by calling emulate_branch() ).
	# The arguments to emulate_branch are:
	#	a0 -- ef (exception frame)
	#	a1 -- the branch instruction
	#	a2 -- the floating-point control and status register
	#
	cfc1	a2,fpc_csr		# get value of fpc_csr
	jal	emulate_branch		# emulate the branch
	sw	v0,V0_OFFSET(sp)	# save the resulting pc
	lw	a0,A0_OFFSET(sp)	# restore exception frame pointer
	lw	a2,fptype_word		# restore a2 with fptype_word value
	lw	a3,EF_EPC*4(a0)		# load the epc into a3

	# Now load the floating-point instruction in the branch delay slot
	# to be emulated by softfp().
	lw	a1,4(a3)
4:
	/*
	 * Check to see if the instruction to be emulated is a floating-point
	 * instruction.  If it is not then this interrupt must have been caused
	 * by writing to the fpc_csr a value which will caused the interrupt.
	 * It is possible however that when writing to the fpc_csr the
	 * instruction that is to be "emulated" when the interrupt is handled
	 * looks like a floating-point instruction and will incorrectly be
	 * emulated and a SIGFPE will not be sent.  This is the user's problem
	 * because he shouldn't write a value into the fpc_csr which should
	 * cause an interrupt.
	 */
	srl	a3,a1,OPCODE_SHIFT
	beq	a3,OPCODE_C1,10f

	/*
	 * Setup and call setsoftnet() to send a SIGFPE to the current process.
	 */
12:
	lw	a0,u+PCB_CPUPTR		# get cpudata pointer
	lw	a1,CPU_FPOWNER(a0)	# get current fpowner
	sw	a1,CPU_FPE_EVENT(a0)
	lw	a1,CPU_FPE_SENDSIG(a0)
	ori	a1,0x80			# send SIGFPE  (or in mask)
	sw	a1,CPU_FPE_SENDSIG(a0)

	jal	setsoftnet


	# We must clear the coprocessor interrupt without looseing fp
	# state, we do this by calling checkfp which will unload
	# the fp to the pcb and clear the fp csr.  A signal is
	# pending, sendsig will clear the csr in the pcb after
	# saving the fp state from the pcb into the sigcontext and
	# before calling the signal handler
	lw	a0,u+PCB_CPUPTR		# get cpudata pointer
	lw	a0,CPU_FPOWNER(a0)	# get current fpowner
	move	a1,zero
	jal	checkfp

	b	8f

10:
	# For now all instructions that cause an interrupt are just handed
	# off to softfp() to emulate it and come up with correct result.
	# The arguments to softfp() are:
	#	a0 -- ef (exception frame)
	#	a1 -- floating-point instruction
	#	a2 -- fptype_word
	#
	# What might have be done is for all exceptions for which the trapped
	# result is the same as the untrapped result is: turn off the enables,
	# re-excute the instruction, restore the enables and then post a SIGFPE.

	jal	softfp
	beq	v0,zero,5f	# no signal posted

	# We must clear the coprocessor interrupt without looseing fp
	# state, we do this by calling checkfp which will unload
	# the fp to the pcb and clear the fp csr.  A signal is
	# pending, sendsig will clear the csr in the pcb after
	# saving the fp state from the pcb into the sigcontext and
	# before calling the signal handler

	lw	a0,u+PCB_CPUPTR		# get cpudata pointer
	lw	a0,CPU_FPOWNER(a0)	# get current fpowner
	move	a1,zero
	jal	checkfp
	b	8f

5:
#ifdef XPRBUG
	lw	a0,xpr_flags
	and	a0,XPR_INTR
	beq	a0,zero,6f
	subu	sp,10*4
	la	a0,9f
	cfc1	a1,fpc_csr
	jal	xprintf
	MSG("exit fpuintr csr = 0x%x")
	addu	sp,10*4
6:
#endif XPRBUG
#ifdef ASSERTIONS
	/*
	 * If going back to user code without posting a signal there must
	 * not be any exceptions which could cause an interrupt.
	 */
	cfc1	a0,fpc_csr
	and	a1,a0,CSR_EXCEPT	# isolate the exception bits
	and	a0,CSR_ENABLE 		# isolate the enable bits 
	or	a0,(UNIMP_EXC >> 5)	# fake an enable for unimplemented
	sll	a0,5			# align both bit sets
	and	a0,a1			# check for coresponding bits
	beq	a0,zero,7f		# if not then ok
	PANIC("fpuintr csr exceptions")
7:
#endif ASSERTIONS

	# Enable fp and hardclock interrupts to allow time to be charged
	# to kernel instead of user. This is done since fpuintr is
	# higher priority than hardclock. Ok because kernel never does
	# floating point. Set it just below clock and fpu.

	# Removed the code to lower the priority to enable clock interrupt.
	# Other than the unknown side effects of lowering the ipl from an
	# interrupt routine, hardclock gets timeout lock which could give
	# trouble if run queue lock is already held from swtch(). run queue
	# lock is got at splclock(). This change means that at the next clock
	# tick, time will be chared to the user.


	# The instruction was emulated by softfp() without a signal being
	# posted so now change the epc to the target pc.  This is a nop if
	# this is the board because we stored the epc in V0_OFFSET when we
	# started.
	lw	a0,A0_OFFSET(sp)	# restore the exception frame pointer
	lw	v0,V0_OFFSET(sp)	# get the resulting pc
	sw	v0,EF_EPC*4(a0)		# store the resulting pc in the epc

8:
	lw	ra,RA_OFFSET(sp)
	addu	sp,FRAME_SIZE
	j	ra

/*
 * At this point there has been an interrupt from the floating-point
 * coprocessor but no process was using it.
 */
strayfp_intr:
	.set	noreorder
	mfc0	a2,C0_SR	# enable coproc 1 for the kernel
	or	a3,a2,SR_CU1		
	mtc0	a3,C0_SR
	nop			# before we can really use cp1
	nop			# before we can really use cp1
	ctc1	zero,fpc_csr	# clear interrupt
	mtc0	a2,C0_SR
	nop
	.set	reorder


	lw	a0,fpu_inited
	beq	a0,zero,7f		# if not inited then don't complain
	PRINTF("stray fp interrupt\012")
7:	lw	ra,RA_OFFSET(sp)
	addu	sp,FRAME_SIZE
	j	ra

	.end	fpuintr
