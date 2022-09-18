#ifndef lint
static char *sccsid = "@(#)ka6400.c	4.10	ULTRIX	4/11/91";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1990 by				*
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

/***********************************************************************
 *
 * Modification History:
 *
 * 11-Apr-91	dlh
 *	ka6400conf()
 *		- get vpmask from cca_vec_enabled field
 *		- correct printf when xrv is enabled
 *		- write 0 to VP enabled bit in ACCS when boot CPU is not in 
 *		  vpmask.
 *		  note: question write of 0 to ACCS when CPU is not in vpmask.
 *		        this may not be necessary.
 *	
 *	ka6400machcheck()
 *		removed debug prinf's
 *	
 *	ka6400harderr()
 *		comment out debug printf's
 *	
 *	ka6400softerr()
 *		removed debug printf's
 *
 * 09-Apr-91	szczypek
 *		Added call to recover_mem_error() in ka6500softerr().
 *		Call made if CRD detected.  Need to call this routine
 *		in order to clear out CRD error bit in XBER.
 *
 * 21-Dec-90	szczypek
 *		Added code to clear_xrperr() routine which will clear
 *		out all pcache, bcache, and scc error bits as well as
 *		xbe (and xbeer on Mariah).  This code is required in
 *		order to ensure that the primary cache is on.
 *		
 * 20-dec-90	dlh
 *		added parameter to vp_reset() call
 *
 * 03-Dec-90	szczypek
 *		Ensured that ka6400harderr() and ka6400softerr() passed
 *		back return values for ka6500harderr() and
 *		ka6500soferr().  Added return value to ka6400setcache()
 *		and ka6500harderr() to satify LINT.  Added line to
 *		initialize xmidata pointer in ka6400machcheck().
 *
 * 06-Nov-90	szczypek
 *		add error handling (retry) code for rer/rse/tto errors
 *		on idents.  For VAX6000-5x0.
 *
 * 10-oct-90	dlh
 *		add vector support for the Mariah (VAX6000-5xx)
 *
 * 4-sep-90	dlh
 *		add vector support to:  ka6400machcheck(), ka6400conf(),
 *		 ka6400harderr()
 *
 * 02-may-90    szczypek
 *		add support for XMP cpu.  Will reuse as much XRP code as
 *              possible, with IF-THEN-ELSE being used in areas where cpu
 * 		specific code must be executed.
 *
 * 12-jan-90	haeck
 *		add a check for CRD (Corrected Read) memory error packets;
 *		prevent creation of an error packet if it is a CRD error.
 *
 * 08-dec-89 	jaw remove prinf in init routine.
 *
 * 29-Nov-89    Paul Grist
 *      modified ka6400machcheck() to call log_xmi_bierrors and log_xmierrors
 *      to look for any pending XBI or VAXBI errors.
 *
 * 24-May-89	darrell
 *	Changed the #include for cpuconf.h to find it in it's new home --
 *	sys/machine/common/cpuconf.h
 *
 * 24-May-89	darrell
 *	Removed the v_ prefix from all cpusw fields, removed cpup from any
 *	arguments being passed in function args.  cpup is now defined
 *	globally -- as part of the new cpusw.
 *
 * 05-Mar-89	Tom Kong
 *		Added workaround so that the cache manipulation code
 *		works for both 1st pass and 2nd pass XRPs.  Removed
 *		conditional compilation based on revision level of
 *		the XRP.  Improved soft error handling for Rigel.
 *
 * 1-Feb-89	Tom Kong
 *		Added work-around for 2nd pass XRP bugs.
 *		My understanding of the bug:  If an XMI bus timeout occurs
 *		at some critical point in the XMI state machine, the
 *		2nd Error bit in the RCSR will be set on the first occurence
 *		of this error (the bit shouldn't be set).  Since this bit
 *		drives the hard error interrupt line, an extraneous hard
 *		error interrupt is posted.
 *		Solution: the machine check handler, the hard error handler,
 *		and the soft error handler all clear the error bits in the
 *		RCSR and XBE that were associated with the errors being
 *		serviced.  The hard error handler can treat the error as
 *		extraneous under the following three conditions:
 *		1) If RCSR<4> (2nd error bit) is set, and no other error
 *		   bits in the XBE and RCSR are set other than XBE<27>,
 *		   XBE<23>, XBE<19>, and XBE<14>.  This condition indicates
 *		   the 2nd error bit was erroneously set during a machine
 *		   check.
 *		2) RCSR<4> and RCSR<27> are set and no other error bits in
 *		   the XBE or RCSR are set other than XBE<27>, XBE<23>,
 *		   XBE<19>, XBE<18>, XBE<17>, XBE<16>, XBE<14>, and XBE<13>.
 *		   This condition indicates the 2nd error bit was erroneously
 *		   set due to a cache-fill error.
 *		3) RCSR and XBER have no error bit set.
 *
 * 10-Jan-89	Kong
 *		Modified to run with SMP kernel.
 *
 * 6-Sep-88	Tom Kong
 *		Created this file from ka6200.c
 *
 **********************************************************************/

#include "../machine/pte.h"
#include "../h/param.h"
#include "../h/conf.h"
#include "../h/time.h"
#include "../h/errno.h"
#include "../h/systm.h"
#include "../h/types.h"
#include "../h/map.h"
#include "../h/buf.h"
#include "../h/errlog.h"
#include "../h/ioctl.h"
#include "../h/tty.h"
#include "../h/cpudata.h"
#include "../../machine/common/cpuconf.h"
#include "../h/kmalloc.h"
#include "../h/vmmac.h"

#include "../machine/cons.h"
#include "../machine/cons.h"
#include "../machine/cpu.h"
#include "../machine/clock.h"
#include "../machine/mtpr.h"
#include "../machine/mem.h"
#include "../machine/nexus.h"
#include "../machine/scb.h"
#include "../io/uba/ubareg.h"
#include "../io/uba/ubavar.h"
#include "../io/bi/buareg.h"
#include "../machine/sas/vmb.h"
#include "../machine/ka6400.h"
#include "../machine/cvax.h"

#include "../io/xmi/xmireg.h"
#include "../io/xmi/xmareg.h"

#include "../h/proc.h"
#include "../machine/vectors.h"

/*#define KILL_USER*/
#ifdef KILL_USER		/* Only do it when not running ASMP */
/*
 * Need u symbols, ...etc to terminate user process
 */
#include "../h/user.h"	
#include "../h/proc.h"
#include "../machine/psl.h"
#endif

#ifndef	KILL_USER	/* need u symbols for vp stuff */
#include "../h/user.h"
#endif

extern struct bidata bidata[];
extern int cache_state;
extern int nNVAXBI;
extern char *calypso_ip_addr;		/* In spt.s	*/
extern int fl_ok;			/* In locore.s  */

char *mcrvax[] = {
	"Unknown machine check type code 0",			    /* 0 */
	"Protocol error during F-chip operand/result transfer",     /* 1 */
	"Illegal opcode detected by F-chip",			    /* 2 */
	"Operand parity error detected by F-chip",		    /* 3 */
	"Unknown status returned by F-chip",			    /* 4 */
	"Returned F-chip result parity error",			    /* 5 */
	"Unknown machine check type code 6",			    /* 6 */
	"Unknown machine check type code 7",			    /* 7 */
	"TB miss status generated in ACCVIO/TNV microflow",	    /* 8 */
	"TB hit status generated in ACCVIO/TNV microflow",	    /* 9 */
	"Undefined INT.ID value during interrupt service",	    /* a */
	"Undefined state bit combination in MOVCx",		    /* b */
	"Undefined trap code produced by I-box",		    /* c */
	"Undefined control store address reached",		    /* d */
	"Unknown machine check type code 0xe",			    /* e */
	"Unknown machine check type code 0xf",			    /* f */
	"P-cache tag or data parity error during read",		    /* 10*/
	"DAL bus or data parity error during read",		    /* 11*/
	"DAL bus error on write or clear write buff",		    /* 12*/
	"Undefined bus error microtrap",			    /* 13*/
	"Vector unit error",					    /* 14*/
        "Error on I-stream read",                                   /* 15*/  /* Added for XMP support */
	"Unknown machine check type code > 0x15"  		    /* others */ /* Modified for XMP support */
};


/*
 * Array of virtual addresses that when written,
 * cause an IP interrupt to be sent.
 *
 * Usage:
 *	*calypso_ip[n] = 0	to send an IP interrupt to node n
 */
char *calypso_ip[16];		

/*
 * Data structure to keep track of the latest machine check information,
 * One structure per processor.
 */
struct	mchk_data {
long	mchk_in_progress;	/* Flag set while we're running mcheck code */
long	time;			/* time stamp of the error */

/* The following are a copy of the machine check frame */
long	mcode;			/* R bit and machine check code	   */
long	vaddr;			/* virtual address		   */
long	viba;			/* */
long	iccs_sisr;		/* ICCS and SISR		   */
long	istate;			/* Internal state		   */
long	sc;			/* SC */
long	pc;			/* program counter		   */
long	psl;			/* PSL				   */
};
struct	mchk_data *mchk_data;	/* Pointer to an array		   */

/*
 * XMP-specific registers which differ from XRP.
 */

#define XMP_BCIDX	112
#define XMP_BCSTS	113
#define XMP_BCCTL	114
#define XMP_BCERA       115
#define XMP_BCBTS	116
#define XMP_BCDET       117
#define XMP_BCERT       118

#define XMI_WCDE0       0x80000
#define XMI_WCDE1       0x8000000

#define XBEER0_ACPE     0x20000000
#define XBEER0_WDPE     0x40000000

#define EVEN_PARITY     0
#define ODD_PARITY      1

extern int cpu_sub_subtype;

int pcache_disable[MAXCPU];
int bcache_disable[MAXCPU];
unsigned int pcache_errtime[MAXCPU];
unsigned int bcache_errtime[MAXCPU];
int pc_enable[MAXCPU];
int bc_enable[MAXCPU];
unsigned int bcache_error[MAXCPU];
unsigned int pcache_error[MAXCPU];

/*
 * Data structure to store snapshot of hardware registers.
 * One structure per XRP processor.
 */

struct error_data {
/* The following are a snapshot of various hardware registers */
long	s_rcsr;			/* REXMI Control & Status Register (XRP only)*/
long	s_xber;			/* REXMI/MAXMI Bus Error Register	*/
long	s_xfadr;		/* REXMI/MAXMI Failing Address Register	*/
long	s_sscbtr;		/* RSSC/MSSC Bus Timeout Register	*/
long	s_bcctl;		/* C-chip Control Register		*/
long	s_bcsts;		/* C-chip Status Register		*/
long	s_bcerr;		/* C-chip Error Address Register	*/
long	s_pcsts;		/* P-cache Status Register		*/
long	s_pcerr;		/* P-cache Error Address Register	*/
long	s_vintsr;		/* C-chip Vector Interface Error Status */
/* Below are the XMP-specific registers */
long    s_xfaer0;               /* MAXMI Failing Address Extension Reg0 */
long    s_xbeer0;               /* MAXMI Bus Error Extension Register 0 */
long    s_wfadr0;               /* MAXMI Failing Address Reg for Wback0 */
long    s_wfadr1;               /* MAXMI Failing Address Reg for Wback1 */
long    s_fdal0;                /* MAXMI Failing DAL register 0         */
long    s_fdal1;                /* MAXMI Failing DAL register 1         */
long    s_fdal2;                /* MAXMI Failing DAL register 2         */
long    s_fdal3;                /* MAXMI Failing DAL register 3         */
long    s_bcert;                /* C-chip Error Tag Register            */
};
struct	error_data *error_data;	              /* Pointer to an array     */

/* the following defines should be in mtpr.h, but since we don't want the 
 * world to rebuild for a FT patch ....
 */
/*
 * Bits within the Rigel VAX6000-4xx P-cache status register
 * (as defined in the XRP CPU spec rev 1.0).  All but one are the same for
 * XMP also.
 *	the following bits may be read or written by the user or the
 *	hardware: PCSTS_FORCE_HIT
 *		  PCSTS_ENABLE_PTS 
 *		  PCSTS_ENABEL_REFRESH  (XRP only)
 *	the following bits may only be written by the user.  if this bit is
 *	read it will always read as a 0:
 *		  PCSTS_FLUSH_CASHE
 *	the following bits may be read or written by the user.  writing a '1' 
 *	to this bit will clear the bit.  writing a '0' to this bit has no 
 *	effect on the bit.  the hardware may read or write this bit:
 *		  PCSTS_INTERRUPT
 *		  PCSTS_TRAP2
 *		  PCSTS_TRAP1
 *	the following bits may be read by the user but not written.  only 
 *	hardware can change the value of the bit.  user writes to this bit 
 *	are ignored:
 *		  PCSTS_P_CASHE_HIT
 *		  PCSTS_TAG_PARITY_ERROR
 *		  PCSTS_DAL_DATA_PARITY_ERROR
 *		  PCSTS_P_DATA_PARITY_ERROR
 *		  PCSTS_BUS_ERROR
 *		  PCSTS_B_CASHE_HIT
 */
#define	PCSTS_FORCE_HIT			0x0001
#define	PCSTS_ENABLE_PTS		0x0002
#define	PCSTS_FLUSH_CASHE		0x0004
#define	PCSTS_ENABEL_REFRESH		0x0008  /* XRP only */
#define	PCSTS_P_CASHE_HIT		0x0010
#define	PCSTS_INTERRUPT			0x0020
#define	PCSTS_TRAP2			0x0040
#define	PCSTS_TRAP1			0x0080
#define	PCSTS_TAG_PARITY_ERROR		0x0100
#define	PCSTS_DAL_DATA_PARITY_ERROR	0x0200
#define	PCSTS_P_DATA_PARITY_ERROR	0x0400
#define	PCSTS_BUS_ERROR			0x0800
#define	PCSTS_B_CASHE_HIT		0x1000

