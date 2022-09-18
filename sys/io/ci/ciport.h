/*
 *	@(#)ciport.h	4.3	(ULTRIX)	12/20/90
 */

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
 *   Abstract:	This module contains Computer Interconnect Port Driver( CI )
 *		constants, data structure definitions, port command
 *		definitions, register definitions, and macros.
 *
 *   Creator:	Todd M. Katz	Creation Date:	April 22, 1985
 *
 *   Modification History:
 *
 *   19-Dec-1990	Pete Keilty
 *	1. Added support for NADP bit in Exp_mask, setckt, fcn_ext2 register
 *	   and new macro NADP_MASK( value ).
 *	2. Added new define CIXCD_XBE_TTO.
 *	3. Added 2 u_shorts to SETCKT/CKTSET structure for REV H of CI PORT.
 *	   VCD and RESEQ resources lower bounds.
 *
 *   16-Oct-1990	Pete Keilty
 *	1. Removed lk_cidevice, now define in cipccb structure cidevice_lk.
 *	2. Added locking around register accesses in the wait macros.
 *	3. The above was done beccuase of the CIXCD XMOV bug.
 *
 *   15-Jun-1990	Pete Keilty
 *	Added temporary smp lock lk_cidevice for CIXCD because of XMOV bug.
 *
 *   06-Jun-1990	Pete Keilty
 *	1. Added six new severity codes for CIXCD, they are SE_INVPA,
 *	   SE_INVSN, SE_INVDDL, SE_IRESVCD, SE_IRESEQ, SE_DISCVCPKT.
 *	2. Added perliminary support for CIKMF and SHAC.
 *
 *   06-Feb-1990	Pete Keilty
 *	Moved Get_pgrp macro here from gvp.h.
 *
 *   08-Dec-1989	Pete Keilty
 *	1. Changed a few XCD register offsets that where wrong.
 *	2. Added two new macros Cibx_wait_unin() & Cibx_wait_pic_clear()
 *	   used in cibx_start() routine.
 *
 *   19-Sep-1989	Pete Keilty
 *	1. Add XCD support, remove XCB.
 *	2. Add new Explicit command addressing ( Subnode ECO ).
 *
 *   17-Jan-1989	Todd M. Katz		TMK0007
 *	1. The macro Scaaddr_lol() has been renamed to Scaaddr_low().  It now
 *	   accesses only the low order word( instead of low order longword ) of
 *	   a SCA system address.
 *	2. Modify macros Ci7b_wait_mif() and Cibx_wait_mif().  Change the
 *	   manual interrupt checking interval from 1 msec to 10 msec to conform
 *	   to the granularity of the clock.
 *
 *   22-Sep-1988	Todd M. Katz		TMK0006
 *	1. Delete the console formatting code CF_RPORT( Remote port station
 *	   address logged ).  It is now defined by the CI PPD.
 *	2. Console log SE_POWER severe error events depending upon the current
 *	   CI severity level.  Formerly, such events were always logged to the
 *	   console irregardless of the current CI severity level.
 *
 *   17-Aug-1988	Todd M. Katz		TMK0005
 *	 1. SCA event codes have been completely revised.  Former CI path crash
 *	    codes are now divided into error events( NOCABLES, CLOSEDVC,
 *	    RPORTSTATE ) and severe error events( BMSE, INVRPKTSIZE, UNRECPKT,
 *	    OSEQMSG ).  Former CI local port crash codes are now divided into
 *	    severe error events( POWER, POWERUP, INVBNAME, INVBSIZE, BACCVIO,
 *	    INVLPKTSIZE, INVDPORT, UNKCMD, ABORTPKT, UNKSTATUS, UNKOPCODE,
 *	    INVOPCODE, MSI, BIMSE, DSE, PARITY, BIPARITY, PORTERROR, BIERROR,
 *	    SANITYTIMER, MFQE ) and fatal error events( BADMAXPORT, BADUCODE,
 *	    NOCI ).  The local port crash severity modifier is applied by
 *	    ci_crash_lport().
 *	 2. Redefine the following informational events( ES_I ) as warning
 *	    events( ES_W ): CABLE0_GB, CABLE1_GB, CABLES_UC, CABLE0_LBGB,
 *	    CABLE1_LBGB, UCODE_WARN, POWER, STRAY.
 *	 3. Redefine the following informational events( ES_I ) as remote error
 *	    events( ES_RE ): RPORT_STATE.
 *	 4. Redefine the following informational events( ES_I ) as error
 *	    events( ES_E ): UCODE_LOAD, UCODE_START.
 *	 5. Redefine the following informational events( ES_I ) as fatal error
 *	    events( ES_FE ): INIT_NOUCODE, INIT_CPU, INIT_NOMEM, INIT_ZEROID,
 *	    INIT_UNKHPT, INIT_MISMTCH, NOCI.
 *	 6. GVP event codes no longer occupy their own name space.  They now
 *	    occupy part of and are defined with each ( appropriate ) port
 *	    drivers' name spaces.  Therefore, add the following former GVP
 *	    severe error event codes: ICMDQ0, ICMDQ1, ICMDQ2, ICMDQ3, IDFREEQ,
 *	    IMFREEQ, RRSPQ, RDFREEQ, RMFREEQ.
 *	 7. Define the informational events: LPORT_INIT and LPORT_REINIT, the
 *	    error event: NOMEM, the severe error event: BADPORTNUM, and the
 *	    fatal error event: PORTERROR.
 *	 8. Rename the informational condition event mnemonic from IE -> I.
 *	 9. Eliminate one of the reserved CI special console logging format
 *	    codes and re-number the remaining codes.  Add CF_LPORT( local port
 *	    station address ), CF_CPU( CPU revision level ), and CF_INIT(
 *	    initial local port initialization information ) as console logging
 *	    format codes.
 *	10. Rename FE_INIT_CPU -> FE_CPU.
 *	11. Add macros Elcidattn(), and Elcicpurevlev(), delete macro Elci(),
 *	    and update macros Elciciregs(), Elcibiregs(), Elcixmiregs(),
 *	    Elcirevlev(), and Elciucode().
 *
 *   23-Apr-1988	Todd M. Katz		TMK0004
 *	 1. Create a single unified hierarchical set of naming conventions for
 *	    use within the CI port driver and describe them.  Apply these
 *	    conventions to all names( routine, macro, constant, and data
 *	    structure ) used within the driver.  Restructure the driver to
 *	    segregate most CI family and port type specific code into separate
 *	    routines.  Such restructuring requires creation of new PCCB fields
 *	    and the corresponding shorthand notations( macros ) to refer to
 *	    them including: Mrltab, Dg_cache, Msg_cache, Start_port,
 *	    Disable_port, Load_ucode, Max_fn_level, Max_rom_level.
 *	 2. Add support for the CIXCB hardware port type by defining
 *	    appropriate constants and macros.
 *	 3. Add macros Cibx_onboard(), Ci7b_allram(), Ci7b_wait_mif(), and
 *	    Cibx_wait_mif() and shorthand notations Lpstatus, Pstatus, and
 *	    Rport_state; and, remove macros Flush_rspq(), Init_dattnopt(), and
 *	    Store_reg() and shorthand notation Bierr_int.
 *	 4. Create cache size constants for each CI family as local port
 *	    internal free queue cache size varies from family to family.
 *	 5. Move the CIBCA Device Type Register Port Revision Field Mask Bits
 *	    constants to ciadapter.h and remove the Adapter Type Code
 *	    constants.
 *	 6. Update the current maximum RAM/functional and PROM/self-test
 *	    microcode revision levels for all hardware port types.
 *	 7. Rename CF_PORT -> CF_RPORT and renumber all console logging format
 *	    codes to reflect additions to format codes used by the CI PPD.
 *	 8. Eliminate the informational condition IE_INVHPT, renumber the
 *	    remaining informational condition codes, and add the informational
 *	    conditions IE_STRAY, IE_POWER, and IE_POWERUP.  Also add the panic
 *	    PANIC_BADUNMAP.
 *	 9. Add the Local Port Mapping Constants.
 *	10. Update many of the error log macros referencing various CI error
 *	    log packet fields to reflect changes in various field names.  Also,
 *	    add macros Elcirevlev(), Elcipacket(), and Elciucode() while
 *	    removing macros Elcidattn(), Elcilpktopt(), and Elcilpkt().
 *
 *   02-Apr-1988	Todd M. Katz		TMK0003
 *	1. Add support for the CIBCA-BA hardware port type and for onboard
 *	   CIBCA-BA port microcode.  Differentiate CIBCA-BA from CIBCA-AA
 *	   hardware ports when necessary; otherwise, refer to both types as
 *	   just CIBCA ports.
 *	2. Delete the definition CI_CSZ_224.  It is an illegal cluster size.
 *	3. Rename:
 *	   1) MIN_CPU750_REV -> MIN_C750_REV
 *	   2) CIBCA_RAM_ADDR -> CIBCA_FN_ADDR
 *	   3) CIBCA_ROM_ADDR -> CIBCA_ST_ADDR
 *	   4) CIBCA_AA_MAXRAM -> CIBCA_AA_MAXFN
 *	   5) CIBCA_BA_MAXRAM -> CIBCA_BA_MAXFN
 *	   6) Cibca_ram_level() -> Cibca_fn_level()
 *	   7) Cibca_rom_level() -> Cibca_st_level()
 *	4. Add the definitions CIPANIC_REQLPC and CIPANIC_ONBOARD.
 *	5. Change the definitions CI_DG_CACHE and CI_MSG_CACHE from 3 -> 4.
 *	   This forces 4 datagrams and 4 messages to be allocated and inserted
 *	   onto the appropriate local port free queue whenever the port is
 *	   initialized.
 *	6. Add macro Elcicommon().
 *
 *   16-Mar-1988	Todd M. Katz		TMK0002
 *	Resolved a problem with the macro Cibca_ucode() which was incorrectly
 *	determining whether or not a given instance of CI port microcode was
 *	of a type appropriate for use by a CIBCA local CI port.
 *
 *   08-Jan-1988	Todd M. Katz		TMK0001
 *	Formated module, revised comments, increased generality and
 *	robustness, made CI PPD and GVP completely independent from underlying
 *	port drivers, and added SMP support.
 */

/* CI Port Driver Naming Conventions.
 *
 * There are currently six CI hardware port types.  All six types are
 * sufficiently related to one another to allow their control by a single port
 * driver.  Differences between the types also exist.  Some of these
 * differences are trivial and may be coped with through appropriate special
 * casing.  Others are not so minor and special casing is not sufficient in and
 * of itself to handle them.
 *
 * Major differences between hardware port types are ultimately based upon
 * discrepancies in port register formats.  Fortunately, not all types possess
 * unique register formats.  This allows the six hardware port types to be
 * grouped into two families based upon shared register formats:
 *
 * 1. The CI7B family consisting of CI750, CI780, and CIBCI hardware port
 *    types.
 * 2. The CIBX family consisting of CIBCA-AA, CIBCA-BA, and CIXCD hardware port
 *    types.
 *
 * It is this grouping into families which provides the basis for the CI port
 * driver naming conventions.  These naming conventions affect routine, macro,
 * constant, and data structure names.  They are listed below in the form of a
 * set of rules:
 *
 * 1. Global names( routines ) pertaining to all hardware ports are prefixed
 *    with "ci_" as in the routine ci_probe().
 * 2. Local names( constants, data structures, macros ) pertaining to all
 *    hardware ports have no specific prefix as in the constant MAX_CABLES, the
 *    macro Comp_loopback(), and the data structure VCD.
 * 3. Names pertaining to a specific family are prefaced with the family name
 *    as in the routine ci7b_load(), the constant CI7B_DG_CACHE, the macro
 *    Ci7b_allram(), and the data structure CI7B_UCODEH.
 * 4. Names pertaining to a specific hardware port type are are prefaced with
 *    the specific port type itself as in the routine cibca_aa_load(), and the
 *    constant CIXCD_MAXFN.
 *
 * There are two exceptions to rule 4 based upon hardware port types so
 * identical that the CI port driver almost never needs to distinguish between
 * them.  The CI750 and CI780 are such a pair of identical ports.  Routines,
 * data structures, and constants pertaining to them use the prefixes "ci780_",
 * "CI780_", and "CI780_" respectively when no need exists to distinguish
 * between them.  The CIBCA-AA and CIBCA-BA are another pair of nearly
 * identical ports.  They use the prefixes "cibca_" "CIBCA_", and "CIBCA_" when
 * no need to distinguish between them exists.
 */

