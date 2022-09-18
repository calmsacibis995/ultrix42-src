/*
 * 	@(#)ka60.h	4.1	(ULTRIX)	7/2/90";
 */
/*
 * 	@(#)ka60.h	2.3	(ULTRIX)	5/12/89";
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
 * Modification History:	ka60.h
 *
 * 17-Jul-89	Darrell A. Dunnuck (darrell)
 *	Move some #defines here from fc.c, and add an external
 *	declaration for FFFQAMCSRmap[].
 *
 * 07-Jul-89	Darrell Dunnuck (darrell)
 *	Moved constants that were in fc.c in UWS V2.1 to here
 *	where they really belong.
 *
 * 12-May-89	darrell
 *	Moved constants that were in ka60.c in UWS V2.0a to here
 *	where they really belong.
 *
 * 16-Jun-88	darrell
 *	Removed external structure definitions  for I/O space
 *	mapping that is no longer used.
 *
 * 12-7-87	darrell
 *	Creation of this file.
 *
 **********************************************************************/


u_long *ka60_ip[16];

/*
 * MBUS macroes
 */

/*
 * Name:	MBUS_SLOT
 *
 * Args:	paddr - a physical address in Firefox Slot I/O space
 *
 * Returns:	The slot number corresponding to the physical address
 */
#define MBUS_SLOT(paddr)   ((int)(((u_long)paddr & (u_long)0x0e000000) >> 25))

/*
 * Name:	FF_WHICH_PROC
 *
 * Args:	paddr - a physical address in Firefox Slot I/O space
 *
 * Returns:	the constant FBIC_PROCA or FBIC_PROCB
 */
#define FF_WHICH_PROC(paddr) ((u_long)(((u_long)paddr & (u_long)0x01000000) >> 22))

/*
 * Name:	MBUS_BASEADDR
 *
 * Args:	slot - a Firefox M-Bus slot number - 0 to 7.
 *
 * Returns:	The base addr of the module in the given M-Bus slot
 */
#define MBUS_BASEADDR(slot)	((u_long)MB_SLOT0_BASE + (slot << 25))

/*
 * NVR physical Constants
 */
#define FF_IO_SLOT	0x2014054A	/* NVR address containing IO Mod slot */
/*
 * M-Bus physical constants
 */
#define	MB_SLOT0_BASE	0x30000000	/* Physical base address of Slot 0    */
#define MB_SLOT7_BASE	0x3e000000	/* Physical base address of Slot 7    */
#define MB_SLOT_SIZE	0x02000000	/* Size of Mbus slot 		      */
/*
 * Module Specfic Addresses
 */
#define FF_SI_OFF	0x00000000	/* Start of SII Registers	      */
#define FF_NI_OFF	0x00200000	/* Start of NI Registers	      */
#define FF_SIB_OFF	0x00400000	/* Start of SII Buffer		      */
#define FF_DZ_OFF	0x00600000	/* Start of Console DZ Registers      */
#define FF_IOCSR_OFF	0x00800000	/* Start of IOCSR Register	      */
#define FF_NIBUF_OFF	0x00A00000	/* Start of NI Buffer		      */
#define FF_BSROM_OFF	0x00e00000	/* Start of Base System ROM (CPU Mod) */
#define MB_FBIC_OFFSET	0x01fffe00	/* I/O page FBIC Regs		      */
#define MB_2ND_FBIC_OFFSET 0x00fffe00	/* I/O page of Processor B FBIC Regs  */
#define FF_SAROM_OFF	FF_IOCSR_OFF	/* Start of NI Station Address ROM    */
#define FF_LEGSS	0x2		/* Device type of LEGSS		      */
#define FF_FQAM_CSR	0x30600000	/* FQAM CSR register (slot 0 only)    */

/*
 * Firefox constants
 */
#define FMOD_FBIC 0x00010000		/* FBIC interface type */
#define FMOD_FMDC 0x00020000		/* FMDC interface type */
#define FMOD_FMCM FMOD_FIRESTARTER	/* Firestarter interface type */
#define FBIC_ANALYZE	0x02		/* Use this FBIC information for
					   error analysis		*/
#define FBIC_VALID	0x01		/* This FBIC error info is valid */

