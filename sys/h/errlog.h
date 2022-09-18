/*
 * @(#)errlog.h	4.16	(ULTRIX)	1/25/91
 */


/************************************************************************
 *									*
 *			Copyright (c) 1986 - 1990 by			*
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

/************************************************************************
 *		      Modification History		                *
 *									*
 * 14-Jan-91		Brian Nadeau					*
 *	Added TA91							*
 *									*
 * 07-Jan-91		Brian Nadeau					*
 *	Change name of RAH72 to RA71					*
 *									*
 * 18-Dec-90		Brian Nadeau					*
 *	Added RF31, RF72, RA92, RD32, RRD40				*
 *									* 
 * 02-Nov-90		Brian Nadeau					*	
 *	Added RA72, RAH72 						*
 *                                                                      *
 * 15-Oct-90		Randall Brown					*
 *	Added support for DS5000_100 (kn02ba).				*
 * 01-Oct-90            Stuart Hollander                                *
 *      Added VAX9000 XJA errors, and machine check                     *
 *      Added vector structure for 6400                                 *
 *                                                                      *
 * 07-Sep-90		Chran-Ham Chang    	                 	*
 *	Added el_fza structure for fza device.				*
 *									*
 * 07-Sep-90		Robin Miller					*
 *	Added ELSDT_RRD42, ELSDT_RX26, and ELSDT_RZ25 defines.		*
 *									*
 * 14-Aug-90            Stuart Hollander                                *
 *      Added support for Aquarius - VAX9000                            *
 *                                                                      *
 * 16-Jul-90            Janet L Schank                                  *
 *      Added ELSDT_RZ23L define                                        *
 *                                                                      *
 * 05-Jul-90		Bill Dallas					*
 *	Added scsi TZ07 (css) TZK10 , TZK08				*
 *                                                                      *
 * 15-Jun-90		Pete Keilty					*
 *	Added new ci_optfmask1 CI_EXPADRS for explicit address support. *
 *	Added size field in struct ci_packet.			 	*
 * 									*
 * 23-May-90            Robin						*
 *      Added yet another general use register for use in the		*
 *      elmemerr structure.						*
 *                                      				*
 * 01-May-90		Joe Szczypek                                    *
 *	Added ELADP_XBIPLUS, ELMCNTR_XMA2, el_xmp, el_xbiplus, el_xma2  *
 *	ELMCKT_KA6500, el_ka6500frame, ELCT_KA6500_INT54,               * 
 *      ELCT_KA6500_INT60.                                              *
 *                                                                      *
 * 20-Mar-90            Paul Grist                                      *
 *      Added support for MIPSMATE - DECsystem 5100.                    *
 *                                                                      *
 * 19-Mar-90            Janet L Schank                                  *
 *      Added ELSTT_RZ24, ELSTT_RZ57, and ELSTT_TZ05                    *
 *                                                                      *
 * 08-Mar-1990          Paul Grist                                      *
 *      Added VMEbus support, consisting of Adapter support for         *
 *      3max, Mipsfair-2, and Cmax, and a general purpose VMEbus        *
 *      device/controller packet for VMEbus drivers to report errors.   *
 *                                                                      *
 * 29-Jan-90            janet                                           *
 *      Added ELSTT_TLZ04 define for the RDAT.                          *
 *                                                                      *
 * 17-Jan-1990		Matthew Sacks                                   *
 *	removed ELCIHPT_CIXCB.  Added ELCIHPT_CIXCD, ELUQHPT_KDM70      *
 *	and ELMPCT_KDM70.  This is for supporting the uerf folks.       *
 * 							                *
 * 21-Nov-1989		David E. Eiche		DEE0081			*
 *	Alphabetized list of constituent structures in el_body		*
 *	and eliminated duplicate structures. (QAR02560)			*
 *									* 
 * 13-Nov-89 		Janet                                           *
 *      Added ASC driver devine ELSCCT_ASC                              *
 *                                                                      *
 * 29-Oct-89		Robin						*
 * 	1. Added DS5500 support.				        *
 *									*
 * 26-Oct-89	       	afd						*
 *	Added el_esrkn02 & kn02 mem ctlr type for 3MAX (DS5000).	*
 *									*
 * 25-Jul-1989		Fred Canter					*
 *	Redefined SCSI device type number so they don't			*
 *	interfere with the MSCP/TMSCP device numbers.			*
 *									*
 * 10-July-89	       	burns						*
 *	Added el_esrkn5800 for ISIS.					*
 *									*
 * 17-Jun-1989		Fred Canter					*
 *	Added data structures and definitions needed for logging	*
 *	errors from the SCSI device driver.				*
 *									*
 * 13-Jun-1989		Fred L. Templin					*
 *	Added XNA error log support for port fatal and non-fatal	*
 *	errors. See the file: if_xna.c for usage.			*
 *									*
 * 16-May-89		Kong						*
 * 	1. Added Mipsfair (DS5400) support.				*
 *	2. Also defined a generic error & status registers type packet.	*
 *	3. Reorganised the file to have all the vax only, mips only 	*
 *	   data types in one place.					*
 *									*
 * 09-May-1989		David E. Eiche		DEE0066			*
 *	Remove VAX "if defined" statements, so that a single		*
 *	copy of the error formatter can be used on all platforms.	*
 *									*
 * 12-May-89		darrell						*
 *	Merged Firefox (VAX60) V3.1 changes.				*
 *									*
 * 07-Mar-1989		Todd M. Katz		TMK0005			*
 *	1. Completely revise the msi event log packet format.		*
 *	2. Add scs event logging packet definitions.			*
 *									*
 * 26-Sep-1988		Tom Kong					*
 *	Added Rigel (VAX6400) support: added new data structures	*
 *	el_xrp which holds information for Rigel soft and hard errors,	*
 *	and el_mc6400frame which holds information for Rigel machine	*
 *	checks.								*
 *									*
 * 22-Feb-1989          Luis Arce / Tom Kong                            *
 *      Added Rigel (VAX6500) support: added new data structures        *
 *      el_xrp which holds information for Rigel soft and hard errors,  *
 *      and el_mc6500frame which holds information for Rigel machine    *
 *      checks.                                                         *
 *                                                                      *
 * 20-Jan-89		darrell						*
 *	Added more Firefox error structures				*
 *									*
 * 18-Nov-88		darrell						*
 *	Added Firefox M-bus error structures				*
 *									*
 * 23-Sep-1988		pmk					 	*
 *	changed EL_DUMPSIZE to sizeof(struct elbuf)			*
 *	moved #ifdef KERNEL from the declaration of elbuf to the	*
 *	the allocation of elbuf.					*
 *									*
 * 14-Sep-1988		Todd M. Katz		TMK0004			*
 *	1. sca error codes are now known as sca event codes.		*
 *	2. Completely redesign sca event code format including:		*
 *		1) Increasing the size of ESEVERITY by 1 bit.		*
 *		2) Redefining the sca event severity codes.		*
 *		3) Defining a new 2 bit field ESEVMOD( sca event	*
 *		   severity modifier codes ) and an initial set of	*
 *		   event modifiers.					*
 *		4) Changing the bit positions of ECLASS and ESUBCLASS.	*
 *		5) Deleting the GVP ESUBCLASS sca event subclass code.	*
 *		6) Reducing the size of ESUBCLASS by 1 bit		*
 *		7) Redefining the remaining sca event subclass codes.	*
 *		8. Define a new 1 bit field ECLALWAYS( event console	*
 *		   logging filter override ).				*
 *	3. Rename ci_optfmask -> ci_optfmask1.				*
 *	4. Rename cippd_optfmask -> cippd_optfmask1.			*
 *	5. Remove field cippdrswtype from the union cippdsystemopt.	*
 *	6. Add the following fields/structures:				*
 *		1) Structure cippdscommon to structure definition	*
 *	   	   elcippd_system.					*
 *		2) Fields ci_evpktver and ci_optfmask2 to structure	*
 *		   cicommon.						*
 *		3) Structure cicpurevlev to structure cidattnopt.	*
 *		4) Fields cippd_evpktver, cippd_optfmask2, and		*
 *		   cippd_npaths to structure cippdcommon.		*
 *		5) Field cippd_pstate to structure cippdpcommon.	*
 *		6) Field cippd_sysap and structure cippdnewpath to	*
 *		   union cippdpathopt.					*
 *	7. Delete CIPPD_RWSTYPE.					*
 *	8. Add the following ci optional event packet field bit mask	*
 *	   definitions: CI_EVPKTVER, and CI_CPUREVLEV.			*
 *	9. Add the following ci ppd optional event packet bit mask	*
 *	   definitions: CIPPD_SCOMMON, CIPPD_EVPKTVER, CIPPD_SYSAP, and	*
 *	   CIPPD_NEWPATH.						*
 *									*
 * 13-Sep-1988		Larry Cohen					*
 *	- increase max errlog message size to 4k.			*
 *	- increase msg packet ascii msg size to 2k.			*
 *									*
 * 12-Sep-1988  arce                                                    *
 *      cleaned up pmax support for uerf                                *
 *                                                                      *
 * 06-Sep-1988  afd                                                     *
 *      Added PMAX support:                                             *
 *          defined el_esrpmax struct and memory type for pmax          *
 *          ifdef'd VAX specifics (missing header files in pmax pool)   *
 *                                                                      *
 * 18-Aug-1988		Larry Cohen					*
 *	- increase size of the errlog buffer (3x)			*
 *									*
 * 08-Jun-1988		Todd M. Katz		TMK0003			*
 *	1. Add support for the CIXCB XMI to CI communications port by	*
 *	   adding:							*
 *		1) CI hardware port type code ELCIHPT_CIXCB.		*
 *		2) Structure definition cixmi_regs( xmi device		*
 *		   registers ) to the union definition ci_icregs within	*
 *		   the structure definition elci_dattn.			*
 *	2. Rename field ci_pcnf -> ci_cnfr and ci_per -> ci_pesr within	*
 *	   structure ciregs.  Rename field cippd_ksaddr[] ->		*
 *	   cippd_krsaddr[] within union cippddbcoll.			*
 *	3. Rename union elci_types -> citypes, structure elcidattn ->	*
 *	   cidattn, and structure elcilpkt -> cilpkt to maintain	*
 *	   consistent naming.						*
 *	4. Re-structure structure definition el_cippd into a common	*
 *	   part( cippdcommon ) and a union( cippdtypes ).  The union	*
 *	   has two members: structure cippdsystem for system specific	*
 *	   information; and, structure cippdpath for path specific	*
 *	   information.							*
 *	5. Add field cippdrswtype to the union cippdsystemopt, field	*
 *	   cippd_klsaddr[] to structure cippddbcoll, and field		*
 *	   cippd_optfmask to structure cippdcommon.			*
 *	6. Define the ci ppd optional error packet field bit mask	*
 *	   definitions.							*
 *	7. Add field ci_opfmask to structure cicommon.			*
 *	8. Define the ci optional error packet field bit mask		*
 *	   definitions.							*
 *									*
 * 05-Jun-1988		Ricky S. Palmer					*
 *	Added MSI port driver support.					*
 *									*
 * 31-May-88 -- afd							*
 *	Added Err/Status structure and memory contoller type		*
 *	for CVAX-star (ka420)						*
 *									*
 * 16-Apr-1988		Todd M. Katz		TMK0002			*
 *	1. Add CIBCA-BA support by adding ci hardware port type code	*
 *	   ELCIHPT_CIBCABA.						*
 *	2. Rename ELCIHPT_CIBCA -> ELCIHPT_CIBCAAA.			*
 *	3. Add cippd_lpname to structure cippd_common.			*
 *	4. Move ci_lsaddr, ci_lsysid, and ci_lname from structure	*
 *	   ci_lcommon to structure elci_common.				*
 *	5. Add ci_lpname to structure elci_common.			*
 *									*
 * 15-Mar-1988	jaw							*
 *	calypso packet clean-up						*
 *									*
 * 08-Mar-1988		Todd M. Katz		TMK0001			*
 *	Changes to ci adapter error log information:			*
 *	1. Created the union definition ci_icregs( optional		*
 *	   interconnect specific registers ) within the structure	*
 *	   definition elci_dattn and moved the structure cibiregs into	*
 *	   it.								*
 *	2. Added the structure cirevlev( optional CI microcode revision	*
 *	   levels ) to the union cidattnopt.				*
 *	3. Removed the ci_rreason and the structures ciprotocol and	*
 *	   cidbcoll from the union cilpktopt.				*
 *	4. Added the subsystem id packet el_cippd( ci ppd errors ) and	*
 *	   the software class type ELSW_CIPPD.				*
 *									*
 *  8-Mar-88    arce							*
 *      added ELVER and changed rhdr_elver to a short			*
 *									*
 * 19-Jan-88 -- jaw							*
 *	add vax6200 support						*
 *									*
 * 12-11-87	Robin L. and Larry C.					*
 *      Added portclass support to the system.				*
 *									*
 * 12-11-87	Robin L. and Larry C.					*
 *      Added portclass support to the system.				*
 *									*
 * 30-Mar-87 -- longo changed lynx structrure names from sx to lx       *
 *									*
 * 12-May-87 -- afd							*
 *	Changes for V1.5 of the Mayfair error spec:			*
 *	    mear => qbear;   sear => dear				*
 *									*
 * 20-Apr-87 -- afd							*
 *		Added IPCR0 to KA650 Error/Status reg pkt		*
 *									*
 * 06-Mar-87 -- afd							*
 *		Added CVAX mcheck frame and KA650 Error/Status reg pkt	*
 *									*
 * 04-Dec-86 -- pmk added address int to stack structure 	 	*
 *									*
 * 28-Aug-86 -- bjg added sxerr to el_rec union (shadowfax)		*
 *									*
 * 09-Jul-86 -- bjg added vaxstar memory controller define		*
 *									*
 * 11-Jun-86 -- bjg added support for tmscp controller and device 	*
 *		logging							*
 *									*
 * 10-Jun-86 -- map added support for mscp controller and device	*
 *		logging							*
 *									*
 * 03-Jun-86 -- bjg added lynx support					*
 *									*
 * 30-May-86 -- pmk  changed stack dump structure and used in el_pnc.	*
 *	also added two new ELMETYP defines.				*
 *									*
 * 14-May-86 -- pmk  added bvp structure, cleaned up bier,bigen,mniadp.	*
 *	added bvp defines, mscp disk defines & tmscp tape defines	*
 *									*
 * 29-Apr-86 -- jaw add error logging for nmi faults.			*
 *									*
 * 23-Apr-86 -- pmk   changed el_bua to el_bigen, added 630 memory	*
 *	type, and added bla device type.				*
 *									*
 * 02-Apr-86 -- pmk   changed EVALID selwakeup to schedeldaemon         *
 *									*
 * 01-Apr-86 -- map   added UQ controller type definitions		*
 *									*
 * 12-Mar-86	bjg   moved uba and sbi macro definitions to errlog.h 	*
 *			from kern_errlog.c				*
 *									*
 * 05-Mar-86 -- bjg   added new SIZE defines (APPEND, DUMP, 2048);	*
 *				added tmslg struct defn;		*
 *				added elbuf and defines 		*
 *									*
 * 21-Feb-86 -- bjg   added 8600, 8650 defines;remove pid,uid from      *
 *				block dev structure;removed ci_rev      *
 *				struct from ci dattn packet             *
 *									*
 * 19-Feb-86 -- pmk   added trailer definition.				*
 *									*
 * 14-Feb-86 -- map   add uq definition					*
 *									*
 * 12-Feb-86 -- pmk   changed 8800 nautilus, added ps/psl to bier & bua	*
 *									*
 * 04-Feb-86 -- jaw   hard link to /sys/vaxbi changed.			*
 *									*
 * 20-Jan-86 -- pmk							*
 *	Initial Creation for Error Logging				*
 *									*
 ************************************************************************/
