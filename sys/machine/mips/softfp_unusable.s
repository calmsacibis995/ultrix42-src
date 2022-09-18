/*	@(#)softfp_unusable.s	4.1      (ULTRIX)        7/2/90	*/
/* ------------------------------------------------------------------ */
/* | Copyright Unpublished, MIPS Computer Systems, Inc.  All Rights | */
/* | Reserved.  This software contains proprietary and confidential | */
/* | information of MIPS and its suppliers.  Use, disclosure or     | */
/* | reproduction is prohibited without the prior express written   | */
/* | consent of MIPS.                                               | */
/* ------------------------------------------------------------------ */

/*
 * softfp_unusable.s -- software floating point emulation of loads,
 * stores, move_to/from, and branches
 */
/* Revision History
 *
 * 14-Nov-89 -- gmm
 *	Bug fix - cp_fpowner was getting updated instead of cp_nofault
 *
 * 13-Oct-89 -- gmm
 *	smp changes. Access nofault through cpudata
 *
 */

#include "../machine/param.h"
#include "../machine/cpu.h"
#include "../machine/reg.h"
#include "../machine/regdef.h"
#include "../machine/asm.h"
#include "../machine/pcb.h"
#include "../machine/vmparam.h"
#include "../machine/softfp.h"
#include "../h/signal.h"
#include "assym.h"

	u	=	UADDR

/*
 * The software floating-point unusable handler is branched to when a
 * floating-point coprocessor unusable exception occurs and there is no
 * floating-point coprocessor.  The branch is from the coprocessor
 * unusable fault (VEC_cpfault).
 *
 * This is the state on entry to this code:
 *
 * Register setup before dispatching to the floating-point software (or an
 * exception handler)
 *	s0 -- SR register at time of exception
 *	a0 -- exception frame pointer
 *	a3 -- cause register
 *
 * The following registers have been saved into the exception frame
 *	a0,a1,a2,a3,s0,k0,k1,gp,sp,ra,sr,cause,epc
 *
 * Interrupts have been enabled.
 *
 * The pointer to the proc structure for the process that is executing is
 * at u+U_PROCP.
 *
 * The floating-point coprocessor revision word is in fptype_word and is zero
 * if there is no floating-point coprocessor.
 */

/*
 * This code exits by branching to exception_exit.
 *
 * The state that must be setup to branch to exception_exit is to have the
 * values of the user registers that are to be restored in either the register
 * or the exception frame location for that register if the value was saved
 * there on entry to this code.
 *
 * Interrupts must be disabled,
 *
 * and s0 must contain the sr at the time of the exception.
 */
LEAF(softfp_unusable)
	/*
	 * Fetching the instruction that caused the cp unusable exception
	 * (or the one in a branch delay slot of a non-fp branch instruction)
	 * can't cause any other exceptions because it was already fetched
	 * therefore nofault is not setup and the instruction is just loaded.
	 * (This is true because currently this code only emulates one instr
	 * plus the branch delay instr if required).
	 */
	lw	a2,EF_EPC*4(a0)	# get the EPC to load the instruction
	lw	a1,0(a2)	# load the instr that caused the exception

	/*
	 * A value in the pcb_bc_cause which has a branch delay bit set
	 * indicates to this code that emulation is being done in a branch
	 * delay slot.  It will get set manually if a branch on cp1 is
	 * emulated.
	 */
	sw	a3,u+PCB_BD_CAUSE

	/*
	 * The epc for the instruction being emulated is saved.  It is used
	 * along with the cause register (saved above) if a signal is to be
	 * posted.  It is also the used to advance the pc to the resulting pc
	 * after the needed instructions are emulated.  The resulting pc is
	 * placed in PCB_SOFTFP_PC and then put back in the exception frame
	 * [EF_EPC(ef)] if there are no errors before exiting.
	 */
	sw	a2,u+PCB_BD_EPC

	bgez	a3,softfp_instr	# this exception did not occured in a branch
				#  delay slot, branch delay bit (bit 31) was
				#  not set in the cause register (a3)
