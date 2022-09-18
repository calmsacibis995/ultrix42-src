/*
 * pcsload.c
 *	@(#)pcsload.c	4.1	(ULTRIX)	7/2/90
 */
/************************************************************************
 *									*
 *			Copyright (c) 1986 by				*
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
#define PCS_BITCNT	0x2000		/* Number of patchbits */
#define PCS_MICRONUM 	0x400		/* Number of Microcode locations */
#define PCS_PATCHADDR	0xf00000	/* Beginning addr of patchbits */
#define PCS_PCSADDR	0x8000		/* offset to pcs */
#define PCS_PATCHBIT	0xc000		/* offset to patchbits register */
#define PCS_ENABLE	0xfff00000	/* enable pcs */

/* 
 * FUNCTIONAL DESCRIPTION:
 *
 *	This routine loads the Patchbits.  The Patchbits are extracted
 *	one bit at a time from longwords and written to the Patchbits.
 *	To write to the Patchbits a '1' has to be written to the Patchbit
 *	Enable Register (CMI address F0C000). Upon completion of this routine
 *	the Patchbit Enable Register is cleared.
 *
 * CALLING SEQUENCE:
 *
 *	call
 *
 * REGISTERS USED:
 *
 *	R2 = bit position of patch bit in long word
 *	R3 = address of long word used for bit extraction
 *	R4 = PCS patch bit address
 *	R5 = long word counter
 *	R6 = address of patch bit load select
 *
 */

	.globl _pcsloadpatch
_pcsloadpatch:
	.word	0x7c
	movl	$0,r3			# Starting PA of buffer for Patchbits
	movl	$PCS_PATCHADDR,r4	# Starting PA of Patchbits
	movl	$PCS_BITCNT,r5		# Number of Patchbits to write
	addl3	$PCS_PATCHADDR,$PCS_PATCHBIT,r6	# Address of PER
	movl	$1,(r6)			# Enable writing to Patchbits
	clrl	r2
1:	extzv	r2,$1,(r3),(r4)		# Write Patchbits
	incl	r2			# Next one
	addl2	$4,r4
	sobgtr	r5,1b			# Done ?
	clrl	(r6)			# Disable writing to Patchbits
	ret

/* 
 * FUNCTIONAL DESCRIPTION:
 *
 *	This routine loads PCS with the microcode and then 
 *	exits causing the buffer to be released.
 *	This is a result of the load option with neither
 *	powerfail or resident specified. It is 
 *	used in non battery backed up systems
 *
 * CALLING SEQUENCE:
 *	
 *	call
 *
 * REGISTERS USED:
 *
 *	R1 = microword position in bits
 *	R2 = # of microwords main loop
 *	R3 = base address of source
 *	R4 = address of destination
 *	R5 = short loop control (4/microword)
 *	R6 = store first 20 bits for enable
 *	R7 = buffer header address
 *
 */

	.globl _pcsloadpcs
_pcsloadpcs:
	.word	0xfe			# Save Regs
	addl3	$0,$1024,r3		# Point R3 to start of PCS microcode
	movl	$PCS_MICRONUM,r2
	addl3	$PCS_PATCHADDR,$PCS_PCSADDR,r4	# Map to PCS
	pushl	r4			# Save first address for later enable
	extzv	$0,$20,(r3),r6		# Save first 20 bits for later enable
	clrl	r1
1:	movzwl	$4,r5			# Short loop control
2:	extzv	r1,$20,(r3),(r4)+	# Store one 20 bit unit
	addl2	$20,r1			# Increment BIT position
	sobgtr	r5,2b			# Finish one microword
	sobgtr	r2,1b			# Finish all microwords
	popr	$0x10			# Get back start dest
	bisl3	$PCS_ENABLE,r6,(r4)	# Set and write bits
	ret

