/*	@(#)vaxhandlr.s	4.1		7/2/90		*/

# include "../machine/emul/vaxemul.h"
# include "../machine/psl.h"
# include "../machine/emul/vaxregdef.h"


/************************************************************************
 *									*
 *			Copyright (c) 1984 by				*
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

/************************************************************************
 *
 *			Modification History
 *
 *	Stephen Reilly, 20-Mar-84
 * 000- This code is the modification of the VMS emulation codethat 
 *	was written by Larry Kenah.  It has been modified to run
 *	on Ultrix.
 *
 ***********************************************************************/

 #++
 # facility: 
 #
 #	vax-11 instruction emulator
 #
 # abstract:
 #
 #	this module contains all interfaces between the vax-11 instruction
 #	emulator and the vax/vms condition handling facility. that is, this is
 #	the only module in the entire vax-11 instruction emulator package that
 #	has any knowledge of vms-specific aspects of condition handling. all
 #	exception knowledge contained in other modules is defined by the
 #	vax-11 architecture (microvax subset). if this emulator is to be used
 #	in an environment other than vms, this is the only module that needs
 #	to be changed. 
 #
 #	note that control flows through this module in two directions. on the
 #	one hand, certain exceptions such as access violation can occur while
 #	the emulator is executing. these exceptions are reported by vms to a
 #	special access violation routine in this module. these routines and
 #	instruction-specific exception handling routines manipulate this
 #	exception so that it appears to have occurred at the site of the
 #	reserved instruction, rather than within the emulator. 
 #
 #	other exceptions such as reserved operand abort for illegal decimal
 #	string length or decimal overflow trap are detected by emulator
 #	routines. these must be reported by the emulator to vms, again in such
 #	a way that it appears to the running code that the exception occurred
 #	at the site of the reserved instruction. 
 #
 # environment: 
 #
 #	these routines run at any access mode, at any ipl, and are ast 
 #	reentrant. 
 #
 # author: 
 #
 #	kathleen d. morse
 #
 # creation date
 #
 #	17 august 1982
 #
 # modified by:
 #
 #	v01-005 ljk0026		lawrence j. kenah	17-jul-1984
 #		fix insv bug in reflect_fault that was causing psl bits
 #		to be incorrectly cleared.
 #
 #	v01-002	ljk0002		lawrence j. kenah	15-mar-1983
 #		handle software detected exceptions from decimal and editpc
 #		emulation. modify initial and final dispatching to use registers
 #		instead of stack space for parameter passing. include stack
 #		format generated by microvax exceptions.
 #
 #	v01-001	original	kathleen d. morse	17-aug-1982
 #		intercept access violations and machine checks and conditionally
 #		dispatch them to emulator for modification. reflect exceptions 
 #		from emulator to regular vms exception dispatcher.
 #--



 # include files:

/*	$chfdef				# offsets into signal array
 *	$opdef				# symbolic names for opcodesot
 *	$prdef				# get definitions of processor registers
 *	$psldef				# define bit fields in psl
 *	$srmdef				# arithmetic trap codes
 *	$ssdef				# status codes in vms

 *	stack_def			# stack storage of exception parameters
 *	pack_def			# stack usage for reflecting exceptions
 */

 # macro definitions

/*	.macro	delta_pc_table_entry	opcode
	sign_extend	op$_'opcode , ...opcode
	.iif	less_than <...opcode - opcode_base>,-
		.error		# opcode not supported by emulator
	.iif	greater <...opcode - opcode_max>,-
		.error		# opcode not supported by emulator
	opcode'_def			# register usage for opcode instruction
. = delta_pc_table_base + <...opcode-opcode_base>
	.byte	opcode'_b_delta_pc
	.endm	delta_pc_table_entry

	.macro	cmp_l_to_a	src,dstadr,dst,type=b
	.if	blank	dst
		pusha'type	dstadr
		cmpl	src,(sp)+
	.if_false
		mova'type	dstadr,dst
		cmpl	src,dst
	.endc		
	.endm
 */

 # symbol definitions

 #	jsb_absolute = <^x9f@8> ! op$_jsb
 #	jsb_size = 6			# size of jsb @$vax$xxxxxx instruction

# define jsb_absolute 0x9f16
# define jsb_size 6

 # global and external declarations

/*	.disable	global

 *	.external	vax$exit_emulator	# return pc to dispatcher

 *	.external	exe$reflect	# entry point in vms
 */
 ### the following declarations are not needed unless we define entry points
 ### into the emulator theough the system service vector pages.
 ###
 ###	.external	sys$vax_begin,-	# lower and upper bounds of jsb
 ###			sys$vax_end	#  entry points in vector page

 # psect declarations:

 #	this label defines the beginning of the emulator image

