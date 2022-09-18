/*	@(#)atomic_op.s	4.1     ULTRIX        7/3/90	*/
  
/************************************************************************
 *									*
 *			Copyright (c) 1989 by				*
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
 *		Modification History
 *
 *
 ************************************************************************/

/*
 * int atomic_op (op, addr)
 * int op;
 * unsigned int addr;
 *
 * atomic_op returns 1 if atomic operation is performed.
 * It returns a 0 if the operation wasn't performed because the lock
 * was already set.  Interlocked clear always succeeds.
 * 
 * If operation type is not a valid value, errno is set to EINVAL.
 * If operation is not performed because lock already set, errno is
 * set to EBUSY.
 */

#include "DEFS.h"
#define	KERNEL
#include <errno.h>
#include <sys/lock.h>
#undef	KERNEL

	.text

	.globl	_errno
	.globl	_atomic_op

_atomic_op:
	.word	0
	movl	4(ap),r0	# fetch operation code.
	movl	8(ap),r2	# fetch address.
	movl	$ATOMIC_LOCKBIT,r1 # bit 31 is soft lock.	
	
	cmpl	r0,$ATOMIC_CLEAR# check if valid operation code.
	bgtru   err_inval	# if not, go to error handler.

	bbs	$0,r0,clear	# if low bit clear, do clear operation.
	
set:	clrl	r0
	bbssi	r1,(r2),err_busy # interlocked set if not set.
	ret

clear:	bbcci	r1,(r2),1f	# interlocked clear.
1:	clrl	r0		# always succeed.
	ret

err_inval:
	movl	$-1,r0		# mark as failed.
	movl	$EINVAL,_errno  # Invalid command.
	ret
	
err_busy:
	movl	$-1,r0
	movl	$EBUSY,_errno   # Bit already set.
	ret

		




