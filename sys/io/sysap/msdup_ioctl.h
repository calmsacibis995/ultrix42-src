/*	@(#)msdup_ioctl.h	4.2	(ULTRIX)	2/19/91	*/

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
 *   Facility:	Systems Communication Architecture
 *		Diagnostic/Utilities Protocol (DUP) Class Driver
 *
 *   Abstract:	This module contains I/O control function definitions
 *		specific to DUP support.
 *
 *   Author:	Andrew J. Moskal	Creation Date:	dd-mmn-yyyy
 *
 *   Modification History:
 *
 */

/*
 *   Conditional check to include this header file only once.
 */
#ifndef MSDUP_ioctl.h
#define MSDUP_ioctl.h	0		/* Disable subsequent includes       */

/*
 *  Include prerequisite definitions
 */
#ifdef	KERNEL
#include "../h/ioctl.h"			/* Common I/O control function defs  */
#include "../io/scs/sca.h"		/* Generic SCA definitions           */
#include "../io/ci/cippdsysap.h"	/* CI PPD defintions                 */
#include "../io/ci/cisysap.h"		/* CI port definitions               */
#include "../io/bi/bvpsysap.h"		/* BI port definitions               */
#include "../io/gvp/gvpsysap.h"		/* Generic VAX port definitions      */
#include "../io/msi/msisysap.h"		/* MSI port definitions              */
#include "../io/uba/uqsysap.h"		/* U/Q port definitions              */
#include "../io/sysap/sysap.h"		/* Generic SYSAP/SCS definitions     */
#include "../io/sysap/msdup_pgm.h"	/* DUP program file/header defs      */
#else	KERNEL
#include <sys/ioctl.h>			/* Common I/O control function defs  */
#include <io/scs/sca.h>			/* Generic SCA definitions           */
#include <io/ci/cippdsysap.h>		/* CI PDD definitions                */
#include <io/ci/cisysap.h>		/* CI port definitions               */
#include <io/bi/bvpsysap.h>		/* BI port definitions               */
#include <io/gvp/gvpsysap.h>		/* Generic VAX port definitions      */
#include <io/msi/msisysap.h>		/* MSI port definitions              */
#include <io/uba/uqsysap.h>		/* U/Q port definitions              */
#include <io/sysap/sysap.h>		/* Generic SYSAP/SCS definitions     */
#include <io/sysap/msdup_pgm.h>		/* DUP program file/header defs      */
#endif	KERNEL

/*
 *  Local constants
 */
#define	PGM_EXT_SIZE	3

/*
 *  DUP ioctl function codes
 */
#define MSDUP_CONNECT	    _IOWR('x', 0, struct _msdup_sdb )
							/* Connect server    */
#define MSDUP_DISCONNECT    _IO('x', 1 )		/* Disconnect server */
#define MSDUP_GETDUST	    _IOR('x', 2, struct _msdup_dust )
							/* Get DUST Status   */
#define MSDUP_EXELCL	    _IOWR('x', 3, struct _msdup_pdb )
							/* EXECUTE LOCAL     */
#define MSDUP_EXESUP	    _IOWR('x', 4, struct _msdup_pdb )
							/* EXECUTE SUPPLIED  */
#define MSDUP_ABORT	    _IO('x', 5 )		/* ABORT PROGRAM     */
#define MSDUP_GETSTATE	    _IOWR('x', 6, MSDUP_STATES)  /* Get the current state of the server. */
#define MSDUP_KILLIO	    _IO('x', 7)			/* Clear all IO buffers */
#define MSDUP_SIGNAL	    _IOW('x', 8, int)		/* Signal to send to application 
							   on an interesting event.  */
#define MSDUP_GETERR	    _IOWR('x', 9, int)		/* Return last error. */
#define MSDUP_GETVERS	    _IOWR('x', 10, VERS_STRING)	/* Return version. */
#define	MSDUP_TMP_GETSCS    _IOWR('x', 11, MSDUP_TMP_SCSINFO)
            						/* Temp for now. 
							   Does the job of 
							   getsysinfo(GSI_SCS,...)
							   system call.  */

typedef struct {
  char		*buf, *arg;
  int		size, start, flag;
} MSDUP_TMP_SCSINFO;

/* Values for flag argument to getsysinfo */

#define	SCS_GETSCS		1	/* SCS information                   */
#define	SCS_GETSYSTEM		2	/* System information                */
#define	SCS_GETCONN		3	/* Logical connection information    */
#define	SCS_GETLISTEN		4	/* Listening SCS connection info     */
#define	SCS_GETLPORT		5	/* Local port information            */
#define	SCS_GETPATH		6	/* Path information                  */

/*
 * Typedef for giving sizeinformation to GETVERS ioctl.
*/
#define	VERS_STRING_SIZE	32
typedef char VERS_STRING[VERS_STRING_SIZE];
/*
 *  DUP server flag definitions
 */
typedef struct _msdup_dustflg {
    u_char	dsbl_srvr	:  1;	/*  Disables all other servers       */
    u_char	lcl_media	:  1;	/*  Local storage media present      */
    u_char	no_exesup	:  1;	/*  EXECUTE SUPPLIED not supported   */
    u_char	active		:  1;	/*  Active (Program executing)       */
    u_char	sdi		:  1;	/*  SEND DATA IMMEDIATE supported    */
    u_char			:  3;	/*  Reserved                         */
} MSDUP_DUSTFLG;			/* DUP server status flags           */

/*
 *  ioctl control block formats
 */

/*
 *  DUP get DUST ioctl request status block
 */
typedef struct _msdup_dust {
    u_char		pgm_ext[ PGM_EXT_SIZE ];
					/* Program extension                 */
    MSDUP_DUSTFLG	flags;		/* Server flags                      */
    u_long		progress;	/* Progress indicator                */
    u_short		exelcl_tmo;	/* Execute Local Program timeout     */
    u_char		abort_tmo;	/* Abort timeout                     */
} MSDUP_DUST;


/*
 *  DUP connect ioctl request system descriptor block
 */
typedef struct _msdup_sdb {
    u_char		srvr_name[ NAME_SIZE ];
					/* DUP server name( blank filled )   */
    u_char		dev_name[ NODENAME_SIZE ];
					/* Device name string                */
    u_char		node_name[ NODENAME_SIZE ];
					/* SCS node name                     */
    c_scaaddr		sysid;		/* SCS System ID                     */
    MSDUP_DUST		dust;		/* Get DUST status buffer            */
} MSDUP_SDB;

/*
 *  DUP execute ioctl request program descriptor block
 */
typedef struct _msdup_pdb {
    u_char		pgm_name[ PGM_NAME_SIZE ];
					/*  Program name( blank filled )     */
    u_long		ini_size;	/*  Initial segment size             */
    u_long		ini_base;	/*  Initial segment base address     */
    u_long		ovr_size;	/*  Overlay segment size             */
    u_long		ovr_base;	/*  Overlay segment base address     */
    u_short		version;	/*  Program version number           */
    u_char		xfer_tmo;	/*  Timeout value for data transfers */
    MSDUP_PGMFLG	flags;		/*  Program control flags            */
} MSDUP_PDB;

#endif	MSDUP_ioctl.h
