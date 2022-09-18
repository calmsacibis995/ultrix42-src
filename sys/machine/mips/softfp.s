/*	@(#)softfp.s	4.4		(ULTRIX)		2/12/91 */
/* ------------------------------------------------------------------ */
/* | Copyright Unpublished, MIPS Computer Systems, Inc.  All Rights | */
/* | Reserved.  This software contains proprietary and confidential | */
/* | information of MIPS and its suppliers.  Use, disclosure or     | */
/* | reproduction is prohibited without the prior express written   | */
/* | consent of MIPS.                                               | */
/* ------------------------------------------------------------------ */

/*
 * softfp.s -- floating point software emulation
 */

/* Revision History
 *
 *
 * 19-Dec-90 -- jaw
 *	sendsig change to mask...
 *
 * 29-Mar-90 -- gmm/jaw
 *	Call setsoftnet to schedule psignal() rather than calling psignal()
 *	directly. This is the result of changing splihigh() and spl6() to
 *	be same as splclock().
 *
 * 13-Oct-89 -- gmm
 *	Made fpowner a per cpu variable in cpudata
 *
 */
#ifdef MOXIE
# define LOCORE
# include "mips/cpu.h"
# include "mips/fpu.h"
# include "mips/reg.h"
# include "mips/regdef.h"
# include "mips/asm.h"
# include "mips/pcb.h"
# include "assym.h"
# include "h/signal.h"
# include "h/user.h"
# include "softfp.h"
#else
#include "../machine/param.h"
#include "../machine/cpu.h"
#include "../machine/fpu.h"
#include "../machine/reg.h"
#include "../machine/regdef.h"
#include "../machine/asm.h"
#include "../machine/pcb.h"
#include "../machine/vmparam.h"
#include "../machine/softfp.h"
#include "../h/signal.h"
#include "assym.h"
#endif MOXIE

/*
 * The software floating-point emulator is called from either the floating-
 * point coprocessor unusable exception handler (in softfp_unusable.s)
 * or the floating-point interrupt handler (in fp_intr.s).  This routine only
 * emulates floating-point operations and compares.  It does not emulate nor
 * does it detect loads/stores, move to/from and branch on condition
 * instructions.
 *
 * This is the state on entry to this routine:
 * Register setup 
 *	a0 -- exception frame pointer
 *	a1 -- fp instruction to be emulated
 *	a2 -- fptype_word
 * The normal calling convention is assumed with the appropriate registers
 * saved by the caller as if it were a high level language routine.
 *
 * The floating-point coprocessor revision word is in fptype_word and is zero
 * if there is no floating-point coprocessor.  If it is non-zero then this
 * routine was called from the floating-point interrupt handler.  In this
 * case the values of the floating point registers are still in the coprocessor
 * and the pointer to the proc structure for process which executed the fp
 * instruction is in fpowner.
 *
 * If fptype_word is zero then this routine was called from the coprocessor
 * unusable handler.  In this case the values of the floating point registers
 * are in the pcb for the current process and the pointer to the proc structure
 * for the current process is at u+U_PROCP.
 *
 * This routune returns a non-zero value in v0 if there was a signal posted
 * to the process as the result of an exception.  Otherwise v0 will be zero.
 */

#define	FRAME_SIZE	40
#define	LOCAL_SIZE	0
#define	A0_OFFSET	FRAME_SIZE+4*0
#define	A1_OFFSET	FRAME_SIZE+4*1
#define	A2_OFFSET	FRAME_SIZE+4*2
#define	A3_OFFSET	FRAME_SIZE+4*3
#define	T0_OFFSET	FRAME_SIZE-LOCAL_SIZE-4*1
#define	T1_OFFSET	FRAME_SIZE-LOCAL_SIZE-4*2
#define	T2_OFFSET	FRAME_SIZE-LOCAL_SIZE-4*3
#define	T3_OFFSET	FRAME_SIZE-LOCAL_SIZE-4*4
#define	T8_OFFSET	FRAME_SIZE-LOCAL_SIZE-4*5
#define	RA_OFFSET	FRAME_SIZE-LOCAL_SIZE-4*6

#ifndef MOXIE
	u	=	UADDR
#endif MOXIE

NESTED(softfp, FRAME_SIZE, ra)
	.mask	0x80000000, -(FRAME_SIZE - 4*4)
	subu	sp,FRAME_SIZE
	sw	ra,RA_OFFSET(sp)
/*
 * In decoding the instruction it is assumed that the opcode field is COP1
 * and that bit 25 is set  (since only floating-point ops are supposed to
 * be emulated in this routine) and therefore these are not checked.
 * The following fields are fully decoded and reserved encodings will result
 * an illegal instruction signal (SIGILL) being posted:
 *      FMT -- (bits 24-21)
 *     FUNC -- (bits 5-0)
 */
	/*
	 * Check the FMT field for reserved encodings and leave it right
	 * justified in register v0 times 4.
	 */
	srl	v0,a1,C1_FMT_SHIFT-2
	and	v0,C1_FMT_MASK<<2
	bgt	v0,C1_FMT_MAX<<2,illfpinst

	/*
	 * Load the floating point value from the register specified by the
	 * RS field into gp registers.  The gp registers which are used for
	 * the value specified by the RS feild are dependent on the FMT (v0)
	 * as follows:
	 * 	single		t2
	 *	double		t2,t3
	 *	extended	t2,t3,s2,s3	(where t3 is really zero)
	 *	quad		t2,t3,s2,s3
	 *
	 * Also load the value of the floating-point control and status
	 * register (fpc_csr) into gp register a3.
	 */
load_rs:
	srl	v1,a1,RS_SHIFT-2	# get the RS field times 4 right
	and	v1,RS_FPRMASK<<2	#  justified into v1 with the last bit
					#  of the field cleared.

	/*
	 * If fptype_word (a2) is non-zero then the floating-point values
	 * are loaded from the coprocessor else they are loaded from the pcb.
	 */
	beq	a2,zero,rs_pcb

	/*
	 * At this point the floating-point value for the specified FPR register
	 * in the RS field (v1)  will loaded from the coprocessor registers for
	 * the FMT specified (v0).  Also the floating-point contol and status
	 * register (fpc_csr) is loaded into gp register a3.
	 */
	cfc1	a3,fpc_csr
					# setup to branch to the code to load
	lw	t9,cp_rs_fmt_tab(v0)	#  the right number of words from the
	j	t9			#  cp for the specified format.

	.rdata
cp_rs_fmt_tab:
	.word	rs_cp_1w:1, rs_cp_2w:1, illfpinst:1, illfpinst:1, rs_cp_1w:1
	.text

/*
 * Load the one word from the coprocessor for the FPR register specified by
 * the RS (v1) field into GPR register t2.
 */
rs_cp_1w:
	srl	v1,1
	lw	v1,rs_cp_1w_tab(v1)
	j	v1

	.rdata
rs_cp_1w_tab:
	.word	rs_cp_1w_fpr0:1,  rs_cp_1w_fpr2:1,  rs_cp_1w_fpr4:1
	.word	rs_cp_1w_fpr6:1,  rs_cp_1w_fpr8:1,  rs_cp_1w_fpr10:1
	.word	rs_cp_1w_fpr12:1, rs_cp_1w_fpr14:1, rs_cp_1w_fpr16:1
	.word	rs_cp_1w_fpr18:1, rs_cp_1w_fpr20:1, rs_cp_1w_fpr22:1
	.word	rs_cp_1w_fpr24:1, rs_cp_1w_fpr26:1, rs_cp_1w_fpr28:1
	.word	rs_cp_1w_fpr30:1
	.text

rs_cp_1w_fpr0:
	mfc1	t2,$f0;		b	load_rs_done
rs_cp_1w_fpr2:
	mfc1	t2,$f2;		b	load_rs_done
rs_cp_1w_fpr4:
	mfc1	t2,$f4;		b	load_rs_done
rs_cp_1w_fpr6:
	mfc1	t2,$f6;		b	load_rs_done
rs_cp_1w_fpr8:
	mfc1	t2,$f8;		b	load_rs_done
rs_cp_1w_fpr10:
	mfc1	t2,$f10;	b	load_rs_done
rs_cp_1w_fpr12:
	mfc1	t2,$f12;	b	load_rs_done
rs_cp_1w_fpr14:
	mfc1	t2,$f14;	b	load_rs_done
rs_cp_1w_fpr16:
	mfc1	t2,$f16;	b	load_rs_done
rs_cp_1w_fpr18:
	mfc1	t2,$f18;	b	load_rs_done
rs_cp_1w_fpr20:
	mfc1	t2,$f20;	b	load_rs_done
rs_cp_1w_fpr22:
	mfc1	t2,$f22;	b	load_rs_done
rs_cp_1w_fpr24:
	mfc1	t2,$f24;	b	load_rs_done
rs_cp_1w_fpr26:
	mfc1	t2,$f26;	b	load_rs_done
rs_cp_1w_fpr28:
	mfc1	t2,$f28;	b	load_rs_done
rs_cp_1w_fpr30:
	mfc1	t2,$f30;	b	load_rs_done

/*
 * Load the two words from the coprocessor for the FPR register specified by
 * the RS (v1) field into GPR registers t2,t3.
 */
rs_cp_2w:
	srl	v1,1
	lw	v1,rs_cp_2w_tab(v1)
	j	v1

	.rdata
rs_cp_2w_tab:
	.word	rs_cp_2w_fpr0:1,  rs_cp_2w_fpr2:1,  rs_cp_2w_fpr4:1
	.word	rs_cp_2w_fpr6:1,  rs_cp_2w_fpr8:1,  rs_cp_2w_fpr10:1
	.word	rs_cp_2w_fpr12:1, rs_cp_2w_fpr14:1, rs_cp_2w_fpr16:1
	.word	rs_cp_2w_fpr18:1, rs_cp_2w_fpr20:1, rs_cp_2w_fpr22:1
	.word	rs_cp_2w_fpr24:1, rs_cp_2w_fpr26:1, rs_cp_2w_fpr28:1
	.word	rs_cp_2w_fpr30:1
	.text

rs_cp_2w_fpr0:
	mfc1	t3,$f0;		mfc1	t2,$f1;		b	load_rs_done
rs_cp_2w_fpr2:
	mfc1	t3,$f2;		mfc1	t2,$f3;		b	load_rs_done
rs_cp_2w_fpr4:
	mfc1	t3,$f4;		mfc1	t2,$f5;		b	load_rs_done
rs_cp_2w_fpr6:
	mfc1	t3,$f6;		mfc1	t2,$f7;		b	load_rs_done
rs_cp_2w_fpr8:
	mfc1	t3,$f8;		mfc1	t2,$f9;		b	load_rs_done
rs_cp_2w_fpr10:
	mfc1	t3,$f10;	mfc1	t2,$f11;	b	load_rs_done
rs_cp_2w_fpr12:
	mfc1	t3,$f12;	mfc1	t2,$f13;	b	load_rs_done
rs_cp_2w_fpr14:
	mfc1	t3,$f14;	mfc1	t2,$f15;	b	load_rs_done
rs_cp_2w_fpr16:
	mfc1	t3,$f16;	mfc1	t2,$f17;	b	load_rs_done
rs_cp_2w_fpr18:
	mfc1	t3,$f18;	mfc1	t2,$f19;	b	load_rs_done
rs_cp_2w_fpr20:
	mfc1	t3,$f20;	mfc1	t2,$f21;	b	load_rs_done
rs_cp_2w_fpr22:
	mfc1	t3,$f22;	mfc1	t2,$f23;	b	load_rs_done
rs_cp_2w_fpr24:
	mfc1	t3,$f24;	mfc1	t2,$f25;	b	load_rs_done
rs_cp_2w_fpr26:
	mfc1	t3,$f26;	mfc1	t2,$f27;	b	load_rs_done
rs_cp_2w_fpr28:
	mfc1	t3,$f28;	mfc1	t2,$f29;	b	load_rs_done
rs_cp_2w_fpr30:
	mfc1	t3,$f30;	mfc1	t2,$f31;	b	load_rs_done

/*
 * At this point the floating-point value for the specified FPR register
 * in the RS field (v1) will be loaded from the process control block (pcb)
 * of the current process for FMT specified (v0).  Also the floating-point
 * contol and status register is loaded into gp register a3.
 */
rs_pcb:
	lw	a3,u+PCB_FPC_CSR

	lw	t9,rs_pcb_fmt_tab(v0)
	j	t9

	.rdata
rs_pcb_fmt_tab:
	.word	rs_pcb_s:1, rs_pcb_d:1, illfpinst:1, illfpinst:1, rs_pcb_w:1
	.text

rs_pcb_s:
rs_pcb_w:
	lw	t2,u+PCB_FPREGS(v1)
	b	load_rs_done
rs_pcb_d:
	lw	t3,u+PCB_FPREGS(v1)
	lw	t2,u+PCB_FPREGS+4(v1)

/*
 * At this point the floating-point value for the specified FPR register
 * in the RS field has been loaded into GPR registers and the fpc_csr has
 * been loaded into the GPR register (a3).  First the exception field is
 * cleared in the fpc_csr.  What is done next is to decode the FUNC field.
 * If this is a dyadic operation then the floating-point value specified
 * by the FPR register in the RT field will be loaded into GPR registers
 * before the instruction is futher decoded.  If this is a monadic
 * instruction is decoded to be emulated.
 */
load_rs_done:
	and	a3,~CSR_EXCEPT

	and	t8,a1,C1_FUNC_MASK
	ble	t8,C1_FUNC_DIV,load_rt
	bge	t8,C1_FUNC_1stCMP,load_rt

	bgt	t8,C1_FUNC_CVTW,illfpinst
	bge	t8,C1_FUNC_CVTS,conv
	bgt	t8,C1_FUNC_NEG,illfpinst

	subu	t8,4
	sll	t8,2
	lw	t9,mon_func_tab(t8)
	j	t9

	.rdata
mon_func_tab:
	.word	func_sqrt:1, func_abs:1, func_mov:1, func_neg:1
	.text

func_sqrt:
	lw	v1,sqrt_fmt_tab(v0)
	j	v1

	.rdata
sqrt_fmt_tab:
	.word	sqrt_s:1, sqrt_d:1, sqrt_e:1, sqrt_q:1, illfpinst:1
	.text

/*
 * Square root single
 */
sqrt_s:
	/*
	 * Break out the operand into its fields (sign,exp,fraction) and
	 * handle a NaN operand by calling rs_breakout_s() .
	 */
	li	t9,C1_FMT_SINGLE*4
	move	v1,zero
	jal	rs_breakout_s

	# Check for sqrt of infinity, and produce the correct action if so
	bne	t1,SEXP_INF,4f	# is RS an infinity?
				# RS is an infinity
	beq	t0,zero,3f	# check for -infinity
	/*
	 * This is -infinity so this is an invalid operation for sqrt so set
	 * the invalid exception in the fpc_csr (a3) and setup the result
	 * depending if the enable for the invalid exception is set.
	 */
1:	or	a3,INVALID_EXC
	and	v0,a3,INVALID_ENABLE
	beq	v0,zero,2f
	/*
	 * The invalid trap was enabled so signal a SIGFPE and leave the
	 * result register unmodified.
	 */
	li	v0,SIGFPE
	jal	post_signal
	li	v0,1
	b	store_fpc_csr
	/*
	 * The invalid trap was NOT enabled so the result is a quiet NaN.
	 * So use the default quiet NaN and exit softfp().
	 */
2:	li	t2,SQUIETNAN_LEAST
	move	v0,zero
	b	rd_1w
	/*
	 * This is +infinity so the result is just +infinity.
	 */
3:	sll	t2,t1,SEXP_SHIFT
	move	v0,zero
	b	rd_1w
4:	# Check for the sqrt of zero and produce the correct action if so
	bne	t1,zero,5f	# check RS for a zero value (first the exp)
	bne	t2,zero,5f	# then the high part of the fraction
	# Now RS is known to be zero so just return it
	move	t2,t0		# get the sign of the zero
	move	v0,zero
	b	rd_1w
5:	# Check for sqrt of a negitive number if so it is an invalid
	bne	t0,zero,1b

	/*
	 * Now that all the NaN, infinity and zero and negitive cases have
	 * been taken care of what is left is a value that the sqrt can be
	 * taken.  So get the value into a format that can be used.  For
	 * normalized numbers set the implied one and remove the exponent
	 * bias.  For denormalized numbers convert to normalized numbers
	 * with the correct exponent.
	 */
	bne	t1,zero,1f	# check for RS being denormalized
	li	t1,-SEXP_BIAS+1	# set denorm's exponent
	jal	rs_renorm_s	# normalize it
	b	2f
1:	subu	t1,SEXP_BIAS	# if RS is not denormalized then remove the
	or	t2,SIMP_1BIT	#  exponent bias, and set the implied 1 bit
2:
	/*
	 * Now take the sqrt of the value.  Written by George Tayor.
	 *  t1		-- two's comp exponent
	 *  t2		-- 24-bit fraction
	 *  t8, t9	-- temps
	 *  v0		-- trial subtraction
	 *  t4		-- remainder
	 *  t6		-- 25-bit result
	 *  t8		-- sticky
	 */

	andi	t9, t1, 1		/*  last bit of unbiased exponent */
	sra	t1, 1			/*  divide exponent by 2 */
	addi	t1, -1			/*  subtract 1, deliver 25-bit result */
	beq	t9, zero, 1f 

	sll	t2, 1			/*  shift operand left by 1 */
					/*    if exponent was odd */

1:	li	t6, 1			/*  initialize answer msw */
	move	t4, zero		/*  initialize remainder msw */

	srl	t4, t2, 23		/*  shift operand left by 9 so that */
	sll	t2, 9			/*    2 bits go into remainder */

	li	t8, 25			/*  set cycle counter */

2:	subu	v0, t4, t6		/*  trial subtraction */

	sll	t6, 1			/*  shift answer left by 1 */
	li	t9, -4			/*  put 01 back in low order bits */
	and	t6, t9			/*    using 0xfffffffc mask */
	or	t6, 1

	bltz	v0, 3f			/*  branch on sign of trial subtract */
	ori	t6, 4			/*  set new bit of answer */
	sll	t4, v0, 2		/*  shift trial result left by 2 */
					/*    and put in remainder */
	b	4f

3:	sll	t4, 2			/*  shift remainder left by 2 */

4:	srl	t9, t2, 30		/*  shift operand left by 2 */
	or	t4, t9
	sll	t2, 2

	addi	t8, -1
	bne	t8, zero, 2b

	srl	t6, 2			/*  shift answer right by 2 */
					/*    to eliminate extra bits */

	move	t8, t4			/*  form sticky bit */
	move	t2, t6

	b	norm_s

/*
 * Square root double
 */
sqrt_d:
	/*
	 * Break out the operand into its fields (sign,exp,fraction) and
	 * handle a NaN operand by calling rs_breakout_d() .
	 */
	li	t9,C1_FMT_DOUBLE*4
	move	v1,zero
	jal	rs_breakout_d

	# Check for sqrt of infinity, and produce the correct action if so
	bne	t1,DEXP_INF,4f	# is RS an infinity?
				# RS is an infinity
	beq	t0,zero,3f	# check for -infinity
	/*
	 * This is -infinity so this is an invalid operation for sqrt so set
	 * the invalid exception in the fpc_csr (a3) and setup the result
	 * depending if the enable for the invalid exception is set.
	 */
1:	or	a3,INVALID_EXC
	and	v0,a3,INVALID_ENABLE
	beq	v0,zero,2f
	/*
	 * The invalid trap was enabled so signal a SIGFPE and leave the
	 * result register unmodified.
	 */
	li	v0,SIGFPE
	jal	post_signal
	li	v0,1
	b	store_fpc_csr
	/*
	 * The invalid trap was NOT enabled so the result is a quiet NaN.
	 * So use the default quiet NaN and exit softfp().
	 */
2:	li	t2,DQUIETNAN_LESS
	li	t3,DQUIETNAN_LEAST
	move	v0,zero
	b	rd_2w
	/*
	 * This is +infinity so the result is just +infinity.
	 */
3:	sll	t2,t1,DEXP_SHIFT
	move	v0,zero
	b	rd_2w
4:	# Check for the sqrt of zero and produce the correct action if so
	bne	t1,zero,5f	# check RS for a zero value (first the exp)
	bne	t2,zero,5f	# then the high part of the fraction
	bne	t3,zero,5f	# then the low part of the fraction
	# Now RS is known to be zero so just return it
	move	t2,t0		# get the sign of the zero
	move	v0,zero
	b	rd_2w
5:	# Check for sqrt of a negitive number if so it is an invalid
	bne	t0,zero,1b

	/*
	 * Now that all the NaN, infinity and zero and negitive cases have
	 * been taken care of what is left is a value that the sqrt can be
	 * taken.  So get the value into a format that can be used.  For
	 * normalized numbers set the implied one and remove the exponent
	 * bias.  For denormalized numbers convert to normalized numbers
	 * with the correct exponent.
	 */
	bne	t1,zero,1f	# check for RS being denormalized
	li	t1,-DEXP_BIAS+1	# set denorm's exponent
	jal	rs_renorm_d	# normalize it
	b	2f
1:	subu	t1,DEXP_BIAS	# if RS is not denormalized then remove the
	or	t2,DIMP_1BIT	#  exponent bias, and set the implied 1 bit
2:
	/*
	 * Now take the sqrt of the value.  Written by George Tayor.
	 *  t1		-- two's comp exponent
	 *  t2, t3	-- 53-bit fraction
	 *  t8, t9	-- temps
	 *  v0, v1	-- trial subtraction
	 *  t4, t5	-- remainder
	 *  t6, t7	-- 54-bit result
	 *  t8		-- sticky
	 */

	andi	t9, t1, 1		/*  last bit of unbiased exponent */
	sra	t1, 1			/*  divide exponent by 2 */
	addi	t1, -1			/*  subtract 1, deliver 54-bit result */
	beq	t9, zero, 1f 

	sll	t2, 1			/*  shift operand left by 1 */
	srl	t9, t3, 31 		/*    if exponent was odd */
	or	t2, t9
	sll	t3, 1

1:	move	t6, zero		/*  initialize answer msw */
	li	t7, 1			/*  initialize answer lsw */
	move	t4, zero		/*  initialize remainder msw */
	move	t5, zero		/*  initialize remainder lsw */

	srl	t5, t2, 20		/*  shift operand left by 12 so that */
	sll	t2, 12			/*    2 bits go into remainder */
	srl	t9, t3, 20 
	or	t2, t9
	sll	t3, 12

	li	t8, 54			/*  set cycle counter */

