#ifndef	lint
static char *sccsid = "@(#)civar.c	4.1	(ULTRIX)	7/2/90";
#endif	lint

/************************************************************************
 *                                                                      *
 *                      Copyright (c) 1988 - 1989 by                    *
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
 *		Computer Interconnect Port Driver
 *
 *   Abstract:	This module contains Computer Interconnect Port Driver(
 *		CI ) internal data structures and variables.
 *
 *   Creator:	Todd M. Katz	Creation Date:	November 12, 1985
 *
 *   Modification History:
 *
 *   06-Jun-1990	Pete Keilty
 *	1. Added six new error events to the ci_clse table for the CIXCD.
 *	2. Added preliminary support for the CIKMF and SHAC.
 *
 *   29-Mar-1990	Pete Keilty
 *	Add microcode entry 6/4,5/5,5/4 to cibca_aa_mrltab[].
 *
 *   02-Jan-1990	Pete Keilty
 *	Add microcode entry 6/3 to cibca_aa_mrltab[].
 *
 *   19-Sep-1989	Pete Keilty
 *	Added XCD support, removed XCB.
 *
 *   05-Mar-1989	Todd M. Katz		TMK0008
 *	Include header file ../vaxmsi/msisysap.h.
 *
 *   07-Oct-1988	Todd M. Katz		TMK0007
 *	Modify cibca_aa_mrltab[] so that a warning is printed whenever revision
 *	level 2 self-test microcode is encountered, regardless of the
 *	functional microcode revision level.  Previously, presence of revision
 *	level 2 self-test microcode was always regarded as a fatal error.
 *
 *   22-Sep-1988	Todd M. Katz		TMK0006
 *	1. Update ci_clppdse[] and ci_cltab[][] to reflect the creation of a
 *	   new path specific CI PPD severe error event logged only by the CI
 *	   PPD itself and never by the CI port driver.
 *	2. Update all console logging message strings.
 *
 *   18-Aug-1988	Todd M. Katz		TMK0005
 *	1. SCA event code formats have been complete redefined.  Both new
 *	   severity levels as well as severity level modifiers have been
 *	   created.  The GVP subclass( ESC_GVP ) has been eliminated.  Most
 *	   current CI event codes have been reassigned to different severity
 *	   levels.  New event codes have been added.  Revise and add new
 *	   console logging format tables, and update ci_cltab[][].
 *	2. Update ci_pdt by removing ci_map_reason( Map_reason ) and
 *	   ci_log_packet( Log_pcrashes ), two functions no longer required by
 *	   the CI PPD, and adding ci_log_badport(), a new mandatory PD routine
 *	   required by the CI PPD.
 *	3. Delete ci_map_cipc[].  Port driver specific path crash reason codes
 *	   codes no longer exist and all reason code mapping takes place
 *	   entirely within the CI PPD.
 *
 *   03-Jun-1988	Todd M. Katz		TMK0004
 *	1. Update console logging format table ci_clppdlpc[].
 *
 *   03-Jun-1988	Todd M. Katz		TMK0003
 *	1. Create a single unified hierarchical set of naming conventions for
 *	   use within the CI port driver and describe them within ciport.h.
 *	   Apply these conventions to all names( routine, macro, constant, and
 *	   data structure ) used within the driver.
 *	2. Add support for the CIXCB hardware port type by adding cixcb_regoff(
 *	   register offsets ) and cixcb_mrltab[]( microcode revision table ).
 *	3. Update all microcode revision tables to current support levels.  Add
 *	   self-test revision levels to cibca_aa_rltab[] and cibca_ba_mrltab[]
 *	   so that both functional and self-test revision levels maybe verified
 *	   following the initial port initialization of all CIBX family ports.
 *	4. Rename CF_PORT -> CF_RPORT.
 *	8. Update the console logging tables ci_cltab[][] and ci_clie[] to
 *	   reflect elimination of the informational event IE_INVHPT, the
 *	   renumbering of the remaining informational event codes, and the
 *	   addition of new codes IE_STRAY, IE_POWER, and IE_POWERUP.
 *
 *   02-Jun-1988	Ricky S. Palmer
 *	Removed inclusion of header file ../vaxmsi/msisysap.h
 *
 *   09-Apr-1988	Todd M. Katz		TMK0002
 *	Add support for the CIBCA-BA hardware port type by renaming
 *	cibca_mrltable[] -> cibca_aa_mrltab[] and by adding cibca_ba_mrltab[],
 *	a CIBCA-BA microcode revison table.
 *
 *   08-Jan-1988	Todd M. Katz		TMK0001
 *	Formated module, revised comments, increased generality and
 *	robustness, made CI PPD and GVP completely independent from underlying
 *	port drivers, and added SMP support.
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
#include		"../io/gvp/gvp.h"
#include		"../io/ci/cippd.h"
#include		"../io/ci/ciport.h"

/* External Variables and Routines.
 */