softfp_bd:
/*
 * The instruction that caused the cp unusable exception is now known to be
 * in a branch delay slot of a normal (non-floating-point) branch instruction.
 * In this case the normal branch instruction needs to be executed (or emulated)
 * then the float-point instruction in the delay slot of that branch needs to
 * be emulated.
 *
 * Register setup on entry to this point:
 *	a0 -- exception frame pointer
 *	a1 -- normal (non-floating-point) branch instruction
 *	a2 -- exception program counter
 *	a3 -- cause register
 */
	/*
	 * Call emulate_instr() to get the branch executed with the following
	 * interface:
	 * input parameters:
	 *	a0 -- the current exception frame
	 *	a1 -- the instruction to emulate
	 *	a2 -- the epc for the instruction to emulate (also in the pcb)
	 *	ra -- the return address
	 * input parameters in the process's pcb (valid on return)
	 *	PCB_BD_EPC -- the epc for the instruction to emulate
	 *	PCB_BD_CAUSE -- the cause register with the branch delay bit set
	 *			correctly with respect to the epc above
	 * return state:
	 *	a3 -- the resulting program counter after the emulated instr
	 *	a0 -- the new or existing exception frame
	 *      user's register values as modified by emulated instruction in
	 *	either the register or in the exception frame (as per the
	 *	locore.s interface)
	 */
	jal	emulate_instr		# call to get the branch emulated

	sw	a3,u+PCB_SOFTFP_PC	# save the resulting pc
	lw	a2,u+PCB_BD_EPC		# get the epc of the emulated branch
	addu	a2,4			# calculate address of the delay slot
	lw	a1,0(a2)		# load the fp instr in the delay slot

	# now fall through to emulate the fp instr in the delay slot

softfp_instr:
	/*
	 * See if this instruction is a branch on cp 1 condition instruction
	 *
	 * Register setup on entry to this point:
	 *	a0 -- exception frame pointer
	 *	a1 -- fp instruction
	 *	a2 -- program counter of fp instruction
	 */
	srl	a3,a1,OPCODE_SHIFT	# check the opcode field to see if
					#  it is a C1 (coprocessor 1 opcode)
	bne	a3,OPCODE_C1,softfp_ls	# if not then try loads and stores

softfp_instr_known:
	srl	a3,a1,COPN_BCSHIFT	# now check to see if this is a branch
	and	a3,COPN_BCMASK		#  on cp 1 cond instr
	bne	a3,COPN_BC,softfp_mtf	# if not then try other C1 instructions

softfp_cpbc:
/*
 * The instruction is now know to be a branch on cp 1 condition instruction.
 * If it is being emulation in a branch delay slot of another branch (which
 * is illegal) this branch is treated as a nop.  Otherwise if the branch
 * is to be taken then the instruction in the delay slot needs to be executed
 * (or emulated) before returning the user program to the target of the branch.
 * If the branch is not taken then the user program can just be returned to
 * the instruction in the branch delay slot.
 *
 * Register setup on entry to this point:
 *	a0 -- exception frame pointer
 *	a1 -- fp instruction
 *	a2 -- program counter of fp instruction
 */
	lw	s0,u+PCB_BD_CAUSE	# check to see if already in the delay
	bgez	s0,1f			#  slot, if so just exit

	lw	a2,u+PCB_SOFTFP_PC	# load the resulting pc
	sw	a2,EF_EPC*4(a0)		# set the epc to the resulting pc
	lw	s0,EF_SR*4(a0)		# load s0 with the SR at the time of 
					#  the exception.
	.set noreorder
	nop
	mtc0	s0,C0_SR		# replace the SR to disable interrupts
	nop
	.set reorder
	j	exception_exit		# branch to exit the exception code

1:
	or	s0,CAUSE_BD		# set the branch delay bit in cause and
	sw	s0,u+PCB_BD_CAUSE	#  store it to indicate the next instr
					#  emulated is in a branch delay slot
	
	lw	s0,u+PCB_FPC_CSR	# get fp control and status register
	srl	s0,CSR_CBITSHIFT	# move the condition bit to the low end
	and	s0,CSR_CBITMASK		# mask all but the condition bit

	srl	a3,a1,BC_TFBITSHIFT	# determine if this branch condition
	and	a3,BC_TFBITMASK		#  instruction is a branch true or false
	beq	a3,BC_FBIT,softfp_bf	# goto softfp_bf if branch on false

	# for branch on true condition determine if the branch is to be taken
softfp_bt:
	beq	s0,zero,not_taken
	b	taken

	# for branch on false condition determine it the branch is to be taken
softfp_bf:
	beq	s0,zero,taken
	# fall through to not_taken

not_taken:
	# set the resulting pc past the branch and the delay slot (epc+8)
	addu	s0,a2,8			# add 8 to the epc (a2)
	sw	s0,u+PCB_SOFTFP_PC	# save it as the resulting pc
	b	1f			# now go off to emulate the instr in the
					#  delay slot

