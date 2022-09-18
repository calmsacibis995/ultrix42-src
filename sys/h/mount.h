/*	@(#)mount.h	4.1	(ULTRIX)	7/2/90	*/

/************************************************************************
 *									*
 *			Copyright (c) 1986, 1987 by			*
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

/***********************************************************************
 *
 *		Modification History
 *
 * 14 Jun 89 -- prs
 *	Added clean byte timeout logic.
 *
 * 09 Jan 89 -- condylis
 *	Made m_lk smp lock part of mount struct rather than KMALLOCed.
 *	Created new m_flgs field of mount struct for state of mount
 *	table entry.
 *
 * 19 May 88 -- cb
 *	Changed GFS interface (mount_ops).
 *
 * 18 May 88 -- prs
 *	SMP - Added m_lk smp lock to the mount structure, and the
 *	macros fs_lock and fs_unlock that lock file system dependent
 *	data.
 *
 * 11 May 87 -- chet
 *	Added mode value defines for new getmnt() arg.
 *
 * 05 May 87 -- logcher
 *	Fixed usage of m_flags with export flags
 *
 * 28 Apr 87 -- logcher
 *	Moved m_exroot out of mount structure and into fs_data
 *	Will use m_flags instead of m_exflags and m_security_flag
 *
 * 02 Mar 87 -- logcher
 *	Merged in diskless changes, new mount struct and flags,
 *	and reorganization of mount_ops and macros
 *
 * 15 Jan 87 -- prs
 *	Added MOUNT_QUOTA define
 *
 * 23 Oct 86 -- chet
 *	Add sync flag to GBMAP macro for synchronous writes
 *
 * 11 sep 86 -- koehler
 *	modifications for alignment of fs_data structure.. new function
 *	introduction, new mount options, and general fun stuff
 *
 * 16 oct 86 -- koehler
 *	returned the GETMP to its original state
 *
 ***********************************************************************/

/*
 * Mount structure.
 * One allocated on every mount.
 * Used to find the super block.
 */
#ifndef	KERNEL
#include <sys/smp_lock.h>
#endif /*	KERNEL */

/* needs param.h */
#ifndef __MOUNT__
#define __MOUNT__
/*
 * fs_data should eliminate the need for fs.h in sys/sys and sys/gfs
 */

struct fs_data_req {	/* required part for all file systems */
		u_int	flags;		/* how mounted */
		u_int	mtsize;		/* max transfer size in bytes */
		u_int	otsize;		/* optimal transfer size in bytes */
		u_int	bsize;		/* fs block size in bytes for vm code */
		u_int	fstype;		/* see ../h/fs_types.h  */
		u_int	gtot;		/* total number of gnodes */
		u_int	gfree;		/* # of free gnodes */
		u_int	btot;		/* total number of 1K blocks */
		u_int	bfree;		/* # of free 1K blocks */
		u_int	bfreen;		/* user consumable 1K blocks */
		u_int	pgthresh;	/* min size in bytes before paging*/
		int	uid;		/* uid that mounted me */
		dev_t	dev;		/* major/minor of fs */
		short	exroot;		/* root mapping from exports */
		char	devname[MAXPATHLEN + 4]; /* name of dev */
		char	path[MAXPATHLEN + 4]; /* name of mount point */
		u_int	nupdate;	/* number of writes */
};					/* 2116 bytes */

/*
 * Changes to this structure will require changing struct v_fs_data
 * in nfs/vfs.h 
 */
struct fs_data {
		struct	fs_data_req fd_req;/* 2112 bytes */
		u_int	fd_spare[112];	/* this structure is exactly  */
					/* 14 * 4 + 2 * 1028 + 112 *4 = 2560 */
					/* bytes - KEEP IT THAT WAY - rr */
					/* since we malloc memory in 512 byte */
					/* chunks, the last 113 u_int's are */
					/* FREE */
};

/* these defines let anyone access the fields very simply */
/* fsdata->fd_flags or v_fs_data->fd_flags work! see nfs/vfs.h */

