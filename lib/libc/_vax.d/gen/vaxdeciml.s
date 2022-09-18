 /*	@(#)vaxdeciml.s	4.1	7/3/90	*/
/************************************************************************
 *									*
 *			Copyright (c) 1983 by				*
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
 *   This software is  derived  from  software  received  from  the	*
 *   University    of   California,   Berkeley,   and   from   Bell	*
 *   Laboratories.  Use, duplication, or disclosure is  subject  to	*
 *   restrictions  under  license  agreements  with  University  of	*
 *   California and with AT&T.						*
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
 *	Stephen Reilly, 07-Nov-83
 * 000- Created this module
 *
 ***********************************************************************/

 #		PLEASE READ
 #	This code should only be used internally and is not intended for
 #	outside use.
 #
 #	This code is a combination of two modules that were taken from
 #	the VMS group.  They are vaxdeciml.mar and vaxeditpc.mar.  This
 #	module only contains a subset of the vaxdeciml.mar moodule.  The 
 #	reason for this was that we only needed to emulate a small fraction
 #      of the packed decimal instructions.  
 #
 #	The instructions that are emulated are ashp,
 #	addp4, cvtlp, movp, editpc.  We need to only emulation these
 #	instructions because it is only these instructions that are
 #	used by the either the kernel or utilities.
 #
 #	Notes:
 #
 #	Most of the comments are kept from the orignal author.
 #
 #	There is no error checking for decimal strings that excess
 #	31 digits.
 #
 #	The halt instruction is used to indicate a fatal error has
 #	occured. 
 #

 #
 #	Routines that are global to the rest of the world
 #
	.globl	vax$addp4
	.globl	vax$ashp
	.globl	vax$cvtlp
	.globl	vax$movp
	.globl	vax$editpc
/*
 *	Mask bits for psw
 */

# define psl$m_c 0x01
# define psl$m_v 0x02 
# define psl$m_z 0x04 
# define psl$m_n 0x08 

/*
 *	Bit offsets into psw
 */

# define psl$v_c 0x00
# define psl$v_v 0x01 
# define psl$v_z 0x02 
# define psl$v_n 0x03
# define psl$v_dv 0x07

/*
 *	Symbols used by the editpc instructions
 */

# define blank 0x20
# define minus 0x2d
# define zero 0x30

/*
 *	Bit positions used by the ash instructions
 */

# define ashp_b_cnt 2
# define ashp_v_round 0
# define ashp_s_round 8
# define ashp_b_round 10
# define ashp_w_dstlen 8

	.text
	.align 2

 #++
 # facility: 
 #
 #	vax-11 instruction emulator
 #
 # abstract:
 #
 #	the routines in this module emulate the vax-11 packed decimal 
 #	instructions. these procedures can be a part of an emulator 
 #	package or can be called directly after the input parameters 
 #	have been loaded into the architectural registers.
 #
 #	the input parameters to these routines are the registers that
 #	contain the intermediate instruction state. 
 #
 # environment: 
 #
 #	these routines run at any access mode, at any ipl, and are ast 
 #	reentrant.
 #
 # author: 
 #
 #	lawrence j. kenah	
 #
 #--

 #+
 # there are several techniques that are used throughout the routines in this 
 # module that are worth a comment somewhere. rather than duplicate near 
 # identical commentary in several places, we will describe these general 
 # techniques in a single place.
 #
 # 1.	the vax-11 architecture specifies that several kinds of input produce 
 #	unpredictable results. they are:
 #
 #	 o  illegal decimal digit in packed decimal string
 #
 #	 o  illegal sign specifier (other than 10 through 15) in low nibble of 
 #	    highest addressed byte of packed decimal string
 #
 #	 o  packed decimal string with even number of digits that contains 
 #	    other than a zero in the high nibble of the lowest addressed byte
 #
 #	these routines take full advantage of the meaning of unpredictable. 
 #	in general, the code assumes that all input is correct. the operation 
 #	of the code for illegal input is not even consistent but is simply 
 #	whatever happens to be convenient in a particular place.
 #
 # 2.	all of these routines accumulate information about condition codes at
 #	several key places in a routine. this information is kept in a
 #	register (usually r11) that is used to set the final condition codes
 #	in the psw. in order to allow the register to obtain its correct
 #	contents when the routine exits (without further affecting the
 #	condition codes), the condition codes are set from the register
 #	(bispsw reg) and the register is then restored with a popr
 #	instruction, which does not affect condition codes. 
 #
 # 3.	there are several instances in these routines where it is necessary to
 #	determine the difference in length between an input and an output
 #	string and perform special processing on the excess digits. when the
 #	longer string is a packed decimal string (it does not matter if the
 #	packed decimal string is an input string or an output string), it is
 #	sometimes useful to convert the difference in digits to a byte count. 
 #
 #	there are four different cases that exist. we will divide these cases
 #	into two sets of two cases, depending on whether the shorter length is
 #	even or odd. 
 #
 #	in the pictures that appear below, a blank box indicates a digit in 
 #	the shorter string. a string of three dots in a box indicates a digit 
 #	in the longer string. a string of three stars indicates an unused 
 #	digit in a decimal string. the box that contains +/- obviously 
 #	indicates the sign nibble in a packed decimal string.
 #
 # (cont.)
 #                                               +-------+-------+
 #                                               |       |       |
 #                                               |  ***  |  ...  |
 #                                               |       |       |
 #          +-------+-------+                    +-------+ - - - +
 #          |               |                    |               |
 #          |  ...  |  ...  |                    |  ...  |  ...  |
 #          |               |                    |               |
 #          + - - - +-------+                    + - - - +-------+
 #          |       |       |                    |       |       |
 #          |  ...  |       |                    |  ...  |       |
 #          |       |       |                    |       |       |
 #          +-------+ - - - +                    +-------+ - - - +
 #          |               |                    |               |
 #          |       |       |                    |       |       |
 #          |               |                    |               |
 #          + - - - + - - - +                    + - - - + - - - +
 #          |               |                    |               |
 #          |       |  +/-  |                    |       |  +/-  |
 #          |               |                    |               |
 #          +-------+-------+                    +-------+-------+
 #
 #        a  longer string odd                 b  longer string even
 #           difference odd                       difference even
 #
 #
 #             case 1  shorter string has even number of digits
 #
 #
 #
 #          +-------+-------+                    +-------+-------+
 #          |               |                    |       |       |
 #          |  ...  |  ...  |                    |  ***  |  ...  |
 #          |               |                    |       |       |
 #          + - - - + - - - +                    +-------+ - - - +
 #          |               |                    |               |
 #          |  ...  |  ...  |                    |  ...  |  ...  |
 #          |               |                    |               |
 #          +-------+-------+                    +-------+-------+
 #          |               |                    |               |
 #          |       |       |                    |       |       |
 #          |               |                    |               |
 #          + - - - + - - - +                    + - - - + - - - +
 #          |               |                    |               |
 #          |       |  +/-  |                    |       |  +/-  |
 #          |               |                    |               |
 #          +-------+-------+                    +-------+-------+
 #
 #        a  longer string odd                 b  longer string even
 #           difference even                      difference odd
 #
 #
 #              case 2  shorter string has odd number of digits
 #
 # (cont.)
 #
 #	in general, the code must calculate the number of bytes that contain 
 #	the excess digits. most of the time, the interesting number includes 
 #	complete excess bytes. the excess digit in the high nibble of the 
 #	highest addressed byte (both parts of case 1) is ignored. 
 #
 #	in three out of four cases, the difference (called r5 from this point 
 #	on) can be simply divided by two to obtain a byte count. in one case 
 #	(case 2 b), this is not correct. (for example, 3/2 = 1 and we want to 
 #	get a result of 2.) note, however, that in both parts of case 2, we 
 #	can add 1 to r5 before we divide by two. in case 2 b, this causes the 
 #	result to be increased by 1, which is what we want. in case 2 a, 
 #	because the original difference is even, an increment of one before we 
 #	divide by two has no effect on the final result. 
 #
 #	the correct code sequence to distinguish case 2 b from the other three
 #	cases involves two blbx instructions. a simpler sequence that
 #	accomplishes correct results in all four cases when converting a digit
 #	count to a byte count is something like 
 #
 #			jlbc	length-of-shorter,10$
 #			incl	r5
 #		10$:	ashl	$-1,r5,r5
 #
 #	where the length of the shorter string will typically be contained in 
 #	either r0 or r2.
 #
 #	note that we could also look at both b parts, performing the extra 
 #	incl instruction when the longer string is even. in case 1 b, this 
 #	increment transforms an even difference to an odd number but does not 
 #	affect the division by two. in case 2 b, the extra increment produces 
 #	the correct result. this option is not used in these routines.
 #
 #	the two routines for cvtsp and cvttp need a slightly different number. 
 #	they want the number of bytes including the byte containing the excess 
 #	high nibble. for case 2, the above calculation is still valid. for 
 #	case 1, it is necessary to add one to r5 after the r5 is divided by 
 #	two to obtain the correct byte count.
 #
 # 4.	there is a routine called strip_zeros that removes high order zeros
 #	from decimal strings. this routine is not used by all of the routines
 #	in this module but only by those routines that perform complicated
 #	calculations on each byte of the input string. for these routines, the
 #	overhead of testing for and discarding leading zeros is less than the
 #	more costly per byte overhead of these routines. 
 #-

 #+
 # the following tables are designed to perform fast conversions between
 # numbers in the range 0 to 99 and their decimal equivalents. the tables
 # are used by placing the input parameter into a register and then using
 # the contents of that register as an index into the table.
 #-

 #+
 #	decimal digits to binary number
 #
 # the following table is used to convert a packed decimal byte to its binary
 # equivalent. 
 #
 # packed decimal numbers that contain illegal digits in the low nibble
 # convert as if the low nibble contained a zero. that is, the binary number
 # will be a multiple of ten. this is done so that this table can be used to
 # convert the least significant (highest addressed) byte of a decimal string
 # without first masking off the sign "digit". 
 #
 # illegal digits in the high nibble produce unpredictable results because the
 # table does not contain entries to handle these illegal constructs. 
 #-

 #		binary equivalent		  decimal digits
 #		-----------------		  --------------