#ifndef __ERRLOG__
#define __ERRLOG__ 

#ifdef KERNEL
#include "../h/ansi_compat.h"
#else
#include <ansi_compat.h>
#endif

#ifndef BI_INCLUDE
#include "../io/bi/bireg.h"
#endif /* BI_INCLUDE */

#ifndef	DEVIO_INCLUDE
#include "../h/devio.h"
#endif /*	DEVIO_INCLUDE */

#ifdef	__vax
#ifndef	SCSIVAR_INCLUDE
#include "../io/scsi/vax/scsivar.h"
#endif /*	SCSIVAR_INCLUDE */
#ifndef	SCSIREG_INCLUDE
#include "../io/scsi/vax/scsireg.h"
#endif /*	SCSIREG_INCLUDE */
#endif /*	__vax */

#ifdef	__mips
#ifndef	SCSIVAR_INCLUDE
#include "../io/scsi/mips/scsivar.h"
#endif /*	SCSIVAR_INCLUDE */
#ifndef	SCSIREG_INCLUDE
#include "../io/scsi/mips/scsireg.h"
#endif /*	SCSIREG_INCLUDE */
#endif /*	__mips */

#include "../io/sysap/mscp_msg.h"

#ifndef ERRLOG_AQ_INCLUDE
#include "../h/errlog_aq.h"
#endif /*	ERRLOG_AQ_INCLUDE */

/* Error Log Packet Structures */

/* class types (el_sub_id.subid_class) */

/* hardware detected errors 100-199 */
#define ELCT_MCK	100			/* machine check */
#define ELCT_MEM	101			/* mem. crd/rds */
#define ELCT_DISK	102			/* disk errs */
#define ELCT_TAPE	103			/* tape errs */
#define ELCT_DCNTL	104			/* device controller errs */
#define ELCT_ADPTR	105			/* adapter errs */
#define ELCT_BUS	106			/* bus errs */
#define ELCT_SINT	107			/* stray intr. */
#define ELCT_AWE	108			/* async. write err */
#define ELCT_EXPTFLT	109			/* panic exception/fault */
#define ELCT_NMIEMM	110			/* 8800 emm exception */
#define ELCT_CTE	111			/* console timeout entry */
#define ELCT_STKDMP	112			/* stack dump */
#define ELCT_ESR650	113			/* ka650 error & status regs */
#define ELCT_6200_INT60 114			/* vector 60 errors */
#define ELCT_6200_INT54 115			/* vector 54 errors */
#define ELCT_ESR420	116			/* ka420 error & status regs */

#define ELCT_ESRPMAX    117                     /* PMAX error & status regs */
#define	ELCT_6400_INT60 118			/* ka6400 vector 0x60 error */
#define	ELCT_6400_INT54 119			/* ka6400 vector 0x54 error */
#define ELCT_MBUS	120			/* Mbus errors		      */
#define ELCT_ESR60	121			/* ka60 error & status regs   */
#define	ELCT_ESR	130			/* Generic error&status regs*/
#define ELCT_INT60	131			/* Generic vector 0x60 (hard) error */
#define ELCT_INT54	132			/* Generic vector 0x54 (soft) error */
#define ELCT_9000_SYNDROME 133			/* 9000 syndrome entry */
#define ELCT_9000_KAF	134		/* 9000 keep alive failure from spu */
#define ELCT_9000_CLK	135			/* 9000 clock */
#define ELCT_9000_SCAN	136			/* 9000 scan */
#define ELCT_9000_CONFIG 137			/* 9000 configuration message */
#define ELCT_VECTOR	138			/* vector*/

/* software detected errors/events 200-249 */
#define ELSW_PNC 	200			/* panic (bug check) */
#define ELSW_CIPPD      201                     /* ci ppd events */
#define	ELSW_SCS	202			/* scs events */

/* informational ascii message 250-299 */
#define ELMSGT_INFO	250			/* info. type msg */
#define ELMSGT_SNAP8600 251			/* 8600 snapshot taken */

/* Operational Class 300-400 */

/* reload/restart 300-350 */
#define ELMSGT_SU	300			/* start up msg */
#define ELMSGT_SD	301			/* shutdown msg */

/* time stamp 310 */
#define ELMSGT_TIM	310			/* time stamp */


/* usage 311-315 */

/* statistics 316-319 */

/* maintenance 350-399 */
#define ELMSGT_DIAG	350			/* diag. info. type msg */
#define ELMSGT_REPAIR   351			/* repair */

/* mchk type frames (el_sub_id.subid_type)*/
#define ELMCKT_780	1			/* 780 machine chk frame */
#define ELMCKT_750	2			/* 750 machine chk frame */
#define ELMCKT_730	3			/* 730 machine chk frame */
#define ELMCKT_8600	4			/* 8600 machin chk frame */
#define ELMCKT_8200	5			/* 8200 machin chk frame */
#define ELMCKT_8800	6			/* 8800 machin chk frame */
#define ELMCKT_UVI	7			/* uvax-1 mach chk frame */
#define ELMCKT_UVII	8			/* uvax-2 mach chk frame */
#define ELMCKT_6200	9			/* fake 6200 machine frame */
#define ELMCKT_CVAX	10			/* cvax mach chk frame */
#define ELMCKT_PVAX	11			/* pvax mach chk frame */
#define	ELMCKT_6400	12			/* fake rigel mcheck frame*/
#define ELMCKT_6500	13			/* mariah mcheck frame */
#define	ELMCKT_9000	14			/* aquarius processor(not spu)*/
#define	ELMCKT_9000SPU	15			/* aquarius service processor */

/* stray intr types (el_sub_id.subid_type) */
#define ELSI_SCB	1			/* interrupt scb */
#define ELSI_UNI	2			/* interrupt unibus */

/* console timeout entry types (el_sub_id.subid_type) */
#define ELCTE_8600	1			/* for 8600 */
#define ELCTE_8800	2			/* for 8800 */
#define ELCTE_9000	3			/* for 9000 */

/* device error types (el_sub_id.subid_type) */
#define ELDEV_MSCP	1
#define ELDEV_REGDUMP	2
#define	ELDEV_SCSI	3

/* device controller error/event types (el_sub_id.subid_type) */
#define ELCI_ATTN	1
#define ELCI_LPKT	2
#define	ELUQ_ATTN	3
#define ELBI_BLA	4
#define ELBI_BVP	5
#define ELMSCP_CNTRL	6
#define ELTMSCP_CNTRL	7
#define ELMSI_ATTN	8
#define ELMSI_LPKT	9
#define	ELSCSI_CNTRL	10
#define	ELBI_XNA	11
#define	ELXMI_XNA	12
#define ELVME_DEV_CNTL  13                     /* general purpose VME dev/ctrl packet */
#define ELFZA		14

/* adapter error types (el_sub_id.subid_type) */
#define ELADP_UBA	1
#define ELADP_BUA	2
#define ELADP_NBI	3
#define ELADP_XBI	4
#define ELADP_NBW	5
#define ELADP_VBA       6                       /* VMEbus adapters */
#define ELADP_XBIPLUS   7
#define ELADP_SJASCM	8
#define ELADP_XJA	9	/* XMI to Jbox adapter */

/* bus error types (el_sub_id.subid_type) */
#define ELBUS_SBI780	1			/*  780 sbi fault type */
#define ELBUS_SBI8600	2			/* 8600 sbi faults */
#define ELBUS_BIER	3			/* 8200 bi errors */
#define ELBUS_NMIFLT	4			/* 8800 nmi fault */

/* stack dump error types (el_sub_id.subid_type) */
#define ELSTK_KER	1
#define ELSTK_INT	2
#define ELSTK_USR	3

/* error & status types (el_sub_id.subid_type) */
#define	ELESR_5400	1			/* DS5400 ESR	*/
#define	ELESR_5800	2			/* DS5800 ESR	*/
#define	ELESR_kn02	3			/* DS5000 ESR   */
#define	ELESR_5500	4			/* DS5500 ESR	*/
#define ELESR_5100      5                       /* DS5100 ESR   */
#define ELESR_KN02BA	6			/* DS5000_100 ESR */

/* Soft & Hard error types (el_sub_id.subid_type) */
#define ELINT_6500      1                       /* VAX6500 CPU */

/* Vector error types (el_sub_id.subid_type) */
#define ELVEC_MCK	1                       /* machine check */
#define ELVEC_HARD      2                       /* hard error */
#define ELVEC_SOFT      3                       /* soft error */
/* vector processor types (el_sub_id.subid_ctldevtyp) */
#define ELVEC_6400      1                       /*  */
#define ELVEC_6500      2                       /*  */
#define ELVEC_9000      3                       /*  */

/* mem cntl types (el_sub_id.subid_ctldevtyp) */
#define ELMCNTR_780C	1			/* 780C mem. cntl */
#define ELMCNTR_780E	2			/* 780E mem. cntl */
#define ELMCNTR_750	3			/* 750/730 mem. cntl. */
#define ELMCNTR_730	4			/* 750/730 mem. cntl. */
#define ELMCNTR_8600	5			/* 8600 mem. cntl */
#define ELMCNTR_BI	6			/* 8200 BI mem. cntl. */
#define ELMCNTR_NMI	7			/* 8800 NMI mem. cntl. */
#define ELMCNTR_630	8			/* 630 mem. cntl. */
#define ELMCNTR_VAXSTAR	9			/* vaxstar mem. cntl. */
#define ELMCNTR_650	10			/* 650 mem. cntl. */
#define ELMCNTR_6200	11			/* 6200 mem. cntl. */
#define ELMCNTR_420	12			/* 420 mem. cntl. */
#define ELMCNTR_PMAX    13                      /* PMAX mem. cntl. */
#define ELMCNTR_60	15			/* 60 mem. cntl. */
#define	ELMCNTR_5400	16			/* DS5400 mem cntl. */
#define	ELMCNTR_5800	17			/* DS5800 mem cntl. */
#define	ELMCNTR_kn02	18			/* DS5000 (3max) mem cntl. */
#define	ELMCNTR_5500	19			/* DS5500 mem cntl. */
#define ELMCNTR_XMA2	20			/* Enhanced XMI mem. cntl. */
#define ELMCNTR_5100    25                      /* DS5100 mem cntl. */
#define	ELMCNTR_9000_SE	26	/* 9000 spu or main mem cntl. soft err */
#define	ELMCNTR_9000_HE	27	/* 9000 spu or main mem cntl. hard err */
#define ELMCNTR_KN02BA	28			/* DS5000_100 mem. cntl. */