vax$begin:

 #	this label locates the end of the emulator image


vax$end:



 #+
 # functional description:
 #
 #	the following table contains a byte entry for each emulated 
 #	instruction. that byte describes where in the saved register set the 
 #	delta-pc quantity is stored in the event that the instruction is 
 #	interrupted and restarted.
 #-


 # because the assembler does not understand sign extension of byte and
 # word quantities, we must accomplish this sign extension with macros. the
 # assignment statements that appear as comments illustrate the sense of the
 # macro invocations that immediately follow.

 #	opcode_base = op$_ashp		# smallest (in signed sense) opcode

 #	sign_extend	op$_ashp , opcode_base

 #	opcode_max = op$_skpc		# largest opcode in this emulator

 #	sign_extend	op$_skpc , opcode_max

 #delta_pc_table_size = -
 #	<opcode_max - opcode_base> + 1	# define table size

 # define delta_pc_table_size 0x44

		.globl	vax$al_delta_pc_table
vax$al_delta_pc_table:

delta_pc_table_base:			# locate start of table for macro

 # initialize entire table to empty

 #	.byte	0[delta_pc_table_size]	# lots of bytes that contain zero

	.byte	ashp_b_delta_pc,cvtlp_b_delta_pc,0,0,0,0,0,0,0,0
	.byte	0,0,0,0,0,0,cvtsp_b_delta_pc,0,0,crc_b_delta_pc
	.byte	0,0,0,0,0,0,0,0,0,0
	.byte	0,0,0,0,0,0,0,0,0,0

	.byte	addp4_b_delta_pc,addp6_b_delta_pc,subp4_b_delta_pc
	.byte	subp6_b_delta_pc,0,mulp_b_delta_pc,cvttp_b_delta_pc
	.byte	divp_b_delta_pc,0,cmpc3_b_delta_pc

	.byte	scanc_b_delta_pc,spanc_b_delta_pc,0,cmpc5_b_delta_pc
	.byte	movtc_b_delta_pc,movtuc_b_delta_pc,0,0,0,0

	.byte	movp_b_delta_pc,cmpp3_b_delta_pc,cvtpl_b_delta_pc
	.byte	cmpp4_b_delta_pc,editpc_b_delta_pc,matchc_b_delta_pc
	.byte	locc_b_delta_pc,skpc_b_delta_pc

 # define table entries for string instructions

/*	delta_pc_table_entry	movtc
 *	delta_pc_table_entry	movtuc
 *	delta_pc_table_entry	cmpc3
 *	delta_pc_table_entry	cmpc5
 *	delta_pc_table_entry	locc
 *	delta_pc_table_entry	skpc
 *	delta_pc_table_entry	scanc
 *	delta_pc_table_entry	spanc
 *	delta_pc_table_entry	matchc
 *	delta_pc_table_entry	crc

 # define table entries for decimal instructions

 *	delta_pc_table_entry	addp4
 *	delta_pc_table_entry	addp6
 *	delta_pc_table_entry	ashp
 *	delta_pc_table_entry	cmpp3
 *	delta_pc_table_entry	cmpp4
 *	delta_pc_table_entry	cvtlp
 *	delta_pc_table_entry	cvtpl
 *	delta_pc_table_entry	cvtps
 *	delta_pc_table_entry	cvtpt
 *	delta_pc_table_entry	cvtsp
 *	delta_pc_table_entry	cvttp
 *	delta_pc_table_entry	divp
 *	delta_pc_table_entry	movp
 *	delta_pc_table_entry	mulp
 *	delta_pc_table_entry	subp4
 *	delta_pc_table_entry	subp6

 # don't forget good old editpc
 
 *	delta_pc_table_entry	editpc
 */

 # locate the table through entry 0

 #vax$al_delta_pc_table == delta_pc_table_base + <0 - opcode_base>