taken:
	# set the resulting pc to the branch target
	sll	s0,a1,IMMED_SHIFT	# sign extend the offset in the instr
	sra	s0,IMMED_SHIFT-2
	addu	s0,a2			# add the epc (a2) to get the target
	addu	s0,4
	sw	s0,u+PCB_SOFTFP_PC	# save it as the resulting pc
	# now fall through to emulate the instr in the delay slot

1:
	# fetch the instruction in the delay slot
	addu	a2,4		# calculate address of the delay slot (epc+4)

#ifdef ASSERTIONS
	lw	ra,u+PCB_CPUPTR		# get cpudata pointer
	lw	ra,CPU_NOFAULT(ra)	
	beq	ra,zero,8f
	PANIC("recursive nofault")
8:
#endif ASSERTIONS
	.set	noreorder
	lw	a1,u+PCB_CPUPTR		# get cpudata pointer
	li	ra,NF_SOFTFP		# LDSLOT setup nofault in case of
	sw	ra,CPU_NOFAULT(a1)	# addressing errors
	lw	a1,0(a2)	# load the instr from the delay slot
	lw	ra,u+PCB_CPUPTR
	nop
	sw	zero,CPU_NOFAULT(ra)
	.set	reorder

	# check to see if this instruction is a floating-point cp instruction
	srl	a3,a1,OPCODE_SHIFT
	beq	a3,OPCODE_C1,softfp_instr_known	# it is, so emulate it

	# check to see if this a load/store CP1 instruction
	and	s0,a3,OP_LSWCOPNMASK
	beq	s0,OP_LSWCOPN,softfp_ls_known	# it is, so emulate it

	/*
	 * At this point it is not a CP1 instr so get it emulated via calling
	 * emulate_instr().  Note branch instructions emulated in this case
	 * get ignored and are treated as nops (the resulting pc from
	 * emulate_instr() is not used).
	 *
	 * Call emulate_instr() to get the instruction in the delay slot
	 * executed with the following interface:
	 * input parameters:
	 *	a0 -- the current exception frame
	 *	a1 -- the instruction to emulate
	 *	a2 -- the epc for the instruction to emulate (also in the pcb)
	 *	ra -- the return address
	 * input parameters in the process's pcb (valid on return)
	 *	PCB_BD_EPC -- the epc for the instruction to emulate
	 *	PCB_BD_CAUSE -- the cause register with the branch delay bit set
	 *			correctly with respect to the epc above
	 * return state:
	 *	a3 -- the resulting program counter after the emulated instr
	 *	a0 -- the new or existing exception frame
	 *      user's register values as modified by emulated instruction in
	 *	either the register or in the exception frame (as per the
	 *	locore.s interface)
	 */
	jal	emulate_instr

	lw	a2,u+PCB_SOFTFP_PC	# load the resulting pc
	sw	a2,EF_EPC*4(a0)		# set the epc to the resulting pc
	j	peek_ahead		# see if next instr is an fp instr

/*
 * At this point all the problems of branch delays have been handled and the
 * floating-point cp instruction can just be emulated.
 *
 * Register setup on entry to this point:
 *	a0 -- exception frame pointer
 *	a1 -- fp instruction
 *	a3 -- (fp instruction) >> OPCODE_SHIFT
 */
softfp_ls:
	# check to see if this a load/store CP1 instruction
	and	s0,a3,OP_LSWCOPNMASK
	bne	s0,OP_LSWCOPN,softfp_mtf

softfp_ls_known:
	/*
	 * Do the address calculation for the load or store instruction.
	 * Get the value of the base register (either from the register
	 * or it's location in the exception frame).  This code is very
	 * dependent on what gets save in the exception code.
	 */
	srl	s0,a1,BASE_SHIFT-2
	and	s0,BASE_MASK<<2
	lw	s0,base_tbl(s0)
	j	s0

base_tbl:
	.word	base_zero:1, base_AT:1, base_v0:1, base_v1:1, base_a0:1
	.word	base_a1:1, base_a2:1, base_a3:1, base_t0:1, base_t1:1
	.word	base_t2:1, base_t3:1, base_t4:1, base_t5:1, base_t6:1
	.word	base_t7:1, base_s0:1, base_s1:1, base_s2:1, base_s3:1
	.word	base_s4:1, base_s5:1, base_s6:1, base_s7:1, base_t8:1
	.word	base_t9:1, base_k0:1, base_k1:1, base_gp:1, base_sp:1
	.word	base_s8:1, base_ra:1

base_zero:
	move	s0,zero;	b	1f
