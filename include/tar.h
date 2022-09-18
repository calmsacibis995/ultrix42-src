
/*
 *	@(#)tar.h	4.2	(ULTRIX)	9/4/90
 */

/****************************************************************
 *								*
 *			Copyright (c) 1985, 1988, 1989 by	*
 *		Digital Equipment Corporation, Maynard, MA	*
 *			All rights reserved.			*
 *								*
 *   This software is furnished under a license and may be used *
 *   and copied  only  in accordance with the terms of such	*
 *   license and with the  inclusion  of  the  above  copyright *
 *   notice. This software  or	any  other copies thereof may	*
 *   not be provided or otherwise made available to any other	*
 *   person.  No title to and ownership of the software is	*
 *   hereby transferred.					*
 *								*
 *   The information in this software is subject to change	*
 *   without  notice  and should not be construed as a		*
 *   commitment by Digital  Equipment Corporation.		*
 *								*
 *   Digital assumes  no responsibility   for  the use	or	*
 *   reliability of its software on equipment which is not	*
 *   supplied by Digital.					*
 *								*
 ****************************************************************
 *
 *
 *	Modification/Revision history:
 *	~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 *	revision			comments
 *	--------	-----------------------------------------------
 *
 *	14-Mar-90	lambert
 *			Corrected size of wdir, hdir; added signal handler
 *			for sigusr1.
 *
 *	12-Jan-90	prs
 *			Added _POSIX_SOURCE and _XOPEN_SOURCE test macros
 *			for POSIX and XOPEN.
 *
 *	31-Oct-89	rsp
 *			Cleaned up for POSIX compliance. Synced up
 *			defines with 1003.1 and removed need for
 *			POSIX ifdef's so that -P switch to command
 *			activates POSIX mode. Berkeley mode is default.
 *
 *	29-Sep-89	rsp
 *			Fixed POSIX/XPG4 bug around 'magic'
 *
 *	30-May-89	bstevens
 *			New -R option added for archive lists
 *
 *	30-May-89	rsp
 *			Positioning ioctl checking added
 *
 *	19-Aug-88	lambert
 *			Added conditional code for POSIX requirement.
 *
 *	09-Jun-88	map
 *			Changed signal handlers to void.
 *
 *	15-Mar-88	mjk - added posix support
 *
 *	20-Apr-87	fries
 *			Changed NBLOCK back to 20 for System V
 *			and Berkeley compatability.
 *
 *	05-Aug-86	fries
 *			Changed NBLOCK to limit blocksize based on
 *			whether system is on U11.
 *
 *	02-Jul-86	lp
 *			Added n-buffer i/o hooks. Cleanup a lot of
 *			different things like correct buffering of
 *			input and output. Removed ifdefs for nonVAX
 *
 *	20.x		Jeff Fries, 15-May-86
 *			Added header files supporting device generic
 *			ioctl.
 *
 *	17.x		Ray Glaser, 18-Dec-85
 *			Create orginal version
 *
 *
 *	File name:
 *
 *		tar.h
 *
 *	Source file description:
 *
 *		This file contains variable & constant
 *		declarations & definitions for the
 *		various tar source modules.
 */


#include <ansi_compat.h>
#define		TMAGIC		"ustar"
#define		TMAGLEN		6
#define		TVERSION	"00"
#define		TVERSLEN	2

/* Values used in the "typeflag" field of tar header block */
#define REGTYPE  '0'	/* Regular file */
#define AREGTYPE '\0'	/* Regular file */
#define LNKTYPE  '1'	/* Hard link */
#define SYMTYPE  '2'	/* Symbolic link */
#define CHRTYPE  '3'	/* Character special */
#define BLKTYPE  '4'	/* Block special */
#define DIRTYPE  '5'	/* Directory */
#define FIFOTYPE '6'	/* FIFO special */
#define CONTTYPE '7'	/* Contiguous file */

/* File Permissions */
#define		TSUID	04000	/* Set UID on execution */
#define		TSGID	02000	/* Set GID on execution */
#define		TSVTX	01000	/* Reserved */
#define		TUREAD	00400	/* read by owner */
#define		TUWRITE 00200	/* write by owner */
#define		TUEXEC	00100	/* execute/search by owner */
#define		TGREAD	00040	/* read by group */
#define		TGWRITE 00020	/* write by group */
#define		TGEXEC	00010	/* execute/search by group */
#define		TOREAD	00004	/* read by other */
#define		TOWRITE 00002	/* write by other */
#define		TOEXEC	00001	/* execute/search by other */

