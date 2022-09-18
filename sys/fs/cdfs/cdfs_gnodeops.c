#ifndef lint
static	char	*sccsid = "@(#)cdfs_gnodeops.c	4.1	(ULTRIX)	11/9/90";
#endif lint

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
/************************************************************************
 *			Modification History
 * 	fs/cdfs/cdfs_gnodeops.c
 *
 *  9-Nov-90 -- prs
 *	Initial creation.
 *
 ***********************************************************************/
#include "../h/param.h"
#include "../h/systm.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/gnode_common.h"	
#include "../h/gnode.h"
#include "../h/proc.h"
#include "../fs/cdfs/cdfs_fs.h"
#include "../fs/cdfs/cdfs_inode.h"
#include "../fs/cdfs/cdfs_mount.h"
#include "../h/fs_types.h"
#include "../h/conf.h"
#include "../h/buf.h"
#include "../h/mount.h"
#include "../h/file.h"
#include "../h/text.h"
#include "../h/uio.h"
#include "../h/ioctl.h"
#include "../h/devio.h"
#include "../h/tty.h"
#include "../h/cmap.h"
#include "../h/stat.h"
#include "../h/kernel.h"
#include "../h/kmalloc.h"

/*
 * Don't cache blocks in non-executable files with the sticky bit
 * set. Used to keep swap files from cluttering up the data cache.
 */

extern int kernel_locking;  /* Sys-V locking: default is kernel */
                            /* kernel == 1;  daemon == 0        */

extern isodebug;

cdfs_rwgp(gp, uio, rw, ioflag, cred)
	register struct gnode *gp;
	register struct uio *uio;
	enum uio_rw rw;
	int ioflag;
	struct ucred *cred;
{
	dev_t dev = (dev_t)gp->g_rdev;
	struct buf *bp;
	daddr_t lbn, bn;
	int error = 0;
	int bsize;
	int rasize, rablock;
	int raoffset; /* Not used */
	int datainbuf, offinbuf;
	struct fs *fs;

	if (isodebug) {
		printf("cdfs_rwgp: gp 0x%x uio 0x%x cred 0x%x count = %d\n", gp, uio,cred,uio->uio_resid);
	}
	/*
	 * If nothing to read......
	 */
	if (uio->uio_resid == 0)
		return (0);
	if (uio->uio_offset >= gp->g_size)
		return(0);

	dev = gp->g_dev;
	fs = FS(gp);
	bsize = ISOFS_LBS(fs);

	do {
		if (uio->uio_offset >= gp->g_size)
			break;
		lbn = uio->uio_offset / bsize;
		bn = cdfs_ibmap(gp, lbn, &datainbuf, &offinbuf);
		if (gp->g_size - (uio->uio_offset + datainbuf) > 0) {
			rablock = cdfs_ibmap(gp, 
					     (uio->uio_offset + datainbuf) / 
					     bsize, &rasize, &raoffset);
		} else {
			rablock = 0;
		}
		offinbuf += (uio->uio_offset % bsize);
		datainbuf -= (uio->uio_offset % bsize);
		datainbuf = MIN(datainbuf, gp->g_size - uio->uio_offset);
		datainbuf = MIN(datainbuf, uio->uio_resid);
		if (datainbuf <= 0)
			break;
		if (rablock)
			bp = breada(dev, bn, fs->fs_ibsize, rablock, 
				    fs->fs_ibsize, (struct gnode *)NULL);
		else
			bp = bread(dev, bn, fs->fs_ibsize, 
				   (struct gnode *)NULL);

		if (bp->b_flags & B_ERROR) 
		{
			error = EIO;
			brelse(bp);
			goto bad;
		}
		u.u_error = uiomove(bp->b_un.b_addr+offinbuf, datainbuf, 
				    rw, uio);
	
		brelse(bp);
	} while (u.u_error == 0 && uio->uio_resid > 0);

	if (error == 0)				/* XXX */
		error = u.u_error;		/* XXX */
bad:
	return (error);
}


cdfs_fcntl(gp, cmd, arg, flag, cred)
	register struct gnode *gp;
	register int cmd;
	register caddr_t arg;
	register int flag;
	struct ucred *cred;
{
	register int fmt = gp->g_mode & GFMT;
	dev_t dev;

