/*
 * 	@(#)ka650.h	4.1	(ULTRIX)	7/2/90";
 */

/************************************************************************
 *									*
 *			Copyright (c) 1986,87,88 by			*
 *		Digital Equipment Corporation, Maynard, MA		*
 *			All rights reserved.				*
 *									*
 *   This software is furnished under a license and may be used and	*
 *   copied  only  in accordance with the terms of such license and	*
 *   with the  inclusion  of  the  above  copyright  notice.   This	*
 *   software  or  any	other copies thereof may not be provided or	*
 *   otherwise made available to any other person.  No title to and	*
 *   ownership of the software is hereby transferred.			*
 *									*
 *   The information in this software is subject to change  without	*
 *   notice  and should not be construed as a commitment by Digital	*
 *   Equipment Corporation.						*
 *									*
 *   Digital assumes no responsibility for the use  or	reliability	*
 *   of its software on equipment which is not supplied by Digital.	*
 *									*
 ************************************************************************/

/***********************************************************************
 *
 * Modification History:	ka650.h
 *
 * 15-Feb-89 -- afd
 *	Re-synch with VAX 3.0 for 3.2 pool merge.
 *
 * 19-Jan-88 -- jaw
 *	move def of MSER to mtpr.h because it is shared with VAX6200
 *
 * 06-Jan-88 -- Robin
 *	Added the memory mapping for the NI and DSSI controllers that
 *	are on the KA640 board
 *
 * 10-Sep-87 -- afd
 *	Eliminate cache_errcnt.cache_state field since it was redundant to
 *	    global "cache_state" word (first level cache state).
 *	Added additional TIME_THRESH constants.  First and Second Level
 *	    cache errors are given longer time thresh-holds for 3 errors to occur.
 *	Make cvq4_cpmbx a char (8 bits) and add a char for the upper half
 *	word.  This is so that we only write the low byte (the true CPMBX),
 *	and don't corrupt the upper half-word contents.
 *
 * 27-Jul-87 -- afd
 *	Added mapping info and struct for local ROM space so we can get
 *	to the sys_type register.
 *
 * 13-Jul-87 -- afd
 *	Added 3 new timers for new recoverable machine checks:
 *	machine checks 1 thru 4: CFPA;
 *	machine check 82/83: primary flags MEM_CDAL and MEM_RDS.
 *
 * 01-June-87 -- afd
 *	Remove timer for CBTCR machine check errors.
 *	Fix decl of CACR register so we don't index into it.
 *
 * 19-May-87 -- afd
 *	Give size of 2nd level cache (CACHE_SIZE) in longwords (not bytes)
 *		and use this constant in decl of size of cache flush space.
 *	Define MEM_ERRDIS in memcsr17 to disable main mem error detect & correct
 *
 * 12-May-87 -- afd
 *	Changes for V1.5 of the error spec:
 *	    mear => qbear;   sear => dear
 *	Fixed defined values for 1st level cache sets enabled.
 *
 * 20-Apr-87 -- afd
 *	Cleanups:
 *	    Moved structures for Mayfair local register space here from ubareg.h
 *	    Moved Mayfair register space externals here from ubavar.h
 *	    Moved declaration of space for Mayfair local registers & 
 *	          declaration of array with CVAX machine check strings, to
 *		  ka650.c since this header file is now included by
 *		  cpuconf.c and machdep.c as well as ka650.c
 *	    Moved defines for restart/boot/halt codes to here from reboot.h
 *	Changes for error spec V1.4:
 *	    Removed macro to build memcon since memcon changed format
 *	    Added defines for a couple more bits
 *	    Added a 6th struct for Mayfair IPCR0 & added its map address 
 *	Changed name CVAXQ to VAX3600 for Mayfair.
 *	Added structure elements to cvq4_regs (SSC) for programable timers.
 *
 **********************************************************************/

/*
 * Error & status register bits for VAX3600 (KA650)
 * In KA650 local register space
 */

/* 
 * Define the rev level of CVAX CPU chip that has the POLY-PASSIVE RELEASE
 * problem fixed.
 */
#define POLY_FIX_REV	4


/*
 * Bits in DSER: DMA System Error Register (cvqmerr->cvq1_dser)
 */
