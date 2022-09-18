/*
 * @(#)kn02ba.h	4.3	(ULTRIX)	10/16/90
 */

/************************************************************************
 *									*
 *			Copyright (c) 1989 by				*
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

/*
 * Modification History:
 *
 * 15-Oct-90	Randall Brown
 *	Added error handling code.
 *
 * 23-Feb-90	Randall Brown
 *	Created file for support of 3MIN (DS5000_100).
 *
 */


#define	KN02BA_ADDR_ERR		0x0e000004	/* Address Error Register */
#define KN02BA_BOOT_0_REG	0x0e000008	/* Boot 0 Register */
#define KN02BA_INTR_REG		0x0e00000c	/* CPU Interrupt Register */

#define	KN02BA_MEM_ERR		0x0c400000	/* Memory Error Register */
#define PAGE_BOUNDRY_ERR	0x00010000	/*   Page Boundry Error */
#define	TRANSFER_LEN_ERR	0x00008000	/*   Transfer Length Error */
#define MEM_ERR_DISABLE		0x00004000	/*   Memory Error Disable */
#define	LAST_BYTE_ERR_MASK	0x00000f00	/*   Last Byte Error Mask */

#define KN02BA_MEM_SIZE		0x0c800000	/* Memory Size Register */
#define	KN02BA_16MB_MEM		0x00002000	/* 16MB if set, 4MB otherwise */

#define KN02BA_SSR_ADDR		0x1c040100	/* System Support Register */
#define	KN02BA_SIR_ADDR		0x1c040110	/* System Interrupt Register */
#define	KN02BA_SIRM_ADDR	0x1c040120	/* System Intr Mask Register */

#define KN02BA_SL_0_ADDR 0x10000000
#define KN02BA_SL_1_ADDR 0x14000000
#define KN02BA_SL_2_ADDR 0x18000000
#define KN02BA_SL_3_ADDR 0x1c300000
#define KN02BA_SL_4_ADDR 0x1c0c0000
#define KN02BA_SL_5_ADDR 0x1c100000
#define KN02BA_SL_6_ADDR 0x0
#define KN02BA_SL_7_ADDR 0x0

#define KN02BA_SCSI_ADDR	0x1c300000
#define KN02BA_LN_ADDR		0x1c0c0000
#define KN02BA_SCC_ADDR		0x1c100000
#define	KN02BA_CLOCK_ADDR	0x1c200000	/* Base Address of Clock */

#define KN02BA_SCSI_INDEX	3
#define KN02BA_LN_INDEX		4
#define KN02BA_SCC_INDEX	5

#define COMM1_XMIT	0x80000000	/* Comm Port 1 Xmit Intr 	*/
#define COMM1_XMIT_DMA	0x40000000	/* Comm Port 1 Xmit DMA Error	*/
#define COMM1_RECV	0x20000000	/* Comm Port 1 Recv Intr 	*/
#define COMM1_RECV_DMA	0x10000000	/* Comm Port 1 Recv DMA Error	*/

#define COMM2_XMIT	0x08000000	/* Comm Port 2 Xmit Intr 	*/
#define COMM2_XMIT_DMA	0x04000000	/* Comm Port 2 Xmit DMA Error	*/
#define COMM2_RECV	0x02000000	/* Comm Port 2 Recv Intr 	*/
#define COMM2_RECV_DMA	0x01000000	/* Comm Port 2 Recv DMA Error	*/

#define	RESERVED_23	0x00800000	/* Reserved Bit 23		*/
#define	RESERVED_22	0x00400000	/* Reserved Bit 22		*/
#define	RESERVED_21	0x00200000	/* Reserved Bit 21		*/
#define	RESERVED_20	0x00100000	/* Reserved Bit 20		*/

#define SCSI_DMA_INTR	0x00080000	/* SCSI DMA buffer ptr loaded	*/
#define SCSI_DMA_ORUN	0x00040000	/* SCSI DMA Overrun Error	*/
#define SCSI_DMA_MEM	0x00020000	/* SCSI DMA Mem Read Error	*/
#define LANCE_DMA_MEM	0x00010000	/* LANCE DMA Mem Read Error	*/

#define RESERVED_15	0x00008000	/* Reserved Bit 15		*/
#define	UNSCUR_JMPR	0x00004000	/* Security Mode Jumper		*/
#define RESERVED_13	0x00002000	/* Reserved Bit 13		*/
#define CPU_IO_TIMEOUT	0x00001000	/* CPU IO-Write Timeout Intr	*/

#define RESERVED_11	0x00000800	/* Reserved Bit 11		*/
#define NRMOD_JMPR	0x00000400	/* Manufacturing Mode Jumper	*/
#define	SCSI_CHIP_INTR	0x00000200	/* SCSI 53c94 Chip Interrupt	*/
#define	LANCE_CHIP_INTR	0x00000100	/* LANCE Chip Interrupt		*/

#define	SCC1_INTR	0x00000080	/* SCC(1) Intr (Com 2 & kybd)	*/
#define	SCC0_INTR	0x00000040	/* SCC(0) Intr (Com 1 & mouse)	*/
#define TOY_INTR	0x00000020	/* Clock Interrupt		*/
#define	PSWARN		0x00000010	/* Power Supply Warning		*/

