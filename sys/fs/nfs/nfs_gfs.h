/*	@(#)nfs_gfs.h	4.1	(ULTRIX)	7/2/90	*/

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
 *	Portions of this software have been licensed to 
 *	Digital Equipment Company, Maynard, MA.
 *	Copyright (c) 1986 Sun Microsystems, Inc.  ALL RIGHTS RESERVED.
 */


struct nfs_gfs_mount {
	struct sockaddr_in *addr;	/* file server address */
	fhandle_t *fh;			/* File handle to be mounted */
	int	flags;			/* flags */
	int	wsize;			/* write size in bytes */
	int	rsize;			/* read size in bytes */
	int	timeo;			/* initial timeout in .1 secs */
	int	retrans;		/* times to retry send */
	char	*hostname;		/* server's name */
	char	*optstr;		/* options string */
	int	gfs_flags;		/* GFS-specific flags */
	int	pg_thresh;		/* page threshold for exec */
        int     acregmin;               /* min seconds for file attr cache */
        int     acregmax;               /* max seconds for file attr cache */
        int     acdirmin;               /* min seconds for dir caching */
        int     acdirmax;               /* max seconds for dir caching */
};

/*
 * NFS mount option flags
 */
#define	NFSMNT_RONLY	0x0001	/* mount read-only */
#define	NFSMNT_SOFT	0x0002	/* soft mount (hard is default) */
#define	NFSMNT_WSIZE	0x0004	/* set write size */
#define	NFSMNT_RSIZE	0x0008	/* set read size */
#define	NFSMNT_TIMEO	0x0010	/* set initial timeout */
#define	NFSMNT_RETRANS	0x0020	/* set number of request retrys */
#define	NFSMNT_HOSTNAME	0x0040	/* set hostname for error printf */
#define	NFSMNT_PGTHRESH 0x0080	/* set page threshold for exec */
#define NFSMNT_INT	0x0100	/* allow hard mount keyboard interrupts */
#define NFSMNT_NOAC	0x0200	/* don't cache attributes */