/*
 * XRP only!
 *
 * Bits within the Rigel VAX6000-4xx REXMI Control And Status Register (RCSR)
 * (as defined in the XRP CPU spec rev 1.0)
 *	the following bits may be read or written by the user or the
 *	hardware (M):
 *		RCSR_WBD
 *		RCSR_ARD
 *		RCSR_ESI
 *		RCSR_IPID
 *		RCSR_TOS
 *		RCSR_RAM_SPD
 *		RCSR_CRDID
 *		RCSR_CCID
 *		RCSR_BPD
 *		RCSR_BP
 *		RCSR_LOCKOUT
 *		RCSR_LTS
 *		RCSR_RSSC_IPL
 *		RCSR_WD
 *	the following bits may be read or written by the user.  writing a '1' 
 *	to this bit will clear the bit.  writing a '0' to this bit has no 
 *	effect on the bit.  the hardware may read or write this bit (WC):
 *		RCSR_CNAKR
 *		RCSR_UWP
 *		RCSR_CFE
 *		RCSR_WDPE
 *		RCSR_XDP0PE
 *		RCSR_XDP1PE
 *		RCSR_XCAPE
 *	the following bits may be read by the user but not written.  only 
 *	hardware can change the value of the bit.  user writes to this bit 
 *	are ignored (RO):
 *		RCSR_XCA_REV
 *		RCSR_WS
 */
#define	RCSR_XCA_REV	0x0000000f	/* RO - XCA REV */
#define	RCSR_WBD	0x00000100	/* M - Write Buffer Disable */
#define	RCSR_ARD	0x00000200	/* M - Auto Retry Disable */
#define	RCSR_ESI	0x00000400	/* M - Enable Self Invalidates */
#define	RCSR_IPID	0x00000800	/* M - IP Interrupt Disable */
#define	RCSR_TOS	0x00001000	/* M - Timeout Select */
#define	RCSR_RAM_SPD	0x00002000	/* M - Ram Speed */
#define	RCSR_CRDID	0x00004000	/* M - CRD Interrupt Disable */
#define	RCSR_CCID	0x00003000	/* M - CC Interrupt Disable */
#define	RCSR_WS		0x00010000	/* RO - WARM Start */
#define	RCSR_BPD	0x00020000	/* M - Boot Processor Disable */
#define	RCSR_BP		0x00040000	/* M - Boot Processor */
#define	RCSR_CNAKR	0x00080000	/* WC - Commander NoAck Received */
#define	RCSR_UWP	0x00100000	/* WC - Unlock Write Pending */
#define	RCSR_LOCKOUT	0x00600000	/* M - (two bits) LOCKOUT */
#define	RCSR_LTS	0x00800000	/* M - Lockout Time Select */
#define	RCSR_RSSC_IPL	0x03000000	/* M - (two bits) RSSC IPL<1:0> */
#define	RCSR_WD		0x04000000	/* M - Write Disable */
#define	RCSR_CFE	0x08000000	/* WC - Cache Fill Error */
#define	RCSR_WDPE	0x10000000	/* WC - Write Data Parity Error */
#define	RCSR_XDP0PE	0x20000000	/* WC - XDP0 Parity Error */
#define	RCSR_XDP1PE	0x40000000	/* WC - XDP1 Parity Error */
#define	RCSR_XCAPE	0x80000000	/* WC - XCA Parity Error */

/*
 * XRP only!
 *
 * The following register exists on both XRP and XMP, but the definitions
 * here only exist on XRP.
 *
 * Bits within the Rigel VAX6000-4xx c-chip status register, BCSTS
 * (as defined in the XRP CPU spec rev 1.0)
 *	the following bits may be read or written by the user.  writing a '1' 
 *	to this bit will clear the bit.  writing a '0' to this bit has no 
 *	effect on the bit.  the hardware may read or write this bit (WC):
 *		BCSTS_STATUS_LOCK
 *	the following bits may be read by the user but not written.  only 
 *	hardware can change the value of the bit.  user writes to this bit 
 *	are ignored (RO):
 *		BCSTS_BTS_PERR
 *		BCSTS_P1TS_PERR
 *		BCSTS_P2TS_PERR
 *		BCSTS_BUS_ERR
 *		BCSTS_BTS_COMPARE
 *		BCSTS_BTS_HIT
 *		BCSTS_P1TS_HIT
 *		BCSTS_P2TS_HIT
 *		BCSTS_DAL_CMD
 *		BCSTS_IBUS_CYCLE
 *		BCSTS_PRED_PARITY
 */

#define	BCSTS_STATUS_LOCK	0x00000001	/* WC */
#define	BCSTS_BTS_PERR		0x00000002	/* RO */
#define	BCSTS_P1TS_PERR		0x00000004	/* RO */
#define	BCSTS_P2TS_PERR		0x00000008	/* RO */
#define	BCSTS_BUS_ERR		0x00000010	/* RO */
#define	BCSTS_BTS_COMPARE	0x00020000	/* RO */
#define	BCSTS_BTS_HIT		0x00040000	/* RO */
#define	BCSTS_P1TS_HIT		0x00080000	/* RO */
#define	BCSTS_P2TS_HIT		0x00100000	/* RO */
#define	BCSTS_DAL_CMD		0x01e00000	/* RO - 4 bits */
#define	BCSTS_IBUS_CYCLE	0x02000000	/* RO */
#define	BCSTS_PRED_PARITY	0x04000000	/* RO */

/*
 * XMP only!
 *
 * The following register exists on the XMP and XRP modules, but the
 * definitions here are unique to the XMP.
 *
 * Bits within the Mariah VAX6000-5xx c-chip status register, BCSTS
 * (as defined in the XMP CPU spec rev 1.0)
 *	the following bits may be read or written by the user.  writing a '1' 
 *	to this bit will clear the bit.  writing a '0' to this bit has no 
 *	effect on the bit.  the hardware may read or write this bit (WC):
 *		BCSTS_BTS_TPERR
 *              BCSTS_BTS_VDPERR
 *              BCSTS_I_PERR
 *              BCSTS_FILL_ABORT
 *              BCSTS_AC_PERR
 *	the following bits may be read by the user but not written.  only 
 *	hardware can change the value of the bit.  user writes to this bit 
 *	are ignored (RO):
 *              BCSTS_ERR_SUMMARY
 *              BCSTS_SECOND_ERR
 *		BCSTS_BTS_HIT_XMP
 *		BCSTS_BTS_COMPARE_XMP
 *		BCSTS_PPG
 *		BCSTS_BTS_PARITY
 *		BCSTS_IBUS_CYCLE_XMP
 *		BCSTS_IBUS_CMD
 *		BCSTS_DAL_CMD_XMP
 *		BCSTS_DMG_L
 *		BCSTS_SYNC_L
 *              BCSTS_AC_PARITY
 *              BCSTS_OREAD_PENDING
 */

#define	BCSTS_ERR_SUMMARY	0x00000001	/* RO */
#define	BCSTS_BTS_TPERR		0x00000002	/* WC */
#define	BCSTS_BTS_VDPERR	0x00000004	/* WC */
#define	BCSTS_I_PERR		0x00000030	/* WC */
#define BCSTS_FILL_ABORT        0x00000040      /* WC */
#define BCSTS_AC_PERR           0x00000080      /* WC */
#define BCSTS_SECOND_ERR        0x00000100      /* RO */
#define	BCSTS_BTS_HIT_XMP	0x00008000	/* RO */
#define	BCSTS_BTS_COMPARE_XMP	0x00010000	/* RO */
#define	BCSTS_PPG		0x00020000	/* RO */
#define	BCSTS_BTS_PARITY	0x000C0000	/* RO */
#define	BCSTS_IBUS_CYCLE_XMP	0x00100000	/* RO */
#define	BCSTS_IBUS_CMD  	0x00200000	/* RO */
#define	BCSTS_DAL_CMD_XMP       0x03C00000	/* RO - four bits */
#define BCSTS_DMG_L             0x04000000      /* RO */
#define BCSTS_SYNC_L            0x08000000      /* RO */
#define BCSTS_AC_PARITY         0x10000000      /* RO */
#define BCSTS_OREAD_PENDING     0x20000000      /* RO */

/*
 * KA6400 machine check exception handler.
 *      Categories of errors causing machine checks:
 *	. Floating point processor problems.
 *	. Memory management, interrrupt, microcode/CPU errors
 *	. Primary cache read error - tag parity errors, data parity errors
 *	. DAL error on memory read - DAL data parity errors, B-cache RAMs, 
 *	  REXMI-->REX520 parity errors
 *	  ERR_L terminator, XBER notified errors, RSSC DAL timeouts
 *	. DAL error on memory write or flush write buffers, ERR_L terminator,
 *	  RSSC DAL timeouts
 *	. Vector unit errors
 *
 * This handler is also executed for ka6500 machine checks.  The routine will
 * use cpu_sub_subtype in places where cpu-specific code must be executed.
 *
 * Environment:
 *	IPL		0x1f
 *	stack		Interrupt stack
 *	SCB vector <1:0> = 01.
 *
 * Parameter:
 * mcf 	Points to machine check frame.  For Ka6400, the
 *	frame format:
 *
 *	. byte count	(0x18)
 *	. R bit (VAX restart bit) in <31>, mcheck code in <0:15>
 *	  Valid machine check codes are:
 *	  1, 2, 3, 4, 5, 8, 9, 0xa, 0xb, 0xc, 0xd, 0x10, 0x11, 0x12,
 *	  0x13, and 0x14.
 *	. Virtual Address processed by REX520 M-box, (not
 *	  necessarily relevant.
 *	. VIBA - The REX520 M-box prefetch virtual instruction
 *	  buffer address at the time of the fault.
 *	. ICCS.SISR   Interrupt state information in the following format:
 *		Bits	Contents
 *		<22>	ICCS<6>
 *		<15:1>	SISR<15:1>
 *	. Internal State, of the following format:
 *	  - bits<3:0>  RN, the value of the E-box RN register at the
 *	    time of the fault, possibly indicating the last GPR referenced
 *	    by the E-box during specifier or instruction flows.
 *	  - bits<7:4> Undefined
 *	  - bits<15:8> Opcode, the opcode (2nd if two-byte) of the instr
 *	    being processed at the time of the fault.
 *	  - bits<17:16> DL, the current setting of the E-box data length
 *	    latch, may relate to the last (or forthcoming) memory reference.
 *	    value: 0     byte
 *		   1     word
 *		   2     long, f_floating
 *		   3     quad, d_floating, g_floating
 *	  - bits<20:18> AT, the current setting of the E-box address type
 *	    latch, possibly relating to the last (or upcoming) memory
 *	    reference.
 *	    value: 0     read
 *		   1     write
 *		   2     modify
 *		   3     unassigned, REX520 chip error
 *		   4     unassigned, REX520 chip error
 *		   5     address
 *		   6     variable bit
 *		   7     branch
 *	  - bits<23:21> Undefined
 *	  - bits<31:24> Delta PC, difference in the values of the current
 *	    incremented PC at the time of the machine check was detected
 *	    and the PC of the instruction opcode.  This field shouldn't
 *	    be used by software to make recovery decisions as it is internal
 *	    pipeline specific.
 *	. SC, Internal microcode-accessible register.
 *	. Program counter.
 *	. PSL.
 *
 * Returns:
 *	0
 */
