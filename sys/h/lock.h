

/* 	@(#)lock.h	4.1	(ULTRIX)	7/2/90 	*/


/************************************************************************
 *									*
 *			Copyright (c) 1984 by				*
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
 * 8 Nov 89 -- jmf
 *	New defines added for atomic_op() system call
 *	(Note: VAX library does not use system call)
 *
 * 18 Sep 85 -- depp
 *	New file -- This header file provides the locking options for 
 *	the system call "plock"
 *
 */

/* @(#)lock.h	6.1 */
/*
 * flags for locking a process' segments into memory 
 */
#define	UNLOCK	 0		/* Unlock all segments */
#define	PROCLOCK 1		/* Lock text and data into memory */
#define	TXTLOCK	 2		/* Lock text segment only */
#define	DATLOCK	 4		/* Lock data segment only */

/* 
 * defines used by atomic_op() system call
 */
#define	ATOMIC_SET	0	/* operation = set */
#define ATOMIC_CLEAR	1	/* operation = clear */
#define ATOMIC_LOCKBIT  31	/* internal bit used for atomic operation */