base_AT:
	lw	s0,EF_AT*4(a0);	b	1f
base_v0:
	move	s0,v0;		b	1f
base_v1:
	move	s0,v1;		b	1f
base_a0:
	lw	s0,EF_A0*4(a0);	b	1f
base_a1:
	lw	s0,EF_A1*4(a0);	b	1f
base_a2:
	lw	s0,EF_A2*4(a0);	b	1f
base_a3:
	lw	s0,EF_A3*4(a0);	b	1f
base_t0:
	move	s0,t0;		b	1f
base_t1:
	move	s0,t1;		b	1f
base_t2:
	move	s0,t2;		b	1f
base_t3:
	move	s0,t3;		b	1f
base_t4:
	move	s0,t4;		b	1f
base_t5:
	move	s0,t5;		b	1f
base_t6:
	move	s0,t6;		b	1f
base_t7:
	move	s0,t7;		b	1f
base_s0:
	lw	s0,EF_S0*4(a0);	b	1f
base_s1:
	move	s0,s1;		b	1f
base_s2:
	move	s0,s2;		b	1f
base_s3:
	move	s0,s3;		b	1f
base_s4:
	move	s0,s4;		b	1f
base_s5:
	move	s0,s5;		b	1f
base_s6:
	move	s0,s6;		b	1f
base_s7:
	move	s0,s7;		b	1f
base_t8:
	move	s0,t8;		b	1f
base_t9:
	move	s0,t9;		b	1f
base_k0:
	move	s0,k0;		b	1f
base_k1:
	lw	s0,EF_K1*4(a0);	b	1f
base_gp:
	lw	s0,EF_GP*4(a0);	b	1f
base_sp:
	lw	s0,EF_SP*4(a0);	b	1f
base_s8:
	move	s0,s8;		b	1f
base_ra:
	lw	s0,EF_RA*4(a0)

	/*
	 * Now that the value of the base register is loaded sign extend the
	 * immediate value in the instruction and add it to the base register.
	 */
1:	sll	ra,a1,IMMED_SHIFT
	sra	ra,IMMED_SHIFT
	addu	s0,ra

	srl	a1,a1,RT_SHIFT-2	# get the coprocessor register specified
	and	a1,RT_MASK<<2		#  by the load or store times 4

	and	a3,OP_LSBITMASK		# check to see if this is load or store
	beq	a3,OP_LBIT,softfp_load	# it is a load instruction

softfp_store:
	bltz	s0,softfp_wadderr	# the k0 seg bit is set (address error)
	lw	a3,u+PCB_FPREGS(a1)	# get the cp reg value to store
#ifdef ASSERTIONS
	lw	ra,u+PCB_CPUPTR
	lw	ra,CPU_NOFAULT(ra)
	beq	ra,zero,8f
	PANIC("recursive nofault")
8:
#endif ASSERTIONS
	.set	noreorder
	lw	a2,u+PCB_CPUPTR
	li	ra,NF_SOFTFP		# LDSLOT setup nofault in case of other
	sw	ra,CPU_NOFAULT(a2)	# addressing errors
	sw	a3,0(s0)		# store the cp reg value
	sw	zero,CPU_NOFAULT(a2)
	.set	reorder

	# increment the pc past the store if not in a branch delay slot
	lw	a2,u+PCB_SOFTFP_PC	# load the resulting pc
	lw	s0,u+PCB_BD_CAUSE	# check to see if in a branch delay slot
	bltz	s0,1f			# if so skip incrementing the pc
	lw	a2,u+PCB_BD_EPC		# if not get the epc to increment it
	addu	a2,4			# increment the epc past the store instr
1:	sw	a2,EF_EPC*4(a0)		# set the epc to the resulting pc
	j	peek_ahead		# see if next instr is an fp instr

softfp_load:
	bltz	s0,softfp_radderr	# the k0 seg bit is set (address error)
#ifdef ASSERTIONS
	lw	ra,u+PCB_CPUPTR
	lw	ra,CPU_NOFAULT(ra)
	beq	ra,zero,8f
	PANIC("recursive nofault")
