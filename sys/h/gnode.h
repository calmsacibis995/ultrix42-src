/*	@(#)gnode.h	4.3	(ULTRIX)	2/28/91	*/

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

/* ---------------------------------------------------------------------
 * Modification History: /sys/h/gnode.h
 *
 *  2 Feb 90 -- chet
 *	change cacheinval macro
 *
 * 10 Dec 89 -- chet
 *	change cacheinval macro
 *
 * 25 Jul 89 -- chet
 *	Remove bawrite() macro
 *
 * 28 Sep 88 -- chet
 *	Change nextgnodeid to an unsigned
 * 
 * 10 Mar 88 -- cb
 *	Added SMP changes.
 *
 * 10 Feb 88 -- prs
 *	Modified to handle new fifo code, and removed conditional
 *	limiting gnode hash size.
 *
 * 26 Jan 88 -- fglover
 *	Modify GRLOCK macro to support combined kernel UFS and daemon 
 *	UFS/NFS Sys V locking
 *
 * 14 Jul 87 -- cb
 *	Changed mknod interface.
 *
 * 02 Mar 87 -- logcher
 *	Merged in diskless changes, changed gnode_ops and moved
 *	gnode macros from mount.h
 *
 * 20 Feb 87 -- depp
 *	Added GTRC flag for indicating that one or more processes are
 *	tracing.
 *
 * 11 Sep 86 -- koehler 
 *	added flags for update change
 *
 * 11 Mar 86 -- lp
 *	Add flag to mark an inode as using n-bufferring. Actually just
 *	reuse ISYNC flag to mean this (since ISYNC is never used to a
 *	raw device anyway - done this way because flags is a short and
 *	not much room left).
 *
 * 24 Dec 85 -- Shaughnessy
 *	Added syncronous write flag.
 *
 * 09 Sep 85 -- Reilly
 *	Modified to handle the new 4.3BSD namei code.
 * 
 * 19 Jul 85 -- depp
 *	Removed #ifdef NPIPE.  
 *
 * 4  April 85 -- Larry Cohen
 *	Add IINUSE flag to support open block if in use capability
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
 *	Any specific file systems that are to be included must
 *	include the definition of gnode_common within their defintion
 *	of the file system specific stuff.  The structure must be the
 *	first item within the definition since gfs will use fields
 *	from gnode_common irrespective of fs type. This is the only way I
 *	can think of to keep from having two copies of gnode_common
 *	around since the common stuff is in the on-disk format for
 *	a ufs inode.
 */


#ifndef __GNODE__
#define __GNODE__

#ifdef KERNEL
#include "../h/ansi_compat.h"
#include "../h/gnode_common.h"
#else
#include <ansi_compat.h>
#include <sys/smp_lock.h>
#include <sys/gnode_common.h>
#endif /* KERNEL */
#define PADLEN 128
#define G_FREEBYTES PADLEN - sizeof(struct gnode_common)

struct gnode {
	struct	gnode_req {
		struct	gnode *gr_chain[2];	/* must be first */
		u_long	gr_flag;
		u_short	gr_count;	/* reference count */
		dev_t	gr_dev;		/* device where gnode resides */
		dev_t	gr_rdev;	/* for special files */
		u_short	gr_shlockc;	/* count of shared locks on gnode */
		u_short	gr_exlockc;	/* count of exclusive locks on gnode */
		gno_t	gr_number;    /* i number, 1-to-1 with device address */
		long	gr_id;		/* unique identifier */
		struct	mount *gr_mp;	/* where my mount structure is */
	struct	gnode_ops {	/* begin mount ops */
/*		return value	function	arguments		*/