2:	sltu	t9, t5, t7		/*  trial subtraction */
	subu	v1, t5, t7
	subu	v0, t4, t6
	subu	v0, t9

	sll	t6, 1			/*  shift answer left by 1 */
	srl	t9, t7, 31
	or	t6, t9
	sll	t7, 1
	li	t9, -4			/*  put 01 back in low order bits */
	and	t7, t9			/*    using 0xfffffffc mask */
	or	t7, 1

	bltz	v0, 3f			/*  branch on sign of trial subtract */
	ori	t7, 4			/*  set new bit of answer */
	sll	t4, v0, 2		/*  shift trial result left by 2 */
	srl	t9, v1, 30		/*    and put in remainder */
	or	t4, t9
	sll	t5, v1, 2
	b	4f
3:
	sll	t4, 2			/*  shift remainder left by 2 */
	srl	t9, t5, 30
	or	t4, t9
	sll	t5, 2

4:	srl	t9, t2, 30		/*  shift operand left by 2 */
	or	t5, t9
	sll	t2, 2
	srl	t9, t3, 30
	or	t2, t9
	sll	t3, 2

	addi	t8, -1
	bne	t8, zero, 2b

	srl	t7, 2			/*  shift answer right by 2 */
	sll	t9, t6, 30		/*    to eliminate extra bits */
	or	t7, t9
	srl	t6, 2

	or	t8, t4, t5		/*  form sticky bit */
 	move	t2, t6
 	move	t3, t7

	b	norm_d

sqrt_e:
sqrt_q:
	b	illfpinst

func_abs:
	lw	v1,abs_fmt_tab(v0)
	j	v1

	.rdata
abs_fmt_tab:
	.word	abs_s:1, abs_d:1, abs_e:1, abs_q:1, illfpinst:1
	.text

/*
 * Absolute value single
 */
abs_s:
	move	t6,t2		# save the unmodified word
	/*
	 * Handle a NaN operand by calling rs_breakout_s().
	 * The broken out results are discarded.
	 */
	li	t9,C1_FMT_SINGLE*4
	jal	rs_breakout_s
	/*
	 * Now just clear the signbit after restoring the unmodified word.
	 */
	move	t2,t6
	and	t2,~SIGNBIT
	move	v0,zero
	b	rd_1w

/*
 * Absolute value double
 */
abs_d:
	move	t6,t2		# save the unmodified word
	/*
	 * Handle a NaN operand by calling rs_breakout_d().
	 * The broken out results are discarded.
	 */
	li	t9,C1_FMT_DOUBLE*4
	jal	rs_breakout_d
	/*
	 * Now just clear the signbit after restoring the unmodified word.
	 */
	move	t2,t6
	and	t2,~SIGNBIT
	move	v0,zero
	b	rd_2w

abs_e:
abs_q:
	b	illfpinst

func_mov:
	lw	v1,mov_fmt_tab(v0)
	j	v1

	.rdata
mov_fmt_tab:
	.word	mov_s:1, mov_d:1, mov_e:1, mov_q:1, illfpinst:1
	.text

/*
 * Move single
 */
mov_s:
	move	v0,zero
	b	rd_1w

/*
 * Move double
 */
mov_d:
	move	v0,zero
	b	rd_2w

mov_e:
mov_q:
	b	illfpinst

func_neg:
	lw	v1,neg_fmt_tab(v0)
	j	v1

	.rdata
neg_fmt_tab:
	.word	neg_s:1, neg_d:1, neg_e:1, neg_q:1, illfpinst:1
	.text

/*
 * Negation single
 */
neg_s:
	move	t6,t2		# save the unmodified word
	/*
	 * Handle a NaN operand by calling rs_breakout_s().
	 * The broken out results are discarded.
	 */
	li	t9,C1_FMT_SINGLE*4
	jal	rs_breakout_s
	/*
	 * Now just negate the operand after restoring the unmodified word.
	 */
	move	t2,t6
	xor	t2,SIGNBIT
	move	v0,zero
	b	rd_1w

/*
 * Negation double
 */
neg_d:
	move	t6,t2		# save the unmodified word
	/*
	 * Handle a NaN operand by calling rs_breakout_d().
	 * The broken out results are discarded.
	 */
	li	t9,C1_FMT_DOUBLE*4
	jal	rs_breakout_d
	/*
	 * Now just negate the operand after restoring the unmodified word.
	 */
	move	t2,t6
	xor	t2,SIGNBIT
	move	v0,zero
	b	rd_2w

neg_e:
neg_q:
	b	illfpinst

/*
 * Load the floating point value from the register specified by the
 * RT field into gp registers.  The gp registers which are used for
 * the value specified by the RT feild is dependent on its format
 * as follows:
 * 	single		t6
 *	double		t6,t7
 *	extended	t6,t7,s6,s7	(where t7 is really zero)
 *	quad		t6,t7,s6,s7
 */
load_rt:
	srl	v1,a1,RT_SHIFT-2	# get the RT field times 4 right
	and	v1,RT_FPRMASK<<2	#  justified into v1 with the last bit
					#  of the field cleared.

	/*
	 * If fptype_word (a2) is non-zero then the floating-point values
	 * are loaded from the coprocessor else they are loaded from the pcb.
	 */
	beq	a2,zero,rt_pcb

	/*
	 * At this point the floating-point value for the specified FPR register
	 * in the RT field will loaded from the coprocessor registers for the
	 * FMT specified (v0).
	 */
					# setup to branch to the code to load
	lw	t9,cp_rt_fmt_tab(v0)	#  the right number of words from the
	j	t9			#  cp for the specified format.

	.rdata
cp_rt_fmt_tab:
	.word	rt_cp_1w:1, rt_cp_2w:1, illfpinst:1, illfpinst:1, rt_cp_1w:1
	.text

/*
 * Load the one word from the coprocessor for the FPR register specified by
 * the RT (v1) field into GPR register t6.
 */
rt_cp_1w:
	srl	v1,1
	lw	v1,rt_cp_1w_tab(v1)
	j	v1

	.rdata
rt_cp_1w_tab:
	.word	rt_cp_1w_fpr0:1,  rt_cp_1w_fpr2:1,  rt_cp_1w_fpr4:1
	.word	rt_cp_1w_fpr6:1,  rt_cp_1w_fpr8:1,  rt_cp_1w_fpr10:1
	.word	rt_cp_1w_fpr12:1, rt_cp_1w_fpr14:1, rt_cp_1w_fpr16:1
	.word	rt_cp_1w_fpr18:1, rt_cp_1w_fpr20:1, rt_cp_1w_fpr22:1
	.word	rt_cp_1w_fpr24:1, rt_cp_1w_fpr26:1, rt_cp_1w_fpr28:1
	.word	rt_cp_1w_fpr30:1
	.text

rt_cp_1w_fpr0:
	mfc1	t6,$f0;		b	load_rt_done
rt_cp_1w_fpr2:
	mfc1	t6,$f2;		b	load_rt_done
rt_cp_1w_fpr4:
	mfc1	t6,$f4;		b	load_rt_done
rt_cp_1w_fpr6:
	mfc1	t6,$f6;		b	load_rt_done
rt_cp_1w_fpr8:
	mfc1	t6,$f8;		b	load_rt_done
rt_cp_1w_fpr10:
	mfc1	t6,$f10;	b	load_rt_done
rt_cp_1w_fpr12:
	mfc1	t6,$f12;	b	load_rt_done
rt_cp_1w_fpr14:
	mfc1	t6,$f14;	b	load_rt_done
rt_cp_1w_fpr16:
	mfc1	t6,$f16;	b	load_rt_done
rt_cp_1w_fpr18:
	mfc1	t6,$f18;	b	load_rt_done
rt_cp_1w_fpr20:
	mfc1	t6,$f20;	b	load_rt_done
rt_cp_1w_fpr22:
	mfc1	t6,$f22;	b	load_rt_done
rt_cp_1w_fpr24:
	mfc1	t6,$f24;	b	load_rt_done
rt_cp_1w_fpr26:
	mfc1	t6,$f26;	b	load_rt_done
rt_cp_1w_fpr28:
	mfc1	t6,$f28;	b	load_rt_done
rt_cp_1w_fpr30:
	mfc1	t6,$f30;	b	load_rt_done

/*
 * Load the two words from the coprocessor for the FPR register specified by
 * the RT (v1) field into GPR registers t6,t7.
 */
rt_cp_2w:
	srl	v1,1
	lw	v1,rt_cp_2w_tab(v1)
	j	v1

	.rdata
rt_cp_2w_tab:
	.word	rt_cp_2w_fpr0:1,  rt_cp_2w_fpr2:1,  rt_cp_2w_fpr4:1
	.word	rt_cp_2w_fpr6:1,  rt_cp_2w_fpr8:1,  rt_cp_2w_fpr10:1
	.word	rt_cp_2w_fpr12:1, rt_cp_2w_fpr14:1, rt_cp_2w_fpr16:1
	.word	rt_cp_2w_fpr18:1, rt_cp_2w_fpr20:1, rt_cp_2w_fpr22:1
	.word	rt_cp_2w_fpr24:1, rt_cp_2w_fpr26:1, rt_cp_2w_fpr28:1
	.word	rt_cp_2w_fpr30:1
	.text

rt_cp_2w_fpr0:
	mfc1	t7,$f0;		mfc1	t6,$f1;		b	load_rt_done
rt_cp_2w_fpr2:
	mfc1	t7,$f2;		mfc1	t6,$f3;		b	load_rt_done
rt_cp_2w_fpr4:
	mfc1	t7,$f4;		mfc1	t6,$f5;		b	load_rt_done
rt_cp_2w_fpr6:
	mfc1	t7,$f6;		mfc1	t6,$f7;		b	load_rt_done
rt_cp_2w_fpr8:
	mfc1	t7,$f8;		mfc1	t6,$f9;		b	load_rt_done
rt_cp_2w_fpr10:
	mfc1	t7,$f10;	mfc1	t6,$f11;	b	load_rt_done
rt_cp_2w_fpr12:
	mfc1	t7,$f12;	mfc1	t6,$f13;	b	load_rt_done
rt_cp_2w_fpr14:
	mfc1	t7,$f14;	mfc1	t6,$f15;	b	load_rt_done
rt_cp_2w_fpr16:
	mfc1	t7,$f16;	mfc1	t6,$f17;	b	load_rt_done
rt_cp_2w_fpr18:
	mfc1	t7,$f18;	mfc1	t6,$f19;	b	load_rt_done
rt_cp_2w_fpr20:
	mfc1	t7,$f20;	mfc1	t6,$f21;	b	load_rt_done
rt_cp_2w_fpr22:
	mfc1	t7,$f22;	mfc1	t6,$f23;	b	load_rt_done
rt_cp_2w_fpr24:
	mfc1	t7,$f24;	mfc1	t6,$f25;	b	load_rt_done
rt_cp_2w_fpr26:
	mfc1	t7,$f26;	mfc1	t6,$f27;	b	load_rt_done
rt_cp_2w_fpr28:
	mfc1	t7,$f28;	mfc1	t6,$f29;	b	load_rt_done
rt_cp_2w_fpr30:
	mfc1	t7,$f30;	mfc1	t6,$f31;	b	load_rt_done

/*
 * At this point the floating-point value for the specified FPR register
 * in the RT field (v1) will loaded from the process control block (pcb)
 * of the current process for FMT specified (v0).
 */
rt_pcb:
	lw	t9,rt_pcb_fmt_tab(v0)
	j	t9

	.rdata
rt_pcb_fmt_tab:
	.word	rt_pcb_s:1, rt_pcb_d:1, illfpinst:1, illfpinst:1, rt_pcb_w:1
	.text

rt_pcb_s:
rt_pcb_w:
	lw	t6,u+PCB_FPREGS(v1)
	b	load_rt_done
rt_pcb_d:
	lw	t7,u+PCB_FPREGS(v1)
	lw	t6,u+PCB_FPREGS+4(v1)

/*
 * At this point the both the floating-point value for the specified FPR
 * registers of the RS and RT fields have been loaded into GPR registers.
 * What is done next is to decode the FUNC field (t8) for the dyadic operations.
 */
load_rt_done:
	bge	t8,C1_FUNC_1stCMP,comp

	sll	t8,2
	lw	t9,dy_func_tab(t8)
	j	t9

	.rdata
dy_func_tab:
	.word	func_add:1, func_add:1, func_mul:1, func_div:1
	.text

/*
 * Both add and subtract functions come here.  The difference is that
 * the FUNC field (t8) is zero for adds.
 */
func_add:
	lw	v1,add_fmt_tab(v0)
	j	v1

	.rdata
add_fmt_tab:
	.word	add_s:1, add_d:1, add_e:1, add_q:1, illfpinst:1
	.text

/*
 * Add (and subtract) single RD = RS + RT (or RD = RS - RT).  Again the FUNC
 * field (t8) is zero for adds.
 */
.globl	add_s
add_s:
	/*
	 * Break out the operands into their fields (sign,exp,fraction) and
	 * handle NaN operands by calling {rs,rt}breakout_s() .
	 */
	li	t9,C1_FMT_SINGLE*4
	li	v1,1
	jal	rs_breakout_s
	jal	rt_breakout_s
	
	beq	t8,zero,1f	# if doing a subtract then negate RT
	lui	v0,SIGNBIT>>16
	xor	t4,v0
1:
	# Check for addition of infinities, and produce the correct action if so
	bne	t1,SEXP_INF,5f	# is RS an infinity?
				# RS is an infinity
	bne	t5,SEXP_INF,4f	# is RT also an infinity?
				# RT is an infinity
	beq	t0,t4,3f	# do the infinities have the same sign?

	/*
	 * The infinities do NOT have the same sign thus this is an invalid
	 * operation for addition so set the invalid exception in the fpc_csr
	 * (a3) and setup the result depending if the enable for the invalid
	 * exception is set.
	 */
	or	a3,INVALID_EXC
	and	v0,a3,INVALID_ENABLE
	beq	v0,zero,2f
	/*
	 * The invalid trap was enabled so signal a SIGFPE and leave the
	 * result register unmodified.
	 */
	li	v0,SIGFPE
	jal	post_signal
	li	v0,1
	b	store_fpc_csr
	/*
	 * The invalid trap was NOT enabled so the result is a quiet NaN.
	 * So use the default quiet NaN and exit softfp().
	 */
2:	li	t2,SQUIETNAN_LEAST
	move	v0,zero
	b	rd_1w

	/*
	 * This is just a normal infinity + infinity so the result is just
	 * an infinity with the sign of the operands.
	 */
3:	move	t2,t0
	sll	t1,SEXP_SHIFT
	or	t2,t1
	move	v0,zero
	b	rd_1w

	/*
	 * This is infinity + x , where RS is the infinity so the result is
	 * just RS (the infinity).
	 */
4:	move	t2,t0
	sll	t1,SEXP_SHIFT
	or	t2,t1
	move	v0,zero
	b	rd_1w

	/*
	 * Check RT for an infinity value.  At this point it is know that RS
	 * is not an infinity.  If RT is an infinity it will be the result.
	 */
5:	bne	t5,SEXP_INF,6f
	move	t2,t4
	sll	t5,SEXP_SHIFT
	or	t2,t5
	move	v0,zero
	b	rd_1w
6:
	# Check for the addition of zeros and produce the correct action if so
	bne	t1,zero,3f	# check RS for a zero value (first the exp)
	bne	t2,zero,3f	# then the fraction
				# Now RS is known to be zero

	bne	t5,zero,2f	# check RT for a zero value (first the exp)
	bne	t6,zero,2f	# then the fraction
	/*
	 * Now RS and RT are known to be zeroes so set the correct result
	 * according to the rounding mode (in the fpc_csr) and exit.
	 */
	and	v0,a3,CSR_RM_MASK	# get the rounding mode
	bne	v0,CSR_RM_RMI,1f	# check for round to - infinity
	or	t2,t0,t4		# set the result and exit, for round to
	move	v0,zero			#  - infinity the zero result is the
	b	rd_1w			#  and of the operand's signs

1:	and	t2,t0,t4		# set the result and exit, for other
	move	v0,zero			#  rounding modes the zero result is the
	b	rd_1w			#  or of the operand's signs

	# RS is a zero and RT is non-zero so the result is RT
2:	move	t2,t4
	sll	t5,SEXP_SHIFT
	or	t2,t5
	or	t2,t6
	move	v0,zero
	b	rd_1w

	# RS is now known not to be zero so check RT for a zero value.
3:	bne	t5,zero,4f	# check RT for a zero value (first the exp)
	bne	t6,zero,4f	# then the fraction
	or	t2,t0		# RT is a zero so the result is RS
	sll	t1,SEXP_SHIFT
	or	t2,t1
	move	v0,zero
	b	rd_1w
4:
	/*
	 * Now that all the NaN, infinity and zero cases have been taken care
	 * of what is left are values that can be added.  So get all values
	 * into a format that can be added.  For normalized numbers set the
	 * implied one and remove the exponent bias.  For denormalized numbers
	 * convert to normalized numbers with the correct exponent.
	 */
	bne	t1,zero,1f	# check for RS being denormalized
	li	t1,-SEXP_BIAS+1	# set denorm's exponent
	jal	rs_renorm_s	# normalize it
	b	2f
1:	subu	t1,SEXP_BIAS	# if RS is not denormalized then remove the
	or	t2,SIMP_1BIT	#  exponent bias, and set the implied 1 bit
2:	bne	t5,zero,3f	# check for RT being denormalized
	li	t5,-SEXP_BIAS+1	# set denorm's exponent
	jal	rt_renorm_s	# normalize it
	b	4f
3:	subu	t5,SEXP_BIAS	# if RT is not denormalized then remove the
	or	t6,SIMP_1BIT	#  exponent bias, and set the implied 1 bit
4:
	/*
	 * If the two values are the same except the sign return the correct
	 * zero according to the rounding mode.
	 */
	beq	t0,t4,2f		# if the sign's are the same continue
	bne	t1,t5,2f		# if the exp's are not the same continue
	bne	t2,t6,2f		# if the fraction's are not the
					#  same continue

	and	v0,a3,CSR_RM_MASK	# get the rounding mode
	bne	v0,CSR_RM_RMI,1f	# check for round to - infinity
	or	t2,t0,t4		# set the result and exit, for round to
					#  - infinity the zero result is the
	move	v0,zero			#  and of the operand's signs
	b	rd_1w

1:	and	t2,t0,t4		# set the result and exit, for other
					#  rounding modes the zero result is the
	move	v0,zero			#  or of the operand's signs
	b	rd_1w
2:
	subu	v1,t1,t5		# find the difference of the exponents
	move	v0,v1			#  in (v1) and the absolute value of
	bge	v1,zero,1f		#  the difference in (v0)
	negu	v0
1:
	ble	v0,SFRAC_BITS+2,2f	# is the difference is greater than the
					#  number of bits of precision?
	li	t8,STKBIT		# set the sticky register
	bge	v1,zero,1f
	# result is RT added with a sticky bit (for RS)
	move	t1,t5			# result exponent will be RT's exponent
	move	t2,zero
	b	4f
1:	# the result is RS added with a sticky bit (for RT)
	move	t6,zero
	b	4f
2:	move	t8,zero			# clear the sticky register
	/*
	 * If the exponent difference is greater than zero shift the smaller
	 * value right by the exponent difference to align the binary point
	 * before the addition.  Also select the exponent of the result to
	 * be the largest exponent of the two values.  The result exponent is
	 * left in (t1) so only if RS is to be shifted does RT's exponent
	 * need to be moved into (t1).
	 */
	beq	v0,zero,4f		# if the exp diff is zero then no shift
	bgt	v1,zero,3f		# if the exp diff > 0 shift RT
	move	t1,t5			# result exponent will be RT's exponent
	# Shift the fraction value of RS by < 32 (the right shift amount (v0))
	negu	v1,v0			# the left shift amount which is 32
	addu	v1,32			#  minus right shift amount (v1)
	srl	t8,v0			# shift the sticky register
	sll	t9,t2,v1
	or	t8,t9
	srl	t2,v0			# shift the fraction
	b	4f
3:	# Shift the fraction value of RT by < 32 (the right shift amount (v0))
	negu	v1,v0			# the left shift amount which is 32
	addu	v1,32			#  minus right shift amount (v1)
	srl	t8,v0			# shift the sticky register
	sll	t9,t6,v1
	or	t8,t9
	srl	t6,v0			# shift the fraction
4:
	/*
	 * Now if the signs are the same add the two fractions, else if the
	 * signs are different then subtract the smaller fraction from the
	 * larger fraction and the result's sign will be the sign of the
	 * larger fraction.
	 */
	bne	t0,t4,1f		# if the signs not the same subtract
	# Add the fractions
	addu	t2,t6			# add fraction words
	b	norm_s
1:
	/*
	 * Subtract the smaller fraction from the larger fraction and set the
	 * sign of the result to the sign of the larger fraction.
	 */
	blt	t2,t6,3f		# determine the smaller fraction
					#  Note the case where they were equal
					#  has already been taken care of
1:	/*
	 * RT is smaller so subtract RT from RS and use RS's sign as the sign
	 * of the result (the sign is already in the correct place (t0)).
	 */
	sltu	t9,zero,t8		# set barrow out for sticky register
	subu	t8,zero,t8
	# subtract least signifiant fraction words
	bne	t9,zero,2f		# see if there is a barrow in
	# no barrow in to be subtracted out
	subu	t2,t2,t6		# subtract fractions
	b	norm_s
2:	# barrow in to be subtracted out
	subu	t2,t2,t6		# subtract least fractions
	subu	t2,1			# subtract barrow in
	b	norm_s
3:
	/*
	 * RS is smaller so subtract RS from RT and use RT's sign as the sign
	 * of the result.
	 */
	move	t0,t4			# use RT's sign as the sign of result
	sltu	t9,zero,t8		# set barrow out for sticky register
	subu	t8,zero,t8
	# subtract least signifiant fraction words
	bne	t9,zero,1f		# see if there is a barrow in
	# no barrow in to be subtracted out
	subu	t2,t6,t2		# subtract least fractions
	b	norm_s
1:	# barrow in to be subtracted out
	subu	t2,t6,t2		# subtract least fractions
	subu	t2,1			# subtract barrow in
	b	norm_s

/*
 * Add (and subtract) double RD = RS + RT (or RD = RS - RT).  Again the FUNC
 * field (t8) is zero for adds.
 */
.globl	add_d
add_d:
	/*
	 * Break out the operands into their fields (sign,exp,fraction) and
	 * handle NaN operands by calling {rs,rt}breakout_d() .
	 */
	li	t9,C1_FMT_DOUBLE*4
	li	v1,1
	jal	rs_breakout_d
	jal	rt_breakout_d
	
	beq	t8,zero,1f	# if doing a subtract then negate RT
	lui	v0,SIGNBIT>>16
	xor	t4,v0
