#ifndef lint
static	char	*sccsid = "@(#)cdfs_namei.c	4.1	(ULTRIX)	11/9/90";
#endif lint

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
/************************************************************************
 *			Modification History
 *	fs/cdfs/cdfs_namei.c
 *
 *  9-Nov-90 -- prs
 *	Initial creation.
 *
 ***********************************************************************/

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/gnode_common.h"
#include "../cdfs/cdfs_fs.h"
#include "../cdfs/cdfs_inode.h"
#include "../cdfs/cdfs_mount.h"
#include "../h/gnode.h"
#include "../h/mount.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/buf.h"
#include "../h/conf.h"
#include "../h/uio.h"
#include "../h/kernel.h"
#include "../h/fs_types.h"

extern isodebug;

/*
 * The routine cdfs_namei performs pathname lookup in ISO9660/HSG
 * formatted filesystems.
 *
 * Unlike the ufs namei, we leave a pointer to the last pathname component
 * in ndp->ni_cp.
 */ 

struct gnode *
cdfs_namei(ndp)
	register struct nameidata *ndp;
{
	struct gnode *gpp, *gp;
	register char *slash, *ptr, *cp, *ncp;
	int lockparent, flag, i, nomount;
	char name[ISO_MAXNAMLEN+1];
	struct uio _auio;
	register struct uio *auio = &_auio;
	struct iovec _aiov;
	register struct iovec *aiov = &_aiov;
	struct mount *mpp;
	int name_len;
	
	if (isodebug)
		printf("Entering cdfs_namei to look up %s\n", ndp->ni_cp);

	flag = ndp->ni_nameiop & (LOOKUP | CREATE | DELETE);
	lockparent = ndp->ni_nameiop & LOCKPARENT;
	nomount = ndp->ni_nameiop & NOMOUNT;
	gp = ndp->ni_pdir;
	cdfs_gunlock(gp);

	if (gp == NULL)
		panic("cdfs_namei: no parent");
	cp = ndp->ni_cp;

	while (*cp == '/')
		cp++;

	while(*cp) {

		/*
		 * Find the next pathname component, move it into name[].
		 */

		for (i = 0; cp[i] != 0 && cp[i] != '/'; i++) {
			if (i == ISO_MAXNAMLEN)
				u.u_error = ENAMETOOLONG;
			if (u.u_error) {
				grele(gp);
				return(NULL);
			}
			name[i] = cp[i];
		}

		name_len = i + 1;
		name[i] = '\0';
		ncp = cp + i;

		/*
		 * If we're at the root of a filesystem and the next
		 * component is ".." then we just bounce back to caller.
		 */

		if ((name[0] == '.') && (name[1] == '.') && !name[2]
		&& ((gp == gp->g_mp->m_rootgp) || (gp == u.u_rdir))) {
			grele(gp);
			/*
 			 * If we are not at the system root directory, nor
			 * at the processes root directory, set gp to the
			 * mounted on gp.
			 */
 			if ((gp->g_mp->m_gnodp != (struct gnode *) NULL) &&
			     gp != u.u_rdir)
 				gp = gp->g_mp->m_gnodp;
			gref(gp);	/* bump ref count */
			gfs_lock(gp);

			gp->g_flag |= GINCOMPLETE;
			ndp->ni_pdir = gp;
			/*
			 * If we're at the system root directory, or the
			 * processes root directory, strip off ".."
			 */
			if (gp == gp->g_mp->m_rootgp || gp == u.u_rdir)
				ndp->ni_cp = ncp;
			else
				ndp->ni_cp = cp;
			return(gp);
		}
		
		/*
		 * Now look up the current pathname component.
		 */

		if (isodebug)
			printf("cdfs_namei: about to do an cdfs_lookup for \"%s\"\n",
			name);

		u.u_error = cdfs_lookup(gp, name, &gpp, u.u_cred);

		if (isodebug) {
			if (u.u_error)
				printf ("cdfs_namei: cdfs_lookup error return %d\n", u.u_error);
			if (gpp)
				printf("cdfs_namei: cdfs_lookup returned #%d\n", (gpp)->g_number);
			else 
				printf("cdfs_namei: cdfs_lookup returned null\n");
		}

		/*
		 * Take appropriate action if the lookup fails.
		 */

		if (u.u_error) {
			grele(gp);
			if (u.u_error == ENOENT && (flag & (CREATE | DELETE)))
				u.u_error = EROFS;
			return(NULL);
		}

		if (gpp->g_mp->m_fstype != GT_CDFS)
			panic("cdfs_namei: gp not type CDFS");

		/*
		 * The lookup has succeeded.  If we've hit a mount point
		 * traverse it if it's type ISO and nomount is not set, 
		 * else bounce back to caller.
		 *
		 */

		if (gpp->g_flag & GMOUNT && !nomount) {
			mpp = gpp->g_mpp;
			if (mpp->m_fstype != GT_CDFS) {
				grele(gp);
				grele(gpp);
				gp = mpp->m_rootgp;
				gref(gp);	/* bump ref count */
				gfs_lock(gp);

				ndp->ni_cp = ncp;
				ndp->ni_pdir = gp;
				gp->g_flag |= GINCOMPLETE;
				return(gp);
			}
			else if (!nomount) {
				grele(gp);
				gp = gpp;
				gpp = mpp->m_rootgp;
				gref(gpp);
			}
		}
		/*
		 * Since gpp is an ISO gnode, enter it into dnlc.
		 */
		dnlc_enter(gp, name, gpp, u.u_cred);

		/*
		 * The lookup has succeeded and we haven't hit a mount
		 * point.  Handle the case where this is the last component
		 * of the pathname.
		 */

		while (*ncp == '/')
			ncp++;
		if (*ncp == '\0') {
			if (ndp->ni_nameiop & NOCACHE)
				dnlc_purge_vp(gpp);
			if (flag & (CREATE|DELETE)) {
				grele(gp);
				gpp->g_flag &= ~GINCOMPLETE;
				grele(gpp);
				u.u_error = EROFS;
				return(NULL);
			}
			ndp->ni_cp = cp;
			if (lockparent) {
				if (gp != gpp)
					gfs_lock(gp);
				ndp->ni_pdir = gp;
			}
			else {
				grele(gp);
			}
			/*
			 * Copy in the last component name in
			 * ni_dent for accounting.
			 */
			bcopy(ndp->ni_cp, ndp->ni_dent.d_name, name_len);
			ndp->ni_dent.d_namlen = name_len;

			cdfs_glock(gpp);
			gpp->g_flag &= ~GINCOMPLETE;
			return(gpp);
		}

		/*
		 * The lookup has succeeded, but this is not the last
		 * component of the pathname.
		 */

		grele(gp);
		gp = gpp;
		cp = ncp;
	}

	/*
	 * "Temporary" hack for handling null pathnames, which denote the
	 * starting directory by convention.
	 * Note: we can also get here if the pathname resolves
	 * to an ISO mount point.
	 */

	cdfs_glock(gp);
	gp->g_flag &= ~GINCOMPLETE;
	return(gp);
}