#define fd_flags	fd_req.flags	/* how mounted */
#define fd_mtsize	fd_req.mtsize	/* max transfer size */
#define fd_otsize	fd_req.otsize	/* optimal transfer size */
#define fd_bsize	fd_req.bsize	/* fs block size for vm code */
#define fd_fstype	fd_req.fstype	/* see ../h/fs_types.h  */
#define fd_gtot		fd_req.gtot	/* total number of gnodes */
#define fd_gfree	fd_req.gfree	/* # of free gnodes */
#define fd_btot		fd_req.btot	/* total number of blocks */
#define fd_bfree	fd_req.bfree	/* # of free blocks */
#define fd_bfreen	fd_req.bfreen	/* user consumable blocks */
#define fd_pgthresh	fd_req.pgthresh	/* min file size before paging*/
#define fd_uid		fd_req.uid	/* uid that mounted me */
#define fd_dev		fd_req.dev	/* major/minor of fs */
#define fd_exroot	fd_req.exroot	/* root mapping from exports */
#define	fd_nupdate	fd_req.nupdate	/* number of writes */
#define fd_devname	fd_req.devname	/* name of dev */
#define fd_path		fd_req.path	/* name of mount point */

struct	mount
{
	struct	mount	*m_forw[2];	/* must be first */
	dev_t	m_dev;			/* device mounted on */
	u_short m_pad;			/* padding */
	union {
		struct	buf	*fs;	/* pointer to superblock */
		char		*cp;	/* whatever specific fs wants */
	} m_sb_un;
	struct	fs_data *m_fs_data;
	struct	gnode	*m_gnodp;	/* pointer to mounted on gnode */
	struct	gnode	*m_rootgp;	/* pointer to root gnode */
	struct	gnode	*m_qinod;	/* QUOTA: pointer to quota file */
	struct  lock_t  m_lk;		/* SMP lock of super block data */
					/* and m_bufp member of this */
					/* structure */
	int	m_flgs;			/* State of structure */
	struct	mount_ops {
                int             (*go_umount)(   /* mp,force             */ );
                int             (*go_sbupdate)( /* mp, last             */ );
                int 		(*go_init)(     /* gp, ptr   		*/ );
		int		(*go_match)(	/* gp, ptr		*/ );
		int		(*go_reclaim)(	/* gp, ptr		*/ );
		int		(*go_inactive)( /* gp			*/ );
                struct fs_data *(*go_getfsdata)(/* mp                   */ );
	} *m_ops;
        /* iostrategy routine per mounted file system */
        int	(*iostrat)(	/* bp */ );
};

#define	m_bufp		m_sb_un.fs		/* to make old code work*/

/* careful with these next defines since they are not valid until */
/* fs_data has been km_alloc'ed in mount system call */

#define m_path		m_fs_data->fd_path
#define m_bsize		m_fs_data->fd_bsize
#define m_fstype	m_fs_data->fd_fstype
#define m_flags		m_fs_data->fd_flags
#define m_exroot	m_fs_data->fd_exroot
#define m_nupdate	m_fs_data->fd_nupdate

/*
 *	getmnt() syscall mode argument values
 */
#define NOSTAT_MANY	1
#define STAT_MANY	2
#define STAT_ONE	3
#define NOSTAT_ONE	4
#define STAT_FD		5
#define NOSTAT_FD	6


#ifdef KERNEL

/*
 * moved gnode macros to gnode.h
 */

struct	mount mount[NMOUNT];
struct	_sfs {
	struct	mount *(*function)();
};
struct	_sfs    mount_sfs[NUM_FS];
#define MOUNTFS(num)	mount_sfs[(num)].function
struct	mount	*mountfs();
struct	mount_ops	*ufs_ops;
struct	mount_ops	*fs_divorce_ops;
#endif /* KERNEL */

/* FLAGS */

#define	M_RONLY		0x0001
#define M_MOD  		0x0002
#define M_QUOTA		0x0004
#define M_LOCAL		0x0008
#define M_NOEXEC	0x0010
#define M_NOSUID	0x0020
#define M_NODEV		0x0040
#define M_FORCE		0x0080
#define M_SYNC		0x0100
#define M_DONE		0x0200
#define M_NOCACHE	0x0400
#define M_EXPORTED      0x0800          /* export flag */
#define M_NOFH          0x1000          /* no fhandle flag */
#define M_EXRONLY       0x2000          /* export read-only */

/* Flags for m_flgs; state of mount table entry */
#define MTE_DONE	0x0001		/* mounted on	*/
#define MTE_UMOUNT	0x0002		/* umount attempt in progress */

#define	ISREADONLY(mp)	((mp) && (mp)->m_flags & M_RONLY)
#define	ISLOCAL(mp)	((mp) && (mp)->m_flags & M_LOCAL)
#define M_USRMNT	(M_NOSUID | M_NODEV)

/* MOUNT TIME OPTIONS */