extern	PB		*cippd_get_pb();
extern	SCSH		*gvp_alloc_dg(), *gvp_alloc_msg(), *gvp_remove_dg(),
			*gvp_remove_msg();
extern	u_long		ci_alloc_buf(), ci_get_port(), ci_init_pb(),
			ci_req_data(), ci_send_data(), ci_remote_reset(),
			ci_remote_start(), ci_send_reqid(), ci_set_circuit(),
			cippd_open_pb(), gvp_map_buf();
extern	void		ci_crash_lport(), ci_dealloc_buf(), ci_init_port(),
			ci_inv_cache(), ci_log_badport(), ci_notify_port(),
			ci_shutdown(), ci_test_lpconn(), ci_update_ptype(),
			cippd_crash_pb(), cippd_remove_pb(), gvp_add_dg(),
			gvp_add_msg(), gvp_dealloc_dg(), gvp_dealloc_msg(),
			gvp_send_dg(), gvp_send_msg(), gvp_unmap_buf();

/* Global Variables and Data Structures.
 */
u_char		*ci_black_hole		/* CI Black Hole Mapping Page	     */
		      = NULL;
u_long		ci_bhole_pfn		/* CI Black Hole Page Frame Number   */
		     = 0;
u_short		ci_ucode_type		/* CI Functional Microcode Type	     */
		     = 0;