# define vax$al_delta_table 0x08

 # finally, set the location counter to the end of the table

 #. = delta_pc_table_base + delta_pc_table_size

 #	the code that does useful work is put into this section.


 #+
 # functional description:
 #
 #	this routine receives control from the exception dispatcher in vms
 #	after the mechanism and signal arrays have been moved from the kernel
 #	stack to the stack of the mode in which the access violation occurred.
 #	this routine determines whether the exception occurred inside the
 #	emulator and, if so, passes control to an instruction-specific routine
 #	that performs further consistenct checks. 
 #
 #	the purpose of all this work is to allow exceptions that occur inside
 #	the emulator to be modified before being passed on to the user. there
 #	are two important pieces of this modification. 
 #
 #		the exception pc is changed from a pc within the emulator to
 #		the pc of the instruction that caused the emulator to be
 #		invoked in the first place. 
 #
 #		any stack usage by the emulator is dissolved. this allows
 #		the exception stack to be placed directly on top of the
 #		user's stack usage, removing any overt indication that an
 #		emulator is being used. 
 #
 #	although the only exception that can be modified in this fashion is an 
 #	access violation, the routine is written in terms of a general signal 
 #	array to allow expansion at a future time.
 #
 # input parameters:
 #
 #	if this exception is one that the emulator is capable of modifying,
 #	then r10 must contain the address of an instruction-specific routine
 #	to store context information in the general registers before passing
 #	control back to this module.
 #
 #	although vms places the mechanism array directly underneath the
 #	argument list and, with one intervening longword, places the signal
 #	array directly underneath that, these lists display general signal and
 #	mechanism arrays. 
 #
 #		00(sp) - return pc in vms exception dispatcher
 #		04(sp) - argument count (always 2)
 #		08(sp) - address of signal array
 #		12(sp) - address of mechanism array
 #
 #	the mechanism array looks like this
 #
 #		00(mech) - argument count (always 4)
 #		04(mech) - value of fp when exception occurred
 #		08(mech) - depth value (initially -3)
 #		12(mech) - value of r0 when exception occurred
 #		16(mech) - value of r1 when exception occurred
 #		
 #	in the general caseof an exception with m optional parameters, the
 #	signal array looks like this 
 #
 #		00(signal) - argument count (m+3)
 #		04(signal) - exception name
 #		08(signal) - first optional parameter if m gtru 0
 #		  .
 #		  .
 #		4*m +  4(signal) - last parameter if m gtru 0
 #		4*m +  8(signal) - pc of faulting instruction
 #		4*m + 12(signal) - psl at time of exception
 #
 #	for the case of an access violation, which has two optional
 #	parameters, the signal array takes this form. 
 #
 #		00(signal) - argument count (always 5)
 #		04(signal) - ss$_accvio
 #		08(signal) - access violation reason mask
 #		12(signal) - inaccessible virtual address
 #		16(signal) - pc of faulting instruction
 #		20(signal) - psl at time of exception
 #
 #	the longword immediately following the exception psl was on top of the
 #	stack when the exception occurred.
 #
 # output parameters:
 #
 #	if the exception passes the small number of tests performed here, 
 #	control is passed to the routine whose address is stored in r10
 #	with the following output parameters.
 #
 #		r0 - address of top of stack (value of sp) when exception occurred
 #		r1 - exception pc
 #
 #	r0 to r3 are saved on the stack to allow this routine and the 
 #	instruction-specific routines to do useful work without juggling
 #	registers.
 #
 #		00(sp) - value of r0 when exception occurred
 #		04(sp) - value of r1 when exception occurred
 #		08(sp) - value of r2 when exception occurred
 #		12(sp) - value of r3 when exception occurred
 #		16(sp) - return pc in vms exception dispatcher
 #		20(sp) - argument count (always 2)
 #		24(sp) - address of signal array
 #		28(sp) - address of mechanism array
 #
 #	if the tests performed here determine that the exception does not
 #	fit the pattern for a candidate to be modified, control is passed
 #	back to vms with an rsb instruction.
 #
 # notes:
 #
 #	some of the code here assumes that it knows what a vms exception stack
 #	looks like. if it finds the stack in a different shape, it does not
 #	continue. the reason is that, if vms changes these insignificant pieces
 #	of the stack, it may also change things that this routine relies on that
 #	are more difficult to detect.
 #-

	.globl	vax$modify_exception