packed_to_binary_table:
	.byte	00 , 01 , 02 , 03 , 04 		# index  ^x00
	.byte	05 , 06 , 07 , 08 , 09		#    to  ^x09

	.byte	00 , 00 , 00 , 00 , 00 , 00	# illegal decimal digits

	.byte	10 , 11 , 12 , 13 , 14 		# index  ^x10
	.byte	15 , 16 , 17 , 18 , 19		#    to  ^x19

	.byte	10 , 10 , 10 , 10 , 10 , 10	# illegal decimal digits

	.byte	20 , 21 , 22 , 23 , 24 		# index  ^x20
	.byte	25 , 26 , 27 , 28 , 29		#    to  ^x29

	.byte	20 , 20 , 20 , 20 , 20 , 20	# illegal decimal digits

	.byte	30 , 31 , 32 , 33 , 34		# index  ^x30
	.byte	35 , 36 , 37 , 38 , 39		#    to  ^x39

	.byte	30 , 30 , 30 , 30 , 30 , 30	# illegal decimal digits

	.byte	40 , 41 , 42 , 43 , 44		# index  ^x40
	.byte	45 , 46 , 47 , 48 , 49		#    to  ^x49

	.byte	40 , 40 , 40 , 40 , 40 , 40	# illegal decimal digits

	.byte	50 , 51 , 52 , 53 , 54		# index  ^x50
	.byte	55 , 56 , 57 , 58 , 59		#    to  ^x59

	.byte	50 , 50 , 50 , 50 , 50 , 50	# illegal decimal digits

	.byte	60 , 61 , 62 , 63 , 64		# index  ^x60
	.byte	65 , 66 , 67 , 68 , 69		#    to  ^x69

	.byte	60 , 60 , 60 , 60 , 60 , 60	# illegal decimal digits

	.byte	70 , 71 , 72 , 73 , 74		# index  ^x70
	.byte	75 , 76 , 77 , 78 , 79		#    to  ^x79

	.byte	70 , 70 , 70 , 70 , 70 , 70	# illegal decimal digits

	.byte	80 , 81 , 82 , 83 , 84 		# index  ^x80
	.byte	85 , 86 , 87 , 88 , 89		#    to  ^x89

	.byte	80 , 80 , 80 , 80 , 80 , 80	# illegal decimal digits

	.byte	90 , 91 , 92 , 93 , 94 		# index  ^x90
	.byte	95 , 96 , 97 , 98 , 99		#    to  ^x99

	.byte	90 , 90 , 90 , 90 , 90 , 90	# illegal decimal digits

 #+
 #		binary number to decimal equivalent
 #
 # the following table is used to do a fast conversion from a binary number
 # stored in a byte to its decimal representation. the table structure assumes
 # that the number lies in the range 0 to 99. numbers that lie outside this
 # range produce unpredictable resutls.
 #-

 #		decimal equivalents			     binary
 #		-------------------			     ------

binary_to_packed_table:

	.byte	0x00 , 0x01 , 0x02 , 0x03 , 0x04 	#  0 through  9
	.byte	0x05 , 0x06 , 0x07 , 0x08 , 0x09 	# 

	.byte	0x10 , 0x11 , 0x12 , 0x13 , 0x14	# 10 through 19
	.byte	0x15 , 0x16 , 0x17 , 0x18 , 0x19 	# 

	.byte	0x20 , 0x21 , 0x22 , 0x23 , 0x24	# 20 through 29
	.byte	0x25 , 0x26 , 0x27 , 0x28 , 0x29 	# 

	.byte	0x30 , 0x31 , 0x32 , 0x33 , 0x34	# 30 through 39
	.byte	0x35 , 0x36 , 0x37 , 0x38 , 0x39 	# 

	.byte	0x40 , 0x41 , 0x42 , 0x43 , 0x44 	# 40 through 49
	.byte	0x45 , 0x46 , 0x47 , 0x48 , 0x49 	# 

	.byte	0x50 , 0x51 , 0x52 , 0x53 , 0x54	# 50 through 59
	.byte	0x55 , 0x56 , 0x57 , 0x58 , 0x59 	# 

	.byte	0x60 , 0x61 , 0x62 , 0x63 , 0x64 	# 60 through 69
	.byte	0x65 , 0x66 , 0x67 , 0x68 , 0x69 	# 

	.byte	0x70 , 0x71 , 0x72 , 0x73 , 0x74	# 70 through 79
	.byte	0x75 , 0x76 , 0x77 , 0x78 , 0x79 	# 

	.byte	0x80 , 0x81 , 0x82 , 0x83 , 0x84	# 80 through 89
	.byte	0x85 , 0x86 , 0x87 , 0x88 , 0x89 	# 

	.byte	0x90 , 0x91 , 0x92 , 0x93 , 0x94	# 90 through 99
	.byte	0x95 , 0x96 , 0x97 , 0x98 , 0x99 	# 


 #+
 # functional description:
 #
 #	in 4 operand format, the addend string specified by  the  addend  length
 #	and  addend address operands is added to the sum string specified by the
 #	sum length and sum address operands and the sum string  is  replaced  by
 #	the result.
 #
 # input parameters:
 #
 #	r0 - addlen.rw		number of digits in addend string
 #	r1 - addaddr.ab		address of addend decimal string
 #	r2 - sumlen.rw		number of digits in sum string
 #	r3 - sumaddr.ab		address of sum decimal string
 #
 # output parameters:
 #
 #	r0 = 0
 #	r1 = address of the byte containing the most significant digit of
 #	     the addend string
 #	r2 = 0
 #	r3 = address of the byte containing the most significant digit of
 #	     the string containing the sum
 #
 # condition codes:
 #
 #	n <- sum string lss 0
 #	z <- sum string eql 0
 #	v <- decimal overflow
 #	c <- 0
 #
 # register usage:
 #
 #	this routine uses all of the general registers. the condition codes 
 #	are recorded in r11 as the routine executes.
 #-

vax$addp4:
	pushr	$0xfff		# save registers 0,1,2,3,4,5,6,7,8,9,10,11
	clrl	r9		# this is addition
2:	movq	r2,r4		# let r4 and r5 describe output string


 #+
 # all four routines converge at this point and execute common initialization
 # code until a later decision is made to do addition or subtraction.
 #
 #	r4 - number of digits in destination string
 #	r5 - address of destination string
 #
 #	r9 - indicates whether operation is addition or subtraction
 #		0 => addition
 #		1 => subtraction
 #-

3:	movpsl	r11			# get initial psl
	insv	$psl$m_z,$0,$4,r11	# set z-bit, clear the rest
	movab	decimal_pack,r10	# store address of handler

/* 	No need to check operand size because the routines that call us
 *      all have defined operand sizes.
 *
 *	roprand_check	r2		# insure that r2 is lequ 31
 */
	bsbw	strip_zeros_r2_r3	# strip high order zeros from r2/r3 string

/*	roprand_check	r0		# insure that r0 is lequ 31
 */

	bsbw	strip_zeros_r0_r1	# strip high order zeros from r0/r1 string

 # rather than totally confuse the already complicated logic dealing with
 # different length strings in the add or subtract loop, we will put the
 # result into an intermediate buffer on the stack. this buffer will be long
 # enough to handle the worst case so that the addition loop need only concern
 # itself with the lengths of the two input loops. the required length is 17
 # bytes, to handle an addition with a carry out of the most significant byte.
 # we will allocate 20 bytes to maintain whatever alignment the stack has.

	clrq	-(sp)			# set aside space for output string
	clrq	-(sp)			# worst case string needs 16 bytes
	clrl	-(sp)			# add slack for a carry
	extzv	$1,$4,r4,r8		# get byte count of destination string
	addl3	r8,r5,-(sp)		# save high address end of destination
	movab	24(sp),r5		# point r5 one byte beyond buffer

 # the number of minus signs will determine whether the real operation that we
 # perform is addition or subtraction. that is, two plus signs or two minus
 # signs will both result in addition, while a plus sign and a minus sign will
 # result in subtraction. the addition and subtraction routines have their own
 # methods for determining the correct sign of the result. 
 # 
 # for the purpose of counting minus signs, we treat subtraction as the
 # addition of the negative of the input operand. that is, subtraction of a
 # positive quantity causes the sign to be remembered as minus and counted as
 # a minus sign while subtraction of a minus quantity stores a plus sign and
 # counts nothing. 
 # 
 # on input to this code sequence, r9 distinguished addition from subtraction.
 # on output, it contains either 0, 1, or 2, indicating the total number of
 # minus signs, real or implied, that we counted. 

	extzv	$1,$4,r0,r6		# get byte count for first input string
	addl2	r6,r1			# point r1 to byte containing sign
	bicb3	$0xf0,(r1),r6		# r6 contains the sign "digit"

/*
 *	No need to check this because we only need addition
 *
 *	jlbs	r9,35$			# use second case if subtraction
 */

 # this case statement is used for addition

 	caseb	r6,$10,$15-10
L1:					# dispatch on sign digit
	.word	5f-L1			# 10 => sign is "+"
	.word	4f-L1			# 11 => sign is "-"
	.word	5f-L1			# 12 => sign is "+"
	.word	4f-L1			# 13 => sign is "-"
	.word	5f-L1			# 14 => sign is "+"
	.word	5f-L1			# 15 => sign is "+"

4:	movl	$1,r9			# count a minus sign
	movzbl	$13,r6			# the preferred minus sign is 13
	jbr	L60			# now check second input sign

5:	clrl	r9			# no real minus signs so far
	movzbl	$12,r6			# the preferred minus sign is 12

L60:	extzv	$1,$4,r2,r7		# get byte count for second input string
	addl2	r7,r3			# point r3 to byte containing sign
	bicb3	$0xf0,(r3),r7	# r7 contains the sign "digit"

	caseb	r7,$10,$15-10
L2:					# dispatch
	.word	8f-L2			# 10 => sign is "+"
	.word	7f-L2			# 11 => sign is "-"
	.word	8f-L2			# 12 => sign is "+"
	.word	7f-L2			# 13 => sign is "-"
	.word	8f-L2			# 14 => sign is "+"
	.word	8f-L2			# 15 => sign is "+"

7:	incl	r9			# remember that sign was minus
	movzbl	$13,r7			# the preferred minus sign is 13
	jbr	L90			# now check second input sign

8:	movzbl	$12,r7			# the preferred minus sign is 12

L90:	jlbc	r9,add_packed		# even parity indicates addition

/*
 *	Don't need this because we don't do subtraction
 *
 *	jbr	subtract_packed		# odd parity calls for subtraction
 */

 #+
 # functional description:
 #
 #	this routine adds two packed decimal strings whose descriptors
 #	are passed as input parameters and places their sum into another
 #	(perhaps identical) packed decimal string.
 #
 #	at the present time, the result is placed into a 16-byte storage
 #	area while the sum is being evaluated. this drastically reduces
 #	the number of different cases that must be dealt with as each
 #	pair of bytes in the two input strings is added.
 #
 #	the signs of the two input strings have already been dealt with
 #	so this routine performs addition in all cases, even if the original
 #	entry was at subp4 or subp6. the cases that arrive in this routine
 #	are as follows.
 #
 #                          r2/r3           r0/r1          result
 #                    +---------------+---------------+---------------+
 #                    |               |               |               |
 #      r2/r3 + r0/r1 |     plus      |     plus      |     plus      |
 #                    |               |               |               |
 #                    +---------------+---------------+---------------+
 #                    |               |               |               |
 #      r2/r3 + r0/r1 |     minus     |     minus     |     minus     |
 #                    |               |               |               |
 #                    +---------------+---------------+---------------+
 #                    |               |               |               |
 #      r2/r3 - r0/r1 |     minus     |     plus      |     minus     |
 #                    |               |               |               |
 #                    +---------------+---------------+---------------+
 #                    |               |               |               |
 #      r2/r3 - r0/r1 |     plus      |     minus     |     plus      |
 #                    |               |               |               |
 #                    +---------------+---------------+---------------+
 #
 #	note that the correct choice of sign in all four cases is the sign
 #	of the second input string, the one described by r2 and r3.
 #
 # input parameters:
 #
 #	r0<4:0> - number of digits in first input decimal string
 #	r1      - address of least significant digit of first input 
 #		  decimal string (the byte containing the sign)
 #
 #	r2<4:0> - number of digits in second input decimal string
 #	r3      - address of least significant digit of second input 
 #		  decimal string (the byte containing the sign)
 #
 #	r4<4:0> - number of digits in output decimal string
 #	r5      - address of one byte beyond least significant digit of 
 #		  intermediate string stored on the stack
 #
 #	r6<3:0> - sign of first input string in preferred form
 #	r7<3:0> - sign of second input string in preferred form
 #
 #	r11     - saved psl (z-bit is set, other condition codes are clear)
 #
 #	(sp)	- saved r5, address of least significant digit of ultimate
 #		  destination string.
 #	4(sp)   - beginning of 20-byte buffer to hold intermediate result
 #
 # output parameters:
 #
 #	the particular input operation (addpx or subpx) is completed in
 #	this routine. see the routine headers for the four routines that
 #	request addition or subtraction for a list of output parameters
 #	from this routine.
 #-