#define DSER_QNXM	0x00000080	/* <7> Q-22 Bus NXM */
#define DSER_QPE	0x00000020	/* <5> Q-22 Bus parity Error */
#define DSER_MEM	0x00000010	/* <4> Main mem err due to ext dev DMA */
#define DSER_LOST	0x00000008	/* <3> Lost error: DSER <7,5,4,0> set */
#define DSER_NOGRANT	0x00000004	/* <2> No Grant timeout on cpu demand R/W */
#define DSER_DNXM	0x00000001	/* <0> DMA NXM */
#define DSER_CLEAR	(DSER_QNXM|DSER_QPE|DSER_MEM|DSER_LOST|DSER_NOGRANT|DSER_DNXM)

/*
 * Bits in MEMCSR16: Main Memory Error Status Register (cvqmerr->cvq1_memcsr16)
 */
#define MEM_EMASK	0xE0000180	/* mask of all err bits in memcsr16 */
#define MEM_RDS		0x80000000	/* <31> uncorrectable main memory */
#define MEM_RDSHIGH	0x40000000	/* <30> high rate RDS errors */
#define MEM_CRD		0x20000000	/* <29> correctable main memory */
#define MEM_DMA		0x00000100	/* <8>  DMA read or write error */
#define MEM_CDAL	0x00000080	/* <7>  CDAL Parity error on write */
#define MEM_BNK		0x03C00000	/* <25:22> Bank Number */

/*
 * Bits in MEMCSR17: Main Memory Control & Diag Status Reg (cvqmerr->cvq1_memcsr17)
 */
#define MEM_CRDINT	0x00001000	/* <12> CRD interrupts enabled */
#define MEM_ERRDIS	0x00000400	/* <10> error detect disable  */

/*
 * Bits in MEMCSR0-15: Main Memory Config Regs
 */
#define MEM_BNKENBLE	0x80000000	/* <31> Bank Enable */
#define MEM_BNKUSAGE	0x00000003	/* <1:0> Bank Usage */

/*
 * Bits in CACR: Cache Control Register (2nd level cache) (cvqcb->cvq2_cacr)
 */
#define CACR_CEN	0x00000010	/* <4>  Cache enable */
#define CACR_CPE	0x00000020	/* <5>  Cache Parity Error */

/*
 * Bits in CBTCR: CDAL Bus Timeout Control Register (cvqssc->cvq4_cbtcr)
 */
#define CBTCR_BTO	0x80000000	/* <31> r/w unimp IPR or unack intr */
#define CBTCR_RWT	0x40000000	/* <30> CDAL Bus Timeout on CPU or DMA R/W */

/*
 * Bits in TCR0/TCR1: Programable Timer Control Registers (cvqssc->cvq4_tcrx)
 * (The rest of the bits are the same as in the standard VAX
 *  Interval timer and are defined in clock.h)
 */
#define TCR_STP 0x00000004		/* <2>  Stop after time-out */

/*
 * Bits in MSER: Memory System Error Register (IPR 39)
 */
#define MSER_DAL	0x00000040	/* <6> CDAL or 2nd level cache data store parity */
#define MSER_MCD	0x00000020	/* <5> mcheck due to DAL parity error */
#define MSER_MCC	0x00000010	/* <4> mcheck due to 1st lev cache parity */
#define MSER_DAT	0x00000002	/* <1> data parity in 1st level cache */
#define MSER_TAG	0x00000001	/* <0> tag parity in 1st level cache */

/*
 * Bits in CADR: Cache Disable Register (IPR 37)
 */
#define CADR_STMASK	0xF0		/* 1st level cache state mask */
#define CVAX_SEN2	0x00000080	/* <7> 1st level cache set 2 enabled */
#define CVAX_SEN1	0x00000040	/* <6> 1st level cache set 1 enabled */
#define CVAX_CENI	0x00000020	/* <5> 1st level I-stream caching enabled */
#define CVAX_CEND	0x00000010	/* <4> 1st level D-stream caching enabled */

/*
 * Bits in PSL: Processor Status Longword (for mcheck recovery)
 */
#define PSL_FPD		0x08000000	/* <27> First Part Done flag */

/*
 * Bits in Internal Status Info 2: (for mcheck recovery)
 */
#define IS2_VCR		0x00008000	/* <15> VAX Can't Restart flag */

