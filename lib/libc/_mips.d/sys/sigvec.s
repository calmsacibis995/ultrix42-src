/*	@(#)sigvec.s	4.1	(ULTRIX)	7/3/90				      */
/* ------------------------------------------------------------------ */
/* | Copyright Unpublished, MIPS Computer Systems, Inc.  All Rights | */
/* | Reserved.  This software contains proprietary and confidential | */
/* | information of MIPS and its suppliers.  Use, disclosure or     | */
/* | reproduction is prohibited without the prior express written   | */
/* | consent of MIPS.                                               | */
/* ------------------------------------------------------------------ */
/* $Header: sigvec.s,v 1.1 87/02/16 11:21:10 dce Exp $ */

/*
 * Copyright 1985 by MIPS Computer Systems, Inc.
 */

#include <mips/regdef.h>
#include <mips/asm.h>
#include <syscall.h>
#include <setjmp.h>

/*
 * struct sigvec {
 *	int (*sv_handler)();
 *	int sv_mask;
 *	int sv_onstack;
 * };
 *
 * sigvec(sig, vec, ovec);
 * int sig;
 * struct sigvec *vec, *ovec;
 *
 * NOTE:  Implement this by passing a hidden argument to the kernel which
 * point to the sigmanager.  sendsig then passes actual signal handler
 * to sigmanager as a parameter.
 */

LEAF(sigvec)
	la	a3,sigtramp		# hidden arg -- address of tramp code
	li	v0,SYS_sigvec
	syscall
	bne	a3,zero,err
	RET

err:
	j	_cerror

#define	SIG_ARG0	0
#define	SIG_ARG1	1
#define	SIG_ARG2	2
#define	SIG_ARG3	3
#define	SIG_SCTXTPTR	4

#define	SIGFRAME	(8*4)	/* should be quad word aligned */

/*
 * Sigtramp is called by the kernel as:
 * 	sigtramp(signal, code, sigcontext_ptr, sighandler)
 *
 * Sigtramp should build a frame appropriate to the language calling
 * conventions and then call the sighandler.  When the sighandler
 * returns, sigtramp does a sigcleanup system call passing the
 * address of the sigcontext struct.
 */
sigtramp:
	/*
	 * Save process state.
	 *
	 * NOTE: sp, v0, a0, a1, a2, and a3 are saved into sigcontext by
	 * by the kernel in sendsig, on a sigreturn the kernel copies the
	 * entire state indicated by the sigcontext into the exception
	 * frame and then returns to user mode via a special exit that
	 * restore the entire process state from the exception frame
	 * (unlike the normal syscall exit which assumes that the C
	 * calling sequence alleviates the necessity of preserving
	 * certain portions of the process state)
	 */
	.set	noat
	sw	AT,JB_AT*4(a2)
	.set	at
	sw	zero,JB_ZERO*4(a2)	# just in case someone looks
	sw	v1,JB_V1*4(a2)
	sw	t0,JB_T0*4(a2)
	sw	t1,JB_T1*4(a2)
	sw	t2,JB_T2*4(a2)
	sw	t3,JB_T3*4(a2)
	sw	t4,JB_T4*4(a2)
	sw	t5,JB_T5*4(a2)
	sw	t6,JB_T6*4(a2)
	sw	t7,JB_T7*4(a2)
	sw	s0,JB_S0*4(a2)
	sw	s1,JB_S1*4(a2)
	sw	s2,JB_S2*4(a2)
	sw	s3,JB_S3*4(a2)
	sw	s4,JB_S4*4(a2)
	sw	s5,JB_S5*4(a2)
	sw	s6,JB_S6*4(a2)
	sw	s7,JB_S7*4(a2)
	sw	s8,JB_S8*4(a2)
	sw	t8,JB_T8*4(a2)
	sw	t9,JB_T9*4(a2)
	sw	gp,JB_GP*4(a2)
	sw	ra,JB_RA*4(a2)
	mflo	t0
	mfhi	t1
	sw	t0,SC_MDLO*4(a2)
	sw	t1,SC_MDHI*4(a2)
	subu	sp,SIGFRAME
	sw	a2,SIG_SCTXTPTR*4(sp)	# save address of sigcontext
	jal	a3			# call signal handler
	lw	a0,SIG_SCTXTPTR*4(sp)	# sigreturn(&sigcontext)
	/*
	 * sigreturn will restore entire user state from sigcontext
	 * struct
	 */
	li	v0,SYS_sigreturn
	syscall
	.set	at
.end sigvec