/* ci hardware port types (el_sub_id.subid_ctldevtyp) */
#define	ELCIHPT_CI780	2
#define	ELCIHPT_CI750	3
#define	ELCIHPT_CIBCI	5
#define	ELCIHPT_CIBCABA	10
#define	ELCIHPT_CIBCAAA	11
#define	ELCIHPT_CIXCD	14

/* msi hardware port type (el_sub_id.subid_ctldevtyp) */
#define ELMSIHPT_SII	32

/* bvp hardware port types (el_sub_id.subid_ctldevtyp) */
#define	ELBVP_AIE	1
#define	ELBVP_AIE_TK	2
#define	ELBVP_AIO	3
#define ELBVP_ACP	4
#define ELBVP_SHDWFAX	5

/* uq hardware port types (el_sub_id.subid_ctldevtyp) */
#define	ELUQHPT_UDA50	0		
#define	ELUQHPT_RC25	1
#define	ELUQHPT_RUX50	2		
#define	ELUQHPT_TK50	3		
#define	ELUQHPT_TU81	5
#define	ELUQHPT_UDA50A	6		
#define	ELUQHPT_RQDX	7
#define	ELUQHPT_KDA50	13
#define	ELUQHPT_TK70	14
#define	ELUQHPT_RV20	15
#define	ELUQHPT_RRD50	16
#define	ELUQHPT_KDB50	18
#define	ELUQHPT_RQDX3	19
#define ELUQHPT_KDM70	27

/* controller types mscp (el_sub_id.subid_ctldevtyp) */
#define	ELMPCT_HSC50	1		
#define	ELMPCT_UDA50	2		
#define	ELMPCT_RC25	3
#define	ELMPCT_VMS	4
#define	ELMPCT_TU81	5
#define	ELMPCT_UDA50A	6		
#define	ELMPCT_RQDX	7
#define	ELMPCT_TOPS	8
#define	ELMPCT_TK50	9		
#define	ELMPCT_RUX50	10		
#define	ELMPCT_KFBTA	12
#define	ELMPCT_KDA50	13
#define	ELMPCT_TK70	14
#define	ELMPCT_RV20	15
#define	ELMPCT_RRD50	16
#define	ELMPCT_KDB50	18
#define	ELMPCT_RQDX3	19
#define	ELMPCT_RQDX4	20
#define	ELMPCT_SII_DISK	21
#define	ELMPCT_SII_TAPE	22
#define	ELMPCT_SII_DISK_TAPE	23
#define	ELMPCT_SII_OTHER	24
#define	ELMPCT_KDM70	27
#define	ELMPCT_HSC70	32
#define	ELMPCT_HSB50	64
#define	ELMPCT_RF30	96
#define	ELMPCT_RF71	97
#define	ELMPCT_ULTRIX	248

/* controller types SCSI (el_sub_id.subid_ctldevtyp) */
#define	ELSCCT_5380	1
#define	ELSCCT_SII	2
#define ELSCCT_ASC	3
/* disks types mscp (el_sub_id.subid_ctldevtyp) */
#define ELDT_RA80	1
#define ELDT_RC25	2
#define ELDT_RCF25	3
#define ELDT_RA60	4
#define ELDT_RA81	5
#define ELDT_RD51	6
#define ELDT_RX50	7
#define ELDT_RD52	8
#define ELDT_RD53	9
#define ELDT_RX33	10
#define ELDT_RA82	11
#define ELDT_RD31	12
#define ELDT_RD54	13
#define ELDT_RRD50	14
#define ELDT_RV20	15
#define ELDT_RD32	15
/* unused	 	16 */
#define ELDT_RX18	17
#define ELDT_RA70	18
#define ELDT_RA90	19
#define ELDT_RX35	20
#define ELDT_RF30	21
#define ELDT_RF71	22
#define ELDT_SVS00	23
#define ELDT_RD33	24
#define ELDT_ESE20	25
#define ELDT_RRD40	26
#define ELDT_RF31	27
#define ELDT_RF72	28
#define ELDT_RA92	29
#define ELDT_RA72	37
#define ELDT_RA71	40

/* tapes types tmscp (el_sub_id.subid_ctldevtyp) */
#define ELTT_TA78	1
#define ELTT_TU81	2
#define ELTT_TK50	3
#define ELTT_TA81	4
#define ELTT_TA79	5
#define ELTT_TA90	7
#define ELTT_RV60	8
#define ELTT_SVS00	9
#define ELTT_TA91	12
#define ELTT_TK70	14
#define ELTT_TRV20	15

/* disks types SCSI (el_sub_id.subid_ctldevtyp) */
#define	ELSDT_RX23	1
#define ELSDT_RX33	2
#define	ELSDT_RZ22	3
#define	ELSDT_RZ23	4
#define	ELSDT_RZ55	5
#define	ELSDT_RZ56	6
#define	ELSDT_RRD40	7
#define	ELSDT_RZxx	8
#define ELSDT_RZ24      9
#define ELSDT_RZ57      10
#define ELSDT_RZ23L     11
#define ELSDT_RRD42	12
#define ELSDT_RX26	13
#define ELSDT_RZ25	14

/* tapes types SCSI (el_sub_id.subid_ctldevtyp) */
#define	ELSTT_TZ30	1
#define	ELSTT_TZK50	2
#define	ELSTT_TZxx	3
#define ELSTT_TLZ04     4
#define ELSTT_TZ05      5
#define ELSTT_TZ07	6
#define ELSTT_TZK08	7
#define	ELSTT_TZK10	8

/* VMEbus adapter types (el_sub_id.subid_ctldevtype) */
#define ELDT_3VIA       1                      /* 3max VME adapter */
#define ELDT_MVIA       2                      /* Mipsfair-2 VME adapter */
#define ELDT_XBIA       3                      /* Cmax VME adapter (XMI) */

/* panic exception/fault errcodes (el_sub_id.subid_type) */
#define ELEF_RAF	1			/* reserved addr fault */
#define ELEF_PIF	2			/* privileged instr fault */
#define ELEF_ROF	3			/* reserved operand fault */
#define ELEF_BPT	4			/* bpt instr fault */
#define ELEF_XFC	5			/* xfc instr fault */
#define ELEF_SYSCALL	6			/* system call exception/fault*/
#define ELEF_AT		7			/* arithmetic exception/fault */
#define ELEF_AST	8			/* ast exception/fault */
#define ELEF_SEG	9			/* segmentation fault */
#define ELEF_PRO	10			/* protection fault */
#define ELEF_TRACE	11			/* trace exception/fault */
#define ELEF_CMF	12			/* compatibility mode fault */
#define ELEF_PF		13			/* page fault  */
#define ELEF_PTF	14			/* page table fault */

/* elmemerr.type */
#define ELMETYP_CRD	1
#define ELMETYP_RDS	2
#define ELMETYP_CNTL	3
#define ELMETYP_WMASK	4
#define ELMETYP_PAR	5
#define ELMETYP_NXM	6

/* el_sub_id.subid_num  for machine check is cpu number
 * 780,750,730  num = 0
 * MVAX		num = 0
 * 8600		num = 0
 * 8200 	num = bi node number
 * 8800	naut.	num = 0 or 1, right or left cpu
 */

/* 780/8600 sbi error codes (el_sub_id.subid_errcode) */
#define SBI_ERROR 0
#define SBI_WTIME 1		/* 780 only */
#define SBI_ALERT 2
#define SBI_FLT   3
#define SBI_FAIL  4

#define Elsbi elrp->el_body.elsbia8600
#define Eluba elrp->el_body.eluba780
#define Sbi8600 elrp->el_body.elsbia8600
#define Sbi780 elrp->el_body.elsbiaw780

/* sca( scs, bvp, uq, ci ppd, ci, msi ) event codes definition
 * ( el_sub_id.subid_errcode )
 *
 *  3 3 2 2 2 2 2 2 2 2 2 2 1 2 1 1 1 1 1 1 1 1 
 *  1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
 * +-+-+-+---+-----+-----------------------------------------------+
 * |E|E| |   |  E  |                                               |
 * |C|S|E| E |  S  |                                               |
 * |L|U|C| S |  E  |                                               |
 * |A|B|L| E |  V  |                                               |
 * |L|C|A| V |  E  |                    ECODE                      |
 * |W|L|S| M |  R  |                                               |
 * |A|A|S| O |  I  |                                               |
 * |Y|S| | D |  T  |                                               |
 * |A|S| |   |  Y  |                                               |
 * +-+-+-+---+-----+-----------------------------------------------+
 *		
 *  Bits		     		Function
 * -----		-----------------------------------------
 *  0-23		ECODE     - Event Code Number
 * 24-26		ESEVERITY - Event Severity Codes
 *		            Informational	0x00
 *			    Warning		0x01
 *			    Remote Error	0x02
 *			    Error		0x03
 * 		            Severe Error	0x04
 * 		            Fatal Error		0x05
 *			    RSVD( Future Use )	0x06
 *			    RSVD( Future Use )	0x07
 * 27-28		ESEVMOD	  - Event Severity Modifier Codes
 *			    None		0x00
 *			    Path Crash		0x01
 *			    Local Port Crash	0x02
 *			    RSVD( Future Use )	0x03
 *    29		ECLASS	  - Event Class Code
 *			    SCS			0x00
 *			    PD			0x01
 *    30		ESUBCLASS - Event Subclass Code( PD dependent )
 *			    PD			0x00( MSI/CI/BVP/UQ )
 *			    PPD			0x01( MSI/CI )
 *    31		ECLALWAYS - Event Console Loggging Filter Override
 *		
 * Event codes ( ECODE ) are densely assigned for each possible combination
 * of ESEVERITY, ECLASS, and ESUBCLASS.
 *
 *			Definition of Severity Conditions
 *			---------------------------------
 *
 * Informational:		Notifies of a fully successful event.
 *				Notifies of a purely informative event.
 *				Does NOT increment any error counters.
 *
 * Warning:			Warns of possible problems associated with an
 *				 otherwise successful event.
 *				Does NOT increment any error counters.
 *
 * Remote Error:		Notifies of the occurrence of a remote error.
 *				Does NOT increment any error counters.
 *
 * Error:			Notifies of the occurrence of a local
 *				 recoverable error associated with a specific
 *				 path or local port.
 *				May have the path crash modifier applied.
 *				Increments the number of errors associated with
 *				 the appropriate local port.
 *
 * Severe Error:		Notifies of the occurrence of a severe( but
 *				 still recoverable ) local error associated
 *				 with a specific path or local port.
 *				May have the path or local port crash severity
 *				 modifier applied.
 *				Increments the number of errors associated with
 *				 the appropriate local port.
 *
 * Fatal Error:			Notifies of the occurrence of a fatal
 *				 non-recoverable error associated with a
 *				 specific local port.
 *				Increments the number of errors associated with
 *				 the local port.
 *				Always logged to the console.
 */

/*  elbd_flags (el_bdhdr) */
/* 0x00 = write */
/* 0x01 = read */
/* 0x02 = done */
/* 0x04 = error */

/* uq error codes definition (el_sub_id.subid_errcode)
 */
#define ELUQ_SA_FATAL           1       /* Fatal error in SA register   */
#define ELUQ_RESET_FAIL         2       /* Initialization failed        */

/* XNA error codes (el_sub_id.subid_errcode) */
#define	XNA_FATAL	0
#define	XNA_NONFATAL	1

/* VMEbus device/controller errcode (el_sub_id.subid_errcode) */
#define VME_DEVICE      1
#define VME_CONTROLLER  2


/* VMEbus adapter error codes (el_sub_id.subid_errcode) */
#define VBA_VME_ERROR         0                /* general VME error */
#define VBA_VME_PARITY        1                /* general parity errors */
#define VBA_VME_TIMEOUT       2                /* vme timeout errors */
#define VBA_IBUS_CABLE_FLT    3                /* ibus cable fault */
#define VBA_IBUS_PARITY       4                /* ibus parity errors */
#define VBA_RMW_INTERLOCK     5                /* Read Mod Write/Interlock */
#define VBA_VME_BERR          6                /* VMEbus *BERR signal */
#define VBA_XBIA_INTERNAL     7                /* XBIA internal gate array */
#define VBA_IO_WRITE_FAIL     8                /* I/O write failure */
#define VBA_PMAP_FAULT        9                /* page map fault errors */
#define VBA_YABUS_ERROR      10                /* YAbus errors */
#define VBA_VME_MOD_FAIL     11                /* VME module failure */
#define VBA_VME_AC_LOW       12                /* VME AC below spec */
#define VBA_VIC_LOC_TOUT     13                /* VIC chip local timeout */
#define VBA_VIC_SELF_ACCESS  14                /* VIC self-acess select err */
#define VBA_VIC_LBERR        15                /* VIC *LBERR signal */
#define VBA_INVALID_PFN      16                /* invalid PFN */
#define VBA_MULTIPLE_ERRS    17                /* multiple errors */
#define VBA_CORR_ECC         18                /* correctable ECC */
#define VBA_UNCORR_ECC       19                /* uncorrectable ECC */
#define VBA_SYS_RESET        20                /* VME reset signal asserted */

/* current ci event packet version
 * (el_body.elci.cicommon.ci_evpktver)
 */
#define	CI_EVPKTVER		 0

/* ci optional event packet field bit mask definitions
 * (el_body.elci.cicommon.ci_optfmask1)
 *
 */
					/* ci device attention info fields */