/*
 * Size of 2nd Level Cache: for cache flush operation
 */
#define CACHE_SIZE	(16*1024)	/* 16K longword (64K byte) cache */

#define CADR_SETMASK 0xC0
#define SET_BOTH (CVAX_SEN2 | CVAX_SEN1)
#define SET_TWO  CVAX_SEN2
#define SET_ONE  CVAX_SEN1
#define SET_NONE 0

/*
 * VAX3600 (ka650) local registers (in I/O space)
 * This is done as 7 disjoint sections whose 
 *    start addresses & sizes are later in this file.
 *    Map names are set up in spt.s and 
 *    they are mapped in routine ka650conf() in file ka650.c
 */
/*
 * VAX3600 (ka650) memory error & configuration registers
 * At 0x2008 0000 in Local Register space; mapped to "cvqmerr"
 */
struct cvq1_regs
{
	u_long cvq1_scr;	/* System Config Register */
	u_long cvq1_dser;	/* DMA System Error Register */
	u_long cvq1_qbear;	/* QBus Error Address Register */
	u_long cvq1_dear;	/* DMA Error Address Register */
	u_long cvq1_qbmbr;	/* Q Bus Map Base address Register */
	u_long pad[59];		/* filler (59 res words) */
	u_long cvq1_memcsr0;	/* Main Memory Config Reg (1 for each of 16 banks) */
	u_long cvq1_memcsr1;	/* Main Memory Config Reg */
	u_long cvq1_memcsr2;	/* Main Memory Config Reg */
	u_long cvq1_memcsr3;	/* Main Memory Config Reg */
	u_long cvq1_memcsr4;	/* Main Memory Config Reg */
	u_long cvq1_memcsr5;	/* Main Memory Config Reg */
	u_long cvq1_memcsr6;	/* Main Memory Config Reg */
	u_long cvq1_memcsr7;	/* Main Memory Config Reg */
	u_long cvq1_memcsr8;	/* Main Memory Config Reg */
	u_long cvq1_memcsr9;	/* Main Memory Config Reg */
	u_long cvq1_memcsr10;	/* Main Memory Config Reg */
	u_long cvq1_memcsr11;	/* Main Memory Config Reg */
	u_long cvq1_memcsr12;	/* Main Memory Config Reg */
	u_long cvq1_memcsr13;	/* Main Memory Config Reg */
	u_long cvq1_memcsr14;	/* Main Memory Config Reg */
	u_long cvq1_memcsr15;	/* Main Memory Config Reg */
	u_long cvq1_memcsr16;	/* MEMory CSR 16: Main Memory Error Status */
	u_long cvq1_memcsr17;	/* MEMory CSR 17: Main Memory Control */
};

/*
 * VAX3600 (ka650) Cache Control & Boot/Diag registers
 * At 0x2008 4000 in Local Register space; mapped to "cvqcb"
 */
struct cvq2_regs
{
	u_char cvq2_cacr1;	/* Low byte: Cache Enable & Parity Err detect */
	  u_char cvq2_cacr2;	/* Cache diagnostic field */
	  u_char cvq2_cacr3;	/* Cache diagnostic field */
	  u_char cvq2_cacr4;	/* Unused  */
	u_long cvq2_bdr;	/* Boot & Diagnostic Register */
};
	
/*
 * VAX3600 (ka650) Qbus map registers
 * At 0x2008 8000 in Local Register space; mapped to "cvqbm"
 */
struct cvq3_regs
{
	union {
		struct {
		long qb_pad[512];		/* need 2k bytes */
		struct pte qb_map[8192];	/* q-bus map registers */
		} cqba; 
		struct uba_regs uba;
	} cvq3_uba;
};

/*
 * VAX3600 (ka650) Cache Diagnostic Space (64K bytes, 16K long words),
 *     for flushing the 2nd level cache.
 * At 0x1000 0000 in VAX Memory space; mapped to "cvqcache"
 */
struct cvq5_regs
{
	int cvq5_cache[CACHE_SIZE];/* Cache Diagnostic Space (to flush cache) */
};

/*
 * VAX3600 (ka650) Inter Processor Communication Register;
 *    determines if memory error was from QBUS device DMA (as opposed to cpu).
 * At 0x2000 1F40 in VAX Memory space; mapped to "cvqipcr"
 */