		struct gnode * 	(*go_namei)(	/* ndp			*/ );
		int		(*go_link)(	/* gp, ndp		*/ );
		int		(*go_unlink)(	/* gp,ndp		*/ );
		struct gnode *	(*go_mkdir)(	/* pgp,name,mode	*/ );
		int		(*go_rmdir)(	/* gp,ndp		*/ );
		struct gnode *	(*go_maknode)(	/* mode,dev,ndp		*/ );
		int		(*go_rename)(	/* gp,from_ndp,to_ndp,flag*/ );
		int		(*go_getdirents)(/* gp, uio, cred	*/ );
		int		(*go_rele)(	/* gp			*/ );
		int		(*go_syncgp)(	/* gp			*/ );
		int		(*go_trunc)(	/* gp, newsize, cred	*/ );
		int		(*go_getval)(	/* gp			*/ );
		int		(*go_rwgp)(	/* gp,uio, rw, flag,cred*/ );
		int		(*go_rlock)(    /* gp, ld, cmd, fp      */ );
		int		(*go_seek)(	/* gp, loc		*/ );
		int		(*go_stat)(	/* gp, statbuf		*/ );
		int		(*go_lock)(	/* gp			*/ );
		int		(*go_unlock)(	/* gp			*/ );
		int		(*go_gupdat)(	/* gp,atime,mtime,wait,cred*/ );
		int		(*go_open)(	/* gp, mode		*/ );
		int		(*go_close)(	/* gp, flag		*/ );
		int		(*go_select)(	/* gp, rw, cred		*/ );
		int		(*go_readlink)(	/* gp, uio		*/ );
		int		(*go_symlink)(	/* gp, source, dest	*/ );
		int		(*go_fcntl)(	/* gp,cmd,args,flag,cred*/ );
		int             (*go_freegn)(   /* gp                   */ );
		int		(*go_bmap)(	/* gp,vbn,rw,size,sync	*/ );
	} *gr_ops;
		struct gnode_ops *gr_altops;
		struct	dquot *gr_dquot; /* hope this can stay ?? */
		u_long	gr_blocks;	/* added to aid matt */
		u_long	gr_gennum;	/* incarnation of the gnode */
		u_long  gr_init;	/* init state of gnode, see below */
		struct lock_t gr_lk;	/* gnode sleep lock */
		union {
			daddr_t	gf_lastr;	/* last read (read-ahead) */
			struct {
			    	int pad;	/* may need read-ahead */
			    	struct text	*gf_text;
				struct x_hcmap { /* remote hashed page array */
				        int x_xcount; /* num texts using gp */
				        int x_hcount; /* num pf in hcmap */
					int x_hcmap[1]; /* pf array, hashed */
				} *gf_hcmap;
			} g_txt;
			struct	socket *gs_socket;
			struct	{
				int pad;	/* need read-ahead for nfs */
				struct gnode  *gf_freef; /* free list forward */
				struct gnode  **gf_freeb;/* free list back */
			} g_fr;
			struct fifonode *gf_fifo;	/* new named pipe */
			struct {
				u_short gs_freadcount;
				u_short gs_fwritecount;
			} g_so;
			struct {
				int pad; /* may need readahead */
				struct mount *gm_mp; /* this is a mount point */
			} g_pmp;
		} gr_un;
	} g_req;
	union {
		char pad[PADLEN];	/* 128 - should be a sizeof */
		struct gnode_common gn;	/* to make the defines easier */
		struct {
			struct gnode_common _x;
			char *free;
		} _freespace;
	} g_in;
};


#define	g_chain	g_req.gr_chain
#define	g_flag	g_req.gr_flag
#define	g_count	g_req.gr_count
#define	g_dev	g_req.gr_dev
#define g_rdev	g_req.gr_rdev		/* until something better comes up*/
#define g_gennum	g_req.gr_gennum
#define g_init		g_req.gr_init
#define g_lk		g_req.gr_lk
#define g_blocks	g_req.gr_blocks /* see above */
#define	g_shlockc	g_req.gr_shlockc
#define	g_exlockc	g_req.gr_exlockc
#define	g_number	g_req.gr_number
#define	g_id		g_req.gr_id
#define	g_mp		g_req.gr_mp
#define	g_ops		g_req.gr_ops
#define	g_altops	g_req.gr_altops
#define	g_dquot		g_req.gr_dquot
#define	g_mode		g_in.gn.gc_mode
#define	g_nlink		g_in.gn.gc_nlink
#define	g_uid		g_in.gn.gc_uid
#define	g_gid		g_in.gn.gc_gid
#define g_freespace	g_in._freespace._free
/* ugh! -- must be fixed */
#ifdef __vax
#define	g_size		g_in.gn.gc_size.val[0]
#endif /* __vax */
#ifdef __mips
#ifdef __MIPSEL
#define	g_size		g_in.gn.gc_size.val[0]
#endif /* __MIPSEL */
#ifdef __MIPSEB
#define	g_size		g_in.gn.gc_size.val[1]
#endif /* __MIPSEB */
#endif /* __mips */
#define	g_atime		g_in.gn.gc_atime
#define	g_mtime		g_in.gn.gc_mtime
#define	g_ctime		g_in.gn.gc_ctime
#define	g_lastr		g_req.gr_un.gf_lastr
#define	g_socket	g_req.gr_un.gs_socket
#define g_fifo		g_req.gr_un.gf_fifo
#define	g_forw		g_chain[0]
#define	g_back		g_chain[1]
#define	g_freef		g_req.gr_un.g_fr.gf_freef
#define	g_freeb		g_req.gr_un.g_fr.gf_freeb
#define	g_frcnt		g_req.gr_un.g_so.gs_freadcount
#define	g_fwcnt		g_req.gr_un.g_so.gs_fwritecount
#define g_mpp		g_req.gr_un.g_pmp.gm_mp
#define g_textp		g_req.gr_un.g_txt.gf_text
#define g_hcmap_struct  g_req.gr_un.g_txt.gf_hcmap
#define g_xcount        g_req.gr_un.g_txt.gf_hcmap->x_xcount
#define g_hcount        g_req.gr_un.g_txt.gf_hcmap->x_hcount
#define g_hcmap         g_req.gr_un.g_txt.gf_hcmap->x_hcmap
/* flags */

