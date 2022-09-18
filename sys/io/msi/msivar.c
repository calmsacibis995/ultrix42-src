#ifndef	lint
static        char    *sccsid = "@(#)msivar.c	4.1  (ULTRIX)        7/2/90";
#endif	lint

/************************************************************************
 *                                                                      *
 *                      Copyright (c) 1989                              *
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
 *		Mayfair Storage Interconnect Port Driver
 *
 *   Abstract:	This module contains Mayfair Storage Interconnect Port
 *		Driver( MSI ) internal data structures and variables.
 *
 *   Creator:	Todd M. Katz	Creation Date:	December 07, 1988
 *
 *   Modification History:
 */

/* Libraries and Include Files.
 */
#include		"../h/types.h"
#include		"../h/param.h"
#include		"../h/time.h"
#include		"../h/ksched.h"
#include		"../h/errlog.h"
#include		"../io/scs/sca.h"
#include		"../io/scs/scaparam.h"
#include		"../io/ci/cippdsysap.h"
#include		"../io/ci/cisysap.h"
#include		"../io/msi/msisysap.h"
#include		"../io/bi/bvpsysap.h"
#include		"../io/gvp/gvpsysap.h"
#include		"../io/uba/uqsysap.h"
#include		"../io/sysap/sysap.h"
#include		"../io/ci/cippdscs.h"
#include		"../io/ci/ciscs.h"
#include		"../io/msi/msiscs.h"
#include		"../io/bi/bvpscs.h"
#include		"../io/gvp/gvpscs.h"
#include		"../io/uba/uqscs.h"
#include		"../io/scs/scs.h"
#include		"../io/ci/cippd.h"
#include		"../io/msi/msiport.h"

/* External Variables and Routines.
 */
extern	PB		*cippd_get_pb();
extern	SCSH		*msi_alloc_dg(),
			*msi_alloc_msg(),
			*msi_remove_dg(),
			*msi_remove_msg();
extern	u_long		cippd_open_pb(),
			gvp_map_buf(),
			msi_get_port(),
			msi_init_pb(),
			msi_remote_rst(),
			msi_remote_strt(),
			msi_send_reqid(),
			msi_set_circuit();
extern	void		cippd_crash_pb(),
			cippd_remove_pb(),
			gvp_unmap_buf(),
			msi_add_dg(),
			msi_add_msg(),
			msi_crash_lport(),
			msi_dealloc_dg(),
			msi_dealloc_msg(),
			msi_init_port(),
			msi_log_badport(),
			msi_send_dg(),
			msi_send_msg(),
			msi_shutdown();

/* Global Variables and Data Structures.
 */
PDT		msi_pdt	= {		/* MSI Port Dispatch Table	     */
					/* Mandatory PD Functions	     */
		    msi_alloc_dg,	/* Allocate Datagram Buffer	     */
		    msi_dealloc_dg,	/* Deallocate Datagram Buffer	     */
		    msi_add_dg,		/* Add Datagram Buffer to Free Pool  */
		    msi_remove_dg,	/* Remove Dg Buffer from Free Pool   */
		    msi_alloc_msg,	/* Allocate Message Buffer	     */
		    msi_dealloc_msg,	/* Deallocate Message Buffer	     */
		    msi_add_msg,	/* Add Message Buffer to Free Pool   */
		    msi_remove_msg,	/* Remove Msg Buffer from Free Pool  */
		    msi_send_msg,	/* Send Sequenced Message	     */
		    gvp_map_buf,	/* Map Buffer			     */
		    gvp_unmap_buf,	/* Unmap Buffer			     */
		    cippd_open_pb,	/* Transition Formative Path to Open */
		    cippd_crash_pb,	/* Crash Path			     */
		    cippd_get_pb,	/* Retrieve Path Block               */
		    cippd_remove_pb,	/* Remove Path Block from Databases  */
		    msi_remote_rst,	/* Reset Remote Port and System	     */
		    msi_remote_strt,	/* Start Remote Port and System	     */

					/* Optional PD Functions	     */
		    msi_send_dg,	/* Send Datagram		     */
		    NULL,		/* Send Block Data		     */
		    NULL,		/* Request Block Data		     */
		    msi_crash_lport,	/* Crash Local Port		     */
		    msi_shutdown,	/* Inform Systems of Local Shutdown  */

					/* Mandatory PD Functions for CI PPD */
		    msi_get_port,	/* Retrieve Port Number from Buffer  */
     ( u_long (*)())msi_init_port,	/* Initialize/Re-initialize a Port   */
     ( u_long (*)())msi_log_badport,	/* Log MSI Originating Bad Port Num  */
		    msi_send_reqid,	/* Request Remote Port Identification*/
		    msi_set_circuit,	/* Set Virtual Circuit State - On/Off*/

					/* Optional PD Functions for CI PPD  */
		    NULL,		/* Allocate Emergency Command Packets*/
		    NULL,		/* Deallocate Emergency Command Pkts */
		    msi_init_pb,	/* Initialize a Path Block	     */
		    NULL,		/* Invalidate Port Translation Cache */
		    NULL,		/* Notify Port of CI PPD Activity    */
		    NULL,		/* Test Local Port Connectivity	     */
		    NULL		/* Update Hardware Port Type	     */
};

