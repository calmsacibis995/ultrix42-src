/*	@(#)setjmp.s	4.1	(ULTRIX)	7/3/90				      */
/* ------------------------------------------------------------------ */
/* | Copyright Unpublished, MIPS Computer Systems, Inc.  All Rights | */
/* | Reserved.  This software contains proprietary and confidential | */
/* | information of MIPS and its suppliers.  Use, disclosure or     | */
/* | reproduction is prohibited without the prior express written   | */
/* | consent of MIPS.                                               | */
/* ------------------------------------------------------------------ */
/* $Header: setjmp.s,v 1.2 87/04/05 15:12:49 dce Exp $ */

/*
 * Modification history
 *
 *	Peter A. Hack, 17-Oct-1989
 *	Added XPG3 sigsetjmp() and associated longjmp() support
 */

/*
 * Copyright 1985 by MIPS Computer Systems, Inc.
 */

#include <mips/regdef.h>
#include <mips/asm.h>
#include <setjmp.h>
#include <syscall.h>
#include <mips/fpu.h>

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
 * NOTE: THIS MUST MATCH UP WITH SIGCONTEXT STRUCTURE, be sure constants
 * in setjmp.h and signal.h are consistent!
 * 
 * Whats happening here: setjmp assumes that all process state except
 * the callee saved registers and the gp has been preserved by the
 * C calling sequence; therefore, setjmp only saves the signal state
 * (sigmask and the signal flag), and the state that must be preserved
 * by the callee (callee saved regs, gp, sp, ra, callee save fp regs
 * and fpc_csr)  into a sigcontext struct.
 *
 * On a longjmp, the jmp_buf is verified to be consistent, the appropriate
 * return value is dropped into the sigcontext, and a sigreturn system
 * call is performed to restore the signal state and restore the
 * callee saved register that were saved in the sigcontext by setjmp.
 */

SETJMPFRM	=	32

NESTED(setjmp, SETJMPFRM, zero)
	subu	sp,SETJMPFRM
	sw	ra,SETJMPFRM-4(sp)
	sw	a0,SETJMPFRM-8(sp)	# save jmp_buf ptr
	move	a0,zero
	jal	sigblock		# find current sigmask
	lw	a0,SETJMPFRM-8(sp)
	sw	v0,+JB_SIGMASK*4(a0)
	lw	a1,+JB_FLAGS*4(a0)	# set savemask bit
	ori a1,01
	sw	a1,+JB_FLAGS*4(a0)
	move	a0,zero
	move	a1,v1
	addu	a1,+JB_ONSIGSTK*4
	#jal	sigstack
	lw	a0,SETJMPFRM-8(sp)
	lw	ra,SETJMPFRM-4(sp)
	addu	sp,SETJMPFRM
	bltz	v0,botch
	sw	ra,+JB_PC*4(a0)
	sw	gp,+JB_GP*4(a0)
	sw	sp,+JB_SP*4(a0)
	sw	s0,+JB_S0*4(a0)
	sw	s1,+JB_S1*4(a0)
	sw	s2,+JB_S2*4(a0)
	sw	s3,+JB_S3*4(a0)
	sw	s4,+JB_S4*4(a0)
	sw	s5,+JB_S5*4(a0)
	sw	s6,+JB_S6*4(a0)
	sw	s7,+JB_S7*4(a0)
	sw	s8,+JB_S8*4(a0)
	cfc1	v0,fpc_csr
	sw	v0,+JB_FPC_CSR*4(a0)
	s.d	$f20,+JB_F20*4(a0)
	s.d	$f22,+JB_F22*4(a0)
	s.d	$f24,+JB_F24*4(a0)
	s.d	$f26,+JB_F26*4(a0)
	s.d	$f28,+JB_F28*4(a0)
	s.d	$f30,+JB_F30*4(a0)
	li	v0,+JBMAGIC
	sw	v0,+JB_MAGIC*4(a0)
	move	v0,zero
	j	ra
.end setjmp

NESTED(sigsetjmp, SETJMPFRM, zero)
	subu	sp,SETJMPFRM
	sw	ra,SETJMPFRM-4(sp)
	sw	a0,SETJMPFRM-8(sp)	# save jmp_buf ptr
	beqz	a1,1f			# test savemask argument
	lw	a1,+JB_FLAGS*4(a0)	# set savemask bit
	ori a1, 1
	sw	a1,+JB_FLAGS*4(a0)
	move	a0,zero
	jal	sigblock		# find current sigmask
	j	2f
1:	lw      a1,+JB_FLAGS*4(a0)      # clear savemask bit
	andi	a1,0Xfffe
	sw      a1,+JB_FLAGS*4(a0)