vax$modify_exception:
	movq	r2,-(sp)		# start with two scratch registers
	moval	12(sp),r3		# r3 locates argument list
	cmpl	$2,(r3)			# is argument count 2?
	bneq	2f			# quit if not
	movl	chf$l_sigarglst(r3),r2	# r2 locates the signal array
	movl	chf$l_mcharglst(r3),r3	# r3 locates the mechanism array
	cmpl	$4,chf$l_mch_args(r3)	# is this argument count 4?
	bneq	2f			# quit if not
	movq	chf$l_mch_savr0(r3),-(sp)	# save original r0 and r1
	movl	chf$l_sig_args(r2),r0	# r0 contains signal argument count
	movl	(8-12)(r2)[r0],r1	# get exception pc

 # check that exception pc and r10 are both within the bounds of the emulator.

	movab	vax$emul_begin,r3	# put base address in convenient place
	cmpl	r1,r3			# is exception pc within this limit?
	blssu	1f			# quit if pc is at smaller address
	cmpl	r10,r3			# is r10 within this limit?
	blssu	1f			# quit if r10 is at smaller address

 # do the same checks with the ending address

	movab	vax$emul_end,r3		# put end address in convenient place
	cmpl	r1,r3			# is exception pc within this limit?
	bgequ	1f			# quit if pc is too large
	cmpl	r10,r3			# is r10 within this limit?
	bgequ	1f			# quit if r10 is too large

 # load r0 with the value of sp when the exception occurred

	moval	(16-12)(r2)[r0],r0	# get top of stack at time of exception
	jmp	(r10)			# call instruction-specific routine

1:	movq	(sp)+,r0		# restore r0 and r1
2:	movq	(sp)+,r2		# ... and r2 and r3
	rsb



 #+
 # functional description:
 #
 #	this routine receives control from the emulator routines for the
 #	decimal instructions or editpc when those routines have detected a
 #	condition that requires signalling of an exception. the exception can
 #	either be a reserved addressing mode fault from cvtpl or a reserved
 #	operand exception, either a fault or an abort, signalled from a
 #	variety of places. this  routine simply makes the stack look like the
 #	stack on entry to vax$reflect_fault and passes control to that routine
 #	to perform the instruction backup in common code. 
 #
 # input parameters:
 #
 #	00(sp) - offset in packed register array to delta pc byte
 #	04(sp) - return pc from vax$xxxxxx routine
 #
 # output parameters:
 #
 #	r0 - locates return pc in middle of stack
 #	r1 - contains delta pc originally stored on top of stack
 #
 #	00(sp) - saved r0
 #	04(sp) - saved r1
 #	08(sp) - saved r2
 #	12(sp) - saved r3
 #	16(sp) - size of signal array (always 3)
 #	20(sp) - exception name (ss$_roprand or ss$_radrmod)
 #	24(sp) - place holder for pc of exception
 #	28(sp) - psl of exception
 #	32(sp) - offset to delta pc byte (no longer needed)
 # r0 ->	36(sp) - return pc from vax$xxxxxx routine
 #
 # implicit output:
 #
 #	this routine exits by dropping into the vax$reflect_fault routine
 #	that decides the particular form the instruction backup will take.
 #
 # notes:
 #
 #	there are three ways that a reserved operand exception can occur.
 #
 #	1. digit count of packed decimal string gtru 31 
 #
 #		this is an abort where the pc points to the offending 
 #		decimal or editpc instruction.
 #
 #	2. illegal numeric or sign digit detected by cvtsp or cvttp
 #
 #		an illegal numeric digit was detected by one of these 
 #		instructions or an illegal sign character was detected by 
 #		cvtsp. this exception is also not restartable.
 #
 #	3. illegal editpc pattern operator
 #
 #		the editpc decoder detected an illegal pattern operator. this
 #		exception stores the intermediate state in registers and may
 #		be restarted.
 #
 #	a reserved addressing mode exception can only occur when the pc is 
 #	used as the destination operand in the cvtpl instruction.
 #-

	.globl	vax$radrmod
vax$radrmod:
	movpsl	-(sp)			# store exception psl
	clrl	-(sp)			# save space for the exception pc
	pushl	$ss$_radrmod		# store exception name
	brb	L10			# join common code

	.globl	vax$roprand
vax$roprand:
	movpsl	-(sp)			# store exception psl
	clrl	-(sp)			# save space for the exception pc
	pushl	$ss$_roprand		# store exception name