struct cvq6_regs
{
	u_long pad[80];		/* filler (80 words from start of page) */
	u_short cvq6_ipcr0;	/* InterProcessor Comm Reg for arbiter */
};

/*
 * VAX3600 (ka650) Local ROM space.
 *    Has sys_type register.
 * At 0x2004 0000 in VAX Memory space; mapped to "cvqrom"
 */
struct cvq7_regs
{
	u_long pad;		/* sys_type register is 2nd long word */
	u_char cvq7_arch;	/* Architecture id (Workstation/timeshare) */
	u_char cvq7_sysdep;	/* system dependant for very similar systems */
	u_char cvq7_firmrev;	/* ka650 firmware rev level */
	u_char cvq7_systype;	/* identifies systype of this chip set */
};

/*
 * VAX3600 (ka650): Mapping info for First 'chunk' of Local Regs (1 page).
 */
#define CVQMERRADDR	((short *)(0x20080000))
#define CVQMERRSIZE	512

/*
 * VAX3600 (ka650): Mapping info for Second 'chunk' of Local Regs (1 page).
 */
#define CVQCBADDR	((short *)(0x20084000))
#define CVQCBSIZE	512

/*
 * VAX3600 (ka650): Mapping info for Fifth 'chunk' of space (128 pages)
 */
#define CVQCACHEADDR	((short *)(0x10000000))
#define CVQCACHESIZE	(512*128)

/*
 * VAX3600 (ka650): Mapping info for Sixth 'chunk' of space (1 page)
 */
#define CVQIPCRADDR	((short *)(0x20001E00))
#define CVQIPCRSIZE	(512)

/*
 * VAX3600 (ka650): Mapping info for Seventh 'chunk' of space (1 page)
 */
#define CVQROMADDR	((short *)(0x20040000))
#define CVQROMSIZE	(512)

/*
 * VAX3300 (ka640): Mapping info for Eight 'chunk' of space (1 page)
 */
#define CVQNIADDR	((short *)(0x20084400))
#define CVQNISIZE	(512)

/*
 * VAX3300 (ka640): Mapping info for Eight & 1/2 'chunk' of space (1 page)
 */
#define CVQNIDPADDR	((short *)(0x20084200))
#define CVQNIDPSIZE	(512)

/*
 * VAX3300 (ka640): Mapping info for Nineth 'chunk' of space (1 page)
 */
#define CVQMSIADDR	((short *)(0x20084600))
#define CVQMSISIZE	(512)

/*
 * VAX3300 (ka640): Mapping info for Tenth 'chunk' of space (256 page)
 */
#define CVQNILRBADDR	((short *)(0x20120000))
#define CVQNILRBSIZE	(512 * 256)

/*
 * VAX3300 (ka640): Mapping info for Eleventh 'chunk' of space (256 page)
 */
#define CVQMSIRBADDR	((short *)(0x20100000))
#define CVQMSIRBSIZE	(512 * 256)

/*
 * External declarations of the map names (declared  in spt.s)
 * for the VAX3600 (ka650) local register space.
 */

extern struct pte CVQMERRmap[];		/* maps to virtual cvqmerr */
extern struct cvq1_regs cvqmerr[];	/* mem err & mem config regs */
extern struct pte CVQCBmap[];		/* maps to virtual cvqcb */
extern struct cvq2_regs cvqcb[];	/* cache control & boot/diag regs */
extern struct pte CVQCACHEmap[];	/* maps to virtual cvqcache */
extern struct cvq5_regs cvqcache[];	/* Cache Diagnostic space (for flush) */
extern struct pte CVQIPCRmap[];		/* maps to virtual cvqipcr */
extern struct cvq6_regs cvqipcr[];	/* InterProcessor Com Regs */
extern struct pte CVQROMmap[];		/* maps to virtual cvqrom */
extern struct cvq7_regs cvqrom[];	/* Local ROM space */

/*
 * Flags for ka650 console program communication
 */
#define RB_CV_RESTART	0x31	/* Restart, english	*/
#define RB_CV_REBOOT	0x32	/* Reboot, english	*/
#define RB_CV_HALTMD	0x33	/* Halt, english	*/