#define	CI_REGS		0x00000001	/*  ci port registers */
#define	CI_BIREGS	0x00000002	/*  biic device registers */
#define	CI_XMIREGS	0x00000004	/*  xmi device registers */
#define	CI_UCODE	0x00000008	/*  bad ucode information */
#define	CI_REVLEV	0x00000010	/*  out-of-rev/invalid ucode info */
#define	CI_CPUREVLEV	0x00000020	/*  out-of-rev CPU ucode information */
					/* ci logged packet info fields */
#define	CI_LCOMMON	0x00000040	/*  common logged packet information */
#define	CI_PACKET	0x00000080	/*  logged packet */
#define	CI_EXPADRS	0x00000100	/*  explicit addrs format used  */
					/*  with logged packet */

/* current ci ppd event packet version
 * (el_body.elcippd.cippdcommon.cippd_evpktver)
 */
#define	CIPPD_EVPKTVER		 0

/* ci ppd optional event packet field bit mask definitions
 * (el_body.elcippd.cippdcommon.cippd_optfmask1)
 *
 */
					/* ci ppd sub id fields */
#define	CIPPD_CLTDEVTYP	0x00000001	/*  controller/device type */
#define	CIPPD_CLTDEVNUM	0x00000002	/*  controller number */
					/* ci ppd path specific info fields */
#define	CIPPD_PCOMMON	0x00000004	/*  common path specific information */
#define	CIPPD_DBCOLL	0x00000008	/*  database collision information */
#define	CIPPD_SYSAP	0x00000010	/*  name of local sysap crashing path*/
#define	CIPPD_NEWPATH	0x00000020	/*  new path information */
#define	CIPPD_PPACKET	0x00000040	/*  ci ppd logged packet */
					/* ci ppd common sys lev info fields */
#define	CIPPD_SCOMMON	0x00000080	/*  common system level information */
#define	CIPPD_PROTOCOL	0x00000100	/*  ci ppd protocol information */
#define	CIPPD_SPACKET	0x00000200	/*  ci ppd logged packet */

/* current msi event packet version
 * (el_body.elmsi.msicommon.msi_evpktver)
 */
#define	MSI_EVPKTVER		 0

/* msi optional event packet field bit mask definitions
 * (el_body.elmsi.msicommon.msi_optfmask1)
 *
 */
					/* msi device attention info fields */
#define	MSI_REGS	0x00000001	/*  msi port registers */
					/* msi logged packet info fields */
#define	MSI_LCOMMON	0x00000002	/*  common logged packet information */
#define	MSI_CMDBLK	0x00000004	/*  logged packet command block info */
#define	MSI_PACKET	0x00000008	/*  logged packet */

/* current scs event packet version
 * (el_body.elscs.scscommon.scs_evpktver)
 */
#define	SCS_EVPKTVER		 0

/* scs optional event packet field bit mask definitions
 * (el_body.elscs.scscommon.scs_optfmask1)
 */
					/* scs sub id fields */
#define	SCS_CLTDEVTYP	0x00000001	/*  controller/device type */
#define	SCS_CLTDEVNUM	0x00000002	/*  controller number */
					/* scs optional information fields */
#define	SCS_CONN	0x00000004	/*  scs connection information */
#define	SCS_LDIRID	0x00000008	/*  local directory id number */
#define	SCS_RREASON	0x00000010	/*  connection rejection reason */

#define EL_INVALID 	0
#define EL_VALID 	1
#define EL_UNDEF	-1
#define EL_PRISEVERE	1
#define EL_PRIHIGH	3
#define EL_PRILOW	5
#define EL_STKDUMP	512	
#define EL_SIZE256	256
#define EL_SIZE128	128
#define EL_SIZE64	64
#define EL_SIZE16	16
#define EL_SIZE12	12
#define EL_SIZE2048	2048
#define EL_SIZEAPPND	1400
#define EL_DUMPSIZE 	sizeof(struct elbuf)
#define EL_BSIZE	(3 * 8192 - 3 * sizeof(char *))
#define EL_SSIZE	(2048 - sizeof(char *))
#define EL_BEG elbuf.kqueue
#define EL_END elbuf.kqueue + EL_BSIZE
#define EL_SBEG elbuf.squeue
#define EL_SEND elbuf.squeue + EL_SSIZE

#define EL_MAXRECSIZE	4096
#define EL_MAXAPPSIZE	(EL_MAXRECSIZE - EL_MISCSIZE)
#define EL_FULL (struct el_rec *) 0

#define EL_RHDRSIZE	sizeof(struct el_rhdr)
#define EL_SUBIDSIZE	sizeof(struct el_sub_id)
#define EL_TRAILERSIZE  4
#define EL_MISCSIZE  	(EL_RHDRSIZE + EL_SUBIDSIZE + EL_TRAILERSIZE)
#define EL_CILPKTSUBTRACT  EL_MISCSIZE + sizeof(struct ci_common)+ \
			sizeof(struct ci_lcommon) + sizeof(struct ci_packet)
#define EL_EXPTFLTSIZE	sizeof(struct el_exptflt)
#define EL_UBASIZE	sizeof(struct el_uba780)
#define EL_PNCSIZE	sizeof(struct el_pnc)
#define EL_MEMSIZE	sizeof(struct el_mem)

#define EL_REGMASK	0x012f3f19

#define ELVER   2


#define trailer "%~<^"

extern struct proc *elprocp;
extern int schedeldaemon();
extern struct callout *callfree;
#define EVALID(eptr) \
	eptr->elrhdr.rhdr_valid = EL_VALID; \
	if (callfree != 0) { \
		(void)schedeldaemon(); \
	}

#define LSUBID(eptr, class, type, ctldev, num, unitnum, errcode) \
	eptr->elsubid.subid_class  = class; \
	eptr->elsubid.subid_type = type; \
	eptr->elsubid.subid_ctldevtyp = ctldev; \
	eptr->elsubid.subid_num = num; \
	eptr->elsubid.subid_unitnum = unitnum; \
	eptr->elsubid.subid_errcode = errcode;

/* Error Logger Buffer */
/* kqueue full implies |in-out| = 1 */
struct elbuf {
	char *in;		/* input ptr */
	char *out;		/* output ptr */
	char *le;		/* logical end */
	char kqueue[EL_BSIZE];
	char *sin;		/* input ptr for severe area */
	char squeue[EL_SSIZE];
};
#ifdef KERNEL
struct elbuf elbuf;
#endif /* KERNEL */

struct el_rhdr {			/* errlog header */
	u_short rhdr_reclen;		/* errlog record length */
	u_short rhdr_seqnum; 		/* seq. number */
	u_long rhdr_time;		/* time in sec */
	u_long rhdr_sid;		/* system id, filled in by elcs */
	u_char rhdr_valid;		/* valid error record */
	u_char rhdr_pri;		/* priority hi - low */
	u_short rhdr_elver;		/* errlog version,filled in by elcs */
	char rhdr_hname[EL_SIZE12];	/* host name, filled in by elcs */
	u_long rhdr_systype;		/* system type register (mvax's) */
	u_long rhdr_mpnum;		/* number of processors in system */
	u_long rhdr_mperr;		/* which cpu serviced the error */
};

struct el_sub_id {			/* sub id packet */
	u_short subid_class;		/* class type, ELCT_DISK */
	u_char subid_type;		/* error type, ELDEV_MSCP */
	u_char subid_ctldevtyp;		/* controller/device type,  ELDT_RA60 */
	u_short subid_num;		/* cpu number - mchk errors */
					/* bus number - bus/adptr/cntr errors */
					/* cntl number - device errors */
	u_short subid_unitnum;		/* adpt number - adapter errors */
					/* cntl number - controller errors */
					/* unit number - device errors */
	u_long subid_errcode;		/* ci,mck summ.,cte,emm,exptflt */
};

struct el_mem {				/* mem. crd/rds packet */
	short elmem_cnt;		/* num. of mem. err structures */
	struct el_memerr {
		short cntl;		/* cntl. number 1-? */
		u_char type;		/* type err 1-crd,2-rds,3-cntl,4-wmask*/
		u_char numerr;		/* num. of errors on this address */
		int regs[5];		/* mem. regs */
	} elmemerr;			/* mem. err structure */
};

struct el_devhdr {			/* device header packet */
	dev_t devhdr_dev;		/* dev. major/minor numbers */
	long devhdr_flags;		/* buffer flags */
	long devhdr_bcount;		/* byte count of transfer */
	daddr_t devhdr_blkno;		/* logical block number */
	short devhdr_retrycnt;		/* retry count */
	short devhdr_herrcnt;		/* hard err count total */
	short devhdr_serrcnt;		/* soft err count total */
	short devhdr_csr;		/* device csr */
};

struct el_bdev {                        /* block device packet disk/tape */
        struct el_devhdr eldevhdr;      /* device header packet */
        union {
                u_int devreg[22];       /* device regs. non mscp drivers */
                struct el_mslg {
                    long mslg_len;      /* mscp packet length */
                    MSLG mscp_mslg;     /* mscp/tmscp datagram packet */
                } elmslg;
		struct el_scsi elscsi;	/* SCSI eror info and registers */
        } eldevdata;
};

struct el_sbi_aw780 {			/* 780 sbi fault/async. write packet */
	int sbiaw_er;			/* sbi error reg */
	int sbiaw_toa;			/* time out address */
	int sbiaw_fs;			/* sbi fault status */
	int sbiaw_sc;			/* sbi silo compare */
	int sbiaw_mt;			/* sbi maint. reg. */
	int sbiaw_silo[EL_SIZE16];	/* sbi silo 16 regs */
	int sbiaw_csr[EL_SIZE16];	/* sbi csr's  num. nexi */
	int sbiaw_pc;
	int sbiaw_psl;
};

struct el_sbia8600 {			/* 8600 sbi fault/alert/error packet */
	int sbia_ioaba;			/* ioa baseaddress */
	int sbia_dmacid;		/* dmac id reg */
	int sbia_dmacca;		/* dmac cmd-addr reg */
	int sbia_dmabid;		/* dmab id reg */
	int sbia_dmabca;		/* dmab cmd-addr reg */
	int sbia_dmaaid;		/* dmaa id reg */
	int sbia_dmaaca;		/* dmaa cmd-addr reg */
	int sbia_dmaiid;		/* dmai id reg */
	int sbia_dmaica;		/* dmai cmd-addr reg */
	int sbia_ioadc;			/* ioa diag cntl reg */
	int sbia_ioaes;			/* ioa error summary reg */
	int sbia_ioacs;			/* ioa cntl-status reg */
	int sbia_ioacf;			/* ioa config reg */
	int sbia_er;			/* sbi error reg */
	int sbia_to;			/* sbi time out address */
	int sbia_fs;			/* sbi fault status */
	int sbia_sc;			/* sbi silo compare */
	int sbia_mr;			/* sbi maint. reg */
	int sbia_silo[EL_SIZE16];	/* silo regs 16 */
	int sbia_csr[EL_SIZE16];	/* sbi csr's  num nexi */
	int sbia_pc;
	int sbia_psl;
};

struct el_uba780 {	 		/* 780 uba fault/error packet */
	int uba_cf;			/* uba config reg */
	int uba_cr;			/* uba control reg */
	int uba_sr;			/* status reg */
	int uba_dcr;			/* diag. cntl. reg. */
	int uba_fmer;			/* failed map entry reg */
	int uba_fubar;			/* failed unibus addr. reg */
	int uba_pc;
	int uba_psl;
};

struct el_esr650 {			/* 650 Error & Status Registers */
	u_long esr_cacr;		/* Cache Control Reg */
	u_long esr_dser;		/* DMA System Error Reg */
	u_long esr_qbear;		/* QBus Error Address Reg */
	u_long esr_dear;		/* DMA Error Address Reg */
	u_long esr_cbtcr;		/* CDAL Bus Timeout Control Reg */
	u_short esr_ipcr0;		/* InterProcessor Com Reg for Arbiter */
	u_long esr_cadr;		/* Cache Disable Reg */
	u_long esr_mser;		/* Memory System Error Reg */
};

struct el_esr420 {			/* 420 Error & Status Registers */
	u_long esr_cacr;		/* Cache Control Reg */
	u_long esr_cadr;		/* Cache Disable Reg */
	u_long esr_mser;		/* Memory System Error Reg */
};

struct el_esrpmax {                     /* PMAX Error & Status Registers */
        u_long esr_cause;               /* Cause Reg */
        u_long esr_epc;                 /* Exception PC (resume PC) */
        u_long esr_status;              /* Status Reg */
        u_long esr_badva;               /* Bad Virtual Address Reg */
        u_long esr_sp;                  /* Stack Ptr */
};

struct el_esr5100 {                     /* MIPSMATE Error & Status Registers */
        u_long esr_cause;               /* Cause Reg */
        u_long esr_epc;                 /* Exception PC (resume PC) */
        u_long esr_status;              /* Status Reg */
        u_long esr_badva;               /* Bad Virtual Address Reg */
        u_long esr_sp;                  /* Stack Ptr */
	u_long esr_icsr;                /* Interrupt CSR */
	u_long esr_leds;                /* LED register */
	u_long esr_wear;                /* Write Error Add. register */
	u_long esr_oid;                 /* Option ID register */
};