/* g_flag is now a u_long */

/* #define	GLOCKED		0x00000001	 moved to gr_lock */
#define	GUPD		0x00000002	/* file has been modified */
#define	GACC		0x00000004	/* gnode access time to be updated */
#define	GMOUNT		0x00000008	/* gnode is mounted on */
#define	GWANT		0x00000010	/* some process waiting on lock */
#define	GTEXT		0x00000020	/* gnode is pure text prototype */
#define	GCHG		0x00000040	/* gnode has been changed */
#define	GSHLOCK		0x00000080	/* file has shared lock */
#define	GEXLOCK		0x00000100	/* file has exclusive lock */
#define	GLWAIT		0x00000200	/* someone waiting on file lock */
#define	GMOD		0x00000400	/* gnode has been modified */
#define GINUSE		0x00000800	/* line turnaround semaphore */
#define	GRENAME		0x00001000	/* gnode is being renamed */
#define GSYNC		0x00002000
#define GINCOMPLETE	0x00004000	/* gnode transitioning between sfs's*/
#define	GXMOD		0x00008000	/* gnode is text, but impure (XXX) */
#define GCMODE		0x00010000	/* permissions of file has changed */
#define GCLINK		0x00020000	/* number of links has changed */
#define GCID		0x00040000	/* owner or group has changed */
#define GTRC		0x00080000	/* one or more procs tracing */

#define ASYNC		GSYNC

/* modes */
#define	GFMT		0170000		/* type of file */

#define	GFPORT		0010000		/* port (named pipe) */
#define	GFCHR		0020000		/* character special */
#define	GFDIR		0040000		/* directory */
#define	GFBLK		0060000		/* block special */
#define	GFREG		0100000		/* regular */
#define	GFLNK		0120000		/* symbolic link */
#define	GFSOCK		0140000		/* socket */
#define GFPIPE		0160000		/* pipe */

#define	GSUID		04000		/* set user id on execution */
#define	GSGID		02000		/* set group id on execution */
#define	GSVTX		01000		/* save swapped text even after use */
#define	GREAD		0400		/* read, write, execute permissions */
#define	GWRITE		0200
#define	GEXEC		0100


/*
 * Here are the possible values for the g_init field of a gnode.  This
 * field tells what initialization is needed for a gnode in the cache.
 * It is manipulated under the control of the gnode sleep lock, except
 * by some low-level code that knows it has the only ref on the gnode.
 * All users of the g_init field reside in gfs_gnode.c.
 */
#define READY_GNODE	1		/* no initialization necessary */
#define RECLAIM_GNODE	2		/* partially initialized */
#define NEW_GNODE	3		/* completely uninitialized */

/*
 * Here are some macros for use with gnodes.
 */

#define gisready(gp) ((gp)->g_init == READY_GNODE)

#define isdevice(gp) \
	((((gp)->g_mode & GFMT) == GFBLK) || (((gp)->g_mode & GFMT) == \
	GFCHR))

