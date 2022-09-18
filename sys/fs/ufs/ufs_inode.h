/*	@(#)ufs_inode.h	4.2	(ULTRIX)	2/28/91	*/

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
***********************************************************************/

/* ---------------------------------------------------------------------
 * Modification History: /sys/h/inode.h
 *
 * 27 Feb 91 -- chet
 *	Fix filesystem timestamping.
 *
 * 09 Sep 85 -- Reilly
 *	Modified to handle the new 4.3BSD namei code.
 * 
 * 19 Jul 85 -- depp
 *	Removed #ifdef NPIPE.  
 *
 * 4  April 85 -- Larry Cohen
 *	Add GINUSE flag to support open block if in use capability
 *
 * 15 Mar 85 -- funding
 *	Added named pipe support (re. System V named pipes)
 *
 * 23 Oct 84 -- jrs
 *	Add definitions for nami cacheing
 *
 * 17 Jul 84 -- jmcg
 *	Insert code to keep track of lockers and unlockers as a debugging
 *	aid.  Conditionally compiled with option RECINODELOCKS.
 *
 * 17 Jul 84 --jmcg
 *	Derived from 4.2BSD, labeled:
 *		inode.h	6.1	83/07/29
 *
 * ---------------------------------------------------------------------
 */


/*
 * The I node is the focus of all file activity in UNIX.
 * There is a unique inode allocated for each active file,
 * each current directory, each mounted-on file, text file, and the root.
 * An inode is 'named' by its dev/inumber pair. (iget/iget.c)
 * Data in icommon is read in from permanent inode on volume.
 */

#define	NDADDR	12		/* direct addresses in inode */
#define	NIADDR	3		/* indirect addresses in inode */


/*
 *	this is the on disk format for an ultrix inode.  this
 *	structure also appears withing the gnode structure
 */


struct	ufs_inode {
	union {
		char	trash[128];
		struct {
			struct gnode_common gn_gc;
			daddr_t	dg_db[NDADDR];	/* 40: disk block addresses */
			daddr_t	dg_ib[NIADDR];	/* 88: indirect blocks */
			long	dg_flags;	/* 100: status, currently unused */
			long	dg_blocks;	/* 104: blocks actually held */
			u_long	dg_gennum; 	/* 108: incarnation of inode */
			long	dg_spare[4];	/* 108: reserved, currently unused */
		} di_gcom;
	} ufs_iu;
};

#define di_ic		ufs_iu.di_gcom
#define	di_mode		di_ic.gn_gc.gc_mode
#define	di_nlink	di_ic.gn_gc.gc_nlink
#define	di_uid		di_ic.gn_gc.gc_uid
#define	di_gid		di_ic.gn_gc.gc_gid
#define di_db		di_ic.dg_db
#define di_ib		di_ic.dg_ib
#define	di_flags	di_ic.dg_flags
#define di_blocks	di_ic.dg_blocks
#define di_gennum	di_ic.dg_gennum
#define di_spare	di_ic.dg_spare
#if defined(vax) || defined(mips)
#define	di_size		di_ic.gn_gc.gc_size.val[0]
#endif
#define	di_atime	di_ic.gn_gc.gc_atime
#define	di_mtime	di_ic.gn_gc.gc_mtime
#define	di_ctime	di_ic.gn_gc.gc_ctime
#define	di_rdev		di_ic.dg_db[0]

#ifndef KERNEL
#define dinode ufs_inode
#endif

#define	G_TO_I(x)	((struct ufs_inode *)(x)->g_in.pad)

struct	gnode	*ufs_gget();
struct	mount	*ufs_mount();