struct el_esrkn02 {                     /* 3MAX Error & Status Registers */
        u_long esr_cause;		/* Cause Reg */
        u_long esr_epc;			/* Exception PC (resume PC) */
        u_long esr_status;		/* Status Reg */
        u_long esr_badva;		/* Bad Virtual Address Reg */
        u_long esr_sp;			/* Stack Ptr */
        u_long esr_csr;			/* system csr */
        u_long esr_erradr;		/* Error Address reg */
};

struct el_esrkn02ba {                     /* 3MAX Error & Status Registers */
        u_long esr_cause;		/* Cause Reg */
        u_long esr_epc;			/* Exception PC (resume PC) */
        u_long esr_status;		/* Status Reg */
        u_long esr_badva;		/* Bad Virtual Address Reg */
        u_long esr_sp;			/* Stack Ptr */
        u_long esr_ssr;                 /* system support reg */
	u_long esr_sir;             	/* system interrupt reg */	
	u_long esr_sirm;             	/* system interrupt mask */
};

struct el_esr5400 {                     /* MIPsfair Error&Status Regs */
        u_long esr_cause;               /* Cause Reg */
        u_long esr_epc;                 /* Exception PC (resume PC) */
        u_long esr_status;              /* Status Reg */
        u_long esr_badva;               /* Bad Virtual Address Reg */
        u_long esr_sp;                  /* Stack Ptr */
	u_long esr_wear;		/* Write Error Address Reg */
	u_long esr_dser;		/* DMA System Error Reg */
	u_long esr_qbear;		/* QBus Error Address Reg */
	u_long esr_dear;		/* DMA Error Address Reg */
	u_long esr_cbtcr;		/* CDAL Bus Timeout Control Reg */
	u_long esr_isr;			/* Interrupt Status Reg	*/
};

struct el_esr5500 {                     /* MIPsfair Error&Status Regs */
        u_long esr_cause;               /* Cause Reg */
        u_long esr_epc;                 /* Exception PC (resume PC) */
        u_long esr_status;              /* Status Reg */
        u_long esr_badva;               /* Bad Virtual Address Reg */
        u_long esr_sp;                  /* Stack Ptr */
	u_long esr_dser;		/* DMA System Error Reg */
	u_long esr_qbear;		/* QBus Error Address Reg */
	u_long esr_dear;		/* DMA Error Address Reg */
	u_long esr_cbtcr;		/* CDAL Bus Timeout Control Reg */
	u_long esr_isr;			/* Interrupt Status Reg	*/
	u_long esr_mser;		/* ECC Memory Error Syndrome Reg */
	u_long esr_mear;		/* ECC Memory Error Address Reg */
	u_long esr_ipcr			/* Inter Process Communication Reg.*/
};

struct el_esr5800 {                     /* ISIS (DS 5800) Error&Status Regs */
        u_long esr_cause;               /* Cause Reg */
        u_long esr_epc;                 /* Exception PC (resume PC) */
        u_long esr_status;              /* Status Reg */
        u_long esr_badva;               /* Bad Virtual Address Reg */
        u_long esr_sp;                  /* Stack Ptr */
	u_long x3p_csr1;		/* X3PA Control and Status 1 Reg. */
	u_long x3p_dtype;		/* XMI Device Type Reg. */
	u_long x3p_xbe;			/* XMI Bus Error Reg. */
	u_long x3p_fadr;		/* XMI Failing Address Reg. */
	u_long x3p_gpr;			/* XMI General Purpose Reg. */
	u_long x3p_csr2;		/* X3PA Control and Status 2 Reg. */
};

struct el_bigen {			/* bi fault/error packet bua/bla */
	int bigen_dev;			/* device type reg */
	int bigen_bicsr;		/* bi csr reg */
	int bigen_ber;			/* bi err reg */
	int bigen_csr;			/* control & status reg */
	int bigen_fubar;		/* failed unibus addr. reg */
	int bigen_pc;
	int bigen_psl;
};

struct el_bier  {				/* bi bus err packet */
	short bier_nument;			/* number of entries */
	struct bi_regs biregs[EL_SIZE16];	/* bi err reg struct */
						/* 16 nodes possible */
	int bier_pc;
	int bier_psl;
};

struct el_uq {				/* uq device attention information */
	u_long	sa_contents;		/* sa register contents		   */
};

struct el_bvp {
	u_long bvp_biic_typ;		/* port biic type reg */
	u_long bvp_biic_csr;		/* port biic csr reg */
	u_long bvp_pcntl;		/* port control reg */
	u_long bvp_pstatus;		/* port status reg */
	u_long bvp_perr;		/* port error reg */
	u_long bvp_pdata;		/* port data reg */
};

struct el_lxerr {
	u_long bi_csr;		/* BIIC error info */
	u_long bi_buserr;	/* valid ONLY if bi_csr bus err bit */
	u_long port_error;	/* BVP error type code */
	u_long port_data;	/* powerup diag reg contents */
	u_long ACP_status;	/* additional ACP status */
	u_long test_num;	/* failing test number in ASCII */
	u_long subtest_num;	/* failing subtest number in ASCII */
	char error_name[8];	/* name of failed test: 8 chars */
	u_long PR_byte1;	/* 1st byte of PR box self test report */
	u_long PR_byte2;	/* 2nd byte of PR box self test report */
	u_long PR_config;	/* config of PR devices from self test */
	u_long PR_rev;		/* firmware revision of PR box */
};
 
struct elci_dattn {			/* ci device attention information */
        struct ci_regs {                /* ci port registers */
		u_long ci_cnfr;			/* configuration register */
		u_long ci_pmcsr;		/* port maint ctrl & status */
		u_long ci_psr;			/* port status */
		u_long ci_pfaddr;		/* port failing address */
		u_long ci_pesr;			/* port error */
		u_long ci_ppr;			/* port parameter */
	} ciregs;
	union ci_icregs {		/* optional interconnect regs */
		struct bi_regs cibiregs;	/* biic device registers */
		struct cixmi_regs {		/* xmi device registers */
			u_long	xdev;			/* device type reg */
			u_long	xbe;			/* bus error reg */
			u_long	xfadrl;			/* failg addr reg low*/
			u_long	xfadrh;			/* failg addr reg hi */
			u_long	pidr;			/* port int dst reg */
			u_long	pvr;			/* port vector reg */
		} cixmiregs;
	} ciicregs;
	union ci_dattnopt {		/* optional device attention info */
		struct ci_ucode {		/* faulty ucode information */
			u_long ci_addr;			/* faulty ucode addr */
			u_long ci_bvalue;		/* bad ucode value */
			u_long ci_gvalue;		/* good ucode value */
		} ciucode;
		struct ci_revlev {		/* port microcode information*/
			u_long ci_romlev;		/* PROM/self-test lev*/
			u_long ci_ramlev;		/* RAM/fn ucode level*/
		} cirevlev;
		struct ci_cpurevlev {		/* out-of-rev CPU ucode */
			u_long ci_hwtype;		/* CPU hardware type */
			u_long ci_mincpurev;		/* min CPU rev lev */
			u_long ci_currevlev;		/* cur CPU rev lev */
		} cicpurevlev;
	} cidattnopt;
};

struct elci_lpkt {			/* ci logged packet information */
        struct ci_lcommon {             /* common logged packet information */
		u_char ci_rsaddr[ 6 ];		/* remote station address */
		u_char ci_rsysid[ 6 ];		/* remote system id number */
		u_char ci_rname[ 8 ];		/* remote system node name */
	} cilcommon;
	union ci_lpktopt {		/* optional logged packet info */
		struct ci_packet {		/* logged packet information */
			u_short size;			/* size of pkt logged */
			u_char ci_port;			/* destination port */
			u_char ci_status;		/* status */
			u_char ci_opcode;		/* cmd operation code*/
			u_char ci_flags;		/* port command flags*/
		} cipacket;
	} cilpktopt;
};


struct el_ci {				/* ci event packet */
        struct ci_common {		/* common to all ci packets */
		u_long	ci_optfmask1;		/* opt err pkt field bit mask*/
		u_long	ci_optfmask2;		/* opt err pkt field bit mask*/
		u_long	ci_evpktver;		/* version of ci event packet*/
		u_char  ci_lpname[ 4 ];		/* local port name */
		u_char  ci_lname[ 8 ];		/* local system name */
		u_char  ci_lsysid[ 6 ];		/* local system id number */
		u_char  ci_lsaddr[ 6 ];		/* local station address */
		u_short ci_nerrs;		/* number of errors */
		u_short ci_nreinits;		/* number of port reinits */
	} cicommon;
	union ci_types {		/* packet/attention specific info */
		struct elci_dattn cidattn;	/* device attention info */
		struct elci_lpkt cilpkt;	/* logged packet info */
	} citypes;
};


struct elmsi_dattn {			/* msi device attention information */
        struct msi_regs {               /* msi port registers */
		u_short msi_csr;		/* control/status */
		u_short msi_idr;		/* id */
		u_short	msi_slcs;		/* selector control/status */
		u_short	msi_destat;		/* selection detector status */
		u_short msi_tr;			/* timeout */
		u_short	msi_dmctlr;		/* dma control */
		u_short	msi_dmlotc;		/* dma length to transfer */
		u_short	msi_dmaaddrl;		/* dma address, low */
		u_short	msi_dmaaddrh;		/* dma address, high */
		u_short	msi_stlp;		/* short target list pointer */
		u_short msi_tlp;		/* target list pointer */
		u_short msi_ilp;		/* initiator list pointer */
		u_short msi_dscr;		/* dssi control */
		u_short msi_dssr;		/* dssi status */
		u_short	msi_dstat;		/* data transfer status */
		u_short msi_dcr;		/* diagnostic control */
		u_short	msi_save_dssr;		/* saved dssi status */
		u_short	msi_save_dstat;		/* saved data transfer status*/
	} msiregs;
};

struct elmsi_lpkt {			/* msi logged packet information */
        struct msi_lcommon {		/* common logged packet information */
		u_char msi_rsaddr[ 6 ];		/* remote station address */
		u_char msi_rsysid[ 6 ];		/* remote system id number */
		u_char msi_rname[ 8 ];		/* remote system node name */
	} msilcommon;
	struct msi_cmdblk {		/* logged packet command block info */
		u_short	msi_thread;		/* addr next command block */
		u_short	msi_status;		/* command block Status */
		u_short	msi_command;		/* command word */
		u_short	msi_opcode;		/* command operation code */
		u_char	msi_dst;		/* destination station addr */
		u_char	msi_src;		/* source port station addr */
		u_short	msi_length;		/* frame length */
	} msilcmdblk;
	struct msi_packet {		/* logged packet information */
		u_char msi_opcode;		/* operation code( pkt type )*/
		u_char msi_flags;		/* operation code modifiers */
	} msipacket;
};

struct el_msi {				/* msi event packet */
        struct msi_common {		/* common to all msi packets */
		u_long	msi_optfmask1;		/* opt err pkt field bit mask*/
		u_long	msi_optfmask2;		/* opt err pkt field bit mask*/
		u_long	msi_evpktver;		/* version - msi event packet*/
		u_char  msi_lpname[ 4 ];	/* local port name */
		u_char  msi_lname[ 8 ];		/* local system name */
		u_char  msi_lsysid[ 6 ];	/* local system id number */
		u_char  msi_lsaddr[ 6 ];	/* local station address */
		u_short msi_nerrs;		/* number of errors */
		u_short msi_nreinits;		/* number of port reinits */
	} msicommon;
	union msi_types {		/* packet/attention specific info */
		struct elmsi_dattn msidattn;	/* device attention info */
		struct elmsi_lpkt msilpkt;	/* logged packet info */
	} msitypes;
};

struct elcippd_system {			/* ci ppd system specific information*/
	struct cippd_scommon {		/* common system specific information*/
		u_char	cippd_rswtype[ 4 ];	/* remote software type */
		u_char	cippd_rswver[ 4 ];	/* remote software version */
		u_char	cippd_rswincrn[ 8 ];	/* remote sw incarnation num */
		u_char	cippd_rhwtype[ 4 ];	/* remote hardware type */
		u_char	cippd_rhwver[ 12 ];	/* remote hardware version */
	} cippdscommon;
	union cippd_systemopt {		/* optional system specific info */
		struct cippd_protocol {		/* ci ppd protocol info */
			u_char	cippd_local;		/* local version */
			u_char	cippd_remote;		/* remote version */
		} cippdprotocol;
		struct cippd_spacket {		/* logged packet */
			u_short cippd_mtype;		/* ci ppd msg type */
		} cippdspacket;
	} cippdsystemopt;
};

