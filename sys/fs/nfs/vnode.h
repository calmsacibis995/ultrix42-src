/*	@(#)vnode.h	4.2	(ULTRIX)	7/17/90	*/

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
 *
 *   Modification history:
 *
 *  7 Jul 90 -- chet
 *	Remove r_writes rnode counter.
 *
 *  9 Mar 90 -- chet
 *	Add r_writes rnode counter and remove R_IN_FSYNC.
 *
 * 15 Jun 89 -- cb
 *	Fixed includes, so both kernel & crash can use this header file
 *
 * 28 Sep 88 -- chet
 *	Add R_IN_FSYNC here, where it belongs
 *
 * 18 Jul 88 -- condylis
 *	VN_HOLD macro now calls SMP gref routine to increment ref count
 *	of gnode.
 *
 * 12 Jan 88 -- fglover
 *	Add RNOCACHE flag for Sys-V file locks
 *
 * 24-Aug-87 -- logcher
 *	Removed VROOT.  Flag coincided with gnode flag, and root
 *	busy check is already performed in GFS
 *
 * 02-Mar-87 -- logcher
 *	Merged in diskless changes, added VFIFO to vtype
 */

/*
 * vnode types. VNON means no type.
 */
enum vtype 	{ VNON, VREG, VDIR, VBLK, VCHR, VLNK, VSOCK, VBAD, VFIFO };
#ifdef KERNEL
#include "../h/mount.h"
#include "../h/gnode_common.h"
#include "../h/gnode.h"
#else /* KERNEL */
#include <sys/mount.h>
#include <sys/gnode_common.h>
#include <sys/gnode.h>
#endif /* KERNEL */

struct rnode {
	fhandle_t	r_fh;		/* file handle */
	u_short		r_flags;	/* flags, see below */
	short		r_error;	/* async write error */
	struct ucred	*r_cred;	/* current credentials */
	struct ucred	*r_unlcred;	/* unlinked credentials */
	char		*r_unlname;	/* unlinked file name */
	struct vnode	*r_unldvp;	/* parent dir of unlinked file */
	struct timeval	r_nfsattrtime;
};

#define v_flag			g_flag
#define	v_rdev			g_rdev
#define v_vfsp			g_in.ve.V_vfsp
#define v_type			g_in.ve.V_type
struct vnode {
	struct gnode_req g_req;
	union {
		char pad[128];
		struct gnode_common gn;
		struct {
			struct gnode_common	gn;
			struct vfs		*V_vfsp;
			enum vtype		V_type;
			struct rnode		V_rn;
		} ve;
	} g_in;
};

/*
 * vnode flags.
 */
#define VTEXT		GTEXT		/* vnode is a pure text prototype */
#define	VEXLOCK		GEXLOCK		/* exclusive lock */
#define	VSHLOCK		GSHLOCK		/* shared lock */
#define	VLWAIT		GLWAIT		/* proc is waiting on shared or excl. lock */

#ifdef KERNEL

#define VOP_OPEN(VPP,F,C)		vop_open(VPP,F,C)
#define VOP_CLOSE(VP,F,C)		vop_close(VP,F,C)
#define VOP_RDWR(VP,UIOP,RW,F,C)	vop_rdwr(VP,UIOP,RW,F,C)
#define VOP_GETATTR(VP,VA,C)		vop_getattr(VP,VA,C)
#define VOP_SETATTR(VP,VA,C)		vop_setattr(VP,VA,C)
#define VOP_ACCESS(VP,M,C)		vop_access(VP,M,C)
#define VOP_LOOKUP(VP,NM,VPP,C)		vop_lookup(VP,NM,VPP,C)
#define VOP_CREATE(VP,NM,VA,E,M,VPP,C)	vop_create(VP,NM,VA,E,M,VPP,C)
#define VOP_REMOVE(VP,NM,C)		vop_remove(VP,NM,C)
#define VOP_LINK(VP,TDVP,TNM,C)		vop_link(VP,TDVP,TNM,C)
#define VOP_RENAME(VP,NM,TDVP,TNM,C)	vop_rename(VP,NM,TDVP,TNM,C)
#define VOP_MKDIR(VP,NM,VA,VPP,C)	vop_mkdir(VP,NM,VA,VPP,C)
#define VOP_RMDIR(VP,NM,C)		vop_rmdir(VP,NM,C)
#define VOP_READDIR(VP,UIOP,C)		vop_readdir(VP,UIOP,C)
#define VOP_SYMLINK(VP,LNM,VA,TNM,C)	vop_symlink(VP,LNM,VA,TNM,C)
#define VOP_READLINK(VP,UIOP,C)		vop_readlink(VP,UIOP,C)	
#define VOP_BREAD(VP,BN,BPP)		vop_bread(VP,BN,BPP)
#define VOP_BRELSE(VP,BP)		vop_brelse(VP,BP)

