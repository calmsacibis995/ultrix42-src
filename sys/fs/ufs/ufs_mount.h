/* @(#)ufs_mount.h	4.2  (ULTRIX)        1/21/91     */

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
 * ufs_mount.h
 *	contains the definition of the structure passed down to handle
 *	per filesystem specific data like paging threshold
 */

struct ufs_specific {
	u_long	ufs_flags;		/* see m_flags in ../h/mount.h */
	u_long	ufs_pgthresh;		/* minimum file size which pages */
};

/*
 * Used to identify a disk partition for mapping between character and block
 * devices.  This info is stored in the fs specific portion of the mount
 * entry by ufs_mount, and is built on the fly by spec_open() when a character
 * device is opened for write.
 */
struct ufs_partid {
	short	bus;			/* Bus				*/
	short	adpt_num;		/* Adapter number		*/
	short	nexus_num;		/* Nexus or node on adapter no. */
	short	bus_num;		/* Bus number			*/
	short	ctlr_num;		/* Controller number		*/
	short	rctlr_num;		/* Remote controller number	*/
	short	slave_num;		/* Plug or line number		*/
	short	unit_num;		/* Ultrix device unit number	*/
	long	category_stat;		/* Category specific mask	*/
};

#define devget_to_partid(d, p) {			\
	(p)->bus = (d)->bus;				\
	(p)->adpt_num = (d)->adpt_num;			\
	(p)->nexus_num = (d)->nexus_num;		\
	(p)->bus_num = (d)->bus_num;			\
	(p)->ctlr_num = (d)->ctlr_num;			\
	(p)->rctlr_num = (d)->rctlr_num;		\
	(p)->slave_num = (d)->slave_num;		\
	(p)->unit_num = (d)->unit_num;			\
	(p)->category_stat = (d)->category_stat;	\
}