L10:	pushl	$3			# store signal array size
	pushr	$0x0f			# store the usual registers
	movl	32(sp),r1		# r1 gets delta pc offset
	moval	36(sp),r0		# r0 locates the return pc
					# drop through to vax$reflect_fault 




 #+
 # functional description:
 #
 #	this routine reflects a fault (such as an access violation that 
 #	occurred inside the emulator) back to the user. the signal array, in 
 #	particular, the exception pc, is modified to point to the reserved 
 #	instruction or the jsb instruction into the emulator.
 #
 # input parameters:
 #
 #	r0 - address on stack of return pc
 #	r1<7:0> - byte offset from top of stack into saved register array
 #		  (r0..r3) where delta-pc will be stored if original path into
 #		  emulator was through a reserved instruction exception 
 #	r1<8>   - distinguishes restartable exceptions (faults) from
 #		  exceptions that cannot be restarted (aborts)
 #	r1<9>   - distinguishes software generated exceptions from exceptions
 #		  detected by hardware, detoured through the emulator, and
 #		  modified by instruction-specific routines.
 #
 #	note that the condition codes in the exception psl are significant for
 #	faults, primarily to make the editpc illegal pattern operator
 #	exception conform to the architecture. 
 #
 #	00(sp) - saved r0
 #	04(sp) - saved r1
 #	08(sp) - saved r2
 #	12(sp) - saved r3
 #
 #	16(sp) - number of additional longwords in signal array (called n)
 #	20(sp) - exception name 
 #
 #	if n gtru 3 then
 #	  24(sp)             - first exception-specific parameter
 #	    .
 #	    .
 #	  <4*<n-2> + 16>(sp) - last exception specific parameter
 #
 #	<4*<n-1> + 16>(sp)   - pc of exception
 #	<4*n + 16>(sp)       - psl of exception
 #
 #	<4*n + 16 + 4>(sp)   - instruction specific storage (no longer needed)
 #	    .
 #	    .
 #	-04(r0)		     - last longword of instruction specific storage
 #
 #	(r0) - return pc from vax$xxxxxx routine in emulator
 #
 # there are three possibilities for the return pc. the action of this 
 # routine depends on this return pc value.
 #
 # case 1. 	(r0) - vax$exit_emulator
 #
 #	this is the usual case where the emulator was entered as a result of
 #	an emulated instruction exception. the signal array from the second
 #	exception is put on top of the original exception array, the rest of
 #	the stack is evaporated, and the exception is reflected to the user. 
 #
 #	the fpd bit in the exception psl is set for those exceptions that are
 #	restartable. 
 #
 # case 2. 	(r0) - address of instruction following 
 #
 #			jsb	@$vax$xxxxxx
 #
 #	in this case, the signal array that is passed back to the user is
 #	reflected has the address of the jsb instruction as the exception pc.
 #	the fpd bit is always clear. note that it is much more difficult (if
 #	not impossible) to back up an arbitrary instruction that transfers
 #	control to the emulator. 
 #
 # case 3. 	(r0) - anything else
 #
 #	in this case, the emulator was entered in some other way. because
 #	instruction state has already been modified, it is no longer possible to
 #	simply reflect the secondary exception (as we did with unrecognized
 #	exceptions in routine vax$modify_exception). we add an additional longword
 #	to the signal array (vax$_abort) and report this slightly modified
 #	exception to the original caller without modifying the return pc. 
 #-

	.globl	vax$reflect_fault
vax$reflect_fault:
	movl	(r0)+,r2		# get return pc from stack
 #	cmp_l_to_a	r2,vax$exit_emulator,r3
	 movab	vax$exit_emulator,r3
	 cmpl	r2,r3
	bnequ	no_signal_array		# branch if no secondary signal array

 # if we drop through the branch, we are examining case 1. we can use the signal
 # array that already exists to hold the modified exception parameters.


 # case 1. 	(r0) - vax$exit_emulator
 #
 #	more input parameters:
 #
 #		04(r0) - opcode of reserved instruction
 #		08(r0) - pc of reserved instruction (old pc)
 #		12(r0) - first operand specifier (no longer needed)
 #		  .
 #		  .
 #		40(r0) - eight operand specifier (place holder)
 #		44(r0) - pc of instruction following reserved instruction 
 #			 (new pc)
 #		48(r0) - psl at time of exception
 #
 #	output parameters for case 1:
 #
 #		r0 through r3 restored from top of stack
 #
 #		00(sp) - size of signal array (called n)
 #		04(sp) - exception name 
 #
 #		if n gtru 3 then
 #		  08(sp)        - first exception-specific parameter
 #		    .
 #		    .
 #		  <4*<n-2>>(sp) - last exception specific parameter
 #
 #		<4*<n-1>>(sp)   - old pc (pc of reserved instruction)
 #		<4*n>(sp)       - psl of second exception (fpd set)
 #


 # we need to capture the fpd information stored in r1 before that register
 # is modified (or used as an index register).

	bbcc	$pack_v_fpd,r1,1f	# branch if fpd bit remains clear
	bbss	$psl$v_fpd,exception_psl(r0),1f	# set fpd bit in exception psl