1:
	# Check for addition of infinities, and produce the correct action if so
	bne	t1,DEXP_INF,5f	# is RS an infinity?
				# RS is an infinity
	bne	t5,DEXP_INF,4f	# is RT also an infinity?
				# RT is an infinity
	beq	t0,t4,3f	# do the infinities have the same sign?

	/*
	 * The infinities do NOT have the same sign thus this is an invalid
	 * operation for addition so set the invalid exception in the fpc_csr
	 * (a3) and setup the result depending if the enable for the invalid
	 * exception is set.
	 */
	or	a3,INVALID_EXC
	and	v0,a3,INVALID_ENABLE
	beq	v0,zero,2f
	/*
	 * The invalid trap was enabled so signal a SIGFPE and leave the
	 * result register unmodified.
	 */
	li	v0,SIGFPE
	jal	post_signal
	li	v0,1
	b	store_fpc_csr
	/*
	 * The invalid trap was NOT enabled so the result is a quiet NaN.
	 * So use the default quiet NaN and exit softfp().
	 */
2:	li	t2,DQUIETNAN_LESS
	li	t3,DQUIETNAN_LEAST
	move	v0,zero
	b	rd_2w

	/*
	 * This is just a normal infinity + infinity so the result is just
	 * an infinity with the sign of the operands.
	 */
3:	move	t2,t0
	sll	t1,DEXP_SHIFT
	or	t2,t1
	move	v0,zero
	b	rd_2w

	/*
	 * This is infinity + x , where RS is the infinity so the result is
	 * just RS (the infinity).
	 */
4:	move	t2,t0
	sll	t1,DEXP_SHIFT
	or	t2,t1
	move	v0,zero
	b	rd_2w

	/*
	 * Check RT for an infinity value.  At this point it is know that RS
	 * is not an infinity.  If RT is an infinity it will be the result.
	 */
5:	bne	t5,DEXP_INF,6f
	move	t2,t4
	sll	t5,DEXP_SHIFT
	or	t2,t5
	move	t3,t7
	move	v0,zero
	b	rd_2w
6:
	# Check for the addition of zeros and produce the correct action if so
	bne	t1,zero,3f	# check RS for a zero value (first the exp)
	bne	t2,zero,3f	# then the high part of the fraction
	bne	t3,zero,3f	# then the low part of the fraction
				# Now RS is known to be zero

	bne	t5,zero,2f	# check RT for a zero value (first the exp)
	bne	t6,zero,2f	# then the high part of the fraction
	bne	t7,zero,2f	# then the low part of the fraction
	/*
	 * Now RS and RT are known to be zeroes so set the correct result
	 * according to the rounding mode (in the fpc_csr) and exit.
	 */
	and	v0,a3,CSR_RM_MASK	# get the rounding mode
	bne	v0,CSR_RM_RMI,1f	# check for round to - infinity
	or	t2,t0,t4		# set the result and exit, for round to
	move	v0,zero			#  - infinity the zero result is the
	b	rd_2w			#  and of the operand's signs

1:	and	t2,t0,t4		# set the result and exit, for other
	move	v0,zero			#  rounding modes the zero result is the
	b	rd_2w			#  or of the operand's signs

	# RS is a zero and RT is non-zero so the result is RT
2:	move	t2,t4
	sll	t5,DEXP_SHIFT
	or	t2,t5
	or	t2,t6
	move	t3,t7
	move	v0,zero
	b	rd_2w

	# RS is now known not to be zero so check RT for a zero value.
3:	bne	t5,zero,4f	# check RT for a zero value (first the exp)
	bne	t6,zero,4f	# then the high part of the fraction
	bne	t7,zero,4f	# then the low part of the fraction
	or	t2,t0		# RT is a zero so the result is RS
	sll	t1,DEXP_SHIFT
	or	t2,t1
	move	v0,zero
	b	rd_2w
4:
	/*
	 * Now that all the NaN, infinity and zero cases have been taken care
	 * of what is left are values that can be added.  So get all values
	 * into a format that can be added.  For normalized numbers set the
	 * implied one and remove the exponent bias.  For denormalized numbers
	 * convert to normalized numbers with the correct exponent.
	 */
	bne	t1,zero,1f	# check for RS being denormalized
	li	t1,-DEXP_BIAS+1	# set denorm's exponent
	jal	rs_renorm_d	# normalize it
	b	2f
1:	subu	t1,DEXP_BIAS	# if RS is not denormalized then remove the
	or	t2,DIMP_1BIT	#  exponent bias, and set the implied 1 bit
2:	bne	t5,zero,3f	# check for RT being denormalized
	li	t5,-DEXP_BIAS+1	# set denorm's exponent
	jal	rt_renorm_d	# normalize it
	b	4f
3:	subu	t5,DEXP_BIAS	# if RT is not denormalized then remove the
	or	t6,DIMP_1BIT	#  exponent bias, and set the implied 1 bit
4:
	/*
	 * If the two values are the same except the sign return the correct
	 * zero according to the rounding mode.
	 */
	beq	t0,t4,2f		# if the sign's are the same continue
	bne	t1,t5,2f		# if the exp's are not the same continue
	bne	t2,t6,2f		# if the fraction's are not the
	bne	t3,t7,2f		#  same continue

	and	v0,a3,CSR_RM_MASK	# get the rounding mode
	bne	v0,CSR_RM_RMI,1f	# check for round to - infinity
	or	t2,t0,t4		# set the result and exit, for round to
	move	t3,zero			#  - infinity the zero result is the
	move	v0,zero			#  and of the operand's signs
	b	rd_2w

1:	and	t2,t0,t4		# set the result and exit, for other
	move	t3,zero			#  rounding modes the zero result is the
	move	v0,zero			#  or of the operand's signs
	b	rd_2w
2:
	subu	v1,t1,t5		# find the difference of the exponents
	move	v0,v1			#  in (v1) and the absolute value of
	bge	v1,zero,1f		#  the difference in (v0)
	negu	v0
1:
	ble	v0,DFRAC_BITS+2,3f	# is the difference is greater than the
					#  number of bits of precision
	li	t8,STKBIT		# set the sticky register
	bge	v1,zero,2f
	# result is RT with a STKBIT added (for RS)
	move	t1,t5			# result exponent will be RT's exponent
	move	t2,zero
	move	t3,zero
	b	9f
2:	# the result is RS with a STKBIT added (for RT)
	move	t6,zero
	move	t7,zero
	b	9f
3:	move	t8,zero			# clear the sticky register
	/*
	 * If the exponent difference is greater than zero shift the smaller
	 * value right by the exponent difference to align the binary point
	 * before the addition.  Also select the exponent of the result to
	 * be the largest exponent of the two values.  The result exponent is
	 * left in (t1) so only if RS is to be shifted does RT's exponent
	 * need to be moved into (t1).
	 */
	beq	v0,zero,9f		# if the exp diff is zero then no shift
	bgt	v1,zero,6f		# if the exp diff > 0 shift RT
	move	t1,t5			# result exponent will be RT's exponent
	# Shift the fraction of the RS value
	blt	v0,32,5f		# check for shifts >= 32
	move	t8,t3			# shift the fraction over 32 bits by
	move	t3,t2			#  moving the words to the right and
	move	t2,zero			#  fill the highest word with a zero
	beq	t8,zero,4f		# if any 1's get into the sticky reg
	or	t8,STKBIT		#  make sure the sticky bit stays set
4:	subu	v0,32			# the right shift amount (v0)
	negu	v1,v0			# the left shift amount which is 32
	addu	v1,32			#  minus right shift amount (v1)
	# Now shift the fraction (only the low two words in this case)
	srl	t8,v0			# shift the sticky register
	sll	t9,t3,v1
	or	t8,t9
	srl	t3,v0			# shift the low word of the fraction
	b	9f
5:	# Shift the fraction value of RS by < 32 (the right shift amount (v0))
	negu	v1,v0			# the left shift amount which is 32
	addu	v1,32			#  minus right shift amount (v1)
	srl	t8,v0			# shift the sticky register
	sll	t9,t3,v1
	or	t8,t9
	srl	t3,v0			# shift the low word of the fraction
	sll	t9,t2,v1
	or	t3,t9
	srl	t2,v0			# shift the high word of the fraction
	b	9f
6:	# Shift the fraction of the RT value
	blt	v0,32,8f		# check for shifts >= 32
	move	t8,t7			# shift the fraction over 32 bits by
	move	t7,t6			#  moving the words to the right and
	move	t6,zero			#  fill the highest word with a zero
	beq	t8,zero,7f		# if any 1's get into the sticky reg
	or	t8,STKBIT		#  make sure the sticky bit stays set
7:	subu	v0,32			# the right shift amount (v0)
	negu	v1,v0			# the left shift amount which is 32
	addu	v1,32			#  minus right shift amount (v1)
	# Now shift the fraction (only the low two words in this case)
	srl	t8,v0			# shift the sticky register
	sll	t9,t7,v1
	or	t8,t9
	srl	t7,v0			# shift the low word of the fraction
	b	9f
8:	# Shift the fraction value of RT by < 32 (the right shift amount (v0))
	negu	v1,v0			# the left shift amount which is 32
	addu	v1,32			#  minus right shift amount (v1)
	srl	t8,v0			# shift the sticky register
	sll	t9,t7,v1
	or	t8,t9
	srl	t7,v0			# shift the low word of the fraction
	sll	t9,t6,v1
	or	t7,t9
	srl	t6,v0			# shift the high word of the fraction
9:
	/*
	 * Now if the signs are the same add the two fractions, else if the
	 * signs are different then subtract the smaller fraction from the
	 * larger fraction and the result's sign will be the sign of the
	 * larger fraction.
	 */
	bne	t0,t4,2f		# if the signs not the same subtract
	# Add the fractions
	not	v0,t3			# set carry out (t9) for the addition
	sltu	t9,v0,t7		#  of the least fraction words
	addu	t3,t7			# add the least fraction words
	# add the less fraction fraction words with the carry in
	bne	t9,zero,1f		# see if there is a carry in
	# no carry in to be added in (carry out is not possible or needed)
	addu	t2,t6			# add the less fraction words
	b	norm_d
	# a carry in is to be added in (carry out is not possible or needed)
1:	addu	t2,t6			# add the less fraction words
	addu	t2,1			# add in the carry in
	b	norm_d
2:
	/*
	 * Subtract the smaller fraction from the larger fraction and set the
	 * sign of the result to the sign of the larger fraction.
	 */
	blt	t2,t6,5f		# determine the smaller fraction
	bgt	t2,t6,1f		#  Note the case where they were equal
	bltu	t3,t7,5f		#  has already been taken care of
1:	/*
	 * RT is smaller so subtract RT from RS and use RS's sign as the sign
	 * of the result (the sign is already in the correct place (t0)).
	 */
	sltu	t9,zero,t8		# set barrow out for sticky register
	subu	t8,zero,t8
	# subtract least signifiant fraction words
	bne	t9,zero,2f		# see if there is a barrow in
	# no barrow in to be subtracted out
	sltu	t9,t3,t7		# set barrow out for least fraction
	subu	t3,t3,t7		# subtract least fractions
	b	3f
2:	# barrow in to be subtracted out
	sltu	t9,t3,t7		# set barrow out for least fraction
	subu	t3,t3,t7		# subtract least fractions
	seq	v0,t3,zero		# set barrow out for barrow in
	or	t9,v0			# final barrow out
	subu	t3,1			# subtract barrow in
3:	# subtract less signifiant fraction words
	bne	t9,zero,4f		# see if there is a barrow in
	# no barrow in to be subtracted out (barrow out not possible or needed)
	subu	t2,t2,t6		# subtract less fractions
	b	norm_d
4:	# barrow in to be subtracted out (barrow out not possible or needed)
	subu	t2,t2,t6		# subtract less fractions
	subu	t2,1			# subtract barrow in
	b	norm_d
5:
	/*
	 * RS is smaller so subtract RS from RT and use RT's sign as the sign
	 * of the result.
	 */
	move	t0,t4			# use RT's sign as the sign of result
	sltu	t9,zero,t8		# set barrow out for sticky register
	subu	t8,zero,t8
	# subtract least signifiant fraction words
	bne	t9,zero,1f		# see if there is a barrow in
	# no barrow in to be subtracted out
	sltu	t9,t7,t3		# set barrow out for least fraction
	subu	t3,t7,t3		# subtract least fractions
	b	2f
1:	# barrow in to be subtracted out
	sltu	t9,t7,t3		# set barrow out for least fraction
	subu	t3,t7,t3		# subtract least fractions
	seq	v0,t3,zero		# set barrow out for barrow in
	or	t9,v0			# final barrow out
	subu	t3,1			# subtract barrow in
2:	# subtract less signifiant fraction words
	bne	t9,zero,3f		# see if there is a barrow in
	# no barrow in to be subtracted out (barrow out not possible or needed)
	subu	t2,t6,t2		# subtract less fractions
	b	norm_d
3:	# barrow in to be subtracted out (barrow out not possible or needed)
	subu	t2,t6,t2		# subtract least fractions
	subu	t2,1			# subtract barrow in
	b	norm_d

add_e:
add_q:
	b	illfpinst

func_mul:
	lw	v1,mul_fmt_tab(v0)
	j	v1

	.rdata
mul_fmt_tab:
	.word	mul_s:1, mul_d:1, mul_e:1, mul_q:1, illfpinst:1
	.text

/*
 * Multiplication single RD = RS * RT
 */
.globl mul_s
mul_s:
	/*
	 * Break out the operands into their fields (sign,exp,fraction) and
	 * handle NaN operands by calling {rs,rt}breakout_s() .
	 */
	li	t9,C1_FMT_SINGLE*4
	li	v1,1
	jal	rs_breakout_s
	jal	rt_breakout_s

	/*
	 * With the NaN cases taken care of the sign of the result for all
	 * other operands is just the exclusive or of the signs of the
	 * operands.
	 */
	xor	t0,t4

	/*
	 * Check for multiplication of infinities, and produce the correct
	 * action if so.
	 */
	bne	t1,SEXP_INF,4f	# is RS an infinity?
	bne	t5,SEXP_INF,1f	# is RT also an infinity?
	/*
	 * The operation is just infinity * infinity so the result is just
	 * a properly signed infinity.
	 */
	or	t2,t0
	sll	t1,SEXP_SHIFT
	or	t2,t1
	move	v0,zero
	b	rd_1w
1:	/*
	 * RS is an infinity and RT is NOT an infinity, if RT is zero then
	 * this is an invalid operation else the result is just the RS infinity.
	 */
	bne	t5,zero,3f
	bne	t6,zero,3f
	/*
	 * RS is an infinity and RT is zero thus this is an invalid operation
	 * for addition so set the invalid exception in the fpc_csr (a3) and
	 * setup the result depending if the enable for the invalid exception
	 * is set.
	 */
	or	a3,INVALID_EXC
	and	v0,a3,INVALID_ENABLE
	beq	v0,zero,2f
	/*
	 * The invalid trap was enabled so signal a SIGFPE and leave the
	 * result register unmodified.
	 */
	li	v0,SIGFPE
	jal	post_signal
	li	v0,1
	b	store_fpc_csr
	/*
	 * The invalid trap was NOT enabled so the result is a quiet NaN.
	 * So use the default quiet NaN and exit softfp().
	 */
2:	li	t2,SQUIETNAN_LEAST
	move	v0,zero
	b	rd_1w
	/*
	 * The operation is just infinity (RS) * x (RT) so the result is just
	 * the infinity (RS).
	 */
3:	or	t2,t0
	sll	t1,SEXP_SHIFT
	or	t2,t1
	move	v0,zero
	b	rd_1w
	/*
	 * RS is known NOT to be an infinity so now check RT for an infinity
	 */
4:	bne	t5,SEXP_INF,8f	# is RT an infinity?
	/*
	 * RT is an infinity, if RS is zero then this is an invalid operation
	 * else the result is just the RT infinity.
	 */
	bne	t1,zero,7f
	bne	t2,zero,7f
	/*
	 * RS is an infinity and RT is zero thus this is an invalid operation
	 * for addition so set the invalid exception in the fpc_csr (a3) and
	 * setup the result depending if the enable for the invalid exception
	 * is set.
	 */
	or	a3,INVALID_EXC
	and	v0,a3,INVALID_ENABLE
	beq	v0,zero,6f
	/*
	 * The invalid trap was enabled so signal a SIGFPE and leave the
	 * result register unmodified.
	 */
	li	v0,SIGFPE
	jal	post_signal
	li	v0,1
	b	store_fpc_csr
	/*
	 * The invalid trap was NOT enabled so the result is a quiet NaN.
	 * So use the default quiet NaN and exit softfp().
	 */
6:	li	t2,SQUIETNAN_LEAST
	move	v0,zero
	b	rd_1w
	/*
	 * The operation is just x (RS) * infinity (RT) so the result is just
	 * the infinity (RT).
	 */
7:	move	t2,t0
	sll	t5,SEXP_SHIFT
	or	t2,t5
	move	v0,zero
	b	rd_1w
8:
	/*
	 * Check for the multiplication of zeros and produce the correct zero
	 * if so.
	 */
	bne	t1,zero,1f	# check RS for a zero value (first the exp)
	bne	t2,zero,1f	# then the fraction
	# Now RS is known to be zero so return the properly signed zero
	move	t2,t0
	move	v0,zero
	b	rd_1w

1:	bne	t5,zero,2f	# check RT for a zero value (first the exp)
	bne	t6,zero,2f	# then the fraction
	# Now RT is known to be zero so return the properly signed zero
	move	t2,t0
	move	v0,zero
	b	rd_1w
2:
	/*
	 * Now that all the NaN, infinity and zero cases have been taken care
	 * of what is left are values that can be added.  So get all values
	 * into a format that can be added.  For normalized numbers set the
	 * implied one and remove the exponent bias.  For denormalized numbers
	 * convert to normalized numbers with the correct exponent.
	 */
	bne	t1,zero,1f	# check for RS being denormalized
	li	t1,-SEXP_BIAS+1	# set denorm's exponent
	jal	rs_renorm_s	# normalize it
	b	2f
1:	subu	t1,SEXP_BIAS	# if RS is not denormalized then remove the
	or	t2,SIMP_1BIT	#  exponent bias, and set the implied 1 bit
2:	bne	t5,zero,3f	# check for RT being denormalized
	li	t5,-SEXP_BIAS+1	# set denorm's exponent
	jal	rt_renorm_s	# normalize it
	b	4f
3:	subu	t5,SEXP_BIAS	# if RT is not denormalized then remove the
	or	t6,SIMP_1BIT	#  exponent bias, and set the implied 1 bit
4:
	/*
	 * Calculate the exponent of the result to be used by the norm_s:
	 * code to figure the final exponent.
	 */
	addu	t1,t5
	addu	t1,9

	multu	t2,t6		# multiply RS(fraction) * RT(fraction)
	mflo	t8		# the low 32 bits will be the sticky register
	mfhi	t2		# the high 32 bits will the result
	b	norm_s

/*
 * Multiplication double RD = RS * RT
 */
.globl mul_d
mul_d:
	/*
	 * Break out the operands into their fields (sign,exp,fraction) and
	 * handle NaN operands by calling {rs,rt}breakout_d() .
	 */
	li	t9,C1_FMT_DOUBLE*4
	li	v1,1
	jal	rs_breakout_d
	jal	rt_breakout_d

	/*
	 * With the NaN cases taken care of the sign of the result for all
	 * other operands is just the exclusive or of the signs of the
	 * operands.
	 */
	xor	t0,t4

	/*
	 * Check for multiplication of infinities, and produce the correct
	 * action if so.
	 */
	bne	t1,DEXP_INF,4f	# is RS an infinity?
	bne	t5,DEXP_INF,1f	# is RT also an infinity?
	/*
	 * The operation is just infinity * infinity so the result is just
	 * a properly signed infinity.
	 */
	or	t2,t0
	sll	t1,DEXP_SHIFT
	or	t2,t1
	move	v0,zero
	b	rd_2w
1:	/*
	 * RS is an infinity and RT is NOT an infinity, if RT is zero then
	 * this is an invalid operation else the result is just the RS infinity.
	 */
	bne	t5,zero,3f
	bne	t6,zero,3f
	bne	t7,zero,3f
	/*
	 * RS is an infinity and RT is zero thus this is an invalid operation
	 * for addition so set the invalid exception in the fpc_csr (a3) and
	 * setup the result depending if the enable for the invalid exception
	 * is set.
	 */
	or	a3,INVALID_EXC
	and	v0,a3,INVALID_ENABLE
	beq	v0,zero,2f
	/*
	 * The invalid trap was enabled so signal a SIGFPE and leave the
	 * result register unmodified.
	 */
	li	v0,SIGFPE
	jal	post_signal
	li	v0,1
	b	store_fpc_csr
	/*
	 * The invalid trap was NOT enabled so the result is a quiet NaN.
	 * So use the default quiet NaN and exit softfp().
	 */
2:	li	t2,DQUIETNAN_LESS
	li	t3,DQUIETNAN_LEAST
	move	v0,zero
	b	rd_2w
	/*
	 * The operation is just infinity (RS) * x (RT) so the result is just
	 * the infinity (RS).
	 */
3:	or	t2,t0
	sll	t1,DEXP_SHIFT
	or	t2,t1
	move	v0,zero
	b	rd_2w
	/*
	 * RS is known NOT to be an infinity so now check RT for an infinity
	 */
4:	bne	t5,DEXP_INF,8f	# is RT an infinity?
	/*
	 * RT is an infinity, if RS is zero then this is an invalid operation
	 * else the result is just the RT infinity.
	 */
	bne	t1,zero,7f
	bne	t2,zero,7f
	bne	t3,zero,7f
	/*
	 * RS is an infinity and RT is zero thus this is an invalid operation
	 * for addition so set the invalid exception in the fpc_csr (a3) and
	 * setup the result depending if the enable for the invalid exception
	 * is set.
	 */
	or	a3,INVALID_EXC
	and	v0,a3,INVALID_ENABLE
	beq	v0,zero,6f
	/*
	 * The invalid trap was enabled so signal a SIGFPE and leave the
	 * result register unmodified.
	 */
	li	v0,SIGFPE
	jal	post_signal
	li	v0,1
	b	store_fpc_csr
	/*
	 * The invalid trap was NOT enabled so the result is a quiet NaN.
	 * So use the default quiet NaN and exit softfp().
	 */
6:	li	t2,DQUIETNAN_LESS
	li	t3,DQUIETNAN_LEAST
	move	v0,zero
	b	rd_2w
	/*
	 * The operation is just x (RS) * infinity (RT) so the result is just
	 * the infinity (RT).
	 */
7:	move	t2,t0
	sll	t5,DEXP_SHIFT
	or	t2,t5
	move	t3,zero
	move	v0,zero
	b	rd_2w
8:
	/*
	 * Check for the multiplication of zeros and produce the correct zero
	 * if so.
	 */
	bne	t1,zero,1f	# check RS for a zero value (first the exp)
	bne	t2,zero,1f	# then the high part of the fraction
	bne	t3,zero,1f	# then the low part of the fraction
	# Now RS is known to be zero so return the properly signed zero
	move	t2,t0
	move	v0,zero
	b	rd_2w

