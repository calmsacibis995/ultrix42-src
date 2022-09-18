/*
 *	@(#)autoconf_data.c	4.2	(ULTRIX)	8/3/90
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
/************************************************************************
 *
 *			Modification History
 *
 * 03-Aug-90 rafiey (Ali Rafieymehr)
 *	Added VAX9000 to CPUs needing emulation code. Also added stub.
 *
 * 06-Jun-90 Pete Keilty
 *	Remove CIADAP replace with CIISR.
 *	1. Remove ciintv[] it is made with config and is in ioconf.c.
 *	2. Remove CIADAP replaced with new structure CIISR.
 *
 * 28-Dec-89 Robin
 *	Added nNKDM nNKLESIB nNMBA so unifind can know if the bus is there
 *	when its called.  If unifind is called now and no devices are
 *	configed on the bus (no bus) then the system crashes.
 *
 * 14-Oct_89 Robin
 *	The cpu.h include needs the types.h include before its used.
 *	This include of types.h should be in cpu.h but I seem to
 *	remember a "rule (?)" about no nested includes to help
 *	make depend run, so I'll put it here.  XXX
 *
 * 13-Oct-1989 gmm
 *	Moved the include location of cpu.h before cpudata.h. Needed for
 *	MIPS smp support
 *
 * 08-June-1989	Robin
 *	added a stub routine for uqdriver so that it would be
 *	defined if nothing causes the uqdriver to be in the system.  This
 *	is needed in machdep gendump routine to allow dumps on Q-bus
 *	devices.  Also added #if on uba_hd structure declaration to make
 *	it a size of one if all the uba devices evaluste to zero.  It
 *	was causing a complie warning on array uba_hd[0,0,0]; no wonder!
 *
 * 20-Jul-89	rafiey (Ali Rafieymehr)
 *	included two stub routines (xmisst(), and get_xmi()).
 *
 * 20-Jul-1989  map (Mark A. Parenti)
 *	Include number of KDM70's when sizing uba structures.
 *
 * 24-May-1989	darrell
 *	changed the #include to find cpuconf.h in it's new location -- 
 *	sys/machine/common/cpuconf.h
 *
 * 24-Mar-1989  Pete Keilty
 * 	Added msi interrupt routine for mips.
 *
 * 21-Mar-1989  Pete Keilty
 *	Added ci interrupt routines for mips. Also added ifdef vax
 * 	around mba.h
 *
 * 30-jan-1989	jaw
 * 	cleanup of SMP per-cpu data.
 *
 * 09-Dec-1988	Todd M. Katz			TMK0002
 *	1. Changed MSI defines completely:
 *		1) The variable nummsi is always defined and declared.
 *		2) Dummy routines are never defined( their definitions have
 *	   	   been moved to conf.c for consistentcy ).
 *		3) Define and declare nNMSI, msi_adapt[], and msiintv[] only
 *		   when local MSI ports have been configured.
 *	2. Currently only one CI port is supported.  Change ciintv[] to reflect
 *	   this and rename the appropriate locore interrupt service routine
 *	   from Xcia0int() -> Xci0int().
 *
 * 20-Apr-1988  Ricky Palmer
 *	Added an omitted dummy "msiintr" routine to MSI defines.
 *
 * 24-Mar-1988	Robin
 *	Added code to protect locore from causing undefines in processors that
 *	do not config in the NI or MSI drivers.  Locore calls the interupt
 *	routines and if they are not there the stub in here is used.  Also
 *	the interupt for a NI on a ka640 needs to go to STRAY if the ka640
 *	does not config in the device and that is also done here.
 *
 * 15-Feb-1988	Fred Canter
 *	Added VAX420 (CVAXstar/PVAX) to CPUs needing emulation code.
 *	Also added VAX3600 & VAX6200, which were missing.
 *
 * 08-Jan-1988	Todd M. Katz			TMK0001
 *	Added the data variable scs_disable.  This variable is initialized
 *	by SCS to contain the address of the SCS shutdown routine.  Otherwise,
 *	it is referenced only during panics when the specified shutdown
 *	routine is invoked to permanently disable all local system ports.
 *
 * 12-11-87	Robin L. and Larry C.
 *	Added portclass support to the system.
 *
 * 12-Aug-86  -- prs	Removed isGENERIC option.
 *
 * 3-Aug-86   -- jaw 	allocate a uba struct for klesib
 *
 * 5-Jun-86   -- jaw 	changes to config.
 *
 * 14-Apr-86 -- jaw
 *	remove MAXNUBA referances.....use NUBA only!
 *
 * 18-Mar-86 -- jaw  add routines to cpu switch for nexus/unibus addreses
 *		     also got rid of some globals like nexnum.
 *		     ka8800 cleanup.
 *
 *	Stephen Reilly, 22-Mar-85
 *	Added new structures for the floating emulation stuff
 *
 ***********************************************************************/