/* CI Constants.
 */
					/* Cable Update Type Codes	     */
#define	CABLE_GB		 1	/* Check good->bad transitions	     */
#define	CABLE_BG		 2	/* Check bad->good transitions	     */
#define	CABLE_CROSSED		 3	/* Check crossing transitions	     */
#define	CABLE_LB_GB		 4	/* Chk loopback good->bad transitions*/
#define	CABLE_LB_BG		 5	/* Chk loopback bad->good transitions*/

					/* Cluster size definitions	     */
#define	CSZ_8		 	 8	/* Cluster size = 8 systems CIKMF    */
#define	CSZ_16			16	/* Cluster size = 16 systems	     */
#define	CSZ_32			32	/* Cluster size = 32 systems	     */
#define	CSZ_64			64	/* Cluster size = 64 systems	     */
#define	CSZ_128		       128	/* Cluster size = 128 systems	     */

					/* Console Logging Format Codes	     */
				 	/* 0 - No optional information logged*/
				 	/* 1 - CI PPD special format code    */
				 	/* 2 - CI PPD special format code    */
				 	/* 3 - CI PPD special format code    */
				 	/* 4 - CI PPD special format code    */
				 	/* 5 - Rem port station addr logged  */
#define	CF_LPORT		 6	/* Local port station address logged */
#define	CF_PPR			 7	/* PPR port register logged	     */
#define	CF_REGS			 8	/* Port registers logged	     */
#define	CF_REGS2		 9	/* Port registers logged	     */
#define	CF_BIREGS		10	/* BIIC registers logged	     */
#define	CF_PKT			11	/* CI packet fields logged	     */
#define	CF_UCODE		12	/* Microcode revision levels logged  */
#define	CF_CPU			13	/* CPU revision levels logged	     */
#define	CF_INIT			14	/* Initial lport init info logged    */

					/* Device Attention Logging Modifiers*/
#define	LOG_NOREGS		 0	/* Do not log device registers       */
#define	LOG_REGS		 1	/* Log device registers		     */

					/* CI Informational Event Codes	     */
/* CI informational events are either local or remote port specific.  There are
 * currently no path specific CI informational events.  ci_console_log(),
 * ci_cli[], and ci_cltab[][] must be updated to reflect new additions.
 * ci_log_dev_attn() must be updated when a new local port specific CI
 * informational event is added.  ci_log_packet() must be updated when a new
 * remote port specific CI informational event is added.
 *
 * The following CI informational events are local port specific but may NOT
 * have the local port crash severity modifier( ESM_LPC ) applied:
 *
 *	LPORT_INIT, LPORT_REINIT, POWERUP
 *
 * The following CI informational events are remote port specific:
 *
 *	CABLE0_BG, CABLE1_BG, CABLES_CU, CABLE0_LBBG, CABLE1_LBBG
 *
 * NOTE: Local port specific CI informational events are never candidates for
 *	 application of the local port crash severity modifier( ESM_LPC ).
 *
 * NOTE: CABLE0_LBBG, CABLE1_LBBG are classified as remote port specific CI
 *	 informational events even though the identity of the remote port is
 *	 actually the local port.
 *
 * NOTE: Choose an appropriate console formatting code( CF ) which includes
 *	 displaying of the local port station address when updating ci_cli[]
 *	 with a new local port specific CI informational event.  Choose one
 *	 which includes displaying of the remote port station address when
 *	 updating ci_cli[] with a new remote port specific CI informational
 *	 event.
 */
#define	I_CABLE0_BG	( PDI | 0x01 )	/* Cable 0 bad->good transition	     */
#define	I_CABLE1_BG	( PDI | 0x02 )	/* Cable 1 bad->good transition	     */
#define	I_CABLES_CU	( PDI | 0x03 )	/* Cables crossed->uncrossed	     */
#define	I_CABLE0_LBBG	( PDI | 0x04 )	/* Loopback Cable 0 bad->good transit*/
#define	I_CABLE1_LBBG	( PDI | 0x05 )	/* Loopback Cable 1 bad->good transit*/
#define	I_LPORT_INIT	( PDI | 0x06 )	/* Initial local port initialization */
#define	I_LPORT_REINIT	( PDI | 0x07 )	/* Local CI port initialization	     */
#define	I_POWERUP	( PDI | 0x08 )	/* Power restored to local port	     */

					/* CI Warning Event Codes	     */
/* CI warning events are either local or remote port specific.  There are
 * currently no path specific CI warning events.  ci_console_log(),
 * ci_cltab[][], and ci_clw[] must be updated to reflect new additions.
 * ci_log_dev_attn() must be updated when a new local port specific CI warning
 * event is added.  ci_log_packet() must be updated when a new remote port
 * specific CI warning event is added.
 *
 * The following CI warning events are local port specific but may NOT have the
 * local port crash severity modifier( ESM_LPC ) applied:
 *
 *	UCODE_WARN, POWER, STRAY
 *
 * The following CI warning events are remote port specific:
 *
 *	CABLE0_GB, CABLE1_GB, CABLES_UC, CABLE0_LBGB, CABLE1_LBGB
 *
 * NOTE: Local port specific CI warning events are never candidates for
 *	 application of the local port crash severity modifier( ESM_LPC ).
 *
 * NOTE: CABLE0_LBGB, CABLE1_LBGB are classified as remote port specific CI
 *	 warning events even though the identity of the remote port is actually
 *	 the local port.
 *
 * NOTE: Choose an appropriate console formatting code( CF ) which includes
 *	 displaying of the local port station address when updating ci_clw[]
 *	 with a new local port specific CI warning event.  Choose one which
 *	 includes displaying of the remote port station address when updating
 *	 ci_clw[] with a new remote port specific CI warning event.
 */
#define	W_CABLE0_GB	( PDW  | 0x01 )	/* Cable 0 good->bad transition	     */
#define	W_CABLE1_GB	( PDW  | 0x02 )	/* Cable 1 good->bad transition	     */
#define	W_CABLES_UC	( PDW  | 0x03 )	/* Cables uncrossed->crossed	     */
#define	W_CABLE0_LBGB	( PDW  | 0x04 )	/* Loopback Cable 0 good->bad transit*/
#define	W_CABLE1_LBGB	( PDW  | 0x05 )	/* Loopback Cable 1 good->bad transit*/
#define	W_UCODE_WARN	( PDAW | 0x06 )	/* Out-of-revision microcode	     */
#define	W_POWER		( PDW  | 0x07 )	/* Power failure occurred	     */
#define	W_STRAY		( PDW  | 0x08 )	/* Stray interrupt received	     */

					/* CI Remote Error Event Codes	     */
/* CI remote error events are always remote port specific.  There are currently
 * no local port or path specific CI remote error events.  ci_console_log(),
 * ci_log_packet(), ci_clre[], and ci_cltab[][] must be updated to reflect new
 * additions.
 *
 * The following CI remote error events are remote port specific:
 *
 *	RPORT_STATE
 *
 * NOTE: Choose an appropriate console formatting code( CF ) which includes 
 *	 displaying of the remote port station address when updating ci_clre[]
 *	 with a new CI remote error event.
 */
#define	RE_RPORT_STATE	( PDRE | 0x01 )	/* Improper remote port state	     */

					/* CI Error Event Codes		     */
/* CI error events are either local port or path specific.  There are
 * currently no remote port specific CI error events.  ci_console_log(),
 * ci_cle[], and ci_cltab[][] must be updated to reflect new additions.
 * ci_log_dev_attn() must be updated when a new local port specific CI error
 * event is added.  ci_log_packet() must be updated when a new path specific CI
 * error event is added.
 *
 * The following CI error events are local port specific but may NOT have the
 * local port crash severity modifier( ESM_LPC ) applied:
 *
 *	NOMEM, UCODE_LOAD, UCODE_START
 *
 * The following CI error events are path specific but may NOT have the path
 * crash severity modifier( ESM_PC ) applied:
 *
 *	NOCABLES, CLOSEDVCD, RPORTSTATE
 *
 * Path specific information is always displayed by default during console
 * logging of path specific CI error events.  The local port station address is
 * always displayed by default during console logging of local port specific CI
 * error events.
 *
 * NOTE: Path and local port specific CI error events are NEVER candidates for
 *	 application of the path( ESM_PC ) or local port( ESM_LPC ) crash
 *	 severity modifiers.
 *
 * NOTE: When updating ci_cle[] with a new CI error event bear in mind the
 *	 information displayed by default when choosing an appropriate console
 *	 formatting code( CF ).
 */
#define	E_NOMEM		( PDE | 0x01 )	/* Insufficient memory to init port  */
#define	E_UCODE_LOAD	( PDE | 0x02 )	/* Unable to verify microcode load   */
#define	E_UCODE_START	( PDE | 0x03 )	/* Unable to start microcode	     */
#define	E_NOCABLES	( PDE | 0x04 )	/* Closing vc due to cable failures  */
#define	E_CLOSEDVCD	( PDE | 0x05 )	/* Message on closed VC received     */
#define	E_RPORTSTATE	( PDE | 0x06 )	/* Remote port in invalid state	     */

					/* CI Severe Error Event Codes	     */
/* CI severe error events are either local port or path specific.  There are
 * currently no remote port specific CI severe error events.  ci_console_log(),
 * ci_clse[], and ci_cltab[][] must be updated to reflect new additions.
 * Either ci_log_dev_attn() or ci_log_packet() must be updated when a new local
 * port specific CI severe error event is added, depending upon the nature of
 * the event.  ci_log_packet() must be updated when a new path specific CI
 * severe error event is added.  ci_crash_lport() also requires updating when
 * the new addition is a candidate for application of the local port crash
 * severity modifier( ESM_LPC ).
 *
 * The following CI severe error events are local port specific and may have
 * the local port crash severity modifier( ESM_LPC ) applied:
 *
 *	INVBNAME, INVBSIZE, BACCVIO, INVLPKTSIZE, INVDPORT, UNKCMD, ABORTPKT,
 *	UNKSTATUS, UNKOPCODE, INVOPCODE, POWER, POWERUP, ICMDQ0, ICMDQ1,
 *	ICMDQ2, ICMDQ3, IDFREEQ, IMFREEQ, RRSPQ, RDFREEQ, RMFREEQ, MSE, BIMSE,
 *	DSE, PARITY, BIPARITY, PORTERROR, BIERROR, SANITYTIMER, MFQE
 *
 * The following CI severe error events are local port specific but may NOT
 * have the local port crash severity modifier( ESM_LPC ) applied:
 *
 *	BADPORTNUM
 *
 * The following CI severe error events are path specific but may NOT have the
 * path crash severity modifier( ESM_PC ) applied:
 *
 *	BMSE, INVRPKTSIZE, UNRECPKT, OSEQMSG
 *
 * Path specific information is always displayed by default during console
 * logging of path specific CI severe error events.  The local port station
 * address is always displayed by default during console logging of local port
 * specific CI severe error events.
 *
 * NOTE: Path specific CI severe error events are NEVER candidates for
 *	 application of the path crash severity modifier( ESM_PC ).  Not all
 *	 local port specific CI severe error events are candidates for
 *	 application of the local port crash severity modifier( ESM_LPC ).
 *
 * NOTE: When updating ci_clse[] with a new CI severe error event bear in mind
 *	 the information displayed by default when choosing an appropriate
 *	 console formatting code( CF ).
 */
