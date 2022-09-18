/* @(#)inode.h	4.1  (ULTRIX)        7/2/90     */

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

/*
 *	this header file exists to make transition to gfs easier
 */
#ifndef KERNEL
#include <sys/gnode_common.h>
#include <ufs/ufs_inode.h>
#include <sys/gnode.h>
#endif
/* flags */
#define	ILOCKED		GLOCKED
#define	IUPD		GUPD
#define	IACC		GACC
#define	IMOUNT		GMOUNT
#define	IWANT		GWANT
#define	ITEXT		GTEXT
#define	ICHG		GCHG
#define	ISHLOCK		GSHLOCK
#define	IEXLOCK		GEXLOCK
#define	ILWAIT		GLWAIT
#define	IMOD		GMOD
#define IINUSE		GINUSE
#define	IRENAME		GRENAME
#define ISYNC		GSYNC
#define	IXMOD		GXMOD

/* modes */
#define	IFMT		GFMT

#define	IFPORT		GFPORT
#define	IFCHR		GFCHR
#define	IFDIR		GFDIR
#define	IFBLK		GFBLK
#define	IFREG		GFREG
#define	IFLNK		GFLNK
#define	IFSOCK		GFSOCK

#define	ISUID		GSUID
#define	ISGID		GSGID
#define	ISVTX		GSVTX
#define	IREAD		GREAD
#define	IWRITE		GWRITE
#define	IEXEC		GEXEC

#define dinode ufs_inode
