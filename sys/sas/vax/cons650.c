/*
 * cons650.c for Mayfair (CVAX)
 */

/*
	@(#)cons650.c	4.1  (ULTRIX)        7/2/90
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
 * We must fake a descriptor for the ka650 console output.
 * descriptor format:
 *		length of output string (always 1)
 *		address of buf (where output char will be stuffed)
 * buf:
 *		where we stuff the output char
 * 
 * We pass the address of the "descriptor" to the console output routine
 */
	.data
desc:	.long	1,buf
buf:	.long
	.text

#define get_character		0x20040008
#define output_message		0x2004000c

	.globl _c650getc
_c650getc:
	.word 0x1e
1:
	clrl	r0			# clear timeout
	jsb	*$get_character		# Get a character
	ret

	.globl _c650putc
_c650putc:
	.word 0x1e
1:
	movl	4(ap),buf		# Put character to output in desc
	movab	desc,r0			# Put desc address in r0
	jsb     *$output_message	# Echo the character
	movl	$1,r0			# assume success
	ret