struct elcippd_path {			/* ci ppd path specific information */
	struct cippd_pcommon {		/* common path specific information */
		u_char cippd_lpname[ 4 ];	/* local port name */
		u_char cippd_lsaddr[ 6 ];	/* local station address */
		u_char cippd_rsaddr[ 6 ];       /* remote station address */
		u_long cippd_pstate;		/* software path state */
	} cippdpcommon;
	union cippd_pathopt {		/* optional path specific information*/
		struct cippd_dbcoll {		/* database collision info */
			u_char cippd_rswincrn[ 8 ];	/* rem sw incrn num */
			u_char cippd_kswincrn[ 8 ];	/* known sw incrn num*/
			u_char cippd_kname[ 8 ];   	/* known system name */
			u_char cippd_ksysid[ 6 ];  	/* known system id # */
			u_char cippd_klsaddr[ 6 ];	/* known lstat'n addr*/
			u_char cippd_krsaddr[ 6 ]; 	/* known rstat'n addr*/
		} cippddbcoll;
		char	cippd_sysap[ 16 ];	/* name SYSAP crashing path */
		struct cippd_newpath {		/* new path information */
			u_short cippd_max_dg;		/* max appl dg size */
			u_short cippd_max_msg;		/* max appl msg size */
			u_char  cippd_swtype[ 4 ];	/* software type */
			u_char  cippd_swver[ 4 ];	/* software version */
			u_char  cippd_swincrn[ 8 ];	/* software incrn num*/
			u_char  cippd_hwtype[ 4 ];	/* hardware type */
			u_char  cippd_hwver[ 12 ];	/* hardware version */
		} cippdnewpath;
		struct cippd_ppacket {		/* logged packet */
			u_short cippd_mtype;		/* ci ppd msg type */
		} cippdppacket;
	} cippdpathopt;
};

struct el_cippd {			/* ci ppd event packet */
	struct cippd_common {		/* common to all packets */
		u_long cippd_optfmask1;		/* opt err pkt field bit mask*/
		u_long cippd_optfmask2;		/* opt err pkt field bit mask*/
		u_long cippd_evpktver;		/* version of cippd event pkt*/
		u_char cippd_lname[ 8 ];	/* local system node name */
		u_char cippd_rname[ 8 ];	/* remote system node name */
		u_char cippd_lsysid[ 6 ];	/* local system id number */
		u_char cippd_rsysid[ 6 ];	/* remote system id number */
		u_short cippd_npaths;		/* num total paths to rem sys*/
		u_short cippd_nerrs;		/* number of errors */
	} cippdcommon;
	union cippd_types {		/* system/path specific information */
		struct elcippd_system cippdsystem; /* system specific info */
		struct elcippd_path cippdpath;	/* path specific info */
	} cippdtypes;
};

struct el_scs {				/* scs event packet */
	struct scs_common {		/* common to all packets */
		u_long  scs_optfmask1;		/* opt err pkt field bit mask*/
		u_long  scs_optfmask2;		/* opt err pkt field bit mask*/
		u_long  scs_evpktver;		/* version of scs event pkt*/
		u_char  scs_lsysap[ 16 ];	/* local sysap name */
		u_char  scs_lconndata[ 16 ];	/* local connection data */
		u_long  scs_lconnid;		/* local connection id number*/
		u_char  scs_lname[ 8 ];		/* local system node name */
		u_char  scs_lsysid[ 6 ];	/* local system id number */
		u_short	scs_cstate;		/* connection state */
	} scscommon;
	union scs_opt {			/* optional scs event information */
		struct scs_conn {		/* scs connection information*/
			u_char  scs_rsysap[ 16 ];	/* remote sysap name */
			u_char  scs_rconndata[ 16 ];	/* remote conn data */
			u_long	scs_rconnid;		/* remote conn id num*/
			u_char  scs_rname[ 8 ];		/* rem sys node name */
			u_char  scs_rsysid[ 6 ];	/* rem system id num */
			u_char  scs_rsaddr[ 6 ];	/* rem station addr */
			u_char  scs_lpname[ 4 ];	/* local port name */
			u_char  scs_lsaddr[ 6 ];	/* local station addr*/
			u_short scs_nconns;		/* num conns on path */
		} scsconn;
		u_short	scs_ldirid;		/* local directory id number */
		u_long	scs_rreason;		/* connection reject reason */
	} scsopt;
};

struct el_nmiflt {
	int nmiflt_nmifsr;		/*  primary cpu */
	int nmiflt_nmiear;		/*  primary cpu */
	int nmiflt_memcsr0;		/*  memory fault bits */
	int nmiflt_nbia0;		/*  nbia0 fault bits */
	int nmiflt_nbia1;		/*  nbia1 fault bits */
	int nmiflt_nmisilo[EL_SIZE256];	/*  silo data */
};

struct el_nbwadp {
	long star_csr0;			
	long star_csr1;			
	long nemo_csr0;		    	
	long nemo_csr1;
	long nemo_csr6;
};

struct el_nmiadp {
	int nmiadp_nbiacsr0;		/* nmi bi adp csr0   */
	int nmiadp_nbiacsr1;		/* nmi bi adp csr1   */
	int nmiadp_nbib0err;		/* bi bus 0 err reg  */
	int nmiadp_nbib1err;		/* bi bus 1 err reg  */
};

/*
 * Rigel VAX6400 hard error interrupt (vector 0x60) error packet and
 * soft error interrupt (vector 0x54) error packet
 */
struct  el_xrp {
        long    s_rcsr;                 /* REXMI Control & Status Register*/
        long    s_xber;                 /* REXMI Bus Error Register */
        long    s_xfadr;                /* REXMI Failing Address Register*/
        long    s_sscbtr;               /* RSSC Bus Timeout Register    */
        long    s_bcctl;                /* C-chip Control Register      */
        long    s_bcsts;                /* C-chip Status Register       */
        long    s_bcerr;                /* C-chip Error Address Register*/
        long    s_pcsts;                /* P-cache Status Register      */
        long    s_pcerr;                /* P-cache Error Address Register*/
        long    s_vintsr;               /* C-chip Vect Interface Err Status*/
};

/*
 * Mariah VAX6500 hard error interrupt (vector 0x60) error packet and
 * soft error interrupt (vector 0x54) error packet
 */
struct  el_xmp {
        long    s_xbe0;                 /* MAXMI Bus Error Register        */
	long    s_xfadr0;               /* MAXMI Failing Address Register 0*/
	long    s_xfaer0;               /* MAXMI Failing Addr Ext. Reg. 0  */
        long    s_xbeer0;               /* MAXMI Bus Error Ext. Register   */
        long    s_wfadr0;               /* MAXMI Failing Addr Reg for Wbck0*/
        long    s_wfadr1;               /* MAXMI Failing Addr Reg for Wbck1*/
        long    s_fdal0;                /* MAXMI Failing DAL Register 0    */
        long    s_fdal1;                /* MAXMI Failing DAL Register 1    */
        long    s_fdal2;                /* MAXMI Failing DAL Register 2    */
        long    s_fdal3;                /* MAXMI Failing DAL Register 3    */
        long    s_sscbtr;               /* MSSC Bus Timeout Register       */
        long    s_bcsts;                /* C-chip Status Register          */
        long    s_bcera;                /* C-chip Error Address Register   */
	long    s_bcert;                /* C-chip Error Tag Register       */
        long    s_pcsts;                /* P-cache Status Register         */
        long    s_pcerr;                /* P-cache Error Address Register  */
        long    s_vintsr;               /* C-chip Vect Interface Err Status*/
};

/* Calypso XCP interrupt vector 54 error packet */
struct el_xcp60 {
	long	xcp_csr1;
	long	xcp_cadr;
	long	xcp_mser;
	long	xcp_dtype;
	long	xcp_xbe;
	long	xcp_fadr;
	long	xcp_gpr;
	long	xcp_csr2;
};
struct el_xcpsoft {
	long	xcp_iqo;
	long	xcp_dtpe;
	long	xcp_cfe;
	long	xcp_cc;
	long	xcp_ipe;
	long	xcp_pe;
	long	xcp_vbpe;
	long	xcp_tpe;
};
/* Calypso XCP interrupt vector 54 error packet */
struct el_xcp54 {
	long	sw_flags;
	long	xcp_csr1;
	long	xcp_cadr;
	long	xcp_mser;
	long	xcp_dtype;
	long	xcp_xbe;
	long	xcp_fadr;
	long	xcp_gpr;
	long	xcp_csr2;
	struct el_xcpsoft xcp_soft;
};


/* Calypso XMA error packet */
struct el_xma {
	long	xma_node;		/* xma's xmi node number */
	long	xma_dtype;		/* device type register */
	long	xma_xbe;		/* bus error register */
	long	xma_seadr;		/* start/end addr reg */
	long 	xma_mctl1;		/* control reg 1 */
	long	xma_mecer;		/* ecc error reg */
	long	xma_mecea;		/* error addr reg */
	long	xma_mctl2;		/* control reg 2 */
};

/* XMA2 error packet */
struct el_xma2 {
	long	xma_node;		/* xma's xmi node number */
	long	xma_dtype;		/* device type register */
	long	xma_xbe;		/* bus error register */
	long	xma_seadr;		/* start/end addr reg */
	long 	xma_mctl1;		/* control reg 1 */
	long	xma_mecer;		/* ecc error reg */
	long	xma_mecea;		/* error addr reg */
	long	xma_mctl2;		/* control reg 2 */
	long    xma_becer;             /* Block ECC Error reg */
	long    xma_becea;             /* Block ECC Address reg */
	long    xma_stadr;             /* Starting Address reg */
	long    xma_enadr;             /* Ending Address reg */
	long    xma_intlv;             /* Segment/Interleave reg */
	long    xma_mctl3;             /* Memory Control Reg. 3 */
	long    xma_mctl4;             /* Memory Control Reg. 4 */
	long    xma_bsctl;             /* Block State Access Control */
	long    xma_bsadr;             /* Block State Access Address */
	long    xma_eectl;             /* EEPROM Access Control/Address */
	long    xma_tmoer;             /* Time-out CSR */
};

/* XNA error packet (DEBNI, DEMNA) */
struct el_xna {
	union	xna_type {
		union	xna_xmi {
			struct xna_xmi_fatal {
				long	xna_type;
				long	xna_date_lo;
				long	xna_date_hi;
				long	xna_r0;
				long	xna_r1;
				long	xna_r2;
				long	xna_r3;
				long	xna_r4;
				long	xna_r5;
				long	xna_r6;
				long	xna_r7;
				long	xna_r8;
				long	xna_r9;
				long	xna_r10;
				long	xna_r11;
				long	xna_r12;
				long	xna_xbe;
				long	xna_xfadr;
				long	xna_xfaer;
				long	xna_gacsr;
				long	xna_diag;
				long	xna_xpst_init;
				long	xna_xpd1_init;
				long	xna_xpd2_init;
				long	xna_xpst_final;
				long	xna_xpd1_final;
				long	xnastack[6];
			} xnaxmi_fatal;
			struct xna_xmi_nonfatal {
				long	xna_date_lo;
				long	xna_date_hi;
				long	xna_r0;
				long	xna_r1;
				long	xna_r2;
				long	xna_r3;
				long	xna_xbe;
				long	xna_xfadr;
				long	xna_xfaer;
				long	xna_gacsr;
			} xnaxmi_nonfatal;
		} xnaxmi;
		union	xna_bi {
			struct	xna_bi_fatal {
				long	xna_type;
				long	xna_date_lo;
				long	xna_date_hi;
				long	xna_r0;
				long	xna_r1;
				long	xna_r2;
				long	xna_r3;
				long	xna_r4;
				long	xna_r5;
				long	xna_r6;
				long	xna_r7;
				long	xna_r8;
				long	xna_r9;
				long	xna_r10;
				long	xna_r11;
				long	xna_r12;
				long	xna_ber;
				long	xna_pad1;
				long	xna_pad2;
				long	xna_bicsr;
				long	xna_bci3_csr;
				long	xna_xpst_init;
				long	xna_xpd1_init;
				long	xna_xpd2_init;
				long	xna_xpst_final;
				long	xna_xpd1_final;
				long	xnastack[6];
			} xnabi_fatal;
			struct	xna_bi_nonfatal {
				long	xna_date_lo;
				long	xna_date_hi;
				long	xna_r0;
				long	xna_r1;
				long	xna_r2;
				long	xna_r3;
				long	xna_ber;
			} xnabi_nonfatal;

		} xnabi;
	} xnatype;
};

/* XMI to BI adapter error packet */
struct el_xbi {
	long 	xbi_node;
	long 	xbi_dtype;
	long	xbi_fadr;
	long	xbi_arear;
	long	xbi_aesr;
	long	xbi_aimr;
	long	xbi_aivintr;
	long	xbi_adg1;	
	long	xbi_bcsr;
	long	xbi_besr;
	long	xbi_bidr;
	long	xbi_btim;
	long	xbi_bvor;
	long	xbi_bvr;
	long	xbi_bdcr1;
};

/* XBI+ adapter error packet */
struct el_xbiplus {
	long 	xbi_node;
	long 	xbi_dtype;
	long	xbi_fadr;
	long	xbi_arear;
	long	xbi_aesr;
	long	xbi_aimr;
	long	xbi_aivintr;
	long	xbi_adg1;	
	long	xbi_bcsr;
	long	xbi_besr;
	long	xbi_bidr;
	long	xbi_btim;
	long	xbi_bvor;
	long	xbi_bvr;
	long	xbi_bdcr1;
	long    xbi_autlr;
	long    xbi_acsr;
	long    xbi_arvr;
	long    xbi_abear;
	long	xbi_xbe;
	long    xbi_xfaer;
};

