/* @(#)cdfs_fs.h	4.1	(ULTRIX)	11/9/90 */

/************************************************************************
 *									*
 *			Copyright (c) 1984, 1986 by			*
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
 *	fs/cdfs/cdfs_fs.h						*
 *									*
 * 22-Oct-90 -- prs							*
 *	Initial creation.						*
 *									*
 ************************************************************************/

/*
 * The first boot and primary volume descriptor blocks are given in 
 * absolute disk addresses.
 */

#define BBSIZE		32768
#define PVD_SIZE	2048
#define ISO_SECSIZE	2048
#define	BBLOCK		((daddr_t)(0))
#define	PVD_BLOCK	((daddr_t)(BBLOCK + BBSIZE / DEV_BSIZE))
#define ISO_MAXNAMLEN	34
#define ISO_DTLEN 17

/*
 * ISO 9660 and HSG on-disk structures.
 */

struct iso_dir
{
	unsigned char dir_len;
	unsigned char dir_xar;
	unsigned char dir_extent_lsb[4];
	unsigned char dir_extent_msb[4];
	unsigned char dir_dat_len_lsb[4];
	unsigned char dir_dat_len_msb[4];
	unsigned char dir_dt[7];
	unsigned char dir_file_flags;
#define ISO_FLG_EXIST 1
#define ISO_FLG_DIR 2
#define ISO_FLG_ASSOC 4
#define ISO_FLG_RECFMT 8
#define ISO_FLG_PROTECT 16
#define ISO_FLG_RESRV1 32
#define ISO_FLG_RESRV2 64
#define ISO_FLG_NOTLAST 128
	unsigned char dir_file_unit_size;
	unsigned char dir_inger_gap_size;
	unsigned short dir_vol_seq_no_lsb;
	unsigned short dir_vol_seq_no_msb;
	unsigned char dir_namelen;
	unsigned char dir_name[1];	/* Up to ISO_MAXNAMLEN */
};

struct hsg_dir
{
	unsigned char dir_len;
	unsigned char dir_xar;
	unsigned char dir_extent_lsb[4];
	unsigned char dir_extent_msb[4];
	unsigned char dir_dat_len_lsb[4];
	unsigned char dir_dat_len_msb[4];
	unsigned char dir_dt[6];
	unsigned char dir_file_flags;
	unsigned char filler;
	unsigned char dir_file_unit_size;
	unsigned char dir_inger_gap_size;
	unsigned short dir_vol_seq_no_lsb;
	unsigned short dir_vol_seq_no_msb;
	unsigned char dir_namelen;
	unsigned char dir_name[1];	/* Up to ISO_MAXNAMLEN */
};

struct iso_fs
{
        unsigned char   iso_vol_desc_type;
#define PRIMARY_VOL_DESC 1
#define SUPPLEMENTARY_VOL_DESC 2
#define TERMINATING_VOL_DESC 255
        char            iso_std_id[5];
        unsigned char   iso_vol_desc_vers;
        char            not_used_1;
        char            iso_system_id[32];
        char            iso_vol_id[32];
        char            not_used_2[8];
        unsigned int    iso_vol_space_size_lsb;
        unsigned int    iso_vol_space_size_msb;
        char            not_used_3[32];
        unsigned short  iso_vol_set_size_lsb;
        unsigned short  iso_vol_set_size_msb;
        unsigned short  iso_vol_seq_num_lsb;
        unsigned short  iso_vol_seq_num_msb;
        unsigned short  iso_logical_block_size_lsb;
	unsigned short  iso_logical_block_size_msb;
        unsigned int    iso_path_tbl_size_lsb;
        unsigned int    iso_path_tbl_size_msb;
        unsigned int    iso_L_path_tbl;
        unsigned int    iso_opt_L_path_tbl;
        unsigned int    iso_M_path_tbl;
        unsigned int    iso_opt_M_path_tbl;
        struct iso_dir  iso_root_dir;
        char            iso_vol_set_id[128];
        char            iso_pub_id[128];
	char		iso_preparer_id[128];
        char            iso_application_id[128];
	char		iso_copyright_id[37];
	char		iso_abstract_id[37];
	char		iso_bibliographic_id[37];
	char		iso_vol_dtcre[ISO_DTLEN];
	char		iso_vol_dtmod[ISO_DTLEN];
	char		iso_vol_dtexp[ISO_DTLEN];
	char		iso_vol_dteff[ISO_DTLEN];
	char		iso_file_struct_version;
	char		not_used_4;
	char		iso_application_use[512];
	char		not_used_5[653];
}; /* 2048 bytes long */

