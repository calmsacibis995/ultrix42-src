/*
 * @(#)kn220.h	4.2	(ULTRIX)	8/9/90
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

/*
 * Revision History:
 *
 * 11-22-89	Robin
 * 	File Created
 *
 * 04-05-90	Robin
 *	Added defines needed to test NVRAM Battery.
 *
 */

/* Program Interval timer bit definition (TCR0-TCR1) 
 * that are in the SSC
 */
#define	TCR_RUN		0x00000001
#define	TCR_XFR		0x00000010
#define	TCR_INT		0x00000080
#define	TCR_ERR		0x80000000
#define	TCR_STP		0x00000004


/* MIPs Kseg 1 address of 10mS timer
 * control/status register
 */
#define TCSR	PHYS_TO_K1(0x10084010)  /* R3000 Interval Timer Register (ITR) */ 
#define	TCSR_IE		0x00000040	/* Enable timer interrupts (IE) */
#define	TCSR_IS		0x00000080	/* Timer interrupts status (IS) */

/* MIPs Kseg 1 address of error interrupt status register */
#define	ISR	PHYS_TO_K1(0x10084000)
#define ISR_CPUP           0x80000000	/* 1 if I/O board not using CPU */
#define	ISR_HALT           0x00000008	/* Halt request posted interrupt */
#define	ISR_PWF            0x00000004	/* Power fail interrupt		*/
#define	ISR_CERR           0x00000002	/* CQBIC or CMCTL error		*/

/* MIPs Kseg 1 address of interrupt vector read registers 		*/
#define	VRR0	PHYS_TO_K1(0x16000050)	/* IRQ0				*/
#define	VRR1	PHYS_TO_K1(0x16000054)	/* IRQ1				*/
#define	VRR2	PHYS_TO_K1(0x16000058)	/* IRQ2				*/
#define	VRR3	PHYS_TO_K1(0x1600005c)	/* IRQ3				*/

/* Second Generation Ethernet Controller Chip defines.
 */
#define	KN220SGEC_ADDR     0x10008000	/* physical addr of SGEC registers  */
#define	KN220_NI_ST_ADDR   0x10120000	/* physical addr of NI Station ROM  */
#define	SGEC_OFFSET	   0x10c	/* SCB offset for network interrupts*/

/* DSSI Disk Controller Chip defines.
 */
#define	KN220SIIBUF_ADDR   0x10100000	/* physical addr of MSI buffer RAM  */
#define KN220MSIREG_ADDR   0x10160000	/* physical addr of MSI registers   */
#define	MSI_OFFSET	   0xc4		/* SCB offset for msi interrupts    */

/* System Support Chip.
 */
#define	KN220SSC_ADDR      0x10140000	/* physical addr of SSC reg set	    */

/* QBUS Map register defines.
 */
#define	KN220QBUSREG	   0x10087800	/* phys addr of Qbus map reg - 0x800*/
#define	KN220QMAPBASEREG   0x10080010	/* phys addr of QBus map base reg   */

/* SCSI disk/tape driver address defines.
 */
#define	KN220_53C94_REG_ADDR 0x17100000	/* physical addr of 53C94 chip reg.'s*/
#define	KN220_SCSI_DMA_ADDR. 0x17140000	/* physical addr of SCSI DMA reg.   */
#define	KN220_SCSI_BUF_ADDR. 0x17180000	/* physical addr of SCSI buffer RAM */
#define SCSI_OFFSET	     0x1fc	/* SCB offset for SCSI interrupts   */

/* DMA System Error Register (DSER) is one of three registers associated 
 * with Q22-Bus interface error reporting.  This register is 
 * implemented in the CQBIC chip and logs main memory 
 * DMA xfer errors.
 */
#define KN220DSER	PHYS_TO_K1(0x10080004)
#define DSER_HALT	0x00008000	/* Q22 Bus BHALT line asserted	*/
#define DSER_DCOK	0x00004000	/* DCOK NEGATION detected	*/
#define DSER_MEMTO	0x00000080	/* DMA NXM no reply after 10us.	*/
#define DSER_PARITY	0x00000020	/* Read cycle returns parity err*/
#define DSER_MME	0x00000010	/* Main Memory error		*/
#define DSER_LOST	0x00000008	/* err addr lost see bits 7,5,4,0 */
#define DSER_NOGRNT	0x00000004	/* No grant timeout exceeded 10ms */
#define DSER_NXM	0x00000001	/* Set on a DMA to non-existent mem */

