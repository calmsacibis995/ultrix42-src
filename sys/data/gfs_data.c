#ifndef lint
static	char	*sccsid = "@(#)gfs_data.c	4.5	(ULTRIX)	4/11/91";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1986,87 by			*
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
 * 27 Dec 90 -- prs
 *	Added sanity check of ufs_blkpref_lookbehind value
 *	to init_fs().
 *
 *  9 Mar 89 -- chet
 *	Add dnlc_purge() and dnlc_purge1() dummy routines
 *	when NFS not defined.
 * 
 * 18 Jul 88 -- condylis
 *	Moved init of nfs and rpc SMP locks from init_main.c to this
 *	file's init_fs.
 *
 * 19 May 88 -- prs
 *	Moved code from init_main here, to make mounting of root cleaner.
 *
 *  4 Apr 88 -- Fred G
 *	Allocate kernel_locking variable for both UFS and non-UFS based
 *	environments.
 *
 * 13 Mar 88 -- chet
 *	Add nfsportmon switch when NFS not defined (referenced by setsysinfo())
 *
 * 26-01-88	Fred Glover
 *	Add flckinit () and cleanlocks () stubs for non-UFS based kernels
 *
 * 18-01-88	Fred Glover
 *	Add klm_drlock stub for non-NFS kernels
 *
 * 12-11-87	Robin L. and Larry C.
 *	Added new kmalloc memory allocation to system.
 *
 * 11 Jun 87 -- logcher
 *	Added null to end of root pathname, added kmem_alloc for 
 *	devname instead of 1K on the stack.  Made size of name
 *	same as fields in vmb.h
 *
 * 02 Mar 87 -- logcher
 *	Merged in diskless changes for mounting an NFS root filesystem
 *
 * 15 Sep 86 -- bglover
 *	Add back declaration of nfs_biod()
 *
 * 11 Sep 86 -- koehler
 *	moved the mounting of root into here
 *
 ***********************************************************************/

#include "../machine/pte.h"

#include "../h/param.h"
#include "../h/mount.h"
#include "../h/user.h"
#include "../h/errno.h"
#include "../h/types.h"
#include "../h/socket.h"
#include "../h/ioctl.h"
#include "../h/kernel.h"
#include "../net/rpc/types.h"
#include "../sas/mop.h"
#include "../net/netinet/in.h"
#include "../net/net/if.h"
#include "../fs/nfs/nfs.h"
#include "../fs/nfs/nfs_gfs.h"
/* for mount dev only */
#include "../h/systm.h"
#include "../h/dir.h"
#include "../h/gnode_common.h"
#include "../h/gnode.h"
#include "../h/proc.h"
#include "../h/buf.h"
#include "../h/file.h"
#include "../h/conf.h"
#include "../h/devio.h"
#ifdef UFS
#include "../fs/ufs/fs.h"
#include "../fs/ufs/ufs_inode.h"
#include "../fs/ufs/ufs_mount.h"
#endif UFS

extern int roottype;

/* include the rest of the sfs's here */
#include "../h/fs_types.h"
#include "cdfs.h"

#ifdef mips
struct pt part_tbl;
#endif mips

#ifdef UFS

int kernel_locking = 1; /* Sys-V style locking; default is kernel */

#endif UFS

/*
 * init_fs - sets up fs
 */
init_fs() {
	int n_fs;
#ifdef UFS
	extern struct mount *ufs_mount();
	extern int ufs_blkpref_lookbehind;
#endif UFS
#ifdef NFS
	extern struct mount *nfs_mount();
	extern struct mount *nfs_mountroot();
#endif NFS

#ifdef CDFS
	extern struct mount *cdfs_mount();
#endif CDFS

	bzero((caddr_t)mount_sfs,NUM_FS*sizeof(struct _sfs));

#ifdef UFS
	MOUNTFS(GT_ULTRIX) = ufs_mount;
#endif UFS
#ifdef NFS
	MOUNTFS(GT_NFS) = nfs_mount;
#endif
#ifdef CDFS
	MOUNTFS(GT_CDFS) = cdfs_mount;
#endif CDFS
#ifdef NFS
	rpcinit();		/* Initialize rpc SMP locks	*/
	nfsinit();		/* Initialize nfs SMP locks	*/
#endif
	/* include the rest of the mount functions here */
	mteinit();		/* Initialize mount table       */
				/* SMP locks			*/
#ifdef UFS
#define max_lookbehind 20

	if (ufs_blkpref_lookbehind < 1)
		ufs_blkpref_lookbehind = 1;
	else if (ufs_blkpref_lookbehind > max_lookbehind)
		ufs_blkpref_lookbehind = max_lookbehind;

#endif UFS
}

#ifndef CDFS
cdfs_mount() {
}
int cdfs_mount_ops;
int cdfs_gnode_ops;
#endif NOT CDFS

#ifndef NFS

nfs_biod() {
}

nfs_mount() {
}

nfs_mountroot(){
}

nfs_getfh() {
}

nfs_svc() {
}

exportfs() {
}

nfs_strategy() {
}

nfs_resolvfh() {
}

klm_drlock() {
}

int nfs_mount_ops;
int nfs_gnode_ops;
int nfsportmon;		/* secure port monitoring switch */
 
#endif NFS

#ifndef UFS

gno_t rootino = 0;
int kernel_locking = 1; /* Sys-V style locking; default is kernel */

ufs_mount() {
}

ufs_seek() {
return(0);
}

rsblk() {
}

ptcmp() {
}

ssblk() {
}

#endif !UFS

#ifndef INET
in_control() {
}
#endif !INET

fhandle_t fh;
struct sockaddr_in sins;
char rmtdir[MAXPATHLEN];
char name[32];  /*  *****magic number from sas/mop.h srvname[]  *** */
struct netblk *netblk_ptr;