	switch (fmt) {
		case GFREG:
		case GFDIR:
			if (cmd == FIONREAD) {
				*(off_t *)arg = gp->g_size - flag;
				return (0);
			}
			if (cmd == FIONBIO || cmd == FIOASYNC ||
			    cmd == FIOSINUSE || cmd == FIOCINUSE) /* XXX */
				return (0);			/* XXX */
			/* fall into ... */
		default:
			return (ENOTTY);
	}
}

cdfs_select(gp, rw, cred)
	register struct gnode *gp;
	register int rw;
	struct ucred *cred;
{ 
  return (1);		/* XXX */
}

cdfs_close(gp, flag)
	register struct gnode *gp;
	register int flag;
{
  return;
}

cdfs_open(gp, mode)
	register struct gnode *gp;
	register int mode;
{

	return (0);
}

cdfs_symlink(ndp, target_name)
	register struct nameidata *ndp;
	register char *target_name;
{
	
	return (u.u_error = EROFS);
}


cdfs_readlink(gp, auio)
	register struct gnode *gp;
	register struct uio *auio;
{
	return (u.u_error = EOPNOTSUPP);
}


cdfs_stat(gp, sb)
	register struct gnode *gp;
	register struct stat *sb;
{
	/*
	 * Copy from gnode table
	 */
	sb->st_dev = gp->g_dev;
	sb->st_ino = gp->g_number;
	sb->st_mode = gp->g_mode;
	sb->st_nlink = gp->g_nlink;
	sb->st_uid = gp->g_uid;
	sb->st_gid = gp->g_gid;
	sb->st_rdev = (dev_t)gp->g_rdev;
	sb->st_size = gp->g_size;
	sb->st_atime = gp->g_atime.tv_sec;
	sb->st_spare1 = gp->g_atime.tv_usec;
	sb->st_mtime = gp->g_mtime.tv_sec;
	sb->st_spare2 = gp->g_mtime.tv_usec;
	sb->st_ctime = gp->g_ctime.tv_sec;
	sb->st_spare3 = gp->g_ctime.tv_usec;
	sb->st_blksize = MAXBSIZE;
	sb->st_blocks = gp->g_size / sb->st_blksize;
	sb->st_gennum = gp->g_gennum;
	sb->st_spare4 = 0;
	return (0);
}

struct fs_data *
cdfs_getfsdata(mp)
	register struct mount *mp;
{
	register struct fs_data *fs_data = mp->m_fs_data;
	register struct fs *fs;
	int blk;
	
	fs = (struct fs *) mp->m_bufp->b_un.b_addr;

	if (fs->fs_format == ISO_9660) {
		blk = ((int)fs->fs_block.isofs.iso_vol_space_size_lsb * 
		       ISOFS_LBS(fs)) / 1024;
		fs_data->fd_gtot = (int)fs->fs_block.isofs.iso_path_tbl_size_lsb;
		fs_data->fd_btot = ((int)fs->fs_block.isofs.iso_vol_space_size_lsb*
			    (int)ISOFS_LBS(fs)) / 1024;
	} else {
		blk = ((int)fs->fs_block.hsgfs.iso_vol_space_size_lsb * 
		       ISOFS_LBS(fs)) / 1024;
		fs_data->fd_gtot = (int)fs->fs_block.hsgfs.iso_path_tbl_size_lsb;
		fs_data->fd_btot = ((int)fs->fs_block.hsgfs.iso_vol_space_size_lsb*
			    (int)ISOFS_LBS(fs)) / 1024;
	}
	fs_data->fd_gfree = 0;
	fs_data->fd_bfree = 0;
	fs_data->fd_otsize = MAXBSIZE;
	fs_data->fd_mtsize = MAXBSIZE;
	fs_data->fd_bfreen = 0;
	fs_data->fd_dev = mp->m_dev;
	fs_data->fd_fstype = GT_CDFS;
	return(fs_data);
}