add_packed:
	movb	r7,r9			# use sign of second string for output
	jlbc	r9,1f			# check if sign is negative
	bisb2	$psl$m_n,r11		# ... so the saved n-bit can be set
1:	bicb3	$0x0f,(r1),r6		# get least significant digit to r6
	bicb3	$0x0f,(r3),r7		# get least significant digit to r7
	clrl	r8			# start the add with carry off
	bsbw	add_packed_byte_r6_r7	# add the two low order digits

 # the following set of instructions computes the number of bytes in the two
 # strings and, if necessary, performs a switch so that r0 and r1 always
 # describe the shorter of the two strings.

	extzv	$1,$4,r0,r0		# convert digit count to byte count
	extzv	$1,$4,r2,r2		# do it for both strings
	cmpl	r0,r2			# we want to compare the byte counts
	blequ	2f			# skip the swap if we're already correct
	movq	r0,r6			# save the longer
	movq	r2,r0			# store the shorter on r0 and r1
	movq	r6,r2			# ... and store the longer in r2 and r3
2:	subl2	r0,r2			# make r2 a difference (r2 gequ 0)

 # r0 now contains the number of bytes remaining in the shorter string.
 # r2 contains the difference in bytes between the two input strings.

	tstl	r0			# does shorter string have any room?
	beql	4f			# skip loop if no room at all

3:	bsbw	add_packed_byte_string	# add the next two bytes together
	sobgtr	r0,3b			# check for end of loop

4:	tstl	r2			# does longer string have any room?
	beql	7f			# skip next loops if all done

5:	jlbc	r8,6f			# life is simple if carry clear

/*  note that r6 is a loop invariant here and can be removed from the loop if
 *  we carefully document this assumption in the per-byte addition routine. the
 *  invariance arises from the fact that a packed decimal byte containing two
 *  zero digits converts to a byte containing zero.
 */

	clrl	r6			# otherwise, carry must propogate
	movzbl	-(r3),r7		# so add carry to single string
	bsbw	add_packed_byte_r6_r7	# use the special entry point
	sobgtr	r2,5b			# check for this string exhausted
	jbr	7f			# join common completion code

6:	movb	-(r3),-(r5)		# simply move src to dst if no carry
	sobgtr	r2,6b			# ... until we're all done

7:	movb	r8,-(r5)		# store the final carry

 #+
 # at this point, the result has been computed. that result must be moved to
 # its ultimate destination, noting whether any nonzero digits are stored
 # so that the z-bit will have its correct setting. 
 #
 # input parameters:
 #
 #	r9<7:0> - sign of result in preferred form
 #	r11     - saved condition codes
 #
 #	(sp)    - saved r5, high address end of destination string
 #-

add_subtract_exit:
	addl3	$1,(sp),r5		# point r5 beyond real destination 
	movab	24(sp),r1		# r1 locates the saved result
	bsbw	store_result		# store the result and record the z-bit
	bbs	$psl$v_z,r11,9f		# step out of line for minus zero check
8:	insv	r9,$0,$4,*(sp)+		# the sign can finally be stored
	addl2	$20,sp			# get rid of intermediate buffer
	jbr	decimal_exit		# exit through common code path

 # if the result is negative zero, then the n-bit is cleared and the sign
 # is changed to a plus sign.

9:	bicb2	$psl$m_n,r11		# clear the n-bit unconditionally
	bbs	$psl$v_v,r11,8b		# do not change the sign on overflow
	movb	$12,r9			# make sure that the sign is plus
	jbr	8b			# ... and rejoin the exit code

 #+
 # functional description:
 #
 #	this routine adds together two bytes containing decimal digits and 
 #	produces a byte containing the sum that is stored in the output 
 #	string. each of the input bytes is converted to a binary number
 #	(with a table-driven conversion), the two numbers are added, and
 #	the sum is converted back to two decimal digits stored in a byte.
 #
 #	this routine makes no provisions for bytes that contain illegal
 #	decimal digits. we are using the unpredictable statement in the
 #	architectural description of the decimal instructions to its fullest.
 #
 #	the bytes that contain a pair of packed decimal digits can either
 #	exist in packed decimal strings located by r1 and r3 or they can
 #	be stored directly in registers. in the former case, the digits must
 #	be extracted from registers before they can be used in later operations
 #	because the sum will be used as an index register.
 #
 # for entry at add_packed_byte_string:
 #
 #	input parameters:
 #
 #		r1  - address one byte beyond first byte that is to be added
 #		r3  - address one byte beyond second byte that is to be added
 #		r5  - address one byte beyond location to store sum
 #
 #		r8  - carry from previous byte (r8 is either 0 or 1)
 #
 #	implicit input:
 #
 #		r6 - scratch
 #		r7 - scratch
 #
 #	output parameters:
 #
 #	r1 - decreased by one to point to current byte in first input string
 #	r3 - decreased by one to point to current byte in second input string
 #	r5 - decreased by one to point to current byte in output string
 #
 #	r8 - either 0 or 1, reflecting whether this most recent add resulted
 #		     in a carry to the next byte.
 #
 # for entry at add_packed_byte_r6_r7:
 #
 #	input parameters:
 #
 #		r6  - first byte containing decimal digit pair
 #		r7  - second byte containing decimal digit pair
 #
 #		r5  - address one byte beyond location to store sum
 #
 #		r8  - carry from previous byte (r8 is either 0 or 1)
 #
 #	output parameters:
 #
 #		r5 - decreased by one to point to current byte in output string
 #
 #		r8 - either 0 or 1, reflecting whether this most recent add
 #		     resulted
 #		     in a carry to the next byte.
 #
 # side effects:
 #
 #	r6 and r7 are modified by this routine
 #
 #	r0, r2, r4, and r9 (and, of course, r10 and r11) are preserved 
 #	by this routine
 #
 # assumptions:
 #
 #	this routine makes two important assumptions.
 #
 #	1.  if both of the input bytes contain only legal decimal digits, then
 #	    it is only necessary to subtract 100 at most once to put all 
 #	    possible sums in the range 0..99. that is,
 #
 #		99 + 99 + 1 = 199 lss 200
 #
 #	2.  the result will be checked in some way to determine whether the
 #	    result is nonzero so that the z-bit can have its correct setting.
 #-

add_packed_byte_string:
	movzbl	-(r1),r6		# get byte from first string
	movzbl	-(r3),r7		# get byte from second string

add_packed_byte_r6_r7:
	movb	packed_to_binary_table[r6],r6	# convert digits to binary
	movb	packed_to_binary_table[r7],r7	# convert digits to binary
	addb2	r6,r7			# form their sum
	addb2	r8,r7			# add carry from last step
	clrb	r8			# assume no carry this time
	cmpb	r7,$99			# check for carry
	blequ	1f			# branch if within bounds
	movb	$1,r8			# propogate carry to next step
	subb2	$100,r7			# put r7 into interval 0..99
1:	movb	binary_to_packed_table[r7],-(r5) # store converted sum byte
	rsb

 #+
 # functional description:
 #
 #	this routine takes a packed decimal string that typically contains
 #	the result of an arithmetic operation and stores it in another
 #	decimal string whose descriptor is specified as an input parameter 
 #	to the original arithmetic operation.
 #
 #	the string is stored from the high address end (least significant
 #	digits) to the low address end (most significant digits). this order
 #	allows all of the special cases to be handled in the simplest fashion.
 #
 # input parameters:
 #
 #	r1      - address one byte beyond high address end of input string
 #		  (note that this string must be at least 17 bytes long.)
 #
 #	r4<4:0> - number of digits in ultimate destination
 #	r5      - address one byte beyond destination string
 #
 #       r11     - contains saved condition codes
 #
 # implicit input:
 #
 #	the input string must be at least 17 bytes long to contain a potential
 #	carry out of the highest digit when doing an add of two large numbers. 
 #	this carry out of the last byte will be detected and reported as a 
 #	decimal overflow, either as an exception or simply by setting the v-bit.
 #
 #	the least significant digit (highest addressed byte) cannot contain a
 #	sign digit because that would cause the z-bit to be incorrectly cleared.
 #
 # output parameters:
 #
 #	r11<psl$v_z> - cleared if a nonzero digit is stored in output string
 #	r11<psl$v_v> - set if a nonzero digit is detected after the output
 #		       string is exhausted
 #
 #	a portion of the result (dictated by the size of r4 on input) is
 #	moved to the destination string.
 #-

store_result:
	incl	r4			# want number of "complete" bytes in
	ashl	$-1,r4,r0		#  output string
	beql	3f			# skip first loop if none

/* at the cost of one more instruction
 *	bbc	$psl$v_z,r11,20$
 * we can make this loop go faster in the usual case of nonzero result
 */

1:	movb	-(r1),-(r5)		# move the next complete byte
	beql	2f			# check whether to clear z-bit
	bicb2	$psl$m_z,r11		# clear z-bit if nonzero
2:	sobgtr	r0,1b			# keep going?

3:	jlbc	r4,5f			# was original r4 odd? branch if yes
	bicb3	0xf0,-(r1),-(r5)	# if r4 was even, store half a byte
	beql	4f			# need to check for zero here, too
	bicb2	$psl$m_z,r11		# clear z-bit if nonzero	
4:	bitb	$0xf0,(r1)		# if high order nibble is nonzero,
	bneq	7f			# ... then overflow has occurred

 # the entire destination has been stored. we must now check whether any of
 # the remaining input string is nonzero and set the v-bit if nonzero is
 # detected. note that at least one byte of the output string has been examined
 # in all cases already. this makes the next byte count calculation correct.

5:	decl	r4			# restore r4 to its original self
	extzv	$1,$4,r4,r0		# extract a byte count
	subb3	r0,$16,r0		# loop count is 16 minus byte count

 # note that the loop count can never be zero because we are testing a 17-byte
 # string and the largest output string can be 16 bytes long.

6:	tstb	-(r1)			# check next byte for nonzero
	bneq	7f			# nonzero means overflow has occurred
	sobgtr	r0,6b			# check for end of this loop

	rsb				# this is return path for no overflow