2:	lw	v1,SETJMPFRM-8(sp)
	sw	v0,+JB_SIGMASK*4(v1)
	move	a0,zero
	move	a1,v1
	addu	a1,+JB_ONSIGSTK*4
	#jal	sigstack
	lw	a0,SETJMPFRM-8(sp)
	lw	ra,SETJMPFRM-4(sp)
	addu	sp,SETJMPFRM
	bltz	v0,botch
	sw	ra,+JB_PC*4(a0)
	sw	gp,+JB_GP*4(a0)
	sw	sp,+JB_SP*4(a0)
	sw	s0,+JB_S0*4(a0)
	sw	s1,+JB_S1*4(a0)
	sw	s2,+JB_S2*4(a0)
	sw	s3,+JB_S3*4(a0)
	sw	s4,+JB_S4*4(a0)
	sw	s5,+JB_S5*4(a0)
	sw	s6,+JB_S6*4(a0)
	sw	s7,+JB_S7*4(a0)
	sw	s8,+JB_S8*4(a0)
	cfc1	v0,fpc_csr
	sw	v0,+JB_FPC_CSR*4(a0)
	s.d	$f20,+JB_F20*4(a0)
	s.d	$f22,+JB_F22*4(a0)
	s.d	$f24,+JB_F24*4(a0)
	s.d	$f26,+JB_F26*4(a0)
	s.d	$f28,+JB_F28*4(a0)
	s.d	$f30,+JB_F30*4(a0)
	li	v0,+JBMAGIC
	sw	v0,+JB_MAGIC*4(a0)
	move	v0,zero
	j	ra
.end setjmp

/*
 * NOTE: SVID says that longjmp() should not cause 0 to be returned.
 * There are programs that depend on this, so don't change it.
 * Horse pucky; it makes no sense -- JLR.
 */

LEAF(longjmp)
	lw	s0,+JB_SP*4(a0)
	bgtu	sp,s0,botch		# jmp_buf no longer on stack
	lw	v0,+JB_MAGIC*4(a0)
	bne	v0,+JBMAGIC,botch	# protect the naive
	bne	a1,$0,1f		# skip for good values
	li	a1,1			# zero becomes 1
1:
	sw	a1,+JB_V0*4(a0)		# let sigreturn set v0
	/*
	 * sigreturn will restore signal state and all callee saved
	 * registers from sigcontext and return to next instruction
	 * after setjmp call, the C calling sequence will then restore
	 * the caller saved registers
	 */
	li	v0,SYS_sigreturn	# sigreturn(&sigcontext)
	syscall

botch:
	jal	longjmperror
	jal	abort
.end longjmp

/*
 * siglongjmp(jmp_buf, retval)
 */
LEAF(siglongjmp)
	lw	s0,+JB_SP*4(a0)
	bgtu	sp,s0,sigbotch		# jmp_buf no longer on stack
	lw	v0,+JB_MAGIC*4(a0)
	bne	v0,+JBMAGIC,sigbotch	# protect the naive
	lw	s0,+JB_FLAGS*4(a0)	# check signal savemask
	andi	s0,1
	beqz	s0,nosigmask
	bne	a1,$0,1f		# skip for good values
	li	a1,1			# zero becomes 1
1:
	sw	a1,+JB_V0*4(a0)		# let sigreturn set v0
	/*
	 * sigreturn will restore signal state and all callee saved
	 * registers from sigcontext and return to next instruction
	 * after setjmp call, the C calling sequence will then restore
	 * the caller saved registers
	 */
	li	v0,SYS_sigreturn	# sigreturn(&sigcontext)
	syscall

	/*
	 * NOTE: SVID says that 0 is not a valid return value. There are
	 * commands that depend on this, so don't change it.
	 * They're broken, then. -- JLR
	 */
nosigmask:
	lw	ra,4*JB_PC(a0)
	lw	sp,4*JB_SP(a0)
	lw	s0,4*JB_S0(a0)
	lw	s1,4*JB_S1(a0)
	lw	s2,4*JB_S2(a0)
	lw	s3,4*JB_S3(a0)
	lw	s4,4*JB_S4(a0)
	lw	s5,4*JB_S5(a0)
	lw	s6,4*JB_S6(a0)
	lw	s7,4*JB_S7(a0)
	lw	s8,4*JB_S8(a0)
	l.d	$f20,+JB_F20*4(a0)
	l.d	$f22,+JB_F22*4(a0)
	l.d	$f24,+JB_F24*4(a0)
	l.d	$f26,+JB_F26*4(a0)
	l.d	$f28,+JB_F28*4(a0)
	l.d	$f30,+JB_F30*4(a0)
	lw	v0,+JB_FPC_CSR*4(a0)
	ctc1	v0,fpc_csr
	move	v0,a1			/* return retval */
	bne	v0,$0,1f
	li	v0,1
1:
	j	ra
sigbotch:
	jal	longjmperror
	jal	abort
.end siglongjmp
