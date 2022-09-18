#ifndef lint
static char *sccsid = "@(#)ufs_xxx.c	4.2	ULTRIX	11/9/90";
#endif

/************************************************************************
 *									*
 *			Copyright (c) 1986, 89 by			*
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
 * 24 Aug 89 -- Tim Burke
 *	In the ptcmp, ssblk routines when looking for a match of the disk 
 *	driver ioctl routine between the bdev and cdev switch table entries 
 *	take into account that an offset may be required for MSCP disks because
 *	they occupy a range of major numbers.
 *
 * 28 Mar 89 -- Tim Burke
 *	Modified disk unit number calculations to handle the case of MSCP
 *	devices which use more than one major number.
 *
 * 28 Jul 88 -- prs
 *	Added a check for a read-only mount table entry in ssblk().
 * 
 * 25 Aug 87 -- prs
 *	Fixed a bug in ssblk that would update the wrong super block
 *	if it was mounted.
 *
 * 14 Jul 87 -- prs
 *	Fixed ssblk routine to update super blocks of all partitions for
 *	a device that start at block offset zero. Previously routine
 *	would only update the incore super block of an "a" partition.
 *
 * 11 Sep 86 -- koehler
 *	registerized a few things
 *
 ***********************************************************************/


#include "../h/param.h"
#include "../h/systm.h"
#include "../h/gnode.h"
#include "../ufs/fs.h"
#include "../h/mount.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/buf.h"
#include "../h/conf.h"
#include "../h/file.h"
#include "../h/errno.h"
#include "../h/ustat.h"

/*
 *	This routine is called by any of the driver routines that need
 *	to handle the partition table.
 */
 
rsblk(strategy, dev, pt )
	register int (*strategy)();
	dev_t dev;
	register struct pt *pt;
{
	register union sblock {
		struct fs fs;
		char pad[MAXBSIZE];
	} *sblk_addr;				/* ptr to the superblock */
	register int unit = minor(dev) >> 3;		/* device unit */
	register struct buf *bp;		/* buffer ptr for the I/O */

	/*
	 *	Get a buffer that will hold a supberblock
	 */
	bp = geteblk( SBSIZE );

	/*
	 *	Set up the buf struct to read the superblock from
	 *	the "a" partition which is located at SBLOCK
	 */

	bp->b_flags |= B_READ;
	bp->b_bcount = SBSIZE;
	bp->b_dev = makedev( major(dev), unit << 3 );
	bp->b_blkno = SBLOCK;

	/*
	 *	Call the driver's strategy routine to read
	 *	in the superblock, if one exists
	 */

	(*strategy)(bp);		

	/*
	 *	Wait for the I/O to complete
	 */
	biowait(bp);

	/*
	 *	If we have an error just return
	 */

	if ( bp->b_flags & B_ERROR ) {
		brelse(bp);
	 	return (-1);
	}
	/*
	 *	If we have a  valid superblock and a valid partition
	 *	table return success
	 */

	sblk_addr = (union sblock *)bp->b_un.b_fs;

	if ( sblk_addr->fs.fs_magic == FS_MAGIC ) {
		struct pt *scr_pt;

		/*
		 *	Get the pointer to the partition table
		 */
	    	scr_pt = (struct pt*)&sblk_addr->pad[ SBSIZE - sizeof(struct pt ) ];

	    	if ( scr_pt->pt_magic == PT_MAGIC ) {
			*pt = *scr_pt;
			/*
			 *	Indicate that we have a valid partition table
			 */
			pt->pt_valid = PT_VALID;
			brelse( bp );		/* 001 release buffer */
			return(0);
	    	}
	 	else {
			brelse( bp );		/* 001 release buffer */
			return(-1);
		}
	}
	else {
		brelse( bp );		/* 001 release buffer */
		return(-1);
	}
}
/*
 *	The purpose of the routine is to make sure that any  partition
 *	changes will not corrupt the system.
 *
 *	There are cases in which a new partition table could corrupt
 *	the system.  The following are those cases
 *
 *	1. If a partition's starting location in the current partition table
 *	   differ from that of the new partition and there
 *	   is an open file descr. on the current partition an error
 *	   is returned.
 *
 *	2. If a new partition size decreases from that of the current
 *	   partition and there is an open file descr. on the current
 *	   partition an error is returned.
 *
 * NOTE:
 *	This routine should only be called if the device in questioned is
 *	a partitionable device (disk). The reason is that there are 
 *	assumptions about the ioctl address being the same for a partitionable
 *	device be it a raw or block access of the same device.
 *
 */