ka6400machcheck (mcf)
register struct el_mc6400frame *mcf;
{
	register int cpunum;
	register struct xrp_reg *xrp_node;
	register int xminode;
	int	restartable;		/* Flag set if we can restart instr*/
	int	must_panic;		/* Flag set if we must panic	*/
	struct xmidata *xmidata;
	register struct el_mc6400frame *framep;
	register struct el_mc6500frame *xmp_framep;
	register struct el_rec *elrp;
	struct el_mck *elmckp;
	register int mcode;
	long	time_now;

	int	r_bit;			/* Value of R bit in mcheck frame */
	int	fpd;			/* Value of PSL<fpd> in mcheck frame*/
	int	uwp;			/* Value of RCSR<uwp> 		  */
	int     trap2;                  /* Value of  */
	long	vector_mchk_type;
	int	ls_bug;			/* true if a hardware bug is found */
					/* this should never happen since  */
					/* we have installed the fix in    */
					/* our lab and the field should    */
					/* never get a board with the bug, */
					/* but ....                        */

	time_now = mfpr(TODR);		/* Log the current time	*/

	restartable = must_panic  = 0;	/* Initial assumption	*/

	cpunum=CURRENT_CPUDATA->cpu_num;


	if (mchk_data[cpunum].mchk_in_progress == 0) {
		/* if no machine check in progress, we're ok */
		mchk_data[cpunum].mchk_in_progress++;
		}
	else	{
		/* nested machine check, we're in big trouble	*/
		asm("halt");
	}

	mtpr(MCESR,0);	/* Tell hardware we acknowledge the machine check */
	snapshot_registers(cpunum); /* Take a snapshot of registers */

	/*
	 * The following bits are always checked to determine
	 * if the instruction can be restarted.  Lets read them in
	 * now.
	 */
	r_bit = (mcf->mcode & 0x80000000) >> 31;
	fpd = (mcf->psl & 0x08000000) >> 27;

	if(cpu_sub_subtype != MARIAH_VARIANT) {
		uwp = (error_data[cpunum].s_rcsr & 0x100000) >> 20;
		ka6400_disable_cache();/* to minimise chance of nested errors */
	}
	else {
		uwp =   (error_data[cpunum].s_xbeer0 & 0x800) >> 11; /* XMP */
		trap2 = (error_data[cpunum].s_pcsts & 0x40) >> 6;    /* XMP */
	        error_disable_cache();
	}
	xmidata = get_xmi(0);
	xrp_node = (struct xrp_reg *)xmidata->xmivirt + (rssc->s_iport & 0xf); 

	/*
	 * Case of the type of machine check, we may be able
	 * to recover from the machine check under certain conditions.
	 */
	mcode = mcf->mcode & 0xffff;
	switch (mcode) {
	case 1:	/* Protocol error during F-chip operand/result transfer */
	case 2:	/* Illegal opcode detected by F-chip */
	case 3:	/* Operand parity error detected by F-chip */
	case 4:	/* Unknown status returned by F-chip */
	case 5:	/* Returned F-chip result parity error */
		/* 
		 * The above errors are restartable under the following
		 * conditions:
		 * 	1) VAX restart bit set (R==1), and
		 *	2) FPD==0 (FPD of the saved PSL on mcheck frame), and
		 *	3) UWP==0 (REXMI RCSR unlock-write-pending clear).
		 * If the error re-occurs, disable the F-chip by writing
		 * a zero to ACCS<1>
		 */
		if (r_bit&& (fpd==0) && (uwp==0)) {
			/* Machine check is restartable */
			restartable = 1;
			if (mcode == (mchk_data[cpunum].mcode & 0xffff)) {
				/* Error occurred previously, turn off FPA */
				mtpr(ACCS,mfpr(ACCS) & ~2);
			}
		}
		break;
		
	case 8: /* TB miss status generated in ACCVIO/TNV microflow */
	case 9:	/* TB hit status generated in ACCVIO/TNV microflow */
	case 0xa: /* Undefined INT.ID value during interrupt service */
		/*
		 * The above are restartable when:
		 *  ((R==1)||(FPD==1)) && (UWP==0)
		 */
		if ((fpd | r_bit) && (uwp==0)) {
			/* Machine check is restartable */
			restartable = 1;
		}
		break;

	case 0xb: /* Undefined state bit combination in MOVCx */
		/* 
		 * Problem due to a MOVCx instruction, if software
		 * determines that no specifier have been over-written
		 * (MOVCx destroys R0-R5 and memory due to string writes),
		 * the instruction can be restarted from the beginning
		 * by clearing PSL<FPD>.  This should be done only if
 		 * the source and destination strings don't overlap and
		 *   (FPD==1) &&(UWP==0).
		 *
		 * Note: We can't determine if string overlaps, so we don't
		 * restart the instruction, just to be safe.
		 */
		break;

	case 0xc: /* Undefined trap code produced by I-box */
		/*
		 * Restartable when:
		 *   (R==1) && (FPD==0) && (UWP==0)
		 */
		if (r_bit && (fpd==0) && (uwp==0)) {
			restartable = 1;
		}
		break;

	case 0xd: /* Undefined control store address reached */
		/*
		 * Restartable when
		 *   ((R==1) || (FPD==1)) && (UWP==0)
		 */
		if ((r_bit | fpd) && (uwp==0)) {
			restartable = 1;
		}
		break;

	case 0x10: 
		/*
		 * P-cache tag or data parity error during read.
		 * PCSTS<TAG_PARITY_ERROR> or PCSTS<P_DATA_PARITY_ERROR>
		 * bits should be set, if neither bits is set,
		 * the state is inconsistent, panic.
		 * If either bit is set, do a full memory error recover.
		 * We can restart the instruction
		 * if ((R==1) || (FPD==1)) && (UWP==0) && (TRAP2==0)
		 */
		if ((error_data[cpunum].s_pcsts & 0x500)==0) {
			/* neither error bit set, panic */
			must_panic = 1;
		}
		else {	
			recover_mem_error();
		        if(cpu_sub_subtype == MARIAH_VARIANT) {
			  if(time_now - pcache_errtime[cpunum] < 500) {
			    pcache_error[cpunum]++;
			  }
			  else {
			    pcache_errtime[cpunum] = time_now;
			  }
			}
			if ((r_bit | fpd) && (uwp==0) && 
			    ((error_data[cpunum].s_pcsts & PCSTS_TRAP2)==0)) {
				restartable = 1;
			}
		}
		break;

	case 0x11: /* DAL bus or data parity error during read */
		/*
		 * Either PCSTS<DAL_DATA_PARITY_ERROR> or 
		 * PCSTS<BUS_ERROR> should be set.  If neither bit
		 * is set, or if both bits are set, something is
		 * seriously wrong, panic.
		 *
		 * If PCSTS<BUS_ERROR> is set, we have a bus error on
		 * D-stream read, which may be a RSSC bus timeout on
		 * D-stream read, or a memory error on requested quadword
		 * of D-stream read.  In either case, we don't really
		 * know how to recover, so panic.
		 *
		 * Thus if and only if PCSTS<DAL_DATA_PARITY_ERROR> is
		 * set can we attempt a restart.  A restart can be performed
		 * if  ((R==1) || (FPD==1)) && (UWP==0) && (TRAP2==0)
		 */
		if ((error_data[cpunum].s_pcsts & 0xa00)==0x200) {
			recover_mem_error();
		        if(((error_data[cpunum].s_pcsts & 0x1080) == 0x1080) && 
                           (cpu_sub_subtype == MARIAH_VARIANT)) {
			  if(time_now - bcache_errtime[cpunum] < 500) {
			    bcache_error[cpunum]++;
			  }
			  else {
			    bcache_errtime[cpunum] = time_now;
			  }
			}
			if ((r_bit | fpd) && (uwp==0) && 
			    ((error_data[cpunum].s_pcsts & PCSTS_TRAP2)==0)) {
				restartable = 1;
			}
		}
		else	{
			/* inconsistent PCSTS, or <BUS_ERROR> */
			must_panic = 1;
		}
		break;

	case 0x12: /* DAL bus error on write or clear write buf */
		if(cpu_sub_subtype == MARIAH_VARIANT)
		   /*
		    * MSSC Bus Timeout on Write or Clear-write-buffer.
		    *
		    * PCSTS<BUS_ERROR, TRAP1> both set, and
		    * SSCBTR<RWT,BTO> bot set
		    */
		   if(((error_data[cpunum].s_pcsts & 0x880) == 0x880) &&
		      ((error_data[cpunum].s_sscbtr & 0xc0000000) == 0xc0000000)) {
		        recover_mem_error();
		   }
		   /*
		    * Backup Cache Error on Write.
		    *
		    * PCSTS<BUS_ERROR, TRAP1 both set, 
		    * BCSTS: DMG_L, SYNC_L, and ES set, DAL_CMD = write,
		    *        and either TPERR or VDPERR set.
		    */
		   else if(((error_data[cpunum].s_pcsts & 0x880) == 0x880) &&
			   ((error_data[cpunum].s_bcsts & 0x0fc00001) == 0x0dc00001) &&
			   ((error_data[cpunum].s_bcsts & 0x6) != 0x0)) {
		             recover_mem_error();
		   }
		   /*
		    * Memory Error on Write.
		    *
		    * XBE: TTO|CNAK|WDNAK set,
		    * XFAER: XMI_CMD = write,
		    * PCSTS<TRAP1, BUS_ERROR> both set.
		    */
		   else if(((error_data[cpunum].s_xber & 0x10a000) != 0) &&
			   ((error_data[cpunum].s_xfaer0 & 0xc0000000) == 0x40000000) &&
			   ((error_data[cpunum].s_pcsts & 0x880) == 0x880)) {
			   /* Memory Error on Write */
		               xrp_node->xrp_xbe = error_data[cpunum].s_xber;
		   }
		must_panic=1;
		break;
	case 0x13: /* Undefined bus error microtrap */
		must_panic=1;
		break;
	case 0x14: /* Vector unit error */
		/*
		 * sanity check: systems with a low rev number should never
		 * generate a vector unit machine check.
		 */

		if ((cpu != VAX_6400) || (ccabase.cca_b_revision <= 3)) {
			goto bad_vector_mchk;
		}

		/* look for the footprint of the load store bug */
		/* note: this bug has been fixed in hardware, so if it
		 *       is ever encountered, there is a VERY old XRV in the
		 *       system.
		 */

		ls_bug = vp_ls_bug (error_data[cpunum].s_vintsr);


		if (ls_bug) {
		    printf ("vector mchk is load store bug\n");
		    must_panic = 1;
		}


		/*
		 * the following printf is a placehoder until I get the uerf
		 * packets defined.
		 */

		if (CURRENT_CPUDATA->cpu_vpdata->vpd_in_kernel ==  
			VPD_IN_KERNEL) {
			must_panic = 1;
			printf ("VP mchk while in kernel mode\n");
		} else {
			printf ("VP mchk while in user mode\n");
			vp_remove ();
			if (CURRENT_CPUDATA->cpu_vpdata->vpd_proc) {
			    u.u_code = TERM_VECT_HARD;
			    psignal (CURRENT_CPUDATA->cpu_vpdata->vpd_proc,
				SIGKILL);
			}
		}

		break;

bad_vector_mchk:

		/*
		 * branch to here if this is a rev before vectors were
		 * used.
		 */
	case 0x15: /* Error on I-stream read */
		/*
                 * This error is similar to the BUSERR_READ_DAL machine
		 * check, but occurs on the instruction stream.  Please
                 * see the description above (0x11).  This is an XMP
		 * only error.
                 */
		if (cpu_sub_subtype == MARIAH_VARIANT) {
		   if ((error_data[cpunum].s_pcsts & 0xa00)==0x200) {
			recover_mem_error();
		        if(((error_data[cpunum].s_pcsts & 0x1080) == 0x1080) && 
                           (cpu_sub_subtype == MARIAH_VARIANT)) {
			  if(time_now - bcache_errtime[cpunum] < 500) {
			    bcache_error[cpunum]++;
			  }
			  else {
			    bcache_errtime[cpunum] = time_now;
			  }
			}
			if ((r_bit | fpd) && (uwp==0) && 
			    ((error_data[cpunum].s_pcsts & PCSTS_TRAP2)==0)) 
				restartable = 1;
		   }
		   else	 { /* inconsistent PCSTS, or BUS ERROR */
		     must_panic = 1;
		   }
		   break;
		}
		else {
		  must_panic = 1;
		  break;
		}
	default:   /* Unknown machine check type code */
		/* 
		 * Panic on above errors since bad things are 
		 * already done and the error may be system-wide
		 */
		must_panic = 1;
		break;
	}

	if ((restartable==0) && (must_panic==0)) {
		/*
		 * Can't restart, but may not need to panic.  We
		 * see if we can't simply kill the user process and
		 * continue.
		 */
#ifdef KILL_USER
		if (USERMODE(mcf->psl) && (u.u_procp->p_pid != 1) &&
			(cpunum == 0)) {
			/*
			 * We were a user process, not the "init"
			 * process, and we are running on the master processor
			 */
			swkill(u.u_procp, "ka6400machcheck");
		}
		else {
			/* Can't kill user or restart, so we must panic */
			must_panic = 1;
		}
#else
		must_panic = 1;	
#endif
	}


	/*
	 * If we are not panicking, we will be recovering.  However,
	 * to avoid getting into a loop of error -> restart -> error ->
	 * restart, we force a panic if we had an identical machine check 
	 * previously or we had a machine check less than 5 seconds ago.
	 */
	if (must_panic == 0) {
		if ((mchk_data[cpunum].mcode == mcf->mcode) ||
			(time_now - mchk_data[cpunum].time < 500))
			must_panic = 1;
	}


	/* allocate a error log packet */
	if(cpu_sub_subtype != MARIAH_VARIANT) {
		elrp = ealloc((sizeof(struct el_mc6400frame)),
			must_panic ? EL_PRISEVERE:EL_PRIHIGH);
	        if (elrp) {
			LSUBID(elrp,ELCT_MCK,ELMCKT_6400,cpu_subtype,cpunum,EL_UNDEF,mcf->mcode);
   	                elmckp = &elrp->el_body.elmck;
			framep = (struct el_mc6400frame *) &elmckp->elmck_frame.
			  el6400mcf.bcnt;
			framep->bcnt = mcf->bcnt;
			framep->mcode = mcf->mcode;
	                framep->vaddr = mcf->vaddr;
			framep->viba = mcf->viba;
			framep->iccs_sisr = mcf->iccs_sisr;
			framep->istate = mcf->istate;
			framep->sc = mcf->sc;
			framep->pc = mcf->pc;
			framep->psl = mcf->psl;
			framep->s_sscbtr = error_data[cpunum].s_sscbtr;
			framep->s_bcsts = error_data[cpunum].s_bcsts;
			framep->s_pcsts = error_data[cpunum].s_pcsts;
			framep->s_pcerr = error_data[cpunum].s_pcerr;
			framep->s_vintsr = error_data[cpunum].s_vintsr;
			framep->s_rcsr = error_data[cpunum].s_rcsr;
			framep->s_xber = error_data[cpunum].s_xber;
			framep->s_xfadr = error_data[cpunum].s_xfadr;
			framep->s_bcctl = error_data[cpunum].s_bcctl;
			framep->s_bcerr = error_data[cpunum].s_bcerr;
			EVALID(elrp);	/* Make error log packet valid */
			cprintf("Machine check data logged in error log buffer.\n");
		}
		else cprintf("Can't log a machine check. (no buffer)\n");
	}
	else { /* XMP-specific */
		elrp = ealloc((sizeof(struct el_mc6500frame)),
			must_panic ? EL_PRISEVERE:EL_PRIHIGH);
	        if (elrp) {
        	        LSUBID(elrp,ELCT_MCK,ELMCKT_6500,cpu_sub_subtype,cpunum,EL_UNDEF,
                               mcf->mcode);
			elmckp = &elrp->el_body.elmck;
			xmp_framep = (struct el_mc6500frame *) &elmckp->elmck_frame.
			              el6500mcf.bcnt;
			xmp_framep->bcnt = mcf->bcnt;
        	        xmp_framep->mcode = mcf->mcode;
			xmp_framep->vaddr = mcf->vaddr;
			xmp_framep->viba = mcf->viba;
			xmp_framep->iccs_sisr = mcf->iccs_sisr;
			xmp_framep->istate = mcf->istate;
			xmp_framep->sc = mcf->sc;
			xmp_framep->pc = mcf->pc;
			xmp_framep->psl = mcf->psl;
			xmp_framep->s_sscbtr = error_data[cpunum].s_sscbtr;
			xmp_framep->s_bcsts = error_data[cpunum].s_bcsts;
			xmp_framep->s_pcsts = error_data[cpunum].s_pcsts;
			xmp_framep->s_pcerr = error_data[cpunum].s_pcerr;
			xmp_framep->s_vintsr = error_data[cpunum].s_vintsr;
			xmp_framep->s_xbe0 = error_data[cpunum].s_xber;
			xmp_framep->s_xfadr0 = error_data[cpunum].s_xfadr;
			xmp_framep->s_xfaer0 = error_data[cpunum].s_xfaer0;
			xmp_framep->s_xbeer0 = error_data[cpunum].s_xbeer0;
			xmp_framep->s_wfadr0 = error_data[cpunum].s_wfadr0;
			xmp_framep->s_wfadr1 = error_data[cpunum].s_wfadr1;
			xmp_framep->s_fdal0 = error_data[cpunum].s_fdal0;
			xmp_framep->s_fdal1 = error_data[cpunum].s_fdal1;
			xmp_framep->s_fdal2 = error_data[cpunum].s_fdal2;
			xmp_framep->s_fdal3 = error_data[cpunum].s_fdal3;
			xmp_framep->s_bcera = error_data[cpunum].s_bcerr;
			xmp_framep->s_bcert = error_data[cpunum].s_bcert;
			EVALID(elrp);		/* Make error log packet valid */
			cprintf("Machine check data logged in error log buffer.\n");
		}
		else cprintf("Can't log a machine check. (no buffer)\n");
	} /* END ELSE */


        /* look for any pending XMI errors */

        log_xmierrors(0,mcf->pc);
        log_xmi_bierrors(0,mcf->pc);

	
	/*
	 * If "must_panic" == 1, we will have to panic, otherwise
	 * we have either killed the user process and can continue,
	 * or we can simply restart the instruction that caused the
 	 * current machine check.
	 */
	if (must_panic) {
		cprintf("cpu %x ",rssc->s_iport & 0xf);
		cprintf("%s\n",mcrvax[mcode]);
		cprintf("iccs,sisr = 0x%x\t\t",mcf->iccs_sisr);
		cprintf("internal state = 0x%x\n",mcf->istate);
		cprintf("pc = 0x%x\t\t", mcf->pc);
		cprintf("psl = 0x%x\n",mcf->psl);
		/* Go check each XMI memory node for errors */
		rxma_check_errors(EL_PRISEVERE);
		if(cpu_sub_subtype == MARIAH_VARIANT) {
		  clear_xrperr();
		}
		panic("mchk");
	}


	/*
	 * When we get here, we are not crashing the system.
	 * clear flag to indicate we're done handling the machine check.
	 */
	mchk_data[cpunum].mchk_in_progress = 0;	


        /*
         * check for memory errors and report the recovery
         */

        rxma_check_errors(EL_PRILOW);

        printf("MACHINE CHECK RECOVERY occured\ntype: %s\n",mcrvax[mcode]);

	/* Save the mcheck info for later use */
	mchk_data[cpunum].time = time_now;
	mchk_data[cpunum].mcode = mcf->mcode;
	mchk_data[cpunum].vaddr = mcf->vaddr;
	mchk_data[cpunum].viba = mcf->viba;
	mchk_data[cpunum].iccs_sisr = mcf->iccs_sisr;
	mchk_data[cpunum].istate = mcf->istate;
	mchk_data[cpunum].sc = mcf->sc;
	mchk_data[cpunum].pc = mcf->pc;
	mchk_data[cpunum].psl = mcf->psl;

	/* Clear RCSR and XBE error bits if XRP, 
         * clear XBE0 and XBEER0 error bits if
         * XMP.
 	 */
	clear_xrperr();

	if(cpu_sub_subtype == MARIAH_VARIANT)
		error_enable_cache();
	else ka6400_enable_cache();	/* Reenable cache and return */

	return(0);	
}