static CLFTAB				/* Console Logging Formating Tables  */
/* NULL entries represent events which should never be logged by the CI port
 * driver.
 */
		msi_cli[] = {		/* MSI Informational Event Table     */
    { CF_LPORT,  "port initialized" },
    { CF_LPORT,  "port initialized" }
},
		msi_clre[] = {		/* MSI Remote Error Event Table	     */
    { CF_RPORT,  "remote port: state precludes communication" }
},
		msi_cle[] = {		/* MSI Error Event Table	     */
    { CF_NONE,   "packet transmission: retrys exhausted" },
    { CF_NONE,	 "remote port: invalid state" },
    { CF_PKT2,	 "remote packet: invalid source address" }
},
		msi_clse[] = {		/* MSI Severe Error Event Table	     */
    { CF_PKT,	 "local/remote packet: invalid destination/source" },
    { CF_PKT,	 "remote packet: unknown port operation code" },
    { CF_PKT,	 "remote packet: invalid port operation code" },
    { CF_PKT,	 "remote packet: invalid size" },
    { CF_PKT,	 "received message: out-of-sequence" },
    { CF_NONE,	 "port message free queue exhausted" },
    { CF_PKT,	 "remote packet: invalid local buffer name" },
    { CF_PKT,	 "remote packet: invalid local buffer length" },
    { CF_REGS,	 "dssi bus error occurred" },
    { CF_REGS,	 "sii chip selected with attention" },
    { CF_REGS,	 "sii chip selected non-dssi device" },
    { CF_REGS,	 "sii chip selected by non-dssi device" }
},
		msi_clfe[] = {		/* MSI Fatal Error Event Table	     */
    { CF_NONE,   "scs_system_id has not been set to a non-zero value" },
    { CF_NONE,   "port initialization: insufficient memory" },
    { CF_NONE,   "port initialization: insufficient ptes for double mapping" }
},
		msi_clppdse[] = {	/* CI PPD Severe Error Event Table   */
    { CF_NONE,	 NULL },
    { CF_NONE,	 NULL },
    { CF_NONE,   "attempt made to crash nonexistent path" },
    { CF_NONE, 	 NULL },
    { CF_NONE,	 NULL }
};
CLSTAB					/* MSI Console Logging Table	     */
	msi_cltab[ ES_FE + 1 ][ ESC_PPD + 1 ] = {
					/* Severity == ES_I		     */
    {  2,	msi_cli     },		/*     Subclass == ESC_PD( CI )      */
    {  0,	NULL	    },		/*     Subclass == ESC_PPD( CI PPD ) */
					/* Severity == ES_W		     */
    {  0,	NULL        },		/*     Subclass == ESC_PD( CI )      */
    {  0,	NULL	    },		/*     Subclass == ESC_PPD( CI PPD ) */
					/* Severity == ES_RE		     */
    {  1,	msi_clre    },		/*     Subclass == ESC_PD( CI )      */
    {  0,	NULL	    },		/*     Subclass == ESC_PPD( CI PPD ) */
					/* Severity == ES_E		     */
    {  3,	msi_cle     },		/*     Subclass == ESC_PD( CI )      */
    {  0,	NULL	    },		/*     Subclass == ESC_PPD( CI PPD ) */
					/* Severity == ES_SE		     */
    { 12,	msi_clse    },		/*     Subclass == ESC_PD( CI )      */
    {  5,	msi_clppdse },		/*     Subclass == ESC_PPD( CI PPD ) */
					/* Severity == ES_FE		     */
    {  3,	msi_clfe    },		/*     Subclass == ESC_PD( CI )      */
    {  0,	NULL        },		/*     Subclass == ESC_PPD( CI PPD ) */
};
