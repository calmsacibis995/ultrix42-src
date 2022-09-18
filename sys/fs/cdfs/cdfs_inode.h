/* @(#)cdfs_inode.h	4.1	(ULTRIX)	11/9/90 */

/************************************************************************
 *									*
 *			Copyright (c) 1985 by				*
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

struct iso_idir
{
	unsigned int dir_extent;
	unsigned int dir_dat_len;
	unsigned char dir_len;
	unsigned char dir_xar;
	unsigned char dir_dt[7];
	unsigned char dir_file_flags;
	unsigned char dir_file_unit_size;
	unsigned char dir_inger_gap_size;
	unsigned short dir_vol_seq_no;
};

/*
 *	The ISOFS file system specific portion of a gnode.
 */

struct	iso_inode {
	union {
		char	trash[128];
		struct {
			struct gnode_common iso_gc;
			struct iso_idir	iso_dirs;
			int iso_gennum;
		} iso_gcom;
	} iso_iu;
};

#define iso_ic			iso_iu.iso_gcom
#define	iso_mode		iso_ic.iso_gc.gc_mode
#define	iso_nlink		iso_ic.iso_gc.gc_nlink
#define	iso_uid			iso_ic.iso_gc.gc_uid
#define	iso_gid			iso_ic.iso_gc.gc_gid

#define iso_dirp		iso_ic.iso_dirs
#define iso_dir_len		iso_ic.iso_dirs.dir_len
#define iso_dir_xar		iso_ic.iso_dirs.dir_xar
#define iso_dir_extent		iso_ic.iso_dirs.dir_extent
#define iso_dir_dat_len		iso_ic.iso_dirs.dir_dat_len
#define iso_dir_dt		iso_ic.iso_dirs.dir_dt
#define iso_dir_file_flags	iso_ic.iso_dirs.dir_file_flags
#define iso_dir_file_unit_size	iso_ic.iso_dirs.dir_file_unit_size
#define iso_dir_inger_gap_size	iso_ic.iso_dirs.dir_inger_gap_size
#define iso_dir_vol_seq_no	iso_ic.iso_dirs.dir_vol_seq_no
#define iso_gennum		iso_ic.iso_gennum

#define	G_TO_DIR(x)	((struct iso_inode *)(x)->g_in.pad)