#define glocked(gp) (smp_owner(&(gp)->g_lk))

#define gfs_lock(gp)  { \
	if ((gp)->g_count < 1) \
		panic("gfs_lock: locking unrefed gnode"); \
	smp_lock(&(gp)->g_lk, LK_RETRY); \
}

#define gfs_unlock(gp)	{ \
	if ((gp)->g_count < 1) \
		panic("gfs_unlock: locking unrefed gnode"); \
	smp_unlock(&(gp)->g_lk); \
}

/*
 * The gassert() macro is used for locked gnode sanity checking.
 */
#ifdef GFSDEBUG
#define gassert(gp) {	\
	if (!glocked(gp)) panic("unlocked gnode should be locked"); \
}
#else /* GFSDEBUG */
#define gassert(gp) ;
#endif /* GFSDEBUG */


#ifdef KERNEL
struct gnode *gnode;		/* the gnode table itself */
struct gnode *gnodeNGNODE;	/* the end of the gnode table */
int	ngnode;			/* number of slots in the table */
u_long	nextgnodeid;		/* unique id generator */

struct	gnode *rootdir;		/* pointer to gnode of root directory */
struct	gnode *galloc();
struct	gnode *gget();
struct	gnode *owner();
struct	gnode *maknode();
struct	gnode *gfs_namei();
struct	gnode *getegnode();
void	gremque();
void	grele();

gno_t	dirpref();
extern union ghead {			/* gnode LRU cache, Chris Maltby */
	union  ghead *gh_head[2];
	struct gnode *gh_chain[2];
} ghead[];

/*
 * Invalidate a gnode. Used by the namei cache to detect stale
 * information. Also used by NFS for quick buffer cache invalidation
 * for a single gnode.  At an absurd rate (at least it is today)
 * of 100 calls/second, cache invalidation (both namei and buffer) 
 * should only occur once every 16 months.
 */
extern struct lock_t lk_gnode;
unsigned cacheinval_count;
#define cacheinval(gp) \
{ \
	int need_lock = 0; \
	++cacheinval_count; \
	if (smp_owner(&lk_gnode) != LK_TRUE) { \
		need_lock = 1; \
		smp_lock(&lk_gnode, LK_RETRY); \
	} \
	(gp)->g_id = ++nextgnodeid; \
	if (nextgnodeid == 0) { \
		cacheinvalall(); \
		(gp)->g_id = ++nextgnodeid; \
	} \
	if (need_lock) \
		smp_unlock(&lk_gnode); \
}

/*
 * GNODE cacheing stuff 
 */
#define GNOHSZ 512

#if	((GNOHSZ&(GNOHSZ-1)) == 0)
#define	GNOHASH(dev,gno)	(((int)((dev)+(gno)))&(GNOHSZ-1))
#else
#define	GNOHASH(dev,gno)	(((int)((dev)+(gno)))%GNOHSZ)
#endif

extern struct gnode *gfreeh, **gfreet;
extern struct timeval *timepick;

#endif /* KERNEL */

#define OPS(gp)	\
		((gp)->g_ops)
#define GNOFUNC -1


/*
 *  These macros are used to help keep track of remote hashed pages
 *  The prefered method would be to simply keep a list of all hashed
 *  pages for a given text structure.  Unfortunately, that would increase
 *  the size of the cmap entry, which is undesirable (at this time ...).
 */
#define G_HCMAP_SIZE(xp) (((((xp)->x_size) >> CLSIZELOG2) * sizeof (int)) + \
			  (2 * sizeof(int)))

#define G_SET_HCMAP(xp,gp,c,cindx) { int *tmp;                          \
	       if ((c)->c_mdev != MSWAPX) {                             \
	           if (((c)->c_page) >= xp->x_size) 			\
			 panic("X_SET_HCMAP: page number too large"); 	\
		   tmp = &((gp)->g_hcmap[((c)->c_page) >> CLSIZELOG2]); \
		   if (*tmp != 0)                                       \
			 panic("X_SET_HCMAP: hcmap != 0"); 		\
		   *tmp = (cindx);                                      \
		   (c)->c_remote = 1; 					\
		   (gp)->g_hcount++; 					\
	       }                                                        \
}

