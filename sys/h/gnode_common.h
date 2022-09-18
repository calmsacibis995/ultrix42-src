/* @(#)gnode_common.h	4.1  (ULTRIX)        7/2/90     */

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
 *	This header file must be included before gnode.h, ufs_inode.h
 *	and any other SFS include files.  The gnode_common stucture
 *	MUST be contained within g_in union in gnode.h
 */

#ifdef KERNEL
#include "../h/time.h"
#else /* KERNEL */
#include <sys/time.h>
#endif /* KERNEL */
#ifndef __GNODE
struct 	gnode_common {
	u_short	gc_mode;		/* 0: mode and type of file */
	short	gc_nlink;		/* 2: number of links to file */
	short	gc_uid;			/* 4: owner's user id */
	short	gc_gid;			/* 6: owner's group id */
	quad	gc_size;		/* 8: number of bytes in file */
	struct timeval	gc_atime;	/*16: time last accessed */
	struct timeval	gc_mtime;	/*24: time last modified */
	struct timeval	gc_ctime;	/*32: last time inode changed */
};
#define __GNODE
#endif