struct el_mbus {
	char	elmb_count;	/* number of module logs in this entry	     */
	char	elmb_dominant;	/* Index of dominant error (increasing from
				   0 to 11 ) ARB, MCPE, MSPE, MDPE, ICMD,
				   MTO, ILCK, MTPE, SERR, IDAT, NOS, FRZN    */
	u_short	elmb_flags;	/* Flags indicating machine check (MB_MCHK) or 
				   backplane at fault (MB_BP_F)		     */
	long	elmb_size;	/* Overall size in bytes of the entire entry */
	u_long	elmb_mod_err;	/* module error mask			     */
	u_long	elmb_module_log[14][12];
};

struct el_fbic {
	char	fbic_size;		/* is 0x2c for this structure	     */
	char	fbic_cpuid;		/* Firefox CPUID.  Low 2 bits are 00
					   for all but the CPU board.  On CPU
					   board, they are either 00 or 11 for
					   CPU A or B respectively	     */
	short	fbic_valid;		/* 1 = Entry is valid
					   2 = Include in the error analysis */
	u_long	fbic_modtype;		/* Firefox Modtype - byte 3 is always
					   01 for this module type	     */
	u_long	fbic_buscsr;
	u_long	fbic_busctl;
	u_long	fbic_busaddr;
	u_long	fbic_busdat;
	u_long	fbic_fbicsr;
	u_long	fbic_range;
	u_long	fbic_ipdvint;
	u_long	fbic_iadr1;
	u_long	fbic_iadr2;
};

struct el_fmdc {
	char	fmdc_size;		/* size is 0x30 for this structure   */
	char	fmdc_cpuid;		/* Firefox CPUID.  Low 2 bits are 00
					   for all but the CPU board.  On CPU
					   board, they are either 00 or 11 for
					   CPU A or B respectively	     */
	short	fmdc_valid;		/* 1 = Entry is valid
					   2 = Include in the error analysis */
	u_long	fmdc_modtype;		/* Firefox Modtype - byte 3 is always
					   02 for this module type	     */
	u_long	fmdc_buscsr;
	u_long	fmdc_busctl;
	u_long	fmdc_busaddr;
	u_long	fmdc_busdat;
	u_long	fmdc_fmdcsr;
	u_long	fmdc_baseaddr;
	u_long	fmdc_eccaddr0;
	u_long	fmdc_eccaddr1;
	u_long	fmdc_eccsynd0;
	u_long	fmdc_eccsynd1;
};

struct el_fmcm {
	char	fmcm_size;		/* size is 0x14 for this structure   */
	char	fmcm_cpuid;		/* Firefox CPUID.  Low 2 bits are 00
					   for all but the CPU board.  On CPU
					   board, they are either 00 or 11 for
					   CPU A or B respectively.  The next
					   three bits are the slot number.   */
	short	fmcm_valid;		/* 1 = Entry is valid
					   2 = Include in the error analysis */
	u_long	fmcm_modtype;		/* Firefox Modtype - byte 3 is always
					   FE for this module type	     */
	u_long	fmcm_buscsr;
	u_long	fmcm_busctl;
	u_long	fmcm_baseaddr;
};


struct el_strayintr {			/* stray intr. packet */
	u_char stray_ipl;		/* ipl level */
	short stray_vec;		/* vector */
};

struct el_exptflt {			/* panic exception/fault packet */
	int exptflt_va;			/* va. if appropriate else zero */
	int exptflt_pc;			/* pc at time of exception/fault */
	int exptflt_psl;		/* psl at time of exception/fault */
};

struct el_stkdmp {
        int addr;
	int size;
	int stack[EL_SIZE128];		/* stack dump of kernel/interpt/user */
};

struct el_pnc {				/* panic packet (bug check) */
	char pnc_asc[EL_SIZE64];	/* ascii panic string */
	int pnc_sp;
	int pnc_ap;
	int pnc_fp;
	int pnc_pc;
	struct pncregs {
	    int pnc_ksp;
	    int pnc_usp;
	    int pnc_isp;
	    int pnc_p0br;
	    int pnc_p0lr;
	    int pnc_p1br;
	    int pnc_p1lr;
	    int pnc_sbr;
	    int pnc_slr;
	    int pnc_pcbb;
	    int pnc_scbb;
	    int pnc_ipl;
	    int pnc_astlvl;
	    int pnc_sisr;
	    int pnc_iccs;
	} pncregs;
	struct el_stkdmp kernstk;		/* dump of kernel stack */
	struct el_stkdmp intstk;		/* dump of interrupt stack */
};

struct el_msg {				/* msg packet */
	short msg_len;			/* length */
	char msg_asc[EL_SIZE256*8];	/* ascii string */
};

struct el_timchg {
	struct timeval timchg_time;	/* time in sec and usec */
	struct timezone timchg_tz;	/* time zone data */
	char timchg_version[EL_SIZE16];	/* ultrix vx.x */
};


/*
 *  VMEbus adapter registers error packet for 3max and Mipsfair2 
 *
 *                     The MVIB is the VME card containing
 *		       the registers for the 3VIA and MVIA,
 *                     which are the host-side cards for the
 *                     VME option on 3max and mipsfair2.
 */

struct el_vba_MVIB {
        long     mvib_viacsr;   /* xVIA control/status register */
        long     mvib_csr;      /* control/status register */
	long     mvib_vfadr;    /* vme failing address register */
	long     mvib_cfadr;    /* cpu failing address register */
	long     mvib_ivs;      /* interrupt vector source */
 	long 	 mvib_besr;	/* VIC bus err summary register */
	long	 mvib_errgi;    /* VIC err group inter. control reg */
	long     mvib_lvb;	/* VIC local vector base reg */
	long     mvib_err;   	/* VIC error vector register */
};

/*
 * XMI-based VMEbus adapter registers error packet (CMAX).
 *
 *           Registers found on both boards, the XBIA on
 *           the host-side, and the XVIB on the VME-side.
 */

struct el_vba_XBIA {	        /* 2 board set -- xvib and xbia */

        long     xvib_vdcr;     /* device/configuration register */
        long     xvib_vesr;     /* error summary register */
	long     xvib_vfadr;    /* vme failing address register */
        long     xvib_vicr;     /* interrupt configuration register */
	long     xvib_vvor;     /* vector offset register */
	long     xvib_vevr;     /* error vector register */

	long	 xbia_dtype;    /* device type/revision register */
	long     xbia_xbe;      /* XMI bus error register */
 	long     xbia_fadr;	/* XMI failing address register */
	long	 xbia_arear;	/* xbia responder error address register */
	long	 xbia_aesr;	/* xbia error summary register */
	long 	 xbia_aimr;     /* xbia interrupt mask register */
	long 	 xbia_aivintr;  /* xbia implied vector inter. dest. register */
        long     xbia_adg1;     /* xbia diag 1 register */

};


/* VMEbus adapter error packet */
struct el_vba {
        union {                                  /* depends on adapter type */
		struct el_vba_MVIB elmvib;       /* 3max, mipsfair-2 */
		struct el_vba_XBIA elxbia;       /* cmax (xmi) */
	} elvba_reg;
};

/* VMEbus general purpose error packet for VME devices and controllers */
struct el_vme_dev_cntl {
        char module[EL_SIZE64];              /* name from uba structure */
        short num;			     /* num from uba structure */
	caddr_t csr1;                        /* csr1 address from uba struct*/
	caddr_t csr2;                        /* csr2 address from uba struct*/
        struct el_msg el_vme_error_msg;      /* error message from driver */
        struct el_vba elvba;                 /* adapter regs for more info */
};

/* XJA adapter (VAX9000 to XMI bus to J-box Adapter) */
struct el_xja {
	u_long el_xja_xdev;		/* device register */
	u_long el_xja_xber;		/* bus error register */
	u_long el_xja_xfadra;		/* failing address register lw0 */
	u_long el_xja_xfadrb;		/* failing address register lw1 */
	u_long el_xja_aosts;		/* xja aost status register */
	u_long el_xja_sernum;		/* xja serial number register */
	u_long el_xja_errs;		/* xja error summary register */
	u_long el_xja_fcmd;		/* xja force command register */
	u_long el_xja_ipintrsrc;	/* xja ipintr source register */
	u_long el_xja_diag;		/* xja diagnostic control */
	u_long el_xja_dmafaddr;		/* xja dma failing address reg */
	u_long el_xja_dmafcmd;		/* xja dma failing command reg */
	u_long el_xja_errintr;		/* xja error interrupt control */
	u_long el_xja_cnf;		/* xja configuration register */
	u_long el_xja_xbiida;		/* xja xbi id a register */
	u_long el_xja_xbiidb;		/* xja xbi id b register */
	u_long el_xja_errscb;		/* xja error scb offset */
};

struct el_mc8600frame {
	int	mc8600_bytcnt;		/* machine check stack frame byte cnt */
	int	mc8600_ehm_sts; 	/* ehm.sts */
	int	mc8600_evmqsav; 	/* ebox vmq sav */
	int	mc8600_ebcs;		/* ebox control status register */
	int	mc8600_edpsr;		/* ebox data path status register */
	int	mc8600_cslint;		/* ebox console/interrupt register */
	int	mc8600_ibesr;		/* ibox error and status register */
	int	mc8600_ebxwd1;		/* ebox write data 1 */
	int	mc8600_ebxwd2;		/* ebox write data 1 */
	int	mc8600_ivasav;		/* ibox va sav */
	int	mc8600_vibasav; 	/* ibox viba */
	int	mc8600_esasav;		/* ibox esa */
	int	mc8600_isasav;		/* ibox isa */
	int	mc8600_cpc;		/* ibox cpc */
	int	mc8600_mstat1;		/* mbox status reg#1 */
	int	mc8600_mstat2;		/* mbox status reg#2 */
	int	mc8600_mdecc;		/* mbox data ecc register */
	int	mc8600_merg;		/* mbox error generator register */
	int	mc8600_cshctl;		/* mbox cache control register */
	int	mc8600_mear;		/* mbox error address register */
	int	mc8600_medr;		/* mbox error data register */
	int	mc8600_accs;		/* accelerator status register */
	int	mc8600_cses;		/* control store error status reg */
	int	mc8600_pc;		/* pc */
	int	mc8600_psl;		/* psl */
};

struct el_mc8800frame {
	int	mc8800_bcnt;			/* byte count */
	int	mc8800_mcsts;			/* cpu error status */
	int	mc8800_ipc;			/* istream pc */
	int	mc8800_vaviba;			/* va/viba register */
	int	mc8800_iber;			/* i reg */
	int	mc8800_cber;			/* c reg */
	int	mc8800_eber;			/* e reg */
	int	mc8800_nmifsr;			/* nmi  */
	int	mc8800_nmiear;			/* nmi  */
	int	mc8800_pc; 			/* macro pc */
	int	mc8800_psl;			/* psl */
};

struct el_mc8200frame {
	int	mc8200_bcnt;
	int	mc8200_summary;
	int	mc8200_parm1;
	int	mc8200_va;
	int	mc8200_vap;
	int	mc8200_mar;
	int	mc8200_stat;
	int	mc8200_pcfail;
	int	mc8200_upcfail;
	int	mc8200_pc;
	int	mc8200_psl;
};

struct el_mc780frame {
	int	mc8_bcnt;			/* byte count == 0x28 */
	int	mc8_summary;			/* summary parameter */
	int	mc8_cpues;			/* cpu error status */
	int	mc8_upc;			/* micro pc */
	int	mc8_vaviba;			/* va/viba register */
	int	mc8_dreg;			/* d register */
	int	mc8_tber0;			/* tbuf error reg 0 */
	int	mc8_tber1;			/* tbuf error reg 1 */
	int	mc8_timo;			/* timeout address */
	int	mc8_parity;			/* parity */
	int	mc8_sbier;			/* sbi error register */
	int	mc8_pc; 			/* trapped pc */
	int	mc8_psl;			/* trapped psl */
};

struct el_mc750frame {
	int	mc5_bcnt;			/* byte count == 0x28 */
	int	mc5_summary;			/* summary parameter */
	int	mc5_va; 			/* virtual address register */
	int	mc5_errpc;			/* error pc */
	int	mc5_mdr;
	int	mc5_svmode;			/* saved mode register */
	int	mc5_rdtimo;			/* read lock timeout */
	int	mc5_tbgpar;			/* tb group parity error reg */
	int	mc5_cacherr;			/* cache error register */
	int	mc5_buserr;			/* bus error register */
	int	mc5_mcesr;			/* machine check status reg */
	int	mc5_pc; 			/* trapped pc */
	int	mc5_psl;			/* trapped psl */
};

struct el_mc730frame {
	int	mc3_bcnt;			/* byte count == 0xc */
	int	mc3_summary;			/* summary parameter */
	int	mc3_parm[2];			/* parameter 1 and 2 */
	int	mc3_pc; 			/* trapped pc */
	int	mc3_psl;			/* trapped psl */
};

struct el_mcUVIframe {
	int	mc1_bcnt;			/* byte count == 0xc */
	int	mc1_summary;			/* summary parameter */
	int	mc1_parm[2];			/* parameter 1 and 2 */
	int	mc1_pc; 			/* trapped pc */
	int	mc1_psl;			/* trapped psl */
};

struct el_mcUVIIframe {
	int	mc1_bcnt;			/* byte count == 0xc */
	int	mc1_summary;			/* summary parameter */
	int	mc1_vap;			/* most recent virtual addr */
	int	mc1_internal_state;		/* internal state ? */
	int	mc1_pc; 			/* trapped pc */
	int	mc1_psl;			/* trapped psl */
};

