#ifndef lint
static char *sccsid = "@(#)machpats.c	4.1  (ULTRIX)        7/2/90";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1986,87, 88 by			*
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
/*
 *
 *   Modification History:
 * 
 *  17 Jul 89 -- Adrian Thoms
 *	Replaced movo by movq
 *
 *  1 Jun 89 -- jmartin
 *	un"optimize" blkcpy, blkcmp, and blkclr.  Autoincrement of
 *	a word operand which was pushed as a longword is confusing.
 *
 *  5 May 89 -- Adrian Thoms
 *	Added bbsc, movo
 *
 *  9 Dec 88 -- jmartin
 *	micro-optimize blkcpy, blkcmp, blkclr, bbssi and bbcci
 *
 * 16 Feb 88 -- George M. Mathew
 *	Changes to bbssi/bbcci to return 1 if the bit was successfully
 *	set/cleared and 0 if the bit was already set/cleared
 *
 * 27 Jan 88 -- George M. Mathew
 *	Added bbssi and bbcci for smp support
 *
 * 08-Jan-1988	Todd M. Katz
 *	1. Added pattern for VAX CRC instruction.
 *	2. Modified the insqti/remqhi patterns to eliminate the problems
 *	   which arise when multiple simultaneous accesses are made to
 *	   memory interlocked relative queues.  Such situations occur in SMP
 *	   environments.  They also occur in a single processor environment
 *	   when both the processor and an I/O adapter interface through
 *	   memory interlocked relative queues and are based upon identical
 *	   chip sets( or have comparable processing speeds ).
 * 
 * 12-11-87	Robin L. and Larry C.
 *      Added portclass support to the system.
 *
 * 16-Jul-1986	Todd M. Katz
 *	Add comments to insqti/remqhi.  When lp checked in my
 *	insqti/remqhi inline patterns he neglected to include comments.
 *	These comments are essential to understanding how to make use of
 *	the insqti/remqhi patterns.
 *
 * 9 Apr 86 -- lp
 *	Added insqti/remqhi.
 *
 * 26 Feb 86 -- depp
 *	Added "movpsl".
 *
 */

#include "inline.h"

/*
 * Pattern table for special VAX instructions.
 */
struct pats machine_ptab[] = {

	{ "3,_blkcpy\n",
"	movl	(sp)+,r1\n\
	movl	(sp)+,r3\n\
	jbr	2f\n\
1:\n\
	subl2	r0,(sp)\n\
	movc3	r0,(r1),(r3)\n\
2:\n\
	movzwl	$65535,r0\n\
	cmpl	(sp),r0\n\
	jgtr	1b\n\
	movl	(sp)+,r0\n\
	movc3	r0,(r1),(r3)\n" },

	{ "3,_bcopy\n",
"	movl	(sp)+,r1\n\
	movl	(sp)+,r3\n\
	movl	(sp)+,r5\n\
	movc3	r5,(r1),(r3)\n" },

	{ "4,_crc\n",
"	movl	(sp)+,r1\n\
	movl	(sp)+,r2\n\
	movl	(sp)+,r3\n\
	movl	(sp)+,r4\n\
	crc	0(r1),r2,r3,0(r4)\n" },

	{ "3,_ovbcopy\n",
"	movl	(sp)+,r3\n\
	movl	(sp)+,r4\n\
	movl	(sp)+,r5\n\
	movc3	r5,(r3),(r4)\n" },

	{ "3,_blkcmp\n",
"	movl	(sp)+,r1\n\
	movl	(sp)+,r3\n\
	jbr	2f\n\
1:\n\
	subl2	r0,(sp)\n\
	cmpc3	r0,(r1),(r3)\n\
	bneq	3f\n\
2:\n\
	movzwl	$65535,r0\n\
	cmpl	(sp),r0\n\
	jgtr	1b\n\
	movl	(sp)+,r0\n\
	cmpc3	r0,(r1),(r3)\n\
3:\n" },

	{ "3,_bcmp\n",
"	movl	(sp)+,r1\n\
	movl	(sp)+,r3\n\
	movl	(sp)+,r5\n\
	cmpc3	r5,(r1),(r3)\n" },

	{ "2,_blkclr\n",
"	movl	(sp)+,r3\n\
	jbr	2f\n\
1:\n\
	subl2	r0,(sp)\n\
	movc5	$0,(r3),$0,r0,(r3)\n\
2:\n\
	movzwl	$65535,r0\n\
	cmpl	(sp),r0\n\
	jgtr	1b\n\
	movl	(sp)+,r0\n\
	movc5	$0,(r3),$0,r0,(r3)\n" },