7:	bisb2	$psl$m_v,r11		# indicate that overflow has occurred
	rsb				# ... and return to the caller

 #+
 # functional description:
 #
 #     the source string specified by the  source  length  and  source  address
 #     operands is scaled by a power of 10 specified by the count operand.  the
 #     destination string specified by the destination length  and  destination
 #     address operands is replaced by the result.
 #
 #	a positive count  operand  effectively  multiplies#   a  negative  count
 #	effectively  divides#  and a zero count just moves and affects condition
 #	codes.  when a negative count is specified, the result is rounded  using
 #	the round operand.
 #
 # input parameters:
 #
 #	r0<15:0>  = srclen.rw	number of digits in source character string
 #	r0<23:16> = cnt.rb	shift count
 #	r1        = srcaddr.ab	address of input character string
 #	r2<15:0>  = dstlen.rw	length in digits of output decimal string
 #	r2<23:16> = round.rb	round operand used with negative shift count 
 #	r3        = dstaddr.ab 	address of destination packed decimal string
 #
 # output parameters:
 #
 #	r0 = 0
 #	r1 = address of byte containing most significant digit of
 #	     the source string
 #	r2 = 0
 #	r3 = address of byte containing most significant digit of
 #	     the destination string
 #
 # condition codes:
 #
 #	n <- destination string lss 0
 #	z <- destination string eql 0
 #	v <- decimal overflow
 #	c <- 0
 #
 # algorithm:
 #
 #	the routine tries as much as possible to work with entire bytes. this 
 #	makes the case of an odd shift count more difficult that of an even 
 #	shift count. the first part of the routine reduces the case of an odd 
 #	shift count to an equivalent operation with an even shift count.
 #
 #	the instruction proceeds in several stages. in the first stage, after 
 #	the input parameters have been verified and stored, the operation is 
 #	broken up into four cases, based on the sign and parity (odd or even) 
 #	of the shift count. these four cases are treated as follows, in order 
 #	of increasing complexity.
 #
 #	case 1. shift count is negative and even
 #
 #	    the actual shift operation can work with the source string in
 #	    place. there is no need to move the source string to an
 #	    intermediate work area. 
 #
 #	case 2. shift count is positive and even
 #
 #	    the source string is moved to an intermediate work area and the
 #	    sign "digit" is cleared before the actual shift operation takes
 #	    place. if the source is worked on in place, then a spurious sign
 #	    digit is moved to the middle of the output string instead of a
 #	    zero. the alternative is to keep track of where, in the several
 #	    special cases of shifting, the sign digit is looked at. we
 #	    believe that the overhead of the work area is worth the relative
 #	    simplicity of the later stages of this instruction. 
 #
 #	cases 3 and 4. shift count is odd
 #
 #	    the case of an odd shift count is considerably more difficult
 #	    than an even shift count, which is only slightly more complicated
 #	    than movp. in the case of an even shift count, various digits
 #	    remain in the same place (high nibble or low nibble) in a byte.
 #	    for odd shift counts, high nibbles become low nibbles and vice
 #	    versa. in addition, digits that were adjacent when viewing the
 #	    decimal string as a string of bits proceeding from low address to
 #	    high are now separated by a full byte. 
 #
 #	    we proceed in two steps. the source string is first moved to a
 #	    work area. the string is then shifted by one. this shift reduces
 #	    the operation to one of the two even shift counts already
 #	    mentioned, where the source to the shift operation is the
 #	    modified source string residing in the work area. the details of
 #	    the shift-by-one are described below near the code that performs
 #	    the actual shift. 
 #-

# define ashp_shift_mask  0xf0f0f0f0	
					# mask used to shift string by one

vax$ashp:
	pushr	$0xfff			# save registers 0-11
	movpsl	r11			# get initial psl
	insv	$psl$m_z,$0,$4,r11	# set z-bit, clear the rest
	movab	decimal_pack,r10	# store address of handler
	movl	sp,r8			# remember current top of stack
	subl2	$20,sp			# allocate work area on stack
/*	
 *	No need to check bound because we will before calling these
 *	routines
 *	roprand_check	r2		# insure that r2 lequ 31
 *	roprand_check	r0		# insure that r0 lequ 31
 */

	bsbw	strip_zeros_r0_r1	# eliminate any high order zeros
	extzv	$1,$4,r2,r2		# convert output digit count to bytes
	incl	r2			# make room for sign as well
	extzv	$1,$4,r0,r0		# same for input string
	addl3	r0,r1,r6		# get address of sign digit
	incl	r0			# include byte containing sign
	bicb3	$0xf0,(r6),r6		# extract sign digit

/*
 * the following can be done with a six-byte table. is the case really
 * necessary here? what else am i forgetting?
 */
	movzbl	$12,r9			# assume that input sign is plus
	caseb	r6,$10,$15-10
L3:					# dispatch on sign
	.word	4f-L3			# 10 => +
	.word	3f-L3			# 11 => -
	.word	4f-L3			# 12 => +
	.word	3f-L3			# 13 => -
	.word	4f-L3			# 14 => +
	.word	4f-L3			# 15 => +

3:	incl	r9			# change preferred plus to minus
	bisb2	$psl$m_n,r11		# set n-bit in saved psw

 # we now retrieve the shift count from the saved r0 and perform the next set
 # of steps based on the parity and sign of the shift count. note that the
 # round operand is ignored unless the shift count is strictly less than zero.

4:	cvtbl	ashp_b_cnt(r8),r4	# extract sign-extended shift count
	blss	5f			# branch if shift count negative
	clrl	r5			# ignore "round" for positive shift
	bsbw	ashp_copy_source	# move source string to work area
	jlbs	r4,6f			# do shift by one for odd shift count
	bicb2	$0x0f,-(r8)		# drop sign in saved source string
	jbr	ashp_shift_positive	# go do the actual shift

 # the "round" operand is important for negative shifts. if the shift count
 # is even, the source can be shifted directly into the destination. for odd
 # shift counts, the source must be moved into the work area on the stack and
 # shifted by one before the rest of the shift operation takes place.

5:	extzv	$ashp_v_round,$ashp_s_round,ashp_b_round(r8),r5	
					# store "round" in a safe place
	jlbc	r4,ashp_shift_negative	# get right to it for even shift count
	bsbw	ashp_copy_source	# move source string to work area

 # for odd shift counts, the saved source string is shifted by one in place.
 # this is equivalent to a shift of -1 so the shift count (r4) is adjusted
 # accordingly. the least significant digit is moved to the place occupied by
 # the sign, the tens digit becomes the units digit, and so on. because the
 # work area was padded with zeros, this shift moves a zero into the high
 # order digit of a source string of even length. 

6:	pushl	r0			# we need a scratch register to count
	decl	r0			# want to map {1..16} onto {0..3}
	ashl	$-2,r0,r0		# convert a byte count to longwords

 # the following loop executes from one to four times such that the entire
 # source, taken as a collection of longwords, is shifted by one. note that 
 # the two pieces of the source are shifted (rotated) in opposite directions. 
 # note also that the shift mask is applied to one string before the shift and 
 # to the other string after the shift. (this points up the arbitrary choice 
 # of shift mask. we just as well could have chosen the one's complement of 
 # the shift mask and reversed the order of the shift and mask operations for 
 # the two pieces of the source string.)

7:	rotl	$-4,-(r8),r6		# shift left one digit
	bicl2	$ashp_shift_mask,r6	# clear out old low order digits
	bicl3	$ashp_shift_mask,-1(r8),r7	# clear out high order digits
	rotl	$4,r7,r7		# shift these digits right one digit
	bisl3	r6,r7,(r8)		# combine the two sets of digits
	sobgeq	r0,7b			# keep going if more

	movl	(sp)+,r0		# restore source string byte count
	incl	r4			# count the shift we did
	blss	ashp_shift_negative	# join common code at the right place
					# drop through to ashp_shift_positive

 #+
 # functional description:
 #
 #	this routine completes the work of the ashp instruction in the case of
 #	an even shift count. (if the original shift count was odd, the source
 #	string has already been shifted by one and the shift count adjusted by
 #	one.) a portion (from none to all) of the source string is moved to
 #	the destination string. pieces of the destination string at either end
 #	may be filled with zeros. if excess digits of the source are not
 #	moved, they must be tested for nonzero to determine the correct
 #	setting of the v-bit. 
 #
 # input parameters:
 #
 #	r0<3:0> - number of bytes in source string 
 #	r1	- address of source string
 #	r2<3:0> - number of bytes in destination string 
 #	r3	- address of destination string
 #	r4<7:0> - count operand (signed longword of digit count)
 #	r5<3:0> - round operand in case of negative shift
 #	r9<3:0> - sign of source string in preferred form
 #
 # implicit input:
 #
 #	r4 is presumed (guaranteed) even on input to this routine
 #
 #	the top of the stack is assumed to contain a 20-byte work area (that 
 #	may or may not have been used). the space must be allocated for this 
 #	work area in all cases so that the exit code works correctly for all 
 #	cases without the need for lots of extra conditional code.
 #
 # output parameters:
 #
 #	this routine completes the operation of vax$ashp. see the routine
 #	header for vax$ashp for details on output registers and conditon codes.
 #
 # details:
 #
 #	put some of the stuff from ashp.txt here.
 #-


ashp_shift_positive:
	divl2	$2,r4			# convert digit count to byte count
	subl3	r4,r2,r7		# modify the destination count
	blss	3f			# branch if simply moving zeros

	movl	r4,r6			# number of zeros at low order end
1:	subl3	r0,r7,r8		# are there any excess high order digits?
	blss	L260			# no, excess is in source.

 # we only move "srclen" source bytes. the rest of the destination string is
 # filled with zeros.

	movl	r0,r7			# get number of bytes to actually move
	jbr	L200			# ... and go move them

 # the count argument is larger than the destination length. all of the source
 # is checked for nonzero (overflow check). all of the destination is filled
 # with zeros.

3:	movl	r2,r6			# number of low order zeros
	clrl	r7			# the source string is untouched
	movl	r0,r8			# number of source bytes to check
	jbr	L280			# go do the actual work

 # if the count is negative, then there is no need to fill in low order zeros
 # (r6 is zero). the following code is similar to the above cases, differing
 # in the roles played by source length (r0) and destination length (r2) and
 # also in the first loop (zero fill or overflow check) that executes.

ashp_shift_negative:
	clrl	r6			# no zero fill at low end of destination
	mnegl	r4,r4			# get absolute value of count
	divl2	$2,r4			# convert digit count to byte count
	subl3	r4,r0,r7		# get modified source length
	blss	L270			# branch if count is larger 

	subl3	r7,r2,r8		# are there zeros at high end?
	bgeq	L200			# exit to zero fill loop if yes

 # the modified source length is larger than the destination length. part
 # of the source is moved. the rest is checked for nonzero.

	movl	r2,r7			# only move "dstlen" bytes

 # in these cases, some digits in the source string will not be moved. if any
 # of these digits is nonzero, then the v-bit must be set.

L260:	mnegl	r8,r8			# number of bytes in source to check
	jbr	L280			# exit to overflow check loop

 # the count argument is larger than the source length. all of the destination 
 # is filled with zeros. the source is ignored.

L270:	clrl	r7			# no source bytes get moved
	movl	r2,r8			# all of the destination is filled
	jbr	L200			# join the zero fill loop

 #+
 # at this point, the three separate counts have all been calculated. each
 # loop is executed in turn, stepping through the source and destination
 # strings, either alone or in step as appropriate.
 #
 #	r6 - number of low order digits to fill with zero
 #	r7 - number of bytes to move intact from source to destination
 #	r8 - number of excess digits in one or the other string. 
 #
 #	if excess source digits, they must be tested for nonzero to
 #	correctly set the v-bit.
 #
 #	if excess destination bytes, they must be filled with zero.
 #-

 # test excess source digits for nonzero

L285:	tstb	(r1)+			# is next byte nonzero
	bneq	L290			# handle overflow out of line