PDT		ci_pdt	= {		/* CI Port Dispatch Table	     */
					/* Mandatory PD Functions	     */
		    gvp_alloc_dg,	/* Allocate Datagram Buffer	     */
		    gvp_dealloc_dg,	/* Deallocate Datagram Buffer	     */
		    gvp_add_dg,		/* Add Datagram Buffer to Free Pool  */
		    gvp_remove_dg,	/* Remove Dg Buffer from Free Pool   */
		    gvp_alloc_msg,	/* Allocate Message Buffer	     */
		    gvp_dealloc_msg,	/* Deallocate Message Buffer	     */
		    gvp_add_msg,	/* Add Message Buffer to Free Pool   */
		    gvp_remove_msg,	/* Remove Msg Buffer from Free Pool  */
		    gvp_send_msg,	/* Send Sequenced Message	     */
		    gvp_map_buf,	/* Map Buffer			     */
		    gvp_unmap_buf,	/* Unmap Buffer			     */
		    cippd_open_pb,	/* Transition Formative Path to Open */
		    cippd_crash_pb,	/* Crash Path			     */
		    cippd_get_pb,	/* Retrieve Path Block               */
		    cippd_remove_pb,	/* Remove Path Block from Databases  */
		    ci_remote_reset,	/* Reset Remote Port and System	     */
		    ci_remote_start,	/* Start Remote Port and System	     */

					/* Optional PD Functions	     */
		    gvp_send_dg,	/* Send Datagram		     */
		    ci_send_data,	/* Send Block Data		     */
		    ci_req_data,	/* Request Block Data		     */
		    ci_crash_lport,	/* Crash Local Port		     */
		    ci_shutdown,	/* Inform Systems of Local Shutdown  */

					/* Mandatory PD Functions for CI PPD */
		    ci_get_port,	/* Retrieve Port Number from Buffer  */
     ( u_long (*)())ci_init_port,	/* Initialize/Re-initialize a Port   */
     ( u_long (*)())ci_log_badport,	/* Log CI Originating Bad Port Number*/
		    ci_send_reqid,	/* Request Remote Port Identification*/
		    ci_set_circuit,	/* Set Virtual Circuit State - On/Off*/

					/* Optional PD Functions for CI PPD  */
		    ci_alloc_buf,	/* Allocate Emergency Command Packets*/
     ( u_long (*)())ci_dealloc_buf,	/* Deallocate Emergency Command Pkts */
     ( u_long (*)())ci_init_pb,		/* Initialize a Path Block	     */
     ( u_long (*)())ci_inv_cache,	/* Invalidate Port Translation Cache */
     ( u_long (*)())ci_notify_port,	/* Notify Port of CI PPD Activity    */
     ( u_long (*)())ci_test_lpconn,	/* Test Local Port Connectivity	     */
     ( u_long (*)())ci_update_ptype	/* Update Hardware Port Type	     */
};
u_long	*ci780_regoff[] = 	{	/* CI780/CI750 Register Offsets	     */
	    ( u_long * )CI780_CNFR,	/* Configuration register	     */
	    ( u_long * )CI780_MADR,	/* Maintenance address register      */
	    ( u_long * )CI780_MDATR,	/* Maintenance data register	     */
	    ( u_long * )CI780_PMCSR,	/* Port maintenance cntl & status reg*/
	    ( u_long * )CI7B_PSR,	/* Port status register		     */
	    ( u_long * )CI7B_PQBBASE,	/* PQB base register 		     */
	    ( u_long * )CI7B_PCQ0CR,	/* Port command queue 0 control reg  */
	    ( u_long * )CI7B_PCQ1CR,	/* Port command queue 1 control reg  */
	    ( u_long * )CI7B_PCQ2CR,	/* Port command queue 2 control reg  */
	    ( u_long * )CI7B_PCQ3CR,	/* Port command queue 3 control reg  */
	    ( u_long * )CI7B_PSRCR,	/* Port status release control reg   */
	    ( u_long * )CI7B_PECR,	/* Port enable control register      */
	    ( u_long * )CI7B_PDCR,	/* Port disable control register     */
	    ( u_long * )CI7B_PICR,	/* Port initialization control reg   */
	    ( u_long * )CI7B_PDFQCR,	/* Port dg free queue control reg    */
	    ( u_long * )CI7B_PMFQCR,	/* Port msg free queue control reg   */
	    ( u_long * )CI7B_PMTCR,	/* Port maintenance timer control reg*/
	    ( u_long * )CI7B_PFAR,	/* Port failing address register     */
	    ( u_long * )CI7B_PESR,	/* Port error status register	     */
	    ( u_long * )CI7B_PPR,	/* Port parameter register	     */
	    ( u_long * )0	 	/* Port parameter ext. register	     */
};
u_long	*cibci_regoff[] = {		/* CIBCI Register Offsets	     */
	    ( u_long * )CIBCI_CNFR,	/* Configuration register	     */
	    ( u_long * )CIBCI_MADR,	/* Maintenance address register      */
	    ( u_long * )CIBCI_MDATR,	/* Maintenance data register	     */
	    ( u_long * )CIBCI_PMCSR,	/* Port maintenance cntl & status reg*/
	    ( u_long * )CI7B_PSR,	/* Port status register		     */
	    ( u_long * )CI7B_PQBBASE,	/* PQB base register 		     */
	    ( u_long * )CI7B_PCQ0CR,	/* Port command queue 0 control reg  */
	    ( u_long * )CI7B_PCQ1CR,	/* Port command queue 1 control reg  */
	    ( u_long * )CI7B_PCQ2CR,	/* Port command queue 2 control reg  */
	    ( u_long * )CI7B_PCQ3CR,	/* Port command queue 3 control reg  */
	    ( u_long * )CI7B_PSRCR,	/* Port status release control reg   */
	    ( u_long * )CI7B_PECR,	/* Port enable control register      */
	    ( u_long * )CI7B_PDCR,	/* Port disable control register     */
	    ( u_long * )CI7B_PICR,	/* Port initialization control reg   */
	    ( u_long * )CI7B_PDFQCR,	/* Port dg free queue control reg    */
	    ( u_long * )CI7B_PMFQCR,	/* Port msg free queue control reg   */
	    ( u_long * )CI7B_PMTCR,	/* Port maintenance timer control reg*/
	    ( u_long * )CI7B_PFAR,	/* Port failing address register     */
	    ( u_long * )CI7B_PESR,	/* Port error status register	     */
	    ( u_long * )CI7B_PPR,	/* Port parameter register	     */
	    ( u_long * )0	 	/* Port parameter ext. register	     */
};
u_long	*cibca_regoff[] = {		/* CIBCA Register Offsets	     */
	    ( u_long * )0,		/* Configuration register	     */
	    ( u_long * )CIBCA_MADR,	/* Maintenance address register      */
	    ( u_long * )CIBCA_MDATR,	/* Maintenance data register	     */
	    ( u_long * )CIBX_PMCSR,	/* Port maintenance cntl & status reg*/
	    ( u_long * )CIBX_PSR,	/* Port status register		     */
	    ( u_long * )CIBCA_PQBBASE,	/* PQB base register 		     */
	    ( u_long * )CIBX_PCQ0CR,	/* Port command queue 0 control reg  */
	    ( u_long * )CIBX_PCQ1CR,	/* Port command queue 1 control reg  */
	    ( u_long * )CIBX_PCQ2CR,	/* Port command queue 2 control reg  */
	    ( u_long * )CIBX_PCQ3CR,	/* Port command queue 3 control reg  */
	    ( u_long * )CIBX_PSRCR,	/* Port status release control reg   */
	    ( u_long * )CIBX_PECR,	/* Port enable control register      */
	    ( u_long * )CIBX_PDCR,	/* Port disable control register     */
	    ( u_long * )CIBX_PICR,	/* Port initialization control reg   */
	    ( u_long * )CIBX_PDFQCR,	/* Port dg free queue control reg    */
	    ( u_long * )CIBX_PMFQCR,	/* Port msg free queue control reg   */
	    ( u_long * )CIBX_PMTCR,	/* Port maintenance timer control reg*/
	    ( u_long * )CIBCA_PFAR,	/* Port failing address register     */
	    ( u_long * )CIBCA_PESR,	/* Port error status register	     */
	    ( u_long * )CIBCA_PPR,	/* Port parameter register	     */
	    ( u_long * )0	 	/* Port parameter ext. register	     */
};
u_long	*cixcd_regoff[] = {		/* CIXCD Register Offsets	     */
	    ( u_long * )0,		/* Configuration register	     */
	    ( u_long * )0,		/* Maintenance address register      */
	    ( u_long * )0,		/* Maintenance data register	     */
	    ( u_long * )CIXCD_PMCSR,	/* Port maintenance cntl & status reg*/
	    ( u_long * )CIXCD_PSR,	/* Port status register		     */
	    ( u_long * )CIXCD_PQBBASE,	/* PQB base register 		     */
	    ( u_long * )CIXCD_PCQ0CR,	/* Port command queue 0 control reg  */
	    ( u_long * )CIXCD_PCQ1CR,	/* Port command queue 1 control reg  */
	    ( u_long * )CIXCD_PCQ2CR,	/* Port command queue 2 control reg  */
	    ( u_long * )CIXCD_PCQ3CR,	/* Port command queue 3 control reg  */
	    ( u_long * )CIXCD_PSRCR,	/* Port status release control reg   */
	    ( u_long * )CIXCD_PECR,	/* Port enable control register      */
	    ( u_long * )CIXCD_PDCR,	/* Port disable control register     */
	    ( u_long * )CIXCD_PICR,	/* Port initialization control reg   */
	    ( u_long * )CIXCD_PDFQCR,	/* Port dg free queue control reg    */
	    ( u_long * )CIXCD_PMFQCR,	/* Port msg free queue control reg   */
	    ( u_long * )CIXCD_PMTCR,	/* Port maintenance timer control reg*/
	    ( u_long * )CIXCD_PFAR,	/* Port failing address register     */
	    ( u_long * )CIXCD_PESR,	/* Port error status register	     */
	    ( u_long * )CIXCD_PPR,	/* Port parameter register	     */
	    ( u_long * )CIXCD_PPER	/* Port parameter ext. register	     */
};
u_long	*cikmf1_regoff[] = {		/* CIKMF CI Port 1 Register Offsets  */
	    ( u_long * )0,		/* Configuration register	     */
	    ( u_long * )CIKMF_XPCSER1, 	/* XPC port specific error register*/
	    ( u_long * )CIKMF_XPCSTAT1,	/* XPC port status register	     */
	    ( u_long * )CIKMF_PMCSR1,	/* Port maintenance cntl & status reg*/
	    ( u_long * )CIKMF_PSR1,	/* Port status register		     */
	    ( u_long * )CIKMF_PQBBR1,	/* PQB base register 		     */
	    ( u_long * )CIKMF_PCQ0CR1,	/* Port command queue 0 control reg  */
	    ( u_long * )CIKMF_PCQ1CR1,	/* Port command queue 1 control reg  */
	    ( u_long * )CIKMF_PCQ2CR1,	/* Port command queue 2 control reg  */
	    ( u_long * )CIKMF_PCQ3CR1,	/* Port command queue 3 control reg  */
	    ( u_long * )CIKMF_PSRCR1,	/* Port status release control reg   */
	    ( u_long * )CIKMF_PECR1,	/* Port enable control register      */
	    ( u_long * )CIKMF_PDCR1,	/* Port disable control register     */
	    ( u_long * )CIKMF_PICR1,	/* Port initialization control reg   */
	    ( u_long * )CIKMF_PDFQCR1,	/* Port dg free queue control reg    */
	    ( u_long * )CIKMF_PMFQCR1,	/* Port msg free queue control reg   */
	    ( u_long * )CIKMF_PMTCR1,	/* Port maintenance timer control reg*/
	    ( u_long * )CIKMF_PFAR1,	/* Port failing address register     */
	    ( u_long * )CIKMF_PESR1,	/* Port error status register	     */
	    ( u_long * )CIKMF_PPR1,	/* Port parameter register	     */
	    ( u_long * )0		/* Port parameter ext. register	     */
};
u_long	*cikmf2_regoff[] = {		/* CIKMF CI Port 2 Register Offsets  */
	    ( u_long * )0,		/* Configuration register	     */
	    ( u_long * )CIKMF_XPCSER2, 	/* XPC port specific error register*/
	    ( u_long * )CIKMF_XPCSTAT2,	/* XPC port status register	     */
	    ( u_long * )CIKMF_PMCSR2,	/* Port maintenance cntl & status reg*/
	    ( u_long * )CIKMF_PSR2,	/* Port status register		     */
	    ( u_long * )CIKMF_PQBBR2,	/* PQB base register 		     */
	    ( u_long * )CIKMF_PCQ0CR2,	/* Port command queue 0 control reg  */
	    ( u_long * )CIKMF_PCQ1CR2,	/* Port command queue 1 control reg  */
	    ( u_long * )CIKMF_PCQ2CR2,	/* Port command queue 2 control reg  */
	    ( u_long * )CIKMF_PCQ3CR2,	/* Port command queue 3 control reg  */
	    ( u_long * )CIKMF_PSRCR2,	/* Port status release control reg   */
	    ( u_long * )CIKMF_PECR2,	/* Port enable control register      */
	    ( u_long * )CIKMF_PDCR2,	/* Port disable control register     */
	    ( u_long * )CIKMF_PICR2,	/* Port initialization control reg   */
	    ( u_long * )CIKMF_PDFQCR2,	/* Port dg free queue control reg    */
	    ( u_long * )CIKMF_PMFQCR2,	/* Port msg free queue control reg   */
	    ( u_long * )CIKMF_PMTCR2,	/* Port maintenance timer control reg*/
	    ( u_long * )CIKMF_PFAR2,	/* Port failing address register     */
	    ( u_long * )CIKMF_PESR2,	/* Port error status register	     */
	    ( u_long * )CIKMF_PPR2,	/* Port parameter register	     */
	    ( u_long * )0		/* Port parameter ext. register	     */
};
u_long	*cishc_regoff[] = {		/* CISHC Register Offsets  */
	    ( u_long * )0,		/* Configuration register	     */
	    ( u_long * )0,		/* Maintenance address register      */
	    ( u_long * )0,		/* Maintenance data register	     */
	    ( u_long * )CISHC_PMCSR,	/* Port maintenance cntl & status reg*/
	    ( u_long * )CISHC_PSR,	/* Port status register		     */
	    ( u_long * )CISHC_PQBBR,	/* PQB base register 		     */
	    ( u_long * )CISHC_PCQ0CR,	/* Port command queue 0 control reg  */
	    ( u_long * )CISHC_PCQ1CR,	/* Port command queue 1 control reg  */
	    ( u_long * )CISHC_PCQ2CR,	/* Port command queue 2 control reg  */
	    ( u_long * )CISHC_PCQ3CR,	/* Port command queue 3 control reg  */
	    ( u_long * )CISHC_PSRCR,	/* Port status release control reg   */
	    ( u_long * )CISHC_PECR,	/* Port enable control register      */
	    ( u_long * )CISHC_PDCR,	/* Port disable control register     */
	    ( u_long * )CISHC_PICR,	/* Port initialization control reg   */
	    ( u_long * )CISHC_PDFQCR,	/* Port dg free queue control reg    */
	    ( u_long * )CISHC_PMFQCR,	/* Port msg free queue control reg   */
	    ( u_long * )CISHC_PMTCR,	/* Port maintenance timer control reg*/
	    ( u_long * )CISHC_PFAR,	/* Port failing address register     */
	    ( u_long * )CISHC_PESR,	/* Port error status register	     */
	    ( u_long * )CISHC_PPR,	/* Port parameter register	     */
	    ( u_long * )0		/* Port parameter ext. register	     */
};
static CLFTAB				/* Console Logging Formating Tables  */
/* NULL entries represent events which should never be logged by the CI port
 * driver.
 */
		ci_cli[] = {		/* CI Informational Event Table	     */
    { CF_RPORT,  "cable a: transitioned from bad to good" },
    { CF_RPORT,  "cable b: transitioned from bad to good" },
    { CF_RPORT,  "cables: transitioned from crossed to uncrossed" },
    { CF_LPORT,  "cable a: connectivity transitioned from bad to good" },
    { CF_LPORT,  "cable b: connectivity transitioned from bad to good" },
    { CF_INIT,   "port initialized" },
    { CF_LPORT,  "port initialized" },
    { CF_LPORT,  "port recovered power" }
},
		ci_clw[] = {		/* CI Warning Event Table	     */
    { CF_RPORT,  "cable a: transitioned from good to bad" },
    { CF_RPORT,  "cable b: transitioned from good to bad" },
    { CF_RPORT,  "cables: transitioned from uncrossed to crossed" },
    { CF_LPORT,  "cable a: connectivity transitioned from good to bad" },
    { CF_LPORT,  "cable b: connectivity transitioned from good to bad" },
    { CF_UCODE,  "port microcode: not at supported revision level" },
    { CF_LPORT,  "port lost power" },
    { CF_LPORT,	 "stray interrupt" }
},
		ci_clre[] = {		/* CI Remote Error Event Table	     */
    { CF_RPORT,  "remote port: state precludes communication" }
},
		ci_cle[] = {		/* CI Error Event Table		     */
    { CF_NONE,   "port initialization: insufficient memory" },
    { CF_NONE,   "port initialization: microcode verification error" },
    { CF_REGS,   "port initialization: unable to start port" },
    { CF_NONE,   "cables: all failed" },
    { CF_PKT,    "received message: virtual circuit closed" },
    { CF_NONE,   "remote port: invalid state" }
},
		ci_clse[] = {		/* CI Severe Error Event Table	     */
    { CF_PKT,    "block data transfer: buffer memory system error" },
    { CF_PKT,    "remote packet: invalid size" },
    { CF_PKT,    "remote packet: unrecognized port operation code" },
    { CF_PKT,    "received message: out-of-sequence" },
    { CF_PKT,    "local/remote packet: invalid destination/source" },
    { CF_PKT,    "local packet: invalid local buffer name" },
    { CF_PKT,    "local packet: invalid local buffer length" },
    { CF_PKT,    "block data transfer: local buffer access violation" },
    { CF_PKT,    "local packet: invalid size" },
    { CF_PKT,    "local packet: invalid destination" },
    { CF_PKT,    "local packet: unknown port command" },
    { CF_PKT,    "local packet: aborted port command" },
    { CF_PKT,    "local packet: unknown status" },
    { CF_PKT,    "local packet: unknown port operation code" },
    { CF_PKT,    "local packet: invalid port operation code" },
    { CF_REGS,   "port lost power" },
    { CF_REGS,   "port lost and recovered power" },
    { CF_NONE,   "port command queue 0: insertion failure" },
    { CF_NONE,   "port command queue 1: insertion failure" },
    { CF_NONE,   "port command queue 2: insertion failure" },
    { CF_NONE,   "port command queue 3: insertion failure" },
    { CF_NONE,   "port datagram free queue: insertion failure"},
    { CF_NONE,   "port message free queue: insertion failure" },
    { CF_NONE,   "port response queue: removal failure" },
    { CF_NONE,   "port datagram free queue: removal failure" },
    { CF_NONE,   "port message free queue: removal failure" },
    { CF_REGS,   "memory system error occurred" },
    { CF_BIREGS, "bi memory system error occurred" },
    { CF_REGS2,  "port data structure error occurred" },
    { CF_REGS,   "port parity error occurred" },
    { CF_BIREGS, "bi parity error occurred" },
    { CF_REGS2,  "port error bit( s ) set" },
    { CF_BIREGS, "bi error bit( s ) set" },
    { CF_REGS,   "port sanity timer expired" },
    { CF_REGS,   "port message free queue: exhausted" },
    { CF_PKT,    "received packet: invalid port address" },
    { CF_PKT,    "received packet: invalid sequence number" },
    { CF_PKT,    "received packet: invalid port datalink address" },
    { CF_PKT,    "set circuit: insufficient VCD entries" },
    { CF_PKT,    "set circuit: insufficient RESEQ resources" },
    { CF_PKT,    "received packet: discarded VC packet" }
},
		ci_clfe[] = {		/* CI Fatal Error Event Table	     */
    { CF_NONE,   "port initialization: insufficient memory" },
    { CF_NONE,   "scs_system_id has not been set to a non-zero value" },
    { CF_NONE,   "functional microcode: unable to locate" },
    { CF_NONE,   "hardware port type: unknown" },
    { CF_NONE,   "hardware port type: functional microcode does not match" },
    { CF_NONE,   "port initialization: microcode verification error" },
    { CF_REGS,   "port initialization: unable to start port" },
    { CF_CPU,    "cpu microcode: not at required revision levels" },
    { CF_REGS,   "port is permanently unavailable" },
    { CF_PPR,    "port recognizes invalid maximum port number" },
    { CF_UCODE,  "port microcode: invalid revision level" },
    { CF_REGS2,  "port error bit( s ) set" }
},
		ci_clppdse[] = {	/* CI PPD Severe Error Event Table   */
    { CF_NONE,	 NULL },
    { CF_NONE,	 NULL },
    { CF_NONE,   "attempt made to crash nonexistent path" },
    { CF_NONE,	 "port failed ci ppd sanity check" },
    { CF_NONE,	 NULL }
};
CLSTAB					/* CI Console Logging Table	     */
	ci_cltab[ ES_FE + 1 ][ ESC_PPD + 1 ] = {
					/* Severity == ES_I		     */
    {  8,	ci_cli     },		/*     Subclass == ESC_PD( CI )      */
    {  0,	NULL	   },		/*     Subclass == ESC_PPD( CI PPD ) */
					/* Severity == ES_W		     */
    {  8,	ci_clw     },		/*     Subclass == ESC_PD( CI )      */
    {  0,	NULL	   },		/*     Subclass == ESC_PPD( CI PPD ) */
					/* Severity == ES_RE		     */
    {  1,	ci_clre    },		/*     Subclass == ESC_PD( CI )      */
    {  0,	NULL	   },		/*     Subclass == ESC_PPD( CI PPD ) */
					/* Severity == ES_E		     */
    {  6,	ci_cle     },		/*     Subclass == ESC_PD( CI )      */
    {  0,	NULL	   },		/*     Subclass == ESC_PPD( CI PPD ) */
					/* Severity == ES_SE		     */
    { 41,	ci_clse    },		/*     Subclass == ESC_PD( CI )      */
    {  5,	ci_clppdse },		/*     Subclass == ESC_PPD( CI PPD ) */
					/* Severity == ES_FE		     */
    { 12,	ci_clfe    },		/*     Subclass == ESC_PD( CI )      */
    {  0,	NULL       },		/*     Subclass == ESC_PPD( CI PPD ) */
};
u_long		ci_crctable[] = {	/* CI Loopback CRC Table	     */
    0, 0x1DB71064, 0x3B6E20C8, 0x26D930AC, 0x76DC4190, 0x6B6B51F4, 0x4DB26158,
    0x5005713C, 0xEDB88320, 0xF00F9344, 0xD6D6A3E8, 0xCB61B38C, 0x9B64C2B0,
    0x86D3D2D4, 0xA00AE278, 0xBDBDF21C
};

					/* CI750/CI780/CIBCI Microcode	     */