struct hsg_fs
{
	unsigned int	iso_vol_desc_lbn_lsb;
	unsigned int	iso_vol_desc_lbn_msb;
        unsigned char   iso_vol_desc_type;
        char            iso_std_id[5];
        unsigned char   iso_vol_desc_vers;
        char            not_used_1;
        char            iso_system_id[32];
        char            iso_vol_id[32];
        char            not_used_2[8];
        unsigned int    iso_vol_space_size_lsb;
        unsigned int    iso_vol_space_size_msb;
        char            not_used_3[32];
        unsigned short  iso_vol_set_size_lsb;
        unsigned short  iso_vol_set_size_msb;
        unsigned short  iso_vol_seq_num_lsb;
        unsigned short  iso_vol_seq_num_msb;
        unsigned short  iso_logical_block_size_lsb;
	unsigned short  iso_logical_block_size_msb;
        unsigned int    iso_path_tbl_size_lsb;
        unsigned int    iso_path_tbl_size_msb;
        unsigned int    iso_L_path_tbl[2];
        unsigned int    iso_opt_L_path_tbl[2];
        unsigned int    iso_M_path_tbl[2];
        unsigned int    iso_opt_M_path_tbl[2];
        struct hsg_dir  iso_root_dir;
        char            iso_vol_set_id[128];
        char            iso_pub_id[128];
	char		iso_preparer_id[128];
        char            iso_application_id[128];
	char		iso_copyright_id[32];
	char		iso_abstract_id[32];
	char		iso_vol_dtcre[ISO_DTLEN-1];
	char		iso_vol_dtmod[ISO_DTLEN-1];
	char		iso_vol_dtexp[ISO_DTLEN-1];
	char		iso_vol_dteff[ISO_DTLEN-1];
	char		iso_file_struct_version;
	char		not_used_4;
	char		iso_application_use[512];
	char		not_used_5[680];
}; /* 2048 bytes long */

struct  iso_xar
{
	union {
		unsigned int xar_filler;
		struct iso_xar_oid_long {
			unsigned short iso_xar_oid_lsb;
			unsigned short iso_xar_oid_msb;
		} oid_un;
	} xar_oid;
#define iso_xar_oid xar_oid.oid_un.iso_xar_oid_lsb
	unsigned short iso_xar_gid_lsb;
	unsigned short iso_xar_gid_msb;
	unsigned short iso_xar_perm;
#define	ISO_NOT_OWN_READ 16
#define ISO_NOT_OWN_EXEC 64
#define ISO_NOT_GRP_READ 256
#define ISO_NOT_GRP_EXEC 1024
#define ISO_NOT_OTH_READ 4096
#define ISO_NOT_OTH_EXEC 16384
	unsigned char iso_xar_dtcre[ISO_DTLEN];
	unsigned char iso_xar_dtmod[ISO_DTLEN];
	unsigned char iso_xar_dtexp[ISO_DTLEN];
	unsigned char iso_xar_dteff[ISO_DTLEN];
	unsigned char iso_xar_recfmt;
#define ISO_RFNULL 0
#define ISO_RFFIX 1
#define ISO_RFLVAR 2
#define ISO_RFMVAR 3
	unsigned char iso_xar_recatt;
#define ISO_RALFCR 0
#define ISO_RAISO 1
#define ISO_RAINREC 2
	unsigned short iso_xar_reclen_lsb;
	unsigned short iso_xar_reclen_msb;
	char iso_xar_sysid[32];
	char iso_xar_sysuse[64];
	unsigned char iso_xar_version;
	unsigned char iso_xar_esclen;
	char not_used_1[64];
	unsigned short iso_xar_aulen_lsb;
	unsigned short iso_xar_aulen_msb;
	char iso_xar_application_use[1]; /* variable length */
};