1:	bne	t5,zero,2f	# check RT for a zero value (first the exp)
	bne	t6,zero,2f	# then the high part of the fraction
	bne	t7,zero,2f	# then the low part of the fraction
	# Now RT is known to be zero so return the properly signed zero
	move	t2,t0
	move	t3,zero
	move	v0,zero
	b	rd_2w
2:
	/*
	 * Now that all the NaN, infinity and zero cases have been taken care
	 * of what is left are values that can be added.  So get all values
	 * into a format that can be added.  For normalized numbers set the
	 * implied one and remove the exponent bias.  For denormalized numbers
	 * convert to normalized numbers with the correct exponent.
	 */
	bne	t1,zero,1f	# check for RS being denormalized
	li	t1,-DEXP_BIAS+1	# set denorm's exponent
	jal	rs_renorm_d	# normalize it
	b	2f
1:	subu	t1,DEXP_BIAS	# if RS is not denormalized then remove the
	or	t2,DIMP_1BIT	#  exponent bias, and set the implied 1 bit
2:	bne	t5,zero,3f	# check for RT being denormalized
	li	t5,-DEXP_BIAS+1	# set denorm's exponent
	jal	rt_renorm_d	# normalize it
	b	4f
3:	subu	t5,DEXP_BIAS	# if RT is not denormalized then remove the
	or	t6,DIMP_1BIT	#  exponent bias, and set the implied 1 bit
4:
	/*
	 * Calculate the exponent of the result to be used by the norm_d:
	 * code to figure the final exponent.
	 */
	addu	t1,t5
	addu	t1,12

	/*
	 * Since the norm_d: code expects the fraction to be in t2,t3,t8
	 * RS's fraction is moved to t4,t5 so to free up t2,t3 to hold
	 * the accululated result of the partal products.
	 */
	move	t4,t2
	move	t5,t3

	multu	t5,t7		# multiply RS(low) * RT(low) fractions
	mflo	ra		# all the low 32 bits (ra) go into the sticky
	sne	ra,ra,zero	#  bit so set it if there are any one bits
	mfhi	t8		# get the high 32 bits (t8)

	multu	t5,t6		# multiply RS(low) * RT(high) fractions
	mflo	v1		# the low 32 bits will be added to t8
	mfhi	t3		# the high 32 bits will go into t3
	move	t2,zero		# the highest accumulator word is zero'ed
	not	v0,v1		# set the carry out of t8 and low 32 bits
	sltu	t9,v0,t8	#  of the mult (v1)
	addu	t8,v1		# do the add of t8 and the low 32 bits (v1)
	beq	t9,zero,1f	# if no carry out continue
	addu	t3,1		# add the carry in
	seq	t2,t3,zero	# set the carry out into t3
1:
	multu	t4,t7		# multiply RS(high) * RT(low) fractions
	mflo	v1		# the low 32 bits will be added to t8
	mfhi	t5		# the high 32 bits will be added to t3
	not	v0,v1		# set the carry out of t8 and low 32 bits
	sltu	t9,v0,t8	#  of the mult (v1)
	addu	t8,v1		# do the add of t8 and the low 32 bits (v1)
	beq	t9,zero,2f	# branch if no carry out
	# add t3 and the high 32 bits of the mult (t5) and the carry in
	not	v0,t5		# set the carry out of t3 and high 32 bits
	sltu	t9,v0,t3	#  of the mult (t5)
	addu	t3,t5		# do the add of t3 and the high 32 bits (t5)
	addu	t3,1		# add the carry in
	seq	v0,t3,zero	# set the carry out of the carry in
	or	t9,v0		# set the final carry out
	b	3f
2:	# add t3 and the high 32 bits of the mult (t5) and no carry in
	not	v0,t5		# set the carry out of t3 and high 32 bits
	sltu	t9,v0,t3	#  of the mult (t5)
	addu	t3,t5		# do the add of t3 and the high 32 bits (t5)
3:	addu	t2,t9		# add the carry out to t2

	multu	t4,t6		# multiply RS(high) * RT(high) fractions
	mflo	v1		# the low 32 bits will be added to t3
	mfhi	t5		# the high 32 bits will be added to t2
	not	v0,v1		# set the carry out of t3 and low 32 bits
	sltu	t9,v0,t3	#  of the mult (v1)
	addu	t3,v1		# do the add of t3 and the low 32 bits (v1)
	beq	t9,zero,4f	# branch if no carry out
	# add t2 and the high 32 bits of the mult (t5) and the carry in
	addu	t2,1		# add the carry in
4:	addu	t2,t5		# do the add of t2 and the high 32 bits (t5)
	or	t8,ra

	b	norm_d

mul_e:
mul_q:
	b	illfpinst

func_div:
	lw	v1,div_fmt_tab(v0)
	j	v1

	.rdata
div_fmt_tab:
	.word	div_s:1, div_d:1, div_e:1, div_q:1, illfpinst:1
	.text

/*
 * Division single RD = RS / RT
 */
.globl div_s
div_s:
	/*
	 * Break out the operands into their fields (sign,exp,fraction) and
	 * handle NaN operands by calling {rs,rt}breakout_s() .
	 */
	li	t9,C1_FMT_SINGLE*4
	li	v1,1
	jal	rs_breakout_s
	jal	rt_breakout_s

	/*
	 * With the NaN cases taken care of the sign of the result for all
	 * other operands is just the exclusive or of the signs of the
	 * operands.
	 */
	xor	t0,t4

	/*
	 * Check for division of infinities, and produce the correct
	 * action if so.
	 */
	bne	t1,SEXP_INF,3f	# is RS an infinity?
	bne	t5,SEXP_INF,2f	# is RT also an infinity?
	/*
	 * The operation is infinity / infinity which is an invalid operation.
	 * So set the invalid exception in the fpc_csr (a3) and setup the
	 * result depending if the enable for the invalid exception is set.
	 */
	or	a3,INVALID_EXC
	and	v0,a3,INVALID_ENABLE
	beq	v0,zero,1f
	/*
	 * The invalid trap was enabled so signal a SIGFPE and leave the
	 * result register unmodified.
	 */
	li	v0,SIGFPE
	jal	post_signal
	li	v0,1
	b	store_fpc_csr
	/*
	 * The invalid trap was NOT enabled so the result is a quiet NaN.
	 * So use the default quiet NaN and exit softfp().
	 */
1:	li	t2,SQUIETNAN_LEAST
	move	v0,zero
	b	rd_1w
	/*
	 * RS is an infinity and RT is NOT an infinity so the result is just a
	 * a properly signed infinity (even if RT is zero).
	 */
2:	or	t2,t0
	sll	t1,SEXP_SHIFT
	or	t2,t1
	move	v0,zero
	b	rd_1w
	/*
	 * RS is known NOT to be an infinity so now check RT for an infinity
	 */
3:	bne	t5,SEXP_INF,4f	# is RT an infinity?
	/*
	 * RT is an infinity and RS is NOT an infinity so the result is just a
	 * a properly signed zero.
	 */
	move	t2,t0
	move	v0,zero
	b	rd_1w
4:
	/*
	 * Check for the division with zeros and produce the correct action
	 * if so.
	 */
	bne	t5,zero,4f	# check RT for a zero value (first the exp)
	bne	t6,zero,4f	# then the fraction
	/*
	 * Now RT is known to be zero, if RS is zero it is an invalid operation
	 * if not it is a divide by zero.
	 */
	bne	t1,zero,2f	# check RS for a zero value (first the exp)
	bne	t2,zero,2f	# then the fraction
	/*
	 * The operation is 0 / 0 which is an invalid operation.
	 * So set the invalid exception in the fpc_csr (a3) and setup the
	 * result depending if the enable for the invalid exception is set.
	 */
	or	a3,INVALID_EXC
	and	v0,a3,INVALID_ENABLE
	beq	v0,zero,1f
	/*
	 * The invalid trap was enabled so signal a SIGFPE and leave the
	 * result register unmodified.
	 */
	li	v0,SIGFPE
	jal	post_signal
	li	v0,1
	b	store_fpc_csr
	/*
	 * The invalid trap was NOT enabled so the result is a quiet NaN.
	 * So use the default quiet NaN and exit softfp().
	 */
1:	li	t2,SQUIETNAN_LEAST
	move	v0,zero
	b	rd_1w
	/*
	 * The operation is x / 0 which is a divide by zero excrption.  So set
	 * the divide by zero exception in the fpc_csr (a3) and setup the
	 * result depending if the enable for the divide by zero exception
	 * is set.
	 */
2:	or	a3,DIVIDE0_EXC
	and	v0,a3,DIVIDE0_ENABLE
	beq	v0,zero,3f
	/*
	 * The divide by zero trap was enabled so signal a SIGFPE and leave the
	 * result register unmodified.
	 */
	li	v0,SIGFPE
	jal	post_signal
	li	v0,1
	b	store_fpc_csr
	/*
	 * The divide by zero trap was NOT enabled so the result is a properly
	 * signed infinity.
	 */
3:	or	t2,t0,SEXP_INF<<SEXP_SHIFT
	move	v0,zero
	b	rd_1w
	/*
	 * Now RT is known NOT to be zero, if RS is zero the result is just
	 * a properly signed zero.
	 */
4:	bne	t1,zero,5f	# check RS for a zero value (first the exp)
	bne	t2,zero,5f	# then the fraction
	move	t2,t0
	move	v0,zero
	b	rd_1w
5:
	/*
	 * Now that all the NaN, infinity and zero cases have been taken care
	 * of what is left are values that can be divded.  So get all values
	 * into a format that can be divded.  For normalized numbers set the
	 * implied one and remove the exponent bias.  For denormalized numbers
	 * convert to normalized numbers with the correct exponent.
	 */
	bne	t1,zero,1f	# check for RS being denormalized
	li	t1,-SEXP_BIAS+1	# set denorm's exponent
	jal	rs_renorm_s	# normalize it
	b	2f
1:	subu	t1,SEXP_BIAS	# if RS is not denormalized then remove the
	or	t2,SIMP_1BIT	#  exponent bias, and set the implied 1 bit
2:	bne	t5,zero,3f	# check for RT being denormalized
	li	t5,-SEXP_BIAS+1	# set denorm's exponent
	jal	rt_renorm_s	# normalize it
	b	4f
3:	subu	t5,SEXP_BIAS	# if RT is not denormalized then remove the
	or	t6,SIMP_1BIT	#  exponent bias, and set the implied 1 bit
4:
	/*
	 * Calculate the exponent of the result to be used by the norm_s:
	 * code to figure the final exponent.
	 */
	subu	t1,t5
	subu	t1,8

	/*
	 * Since the norm_s: code expects the fraction to be in t2,t8
	 * RS's fraction is moved to t4 so to free up t2 to hold
	 * the final quotient of the division.
	 */
	move	t4,t2

	move	v0,zero		# set the number of quotient bits calculated
				#  to zero
	move	t2,zero		# clear the quotient accumulator

1:	bltu	t4,t6,3f	# check if dividend (RS) >= divisor (RT)

2:	# subtract divisor (RT) from dividend (RS)
	subu	t4,t6		# subtract the fraction words
	addu	t2,1		# add one to the quotient accumulator

	bne	t4,zero,3f	# see if division is done (remainder of zero)
	move	t8,zero		# clear the sticky register (no remainder)
	negu	v0		# shift the quotient accumulator into it's
	addu	v0,31		#  final possition and goto norm_s:
	sll	t2,v0
	b	norm_s

3:	sll	t4,1		# shift the dividend (RS) left one bit
	addu	v0,1		# add one to the number of quotient bits
				#  calculated
	sll	t2,1		# shift the quotient accumulator left one bit
	# see if enough quotient bits have been calculated if not continue
	blt	v0,SFRAC_BITS+3,1b

	negu	v0		# shift the quoient accumulator into it's
	addu	v0,31		#  final possition
	sll	t2,v0

	move	t8,t4		# set the sticky register with all the bits of
				#  remainder
	b	norm_s

/*
 * Division double RD = RS / RT
 */
.globl div_d
div_d:
	/*
	 * Break out the operands into their fields (sign,exp,fraction) and
	 * handle NaN operands by calling {rs,rt}breakout_d() .
	 */
	li	t9,C1_FMT_DOUBLE*4
	li	v1,1
	jal	rs_breakout_d
	jal	rt_breakout_d

	/*
	 * With the NaN cases taken care of the sign of the result for all
	 * other operands is just the exclusive or of the signs of the
	 * operands.
	 */
	xor	t0,t4

	/*
	 * Check for division of infinities, and produce the correct
	 * action if so.
	 */
	bne	t1,DEXP_INF,3f	# is RS an infinity?
	bne	t5,DEXP_INF,2f	# is RT also an infinity?
	/*
	 * The operation is infinity / infinity which is an invalid operation.
	 * So set the invalid exception in the fpc_csr (a3) and setup the
	 * result depending if the enable for the invalid exception is set.
	 */
	or	a3,INVALID_EXC
	and	v0,a3,INVALID_ENABLE
	beq	v0,zero,1f
	/*
	 * The invalid trap was enabled so signal a SIGFPE and leave the
	 * result register unmodified.
	 */
	li	v0,SIGFPE
	jal	post_signal
	li	v0,1
	b	store_fpc_csr
	/*
	 * The invalid trap was NOT enabled so the result is a quiet NaN.
	 * So use the default quiet NaN and exit softfp().
	 */
1:	li	t2,DQUIETNAN_LESS
	li	t3,DQUIETNAN_LEAST
	move	v0,zero
	b	rd_2w
	/*
	 * RS is an infinity and RT is NOT an infinity so the result is just a
	 * a properly signed infinity (even if RT is zero).
	 */
2:	or	t2,t0
	sll	t1,DEXP_SHIFT
	or	t2,t1
	move	v0,zero
	b	rd_2w
	/*
	 * RS is known NOT to be an infinity so now check RT for an infinity
	 */
3:	bne	t5,DEXP_INF,4f	# is RT an infinity?
	/*
	 * RT is an infinity and RS is NOT an infinity so the result is just a
	 * a properly signed zero.
	 */
	move	t2,t0
	move	t3,zero
	move	v0,zero
	b	rd_2w
4:
	/*
	 * Check for the division with zeros and produce the correct action
	 * if so.
	 */
	bne	t5,zero,4f	# check RT for a zero value (first the exp)
	bne	t6,zero,4f	# then the high part of the fraction
	bne	t7,zero,4f	# then the low part of the fraction
	/*
	 * Now RT is known to be zero, if RS is zero it is an invalid operation
	 * if not it is a divide by zero.
	 */
	bne	t1,zero,2f	# check RS for a zero value (first the exp)
	bne	t2,zero,2f	# then the high part of the fraction
	bne	t3,zero,2f	# then the low part of the fraction
	/*
	 * The operation is 0 / 0 which is an invalid operation.
	 * So set the invalid exception in the fpc_csr (a3) and setup the
	 * result depending if the enable for the invalid exception is set.
	 */
	or	a3,INVALID_EXC
	and	v0,a3,INVALID_ENABLE
	beq	v0,zero,1f
	/*
	 * The invalid trap was enabled so signal a SIGFPE and leave the
	 * result register unmodified.
	 */
	li	v0,SIGFPE
	jal	post_signal
	li	v0,1
	b	store_fpc_csr
	/*
	 * The invalid trap was NOT enabled so the result is a quiet NaN.
	 * So use the default quiet NaN and exit softfp().
	 */
1:	li	t2,DQUIETNAN_LESS
	li	t3,DQUIETNAN_LEAST
	move	v0,zero
	b	rd_2w
	/*
	 * The operation is x / 0 which is a divide by zero excrption.  So set
	 * the divide by zero exception in the fpc_csr (a3) and setup the
	 * result depending if the enable for the divide by zero exception
	 * is set.
	 */
2:	or	a3,DIVIDE0_EXC
	and	v0,a3,DIVIDE0_ENABLE
	beq	v0,zero,3f
	/*
	 * The divide by zero trap was enabled so signal a SIGFPE and leave the
	 * result register unmodified.
	 */
	li	v0,SIGFPE
	jal	post_signal
	li	v0,1
	b	store_fpc_csr
	/*
	 * The divide by zero trap was NOT enabled so the result is a properly
	 * signed infinity.
	 */
3:	or	t2,t0,DEXP_INF<<DEXP_SHIFT
	move	t3,zero
	move	v0,zero
	b	rd_2w
	/*
	 * Now RT is known NOT to be zero, if RS is zero the result is just
	 * a properly signed zero.
	 */
4:	bne	t1,zero,5f	# check RS for a zero value (first the exp)
	bne	t2,zero,5f	# then the high part of the fraction
	bne	t3,zero,5f	# then the low part of the fraction
	move	t2,t0
	move	v0,zero
	b	rd_2w
5:
	/*
	 * Now that all the NaN, infinity and zero cases have been taken care
	 * of what is left are values that can be divded.  So get all values
	 * into a format that can be divded.  For normalized numbers set the
	 * implied one and remove the exponent bias.  For denormalized numbers
	 * convert to normalized numbers with the correct exponent.
	 */
	bne	t1,zero,1f	# check for RS being denormalized
	li	t1,-DEXP_BIAS+1	# set denorm's exponent
	jal	rs_renorm_d	# normalize it
	b	2f
1:	subu	t1,DEXP_BIAS	# if RS is not denormalized then remove the
	or	t2,DIMP_1BIT	#  exponent bias, and set the implied 1 bit
2:	bne	t5,zero,3f	# check for RT being denormalized
	li	t5,-DEXP_BIAS+1	# set denorm's exponent
	jal	rt_renorm_d	# normalize it
	b	4f
3:	subu	t5,DEXP_BIAS	# if RT is not denormalized then remove the
	or	t6,DIMP_1BIT	#  exponent bias, and set the implied 1 bit
4:
	/*
	 * Calculate the exponent of the result to be used by the norm_d:
	 * code to figure the final exponent.
	 */
	subu	t1,t5
	subu	t1,11

	/*
	 * Since the norm_d: code expects the fraction to be in t2,t3,t8
	 * RS's fraction is moved to t4,t5 so to free up t2,t3 to hold
	 * the final quotient of the division.
	 */
	move	t4,t2
	move	t5,t3

	move	v0,zero		# set the number of quotient bits calculated
				#  to zero
	move	v1,zero		# set the number of quotient bits in the
				#  quotient accumulator to zero
	move	t3,zero		# clear the quotient accumulator

1:	bltu	t4,t6,3f	# check if dividend (RS) >= divisor (RT)
	bne	t4,t6,2f
	bltu	t5,t7,3f

2:	# subtract divisor (RT) from dividend (RS)
	sltu	t9,t5,t7	# set the barrow of the low fraction words
	subu	t5,t7		# subtract the low fraction words
	subu	t4,t6		# subtract the high fraction words
	subu	t4,t9		# subtract the barrow
	addu	t3,1		# add one to the quotient accumulator

	bne	t4,zero,3f	# see if division is done (remainder of zero)
	bne	t5,zero,3f
	move	t8,zero		# clear the sticky register (no remainder)
	negu	v1		# shift the quotient accumulator into it's
	addu	v1,31		#  final possition and place in the proper
	sll	t3,v1		#  word of the final quotient and goto norm_d:
	bge	v0,32,norm_d
	move	t2,t3
	move	t3,zero
	b	norm_d

3:	sll	t4,1		# shift the dividend (RS) left one bit
	srl	t9,t5,31	# if the high bit of the low word of the
	sll	t5,1		#  fraction is set get it in to the low bit
	addu	t4,t9		#  of the high word of the fraction

	addu	v0,1		# add one to the number of quotient bits
				#  calculated
	addu	v1,1		# add one to the number of quotient bits
				#  calculated in the quotient accumulator
	blt	v1,32,4f	# see if quotient accumulator is full
	move	t2,t3		# if so place it in the high word of the
	move	t3,zero		#  of the final quotient, clear it and
	move	v1,zero		#  set it's count of bits to zero
	b	1b

4:	sll	t3,1		# shift the quotient accumulator left one bit
	# see if enough quotient bits have been calculated if not continue
	blt	v0,DFRAC_BITS+3,1b

	negu	v1		# shift the quoient accumulator into it's
	addu	v1,31		#  final possition
	sll	t3,v1

	move	t8,t4		# set the sticky register with all the bits of
	or	t8,t5		#  remainder

	b	norm_d

div_e:
div_q:
	b	illfpinst

/*
 * To get to here the FUNC field (t8) was one of the comparison functions.
 * At this point the both the floating-point value for the specified FPR
 * registers of the RS and RT fields have been loaded into GPR registers.
 * What is done next is to decode the FMT field (v0) and branch to the code
 * to compare that format.
 */
comp:
	lw	v1,comp_fmt_tab(v0)
	j	v1

	.rdata
comp_fmt_tab:
	.word	comp_s:1, comp_d:1, comp_e:1, comp_q:1, illfpinst:1
	.text

/*
 * Comparison single RS : RT . After the result of the comparison is determined
 * the predicate to use to set the condition bit is in the low four bits of the
 * FUNC field (t8).
 */
.globl comp_s
comp_s:
	/*
	 * Check either operand for being a NaN.
	 */
	srl	t1,t2,SEXP_SHIFT	# check RS and RT for a NaN
	and	t1,SEXP_MASK
	srl	t5,t6,SEXP_SHIFT
	and	t5,SEXP_MASK
	bne	t1,SEXP_NAN,1f
	and	t9,t2,SFRAC_MASK
	bne	t9,zero,2f
1:	bne	t5,SEXP_NAN,7f
	and	t9,t6,SFRAC_MASK
	beq	t9,zero,7f
	/*
	 * At this point one of the operands is an NaN so the result of
	 * the comparision is unordered.  Set the condition bit with
	 * respect to the predicate.
	 */
2:	move	v0,zero			# a zero exit value (hopefully)
	and	v1,t8,COND_UN_MASK	# set or clear the condition bit
	beq	v1,zero,3f
	or	a3,CSR_CBITSET
	b	4f
3:	and	a3,CSR_CBITCLEAR
	/*
	 * Now see if the invalid exception is to be set.  This can occur
	 * for one of two reasons.  The first if the high bit of the predicate
	 * is set.  Second if either operand is a signaling NaN.
	 */
4:	and	v1,t8,COND_IN_MASK	# see if this predicate causes an
	bne	v1,zero,6f		#  invalid exception is to be set
					#  for unordered comparisons

	bne	t1,SEXP_NAN,5f		# check RS for a signaling NaN
	and	t9,t2,SFRAC_MASK
	beq	t9,zero,5f
	and	v1,t2,SSNANBIT_MASK
	bne	v1,zero,6f
5:	bne	t5,SEXP_NAN,store_fpc_csr # check RT for a signaling NaN
	and	t9,t6,SFRAC_MASK
	beq	t9,zero,store_fpc_csr
	and	v1,t6,SSNANBIT_MASK
	beq	v1,zero,store_fpc_csr
	/*
	 * The set the invalid trap and if it is enabled signal a SIGFPE.
	 */