1:	subl3	old_pc(r0),new_pc(r0),r2	# calculate delta pc
	movzbl	r1,r3			# isolate delta-pc offset in r3
	movb	r2,(sp)[r3]		# store delta pc in one of r0..r3
	bbs	$pack_v_accvio,r1,2f	# branch if more than signal array

 # in this case, the signal array is located immediately underneath the
 # saved register array. 

	movzbl	(pack_l_signal_array+chf$l_sig_args)(sp),r3
					# get signal array size
	moval	pack_l_signal_array(sp)[r3],r2	
					# r2 points to exception psl
	brb	3f			# rejoin common code

 # in this case, there is other information on the stack between the saved
 # register array and the signal array, namely a return address in vms, an
 # argument list that would have been passed to condition hanlders had the
 # exception not been detoured through this code, and a mechanism array.
 # all of this extra data is discarded and the exception as the modified
 # stack is passed back to the vms exception dispatcher.

2:	movl	pack_l_signal_array_pointer(sp),r2
					# r2 locates signal array
	movl	chf$l_sig_args(r2),r3	# get signal array size
	moval	chf$l_sig_args(r2)[r3],r2
					# r2 points to exception psl
3:	moval	exception_psl(r0),r1	# r1 points to original psl

 #+
 # r0 - address of vax$_opcdec exception array
 # r1 - address of psl of original vax$_opcdec exception
 # r2 - address of psl in signal array of exception being backed up
 # r3 - number of longwords in signal array
 #-

	movl	old_pc(r0),-4(r2)	# this extra movl actually saves code!

 # there are two more operations that need to be performed on the psl that
 # will appear in the signal array. these operations are only significant for
 # faults. the affected psl fields are defined to be unpredictable in the case
 # of aborts. rather than complicate the code with unnecessary branches,
 # however, the condition codes will be propogated and the tp bit will be
 # cleared in all cases. 

	insv	(r2),$0,$4,(r1)		# copy condition codes to psl	
	bbsc	$psl$v_tp,(r1),4f	# clear the tp bit

 # we now move a modified signal array down the stack, from the back (psl) end
 # to the front (argument count) end. the psl has already been moved before
 # the loop executes to allow the fpd bit to get set and to make the loop
 # count work correctly. 

4:	movl	-(r2),-(r1)		# move next longword
	sobgtr	r3,4b			# check for any more

 # at the end of this loop, r1 points to what we want to report as the top of the
 # stack to the vms exception dispatcher. we store r1 on the stack following the 
 # saved r0 through r3 and set the new stack pointer as part of the popr 
 # instruction.

	movl	r1,pack_l_saved_sp(sp)	# load new sp underneath r0..r3 array
	popr	$0x400f			# restore registers and set sp
	jmp	vax$reflect_to_vms	# use common exit path to vms



 # case 2. 	(r0) - address of instruction following 
 #
 #			jsb	@$vax$xxxxxx
 #
 #	more input parameters:
 #
 #		there is nothing else of interest on the stack in this case.
 #
 #	output parameters for case 2:
 #
 #		r0 through r3 restored from top of stack
 #
 #		00(sp) - size of signal array (called n)
 #		04(sp) - exception name 
 #
 #		if n gtru 3 then
 #		  08(sp)        - first exception-specific parameter
 #		    .
 #		    .
 #		  <4*<n-2>>(sp) - last exception specific parameter
 #
 #		<4*<n-1>>(sp)   - address of jsb instruction
 #		<4*n>(sp)       - psl of second exception (fpd clear!)
 #

 #+
 # this is either case 2 or case 3, depending on the instruction located by
 # the return pc. if this is a return pc from a jsb @$ into the emulator,
 # then the instruction that we wish to examine lies six bytes before the
 # location pointed to by r2.
 #-

