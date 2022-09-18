/*	@(#)vfs.h	4.1	(ULTRIX)	7/2/90	*/

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
 * Structure per mounted file system.
 * Each mounted file system has an array of
 * operations and an instance record.
 * The file systems are put on a singly linked list.
 */
struct vfs {
	struct vfs	*vfs_next;		/* next vfs in vfs list */
	struct vnode	*vfs_vnodecovered;	/* vnode we mounted on */
	int		vfs_flag;		/* flags */
	int		vfs_bsize;		/* native block size */
	u_short		vfs_exroot;		/* exported fs uid 0 mapping */
	short		vfs_exflags;		/* exported fs flags */
	caddr_t		vfs_data;		/* private data */
};


/* the v_fs_data structure is an overlay for the fs_data structure */
/* to make nfs faster and cleaner we only malloc the fs_data structure */
/* and use the spare area in the end for the mntinfo and vfs structures */
/* ALWAYS USE THE TWO MACROS TO GET TO THE MNTINFO AND VFS STRUCTURES */

struct v_fs_data {
		struct fs_data_req fd_req;	/* required by gfs - mount.h */
		union {
			u_int	fd_spare[112];	/* lots of room left, use it */
			struct gfs_vfs {	/* keep the # of fd_spare in */
				struct mntinfo mi;	/* sync with mount.h */
				struct vfs vfs;
			} gvfs;
		} fd_un;
};

#define MP_TO_MIP(mp)	((struct mntinfo *) &(((struct v_fs_data *) \
					(mp->m_fs_data))->fd_un.gvfs.mi))
#define MP_TO_VFSP(mp)  ((struct vfs *) &(((struct v_fs_data *) \
					(mp->m_fs_data))->fd_un.gvfs.vfs))

/*
 * vfs flags.
 * VFS_MLOCK lock the vfs so that name lookup cannot proceed past the vfs.
 * This keeps the subtree stable during mounts and unmounts.
 */
#define VFS_RDONLY	0x01		/* read only vfs */
#define VFS_MLOCK	0x02		/* lock vfs so that subtree is stable */
#define VFS_MWAIT	0x04		/* someone is waiting for lock */
#define VFS_NOSUID	0x08		/* someone is waiting for lock */
#define	VFS_EXPORTED	0x10		/* file system is exported (NFS) */

/*
 * exported vfs flags.
 */
#define	EX_RDONLY	0x01		/* exported read only */
#define EXPORTFS_CREATE	0x01		/* create a new export record */
#define EXPORTFS_REMOVE	0x02		/* remove an old export record */
#define EXPORTFS_READ	0x03		/* read an old export record */

struct exportfsdata {
	int  e_flags;
	char e_path[MAXPATHLEN];
	dev_t	e_fsid;
	u_long	e_gnum;
	u_long	e_gen;
	short	e_rootmap;
	int	e_more;
};

/*
 * file system statistics
 */
typedef long fsid_t[2];			/* file system id type */

struct statfs {
	long f_type;			/* type of info, zero for now */
	long f_bsize;			/* fundamental file system block size */
	long f_blocks;			/* total blocks in file system */
	long f_bfree;			/* free block in fs */
	long f_bavail;			/* free blocks avail to non-superuser */
	long f_files;			/* total file nodes in file system */
	long f_ffree;			/* free file nodes in fs */
	fsid_t f_fsid;			/* file system id */
	long f_spare[7];		/* spare for later */
};

#ifdef KERNEL
/*
 * public operations
 */
extern void	vfs_mountroot();	/* mount the root */
extern int	vfs_add();		/* add a new vfs to mounted vfs list */
extern void	vfs_remove();		/* remove a vfs from mounted vfs list */
extern int	vfs_lock();		/* lock a vfs */
extern void	vfs_unlock();		/* unlock a vfs */

#define VFS_INIT(VFSP, DATA)	{ \
	(VFSP)->vfs_next = (struct vfs *)0; \
	(VFSP)->vfs_flag = 0; \
	(VFSP)->vfs_exflags = 0; \
	(VFSP)->vfs_data = (DATA); \
}

/*
 * globals
 */
extern struct vfs *rootvfs;		/* ptr to root vfs structure */


#endif /* KERNEL */