#include "ci.h"
#include "uq.h"
#include "msi.h"
#include "ln.h"
#include "vaxbi.h"
#ifdef mips
#include "ne.h"
#endif

#ifdef vax
#include "mba.h"
#else
#define NMBA 0
#endif vax

#include "uba.h"
#include "kdb.h"
#include "klesib.h"
#include "kdm.h"

#include "../h/types.h"			/* cpu.h needs this */
#include "../machine/pte.h"
#include "../machine/cpu.h"

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/map.h"
#include "../h/buf.h"
#include "../h/dk.h"
#include "../h/vm.h"
#include "../h/conf.h"
#include "../h/dmap.h"
#include "../h/reboot.h"
#include "../h/cpudata.h"


#ifdef vax
#include "../machine/mem.h"
#include "../machine/mtpr.h"
#include "../machine/ioa.h"
#include "../machine/nexus.h"
#endif vax

#include "../machine/scb.h"
#include "../io/ci/ciadapter.h"

#ifdef vax
#include "../io/mba/vax/mbareg.h"
#include "../io/mba/vax/mbavar.h"
#endif vax

#include "../io/uba/ubareg.h"
#include "../io/uba/ubavar.h"
#include "../../machine/common/cpuconf.h"

#ifdef mips
struct qbm_regs *qb_ptr;	/* Points to unibus adaptor regs */
#endif mips

#ifdef	BINARY
#if NCI > 0
extern	int	(*ciintv[])();
extern	int	nNCI;
#endif NCI

#if NMSI > 0
extern	int	(*msiintv[])();
extern	int	nNMSI;
#endif NMSI

extern	int	numbvp;
extern	int	numci;
extern	int	nummsi;
#ifdef vax
extern	int	(*mbaintv[])();
#endif vax
extern	int	(*ubaintv[])();
extern	int	nNMBA;
extern	(*Mba_find)();
extern	int	nNUBA;
extern  int	nNKDM;
extern  int	nNKDB;
extern  int	nNMBA;
extern  int	nNKLESIB;

#else
int cpu_avail = 1;

/*
 * Addresses of the (locore) routines which bootstrap us from
 * hardware traps to C code.  Filled into the system control block
 * as necessary.
 */
#ifdef mips
#if NVAXBI == 0
bisst() {}	/* stub for VAXBI start self test */
#endif NVAXBI
#endif mips

#if NUQ == 0
uqdriver() {}	/* stub for machdep gendump() */
#endif NUQ

#if NMSI > 0
extern	int	Xmsi0int();
int	(*msiintv[1])() =	{ Xmsi0int };
#endif NMSI

#if NMBA > 0 
int	(*mbaintv[4])() =	{ Xmba0int, Xmba1int, Xmba2int, Xmba3int };
#endif NMBA 

#ifdef VAX8600
int	(*ubaintv[7])() =	{ Xua0int, Xua1int, Xua2int, Xua3int, Xua4int, Xua5int, Xua6int }; 
#else
#ifdef VAX780
int	(*ubaintv[4])() =	{ Xua0int, Xua1int, Xua2int, Xua3int };
#else VAX780
int	(*ubaintv[4])() = 	{ (int (*)()) -1, ( int(*)()) -1, ( int(*)()) -1, ( int(*)()) -1};
#endif VAX780
#endif VAX8600

#ifdef mips		/* Stubs for the SGEC */
#if NNE == 0
neintr(){stray(0,0xd4);}
neprobe(){return(-1);     /* stub returns -1 if its not configured */ }
#endif NNE
#endif mips
		/* Make locore and ka650 happy if no NI device is configed in */
