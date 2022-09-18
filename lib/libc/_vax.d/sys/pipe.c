/*
 * @(#)pipe.c	4.1	(ULTRIX)	7/3/90
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
 *			Modification History
 *
 *	David L Ballenger, 30-May-1985
 * 001	Test accessibility of file fildes argument.  Return -1 and set
 *	errno to EFAULT if not.
 *
 ************************************************************************/
/* pipe.c 4.1 82/12/04 */

#include "SYS.h"
#define KERNEL
#include <errno.h>

	.globl	cerror

SYSCALL(pipe)
	movl	4(ap),r2	# get address of "int fildes[2]
	probew	$3,$8,(r2)	# make sure we can write to it
	beql	fault		
	movl	r0,(r2)+	# put file descriptors in
	movl	r1,(r2)		# the fildes argument
	clrl	r0
	ret
fault:	movl	$EFAULT,r0	# Couldn't write to fildes
	jmp	cerror