/* initialization code that the slave processor must run
   before starting up */

ka6400initslave() 
{
if(cpu_sub_subtype != MARIAH_VARIANT)
	ka6400_clear_xbe();
else {
	ka6500_clear_xbe0();
	ka6500_clear_xbeer0();
}
ka6400_init_cache();
ka6400_enable_cache();

/* make sure the vector processor is disabled.  
 * take advantage of the fact that (according to section 13.2.3 of the VAX
 * Architecture Reference Manual) neither a mtpr(VPSR) nor a mfpr(VPSR)
 * will cause a reserved operand fault on a vector absent VAX.
 */

mtpr (VPSR, 0);
}


/*
 * Initial configuration routine to bring up the system.
 * It is used for both ka6400 (xrp) and ka6500 (xmp).
 */

extern	int	max_vec_procs;

ka6400conf()
{
	extern struct xmi_reg xmi_start[];
	register char *nxv;
	register int nxp;
	register int i;
	register int xrp_node;
	register struct xmidata *xmidata;
	register char *start;
	struct xrp_reg *nxvirt;
	int cpu_subtype;
	union cpusid cpusid;

	cpusid.cpusid = mfpr(SID);

	/* allocate up 1 xmi structure */
	KM_ALLOC(xmidata,struct xmidata *,sizeof(struct xmidata ),KM_DEVBUF,
			KM_NOW_CL_CO_CA);

	head_xmidata = xmidata;
	xmidata->next = 0;
	xmidata->xminum = 0;

	/*
	 * make processor's XMI private space accessible
         * Note: rssc (xrp) and mssc (xmp) are equivalent.
	 * Usage:
 	 * 	rssc->xxx
	 */
	nxaccess(RSSCADDR,RSSCmap,RSSCSIZE);

	/* Make processor's XMI MDA private space accessible.
 	 * Note: mda (xmp only)
	 * Usafe:
	 *	mda->xxx
	 */
	if(cpu_sub_subtype == MARIAH_VARIANT)
	        nxaccess(MDAADDR,MDAmap,MDASIZE);	
	
	/* 
	 * fill in table of IP interrupt addresses.  Addresses 
	 * are of the form:
	 *
	 *	2101nnnn  where nnnn is the decoded XMI node number
	 */

	start = (char *) &calypso_ip_addr;
	for (i=0 ; i< MAX_XMI_NODE ; i++ ) {
		nxp = 0x21010000 + (1<<i);
		if (i < 9)
			nxv =  start + (1<<i);
		else {
			nxv = start + ((i-8)*NBPG);
		}
		calypso_ip[i]=nxv;
		nxaccess(nxp,&Sysmap[btop((int)(nxv) & ~VA_SYS)],512);
	}

	/*
	 * Get node ID of the processor.  (The node ID is stored
	 * in bits<3:0> of the RSSC IPORT register.)
	 */
	xrp_node = rssc->s_iport & 0xf;
	cprintf("Node ID = %x\n",xrp_node);


	xmidata->xmiintr_dst = 1<<xrp_node;
	xmidata->xmiphys = (struct xmi_reg *) XMI_START_PHYS;
	xmidata->xmivec_page = &scb.scb_stray;

	cpu_avail  = cca_setup() + 1;

	/* Allocate memory to store machine check information */
	KM_ALLOC(mchk_data, struct mchk_data *, 16 * 
		sizeof(struct mchk_data), KM_MBUF, KM_CLEAR | KM_CONTIG);
	/* Allocate memory to store snapshot of error registers */
	KM_ALLOC(error_data, struct error_data *, 16 * 
		sizeof(struct error_data), KM_MBUF, KM_CLEAR | KM_CONTIG);

	ka6400_init_cache();		/* Initialise the caches */
	ka6400_enable_cache();		/* Enable them.		 */

	if (cpu_sub_subtype != MARIAH_VARIANT)
	printf("VAX64%d0, ucode rev %d, ucode opts %d, system type 0x%8x.\n",
		cpu_avail, cpusid.cpuRIGEL.cp_urev, cpusid.cpuRIGEL.cp_uopt,
		cpu_systype);
	else
	printf("VAX6000-5%d0, ucode rev %d, ucode opts %d, system type 0x%8x.\n",
		cpu_avail, cpusid.cpuRIGEL.cp_urev, cpusid.cpuRIGEL.cp_uopt,
		cpu_systype);

	if (fl_ok) {
		printf("FPA is enabled\n");
	}
	else	{
		printf("FPA is disabled  or not present\n");
	}

	/*
	 * Map the node space of the XRP (XMP) so that the processor
	 * can get to its node space CSRs.
	 */
	xmidata = get_xmi(0);
	xmidata->xmivirt = xmi_start;
	nxp = ka6400nexaddr(0,xrp_node);
	nxvirt = (struct xrp_reg *)xmidata->xmivirt + 
			(rssc->s_iport & 0xf);
	nxaccess(nxp,&Sysmap[btop((int)(nxvirt) & ~VA_SYS)],1024);

	spl0();  			/* Ready for interrupts */
	
	xmiconf(0);		/* Config all XMI, BI, ..etc devices*/

	/* clear warm and cold boot flags */
	ccabase.cca_hflag &= ~(CCA_V_BOOTIP|CCA_V_WARMIP);

	vpmask = vpfree = vptotal = 0;
	if ((cpu == VAX_6400) && (ccabase.cca_b_revision > 3)) {
	    if (max_vec_procs > 0) {
		/*
		 * set up vector processing system wide variables.  record
		 * which scalar cpu's have an attached vector processor
		 * note:  the console may be used to disable the vector 
		 * processor.  In this case the VP would show up as present, 
		 * but not enabled.  Therefore, for our purposes here, we 
		 * need to look at the enabled field.
		 */

		vpmask = ccabase.cca_rev_dependent.cca_rev4.cca_vec_enabled1;
		vpfree = 1;
		for (i=0; i<=(sizeof(int)*8); i++) {
			if (vpfree & vpmask) {
			    printf ("xrv attached to cpu #%d is enabled\n", i);
			    vptotal++;
			}
			vpfree <<=1;
		}
		vpfree = vpmask;
		num_vec_procs = 0;

		/*
		 * vector processors are attached to scalar cpu's only after 
		 * cca rev #3
		 */
		KM_ALLOC (( CURRENT_CPUDATA-> cpu_vpdata), struct vpdata *, 
		  sizeof(struct vpdata), KM_VECTOR , KM_CLEAR | KM_CONTIG);

		if (vpmask & CURRENT_CPUDATA->cpu_mask ) {
			/* enable VP on boot processor
			 * note:  This routine always runs on the boot cpu.  
			 *	  vp_reset must be called while running on 
			 *	  the cpu whose vp is being reset.  
			 *	  Therefore, the call to vp_reset for a 
			 *	  secondary cpu is done in _slavestart in 
			 *	  locore.s
			 */
			vp_reset (CURRENT_CPUDATA->cpu_num);
		} else { 
			/* disable VP on boot processor */
			mtpr (ACCS, mfpr(ACCS) & ~1);
		}
	    } else {
		/* disable VP on boot processor */
		mtpr (ACCS, mfpr(ACCS) & ~1);
	    }
	}
	/* make sure the vector processor is disabled.  
	 * take advantage of the fact that (according to section 13.2.3
	 * of the VAX Architecture Reference Manual) neither a mtpr(VPSR)
	 * nor a mfpr(VPSR) will cause a reserved operand fault on a
	 * vector absent VAX.
	 */

	mtpr (VPSR, 0);

	return(0);
}

/*
 * Make the processor's XMI private space accessible by
 * initialising the PTEs allocated in spt.s to point to
 * the physical XMI private space addresses.
 */
ka6400mapcsr() {
	nxaccess(RSSCADDR,RSSCmap,RSSCSIZE);
}

/*
 * ka6400harderr  ---  Hard errors are reported through SCB vector
 * 0x60, at IPL 0x1d.  This routine runs on the interrupt stack.
 * These errors include the following:
 * . XBER-notfied errors
 * . RCSR<WDPE> (DAL write data parity error)
 * . Vector unit errors
 *
 * None of the hard errors can be retried.  We simply log the
 * error and panic.
 *
 * NOTE: Due to a bug in 2nd pass XRP, extraneous hard error interrupt
 * must be checked for and ignored.
 * Also, at boot time, we  may get an extraneous error interrupt.  
 * Since at boot time we haven't set up our XMI data structures yet, 
 * we can't access the XMI registers, so we can't tell if the error was
 * real or extraneous.  We can either panic (and therefore 
 * never boot), or we can ignore the error and pretend nothing has happened.
 * Here we ignore the error until we are fully booted.
 */