#if NLN == 0

#ifdef vax
lnintr(){logstray(1,0x14,0xd4);} /* ELSI_SCB */
#endif vax
#ifdef mips
lnintr() {stray(0,0xd4);} 
#endif mips

lnprobe(){return(-1);     /* stub returns -1 if its not configured */ }
#endif NLN
		/* Protect ka650.c from undefines if no MSI devices are configed in */

/*
 * Allocate the MSI adapter data structures.
 */
int		nummsi = 0;		/* Number of local MSI ports	     */
#if NMSI > 0
int		nNMSI = NMSI;		/* Number of configured MSI ports    */
struct _pccb	*msi_adapt[ NMSI ];	/* MSI adapter structures	     */

#ifdef mips
Xmsi0int() { msi_isr(0); } 
#endif mips

#endif NMSI

int		numbvp = 0;		/* Number of local BVP ports	     */

/*
 * Allocate the CI adapter structures
 */
int		numci = 0;		/* Number of local CI ports	     */
#if NCI > 0
int		nNCI = NCI;		/* Number of configured CI ports     */
CIISR		ci_isr[ NCI ];

#else
CIISR		ci_isr[ 1 ];
#endif NCI

void		( *scs_disable )()	/* Address of SCS shutdown routine   */
		    = NULL;		/* Initialized by SCS		     */

/*
 * This allocates the space for the per-uba information,
 * such as buffered data path usage.
 */
#if NUBA != 0  || NKDB != 0 || NKLESIB != 0 || NKDM != 0
struct	uba_hd uba_hd[NUBA+NKDB+NKLESIB+NKDM];
#else
struct uba_hd uba_hd[1];
#endif NUBA

#if NUBA > 0
int	tty_ubinfo[NUBA];
#else
int	tty_ubinfo[1];
#endif

#if NMBA > 0
extern	mbafind();
int	(*Mba_find)() = mbafind;
#else NMBA
int	(*Mba_find)() = (int (*)()) -1;
mbintr()	{/* Keep locore happy */ }
#endif NMBA

#include "xmi.h"
#if NXMI == 0
get_xmi() {/* stub */}
xmisst() {/* stub */}
int numxmi;
#endif

int	nNKDM = NKDM;
int	nNUBA = NUBA;
int	nNKDB = NKDB;
int	nNMBA = NMBA;
int	nNKLESIB = NKLESIB;

#ifdef vax
#ifdef	EMULFLT

asm(".globl	_vaxopcdec");
asm("_vaxopcdec:	.long	vax$opcdec");
asm(".globl	_vaxspecialhalt");
asm("_vaxspecialhalt: .long	vax$special_halt");
asm(".globl	_vaxspecialcont");
asm("_vaxspecialcont: .long	vax$special_cont");
asm(".globl	_vaxemulbegin");
asm("_vaxemulbegin:	.long	vax$emul_begin");
asm(".globl	_vaxemulend");
asm("_vaxemulend:	.long	vax$emul_end");
asm(".globl	_exeacviolat");
asm("_exeacviolat:	.long	exe$acviolat");

#else	EMULFLT

int (*vaxopcdec)() = 0;

#if defined (MVAX) || defined (VAX3600) || defined (VAX6200) || defined (VAX420) || defined (VAX9000)
asm(".globl	_vaxspecialhalt");
asm("_vaxspecialhalt: .long	vax$special_halt");
asm(".globl	_vaxspecialcont");
asm("_vaxspecialcont: .long	vax$special_cont");
asm(".globl	_vaxemulbegin");
asm("_vaxemulbegin:	.long	vax$emul_begin");
asm(".globl	_vaxemulend");
asm("_vaxemulend:	.long	vax$emul_end");
asm(".globl	_exeacviolat");
asm("_exeacviolat:	.long	exe$acviolat");

#else MVAX || VAX420

int (*vaxspecialhalt)() = 0;
int (*vaxspecialcont)() = 0;
int (*vaxemulbegin)() = 0;
int (*vaxemulend)() = 0;
int (*exeacviolat)() = 0;
#endif MVAX || VAX420

#endif EMULFLT
#endif vax

#endif BINARY

