#ifndef lint
static	char	*sccsid = "@(#)cdfs_bmap.c	4.1	(ULTRIX)	11/9/90";
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
 *									*
 *			Modification History				*
 *	fs/cdfs/cdfs_bmap.c						*
 *									*
 * 22-Oct-90 -- prs							*
 *	Initial creation.						*
 *									*
 ************************************************************************/

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/conf.h"
#include "../h/gnode_common.h"
#include "../h/gnode.h"
#include "../h/mount.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/buf.h"
#include "../h/proc.h"
#include "../h/kernel.h"
#include "../h/fs_types.h"
#include "../fs/cdfs/cdfs_fs.h"
#include "../fs/cdfs/cdfs_inode.h"

extern int isodebug;

/*
 * cdfs_bmap() is the ISO 9660 subordinate interface to bmap().
 * Currently, the only callers are vinifod() and nfs_bread().
 * The caller expects blocks to be m_bsize (volume block size
 * according to data preparer) lengths and increments.
 */
cdfs_bmap(gp, lbn, rwflg, size, sync)
register struct gnode *gp;
daddr_t lbn;
int rwflg;
int size;
int *sync;
{
	int datainbuf = 0;
	int offsetinbuf = 0;

	lbn = (lbn * (gp->g_mp)->m_bsize) / ISOFS_LBS(FS(gp));
	return(cdfs_setuptransfer(gp, lbn, &datainbuf, &offsetinbuf,
				 (gp->g_mp)->m_bsize, 0));
}

/*
 * cdfs_ibmap() is the internal (cdfs) block map routine. Since the
 * cdfs code makes the most efficient use of memory through the
 * buffer cache, all read requests are MAXBSIZE lengths and increments.
 * Since an ISO 9660 volume's logical block size will always be significantly
 * less than MAXBSIZE, two additional parameters are needed. These are:
 * datainbuf (the total number of data bytes within buffer); and offsetinbuf
 * (offset within buffer, datainbuf bytes start at).
 */

cdfs_ibmap(gp, lbn, datainbuf, offsetinbuf)
register struct gnode *gp;
daddr_t lbn;
int *datainbuf;
int *offsetinbuf;
{

	*datainbuf = *offsetinbuf = 0;
	return(cdfs_setuptransfer(gp, lbn, datainbuf, offsetinbuf, 
				 FS(gp)->fs_ibsize, 1));
}

cdfs_setuptransfer(gp, lbn, datainbuf, offsetinbuf, bsize, align_buf)
	register struct gnode *gp;
	int lbn;		/* Logical block number uio_offset refers to */
	int *datainbuf;		/* Number of data bytes in buffer */
	int *offsetinbuf;	/* Offset within buffer data resides */
	int bsize;		/* block size */
	int align_buf;
{
	struct fs *fs;
	int offset_lbn;
	int offset;
	int tmp;
	int file_unit_size = 0;
	int lbs;
	int lbs_bn;
	int bn;

	fs = (struct fs *)FS(gp);
	lbs = ISOFS_LBS(fs);

	if (G_TO_DIR(gp)->iso_dir_file_unit_size)
		file_unit_size = G_TO_DIR(gp)->iso_dir_file_unit_size * lbs;
	/*
	 * Set offset to byte offset within file, lbn refers to.
	 */
	offset = lbn * lbs;
	/*
	 * If an XAR exists, skip over it by incrementing offset. Note,
	 * iso_dir_xar is the number of logical blocks XAR is recorded
	 * over.
	 */
	if (G_TO_DIR(gp)->iso_dir_xar)
		offset += (G_TO_DIR(gp)->iso_dir_xar * lbs);

	/*
	 * For initialization purposes.
	 */
	*datainbuf = bsize;
	/*
	 * Calculate logical block number offset resides at.
	 */
	lbs_bn = offset / lbs;
	/*
	 * If recorded in interleave mode, calculate logical block number
	 * offseted by file unit and gap size.
	 */
	if (file_unit_size) {
		/*
		 * tmp is set to which file unit offset exists in.
		 */
		tmp = offset / file_unit_size;
		/*
		 * offset_lbn is set to the logical block number offset
		 * resides at, relative to beginning of file.
		 */
		offset_lbn = 
			(lbs_bn % 
			 (int)G_TO_DIR(gp)->iso_dir_file_unit_size)
			+ (tmp * ((int)G_TO_DIR(gp)->iso_dir_file_unit_size +
				  (int)G_TO_DIR(gp)->iso_dir_inger_gap_size));
		/*
		 * file_unit_size has to be in 2K increments. We can calculate
		 * datainbuf by subtracting the modula of lbs_bn and 
		 * iso_dir_file_unit_size, taking note that both values are
		 * in logical block size units. This value can be coverted
		 * into bytes and subtracted from the size of a file unit,
		 * resulting in the maximum number of bytes remaining in the
		 * file unit.
		 */
		*datainbuf = file_unit_size - 
			((lbs_bn % (int)G_TO_DIR(gp)->iso_dir_file_unit_size)
			 * lbs);
		/*
		 * Since datainbuf is the maximum number of bytes remaining in
		 * the file unit, set value to be amount which can fit in a
		 * buffer.
		 */
		*datainbuf = MIN(*datainbuf, bsize);
	} else
		/*
		 * If the file was not recorded in interleave mode,
		 * offset_lbn is set to the value of lbs_bn.
		 */
		offset_lbn = (int)lbs_bn;

	/*
	 * offset_lbn and iso_dir_extent are in logical block size
	 * units; increment offset_lbn by iso_dir_extent so value
	 * is relative to beginning of volume.
	 */
	offset_lbn += G_TO_DIR(gp)->iso_dir_extent;
	/*
	 * offset_lbn is the logical block number where transfer should
	 * start.
	 *
	 * Now calculate disk block number to begin transfer. Buffers will
	 * begin on bsize increments, and obviously be bsize in length.
	 * We may have to recalculate offsetinbuf and datainbuf to properly 
	 * offset uiomove.
	 */
	bn = (offset_lbn * lbs) / DEV_BSIZE;
	if (align_buf && (bn * DEV_BSIZE) % bsize) {
		unsigned int data_begin;
		unsigned int data_end;

		data_begin = bn * DEV_BSIZE;
		data_end = data_begin + *datainbuf;

		*offsetinbuf += ((bn * DEV_BSIZE) % bsize);
		bn = ((bn * DEV_BSIZE) - ((bn * DEV_BSIZE) % bsize)) 
			/ DEV_BSIZE;

		if ((bn * DEV_BSIZE) + bsize < data_begin) {
			printf("cdfs_setuptransfer: buffer remap messup\n");
			return(0);
		}
		if ((bn * DEV_BSIZE) + bsize < data_end)
			*datainbuf -= (data_end -
				       ((bn * DEV_BSIZE) + bsize));
	} else
		*offsetinbuf = 0;

	if (isodebug) {
		printf("cdfs_setuptransfer: gp 0x%x lbn %d bn %d bsize %d offsetinbuf %d datainbuf %d\n",
		       gp, lbn, bn, bsize, *offsetinbuf, *datainbuf);
	}
	return(bn);
}
