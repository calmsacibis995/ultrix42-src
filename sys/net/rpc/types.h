/* @(#)types.h	4.2 (ULTRIX) 9/4/90 */

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
 *	Portions of this software have been licensed to 
 *	Digital Equipment Company, Maynard, MA.
 *	Copyright (c) 1986 Sun Microsystems, Inc.  ALL RIGHTS RESERVED.
 */

/*
 * Rpc additions to <sys/types.h>
 *
 *	History:
 *
 * 12-11-87	Robin L. and Larry C. and Ricky P.
 *	Added new kmalloc memory allocation to system.
 *
 * 12-18-87	Joe A.
 *	moved <sys/types> above kmalloc.h
 *
 * 05-01-90	Thomas
 * DECwest ANSI mt 1990 May 01
 * Changed __RPC_TYPES.H__ to __RPC_TYPES_H__ for ANSI
 *
 * 08-20-90	Thomas
 * DECwest ANSI 3.8.3 mt 1990 Aug 20
 * Made names of dummy arguments unique within definition of mem_alloc,
 * since the standard says that the scope of the identifier list
 * (arguments) extends from their declaration in the list until the
 * newline character that terminates the #define.
 */

#ifndef __RPC_TYPES_H__
#define __RPC_TYPES_H__

#ifndef	KERNEL
#ifndef major		/* ouch! */
#include <sys/types.h>
#endif
#include	<sys/kmalloc.h>
#else /* KERNEL */
#include	"../h/kmalloc.h"
#endif /* KERNEL */

#define	bool_t	int
#define	enum_t	int
#define	FALSE	(0)
#define	TRUE	(1)
#ifndef NULL
#define NULL 0
#endif
#define __dontcare__	-1

#ifndef KERNEL
#define mem_alloc(dummy1, dummy2, bsize, dummy3)	malloc(bsize)
#define mem_free(ptr, dummy)	free(ptr)
#else /* KERNEL */
#define mem_alloc(ptr, cast, bsize, type)	KM_ALLOC((ptr), cast, bsize, type, KM_NOARG)
#define mem_free(ptr, type)                     KM_FREE((ptr), type)
#define kmem_alloc(ptr, cast, bsize, type)	KM_ALLOC((ptr), cast, bsize, type, KM_CLEAR)
#define kmem_free(ptr, type)	                KM_FREE((ptr), type)
#endif /* KERNEL */
#endif /* __RPC_TYPES_H__ */