#define	SE_BMSE	       ( PDSE  | 0x01 )	/* Port closed vc - buf mem failures */
#define	SE_INVRPKTSIZE ( PDSE  | 0x02 )	/* received invalid sized packet     */
#define	SE_UNRECPKT    ( PDSE  | 0x03 )	/* Unrecognized packet received	     */
#define	SE_OSEQMSG     ( PDSE  | 0x04 )	/* Out-of-sequenced message received */
#define	SE_BADPORTNUM  ( PDSE  | 0x05 )	/* Invalid port number encountered   */
#define	SE_INVBNAME    ( PDSE  | 0x06 )	/* Invalid local buffer name	     */
#define	SE_INVBSIZE    ( PDSE  | 0x07 )	/* Local buffer length violation     */
#define	SE_BACCVIO     ( PDSE  | 0x08 )	/* Accvio during local buffer access */
#define	SE_INVLPKTSIZE ( PDSE  | 0x09 )	/* local packet size violation	     */
#define	SE_INVDPORT    ( PDSE  | 0x0A )	/* Invalid destination port	     */
#define	SE_UNKCMD      ( PDSE  | 0x0B )	/* Unknown local port command	     */
#define	SE_ABORTPKT    ( PDSE  | 0x0C )	/* Aborted port command encountered  */
#define	SE_UNKSTATUS   ( PDSE  | 0x0D )	/* Unknown status in packet	     */
#define	SE_UNKOPCODE   ( PDSE  | 0x0E )	/* Unknown opcode in packet	     */
#define	SE_INVOPCODE   ( PDSE  | 0x0F )	/* Invalid opcode in packet	     */
#define	SE_POWER       ( PDSE  | 0x10 )	/* Power failure occurred	     */
#define	SE_POWERUP     ( PDSE  | 0x11 )	/* Power restored to local port	     */
#define	SE_ICMDQ0      ( PDSE  | 0x12 )	/* CMDQ0 interlocked on insertion    */
#define	SE_ICMDQ1      ( PDSE  | 0x13 )	/* CMDQ1 interlocked on insertion    */
#define	SE_ICMDQ2      ( PDSE  | 0x14 )	/* CMDQ2 interlocked on insertion    */
#define	SE_ICMDQ3      ( PDSE  | 0x15 )	/* CMDQ3 interlocked on insertion    */
#define	SE_IDFREEQ     ( PDSE  | 0x16 )	/* DFREEQ interlocked on insertion   */
#define	SE_IMFREEQ     ( PDSE  | 0x17 )	/* MFREEQ interlocked on insertion   */
#define	SE_RRSPQ       ( PDSE  | 0x18 )	/* RSPQ interlocked on removal	     */
#define	SE_RDFREEQ     ( PDSE  | 0x19 )	/* DFREEQ interlocked on removal     */
#define	SE_RMFREEQ     ( PDSE  | 0x1A )	/* MFREEQ interlocked on removal     */
#define	SE_MSE         ( PDSE  | 0x1B )	/* Memory system error reported	     */
#define	SE_BIMSE       ( PDSE  | 0x1C )	/* BI Memory system error reported   */
#define	SE_DSE	       ( PDSE  | 0x1D )	/* Data structure error reported     */
#define	SE_PARITY      ( PDSE  | 0x1E )	/* Parity error reported	     */
#define	SE_BIPARITY    ( PDSE  | 0x1F )	/* BI parity error reported	     */
#define	SE_PORTERROR   ( PDSE  | 0x20 )	/* Miscellaneous port error reported */
#define	SE_BIERROR     ( PDSE  | 0x21 )	/* Miscellaneous BI error reported   */
#define	SE_SANITYTIMER ( PDSE  | 0x22 )	/* Sanity timer expired		     */
#define	SE_MFQE        ( PDSE  | 0x23 )	/* Message free queue exhausted	     */
#define	SE_INVPA       ( PDSE  | 0x24 )	/* Invalid port addr. in recv. pkt.  */
#define	SE_INVSN       ( PDSE  | 0x25 )	/* Invalid sequence number received  */
#define	SE_INVDDL      ( PDSE  | 0x26 )	/* Invalid datalink address received */
#define	SE_IRESVCD     ( PDSE  | 0x27 )	/* Insufficient VCD entries	     */
#define	SE_IRESEQ      ( PDSE  | 0x28 )	/* Insuff. RESEQ resources (RDP only)*/
#define	SE_DISCVCPKT   ( PDSE  | 0x29 )	/* Discarded VC packet ( RDP only )  */

					/* CI Fatal Error Event Codes	     */
/* CI fatal error events are always local port specific.  There are currently
 * no path or remote port specific CI fatal error events.  ci_console_log(),
 * ci_clfe[], and ci_cltab[][] must be updated to reflect new additions. 
 * ci_log_initerr() must be updated when the new local port specific CI fatal
 * error event may occur during initial probing of the local CI port.
 * ci_log_dev_attn() must be updated when the new local port specific CI fatal
 * error event may NOT occur during initial probing of the local CI port.
 * ci_crash_lport() also requires updating when the new addition is a candidate
 * for application of the local port crash severity modifier( ESM_LPC ).
 *
 * The following CI fatal error events are local port specific and may have the
 * local port crash severity modifier( ESM_LPC ) applied:
 *
 *	NOCI, BADMAXPORT, BADUCODE, PORTERROR
 *
 * The following CI fatal error events are local port specific but may NOT have
 * the local port crash severity modifier( ESM_LPC ) applied:
 *
 *	INIT_NOMEM, INIT_ZEROID, INIT_NOUCODE, INIT_UNKHPT, INIT_MISMTCH,
 *	UCODE_LOAD, UCODE_START, CPU
 *
 * The local port station address is displayed by default during console
 * logging of all CI fatal error events.
 *
 * NOTE: Not all local port specific CI fatal error events are candidates for
 *	 application of the local port crash severity modifier( ESM_LPC ).
 *
 * NOTE: When updating ci_clfe[] with a new CI fatal error event bear in mind
 *	 the information displayed by default when choosing an appropriate
 *	 console formatting code( CF ).
 */
#define	FE_INIT_NOMEM	( PDFE | 0x01 )	/* Init - insufficient memory        */
#define	FE_INIT_ZEROID	( PDFE | 0x02 )	/* Init - zero system id number      */
#define	FE_INIT_NOUCODE	( PDFE | 0x03 )	/* Init - can't find CI microcode    */
#define	FE_INIT_UNKHPT	( PDFE | 0x04 )	/* Init - unknown hardware port type */
#define	FE_INIT_MISMTCH	( PDFE | 0x05 )	/* Init - ucode-port type mismatch   */
#define	FE_UCODE_LOAD	( PDFE | 0x06 )	/* Unable to verify microcode load   */
#define	FE_UCODE_START	( PDFE | 0x07 )	/* Unable to start microcode	     */
#define	FE_CPU		( PDFE | 0x08 )	/* Init - CPU ucode not at rev level */
#define	FE_NOCI		( PDFE | 0x09 )	/* CI adapter permanently absent     */
#define	FE_BADMAXPORT	( PDFE | 0x0A )	/* Invalid maximum port number	     */
#define	FE_BADUCODE	( PDFE | 0x0B )	/* Invalid CI ucode revision levels  */
#define	FE_PORTERROR	( PDFE | 0x0C )	/* Misc fatal port error reported    */

					/* Loadable Microcode Types	     */
#define	UCODE_CI780		 1	/* CI780/CI780/CIBCI Functional Ucode*/
#define	UCODE_CIBCA		 2	/* CIBCA-AA Functional Microcode     */

					/* Local Port Mapping Constants      */
#define	MAP_REGS		 0	/* Map only the I/O adapter space    */
#define	MAP_FULL		 1	/* Map fully the local CI port	     */

					/* Port Command Operation Codes	     */
#define	RETCNF			 3	/* Returned confirm - UNUSED	     */
#define	CNFRET			 3	/* Confirm returned - UNUSED	     */
#define	REQID			 5	/* Request identification	     */
#define	IDREQ			 5	/* Identification requested	     */
#define	SNDRST			 6	/* Send reset 			     */
#define	RSTSNT			 6	/* Reset sent 			     */
#define	SNDSTRT			 7	/* Send start			     */
#define	STRTSNT			 7	/* Start send			     */
#define REQDAT0			 8	/* Request data @ priority 0 - UNUSED*/
#define DATREQ0			 8	/* Data requested @ priority 0 UNUSED*/
#define REQDAT1			 9	/* Request data @ priority 1	     */
#define DATREQ1			 9	/* Data requested @ priority 1	     */
#define REQDAT2			10	/* Request data @ priority 2 - UNUSED*/
#define DATREQ2			10	/* Data requested @ priority 2 UNUSED*/
#define	SNDLB			13	/* Send loopback		     */
#define	LBSNT			13	/* Loopback sent		     */
#define	REQMDAT			14	/* Request maintenance data - UNUSED */
#define	MDATREQ			14	/* Maintenance data requested UNUSED */
#define	SNDDAT			16	/* Send data			     */
#define	DATSNT			16	/* Data sent			     */
#define	RETDAT			17	/* Return data - UNUSED		     */
#define	DATRET			17	/* Data returned - UNUSED	     */
#define	SNDMDAT			18	/* Send maintenance data - UNUSED    */
#define	MDATSNT			18	/* Maintenance data sent - UNUSED    */
#define	INVTC			24	/* Invalidate translation cache	     */
#define	TCINV			24	/* Translation cache invalidated     */
#define	SETCKT			25	/* Set circuit			     */
#define	CKTSET			25	/* Circuit set			     */
#define	RDCNT			26	/* Read counters - UNUSED	     */
#define	CNTRD			26	/* Counters read - UNUSED	     */
#define	CNFREC			35	/* Confirm received		     */
#define	MCNFREC			36	/* Maintenance confirm recvd - UNUSED*/
#define	IDREC			43	/* Identification received	     */
#define	LBREC			45	/* Loopback received		     */
#define	DATREC			49	/* Data received		     */
#define	MDATREC			51	/* Maintenance data received - UNUSED*/

					/* Port Driver Panic Strings	     */
#define	PANIC_BADUNMAP	"ci\t- invalid unmapping of local port\n"
#define	PANIC_CABLE	"ci\t- unknown cable status check requested\n"
#define	PANIC_HPT	"ci\t- unknown/invalid hardware port type\n"
#define	PANIC_IC	"ci\t- unknown interconnect type\n"
#define	PANIC_MAP	"ci\t- attempting to map already mapped adapter\n"
#define	PANIC_NOSCPKT	"ci\t- no set circuit off command packet\n"
#define	PANIC_NOTCPKT  "ci\t- no invalidate translation cache command packet\n"
#define	PANIC_ONBOARD	"ci\t- attempting to load unnecessary microcode\n"
#define	PANIC_PCCBFB	"ci\t- invalid pccb fork block\n"
#define	PANIC_REQLPC	"ci\t- panic requested on all local port failures\n"
#define	PANIC_UNKCF	"ci\t- unknown console logging formatting code\n"
#define	PANIC_UNKCODE	"ci\t- unknown/invalid event code\n"
#define	PANIC_UNKLPC	"ci\t- unknown local port crash reason\n"
#define	PANIC_UNMAP	"ci\t- attempting to unmap already unmapped adapter\n"

					/* Status Error Subtypes	     */
