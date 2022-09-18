/*
 * qdio.c
 */

/*
	@(#)qdio.c	4.1  (ULTRIX)        7/2/90
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
#define CP_GETCHR	0x1c
#define CP_PUTCHRPOLL	0x20
#define CP_PUTCHR	0x24

	.globl _conspg
	.globl _qdgetc
_qdgetc:
	.word 0x800
	movl	_conspg,r11		# point to the console page
1:
	jsb	*CP_GETCHR(r11)		# Get a character?
	blbc	r0, 1b			# No, wait
	movl	r1,r0
	ret

	.globl _qdputc
_qdputc:
	.word 0x800
	movl	_conspg,r11		# point to the console page
1:
	jsb     *CP_PUTCHRPOLL(r11)	# Put a character?
	blbc	r0, 1b			# No, wait
	movl	4(ap),r1		# Restore R1
	jsb     *CP_PUTCHR(r11)		# Echo the character
	movl	$1,r0
	ret
						      