6:	or	a3,INVALID_EXC
	and	v0,a3,INVALID_ENABLE
	beq	v0,zero,store_fpc_csr
	li	v0,SIGFPE
	jal	post_signal
	li	v0,1			# a non-zero exit value
	b	store_fpc_csr
7:
	/*
	 * Set up to do the comparison by negating and setting the sign bits
	 * of negitive operands.
	 */
	li	v0,SIGNBIT
	and	v1,t2,v0	# check to see if RS is negitive
	beq	v1,zero,1f
	negu	t2
	xor	t2,v0
1:	and	v1,t6,v0	# check to see if RT is negitive
	beq	v1,zero,2f
	negu	t6
	xor	t6,v0
2:
	move	v0,zero		# zero exit value
	/*
	 * Now compare the two operands.
	 */
	blt	t2,t6,2f
	bne	t2,t6,4f
	/*
	 * At this point the comparison is known to be equal so set the
	 * condition bit if the equal condition is being compared for in
	 * the predicate.
	 */
	and	v1,t8,COND_EQ_MASK	# set or clear the condition bit
	beq	v1,zero,1f
	or	a3,CSR_CBITSET
	b	store_fpc_csr
1:	and	a3,CSR_CBITCLEAR
	b	store_fpc_csr
	/*
	 * At this point the comparison is known to be less than so set the
	 * condition bit if the less than condition is being compared for in
	 * the predicate.
	 */
2:	and	v1,t8,COND_LT_MASK	# set or clear the condition bit
	beq	v1,zero,3f
	or	a3,CSR_CBITSET
	b	store_fpc_csr
3:	and	a3,CSR_CBITCLEAR
	b	store_fpc_csr
	/*
	 * At this point the comparison is known to be greater than so clear the
	 * condition bit.
	 */
4:	and	a3,CSR_CBITCLEAR
	b	store_fpc_csr

/*
 * Comparison double RS : RT . After the result of the comparison is determined
 * the predicate to use to set the condition bit is in the low four bits of the
 * FUNC field (t8).
 */
.globl comp_d
comp_d:
	/*
	 * Check either operand for being a NaN.
	 */
	srl	t1,t2,DEXP_SHIFT	# check RS for a NaN
	and	t1,DEXP_MASK
	srl	t5,t6,DEXP_SHIFT	# check RT for a NaN
	and	t5,DEXP_MASK
	bne	t1,DEXP_NAN,1f
	and	t9,t2,DFRAC_MASK
	bne	t9,zero,2f
	bne	t3,zero,2f
1:	bne	t5,DEXP_NAN,9f
	and	t9,t6,DFRAC_MASK
	bne	t9,zero,2f
	beq	t7,zero,9f
	/*
	 * At this point one of the operands is an NaN so the result of
	 * the comparision is unordered.  Set the condition bit with
	 * respect to the predicate.
	 */
2:	move	v0,zero			# a zero exit value (hopefully)
	and	v1,t8,COND_UN_MASK	# set or clear the condition bit
	beq	v1,zero,3f
	or	a3,CSR_CBITSET
	b	4f
3:	and	a3,CSR_CBITCLEAR
	/*
	 * Now see if the invalid exception is to be set.  This can occur
	 * for one of two reasons.  The first if the high bit of the predicate
	 * is set.  Second if either operand is a signaling NaN.
	 */
4:	and	v1,t8,COND_IN_MASK	# see if this predicate causes an
	bne	v1,zero,8f		#  invalid exception is to be set
					#  for unordered comparisons

	bne	t1,DEXP_NAN,6f		# check RS for a signaling NaN
	and	t9,t2,DFRAC_MASK
	bne	t9,zero,5f
	beq	t3,zero,6f
5:	and	v1,t2,DSNANBIT_MASK
	bne	v1,zero,8f
6:	bne	t5,DEXP_NAN,store_fpc_csr # check RT for a signaling NaN
	and	t9,t6,DFRAC_MASK
	bne	t9,zero,7f
	beq	t7,zero,store_fpc_csr
7:	and	v1,t6,DSNANBIT_MASK
	beq	v1,zero,store_fpc_csr
	/*
	 * The set the invalid trap and if it is enabled signal a SIGFPE.
	 */
8:	or	a3,INVALID_EXC
	and	v0,a3,INVALID_ENABLE
	beq	v0,zero,store_fpc_csr
	li	v0,SIGFPE
	jal	post_signal
	li	v0,1			# a non-zero exit value
	b	store_fpc_csr
9:
	/*
	 * Set up to do the comparison by negating and setting the sign bits
	 * of negitive operands.
	 */
	li	v0,SIGNBIT
	and	v1,t2,v0	# check to see if RS is negitive
	beq	v1,zero,2f
	not	t3
	not	t2
	addu	t3,1
	bne	t3,zero,1f
	addu	t2,1
1:	xor	t2,v0
2:	and	v1,t6,v0	# check to see if RT is negitive
	beq	v1,zero,4f
	not	t7
	not	t6
	addu	t7,1
	bne	t7,zero,3f
	addu	t6,1
3:	xor	t6,v0
4:
	move	v0,zero		# zero exit value
	/*
	 * Now compare the two operands.
	 */
	blt	t2,t6,2f
	bne	t2,t6,4f
	bltu	t3,t7,2f
	bne	t3,t7,4f
	/*
	 * At this point the comparison is known to be equal so set the
	 * condition bit if the equal condition is being compared for in
	 * the predicate.
	 */
	and	v1,t8,COND_EQ_MASK	# set or clear the condition bit
	beq	v1,zero,1f
	or	a3,CSR_CBITSET
	b	store_fpc_csr
1:	and	a3,CSR_CBITCLEAR
	b	store_fpc_csr
	/*
	 * At this point the comparison is known to be less than so set the
	 * condition bit if the less than condition is being compared for in
	 * the predicate.
	 */
2:	and	v1,t8,COND_LT_MASK	# set or clear the condition bit
	beq	v1,zero,3f
	or	a3,CSR_CBITSET
	b	store_fpc_csr
3:	and	a3,CSR_CBITCLEAR
	b	store_fpc_csr
	/*
	 * At this point the comparison is known to be greater than so clear the
	 * condition bit.
	 */
4:	and	a3,CSR_CBITCLEAR
	b	store_fpc_csr

comp_e:
comp_q:
	b	illfpinst

/*
 * To get to here the FUNC field (t8) was one of the conversion functions.
 * At this point the floating-point value for the specified FPR register of
 * the RS field has been loaded into GPR registers.  What is done next is to
 * futher decode the FUNC field (t8) for the conversion functions.  First
 * the format the conversion is going to is decoded from the FUNC field (t8).
 * Second the format the conversion is coming from is decoded from the FMT
 * field (v0).
 */
conv:
	subu	t8,C1_FUNC_CVTS		# set up to branch on the format the
	sll	t8,2
	lw	v1,conv_to_tab(t8)	#  the conversion is going to.
	j	v1

	.rdata
conv_to_tab:
	.word	cvts:1, cvtd:1, cvte:1, cvtq:1, cvtw:1
	.text

cvts:
	lw	v1,cvts_from_tab(v0)
	j	v1

	.rdata
cvts_from_tab:
	.word	illfpinst:1, cvts_from_d:1, cvts_from_e:1, cvts_from_q:1
	.word	cvts_from_w:1
	.text

/*
 * Convert to single format from double format.
 */
.globl cvts_from_d
cvts_from_d:
	/*
	 * Break out the operand into it's fields (sign,exp,fraction) and
	 * handle NaN operands by calling rs_breakout_d() and telling it
	 * to convert NaN's to single if it finds one.
	 */
	li	t9,C1_FMT_SINGLE*4
	move	v1,zero
	jal	rs_breakout_d

	/*
	 * Check for infinities, and produce the correct infinity if so.
	 */
	bne	t1,DEXP_INF,1f	# is RS an infinity?
	move	t2,t0		# use the sign of the infinity
	or	t2,SEXP_INF<<SEXP_SHIFT
	move	v0,zero
	b	rd_1w
1:
	/*
	 * Check for zeroes, and produce the correct zero if so.
	 */
	bne	t1,zero,1f	# check RS for a zero value (first the exp)
	bne	t2,zero,1f	# then the high part of the fraction
	bne	t3,zero,1f	# then the low part of the fraction
	move	t2,t0		# use the sign of the zero
	move	v0,zero
	b	rd_1w
1:
	/*
	 * Now that all the NaN, infinity and zero cases have been taken care
	 * of what is left a value that can be converted by setting it up
	 * for norm_s: . For normalized numbers set the implied one and remove
	 * the exponent bias.  For denormalized numbers convert to a normalized
	 * number with the correct exponent.
	 */
	bne	t1,zero,1f	# check for RS being denormalized
	li	t1,-DEXP_BIAS+1	# set denorm's exponent
	jal	rs_renorm_d	# normalize it
	b	2f
1:	subu	t1,DEXP_BIAS	# if RS is not denormalized then remove the
	or	t2,DIMP_1BIT	#  exponent bias, and set the implied 1 bit
2:
	/*
	 * Shift the double fraction over to where a normalized single fraction 
	 * is, and branch to norm_s_noshift: to put the value together as a
	 * single and handle the underflow, overflow and inexact exceptions.
	 */
	sll	t2,32-(DFRAC_BITS-SFRAC_BITS)
	srl	v0,t3,DFRAC_BITS-SFRAC_BITS
	or	t2,v0
	sll	t8,t3,32-(DFRAC_BITS-SFRAC_BITS)
	b	norm_s_noshift

cvts_from_e:
cvts_from_q:
	b	illfpinst

/*
 * Convert to single format from a word
 */
cvts_from_w:
	bne	t2,zero,1f		# check for zero
	move	v0,zero			# a zero exit value
	b	rd_1w
1:	move	t0,zero			# clear sign bit
	bge	t2,zero,2f		# if negitive negate it and set the
	negu	t2			#  sign bit
	li	t0,SIGNBIT
2:
	li	t1,SFRAC_BITS		# set exponent

	/*
	 * Determine where the first one bit is in the fraction (t2).  After
	 * this series of tests the shift count to shift the fraction left so
	 * the first 1 bit is in the high bit will be in t9.  This sequence of
	 * code uses registers v0,v1 and t9 (it could be done with two but
	 * with reorginization this is faster).  
	 */
	move	v0,t2
	move	t9,zero

	srl	v1,v0,16
	bne	v1,zero,1f
	addu	t9,16
	sll	v0,16
1:	srl	v1,v0,24
	bne	v1,zero,2f
	addu	t9,8
	sll	v0,8
2:	srl	v1,v0,28
	bne	v1,zero,3f
	addu	t9,4
	sll	v0,4
3:	srl	v1,v0,30
	bne	v1,zero,4f
	addu	t9,2
	sll	v0,2
4:	srl	v1,v0,31
	bne	v1,zero,5f
	addu	t9,1
5:
	/*
	 * Now that the it is known where the first one bit is calculate the
	 * amount to shift the fraction to put the first one bit in the
	 * implied 1 position (also the amount to adjust the exponent by).
	 * Then adjust the exponent and shift the fraction (the fraction starts
	 * out only in (t2) but ends up as a single fraction in (t2,t8) ).
	 */
	subu	t9,SFRAC_LEAD0S	# the calulated shift amount
	# Check to see if any shift or adjustment is needed
	beq	t9,zero,2f
	subu	t1,t9		# adjust the exponent
	blt	t9,zero,1f	# if the shift amount is negitive shift right
	# Shift the fraction left by the shift amount (t9)
	negu	v0,t9		# shift the fraction left for < 32 bit shifts
	addu	v0,32
	sll	t2,t9
	move	t8,zero
	b	2f
1:	move	v0,t9		# shift the fraction right for < 32 bit shifts
	negu	t9		# Note the shift amount (t9) starts out negative
	addu	v0,32		#  for right shifts.
	sll	t8,t2,v0
	srl	t2,t9
2:
	/*
	 * If the result is inexact then it must be rounded else it can just
	 * be put together.
	 */
	bne	t8,zero,norm_s_noshift

	and	t2,~SIMP_1BIT		# clear the implied 1
	or	t2,t0			# put the sign back in
	addu	t1,SEXP_BIAS		# add back in the exponent bias
	sll	t1,SEXP_SHIFT		# shift the exponent back in place
	or	t2,t1			# put the exponent back in
	move	v0,zero			# a zero exit value
	b	rd_1w

cvtd:
	lw	v1,cvtd_from_tab(v0)
	j	v1

	.rdata
cvtd_from_tab:
	.word	cvtd_from_s:1, illfpinst:1, cvtd_from_e:1, cvtd_from_q:1
	.word	cvtd_from_w:1
	.text

/*
 * Convert to double format from single format.
 */
.globl cvtd_from_s
cvtd_from_s:
	/*
	 * Break out the operand into it's fields (sign,exp,fraction) and
	 * handle NaN operands by calling rs_breakout_s() and telling it
	 * to convert NaN's to double if it finds one.
	 */
	li	t9,C1_FMT_DOUBLE*4
	move	v1,zero
	jal	rs_breakout_s

	/*
	 * Check for infinities, and produce the correct infinity if so.
	 */
	bne	t1,SEXP_INF,1f	# is RS an infinity?
	move	t2,t0		# use the sign of the infinity
	or	t2,DEXP_INF<<DEXP_SHIFT
	move	t3,zero
	move	v0,zero
	b	rd_2w
1:
	/*
	 * Check for zeroes, and produce the correct zero if so.
	 */
	bne	t1,zero,1f	# check RS for a zero value (first the exp)
	bne	t2,zero,1f	# then the fraction
	move	t2,t0		# use the sign of the zero
	move	t3,zero
	move	v0,zero
	b	rd_2w
1:
	/*
	 * Now that all the NaN, infinity and zero cases have been taken care
	 * of what is left a value that can be converted.  For normalized
	 * numbers set the implied one and remove the exponent bias.  For
	 * denormalized numbers convert to a normalized number with the
	 * correct exponent.
	 */
	bne	t1,zero,1f	# check for RS being denormalized
	li	t1,-SEXP_BIAS+1	# set denorm's exponent
	jal	rs_renorm_s	# normalize it
	b	2f
1:	subu	t1,SEXP_BIAS	# if RS is not denormalized then remove the
	or	t2,SIMP_1BIT	#  exponent bias, and set the implied 1 bit
2:
	/*
	 * Shift the single fraction over to where a normalized double fraction 
	 * is, and put the result together as a double.  Note underflow,
	 * overflow and inexact exceptions are not possible.
	 */
	sll	t3,t2,DFRAC_BITS-SFRAC_BITS
	srl	t2,32-(DFRAC_BITS-SFRAC_BITS)

	and	t2,~DIMP_1BIT		# clear the implied 1
	or	t2,t0			# put the sign back in
	addu	t1,DEXP_BIAS		# add back in the exponent bias
	sll	t1,DEXP_SHIFT		# shift the exponent back in place
	or	t2,t1			# put the exponent back in
	move	v0,zero			# a zero exit value
	b	rd_2w

cvtd_from_e:
cvtd_from_q:
	b	illfpinst

/*
 * Convert to double format from a word
 */
cvtd_from_w:
	bne	t2,zero,1f		# check for zero
	move	t3,zero			# if so return a +0
	move	v0,zero			# a zero exit value
	b	rd_2w
1:	move	t0,zero			# clear sign bit
	bge	t2,zero,2f		# if negitive negate it and set the
	negu	t2			#  sign bit
	li	t0,SIGNBIT
2:
	li	t1,DFRAC_BITS-32	# set exponent

	/*
	 * Determine where the first one bit is in the fraction (t2).  After
	 * this series of tests the shift count to shift the fraction left so
	 * the first 1 bit is in the high bit will be in t9.  This sequence of
	 * code uses registers v0,v1 and t9 (it could be done with two but
	 * with reorginization this is faster).  
	 */
	move	v0,t2
	move	t9,zero

	srl	v1,v0,16
	bne	v1,zero,1f
	addu	t9,16
	sll	v0,16
1:	srl	v1,v0,24
	bne	v1,zero,2f
	addu	t9,8
	sll	v0,8
2:	srl	v1,v0,28
	bne	v1,zero,3f
	addu	t9,4
	sll	v0,4
3:	srl	v1,v0,30
	bne	v1,zero,4f
	addu	t9,2
	sll	v0,2
4:	srl	v1,v0,31
	bne	v1,zero,5f
	addu	t9,1
5:
	/*
	 * Now that the it is known where the first one bit is calculate the
	 * amount to shift the fraction to put the first one bit in the
	 * implied 1 position (also the amount to adjust the exponent by).
	 * Then adjust the exponent and shift the fraction (the fraction starts
	 * out only in (t2) but ends up as a double fraction in (t2,t3) ).
	 */
	subu	t9,DFRAC_LEAD0S	# the calulated shift amount
	# Check to see if any shift or adjustment is needed
	move	t3,zero
	beq	t9,zero,2f
	subu	t1,t9		# adjust the exponent
	blt	t9,zero,1f	# if the shift amount is negitive shift right
	# Shift the fraction left by the shift amount (t9)
	negu	v0,t9		# shift the fraction left for < 32 bit shifts
	addu	v0,32
	sll	t2,t9
	move	t3,zero
	b	2f
1:	move	v0,t9		# shift the fraction right for < 32 bit shifts
	negu	t9		# Note the shift amount (t9) starts out negative
	addu	v0,32		#  for right shifts.
	sll	t3,t2,v0
	srl	t2,t9
2:
	/*
	 * Put the result together as a double.  Note underflow,
	 * overflow and inexact exceptions are not possible.
	 */
	and	t2,~DIMP_1BIT		# clear the implied 1
	or	t2,t0			# put the sign back in
	addu	t1,DEXP_BIAS		# add back in the exponent bias
	sll	t1,DEXP_SHIFT		# shift the exponent back in place
	or	t2,t1			# put the exponent back in
	move	v0,zero			# a zero exit value
	b	rd_2w

cvte:
	lw	v1,cvte_from_tab(v0)
	j	v1

	.rdata
cvte_from_tab:
	.word	cvte_from_s:1, cvte_from_d:1, illfpinst:1, cvte_from_q:1
	.word	cvte_from_w:1
	.text

cvte_from_s:
cvte_from_d:
cvte_from_q:
cvte_from_w:
	b	illfpinst

cvtq:
	lw	v1,cvtq_from_tab(v0)
	j	v1

	.rdata
cvtq_from_tab:
	.word	cvtq_from_s:1, cvtq_from_d:1, cvtq_from_e:1, illfpinst:1
	.word	cvtq_from_w:1
	.text

cvtq_from_s:
cvtq_from_d:
cvtq_from_e:
cvtq_from_w:
	b	illfpinst

.globl cvtw
cvtw:
	lw	v1,cvtw_from_tab(v0)
	j	v1

	.rdata
cvtw_from_tab:
	.word	cvtw_from_s:1, cvtw_from_d:1, cvtw_from_e:1, cvtw_from_q:1
	.word	illfpinst:1
	.text

/*
 * Convert to a word from single format
 */
.globl cvtw_from_s
cvtw_from_s:
	/*
	 * Break out the fields of the RS single value in gp register (t2)
	 * into:
	 *	t0 -- sign bit		     (left justified)
	 *	t1 -- exponent		     (right justified, still biased)
	 *	t2 -- fraction bits [23-0]   (implied one bit NOT set)
	 */
	srl	t1,t2,SEXP_SHIFT
	move	t0,t1
	and	t1,SEXP_MASK
	and	t0,SIGNBIT>>SEXP_SHIFT
	sll	t0,SEXP_SHIFT
	and	t2,SFRAC_MASK

	/*
	 * Check to see if this is a NaN or an infinity and set the invalid 
	 * exception in the fpc_csr (a3).  Setup the result depending if the
	 * enable for the invalid exception is set and if the value is a NaN
	 * or and infinity.
	 */
	bne	t1,SEXP_NAN,4f

	or	a3,INVALID_EXC
	and	v0,a3,INVALID_ENABLE
	beq	v0,zero,1f
	/*
	 * The invalid trap was enabled so signal a SIGFPE and leave the
	 * result register unmodified.
	 */
	li	v0,SIGFPE
	jal	post_signal
	li	v0,1			# a non-zero exit value
	b	store_fpc_csr
	/*
	 * The invalid trap was NOT enabled so the result (implementation
	 * dependent) for infinities is the maximum or minimum value and for
	 * NaN's is the maximum positive value.
	 */ 
1:	bne	t2,zero,3f	# is this a NaN?
	# it is an infinity, so see what the sign is and return the result
	bne	t0,zero,2f
	li	t2,WORD_MAX	# plus infinity returns maximum word value
	move	v0,zero
	b	rd_1w
2:	li	t2,WORD_MIN	# minis infinity returns minimum word value
	move	v0,zero
	b	rd_1w
3:	li	t2,WQUIETNAN_LEAST	# NaN's return maximum positive value
	move	v0,zero
	b	rd_1w
4:
	/*
	 * Check the operand for a zero value and return a zero if so.
	 */
	bne	t1,zero,1f
	bne	t2,zero,1f
	# t2 is already is zero so just use it
	move	v0,zero
	b	rd_1w
1:
	/*
	 * Now that all the NaN, infinity and zero cases have been taken care
	 * of what is left are values that can be converted.  For normalized
	 * numbers set the implied one and remove the exponent bias.  For
	 * denormalized numbers the result is an inexact zero.
	 */
	bne	t1,zero,1f	# check for RS being denormalized
	move	t2,zero		# load the zero return value
	li	t3,STKBIT	# set the sticky bit
	b	cvtw_round	# branch to round
1:	subu	t1,SEXP_BIAS	# if RS is not denormalized then remove the
	or	t2,SIMP_1BIT	#  exponent bias, and set the implied 1 bit

	/*
	 * If the value is too small to have any integer digits then
	 * just set to zero with a sticky bit and round.
	 */
	bge	t1,WEXP_MIN,1f
	move	t2,zero		# load the zero value
	li	t3,STKBIT	# load the sticky bit
	b	cvtw_round	# branch to round it
1:
	/*
	 * If the exponent is too large then branch to set the overflow
	 * exception and to determine the result.  Note it is still possible
	 * to overflow after rounding in one case.
	 */
	/* JRP HACK:  See if this was in fact an unsigned conversion.
	 * Unsigned conversions get an extra bit of precision before
	 * we consider them to be overflows.  What we do is find the
	 * instruction where we took the exception, then look at the
	 * PREVIOUS instruction.  The assembler generates a special
	 * "no-op" instruction just before the cvt; for unsigned
	 * conversion, the instruction is 0x00000003 (sra zero,zero,0),
	 * and for signed conversion it's 0x00000002 (srl zero,zero,0).
	 *
	 * Registers:
	 *      t9      Comparison value
	 *      t8      Address of instruction
	 *      t7      Contents of (t8)
	 */
	cfc0    t8,C0_EPC       # Get the exception PC
	sub     t8,4            # Point to previous instruction
	lw      t7,0(t8)        # Get the instruction itself.
	li      t9,0x00000003   # Magic value for unsigned conversion.
	bne     t7,t9,8f
	
	bgt     t1,WEXP_MAX+2,cvtw_overflow
	bne     t1,WEXP_MAX+2,1f
	j       9f
	
