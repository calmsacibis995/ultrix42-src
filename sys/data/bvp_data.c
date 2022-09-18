/*
 * 	@(#)bvp_data.c	4.1	(ULTRIX)	7/2/90
 */
/************************************************************************
 *									*
 *                      Copyright (c) 1987 - 1989 by                    *
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
 *	Modification History
 *
 *	07-Mar-1989	Todd M. Katz		TMK0003
 *		1. Include header file ../vaxmsi/msisysap.h.
 *		2. Use the ../machine link to refer to machine specific header
 *		   files.
 *
 *	02-Jun-1988	Ricky S. Palmer
 *		Removed inclusion of header file ../vaxmsi/msisysap.h
 *
 *	19-Jan-1988	Todd M. Katz		TMK0002
 *		Included new header file ../vaxscs/scaparam.h.
 *
 *	09-Jan-1988	Todd M. Katz		TMK0001
 *		Included new header files ../vaxmsi/msisysap.h, and
 *		../vaxmsi/msiscs.h.
 */

#include	"../machine/pte.h"

#include	"../h/param.h"
#include	"../h/systm.h"
#include	"../h/buf.h"
#include	"../h/conf.h"
#include	"../h/dir.h"
#include	"../h/user.h"
#include	"../h/map.h"
#include	"../h/vm.h"
#include	"../h/dk.h"
#include	"../h/cmap.h"
#include	"../h/uio.h"
#include	"../h/ioctl.h"
#include	"../h/fs.h"

#include	"../machine/cpu.h"
#include	"../machine/scb.h"
#include	"../io/uba/ubareg.h"
#include	"../io/uba/ubavar.h"


#include	"../io/bi/bireg.h"
#include	"../io/bi/buareg.h"
#include	"../io/bi/bvpreg.h"
#include	"../io/bi/bvpport.h"
#include	"../io/bi/bdareg.h"

#include	"../h/errlog.h"
#include	"../h/ksched.h"
#include	"../io/scs/sca.h"
#include	"../io/scs/scaparam.h"
#include	"../io/ci/cippdsysap.h"
#include	"../io/ci/cisysap.h"
#include	"../io/msi/msisysap.h"
#include	"../io/bi/bvpsysap.h"
#include	"../io/gvp/gvpsysap.h"
#include	"../io/uba/uqsysap.h"
#include	"../io/sysap/sysap.h"
#include	"../io/ci/cippdscs.h"
#include	"../io/ci/ciscs.h"
#include	"../io/msi/msiscs.h"
#include	"../io/bi/bvpscs.h"
#include	"../io/gvp/gvpscs.h"
#include	"../io/uba/uqscs.h"
#include	"../io/scs/scs.h"
#include	"../io/gvp/gvp.h"
#include	"../io/bi/bvpppd.h"


#include	"bvpssp.h"



/* 	Port Info Block
 */

struct bvp_port_info bvp_port_info[NBVPSSP];


struct bvp_sw bvp_sw [] =
{
/*	BVP adap type		Address offset	Error log type	*/
	{ BI_AIO,		   0xF0,	ELBVP_AIO},
	{ BI_AIE,		   0xF0,	ELBVP_AIE},
	{ BI_AIE_TK,		   0xF0,	ELBVP_AIE},
	{ BI_AIE_TK70,		   0xF0,	ELBVP_AIE},
	{ BI_HSB,		   0xF0,	ELBVP_AIO}
};
int nbvptypes = sizeof (bvp_sw) / sizeof (bvp_sw[0]);

/*	BVP command table
 */

struct 	bvp_cmd	bvp_cmd[] =
{
	{BVP_PC_OWN | BVP_CMD_FQNE | BVPQ1}, /* Message free queue	   */
	{BVP_PC_OWN | BVP_CMD_FQNE | BVPQ0}, /* Datagram free queue	   */
	{BVP_PC_OWN | BVP_CMD_CQNE | BVPQ0}, /* Command free queue 0	   */
	{BVP_PC_OWN | BVP_CMD_CQNE | BVPQ1}, /* Command free queue 1	   */
	{BVP_PC_OWN | BVP_CMD_CQNE | BVPQ2}, /* Command free queue 2 	   */
	{BVP_PC_OWN | BVP_CMD_CQNE | BVPQ3}  /* Command free queue 3 	   */
};