no_signal_array:
	cmpw	-6(r2),$jsb_absolute	# is the opcode jsb @$ ?
	bneq	unknown			# branch if not

 ### if we ever install entry points into the emulator through the system
 ### service vector page, the following checks can be turned on. the intent
 ### is that, like system service calls, the emulator references would
 ### generate jsb @$ vax$xxxxxx instructions to jmp instructions in the
 ### vector pages. it is only this set of jsb instructions that would be
 ### backed up according to method 2. all other paths into the emulator
 ### will generate exceptions with an exception pc inside the emulator itself.

 ###	movl	-4(r2),r2		# get destination of jsb @$
 ###	cmp_l_to_a	r2,sys$vax_begin,r3	# make lower bounds check 
 ###	blssu	unknown			# branch if too small
 ###	cmp_l_to_a	r2,sys$vax_end,r3	# make upper bounds check 
 ###	bgequ	unknown			# branch if too large

 # this is case 2 as described above. it differs from case 1 in two ways. there
 # is no signal array on the stack underneath the stack that is overwritten.
 # the fpd bit in the saved psl is not set.

	movzbl	(pack_l_signal_array+chf$l_sig_args)(sp),r3
					# get signal array size
	incl 	r3			# make loop count correct
	moval	pack_l_signal_array(sp)[r3],r2	# r2 points beyond exception psl
	subl3	$jsb_size,-4(r0),-8(r2)	# pc of jsb becomes exception pc
	movl	r0,r1			# start writing signal array at return pc
	brb	4b			# join common exit at top of loop






 # case 3. 	(r0) - anything else
 #
 #		this is a case where the emulator was entered in a nonstandard
 #		way. this code has no way of creating an exception pc that
 #		will cause the emulator to be reentered. instead, the
 #		exception is redefined to be vax$_abort with the rest of the
 #		original signal array comprising the exception parameters. the
 #		pc is not modified so that knowledgable code in the form of a
 #		condition handler could conceivably restart such an exception
 #		if it knew how to reenter the emulator. 
 #
 #	more input parameters:
 #
 #		there is nothing else of interest on the stack in this case.
 #
 #	output parameters for case 3:
 #
 #		r0 through r3 restored from top of stack
 #
 #		00(sp) - size of new signal array (n + 1)
 #		04(sp) - new exception name (vax$_abort)
 #		08(sp) - original exception name 
 #
 #		if n gtru 3 then
 #		  12(sp)          - first exception-specific parameter
 #		    .
 #		    .
 #		  <4*<n-2>+4>(sp) - last exception specific parameter
 #
 #		<4*<n-1>+4>(sp)   - "anything else" (original return pc)
 #		<4*n+4>(sp)       - psl of second exception (fpd clear!)
 #-

 #+
 # this is case 3. the emulator was entered in some unorthodox fashion. we leave
 # the exception pc alone but change the facility field in the exception name
 # located in the signal array to vax$_.
 #-

unknown:
	movzbl	(pack_l_signal_array+chf$l_sig_args)(sp),r3
					# get signal array size
	incl 	r3			# make loop count correct
	moval	pack_l_signal_array(sp)[r3],r2	# r2 points beyond exception psl
	movl	-4(r0),-8(r2)		# unmodified return pc is exception pc
	movl	r0,r1			# start writing signal array at return pc

	brb	4b			# join common exit at top of loop


 #+
 # functional description:
 #
 #	this routine receives control from the various instruction specific
 #	emulator routines in order to reflect arithmetic traps back to the
 #	caller. there are three arithmetic traps that can occur.
 #
 #	decimal overflow
 #
 #		this can occur in most of the decimal instructions and editpc
 #		when there is not enough room in the destination string to 
 #		store all of the nonzero digits in the source.
 #
 #	integer overflow
 #
 #		cvtpl can incur this exception when the input decimal string
 #		converts into a longword that cannot fit into 31 bits.
 #
 #	divide by zero
 #
 #		divp generates this exception when the divisor is zero.
 #
 # input parameters:
 #
 #	00(sp) - arithmetic trap code (from $srmdef)
 #	04(sp) - psl on exit from vax$xxxxxx routine
 #	08(sp) - return pc from vax$xxxxxx routine
 #
 #	the return pc will determine whether an exception frame (signal array)
 #	already exists or must be built. briefly, if 08(sp) is equal to the
 #	address called vax$exit_emulator, then the emulator was entered as
 #	a result of execution of a reserved instruction and a signal array
 #	already exists. if 08(sp) is anything else, then it is treated as the
 #	address of an instruction following a jsb into the emulator.
 #
 # implicit input:
 #
 #	if 08(sp) is vax$exit_emulator, then the rest of the stack that is
 #	relevant looks like this.
 #
 #	12(sp) - opcode of reserved instruction
 #	16(sp) - pc of reserved instruction
 #	20(sp)
 #	  .
 #	  .
 #	48(sp)
 #	52(sp) - pc of next instruction
 #	56(sp) - psl of original exception
 #
 # output parameters:
 #
 #	00(sp) - count of longwords in signal array (always equal to 3)
 #	04(sp) - signal name (modified form of trap code)
 #	08(sp) - pc of next instruction 
 #	12(sp) - psl on exit from vax$xxxxxx routine
 #
 # implicit output:
 #
 #	this routine passes control to exe$reflect, which will eventually
 #	reflect the arithmetic trap back to the user.
 #-

 # insure that architectural status codes for arithmetic traps are consistent
 # with the vms status codes that they are mapped onto.

	.globl	vax$reflect_trap