#define FMDC_ERROR 	0x80000000	/* FMDC Error Summary */
#define FMDC_MBE	0x00000200	/* Multiple Bit Error		      */
#define FMDC_SBE	0x00000100	/* Single Bit Error		      */
#define FMDC_CNTLR	0xf0000000	/* High nibble of mb_node.mb_flags    */
#define FQCSR_DUMP_ERR	0x00000002	/* Dump Error bit in FQAM CSR	      */

/*
 * FMDC Memory error register bits
 */
#define FMDCSR_EFS		0x80000000	/* Error Flag Summary	      */
#define FMDCSR_ONLINE		0x40000000	/* Module online	      */
#define FMDCSR_INH_SBE_REPORT	0x00400000	/* Inhibit SBE reporting      */
#define FMDC_ECCSYDN_SBE	0x00000010	/* Single bit error	      */
#define FMDC_ECCSYND_MBE	0x00000020	/* Multiple bit error	      */

/*
 * Bits in MSER: Memory System Error Register (IPR 39)
 */
#define FF_MSER_DAL	0x00000040	/* <6> CDAL or 2nd level cache data store parity */
#define FF_MSER_MCD	0x00000020	/* <5> mcheck due to DAL parity error */
#define FF_MSER_MCC	0x00000010	/* <4> mcheck due to 1st lev cache parity */
#define FF_MSER_DAT	0x00000002	/* <1> data parity in 1st level cache */
#define FF_MSER_TAG	0x00000001	/* <0> tag parity in 1st level cache */
#define FF_MEM_CRD	0x20000000	/* Corrected Read Data */
#define FF_MEM_RDS	0x80000000	/* Read Data Substitute */


/*
 * FBIC registers
 */

struct fbic_regs {
	u_long	pad[113];
	u_long	f_savgpr;	/* Scratch register for halt code	      */
	u_long	f_pad1[2];
	u_long	f_iadr2;	/* Interlock 2 address register		      */
	u_long	f_iadr1;	/* Interlock 1 address register		      */
	u_long	f_cpuid;	/* Unique hardware ID register		      */
	u_long	f_whami;	/* Unique software ID register		      */
	u_long	f_ipdvint;	/* Interprocessor/device interrupt register   */
	u_long	f_range;	/* I/O space range decode register	      */
	u_long	f_fbicsr;	/* FBIC control status register		      */
	u_long	f_busdat;	/* M-bus error data signal log register	      */
	u_long	f_busaddr;	/* M-bus error address signal log register    */
	u_long	f_busctl;	/* M-bus error control signal log	      */
	u_long	f_buscsr;	/* M-bus error status register		      */
	u_long	f_modtype;	/* Module type register			      */
};

/*
 * FMDC registers
 */

struct fmdc_regs {
	u_long	pad[109];
	u_long	fm_exp_selfsig;	/* Self test signature expect register	      */
	u_long	fm_exp_dramsig;	/* DRAM control signature expect register     */
	u_long	fm_exp_mbussig;	/* M-Bus control signature expect register    */
	u_long	fm_ledlatch;	/* Diagnostics/self test LED latch	      */
	u_long	fm_selfsig;	/* Self test signature register		      */
	u_long	fm_dramsig;	/* DRAM control signature register	      */
	u_long	fm_mbussig;	/* M-bus control signature register	      */
	u_long	fm_msecterr;	/* Memory section had error register	      */
	u_long	fm_eccsynd1;	/* Memory ECC error status reg.(QW1)	      */
	u_long	fm_eccsynd0;	/* Memory ECC error status reg.(QW0)	      */
	u_long	fm_eccaddr1;	/* Memory ECC error address reg.(QW0)	      */
	u_long	fm_eccaddr0;	/* Memory ECC error address reg.(QW0)	      */
	u_long	fm_baseaddr;	/* Memory space base address register	      */
	u_long	fm_fmdcsr;	/* FMDC control/status register		      */
	u_long	fm_busdat;	/* M-bus error data signal log register	      */
	u_long	fm_busaddr;	/* M-bus error address signal log register    */
	u_long	fm_busctl;	/* M-bus error control signal log register    */
	u_long	fm_buscsr;	/* M-bus error status register		      */
	u_long	fm_modtype;	/* Module type register			      */
};