	{ "2,_bzero\n",
"	movl	(sp)+,r3\n\
	movl	(sp)+,r5\n\
	movc5	$0,(r3),$0,r5,(r3)\n" },

	{ "3,_llocc\n",
"	movl	(sp)+,r4\n\
	movl	(sp)+,r5\n\
	movl	(sp)+,r1\n\
1:\n\
	movzwl	$65535,r0\n\
	cmpl	r5,r0\n\
	jleq	1f\n\
	subl2	r0,r5\n\
	locc	r4,r0,(r1)\n\
	jeql	1b\n\
	addl2	r5,r0\n\
	jbr	2f\n\
1:\n\
	locc	r4,r5,(r1)\n\
2:\n" },

	{ "3,_locc\n",
"	movl	(sp)+,r3\n\
	movl	(sp)+,r4\n\
	movl	(sp)+,r5\n\
	locc	r3,r4,(r5)\n" },

	{ "4,_scanc\n",
"	movl	(sp)+,r2\n\
	movl	(sp)+,r3\n\
	movl	(sp)+,r4\n\
	movl	(sp)+,r5\n\
	scanc	r2,(r3),(r4),r5\n" },

	{ "3,_skpc\n",
"	movl	(sp)+,r3\n\
	movl	(sp)+,r4\n\
	movl	(sp)+,r5\n\
	skpc	r3,r4,(r5)\n" },

	{ "2,_insque\n",
"	movl	(sp)+,r4\n\
	movl	(sp)+,r5\n\
	insque	(r4),(r5)\n" },

	{ "1,_remque\n",
"	movl	(sp)+,r5\n\
	remque	(r5),r0\n" },

	{ "0,_movpsl\n",
"	movpsl	r0\n" },

/* This pattern matches the following function call:
 *
 *	u_long	retry_count,	- Number attempts to obtain queue interlock
 *		status;		- Status of interlocked queue insertion
 *	struct	qptr	{
 *		u_long	*flink;
 *		u_long	*blink;
 *		}
 *		*pktptr,	- Address of packet's queue pointers
 *		*qhptr;		- Address of head of interlocked queue
 *
 *		.....
 *
 *	status = insqti( pktptr, qhptr, retry_count );
 *
 *	where:
 *		status <  0	- Successfully inserted packet onto queue
 *		status == 0	- Successfully inserted packet onto
 *				   previously empty queue
 *		status >  0	- Retry_count attempts failed to obtain queue
 *				   interlock, packet not inserted onto queue
 *
 * SMP: Many threads may simultaneously attempt to obtain the same memory queue
 *	interlock in a SMP environment.  When this occurs both of the following
 *	problems may arise:
 *
 *	1. A large amount of very expensive bus traffic may occur as multiple
 *	   attempts to hardware lock the same memory location are made.
 *	2. Lock step synchronization between multiple processors may occur as
 *	   one processor attempts to gain the hardware lock in order to release
 *	   the secondary software lock while others attempt to gain the
 *	   hardware lock in order to obtain the secondary software lock.
 *
 *	Note that these problems also exist in a single processor environment
 *	when both the processor and an I/O adapter interface through memory
 *	interlocked relative queues and are based upon identical chip sets(
 *	or have comparable processing speeds ).
 *
 *	The solution to both of these problem is to avoid making iterative
 *	attempts to obtain the memory queue interlock until either an attempts
 *	succeeds or the retry count is exceeded.  Instead, after each failure
 *	the secondary software lock is directly and iteratively checked.  Not
 *	until the software lock is explicitly seen to be released are further
 *	attempts to obtain the memory queue interlock made.
 *
 *	Direct checking of the secondary software interlock occurs in cache
 *	until the lock is actually released by whomever held it.  This serves
 *	to eliminate both problems by preventing iterative attempts to obtain
 *	the hardware lock until some possibility of actually obtaining it
 *	exists.  This solution does not prevent several processors from trading
 *	the same memory queue interlock back and forth to the exclusion of
 *	others although the probability of this particular scenerio is probably
 *	negligible.
 *
 *	An infinite number of explicit direct checks of the secondary software
 *	lock are not made, nor does each failed check automatically decrement
 *	the number of retries remaining.  Instead, 100 direct checks are made
 *	before decrementing the number of retries remaining and continuing with
 *	the direct checks provided any attempts remain.  This number( 100 ) was
 *	chosen to reflect the relative costs of directly checking the software
 *	lock vs attempting to obtain the memory interlock including the costs
 *	of accessing cache vs the costs of actually accessing memory and
 *	attempting to obtain the hardware lock.
 */
	{ "3,_insqti\n",
"		movl	(sp)+,r1\n\
		movl	(sp)+,r2\n\
		movl	(sp)+,r3\n\
		mnegl	$1,r0\n\
	0:	insqti	0(r1),0(r2)\n\
		bcc	4f\n\
	1:	sobgeq	r3,2f\n\
		movzbl	$1,r0\n\
	 	brb	5f\n\
	2:	movl	$100,r4\n\
	3:	blbc	0(r2),0b\n\
		sobgeq	r4,3b\n\
		brb	1b\n\
	4:	bneq	5f\n\
		clrf	r0\n\
 	5:\n" },

/* This pattern matches the following function call:
 *
 *	u_long	retry_count;	- Number attempts to obtain queue interlock
 *	struct  qptr	{
 *		u_long	*flink;
 *		u_long	*blink;
 *		}
 *		*pktptr,	- System virtual addr of packet's queue ptrs
 *				-  pointers
 *		*qhptr;		- Address of head of interlocked queue
 *
 *		.....
 *
 *	pktptr = ( struct qptr * )remqhi( qhptr, retry_count );
 *
 *	where:
 *		pktptr <  0	- Successfully removed packet from queue
 *		pktptr == 0	- Queue empty, no packet removed
 *		pktptr >  0	- Retry_count attempts failed to obtain queue
 *				   interlock,  packet not removed from queue
 *
 * SMP: See SMP comments for the insqti pattern!
 */
	{ "2,_remqhi\n",
"		movl	(sp)+,r1\n\
		movl	(sp)+,r2\n\
	0:	remqhi	0(r1),r0\n\
		bvc	5f\n\
		bcc	4f\n\
	1:	sobgeq	r2,2f\n\
		movzbl	$1,r0\n\
		brb	5f\n\
	2:	movl	$100,r3\n\
	3:	blbc	0(r1),0b\n\
		sobgeq	r3,3b\n\
		brb	1b\n\
	4:	clrf	r0\n\
	5:\n" },

