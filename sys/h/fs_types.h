/* 	@(#)fs_types.h	4.2	(ULTRIX)	11/9/90 	*/

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

/***********************************************************************
 *
 *		Modification History
 *
 * 09-Nov-90 -- prs
 *	Added cdfs mount type.
 *
 * 11 Sep 86 -- koehler
 *	renumber of fs types
 *
 ***********************************************************************/

/*
 * used to bind a fs type to an function pointer
 */
#ifndef _FS_TYPES_

#define GT_UNKWN	0x00		/* unknown file system type	*/
#define GT_ULTRIX	0x01		/* Ultrix file system (ufs)	*/
#define GT_ODS		0x02		/* ODS-II			*/
#define GT_RFS		0x03		/* tektronics remote fs		*/
#define GT_MSDOS	0x04		/* ms-dos file system		*/
#define GT_NFS		0x05		/* sun's nfs			*/
#define GT_DFS		0x06		/* DFS file system		*/
#define GT_SYSV		0x07		/* System 5 file system		*/
#define GT_CDFS		0x08		/* ISO9660/HSG CD-ROM		*/
#define GT_RSRV1	0xfa		/* Ultrix reserved 1		*/
#define GT_RSRV2	0xfb		/* Ultrix reserved 2		*/
#define GT_RSRV3	0xfc		/* Ultrix reserved 3		*/
#define GT_RSRV4	0xfd		/* Ultrix reserved 4		*/
#define GT_RSRV5	0xfe		/* Ultrix reserved 5		*/

#define GT_NUMTYPES	0xff
#ifndef KERNEL
char *gt_names[GT_NUMTYPES] = {
	"unknown", "ufs", "ods", "brfs", "msdos", "nfs", "dfs", "sysv", "cdfs", 0
};
#endif
#define _FS_TYPES_
#endif