8:
#endif ASSERTIONS
	.set	noreorder
	lw	a2,u+PCB_CPUPTR
	li	ra,NF_SOFTFP		# LDSLOT setup nofault in case of other
	sw	ra,CPU_NOFAULT(a2)	# addressing errors
	lw	a3,0(s0)		# get the word to load into the cp reg
	sw	zero,CPU_NOFAULT(a2)	# clear nofault
	.set	reorder

	sw	a3,u+PCB_FPREGS(a1)	# load it into the cp reg
	# increment the pc past the load if not in a branch delay slot
	lw	a2,u+PCB_SOFTFP_PC	# load the resulting pc
	lw	s0,u+PCB_BD_CAUSE	# check to see if in a branch delay slot
	bltz	s0,1f			# if so skip incrementing the pc
	lw	a2,u+PCB_BD_EPC		# if not get the epc to increment it
	addu	a2,4			# increment the epc past the load instr
1:	sw	a2,EF_EPC*4(a0)		# set the epc to the resulting pc
	j	peek_ahead		# see if next instr is an fp instr

/*
 * Check to see if this is a move to or move from instruction.
 *
 * Register setup on entry to this point:
 *	a0 -- exception frame pointer
 *	a1 -- fp instruction
 */
softfp_mtf:
	srl	a3,a1,COPN_MTFSHIFT
	and	a3,COPN_MTFMASK
	bne	a3,COPN_MTF,softfp_op
	# it is now known to be a move to or move from instruction so check to
	# see which of the two it is
	srl	a3,a1,COPN_MTFBITSHIFT
	and	a3,COPN_MTFBITMASK
	beq	a3,COPN_MFBIT,softfp_movefrom

softfp_moveto:
	/*
	 * Get the value of the cpu register (either from the register
	 * or it's location in the exception frame).  This code is very
	 * dependent on what gets save in the exception code.
	 */
	srl	a3,a1,RT_SHIFT-2
	and	a3,RT_MASK<<2
	lw	a3,mt_tbl(a3)
	j	a3

mt_tbl:
	.word	mt_zero:1, mt_AT:1, mt_v0:1, mt_v1:1, mt_a0:1, mt_a1:1, mt_a2:1
	.word	mt_a3:1, mt_t0:1, mt_t1:1, mt_t2:1, mt_t3:1, mt_t4:1, mt_t5:1
	.word	mt_t6:1, mt_t7:1, mt_s0:1, mt_s1:1, mt_s2:1, mt_s3:1, mt_s4:1
	.word	mt_s5:1, mt_s6:1, mt_s7:1, mt_t8:1, mt_t9:1, mt_k0:1, mt_k1:1
	.word	mt_gp:1, mt_sp:1, mt_s8:1, mt_ra:1

mt_zero:
	move	s0,zero;	b	1f
mt_AT:
	lw	s0,EF_AT*4(a0);	b	1f
mt_v0:
	move	s0,v0;		b	1f
mt_v1:
	move	s0,v1;		b	1f
mt_a0:
	lw	s0,EF_A0*4(a0);	b	1f
mt_a1:
	lw	s0,EF_A1*4(a0);	b	1f
mt_a2:
	lw	s0,EF_A2*4(a0);	b	1f
mt_a3:
	lw	s0,EF_A3*4(a0);	b	1f
mt_t0:
	move	s0,t0;		b	1f
mt_t1:
	move	s0,t1;		b	1f
mt_t2:
	move	s0,t2;		b	1f
mt_t3:
	move	s0,t3;		b	1f
mt_t4:
	move	s0,t4;		b	1f
mt_t5:
	move	s0,t5;		b	1f
mt_t6:
	move	s0,t6;		b	1f
mt_t7:
	move	s0,t7;		b	1f
mt_s0:
	lw	s0,EF_S0*4(a0);	b	1f
mt_s1:
	move	s0,s1;		b	1f
mt_s2:
	move	s0,s2;		b	1f
mt_s3:
	move	s0,s3;		b	1f
mt_s4:
	move	s0,s4;		b	1f
mt_s5:
	move	s0,s5;		b	1f
mt_s6:
	move	s0,s6;		b	1f
mt_s7:
	move	s0,s7;		b	1f
mt_t8:
	move	s0,t8;		b	1f
mt_t9:
	move	s0,t9;		b	1f
mt_k0:
	move	s0,k0;		b	1f
mt_k1:
	lw	s0,EF_K1*4(a0);	b	1f
mt_gp:
	lw	s0,EF_GP*4(a0);	b	1f
mt_sp:
	lw	s0,EF_SP*4(a0);	b	1f
mt_s8:
	move	s0,s8;		b	1f
mt_ra:
	lw	s0,EF_RA*4(a0)