#define G_RST_HCMAP(xp,gp,c) { int *tmp;				\
               if (c->c_remote) { 					\
	           if (((c)->c_page) >= xp->x_size) 			\
	               panic("X_RST_HCMAP: page number too large"); 	\
		   tmp = &((gp)->g_hcmap[((c)->c_page) >> CLSIZELOG2]); \
	           if (*tmp == 0)	                                \
		       panic("X_RST_HCMAP: hcmap == 0"); 		\
	           *tmp = 0; 	                                        \
	           (c)->c_remote = 0; 					\
		   (gp)->g_hcount--; 					\
	       } 							\
}
	           

/*
 ***************************************************************************
 ******************* START GFS FS MACROS ***********************************
 ***************************************************************************
 */

/*
 * Gnode function callout macros to leave the code readable.
 */

/*
 * GRELE(gp)
 *	struct	gnode	*gp;
 *
 *	Release a gnode
						

#define	GRELE(gp)	\
		(OPS(gp)->go_rele ? OPS(gp)->go_rele(gp) : GNOFUNC)

 */

/*
 * GALLOC(pgp, gpref, mode)
 *	struct	gnode	*pgp;
 *	gno_t	gpref;
 *	int	mode;
 *
 * Allocate a gnode around prefered gnode gpref with parent pgp and
 * mode mode.  gpref is only advisory and may be ignored.
 */

#define	GALLOC(pgp, gpref, mode)	\
		((struct gnode *)(OPS(pgp)->go_alloc ? \
		OPS(pgp)->go_alloc((pgp), (gpref), (mode)) : \
		(struct gnode *) GNOFUNC))
		
		
/*
 * GSYNCG(gp, cred)
 *	struct	gnode	*gp;
 *	struct  ucred *cred;
 *
 * Cause any cached data to be written out.
 */

#define GSYNCG(gp,cred)	\
		((OPS(gp)->go_syncgp ? (OPS(gp)->go_syncgp)(gp,cred) : GNOFUNC))


/*
 * GFREE(gp)
 *	struct	gnode	*gp;
 *
 * Free a gnode
 */

#define GFREE(gp)	\
		((OPS(gp)->go_free ? OPS(gp)->go_free(gp)) : GNOFUNC)


/*
 * GTRUNC(gp, newsize, cred) 
 *	struct	gnode	*gp;
 *	u_long newsize;
 *	struct ucred *cred;
 *
 * Truncate a file to the specified size.
 */

#define GTRUNC(gp, newsize, cred)	\
		(OPS(gp)->go_trunc ? OPS(gp)->go_trunc((gp), (newsize), \
		(cred)) : GNOFUNC)


/*
 * GGETVAL(gp)
 *	struct	gnode	*gp;
 *
 *	Get uptodate values in the gnode_common fields of the gnode
 *	This is primarily for remote file systems.
 */
	
#define GGETVAL(gp)	\
		(OPS(gp)->go_getval ? OPS(gp)->go_getval(gp) : GNOFUNC)
		
		
/*
 * GRWGP(gp, uio, rw, ioflag, cred)
 *	struct	gnode	*gp;
 *	struct	uio	*uio;
 *	enum	uio_rw	rw;
 *	int ioflag;
 *	struct ucred 	*cred;
 *
 * Read or write data on a gnode using the uio structure passed to the
 * function.
 */
					
#define GRWGP(gp, uio, rw, ioflag, cred)	\
		((OPS(gp)->go_rwgp) ((gp), (uio), (rw), (ioflag), (cred)))


/*
 * GLINK(gp, pgp, source, target)
 *	struct gnode *gp;
 *	struct gnode *pgp;
 *	char *source
 *	char *target;
 *
 * link the file target whose parent is pgp to source which is
 * pointed to by gp
 */

#define	GLINK(gp, ndp)	\
		(OPS(gp)->go_link ? OPS(gp)->go_link((gp), (ndp)) \
		: GNOFUNC)
	

/*
 * GUNLINK(gp, ndp)
 *	struct gnode *gp;
 *	struct nameidata *ndp
 *
 * unlink the file pointed to by gp having the ndp set for us
 */
	
#define GUNLINK(gp, ndp)	\
		(OPS(gp)->go_unlink ? OPS(gp)->go_unlink((gp), (ndp)) \
		: GNOFUNC)


