/*
 * GENERIC Ultrix-32/32m bootblock
 */

/*
 * @(#)bootblk.c	4.1	(ULTRIX)	7/2/90
 */
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


/***********************************************************************
 *
 * Modification History:
 *
 * 06-Jun-90  rafiey (Ali Rafieymehr)
 *	For VAXes, flush "rei" has to be performed for instruction streams.
 *	We have not done them before and we have been fortunate. VAX9000 
 *	wouldn't boot without fixing the problem.
 *
 **********************************************************************/

#define LOCORE
#include "vmb.h"

/*
    This block contains both MVAX bootblock descriptor information which
    points to itself.  The block is executed upon entry at location 2 for
    VMB startup and location 0xc for 11/750 and 8200 ROMs.

	The first part of MVAX descriptor information must be as
	follows:

		+---------+---------+---------+---------+
	BB+0	|    1    |    n    |    Any value      |
		+---------+---------+---------+---------+
	BB+4	|    Low LBN        |    High LBN       |
		+---------+---------+---------+---------+

		n = anything, but is used to position the second piece
		    of the MVAX descriptor table at (n*2).
		    0xcf is used with this bootblock to allow a dummy
		    `casel' instruction to gobble up the next 5 bytes 
		    as operands and fall through to a `brb' instruction
		    at location start+8

	The second part of the MVAX descriptor information is described
	below.
 */
	.text
start:

/*
 * TK50 ENTRY
 */
/*00*/	brb	tk50_start		# tk50 entry point
					# not used as part of MVAX descriptor
/* 
 * VMB ENTRY
 */
	/* Location 2 and the next 5 bytes comprise a casel 
 	 * instruction which is intended to act as a `nop' used to 
 	 * get to location 0x08 where we branch to the vmb code.
 	 */
/*02*/	.byte	0xcf			# VMB entry point VAX 
					# - casel opcode
/*03*/	.byte	0x01			# MUST BE 1
	/*
	 * The next two words describe the lbn of the bootblock on a MVAX.
	 * Notice they descirbe this same block.
	 */
/*04*/	.word	0x00			# High 16 bits of secondary boot lbn
/*06*/	.word	0x00			# Low 16 bits of secondary boot lbn
					# load this block on MVAX & execute it
/*08*/	brb	vmb_start 		# fall through to here on VMB entry

/*
 * MVAX/VMB ENTRY
 */
/*0a*/	brb	mvax_start		# MVAX/VMB Entry

/*
 * 11/750 STYLE ROM ENTRY
 */
/*0c*/	brw	rom_start 		# 11/750 and 8200 entry

/*
 * VMB MODE Boots have the following inputs
 *	
 *	r10 = base address of secondary bootstrap
 *	r11 = physical address of base of RPB
 *	AP  = physical address of the VMB ARG list
 *	SP  = current stack pointer (physical)
 *	PR$_SCBB = physical address of SCB
 */
tk50_start:
	xorl2	$TK50_BOOT,mode		# Booting a TK50
mvax_start:
	xorl2	$VMB_BOOT,mode		# Booting using VMB
	brb	exec			# jump directly to the main code

vmb_start:
	xorl2	$VMB_BOOT,mode		# Booting using VMB
	cmpb	$BTD$K_AIE_TK50,RPB$B_DEVTYP(r11)
					# is this a TK50 on an AIE?
	bneq	1f			# if not, continue
	xorl2	$TK50_BOOT,mode		# Booting a TK50
1:
	movl	RPB$L_IOVEC(r11),r7	# Compute address of bootdriver
	addl3	r7,BQO$L_QIO(r7),qioentry # and place it in qioentry
	moval	start+0x200,bufptr	# new buffer ptr is 2nd page
	pushl	r11			# Push address of RPB
	pushl	$PHYSMODE		# Physical read mode
	pushl	$IO$_READLBLK		# Read logical block function
	pushl	$1			# Starting block number for VAX
	pushl	$BOOTSZ			# Transfer size in bytes
	pushl	bufptr			# Address of buffer
	calls	$6, *qioentry		# Call bootstrap QIO routine
	blbs	r0,exec			# Check status
	cmpw	$SS$_ENDOFFILE,r0	# Is this an end of file from tape?
	beql	exec			# continue if it is
	halt				# Halt on I/O error

exec:
	moval	start+0x204,entry	# compute address of startup code
	movpsl	-(sp)			# save the psl
	movab	1f,-(sp)		# save the return address
	rei				# pop
1:
	pushl	ap			# pass the address of the VMB arglist
	pushl	r11			# RPB is pointed to by r11
	pushl	mode			# Tell the next image how to boot
	calls	$3,*entry		# Call the startup code
	halt				# Die