#ifndef __POSIX		/* Non __POSIX and XOPEN stuff */

/*	Generic includes.. */
#include <stdio.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <sys/dir.h>
#include <grp.h>
#include <pwd.h>
#include <signal.h>
#include <errno.h>
#include <a.out.h>

/*
 *
 *	Ultrix-32  specific includes...
 */
#include <sys/time.h>

#ifdef DEBUG
#include "ioctl.h"
#include "devio.h"
#include "mtio.h"
#else
#include <sys/ioctl.h>
#include <sys/devio.h>
#include <sys/mtio.h>
#endif

/*	typedefs - for better readability.	*/
typedef		int	COUNTER;
typedef		int	COUNT_INDEX;
typedef short	int	FLAG;
typedef		int	FILE_D;
typedef		int	INDEX;
typedef		int	SIZE_I;
typedef		long	SIZE_L;
typedef		char	*STRING_POINTER;

#undef	FAIL
#undef	FALSE
#undef	SUCCEED
#undef	TRUE

#define FAIL	-1
#define FALSE	0
#define SUCCEED 1
#define TRUE	1

#define A_WRITE_ERR	0
#define MAXARCHIVE	99
#define N	200
#define NAMSIZ	100	/* Maximum length a path/file name allowed
			 * in a tar header block.
			 */

#define DEF_NBLK 20	/* Default Number of TBLOCKs blocked for one
			 * read/write call.
			 */

#define NBLOCK	20	/* Number of TBLOCKs blocked for one
			 * read/write call.
			 */

#define MAXBLK	127	/* Maximum Number of TBLOCKs permitted
			 * for one read/write call.
			 */

/* Symbolically define various "types" of tar headers liable to be
 * encountered.
 *			NOTE:
 *
 *	The logic depends on form "OTA" being the lowest
 *	defined value.
 */
#define OTA 1	/* File was written by a very old tar. Does not
		 * contain any fields defined after linkname[].
		 * ie. Original Tar Archive format.
		 */
#define OUA 2	/* File was written by an older version of Ultrix tar.
		 * ie. The field following linkname[] may contain
		 * special device (major/minor) numbers.
		 */
#define UGS 3	/* File was written by a tar conforming to the
		 * User group standard. Contains fields up to
		 * UEGdummy[] in the header.
		 */
#define UMA 4	/* File was written by Ultrix tar using multi-archive
		 * extension fields in the header.
		 */

		/* Non-POSIX definitions */
#define		OTMAGIC		"ustar  "
#define		OTMAGLEN 	8

#undef	TSIZLEN
#undef	TDEVLEN

#define RGRP	040
#define ROTH	04
#define ROWN	0400
#define SGID	02000
#define SUID	04000
#define STXT	01000
#define TBLOCK	512	/* Size of a tape block */
#define TCKSLEN 8
#define TDEVLEN 8
#define TGIDLEN 8
#define TGNMLEN 32
#define TPRFXLEN 155
#define TMTMLEN 12
#define TMODLEN 8
#define TSIZLEN 12
#define TUIDLEN 8
#define TUNMLEN 32
#define TVLEN 3
#define WGRP	020
#define WOTH	02
#define WOWN	0200
#define XGRP	010
#define XOTH	01
#define XOWN	0100

/* Values used in the "typeflag" field of tar header block */
#define OAREGTYPE ' '	/* Regular file (non-POSIX) */

/*
 *	POSIX TAR HEADER FORMAT:   POSIX 1003.1 Standard
 *				 + Ultrix extensions for multi-archive.
 */