ptcmp ( dev, cptbl, nptbl )
	dev_t dev;			/* major and minor number */
	register struct pt *cptbl, *nptbl;/* ptr to the current and new
					 * partition tables 
					 */
{
	register int majno = major(dev);	/* major number */
	dev_t bdev, cdev;		/* char and block devices */
	int posserr[8];			/* possible error */
	register int part;		/* which partition */
	int bmajno, bunitno;		/* block's major and minor number */
	int cmajno, cunitno;		/* char's major and minor number */
	register struct file *fp;	/* pointer to incore file descp*/
	register struct mount *mp;	/* ptr to mounted filesystem */
	int i;				/* temp variable */
	int anyerrors = 0;		/* if set then an error case was found */
	register int unit_no;
	/*
	 *	Before we go through the incore file desc. we must
	 *      check to determine if any possible errors exists.
	 */

	for ( part = 0; part <= 'h' - 'a'; part++) {
		/*
		 *	If starting locations are different then
		 *	a possible error condition exist. 
		 */

		if ( cptbl->pt_part[part].pi_blkoff !=
			 nptbl->pt_part[part].pi_blkoff ) {

			posserr[part] = -1;
			anyerrors = -1;
			continue;

		}
		/*
		 *	If current partition size is going to decrease
		 *	a possible error case exists
		 */
		
		if ( cptbl->pt_part[part].pi_nblocks >
			 nptbl->pt_part[part].pi_nblocks ) {

			posserr[part] = -1;
			anyerrors = -1;
			continue;
		}

		
		/*
		 *	Since we can not initialize local aggragates we
		 *	initialize it here.
		 */

		 posserr[part] = 0;
	}

	/*
	 *	If no error cases are found then no additional checking is
	 *	needed.
	 */

	if ( !anyerrors )
		return(0);

	/*
	 *	The major and minor number are not unique because the
	 *	raw and block device number are not the same for the same
	 *	device. We find the the equivalent block number for
	 *	the same device.
	 */
	cdev = dev;
	/*
 	 *	We know that the ioctl address will be the 
 	 *	the same for the raw and block device.
	 *	For MSCP disks adjust the major number to correspond
	 *	to the appropriate unit number.
 	 */
	for ( i = 0; i < nblkdev ; i++ ) {

            	if ( bdevsw[i].d_ioctl == cdevsw[majno].d_ioctl) {
			if (MSCP_C_DEV(cdev)) {
				i += (majno - MSCP_C_MIN);
			}
		 	bdev = makedev( i, minor(dev) );
			break;
		}
	}

	/*
	 *	Do a sanity check to make sure that we did not go over
	 *	then end of the block device table
	 */
	if ( i >= nblkdev )
		panic("ptcmp: No matching ioctl address in block device table");


	/*
	 *	Use local variables for optimization later
	 */
	cmajno = major(cdev);
	cunitno = minor(cdev) >> 3;
	bmajno = major(bdev);
	bunitno = minor(bdev) >> 3;

	 
	/*
	 * Determine unit number.  MSCP type disks can occupy more than one
	 * major number.
	 */
	if (MSCP_C_DEV(cdev)) {
		cunitno += MSCP_C_UNIT_OFFSET(cdev);
	}
	if (MSCP_B_DEV(bdev)) {
		bunitno += MSCP_B_UNIT_OFFSET(bdev);
	}

	/*
	 *	Go through the mounted table to see if we have
	 *	and error
	 */
	for (mp = &mount[0]; mp < &mount[NMOUNT]; mp++) {
	
		/*
		 *	Use the major and unit number to determine
		 *	if we are looking at the right device
		 */
		if ( major(mp->m_dev) == bmajno ) {
			unit_no = (minor(mp->m_dev) >> 3);
			if (MSCP_B_DEV(mp->m_dev))
				unit_no += MSCP_B_UNIT_OFFSET(mp->m_dev);
			if (unit_no == bunitno ) {
				if ( posserr[ minor( mp->m_dev ) & 0x07] ) 
					return(EBUSY);
			}
		}
	}

	/*
	 *	Now go through the kernel open file descriptor table
	 *	looking to see if any of the descriptors are open on
	 *	the device in question.
	 */
	for ( fp = file; fp < fileNFILE; fp++) {
		struct gnode *gp;
		int majorno, unitno;


		/*
		 *	Make sure that we have an active gnode
		 */

		if ( fp->f_type == DTYPE_INODE && fp->f_count && 
		     (gp = (struct gnode *)fp->f_data ) ) {
		
			/*
		 	 *	We only need to check block or char file types
		 	 */
			if ( ( gp->g_mode & GFMT ) == GFCHR ||
				( gp->g_mode & GFMT ) == GFBLK ) {
				/*
			 	 *	Check to see if the file descriptor
			 	 *	in question is the right device by
			 	 *	checking the major and unit
			 	 *	number.
			 	 */
				if ( gp->g_mode & GFMT == GFBLK ) {
					majorno = bmajno;
					unitno = bunitno;
				}
				else {
					majorno = cmajno;
					unitno = cunitno;
				}

				/*
				 *	Everything is now ready to check to
				 *	see if we have an error
				 */
				if ( major(gp->g_rdev) == majorno ) {
					unit_no = (minor(gp->g_rdev) >> 3);
					if (majorno == bmajno) { /* Block */
					    if (MSCP_B_DEV(gp->g_rdev))
						unit_no += MSCP_B_UNIT_OFFSET(gp->g_rdev);
					}
					else {			 /* Character */
					    if (MSCP_C_DEV(gp->g_rdev))
						unit_no += MSCP_C_UNIT_OFFSET(gp->g_rdev);
					}
					if (unit_no == unitno ) {
						if ( posserr[ minor( gp->g_rdev ) & 0x07] ) 
							return(EBUSY);
					}
				}
			}
		}
	}

	return(0);
}
/*
 *	This routine is used to write out the partition table if the "a"
 *	paritition of the device is mounted.  The reason for this if
 *	the partition tables are changes via the "chpt" on the mounted
 *	"a" paritition the command has no way of changing the incore 
 *	superblock of the "a" paritition.  This routine enables us to
 *	modified the superblock.
 */
