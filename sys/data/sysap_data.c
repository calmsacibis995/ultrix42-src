#ifndef	lint
static	char	*sccsid = "@(#)sysap_data.c	4.1	(ULTRIX)	7/2/90";
#endif	lint

/************************************************************************
 *                                                                      *
 *                      Copyright (c) 1986 - 1988 by                    *
 *              Digital Equipment Corporation, Maynard, MA              *
 *                      All rights reserved.                            *
 *                                                                      *
 *   This software is furnished under a license and may be used and     *
 *   copied  only  in accordance with the terms of such license and     *
 *   with the  inclusion  of  the  above  copyright  notice.   This     *
 *   software  or  any  other copies thereof may not be provided or     *
 *   otherwise made available to any other person.  No title to and     *
 *   ownership of the software is hereby transferred.                   *
 *                                                                      *
 *   The information in this software is subject to change  without     *
 *   notice  and should not be construed as a commitment by Digital     *
 *   Equipment Corporation.                                             *
 *                                                                      *
 *   Digital assumes no responsibility for the use  or  reliability     *
 *   of its software on equipment which is not supplied by Digital.     *
 *                                                                      *
 ************************************************************************
 *
 *
 *   Facility:	Systems Communication Architecture
 *
 *   Abstract:	This module contains the global table of routines that
 *		initialize and or start System Applications (SYSAPS).
 *
 *
 *   Creator:	Larry Cohen	Creation Date:  March 13, 1987
 *
 *   History:
 *
 *   14-Mar-88	Larry Cohen
 *	Initialize scs before other sysaps start up.
 *
 *   18-Apr-88  Ricky Palmer
 *	Added support for MSI (Mass Storage Interconnect) on MF2.
 *
 *   16-Jun-88		larry
 *	mscp drivers initialized when uq,bvpssp,ci, or msi are configured in.
 *
 */
/**/


#include	"../h/types.h"
#include	"../h/param.h"
#include	"../h/ksched.h"
#include	"../h/time.h"
#include	"../h/errlog.h"
#include 	"../io/sysap/sysap_start.h"

#include "uq.h"
#include "ci.h"
#include "msi.h"
#include "bvpssp.h"
#include "inet.h"
#include "scsnet.h"


#if NCI > 0 || NUQ > 0 || NBVPSSP > 0 || NMSI > 0
	extern void scs$dir_init();
	extern void scs_initialize();
#	define SYSAP_SCS$DIRECTORY scs$dir_init
#	define SCS_INIT scs_initialize
#else
#	define SYSAP_SCS$DIRECTORY 0
#	define SCS_INIT 0
#endif

#if NCI > 0 && NINET > 0 && NSCSNET > 0
	extern void scsnet_attach();
#	define SYSAP_SCSNET scsnet_attach
#else
#	define SYSAP_SCSNET 0
#endif

#if NCI > 0 || NUQ > 0 || NBVPSSP > 0 || NMSI > 0
	extern void mscp_init_driver();
#	define SYSAP_MSCP mscp_init_driver
#else
#	define SYSAP_MSCP 0
#endif


#if NCI > 0 || NUQ > 0 || NBVPSSP > 0 || NMSI > 0
	extern void      tmscp_init_driver();
#	define SYSAP_TMSCP tmscp_init_driver
#else
#	define SYSAP_TMSCP 0
#endif



#ifndef BINARY

	Sysap_start sysaps[] = {
		SCS_INIT,
		SYSAP_SCS$DIRECTORY,
		SYSAP_SCSNET,
		SYSAP_TMSCP,
		SYSAP_MSCP,
		SYSAP_LAST
	};

#endif
