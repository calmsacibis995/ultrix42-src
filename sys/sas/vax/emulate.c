
/************************************************************************
 *									*
 *			Copyright (c) 1985 by				*
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
/*
	@(#)emulate.c	4.1	(ULTRIX)	7/2/90
 */

/************************************************************************
 *
 *			Modification History
 *
 *	12-May-86 - tresvik
 *	This is a mini version of the emulation intended to support
 *	three instructions used by the MVAX QNA boot driver.  It is
 *	extracted from the kernel emulatation package.  It is cut down
 *	simply to save space.
 */
#include "../../machine/vax/emul/vaxemul.h"
#include "../../machine/vax/emul/vaxregdef.h"

#define CMPC3	0x29
#define CMPC5	0x2d
#define LOCC	0x3a

	.globl	vax$emulate
vax$emulate:

	cmpb	opcode(sp),$CMPC3
	beql	Lcmpc3
	cmpb	opcode(sp),$CMPC5
	beql	Lcmpc5
	cmpb	opcode(sp),$LOCC
	beql	Llocc

1:	pushl	$vax$_opcdec		# store signal name
	pushl	$13			# total of 13 longwords in signal array
	halt

 #+
 # input parameters:
 #
 #	opcode(sp)
 #	old_pc(sp)
 #	operand_1(sp) - len.rw
 #	operand_2(sp) - src1addr.ab
 #	operand_3(sp) - src2addr.ab
 #	operand_4(sp)
 #	operand_5(sp)
 #	operand_6(sp)
 #	operand_7(sp)
 #	operand_8(sp)
 #	new_pc(sp)
 #	exception_psl(sp)
 #
 # output parameters:
 #
 #	r0<15:0> - len.rw
 #	r1       - src1addr.ab
 #	r3       - src2addr.ab
 #
 # implicit output:
 #
 #	r0<31:16> - 0
 #	r2        - unpredictable
 #-

Lcmpc3:

	movzwl	operand_1(sp),r0	# r0<15:0> <- srclen.rw 
	movl	operand_2(sp),r1	# r1       <- src1addr.ab 
	movl	operand_3(sp),r3	# r3       <- src2addr.ab 

 # now that the operands have been loaded, the only exception parameter
 # other than the pc/psl pair that needs to be saved is the old pc. however,
 # there is no reason why the state of the stack needs to be altered and we
 # save two instructions if we leave the stack alone.

	pushab	vax$exit_emulator	# store the return pc
	jmp	vax$cmpc3		# do the actual work


 #+
 # input parameters:
 #
 #	opcode(sp)
 #	old_pc(sp)
 #	operand_1(sp) - src1len.rw
 #	operand_2(sp) - src1addr.ab
 #	operand_3(sp) - fill.rb
 #	operand_4(sp) - src2len.rw
 #	operand_5(sp) - src2addr.ab
 #	operand_6(sp)
 #	operand_7(sp)
 #	operand_8(sp)
 #	new_pc(sp)
 #	exception_psl(sp)
 #
 # output parameters:
 #
 #	r0<15:0>   - srclen.rw
 #	r0<23:16>  - fill.rb
 #	r1         - srcaddr.ab
 #	r2<15:0>   - src2len.rw
 #	r3         - src2addr.ab
 #
 # implicit output:
 #
 #	r0<31:24> - unpredictable
 #	r2<31:16> - 0
 #-

Lcmpc5:

	rotl	$16,operand_3(sp),r0	# r0<23:16> <- fill.rb
	movw	operand_1(sp),r0	# r0<15:0>  <- src1len.rw 
	movl	operand_2(sp),r1	# r1        <- src1addr.ab 
	movzwl	operand_4(sp),r2	# r2<15:0>  <- src2len.rw 
	movl	operand_5(sp),r3	# r3        <sca- src2addr.ab 

 # now that the operands have been loaded, the only exception parameter
 # other than the pc/psl pair that needs to be saved is the old pc. however,
 # there is no reason why the state of the stack needs to be altered and we
 # save two instructions if we leave the stack alone.

	pushab	vax$exit_emulator	# store the return pc
	jmp	vax$cmpc5		# do the actual work


 #+
 # input parameters:
 #
 #	opcode(sp)
 #	old_pc(sp)
 #	operand_1(sp) - char.rb
 #	operand_2(sp) - len.rw
 #	operand_3(sp) - addr.ab
 #	operand_4(sp)
 #	operand_5(sp)
 #	operand_6(sp)
 #	operand_7(sp)
 #	operand_8(sp)
 #	new_pc(sp)
 #	exception_psl(sp)
 #
 # output parameters:
 #
 #	r0<15:0>  - len.rw
 #	r0<23:16> - char.rb
 #	r1        - addr.ab
 #
 # implicit output:
 #
 #	r0<31:24> - unpredictable
 #-

Llocc:

	rotl	$16,operand_1(sp),r0	# r0<23:16> <- char.ab
	movw	operand_2(sp),r0	# r0<15:0>  <- len.rw 
	movl	operand_3(sp),r1	# r1        <- addr.ab 

 # now that the operands have been loaded, the only exception parameter
 # other than the pc/psl pair that needs to be saved is the old pc. however,
 # there is no reason why the state of the stack needs to be altered and we
 # save two instructions if we leave the stack alone.

	pushab	vax$exit_emulator	# store the return pc
	jmp	vax$locc		# do the actual work

	.globl	vax$exit_emulator

vax$exit_emulator:
	movpsl	-(sp)			# save the new psl on the stack

 # note that the next instruction makes no assumptions about the condition 
 # codes in the saved psl. 

	insv	(sp)+,$0,$4,exception_psl(sp)	# replace saved condition codes
	addl2	$new_pc,sp		# adjust stack pointer (discard old pc)
	rei				# return

	.globl	vax$reflect_fault
vax$reflect_fault:
	halt