L280:	sobgeq	r8,L285			# otherwise, keep on looking

	jbr	L320			# join top of second loop

L290:	bisb2	$psl$m_v,r11		# set saved v-bit
	addl2	r8,r1			# skip past rest of excess
	jbr	L320			# join top of second loop

 # in this case, the excess digits are found in the destination string. they
 # must be filled with zero.

L200:	tstl	r8			# is there really something to do?
	beql	L320			# skip first loop if nothing

L310:	clrb	(r3)+			# store another zero
	sobgtr	r8,L310			# ... and keep on looping

 # the next loop is where something interesting happens, namely that parts of
 # the source string are moved to the destination string. note that the use of
 # bytes rather than digits in this operation makes the detection of nonzero 
 # digits difficult because the presence of a nonzero digit in the place 
 # occupied by the sign or in the high order nibble of an even output string 
 # and nowhere else would cause the z-bit to be incorrectly cleared. for this 
 # reason, we ignore the z-bit here and make a special pass over the output 
 # string after all of the special cases have been dealt with. the extra 
 # overhead of a second trip to memory is offset by the simplicity in other 
 # places in this routine.

L320:	tstl	r7			# something to do here?
	beql	L340			# skip this loop if nothing

L330:	movb	(r1)+,(r3)+		# move the next byte
	sobgtr	r7,L330			# ... and keep on looping

 # the final loop occurs in some cases of positive shift count where the low
 # order digits of the destination must be filled with zeros.

L340:	tstl	r6			# something to do here?
	beql	L360			# skip if loop count is zero

L350:	clrb	(r3)+			# store another zero
	sobgtr	r6,L350			# ... until we're done

 #+
 # at this point, the destination string is complete except for the sign.
 # if there is a round operand, that must be added to the destination string.
 #
 #	r3 - address one byte beyond destination string
 #	r5 - round operand
 #-

L360:	addl2	$20,sp			# deallocate work area
	extzv	$1,$4,ashp_w_dstlen(sp),r2	# get original destination byte count
	movq	r2,-(sp)		# save address and count for z-bit loop
	movzbl	r5,r8			# load round into carry register
	beql	L380			# skip next mess unless "round" exists

	movl	r3,r5			# r5 tracks the addition output
	clrl	r6			# we only need one term and carry in sum

L370:	movzbl	-(r3),r7		# get next digit
	bsbw	add_packed_byte_r6_r7	# perform the addition
	tstl	r8			# see if this add produced a carry
	beql	L380			# all done if no more carry
	sobgeq	r2,L370			# back for the next byte

 # if we drop through the end of the loop, then the final add produced a carry.
 # this must be reflected by setting the v-bit in the saved psw.

	bisb2	$psl$m_v,r11		# set the saved v-bit

 # all of the digits are now loaded into the destination string. the condition
 # codes, except for the z-bit, have their correct settings. the sign must be
 # set, a check must be made for even digit count in the output string, and
 # the various special cases (negative zero, decimal overflow trap, ans so on)
 # must be checked before completing the routine. 

 # this entire routine worked with entire bytes, ignoring whether digit counts
 # were odd or even. an illegal digit in the upper nibble of an even input string
 # is ignored. a nonzero digit in the upper nibble of an even output string is
 # not allowed but must be checked for. if one exists, it indicates overflow.

L380:	jlbs	16(sp),L385		# skip next if output digit count is odd
	bitb	$0xf0,*20(sp)		# is most significant digit nonzero?
	beql	L385			# nothing to worry about if zero
	bicb2	$0xf0,*20(sp)		# make the digit zero
	bisb2	$psl$m_v,r11		# ... and set the overflow bit

 # we have not tested for nonzero digits in the output string. this test is
 # made by making another pass over the ouptut string. note that the low
 # order digit is unconditionally checked.

L385:	movq	(sp),r2			# get address and count
	bitb	$0xf0,-(r3)		# do not test sign in low order byte
	bneq	L387			# skip loop if nonzero
	jbr	L386			# start at bottom of loop

L383:	tstb	-(r3)			# is next higher byte nonzero?
	bneq	L387			# exit loop if yes
L386:	sobgeq	r2,L383			# keep looking for nonzero if more bytes

 # the entire output string has been scanned and contains no nonzero
 # digits. the z-bit retains its original setting, which is set. if the
 # n-bit is also set, then the negative zero must be changed to positive
 # zero (unless the v-bit is also set). note that in the case of overflow,
 # the n-bit is cleared but the output string retrins the minus sign.

	bbc	$psl$v_n,r11,L390	# n-bit is off already
	bicb2	$psl$m_n,r11		# turn off saved n-bit unconditionally
	bbs	$psl$v_v,r11,L390	# no fixup if v-bit is also set 
	movb	$12,r9			# use preferred plus as sign of output
	jbr	L390			# ... and rejoin the exit code

 # the following instruction is the exit point for all of the nonzero byte
 # checks. its direct effect is to clear the saved z-bit. it also bypasses
 # whatever other zero checks have not yet been performed.

L387:	bicb2	$psl$m_z,r11		# clear saved z-bit

 # the following code executes in all cases. it is the common exit path for
 # all of the ashp routines when the count is even.

L390:	movq	(sp)+,r2		# get address of end of output string
	insv	r9,$0,$4,-1(r3)		# store sign that we have been saving

 #+
 # this is the common exit path for many of the routines in this module. this
 # exit path can only be used for instructions that conform to the following
 # restrictions.
 #
 # 1.  registers r0 through r11 were saved on entry.
 #
 # 2.  the architecture requires that r0 and r2 are zero on exit.
 #
 # 3.  all other registers that have instruction-specific values on exit are 
 #     correctly stored in the appropriate locations on the stack.
 #
 # 4.  the saved psw is contained in r11
 #
 # 5.  this instruction/routine should generate a decimal overflow trap if 
 #     both the v-bit and the dv-bit are set on exit.
 #-

decimal_exit:
	clrl	(sp)			# r0 must be zero on exit
	clrl	8(sp)			# r2 must also be zero
	bicpsw  $psl$m_n|psl$m_z|psl$m_v|psl$m_c
	bispsw	r11			# set appropriate condition codes
	bbs	$psl$v_v,r11,2f		# see if exceptions are enabled
1:	popr	$0xfff			# restore reg. 0-11
	rsb				# ... and return		

 # if the v-bit is set and decimal traps are enabled (dv-bit is set), then
 # a decimal overflow trap is generated.

2:	bbc	$psl$v_dv,r11,1b	# only return v-bit if dv-bit is clear
	popr	$0x0fff			# restore reg 0-11
 	jbr	decimal_overflow	# report exception

 #+
 # functional description:
 #
 #	for certain cases (three out of four), it is necessary to put the
 #	source string in a work area so that later portions of vax$ashp can
 #	proceed in a straightforward manner. in one case (positive even shift
 #	count), the sign must be eliminated before the least significant
 #	byte of the source is moved to its appropriate place (not the least
 # 	significant byte) in the destination string. for odd shift counts,
 #	the source string in the work area is shifted by one to reduce the
 #	complicated case of an odd shift count to an equivalent but simpler
 #	case with an even shift count.
 #
 #	this routine moves the source string to a 20-byte work area already
 #	allocated on the stack. note that the work area is zeroed by this
 #	routine so that, if the work area is used, it consists of either
 #	valid bytes from the source string or bytes containing zero. if the
 #	work area is not needed (shift count is even and not positive), the
 #	overhead of zeroing the work area is avoided. 
 #
 # input parameters:
 #
 #	r0 - byte count of source string (preserved)
 #	r1 - address of most significant byte in source string
 #	r8 - address one byte beyond end of work area (preserved)
 #
 # output parameters:
 #
 #	r1 - address of most significant byte of source string in
 #	     work area
 #
 # side effects:
 #
 #	r6 and r7 are modified by this routine.
 #-

ashp_copy_source:
	clrq	-8(r8)			# insure that the work area 
	clrq	-16(r8)			# ... is entirely filled
	clrl	-20(r8)			# ... with zeros
	addl3	r0,r1,r7		# r7 points one byte beyond source
	movl	r8,r1			# r1 will step through work area
	movl	r0,r6			# use r6 as the loop counter

1:	movb	-(r7),-(r1)		# move the next source byte
	sobgtr	r6,1b			# check for end of loop

	rsb				# return with r1 properly loaded

 #+
 # functional description:
 #
 #	this routine strips leading (high-order) zeros from a packed decimal 
 #	string. the routine exists based on two assumptions.
 #
 #	1.  many of the decimal strings that are used in packed decimal 
 #	    operations have several leading zeros.
 #
 #	2.  the operations that are performed on a byte containing packed 
 #	    decimal digits are more complicated that the combination of this 
 #	    routine and any special end processing that occurs in the various 
 #	    vax$xxxxxx routines when a string is exhausted.
 #
 #	this routine exists as a performance enhancement. as such, it can only
 #	succeed if it is extremely efficient. it does not attempt to be
 #	rigorous in squeezing every last zero out of a string. it eliminates
 #	only entire bytes that contain two zero digits. it does not look for a
 #	leading zero in the high order nibble of a string of odd length. 
 #
 #	the routine also assumes that the input decimal strings are well 
 #	formed. if an even-length decimal string does not have a zero in its 
 #	unused high order nibble, then no stripping takes place, even though 
 #	the underlying vax$xxxxxx routine may work correctly. 
 #
 #	finally, there is no explicit test for the end of the string. the 
 #	routine assumes that the low order byte, the one that contains the 
 #	sign, is not equal to zero. (note that the routine cannot run forever,
 #	even for garbage, because the digit counts have already been checked
 #	and are lequ 31 on input.)
 #
 # input and output parameters:
 #
 #	there are really two identical but separate routines here. one is
 #	used when the input decimal string descriptor is in r0 and r1. the
 #	other is used when r2 and r3 describe the decimal string. note that
 #	we have already performed the reserved operand checks so that r0 (or
 #	r2) is guaranteed lequ 31.
 #
 #	if the high order digit of an initially even length string is zero,
 #	then the digit count (r0 or r2) is reduced by one. for all other
 #	cases, the digit count is reduced by two as an entire byte of zeros
 #	is skipped.
 #
 # input parameters (for entry at strip_zeros_r0_r1):
 #
 #	r0<4:0> - len.rw	length of input decimal string
 #	r1      - addr.ab	address of input packed decimal string
 #
 # output parameters (for entry at strip_zeros_r0_r1):
 #
 #	r1	advanced to first nonzero byte in string
 #	r0	reduced accordingly (note that if r0 is altered at all,
 #		then r0 is always odd on exit.)
 #
 # input parameters (for entry at strip_zeros_r2_r3):
 #
 #	r2<4:0> - len.rw	length of input decimal string
 #	r3      - addr.ab	address of input packed decimal string
 #
 # output parameters (for entry at strip_zeros_r2_r3):
 #
 #	r3	advanced to first nonzero byte in string
 #	r2	reduced accordingly (note that if r2 is altered at all,
 #		then r2 is always odd on exit.)
 #-

 # this routine is used when the decimal string is described by r0 (digit
 # count) and r1 (string address).

strip_zeros_r0_r1:
	blbs	r0,1f			# skip first check if r0 starts out odd
	tstb	(r1)+			# is first byte zero?
	bneq	2f			# all done if not
	decl	r0			# skip leading zero digit (r0 nequ 0)

