#ifndef lint
static char *sccsid = "@(#)msdup_data.c	4.1	(ULTRIX)	2/19/91";
#endif lint
/************************************************************************
 *                                                                      *
 *                      Copyright (c) 1989 by                           *
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
 *   This software is  derived  from  software  received  from  the     *
 *   University    of   California,   Berkeley,   and   from   Bell     *
 *   Laboratories.  Use, duplication, or disclosure is  subject  to     *
 *   restrictions  under  license  agreements  with  University  of     *
 *   California and with AT&T.                                          *
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
 *   Facility:	Systems Communications Architecture
 *		Diagnostic/Utilities Protocol (DUP) Class Driver
 *
 *   Abstract:	This file contains DUP class driver data structures.
 *
 *   Author:	Andrew J. Moskal	Creation Date: dd-mmm-yyyy
 *
 *   Modification history:
 *
 */


/*
 *  Libraries and Include Files.
 */
/*
#include	"../h/types.h"
#include	"../h/time.h"
#include	"../h/param.h"
#include	"../h/kmalloc.h"
#include	"../h/buf.h"
#include	"../h/errno.h"
#include	"../h/ioctl.h"
#include	"../h/devio.h"
#include	"../h/file.h"
#include	"../fs/ufs/fs.h"
#include	"../h/errlog.h"
#include	"../machine/pte.h"
#include	"../h/vmmac.h"
*/
/*#include	"../io/scs/sca.h"	*/
/*#include	"../io/ci/cippdsysap.h"	*/
/*#include	"../io/ci/cisysap.h"	*/
/*#include	"../io/bi/bvpsysap.h"	*/
/*#include	"../io/gvp/gvpsysap.h"	*/
/*#include	"../io/msi/msisysap.h"	*/
/*#include	"../io/uba/uqsysap.h"	*/
/*#include	"../io/sysap/sysap.h"	*/
/*#include	"../io/uba/ubavar.h"	*/
/* #include	"../io/sysap/msdup_defs.h" */

/*
 *  External Variables and Routines.
 */

#define	MSDUP_NUNIT	16	/* Defined here and in msdup_defs.h. 
				   Change BOTH together.	*/
/*
 *  DUP class driver configuration parameters
 */
int		msdup_max_nunit = MSDUP_NUNIT;	/* Psuedo device count       */