union hblock {
	char dummy[TBLOCK];
	struct header {
		char name[NAMSIZ];	/* Pathname of file */
		char mode[TMODLEN];	/* Permissions/modes of file */
		char uid[TUIDLEN];	/* User ID of file owner */
		char gid[TGIDLEN];	/* Group ID of file owner */
		char size[TSIZLEN];	/* Number of bytes in file */
		char mtime[TMTMLEN];	/* Modification time of file */
		char chksum[TCKSLEN];	/* Checksum of header values */
		char typeflag;		/* Specifies this files' type
					 * Formerly named -> linkflag */
		char linkname[NAMSIZ];	/* Linked-to file name */
		/*
		 * Point of departure from very old original
		 * tar	header format and older Ultrix format.
		 * Start of User Group standard extension fields.
		 */
		union {
			char reg_magic[OTMAGLEN];/* Value == OTMAGIC to identify
						 * new archive format */
						/* Is rdev[6] field for older
						 * Ultrix archive formats */
			struct {
				char pos_magic[TMAGLEN];
				char pos_version[TVERSLEN];
			} p_magic;
		} magic_number;
		char uname[TUNMLEN];	/* File owner user name */
		char gname[TGNMLEN];	/* File owner group name */
					/* Next 2 fields apply only to
					 * device special files. */
		char devmajor[TDEVLEN]; /* Major device number */
		char devminor[TDEVLEN]; /* Minor device number */
		/*
		 * Point of departure from User Group standard
		 * extension fields.
		 * Start of  ULTRIX  multi-archive extension fields.
		 * also path name for POSIX
		 */
		union {
			struct {
				char UEGdummy[1];
				 /* Archive numbers are ASCII 01 - 99 */
				char m_carch[TVLEN];/* Number of this (current) archive */
				char m_oarch[TVLEN];/* Orginal archive # on which this
						   * file was begun */
				char m_org_size[TSIZLEN];/* Original size of file if
							* this is a continued file. */
			} mult_v;
			char p_prefix[TPRFXLEN]; /* the prefix for POSIZ */
		} extension;
	} dbuf;
};


#define t_magic
/*
 * As an EOA (End Of Archive) indicator, tar will write the above form
 * of a directory block with these changes to identify it as an EOA.
 * The EOA block is written AFTER the 2 normal zero blocks that older
 * versions of tar use to indicate the end of archive. This is done
 * in order to prevent them crashing when reading a multi-archive
 * archive produced by this version of tar/mdtar.
 *
 *	a. The name field will contain the name of the file
 *	   that has been "split" across an archive.
 *
 *	b. All other fields will contain ASCII zeroes (as opposed
 *	   to real zero bytes) to flag this as the EOA record.
 *
 *	c. The last archive of a set contains an EOA block filled
 *	   with actual zeroes to indicate the end of the set.
 */

struct linkbuf {
	int	count;
	dev_t	devnum;
	ino_t	inum;
	struct	linkbuf *nextp;
	char	pathname[NAMSIZ];
};

struct DIRE {
		dev_t	rdev;		/* device for directory */
		ino_t	inode;		/* i-node number of file */
	struct	DIRE	*dir_next;	/* Pointer to next entry*/
};

#define magic magic_number.reg_magic
#define posix_magic magic_number.p_magic.pos_magic
#define posix_version magic_number.p_magic.pos_version
#define carch extension.mult_v.m_carch
#define oarch extension.mult_v.m_oarch
#define org_size extension.mult_v.m_org_size
#define posix_prefix extension.p_prefix

struct atblock {
	char tblock[TBLOCK];
};
#define PIPSIZ 8	/* 4k pipes work! */
#define MAXASYNC 4	/* only 4 buffers for N-buffer IO for now */

#ifdef __POSIX_REFERENCE
/*
	These definitions are here for POSIX reference ONLY. They
	are not used by Ultrix tar.  These definitions should
	only be changed if the POSIX specification itself changes.
*/
union hblock {
	struct header {
		char name[NAMSIZ];	/* Pathname of file */
		char mode[TMODLEN];	/* Permissions/modes of file */
		char uid[TUIDLEN];	/* User ID of file owner */
		char gid[TGIDLEN];	/* Group ID of file owner */
		char size[TSIZLEN];	/* Number of bytes in file */
		char mtime[TMTMLEN];	/* Modification time of file */
		char chksum[TCKSLEN];	/* Checksum of header values */
		char typeflag;		/* Specifies this files' type
					 * Formerly named -> linkflag */
		char linkname[NAMSIZ];	/* Linked-to file name */
		char magic[TMAGLEN];	/* Value == TMAGIC to identify
					 * new archive format */
		char version[TVERSLEN];
		char uname[TUNMLEN];	/* File owner user name */
		char gname[TGNMLEN];	/* File owner group name */
					/* Next 2 fields apply only to
					 * device special files. */
		char devmajor[TDEVLEN]; /* Major device number */
		char devminor[TDEVLEN]; /* Minor device number */
		char prefix[TPRFXLEN]; /* the prefix for POSIX */
	} dbuf;
};
#endif /* __POSIX_REFERENCE */