#define	RESERVED_3	0x00000008	/* Reserved Bit 3		*/
#define	SCSI_DATA_RDY	0x00000004	/* SCSI Data Ready		*/
#define PBNC		0x00000002	/* PBNC				*/
#define	PBNO		0x00000001	/* PBNO				*/

#define RESERVED_BITS	(RESERVED_23 | RESERVED_22 | RESERVED_21 | \
			 RESERVED_20 | RESERVED_15 | RESERVED_13 | RESERVED_3  )


#define SLU_INTR	(COMM1_XMIT | COMM1_XMIT_DMA | COMM1_RECV | \
			 COMM1_RECV_DMA | COMM2_XMIT | COMM2_XMIT_DMA | \
			 COMM2_RECV | COMM2_RECV_DMA | SCC1_INTR | \
			 SCC0_INTR )
#define SCSI_INTR	(SCSI_DMA_INTR | SCSI_DMA_ORUN | SCSI_DMA_MEM | \
			 SCSI_CHIP_INTR )
#define LANCE_INTR	(LANCE_DMA_MEM | LANCE_CHIP_INTR)


/*	Theses  masks are defined as the bits that are allowed to continue 	*/
/*	to cause an interrupt after the mask is set into the SIRM 	*/
#define SPL0_MASK	(CPU_IO_TIMEOUT | TOY_INTR)
#define	SPL1_MASK	(SPL0_MASK)
#define SPL2_MASK	(SPL0_MASK)
#define	SPLIO_MASK	(TOY_INTR | CPU_IO_TIMEOUT )
#define	SPLCLOCK_MASK	(CPU_IO_TIMEOUT)
#define SPLMEM_MASK	(0)
#define SPLFPU_MASK	(0)

/* SR_IBIT7 allows halt to always come in */
#define KN02BA_SR_IMASK0	(SR_IEC | SR_IBIT1 | SR_IBIT2 | SR_IBIT6 | \
				 SR_IBIT7 | SR_IBIT8)
#define KN02BA_SR_IMASK1	(SR_IEC | SR_IBIT2 | SR_IBIT6 | SR_IBIT7 | \
				 SR_IBIT8)
#define KN02BA_SR_IMASK2	(SR_IEC | SR_IBIT6 | SR_IBIT7 | SR_IBIT8)
#define KN02BA_SR_IMASK5	(SR_IEC | SR_IBIT6 | SR_IBIT7 | SR_IBIT8)
#define KN02BA_SR_IMASK6	(KN02BA_SR_IMASK5)
#define KN02BA_SR_IMASK7	(KN02BA_SR_IMASK5)
#define KN02BA_SR_IMASK8	(SR_IEC | SR_IBIT7)

#define CR_FPU		8
#define CR_HALT		7
#define CR_SYSTEM	6
#define CR_OPTION_2	5
#define CR_OPTION_1	4
#define CR_OPTION_0	3
#define CR_SOFTNET	2
#define CR_SOFTCLOCK	1
#define CR_NONE		0

#define	KN02BA_SIRM_K1ADDR	0xbc040120

/* rpbfix: don't mask HALT */
/*#define KN02BA_SPL_MASK		(SR_IMASK8 | SR_IEC)*/
#define KN02BA_SPL_MASK		(0)


#define KN02BA_ESRPKT 1
#define KN02BA_MEMPKT 2

/*
 * These defines, macros, and variables are for memory parity errors.
 * Format of "memreg" for logging memory parity errors.
 */
#define SIMMOFF 28
#define TYPEOFF 26
#define BYTEOFF 25
#define DPOFF 24
#define TCOUNTOFF 16
#define SCOUNTOFF 8
#define HCOUNTOFF 0
#define MEMREGFMT(simm, type, byte, dp, tcount, scount, hcount) \
(simm << SIMMOFF | type << TYPEOFF | byte << BYTEOFF | dp << DPOFF | \
tcount << TCOUNTOFF | scount << SCOUNTOFF | hcount << HCOUNTOFF)

#define KN02BA_TRANSINTVL (60*15)	/* time delta to enable parity log - 15 mins */

#ifdef LANGUAGE_C
struct trans_errcnt {             /* trans parity errors */
    long   trans_last;	/* time of most recent trans err */
    long   trans_prev;	/* time of previous trans err */
};

#define TRANSPAR 0x1
#define SOFTPAR 0x2
#define HARDPAR 0x3
#define MAXSIMM 16


struct kn02ba_consinfo_esr_t {
    u_int cause;	/* from the exception frame */
    u_int epc;		/* from the exception frame */
    u_int status;	/* from the exception frame */
    u_int badva;	/* from the exception frame */
    u_int sp;		/* from the exception frame */
    u_int ssr;     	/* system support reg */
    u_int sir;     	/* system interrupt reg */	
    u_int sirm;    	/* system interrupt mask */
};

struct kn02ba_consinfo_mem_t {
    u_int memreg;
    u_int pa;
    u_int epc;
    u_int badva;
};

struct kn02ba_consinfo_t {
    int	pkt_type;
    union {
	struct kn02ba_consinfo_esr_t esrp;
	struct kn02ba_consinfo_mem_t memp;
    } pkt;
};
    
#endif /* LANGUAGE_C */