/*
 * FBIC MODTYPE Register
 */
#define	FMOD_QBUS	0x01		/* FBIC Q-bus Adaptor Module	      */
#define FMOD_GRAPHICS	0x02		/* FBIC Graphics Module		      */
#define FMOD_IO		0x04		/* FBIC I/O Module		      */
#define FMOD_CPU	0x08		/* FBIC CPU Module		      */
#define FMOD_MEM	0x10		/* FMDC Memory Module		      */
#define FMOD_FIRESTARTER 0x00FE0000	/* Firestarter Memory		      */
#define FMOD_REV	0xff000000	/* Revision Mask		      */
#define FMOD_INTERFACE	0x00ff0000	/* Interface Mask		      */
#define FMOD_SUBCLASS	0x0000ff00	/* Subclass Mask		      */
#define FMOD_CLASS	0x000000ff	/* Class Mask			      */


/*
 * FBIC BUSCSR Register
 *
 * NOTE: BUSCSR register bits are active low.
 */
#define	FBCSR_FRZN	0x80000000	/* M-bus Error Logging Frozen	      */
#define	FBCSR_ARB	0x40000000	/* M-bus Arbitration Error	      */
#define FBCSR_ICMD	0x20000000	/* M-bus Invalid MCMD Encoding	      */
#define FBCSR_IDAT	0x10000000	/* M-bus Invalid Data Supplied	      */
#define FBCSR_MTPE	0x08000000	/* M-bus Tag Parity Error	      */
#define FBCSR_MDPE	0x04000000	/* M-bus MDAL Parity Error	      */
#define FBCSR_MSPE	0x02000000	/* M-bus STATUS Parity Error	      */
#define FBCSR_MCPE	0x01000000	/* M-Bus MCMD Parity Error	      */
#define FBCSR_ILCK	0x00800000	/* M-bus Interlock Violation	      */
#define FBCSR_MTO	0x00400000	/* M-bus Slave Timeout		      */
#define FBCSR_NOS	0x00200000	/* M-bus No Slave Response	      */
#define FBCSR_CTO	0x00100000	/* CDAL Timeout			      */
#define FBCSR_CDPE	0x00080000	/* CDAL Parity Error		      */
#define FBCSR_CTPE	0x00040000	/* CDAL Tag Store Parity Error	      */
#define FBCSR_CLEAR	0xffffffff	/* Used to re-enable error logging    */

/*
 * FBIC FBICSR Register
 */
#define FFCSR_EXCAEN	0x04000000	/* External Cache Enable	      */
#define FFCSR_HALTCPU	0x02000000	/* C-bus HALT Control		      */
#define FFCSR_RESET	0x01000000	/* C-bus RESET Control		      */
#define FFCSR_IRQE_3	0x00800000	/* Interrupt Request Enable CIRQ3     */
#define FFCSR_IRQE_2	0x00400000	/* Interrupt Request Enable CIRQ2     */
#define FFCSR_IRQE_1	0x00200000	/* Interrupt Request Enable CIRQ1     */
#define FFCSR_IRQE_0	0x00100000	/* Interrupt Request Enable CIRQ0     */
#define FFCSR_IRQD_3	0x00080000	/* Interrupt Request Direction	      */
#define FFCSR_IRQD_2	0x00040000	/* Interrupt Request Direction	      */
#define FFCSR_IRQD_1	0x00020000	/* Interrupt Request Direction	      */
#define FFCSR_IRQD_0	0x00010000	/* Interrupt Request Direction	      */
					/* 1 = CIRQ -> MIRQ, 0 = MIRQ -> CIRQ */
#define FFCSR_LEDS_OFF	0x00003f00	/* All LEDS off			      */
#define FFCSR_HALTEN	0x00000080	/* Enable CPU Halts		      */
#define FFCSR_PRI0EN	0x00000040	/* Mbus Hog mode		      */
#define FFCSR_NORMAL	0x0000003e	/* FBIC Diagnostic Test Funciton Mask */
#define FFCSR_CDPE	0x00000001	/* C-bus Parity Check Enable	      */
#define FFCSR_IRQE_X	FFCSR_IRQE_3|FFCSR_IRQE_2|FFCSR_IRQE_1|FFCSR_IRQE_0
#define FFCSR_IRQD_X	FFCSR_IRQD_3|FFCSR_IRQD_2|FFCSR_IRQD_1|FFCSR_IRQD_0
/*
 * FBIC RANGE Register
 */
