/*
 * cons60.c for Firefox
 */

/*
	@(#)cons60.c	4.1  (ULTRIX)        7/2/90
 */
/************************************************************************
 *									*
 *			Copyright (c) 1987 by				*
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
 * The physical address of the get_character and put_character routines
 * for Firefox are in the CTSIA.  Here we define the offsets into the
 * CSTIA for the ff_get_character and ff_put_character routines.  The 
 * ff_consinit routine below finds the CTSIA and puts the address 
 * into ctsia.
 */

	.data
ctsia:	.long
	.text

#define	ff_get_character	0xfc	/* offset into CTSIA		      */
#define ff_put_character	0x114	/* offset into CTSIA		      */
#define ff_nvr_ctsia		0x20140514	/* addr of ctsia in nvr	      */

	.globl _c60getc
_c60getc:
	.word 0x3e
1:
	movl	ctsia,r0		# ctsia addr passed in r0
	jsb	*ff_get_character(r0)	# get a character?
	blbc	r0,1b			# no character, wait
	movl	r1,r0			# return the character
	ret

	.globl _c60putc
_c60putc:
	.word 0x3e
1:
	movl	4(ap),r1		# get the character
	movl	ctsia,r0		# pass ctsia addr
	jsb     *ff_put_character(r0)	# Put a character?
	blbc	r0,1b			# no character, wait
	ret
						      
	.globl _c60init
_c60init:
	.word 0x3e
	movl	*$ff_nvr_ctsia,r0
	movl	(r0),ctsia		# get addr of ctsia
	ret