struct hsg_xar
{
	union {
		unsigned int xar_filler;
		struct iso_xar_oid_long oid_un;
	} xar_oid;
	unsigned short iso_xar_gid_lsb;
	unsigned short iso_xar_gid_msb;
	unsigned short iso_xar_perm;
	unsigned char iso_xar_dtcre[ISO_DTLEN-1];
	unsigned char iso_xar_dtmod[ISO_DTLEN-1];
	unsigned char iso_xar_dtexp[ISO_DTLEN-1];
	unsigned char iso_xar_dteff[ISO_DTLEN-1];
	unsigned char iso_xar_recfmt;
	unsigned char iso_xar_recatt;
	unsigned short iso_xar_reclen_lsb;
	unsigned short iso_xar_reclen_msb;
	char iso_xar_sysid[32];
	char iso_xar_sysuse[64];
	unsigned char iso_xar_version;
	char iso_xar_not_used_1[65];
	unsigned short iso_xar_pdir_num_lsb;
	unsigned short iso_xar_pdir_num_msb;
	unsigned short iso_xar_aulen_lsb;
	unsigned short iso_xar_aulen_msb;
	struct hsg_dir iso_xar_dirrec;	 /* variable length */
	char iso_xar_application_use[1]; /* variable length */
};

/*
 * Contents of mount point buf.
 */

struct	fs
{
	struct	fs *fs_link;		/* linked list of file systems */
	struct	fs *fs_rlink;		/* used for incore super blocks */
	char	fs_ronly;		/* Read only ? */
	char	fs_format;		/* ISO9660 or HSG */
#define ISO_9660 0
#define ISO_HSG  1
	union {
		struct iso_fs isofs;	/* pointer to primary volume desc */
		struct hsg_fs hsgfs;
        } fs_block;
	int iso_rootino;		/* unique root number */
	int fs_ibsize;		/* File system block size */
};

/*
 * CDFS macros
 */

#define ISOFS_LBS(fs) \
	((fs->fs_format == ISO_9660 ?  \
	  fs->fs_block.isofs.iso_logical_block_size_lsb : \
	  fs->fs_block.hsgfs.iso_logical_block_size_lsb))

#define ISOFS_SETSIZE(fs) \
	((fs->fs_format == ISO_9660 ?  \
	  fs->fs_block.isofs.iso_vol_set_size_lsb : \
	  fs->fs_block.hsgfs.iso_vol_set_size_lsb))

#define ISOFS_VOLSEQNUM(fs) \
	((fs->fs_format == ISO_9660 ?  \
	  fs->fs_block.isofs.iso_vol_seq_num_lsb : \
	  fs->fs_block.hsgfs.iso_vol_seq_num_lsb))

#define MAXBUFHEADERS (MAXBSIZE / ISO_SECSIZE)
struct iso_strat
{
	struct iso_strat *strat_forw, *strat_back;
	int strat_numbufhdr;
	int strat_outstanding;
	struct buf *strat_bufhdr[MAXBUFHEADERS];
	caddr_t strat_save_baddr[MAXBUFHEADERS];
	struct buf *strat_bp;
};

#ifdef KERNEL
struct	fs *getfs();
#define FS(gp) ((gp)->g_mp->m_bufp->b_un.b_fs)
#endif