1:
	# Now move the value of the specified cpu register (now in s0) into the
	# specified cp register.

	# check to see if this is a move to control register instruction
	srl	a3,a1,M_CONBITSHIFT
	and	a3,M_CONBITMASK
	beq	a3,zero,softfp_norm_mt

	# Decode the RS field of the instruction to determine which control
	# register to move to.  Reserved register numbers do nothing.
	srl	a3,a1,RS_SHIFT
	and	a3,RS_MASK

	bne	a3,FPR_CSR,1f
	sw	s0,u+PCB_FPC_CSR
	/*
	 * Now check to see if the value moved into the csr would cause a
	 * SIGFPE because the unimplemented exception is set or an exception
	 * bit is set with a coresponding enable bit set.
	 */
	and	a3,s0,CSR_EXCEPT	# isolate the exception bits
	and	s0,CSR_ENABLE 		# isolate the enable bits 
	or	s0,(UNIMP_EXC >> 5)	# fake an enable for unimplemented
	sll	s0,5			# align both bit sets
	and	s0,a3			# check for coresponding bits
	beq	s0,zero,softfp_mt_done	# if not then done

	/*
	 * Setup and call psignal() to send a SIGFPE to the current process.
	 * u.u_pcb.pcb_resched is set to force entry into trap so the signal
	 * will be posted before returning to user mode.
	 */
	jal	tfi_save
	sw	gp,u+PCB_RESCHED	# force resched
	li	a1,SIGFPE
	lw	a0,u+U_PROCP
	jal	psignal

	jal	tfi_restore
	lw	s0,EF_SR*4(sp)	# load s0 with the SR at the time of 
				# the exception.
	.set noreorder
	nop
	mtc0	s0,C0_SR	# replace the SR to disable interrupts
	nop
	.set reorder
	j	exception_exit	# branch to exit the exception code
1:
	bne	a3,FPR_EIR,softfp_mt_done
	sw	s0,u+PCB_FPC_EIR
	b	softfp_mt_done

softfp_norm_mt:
	srl	a3,a1,RS_SHIFT-2
	and	a3,RS_MASK<<2
	sw	s0,u+PCB_FPREGS(a3)

softfp_mt_done:
	# increment the pc past the move to if not in a branch delay slot
	lw	a2,u+PCB_SOFTFP_PC	# load the resulting pc
	lw	s0,u+PCB_BD_CAUSE	# check to see if in a branch delay slot
	bltz	s0,1f			# if so skip incrementing the pc
	lw	a2,u+PCB_BD_EPC		# if not get the epc to increment it
	addu	a2,4			# increment the epc past the move to
1:	sw	a2,EF_EPC*4(a0)		# set the epc to the resulting pc
	j	peek_ahead		# see if next instr is an fp instr

softfp_movefrom:
	# check to see if this is a move from control register instruction
	srl	a3,a1,M_CONBITSHIFT
	and	a3,M_CONBITMASK
	beq	a3,zero,softfp_norm_mf

	# Decode the RS field of the instruction to determine which control
	# register to move from.  Reserved register numbers return zero values.
	srl	a3,a1,RS_SHIFT
	and	a3,RS_MASK

	bne	a3,FPR_CSR,1f
	lw	s0,u+PCB_FPC_CSR
	b	softfp_do_mf
1:
	bne	a3,FPR_EIR,2f
	lw	s0,u+PCB_FPC_EIR
	b	softfp_do_mf
2:
	bne	a3,FPR_REV,3f
	li	s0,SOFTFP_REVWORD
	b	softfp_do_mf
3:
	move	s0,zero
	b	softfp_do_mf

softfp_norm_mf:
	# for normal move from instructions get the fp register
	srl	a3,a1,RS_SHIFT-2
	and	a3,RS_MASK<<2
	lw	s0,u+PCB_FPREGS(a3)	# get the cp reg to move from
softfp_do_mf:
	/*
	 * Now store the value of the fp register (either in the register
	 * or it's location in the exception frame).  This code is very
	 * dependent on what gets save in the exception code.
	 */
	srl	a3,a1,RT_SHIFT-2
	and	a3,RT_MASK<<2
	lw	a3,mf_tbl(a3)
	j	a3

mf_tbl:
	.word	mf_zero:1, mf_AT:1, mf_v0:1, mf_v1:1, mf_a0:1, mf_a1:1, mf_a2:1
	.word	mf_a3:1, mf_t0:1, mf_t1:1, mf_t2:1, mf_t3:1, mf_t4:1, mf_t5:1
	.word	mf_t6:1, mf_t7:1, mf_s0:1, mf_s1:1, mf_s2:1, mf_s3:1, mf_s4:1
	.word	mf_s5:1, mf_s6:1, mf_s7:1, mf_t8:1, mf_t9:1, mf_k0:1, mf_k1:1
	.word	mf_gp:1, mf_sp:1, mf_s8:1, mf_ra:1