ka6400harderr()
{
register int cpunum;
register struct el_rec *elrp;
register struct el_xrp *ptr;
register xrp_node;

register struct xrp_reg *nxv;
register struct xmidata *xmidata;
int	rcsr, xber;
extern int cold;	/* Cold start flag */

if (cpu_sub_subtype == MARIAH_VARIANT)
	return(ka6500harderr());
else {
if (cold) {		/* booting, may not be able to access XMI yet */
	if (head_xmidata) {	/* We can access XMI */
		clear_xrperr();
	}
	return(0);
}
/*
 * Check if this is an extraneous error, if it is, ignore and return.
 */

xmidata = get_xmi(0);	/* get pointer to XMI data structure */
nxv = (struct xrp_reg *)xmidata->xmivirt + (rssc->s_iport & 0xf);
rcsr = (int)nxv->xrp_rcsr;	/* read RCSR */
xber = (int)nxv->xrp_xbe;	/* read XBER */

if (((rcsr & 0xf8000000) == 0) && ((xber & 0x8ffff000) == 0)) {
	/* No error bits set in RCSR or in XBER */
	return(0);
}
if ((rcsr & 0xf8000010) == 0x10) {
	/* RCSR<4> set, no other error bits set */
	if ((xber & 0x0377a000) == 0) {
		/* No error other than bits 27,23,19,14 */
		clear_xrperr();
		return(0);
	}
}
if ((rcsr & 0xf8000010) == 0x08000010) {
	/* RCSR<4> and RCSR<27> set, no other error bits set */
	if ((xber & 0x03708000) == 0) {
		/* No error other than bits 27,23,19,18,17,16,14,13 */
		clear_xrperr();
		return(0);
	}
}
if ((xber & 0x0000000f) == 0x00000009) {
	/* If RER, RSE, or TTO occurred during IDENT, retry transaction */
	if ((xber & (XMI_RER | XMI_RSE | XMI_TTO)) != 0) {
		/* RSE, RER, or TTO are set */ 
		clear_xrperr();
		return(0);
	}
}
	


cpunum=CURRENT_CPUDATA->cpu_num;

/* Test for Vector Errors.  If vector error occured in kernel mode,
 * then we must panic.  If the error occured in user mode, then disable
 * the vector unit and kill the user process.  In any case, make up an
 * error packet.
 */

#define	VINTSR_HARD_ERROR	(VINTSR_VECTOR_UNIT_HERR | \
				 VINTSR_VECTL_VIB_HERR   | \
				 VINTSR_CCHIP_VIB_HERR   | \
				 VINTSR_BUS_TIMEOUT        )

/* if (error_data[cpunum].s_vintsr & VINTSR_HARD_ERROR) {
 * 	printf ("ka6400harderr(): error packet to be made up here\n");
 * 	if (CURRENT_CPUDATA->cpu_vpdata->vpd_in_kernel != VPD_IN_KERNEL) {
 * 		vp_remove();
 * 		return (0);
 * 	}
 * }
 */

snapshot_registers(cpunum);	/* Read in all relevent registers */
ka6400_disable_cache();		/* Disable caches */
xrp_node = rssc->s_iport & 0xf;	/* Node ID of processor		  */

elrp = ealloc(sizeof(struct el_xrp), EL_PRISEVERE);
if (elrp) {
	LSUBID(elrp,ELCT_6400_INT60,EL_UNDEF,EL_UNDEF,EL_UNDEF,
		EL_UNDEF,EL_UNDEF);
	ptr = &elrp->el_body.el_xrp;
	ptr->s_rcsr = error_data[cpunum].s_rcsr;
	ptr->s_xber = error_data[cpunum].s_xber;
	ptr->s_xfadr = error_data[cpunum].s_xfadr;
	ptr->s_sscbtr = error_data[cpunum].s_sscbtr;
	ptr->s_bcctl = error_data[cpunum].s_bcctl;
	ptr->s_bcsts = error_data[cpunum].s_bcsts;
	ptr->s_bcerr = error_data[cpunum].s_bcerr;
	ptr->s_pcsts = error_data[cpunum].s_pcsts;
	ptr->s_pcerr = error_data[cpunum].s_pcerr;
	ptr->s_vintsr = error_data[cpunum].s_vintsr;
	EVALID(elrp);
	cprintf("Hard error logged in error log buffer.\n");
	}
else	{
	cprintf("Can't log hard error. (no buffer)\n");
	}

/*
 * We are crashing, print something on the console
 */
cprintf("Fatal hard error detected by processor at node %d\n",
	xrp_node);
cprintf("rcsr   = %x\n",error_data[cpunum].s_rcsr);
cprintf("xber   = %x\n",error_data[cpunum].s_xber);
cprintf("xfadr  = %x\n",error_data[cpunum].s_xfadr);
cprintf("sscbtr = %x\n",error_data[cpunum].s_sscbtr);
cprintf("bcctl  = %x\n",error_data[cpunum].s_bcctl);
cprintf("bcsts  = %x\n",error_data[cpunum].s_bcsts);
cprintf("bcerr  = %x\n",error_data[cpunum].s_bcerr);
cprintf("pcsts  = %x\n",error_data[cpunum].s_pcsts);
cprintf("pcerr  = %x\n",error_data[cpunum].s_pcerr);
cprintf("vintsr  = %x\n",error_data[cpunum].s_vintsr);

/*
 * Go find any pending memory (XMA) errors and log them in the error log.
 */
rxma_check_errors(EL_PRISEVERE);

panic("Hard error");
} /* END IF Mariah ELSE Rigel */
/*NOTREACHED*/
}

/*
 *  ka6400softerr() --- "Soft" errors are reported through SCB vector
 *  0x54, at IPL 0x1a. This routine runs on the interrupt stack.
 *  These errors include the following:
 *	. P-cache errors
 *	. REXMI-detected errors
 *	. C-chip tag store parity errors
 *	. C-chip detected errors
 *	. Vector unit errors
 *  These errors do not affect instruction execution.  
 *
 *  Note: This routine will be entered whether cpu type is xrp or xmp.
 *        Routine will check for cpu type, and if xmp, will dispatch to
 *        xmp-specific soft error handler.
 *
 */
ka6400softerr()
{
register int cpunum;
register struct el_rec *elrp;
register struct el_xrp *ptr;
register xrp_node;

if(cpu_sub_subtype == MARIAH_VARIANT)
	return(ka6500softerr());
else {
cpunum = CURRENT_CPUDATA->cpu_num;
snapshot_registers(cpunum);	/* Read in all relevent registers */
xrp_node = rssc->s_iport & 0xf;	/* Node ID of processor		  */

ka6400_disable_cache();		/* Disable caches */

#define	VINTSR_SOFT_ERROR	(VINTSR_VECTOR_UNIT_SERR | \
				 VINTSR_VECTL_VIB_SERR   | \
				 VINTSR_CCHIP_VIB_SERR     )

/*
 * check to see if there really is an error.  note: don't check for a CRD 
 * (Corrected Read error) here, because it does not need an error packet.
 */
if ( error_data[cpunum].s_pcsts & PCSTS_INTERRUPT ||
     error_data[cpunum].s_rcsr  & RCSR_CFE        ||
     error_data[cpunum].s_xber  & XMI_PE          ||
     error_data[cpunum].s_xber  & XMI_TE          ||
     error_data[cpunum].s_xber  & XMI_CC          ||
     error_data[cpunum].s_bcsts & BCSTS_STATUS_LOCK) {
	elrp = ealloc(sizeof(struct el_xrp), EL_PRILOW);
	if (elrp) {
		LSUBID(elrp,ELCT_6400_INT54,EL_UNDEF,EL_UNDEF,EL_UNDEF,
			EL_UNDEF,EL_UNDEF);
		ptr = &elrp->el_body.el_xrp;
		ptr->s_rcsr = error_data[cpunum].s_rcsr;
		ptr->s_xber = error_data[cpunum].s_xber;
		ptr->s_xfadr = error_data[cpunum].s_xfadr;
		ptr->s_sscbtr = error_data[cpunum].s_sscbtr;
		ptr->s_bcctl = error_data[cpunum].s_bcctl;
		ptr->s_bcsts = error_data[cpunum].s_bcsts;
		ptr->s_bcerr = error_data[cpunum].s_bcerr;
		ptr->s_pcsts = error_data[cpunum].s_pcsts;
		ptr->s_pcerr = error_data[cpunum].s_pcerr;
		ptr->s_vintsr = error_data[cpunum].s_vintsr;
		EVALID(elrp);
	}
}

/*
 * Go find any memory (XMA) CRD errors and log them in the error log.
 * At the same time, disable CRD errors on the memory nodes.
 * note: this will be re-enabled at regular intervals by ka6400memenable ().
 * It is disabled to keep the number of corrected errors reported down to 
 * a dull roar.
 */
if (error_data[cpunum].s_xber & XMI_CRD) {
	rxma_check_crd(EL_PRILOW);
	recover_mem_error();	/* Do a full memory error recovery */
}
ka6400_init_cache();
ka6400_enable_cache();
return(0);
} /* END IF Mariah ELSE Rigel */
}

/*
 * Routine to check each node in the XMI to see if it is a
 * memory node (device type register indicating an XMA).  If
 * a node is an XMA, check for all error bits.  If any of
 * the error bits is set, call rlog_mem_error to log the errors
 * in the error log.
 */
rxma_check_errors(priority)
int priority;
{
	register struct xmi_reg *nxv;
	register struct xma_reg *xma;
	register struct xmidata *xmidata;
	register int xminode;
	
	xmidata = (struct xmidata *)get_xmi(0);
	nxv = xmidata->xmivirt;
	for(xminode = 0; xminode < MAX_XMI_NODE; xminode++,nxv++) {
	  if ((xmidata->xminodes_alive & (1<<xminode)) &&
	      ((short) (nxv->xmi_dtype) == XMI_XMA)) {
	    /* The node is a memory node */
	    xma = (struct xma_reg *) nxv;
	    if ((nxv->xmi_dtype & XMA2_MASK) == XMI_XMA) {
	      if ((xma->xma_xbe & XMI_ES)  ||
		  (xma->xma_mctl1 & (XMA_CTL1_LOCK_ERR|
		   XMA_CTL1_UNLOCK_ERR|XMA_CTL1_RDS_WRITE)) ||
		  (xma->xma_mecer & (XMA_ECC_RDS_ERROR))) {
		rlog_mem_error(xma,priority);
	      }
	    }
	    else /* Must be XMA2 */
	      if (xma->xma_xbe & XMI_ES)
		rlog_mem_error(xma,priority);
	  }
	}
}

/*
 * Routine to check each node in the XMI to see if it is a
 * memory node (device type register indicating an XMA).  If
 * a node is an XMA, check for corrected read data (CRD).  If 
 * the error bit is set, call rlog_mem_error to log the errors
 * in the error log.
 *
 * Disable CRD on the board
 */
rxma_check_crd(priority)
int priority;
{
	register struct xmi_reg *nxv;
	register struct xma_reg *xma;
	register struct xmidata *xmidata;
	register int xminode;
	
	xmidata = (struct xmidata *)get_xmi(0);
	nxv = xmidata->xmivirt;
	for(xminode = 0; xminode < MAX_XMI_NODE; xminode++,nxv++) {
		if ((xmidata->xminodes_alive & (1<<xminode)) &&
		   ((short) (nxv->xmi_dtype) == XMI_XMA)) {
			/* The node is a memory node */
		    	xma = (struct xma_reg *) nxv;
		    	if ((xma->xma_mecer&XMA_ECC_CRD_ERROR) &&
			    ((xma->xma_mctl1&XMA_CTL1_CRD_DISABLE)==0)) {
				/* CRD error, log it */
				rlog_mem_error(xma,priority);
				/* 
				 * clear CRD request and disable 
				 * further CRDs.
				 */
				xma->xma_mecer = XMA_ECC_CRD_ERROR;
				xma->xma_mctl1 |= XMA_CTL1_CRD_DISABLE;
			}
		}	
	}
}

/* 
 * log XMA memory error.  If priority is not LOW then print to 
 * console.
 */
rlog_mem_error(xma,priority)
int priority;
register struct xma_reg *xma;
{
	register struct el_xma *xma_pkg;
	register struct el_rec *elrp;
	
	/* If xma2, go to its handler */
	if((xma->xma_type & XMA2_MASK) == XMI_XMA2)
	        xma2_mem_error(xma,priority);
	else {
	/* log the XMA error */
	elrp = ealloc(sizeof(struct el_xma),priority);
	if (elrp) {
		LSUBID(elrp,ELCT_MEM,EL_UNDEF,ELMCNTR_6200,EL_UNDEF,EL_UNDEF,EL_UNDEF);
		xma_pkg = &elrp->el_body.el_xma;
		xma_pkg->xma_node = ((svtophy(xma))>>19)&0xf;
		xma_pkg->xma_dtype = xma->xma_type;
		xma_pkg->xma_xbe = xma->xma_xbe;
		xma_pkg->xma_seadr = xma->xma_seadr;
		xma_pkg->xma_mctl1 = xma->xma_mctl1;
		xma_pkg->xma_mecer = xma->xma_mecer;
		xma_pkg->xma_mecea = xma->xma_mecea;
		xma_pkg->xma_mctl2 = xma->xma_mctl2;
     		EVALID(elrp);
	}
	/* print to console if HIGH priority */
	if (priority < EL_PRILOW) {
		cprintf("Memory error \n");
		cprintf("xma_phys = %x\n",svtophy(xma));
		cprintf("xma_type = %x\n",xma->xma_type);
		cprintf("xma_xbe = %x\n",xma->xma_xbe);
		cprintf("xma_seadr = %x\n",xma->xma_seadr);
		cprintf("xma_mctl1 = %x\n",xma->xma_mctl1);
		cprintf("xma_mecer = %x\n",xma->xma_mecer);
		cprintf("xma_mecea = %x\n",xma->xma_mecea);
		cprintf("xma_mctl2 = %x\n",xma->xma_mctl2);
				
	}
      } /* END BIG IF-ELSE */
}

/* 
 * log XMA2 memory error.  If priority is not LOW then print to 
 * console.
 */
