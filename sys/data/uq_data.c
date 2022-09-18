/*
 *	@(#)uq_data.c	4.1	(ULTRIX)	7/2/90
 */

/************************************************************************
 *									*
 *			Copyright (c) 1985 - 1989 by			*
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
 *	20-Jul-1989	Mark A. Parenti
 *	Remove uq_burst table. This information is now the the uq_cinfo
 *	table in uqport.h
 *
 *	07-Mar-1989	Todd M. Katz		TMK0002
 *		1. Include header file ../vaxmsi/msisysap.h.
 *		2. Use the ../machine link to refer to machine specific header
 *		   files.
 *
 *	18-July-1988 - map
 *		Dynamically allocate data structures.
 *
 *      02-Jun-1988     Ricky S. Palmer
 *              Removed inclusion of header file ../vaxmsi/msisysap.h
 *
 *	14-Mar-1988	Larry Cohen
 *		Remove all references to ra_info.
 *
 *	09-Jan-1988	Todd M. Katz		TMK0001
 *		Included new header files ../vaxscs/scaparam.h,
 *		../vaxmsi/msisysap.h, and  ../vaxmsi/msiscs.h.
 */

#include "uq.h"

#include "../machine/pte.h"
#include "../h/param.h"
#include "../h/systm.h"
#include "../h/buf.h"
#include "../h/conf.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/map.h"
#include "../h/vm.h"
#include "../h/vmmac.h"
#include "../h/dk.h"
#include "../h/cmap.h"
#include "../h/uio.h"
#include "../h/ioctl.h"
#include "../h/fs.h"

#include "../machine/cpu.h"
#include "../io/uba/ubareg.h"
#include "../io/uba/ubavar.h"
#include "../io/sysap/mscp_msg.h"


#include "../io/bi/bireg.h"
#include "../io/bi/buareg.h"
#include "../io/bi/bdareg.h"

#include	"../h/types.h"
/*
#include	"../h/time.h"
*/
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
#include	"../io/uba/uqppd.h"




/* 	Port Info Block
 */

struct port_info *port_info_ptr[NUQ];

struct	uba_ctlr *uqminfo[NUQ];

int	nNUQ = NUQ;