1:	tstb	(r1)+			# is next byte zero?
	bneq	2f			# all done if not	
	subl2	$2,r0			# decrease digit count by 2
	brb	1b			# ... and charge on

2:	decl	r1			# back up r1 to last nonzero byte
	rsb

 # this routine is used when the decimal string is described by r2 (digit
 # count) and r3 (string address).

strip_zeros_r2_r3:
	blbs	r2,1f			# skip first check if r2 starts out odd
	tstb	(r3)+			# is first byte zero?
	bneq	2f			# all done if not
	decl	r2			# skip leading zero digit (r2 nequ 0)

1:	tstb	(r3)+			# is next byte zero?
	bneq	2f			# all done if not	
	subl2	$2,r2			# decrease digit count by 2
	brb	1b			# ... and charge on

2:	decl	r3			# back up r3 to last nonzero byte
	rsb

 #+
 # functional description:
 #
 #	the source operand is converted to  a  packed  decimal  string  and  the
 #	destination  string  operand  specified  by  the  destination length and
 #	destination address operands is replaced by the result.
 #
 # input parameters:
 #
 #	r0 - src.rl		input longword to be converted
 #	r2 - dstlen.rw		length of output decimal string
 #	r3 - dstaddr.ab		address of output packed decimal string
 #
 # output parameters:
 #
 #	r0 = 0
 #	r1 = 0
 #	r2 = 0
 #	r3 = address of byte containing most significant digit of
 #	     the destination string
 #
 # condition codes:
 #
 #	n <- destination string lss 0
 #	z <- destination string eql 0
 #	v <- decimal overflow
 #	c <- 0
 #
 # register usage:
 #
 #	this routine uses r0 through r5. the condition codes are recorded
 #	in r4 as the routine executes.
 #
 # notes:
 #
 #;;	the following comment needs to be updated to reflect the revised
 #;;	algorithm.
 #
 #	the algorithm used in this routine builds the packed decimal from 
 #	least significant digit to most significant digit, by repeatedly 
 #	dividing by 10 and placing the resulting remainder in the next most 
 #	significant output digit. this process continues until the quotient 
 #	goes to zero.
 #
 #	no special processing is observed for either an input longword of zero
 #	or an output string length of zero. the correct results for these cases
 #	drop out of normal processing.
 #-

vax$cvtlp:
	pushr	$0x0c30			# save registers 4,5,10,11
	movab	decimal_pack,r10	# store handler address

 # get initial settings for condition codes. the initial settings for v and c
 # will be zero. the initial setting of n depends on the sign of the source
 # operand. the z-bit starts off set and remains set until a nonzero digit is
 # stored in the output string. note that the final z-bit may be set for
 # nonzero input if the output string is not large enough. (the v-bit is set
 # in this case.) in this case, the saved dv bit will determine whether to
 # reflect an exception or merely report the result to the caller. 

	movpsl	r11			# get dv bit from psl on input
	insv	$psl$m_z,$0,$4,r11	# start with z-bit set, others clear
/*
 *	Do not need this check since we will check it before
 *	we call these routines
 *
 *	roprand_check	r2		# insure that r2 lequ 31
 */
	ashl	$-1,r2,r1		# convert digit count to byte count
	addl2	r1,r3			# get address of sign byte
	movb	$12,(r3)		# assume that sign is plus
	clrl	r1			# prepare r1 for input to ediv
	tstl	r0			# check sign of source operand
	bgeq	1f			# start getting digits if not negative

 # source operand is minus. we remember that by setting the saved n-bit but work
 # with the absolute value of the input operand from this point on.

	incl	(r3)			# convert "+" to "-" (12 -> 13)
	mnegl	r0,r0			# normalize source operand
	bisb2	$psl$m_n,r11		# set n-bit in saved psw

 #+ 
 # the first (least significant) digit is obtained by dividing the source 
 # longword by ten and storing the remainder in the high order nibble of the
 # sign byte. note that at this point, the upper four bits of the sign byte
 # contain zero.
 #-

1:	movl	r2,r4			# special exit if zero source length
	beql	9f			# only overflow check remains
	ediv	$10,r0,r0,r5		# r5 gets remainder, first digit
	ashl	$4,r5,r5		# shift digit to high nibble position
	beql	2f			# leave z-bit alone if digit is zero
	bicb2	$psl$m_z,r11		# turn off z-bit if nonzero
	addb2	r5,(r3)			# merge this digit with low nibble
2:	decl	r4			# one less output digit
	beql	9f			# no more room in output string
	ashl	$-1,r4,r4		# number of complete bytes remaining
	beql	8f			# check for last digit if none
	tstl	r0			# is source exhausted?
	bneq	3f			# go get next digits if not
	clrb	-(r3)			# store a pair of zeros
	jbr	5f			# fill rest of output with zeros

 #+
 # the following loop obtains two digits at a time from the source longword. it
 # accomplishes this by dividing the current value of r0 by 100 and converting
 # the remainder to a pair of decimal digits using the table that converts
 # binary numbers in the range from 0 to 99 to their packed decimal equivalents.
 # note that this technique may cause nonzero to be stored in the upper nibble
 # of the most significant byte of an even length string. this condition will
 # be tested for at the end of the loop.
 #-

3:	ediv	$100,r0,r0,r5		# r5 gets remainder, next digit
	movb	binary_to_packed_table[r5],-(r3) # store converted remainder
	beql	4f			# leave z-bit alone if digit is zero
	bicb2	$psl$m_z,r11		# turn off z-bit if nonzero
4:	tstl	r0			# is source exhausted?
	beql	5f			# exit loop is no more source
	sobgtr	r4,3b			# check for end of loop
	
	jbr	8f			# check for remaining digit

 # the following code executes if the source longword is exhausted. if there
 # are any remaining digits in the destination string, they must be filled
 # with zeros. note that one more byte is cleared if the original input length
 # was odd. this includes the most significant digit and the unused nibble.

5:
	jlbs	r2,0f			# one less byte to zero if odd input length

6:	clrb	-(r3)			# set a pair of digits to zero
0:	sobgtr	r4,6b			# any more digits to zero?

 # the following code is the exit path for this routine. note that all
 # code paths that arrive here do so with r2 containing zero. in addition, r0
 # always contains zero at this point. r1, however, must be cleared on exit. 

7:	clrq	r1			# comform to architecture
	bicpsw	$psl$m_n|psl$m_z|psl$m_v|psl$m_c	# clear condition codes
	bispsw	r11			# set appropriate condition codes
	popr	$0x0c30			# restore regs 4,5,10,11
	rsb

 #+
 # the following code executes when there is no more room in the destination
 # string. we first test for the parity of the output length and, if even, 
 # determine whether a nonzero digit was stored in the upper nibble of the 
 # most significant byte. such a nonzero store causes an overflow condition.
 #
 # if the source operand is not yet exhausted, then decimal overflow occurs.
 # if decimal overflow exceptions are enabled, an exception is signalled.
 # otherwise, the v-bit in the psw is set and a normal exit is issued. note
 # that negative zero is only an issue for this instruction when overflow
 # occurs. in the no overflow case, the entire converted longword is stored in
 # the output string and there is only one form of binary zero. 
 #-

8:	jlbs	r2,9f			# no last digit if odd output length
	ediv	$10,r0,r0,r5		# get next input digit
	movb	r5,-(r3)		# store in last output byte
	beql	9f			# leave z-bit alone if zero
	bicb2	$psl$m_z,r11

9:	tstl	r0			# is source also all used up?
	beql	7b			# yes, continue with exit processing

 # an overflow has occurred. if the z-bit is still set, then the n-bit is cleared
 
 # note that, because all negative zero situations occur simultaneously with
 # overflow, the output sign is left as minus. 

1:	clrl	r0			# r0 must be zero on exit
	bbc	$psl$v_z,r11,2f		# z-bit and n-bit cannot both be set
	bicb2	$psl$m_n,r11		# clear n-bit if z-bit still set
2:	bisb2	$psl$m_v,r11		# set v-bit in saved psw

/*
 * the following logic screws up the stack and registers and needs to be fixed.
 * perhaps i should simply reproduce the exit code or some such.
 */
	jbc	$psl$v_dv,r11,7b	# simply exit if exceptions disabled
  	pushab	decimal_overflow	# otherwise, signal exception
	jbr	7b			# ... after setting up final state

 #+
 # functional description:
 #
 #	the destination string specified by the length and  destination  address
 #	operands  is  replaced  by the source string specified by the length and
 #	source address operands.
 #
 # input parameters:
 #
 #	r0 - len.rw		length of input and output decimal strings
 #	r1 - srcaddr.ab		address of input packed decimal string
 #	r3 - dstaddr.ab		address of output packed decimal string
 #
 # output parameters:
 #
 #	r0 = 0
 #	r1 = address of byte containing most significant digit of
 #	     the source string
 #	r2 = 0
 #	r3 = address of byte containing most significant digit of
 #	     the destination string
 #
 # condition codes:
 #
 #	n <- destination string lss 0
 #	z <- destination string eql 0
 #	v <- 0
 #	c <- c				# note that c-bit is preserved!
 #
 # register usage:
 #
 #	this routine uses r0 through r3. the condition codes are recorded
 #	in r2 as the routine executes.
 #-

vax$movp:
	movpsl	r2			# save initial psl (to preserve c-bit)
/*
 *	Don't need to check this because when we call this we
 *	know the string sizes
 *	roprand_check	r0		# insure that r0 lequ 31
 */
	pushl	r3			# save starting addresses of output
	pushl	r2			# ... and input strings. store a
	pushl	r1			# place holder for saved r2.

 ### it would help if symbolics were used in the following instruction

	insv	$psl$m_z&-1,$1,$3,r2	# set z-bit. clear n- and v-bits.
	extzv	$1,$4,r0,r0		# convert digit count to byte count
	beql	3f			# skip loop if zero or one digit

 ### at the cost of one more instruction
 ###	bbc	#psl$v_z,r11,20$
 ### we can make this loop go faster in the usual case of nonzero result

1:	movb	(r1)+,(r3)+		# move next two digits
	beql	2f			# leave z-bit alone if both zero
	bicb2	$psl$m_z,r2		# otherwise, clear saved z-bit
2:	sobgtr	r0,1b			# check for end of loop

 # the last byte must be processed in a special way. the digit must be checked
 # for nonzero because that affects the condition codes. the sign must be
 # transformed into the preferred form. the n-bit must be set if the input
 # is negative, but cleared in the case of negative zero.

3:	movb	(r1),r0			# get last input byte (r1 now scratch)
	bitb	$0xf0,r0		# is digit nonzero?
	beql	4f			# branch if zero
	bicb2	$psl$m_z,r2		# otherwise, clear saved z-bit	
4:	bicb3	$0xf0,r0,r1		# sign "digit" to r1
 # assume that the sign is "+". if the input sign is minus, one of the several
 # fixups that must be done is to change the output sign from "+" to "-".

	insv	$12,$0,$4,r0		# 12 is preferred plus sign
	caseb	r1,$10,$15-10		# dispatch on sign type
L4:
	.word	6f-L4			# 10 => +
	.word   5f-L4			# 11 => -
	.word	6f-L4			# 12 => +
	.word	5f-L4			# 13 => -
	.word	6f-L4			# 14 => +
	.word	6f-L4			# 15 => +

 # input sign is "-"