xma2_mem_error(xma2,priority)
int priority;
register struct xma_reg *xma2;
{
	register struct el_xma2 *xma_pkg;
	register struct el_rec *elrp;
	
	/* log the XMA2 error */
	elrp = ealloc(sizeof(struct el_xma2),priority);
	if (elrp) {
                LSUBID(elrp,ELCT_MEM,EL_UNDEF,ELMCNTR_XMA2,EL_UNDEF,EL_UNDEF,EL_UNDEF);
		xma_pkg = &elrp->el_body.el_xma2;
		xma_pkg->xma_node = ((svtophy(xma2))>>19)&0xf;
		xma_pkg->xma_dtype = xma2->xma_type;
		xma_pkg->xma_xbe = xma2->xma_xbe;
		xma_pkg->xma_seadr = xma2->xma_seadr;
		xma_pkg->xma_mctl1 = xma2->xma_mctl1;
		xma_pkg->xma_mecer = xma2->xma_mecer;
		xma_pkg->xma_mecea = xma2->xma_mecea;
		xma_pkg->xma_mctl2 = xma2->xma_mctl2;
		xma_pkg->xma_becer = xma2->xma_becer;
		xma_pkg->xma_becea = xma2->xma_becea;
		xma_pkg->xma_stadr = xma2->xma_stadr;
		xma_pkg->xma_enadr = xma2->xma_enadr;
		xma_pkg->xma_intlv = xma2->xma_intlv;
		xma_pkg->xma_mctl3 = xma2->xma_mctl3;
		xma_pkg->xma_mctl4 = xma2->xma_mctl4;
		xma_pkg->xma_bsctl = xma2->xma_bsctl;
		xma_pkg->xma_bsadr = xma2->xma_bsadr;
		xma_pkg->xma_eectl = xma2->xma_eectl;
		xma_pkg->xma_tmoer = xma2->xma_tmoer;
     		EVALID(elrp);
	}
	/* print to console if HIGH priority */
	if (priority < EL_PRILOW) {
		cprintf("Memory error \n");
		cprintf("xma_phys = %x\n",svtophy(xma2));
		cprintf("xma_type = %x\n",xma2->xma_type);
		cprintf("xma_xbe = %x\n",xma2->xma_xbe);
		cprintf("xma_seadr = %x\n",xma2->xma_seadr);
		cprintf("xma_mctl1 = %x\n",xma2->xma_mctl1);
		cprintf("xma_mecer = %x\n",xma2->xma_mecer);
		cprintf("xma_mecea = %x\n",xma2->xma_mecea);
		cprintf("xma_mctl2 = %x\n",xma2->xma_mctl2);
		cprintf("xma_becer = %x\n", xma2->xma_becer);
		cprintf("xma_becea = %x\n", xma2->xma_becea);
		cprintf("xma_stadr = %x\n", xma2->xma_stadr);
		cprintf("xma_enadr = %x\n", xma2->xma_enadr);
		cprintf("xma_intlv = %x\n", xma2->xma_intlv);
		cprintf("xma_mctl3 = %x\n", xma2->xma_mctl3);
		cprintf("xma_mctl4 = %x\n", xma2->xma_mctl4);
		cprintf("xma_bsctl = %x\n", xma2->xma_bsctl);
		cprintf("xma_bsadr = %x\n", xma2->xma_bsadr);
		cprintf("xma_eectl = %x\n", xma2->xma_eectl);
		cprintf("xma_tmoer = %x\n", xma2->xma_tmoer);
	}
}

/*
 * Memenable enables the memory controller corrected data reporting.
 * This runs at regular intervals, turning on the interrupt.
 * The interrupt is turned off, per memory controller, when error
 * reporting occurs.  Thus we report at most once per memintvl.
 */

ka6400memenable ()
{
	register struct xmi_reg *nxv;
	register struct xma_reg *xma;
	register struct xmidata *xmidata;
	register int xminode;
	
	xmidata = (struct xmidata *)get_xmi(0);
	nxv = xmidata->xmivirt;
	for(xminode = 0; xminode < MAX_XMI_NODE; xminode++,nxv++) {
		if ((xmidata->xminodes_alive & (1<<xminode)) &&
		   ((short) (nxv->xmi_dtype) == XMI_XMA)) {
			/* The node is a memory node, enable CRD */
		    	xma = (struct xma_reg *) nxv;
			xma->xma_mctl1 &= ~XMA_CTL1_CRD_DISABLE;
		}	
	}

}

/*
 * this routine sets the cache to the state passed.  enabled/disabled
 */

ka6400setcache(state)
int state;

{
	if (state) {
		/* enable the cache */
		ka6400_enable_cache(); 
		return(0); 
        }
	return(-1);
}



ka6400cachenbl()
{
	cache_state = 0x1;
	return(0);
}

ka6400tocons(c)
register int c;
{
	register int timeo;

	timeo = 100000;
	while ((mfpr (TXCS) & TXCS_RDY) == 0) {
		if (timeo-- <= 0) {
			return(0);
		}
	}
	mtpr (TXDB, c);
	return(0);
}

/*
 * Routine to determine if memory of given virtual address and given
 * length in bytes can be accessed without causing a machine check.
 * It calls "bbadaddr" in locore.s check.  However, for
 * some addresses on the UNIBUS, the access does not cause a machine
 * check, but the error is reported to the BIIC of the BUA,
 * so bbadaddr can't detect it.  In this case we clear all BIIC
 * error bits, access the address, and check for the setting of any 
 * BIIC error.
 */
int ka6400badaddr(addr,len)
caddr_t addr;
int len;
{
	register int foo,s,i;	
	register struct bi_nodespace *biptr;
	
#ifdef lint
	len=len;
#endif lint

	s=spl7();

	for (i=0; i < nNVAXBI ; i++) {
		if (biptr = bidata[i].cpu_biic_addr) {
			biptr->biic.biic_err = biptr->biic.biic_err;
			foo = biptr->biic.biic_gpr0;
		}
	}

	foo = bbadaddr(addr,len);


	for (i=0; i < nNVAXBI ; i++) {
		if (biptr = bidata[i].cpu_biic_addr) {
			if ((biptr->biic.biic_err 
				& ~BIERR_UPEN)!=0) foo=1;
			biptr->biic.biic_err = biptr->biic.biic_err;
		}
	}

	splx(s);
	return(foo);
}

/* reboot VAX64xx/VAX65xx machine */
ka6400reboot() {

	struct xmidata *xmidata;
	struct xrp_reg *xrp_node;


	/* set O/S reboot flag so reboot happens */
	ccabase.cca_hflag |=CCA_V_REBOOT;
	
	/* get pointer to xcp node to reboot*/
	xmidata = get_xmi(0);
	xrp_node =(struct xrp_reg *)xmidata->xmivirt + (rssc->s_iport & 0xf);

	/* hit the halt bit... so long */
	xrp_node->xrp_xbe = (xrp_node->xrp_xbe & ~XMI_XBAD) | XMI_NHALT;

	DELAY(10000);	/* give time for the "XMI_NHALT" to work */
	
	/* just in case "XMI_NHALT" doesn't work. We should never reach
	   the halt */
	asm("halt");  
}

/* halt VAX64xx/VAX65xx machine */
ka6400halt() {

	struct xmidata *xmidata;
	struct xrp_reg *xrp_node;

	/* get pointer to xcp node to reboot*/
	xmidata = get_xmi(0);
	xrp_node =(struct xrp_reg *)xmidata->xmivirt + (rssc->s_iport & 0xf);

	/* hit the halt bit... so long */
	xrp_node->xrp_xbe = (xrp_node->xrp_xbe & ~XMI_XBAD) | XMI_NHALT;

	DELAY(10000);	/* give time for the "XMI_NHALT" to work */
	
	/* just in case "XMI_NHALT" doesn't work. We should never reach
	   the halt */
	asm("halt");  
}



xrpinit(nxv,nxp,xminumber,xminode)
char *nxv;
char *nxp;
int xminumber,xminode;
{
#ifdef lint
	nxv=nxv; nxp=nxp;
	xminumber=xminumber;
	xminode = xminode;
#endif lint


}

/*
 * Return the physical address of the node space of
 * an XMI node given the node ID.
 * 
 * Parameters:
 *	xminumber		Ignored
 *	xminode			Node ID (0 to 15)
 */
ka6400nexaddr(xminumber,xminode) 
int xminode,xminumber;
{
	/* Each node space is 512Kb (0x80000) in length */
	return((XMI_START_PHYS+(xminode * 0x80000)));		
}

ka6400umaddr(binumber,binode) 
int binumber,binode;
{

	return(((int) bidata[binumber].biphys)  
		+ 0x400000 + (0x40000 * binode));

}

ka6400udevaddr(binumber,binode) 
int binumber,binode;
{
	return(((int) bidata[binumber].biphys)  
		+ 0x400000 + 0x3e000 + (0x40000 * binode));
}

/*
 * Routine to initialise the primary and the backup caches.
 * The caches need to be initialised after a power up, and
 * after a tag parity error has been detected.
 *
 * Note:  Both primary and backup caches are disabled when
 *	  this routine returns.
 */
ka6400_init_cache()
{
register int	i;
register int	oldipl;

oldipl = spl7();	/* Raise to highest IPL */

/* 
 * Enable refresh, clear errors, disable primary cache
 * should be 0xec, this is workaround bug 1st pass chip bug 
 * 2nd pass can flush the cache and needs not use a loop.  Howvever,
 * we do the same for both types.  This code works for both
 * 1st and 2nd pass machines.
 */

if(cpu_sub_subtype != MARIAH_VARIANT) {
   	mtpr(PCSTS, 0xe8);  /* non-functional Pcache, don't set flush bit */

   	/* Write invalid tags with good parity to the primary cache */
   	for (i=0; i<0x800; i+=8) {
		mtpr(PCIDX, i);
		mtpr(PCTAG, 0x40000000);
   	}

	/* Enable refresh, clear errors, disable backup cache */
	mtpr(BCCTL,0x8);
	mtpr(BCSTS,1);

	/* Write all backup cache tag with valid bit clear and good parity */
	for (i=0; i<20000; i+=0x40) {
		mtpr(BCIDX,i);
		mtpr(BCBTS,0x20000000);
	}

	/* Write all backup PTS tag entries with clear V bit and good parity */
	for (i=0; i<0x800; i+=0x10) {
		mtpr(BCIDX,i);
		mtpr(BCP1TS, 0x20000000);
		mtpr(BCP2TS, 0x20000000);
	}
}

else { /* Mariah support */
   	mtpr(PCSTS, 0xe4);  /* clear errors and turn off primary cache */

   	/* Write invalid tags with good parity to the primary cache */
   	for (i=0; i<0x100; i+=8) {
		mtpr(PCIDX, i);
		mtpr(PCTAG, 0x80000000);
   	}

	/* Clear errors, disable backup cache */
	mtpr(XMP_BCCTL,0x0);
	mtpr(XMP_BCSTS,0x000001f6);

	/* Write all backup cache tag with valid bit clear and good parity */
	for (i=0; i<0x1000; i+=0x80) {
		mtpr(XMP_BCIDX,i);
		mtpr(XMP_BCBTS,0x00000300);
	}
}

splx(oldipl);		/* Restore to old IPL	*/

}


/*
 * Routine to enable the primary and backup caches.
 */
ka6400_enable_cache()
{
	union cpusid cpusid;
	int s;

if(cpu_sub_subtype != MARIAH_VARIANT) {	
	cpusid.cpusid = mfpr(SID);

	/* 
	 * To ensure cache coherency, always enable shadow primary tag in 
	 * the backup cache first before enabling the primary cache.
	 * 
	 * Note: when writing PCSTS, always clear out the trap and
	 * interrupt bits to ensure that the primary cache is enabled
	 * properly.
	 */

	if (cpusid.cpuRIGEL.cp_urev == 1) {
		/* Can't set FLUSH bit in Pass 1 XRP PCSTS */
		s = spl7();
		mtpr(PCSTS,0xe8); /* enable refresh */
		mtpr(BCFBTS,0);	  /* flush backup tag store	*/
		mtpr(BCFPTS,0);	  /* flush shadown primary tag store*/
		mtpr(BCCTL,0xe);  /* enable backup cache, refresh, */
				  /* and shadow primary tag */
		mtpr(BCFBTS,0);	  /* flush backup tag store	*/
		mtpr(PCSTS,0xea); /* enable primary cache, enable refresh */
	}
	else {
		/* fully functional P cache */
		s = spl7();
		mtpr(PCSTS,0xec); /* enable refresh, flush cache before */
				  /* enabling backup Ptag*/
		mtpr(BCFBTS,0);	  /* flush backup tag store	*/
		mtpr(BCFPTS,0);	  /* flush shadown primary tag store*/
		mtpr(BCCTL,0xe);  /* enable backup cache, refresh, and */
				  /* shadow primary tag */
		mtpr(BCFBTS,0);	  /* flush backup tag store	*/
		mtpr(PCSTS,0xee); /* enable primary cache, enable refresh,*/
				  /* flush cache*/
	}
}
else { /* Mariah support */
	s = spl7();
	mtpr(PCSTS,0xe4);     /* flush cache before enabling primary cache*/
	mtpr(XMP_BCCTL,0x2);  /* enable backup cache */
	mtpr(PCSTS,0xe6);     /* enable primary cache, flush cache */
}
	splx(s);
}

/*
 * Routine to enable caches while in error handling code.  This code is
 * for XMP machines only.
 */
error_enable_cache()
{
	register int i;
	register int cpunum;

	cpunum=CURRENT_CPUDATA->cpu_num;
	bc_enable[cpunum] = bc_enable[cpunum] | 0x2 & ~0x5;/*Set bcache flags*/
	pc_enable[cpunum] = pc_enable[cpunum] | 
                            (PCSTS_ENABLE_PTS | PCSTS_FLUSH_CASHE) &
			    ~PCSTS_FORCE_HIT; 
	if (bcache_disable[cpunum])  {
		bc_enable[cpunum] = 0x0; /* Disable bcache */
		if(error_data[cpunum].s_bcctl & 0x2)
		        printf("Backup cache is being disabled on cpu %x\n",cpunum);
	}
	if (pcache_disable[cpunum]) {
		pc_enable[cpunum] &= ~PCSTS_ENABLE_PTS; /* Disable pcache */
		if(error_data[cpunum].s_pcsts & PCSTS_ENABLE_PTS)
		        printf("Primary cache is being disabled on cpu %x\n",cpunum);
	}
	if(error_data[cpunum].s_bcctl & 0x2) { /* deallocate only if enabled */
		for(i=0;i<=4095;i++) {
			mtpr(XMP_BCIDX,i*128);
			mtpr(XMP_BCDET,0x0);
			mtpr(XMP_BCBTS,0x300);
		}
	}		
	mtpr(XMP_BCCTL,bc_enable[cpunum]);
	mtpr(PCSTS,pc_enable[cpunum]);
}

/*
 * Routine to disable the primary and backup caches.
 * This code works for both 1st and 2nd pass machines.  Because
 * we don't set the flush bit here.  We must flush the cache
 * before we enable the cache.
 *
 * NOTE:  For XMP support, cache disabling is part of error handling code.
 */