cdfs_getdirent(gp, uio, cred)
	register struct gnode *gp;
	register struct uio *uio;
	register struct ucred *cred;
{
	dev_t dev = (dev_t)gp->g_rdev;
	struct buf *bp;
	struct fs *fs;
	daddr_t bn;
	int error = 0;
	int dirblkstotransfer;
	int lbs, lbn;
	struct iso_dir *tmp_iso_dir;
	struct hsg_dir *tmp_hsg_dir;
	struct gen_dir *gen_dir;
	union {
		unsigned char incoming[4];
		unsigned int  outgoing;
	} iso_convert_int;
	unsigned int ubuf_size;
	unsigned int isodir_reclen;
	unsigned int isodir_offset;
	unsigned int offinbuf;
	int iso_secsize_resid;
	int rablock, rasize;
	int wasdot;
	unsigned int diskaddr;
	int datainbuf, tsize;
	int isiso;
	int skip_file;

	if (isodebug) {
		printf("cdfs_getdirent: gp 0x%x uio 0x%x cred 0x%x count = %d\n",
			gp, uio, cred, uio->uio_resid);
	}
	KM_ALLOC(gen_dir, struct gen_dir *, ISO_SECSIZE, KM_TEMP, KM_NOARG);

	fs = FS(gp);
	if (fs->fs_format == ISO_9660)
		isiso = 1;
	else
		isiso = 0;
	/*
	 * If nothing to read......
	 */
	if (uio->uio_resid == 0) {
		KM_FREE(gen_dir, KM_TEMP);
		return (0);
	}

	lbs = ISOFS_LBS(fs);

	ubuf_size = (uio->uio_resid / ISO_SECSIZE) * ISO_SECSIZE;

	isodir_offset = uio->uio_offset;
	if (isodir_offset % ISO_SECSIZE) {
		if (isodebug)
			printf("cdfs_getdirent: request not on a ISO_SECSIZE bound\n");
		isodir_offset = (isodir_offset / ISO_SECSIZE) * ISO_SECSIZE;
	}
	/*
	 * Set dirblkstotransfer to number of directory blocks which will
	 * fit in the users buffer.
	 */
	dirblkstotransfer = ubuf_size / ISO_SECSIZE;
	if (dirblkstotransfer <= 0) {
		error = EINVAL;
		if (isodebug)
			printf("cdfs_getdirent: dirblkstotransfer = %d\n",
			       dirblkstotransfer);
		goto out;
	}
	/*
	 * Set dirblkstotransfer to the minimum of the number of directory
	 * blocks that would fit into the user buffer, and the number of
	 * directory blocks left to read in the directory.
	 */
	dirblkstotransfer = MIN(dirblkstotransfer, 
				(gp->g_size - isodir_offset) / ISO_SECSIZE);
	if (isodebug)
		printf("cdfs_getdirent: dirblkstotransfer = %d\n", 
			dirblkstotransfer);
	/*
	 * If there is nothing left to transfer, just return.
	 */
	if (dirblkstotransfer == 0) {
		KM_FREE(gen_dir, KM_TEMP);
		return(0);
	}

	if (isodebug) {
		if (gp->g_number % ISO_SECSIZE) {
			printf("cdfs_getdirentries: gnode number %d (dir) does not start on a sector boundary\n", gp->g_number);
			KM_FREE(gen_dir, KM_TEMP);
		return(0);
		}
	}
	dev = gp->g_dev;
	diskaddr = ((unsigned int)G_TO_DIR(gp)->iso_dir_extent +
		(unsigned int)G_TO_DIR(gp)->iso_dir_xar) * lbs;
	do {
		lbn = isodir_offset / ISOFS_LBS(fs);
		/*
		 * Since directories cannot be interleaved, datainbuf
		 * and tsize will both equal MAXBSIZE, We are only
		 * interested in bn and offinbuf.
		 */
		bn = cdfs_ibmap(gp, lbn, &datainbuf, &offinbuf);
		datainbuf = MIN(datainbuf, gp->g_size - isodir_offset);
		bp = bread(dev, bn, fs->fs_ibsize, (struct gnode *)NULL);
		if (bp->b_flags & B_ERROR) 
		{
			error = EIO;
			brelse(bp);
			goto out;
		}

		if (isiso)
			tmp_iso_dir = (struct iso_dir *)
				((unsigned int)bp->b_un.b_addr + offinbuf);
		else
			tmp_hsg_dir = (struct hsg_dir *)
				((unsigned int)bp->b_un.b_addr + offinbuf);
		
		wasdot = 0;
		tsize = datainbuf;
		iso_secsize_resid = ISO_SECSIZE;;
		do {
			skip_file = 0;
			switch(fs->fs_format) {
			      case ISO_9660:
				if (tmp_iso_dir->dir_file_flags&ISO_FLG_DIR) {
					bcopy(tmp_iso_dir->dir_extent_lsb, 
					      iso_convert_int.incoming,
					      sizeof(int));
					gen_dir->d_ino = 
						(iso_convert_int.outgoing +
						 tmp_iso_dir->dir_xar) * lbs;
					if (tmp_iso_dir->dir_name[0] == '\0') {
						gen_dir->d_namlen = 1;
						bcopy(".", gen_dir->d_name, 1);
						gen_dir->d_name[1] = '\0';
						wasdot = 1;
					} else if (wasdot) {
						gen_dir->d_namlen = 2;
						bcopy("..", gen_dir->d_name, 
						      2);
						gen_dir->d_name[2] = '\0';
						wasdot = 0;
					} else {
						gen_dir->d_namlen = 
							tmp_iso_dir->dir_namelen;
						bcopy(tmp_iso_dir->dir_name,
						      gen_dir->d_name, 
						      tmp_iso_dir->dir_namelen);
						gen_dir->d_name[tmp_iso_dir->dir_namelen] = '\0';
					}

				} else {
					int length;

					/*
					 * If associated file, or volume seq number
					 * does not match file primary volume descriptor
					 * volume sequence number, skip over file.
					 */
					if ((tmp_iso_dir->dir_file_flags&ISO_FLG_ASSOC) ||
					    tmp_iso_dir->dir_vol_seq_no_lsb !=
					    ISOFS_VOLSEQNUM(fs)) {
						skip_file = 1;
						length = 0;
					}
					/*
					 * Subtract version number if appropriate
					 */
					if ((gp->g_mp)->m_flags & M_NOVERSION) {
						for(length = 0; length < 
						    tmp_iso_dir->dir_namelen;
						    length++)
							if (tmp_iso_dir->dir_name[length] == ';')
								break;
					} else
						length = tmp_iso_dir->dir_namelen;
					gen_dir->d_ino = diskaddr + 
						isodir_offset;
					gen_dir->d_namlen = length;
					bcopy(tmp_iso_dir->dir_name,
					      gen_dir->d_name, length);
					gen_dir->d_name[length] = '\0';
				}
				isodir_reclen = 
					(unsigned int)tmp_iso_dir->dir_len;
				tmp_iso_dir = (struct iso_dir *)
					((unsigned int)tmp_iso_dir + 
					 isodir_reclen);
				break;
			      default: /* HSG */
				if (tmp_hsg_dir->dir_file_flags&ISO_FLG_DIR) {
					bcopy(tmp_hsg_dir->dir_extent_lsb, 
					      iso_convert_int.incoming,
					      sizeof(int));
					gen_dir->d_ino = 
						(iso_convert_int.outgoing +
						 tmp_hsg_dir->dir_xar) * lbs;
					if (tmp_hsg_dir->dir_name[0] == '\0') {
						gen_dir->d_namlen = 1;
						bcopy(".", gen_dir->d_name, 1);
						gen_dir->d_name[1] = '\0';
						wasdot = 1;
					} else if (wasdot) {
						gen_dir->d_namlen = 2;
						bcopy("..", gen_dir->d_name, 
						      2);
						gen_dir->d_name[2] = '\0';
						wasdot = 0;
					} else {
						gen_dir->d_namlen = 
							tmp_hsg_dir->dir_namelen;
						bcopy(tmp_hsg_dir->dir_name,
						      gen_dir->d_name, 
						      tmp_hsg_dir->dir_namelen);
						gen_dir->d_name[tmp_hsg_dir->dir_namelen] = '\0';
					}

				} else {
					int length;
					/*
					 * If associated file, or volume seq number
					 * does not match file primary volume descriptor
					 * volume sequence number, skip over file.
					 */
					if ((tmp_hsg_dir->dir_file_flags&ISO_FLG_ASSOC) ||
					    tmp_hsg_dir->dir_vol_seq_no_lsb !=
					    ISOFS_VOLSEQNUM(fs)) {
						skip_file = 1;
						length = 0;
					}
					/*
					 * Subtract version number if appropriate
					 */
					if ((gp->g_mp)->m_flags & M_NOVERSION) {
						for(length = 0; length < 
						    tmp_hsg_dir->dir_namelen;
						    length++)
							if (tmp_hsg_dir->dir_name[length] == ';')
								break;
					} else
						length = tmp_hsg_dir->dir_namelen;
					gen_dir->d_ino = diskaddr + 
						isodir_offset;
					gen_dir->d_namlen = length;
					bcopy(tmp_hsg_dir->dir_name,
					      gen_dir->d_name, length);
					gen_dir->d_name[length] = '\0';
				}
				isodir_reclen = 
					(unsigned int)tmp_hsg_dir->dir_len;
				tmp_hsg_dir = (struct hsg_dir *)
					((unsigned int)tmp_hsg_dir + 
					 isodir_reclen);
			} /* switch */
			if (skip_file)
				gen_dir->d_reclen = 0;
			else
				gen_dir->d_reclen = DIRSIZ(gen_dir);
			iso_secsize_resid -= gen_dir->d_reclen;

			isodir_offset += isodir_reclen;
			tsize -= isodir_reclen;

			if (tsize <= 0) { 
				gen_dir->d_reclen += iso_secsize_resid;
			} else if (isodir_offset % ISO_SECSIZE == 0) {
				gen_dir->d_reclen += iso_secsize_resid;
			} else if ((isiso && (tmp_iso_dir->dir_len == 0)) ||
				   (!isiso && (tmp_hsg_dir->dir_len == 0))) {
				gen_dir->d_reclen += iso_secsize_resid;
				isodir_reclen = (ISO_SECSIZE -
						  isodir_offset % ISO_SECSIZE);
				tsize -= isodir_reclen;
				isodir_offset += isodir_reclen;
				if (isiso)
					tmp_iso_dir = (struct iso_dir *)
						((unsigned int)tmp_iso_dir +
						 isodir_reclen);
				else
					tmp_hsg_dir = (struct hsg_dir *)
						((unsigned int)tmp_hsg_dir +
						 isodir_reclen);
			}
			if (isodebug) {
				printf("ino %d reclen %d namelen %d\nname %s\n",
					gen_dir->d_ino, gen_dir->d_reclen,
					gen_dir->d_namlen, gen_dir->d_name);
			}
			u.u_error = uiomove(gen_dir, gen_dir->d_reclen, UIO_READ, 
					    uio);
			ubuf_size -= gen_dir->d_reclen;
			if (isodir_offset % ISO_SECSIZE == 0 || tsize <= 0) {
				iso_secsize_resid = ISO_SECSIZE;
				dirblkstotransfer--;
			}
			
		} while (u.u_error == 0 && dirblkstotransfer > 0 && tsize > 0);

		brelse(bp);
	} while (u.u_error == 0 && ubuf_size > 0 && dirblkstotransfer > 0);

	if (error == 0)
		error = u.u_error;
out:
	KM_FREE(gen_dir, KM_TEMP);
	return (error);
}

