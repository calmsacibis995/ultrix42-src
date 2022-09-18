/*	@(#)process.s	4.1      (ULTRIX)        7/2/90	*/
/* ------------------------------------------------------------------ */
/* | Copyright Unpublished, MIPS Computer Systems, Inc.  All Rights | */
/* | Reserved.  This software contains proprietary and confidential | */
/* | information of MIPS and its suppliers.  Use, disclosure or     | */
/* | reproduction is prohibited without the prior express written   | */
/* | consent of MIPS.                                               | */
/* ------------------------------------------------------------------ */


/*
 *		Modification History
 *
 * 19-Oct-89 -- jmartin
 *	For R2000/R3000, shift PTE left 8 just before writing C0_TLBLO.
 *
 * 13-Oct-89 -- gmm
 *	Removed switch_to_idle_stack(). Now there is an idle process for
 *	every processor. Added the new routine map_to_idleproc().
 *
 * 08-Jun-89 -- gmm
 *	Added the new routine switch_to_idle_stack(), to be used when
 *	switching the stack from kernel to idle. See the comments above
 *	the routine on its usage.
 *
 */


#if NOMEMCACHE==1
#include "machine/pte.h"
#endif

#include "../machine/param.h"
#include "../machine/cpu.h"
#include "../machine/asm.h"
#include "../machine/pcb.h"
#include "../machine/regdef.h"
#include "../machine/vmparam.h"
#include "assym.h"

u	=	UADDR		# address of u area

/*
 * stackdepth()
 * return the number of bytes from the current stack pointer to the
 * top of the stack
 */
LEAF(stackdepth)
	li	v0,KERNELSTACK
	subu	v0,sp
	j	ra
	END(stackdepth)

/*
 * save()
 * save process context in u area pcb - return 0
 * NOTE: pcb must be first in u area struct!
 */
LEAF(save)
	/*
	 * Save C linkage registers
	 */
	sw	ra,u+PCB_PC*4
	sw	sp,u+PCB_SP*4
	/*
	 * ?????
	 * SHOULD SAVE FP EXCEPTION INST REGISTER like:
	 * if (fpowner)
	 *	if (cp1_present)
	 *		swc1	FP_EXCEPT,u_PCB_FPEXCEPT*4
	 */
	/*
	 * Save callee saved registers, all other live registers should have
	 * been saved on call to save() via C calling conventions
	 */
	sw	s0,u+PCB_S0*4
	sw	s1,u+PCB_S1*4
	sw	s2,u+PCB_S2*4
	sw	s3,u+PCB_S3*4
	sw	s4,u+PCB_S4*4
	sw	s5,u+PCB_S5*4
	sw	s6,u+PCB_S6*4
	sw	s7,u+PCB_S7*4
	sw	s8,u+PCB_S8*4
	.set noreorder
	nop
	mfc0	v0,C0_SR
	nop
	.set reorder
	sw	v0,u+PCB_SR*4
	move	v0,zero			# return 0
	j	ra
	END(save)

/*
 * resume(p)
 * restore state for process p
 * Set per process mappings in tlb,
 * then restore context from pcb
 * return 1
 * NOTE: pcb must be first in u area!
 */
LEAF(resume)
	/*
	 * resume(p) -- restore context of process p
	 * NOTE: resume should ONLY be called from swtch()!
	 *
	 * noreorder here, because I'm not sure that the assembler
	 * will realize that changing the TLBPID is dependent on
	 * setting the SR to disable interrupts.
	 */
	.set	noreorder
	mtc0	zero,C0_SR		# disable interrupts
	lw	t5,u+PCB_CPUPTR		# save the cpudata ptr. of the current
					# processor, before u page map changed
	lw	t0,P_TLBPID(a0)		# load new tlbpid
	lw	t1,P_ADDR(a0)		# load phys address of tlb mappings
	sll	t0,TLBHI_PIDSHIFT	# line up pid bits
	or	t4,t0,UVPN<<TLBHI_VPNSHIFT # or in u page vpn
	mtc0	t4,C0_TLBHI		# set pid and upage vpn
	lw	t3,0(t1)		# upage pte
	li	t2,(TLBWIREDBASE<<TLBINX_INXSHIFT) # initial wired entry index
#if NOMEMCACHE==1
	or	t3,PG_N
#endif
	sll	t3,8			# abstract format to R2000/R3000
	mtc0	t3,C0_TLBLO		# physical side for upage
	mtc0	t2,C0_INX		# set index to upage wired entry
	addu	t4,1<<TLBHI_VPNSHIFT	# bump vpn in TLBHI to k stack page
	c0	C0_WRITEI		# drop in upage mapping
	lw	t3,4(t1)		# load kstack pte
	mtc0	t4,C0_TLBHI		# kstack virtual into tlbhi
	li	t2,((TLBWIREDBASE+1)<<TLBINX_INXSHIFT) # kstack wired entry
#if NOMEMCACHE==1
	or	t3,PG_N
#endif
	sll	t3,8			# abstract format to R2000/R3000
	mtc0	t3,C0_TLBLO		# kstack physical into tlblo
	mtc0	t2,C0_INX		# set index to kstack wired entry
	move	t7,zero			# zero loop counter
	c0	C0_WRITEI		# drop in kstack mapping

	nop
	sw	t5,u+PCB_CPUPTR		# store the correct cpudata ptr
	lw	t1,P_TLBINFO(a0)	# 2nd level map cache
	li	t2,((TLBWIREDBASE+2)<<TLBINX_INXSHIFT)
	/*
	 * Loop, dropping all per process mappings into tlb wired entries
	 */
