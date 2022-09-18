/*    @(#)sysinfo.h	4.8     (ULTRIX)        10/16/90     */

/************************************************************************
 *									*
 *			Copyright (c) 1988 by				*
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
 *
 *   Modification history:
 *  10/16/90 	U. Sinkewicz
 *	Added, for Andrew Moskal,  GSI_SCS entries.
 *
 *  9/4/90	dlh
 *	added a new getsysinfo() operation type:  GSI_VPTOTAL - return 
 *	number of vector processors in system
 *
 *  13 Aug 90 	sekhar
 *	Defined GSI_MMAPALIGN for both vax and mips.
 *
 *  09 July 90  Fred L. Templin
 *	Added GSI_BOOTTYPE
 *
 *  22 Jun 90	sekhar
 *	Added GSI_MMAPALIGN for mmap support(mips only).
 *
 *  15 Dec 89    Alan Frechette
 *	Added GSI_WSD_TYPE and GSI_WSD_UNITS.
 *
 *  1 Jun 89    Giles Atkinson
 *	Added SSI_LOGIN
 *
 *  8 May 89	Giles Atkinson
 *	Added entries for LMF
 *
 *  9 Mar 88 -- chet
 *	Created this file.
 *
 */

/*
 *	This file contains constants used with the getsysinfo() and
 *	setsysinfo() system calls.
 *
 *	Both of these calls are operation driven; particular
 *	flavors of operation may use arguments, identifiers, flags, etc.
 *	to define the actual result.
 *
 */

/*
 *	getsysinfo() operation types
 */

#ifdef KERNEL
#include "../h/ansi_compat.h"
#else
#include <ansi_compat.h>
#endif

#define GSI_PROG_ENV	1	/* Return process compatibility mode of */
                                /* the process as defined in <sys/exec.h> */

#define GSI_MAX_UPROCS	2	/* Return the maximum number of processes */
                                /* allowed per user id */

#define	GSI_TTYP	3	/* Return the device number of the */
                                /* controlling terminal */

#define GSI_NETBLK	4	/* Return the entire netblk structure */
				/* which is used for network install */

#define GSI_BOOTDEV	5	/* Return the bootdev string */
				/* which is used for install */

#ifdef __mips
/* 
 * these return value of the flag 
 * that turns on/off printing the 
 * fixed up unaligned access message 
 */
#define GSI_UACSYS      6       /* get system wide flag */

#define GSI_UACPARNT    7	/* get parents */

#define GSI_UACPROC     8	/* get current proc */
#endif /* __mips */

#define GSI_LMF         9       /* License managment faciility (LMF) */

#define GSI_WSD_TYPE    10      /* Workstation Display Type Info */

#define GSI_WSD_UNITS   11      /* Workstation Display Units Info */

#define GSI_MMAPALIGN   12      /* support for mmap device drivers */

#define	GSI_BOOTTYPE	13	/* Network Interface boot type */

#define GSI_VPTOTAL	14      /* number of vector processors in system */

#define GSI_SCS		15	/* Systems Communications Services */

#define	GSI_PHYSMEM	19	/* Amount of physical memory in KB */

#define GSI_DNAUID	20	/* DNA UID genterator (UUID) */

#define	GSI_BOOTCTLR	21	/* Logical Controller # for TURBOchannel slot */

#define	GSI_CONSTYPE	22	/* MIPS console type identifier */

/*
 *	setsysinfo() operation types
 */

#define	SSI_NVPAIRS	1	/* Use a list of name value pairs to modify */
                                /* pre-defined system variables */

#define	SSI_ZERO_STRUCT	2	/* Zero a pre-defined system structure */

#define	SSI_SET_STRUCT	3	/* Set a pre-defined system structure to */
                                /* supplied values */

/*
 *	setsysinfo() SSI_NVPAIRS variable names
 */

#define	SSIN_NFSPORTMON 1	/* A boolean which determines whether */
                                /* incoming NFS traffic is originating */
                                /* at a privileged port or not */

#define	SSIN_NFSSETLOCK	2	/* A boolean which determines whether NFS */
                                /* (daemon) style file and record locking */
                                /* is enabled or not */

#define SSIN_PROG_ENV	3	/* set prog environment, BSD, SYSV, POSIX */

#ifdef __mips
/* see GSI_UACxxx */
#define SSIN_UACSYS	4	/* set system printing on/off */
#define SSIN_UACPARNT	5	/* set parent proc on/off */
#define SSIN_UACPROC	6	/* set current proc on/off */
#endif /* __mips */

#define SSI_LMF         7       /* License managment faciility (LMF) */

#define SSI_LOGIN	8	/* Identify caller as a login process */
				/* (Sets SLOGIN flag in proc struct) */
/*
 *	setsysinfo() SSI_ZERO_STRUCT and SSI_SET_STRUCT structure types
 */