ka6400_disable_cache()
{
	register int cpunum,i;

	if(cpu_sub_subtype != MARIAH_VARIANT) {	
		mtpr(PCSTS,0xe8);  /* disable P-cache, enable refresh */
		mtpr(BCCTL,0x8);   /* disable B-cache & shadow primary tag, */
				   /* enable refresh */
	}
	else{
		mtpr(PCSTS,0x0);      /* disable P-cache */
		mtpr(XMP_BCCTL,0x6);  /* B-cache ETM */ 
		for(i=0;i<=4095;i++) { /* Writeback dirty data */
			mtpr(XMP_BCIDX,i*128);
			mtpr(XMP_BCDET,0x0);
		}
		mtpr(XMP_BCCTL,0x0);  /* Disable secondary cache */
	}
}

/*
 * Disable cache while in error code.  Note that the pcsts and bcctl
 * registers must have already had their snapshots taken.
 */
error_disable_cache()
{
	register int cpunum;

	cpunum=CURRENT_CPUDATA->cpu_num;
	mtpr(PCSTS,mfpr(PCSTS)&0xffffff18); /* disable P-cache */
	mtpr(XMP_BCCTL,0x6);   /* Put B-cache in ETM */
	pcache_disable[cpunum] = !((error_data[cpunum].s_pcsts & 0x2) & 
			 (error_data[cpunum].s_bcctl & 0x2));
	bcache_disable[cpunum] = !(error_data[cpunum].s_bcctl & 0x2);
}

/*
 * Routine to read a number of C-chip, P-cache, and XMI registers
 * into a per processor data structure (error_data).
 *
 * This routine reads these registers at IPL 31 to get a clear snapshot.
 *
 * Parameter:
 *	cpuindex		CPU index
 */
snapshot_registers(cpuindex)
register int	cpuindex;
{
register int	oldipl;		/* IPL when this routine is entered */
register struct xrp_reg *nxv;	/* Virtual pointer to XMI node */
register struct xmidata *xmidata; 
union cpusid cpusid;

xmidata = get_xmi(0);		/* Get pointer to XMI data structure */

/* Get virtual address of the processor's XMI node space*/
nxv = (struct xrp_reg *)xmidata->xmivirt + (rssc->s_iport & 0xf);

oldipl = spl7();	/* Raise IPL to 31 */

error_data[cpuindex].s_xber =  (long)nxv->xrp_xbe;
error_data[cpuindex].s_xfadr = (long)nxv->xrp_fadr;
error_data[cpuindex].s_sscbtr = rssc->s_sscbtr;
error_data[cpuindex].s_pcerr = mfpr(PCERR);
error_data[cpuindex].s_pcsts = mfpr(PCSTS);

if (cpu_sub_subtype != MARIAH_VARIANT) {
	error_data[cpuindex].s_bcctl = mfpr(BCCTL);
	error_data[cpuindex].s_rcsr = (long)nxv->xrp_rcsr;
	error_data[cpuindex].s_bcsts = mfpr(BCSTS);
	error_data[cpuindex].s_bcerr = mfpr(BCERR);
	/* Pass 1 XRPs does not have VINTSR, and reading it causes machine checks. */
	if (cpusid.cpuRIGEL.cp_urev == 1) {
		error_data[cpuindex].s_vintsr = 0;
	}	
	else {	
		/* if a machine check is caused by reading the VINTSR
		 * on a Rigel, then it must have a pass 1 XRP.  There
		 * should no longer be any such XRPs around, but I didn't
		 * want to forget that the problem once existed, just in
		 * case it crops up again.
		 */
		error_data[cpuindex].s_vintsr = mfpr(VINTSR); 
}
}
else {
	error_data[cpuindex].s_xfaer0 = (long)nxv->xmp_xfaer0;
	error_data[cpuindex].s_xbeer0 = (long)nxv->xmp_xbeer0;
	error_data[cpuindex].s_wfadr0 = (long)nxv->xmp_wfadr0;
	error_data[cpuindex].s_wfadr1 = (long)nxv->xmp_wfadr1;
	error_data[cpuindex].s_fdal0  = (long)mda->fdal0;
	error_data[cpuindex].s_fdal1  = (long)mda->fdal1;
	error_data[cpuindex].s_fdal2  = (long)mda->fdal2;
	error_data[cpuindex].s_fdal3  = (long)mda->fdal3; 
	error_data[cpuindex].s_bcctl  = mfpr(XMP_BCCTL);
	error_data[cpuindex].s_bcsts  = mfpr(XMP_BCSTS);
	error_data[cpuindex].s_bcerr  = mfpr(XMP_BCERA);
	error_data[cpuindex].s_bcert  = mfpr(XMP_BCERT);
	error_data[cpuindex].s_vintsr = mfpr(VINTSR); 
}

splx(oldipl);		/* Restore old IPL */
}


/*
 * Routine to clear all the write to clear bits of the XMI bus error
 * register, and to deassert the XMI BAD signal on the backplane.
 */
ka6400_clear_xbe()
{
register struct xmidata *xmidata;
register struct xrp_reg *nxv;

/* get pointer to the xrp node */
xmidata = get_xmi(0);

nxv =(struct xrp_reg *)xmidata->xmivirt + (rssc->s_iport & 0xf);

nxv->xrp_xbe = nxv->xrp_xbe & ~XMI_XBAD;
}

/*
 * Routine to recover from cache or memory system errors.
 * Error handling consists mostly of writing the original register
 * contents back into the XRP registers. This restores the "M" bits
 * to their original state, and clears the "WC" error bits.
 *
 * The strategy of memory recovery is to start from the
 * most distant component and work towards the processor(s).
 * Thus, XMI errors are processed first, followed by RSSC errors,
 * C-chip errors, and finally, P-cache errors.
 *
 * The following assumptions are made when this routine
 * is entered:
 *	1. The routine "snapshot_registers" has been called
 *	   recently to capture the values of various registers.
 *	2. Both the primary and the backup caches are disabled.
 *
 * The caches are not enabled when this routine returns.
 */
recover_mem_error()
{
register struct xrp_reg *nxv;	/* Virtual pointer to XMI node */
register int cpunum;
register int i;
register struct xmidata *xmidata;

if (cpu_sub_subtype == MARIAH_VARIANT) 
	xmp_recover_mem_error();
else {
xmidata = get_xmi(0);		/* Get pointer to XMI data structure */
/* get virtual addr of processor's XMI node space */
nxv = (struct xrp_reg *)xmidata->xmivirt + (rssc->s_iport & 0xf); 
cpunum = CURRENT_CPUDATA->cpu_num;		/* Get CPU number	*/

/*
 * Clear XMI errors, note that error bits in RCSR must be cleared
 * before those in XBER for the hardware to work properly.
 * Both registers are write one to clear.
 */
nxv->xrp_rcsr = error_data[cpunum].s_rcsr;
nxv->xrp_xbe = error_data[cpunum].s_xber;

/*
 * Clear RSSC bus timeout errors
 */
rssc->s_sscbtr = error_data[cpunum].s_sscbtr;

/*
 * Check for C-chip backup tag store parity error.
 */
if (error_data[cpunum].s_bcsts & 0x2) { /* if BTS_PERR bit set */
	/* backup tag store parity error */
	mtpr(BCIDX,error_data[cpunum].s_bcerr); /* use err addr for tag */
	mtpr(BCBTS,0x20000000);		/* good parity, invalid tag */
}

/*
 * Check for C-chip primary tag store parity error.
 */
if (error_data[cpunum].s_bcsts & 0x4) { /* if P1TS_PERR bit set */
	mtpr(BCIDX,error_data[cpunum].s_bcerr); /* use err addr for tag */
	mtpr(BCP1TS,0x20000000);	/* good parity, invalid tag */
}
if (error_data[cpunum].s_bcsts & 0x8) { /* if P2TS_PERR bit set */
	mtpr(BCIDX,error_data[cpunum].s_bcerr); /* use err addr for tag */
	mtpr(BCP2TS,0x20000000);	/* good parity, invalid tag */
}

/* 
 * Clear C-chip error bits.
 */
mtpr(BCSTS,1);

/*
 * Check P-cache tag parity errors, which require rewritting the entire
 * tag store.
 */
if (error_data[cpunum].s_pcsts & 0x100) { /* if TAG_PARITY_ERROR */
	/* Write invalid tags with good parity to the primary cache */
	for (i=0; i<0x800; i+=8) {
		mtpr(PCIDX, i);
		mtpr(PCTAG, 0x40000000);
	}
}

/*
 * Clear P-cache errors.
 */
mtpr(PCSTS,error_data[cpunum].s_pcsts & 0xec);

} /* END IF Mariah ELSE Rigel */
return(1);
}

/*
 * Routine to clear the error bits in the RCSR (Rigel), XBER, XBEER (Mariah),
 * primary cache, backup cache, and ssc.
 */
clear_xrperr()
{
register struct xrp_reg *nxv;
register struct xmidata *xmidata;
int s;

s = spl7();
	xmidata = get_xmi(0);	/* get pointer to XMI data structure */
	nxv = (struct xrp_reg *)xmidata->xmivirt + (rssc->s_iport & 0xf);

	snapshot_registers(CURRENT_CPUDATA->cpu_num);

        if (cpu_sub_subtype == MARIAH_VARIANT) {
	     error_disable_cache();
	     recover_mem_error();
             nxv->xrp_xbe = nxv->xrp_xbe & ~XMI_XBAD; /* Clear XBAD bit */
	     error_enable_cache();
	}
        else {
	     ka6400_disable_cache();
	     recover_mem_error();	
             nxv->xrp_xbe = nxv->xrp_xbe & ~XMI_XBAD; /* Clear XBAD bit */
	     ka6400_init_cache();
             ka6400_enable_cache();
	}
splx(s);

}

/*
 * ka6500harderr  ---  Hard errors are reported through SCB vector
 * 0x60, at IPL 0x1d.  This routine runs on the interrupt stack.
 * These errors include the following:
 *
 * . XBE0, XBEER0 -notified errors
 * . BCSTS - notified errors
 * . Vector unit errors
 *
 * This routine is called from the ka6400harderr routine.  If that routine 
 * determines that the system is a MARIAH, control is transferred here.
 *
 * NOTE: At boot time, we  may get an extraneous error interrupt.  
 * Since at boot time we haven't set up our XMI data structures yet, 
 * we can't access the XMI registers, so we can't tell if the error was
 * real or extraneous.  We can either panic (and therefore 
 * never boot), or we can ignore the error and pretend nothing has happened.
 * Here we ignore the error until we are fully booted.
 */