#define	ST_PSVIO		 0	/* Packet size violation	     */
#define	ST_UPKT			 1	/* Unrecognized packet		     */
#define	ST_DPORT		 2	/* Invalid destination port	     */
#define	ST_UCMD			 3	/* Unrecognized port command	     */
#define	ST_ABORT		 4	/* Command aborted( port disabled )  */
#define	ST_INVPA		 5	/* Invalid port addr. in recv. pkt.  */
#define	ST_INVSN		 6	/* Invalid sequence number on VC     */
#define	ST_IRESVCD		 7	/* Insufficient VCD resources        */
#define	ST_IRESEQ		 8	/* Insufficient reseq. resources RDP */
#define	ST_DISCVCPKT		 9	/* Discarded VC pkt RDP adap. only   */
#define	ST_INVDDL		10	/* Invalid destination datalink addr */

					/* Status Error Types		     */
#define	T_OK			 0	/* Status is OK			     */
#define	T_VCC			 1	/* VC closed before command execution*/
#define	T_INVBNAME		 2	/* Invalid buffer name		     */
#define	T_INVBSIZE		 3	/* Buffer length violation	     */
#define	T_ACCVIO		 4	/* Access control violation	     */
#define	T_NOPATH		 5	/* No path			     */
#define	T_BMSE			 6	/* Buffer memory system error	     */
#define	T_OTHER			 7	/* Other - see status error subtypes */

					/* Miscellaneous Constants	     */
#define	MIN_VAX750_REV		97	/* Minimum req 11/750 CPU ucode level*/
#define	Q_LOCKED	0x00000001	/* Queue interlock bit		     */

/* CI Port Specific Constants.
 */
					/* CI750/CI780/CIBCI Cache Sizes     */
#define	CI7B_DG_CACHE		 3 	/* Size of datagram cache	     */
#define	CI7B_MSG_CACHE		 3	/* Size of message cache	     */

					/* CIBCA/CIXCD/CIKMF/CISHC Cache Sizes*/
#define	CIBX_DG_CACHE		 4 	/* Size of datagram cache	     */
#define	CIBX_MSG_CACHE		 4	/* Size of message cache	     */

					/* CI750/CI780/CIBCI Microcode 	     */
					/*  Related Constants		     */
#define	CI7B_ARAM_LOAD	0x00000000	/* All RAM port ucode load address   */
#define	CI7B_MAX_RAM		32	/* Current maximum RAM revision level*/
#define	CI7B_MAX_ROM		32	/* Current maximum PROM rev level    */
#define	CI7B_RAM_LOAD	0x00000400	/* PROM/RAM port ucode load address  */
#define	CI7B_ROM_ADDR5	0x000003FA	/* PROM revision level wcs addr < V6 */
#define	CI7B_ROM_ADDR6	0x000003F9	/* PROM revision level wcs addr => V6*/
#define	CI7B_RAM_ADDR5	0x00000BFF	/* RAM revision level wcs addr < V6  */
#define	CI7B_RAM_ADDR6	0x00000BFE	/* RAM revision level wcs addr => V6 */
#define	CI7B_REV_OFF		24	/* RAM/PROM data revision offset     */
#define	CI7B_SEL_CSHO	0x00001000	/* High order control store selector */
#define	CI7B_STARTADDR	0x00000400	/* Microcode starting address	     */
#define	CI7B_UCODEWDSZ		 6	/* Size of microcode word	     */
#define	CI7B_WCS_SIZE	0x00000C00	/* Size of control store	     */

					/* CIBCA Microcode Related Constants */
#define	CIBCA_FN_ADDR	0x0000108C	/* Fn ucode rev level adapter offset */
#define	CIBCA_ST_ADDR	0x00001090	/* Self-test rev level adapter offset*/
#define	CIBCA_BA_MAXFN		 1	/* Current CIBCA-BA max fn ucode lev */
#define	CIBCA_BA_MAXST		 1	/* Current CIBCA-BA max st ucode lev */

					/* CIBCA-AA Microcode Related	     */
					/*  Constants			     */
#define	CIBCA_AA_MAXFN		 6	/* Current CIBCA-AA max fn ucode lev */
#define	CIBCA_AA_MAXST		 5	/* Current CIBCA-AA max st ucode lev */
#define	CIBCA_AA_NAMSIZ		10	/* Microcode name field size	     */
#define	CIBCA_AA_NSECT		 3	/* Maximum num control store sections*/
#define	CIBCA_AA_CSADDR	0x00000000	/* Control store load address	     */
#define	CIBCA_AA_UCWDSZ		 2	/* Microcode word size		     */
#define	CIBCA_AA_CSSIZ	0x00001000	/* Control store size		     */
#define	CIBCA_AA_CSSECT	0x00004000	/* Control store section selector    */

					/* CIXCD Microcode Related Constants */
/* TEMP */
#define	CIXCD_MAXFN		 1	/* Current CIXCD max fn ucode level  */
#define	CIXCD_MAXST		 1	/* Current CIXCD max st ucode level  */

#define	CIKMF_MAXFN		 1	/* Current CIKMF max fn ucode level  */
#define	CIKMF_MAXST		 1	/* Current CIKMF max st ucode level  */

#define	CISHC_MAXFN		 1	/* Current CISHC max fn ucode level  */
#define	CISHC_MAXST		 1	/* Current CISHC max st ucode level  */

					/* CI750/CI780 Register Offsets      */
#define	CI780_CNFR	     0x000	/* Configuration register	     */
#define	CI780_PMCSR	     0x004	/* Port maintenance cntl & status reg*/
#define	CI780_MADR	     0x014	/* Maintenance address register      */
#define	CI780_MDATR	     0x018	/* Maintenance data register	     */

					/* CIBCI Register Offsets	     */
#define	CIBCI_CNFR	     0x100	/* Configuration register	     */
#define	CIBCI_PMCSR	     0x110	/* Port maintenance cntl & status reg*/
#define	CIBCI_MADR	     0x114	/* Maintenance address register      */
#define	CIBCI_MDATR	     0x118	/* Maintenance data register	     */

					/* CI750/CI780/CIBCI Register Offsets*/
#define	CI7B_PSR	     0x900	/* Port status register		     */
#define	CI7B_PQBBASE	     0x904	/* PQB base register 		     */
#define	CI7B_PCQ0CR	     0x908	/* Port command queue 0 control reg  */
#define	CI7B_PCQ1CR	     0x90C	/* Port command queue 1 control reg  */
#define	CI7B_PCQ2CR	     0x910	/* Port command queue 2 control reg  */
#define	CI7B_PCQ3CR	     0x914	/* Port command queue 3 control reg  */
#define	CI7B_PSRCR	     0x918	/* Port status release control reg   */
#define	CI7B_PECR	     0x91C	/* Port enable control register      */
#define	CI7B_PDCR	     0x920	/* Port disable control register     */
#define	CI7B_PICR	     0x924	/* Port initialization control reg   */
#define	CI7B_PDFQCR	     0x928	/* Port dg free queue control reg    */
#define	CI7B_PMFQCR	     0x92C	/* Port msg free queue control reg   */
#define	CI7B_PMTCR	     0x930	/* Port maintenance timer control reg*/
#define	CI7B_PFAR	     0x938	/* Port failing address register     */
#define	CI7B_PESR	     0x93C	/* Port error status register	     */
#define	CI7B_PPR	     0x940	/* Port parameter register	     */

					/* CIBCA Register Offsets	     */
#define	CIBCA_PQBBASE	    0x00F0	/* PQB base register 		     */
#define	CIBCA_PFAR	    0x00F4	/* Port failing address register     */
#define	CIBCA_PPR	    0x00F8	/* Port parameter register	     */
#define	CIBCA_PESR	    0x00FC	/* Port error status register	     */

#define	CIBX_PSR	    0x1000	/* Port status register		     */
#define	CIBX_PMCSR	    0x1004	/* Port maintenance cntl & status reg*/
#define	CIBCA_MADR	    0x1008	/* Maintenance addr reg( CIBCA-AA )  */
#define	CIBCA_MDATR	    0x100C	/* Maintenance data reg( CIBCA-AA )  */
#define	CIBX_PCQ0CR	    0x1010	/* Port command queue 0 control reg  */
#define	CIBX_PCQ1CR	    0x1014	/* Port command queue 1 control reg  */
#define	CIBX_PCQ2CR	    0x1018	/* Port command queue 2 control reg  */
#define	CIBX_PCQ3CR	    0x101C	/* Port command queue 3 control reg  */
#define	CIBX_PSRCR	    0x1020	/* Port status release control reg   */
#define	CIBX_PECR	    0x1024	/* Port enable control register      */
#define	CIBX_PDCR	    0x1028	/* Port disable control register     */
#define	CIBX_PICR	    0x102C	/* Port initialization control reg   */
#define	CIBX_PDFQCR	    0x1030	/* Port dg free queue control reg    */
#define	CIBX_PMFQCR	    0x1034	/* Port msg free queue control reg   */
#define	CIBX_PMTCR	    0x1038	/* Port maintenance timer control reg*/

					/* CIXCD Register Offsets	     */
#define	CIXCD_XCOMM	    0x0010	/* XMI Comm. register    */
#define	CIXCD_PSCR	    0x0014	/* Port scan cntrl register	     */
#define	CIXCD_PSDR	    0x0018	/* Port scan data  reg */
#define	CIXCD_PMCSR	    0x001C	/* Port maintenance cntl & status reg*/
#define	CIXCD_PDCSR	    0x0020	/* Port diag. control & status reg */
#define	CIXCD_PSR	    0x0024	/* Port status register */
#define	CIXCD_XFAER	    0x002C	/* XMI failing address lw1 reg*/

#define	CIXCD_PQBBASE	    0x1000	/* PQB base register 		     */
#define	CIXCD_PESR	    0x1008	/* Port error status register	     */
#define	CIXCD_PFAR	    0x100C	/* Port failing address register     */
#define	CIXCD_PPR	    0x1010	/* Port parameter register	     */
#define	CIXCD_PSNR	    0x1014	/* Port serial number reg */
#define	CIXCD_PIDR	    0x1018	/* Port interrupt destination reg    */
#define	CIXCD_PVR	    0x1020	/* Port vector register		     */
#define	CIXCD_PCQ0CR	    0x1028	/* Port command queue 0 control reg  */
#define	CIXCD_PCQ1CR	    0x102C	/* Port command queue 1 control reg  */
#define	CIXCD_PCQ2CR	    0x1030	/* Port command queue 2 control reg  */
#define	CIXCD_PCQ3CR	    0x1034	/* Port command queue 3 control reg  */
#define	CIXCD_PSRCR	    0x1038	/* Port status release control reg   */
#define	CIXCD_PECR	    0x103C	/* Port enable control register      */
#define	CIXCD_PDCR	    0x1040	/* Port disable control register     */
#define	CIXCD_PICR	    0x1044	/* Port initialization control reg   */
#define	CIXCD_PDFQCR	    0x1048	/* Port dg free queue control reg    */
#define	CIXCD_PMFQCR	    0x104C	/* Port msg free queue control reg   */
#define	CIXCD_PMTCR	    0x1050	/* Port maintenance timer control reg*/
#define	CIXCD_PMTECR	    0x1054	/* Port maint timer expiration ctl reg*/
#define	CIXCD_PPER	    0x1058	/* Port parameter ext. register	     */
#define	CIXCD_PPE2R	    0x101C	/* Port parameter ext. 2 register    */

					/* CIDASH Register Offsets 	     */