5:	bbs	$psl$v_z,r2,6f		# treat as "+" if negative zero
	incl	r0			# 13 is preferred minus sign
	bisb2	$psl$m_n,r2		# set n-bit

 # input sign is "+" or input is negative zero. nothing special to do.

6:	movb	r0,(r3)			# move modified final digit
	clrl	r0			# r0 and r2 must be zero on output
	clrl	4(sp)			# ... but need r2 so clear saved r2
	bicpsw	$psl$m_n|psl$m_z|psl$m_v|psl$m_c	# clear all codes
	bispsw	r2			# reset codes as appropriate
	popr	$0xe			# restore reg 1-3
	rsb				# return

 #+
 # functional description:
 #
 #	tbs
 #
 # input parameters:
 #
 #	r0 - srclen.rw		length of input packed decimal string
 #	r1 - srcaddr.ab		address of input packed decimal string
 #	r3 - pattern.ab		address of table of editing pattern operators
 #	r5 - dstaddr.ab		address of output character string
 #
 # output parameters:
 #
 #	r0 - length of input decimal string
 #	r1 - address of most significant byte of input decimal string
 #	r2 - 0
 #	r3 - address of byte containing eo$end pattern operator
 #	r4 - 0
 #	r5 - address of one byte beyond destination character string
 #
 # condition codes:
 #
 #	n <- source string lss 0	(src = -0 => n = 0)
 #	z <- source string eql 0
 #	v <- decimal overflow		(nonzero digits lost)
 #	c <- significance
 #-

vax$editpc:
 	pushr	$0xbc3				# save reg 0,1,6,7,8,9,11
/*	
 *	Don't need to to this because when we call this routine
 *	we will check this
 *
 *	cmpw	r0,$31			# check for r0 gtru 31
 *	bgtru	5$			# signal roprand if r0 gtru 31
 */
	movzwl	r0,r0			# clear any junk from high-order word
	movzbl	$blank,r2		# set fill to blank, stored in r2
	movpsl	r11			# get current psl
	bicb2	$psl$m_n|psl$m_v|psl$m_c,r11	# clear n-, v-, and c-bits
	bisb2	$psl$m_z,r11		# set z-bit. 

 # we need to determine the sign in the input decimal string to choose
 # the initial setting of the n-bit in the saved psw.

	extzv	$1,$4,r0,r6		# get byte offset to end of string
	addl2	r1,r6			# get address of byte containing sign
	extzv	$0,$4,(r6),r6		# get sign "digit" into r6

	caseb	r6,$10,$15-10		# dispatch on sign
L5:
	.word	2f-L5			# 10 => +
	.word	1f-L5			# 11 => -
	.word	2f-L5			# 12 => +
	.word	1f-L5			# 13 => -
	.word	2f-L5			# 14 => +
	.word	2f-L5			# 15 => +

 # sign is minus

1:	bisb2	$psl$m_n,r11		# set n-bit in saved psw
	movzbl	$minus,r4		# set sign to minus, stored in r4
	brb	top_of_loop		# join common code

 # sign is plus (but initial contents of sign register is blank)

2:	movzbl	$blank,r4		# set sign to blank, stored in r4

 # the architectural description of the editpc instruction uses an exit flag
 # to determine whether to continue reading edit operators from the input
 # stream. this implementation does not use an explicit exit flag. rather, all
 # of the end processing is contained in the routine that handles the eo$end
 # operator.

 # the next several instructions are the main routine in this module. each
 # pattern is used to dispatch to a pattern-specific routine that performs
 # its designated action. these routines (except for eo$end) return control
 # to top_of_loop to allow the next pattern operator to be processed.

top_of_loop:
	pushab	top_of_loop			# store "return pc"

 # the following instructions pick up the next byte in the pattern stream and
 # dispatch to a pattern specific subroutine that performs the designated
 # action. control is passed back to the main editpc loop by the rsb
 # instructions located in each pattern-specific subroutine. 

 # note that the seemingly infinite loop actually terminates when the eo$end
 # pattern operator is detected. that routine insures that we do not return
 # to this loop but rather to the caller of vax$editpc.

	caseb	(r3)+,$0,$4
L6:
	.word	eo$end_routine-L6		# 00 - eo$end
	.word	eo$end_float_routine-L6		# 01 - eo$end_float
	.word	eo$clear_signif_routine-L6	# 02 - eo$clear_signif
	.word	eo$set_signif_routine-L6	# 03 - eo$set_signif
	.word	eo$store_sign_routine-L6	# 04 - eo$store_sign

 	caseb	-1(r3),$0x40,$7
L8:
 	.word	eo$load_fill_routine-L8		# 40 - eo$load_fill
	.word	eo$load_sign_routine-L8		# 41 - eo$load_sign
 	.word	eo$load_plus_routine-L8		# 42 - eo$load_plus
 	.word	eo$load_minus_routine-L8	# 43 - eo$load_minus
	.word	eo$insert_routine-L8		# 44 - eo$insert
 	.word	eo$blank_zero_routine-L8	# 45 - eo$blank_zero
 	.word	eo$replace_sign_routine-L8	# 46 - eo$replace_sign
 	.word	eo$adjust_input_routine-L8	# 47 - eo$adjust_input

	bitb	$0x0f,-1(r3)		# check for 80, 90, or a0	
	beql	3f			# reserved operand on repeat of zero
	extzv	$4,$4,-1(r3),r6		# ignore repeat count in dispatch

	caseb	r6,$8,$10-8
L7:
	.word	eo$fill_routine-L7		# 81 to 8f - eo$fill
	.word	eo$move_routine-L7		# 91 to 9f - eo$move
	.word	eo$float_routine-L7		# a1 to af - eo$float

 # if we drop through all three case instructions, the pattern operator is
 # unimplemented or reserved. r3 is backed up to point to the illegal
 # pattern operator and a reserved operand fault is signalled.

3:	decl	r3			# point r3 to illegal operator
	addl2	$4,sp			# discard return pc
	brw	editpc_roprand_fault	# initiate exception processing
 #+
 # functional description:
 #
 #	there is a separate action routine for each pattern operator. these 
 #	routines are entered with specific register contents and several 
 #	scratch registers at their disposal. they perform their designated 
 #	action and return to the main vax$editpc routine.
 #
 #	there are several words used in the architectural description of this
 #	instruction that are carried over into comments in this module. these
 #	words are briefly mentioned here.
 #
 #	char	character in byte following pattern operator (used by
 #		eo$load_fill, eo$load_sign, eo$load_plus, eo$load_minus,
 #		and eo$insert)
 #
 #	length	length in byte following pattern operator (used by
 #		eo$blank_zero, eo$replace_sign, and eo$adjust_input)
 #
 #	repeat	repeat count in bits <3:0> of pattern operator (used by
 #		eo$fill, eo$move, and eo$float)
 #
 #	the architecture makes use of two character registers, described
 #	as appearing in different bytes of r2. for simplicity, we use an
 #	additional register.
 #
 #	fill	stored in r2<7:0>
 #
 #	sign	stored in r4<7:0> 
 #
 #	finally, the architecture describes two subroutines, one that obtains
 #	the next digit from the input string and the other that stores a 
 #	character in the output string. 
 #
 #	read	subroutine eo_read provides this functionality
 #
 #	store	a single instruction of the form
 #
 #			movb	xxx,(r5)+
 #
 #		stores a single character and advances the pointer.
 #
 # input parameters:
 #
 #	r0 - updated length of input decimal string
 #	r1 - address of next byte of input decimal string
 #	r2 - fill character
 #	r3 - address of one byte beyond current pattern operator
 #	r4 - sign character 
 #	r5 - address of next character to be stored in output character string
 #
 # implicit input:
 #
 #	several registers are used to contain intermediate state, passed
 #	from one action routine to the next.
 #
 #	r9  - contains the value described in the architecture as r0<31:16>
 #	r11 - pseudo-psw that contains the saved condition codes
 #
 #	r4<31:16> preserves the original input value of r0
 #
 # side effects:
 #
 #	the remaining registers are used as scratch by the action routines.
 #	
 #	r6 - scratch register used by vax$editpc
 #	r7 - output parameter of eo_read routine
 #	r8 - scratch register used by pattern-specific routines
 #
 # output parameters:
 #
 #	the actual output depends on the pattern operator that is currently
 #	executing. the routine headers for each routine will describe the
 #	specific output parameters.
 #-
 #+
 # functional description:
 #
 #	this routine reads the next character from the input decimal
 #	string, encodes it into ascii, and passes it back to the caller.
 #
 # input parameters:
 #
 #	r0 - updated length of input decimal string
 #	r1 - address of next byte of input decimal string
 #	r9 - count of extra zeros (see eo$adjust_input)
 #
 #	note that r9<15:0> contains the data that is described by the
 #	architecture as appearing in r0<31:16>. in the event of an exception,
 #	the contents of r9<15:0> will be stored in r0<31:16>, either to
 #	conform to the architectural specification of register contents in
 #	the event of a reserved operand abort, or to allow the instruction
 #	to be restarted in the event of an access violation.
 #
 # output parameters:
 #
 #	r9 is zero on input
 #
 #		r0 - updated by one 
 #		r1 - updated by one if r0<0> is clear on input
 #		r7 - ascii code for next decimal digit in input string
 #		r9 - unchanged
 #
 #	r9 is nonzero (lss 0) on input
 #
 #		r0 - unchanged
 #		r1 - unchanged
 #		r7 - ascii code for zero (30 hex)
 #		r9 - incremented by one (toward zero)
 #
 # notes:
 #
 #	there are two return pcs on the stack. this may screw up exception
 #	dispatching as it is currently implemented.
 #-

eo_read:
	tstl	r9			# check for "r0" lss 0
	bneq	3f			# special code if nonzero
	tstl	r0			# insure that digits still remain
	beql	4f			# reserved operand if none
	blbs	r0,1f			# next code path is flip flop

 # r0 is even, indicating that we want low order nibble in input stream. the
 # input pointer r1 must be advanced to point to the next byte.

	extzv	$0,$4,(r1)+,r7		# load low order nibble into r7
	brb	2f			# join common code

 # r0 is even, indicating that we want high order nibble in input stream.
 # the next pass through this routine will pick up the low order nibble
 # of the same input byte.

1:	extzv	$4,$4,(r1),r7		# load high order nibble into r7

2:	addb2	$zero,r7		# convert digit to ascii
	decl	r0			# one less digit in input stream
	rsb

 # r9 was nonzero on input, indicating that zeros should replace the original
 # input digits.

3:	movb	$zero,r7		# return ascii code for zero
	incl	r9			# advance r9 toward zero
	rsb

 # the input decimal string ran out of digits before its time. the architecture
 # dictates that r3 points to the pattern operator that requested the input
 # digit and r0 contains a -1 when the reserved operand abort is reported.

4:	decl	r0			# set r0 to -1
	decl	r3			# back up r3 to current pattern operator
	brw	editpc_roprand_abort	# branch aid for reserved operand abort
 #+
 # functional description:
 #
 #	insert a fixed character, substituting the fill character if
 #	not significant.
 #
 # input parameters:
 #
 #	r2 - fill character
 #	r3 - address of character to be inserted if significance is set
 #	r5 - address of next character to be stored in output character string
 #	r11<c> - current setting of significance
 #
 # output parameters:
 #
 #	character in pattern stream (or fill character if no significance)
 #	is stored in the the output string.
 #
 #	r3 - advanced beyond character in pattern stream
 #	r5 - advanced one byte as a result of the store operation
 #-