/* These are all declared in tar.c, command.c, writetape.c, and readtape.c */
extern	struct	DIRE	*Dhead;
extern	struct	passwd	*getpwnam();
extern	struct	group	*getgrgid();
extern	struct	group	*getgrnam();
extern	struct	group	*gp;
extern	struct	linkbuf *ihead;
extern	struct	linkbuf *lp;
extern	struct	devget	mtsts;
extern	struct	mtop	mtops;
extern	struct	stat	stbuf;
extern	int	device_open;
extern	int	errno;
extern	char	Archive[];
extern	daddr_t bsrch();
extern	union	hblock	cblock;
extern	char	CARCHS[];
extern	int	chksum;
extern	char	DIRECT[];
extern	union	hblock	dblock;
extern	char	eoa[];
extern	int	errno;
extern	char	file_name[];
extern	int	found;
extern	daddr_t high;
extern	char	hdir[MAXPATHLEN];
extern	char	iobuf[TBLOCK];
extern	daddr_t low;
extern	char	magtape[];
extern	char	mdtar[];
extern	int	njab;
extern	char	NULS[];
extern	void	onhup();
extern	void	onintr();
extern	void	onquit();
extern	void	onterm();
extern	void	onusr1();
extern	char	*progname;
extern	int	revwhole;
extern	int	revdec;
extern	char	*rindex();
extern	char	*sprintf();
extern	char	*strcat();
extern	char	*strfind();
extern	union	hblock	*tbuf;
extern	time_t	modify_time;
extern	char	tname[];
extern	char	unitc;
extern	char	*usefile;
extern	char	wdir[MAXPATHLEN];
extern	int	is_posix;
extern	int	is_tape;
extern	FLAG	AFLAG;
extern	FLAG	Bflag;
extern	FLAG	bflag;
extern	FLAG	cflag;
extern	FLAG	DFLAG;
extern	FLAG	dflag;
extern	FLAG	EOTFLAG;
extern	FLAG	EODFLAG;
extern	FLAG	FEOT;
extern	FLAG	FILE_CONTINUES;
extern	FLAG	Fflag;
extern	FLAG	fflag;
extern	FLAG	first;
extern	FLAG	hdrtype;
extern	FLAG	header_flag;
extern	FLAG	HELP;
extern	FLAG	hflag;
extern	FLAG	iflag;
extern	FLAG	lflag;
extern	FLAG	MDTAR;
extern	FLAG	MFLAG;
extern	FLAG	mflag;
extern	FLAG	MULTI;
extern	FLAG	new_file;
extern	FLAG	nextvol;
extern	FLAG	NFLAG;
extern	FLAG	NMEM4D;
extern	FLAG	NMEM4L;
extern	FLAG	OARCH;
extern	FLAG	OFLAG;
extern	FLAG	oflag;
extern	FLAG	pflag;
extern	FLAG	pipein;
extern	FLAG	PUTE;
extern	FLAG	rflag;
extern	FLAG	SFLAG;
extern	FLAG	sflag;
extern	FLAG	term;
extern	FLAG	tflag;
extern	FLAG	unitflag;
extern	FLAG	VFLAG;
extern	FLAG	vflag;
extern	FLAG	Rflag;
extern	FLAG	volann;
extern	FLAG	VOLCHK;
extern	FLAG	wflag;
extern	FLAG	xflag;
extern	SIZE_I	blocks;
extern	SIZE_I	MAXAR;
extern	SIZE_I	nblock;
extern	SIZE_L	blocks_used;
extern	SIZE_L	bytes;
extern	SIZE_L	chctrs_in_this_chunk;
extern	SIZE_L	cmtime;
extern	SIZE_L	corgsize;
extern	SIZE_L	cremain;
extern	SIZE_L	extracted_size;
extern	SIZE_L	original_size;
extern	SIZE_L	remaining_chctrs;
extern	SIZE_L	written;
extern	SIZE_L	size_of_media[];
extern	COUNT_INDEX CARCH;
extern	COUNTER dcount1;
extern	COUNTER dcount2;
extern	COUNTER dcount3;
extern	COUNTER lcount1;
extern	COUNTER lcount2;
extern	COUNTER start_archive;
extern	STRING_POINTER	getcwd();
extern	STRING_POINTER	getwd();
extern	FILE_D	mt;
extern	INDEX	recno;
extern	FILE	*tfile;

#endif	/* NOT __POSIX */