	{ "1,_remqck\n",
"		movl	(sp)+,r5\n\
		remque	(r5),r0\n\
		bvc	0f\n\
		clrl	r0\n\
	0:\n" },

/* bbssi(bit,base)
 *	bit: bit to be set interlocked (0 <= bit <=31)
 *	base: address of a longword
 *	return values:
 *		1 : if the bit could be successfully set interlocked
 *		0 : if the bit was already set
 */
	{ "2,_bbssi\n",
"		movl	(sp)+,r1\n\
		movl	(sp)+,r2\n\
		clrl	r0\n\
		bbssi	r1,(r2),1f\n\
		incl	r0\n\
	1:\n"},
		
/* bbcci(bit,base)
 *	bit: bit to be cleared interlocked (0 <= bit <= 31)
 *	base: address of a longword
 *	return values:
 *		1: if the bit was cleared interlocked
 *		0: if the bit was already clear
 */
	{ "2,_bbcci\n",
"		movl	(sp)+,r1\n\
		movl	(sp)+,r2\n\
		clrl	r0\n\
		bbcci	r1,(r2),1f\n\
		incl	r0\n\
	1:\n"},


/*
 * bbsc(bit, base)
 *	clear the bit and return if it was set
 *	return values:
 *		1: if the bit was set and was cleared
 *		0: if the bit was clear already
 */
	{"2,_bbsc\n",
"		movl	(sp)+,r1\n\
		movl	(sp)+,r2\n\
		movl	$1,r0\n\
		bbsc	r1,(r2),1f\n\
		clrl	r0\n\
	1:\n" },

/*
 * movq(src, dst)
 *	perform a movq and return zero if target is zero and non-zero if not
 */
	{"2,_movq\n", 
"		movl    (sp)+, r2\n\
		movl    (sp)+, r1\n\
		clrl	r0\n\
		movq    (r2), (r1)\n\
		beql	1f\n\
		incl	r0\n\
	1:\n" },

	{ "", "" }
};
