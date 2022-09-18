/*
 * qio.c
 */
/*
 * @(#)qio.c	4.1	(ULTRIX)	7/2/90
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
#define LOCORE
#include "vmb.h"
	.globl	_SAVE_fp
        .globl  _qio
	.globl	_ROM_buffer
	.globl	_ROM_r0
	.globl	_ROM_r1
	.globl	_ROM_r2
 	.globl	_ROM_r3
 	.globl	_ROM_r4			# needed for calypso
	.globl	_ROM_r5
	.globl	_ROM_r6
	.globl	_ROM_r7			# needed for calypso for CCA addr.
	.globl	_ROM_sp
_qio:
        .word   0xefe			# save r1 - r11
	bitl	$ROM_BOOT,_mode		# is this a ROM mode boot?
	beql	1f			# no, goto read using VMB driver
	/*
	 * ROM Style read
	 */
	movl	_ROM_r0,r0		# restore R0
	bitl	$TK50_BOOT,_mode	# is this a TK50 mode boot?
	beql	3f			# if not, continue
	movl	$IO$_READPBLK,r0	# the EEPROM code wants this
3:
	movl	_ROM_r1,r1		# restore R1
	movl	_ROM_r2,r2		# restore R2
	movl	_ROM_r3,r3		# restore R3
	movl	_ROM_r4,r4		# restore R4
	movl	_ROM_r6,r6		# restore R6
	movl	_ROM_r7,r7		# restore R7
	movl	12(ap),r8		# set up the LBN
	/*
	 * ALWAYS USE THE PAGE ALIGNED BUFFER BECAUSE OF A BUG IN THE
	 * 750 ROM UDA DRIVER.
	 */
	 
	movl	_ROM_buffer,r5		# KLUDGE
	pushl	fp			# save off fp and restore it after 
					# jump into ROM.

	movl	_SAVE_fp,fp		# restore saved off fp (calypso).
	pushl	r5			# set transfer address
	jsb	(r6)			# do the read (1 512 Byte Sector)
	tstl	(sp)+			# pop address from stack
	movl	(sp)+,fp		# restore frame pointer (calypso).

	pushr	$0x01			# Save status in R0
	movl	20(ap),r8		# set transfer address
	/*
	 * MOVE THE DATA BACK TO THE REAL BUFFER ADDRESS
	 */
	movc3	$0x200,(r5),(r8) 	# Move the data back in to the
	popr	$0x01			# Restore status in R0 before returning
	ret				# R0 contains error
1:
	/*
	 * VMB Style read
	 */
	movl	_vmbinfo,r9		# point to the info list
	movl	INFO_RPBBAS(r9),r9	# get the address of the RPB
        pushl   r9			# Push address of RPB
        pushl   4(ap)			# Push address mode
        pushl   8(ap)			# Push I/O function code
        pushl   12(ap)			# Push starting block number
        pushl   16(ap)			# Push transfer size in bytes
        pushl   20(ap)			# Push address of buffer
        calls   $6, *_qioentry		# Call QIO routine
        ret				# Return completion code

	.data
_ROM_buffer:				# contains a page aligned buf address
	.long 0
_ROM_r0:
	.long 0
_ROM_r1:
	.long 0
_ROM_r2:
	.long 0
_ROM_r3:
	.long 0
_ROM_r4:
	.long 0
_ROM_r5:
	.long 0
_ROM_r6:
	.long 0
_ROM_r7:
	.long 0
_ROM_sp:
	.long 0
_SAVE_fp:
	.long 0