#define	CIKMF_XFAER	    0x000C	/* XMI failing address lw1 reg PASS1 */
/*#define	CIKMF_XCOMM	    0x0010*//* XMI COMM address  reg PASS2 */
#define	CIKMF_XPCSER1	    0x0010	/* register PASS1     */
#define	CIKMF_XPCSER2	    0x0014	/* register PASS1     */
#define	CIKMF_XPCCTRL	    0x0018	/* reg */
#define	CIKMF_XPCSTAT1	    0x001C	/* reg*/
#define	CIKMF_XPCSTAT2	    0x0020	/* reg */
#define	CIKMF_XBIST	    0x0024	/* register */
#define	CIKMF_XPCCSR	    0x0028	/* register */
/*#define	CIKMF_XPCPSER1	    0x0020*//* register	PASS2 */
/*#define	CIKMF_XPCPSER2	    0x0024*//* register PASS2 */
/*#define	CIKMF_XFAER	    0x002C*//* XMI failing address lw1 reg PASS2 */
#define	CIKMF_IPL1	    0x0030	/* register */
#define	CIKMF_IPL2	    0x0034	/* register */
#define	CIKMF_IPL3	    0x0038	/* register */
#define	CIKMF_IDEST1	    0x003C	/* register */
#define	CIKMF_IDEST2	    0x0040	/* register */
#define	CIKMF_IDEST3	    0x0044	/* register */
#define	CIKMF_IVECT1	    0x0048	/* register */
#define	CIKMF_IVECT2	    0x004C	/* register */
#define	CIKMF_IVECT3	    0x0050	/* register */

					/* DASHAC1 Specific Registers 	*/
#define CIKMF_SSWCR1	    0x4030	/* Shac software chip reset	*/
#define CIKMF_SSHMA1	    0x4044	/* Shac shared host mem addr	*/
#define CIKMF_SLAVE_ADD1    0x4070	/*				*/
#define CIKMF_DRI1	    0x4074	/*				*/
#define CIKMF_DRO1	    0x4078	/*				*/

					/* DASHAC1 CI Port Registers 	*/
#define CIKMF_PQBBR1	    0x4048	/* PQB base register		*/
#define CIKMF_PSR1	    0x404C	/* Port status register		*/
#define CIKMF_PESR1	    0x4050	/* Port error status register	*/
#define CIKMF_PFAR1	    0x4054	/* Port failing address register*/
#define CIKMF_PPR1	    0x4058	/* Port parameter register	*/
#define CIKMF_PMCSR1	    0x405C	/* Port maintenance cntl & status reg*/
#define CIKMF_PCQ0CR1	    0x4080	/* Port command queue 0 control reg*/
#define CIKMF_PCQ1CR1	    0x4084	/* Port command queue 1 control reg*/
#define CIKMF_PCQ2CR1	    0x4088	/* Port command queue 2 control reg*/
#define CIKMF_PCQ3CR1	    0x408C	/* Port command queue 3 control reg*/
#define CIKMF_PDFQCR1	    0x4090	/* Port dg free queue control reg*/
#define CIKMF_PMFQCR1	    0x4094	/* Port msg free queue control reg*/
#define CIKMF_PSRCR1	    0x4098	/* Port status release control reg*/
#define CIKMF_PECR1	    0x409C	/* Port enable control register	*/
#define CIKMF_PDCR1	    0x40A0	/* Port disable control register*/
#define CIKMF_PICR1	    0x40A4	/* Port initialization control reg */
#define CIKMF_PMTCR1	    0x40A8	/* Port maint timer control reg */
#define CIKMF_PMTEC1	    0x40AC	/* Port maint timer expiration ctl reg*/

					/* DASHAC2 Specific Registers 	*/
#define CIKMF_SSWCR2	    0x4230	/* Shac software chip reset	*/
#define CIKMF_SSHMA2	    0x4244	/* Shac shared host mem addr	*/
#define CIKMF_SLAVE_ADD2    0x4270	/*				*/
#define CIKMF_DRI2	    0x4274	/*				*/
#define CIKMF_DRO2	    0x4278	/*				*/

					/* DASHAC2 CI Port Registers 	*/
#define CIKMF_PQBBR2	    0x4248	/* PQB base register		*/
#define CIKMF_PSR2	    0x424C	/* Port status register		*/
#define CIKMF_PESR2	    0x4250	/* Port error status register	*/
#define CIKMF_PFAR2	    0x4254	/* Port failing address register*/
#define CIKMF_PPR2	    0x4258	/* Port parameter register	*/
#define CIKMF_PMCSR2	    0x425C	/* Port maintenance cntl & status reg*/
#define CIKMF_PCQ0CR2	    0x4280	/* Port command queue 0 control reg*/
#define CIKMF_PCQ1CR2	    0x4284	/* Port command queue 1 control reg*/
#define CIKMF_PCQ2CR2	    0x4288	/* Port command queue 2 control reg*/
#define CIKMF_PCQ3CR2	    0x428C	/* Port command queue 3 control reg*/
#define CIKMF_PDFQCR2	    0x4290	/* Port dg free queue control reg*/
#define CIKMF_PMFQCR2	    0x4294	/* Port msg free queue control reg*/
#define CIKMF_PSRCR2	    0x4298	/* Port status release control reg*/
#define CIKMF_PECR2	    0x429C	/* Port enable control register	*/
#define CIKMF_PDCR2	    0x42A0	/* Port disable control register*/
#define CIKMF_PICR2	    0x42A4	/* Port initialization control reg */
#define CIKMF_PMTCR2	    0x42A8	/* Port maint timer control reg */
#define CIKMF_PMTEC2	    0x42AC	/* Port maint timer expiration ctl reg*/

					/* SHAC CI Port Registers 	*/
#define CISHC_SSWCR	    0x0030	/* Shac software chip reset	*/
#define CISHC_SSHMA	    0x0044	/* Shac shared host mem addr	*/
#define CISHC_PQBBR	    0x0048	/* PQB base register		*/
#define CISHC_PSR	    0x004C	/* Port status register		*/
#define CISHC_PESR	    0x0050	/* Port error status register	*/
#define CISHC_PFAR	    0x0054	/* Port failing address register*/
#define CISHC_PPR	    0x0058	/* Port parameter register	*/
#define CISHC_PMCSR	    0x005C	/* Port maintenance cntl & status reg*/
#define CISHC_PCQ0CR	    0x0080	/* Port command queue 0 control reg*/
#define CISHC_PCQ1CR	    0x0084	/* Port command queue 1 control reg*/
#define CISHC_PCQ2CR	    0x0088	/* Port command queue 2 control reg*/
#define CISHC_PCQ3CR	    0x008C	/* Port command queue 3 control reg*/
#define CISHC_PDFQCR	    0x0090	/* Port dg free queue control reg*/
#define CISHC_PMFQCR	    0x0094	/* Port msg free queue control reg*/
#define CISHC_PSRCR	    0x0098	/* Port status release control reg*/
#define CISHC_PECR	    0x009C	/* Port enable control register	*/
#define CISHC_PDCR	    0x00A0	/* Port disable control register*/
#define CISHC_PICR	    0x00A4	/* Port initialization control reg */
#define CISHC_PMTCR	    0x00A8	/* Port maint timer control reg */
#define CISHC_PMTEC	    0x00AC	/* Port maint timer expiration ctl reg*/

					/* Miscellaneous Constants	     */
#define	MAX_CABLES		 2	/* Maximum cable number		     */

/* CI Register Definitions.
 */
					/* BIIC Error Register Bit Masks     */
#define	CIBCA_MSE_ERRS	0x297F0000	/* CIBCA memory system errs BER mask */
#define	CIBCI_MSE_ERRS	0x013F0000	/* CIBCI memory system errs BER mask */
#define	CIBCI_PAR_ERRS  0x08C00000	/* CIBCI parity errors BER mask	     */

					/* CIBCA/CIXCD BIIC/XMI Device Type  */
					/*  Register Port/Hardware Revision  */
					/*  Field Mask Bits		     */
#define	CIBX_DEV_UCODE	  0x800000	/* Port microcode is onboard	     */

					/* CI750/CI780 Configuration Register*/
					/*   Mask Bits			     */
#define	CI780_CNF_ADAP	0x000000FF	/* Adapter code( device type )	     */
#define	CI780_CNF_PFD	0x00000100	/* Power fail disable		     */
#define	CI780_CNF_TDEAD	0x00000200	/* Transmit dead( CI780 )	     */
#define	CI780_CNF_TDCLO	0x00000200	/* Transmit DCLO( CI750 ) 	     */
#define	CI780_CNF_TFAIL	0x00000400	/* Transmit fail( CI780 )	     */
#define	CI780_CNF_TACLO	0x00000400	/* Transmit ACLO( CI750 ) 	     */
#define	CI780_CNF_NOCI	0x00001000	/* Adapter not present( CI750 )      */
#define	CI780_CNF_CTO	0x00002000	/* CI port timeout( CI750 )	     */
#define	CI780_CNF_CBPE	0x00008000	/* CI port parity error( CI750 )     */
#define	CI780_CNF_CRD	0x00010000	/* Corrected read data		     */
#define	CI780_CNF_UCE	0x00020000	/* Uncorrectable data error( CI750 ) */
#define	CI780_CNF_RDS	0x00020000	/* Read data substitute( CI780 )     */
#define	CI780_CNF_CXTER	0x00040000	/* Command transmit error( CI780 )   */
#define	CI780_CNF_RLTO	0x00080000	/* Readlock transfer timeout( CI750 )*/
#define	CI780_CNF_RDTO	0x00080000	/* Read data timeout( CI780 )	     */
#define	CI780_CNF_NXM	0x00100000	/* Nonexistent memory( CI750 )	     */
#define	CI780_CNF_CXTMO	0x00100000	/* Command transmit timeout( CI780 ) */
#define	CI780_CNF_PUP	0x00400000	/* Power up			     */
#define	CI780_CNF_PDWN	0x00800000	/* Power down			     */
#define	CI780_CNF_XMT	0x04000000	/* Transmit fault( CI780 )	     */
#define	CI780_CNF_MXT	0x08000000	/* Multiple transmit fault( CI780 )  */
#define	CI780_CNF_ISEQ	0x10000000	/* Interlocked seq fault( CI780 )    */
#define	CI780_CNF_URD	0x20000000	/* Unexpected read fault( CI780 )    */
#define	CI780_CNF_WSQ	0x40000000	/* Write sequence fault( CI780 )     */
#define	CI780_CNF_PAR	0x80000000	/* Parity fault( CI780 )	     */
#define	CI780_CNF_SBI	0xFC000000	/* SBI fault CNFR mask( CI780 )      */
#define	CI780_CNF_MSE	0x001E0000	/* Memory system error CNFR mask     */
#define	CI780_CNF_ERRS	0x03FEFF00	/* Configuration register error bits */

					/* CIBCI Configuration Register Mask */
					/*  Bits			     */
#define	CIBCI_CNF_NOCI	0x00800000	/* Port not present		     */
#define	CIBCI_CNF_DCLO	0x01000000	/* Port without power or uncabled    */
#define	CIBCI_CNF_PUP	0x04000000	/* Power up			     */
#define	CIBCI_CNF_PDWN	0x08000000	/* Power down			     */
#define	CIBCI_CNF_BBE	0x10000000	/* BI busy error		     */
#define	CIBCI_CNF_BIPE	0x20000000	/* BI parity error		     */
#define	CIBCI_CNF_BAPE	0x40000000	/* BI adapter parity error	     */
#define	CIBCI_CNF_CPPE	0x80000000	/* Internal bus parity error	     */
#define	CIBCI_CNF_ERRS	0xff800000	/* Configuration register error bits */
#define	CIBCI_CNF_PE	0xe0000000	/* Configuration register parity errs*/

					/* Port Control Register Mask Bits   */