8:	bgt     t1,WEXP_MAX+1,cvtw_overflow
	/*
	 * A special check is needed for -1.0*2^31 so that it does not
	 * indicate an overflow and just returns the minimum word value.
	 */
	bne	t1,WEXP_MAX+1,1f
9:	bne	t2,SIMP_1BIT,cvtw_overflow
	beq	t0,zero,cvtw_overflow
	li	t2,WORD_MIN
	move	v0,zero
	b	rd_1w
1:
	/*
	 * Now shift the fraction so it is a fix point value.
	 */
	subu	t9,t1,SFRAC_BITS	# calculate the shift amount
	# Check to see if any shift is needed
	move	t3,zero		# clear the sticky register
	beq	t9,zero,cvtw_round
	blt	t9,zero,2f	# if the shift amount is negative shift right
	# Shift the fraction left by the shift amount (t9)
1:	negu	v0,t9		# shift the fraction left for < 32 bit shifts
	addu	v0,32
	sll	t2,t9
	b	cvtw_round
2:	move	v0,t9		# shift the fraction right for < 32 bit shifts
	negu	t9		# Note the shift amount (t9) starts out negative
	addu	v0,32		#  for right shifts.
	sll	t3,t2,v0
	srl	t2,t9
	b	cvtw_round

/*
 * Convert to a word from double format
 */
.globl cvtw_from_d
cvtw_from_d:
	/*
	 * Break out the fields of the RS double value in gp registers (t2,t3)
	 * into:
	 *	t0 -- sign bit		     (left justified)
	 *	t1 -- exponent		     (right justified, still biased)
	 *	t2 -- fraction bits [51-32]  (implied one bit NOT set)
	 *	t3 -- fraction bits [31-0]
	 */
	srl	t1,t2,DEXP_SHIFT
	move	t0,t1
	and	t1,DEXP_MASK
	and	t0,SIGNBIT>>DEXP_SHIFT
	sll	t0,DEXP_SHIFT
	and	t2,DFRAC_MASK

	/*
	 * Check to see if this is a NaN or an infinity and set the invalid 
	 * exception in the fpc_csr (a3).  Setup the result depending if the
	 * enable for the invalid exception is set and if the value is a NaN
	 * or and infinity.
	 */
	bne	t1,DEXP_NAN,4f

	or	a3,INVALID_EXC
	and	v0,a3,INVALID_ENABLE
	beq	v0,zero,1f
	/*
	 * The invalid trap was enabled so signal a SIGFPE and leave the
	 * result register unmodified.
	 */
	li	v0,SIGFPE
	jal	post_signal
	li	v0,1			# a non-zero exit value
	b	store_fpc_csr
	/*
	 * The invalid trap was NOT enabled so the result (implementation
	 * dependent) for infinities is the maximum or minimum value and for
	 * NaN's is the maximum positive value.
	 */ 
1:	bne	t2,zero,3f	# is this a NaN?
	bne	t3,zero,3f
	# it is an infinity, so see what the sign is and return the result
	bne	t0,zero,2f
	li	t2,WORD_MAX	# plus infinity returns maximum word value
	move	v0,zero
	b	rd_1w
2:	li	t2,WORD_MIN	# minis infinity returns minimum word value
	move	v0,zero
	b	rd_1w
3:	li	t2,WQUIETNAN_LEAST	# NaN's return maximum positive value
	move	v0,zero
	b	rd_1w
4:
	/*
	 * Check the operand for a zero value and return a zero if so.
	 */
	bne	t1,zero,1f
	bne	t2,zero,1f
	bne	t3,zero,1f
	# t2 is already is zero so just use it
	move	v0,zero
	b	rd_1w
1:
	/*
	 * Now that all the NaN, infinity and zero cases have been taken care
	 * of what is left are values that can be converted.  For normalized
	 * numbers set the implied one and remove the exponent bias.  For
	 * denormalized numbers the result is an inexact zero.
	 */
	bne	t1,zero,1f	# check for RS being denormalized
	move	t2,zero		# load the zero return value
	li	t3,STKBIT	# set the sticky bit
	b	cvtw_round	# branch to round
1:	subu	t1,DEXP_BIAS	# if RS is not denormalized then remove the
	or	t2,DIMP_1BIT	#  exponent bias, and set the implied 1 bit

	/*
	 * If the value is too small to have any integer digits then
	 * just set to zero with a sticky bit and round.
	 */
	bge	t1,WEXP_MIN,1f
	move	t2,zero		# load the zero value
	li	t3,STKBIT	# load the sticky bit
	b	cvtw_round	# branch to round it
1:
	/*
	 * If the exponent is too large then branch to set the overflow
	 * exception and to determine the result.  Note it is still possible
	 * to overflow after rounding in one case.
	 */
	/* JRP HACK:  See if this was in fact an unsigned conversion.
	 * Unsigned conversions get an extra bit of precision before
	 * we consider them to be overflows.  What we do is find the
	 * instruction where we took the exception, then look at the
	 * PREVIOUS instruction.  The assembler generates a special
	 * "no-op" instruction just before the cvt; for unsigned
	 * conversion, the instruction is 0x00000003 (sra zero,zero,0),
	 * and for signed conversion it's 0x00000002 (srl zero,zero,0).
	 *
	 * Registers:
	 *      t9      Comparison value
	 *      t8      Address of instruction
	 *      t7      Contents of (t8)
	 */
	cfc0    t8,C0_EPC       # Get the exception PC
	sub     t8,4            # Point to previous instruction
	lw      t7,0(t8)        # Get the instruction itself.
	li      t9,0x00000003   # Magic value for unsigned conversion.
	bne     t7,t9,8f
	
	bgt     t1,WEXP_MAX+2,cvtw_overflow
	bne     t1,WEXP_MAX+2,1f
	j       9f
	
8:	bgt     t1,WEXP_MAX+1,cvtw_overflow
	/*
	 * A special check is needed for -1.0*2^31 so that it does not
	 * indicate an overflow and just returns the minimum word value.
	 */
	bne	t1,WEXP_MAX+1,1f
9:	bne	t2,DIMP_1BIT,cvtw_overflow
	bne	t3,zero,cvtw_overflow
	beq	t0,zero,cvtw_overflow
	li	t2,WORD_MIN
	move	v0,zero
	b	rd_1w
1:

	/*
	 * Now shift the fraction so it is a fix point value.
	 */
	subu	t9,t1,DFRAC_BITS-32	# calculate the shift amount
	# Check to see if any shift is needed
	beq	t9,zero,4f
	blt	t9,zero,2f	# if the shift amount is negative shift right
	# Shift the fraction left by the shift amount (t9)
1:	negu	v0,t9		# shift the fraction left for < 32 bit shifts
	addu	v0,32
	sll	t2,t9
	srl	v1,t3,v0
	or	t2,v1
	sll	t3,t9
	b	4f
2:	move	v0,t9		# shift the fraction right for < 32 bit shifts
	negu	t9		# Note the shift amount (t9) starts out negative
	addu	v0,32		#  for right shifts.
	sll	v1,t3,v0
	srl	t3,t9
	beq	v1,zero,3f
	or	t3,STKBIT
3:	sll	v1,t2,v0
	or	t3,v1
	srl	t2,t9
	b	cvtw_round

/*
 * cvtw_round finishes the conversions to words (fixed point) by rounding
 * the fixed point value and coverting it to 2-complement form.  It takes as
 * input:
 *	t0 -- sign bit		     (left justified)
 *	t2 -- fixed point value
 *	t3 -- fraction value	     (including the sticky bit)
 * It handles the inexact exceptions that may happen and the one case that
 * the overflow exception can occur due to rounding.
 */
cvtw_round:
	/*
	 * Now round the fixed point result.
	 */
	and	v0,a3,CSR_RM_MASK	# get the rounding mode
	beq	v0,CSR_RM_RN,3f		# round to nearest
	beq	v0,CSR_RM_RZ,5f		# round to zero (truncate)
	beq	v0,CSR_RM_RPI,1f	# round to plus infinity
	# Round to minus infinity
	beq	t0,zero,5f		# if the sign is plus truncate
	b	2f
1:	# Round to plus infinity
	bne	t0,zero,5f		# if the sign is minus truncate
2:	beq	t3,zero,5f		# if there are no fraction bits go on
	addu	t2,1
	beq	t2,SIGNBIT,cvtw_overflow	# if overflow then branch
	b	5f
	# Round to nearest
3:	li	v0,GUARDBIT		# load the guard bit for rounding
	not	v1,t3			# set carry out for addition of the
	sltu	t9,v1,v0		#  the sticky register and guard bit
	addu	v0,t3
	beq	t9,zero,4f		# if there was no carry out go on
	addu	t2,1
	beq	t2,SIGNBIT,cvtw_overflow	# if overflow then branch
4:	bne	v0,zero,5f		# if sticky register is zero clear the
	li	v1,~1			#  last bit in the fraction (round to
	and	t2,v1			#  nearest)
5:
	/*
	 * If the value is negative negate it.
	 */
	beq	t0,zero,1f
	negu	t2
1:
	/*
	 * Check for the inexact exception and exit.
	 */
	move	v0,zero			# a zero exit value (hopefully)
	beq	t3,zero,1f		# check for inexact exception
cvtw_inexact:
	or	a3,INEXACT_EXC		# set the inexact exception
	and	v1,a3,INEXACT_ENABLE	# see if inexact trap is enabled
	beq	v1,zero,1f		# if it is enabled post a signal
	li	v0,SIGFPE
	jal	post_signal
	li	v0,1			# a non-zero exit value
1:
	b	rd_1w

/*
 * Overflows (which are invalid not overflow) on conversions to fixed point
 * which are trapped (implementation dependent) will delivered to the trap
 * handler the floating point value in widest supported format rounded to
 * fixed point.  This conversion is done in the signal handler (since there's
 * no place to put the widest supported format) and the result register is left
 * unmodified here.  If the invalid exception is enabled signal a SIGFPE and
 * leave the result register unmodified.  If the invalid exception is not
 * enabled then return the NaN value for a word.
 */
cvtw_overflow:
	or	a3,INVALID_EXC		# set the invalid exception
	and	v0,a3,INVALID_ENABLE	# see if the invalid trap is enabled
	beq	v0,zero,1f		# if it is enabled post a signal
	li	v0,SIGFPE
	jal	post_signal
	li	v0,1			# a non-zero exit value
	b	store_fpc_csr		# don't modify the result register
1:
	/*
	 * The invalid trap was NOT enabled so the result (implementation
	 * dependent) is the NaN value for a word.
	 */ 
	li	t2,WQUIETNAN_LEAST	
	move	v0,zero
	b	rd_1w

cvtw_from_e:
cvtw_from_q:
	b	illfpinst

/*
 * Normalize a single value and handle the overflow, underflow and inexact
 * exceptions that may arise.  The input single value is as follows:
 *	t0 -- sign bit		     (left justified)
 *	t1 -- exponent		     (right justified, not biased)
 *	t2 -- fraction bits [22-0]   (implied one bit set)
 *	t8 -- fraction sticky bits
 */
.globl norm_s
norm_s:
	/*
	 * Determine the ammount to shift the fraction and adjust the exponent
	 * so the first one bit is in the implied 1 position.
	 */
	/*
	 * The first step in this process is to determine where the first
	 * one bit is in the fraction (t2,t8).  After this series of tests
	 * the shift count to shift the fraction left so the first 1 bit is
	 * in the high bit will be in t9.  This sequence of code uses registers
	 * v0,v1 and t9 (it could be done with two but with reorginization this
	 * is faster).  Note it is not possible for the first one bit to be in
	 * the sticky register (t8) but it does participate in the shift.
	 */
	move	v0,t2
	move	t9,zero
	bne	t2,zero,1f
	move	v0,t8
	addu	t9,32
1:

	srl	v1,v0,16
	bne	v1,zero,1f
	addu	t9,16
	sll	v0,16
1:	srl	v1,v0,24
	bne	v1,zero,2f
	addu	t9,8
	sll	v0,8
2:	srl	v1,v0,28
	bne	v1,zero,3f
	addu	t9,4
	sll	v0,4
3:	srl	v1,v0,30
	bne	v1,zero,4f
	addu	t9,2
	sll	v0,2
4:	srl	v1,v0,31
	bne	v1,zero,5f
	addu	t9,1
5:
	/*
	 * Now that the it is known where the first one bit is calculate the
	 * amount to shift the fraction to put the first one bit in the
	 * implied 1 position (also the amount to adjust the exponent by).
	 * Then adjust the exponent and shift the fraction.
	 */
	subu	t9,SFRAC_LEAD0S	# the calulated shift amount
	# Check to see if any shift or adjustment is needed
	beq	t9,zero,norm_s_noshift
	subu	t1,t9		# adjust the exponent
	blt	t9,zero,2f	# if the shift amount is negitive shift right
	# Shift the fraction left by the shift amount (t9)
1:	negu	v0,t9		# shift the fraction left for < 32 bit shifts
	addu	v0,32
	sll	t2,t9
	srl	v1,t8,v0
	or	t2,v1
	sll	t8,t9
	b	4f
2:	move	v0,t9		# shift the fraction right for < 32 bit shifts
	negu	t9		# Note the shift amount (t9) starts out negative
	addu	v0,32		#  for right shifts.
	beq	t8,zero,3f
	or	t8,STKBIT
3:	srl	t8,t9
	sll	v1,t2,v0
	or	t8,v1
	srl	t2,t9
4:
	/*
	 * This point can be branched to instead of norm_s if it is known that
	 * the value is normalized.
	 */
norm_s_noshift:

	/*
	 * Now round the result.  The unrounded result (exponent and fraction)
	 * must be saved in the case of untrapped underflow so a correct
	 * denormalized number can be produced with only one rounding.
	 */
	move	t5,t1			# save unrounded exponent
	move	t6,t2			# save unrounded fraction
	and	v0,a3,CSR_RM_MASK	# get the rounding mode
	beq	v0,CSR_RM_RN,3f		# round to nearest
	beq	v0,CSR_RM_RZ,5f		# round to zero (truncate)
	beq	v0,CSR_RM_RPI,1f	# round to plus infinity
	# Round to minus infinity
	beq	t0,zero,5f		# if the sign is plus truncate
	b	2f
1:	# Round to plus infinity
	bne	t0,zero,5f		# if the sign is minus truncate
2:	beq	t8,zero,5f		# if not inexact go on
	addu	t2,1
	bne	t2,SIMP_1BIT<<1,5f	# see if the carry requires an exponent
	addu	t1,1			#  adjustment and the fraction to be
	srl	t2,1			#  shifted
	b	5f
	# Round to nearest
3:	li	v0,GUARDBIT		# load the guard bit for rounding
	not	v1,t8			# set carry out for addition of the
	sltu	t9,v1,v0		#  the sticky register and guard bit
	addu	v0,t8
	beq	t9,zero,4f		# if there was no carry out go on
	addu	t2,1
	bne	t2,SIMP_1BIT<<1,4f	# see if the carry requires an exponent
	addu	t1,1			#  adjustment and the fraction to be
	srl	t2,1			#  shifted
4:	bne	v0,zero,5f		# if sticky register is zero clear the
	li	v1,~1			#  last bit in the fraction (round to
	and	t2,v1			#  nearest)
5:
	/*
	 * Now check for overflow and produce the correct result for both the
	 * trapped and untrapped cases.
	 */
	ble	t1,SEXP_MAX,9f		# branch if no overflow
	or	a3,OVERFLOW_EXC		# set the overflow flags in fpc_csr (a3)
	and	v0,a3,OVERFLOW_ENABLE	# see if overflow trap is enabled
	beq	v0,zero,1f
	/*
	 * The overflow trap was enabled so signal a SIGFPE and put the correct
	 * result in the destination register.
	 */
	li	v0,SIGFPE
	jal	post_signal
	subu	t1,SEXP_OU_ADJ-SEXP_BIAS # adjust the exponent down
	sll	t1,SEXP_SHIFT
	and	t2,~SIMP_1BIT		# clear implied 1
	or	t2,t0			# put the sign back in
	or	t2,t1			# put the exponent back in
	beq	t8,zero,$1000		# check for inexact exception
	or	a3,INEXACT_EXC		# set the inexact exception
$1000:	li	v0,1			# a non-zero exit value
	b	rd_1w
	/*
	 * The overflow trap was not enabled so just put the correct
	 * result in the destination register according to the rounding
	 * mode.  Also set the inexact trap and if it is enabled signal a
	 * SIGFPE.
	 */
1:	move	v0,zero			# zero exit value (hopefully)
	or	a3,INEXACT_EXC		# set the inexact exception
	and	v1,a3,INEXACT_ENABLE	# see if inexact trap is enabled
	beq	v1,zero,2f		# if it is enabled post a signal
	li	v0,SIGFPE
	jal	post_signal
	li	v0,1			# a non-zero exit value
2:	and	v1,a3,CSR_RM_MASK	# get the rounding mode
	beq	v1,CSR_RM_RN,8f		# round to nearest
	beq	v1,CSR_RM_RZ,7f		# round to zero (truncate)
	beq	v1,CSR_RM_RPI,5f	# round to plus infinity
	/*
	 * Round to minus infinity caries negative results to minus infinity and
	 * positive results to the format's largest positive finite number.
	 */
3:	beq	t0,zero,4f
	li	t2,SIGNBIT|(SEXP_INF<<SEXP_SHIFT)
	b	rd_1w
4:	li	t2,((SEXP_MAX+SEXP_BIAS)<<SEXP_SHIFT)|SFRAC_LEAST_MAX
	b	rd_1w
	/*
	 * Round to plus infinity caries positive results to plus infinity and
	 * negative results to the format's largest negative finite number.
	 */
5:	bne	t0,zero,6f
	li	t2,SEXP_INF<<SEXP_SHIFT
	b	rd_1w
6:	li	t2,SIGNBIT|((SEXP_MAX+SEXP_BIAS)<<SEXP_SHIFT)|SFRAC_LEAST_MAX
	b	rd_1w
	/*
	 * Round to zero caries the result to the format's largest finite
	 * number with the sign of the result.
	 */
7:	or	t2,t0,((SEXP_MAX+SEXP_BIAS)<<SEXP_SHIFT)|SFRAC_LEAST_MAX
	b	rd_1w
	/*
	 * Round to nearest caries the result to infinity with the sign of the
	 * result.
	 */
8:	or	t2,t0,SEXP_INF<<SEXP_SHIFT
	b	rd_1w
9:
	/*
	 * Now check for underflow and produce the correct result for both the
	 * trapped and untrapped cases.  In the Mips implemention "tininess"
	 * is detected "after rounding" and "loss of accuracy" is detected as
	 * "an inexact result".
	 */
	/*
	 * If underflow is signaled differently if the underflow trap is
	 * enabled or not enabled.  So see if the trap is enabled.
	 */
	and	v0,a3,UNDERFLOW_ENABLE	# see if underflow trap is enabled
	beq	v0,zero,2f		# branch if the trap is not enabled
	/*
	 * The underflow trap is enabled so the underflow is to be signaled
	 * when "tininess" is detected regardless of "loss of accuracy".
	 */
	bge	t1,SEXP_MIN,$3000	# check for tininess
	/*
	 * Underflow has occured and the underflow trap was enabled so signal a
	 * SIGFPE and put the correct result in the destination register.
	 */
	or	a3,UNDERFLOW_EXC
	li	v0,SIGFPE
	jal	post_signal
	addu	t1,SEXP_OU_ADJ+SEXP_BIAS # adjust the exponent up
	sll	t1,SEXP_SHIFT
	and	t2,~SIMP_1BIT		# clear implied one bit
	or	t2,t0			# put the sign back in
	or	t2,t1			# put the exponent back in
	beq	t8,zero,1f		# check for inexact exception
	or	a3,INEXACT_EXC		# set the inexact exception
1:	li	v0,1			# a non-zero exit value
	b	rd_1w
	/*
	 * The underflow trap is not enabled so the underflow is to be signaled
	 * when both "tininess" and "loss of accuracy" is detected.
	 */
2:	bge	t1,SEXP_MIN,$3000	# check for tininess
	/*
	 * Now that tininess has occrued the number will be a denormalized
	 * number or zero.  So produce the denormalized or zero value from
	 * the unrounded result and then check for "loss of accuracy" to
	 * see if underflow is to be detected.
	 */
	move	t1,t5			# get the unrounded exponent
	move	t2,t6			# get the unrounded fraction
	subu	t1,SEXP_MIN		# calculate the shift amnount to
	negu	t1			#  make the value a denormalized value
	blt	t1,SFRAC_BITS+2,3f	# if the shift amount would shift out
	move	t2,zero			#  all the fraction bits then the result
	li	t8,STKBIT		#  will be an inexact zero.
	b 	7f
3:	negu	v0,t1			# shift the fraction < 32 bits
	addu	v0,32
	sll	v1,t8,v0		# make sure the sticky bit stays set
	srl	t8,t1
	beq	v1,zero,6f
	or	t8,STKBIT
6:	sll	v1,t2,v0
	or	t8,v1
	srl	t2,t1
7:
	/*
	 * Now round the denormalized result.
	 */
	and	v0,a3,CSR_RM_MASK	# get the rounding mode
	beq	v0,CSR_RM_RN,3f		# round to nearest
	beq	v0,CSR_RM_RZ,$2000	# round to zero (truncate)
	beq	v0,CSR_RM_RPI,1f	# round to plus infinity
	# Round to minus infinity
	beq	t0,zero,$2000		# if the sign is plus truncate
	b	2f
1:	# Round to plus infinity
	bne	t0,zero,$2000		# if the sign is minus truncate
2:	beq	t8,zero,$2000		# if not inexact go on
	addu	t2,1
	b	$2000
	# Round to nearest
3:	li	v0,GUARDBIT		# load the guard bit for rounding
	not	v1,t8			# set carry out for addition of the
	sltu	t9,v1,v0		#  the sticky register and guard bit
	addu	v0,t8
	beq	t9,zero,4f		# if there was no carry out go on
	addu	t2,1
4:	bne	v0,zero,$2000		# if sticky register is zero clear the
	li	v1,~1			#  last bit in the fraction (round to
	and	t2,v1			#  nearest)
	/*
	 * At this point "tininess" has been detected so now if "loss of
	 * accurcy" has also occured then underflow is has occured. (the
	 * detection of underflow in untrapped underflow case).
	 */