vax$reflect_trap:
	movq	r0,-(sp)		# get some scratch registers
	mull3	$8,8(sp),r1		# turn trap code into status
	addl2	$ss$_artres,r1		# ... by adding in base status code
	movab	vax$exit_emulator,r0	# this allows address comparisons
	cmpl	r0,16(sp)		# compare with return pc
	beql	1f			# some signal array already exists
	
 #+
 # this code path is taken if the emulator was entered in any way other than
 # by executing one of the reserved instructions. we assume that the longword
 # on the stack represents a return pc, the address of the next instruction
 # that will execute.
 #
 #	r0 - scratch
 #	r1 - modified trap code
 #
 #	00(sp) - saved r0
 #	04(sp) - saved r1
 #	08(sp) - space for trap code
 #	12(sp) - psl on exit from vax$xxxxxx routine
 #	16(sp) - return pc
 #-

	movl	r1,8(sp)		# store exception code
	movl	12(sp),r0		# save psl in r0 to start switch
	movl	16(sp),12(sp)		# put pc into proper place
	movl	r0,16(sp)		# store new exception psl
	movq	(sp)+,r0		# restore saved r0 and r1
	pushl	$3			# store size of signal array
	jmp	vax$reflect_to_vms	# join common exit to vms

 #+
 # this code path is taken if the emulator was entered as a result of executing
 # one of the reserved instructions. there is lots of extra space on the stack
 # to fool around with so the juggling act exhibited by the previous block of
 # code is not necessary here.
 #
 #	r0 - scratch
 #	r1 - modified trap code
 #
 #	00(sp) - saved r0
 #	04(sp) - saved r1
 #	08(sp) - initial trap code (no longer needed)
 #	12(sp) - psl on exit from vax$xxxxxx routine
 #	16(sp) - vax$exit_emulator (no longer needed)
 #	20(sp)
 #	  .
 #	  .
 #	56(sp)
 #	60(sp) - pc of next instruction (preserved by this code)
 #	64(sp) - psl of original exception (with modified condition codes)
 #
 # the condition codes that were generated by the vax$xxxxxx routine are
 # stored in the exception psl.
 #-

1:	insv	12(sp),$0,$4,(exception_psl+20)(sp)
					# store new condition codes
	movl	r1,(operand_8+20)(sp)	# ... and modified trap code
	movq	(sp)+,r0		# restore saved registers
	movab	(operand_8+12)(sp),sp	# eliminate unneeded stack space
	pushl	$3			# store signal array size and ...
					#  drop through to vax$reflect_to_vms










 #+
 # functional description:
 #
 #	this routine is the common exit path to the vms exception dispatcher.
 #	it executes in three different sets of circumstances. 
 #	
 #    1. software detected exceptions 
 #	
 #	there are several exceptions that are detected by software.
 #	exception-specific and context-specific routines build a signal array
 #	om the stack. this code then passes that signal array to vms. 
 #	
 #    2. modified hardware exceptions 
 #	
 #	certain forms of access violation are modified to appear as if they
 #	occurred at the site of a reserved instruction rather than within the
 #	emulator. any extraneous stack storage is removed. the pc within the
 #	access violation signal array is modified. this code then passes the
 #	modified signal array to vms. 
 #	
 #    3. all unmodified exceptions 
 #	
 #	certain exceptions cause the emulator to receive control, even though
 #	the exception in question will be passed intact to vms. these include
 #	exceptions caused by a random control transfer into the emulator and
 #	access violations such as stack overflow that would not have occurred
 #	in the first place if the reserved instructions were being executed by
 #	the base machine rather than by a software emulator. 
 #
 # input parameters:
 #
 #	the signal array is on the stack of the access mode in which the 
 #	exception occurred.
 #
 #		00(sp) - number of signal array elements (called n)
 #		04(sp) - signal name (integer value)
 #		08(sp) - first exception-specific parameter (if any)
 #		12(sp) - second exception-specific parameter (if any)
 #		  .
 #		  .
 #		<8+4*n>(sp)  - exception pc that will be reported
 #		<12+4*n>(sp) - exception psl that will be reported
 #
 # implicit output:
 #
 #	control is passed to the vms exception dispatcher at label exe$reflect.
 #-

	.globl	vax$reflect_to_vms
vax$reflect_to_vms:
	jmp	exe$reflect		# let vms handle this exception