int isodebug1 = 1;

int
cdfs_lookup(dgp, nm, gpp, cred)
	struct gnode *dgp;
	char *nm;
	struct gnode **gpp;
	struct ucred *cred;
{
	int gnumber;
	int lbs;
	struct mount *mp = dgp->g_mp;

	if ((dgp->g_mode & GFMT) != GFDIR) {
		u.u_error = EISDIR;
		*gpp = 0;
		return(u.u_error);
	}

	*gpp = (struct gnode *) dnlc_lookup(dgp, nm, cred);
	if (*gpp) {
		return (0);
	}

	/*
	 * cdfs_getnumber will search dgp directory entries for nm.
	 * Upon success, the disk address for the corresponding
	 * directory entry for nm will be returned.
	 */
	gnumber = cdfs_getnumber(dgp, nm, cred);
	if (gnumber == 0) {
		u.u_error = ENOENT;
		*gpp = 0;
		return(u.u_error);
	}

	if (((*gpp = gget(mp, gnumber, 1, NULL)) == NULL) || u.u_error) {
	        if (!u.u_error)
			u.u_error = EIO; /* XXX */
		*gpp = 0;
		return(u.u_error);
	}

	/*
	 * If a file was recorded in interleave mode, verify the
	 * file unit size and interleave gap size are both multiples
	 * of 2k.
	 */
	lbs = ISOFS_LBS(FS(*gpp));
	if (G_TO_DIR(*gpp)->iso_dir_file_unit_size) {
		if ((G_TO_DIR(*gpp)->iso_dir_file_unit_size * lbs) % 
		    ISO_SECSIZE) {
			printf("cdfs_namei: gnode number %d file unit size %d not 2k multiple\n", 
			       gnumber, 
			       (G_TO_DIR(*gpp)->iso_dir_file_unit_size * lbs));
			gput(*gpp);
			u.u_error = EIO;
			*gpp = 0;
			return(u.u_error);
		}
		if ((G_TO_DIR(*gpp)->iso_dir_inger_gap_size * lbs) % 
		    ISO_SECSIZE) {
			printf("cdfs_namei: gnode number %d file gap size %d not 2k multiple\n", 
			       gnumber, 
			       (G_TO_DIR(*gpp)->iso_dir_inger_gap_size * lbs));
			gput(*gpp);
			u.u_error = EIO;
			*gpp = 0;
			return(u.u_error);
		}
		/*
		 * If a file unit contains an XAR, and it was recorded in 
		 * interleave mode, verify size of XAR is equal to the file 
		 * unit size.
		 */
		if (G_TO_DIR(*gpp)->iso_dir_xar) {
			if (G_TO_DIR(*gpp)->iso_dir_xar != 
			    G_TO_DIR(*gpp)->iso_dir_file_unit_size) {
				printf("cdfs_setuptransfer:  gnode number %d xar size %d != file unit size\n", 
				       gnumber, 
				       G_TO_DIR(*gpp)->iso_dir_xar,
				       G_TO_DIR(*gpp)->iso_dir_file_unit_size);
				gput(*gpp);
				u.u_error = EIO;
				*gpp = 0;
				return(u.u_error);
			}
		}
		if (isodebug1 && G_TO_DIR(*gpp)->iso_dir_file_unit_size)
			printf("cdfs_namei: %s recorded in interleave mode !!!\n", nm);
	} /* file unit size */

