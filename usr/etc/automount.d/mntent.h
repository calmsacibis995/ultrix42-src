/* @(#)mntent.h	4.1  (ULTRIX)        7/2/90     */

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
 *	Copyright (c) 1987 Sun Microsystems, Inc.  ALL RIGHTS RESERVED.
 */
/*
 *	Modification History:
 *
 * 	14 Jun 89 -- condylis
 *		Added copyright header.
 *
 */



#define max(a,b)        ((a) > (b) ? (a) : (b))

#define MNTTAB          "/etc/fstab"
#define MOUNTED         "/etc/mtab"

#define MNTTYPE_42      "4.2"   /* 4.2 file system */
#define MNTTYPE_NFS     "nfs"   /* network file system */
#define MNTTYPE_PC      "pc"    /* IBM PC (MSDOS) file system */
#define MNTTYPE_SWAP    "swap"  /* swap file system */
#define MNTTYPE_IGNORE  "ignore"/* No type specified, ignore this entry */
#define MNTTYPE_LO      "lo"    /* Loop back File system */

#define MNTOPT_RO       "ro"            /* read only */
#define MNTOPT_RW       "rw"            /* read/write */
#define MNTOPT_SOFT     "soft"          /* soft mount */
#define MNTOPT_HARD     "hard"          /* hard mount */
#define MNTOPT_BG       "bg"            /* background mount if no answer */
#define MNTOPT_NOSUID   "nosuid"        /* no set uid allowed */
#define MNTOPT_NOEXEC   "noexec"        /* no execution allowed */
#define MNTOPT_NODEV    "nodev"         /* no devices access allowed */
#define MNTOPT_FORCE    "force"         /* force the mount */
#define MNTOPT_SYNC     "sync"          /* synchronous writes */
#define MNTOPT_NOCACHE  "nocache"       /* don't keep in cache -- write thru */
#define MNTOPT_INT      "intr"          /* allow hard mount keyboard interrupts */
#define MNTOPT_SECURE	"secure"	/* use secure rpc	*/
#define MNTOPT_RSIZE    "rsize="        /* read size */
#define MNTOPT_WSIZE    "wsize="        /* write size */
#define MNTOPT_RETRANS  "retrans="      /* # of NFS retries */
#define MNTOPT_RETRY    "retry="        /* # of mount retries */
#define MNTOPT_TIMEO    "timeo="        /* timeout interval */
#define MNTOPT_PORT     "port="         /* NFS port # */
#define MNTOPT_PGTHRESH "pgthresh="     /* paging threshold */
#define MNTOPT_NOAC	"noac"		/* no attribute caching */
#define MNTOPT_ACREGMIN "acregmin="     /* min seconds for caching file attrs*/
#define MNTOPT_ACREGMAX "acregmax="     /* max seconds for caching file attrs*/
#define MNTOPT_ACDIRMIN "acdirmin="     /* min seconds for caching dir attrs*/
#define MNTOPT_ACDIRMAX "acdirmax="     /* max seconds for caching dir attrs*/
#define MNTOPT_ACTIMEO 	"actimeo="     	/* min and max times for both files
					   and directories 		*/

#define NFSAC_REGMIN    3
#define NFSAC_REGMAX    60
#define NFSAC_DIRMIN    30
#define NFSAC_DIRMAX    60
#define MAXACTIME       3600
#define SIXTYFOUR       64      /* default page threshhold */



struct  mntent{
        char    *mnt_fsname;            /* name of mounted file system */
        char    *mnt_dir;               /* file system path prefix */
	char    *mnt_type;              /* MNTTYPE_* */
        char    *mnt_opts;              /* MNTOPT* */
	int     mnt_freq;               /* dump frequency, in days */
	int     mnt_passno;             /* pass number on parallel fsck */
};