/*
 * 11/750 and 8200 and VAX6200 ENTRY
 *
 * Inputs:
 *
 *	R0	- type of boot device
 *	R1	- (UNIBUS) address of the I/O page for the boot device's
 *	  	   UNIBUS
 *		  (MASSBUS) address of the device's MASSBUS adapter
 *	R2	- (UNIBUS) 32-bit physical address of the boot device's
 *			   CSR (bits <31:24> must be zero)
 *		  (MASSBUS) adapter's controller/formatter number
 *	R3	- unit number of the boot device
 *	R5	- software boot control flags
 *	R6	- physical address of the device-dependent ROM routine
 *		  that reads an arbitrary LBN into memory
 *
 *	FP	- used by calypso boot ROM for parameters
 *	SP	- <base_address + ^X200> of 64kb of good memory
 *
 */
rom_start:
	xorl2	$ROM_BOOT,mode		# Booting using 750/8200 boot ROM
	cmpb	$BTD$K_AIE_TK50,r0	# is this a TK50 on an AIE?
	bneq	1f			# if not, continue
	xorl2	$TK50_BOOT,mode		# Booting a TK50
	brb	skip_read		# now, skip the read
1:
	pushr	$0x131			# Save registers r0,r4,r5,r8 for temps.
	movl	$BBLKS,r4		# Get number of blocks in VAXBOOT.
	movzbl	$1,r8			# Get starting LBN.
	moval	start+0x200,r5		# Get primary boot relative load addr.
	pushl	r5			# Copy physical transfer address to
					# top of stack for those devices such
					# as the TU58 which need physical
					# rather than UNIBUS virtual addresses.

read_block:				# vaxboot read loop.
	jsb	(r6)			# Call ROM read LBN routine.
	blbs	r0,next_block		# Branch on successful read.
	halt				# Halt on failure to read.

next_block:				# Read next block.
	addl2	$0x200,r5		# Increment relative address 512 bytes.
	addl2	$0x200,(sp)		# Increment physical address one page.
	incl	r8			# Increment LBN number.
	sobgtr	r4,read_block		# If more blocks, loop.
	/*
 	* The primary bootstrap program is now in physical memory starting at
 	* the specified load address. Restore the saved registers, convert the
 	* CSR address to an 18-bit UNIBUS address, and transfer control to the
 	* program.
 	*/
	tstl	(sp)+			# pop now useless info from stack
	popr	$0x131			# Restore registers.
skip_read:
	moval	start+0x204,entry	# compute the address of startup code
	pushl	fp			# needed for calypso.. 
	pushl	mode			# Tell the next image how to boot
	calls	$1,*entry		# Call the startup code
	halt				# Die

rpbas:	.long	0 			# Pointer to start of RPB	
qioentry: .long	0	 		# Pointer to VMB boot driver
entry:	.long 	0 			# Pointer to bootblk extension program
mode:	.long 	0 			# type of boot
bufptr:	.long	0 			# Pointer to start of buffer

/*
    This is the second part of the MVAX bootblock descriptor
    information.  It is structured as follows:

                +---------+---------+---------+---------+
     .+0        |  Chk    |       k |             0x18  |
                +---------+---------+---------+---------+
     .+4        |    any value, most likely 0           |
                +---------+---------+---------+---------+
     .+8        |  size in blocks of the image          |
                +---------+---------+---------+---------+
     .+12       |  load offset                          |
                +---------+---------+---------+---------+
     .+16       |  offset into image to start           |
                +---------+---------+---------+---------+
     .+20       |  sum of the previous 3 longwords      |
                +---------+---------+---------+---------+

	   Where:
		1) the 0x18 indicates this is a VAX instruction set
		2) Chk = ~(0x18 + k)  ones complement of the sum

 */

ident:				# beginning of identification region
	.org	0x19e		# contents of location 1 times 2.  (0xcf*2)
/*.+0*/	.word	0x18		# signifies VAX instruction set
/*.+2*/	.byte	0x54		# signifies "VAX 4.2BSD file system format"
/*.+3*/	.byte	~(0x18+0x54)	# complement of previous two value
/*.+4*/	.long	0		# Anything
/*.+8*/	.long	(BBLKS+1)	# size (512-byte blks) of secondary boot image
/*.+12*/.long	0x00		# Load offset into good memory
/*.+16*/.long	0x0a		# transfer address of secondary boot image
/*.+20*/.long	(BBLKS+1+0x00+0x0a) # longword sum of previous three longwords

