
/*	@(#)coproc_control.s	4.1      (ULTRIX)        7/2/90	*/
/*
 * Copyright 1985 by MIPS Computer Systems, Inc.
 */

/*
 * This file contains routines to get and set the coprocessor point control
 * registers.
 */

/* Revision History
 *
 * 13-Oct-89 -- gmm
 *	smp changes. Access nofault through cpudata 
 */
 
#include "../machine/regdef.h"
#include "../machine/fpu.h"
#include "../machine/cpu.h"
#include "../machine/asm.h"

#include "assym.h"

/*
 * get_fpc_csr returns the fpc_csr.
 */
LEAF(get_fpc_csr)
#ifndef SABLE
	.set	noreorder
	mfc0	v1,C0_SR
	li	a0,SR_CU1
	mtc0	a0,C0_SR
	nop				# before we can really use cp1
	nop				# before we can really use cp1
	cfc1	v0,fpc_csr
	mtc0	v1,C0_SR
	.set	reorder
#else SABLE
	move	v0,zero
#endif !SABLE
	j	ra
	END(get_fpc_csr)

/*
 * set_fpc_csr sets the fpc_csr and returns the old fpc_csr.
 */
LEAF(set_fpc_csr)
#ifndef SABLE
	.set	noreorder
	mfc0	v1,C0_SR
	li	a1,SR_CU1
	mtc0	a1,C0_SR
	nop				# before we can really use cp1
	nop				# before we can really use cp1
	cfc1	v0,fpc_csr
	ctc1	a0,fpc_csr
	mtc0	v1,C0_SR
	.set	reorder
#endif !SABLE
	j	ra
	END(set_fpc_csr)

/*
 * get_fpc_eir returns the fpc_eir.
 */
LEAF(get_fpc_eir)
#ifndef SABLE
	.set	noreorder
	mfc0	v1,C0_SR
	li	a0,SR_CU1
	mtc0	a0,C0_SR
	nop				# before we can really use cp1
	nop				# before we can really use cp1
	cfc1	v0,fpc_eir
	mtc0	v1,C0_SR
	.set	reorder
#else SABLE
	move	v0,zero
#endif !SABLE
	j	ra
	END(get_fpc_eir)

/*
 * get_cpu_irr -- returns cpu revision id word
 * returns zero for cpu chips without a PRID register
 */
LEAF(get_cpu_irr)
#ifndef SABLE
	.set noreorder
	nop
	mfc0	a1,C0_SR		# save sr
	nop
	mtc0	zero,C0_SR		# interrupts off
	nop
	.set reorder


#ifdef ASSERTIONS
	lw	a0,u+PCB_CPUPTR		# get cpudata pointer
	lw	a0,CPU_NOFAULT(a0)	
	beq	a0,zero,8f
	PANIC("recursive nofault")
8:
#endif ASSERTIONS
	lw	v0,u+PCB_CPUPTR		# get cpudata pointer
	li	a0,NF_REVID		# set-up nofault handling
	sw	a0,CPU_NOFAULT(v0)
	move	v0,zero			# return zero for chips w/o PRID reg
	.set	noreorder
	li	a0,SR_BEV		# chips w/o PRID don't have BEV
	mtc0	a0,C0_SR
	nop
	nop
	mfc0	a0,C0_SR
	nop
	and	a0,SR_BEV
	beq	a0,zero,1f		# no BEV, so no PRID
	nop
	mfc0	v0,C0_PRID
1:
	.set	reorder
	lw	a0,u+PCB_CPUPTR		# get cpudata pointer
	sw	zero,CPU_NOFAULT(a0)
	j	ra
#else SABLE
	move	v0,zero
	j	ra
#endif !SABLE
	END(get_cpu_irr)

/*
 * get_fpc_irr -- returns fp chip revision id
 * NOTE: should not be called if no fp chip is present
 */
LEAF(get_fpc_irr)
#ifndef SABLE
	.set	noreorder
	mfc0	a1,C0_SR		# save sr
	nop
	mtc0	zero,C0_SR
#ifdef ASSERTIONS
	lw	a0,u+PCB_CPUPTR		# get cpudata pointer
	nop
	lw	a0,CPU_NOFAULT(a0)	
	nop
	beq	a0,zero,8f
	nop
	PANIC("recursive nofault")
8:
#endif ASSERTIONS
	lw	v0,u+PCB_CPUPTR		# get cpudata pointer
	li	a0,NF_REVID		# LDSLOT
	sw	a0,CPU_NOFAULT(v0)	
	li	v0,SR_CU1
	mtc0	v0,C0_SR		# set fp usable
	nop				# before we can really use cp1
	nop				# before we can really use cp1
	cfc1	v0,fpc_irr		# get revision id
	lw	a0,u+PCB_CPUPTR		# get cpudata pointer
	nop
	sw	zero,CPU_NOFAULT(a0)	
	mtc0	a1,C0_SR
	j	ra
	nop
	.set	reorder
#else SABLE
	move	v0,zero
	j	ra
#endif !SABLE
	END(getfp_rid)

/*
 * set_fpc_led -- set floating point board leds
 */
LEAF(set_fpc_led)
#ifndef SABLE
	.set	noreorder
	mfc0	v1,C0_SR
	li	a1,SR_CU1
	mtc0	a1,C0_SR
	nop				# before we can really use cp1
	nop				# before we can really use cp1
	ctc1	a0,fpc_led		# set leds
	mtc0	v1,C0_SR
	.set	reorder
#endif !SABLE
	j	ra
	END(getfp_rid)

LEAF(reviderror)
	.set noreorder
	nop
	mtc0	a1,C0_SR		# restore sr
	nop
	.set reorder
	move	v0,zero
	j	ra
	END(reviderror)