1:
	lw	t3,0(t1)		# load pte
	lw	t4,4(t1)		# load vpn
#if NOMEMCACHE==1
	or	t3,PG_N			# set nocache bit
#endif NOMEMCACHE
	sll	t3,8			# abstract format to R2000/R3000
	mtc0	t2,C0_INX		# set wired entry index
	or	t4,t0			# or in tlbpid with vpn
	mtc0	t3,C0_TLBLO		# set pfn and access bits
	mtc0	t4,C0_TLBHI		# set pid and vpn
	addu	t7,1			# bump loop counter
	c0	C0_WRITEI		# perform mapping
	addu	t2,(1<<TLBINX_INXSHIFT)	# bump wired entry index
	bne	t7,NPAGEMAP,1b		# loop test
	addu	t1,8			# bump per process mapping offset
	.set	reorder

	/*
	 * Reload callee saved registers, all other live registers
	 * should be reloaded from stack via C calling conventions.
	 */
	lw	a0,u+PCB_SSWAP		# check for non-local goto
	beq	a0,zero,1f		# return normally
	sw	zero,u+PCB_SSWAP
	j	longjmp			# TODO: is this really different?

1:	lw	ra,u+PCB_PC*4
	lw	sp,u+PCB_SP*4
	lw	s0,u+PCB_S0*4
	lw	s1,u+PCB_S1*4
	lw	s2,u+PCB_S2*4
	lw	s3,u+PCB_S3*4
	lw	s4,u+PCB_S4*4
	lw	s5,u+PCB_S5*4
	lw	s6,u+PCB_S6*4
	lw	s7,u+PCB_S7*4
	lw	s8,u+PCB_S8*4
2:	lw	v0,u+PCB_SR*4
	.set noreorder
	nop
	mtc0	v0,C0_SR
	nop
	.set reorder
	li	v0,1			# return non-zero
	j	ra
	END(resume)

/* map_to_idleproc(idleproc) changes the uarea mapping to that of the the
 * process passed as argument. This should be called only for the idle 
 * process */

LEAF(map_to_idleproc)
	.set	noreorder
	mfc0	t5,C0_SR		# save current SR
	mtc0	zero,C0_SR		# disable interrupts
	lw	t0,P_TLBPID(a0)		# load new tlbpid
	lw	t1,P_ADDR(a0)		# load phys address of tlb mappings
	sll	t0,TLBHI_PIDSHIFT	# line up pid bits
	or	t4,t0,UVPN<<TLBHI_VPNSHIFT # or in u page vpn
	mtc0	t4,C0_TLBHI		# set pid and upage vpn
	lw	t3,0(t1)		# upage pte
	li	t2,(TLBWIREDBASE<<TLBINX_INXSHIFT) # initial wired entry index
	sll	t3,8			# abstract format to R2000/R3000
	mtc0	t3,C0_TLBLO		# physical side for upage
	mtc0	t2,C0_INX		# set index to upage wired entry
	addu	t4,1<<TLBHI_VPNSHIFT	# bump vpn in TLBHI to k stack page
	c0	C0_WRITEI		# drop in upage mapping
	lw	t3,4(t1)		# load kstack pte
	mtc0	t4,C0_TLBHI		# kstack virtual into tlbhi
	li	t2,((TLBWIREDBASE+1)<<TLBINX_INXSHIFT) # kstack wired entry
	sll	t3,8			# abstract format to R2000/R3000
	mtc0	t3,C0_TLBLO		# kstack physical into tlblo
	mtc0	t2,C0_INX		# set index to kstack wired entry
	nop
	c0	C0_WRITEI		# drop in kstack mapping
	nop
	/* change 128 to STARTFRM as in entry.s */
	li	sp,KERNELSTACK-128	# change the sp to top of kernel stack
	mtc0	t5,C0_SR	# restore SR
	nop
	.set reorder
	j	ra		# go back to called routine
	END(map_to_idleproc)


/*
 * setjmp(jmp_buf) -- save current context for non-local goto's
 * return 0
 */
LEAF(setjmp)
	sw	ra,JB_PC*4(a0)
	sw	sp,JB_SP*4(a0)
	sw	s0,JB_S0*4(a0)
	sw	s1,JB_S1*4(a0)
	sw	s2,JB_S2*4(a0)
	sw	s3,JB_S3*4(a0)
	sw	s4,JB_S4*4(a0)
	sw	s5,JB_S5*4(a0)
	sw	s6,JB_S6*4(a0)
	sw	s7,JB_S7*4(a0)
	sw	s8,JB_S8*4(a0)
	.set noreorder
	nop
	mfc0	v0,C0_SR
	nop
	.set reorder
	sw	v0,JB_SR*4(a0)
	move	v0,zero
	j	ra
	END(setjmp)


/*
 * longjmp(jmp_buf)
 */
LEAF(longjmp)
	lw	ra,JB_PC*4(a0)
	lw	sp,JB_SP*4(a0)
	lw	s0,JB_S0*4(a0)
	lw	s1,JB_S1*4(a0)
	lw	s2,JB_S2*4(a0)
	lw	s3,JB_S3*4(a0)
	lw	s4,JB_S4*4(a0)
	lw	s5,JB_S5*4(a0)
	lw	s6,JB_S6*4(a0)
	lw	s7,JB_S7*4(a0)
	lw	s8,JB_S8*4(a0)
	lw	v0,JB_SR*4(a0)
	.set noreorder
	nop
	mtc0	v0,C0_SR
	nop
	.set reorder
	li	v0,1		/* return non-zero */
	j	ra
	END(longjmp)