$2000:	beq	t8,zero,7f		# test for "loss of accurcy"
	or	a3,UNDERFLOW_EXC	# set the underflow exception
	or	a3,INEXACT_EXC		# set the inexact exception
	and	v0,a3,INEXACT_ENABLE	# see if inexact trap is enabled
	beq	v0,zero,7f		# if it is enabled post a signal
	li	v0,SIGFPE
	jal	post_signal
	/*
	 * Now put together the denormalized or zero result and exit. Note the
	 * exponet field is always zero, and there never is an implied 1 bit.
	 */
	or	t2,t0			# put back the sign
	li	v0,1			# a non-zero exit value
	b	rd_1w
	/*
	 * Now put together the denormalized or zero result and exit. Note the
	 * exponet field is always zero, and there never is an implied 1 bit.
	 */
7:	or	t2,t0			# put back the sign
	move	v0,zero			# a zero exit value
	b	rd_1w

	/*
	 * Now check for inexact exception, set the inexact exception if so and
	 * if the trap is enabled signal a SIGFPE.
	 */
$3000:	move	v0,zero			# a zero exit value (hopefully)
	beq	t8,zero,1f		# check for inexact exception
	or	a3,INEXACT_EXC		# set the inexact exception
	and	v0,a3,INEXACT_ENABLE	# see if inexact trap is enabled
	beq	v0,zero,1f		# if it is enabled post a signal
	li	v0,SIGFPE
	jal	post_signal
	li	v0,1			# a non-zero exit value
1:
	and	t2,~SIMP_1BIT		# clear the implied 1
	or	t2,t0			# put the sign back in
	addu	t1,SEXP_BIAS		# add back in the exponent bias
	sll	t1,SEXP_SHIFT		# shift the exponent back in place
	or	t2,t1			# put the exponent back in
	b	rd_1w

/*
 * Normalize a double value and handle the overflow, underflow and inexact
 * exceptions that may arise.  The input double value is as follows:
 *	t0 -- sign bit		     (left justified)
 *	t1 -- exponent		     (right justified, not biased)
 *	t2 -- fraction bits [51-32]  (implied one bit set)
 *	t3 -- fraction bits [31-0]
 *	t8 -- fraction sticky bits
 */
.globl norm_d
norm_d:
	/*
	 * Determine the ammount to shift the fraction and adjust the exponent
	 * so the first one bit is in the implied 1 position.
	 */
	/*
	 * The first step in this process is to determine where the first
	 * one bit is in the fraction (t2,t3,t8).  After this series of tests
	 * the shift count to shift the fraction left so the first 1 bit is
	 * in the high bit will be in t9.  This sequence of code uses registers
	 * v0,v1 and t9 (it could be done with two but with reorginization this
	 * is faster).  Note it is not possible for the first one bit to be in
	 * the sticky register (t8) but it does participate in the shift.
	 */
	move	v0,t2
	move	t9,zero
	bne	t2,zero,1f
	move	v0,t3
	addu	t9,32
	bne	t3,zero,1f
	move	v0,t8
	addu	t9,32
1:
	srl	v1,v0,16
	bne	v1,zero,1f
	addu	t9,16
	sll	v0,16
1:	srl	v1,v0,24
	bne	v1,zero,2f
	addu	t9,8
	sll	v0,8
2:	srl	v1,v0,28
	bne	v1,zero,3f
	addu	t9,4
	sll	v0,4
3:	srl	v1,v0,30
	bne	v1,zero,4f
	addu	t9,2
	sll	v0,2
4:	srl	v1,v0,31
	bne	v1,zero,5f
	addu	t9,1
5:
	/*
	 * Now that the it is known where the first one bit is calculate the
	 * amount to shift the fraction to put the first one bit in the
	 * implied 1 position (also the amount to adjust the exponent by).
	 * Then adjust the exponent and shift the fraction.
	 */
	subu	t9,DFRAC_LEAD0S	# the calulated shift amount
	# Check to see if any shift or adjustment is needed
	beq	t9,zero,norm_d_noshift
	subu	t1,t9		# adjust the exponent
	blt	t9,zero,2f	# if the shift amount is negitive shift right
	# Shift the fraction left by the shift amount (t9)
	blt	t9,32,1f
	subu	t9,32		# shift the fraction left for >= 32 bit shifts
	negu	v0,t9
	addu	v0,32
	sll	t2,t3,t9
	srl	v1,t8,v0
	or	t2,v1
	sll	t3,t8,t9
	move	t8,zero
	b	4f
1:	negu	v0,t9		# shift the fraction left for < 32 bit shifts
	addu	v0,32
	sll	t2,t9
	srl	v1,t3,v0
	or	t2,v1
	sll	t3,t9
	srl	v1,t8,v0
	or	t3,v1
	sll	t8,t9
	b	4f
2:	move	v0,t9		# shift the fraction right for < 32 bit shifts
	negu	t9		# Note the shift amount (t9) starts out negative
	addu	v0,32		#  for right shifts.
	beq	t8,zero,3f
	or	t8,STKBIT
3:	srl	t8,t9
	sll	v1,t3,v0
	or	t8,v1
	srl	t3,t9
	sll	v1,t2,v0
	or	t3,v1
	srl	t2,t9
4:
	/*
	 * This point can be branched to instead of norm_d if it is known that
	 * the value is normalized.
	 */
norm_d_noshift:

	/*
	 * Now round the result.  The unrounded result (exponent and fraction)
	 * must be saved in the case of untrapped underflow so a correct
	 * denormalized number can be produced with only one rounding.
	 */
	move	t5,t1			# save unrounded exponent
	move	t6,t2			# save unrounded fraction (less)
	move	t7,t3			# save unrounded fraction (least)
	and	v0,a3,CSR_RM_MASK	# get the rounding mode
	beq	v0,CSR_RM_RN,3f		# round to nearest
	beq	v0,CSR_RM_RZ,5f		# round to zero (truncate)
	beq	v0,CSR_RM_RPI,1f	# round to plus infinity
	# Round to minus infinity
	beq	t0,zero,5f		# if the sign is plus truncate
	b	2f
1:	# Round to plus infinity
	bne	t0,zero,5f		# if the sign is minus truncate
2:	beq	t8,zero,5f		# if not inexact go on
	addu	t3,1
	bne	t3,zero,5f		# if there was no carry out go on
	addu	t2,1
	bne	t2,DIMP_1BIT<<1,5f	# see if the carry requires an exponent
	addu	t1,1			#  adjustment and the fraction to be
	srl	t2,1			#  shifted
	b	5f
	# Round to nearest
3:	li	v0,GUARDBIT		# load the guard bit for rounding
.globl point4
point4:
	not	v1,t8			# set carry out for addition of the
	sltu	t9,v1,v0		#  the sticky register and guard bit
	addu	v0,t8
	beq	t9,zero,4f		# if there was no carry out go on
	addu	t3,1
	bne	t3,zero,4f		# if there was no carry out go on
	addu	t2,1
	bne	t2,DIMP_1BIT<<1,4f	# see if the carry requires an exponent
	addu	t1,1			#  adjustment and the fraction to be
	srl	t2,1			#  shifted
4:	bne	v0,zero,5f		# if sticky register is zero clear the
	li	v1,~1			#  last bit in the fraction (round to
	and	t3,v1			#  nearest)
5:
	/*
	 * Now check for overflow and produce the correct result for both the
	 * trapped and untrapped cases.
	 */
	ble	t1,DEXP_MAX,9f		# branch if no overflow
	or	a3,OVERFLOW_EXC		# set the overflow flags in fpc_csr (a3)
	and	v0,a3,OVERFLOW_ENABLE	# see if overflow trap is enabled
	beq	v0,zero,1f
	/*
	 * The overflow trap was enabled so signal a SIGFPE and put the correct
	 * result in the destination register.
	 */
	li	v0,SIGFPE
	jal	post_signal
	subu	t1,DEXP_OU_ADJ-DEXP_BIAS # adjust the exponent down
	sll	t1,DEXP_SHIFT
	and	t2,~DIMP_1BIT		# clear implied 1
	or	t2,t0			# put the sign back in
	or	t2,t1			# put the exponent back in
	beq	t8,zero,$100		# check for inexact exception
	or	a3,INEXACT_EXC		# set the inexact exception
$100:	li	v0,1			# a non-zero exit value
	b	rd_2w
	/*
	 * The overflow trap was not enabled so just put the correct
	 * result in the destination register according to the rounding
	 * mode.  Also set the inexact trap and if it is enabled signal a
	 * SIGFPE.
	 */
1:	move	v0,zero			# zero exit value (hopefully)
	or	a3,INEXACT_EXC		# set the inexact exception
	and	v1,a3,INEXACT_ENABLE	# see if inexact trap is enabled
	beq	v1,zero,2f		# if it is enabled post a signal
	li	v0,SIGFPE
	jal	post_signal
	li	v0,1			# a non-zero exit value
2:	and	v1,a3,CSR_RM_MASK	# get the rounding mode
	beq	v1,CSR_RM_RN,8f		# round to nearest
	beq	v1,CSR_RM_RZ,7f		# round to zero (truncate)
	beq	v1,CSR_RM_RPI,5f	# round to plus infinity
	/*
	 * Round to minus infinity caries negative results to minus infinity and
	 * positive results to the format's largest positive finite number.
	 */
3:	beq	t0,zero,4f
	li	t2,SIGNBIT|(DEXP_INF<<DEXP_SHIFT)
	move	t3,zero
	b	rd_2w
4:	li	t2,((DEXP_MAX+DEXP_BIAS)<<DEXP_SHIFT)|DFRAC_LESS_MAX
	li	t3,DFRAC_LEAST_MAX
	b	rd_2w
	/*
	 * Round to plus infinity caries positive results to plus infinity and
	 * negative results to the format's largest negative finite number.
	 */
5:	bne	t0,zero,6f
	li	t2,DEXP_INF<<DEXP_SHIFT
	move	t3,zero
	b	rd_2w
6:	li	t2,SIGNBIT|((DEXP_MAX+DEXP_BIAS)<<DEXP_SHIFT)|DFRAC_LESS_MAX
	li	t3,DFRAC_LEAST_MAX
	b	rd_2w
	/*
	 * Round to zero caries the result to the format's largest finite
	 * number with the sign of the result.
	 */
7:	or	t2,t0,((DEXP_MAX+DEXP_BIAS)<<DEXP_SHIFT)|DFRAC_LESS_MAX
	li	t3,DFRAC_LEAST_MAX
	b	rd_2w
	/*
	 * Round to nearest caries the result to infinity with the sign of the
	 * result.
	 */
8:	or	t2,t0,DEXP_INF<<DEXP_SHIFT
	move	t3,zero
	b	rd_2w
9:
	/*
	 * Now check for underflow and produce the correct result for both the
	 * trapped and untrapped cases.  In the Mips implemention "tininess"
	 * is detected "after rounding" and "loss of accuracy" is detected as
	 * "an inexact result".
	 */
	/*
	 * If underflow is signaled differently if the underflow trap is
	 * enabled or not enabled.  So see if the trap is enabled.
	 */
	and	v0,a3,UNDERFLOW_ENABLE	# see if underflow trap is enabled
	beq	v0,zero,2f		# branch if the trap is not enabled
	/*
	 * The underflow trap is enabled so the underflow is to be signaled
	 * when "tininess" is detected regardless of "loss of accuracy".
	 */
	bge	t1,DEXP_MIN,$300	# check for tininess
	/*
	 * Underflow has occured and the underflow trap was enabled so signal a
	 * SIGFPE and put the correct result in the destination register.
	 */
	or	a3,UNDERFLOW_EXC
	li	v0,SIGFPE
	jal	post_signal
	addu	t1,DEXP_OU_ADJ+DEXP_BIAS # adjust the exponent up
	sll	t1,DEXP_SHIFT
	and	t2,~DIMP_1BIT		# clear implied one bit
	or	t2,t0			# put the sign back in
	or	t2,t1			# put the exponent back in
	beq	t8,zero,1f		# check for inexact exception
	or	a3,INEXACT_EXC		# set the inexact exception
1:	li	v0,1			# a non-zero exit value
	b	rd_2w
	/*
	 * The underflow trap is not enabled so the underflow is to be signaled
	 * when both "tininess" and "loss of accuracy" is detected.
	 */
2:	bge	t1,DEXP_MIN,$300	# check for tininess
	/*
	 * Now that tininess has occrued the number will be a denormalized
	 * number or zero.  So produce the denormalized or zero value from
	 * the unrounded result and then check for "loss of accuracy" to
	 * see if underflow is to be detected.
	 */
	move	t1,t5			# get the unrounded exponent
	move	t2,t6			# get the unrounded fraction (less)
	move	t3,t7			# get the unrounded fraction (least)
.globl point7
point7:
	subu	t1,DEXP_MIN		# calculate the shift amnount to
	negu	t1			#  make the value a denormalized value
	blt	t1,DFRAC_BITS+2,3f	# if the shift amount would shift out
	move	t2,zero			#  all the fraction bits then the result
	move	t3,zero			#  will be an inexact zero.
	li	t8,STKBIT
	b 	7f
3:	blt	t1,32,5f
	subu	t1,32			# shift the fraction >= 32 bits
	beq	t8,zero,4f		# make sure the sticky bit stays set
	or	t3,STKBIT		#  if there are any bits in sticky reg.
4:	move	t8,t3
	move	t3,t2
	move	t2,zero
	negu	v0,t1
	addu	v0,32
	srl	t8,t1
	sll	v1,t3,v0
	or	t8,v1
	srl	t3,t1
	b	7f
5:	negu	v0,t1			# shift the fraction < 32 bits
	addu	v0,32
	sll	v1,t8,v0		# make sure the sticky bit stays set
	srl	t8,t1
	beq	v1,zero,6f
	or	t8,STKBIT
6:	sll	v1,t3,v0
	or	t8,v1
	srl	t3,t1
	sll	v1,t2,v0
	or	t3,v1
	srl	t2,t1
7:
	/*
	 * Now round the denormalized result.
	 */
	and	v0,a3,CSR_RM_MASK	# get the rounding mode
	beq	v0,CSR_RM_RN,3f		# round to nearest
	beq	v0,CSR_RM_RZ,$200	# round to zero (truncate)
	beq	v0,CSR_RM_RPI,1f	# round to plus infinity
	# Round to minus infinity
	beq	t0,zero,$200		# if the sign is plus truncate
	b	2f
1:	# Round to plus infinity
	bne	t0,zero,$200		# if the sign is minus truncate
2:	beq	t8,zero,$200		# if not inexact go on
	addu	t3,1
	bne	t3,zero,$200		# if there was no carry out go on
	addu	t2,1
	b	$200
	# Round to nearest
3:	li	v0,GUARDBIT		# load the guard bit for rounding
.globl point8
point8:
	not	v1,t8			# set carry out for addition of the
	sltu	t9,v1,v0		#  the sticky register and guard bit
	addu	v0,t8
	beq	t9,zero,4f		# if there was no carry out go on
	addu	t3,1
	bne	t3,zero,4f		# if there was no carry out go on
	addu	t2,1
4:	bne	v0,zero,$200		# if sticky register is zero clear the
	li	v1,~1			#  last bit in the fraction (round to
	and	t3,v1			#  nearest)
	/*
	 * At this point "tininess" has been detected so now if "loss of
	 * accurcy" has also occured then underflow is has occured. (the
	 * detection of underflow in untrapped underflow case).
	 */
$200:	beq	t8,zero,7f		# test for "loss of accurcy"
	or	a3,UNDERFLOW_EXC	# set the underflow exception
	or	a3,INEXACT_EXC		# set the inexact exception
	and	v0,a3,INEXACT_ENABLE	# see if inexact trap is enabled
	beq	v0,zero,7f		# if it is enabled post a signal
	li	v0,SIGFPE
	jal	post_signal
	/*
	 * Now put together the denormalized or zero result and exit. Note the
	 * exponet field is always zero, and there never is an implied 1 bit.
	 */
	or	t2,t0			# put back the sign
	li	v0,1			# a non-zero exit value
	b	rd_2w
	/*
	 * Now put together the denormalized or zero result and exit. Note the
	 * exponet field is always zero, and there never is an implied 1 bit.
	 */
7:	or	t2,t0			# put back the sign
	move	v0,zero			# a zero exit value
	b	rd_2w

	/*
	 * Now check for inexact exception, set the inexact exception if so and
	 * if the trap is enabled signal a SIGFPE.
	 */
$300:	move	v0,zero			# a zero exit value (hopefully)
	beq	t8,zero,1f		# check for inexact exception
	or	a3,INEXACT_EXC		# set the inexact exception
	and	v0,a3,INEXACT_ENABLE	# see if inexact trap is enabled
	beq	v0,zero,1f		# if it is enabled post a signal
	li	v0,SIGFPE
	jal	post_signal
	li	v0,1			# a non-zero exit value
1:
	and	t2,~DIMP_1BIT		# clear the implied 1
	or	t2,t0			# put the sign back in
	addu	t1,DEXP_BIAS		# add back in the exponent bias
	sll	t1,DEXP_SHIFT		# shift the exponent back in place
	or	t2,t1			# put the exponent back in
	b	rd_2w

norm_e:
norm_q:
	b	illfpinst

/*
 * This leaf routine is called to break out the fields of the RS single value 
 * in gp register (t2) into:
 *	t0 -- sign bit		     (left justified)
 *	t1 -- exponent		     (right justified, still biased)
 *	t2 -- fraction bits [23-0]   (implied one bit NOT set)
 * If the value is a NaN then the action for it is taken and this routine will
 * then branch to rd_[124]w: or store_fpc_csr: to exit softfp() after converting
 * the NaN to the format specified in (t9).  If the value is a quiet NaN then
 * if (v1) is non-zero then RT must be checked for a signaling NaN.
 */
rs_breakout_s:
	srl	t1,t2,SEXP_SHIFT
	move	t0,t1
	and	t1,SEXP_MASK
	and	t0,SIGNBIT>>SEXP_SHIFT
	sll	t0,SEXP_SHIFT
	and	t2,SFRAC_MASK

	/* If this is not a NaN then return */
	beq	t1,SEXP_NAN,1f
	j	ra
1:	bne	t2,zero,2f
	j	ra
	
2:	/* Check to see if this is a signaling NaN */
	and	v0,t2,SSNANBIT_MASK
	bne	v0,zero,4f
	/*
	 * RS is not a signaling NaN so if (v1) is non-zero check RT for a
	 * signaling NaN and if it is return the default quiet nan.
	 */
	beq	v1,zero,3f
	/* Check RT for a signaling NaN */
	srl	t5,t6,SEXP_SHIFT
	move	t4,t5
	and	t5,SEXP_MASK
	and	t4,SIGNBIT>>SEXP_SHIFT
	sll	t4,SEXP_SHIFT
	and	t6,SFRAC_MASK

	bne	t5,SEXP_NAN,3f
	beq	t6,zero,3f
	and	v0,t6,SSNANBIT_MASK
	bne	v0,zero,4f

	/*
	 * RS and RT are not a signaling NaNs so just use RS as the result by
	 * converting it to the format specified in (t9) preserving the
	 * high bits of the fraction.
	 */
3:	lw	v1,rs_snan_fmt(t9)
	j	v1

	/*
	 * This is a signaling NaN so set the invalid exception in the fpc_csr
	 * (a3) and setup the result depending if the enable for the invalid
	 * exception is set.
	 */
4:	or	a3,INVALID_EXC
	and	v0,a3,INVALID_ENABLE
	beq	v0,zero,5f
	/*
	 * The invalid trap was enabled so signal a SIGFPE and leave the
	 * result register unmodified.
	 */
	li	v0,SIGFPE
	jal	post_signal
	li	v0,1			# a non-zero exit value
	b	store_fpc_csr
5:
	/*
	 * The invalid trap was NOT enabled so the result is a quiet version 
	 * of the NaN.  So use the default quiet NaN to build a quiet of the
	 * in the format specified in (t9) preserving the high bits of the
	 * fraction.
	 */
	move	t0,zero
	li	t1,SEXP_NAN
	li	t2,SQUIETNAN_LEAST & SFRAC_MASK
	lw	v1,rs_snan_fmt(t9)
	j	v1

	.rdata
rs_snan_fmt:
	.word	rs_snan_s:1, rs_snan_d:1
	.text

rs_snan_s:
	or	t2,t0
	sll	t1,SEXP_SHIFT
	or	t2,t1
	move	v0,zero
	b	rd_1w

rs_snan_d:
	sll	t3,t2,DFRAC_BITS-SFRAC_BITS
	srl	t2,32-(DFRAC_BITS-SFRAC_BITS)
	or	t2,t0
	or	t2,DEXP_NAN<<DEXP_SHIFT
	move	v0,zero
	b	rd_2w

/*
 * This leaf routine is called to break out the fields of the RS double value 
 * in gp registers (t2,t3) into:
 *	t0 -- sign bit		     (left justified)
 *	t1 -- exponent		     (right justified, still biased)
 *	t2 -- fraction bits [51-32]  (implied one bit NOT set)
 *	t3 -- fraction bits [31-0]
 * If the value is a NaN then the action for it is taken and this routine will
 * then branch to rd_[124]w: or store_fpc_csr: to exit softfp() after converting
 * the NaN to the format specified in (t9).  If the value is a quiet NaN then
 * if (v1) is non-zero then RT must be checked for a signaling NaN.
 */
rs_breakout_d:
	srl	t1,t2,DEXP_SHIFT
	move	t0,t1
	and	t1,DEXP_MASK
	and	t0,SIGNBIT>>DEXP_SHIFT
	sll	t0,DEXP_SHIFT
	and	t2,DFRAC_MASK

	/* If this is not a NaN then return */
	beq	t1,DEXP_NAN,1f
	j	ra
1:	bne	t2,zero,2f
	bne	t3,zero,2f
	j	ra
	
2:	/* Check to see if this is a signaling NaN */
	and	v0,t2,DSNANBIT_MASK
	bne	v0,zero,4f
	/*
	 * RS is not a signaling NaN so if (v1) is non-zero check RT for a
	 * signaling NaN and if it is use the default quiet NaN.
	 */
	beq	v1,zero,3f
	/* Check RT for a signaling NaN */
	srl	t5,t6,DEXP_SHIFT
	move	t4,t5
	and	t5,DEXP_MASK
	and	t4,SIGNBIT>>DEXP_SHIFT
	sll	t4,DEXP_SHIFT
	and	t6,DFRAC_MASK

	bne	t5,DEXP_NAN,3f
	beq	t6,zero,3f
	and	v0,t6,DSNANBIT_MASK
	bne	v0,zero,4f

	/*
	 * RS and RT are not a signaling NaNs so just use RS as the result by
	 * converting it to the format specified in (t9) preserving the
	 * high bits of the fraction.
	 */
3:	lw	v1,rs_dnan_fmt(t9)
	j	v1

	/*
	 * This is a signaling NaN so set the invalid exception in the fpc_csr
	 * (a3) and setup the result depending if the enable for the invalid
	 * exception is set.
	 */