int
cdfs_rlock(gp, ld, cmd, fp)
	struct gnode *gp;
	struct flock *ld;
	int cmd;
	struct file *fp;
{
	extern int kernel_locking;  /* Sys-V locking: default is kernel */
				    /* kernel == 1;  daemon == 0        */

	/*
	 *	There are two mechanisms by which iso Sys-V locking
	 *	are supported.  The default support is kernel based
	 *	and the optional support is daemon based, and requires
	 *	that nfs is configured and that daemon based locking
	 *	has been enabled (via nfssetup).
	 */

	if (kernel_locking) {	/* kernel based locking */

		switch(cmd) {

			case F_GETLK:

				/* get region lock */

				if (u.u_error = getflck (fp, ld)) {
					break;
				}

				break;

			default:

				if (cmd == F_SETLK) {
					u.u_error = setflck (fp, ld, 0);
				} else { 
					u.u_error = setflck (fp, ld, 1);
				}

				break;

		} /* End switch */


	} else {
	
		/* daemon based locking */

		u.u_error = klm_drlock (gp, ld, cmd, fp->f_cred);
	}

	return (u.u_error); 

}

#ifdef notdef 

cdfs_biodone(bp)
struct buf *bp;
{
	struct iso_strat *stratp;
	int ind;
	extern struct iso_strat iso_strat_begin;