mf_zero:
	move	zero,s0;	b	3f
mf_AT:
	sw	s0,EF_AT*4(a0);	b	3f
mf_v0:
	move	v0,s0;		b	3f
mf_v1:
	move	v1,s0;		b	3f
mf_a0:
	sw	s0,EF_A0*4(a0);	b	3f
mf_a1:
	sw	s0,EF_A1*4(a0);	b	3f
mf_a2:
	sw	s0,EF_A2*4(a0);	b	3f
mf_a3:
	sw	s0,EF_A3*4(a0);	b	3f
mf_t0:
	move	t0,s0;		b	3f
mf_t1:
	move	t1,s0;		b	3f
mf_t2:
	move	t2,s0;		b	3f
mf_t3:
	move	t3,s0;		b	3f
mf_t4:
	move	t4,s0;		b	3f
mf_t5:
	move	t5,s0;		b	3f
mf_t6:
	move	t6,s0;		b	3f
mf_t7:
	move	t7,s0;		b	3f
mf_s0:
	sw	s0,EF_S0*4(a0);	b	3f
mf_s1:
	move	s1,s0;		b	3f
mf_s2:
	move	s2,s0;		b	3f
mf_s3:
	move	s3,s0;		b	3f
mf_s4:
	move	s4,s0;		b	3f
mf_s5:
	move	s5,s0;		b	3f
mf_s6:
	move	s6,s0;		b	3f
mf_s7:
	move	s7,s0;		b	3f
mf_t8:
	move	t8,s0;		b	3f
mf_t9:
	move	t9,s0;		b	3f
mf_k0:
mf_k1:
	b	3f
mf_gp:
	sw	s0,EF_GP*4(a0);	b	3f
mf_sp:
	sw	s0,EF_SP*4(a0);	b	3f
mf_s8:
	move	s8,s0;		b	3f
mf_ra:
	sw	s0,EF_RA*4(a0)
3:
	# increment the pc past the move from if not in a branch delay slot
	lw	a2,u+PCB_SOFTFP_PC	# load the resulting pc
	lw	s0,u+PCB_BD_CAUSE	# check to see if in a branch delay slot
	bltz	s0,1f			# if so skip incrementing the pc
	lw	a2,u+PCB_BD_EPC		# if not get the epc to increment it
	addu	a2,4			# increment the epc past the move from
1:	sw	a2,EF_EPC*4(a0)		# set the epc to the resulting pc
	j	peek_ahead		# see if next instr is an fp instr

/*
 * At this point the instruction must be a floating-point op (including compare)
 * or an illegal instruction.  The routine softfp() is called to emulate or
 * handle it.  The arguments to softfp() are:
 *	a0 -- exception frame pointer
 *	a1 -- fp instruction
 *	a2 -- fptype_word
 *
 * Register setup on entry to this point:
 *	a0 -- exception frame pointer
 *	a1 -- fp instruction
 */
softfp_op:
	jal	tfi_save
	lw	a2,fptype_word
	jal	softfp
	# softfp() takes care to not change a0 so softfp_exit: can use it
	beq	v0,zero,1f	# check for errors (v0 is non-zero)
	jal	tfi_restore	# if there were errors don't advance the pc
	lw	s0,EF_SR*4(a0)	# load s0 with the SR at the time of 
				# the exception.
	.set noreorder
	nop
	mtc0	s0,C0_SR	# replace the SR to disable interrupts
	nop
	.set reorder
	j	exception_exit	# branch to exit the exception code
1:
	jal	tfi_restore
	# increment the pc past the op if not in a branch delay slot
	lw	a2,u+PCB_SOFTFP_PC	# load the resulting pc
	lw	s0,u+PCB_BD_CAUSE	# check to see if in a branch delay slot
	bltz	s0,2f			# if so skip incrementing the pc
	lw	a2,u+PCB_BD_EPC		# if not get the epc to increment it
	addu	a2,4			# increment the epc past the op
2:	sw	a2,EF_EPC*4(a0)		# set the epc to the resulting pc
	j	peek_ahead		# see if next instr is an fp instr

softfp_radderr:
	li	a1,EXC_RADE
	lw	a3,u+PCB_CPUPTR		# a3 loaded in softfp_adderr. So should
					# be trashable now??
	sw	a1,CPU_NOFAULT_CAUSE(a3)
	sw	s0,CPU_NOFAULT_BADVADDR(a3)
	b	softfp_adderr