/*
 * GMKDIR(pgp, name, mode)
 *	struct gnode *gp;
 *	char *name;
 *	u_int mode;
 *
 * create a directory whose parent is pointed to by pgp with modes mode
 */

#define GMKDIR(pgp, name, mode)	\
		(OPS(pgp)->go_mkdir ? OPS(pgp)->go_mkdir((pgp), (name), \
		(mode)) : (struct gnode *) GNOFUNC)


/*
 * GRMDIR(gp, ndp)
 *	struct gnode *gp;
 *	struct nameidata *ndp;
 *
 * remove the directory pointed to by gp with ndp set up
 */
	
#define GRMDIR(gp, ndp)	\
		(OPS(gp)->go_rmdir ? OPS(gp)->go_rmdir((gp), (ndp)) : GNOFUNC)


/*
 * GMAKNODE(mode, dev, ndp)
 *	u_int mode;
 *      dev_t dev;
 *	struct nameidata *ndp;
 *
 * make a special file with modes mode (note mode has GFMT meaning)
 */

#define GMAKNODE(mode,dev,ndp)	\
		(OPS((ndp)->ni_pdir)->go_maknode ? \
		(OPS((ndp)->ni_pdir)->go_maknode)((mode),(dev),(ndp)) : \
		(struct gnode *) GNOFUNC)
	

/*
 * GRENAMEG(gp, ssd, source_ndp, tsd, target_ndp, flag)
 *	struct gnode *gp;
 *	struct gnode *ssd, *tsd;
 *	struct nameidata *source_ndp;
 *	struct nameidata *target_ndp;
 *	int flag;
 *
 * rename the file pointed to by gp with source_ndp set up from the namei
 * obtained from the call for gp and move it to target_ndp (note that dirp
 * is used).  ssd and tsd are the starting directories for parsing the
 * source and target respectively.
 */
	
#define GRENAMEG(gp, ssd, source_ndp, tsd, target_ndp, flag)	\
		(OPS(gp)->go_rename ? OPS(gp)->go_rename((gp), (ssd), \
		(source_ndp), (tsd), (target_ndp), (flag)) : GNOFUNC)


/*
 * GGETDIRENTS(gp, uio, cred)
 *	struct gnode *gp;
 *	struct uio *uio;
 *	struct ucred *cred
 *
 * read the directory pointed to by gp, format the output according to
 * the generic directory format, and return it to the place pointed to
 * by uio
 */
	
#define GGETDIRENTS(gp, uio, cred)	\
		((OPS(gp)->go_getdirents)((gp),(uio), (cred)))


/* 
 * GSYMLINK(ndp, target)
 *	struct nameidata *ndp;
 *	char *target;
 *
 * create a symbolic link named source using pgp as the pointer to the parent
 * and point it towards the name target
 */

#define GSYMLINK(ndp, target)	\
		(OPS(ndp->ni_pdir)->go_symlink ? OPS(ndp->ni_pdir)\
		->go_symlink((ndp), (target)) : GNOFUNC)
					

/*
 * GNAMEI(ndp)
 *	struct nameidata *ndp;
 *
 * parse the pathname pointed to by ndp and return a gnode pointer on
 * success
 */

#define GNAMEI(ndp)	\
		(gfs_namei(ndp))


/*
 * GRLOCK(gp, ld, cmd, fp)
 *	struct gnode *gp;
 *	struct flock *ld;
 *	int cmd;
 *	struct file *fp;
 *
 * perform the command cmd on the file/region pointed to by gp
 * using the given structures
 */

#define GRLOCK(gp, ld, cmd, fp) \
		(OPS(gp)->go_rlock ? OPS(gp)->go_rlock((gp), (ld), \
		(cmd), (fp)) : GNOFUNC)

/*
 * GSEEK(gp, loc)
 *	struct gnode *gp;
 *	int loc;
 *
 * seek to loc on the file pointed to by gp
 */

#define GSEEK(gp, loc)	\
		(OPS(gp)->go_seek ? OPS(gp)->go_seek((gp), (loc)) : GNOFUNC)
		
		
/*
 * GSTAT(gp, statbuf)
 *	struct gnode *gp;
 *	struct stat *statbuf;
 *
 * fill in various file stats into the buf statbuf on the file pointed
 * to by gp
 */