4:	or	a3,INVALID_EXC
	and	v0,a3,INVALID_ENABLE
	beq	v0,zero,5f
	/*
	 * The invalid trap was enabled so signal a SIGFPE and leave the
	 * result register unmodified.
	 */
	li	v0,SIGFPE
	jal	post_signal
	li	v0,1			# a non-zero exit value
	b	store_fpc_csr
5:
	/*
	 * The invalid trap was NOT enabled so the result is a quiet version 
	 * of the NaN.  So use the default quiet NaN to build a NaN
	 * in the format specified in (t9) preserving the high bits of the
	 * fraction.
	 */
	move	t0,zero
	li	t1,DEXP_NAN
	li	t2,DQUIETNAN_LESS & SFRAC_MASK
	li	t3,DQUIETNAN_LEAST
	lw	v1,rs_dnan_fmt(t9)
	j	v1

	.rdata
rs_dnan_fmt:
	.word	rs_dnan_s:1, rs_dnan_d:1
	.text

rs_dnan_s:
	srl	t2,32-(DFRAC_BITS-SFRAC_BITS)
	or	t2,t0
	or	t2,SEXP_NAN<<SEXP_SHIFT
	move	v0,zero
	b	rd_1w

rs_dnan_d:
	or	t2,t0
	sll	t1,DEXP_SHIFT
	or	t2,t1
	move	v0,zero
	b	rd_2w

/*
 * This leaf routine is called to break out the fields of the RT single value 
 * in gp registers (t6) into:
 *	t4 -- sign bit		     (left justified)
 *	t5 -- exponent		     (right justified, still biased)
 *	t6 -- fraction bits [23-0]   (implied one bit NOT set)
 * If the value is a NaN then the action for it is taken and this routine will
 * then branch to rd_1w: or store_fpc_csr: to exit softfp().
 */
rt_breakout_s:
	srl	t5,t6,SEXP_SHIFT
	move	t4,t5
	and	t5,SEXP_MASK
	and	t4,SIGNBIT>>SEXP_SHIFT
	sll	t4,SEXP_SHIFT
	and	t6,SFRAC_MASK

	/* If this is not a NaN then return */
	beq	t5,SEXP_NAN,1f
	j	ra
1:	bne	t6,zero,2f
	j	ra
	
2:	/* Check to see if this is a signaling NaN */
	and	v0,t6,SSNANBIT_MASK
	bne	v0,zero,3f
	/* This is not a signaling NaN so just use it as the result */
	move	t2,t6
	sll	t5,SEXP_SHIFT
	or	t2,t4
	or	t2,t5
	move	v0,zero
	b	rd_1w

	/*
	 * This is a signaling NaN so set the invalid exception in the fpc_csr
	 * (a3) and setup the result depending if the enable for the invalid
	 * exception is set.
	 */
3:	or	a3,INVALID_EXC
	and	v0,a3,INVALID_ENABLE
	beq	v0,zero,4f
	/*
	 * The invalid trap was enabled so signal a SIGFPE and leave the
	 * result register unmodified.
	 */
	li	v0,SIGFPE
	jal	post_signal
	li	v0,1			# a non-zero exit value
	b	store_fpc_csr

4:	/*
	 * The invalid trap was NOT enabled so the result is the default quiet
	 * NaN.
	 */
	li	t2,SQUIETNAN_LEAST
	move	v0,zero
	b	rd_1w

/*
 * This leaf routine is called to break out the fields of the RT double value 
 * in gp registers (t6,t7) into:
 *	t4 -- sign bit		     (left justified)
 *	t5 -- exponent		     (right justified, still biased)
 *	t6 -- fraction bits [51-32]  (implied one bit NOT set)
 *	t7 -- fraction bits [31-0]
 * If the value is a NaN then the action for it is taken and this routine will
 * then branch to rd_2w: or store_fpc_csr: to exit softfp().
 */
rt_breakout_d:
	srl	t5,t6,DEXP_SHIFT
	move	t4,t5
	and	t5,DEXP_MASK
	and	t4,SIGNBIT>>DEXP_SHIFT
	sll	t4,DEXP_SHIFT
	and	t6,DFRAC_MASK

	/* If this is not a NaN then return */
	beq	t5,DEXP_NAN,1f
	j	ra
1:	bne	t6,zero,2f
	bne	t7,zero,2f
	j	ra
	
2:	/* Check to see if this is a signaling NaN */
	and	v0,t6,DSNANBIT_MASK
	bne	v0,zero,3f
	/* This is not a signaling NaN so just use it as the result */
	move	t2,t6
	sll	t5,DEXP_SHIFT
	or	t2,t4
	or	t2,t5
	move	t3,t7
	move	v0,zero
	b	rd_2w

	/*
	 * This is a signaling NaN so set the invalid exception in the fpc_csr
	 * (a3) and setup the result depending if the enable for the invalid
	 * exception is set.
	 */
3:	or	a3,INVALID_EXC
	and	v0,a3,INVALID_ENABLE
	beq	v0,zero,4f
	/*
	 * The invalid trap was enabled so signal a SIGFPE and leave the
	 * result register unmodified.
	 */
	li	v0,SIGFPE
	jal	post_signal
	li	v0,1			# a non-zero exit value
	b	store_fpc_csr

4:	/*
	 * The invalid trap was NOT enabled so the result is the default quiet
	 * NaN.
	 */
	li	t2,DQUIETNAN_LESS
	li	t3,DQUIETNAN_LEAST
	move	v0,zero
	b	rd_2w

/*
 * This leaf routine is called to renormalize the RS denormalized single value
 * in gp registers (t0,t1,t2).  This must be a denormalized value not zero.
 */
rs_renorm_s:
	/*
	 * The first step in this process is to determine where the first
	 * one bit is in the fraction (t2).  After this series of tests
	 * the shift count to shift the fraction left so the first 1 bit is
	 * in the high bit will be in t9.  This sequence of code uses registers
	 * v0,v1 and t9 (it could be done with two but with reorginization this
	 * is faster).
	 */
	move	v0,t2
	move	t9,zero

	srl	v1,v0,16
	bne	v1,zero,1f
	addu	t9,16
	sll	v0,16
1:	srl	v1,v0,24
	bne	v1,zero,2f
	addu	t9,8
	sll	v0,8
2:	srl	v1,v0,28
	bne	v1,zero,3f
	addu	t9,4
	sll	v0,4
3:	srl	v1,v0,30
	bne	v1,zero,4f
	addu	t9,2
	sll	v0,2
4:	srl	v1,v0,31
	bne	v1,zero,5f
	addu	t9,1
5:
	/*
	 * Now that the it is known where the first one bit is calculate the
	 * amount to shift the fraction to put the first one bit in the
	 * implied 1 position (also the amount to adjust the exponent by).
	 * Then adjust the exponent and shift the fraction.
	 */
	subu	t9,SFRAC_LEAD0S	# the calulated shift amount
	subu	t1,t9		# adjust the exponent
	sll	t2,t9		# shift the fraction
	j	ra

/*
 * This leaf routine is called to renormalize the RS denormalized double value
 * in gp registers (t0,t1,t2,t3).  This must be a denormalized value not zero.
 */
.globl rs_renorm_d
rs_renorm_d:
	/*
	 * The first step in this process is to determine where the first
	 * one bit is in the fraction (t2,t3).  After this series of tests
	 * the shift count to shift the fraction left so the first 1 bit is
	 * in the high bit will be in t9.  This sequence of code uses registers
	 * v0,v1 and t9 (it could be done with two but with reorginization this
	 * is faster).
	 */
	move	v0,t2
	move	t9,zero
	bne	t2,zero,1f
	move	v0,t3
	addu	t9,32
1:
	srl	v1,v0,16
	bne	v1,zero,1f
	addu	t9,16
	sll	v0,16
1:	srl	v1,v0,24
	bne	v1,zero,2f
	addu	t9,8
	sll	v0,8
2:	srl	v1,v0,28
	bne	v1,zero,3f
	addu	t9,4
	sll	v0,4
3:	srl	v1,v0,30
	bne	v1,zero,4f
	addu	t9,2
	sll	v0,2
4:	srl	v1,v0,31
	bne	v1,zero,5f
	addu	t9,1
5:
	/*
	 * Now that the it is known where the first one bit is calculate the
	 * amount to shift the fraction to put the first one bit in the
	 * implied 1 position (also the amount to adjust the exponent by).
	 * Then adjust the exponent and shift the fraction.
	 */
	subu	t9,DFRAC_LEAD0S	# the calulated shift amount
	subu	t1,t9		# adjust the exponent
	blt	t9,32,1f
	subu	t9,32		# shift the fraction for >= 32 bit shifts
	sll	t2,t3,t9
	move	t3,zero
	j	ra
1:
	negu	v0,t9		# shift the fraction for < 32 bit shifts
	addu	v0,32
	sll	t2,t9
	srl	v1,t3,v0
	or	t2,v1
	sll	t3,t9
	j	ra

/*
 * This leaf routine is called to renormalize the RT denormalized single value
 * in gp registers (t4,t5,t6).  This must be a denormalized value not zero.
 */
rt_renorm_s:
	/*
	 * The first step in this process is to determine where the first
	 * one bit is in the fraction (t6).  After this series of tests
	 * the shift count to shift the fraction left so the first 1 bit is
	 * in the high bit will be in t9.  This sequence of code uses registers
	 * v0,v1 and t9 (it could be done with two but with reorginization this
	 * is faster).
	 */
	move	v0,t6
	move	t9,zero

	srl	v1,v0,16
	bne	v1,zero,1f
	addu	t9,16
	sll	v0,16
1:	srl	v1,v0,24
	bne	v1,zero,2f
	addu	t9,8
	sll	v0,8
2:	srl	v1,v0,28
	bne	v1,zero,3f
	addu	t9,4
	sll	v0,4
3:	srl	v1,v0,30
	bne	v1,zero,4f
	addu	t9,2
	sll	v0,2
4:	srl	v1,v0,31
	bne	v1,zero,5f
	addu	t9,1
5:
	/*
	 * Now that the it is known where the first one bit is calculate the
	 * amount to shift the fraction to put the first one bit in the
	 * implied 1 position (also the amount to adjust the exponent by).
	 * Then adjust the exponent and shift the fraction.
	 */
	subu	t9,SFRAC_LEAD0S	# the calulated shift amount
	subu	t5,t9		# adjust the exponent
	sll	t6,t9		# shift the fraction
	j	ra

/*
 * This leaf routine is called to renormalize the RT denormalized double value
 * in gp registers (t4,t5,t6,t7).  This must be a denormalized value not zero.
 */
rt_renorm_d:
	/*
	 * The first step in this process is to determine where the first
	 * one bit is in the fraction (t6,t7).  After this series of tests
	 * the shift count to shift the fraction left so the first 1 bit is
	 * in the high bit will be in t9.  This sequence of code uses registers
	 * v0,v1 and t9 (it could be done with two but with reorginization this
	 * is faster).
	 */
	move	v0,t6
	move	t9,zero
	bne	t6,zero,1f
	move	v0,t7
	addu	t9,32
1:
	srl	v1,v0,16
	bne	v1,zero,1f
	addu	t9,16
	sll	v0,16
1:	srl	v1,v0,24
	bne	v1,zero,2f
	addu	t9,8
	sll	v0,8
2:	srl	v1,v0,28
	bne	v1,zero,3f
	addu	t9,4
	sll	v0,4
3:	srl	v1,v0,30
	bne	v1,zero,4f
	addu	t9,2
	sll	v0,2
4:	srl	v1,v0,31
	bne	v1,zero,5f
	addu	t9,1
5:
	/*
	 * Now that the it is known where the first one bit is calculate the
	 * amount to shift the fraction to put the first one bit in the
	 * implied 1 position (also the amount to adjust the exponent by).
	 * Then adjust the exponent and shift the fraction.
	 */
	subu	t9,DFRAC_LEAD0S	# the calulated shift amount
	subu	t5,t9		# adjust the exponent
	blt	t9,32,1f
	subu	t9,32		# shift the fraction for >= 32 bit shifts
	sll	t6,t7,t9
	move	t7,zero
	j	ra
1:
	negu	v0,t9		# shift the fraction for < 32 bit shifts
	addu	v0,32
	sll	t6,t9
	srl	v1,t7,v0
	or	t6,v1
	sll	t7,t9
	j	ra

/*
 * This exit point stores a one word floating point value from the gp
 * register (t2) into the fpr register specified by the RD field from
 * the floating-point instruction (a1).  From here until the return
 * to the caller the return value (v0) must not be touched.
 */
rd_1w:
	srl	v1,a1,RD_SHIFT-2	# get the RD field times 4 right
	and	v1,RD_FPRMASK<<2	#  justified into v1 with the last bit
					#  of the field cleared.
	/*
	 * If fptype_word (a2) is non-zero then the floating-point values
	 * are stored into the coprocessor else they are stored into the pcb.
	 */
	bne	a2,zero,rd_cp_1w

	sw	t2,u+PCB_FPREGS(v1)
	b	store_fpc_csr

rd_cp_1w:
	srl	v1,1
	lw	v1,rd_cp_1w_tab(v1)
	j	v1

	.rdata
rd_cp_1w_tab:
	.word	rd_cp_1w_fpr0:1,  rd_cp_1w_fpr2:1,  rd_cp_1w_fpr4:1
	.word	rd_cp_1w_fpr6:1,  rd_cp_1w_fpr8:1,  rd_cp_1w_fpr10:1
	.word	rd_cp_1w_fpr12:1, rd_cp_1w_fpr14:1, rd_cp_1w_fpr16:1
	.word	rd_cp_1w_fpr18:1, rd_cp_1w_fpr20:1, rd_cp_1w_fpr22:1
	.word	rd_cp_1w_fpr24:1, rd_cp_1w_fpr26:1, rd_cp_1w_fpr28:1
	.word	rd_cp_1w_fpr30:1
	.text

rd_cp_1w_fpr0:
	mtc1	t2,$f0;		b	store_fpc_csr
rd_cp_1w_fpr2:
	mtc1	t2,$f2;		b	store_fpc_csr
rd_cp_1w_fpr4:
	mtc1	t2,$f4;		b	store_fpc_csr
rd_cp_1w_fpr6:
	mtc1	t2,$f6;		b	store_fpc_csr
rd_cp_1w_fpr8:
	mtc1	t2,$f8;		b	store_fpc_csr
rd_cp_1w_fpr10:
	mtc1	t2,$f10;	b	store_fpc_csr
rd_cp_1w_fpr12:
	mtc1	t2,$f12;	b	store_fpc_csr
rd_cp_1w_fpr14:
	mtc1	t2,$f14;	b	store_fpc_csr
rd_cp_1w_fpr16:
	mtc1	t2,$f16;	b	store_fpc_csr
rd_cp_1w_fpr18:
	mtc1	t2,$f18;	b	store_fpc_csr
rd_cp_1w_fpr20:
	mtc1	t2,$f20;	b	store_fpc_csr
rd_cp_1w_fpr22:
	mtc1	t2,$f22;	b	store_fpc_csr
rd_cp_1w_fpr24:
	mtc1	t2,$f24;	b	store_fpc_csr
rd_cp_1w_fpr26:
	mtc1	t2,$f26;	b	store_fpc_csr
rd_cp_1w_fpr28:
	mtc1	t2,$f28;	b	store_fpc_csr
rd_cp_1w_fpr30:
	mtc1	t2,$f30;	b	store_fpc_csr

/*
 * This exit point stores a two word floating point value from the gp
 * registers (t2,t3) into the fpr register specified by the RD field from
 * the floating-point instruction (a1).  From here until the return
 * to the caller the return value (v0) must not be touched.
 */
rd_2w:
	srl	v1,a1,RD_SHIFT-2	# get the RD field times 4 right
	and	v1,RD_FPRMASK<<2	#  justified into v1 with the last bit
					#  of the field cleared.
	/*
	 * If fptype_word (a2) is non-zero then the floating-point values
	 * are stored into the coprocessor else they are stored into the pcb.
	 */
	bne	a2,zero,rd_cp_2w

	sw	t2,u+PCB_FPREGS+4(v1)
	sw	t3,u+PCB_FPREGS(v1)
	b	store_fpc_csr

rd_cp_2w:
	srl	v1,1
	lw	v1,rd_cp_2w_tab(v1)
	j	v1

	.rdata
rd_cp_2w_tab:
	.word	rd_cp_2w_fpr0:1,  rd_cp_2w_fpr2:1,  rd_cp_2w_fpr4:1
	.word	rd_cp_2w_fpr6:1,  rd_cp_2w_fpr8:1,  rd_cp_2w_fpr10:1
	.word	rd_cp_2w_fpr12:1, rd_cp_2w_fpr14:1, rd_cp_2w_fpr16:1
	.word	rd_cp_2w_fpr18:1, rd_cp_2w_fpr20:1, rd_cp_2w_fpr22:1
	.word	rd_cp_2w_fpr24:1, rd_cp_2w_fpr26:1, rd_cp_2w_fpr28:1
	.word	rd_cp_2w_fpr30:1
	.text

rd_cp_2w_fpr0:
	mtc1	t3,$f0;		mtc1	t2,$f1;		b	store_fpc_csr
rd_cp_2w_fpr2:
	mtc1	t3,$f2;		mtc1	t2,$f3;		b	store_fpc_csr
rd_cp_2w_fpr4:
	mtc1	t3,$f4;		mtc1	t2,$f5;		b	store_fpc_csr
rd_cp_2w_fpr6:
	mtc1	t3,$f6;		mtc1	t2,$f7;		b	store_fpc_csr
rd_cp_2w_fpr8:
	mtc1	t3,$f8;		mtc1	t2,$f9;		b	store_fpc_csr
rd_cp_2w_fpr10:
	mtc1	t3,$f10;	mtc1	t2,$f11;	b	store_fpc_csr
rd_cp_2w_fpr12:
	mtc1	t3,$f12;	mtc1	t2,$f13;	b	store_fpc_csr
rd_cp_2w_fpr14:
	mtc1	t3,$f14;	mtc1	t2,$f15;	b	store_fpc_csr
rd_cp_2w_fpr16:
	mtc1	t3,$f16;	mtc1	t2,$f17;	b	store_fpc_csr
rd_cp_2w_fpr18:
	mtc1	t3,$f18;	mtc1	t2,$f19;	b	store_fpc_csr
rd_cp_2w_fpr20:
	mtc1	t3,$f20;	mtc1	t2,$f21;	b	store_fpc_csr
rd_cp_2w_fpr22:
	mtc1	t3,$f22;	mtc1	t2,$f23;	b	store_fpc_csr
rd_cp_2w_fpr24:
	mtc1	t3,$f24;	mtc1	t2,$f25;	b	store_fpc_csr
rd_cp_2w_fpr26:
	mtc1	t3,$f26;	mtc1	t2,$f27;	b	store_fpc_csr
rd_cp_2w_fpr28:
	mtc1	t3,$f28;	mtc1	t2,$f29;	b	store_fpc_csr
rd_cp_2w_fpr30:
	mtc1	t3,$f30;	mtc1	t2,$f31;	b	store_fpc_csr

/*
 * This exit point stores the floating-point fpc_csr value from the gp
 * register (a3) into the fpc_csr register.  From here until the return
 * to the caller the return value (v0) must not be touched.
 */
store_fpc_csr:
	/*
	 * If fptype_word (a2) is non-zero then the floating-point fpc_csr
	 * register is stored into the coprocessor else it is stored into
	 * the pcb.
	 */
	bne	a2,zero,1f

	sw	a3,u+PCB_FPC_CSR
	b	2f
1:
	ctc1	a3,fpc_csr
2:
	# exit the softfp() emulation routine.
	lw	ra,RA_OFFSET(sp)
	addu	sp,FRAME_SIZE
	j	ra

/*
 * This exit point posts an illegal instruction signal.  Set v0 to SIGILL and
 * call post_signal().  The return value to be used for softfp() will be
 * non-zero to indicate the error.
 */
illfpinst:
	li	v0,SIGILL
	jal	post_signal
	# exit the softfp() emulation routine with an non-zero return value (v0)
	# to indicate the error.
	li	v0,1
	lw	ra,RA_OFFSET(sp)
	addu	sp,FRAME_SIZE
	j	ra

	.end softfp
/*
 * This routine posts a signal to the process who's instruction is being
 * emulating.  The signal type to post is in (v0). Only specific registers
 * are saved in this routine.
 */
NESTED(post_signal, FRAME_SIZE, ra)
	.mask	0xa0000f90, -(FRAME_SIZE - 4*4)
	subu	sp,FRAME_SIZE

	sw	a0,A0_OFFSET(sp)	# a0 (exception frame)
	sw	a1,A1_OFFSET(sp)	# a1 (fp instruction)
	sw	a3,A3_OFFSET(sp)	# a3 (control and status register)
	sw	t0,T0_OFFSET(sp)
	sw	t1,T1_OFFSET(sp)
	sw	t2,T2_OFFSET(sp)
	sw	t3,T3_OFFSET(sp)
	sw	t8,T8_OFFSET(sp)
	sw	ra,RA_OFFSET(sp)
	/*
	 * If fptype_word (a2) is non-zero then the pointer to the proc
	 * structure for process who's instruction is being emulated is
	 * in fpowner.  Else it is in u+U_PROCP (the current process).
	 *
	 * If current process, set u.u_pcb.pcb_resched to force entry into
	 * trap so signal will be posted before returning to user mode
	 */

	beq	a2,zero,1f
	lw	a0,u+PCB_CPUPTR		# get cpudata pointer
	lw	a1,CPU_FPOWNER(a0)	# get current fpowner
	b	2f
1:
	lw	a1,u+U_PROCP
2:
	lw	a0,u+PCB_CPUPTR		# get cpudata pointer
	sw	a1,CPU_FPE_EVENT(a0)	# store proc for psignal call.
	li	a2,1			
	subu 	v0,1			# signals are 1-32... need to dec 
	sll	a3,a2,v0		#	for the shift.

	lw 	a2,CPU_FPE_SENDSIG(a0)	# fetch current signals.
	or	a3,a2			# OR in new signal
	sw	a3,CPU_FPE_SENDSIG(a0)	# send it.
 	jal     setsoftnet		# post interrupt to handle signal.


	lw	a0,A0_OFFSET(sp)	# get back a0 (exception frame)
	lw	a1,A1_OFFSET(sp)	# get back a1 (fp instruction)
	lw	a2,fptype_word		# get back a2 (fptype_word)
	lw	a3,A3_OFFSET(sp)	# get back a3 (control and status reg)
	lw	t0,T0_OFFSET(sp)
	lw	t1,T1_OFFSET(sp)
	lw	t2,T2_OFFSET(sp)
	lw	t3,T3_OFFSET(sp)
	lw	t8,T8_OFFSET(sp)
	lw	ra,RA_OFFSET(sp)

	addu	sp,FRAME_SIZE
	j	ra

	.end post_signal