/* The IOPRE register is used to indicate to the CPU module weather 
 * any of the other modules which sit on the RIO bus are present.  
 * Currently only two are allowed, the I/O module and the VME
 * module.  Also shown are the bit masks to get into the WEAR (IOPRE).
 */
#define KN220IOPRES	PHYS_TO_K1(0x17000000) /*I/O Presence register (IOPRE) */
#define KN220_VMEP    	0x10000000	/* Bit 28 a 1 indicates the VME 
					 * board is missing
					 */
#define KN220_IOP       0x20000000	/* Bit 29 a 1 indicates the I/O
					 *  board is missing
					 */

#define KN220_BOK       0x04000000	/* Bit 26 a 1 indicates the NVR
					 * battery voltage is OK
					 */
/*
 * MEMORY Erros Syndrome 
 * Register (MESR) 	
 *
 * The MESR register latches the ECC syndrome bits and 
 * ECC error type bits for any memory cycle producing
 * an error.  Also shown are the bit masks to get into the MESR.
 */
#define KN220MESR    PHYS_TO_K1(0x17040000) 
/* BITs used in KN220MESR
 */
#define KN220_HCB    0xfe000000		/* The seven bit ECC for 
				         * High word of data 	
					 */
#define KN220_LCB    0x01fc0000		/* The seven bit ECC for 
					 * High word of data 	
					 */
#define KN220_HSYN   0x0003f800		/* seven bit ECC syndrome 
					 * checking high word of data 
					 */
#define KN220_HME    0x00000400		/* bit 10 a zero indicates 
					 * multi ECC error on high word 
					 */
#define KN220_HER    0x00000200		/* Bit 9 a zero indicates a 
					 * single ECC error on high word 
					 */
#define KN220_LSYN   0x000001fc		/* Bit 2-8 seven bit ECC syndrome 
					 * checking low word of data 
					 */
#define KN220_LME    0x00000002		/* Bit 1 a zero indicates multi 
					 * ECC error on low word 
					 */
#define KN220_LER    0x00000001		/* Bit 0 a zero indicates single 
					 * ECC error on low word 
					 */

/* The MEAR register latches bits 29-2 of the bus address for 
 * any memory cycle producing an
 * error.  Bit 0 is used to indicate an access 
 * to non existant memory address.
 */
#define KN220MEAR    PHYS_TO_K1(0x17080000) /*Memory Err Address Register(MEAR) 
					     */
/* BITs used in KN220MEAR
 */
#define KN220_WEA    0x1ffffffc		    /* Bits 2-29 bus address of any 
					     * memory cycle producing an error 
					     */
#define KN220_NXM    0x00000001		    /* Bit 1 non zero indicates a non 
					     * existant memory access error 
					     */

/* The MIDR is used by manufacturing to test memory and by Diagnostics.
 * We will use two of the Diag bits to validate the NVRAM batery information
 * in the IOPRES register.
 */
#define KN220MIDR    PHYS_TO_K1(0x170c0000) /*Memory ID Register(MIDR) 

/* BIT used to set the NVRAM Battery load on/off
 */
#define KN220BLOAD	0x00000020
#define KN220JUMPER	0x00000040


/* Q22-Bus Error address Register (BEAR) read only 
 * register in the CQBIC chip that contains
 * the page of the Q22-Bus space which caused a parity error.
 */
#define	KN220QBEAR	PHYS_TO_K1(0x10080008)

/* DMA Error Address Register (DEAR) is read only register 
 * in the CQBIC chip and contains the map
 * translated address of the page in local memory which 
 * caused a memory error or non existent memory error.
 *
 * This is also listed as SEAR in some doc's. Slave Error Address Register.
 */
#define	KN220DEAR	PHYS_TO_K1(0x1008000c)

#define KN220CBTCR	PHYS_TO_K1(0x10140020) /* CDAL Bus Timeout 
						* Register (CBTCR) 
						*/

/* Interprocess Communication Reg.
 * NOTE: reads or writes to this register will cause an interrupt.
 */
#define KN220IPCR	PHYS_TO_K1(0x10001f40) 
#define IPCR_DMA_QME	0x8000	/* Q22 Bus Address Space Memory Error	*/
#define IPCR_QMCIA	0x4000	/* Q22 Bus Map Cache Invalidate All	*/
#define IPCR_DBI_IE	0x0040	/* Door Bell Interrupt Enable		*/
#define IPCR_LM_EAE	0x0020	/* Local Memory External Access Enable	*/
#define IPCR_DBI_RQ	0x0001	/* Door Bell Interrupt Request		*/

/* Size of the array used to limit the number of times a
 * single bit ECC error get logged.
 */
#define SBIT_SIZE 25