#define GSTAT(gp, statbuf)	\
	((OPS(gp)->go_stat)((gp), (statbuf)))

		
/*
 * GLOCK(gp)
 *	struct gnode *gp;
 *
 * lock the gnode gp
 */

#define GLOCK(gp)	\
		(OPS(gp)->go_lock ? OPS(gp)->go_lock(gp) : GNOFUNC)
		
		
/*
 * GUNLOCK(gp)
 *	struct gnode *gp;
 *
 * unlock the gnode gp
 */

#define GUNLOCK(gp)	\
		(OPS(gp)->go_unlock ? OPS(gp)->go_unlock(gp) : GNOFUNC)

		
/*
 * GUPDATE(gp, atime, mtime, waitfor, cred)
 *	struct gnode *gp;		
 *	struct timeval	*atime;
 *	struct timeval	*mtime;
 *	int	waitfor;
 *	struct ucred	*cred;
 *
 * update the gnode gp with the access and modify times atime and mtime.
 * wait for completion if waitfor set
 */

#define GUPDATE(gp, atime, mtime, waitfor, cred)	\
		(OPS(gp)->go_gupdat ? OPS(gp)->go_gupdat((gp), (atime), \
		(mtime), (waitfor), (cred)) : GNOFUNC)
		
		
/*
 * GOPEN(gp, ioflag)
 *	struct gnode *gp;
 *	int ioflag;
 *
 * attempt to open the file pointed to by gp with the flags ioflag
 */

#define GOPEN(gp, ioflag)	\
		(OPS(gp)->go_open)((gp), (ioflag))
		
		
/*
 * GCLOSE(gp, ioflag)
 *	struct gnode *gp;
 *	int ioflag;
 *
 * attempt to close the gnode gp
 */
		
#define GCLOSE(gp, ioflag)	\
		(OPS(gp)->go_close) ((gp), (ioflag))
		

/*
 * GSELECT(gp, rw, cred)
 *	struct gnode *gp;
 *	int rw;
 *	struct ucred *cred;
 *
 * perform a select on gp of type rw
 */
		
#define GSELECT(gp, rw, cred)		\
		(OPS(gp)->go_select ? OPS(gp)->go_select((gp), (rw), (cred)) \
		: GNOFUNC)
		
		
/*
 * GREADLINK(gp, uio)
 *	struct gnode *gp;
 *	struct uio *uio;
 *
 * read the symbolic link information of gp into uio
 */
		
#define GREADLINK(gp, uio)	\
		(OPS(gp)->go_readlink ? OPS(gp)->go_readlink((gp), (uio)) \
		: GNOFUNC)
		
		

/*
 * GFNCTL(gp, cmd, args, flag, cred)
 *	struct gnode *gp;
 *	int cmd;
 *	char *args;
 *	int flag;
 *	struct cred *ucred;
 *
 * perform some non-generic action on the gnode gp
 */
	
#define GFNCTL(gp, cmd, args, flag, cred)	\
		(OPS(gp)->go_fcntl ? OPS(gp)->go_fcntl((gp), (cmd), \
		(args), (flag), (cred)) : GNOFUNC)
		
		
/*
 * GBMAP(gp,vbn,rw,size)
 *	struct gnode *gp;	gnode to map
 *	int vbn;		virtual block number
 *	int rw;			read or write operation, B_READ or B_WRITE
 *	int size;		size of request - used only on write
 *
 * interface to block mapping routine (mainly for virtual memory (vinifod))
 * but it is used in ufs_namei.c and ufs_gnodeops.c and ufs_subr.c
 * 
 * Return value is the actual starting disk block # (lbn or logical block #)
 * or -1 on some error with u.u_error set appropriately.
 */

#define GBMAP(gp,vbn,rw,size,sync)				\
	((OPS((gp))->go_bmap) ((gp),(vbn),(rw),(size),(sync)))
	
/*
 * GFREEGN(gp)
 *      struct gnode *gp;
 *
 * call the specific file system routine to release resources attached
 * to a gnode that will be used by another filesystem type
 */

#define GFREEGN(gp)                                     \
                 (OPS(gp)->go_freegn ? OPS(gp)->go_freegn(gp) : GNOFUNC)

/*
 ***************************************************************************
 ***************************************************************************
 ***************************************************************************
 */


#endif /* __GNODE__ */
