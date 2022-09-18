/* @(#)automount.h	4.2      (ULTRIX)        1/3/91 */

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
 *	Copyright (c) 1987 Sun Microsystems, Inc.  ALL RIGHTS RESERVED.
 */
/*
 *	Modification History:
 * 
 *	27 Dec 90 -- condylis
 *		Added fs_dev member to filsys structure.  Filled in
 *		at mount time and used at umount time.
 *
 *      10 Nov 89 -- lebel
 *              Incorporated direct maps, bugfixes, metacharacter handling 
 *              and other fun stuff from the reference tape.
 * 	14 Jun 89 -- condylis
 *		Added copyright header.
 *
 */

/* 3.x compatibility */
#ifdef OLDMOUNT
#define LOG_DAEMON 0
#define MAXHOSTNAMELEN  64
#define MAXNETNAMELEN   255
#endif OLDMOUNT

#define	FH_HASH_SIZE	8

/*
 * General queue structure 
 */
struct queue {
	struct queue	*q_next;
#define	q_head	q_next
	struct queue	*q_prev;
#define	q_tail	q_prev
};

#define	INSQUE(head, ptr) my_insque(&(head), &(ptr)->q)
#define	REMQUE(head, ptr) my_remque(&(head), &(ptr)->q)
#define HEAD(type, head) ((type *)(head.q_head))
#define NEXT(type, ptr)	((type *)(ptr->q.q_next))
#define	TAIL(type, head) ((type *)(head.q_tail))
#define PREV(type, ptr)	((type *)(ptr->q.q_prev))
	

/*
 * Types of filesystem entities (vnodes)
 * We support only one level of DIR; everything else is a symbolic LINK
 */
enum vn_type { VN_DIR, VN_LINK};
struct vnode {
	struct queue q;
	nfs_fh	vn_fh;		/* fhandle */
	struct fattr vn_fattr;	/* file attributes */
	enum vn_type vn_type;	/* type of vnode */
	caddr_t	vn_data;	/* vnode private data */
};

struct vnode *fhtovn();		/* lookup vnode given fhandle */

/*
 * Structure describing a host/filesystem/dir tuple in a YP map entry
 */
struct mapfs {
	struct mapfs *mfs_next;		/* next in entry */
	char	*mfs_host;		/* host name */
	struct in_addr mfs_addr;	/* address of host */
	char	*mfs_dir;		/* dir to mount */
	char	*mfs_subdir;		/* subdir of dir */
};

/*
 * YP entry - lookup of name in DIR gets us this
 */
struct mapent {
	char	*map_root;
	char	*map_mntpnt;
	char	*map_mntopts;
	struct mapfs *map_fs;
	struct mapent *map_next;
};
struct mapent *getmapent();

/*
 * Everthing we know about a mounted filesystem
 * Can include things not mounted by us (fs_mine == 0)
 */
struct filsys {
	struct queue q;			/* next in queue */
	int	fs_death;		/* time when no longer valid */
	int	fs_mine;		/* 1 if we mounted this fs */
	int	fs_present;		/* for checking unmounts */
	char	*fs_type;		/* type of filesystem */
	char	*fs_host;		/* host name */
	char	*fs_dir;		/* dir of host mounted */
	char	*fs_mntpnt;		/* local mount point */
	char	*fs_opts;		/* mount options */
	struct nfs_gfs_mount fs_nfsargs;/* nfs mount args */
	struct sockaddr_in fs_addr;	/* host address */
	struct filsys *fs_rootfs;	/* root for this hierarchy */
	nfs_fh	fs_rootfh;		/* file handle for nfs mount */
	int	fs_mflags;		/* mount flags */
	dev_t	fs_dev			/* device number used by umount */
};
struct queue fs_q;
struct filsys *already_mounted(), *alloc_fs();

/*
 * Structure for recently referenced links
 */
struct link {
	struct queue q;		/* next in queue */
	struct vnode link_vnode;	/* space for vnode */
	struct autodir *link_dir;	/* dir which we are part of */
	char	*link_name;	/* this name in dir */
	struct filsys *link_fs;	/* mounted file system */
	char	*link_path;	/* dir within file system */
	long	link_death;	/* time when no longer valid */
};
struct link *makelink();
struct link *findlink();
	
/*
 * Descriptor for each directory served by the automounter 
 */
struct autodir {
	struct queue q;
	struct	vnode dir_vnode;	/* vnode */
	char	*dir_name;	/* mount point */
	char	*dir_map;	/* name of map for dir */
	char	*dir_opts;	/* default mount options */
	int	dir_remove;	/* remove mount point */
	struct queue dir_head;
};
struct queue dir_q;

char self[64];		/* my hostname */
char mydomain[64];	/* my domain name */
char tmpdir[200];	/* real name of /tmp */
struct in_addr my_addr;	/* my inet address */

char *malloc(), *index(), *strdup();
time_t time_now;		/* set at start of processing of each RPC call */
int mount_timeout;	/* max seconds to wait for mount */
int max_link_time;	/* seconds to keep link around */
int nomounts;		/* don't do any mounts - for cautious servers */
nfsstat lookup(), nfsmount();