#define IFTOVT(M)	(gftovt_tab[((M) & GFMT) >> 13])
#define VTTOIF(T)	(vttogf_tab[(int)(T)])

/*
 * Check that file is owned by current user or user is superuser.
 */
#define OWNER(GP, CR)	(((CR)->cr_uid == (GP)->g_uid)? 0: (suser()? 0: u.u_error))

#endif

/*
 * Vnode attributes.  A field value of -1
 * represents a field whose value is unavailable
 * (getattr) or which is not to be changed (setattr).
 */
struct vattr {
	enum vtype	va_type;	/* vnode type (for create) */
	u_short		va_mode;	/* files access mode and type */
	short		va_uid;		/* owner user id */
	short		va_gid;		/* owner group id */
	long		va_fsid;	/* file system id (dev for now) */
	long		va_nodeid;	/* node id */
	short		va_nlink;	/* number of references to file */
	u_long		va_size;	/* file size in bytes (quad?) */
	long		va_blocksize;	/* blocksize preferred for i/o */
	struct timeval	va_atime;	/* time of last access */
	struct timeval	va_mtime;	/* time of last modification */
	struct timeval	va_ctime;	/* time file ``created */
	dev_t		va_rdev;	/* device the file represents */
	long		va_blocks;	/* kbytes of disk space held by file */
};

/*
 *  Modes. Some values same as Ixxx entries from inode.h for now
 */
#define	VSUID	04000		/* set user id on execution */
#define	VSGID	02000		/* set group id on execution */
#define VSVTX	01000		/* save swapped text even after use */
#define	VREAD	0400		/* read, write, execute permissions */
#define	VWRITE	0200
#define	VEXEC	0100

#ifdef KERNEL

extern void vattr_null();		/* set attributes to null */

#define	VN_HOLD(VP)	{ \
	gref((struct gnode *)VP); \
}

#define VN_RELE(VP) { \
	grele((struct gnode *)VP); \
}

/*
 * flags for above
 */
enum rm		{ FILE, DIRECTORY };		/* rmdir or rm (remove) */
enum symfollow	{ NO_FOLLOW, FOLLOW_LINK };	/* follow symlinks (lookuppn) */
enum vcexcl	{ NONEXCL, EXCL};		/* (non)excl create (create) */

#endif

/* RNODE STUFF */

/*
 * Flags
 */
#define	RLOCKED		0x01		/* rnode is in use */
#define	RWANT		0x02		/* someone wants a wakeup */
#define	RATTRVALID	0x04		/* Attributes in the rnode are valid */
#define	REOF		0x08		/* EOF encountered on read */
#define	RDIRTY		0x10		/* dirty buffers may be in buf cache */
#define ROPEN		0x20		/* the vnode is currently open */
#define RNOCACHE	0x40		/* don't cache read and write blocks */

/*
 * Convert between vnode and rnode
 */

#define	rtov(rp)	((rp)->r_vnode)

#define	vtor(vp)	(&((vp)->g_in.ve.V_rn))
#define	vtofh(vp)	(&(vtor(vp)->r_fh))
#define	rtofh(rp)	(&(rp)->r_fh)
