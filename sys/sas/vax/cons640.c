/*
 * cons640.c for VAXstar
 */

/*
	@(#)cons640.c	4.1	(ULTRIX)	7/2/90
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
#define cp$getchar_r4		0x20040044
#define cp$output_one_char	0x20040058

	.globl _c640getc
_c640getc:
	.word 0x2
1:
	clrl	r0			# clear timeout value
	jsb	*$cp$getchar_r4		# Get a character?
	blbc	r0, 1b			# No yet?, wait
	movl	r1,r0			# return the character
	ret

	.globl _c640putc
_c640putc:
	.word 0x4
	movl	4(ap),r2		# get the character
	jsb     *$cp$output_one_char	# Put a character?
	movl	$1,r0			# assume success
	ret
						      