	cdfs_gunlock(*gpp);
	return (0);
}


int
cdfs_getnumber(dgp, nm, cred)
	struct gnode *dgp;
	char *nm;
	struct ucred *cred;
{
	int isdotdot = 0;
	int wasdot;
	struct buf *bp;
	struct fs *fs;
	int foundit;
	int gnumber;
	struct iso_dir *tmp_iso_dir;
	struct hsg_dir *tmp_hsg_dir;
	struct iso_idir idir;
	int resid;
	int tsize;
	int lbn, bn;
	int datainbuf, offinbuf;
	unsigned int diskaddr;
	unsigned int isodir_offset;
	unsigned int isodir_reclen;
	union {
		unsigned char incoming[4];
		unsigned int  outgoing;
	} iso_convert_int;
	int rablock, rasize;
	int isiso;
	int length;
	int skip_file;

	fs = FS(dgp);

	if (!strcmp(nm, "."))
		return(dgp->g_number);

	if (fs->fs_format == ISO_9660)
		isiso = 1;
	else
		isiso = 0;
	if (!strcmp(nm, ".."))
		isdotdot = 1;

	resid = dgp->g_size;

	isodir_offset = 0;
	diskaddr = ((unsigned int)G_TO_DIR(dgp)->iso_dir_extent +
	       (unsigned int)G_TO_DIR(dgp)->iso_dir_xar) * 
		       (unsigned int)ISOFS_LBS(fs);
	tsize = 0;
	while (resid && !u.u_error) {
		foundit = 0;
		lbn = isodir_offset / ISOFS_LBS(FS(dgp));
		/*
		 * Since directories cannot be interleaved, datainbuf
		 * and tsize will both equal MAXBSIZE, We are only
		 * interested in bn and offinbuf.
		 */
		bn = cdfs_ibmap(dgp, lbn, &datainbuf, &offinbuf);
		datainbuf = MIN(datainbuf, dgp->g_size - isodir_offset);
		bp = bread(dgp->g_dev, bn, FS(dgp)->fs_ibsize, 
			   (struct gnode *)NULL);
		if (bp->b_flags & B_ERROR) {
			brelse(bp);
			return(0);
		}
		tsize = datainbuf;
		wasdot = 0;
		if (isiso)
			tmp_iso_dir = (struct iso_dir *)
				((unsigned int)bp->b_un.b_addr + offinbuf);
		else
			tmp_hsg_dir = (struct hsg_dir *)
				((unsigned int)bp->b_un.b_addr + offinbuf);
		do {
			skip_file = 0;
			switch (fs->fs_format) {
			      case ISO_9660:
				length = tmp_iso_dir->dir_namelen;
				if (tmp_iso_dir->dir_file_flags&ISO_FLG_DIR) {
					if (tmp_iso_dir->dir_namelen == 1) {
						if (tmp_iso_dir->dir_name[0] == '\0') {
							wasdot = 1;
						} else if (wasdot) {
							wasdot = 0;
							if (isdotdot) {
								foundit = 1;
								goto found;
							}
						}
					}
						
				} else {
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
					 * Subtract version number is appropriate
					 */
					if ((dgp->g_mp)->m_flags & M_NOVERSION) {
						for(length = 0; length < 
						    tmp_iso_dir->dir_namelen;
						    length++)
							if (tmp_iso_dir->dir_name[length] == ';')
								break;
					}

				}
				if(skip_file == 0 && strlen(nm) == length &&
				   !strncmp(nm, tmp_iso_dir->dir_name,
					    length)) {
					foundit = 1;
					goto found;
				}
				isodir_reclen = tmp_iso_dir->dir_len;
				tmp_iso_dir = (struct iso_dir *)
					((unsigned int)tmp_iso_dir + 
						  isodir_reclen);
				break;
			      default: /* HSG */
				length = tmp_hsg_dir->dir_namelen;
				if (tmp_hsg_dir->dir_file_flags&ISO_FLG_DIR) {
					if (tmp_hsg_dir->dir_namelen == 1) {
						if (tmp_hsg_dir->dir_name[0] == '\0') {
							wasdot = 1;
						} else if (wasdot) {
							wasdot = 0;
							if (isdotdot) {
								foundit = 1;
								goto found;
							}
						}
					}
						
				} else {
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
					 * Subtract version number is appropriate
					 */
					if ((dgp->g_mp)->m_flags & M_NOVERSION) {
						for(length = 0; length < 
						    tmp_hsg_dir->dir_namelen;
						    length++)
							if (tmp_hsg_dir->dir_name[length] == ';')
								break;
					}

				}
				if(skip_file == 0 && strlen(nm) == length &&
				   !strncmp(nm, tmp_hsg_dir->dir_name, 
					    length)) { 
					foundit = 1;
					goto found;
				}
				isodir_reclen = tmp_hsg_dir->dir_len;
				tmp_hsg_dir = (struct hsg_dir *)
					((unsigned int)tmp_hsg_dir + 
						  isodir_reclen);
			} /* switch */

			isodir_offset += isodir_reclen;
			tsize -= isodir_reclen;
			switch (fs->fs_format) {
			      case ISO_9660:
				if ((tsize > 0) && 
				    (tmp_iso_dir->dir_len == 0)) {
					isodir_reclen = ISO_SECSIZE -
						(isodir_offset % ISO_SECSIZE);
					tsize -= isodir_reclen;
					isodir_offset += isodir_reclen;
					tmp_iso_dir = (struct iso_dir *)
						((unsigned int)tmp_iso_dir +
						 isodir_reclen);
				}
				break;
			      default:
				if ((tsize > 0) && 
				    (tmp_hsg_dir->dir_len == 0)) {
					isodir_reclen = ISO_SECSIZE -
						(isodir_offset % ISO_SECSIZE);
					tsize -= isodir_reclen;
					isodir_offset += isodir_reclen;
					tmp_hsg_dir = (struct hsg_dir *)
						((unsigned int)tmp_hsg_dir +
						 isodir_reclen);
				}
			} /* switch */
			if (isodir_offset % ISO_SECSIZE == 0)
				resid -= ISO_SECSIZE;
		} while (tsize > 0);
found:
		if (foundit) {
			/*
			 * Found the correct directory entry.
			 * Set gnumber to disk address of directory
			 * record.
			 */
			switch (fs->fs_format) {
			      case ISO_9660:
				if (tmp_iso_dir->dir_file_flags&ISO_FLG_DIR) {
					bcopy(tmp_iso_dir->dir_extent_lsb, 
					      iso_convert_int.incoming, 
					      sizeof(int));
					gnumber = (iso_convert_int.outgoing +
						   tmp_iso_dir->dir_xar) *
							   (int)ISOFS_LBS(fs);
				} else
					gnumber = diskaddr + isodir_offset;
				break;
			      default:
				if (tmp_hsg_dir->dir_file_flags&ISO_FLG_DIR) {
					bcopy(tmp_hsg_dir->dir_extent_lsb, 
					      iso_convert_int.incoming, 
					      sizeof(int));
					gnumber = (iso_convert_int.outgoing +
						   tmp_hsg_dir->dir_xar) *
							   (int)ISOFS_LBS(fs);
				} else
					gnumber = diskaddr + isodir_offset;
			} /* switch */
			/*
			 * Release buffer.
			 */
			brelse(bp);
			if (isodebug)
				printf("cdfs_getnumber: entry %s number %d\n",
				       nm, gnumber);
			return(gnumber);
		}
		brelse(bp);
	}
	if (isodebug)
		printf("cdfs_getnumber: entry %s not found\n", nm);
	return(0);
}