struct el_mcCVAXframe {
	int	mc1_bcnt;			/* byte count == 0x10 */
	int	mc1_summary;			/* summary parameter */
	int	mc1_vap;			/* most recent virtual addr */
	int	mc1_internal_state1;		/* internal state 1 */
	int	mc1_internal_state2;		/* internal state 2 */
	int	mc1_pc; 			/* trapped pc */
	int	mc1_psl;			/* trapped psl */
};

struct el_mc6200frame {
	int	mc1_bcnt;			/* byte count == 0x10 */
	int	mc1_summary;			/* summary parameter */
	int	mc1_vap;			/* most recent virtual addr */
	int	mc1_internal_state1;		/* internal state 1 */
	int	mc1_internal_state2;		/* internal state 2 */
	int	mc1_pc; 			/* trapped pOSOSOSc */
	int	mc1_psl;			/* trapped psl */
	int	xcp_dtype;			/* device type */
	int	xcp_xbe;			/* xmi error register */
	int 	xcp_csr2;			/* csr2 */
	int	xcp_csr1;			/* csr1 */
	int	xcp_mser;			/* memory system error reg */
};

struct el_mc6400frame {
        long    bcnt;                   /* byte count == 0x18              */
        long    mcode;                  /* R bit and machine check code    */
        long    vaddr;                  /* virtual addr processed by M-box */
        long    viba;                   /* virtual addr of M-box prefetch  */
        long    iccs_sisr;              /* ICCS and SISR                   */
        long    istate;                 /* Internal state                  */
        long    sc;                     /* SC (internal ucode register)    */
        long    pc;                     /* program counter                 */
        long    psl;                    /* PSL                             */
        /* The following are a snapshot of various hardware registers */
        long    s_rcsr;                 /* REXMI Control & Status Register*/
        long    s_xber;                 /* REXMI Bus Error Register     */
        long    s_xfadr;                /* REXMI Failing Address Register*/
        long    s_sscbtr;               /* RSSC Bus Timeout Register    */
        long    s_bcctl;                /* C-chip Control Register      */
        long    s_bcsts;                /* C-chip Status Register       */
        long    s_bcerr;                /* C-chip Error Address Register*/
        long    s_pcsts;                /* P-cache Status Register      */
        long    s_pcerr;                /* P-cache Error Address Register*/
        long    s_vintsr;               /* C-chip Vect Interface Err Status*/
};

struct el_mc6500frame {
        long    bcnt;                   /* byte count == 0x18              */
        long    mcode;                  /* R bit and machine check code    */
        long    vaddr;                  /* virtual addr processed by M-box */
        long    viba;                   /* virtual addr of M-box prefetch  */
        long    iccs_sisr;              /* ICCS and SISR                   */
        long    istate;                 /* Internal state                  */
        long    sc;                     /* SC (internal ucode register)    */
        long    pc;                     /* program counter                 */
        long    psl;                    /* PSL                             */
        /* The following are a snapshot of various hardware registers */
        long    s_xbe0;                 /* MAXMI Bus Error Register        */
	long    s_xfadr0;               /* MAXMI Failing Address Register 0*/
	long    s_xfaer0;               /* MAXMI Failing Addr Ext. Reg. 0  */
        long    s_xbeer0;               /* MAXMI Bus Error Ext. Register   */
        long    s_wfadr0;               /* MAXMI Failing Addr Reg for Wbck0*/
        long    s_wfadr1;               /* MAXMI Failing Addr Reg for Wbck1*/
        long    s_fdal0;                /* MAXMI Failing DAL Register 0    */
        long    s_fdal1;                /* MAXMI Failing DAL Register 1    */
        long    s_fdal2;                /* MAXMI Failing DAL Register 2    */
        long    s_fdal3;                /* MAXMI Failing DAL Register 3    */
        long    s_sscbtr;               /* MSSC Bus Timeout Register       */
        long    s_bcsts;                /* C-chip Status Register          */
        long    s_bcera;                /* C-chip Error Address Register   */
	long    s_bcert;                /* C-chip Error Tag Register       */
        long    s_pcsts;                /* P-cache Status Register         */
        long    s_pcerr;                /* P-cache Error Address Register  */
        long    s_vintsr;               /* C-chip Vect Interface Err Status*/
};

struct el_mck {					/* machine check packet */
	union {
		struct el_mc8800frame el8800mcf;
		struct el_mc8600frame el8600mcf;	/* mck frame that is */
		struct el_mc8200frame el8200mcf;	/* pushed on intr.   */
		struct el_mc780frame el780mcf;		/* stack by micro    */
		struct el_mc750frame el750mcf;		/* code              */
		struct el_mc730frame el730mcf;
		struct el_mcUVIIframe elUVIImcf;
		struct el_mcUVIframe elUVImcf;
		struct el_mcCVAXframe elCVAXmcf;
		struct el_mc6200frame el6200mcf;
		struct el_mc6400frame el6400mcf;
		struct el_mc6500frame el6500mcf;
		struct el_mc9000frame el9000mcf;
		struct el_mc9000eboxframe el9000eboxmcf;
		struct el_mc9000spuframe el9000spumcf;
	} elmck_frame;
};

struct el_esr {				/* Generic error & status regs	*/
	union {
		struct el_esrkn02 el_esrkn02;	/* 3MAX (DS5000)	*/
		struct el_esrkn02ba el_esrkn02ba;	/* 3MIN (DS5000_100)	*/
		struct el_esr5400 el_esr5400;	/* Mipsfair (DS5400)	*/
		struct el_esr5500 el_esr5500;	/* Mipsfair (DS5500)	*/
		struct el_esr5800 el_esr5800;	/* ISIS (DS5800)	*/
		struct el_esr5100 el_esr5100;   /* MIPSmate (DS5100)    */
	} elesr;
};

struct el_vec6400 {			/*  vector process for 6400 */
	u_int	vec6400_vintsr;
	u_int	vec6400_vpsr;
	u_int	vec6400_vctl_csr;
	u_int	vec6400_lsx_ccsr;
};

struct el_fza {
	u_short	fza_id;			/* fza id or version */
	u_short reset_count;		/* reset counter */
	u_long	timestamp_hi;		/* time stamp hi */
	u_short timestamp_lo;		/* time stamp lo */
	u_short write_count;			
	u_short int_reason;		/* Internal failure reason */
	u_short ext_reason;		/* External failure reason */
	u_long  cmd_next_ptr;		/* Next cmd entry to service */
	u_long  cmd_next_cmd;		/* Next cmd descr, 1st entry */
	u_long  dma_next_rmc_ptr;
	u_long  dma_next_rmc_descr;
	u_long  dma_next_rmc_own;
	u_long  dma_next_host_ptr;
	u_long  dma_next_host_descr;
	u_long  lmgr_next_ptr;
	u_long  lmgr_next_descr;
	u_long  smt_next_put_ptr;
	u_long  smt_next_put_descr;
	u_long  smt_next_take_ptr;
	u_long  smt_next_take_descr;
	u_short pm_csr;			/* Packet mem CSR */
	u_short int_68k_present;	/* 68k interrupt ctrl reg */
	u_short int_68k_mask;		/* 68k interrupt ctrl mask reg */
	u_short pint_event;		/* Port interrupt reg */
	u_short port_ctrl_a;		/* Port control A */
	u_short port_ctrl_a_mask;	/* Port control A mask */
	u_short port_ctrl_b;		/* Port control B */
	u_short port_status;		/* Port status */
	u_short ram_rom_map;		/* Map register */
	u_short phy_csr;		/* Phy CSR */
	u_short dma_done;		/* DMA done */
	u_short dma_err;		/* DMA error */
	u_short dma_start_lo;		/* DMA start dma low addr */
	u_short dma_start_hi;		/* DMA start high addr */
	u_short rmc_cmd;		/* RMC command */
	u_short rmc_mode;		/* RMC mode */
	u_short rmc_rcv_page;		/* RMC rcv page */
	u_short rmc_rcv_params;		/* RMC rcv parameters */
	u_short rmc_xmt_page;		/* RMC xmt page */
	u_short rmc_xmt_params;		/* RMC xmt parameters */
	u_short rmc_interrupts;		/* RMC interrupts */
	u_short rmc_int_mask;		/* RMC interrupt mask */
	u_short rmc_chan_status;	/* RMC channel status */
	u_short mac_rcv_cntrl;		/* MAC */
	u_short mac_xmt_cntrl;
	u_short mac_int_mask_a;
	u_short mac_int_mask_b;
	u_short mac_rcv_status;
	u_short mac_xmt_status;
	u_short mac_mla_a;
	u_short mac_mla_b;
	u_short mac_mla_c;
	u_short mac_t_req;
	u_short mac_tvx_value;
	u_long  crc;			/* TEMPORARY, REMOVE ON NEW ERR LOG */
 } ;

struct el_rec {					/* errlog record packet */ 
	struct el_rhdr elrhdr;			/* record header */
	struct el_sub_id elsubid;		/* subsystem id packet */
	union {
		struct el_bdev elbdev;                  /* device errors     */
		struct el_bier elbier;                  /* bi errors         */
		struct el_bigen elbigen;                /* bi adp/cntl errors*/
		struct el_bvp elbvp;                    /* bvp port errors   */
		struct el_ci elci;			/* ci events         */
		struct el_cippd elcippd;                /* ci ppd events     */
		struct el_esr elesr;			/* gernic err/stat  */
		struct el_esr420 elesr420;		/* 420 Err/Stat Regs */
		struct el_esr650 elesr650;		/* 650 Err/Stat Regs */
		struct el_esrpmax elesrpmax;            /* 650 Err/Stat Regs */
		struct el_exptflt elexptflt;            /* panic exception & */
                                                        /* fault             */
		struct el_lxerr ellxerr;                /* shadowfax errors  */
		struct el_mbus elmbus;			/* mbus error packets*/
		struct el_mck elmck;                    /* machine check     */
		struct el_mem elmem;                    /* memory crd errors */
		struct el_msg elmsg;                    /* ascii text msg    */
		struct el_msi elmsi;			/* msi events	     */
		struct el_nbwadp el_nbwadp;             /* 8820 bus window   */
		struct el_nmiadp elnmiadp;              /* 8800 nmi adapter  */
		struct el_nmiflt elnmiflt;              /* 8800 nmi faults   */
		struct el_pnc elpnc;                    /* panic frame       */
		struct el_sbi_aw780 elsbiaw780;         /* sbi faults, async */
                                                        /* writes            */
		struct el_sbia8600 elsbia8600;          /* sbi alerts 8600   */
		struct el_scs elscs;               	/* scs events	     */
		struct el_stkdmp elstkdmp;              /* stack dump        */
		struct el_strayintr elstrayintr;        /* stray interrupts  */
		struct el_timchg eltimchg;		/* time change  &    */
							/* startup/shutdown  */
		struct el_uba780 eluba780;              /* uba errors        */
		struct el_uq eluq;                      /* uq port errors    */
		struct el_xbi el_xbi;                   /* xbi error packet */
		struct el_xbiplus el_xbiplus;		/* xbiplus error packet */
		struct el_xcp54 el_xcp54;               /* xcp level 54 err */
		struct el_xcp60 el_xcp60;               /* xcp level 60 err */
		struct el_xma   el_xma;                 /* xma error packet */
		struct el_xma2  el_xma2;		/* xma2 error packet */
		struct el_xna elxna;			/* xna port errors   */
		struct el_xrp	el_xrp;			/* xrp hard/ soft err*/
		struct el_vba   el_vba;                 /* VME adapter errs */
		struct el_vme_dev_cntl elvme_devcntl;   /* VME dev/crtl errs */
		struct el_xmp	el_xmp;			/* xmp hard/ soft err */
		struct el_aq_cpusyn el_aqcpusyn;	/* aqua cpu syndrome */
		struct el_aq_iosyn el_aqiosyn;	/* aqua io syndrome */
		struct el_aq_hese el_aqhese;	/* aqua hard/soft mem err */
		struct el_aq_spuhe el_spuhe;		/* spu hard mem err */
		struct el_aq_spuse el_spuse;		/* spu soft mem err */
		struct el_aq_pcs el_aqpcs;	/* aqua pcs exceptions */
		struct el_aq_pcsstat el_aqpcsstat;	/* aqua pcs status */
		struct el_aq_scan el_aqscan;		/* aqua scan */
		struct el_aq_bi el_aqbi;		/* aqua bi adapter */
		struct el_aq_kaf el_aqkaf;	/* aqua keep alive fail. */
		struct el_aq_clk el_aqclk;		/* aqua clock errors */
		struct el_aq_config el_aqconfig;	/* aqua config change */
		struct el_xja el_xja;			/* xja adapter errs */
		struct el_fza	el_fza;			/* fza port errors */
		struct el_vec6400 el_vec6400;	/* vector processor for 6400 */
	} el_body;
	char eltrailer[EL_TRAILERSIZE];			/* asc trailer code  */
};

#ifdef KERNEL
extern struct el_rec *ealloc();
struct lock_t lk_errlog;
#endif /* KERNEL */
#endif /* __ERRLOG__ */