softfp_wadderr:
	li	a1,EXC_WADE
	lw	a3,u+PCB_CPUPTR		# a3 loaded in softfp_adderr. So should
					# be trashable now??
	sw	a1,CPU_NOFAULT_CAUSE(a3)
	sw	s0,CPU_NOFAULT_BADVADDR(a3)
	b	softfp_adderr

/*
 * All address errors during emulation come here.  The epc which is used
 * when posting the error is in PCB_BD_EPC.  The cause register which is used
 * is in PCB_BD_CAUSE.
 */
EXPORT(softfp_adderr)
	lw	a1,u+PCB_BD_EPC		# original epc
	lw	a3,u+PCB_BD_CAUSE	# original cause
	sw	a1,EF_EPC*4(a0)		# epc fault occurred at
	lw	a1,u+PCB_CPUPTR		
	lw	a1,CPU_NOFAULT_CAUSE(a1)
	and	a3,~CAUSE_EXCMASK	# zero exception type in original cause
	and	a1,CAUSE_EXCMASK	# isolate exception type in noflt cause
	or	a3,a1			# emulated cause
	sw	a3,EF_CAUSE*4(a0)
	lw	a2,u+PCB_CPUPTR
	lw	a2,CPU_NOFAULT_BADVADDR(a2)
	sw	a2,EF_BADVADDR*4(a0)
	lw	s0,EF_SR*4(a0)		# SR at the time of exception
	move	a2,s0
	j	VEC_trap		# VEC_trap(ef_ptr, code, sr, cause)

/*
 * peek_ahead is branched to after an instruction as been emulated without
 * any errors to look and see if the next instruction is also a floating-point
 * instruction.
 *
 * Register setup on entry to this point:
 *	a0 -- exception frame pointer
 *	a2 -- resulting epc (pc of next instruction)
 * Also the resulting pc has been stored into the exception frame
 * before getting to here.
 */
peek_ahead:
	lw	s0,u+PCB_RESCHED	# see if rescheduling is needed
	bnez	s0,softfp_exit		#  if so just exit

	/*
	 * Get the next instruction if any error occur just exit and let the
	 * normal fetch cause the error.
	 */
	bltz	a2,softfp_exit		# if the k0 seg bit is set just exit
#ifdef ASSERTIONS
	lw	ra,u+PCB_CPUPTR		
	lw	ra,CPU_NOFAULT(ra)
	beq	ra,zero,8f
	PANIC("recursive nofault")
8:
#endif ASSERTIONS
	.set	noreorder
	lw	ra,u+PCB_CPUPTR		
	li	a1,NF_SOFTFPI		# LDSLOT setup nofault in case of
	sw	a1,CPU_NOFAULT(ra)	# addressing errors
	lw	a1,0(a2)		# load the next instruction
	sw	zero,CPU_NOFAULT(ra)	# clear nofault
	.set	reorder

	/*
	 * Now check to see if this next instruction is a floating-point
	 * instruction.
	 */
	srl	a3,a1,OPCODE_SHIFT	# check the opcode field to see if
					#  it is a C1 (coprocessor 1 opcode)
	bne	a3,OPCODE_C1,1f		# if not then try loads and stores

	# set up to branch back to emulate this instruction
	lw	s0,EF_CAUSE*4(a0)	# get cause register
	and	s0,~CAUSE_BD		# clear branch delay bit
	sw	s0,u+PCB_BD_CAUSE	# store the cause register
	sw	a2,u+PCB_BD_EPC		# store the new "epc"
	b	softfp_instr_known	# emulate this instruction

1:	# check to see if this a load/store CP1 instruction
	and	s0,a3,OP_LSWCOPNMASK
	bne	s0,OP_LSWCOPN,2f

	# set up to branch back to emulate this instruction
	lw	s0,EF_CAUSE*4(a0)	# get cause register
	and	s0,~CAUSE_BD		# clear branch delay bit
	sw	s0,u+PCB_BD_CAUSE	# store the cause register
	sw	a2,u+PCB_BD_EPC		# store the new "epc"
	b	softfp_ls_known
2:

EXPORT(softfp_insterr)
softfp_exit:
	lw	s0,EF_SR*4(a0)		# load s0 with the SR at the time of 
					#  the exception.
	.set noreorder
	nop
	mtc0	s0,C0_SR		# replace the SR to disable interrupts
	nop
	.set reorder
	j	exception_exit		# branch to exit the exception code

	END(softfp_unusable)