	for(stratp = iso_strat_begin.strat_forw; 
	    stratp != &iso_strat_begin;
	    stratp = stratp->strat_forw) {
		for (ind = 0; ind < stratp->strat_numbufhdr; ind++) {
			if ((stratp->strat_bufhdr[ind] == bp) ||
			    ((stratp->strat_bufhdr[ind])->b_flags & B_DONE &&
			     (stratp->strat_bufhdr[ind])->b_flags & B_BUSY)) {
				bp->b_un.b_addr = 
					stratp->strat_save_baddr[ind];
				if (bp->b_flags & B_ERROR) {
					stratp->strat_bp->b_flags |= B_ERROR;
					stratp->strat_bp->b_error = bp->b_error;
				}
				brelse(bp);
				if (--stratp->strat_outstanding == 0) {
					biodone(stratp->strat_bp);
					remque(stratp);
					KM_FREE(stratp, KM_TEMP);
					return(0);
				}
			}
		}
	}
}

cdfs_strategy(bp)
struct buf *bp;
{
	struct buf *tbuf = NULL;
	int used_buf_headers;
	struct gnode *gp = bp->b_gp;
	struct fs *fs;
	int lbs;
	int ind;
	int offset;
	int total_transfer_size;
	int bn, lbn, length, amount_transfered;
	int saveaffinity;
	struct iso_strat *iso_strat;
	extern struct iso_strat iso_strat_begin;

