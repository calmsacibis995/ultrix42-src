 /************************************************************************
 *                                                                      *
 *                      Copyright (c) 1990 by                           *
 *              Digital Equipment Corporation, Maynard, MA              *
 *                      All rights reserved.                            *
 *                                                                      *
 *   This software is furnished under a license and may be used and     *
 *   copied  only  in accordance with the terms of such license and     *
 *   with the  inclusion  of  the  above  copyright  notice.   This     *
 *   software  or  any  other copies thereof may not be provided or     *
 *   otherwise made available to any other person.  No title to and     *
 *   ownership of the software is hereby transferred.                   *
 *                                                                      *
 *   The information in this software is subject to change  without     *
 *   notice  and should not be construed as a commitment by Digital     *
 *   Equipment Corporation.                                             *
 *                                                                      *
 *   Digital assumes no responsibility for the use  or  reliability     *
 *   of its software on equipment which is not supplied by Digital.     *
 *                                                                      *
 ************************************************************************/
/* ------------------------------------------------------------------ */
/* | Copyright Unpublished, MIPS Computer Systems, Inc.  All Rights | */
/* | Reserved.  This software contains proprietary and confidential | */
/* | information of MIPS and its suppliers.  Use, disclosure or     | */
/* | reproduction is prohibited without the prior express written   | */
/* | consent of MIPS.                                               | */
/* ------------------------------------------------------------------ */

 /*
  * Modification History:
  *
  * 	12-May-1990	Paul Grist
  *		Created this file, and the word byte swap routine for
  *          	user level VME support.
  */

#include <asm.h>
#include <regdef.h>

/***************************************************************************
 *
 *   swap_words(buffer);
 *   unsigned long buffer;
 *
 *   RETURNS: resulting byte swapped long
 *
 ***************************************************************************
 *
 *   swap words: will operate on one unsigned 32 bit quantity
 *
 *
 *  			31       |       0
 *			 +---+---+---+---+
 *	start with:	 | a | b | c | d |
 *			 +---+---+---+---+
 *                               |
 *
 *
 *			31       |       0
 *	end with:        +---+---+---+---+
 *			 | c | d | a | b |
 *			 +---+---+---+---+
 *                               |
 *
 ***************************************************************************/

LEAF(swap_words)
#ifdef MIPSEL
	# a0 has the input, unsigned 32 bit quanity - abcd
	sll	v0,a0,16		# shift left 16      - cd00  in v0
	srl	v1,a0,16		# shift right 16     - 00ab  in v1
	or	v0,v1			# v0 <- v0 | v1	     - cdab  in v0
#else
	move	v0,a0			# move v0 into ret value
#endif
	j	ra			# return
.end swap_words

