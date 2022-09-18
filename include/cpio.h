/*	@(#)cpio.h	4.2	(ULTRIX)	10/16/90	*/

/************************************************************************
 *									*
 *			Copyright (c) 1989 by				*
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

/*
 * cpio.h
 *
 *	Modification History
 *	------------ -------
 *
 *	08-Jun-90	kegel
 *			XPG3 fix - changed MAGIC from int 070707 to
 *			string "070707" to pass VSX test.
 *
 *	28-Sep-89	lambert
 *			Creation of file.
 *

 ******************************************************************************
 *
 *	This header file defines structures and constants specified by
 *	the POSIX and X/Open standards pertaining to the "Cpio Data
 *	Interchange Format".
 *
 ******************************************************************************
 */
#ifndef MAGIC
#define 	MAGIC	"070707"	/* cpio magic number - XPG3 */
#endif /* not MAGIC */

/* The X/Open (and POSIX) specified cpio header */
struct XOPEN_header {
	char	c_magic[6],
		c_dev[6],
		c_ino[6],
		c_mode[6],
		c_uid[6],
		c_gid[6],
		c_nlink[6],
		c_rdev[6],
		c_mtime[11],
		c_namesize[6],
		c_filesize[11],
		c_name[256],
		c_filedata[11];
} XOpen_hdr;

/*	These definitions are used in the 'c_mode' field of the cpio header.

	File Permissions	*/
#define C_IRUSR		0000400			/* Read by owner */
#define C_IWUSR		0000200			/* Write by owner */
#define C_IXUSR		0000100			/* Execute by owner */
#define C_IRGRP		0000040			/* Read by group */
#define C_IWGRP		0000020			/* Write by group */
#define C_IXGRP		0000010			/* Execute by group */
#define C_IROTH		0000004			/* Read by other */
#define C_IWOTH		0000002			/* Write by other */
#define C_IXOTH		0000001			/* Execute by other */
#define C_ISUID		0004000			/* Set uid on execution */
#define C_ISGID		0002000			/* Set gid on execution */
#define C_ISVTX		0001000			/* Reserved */

/*	File Types	*/
#define C_ISDIR		0040000			/* Directory */
#define C_ISFIFO	0010000			/* FIFO */
#define C_ISREG		0100000			/* Regular file */
#define C_ISBLK		0060000			/* Block Special file */
#define C_ISCHR		0020000			/* Character Special file */
#define C_ISCTG		0110000			/* Reserved */
#define C_ISLNK		0120000			/* Reserved */
#define C_ISSOCK	0140000			/* Reserved */