eo$insert_routine:
	bbc	$psl$v_c,r11,1f		# skip next if no significance
	movb	(r3)+,(r5)+		# store "ch" in output string
	rsb

1:	movb	r2,(r5)+		# store fill character
	incl	r3			# skip over unused character
	rsb
 #+
 # functional description:
 #
 #	the contents of the sign register are placed into the output string.
 #
 # input parameters:
 #
 #	r4 - sign character
 #	r5 - address of next character to be stored in output character string
 #
 # output parameters:
 #
 #	sign character is stored in the the output string.
 #
 #	r5 - advanced one byte as a result of the store operation
 #-


eo$store_sign_routine:
	movb	r4,(r5)+		# store sign character
	rsb
 #+
 # functional description:
 #
 #	the contents of the fill register are placed into the output string
 #	a total of "repeat" times. 
 #
 # input parameters:
 #
 #	r2 - fill character
 #	r5 - address of next character to be stored in output character string
 #
 #	-1(r3)<3:0> - repeat count is stored in right nibble of pattern operator
 #
 # output parameters:
 #
 #	fill character is stored in the output string "repeat" times
 #
 #	r5 - advanced "repeat" bytes as a result of the store operations
 #-


eo$fill_routine:
	extzv	$0,$4,-1(r3),r8		# get repeat count from pattern operator
1:	movb	r2,(r5)+		# store fill character
	sobgtr	r8,1b			# test for end of loop
	rsb
 #+
 # functional description:
 #
 #	tbs
 #-

eo$move_routine:
	extzv	$0,$4,-1(r3),r8		# get repeat count

1:	bsbw	eo_read			# get next input digit
	cmpb	$zero,r7		# is this digit zero?
	beql	3f			# branch if yes
	bisb2	$psl$m_c,r11		# indicate significance 
	bicb2	$psl$m_z,r11		# also indicate nonzero
2:	movb	r7,(r5)+		# store digit in output stream
	brb	4f			# join common end of loop

3:	bbs	$psl$v_c,r11,2b		# if significance, then store digit
	movb	r2,(r5)+		# otherwise, store fill character

4:	sobgtr	r8,1b			# test for end of loop

	rsb
 #+
 # functional description:
 #
 #	tbs
 #-

eo$float_routine:
	extzv	$0,$4,-1(r3),r8		# get repeat count

1:	bsbw	eo_read			# get next input digit
	cmpb	$zero,r7		# is this digit zero?
	beql	3f			# branch if yes
	bbs	$psl$v_c,r11,2f	# if no significance, then store sign
	movb	r4,(r5)+		# store sign
	bisb2	$psl$m_c,r11		# indicate significance 
	bicb2	$psl$m_z,r11		# also indicate nonzero
2:	movb	r7,(r5)+		# store digit in output stream
	brb	4f			# join common end of loop

3:	bbs	$psl$v_c,r11,2b		# if significance, then store digit
	movb	r2,(r5)+		# otherwise, store fill character

4:	sobgtr	r8,1b			# test for end of loop

	rsb
 #+
 # functional description:
 #
 #	if the floating sign has not yet been placed into the destination
 #	string (that is, if significance is not yet set), then the contents
 #	of the sign register are stored in the output string and significance 
 #	is set.
 #
 # input parameters:
 #
 #	r4 - sign character
 #	r5 - address of next character to be stored in output character string
 #	r11<c> - current setting of significance
 #
 # output parameters:
 #
 #	sign character is optionally stored in the output string (if 
 #	significance was not yet set).
 #
 #	r5 - optionally advanced one byte as a result of the store operation
 #	r11<c> - (significance) is unconditionally set
 #-

eo$end_float_routine:
	bbss	$psl$v_c,r11,1f		# test and set significance
	movb	r4,(r5)+		# store sign character
1:	rsb
 #+
 # functional description:
 #
 #	if the value of the source string is zero, then the contents of the
 #	fill register are stored into the last "length" bytes of the
 #	destination string.
 #
 # input parameters:
 #
 #	r2 - fill character
 #	r3 - address of "length", number of characters to blank
 #	r5 - address of next character to be stored in output character string
 #	r11<z> - set if input string is zero
 #
 # output parameters:
 #
 #	contents of fill register are stored in last "length" characters
 #	of output string if input string is zero.
 #
 #	r3 - advanced one byte over "length"
 #	r5 - unchanged
 #
 # side effects:
 #
 #	r8 is destroyed
 #-


eo$blank_zero_routine:
	movzbl	(r3)+,r8		# get length
	bbc	$psl$v_z,r11,2f		# skip rest if source string is zero
	subl2	r8,r5			# back up destination pointer
1:	movb	r2,(r5)+		# store fill character
	sobgtr	r8,1b			# check for end of loop
2:	rsb
 #+
 # functional description:
 #
 #	if the value of the source string is zero, then the contents of the
 #	fill register are stored into the byte of the destination string
 #	that is "length" bytes before the current position.
 #
 # input parameters:
 #
 #	r2 - fill character
 #	r3 - address of "length", number of characters to blank
 #	r5 - address of next character to be stored in output character string
 #	r11<z> - set if input string is zero
 #
 # output parameters:
 #
 #	contents of fill register are stored in byte of output string
 #	"length" bytes before current position if input string is zero.
 #
 #	r3 - advanced one byte over "length"
 #	r5 - unchanged
 #
 # side effects:
 #
 #	r8 is destroyed
 #-

eo$replace_sign_routine:
	movzbl	(r3)+,r8		# get length
	bbc	$psl$v_z,r11,1f		# skip rest if source string is zero
	subl3	r8,r5,r8		# get address of indicated byte
	movb	r2,(r8)			# store fill character
1:	rsb
 #+
 # functional description:
 #
 #	the contents of the fill or sign register are replaced with the
 #	character that follows the pattern operator in the pattern stream.
 #
 #	eo$load_fill	load fill register
 #
 #	eo$load_sign	load sign register
 #
 #	eo$load_plus	load sign register if source string is positive (or zero)
 #
 #	eo$load_minus	load sign register if source string is negative
 #
 # input parameters:
 #
 #	r3 - address of character to be loaded
 #	r11<n> - set if input string is lss zero (negative)
 #
 # output parameters:
 #
 #	if entry is at eo$load_fill, the fill register contents (r2<7:0>) are 
 #	replaced with the next character in the pattern stream. 
 # 
 #	if one of the other entry points is used (and the appropriate conditions
 #	obtain), the contents of the sign register are replaced with the next
 #	character in the pattern stream. for simplicity of implementation, the
 #	sign character is stored in r4<7:0> while this routine executes. 
 #
 #	in the event of an exception, the contents of r4<7:0> will be stored
 #	in r2<15:8>, either to conform to the architectural specification of
 #	register contents in the event of a reserved operand fault, or to
 #	allow the instruction to be restarted in the event of an access
 #	violation. 
 #
 #	r3 - advanced one byte over new fill or sign character
 #-


eo$load_fill_routine:
	movb	(r3)+,r2		# load new fill character
	rsb

eo$load_sign_routine:
	movb	(r3)+,r4		# load new sign character into r4
	rsb

eo$load_plus_routine:
 	bbc	$psl$v_n,r11,eo$load_sign_routine # use common code if plus
	incl	r3			# otherwise, skip unused character
	rsb

eo$load_minus_routine:
 	bbs	$psl$v_n,r11,eo$load_sign_routine # use common code if minus
	incl	r3			# otherwise, skip unused character
	rsb
 #+
 # functional description:
 #
 #	the significance indicator (c-bit in auxiliary psw) is set or
 #	cleared according to the entry point.
 #
 # input parameters:
 #
 #	none
 #
 # output parameters:
 #
 #	eo$clear_signif		r11<c> is cleared
 #
 #	eo$set_signif		r11<c> is set 
 #-

eo$clear_signif_routine:
	bbcc	$psl$v_c,r11,1f		# clear significance
1:	rsb

eo$set_signif_routine:
	bbss	$psl$v_c,r11,2f		# set significance
2:	rsb
 #+
 # functional description:
 #
 #	tbs
 #-

eo$adjust_input_routine:
 #	mark_point	adjust_input_1
	movzbl	(r3)+,r8		# get "length" from pattern stream
	subl3	r8,r0,r8		# is length larger than input length?
	blequ	3f			# branch if yes
	clrl	r9			# clear count of zeros ("r0<31:16>")

1:	bsbw	eo_read			# get next input digit
	cmpb	$zero,r7		# is it zero?
	beql	2f			# skip to end of loop if zero
	bicb2	$psl$m_z,r11		# otherwise, indicate nonzero
	bisb2	$psl$m_c|psl$m_v,r11	# indicate significance and overflow
2:	sobgtr	r8,1b			# test for end of loop
	rsb

3:	movl	r8,r9			# store difference into "r0<31:16>"
	rsb
 #+
 # functional description:
 #
 #	the architectural description of editpc divides end processing between
 #	the eo$end routine and code at the end of the main loop. this 
 #	implementation performs all of the work in a single place.
 #
 #	the edit operation is terminated. there are several details that this
 #	routine must take care of.
 #
 #	1.  the return pc to the main dispatch loop is discarded.
 #
 #	2.  r3 is backed up to point to the eo$end pattern operator.
 #
 #	3.  a special check must be made for negative zero to insure that
 #	    the n-bit is cleared.
 #
 #	4.  if any digits still remain in the input string, a reserved
 #	    operand abort is taken.
 #
 #	5.  r2 and r4 are set to zero according to the architecture.
 #
 # input parameters:
 #
 #	r0 - number of digits remaining in input string
 #	r3 - address of one byte beyond the eo$end operator
 #
 #	(sp)  - return address in dispatch loop in this module (discarded)
 #	4(sp) - return address to caller of vax$editpc
 #
 # output parameters:
 #
 #	r2 - set to zero to conform to architecture
 #	r3 - backed up one byte to point to eo$end operator
 #	r4 - set to zero to conform to architecture
 #-


eo$end_routine:
	addl2	$4,sp			# discard return pc to main loop
	decl	r3			# back up pattern pointer one byte
	bbc	$psl$v_z,r11,1f		# check for negative zero
	bicb2	$psl$m_n,r11		# turn off n-bit if zero
1:	tstl	r0			# any digits remaining?
	bneq	editpc_roprand_fault	# error if yes
	tstl	r9			# any zeros (r0<31:16>) remaining?
	bneq	editpc_roprand_fault	# error if yes
	clrl	r2			# architecture specifies that r2
	clrl	r4			#  and r4 are zero on exit
	bicpsw	$psl$m_n|psl$m_z|psl$m_v|psl$m_c	# clear condition codes
	bispsw	r11			# set codes according to saved psw
	bbs	$psl$v_v,r11,3f		# get out of line if overflow
2:
	popr	$0xbc3			# retore reg 0,1,6,7,8,9,11
	rsb				# return to caller's caller

3:	bbc	$psl$v_dv,r11,2b	# back in line if we're not interested
	brb	decimal_overflow	# otherwise, signal overflow trap

 #+
 #	We come here if we have detected a fatal error
 #-
decimal_overflow:
decimal_pack:
editpc_roprand_fault:
editpc_roprand_abort:

	halt