#define MOUNT_NOEXEC	"noexec"
#define MOUNT_NOSUID	"nosuid"
#define MOUNT_NODEV	"nodev"
#define MOUNT_PGTHRESH	"pgthresh="
#define MOUNT_FORCE	"force"
#define MOUNT_SYNC	"sync"
#define MOUNT_NOCACHE	"nocache"
#define MOUNT_QUOTA	"quota"

/* Macros */

#define GETMP(assigned, dev)						\
	{								\
		register struct mount *__mp__;				\
									\
		if((dev) == swapdev) {					\
			assigned = (struct mount *)MSWAPX;		\
		} else {						\
			assigned = NULL;				\
			for(__mp__ = mount; __mp__ < &mount[nmount]; __mp__++){\
				if(((dev) == __mp__->m_dev) && __mp__->m_fs_data) {		\
					assigned = __mp__;		\
					break;				\
				}					\
			}						\
		}							\
	}
	
extern int nmount;

/*
 * These macros hide the implementation of SMP locks on individual
 * filesystems, in case we want to move them to fs_data, etc.
 *
 * These locks control operations on the mount table entry or fsdata
 * entry at the GFS level. They also may be used by specific filesystem
 * implementations for their own purposes. GFS will never call a specific
 * filesystem while holding a mount table spin lock.
 *
 */

#define fs_lock(mp) \
        smp_lock(&((mp)->m_lk), LK_RETRY);

#define fs_unlock(mp) \
        smp_unlock(&((mp)->m_lk));

/*
 * FS MACROS
 */

/*
 * GINIT(gp, ptr)
 *	struct	gnode	*gp;
 *	caddr_t ptr;
 *
 *	Initialize an new gnode.
 */


#define	GINIT(gp, iflag, ptr)	\
		((gp)->g_mp->m_ops->go_init ? (gp)->g_mp->m_ops->go_init(gp, iflag, ptr) : 1)

			
/*
 * GMATCH(gp, ptr)
 *	struct	gnode	*gp;
 *	caddr_t ptr;
 *
 *	Boolean function for gnode cache matching.
 */
						

#define	GMATCH(gp, ptr)	\
		((gp)->g_mp->m_ops->go_match ? (gp)->g_mp->m_ops->go_match(gp, ptr) : 1)


		
/*
 * GRECLAIM(gp, ptr)
 *	struct	gnode	*gp;
 *	caddr_t ptr;
 *
 *	(Re)initialize a reclaimed gnode.
 */
						

#define	GRECLAIM(gp, iflag, ptr)	\
		((gp)->g_mp->m_ops->go_reclaim ? (gp)->g_mp->m_ops->go_reclaim(gp, iflag, ptr) : 1)


/*
 * GINACTIVE(gp)
 *	struct	gnode	*gp;
 *
 *	Make a gnode inactive.
 */
						

#define	GINACTIVE(gp)	\
		((gp)->g_mp->m_ops->go_inactive ? (gp)->g_mp->m_ops->go_inactive(gp) : 1)

/*
 * GUMOUNT(mp, flag)
 *      struct mount *mp;
 *      int flag;
 *
 * umount the file system pointed to by mp
 */

#define GUMOUNT(mp,flag)        \
                (((mp)->m_ops->go_umount)((mp),(flag)))


/*
 * GSBUPDATE(mp)
 *      struct mount *mp;
 *      int last;
 *
 * flush the superblock pointed to by mp
 */

#define GSBUPDATE(mp, last)     \
	((((mp)->m_ops->go_sbupdate ? ((mp)->m_ops->go_sbupdate) \
        ((mp), (last)) : GNOFUNC)))

/*
 * GGETFSDATA(mp)
 *      struct mount *mp;
 *
 * get the generic file system data for the file system mp
 */

#define GGETFSDATA(mp)  \
                (((mp)->m_ops->go_getfsdata) (mp))


/*
 * BIOSTATEGY(bp)
 *      struct buf *bp;
 * GIOSTATEGY(gp)
 *      struct gnode *gp;
 *
 * interface to an I/O strategy routine (mainly for remote filesystems)
 * when I/O is done on the VBN rather than the LBN.
 */
#define BIOSTRATEGY(bp)                         \
        (((bp)->b_gp) ? (((bp)->b_gp->g_mp->iostrat) (bp)) : GNOFUNC)
/* direct pointer to the strategy routine from the gnode */
#define GIOSTRATEGY(gp) ((gp)->g_mp->iostrat)

#endif /* __MOUNT__ */
