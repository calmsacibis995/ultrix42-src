/*	@(#)asm.h	4.2	(ULTRIX)	9/4/90 */

/* ------------------------------------------------------------------ */
/* | Copyright Unpublished, MIPS Computer Systems, Inc.  All Rights | */
/* | Reserved.  This software contains proprietary and confidential | */
/* | information of MIPS and its suppliers.  Use, disclosure or     | */
/* | reproduction is prohibited without the prior express written   | */
/* | consent of MIPS.                                               | */
/* ------------------------------------------------------------------ */
/*  asm.h 4.2 */

/*
 * Change History:
 *
 * 09-Nov-1989	bp
 *	changed SYSMAP macro so that mips pte go into bss instead of data
 *
 * 23-Feb-1989	Kong
 *	Changed the macro SYSMAP so that it correctly evaluates expression
 *	for the number of PTEs to allocate.
 */

/*
 * asm.h -- cpp definitions for assembler files
 */

/*
 * Notes on putting entry pt and frame info into symbol table for debuggers
 *
 *	.ent	name,lex-level	# name is entry pt, lex-level is 0 for c
 * name:			# actual entry point
 *	.frame	fp,framesize,saved_pc_reg
 *				# fp -- register which is pointer to base
 *				#	of previous frame, debuggers are special
 *				#	cased if "sp" to add "framesize"
 *				#	(sp is usually used)
 *				# framesize -- size of frame
 *				#	the expression:
 *				#		new_sp + framesize == old_sp
 *				#	should be true
 *				# saved_pc_reg -- either a register which
 *				#	contains callers pc or $0, if $0
 *				#	saved pc is assumed to be in
 *				#	(fp)+framesize-4
 *
 * Notes regarding multiple entry points: Use "LEAF" for the first (main)
 * entry point and "XLEAF" for alternate (additional) entry points. The
 * "XLEAF"s must be nested within a "LEAF" and a ".end".
 * LEAF(copyseg)		# declare main entry point
 * XLEAF(copypage)		# declare alternate entry point
 */
/*
 * LEAF -- declare leaf routine
 */
#ifdef KERNEL
#include "../h/ansi_compat.h"
#else
#include <ansi_compat.h>
#endif

#define	LEAF(x)						\
	.globl	x;					\
	.ent	x,0;					\
x:;							\
	.frame	sp,0,ra

/*
 * XLEAF -- declare alternate entry to leaf routine
 */
#define	XLEAF(x)					\
	.globl	x;					\
	.aent	x,0;					\
x:

/*
 * VECTOR -- declare exception routine entry
 */
#ifdef ASM_FIXED
#define	VECTOR(x, regmask)				\
	.globl	x;					\
	.ent	x,0;					\
x:;							\
	.frame	sp,EF_SIZE,$0;				\
	.mask	+(regmask)|M_EXCFRM,-(EF_SIZE-(EF_RA*4))
#else
#define	VECTOR(x, regmask)				\
	.globl	x;					\
	.ent	x,0;					\
x:;							\
	.frame	sp,EF_SIZE,$0;				\
	.mask	regmask,-(EF_SIZE-(EF_RA*4))
#endif

/*
 * NESTED -- declare nested routine entry point
 */
#define	NESTED(x, fsize, rpc)				\
	.globl	x;					\
	.ent	x,0;					\
x:;							\
	.frame	sp,fsize, rpc

/*
 * XNESTED -- declare alternate entry point to nested routine
 */
#define	XNESTED(x)					\
	.globl	x;					\
	.aent	x,0;					\
x:

/*
 * END -- mark end of procedure
 */
#define	END(proc)					\
	.end	proc

/*
 * IMPORT -- import external symbol
 */
#define	IMPORT(sym, size)				\
	.extern	sym,size

/*
 * ABS -- declare absolute symbol
 */
#define	ABS(x, y)					\
	.globl	x;					\
x	=	y

/*
 * EXPORT -- export definition of symbol
 */
#define	EXPORT(x)					\
	.globl	x;					\
x:

/*
 * BSS -- allocate space in bss
 */
#define	BSS(x,y)		\
	.comm	x,y

/*
 * LBSS -- allocate static space in bss
 */
#define	LBSS(x,y)		\
	.lcomm	x,y

/*
 * SYSCALL -- standard system call sequence
 * The kernel expects arguments to be passed with the normal C calling
 * sequence.  v0 should contain the system call number.  On return from
 * the kernel mode, a3 will be 0 to indicate no error and non-zero to
 * indicate an error; if an error occurred v0 will contain an errno.
 */
#define	SYSCALL(x)					\
LEAF(x);						\
	li	v0,SYS_/**/x;				\
	syscall;					\
	beq	a3,zero,9f;				\
	j	_cerror;				\
9:

/*
 * PSEUDO -- system call sequence for syscalls that are variations of other
 * system calls
 */
#define	PSEUDO(x,y)					\
LEAF(x);						\
	li	v0,SYS_/**/y;				\
	syscall

#define	CALL(y)						\
	jal	y

#define	RET						\
	j	ra

/*
 * The following macros reserve the usage of the local label '9'
 */
#define	PANIC(msg)					\
	sw	zero,waittime;				\
	la	a0,9f;					\
	jal	panic;					\
	MSG(msg)

#define	PRINTF(msg)					\
	la	a0,9f;					\
	jal	printf;					\
	MSG(msg)

#define	MSG(msg)					\
	.rdata;						\
9:	.asciiz	msg;					\
	.text

#define	PUTMSG(msg)					\
	la	a0, 9f;					\
	jal	putstr;					\
	.rdata;						\
9:	.asciiz	msg;					\
	.text


#define	SYSMAP(mname, vname, page, len)			\
	.globl	mname;					\
	.lcomm	mname,(len)*4;				\
	.data;						\
	.globl	vname;					\
vname	=	((page)*NBPG)

#if !defined(__MIPSEB) && !defined(__MIPSEL)
	.error Must define either MIPSEB or MIPSEL
#endif
#if defined(__MIPSEB) && defined(__MIPSEL)
	.error Only one of MIPSEB and MIPSEL may be defined
#endif

/*
 * register mask bit definitions
 */
#define	M_EXCFRM	0x00000001
#define	M_AT		0x00000002
#define	M_V0		0x00000004
#define	M_V1		0x00000008
#define	M_A0		0x00000010
#define	M_A1		0x00000020
#define	M_A2		0x00000040
#define	M_A3		0x00000080
#define	M_T0		0x00000100
#define	M_T1		0x00000200
#define	M_T2		0x00000400
#define	M_T3		0x00000800
#define	M_T4		0x00001000
#define	M_T5		0x00002000
#define	M_T6		0x00004000
#define	M_T7		0x00008000
#define	M_S0		0x00010000
#define	M_S1		0x00020000
#define	M_S2		0x00040000
#define	M_S3		0x00080000
#define	M_S4		0x00100000
#define	M_S5		0x00200000
#define	M_S6		0x00400000
#define	M_S7		0x00800000
#define	M_T8		0x01000000
#define	M_T9		0x02000000
#define	M_K0		0x04000000
#define	M_K1		0x08000000
#define	M_GP		0x10000000
#define	M_SP		0x20000000
#define	M_S8		0x40000000
#define	M_RA		0x80000000