ka6500harderr()
{
register int cpunum;
register struct el_rec *elrp;
register struct el_xmp *ptr;
register xmp_node;
long time_now;

register struct xrp_reg *nxv;
register struct xmidata *xmidata;
extern int cold;	/* Cold start flag */

if (cold) {		/* booting, may not be able to access XMI yet */
	if (head_xmidata) {	/* We can access XMI */
	     clear_xrperr();
	}
	return(0);
}

time_now = mfpr(TODR);		/* Log the current time	*/
xmidata = get_xmi(0);	        /* get pointer to XMI data structure */
nxv = (struct xrp_reg *)xmidata->xmivirt + (rssc->s_iport & 0xf);
cpunum=CURRENT_CPUDATA->cpu_num;

snapshot_registers(cpunum);     /* get all relevant registers */

/*
 * Disable caches to minimize possibility of generating a machine check
 * while in this handler.
 */

error_disable_cache();		/* Disable caches */
xmp_node = rssc->s_iport & 0xf;	/* Node ID of processor		  */

/*
 * Perform a full memory recovery.  This will clear out errors in XBE0, XBEER0, and
 * will act on TPERR and VDPERR errors if there are any.  If no hard error is 
 * indicated by these registers, it must have been an extraneous hard error.  Such 
 * an extranious error will be considered as inconsistent state and panic'd.
 */

if ((error_data[cpunum].s_bcsts & 0x1b6) || 
    (error_data[cpunum].s_xber & 0x03000000) || 
    (error_data[cpunum].s_xbeer0 & 0x67272004) || 
    (((error_data[cpunum].s_xfaer0 & 0xf0000000) == 0x90000000) && ((error_data[cpunum].s_xber & (XMI_RER | XMI_RSE | XMI_TTO)) != 0)) ||
    (((error_data[cpunum].s_xber & 0x60000) != 0) && ((error_data[cpunum].s_bcsts & 0x40) != 0) && (error_data[cpunum].s_xber & 0x808000) == 0x800000))
        recover_mem_error();
else 
  if (mchk_data[cpunum].mchk_in_progress > 0) {
    return(0);  /* spurious error during machine check */
  }

/*
 * Perform a full vector error recovery procedure for vector errors.
 */

       /* PLACEHOLDER */

/*
 * Log all register and PC/PSL pair for later use here and for later
 * analysis.
 */

elrp = ealloc(sizeof(struct el_xmp), EL_PRISEVERE);
if (elrp) {
	LSUBID(elrp,ELCT_INT60,ELINT_6500,EL_UNDEF,EL_UNDEF,
		EL_UNDEF,EL_UNDEF); 
	ptr = &elrp->el_body.el_xmp;
	ptr->s_xbe0 = error_data[cpunum].s_xber;
	ptr->s_xfadr0 = error_data[cpunum].s_xfadr;
	ptr->s_xfaer0 = error_data[cpunum].s_xfaer0;
	ptr->s_xbeer0 = error_data[cpunum].s_xbeer0;
	ptr->s_wfadr0 = error_data[cpunum].s_wfadr0;
	ptr->s_wfadr1 = error_data[cpunum].s_wfadr1;
	ptr->s_fdal0 = error_data[cpunum].s_fdal0;
	ptr->s_fdal1 = error_data[cpunum].s_fdal1;
	ptr->s_fdal2 = error_data[cpunum].s_fdal2;
	ptr->s_fdal3 = error_data[cpunum].s_fdal3;
	ptr->s_sscbtr = error_data[cpunum].s_sscbtr;
	ptr->s_bcsts = error_data[cpunum].s_bcsts;
	ptr->s_bcera = error_data[cpunum].s_bcerr;
	ptr->s_bcert = error_data[cpunum].s_bcert;
	ptr->s_pcsts = error_data[cpunum].s_pcsts;
	ptr->s_pcerr = error_data[cpunum].s_pcerr;
	ptr->s_vintsr = error_data[cpunum].s_vintsr;
	EVALID(elrp);
	cprintf("Hard error logged in error log buffer.\n");
	}
else	{
	cprintf("Can't log hard error. (no buffer)\n");
	}


if (((error_data[cpunum].s_xfaer0 & 0xf0000000) == 0x90000000) &&  /* IDENT errors are retryable! */
    ((error_data[cpunum].s_xber & (XMI_RER | XMI_RSE | XMI_TTO)) != 0)) {
	error_enable_cache();
	return(0);
}	

/*
 * Print something on the console
 */
cprintf("Fatal hard error detected by processor at node %d\n",
	xmp_node);
cprintf("xbe0    = %x\n",error_data[cpunum].s_xber);
cprintf("xfadr0  = %x\n",error_data[cpunum].s_xfadr);
cprintf("xfaer0  = %x\n",error_data[cpunum].s_xfaer0);
cprintf("xbeer0  = %x\n",error_data[cpunum].s_xbeer0);
cprintf("wfadr0  = %x\n",error_data[cpunum].s_wfadr0);
cprintf("wfadr1  = %x\n",error_data[cpunum].s_wfadr1);
cprintf("fdal0   = %x\n",error_data[cpunum].s_fdal0);
cprintf("fdal1   = %x\n",error_data[cpunum].s_fdal1);
cprintf("fdal2   = %x\n",error_data[cpunum].s_fdal2);
cprintf("fdal3   = %x\n",error_data[cpunum].s_fdal3);
cprintf("sscbtr  = %x\n",error_data[cpunum].s_sscbtr);
cprintf("bcctl   = %x\n",error_data[cpunum].s_bcctl);
cprintf("bcsts   = %x\n",error_data[cpunum].s_bcsts);
cprintf("bcera   = %x\n",error_data[cpunum].s_bcerr);
cprintf("bcert   = %x\n",error_data[cpunum].s_bcert);
cprintf("pcsts   = %x\n",error_data[cpunum].s_pcsts);
cprintf("pcerr   = %x\n",error_data[cpunum].s_pcerr);
cprintf("vintsr  = %x\n",error_data[cpunum].s_vintsr);

/*
 * Go find any pending memory (XMA) errors and log them in the error log.
 */

rxma_check_errors(EL_PRISEVERE);

/*
 * If one of the following conditions has occurred, we must panic since no retry
 * is possible for these conditions.
 */

/*
 * Note on I_PERR.  Must panic since write data in memory write queue could be lost if
 * this error occurs.
 */

if(((error_data[cpunum].s_bcsts & (BCSTS_SECOND_ERR|BCSTS_AC_PERR|BCSTS_I_PERR|
     BCSTS_BTS_VDPERR|BCSTS_BTS_TPERR)) != 0)    || /* AC_PERR,S_ERR,I_PERR,TP/VDPERR */
   ((error_data[cpunum].s_xber & 0x03000000) != 0)   ||            /* IPE or WEI */
   ((error_data[cpunum].s_xbeer0 & 0x67272004) != 0)) {            /* XBEER0 errors */
          panic("Hard error");
}

/*
 * Mark those errors which, if they occur again, will disable the backup cache.
 * Also, reenable the cache if this was the first time the error occurred.
 */

/*
 * Restart cache fill abort errors if cmd was RD.  Panic on IRD and ORD and if CNAK set.
 */

if ((error_data[cpunum].s_bcsts & BCSTS_FILL_ABORT) && ((error_data[cpunum].s_xber & 0x60000) != 0) && ((error_data[cpunum].s_xber & 0x808000) == 0x800000)) {
  if((error_data[cpunum].s_xfaer0 & 0xf0000000) == 0x10000000) {
    if(time_now - bcache_errtime[cpunum] < 500) {
      bcache_error[cpunum]++;
    }
    else {
      bcache_errtime[cpunum] = time_now;
    }
    error_enable_cache();
    return(0);
  }
}

panic("Hard error");
/*NOTREACHED*/
}

/*
 *  ka6500softerr() --- "Soft" errors are reported through SCB vector
 *  0x54, at IPL 0x1a. This routine runs on the interrupt stack.
 *
 *  These errors include the following:
 *
 *	. P-cache errors
 *	. MAXMI-detected errors
 *	. C-chip tag store parity errors
 *	. C-chip detected errors
 *	. Vector unit errors
 *
 *  These errors do not affect instruction execution.  
 *  
 *  NOTE: This routine is called from ka6400softerr whenever that
 *  routine determines that the system is a MARIAH.
 *
 */
ka6500softerr()
{
register int cpunum;
register struct el_rec *elrp;
register struct el_xmp *ptr;
register xmp_node;
long time_now;

time_now = mfpr(TODR);		/* Log the current time	*/
cpunum = CURRENT_CPUDATA->cpu_num;
snapshot_registers(cpunum);	/* Read in all relevent registers */
xmp_node = rssc->s_iport & 0xf;	/* Node ID of processor		  */

error_disable_cache();	/* Disable caches */

/*
 * If an error has occurred, perform full memory error recovery!
 */

if ( error_data[cpunum].s_pcsts & PCSTS_INTERRUPT ||
     error_data[cpunum].s_xber  & XMI_PE          ||
     error_data[cpunum].s_xber  & XMI_CC          ||
     error_data[cpunum].s_xbeer0 & XMI_WCDE0      ||
     error_data[cpunum].s_xbeer0 & XMI_WCDE1) 
	recover_mem_error();

/*
 * Go find any memory (XMA2) CRD errors.
 *
 * At the same time, disable CRD errors on the memory nodes.
 *
 * note: this will be re-enabled at regular intervals by ka6500memenable ().
 * It is disabled to keep the number of corrected errors reported down to 
 * a dull roar.
 */

if (error_data[cpunum].s_xber & XMI_CRD) {
	rxma_check_crd(EL_PRILOW);
	recover_mem_error();
}

/*
 * If any vector errors occurred, perform full vector error recovery!
 */
 
     /* PLACEHOLDER */

/*
 * check to see if there really is an error.  note: don't check for a CRD 
 * (Corrected Read error) here, because it does not need an error packet.
 */

if ( error_data[cpunum].s_pcsts & PCSTS_INTERRUPT ||
     error_data[cpunum].s_xber  & XMI_PE          ||
     error_data[cpunum].s_xber  & XMI_CC          ||
     error_data[cpunum].s_xbeer0 & XMI_WCDE0      ||
     error_data[cpunum].s_xbeer0 & XMI_WCDE1) {
	elrp = ealloc(sizeof(struct el_xmp), EL_PRILOW);
	if (elrp) {
		LSUBID(elrp,ELCT_INT54,ELINT_6500,EL_UNDEF,EL_UNDEF,
			EL_UNDEF,EL_UNDEF); 
		ptr = &elrp->el_body.el_xmp;
		ptr->s_xbe0 = error_data[cpunum].s_xber;
		ptr->s_xfadr0 = error_data[cpunum].s_xfadr;
		ptr->s_xfaer0 = error_data[cpunum].s_xfaer0;
		ptr->s_xbeer0 = error_data[cpunum].s_xbeer0;
		ptr->s_wfadr0 = error_data[cpunum].s_wfadr0;
		ptr->s_wfadr1 = error_data[cpunum].s_wfadr1;
		ptr->s_fdal0 = error_data[cpunum].s_fdal0;
		ptr->s_fdal1 = error_data[cpunum].s_fdal1;
		ptr->s_fdal2 = error_data[cpunum].s_fdal2;
		ptr->s_fdal3 = error_data[cpunum].s_fdal3;
		ptr->s_sscbtr = error_data[cpunum].s_sscbtr;
		ptr->s_bcsts = error_data[cpunum].s_bcsts;
		ptr->s_bcera = error_data[cpunum].s_bcerr;
		ptr->s_bcert = error_data[cpunum].s_bcert;
		ptr->s_pcsts = error_data[cpunum].s_pcsts;
		ptr->s_pcerr = error_data[cpunum].s_pcerr;
		ptr->s_vintsr = error_data[cpunum].s_vintsr;
		EVALID(elrp);
	}
}

/*
 * Do the thresholding!
 */

/*
 * P-Cache Tag and Data Parity Errors.
 */
 
if((error_data[cpunum].s_pcsts & (PCSTS_P_DATA_PARITY_ERROR|PCSTS_TAG_PARITY_ERROR)) && 
   (error_data[cpunum].s_pcsts & PCSTS_INTERRUPT)) {
  if(time_now - pcache_errtime[cpunum] < 500) {
    pcache_error[cpunum]++;
  }
  else {
    pcache_errtime[cpunum] = time_now;
  }
}

/* 
 * Backup Cache Data Parity Error.
 */
if((error_data[cpunum].s_pcsts & (PCSTS_B_CASHE_HIT|PCSTS_DAL_DATA_PARITY_ERROR)) && 
   (error_data[cpunum].s_pcsts & PCSTS_INTERRUPT)) {
  if(time_now - bcache_errtime[cpunum] < 500) {
    bcache_error[cpunum]++;
  }
  else {
    bcache_errtime[cpunum] = time_now;
  }
}

/*
 * Selectively reenable caches.
 */
error_enable_cache();

return(0);
}

/*
 * Routine to recover from cache or memory system errors.
 * Error handling consists mostly of writing the original register
 * contents back into the XMP registers. This restores the "M" bits
 * to their original state, and clears the "WC" error bits.
 *
 * The strategy of memory recovery is to start from the
 * most distant component and work towards the processor(s).
 * Thus, XMI2 errors are processed first, followed by MSSC errors,
 * C-chip errors, and finally, P-cache errors.
 *
 * The following assumptions are made when this routine
 * is entered:
 *	1. The routine "snapshot_registers" has been called
 *	   recently to capture the values of various registers.
 *	2. Primary cache is disabled and backup cache is in ETM
 *         mode.
 *
 * The caches are not enabled when this routine returns.
 */
xmp_recover_mem_error()
{
register struct xrp_reg *nxv;	/* Virtual pointer to XMI node */
register int cpunum;
register int i;
register struct xmidata *xmidata;

xmidata = get_xmi(0);		/* Get pointer to XMI data structure */
/* get virtual addr of processor's XMI node space */
nxv = (struct xrp_reg *)xmidata->xmivirt + (rssc->s_iport & 0xf); 
cpunum = CURRENT_CPUDATA->cpu_num;		/* Get CPU number */

/*
 * Clear XMI errors.  They are in XBE0 and XBEER0 registers.
 * All registers are write one to clear.  
 */
nxv->xrp_xbe = error_data[cpunum].s_xber;
nxv->xmp_xbeer0 = error_data[cpunum].s_xbeer0;

/*
 * Clear MSSC bus timeout errors
 */
rssc->s_sscbtr = error_data[cpunum].s_sscbtr;

/* 
 * Backup Cache thresholding.
 */
if(bcache_error[cpunum] >= 1)
  bcache_disable[cpunum] = 1;


mtpr(XMP_BCSTS,error_data[cpunum].s_bcsts); /* clear bcsts error bits */

/*
 * Check P-cache tag parity errors, which require rewriting the entire
 * tag store.
 */
if (error_data[cpunum].s_pcsts & PCSTS_TAG_PARITY_ERROR) { /* if TAG_PARITY_ERROR */
	/* Write invalid tags with good parity to the primary cache */
	for (i=0; i<=255; i++) {
		mtpr(PCIDX, i*8);
		mtpr(PCTAG, 0x80000000);
	}
	/* Primary Cache thresholding */
	if(pcache_error[cpunum] >= 1)
	        pcache_disable[cpunum] = 1;
}

/*
 * Clear P-cache errors.
 */

pc_enable[cpunum] = error_data[cpunum].s_pcsts & 0xe0;
if(error_data[cpunum].s_pcsts & 0x2)
  pc_enable[cpunum] = pc_enable[cpunum] | 0x2;

return(0);
}


/*
 * Routine to clear all the write to clear bits of the XMI bus error
 * register,  the XMI bus error extension register,  and to deassert 
 * the XMI BAD signal on the backplane.
 */
ka6500_clear_xbe0()
{
register struct xmidata *xmidata;
register struct xrp_reg *nxv;

/* get pointer to the xmp node */
xmidata = get_xmi(0);

nxv =(struct xrp_reg *)xmidata->xmivirt + (rssc->s_iport & 0xf);

nxv->xrp_xbe = nxv->xrp_xbe & ~XMI_XBAD;
}

/*
 * Routine to clear all the write to clear bits of the XMI bus error
 * register,  the XMI bus error extension register,  and to deassert 
 * the XMI BAD signal on the backplane.
 */
ka6500_clear_xbeer0()
{
register struct xmidata *xmidata;
register struct xrp_reg *nxv;

/* get pointer to the xmp node */
xmidata = get_xmi(0);

nxv =(struct xrp_reg *)xmidata->xmivirt + (rssc->s_iport & 0xf);

nxv->xmp_xbeer0 = nxv->xmp_xbeer0;
}

/*
 * Routine to enable the VAX6500 primary cache without flushing it.
 */
ka6500_enable_primary()
{
	union cpusid cpusid;
	int s;

	s = spl7();
	mtpr(PCSTS,0x2);  /* enable primary cache */
	splx(s);
}

/*
 * Routine to enable the VAX6500 backup cache without flushing it.
 */
ka6500_enable_backup()
{
	union cpusid cpusid;
	int s;

	s = spl7();
	mtpr(XMP_BCCTL,0x2);  /* enable backup cache */
	splx(s);
}