#define	PCQ0CR_CMDQ0C	0x00000001	/* Port Command queue 0 control      */
#define	PCQ1CR_CMDQ1C	0x00000001	/* Port Command queue 1 control      */
#define	PCQ2CR_CMDQ2C	0x00000001	/* Port Command queue 2 control      */
#define	PCQ3CR_CMDQ3C	0x00000001	/* Port Command queue 3 control      */
#define	PDFQCR_DFQC	0x00000001	/* Port Datagram free queue control  */
#define	PMFQCR_MFQC	0x00000001	/* Port Message free queue control   */
#define	PDCR_PDC	0x00000001	/* Port disable control		     */
#define	PICR_PIC	0x00000001	/* Port initialize control	     */
#define	PECR_PEC	0x00000001	/* Port enable control		     */
#define	PMTCR_MTC	0x00000001	/* Port Maintenance timer control    */
#define	PSRCR_PSRC	0x00000001	/* Port status reg release control   */

					/* Port Error Status Register	     */
					/*   Miscellaneous Error Codes	     */
#define	PESR_BRKLINK		14	/* Broken link module( L0100 )	     */
					/*  ( CI750/CI780/CIBCI only )

					/* CI750/CI780/CIBCI Port Maintenance*/
					/*  Control and Status Register Mask */
					/*  Bits			     */
#define	CI7B_PMCS_MIN	0x00000001	/* Maintenance initialize	     */
#define	CI7B_PMCS_MIE	0x00000004	/* Maintenance interrupt enable	     */
#define	CI7B_PMCS_MIF	0x00000008	/* Maintenance interrupt flag	     */
#define	CI7B_PMCS_PSA	0x00000040	/* Programmable starting address     */
#define	CI7B_PMCS_XBPE	0x00000100	/* Transmit buffer parity error	     */
#define	CI7B_PMCS_OPE	0x00000200	/* Output parity error		     */
#define	CI7B_PMCS_IPE	0x00000400	/* Input parity error( CI780 )	     */
#define	CI7B_PMCS_CBPE	0x00000400	/* Adapter bus error( CI750/CIBCI )  */
#define	CI7B_PMCS_XMPE	0x00000800	/* Transmit buffer parity error	     */
#define	CI7B_PMCS_RBPE	0x00001000	/* Receive buffer parity error	     */
#define	CI7B_PMCS_LSPE	0x00002000	/* Local store parity error	     */
#define	CI7B_PMCS_CSPE	0x00004000	/* Control store parity error	     */
#define	CI7B_PMCS_PE	0x00008000	/* Parity error			     */

					/* CIBCA Port Maintenance Control and*/
					/*  Status Register Mask Bits	     */
#define	CIBX_PMCS_START	0x00000001	/* Start sequencer( CIBCA )	     */
#define	CIBX_PMCS_MIE	0x00000020	/* Maintenance interrupt enable	     */
#define	CIBX_PMCS_HALT	0x00000080	/* Halt sequencer		     */
#define	CIBCA_PMCS_BTO	0x00000100	/* BI bus timeout		     */
#define	CIBCA_PMCS_IIPE	0x00000200	/* II parity error		     */
#define	CIBCA_PMCS_MBIE	0x00000400	/* MAP BCI/BI error		     */
#define	CIBX_PMCS_CPE	0x00000800	/* CILP parity error		     */
#define	CIBX_PMCS_XMPE	0x00001000	/* Transmit buffer parity error	     */
#define	CIBX_PMCS_IBPE	0x00002000	/* Internal bus parity error	     */
#define	CIBX_PMCS_CSPE	0x00004000	/* Control store parity error	     */
#define	CIBCA_PMCS_BCIP	0x00008000	/* BCI parity error		     */

					/* CIXCD Port Maintenance Control */
#define	CIXCD_PMCS_MIN	0x00000001	/* Maintence initialize		     */
#define	CIXCD_PMCS_MIE	0x00000004	/* Maintence interrupt enable	     */
#define	CIXCD_PMCS_CPDE	0X00000010	/* CPU no responce error	     */
#define	CIXCD_PMCS_CPER	0X00000020	/* CPU error status	  	    */
#define	CIXCD_PMCS_PRER	0X00000040	/* Port read error responce	     */
#define	CIXCD_PMCS_PWER	0X00000080	/* Port write error responce	     */
#define	CIXCD_PMCS_STUF	0X00000100	/* ustack underflow error	     */
#define	CIXCD_PMCS_STOF	0x00000200	/* ustack overflow error	     */
#define	CIXCD_PMCS_YRPE	0X00000400	/* Y register parity error	     */
#define	CIXCD_PMCS_XRPE	0X00000800	/* X register parity error	     */
#define	CIXCD_PMCS_IBPE	0X00001000	/*  error	     */
#define	CIXCD_PMCS_CSPE	0X00002000	/*  error	     */
#define	CIXCD_PMCS_PRIBPE 0X00004000	/*  error		     */

					/* CIDASH/SHAC Port Maint Control */
#define CISHC_PMCS_SIMP 0x00000008	/* Simple Shac mode diag only	*/
#define CISHC_PMCS_HC   0x00000010	/* Host access diag only	*/

					/* Port Maintenance Control and	     */
					/*  Status Register Mask Bits	     */
#define	PMCSR_MIN	0x00000001	/* Maintenance timer initialize	     */
#define	PMCSR_MTD	0x00000002	/* Maintenance timer disable	     */
#define	PMCSR_MIE	0x00000004	/* Maintence interrupt enable	     */

					/* Port Parameter Register Mask Bits */
#define	PPR_LPORT_ADDR	0x000000FF	/* Local port station address	     */
#define	PPR_DIAG_INFO	0x00007F00	/* Diagnostic information	     */
#define PPR_PPE_BIT 	0X00008000	/* Port Parameter Extention Reg. Bit */
#define	PPR_L0100_CSZ	0x80000000	/* Cluster size( L0100 link module ) */
#define	PPR_CSZ		0xE0000000	/* Cluster size( new format )	     */
#define	PPR_IBUF_LEN	0x1FFF0000	/* IBUF_LEN of port	*/

					/* Port Parameter Extention Register */
					/*  Mask Bits 		             */
#define	PPER_EAF	0x00000080	/* Explicit address format	     */
#define	PPER_MEM_NO	0x0000FF00	/* Member number for local port      */
#define PPER_NADP	0x00010000	/* NADP field supported in VCD	     */
#define PPER_RDPS	0x00020000	/* RDP adapter support  	     */
#define PPER_ESS	0x00040000	/* Explicit state supported in VCD   */

					/* Port Parameter Extention-2 Register*/
					/*  Mask Bits 		             */
#define PPE2R_ASB_LEN   0x0000FFFF	/* Requested adapter state block len.*/

					/* CI750/CI780/CIBCI Port Status     */
					/*  Register Mask Bits		     */
#define	CI7B_PS_MTE	0x80000000	/* Maintenance error		     */
#define	CI7B_PS_ERRS	0xFFFFFFFE	/* PSR error bits		     */

					/* CIBCA/CIXCD Port Status Register  */
					/*  Mask Bits			     */
#define	CIBX_PS_MTE	0x00000100	/* Maintenance error		     */
#define	CIBX_PS_MIF	0x00000200	/* Maintenance interrupt flag	     */
#define	CIBX_PS_UNIN	0x00000400	/* CIBCA/CIXCD uninitialized  	     */
#define	CIBX_PS_NRSPE	0x00008000	/* CIBCA No response error	     */
#define	CIXCD_PS_NRSPE	0x80000000	/* CIXCD No response error	     */
#define	CIBX_PS_ERRS	0xFFFFFDFE	/* PSR error bits		     */

					/* CIKMF Port Status Register        */
#define CIKMF_PS_CRDE   0x00000100	/* Corrected read data error detected */
#define	CIKMF_PS_MTE	0x80000000	/* Maintenance error		     */

					/* Shac Port Status Register 	     */
#define CISHC_PS_SHME   0x00010000	/* Shared host memory error	     */
#define CISHC_PS_SMPE   0x00020000	/* Slave mode parity error	     */
#define CISHC_PS_ISNE   0x00040000	/* Illegal segment number error	     */
#define CISHC_PS_DE     0x00080000	/* Diagnostic error		     */
#define CISHC_PS_QDE    0x00100000	/* Quip Detected error		     */
#define CISHC_PS_IIE    0x00200000	/* Illegal interrupt error	     */
#define CISHC_PS_MTE    0x80000000	/* Maintenance error		     */
#define	CISHC_PS_ERRS	0xFFFFFFFE	/* PSR error bits		     */

					/* Port Status Register Mask Bits    */
#define	PSR_RQA		0x00000001	/* Response queue available	     */
#define	PSR_MFQE	0x00000002	/* Message free queue empty	     */
#define	PSR_PDC		0x00000004	/* Port disablement complete	     */
#define	PSR_PIC		0x00000008	/* Port initialization complete	     */
#define	PSR_DSE		0x00000010	/* Data structure error		     */
#define	PSR_MSE		0x00000020	/* Memory structure error	     */
#define	PSR_SE		0x00000040	/* Sanity timer expiration	     */
#define	PSR_MISC	0x00000080	/* Miscellaneous errors		     */

					/* XMI Error Register Bit Masks	     */
#define	CIXCD_XBE_TTO	0x00000004	/* CIXCD XBE disable TTO 	     */
#define	CIXCD_XBE_ERRS	0x88F7B400	/* CIXCD XBE errors mask	     */
#define	CIXCD_XBE_HARDE	0x00F7B400	/* CIXCD XBE hard errors mask	     */
#define	CIXCD_XBE_SOFTE	0x08000000	/* CIXCD XBE soft errors mask	     */
#define	CIKMF_XBE_ERRS	0x09E01400	/* CIKMF XBE errors mask	     */
#define	CIKMF_XBE_PERRS	0x01800000	/* CIKMF XBE parity errors mask	     */
#define	CIKMF_XBE_HERRS	0x00601400	/* CIKMF XBE port errors mask	     */
#define	CIKMF_PSER_ERRS 0x801EA000	/* CIKMF port specific hard err mask */
#define	CIKMF_PSER_HERRS 0x0017A000	/* CIKMF port specific hard err mask */
#define	CIKMF_PSER_SERRS 0x00080000	/* CIKMF port specific soft err mask */

#define FCN_EXT2_MMV 	0x80000000	/* Member map valid 		     */
#define FCN_EXT2_EAS 	0x00008000	/* Explicit address support	     */
#define FCN_EXT2_FSN 	0x00004000	/* Full sequence numbers	     */
#define FCN_EXT2_RDP 	0x00002000	/* Resequence dual path  	     */
#define FCN_EXT2_NADP 	0x00001000	/* Non-adp adapter support	     */
#define FCN_EXT2_ONI 	0x00000800	/* Order not important support	     */
#define FCN_EXT2_NMEMS 	0x000000FF	/* Number of members in pgrp <= 64   */
#define FCN_EXT2_EFR 	0x0000E000	/* MASK EAS,FSN,RDP      	     */
#define FCN_EXT2_FR 	0x00006000	/* MASK FSN,RDP		      	     */


/* CI Data Structure Definitions.
 */