	if (gp == NULL)
		panic("cdfs_strategy: null gp");

	fs = FS(gp);
	lbs = ISOFS_LBS(fs);

	offset = bp->b_blkno;

	if (G_TO_DIR(gp)->iso_dir_file_unit_size) {
		KM_ALLOC(iso_strat, struct iso_strat *, 
			 sizeof(struct iso_strat), KM_TEMP, KM_NOARG);
		length = ISO_SECSIZE;
	} else
		length = bp->b_bcount;

	total_transfer_size = bp->b_bcount;
	used_buf_headers = amount_transfered = 0;

	while (amount_transfered < total_transfer_size) {
		used_buf_headers++;

		lbn = offset / lbs;

		bn = cdfs_setuptransfer(gp, lbn, &ind, &ind, lbs);

		if (length != bp->b_bcount) {
			tbuf = geteblk(length);
			iso_strat->strat_save_baddr[used_buf_headers-1] = 
				tbuf->b_un.b_addr;
			iso_strat->strat_bufhdr[used_buf_headers - 1] = tbuf;

			tbuf->b_un.b_addr = (caddr_t)
				((unsigned int)bp->b_un.b_addr +
						      amount_transfered);
			tbuf->b_dev = gp->g_dev;
			tbuf->b_blkno = bn;
			tbuf->b_flags |= (bp->b_flags | B_CALL);
			tbuf->b_iodone = cdfs_biodone;
			tbuf->b_proc = bp->b_proc;
			tbuf->b_gp = bp->b_gp;
			tbuf->b_pfcent = bp->b_pfcent;
		} else
			bp->b_blkno = bn;
		/*
		 * Schedule transfer
		 */
		STRATEGY((struct gnode *)NULL, gp->g_dev, 
			 (tbuf != NULL ? tbuf : bp), saveaffinity);
		amount_transfered += length;
		offset += length;
	}
	if (tbuf) {
		iso_strat->strat_outstanding = iso_strat->strat_numbufhdr =
			used_buf_headers;
		iso_strat->strat_bp = bp;
		insque(iso_strat, &iso_strat_begin);
	}
}

#endif notdef