ssblk ( dev, nptbl )
	dev_t dev;
	struct pt *nptbl;			/* New partition table */
{
	register int i;
	register int majno = major(dev);	/* Major cdevsw number */
	register struct pt *pt;			/* ptr to the part tbl */
	int bdev;			/* block device number */
	register struct mount *mp;		/* ptr to the mount tbl */
	register union sblock {
		struct fs fs;
		char pad[MAXBSIZE];
	} *sblk_addr;
	register struct buf *bp;			/* buf ptr */
	int blk;

	/*
	 *	We assume that the dev is of a character device.  We also
	 *	assume that the ioctl for both the raw and block device 
	 *	are the same.
	 */
	for ( i = 0; i < nblkdev ; i++ ) {

            	if ( bdevsw[i].d_ioctl == cdevsw[majno].d_ioctl) {
			/*
			 *	Make a bdev with just a unit number
			 */
			if (MSCP_C_DEV(dev)) {
				i += (majno - MSCP_C_MIN);
			}
		 	bdev = makedev( i, minor(dev) & ~0x07 );
			break;
		}
	}

	for (mp = &mount[0]; mp < &mount[NMOUNT]; mp++) {
	
		/*
		 *	See if any partition of the disk is mounted
		 */

		if ( (mp->m_dev & ~0x07) == bdev && mp->m_bufp != NULL ) {

			/*
			 * 	If the mounted partition does not start 
			 *	at sector zero then continue.
			 */
			if (nptbl->pt_part[minor(mp->m_dev)&0x7].pi_blkoff != 0)
				continue;
			/*
			 * Don't even attempt read-only mounted file systems,
			 * because they are not synced out.
			 */
			if (ISREADONLY(mp))
				continue;

			/*
			 *	We now know that we have a superblock
			 *	that must possible need updating. If
			 *	updating is required then get the superblock
			 *	and copy the user specified data
			 */
			bp = mp->m_bufp;

			sblk_addr = (union sblock *)bp->b_un.b_fs;
			pt = ( struct pt *)&sblk_addr->pad[ SBSIZE - sizeof(struct pt ) ];

			/*
			 *	If the new partition table will overwrite
			 *	the rotational table then remove it
			 *		Note
			 *	I assume that the user will be warned
			 *	of this case by a user level command.
			 */
			blk = sblk_addr->fs.fs_spc * sblk_addr->fs.fs_cpc /
				 NSPF(bp->b_un.b_fs);
			for (i = 0; i < blk; i += sblk_addr->fs.fs_frag)
				/* void */;
			if ((struct pt *)(&sblk_addr->fs.fs_rotbl[
			(i - sblk_addr->fs.fs_frag) /sblk_addr->fs.fs_frag])
				 >= pt) {
				sblk_addr->fs.fs_cpc = 0;
				mp->m_flags |= M_MOD;
			}

			/*
			 *	If the superblock size is less than the
			 *	max. superblock size we will not overwrite
			 *	what was put there, so just break out
			 */

			if( sblk_addr->fs.fs_sbsize < SBSIZE )
				break;
			/*
			 *	Copy the user specified table into the
			 *	superblock and indicate that we have
			 *	updated the superblock
			 */
			*pt = *nptbl;
			mp->m_flags |= M_MOD;
		}
	}
}