#define FRANGE_ENA	0x00008000	/* M-Bus I/O-Space Addr-Range Enable  */

/*
 * FBIC IPDVINT Register
 */
#define FIPD_IPL17	0x08000000	/* Generate IPL 17 Interrupt	      */
#define FIPD_IPL16	0x04000000	/* Generate IPL 16 Interrupt	      */
#define FIPD_IPL15	0x02000000	/* Generate IPL 15 Interrupt	      */
#define FIPD_IPL14	0x01000000	/* Generate IPL 14 Interrupt	      */
#define FIPD_IPUNIT	0x00020000	/* Interprocessor-Interrupt Unit      */
#define FIPD_DEVUNIT	0x00010000	/* Device-Interrupt Unit	      */

/*
 * FBIC CPUID Register
 */
#define FCPU_PROCB	0x00000000	/* Identifier for Processor B	      */
#define FCPU_PROCA	0x00000003	/* Identifier for Processor A	      */
#define FCPU_MID_MSK	0x0000001c	/* Module Slot Itentifier Mask	      */

/*
 * VAX60 (ka60) Console ROM space.
 *    Has sys_type register.
 * At 0x2004 0000 in VAX Memory space; mapped to "ffcrom"
 */
struct ffcrom_regs {
	u_long pad;             /* sys_type register is 2nd long word	      */
	u_short	ffcrom_licbits;	/* Digital Product License Bits <TBD>	      */
        u_char ffcrom_firmrev;	/* ka60 firmware rev level		      */
	u_char ffcrom_systype;	/* identifies systype of this chip set	      */
};

/*
 * There is one mb_node structure per Mbus node (FBIC/FMDC)
 */
struct mb_node {
	u_long	mb_modtype;		/* Module Type			      */
	u_long	mb_physaddr;		/* Physical Addr of FBIC regs	      */
	struct fbic_regs *mb_vaddr;	/* Virtual Addr of FBIC regs	      */
	u_long	mb_flags;		/* Flags			      */
	u_long	mb_slot;		/* Slot this module is plugged into   */
};
 
/*
 * mb_node flags
 */
#define	FBIC_MAPPED	0x00000001	/* FBIC regs mapped		      */
#define FBIC_ALIVE	0x00000002	/* FBIC is alive		      */
#define FBIC_PROCA	0x00000004	/* Processor A on this Module	      */
#define FBIC_PROCB	0x00000008	/* Processor B on this Module	      */
#define FBIC_PRI_CPU	0x00000010	/* This is the primary CPU	      */#define FBIC_SEC_CPU	0X00000020	/* This is a secondary CPU	      */

/*
 * External declarations of the map names (declared  in spt.s)
 * for the VAX60 (ka60) local register space.
 */
extern struct pte FFQBMmap[];		/* maps to virtual cvqbm	      */
extern struct cvqbm_regs cvqbm[];	/* Qbus map regs		      */
extern struct pte FFQREGmap[];		/* maps to virtual ffqreg	      */
extern struct cqbic_regs ffqregs[];	/* mem err & mem config regs	      */
extern struct pte CVQSSCmap[];		/* maps to virtual cvqssc	      */
extern struct ssc_regs cvqssc[];	/* SSC registers		      */
extern struct pte FFCONSmap[];		/* maps to virtual ffcons	      */
extern struct fc_regs ffcons[];		/* Console DZ registers		      */
extern struct pte FFCROMmap[];		/* maps to virtual ffcrom	      */
extern struct ffcrom_regs ffcrom[];	/* Console ROM registers	      */
extern struct pte FFIOmap[];		/* maps to virtual ffio		      */
extern struct fbic_regs ffiom[];	/* FBIC registers		      */
extern struct pte FFIOCSRmap[];		/* maps to virtual ffiocsr	      */
extern struct ffiocsr ffiocsr[];	/* IOCSR register		      */
extern struct pte FFFQAMCSRmap[];	/* Maps to virtual fqamcsr	      */