typedef struct		  {		/* Loopback CRC Table		     */
    u_char			:  8;	/* MBZ				     */
    u_char	lblen7;			/* Loopback data size plus 7	     */
    u_char	port_num;		/* Local port number		     */
    u_char	not_port_num;		/* One's complement( local port num )*/
    u_char	lport_addr;		/* Local port station address	     */
    u_char	opcode;			/* Value of SNDLB opcode	     */
    u_char			:  8;	/* MBZ				     */
    u_char	lbdata[ LBDSIZE ];	/* Loopback data		     */
} LBCRC;

					/* Microcode Revision Level Table    */
typedef struct _mrltab {		/*  Entry			     */
    u_char	 ram;			/* RAM revision level		     */
    u_char	 rom;			/* PROM/EEPROM revision level	     */
    u_short	 warn;			/* Warning message flag		     */
#define	UCODE_NOWARNING		0	/*  No warning message required	     */
#define	UCODE_WARNING		1	/*  Warning message required	     */
} MRLTAB;

typedef	struct _vcd	{		/* Virtual Circuit Descriptor	     */
    union {
	struct {
    	    u_long			:  9;	/* MBZ			     */
    	    u_long	pstatus		:  2;	/* Cable status		     */
#define	VCD_BAD		0		/*  Both cables are bad		     */
#define	VCD_CABLE0 	1		/*  Cable 0 is good		     */
#define	VCD_CABLE1	2		/*  Cable 1 is good		     */
#define	VCD_GOOD	3		/*  Both cables are good	     */
    	    u_long	nadp		:  1;	/* NADP     		     */
    	    u_long	dqi		:  1;	/* DFREEQ inhibit	     */
    	    u_long	sseq_num	:  1;	/* Sending sequence number   */
    	    u_long	rseq_num	:  1;	/* Receiving sequence number */
    	    u_long	vcstate		:  1;	/* Virtual circuit state     */
#define	VC_OFF		0		/*  Virtual circuit is off	     */
#define	VC_ON		1		/*  Virtual circuit is on	     */
    	    u_long	snd_st		:  3;	/* Send states      	     */
    	    u_long			:  5;	/* RSV     		     */
    	    u_long	rcv_st		:  2;	/* Receive states     	     */
    	    u_long			:  6;	/* RSV   		     */
	} ssn;
	struct {
    	    u_long	sseq_num	:  3;	/* Sending sequence number   */
    	    u_long	rseq_num	:  3;	/* Sending sequence number   */
    	    u_long	rdp		:  1;	/* Resequence dual path      */
    	    u_long	fsn		:  1;	/* Full sequence numbers     */
    	    u_long	eas		:  1;	/* Subnode Addressing 	     */
    	    u_long	pstatus		:  2;	/* Cable status		     */
    	    u_long	nadp		:  1;	/* NADP     		     */
    	    u_long	dqi		:  1;	/* DFREEQ inhibit	     */
    	    u_long			:  2;	/* MBZ			     */
    	    u_long	vcstate		:  1;	/* Virtual circuit state     */
    	    u_long	snd_st		:  3;	/* Send states      	     */
    	    u_long			:  5;	/* RSV     		     */
    	    u_long	rcv_st		:  2;	/* Receive states     	     */
    	    u_long			:  6;	/* RSV   		     */
	} fsn;
    } vcd_ov;
} VCD;

/* CI Port Specific Data Structure Definitions.
 */
typedef struct 		{		/* CI750/CI780/CIBCI Microcode Header*/
    u_char	unused[ 8 ];		/* Fields unused by CI port driver   */
    u_long	port_type;		/* Microcode port type indicator     */
#define	CI7B_ALLRAM	0x5F6A00A1	/*  All RAM port indicator	     */
} CI7B_UCODEH;

typedef struct 		{		/* CIBCA Microcode Header	     */
    u_char	name[ CIBCA_AA_NAMSIZ ];/* Name of microcode file	     */
    u_char	unused1[ 254 ];		/* Fields unused by CI port driver   */
    u_long	ucode_lbn_off;		/* LBN offset to CIBCA ucode	     */
    u_char	unused[ 244 ];		/* Fields unused by CI port driver   */
} CIBCA_UCODEH;

/* CI Port Command Definitions.
 */
typedef	struct	{			/* Packet Header Cable Status Bits   */
    u_char			:  1;
    u_char cable0		:  2;	/* Cable 0 status		     */
    u_char cable1		:  2;	/* Cable 1 status		     */
#define	CABLE_ACK	0		/*  ACK on cable or cable not use    */
#define	CABLE_NAK	1		/*  NAK on cable		     */
#define	CABLE_NORSP	2		/*  No response on cable	     */
#define	CABLE_TIMEOUT	3		/* CI arbitration timeout	     */
    u_char			:  3;
} CABLE_STATUS;

typedef struct	{			/* Packet Header Opcode Modifiers    */
    u_char rsp			:  1;	/* Response requested when set       */
    u_char cable_select		:  2;	/* Cable select			     */
#define	CS_AUTO		0		/*  Automatic cable selection	     */
#define	CS_CABLE0	1		/*  Select cable 0		     */
#define	CS_CABLE1	2		/*  Select cable 1		     */
    u_char			:  1;
    u_char pkt_multiple		:  3;	/* Packet multiple 		     */
    u_char pkt_size		:  1;	/* Data packet base size	     */
#define	Pack_format	pkt_size	/*  Packing format		     */
#define	Force_reset	pkt_size	/*  Force reset			     */
#define	Default_start	pkt_size	/*  Default start address	     */
} OPCODE_MODS;

typedef struct	{			/* Packet Header Cable Flags	     */
    u_char			:  1;
    u_char rcable		:  2;   /* Receive cable		     */
    u_char			:  1;
    u_char scable		:  2;   /* Send cable			     */
    u_char			:  2;
} CABLE_FLAGS;

typedef	struct	{			/* CKTSET/SETCKT Packet Header Format*/
    struct _vcd	mask;			/* Mask controlling VCD modification */
#define Imp_mask	0x0000E000	/* Implicit Mask - CST, NR, NS       */
#define Kmf_mask	0x000080BF	/* KMF Mask - CST, FSN, NR0:2, NS0:2 */
#define Kmf_mvalue	0x00000080	/* KMF Mvalue - FSN 		     */
#define Exp_mask	0x000089FF	/* Explicit Mask - CST,NADP,EAS,FSN, */
					/* 		   RDP, NR0:2, NS0:2 */
#define Exp_mask_eas	0x00008100	/* Explicit Mask Close - CST, EAS    */
    struct _vcd	mvalue;			/* VCD modification values	     */
    struct _vcd ivalue;			/* VCD prior to modification	     */
    u_short vcd_res;			/* VCD resources remaining lower bound*/
    u_short reseq_res;			/* Reseq. resources remaining lower bo*/
} CKTSETH, SETCKTH;

typedef	struct	{			/* IDREC Packet Header Format	     */
    struct _tid	xctid;			/* Transaction identifier	     */
    u_long	port_type;		/* Port type			     */
    u_long	ucode_rev;		/* Port microcode revision level     */
    u_long	port_fcn;		/* Port functionality mask	     */
    u_long	reset_port	:  8;	/* Port which caused last reset	     */
    u_long	port_state	:  3;	/* Port state			     */
    u_long	sys_state	: 21;	/* Implementation specific state     */
    u_long	port_fcn_ext;		/* Port functionality extension	     */
    u_long	port_fcn_ext2;		/* Port functionality extension #2   */
    u_long			: 32;	/* Sub_map field		     */
    u_long			: 32;	/* Sub_map field		     */
    u_long			: 32;	/* RSV field			     */
    u_long			: 32;	/* RSV field			     */
} IDRECH;

					/* IDREQ/REQID/SNDRST/CNFREC/DATREC  */
typedef	struct	{			/*  Packet Header Format	     */
    struct _tid	xctid;			/* Transaction identifier	     */
} CNFRECH, REQIDH, IDREQH, SNDRSTH, DATRECH;

typedef struct	{			/* LBREC Packet Header Format	     */
    u_short	lblength;		/* Size of loopback data	     */
    u_char	lbdata[ LBDSIZE ];	/* Loopback data		     */
} LBRECH;

typedef	struct	{			/* REQDAT/SNDDAT Packet Header Format*/
    struct _tid	     xctid;		/* Transaction identifier	     */
    u_long	     length;		/* Transaction length		     */
    struct _gvpbname sbname;		/* Sending buffer name		     */
    u_long	     sboff;		/* Sending buffer byte offset	     */
    struct _gvpbname rbname;		/* Receiving buffer name	     */
    u_long	     rboff;		/* Receiving buffer byte offset	     */
} REQDATH, SNDDATH;

typedef struct	{			/* SNDLB Packet Header Format	     */
    u_short	lblength;		/* Size of loopback data	     */
    u_char	lbdata[ LBDSIZE ];	/* Loopback data		     */
    u_char	crc[ 4 ];		/* Loopback packet CRC		     */
} SNDLBH;

typedef	struct	{			/* SNDSTRT Packet Header Format	     */
    struct _tid	xctid;			/* Transaction identifier	     */
    u_long	start_addr;		/* Starting address		     */
} SNDSTRTH;

/* CI Macros.
 */
					/* Svtophy Address Shift Macro 
					 * PQB & ASB base address shift 
					 * right for CI Port arch. ECO	    
					 */
#define Svtophy_shift( value )		( svtophy( value ) >> 9 )

					/* Cluster Size Macros
					 */
#define	Cluster_oldsize( value )	(( value & PPR_L0100_CSZ ) >> 31 )
#define	Cluster_size( value )		(( value & PPR_CSZ ) >> 29 )

					/* Event Logging Macros
					 */
#define	Clog_ftab( tab, event )						\
    ( tab[ Eseverity( event )][ Esubclass( event )].ftable + Ecode( event) - 1)
#define	Clog_maxcode( tab, event )					\
    ( tab[ Eseverity( event )][ Esubclass( event )].max_code )
#define	Clog_tabcode( tab, event )	( Clog_ftab( tab, event )->fcode )
#define	Clog_tabmsg( tab, event )	( Clog_ftab( tab, event )->msg )
#define	Elcibiregs( p )		(( struct bi_regs * )p )
#define	Elciciregs( p )		(( struct ci_regs * )p )
#define	Elcicommon( elp )	( &elp->el_body.elci.cicommon )
#define	Elcicpurevlev( p )	(( struct ci_cpurevlev * )p )
#define	Elcidattn( elp )	( &elp->el_body.elci.citypes.cidattn )
#define	Elcilcommon( elp )						\
    ( &elp->el_body.elci.citypes.cilpkt.cilcommon )
#define	Elcipacket( elp )						\
    ( &elp->el_body.elci.citypes.cilpkt.cilpktopt.cipacket )
#define	Elcirevlev( p )		(( struct ci_revlev * )p )
#define	Elciucode( p )		(( struct ci_ucode * )p )
#define	Elcixmiregs( p )	(( struct cixmi_regs * )p )

					/* Loopback Macros
					 */
#define	Comp_loopback( pccb, bp )					\
    ( bp->lblength == LBDSIZE &&					\
      bcmp( bp->lbdata, pccb->Lbdata, LBDSIZE ) == 0 )
#define	Compute_lbcrc( pccb ) {						\
    LBCRC	  buf;							\
    ( void )bzero(( u_char * )&buf, sizeof( LBCRC ));			\
    buf.lblen7 = ( u_char )( LBDSIZE + 7 );				\
    buf.port_num = ( u_char )Scaaddr_low( pccb->lpinfo.addr );		\
    buf.not_port_num = ( u_char )~buf.port_num;				\
    buf.lport_addr = ( u_char )buf.port_num;				\
    buf.opcode = SNDLB;							\
    ( void )bcopy( pccb->Lbdata, buf.lbdata, LBDSIZE );			\
    pccb->Lbcrc = ( u_long )~( crc(( u_char * )ci_crctable,		\
			           ( u_long )-1,			\
				   ( u_long )buf.lblen7,		\
				   ( u_char * )&buf ));			\
}

					/* Packet Header Macros
				         */