/*
 * mount_root - mounts either nfs or ufs according to roottype
 */
mount_root(mp)
	struct mount *mp;
{
int srvlen,dirlen;
int testres;
struct nfs_gfs_mount args;
char *devname;

/*
        kmem_alloc(devname, char *, MAXPATHLEN, KM_TEMP);
*/
        KM_ALLOC(devname, char *, MAXPATHLEN, KM_TEMP, KM_NOWAIT | KM_CLEAR);
        if (devname == NULL)
		panic("mount_root: can't kmem_alloc. . .");
        KM_ALLOC(mp->m_fs_data, struct fs_data *, sizeof(struct fs_data),
		 KM_MOUNT, KM_NOWAIT | KM_CLEAR);
        if (mp->m_fs_data == NULL)
	        panic("mount_root: can't km_alloc fs_data");

	bzero(&args,sizeof(args));
	bzero(name,sizeof(name));
#ifdef NFS
	dnlc_init();
#endif
if (roottype < 0 || roottype >= NUM_FS) {  /* fs_type not in range 0-0xff */
	u.u_error = ENXIO;
/*		mp->m_bufp = NULL; */
#ifdef vax
	asm("halt");
#endif vax
#ifdef mips
	while(1);
#endif mips
	}

switch(roottype) 
	{
	case GT_NFS:	/* GT_NFS  NFS_TYPE */ 
		srvlen=strlen(netblk_ptr->srvname); 
		dirlen=strlen(netblk_ptr->rootdesc);

		if ((srvlen+1+dirlen) > MAXPATHLEN){
			printf("mount path too long");
#ifdef vax
			asm("halt");
#endif vax
#ifdef mips
			while(1);
#endif mips
			}				/* in this order */
		bcopy(netblk_ptr->srvname,devname,srvlen);
		bcopy(netblk_ptr->srvname,name,srvlen);
		args.hostname= name;
		bcopy(netblk_ptr->rootdesc,rmtdir,dirlen);
		nfs_resolvfh(&fh,rmtdir);
		args.fh= &fh;
		bcopy(":",devname+srvlen,1);
		bcopy(rmtdir,(devname+srvlen+1),dirlen);
		bcopy("\0", (devname+srvlen+1+dirlen), 1);
/* put rest of junk in nfsmntargs */
		nfsmntargs(&args);

		testres= (int)(*nfs_mountroot)(devname, "/",0,mp, &args);
		break;
	case GT_ULTRIX:   /*  GT_ULTRIX */
		testres = (int)ufs_mount((caddr_t) 0, "/", 0, mp, (caddr_t) 0);
		break;
	}
	if (testres == 0) {
		if (u.u_error == EROFS) {
			printf("root file system is read only");
#ifdef mips
			while(1);
#endif mips
		}
		else if (u.u_error==ENODEV) {
			printf("root file system device is offline");
#ifdef vax
			asm("halt");
#endif vax
#ifdef mips
			while(1);
#endif mips
		}
		else {
			printf("no root file system");
#ifdef vax
			asm("halt");
#endif vax
#ifdef mips
			while(1);
#endif mips
		}
	}
	mp->m_fstype=roottype;
        mp->m_gnodp = (struct gnode *) NULL;
        mp->m_flgs |= MTE_DONE;
        nmount++;
#ifdef ROOTSYNC
	mp->m_flags |= M_SYNC;
#endif
        (void)km_free(devname, KM_TEMP);
}

#ifdef DEBUG
printargs(args)
struct nfs_gfs_mount *args;
{
#ifdef notdef
struct nfs_gfs_mount {
	struct sockaddr_in *addr;	/* file server address */
	fhandle_t *fh;			/* File handle to be mounted */
	int	flags;			/* flags */
	int	wsize;			/* write size in bytes */
	int	rsize;			/* read size in bytes */
	int	timeo;			/* initial timeout in .1 secs */
	int	retrans;		/* times to retry send */
	char	*hostname;		/* server's name */
	char	*optstr;		/* options string */
	int	gfs_flags;		/* GFS-specific flags */
	int	pg_thresh;		/* page threshold for exec */
};
#endif notdef
	cprintf("args:addr 0x%x, fh 0x%x, flags 0x%x wsize %d rsize %d\n",
		args->addr,args->fh,args->flags,args->wsize,args->rsize);
	cprintf("args:timeo %d, retrans %d, hostname %s optstr %s gfs_flags 0x%x pgthresh %d\n",
		args->timeo,args->retrans,args->hostname,args->optstr,args->gfs_flags, args->pg_thresh);
}
#endif DEBUG

/*
 * nfsmntsrgs - sets up remaining mount args for nfs_mountroot() 
 */
nfsmntargs(args)
struct nfs_gfs_mount *args;
{
	args->flags=args->gfs_flags=0;
	args->flags &= ~NFSMNT_SOFT;
	args->pg_thresh=256;
	args->flags |= NFSMNT_PGTHRESH;
	args->addr = &sins;
	args->addr->sin_addr.s_addr=ntohl(netblk_ptr->srvipadr);
	args->addr->sin_family= AF_INET; 
	args->addr->sin_port = htons(NFS_PORT);
	args->wsize=args->rsize=8192;
	args->flags |= NFSMNT_WSIZE;
	args->flags |= NFSMNT_RSIZE;
	args->timeo=11;
	args->flags |= NFSMNT_TIMEO;
	args->retrans=4;
	args->flags |= NFSMNT_RETRANS;
	args->flags |= NFSMNT_HOSTNAME;
	args->optstr = "hard";
	args->acregmin=3;
	args->acregmax=60;
	args->acdirmin=30;
	args->acdirmax=60;
}

