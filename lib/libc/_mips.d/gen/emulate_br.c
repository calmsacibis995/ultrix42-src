#ifndef lint
static char *sccsid = "@(#)emulate_br.c	4.1	(ULTRIX)	7/3/90";
#endif not lint
/* ------------------------------------------------------------------ */
/* | Copyright Unpublished, MIPS Computer Systems, Inc.  All Rights | */
/* | Reserved.  This software contains proprietary and confidential | */
/* | information of MIPS and its suppliers.  Use, disclosure or     | */
/* | reproduction is prohibited without the prior express written   | */
/* | consent of MIPS.                                               | */
/* ------------------------------------------------------------------ */
/* $Header: emulate_br.c,v 1.2 87/08/19 11:21:03 dce Exp $ */

#include "mips/inst.h"
#include "mips/fpu.h"
#include "signal.h"

/*
 * Masks and constants for the rs field of "coprocessor instructions" (25-21)
 * which are branch on coprocessor condition instructions.
 */
#define	COPz_BC_MASK	0x1a
#define COPz_BC		0x08

/*
 * Masks and constants for the rt field of "branch on coprocessor condition
 * instructions" (20-16).
 */
#define	COPz_BC_TF_MASK	0x01
#define	COPz_BC_TRUE	0x01
#define	COPz_BC_FALSE	0x00

#define	PC_JMP_MASK	0xf0000000

/*
 * emulate_branch is used by a signal handler to step over an instruction in
 * a branch delay slot.  It is passed a pointer to the signal context and
 * the branch instruction to emulate.  It emulates the branch instruction
 * by modifing the signal context.  The routine returns a zero value for
 * branches emulated and a non-zero value for things not emulated (because
 * it is not a branch or it is an instruction that this routine doesn't know
 * about).
 */
long
emulate_branch(scp, instr)
struct sigcontext *scp;
unsigned long instr;
{
    union mips_instruction cpu_instr;
    union fpc_csr fpc_csr;
    long condition;

	cpu_instr.word = instr;

	switch(cpu_instr.i_format.opcode){

	case spec_op:
	    switch(cpu_instr.r_format.func){
	    case jalr_op:
		/* r31 has already been updated by the hardware */
	    case jr_op:
		scp->sc_pc = scp->sc_regs[cpu_instr.r_format.rs];
		break;
	    default:
		return(1);
	    }
	break;

	case jal_op:
	    /* r31 has already been updated by the hardware */
	case j_op:
	    scp->sc_pc = ((scp->sc_pc + 4) & PC_JMP_MASK) |
			 (cpu_instr.j_format.target << 2);
	    break;

	case beq_op:
	    condition = scp->sc_regs[cpu_instr.r_format.rs] ==
			scp->sc_regs[cpu_instr.r_format.rt];
	    goto conditional;

	case bne_op:
	    condition = scp->sc_regs[cpu_instr.r_format.rs] !=
			scp->sc_regs[cpu_instr.r_format.rt];
	    goto conditional;

	case blez_op:
	    condition = scp->sc_regs[cpu_instr.r_format.rs] <= 0;
	    goto conditional;

	case bgtz_op:
	    condition = scp->sc_regs[cpu_instr.r_format.rs] > 0;
	    goto conditional;

	case bcond_op:
	    switch(cpu_instr.r_format.func){
	    case bltzal_op:
		/* r31 has already been updated by the hardware */
	    case bltz_op:
		condition = scp->sc_regs[cpu_instr.r_format.rs] < 0;
		goto conditional;

	    case bgezal_op:
		/* r31 has already been updated by the hardware */
	    case bgez_op:
		condition = scp->sc_regs[cpu_instr.r_format.rs] >= 0;
		goto conditional;
	    default:
		return(1);
	    }

#ifdef notdef
	case cop1_op:
	    if((cpu_instr.r_format.rs & COPz_BC_MASK) == COPz_BC){
		fpc_csr.fc_word = scp->sc_fpc_csr;
		if((cpu_instr.r_format.rt & COPz_BC_TF_MASK) == COPz_BC_TRUE)
		    condition = fpc_csr.fc_struct.condition;
		else
		    condition = !(fpc_csr.fc_struct.condition);
		goto conditional;
	    }
	    return(1);
#endif
case cop1_op:

	case cop2_op:
	case cop3_op:
	    if((cpu_instr.r_format.rs & COPz_BC_MASK) == COPz_BC){
		if((cpu_instr.r_format.rt & COPz_BC_TF_MASK) == COPz_BC_TRUE)
		    condition = execute_branch(instr);
		else
		    condition = !(execute_branch(instr));
		goto conditional;
	    }
	    return(1);

	default:
	    return(1);

	}
	return(0);

conditional:
	if(condition)
	    scp->sc_pc = scp->sc_pc + (cpu_instr.i_format.simmediate << 2) + 4;
	else
	    scp->sc_pc += 8;
	return(0);
}