#define	Cable0_status( bp )	(( CABLE_STATUS * )&bp->status )->cable0
#define	Cable1_status( bp )	(( CABLE_STATUS * )&bp->status )->cable1
#define	Cnfrec( bp )		(( CNFRECH * )bp )
#define	Cselect( bp )		(( OPCODE_MODS * )&bp->flags )->cable_select
#define	Dstart( bp )		(( OPCODE_MODS * )&bp->flags )->Default_start
#define	Freset( bp )		(( OPCODE_MODS * )&bp->flags )->Force_reset
#define Idrec( bp )    		(( IDRECH * )bp )
#define	Lbrec( bp )		(( LBRECH * )bp )
#define	Receive_cable( bp )	(( CABLE_FLAGS * )&bp->flags )->rcable
#define	Send_cable( bp )	(( CABLE_FLAGS * )&bp->flags )->scable
#define	Sndlb( bp )		(( SNDLBH * )bp )

					/* Port Crashing Macros
					 */
/* Flushq() removes all packets on the target queue.  In performing this task
 * it makes the following assumptions:
 *
 * 1. The local CI port is disabled and can not access the target queue.
 * 2. The only thread active on the local port is the one flushing the queue.
 *
 * These assumptions allow Flushq() to immediately zero a locked target queue
 * without worrying about possible size effects on other threads or on the
 * local port itself.  Flushq() also zeroes a queue on failure to obtain a
 * queue interlock.  Zeroing a queues results in permanent loss of all packets
 * currently residing on it.
 */
#define	Flushq( pccb, q ) {						\
    if( q->flink ) {							\
	if( U_long( q->flink ) & Q_LOCKED ) {				\
	    q->flink = NULL;						\
	    q->blink = NULL;						\
	} else {							\
	    register GVPH	*gvpbp;					\
	    do	{							\
		gvpbp = ( GVPH * )remqhi( q, gvp_queue_retry );		\
		if(( long )gvpbp < 0 ) {				\
		    ( void )ci_dealloc_pkt( gvpbp );			\
		} else if( gvpbp ) {					\
		    q->flink = NULL;					\
		    q->blink = NULL;					\
		    gvpbp = NULL;					\
		}							\
	    } while( gvpbp );						\
	}								\
    }									\
}

					/* Port Register Access Macros
					 */
#define	Bad_reg( reg )		( BADADDR( reg, 4 ))
#define	Get_reg( reg )		(( !Bad_reg( reg )) ? *reg : 0 )

					/* Miscellaneous Macros
					 */
#define	Pesr_misc( code )	(( code & 0xFFFF0000 ) >> 16 )
#define Ppe2r_asblen( ppe2r )	( ppe2r & PPE2R_ASB_LEN )
#define Asblen_bytes( value )	( value * 512 )
#define Imp_lport_addr( ppr )	( ppr & PPR_LPORT_ADDR )
#define Exp_lport_addr( ppr, pper )	\
   ((( pper & PPER_MEM_NO ) >> 8 ) | (( ppr & PPR_LPORT_ADDR ) << 8 ))
#define EFR_MASK( value )	((( value ) & FCN_EXT2_EFR ) >> 7 )
#define FR_MASK( value )	((( value ) & FCN_EXT2_FR ) >> 7 )
#define NADP_MASK( value )	((( value ) & FCN_EXT2_NADP ) >> 1 )
#define Mmv( value )		(( value ) & FCN_EXT2_MMV )

					/* Shorthand Notations
					 */
#define	Asb	 	pd.gvp.type.ci.asb
#define	Asb_base	type.ci.asb_base
#define	Asb_len		type.ci.asb_len
#define	Bdt_base	type.ci.bdt_base
#define	Bdt_len		type.ci.bdt_len
#define	Ciadap		pd.gvp.type.ci.ciadap
#define	Ciisr		pd.gvp.type.ci.ciisr
#define	Ciregptrs	pd.gvp.type.ci.ciregptrs
#define	Cnfr		Ciregptrs.type.old.cnfr
#define	Dg_cache	pd.gvp.type.ci.dg_cache
#define	Devattn		pd.gvp.type.ci.devattn
#define	Dfreeq		pd.gvp.type.ci.dfreeq
#define	Dfreeq_hdr	type.ci.dfreeq_hdr
#define	Dqe_len		type.ci.dqe_len
#define	Dqe_logout	type.ci.dqe_logout
#define	Fn_level	pd.gvp.type.ci.fn_level
#define	Func_mask	type.ci.func_mask
#define	Gpt_base	type.ci.gpt_base
#define	Gpt_len		type.ci.gpt_len
#define	Invtcpkt	pd.gvp.ci.invtcpkt
#define	Interconnect	pd.gvp.type.ci.interconnect
#define	Keepalive	type.ci.keepalive
#define	Lbcrc		pd.gvp.type.ci.lbcrc
#define	Lbdata		pd.gvp.type.ci.lbdata
#define	Lbstatus	pd.gvp.type.ci.lbstatus
#define	Load_ucode	pd.gvp.type.ci.load_ucode
#define	Lpstatus	pd.gvp.type.ci.lpstatus
#define	Madr		Ciregptrs.type.old.madr
#define	Max_fn_level	pd.gvp.type.ci.max_fn_level
#define	Max_rom_level	pd.gvp.type.ci.max_rom_level
#define	Mdatr		Ciregptrs.type.old.mdatr
#define	Mfreeq		pd.gvp.type.ci.mfreeq
#define	Mfreeq_hdr	type.ci.mfreeq_hdr
#define	Mqe_len		type.ci.mqe_len
#define	Mrltab		pd.gvp.type.ci.mrltab
#define	Msg_cache	pd.gvp.type.ci.msg_cache
#define	Pcq0cr		Ciregptrs.pcq0cr
#define	Pcq1cr		Ciregptrs.pcq1cr
#define	Pcq2cr		Ciregptrs.pcq2cr
#define	Pcq3cr		Ciregptrs.pcq3cr
#define	Pdcr		Ciregptrs.pdcr
#define	Pdfqcr		Ciregptrs.pdfqcr
#define	Pecr		Ciregptrs.pecr
#define	Pesr		Ciregptrs.pesr
#define	Pfar		Ciregptrs.pfar
#define	Picr		Ciregptrs.picr
#define	Pkt_size	pd.gvp.type.ci.pkt_size
#define	Pkt_mult	pd.gvp.type.ci.pkt_mult
#define	Pmcsr		Ciregptrs.pmcsr
#define	Pmfqcr		Ciregptrs.pmfqcr
#define	Pmtcr		Ciregptrs.pmtcr
#define	Port_fcn	pd.gvp.type.ci.port_fcn
#define	Port_fcn_ext	pd.gvp.type.ci.port_fcn_ext
#define	Port_fcn_ext2	pd.gvp.type.ci.port_fcn_ext2
#define	Pqbbase		Ciregptrs.pqbbase
#define	Ppr		Ciregptrs.ppr
#define	Pper		Ciregptrs.pper
#define	Psrcr		Ciregptrs.psrcr
#define	Psr		Ciregptrs.psr
#define	Pstatus		pd.gvp.ci.pstatus
#define	Reinit_tries	pd.gvp.type.ci.reinit_tries
#define	Reset_port	pd.gvp.type.ci.reset_port
#define	Rom_level	pd.gvp.type.ci.rom_level
#define	Rport_state	pd.gvp.type.ci.rport_state
#define	Rpslogmap	pd.gvp.type.ci.rpslogmap
#define	Scpkt		pd.gvp.ci.scpkt
#define	Spt_base	type.ci.spt_base
#define	Spt_len		type.ci.spt_len
#define	Start_port	pd.gvp.type.ci.start_port
#define	St_level	Rom_level
#define	Disable_port	pd.gvp.type.ci.disable_port
#define	Ucode_rev	pd.gvp.type.ci.ucode_rev
#define	Vpqb_base	type.ci.vpqb_base
#define	Xpcpser		Ciregptrs.type.kmf.xpcpser
#define	Xpcpstat	Ciregptrs.type.kmf.xpcpstat

/* CI Port Specific Macros.
 */
#define Get_pgrp( pccb, cibp ) 					  	  \
    ( pccb->lpinfo.flags.expl ? ( cibp->opcode & 0x20 ) ?   		  \
    ( u_long )( cibp->srcport >> 8 ) : ( u_long )( cibp->dstport >> 8 ) : \
    ( u_long )( cibp->port ))

					/* CIBCA Ucode Revision Level Macros
				 	 */
#define	Cibca_fn_level( pccb )	(*(( u_char * )pccb->Bityp + CIBCA_FN_ADDR ))
#define	Cibca_st_level( pccb )	(*(( u_char * )pccb->Bityp + CIBCA_ST_ADDR ))

					/* CIXCD Ucode Revision Level Macros
				 	 */
#define	Cixcd_fn_level( pccb )	(*(( u_char * )pccb->Xdev  ))
#define	Cixcd_st_level( pccb )	(*(( u_char * )pccb->Xdev  ))

					/* Maintenance Interrupt Macros
					 */
/* These macros are used to manually poll for maintenance interrupts when port
 * interrupts have been explicitly disabled.  Interrupts are manually checked
 * for every 10 msec up to a maximum of 100 msec.
 */
#define	Ci7b_wait_mif() {						\
    register u_long	dsecs;						\
    for( dsecs = 10; dsecs > 0; --dsecs ) {				\
	if( *pccb->Pmcsr & CI7B_PMCS_MIF ) {				\
	    break;							\
	} else {							\
	    DELAY( 10000 );						\
	}								\
    }									\
}
#define	Cibx_wait_mif() {						\
    register u_long	dsecs;						\
    for( dsecs = 10; dsecs > 0; --dsecs ) {				\
	Lock_cidevice( pccb ) 						\
	if( *pccb->Psr & CIBX_PS_MIF ) {				\
	    Unlock_cidevice( pccb )					\
	    break;							\
	} else {							\
	    Unlock_cidevice( pccb )					\
	    DELAY( 10000 );						\
	}								\
    }									\
}
#define	Cibx_wait_unin() {						\
    register u_long	dsecs;						\
    for( dsecs = 20; dsecs > 0; --dsecs ) {				\
	Lock_cidevice( pccb ) 						\
	if( *pccb->Psr & CIBX_PS_UNIN ) {				\
	    Unlock_cidevice( pccb )					\
	    break;							\
	} else {							\
	    Unlock_cidevice( pccb )					\
	    DELAY( 10000 );						\
	}								\
    }									\
}
#define	Cibx_wait_pic_clear() {						\
    register u_long	dsecs;						\
    for( dsecs = 20; dsecs > 0; --dsecs ) {				\
	Lock_cidevice( pccb ) 						\
	if( !(*pccb->Psr & PSR_PIC) ) {					\
	    Unlock_cidevice( pccb )					\
	    break;							\
	} else {							\
	    Unlock_cidevice( pccb )					\
	    DELAY( 10000 );						\
	}								\
    }									\
}

					/* Microcode Related Macros
					 */
#define	Ci7b_allram( ucode )						\
    ((( CI7B_UCODEH * )ucode )->port_type == CI7B_ALLRAM )
#define	Cibca_aa_ucode( ucode )						\
    ( bcmp((( CIBCA_UCODEH * )ucode )->name,				\
	   "CIBCA.BIN ",						\
	   CIBCA_AA_NAMSIZ ) == 0 )
#define	Cibx_onboard( devreg ) (( u_long )devreg & CIBX_DEV_UCODE )
