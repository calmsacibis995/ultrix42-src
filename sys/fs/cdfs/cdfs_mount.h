/* @(#)cdfs_mount.h	4.1	(ULTRIX)	11/9/90 */

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
 * cdfs_mount.h
 *	contains the definition of the structure passed down to handle
 *	per filesystem specific data like paging threshold
 */

struct iso_specific {
	u_long	iso_flags;		/* see m_flags in ../h/mount.h */
	u_long	iso_pgthresh;		/* minimum file size which pages */
};

#define M_DEFPERM	0x10		/* Default permissions */
#define M_NODEFPERM	0x20		/* No default permissions */
#define M_NOVERSION	0x40		/* No version number */
#define M_PRIMARY	0x80		/* Use Primary Vol Desc */