MRLTAB		ci7b_mrltable[] = {	/*  Revision Table		     */
/* Additions to this table may require appropriate updating of CI7B_MAX_RAM
 * and CI7B_MAX_ROM.
 */
	{  2,  2, UCODE_WARNING   },	/* RAM:  2  PROM:  2  - WARNING	     */
	{  3,  3, UCODE_WARNING   },	/* RAM:  3  PROM:  3  - WARNING	     */
	{  4,  3, UCODE_WARNING   },	/* RAM:  4  PROM:  3  - WARNING	     */
	{  5,  3, UCODE_WARNING   },	/* RAM:  5  PROM:  3  - WARNING	     */
	{  7,  7, UCODE_NOWARNING },	/* RAM:  7  PROM:  7  - NO WARNING   */
	{  8,  7, UCODE_NOWARNING },	/* RAM:  8  PROM:  7  - NO WARNING   */
	{ 32, 32, UCODE_NOWARNING },	/* RAM: 32  PROM: 32  - NO WARNING   */
	{  0,  0, 0		  }
},
		cibca_aa_mrltab[] = {	/* CIBCA-AA Microcode Revision Table */
/* Additions to this table may require appropriate updating of CIBCA_AA_MAXST
 * and CIBCA_AA_MAXFN.  When updating this table with a new functional
 * microcode revision level, add entries for each and every known self-test
 * microcode revision level.  In other words, when adding functional microcode
 * revision level n, add entries for( functional/self-test ): n/2, n/3, etc....
 * If this is not done, a fatal error is detected(  invalid microcode revision
 * level ) during local port initialization and the port is taken permanently
 * offline.  This is a rather severe penalty to pay for failing to update the
 * on-board EEPROM with new self-test microcode, especially as the port is
 * fully usable.
 */
	{  3,  2, UCODE_WARNING	  },	/* Fn:   3  St:    2  - WARNING	     */
	{  4,  2, UCODE_WARNING   },	/* Fn:   4  St:    2  - WARNING	     */
	{  5,  2, UCODE_WARNING   },	/* Fn:   5  St:    2  - WARNING	     */
	{  3,  3, UCODE_WARNING	  },	/* Fn:   3  St:    3  - WARNING	     */
	{  4,  3, UCODE_WARNING   },	/* Fn:   4  St:    3  - WARNING	     */
	{  5,  3, UCODE_WARNING   },	/* Fn:   5  St:    3  - WARNING      */
	{  6,  3, UCODE_WARNING   },	/* Fn:   6  St:    3  - WARNING      */
	{  7,  3, UCODE_WARNING   },	/* Fn:   7  St:    3  - WARNING      */
	{  5,  4, UCODE_NOWARNING },	/* Fn:   5  St:    4  - NO WARNING   */
	{  6,  4, UCODE_NOWARNING },	/* Fn:   6  St:    4  - NO WARNING   */
	{  7,  4, UCODE_NOWARNING },	/* Fn:   7  St:    4  - NO WARNING   */
	{  5,  5, UCODE_NOWARNING },	/* Fn:   5  St:    5  - NO WARNING   */
	{  6,  5, UCODE_NOWARNING },	/* Fn:   6  St:    5  - NO WARNING   */
	{  7,  5, UCODE_NOWARNING },	/* Fn:   7  St:    5  - NO WARNING   */
	{  0,  0, 0		  }
},
		cibca_ba_mrltab[] = {	/* CIBCA-BA Microcode Revision Table */
/* Additions to this table may require appropriate updating of CIBCA_BA_MAXST
 * and CIBCA_BA_MAXFN.  When updating this table with a new functional
 * microcode revision level, add entries for each and every known self-test
 * microcode revision level.  In other words, when adding functional microcode
 * revision level n, add entries for( functional/self-test ): n/2, n/3, etc....
 * If this is not done, a fatal error is detected(  invalid microcode revision
 * level ) during local port initialization and the port is taken permanently
 * offline.  This is a rather severe penalty to pay for failing to update the
 * on-board EEPROM with new self-test microcode, especially as the port is
 * fully usable.
 */
	{  1,  1, UCODE_NOWARNING },	/* Fn:   1  St:    1  - NO WARNING   */
	{  0,  0, 0		  }
},
		cixcd_mrltable[] = {	/* CIXCD Microcode Revision Table    */
/* Additions to this table may require appropriate updating of CIXCD_MAXST
 * and CIXCD_MAXFN.  When updating this table with a new functional
 * microcode revision level, add entries for each and every known self-test
 * microcode revision level.  In other words, when adding functional microcode
 * revision level n, add entries for( functional/self-test ): n/2, n/3, etc....
 * If this is not done, a fatal error is detected(  invalid microcode revision
 * level ) during local port initialization and the port is taken permanently
 * offline.  This is a rather severe penalty to pay for failing to update the
 * on-board EEPROM with new self-test microcode, especially as the port is
 * fully usable.
 */
	{  1,  1, UCODE_NOWARNING },	/* Fn:   1  St:    1  - NO WARNING   */
	{  0,  0, 0		  }
},
		cikmf_mrltable[] = {	/* CIKMF Microcode Revision Table    */
	{  1,  1, UCODE_NOWARNING },	/* Fn:   1  St:    1  - NO WARNING   */
	{  0,  0, 0		  }
},
		cishc_mrltable[] = {	/* CISHC Microcode Revision Table    */
	{  1,  1, UCODE_NOWARNING },	/* Fn:   1  St:    1  - NO WARNING   */
	{  0,  0, 0		  }
};
