/*	10/10/90 (ULTRIX) @(#)ka9000.h	4.3	*/	
/************************************************************************
 *									*
 *			Copyright (c) 1988,1989,1990 by			*
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
 * Modification History: 	ka9000.h
 *
 * 15-Sep-90 -- stuarth (Stuart Hollander)
 *	Added struct for xja specific registers in xmi nodespace.
 *
 * 31-Aug-90 -- paradis
 *	Added machine check stack frame definitions.
 *	General cleanups, removed unused stuff.
 *
 * 22-Nov-89 -- paradis (Jim Paradis)
 *	Merged in SPU definitions.
 *
 * 13-Nov-89 -- rafiey (Ali Rafieymehr)
 *	Merged in changes required for booting VAX9000
 *
 * 27-Sep-89 -- rafiey (Ali Rafieymehr)
 *	Created this file.
 *
 ************************************************************************/

#define	XMI_START_PHYS 	0x20000000
#define	XJA_START_PHYS 	0x3E000000
/*#define	XJA_START_PHYS 	0x3E080000*/
#define	XJA_SIZE	0x80000
#define	XJA_SIZE_MAP	512
#define CPUCNF_XJA_PRESENT 12
#define	XJA_0	1
#define	XJA_1	2
#define	XJA_2	4
#define	XJA_3	8

extern 	struct pte XJAmap[];
extern 	struct pte XJAmap0[];
extern 	struct pte XJAmap1[];
extern 	struct pte XJAmap2[];
extern 	struct pte XJAmap3[];


struct v9000csr {
	long 	csr1;
};

struct	xja_regs {
	u_long	xja_errs;
	u_long	xja_fcmd;
	u_long	xja_ipint;
	u_long	xja_diag;
	u_long	xja_dmafaddr;
	u_long	xja_dmafcmd;
	u_long	xja_errintr;
	u_long	xja_cnf;
	u_long	xja_xbiida;
	u_long	xja_xbiidb;
	u_long	xja_errscb;
};
	/* Struct xmi_reg has registers present in all generic xmi nodes.
	   It does not have registers specific to xja nodes.
	   Struct xja_node_regs is used to reference these xja specific regs.
		But, note: the size of xja_node_regs is incomplete
		in that it does not occupy the same amount of
		space as xmi_reg.  For code that needs the complete
		size of an xmi node (of which the xja is one such node)
		use struct xmi_reg.  See the definition of xmi_reg.
	 */

	/* xja registers in xmi nodespace */
struct xja_node_regs {
	u_long 	xja_node_dev;		/* device type register */
	u_long 	xja_node_ber;		/* bus error status */
	u_long	xja_node_xfadra;	/* fail address on error LW0 */
	u_long	xja_node_xfadrb;	/* fail address on error LW1 */
	u_long	xja_node_xgpr;		/* XJA general purpose register */
	u_long	xja_node_faemc;		/* XJA FAEM control register */
	u_long	xja_node_aosts;		/* XJA AOST status register */
	u_long	xja_node_sernum;	/* XJA serial number register */
};



/* definitions for XJA Error Summary Register (ERRS) */
#define XJA_ERRS_JXDI_PE	0xc0000000
#define XJA_ERRS_JXDI_MPE	0x20000000
#define XJA_ERRS_XCE_TE		0x10000000
#define XJA_ERRS_ICU_BC		0x08000000
#define XJA_ERRS_CPU_RO		0x04000000
#define XJA_ERRS_RBO		0x02000000
#define XJA_ERRS_CLE		0x01000000
#define XJA_ERRS_CBI_PE		0x00800000
#define XJA_ERRS_XMI_ATO	0x00400000
#define XJA_ERRS_XMI_PF		0x00200000
#define XJA_ERRS_XMI_PU		0x00100000
#define XJA_ERRS_XMI_PE		0x000e0000
#define XJA_ERRS_CPU_ID		0x0000003f

/* definitions for XJA Error Interrupt Control ERRINTR bits */

#define XJA_XMI_ARB		0x00000001
#define XJA_JXDI		0x00000002
#define XJA_TRANS_TOUT		0x00002000
#define XJA_XMIT_ERR		0x00004000
#define XJA_CMD_NOACK		0x00008000
#define XJA_RD_RES		0x00010000
#define XJA_RD_SEQ_ERR		0x00020000
#define XJA_REATTEMP_TOUT	0x00040000
#define XJA_CRD			0x00080000
#define XJA_WR_NOACK		0x00100000
#define XJA_RD_NOACK		0x00200000
#define XJA_WR_SEQ_ERR		0x00400000
#define XJA_PAR_ERR		0x00800000
#define XJA_IPE			0x01000000
#define XJA_CC			0x08000000



struct xcp_reg {
	unsigned int xcp_dtype;
	unsigned int xcp_xbe;
	unsigned int xcp_fadr;
	unsigned int xcp_gpr;
	unsigned int xcp_csr2;
	char	xcp_pad[1004];
};


char *ka9000_ip[16];

/* definitions for CSR2 bits */
#define	CSR2_VBPE	0x80000000
#define	CSR2_TPE 	0x40000000
#define	CSR2_IQO 	0x20000000
#define	CSR2_WDPE	0x10000000
#define	CSR2_CFE	0x08000000
#define	CSR2_DTPE	0x04000000
#define	CSR2_LOCKOUT	0x00600000
#define CSR2_ERRORS	0xff000000

#define CSRV6200 0x20000000

/*
 * Data structure to keep track of the previous machine check information.
 * One structure per processor.
 */
struct	xcp_machdep_data {
long	mchk_in_progress;	/* Flag set while we are running mcheck code */
long	time;			/* time stamp of the machine check */
long	code;			/* machine check code		   */
long	maddr;			/* most recent memory address	   */
long	istate1;		/* internal state 1		   */
long	istate2;		/* internal state 2		   */
long	pc;			/* program counter		   */
long	psl;			/* PSL				   */

struct el_xcpsoft xcpsoft;
};

/* Values for type field */
#define	MCHK_9000_TYPE_SPU 1	/* Error generated by SPU (use regular
				 * stack frame to decode)
				 */

#define MCHK_9000_TYPE_EBOX 0	/* Error generated by EBOX (use short
				 * stack frame to decode)
				 */


/* Bit-mask definitions for ERR SUMM field */

/* Error conditions */
#define MCHK_9K_ERR_REG_PE	0x00000020	/* Register parity error */
#define MCHK_9K_ERR_MB_ADDR_FLT 0x00020000	/* MBOX address failure */
#define MCHK_9K_ERR_MB_DATA_FLT	0x00040000	/* MBOX data failure */
#define MCHK_9K_ERR_MB_WRBK_FLT	0x00080000	/* MBOX writeback failure */
#define MCHK_9K_ERR_IB_PEND_GPR 0x00100000	/* IBOX pending GPR mod fail */
#define MCHK_9K_ERR_IB_LOST_PC  0x00200000	/* IBOX lost PC */
#define MCHK_9K_ERR_RDS		0x00400000	/* Double-bit memory error */
#define MCHK_9K_ERR_BAD_DATA	0x00800000	/* Bad data */
#define MCHK_9K_ERR_VBOX_ABORT	0x01000000	/* VBOX abort */
#define MCHK_9K_ERR_VBOX_REG_PE 0x02000000	/* VBOX register parity error */

/* Qualifiers */
#define MCHK_9K_ERR_SOLID	0x00000001	/* Hard error */ 
#define MCHK_9K_ERR_INSERTION	0x40000000	/* Err insertion in progress */

/* Side-effects of error condition */
#define MCHK_9K_ERR_MEM_MOD	0x00000002 	/* Instr'n modified memory */
#define MCHK_9K_ERR_REG_MOD	0x00000004	/* Instr'n modified GPR */
#define MCHK_9K_ERR_IO_REF	0x00000008	/* Instr'n did IO ref */
#define MCHK_9K_ERR_FPD		0x00000010	/* PSL<FPD> set at failure */
#define MCHK_9K_ERR_MB_PEND_WRT	0x00010000	/* MBOX wrt pending when
						 * error occurred
						 */

/* Field definitions in SYS SUMM field */
#define	MCHK_9K_SYS_MBOX	0x0000000f	/* MBOX in error */
#define MCHK_9K_SYS_IBOX	0x000000f0	/* IBOX in error */
#define MCHK_9K_SYS_EBOX	0x00000f00	/* EBOX in error */
#define MCHK_9K_SYS_VBOX	0x0000f000	/* VBOX in error */
#define MCHK_9K_SYS_MEM		0x00030000	/* MMU in error */
#define MCHK_9K_SYS_SCU		0x00040000	/* SCU in error */

/* Field definitions in MISC INFO field */
#define MCHK_9K_MISC_PA_EXT	0x00000003	/* PA<33:32> */
#define MCHK_9K_MISC_PA_VALID	0x00000004	/* Set if paddr valid */
#define MCHK_9K_MISC_VA_VALID	0x00000008	/* Set if vaddr valid */
#define MCHK_9K_MISC_REGNUM	0x00007ff0	/* Register accessed */
#define MCHK_9K_MISC_RN_VALID	0x00008000	/* REGNUM is valid */

/* Structure to keep track of soft errors on a module (CPU, VBOX,
 * or memory).
 */

struct	mc9000_softerr_stats {
	int		err_cnt;	/* Number of errors on this module */
	unsigned long lasterr_time;	/* Time of most recent error */
};

/* Soft error parameters (when soft errors exceed MCHK_9K_MAX_ERRS
 * in MCHK_9K_ERR_TIMEOUT TODR register ticks, we take drastic
 * action -- disable the module or crash the system)
 * (Current parameters:  3 errors in 3 minutes)
 */
#define	MCHK_9K_ERR_TIMEOUT	18000	/* 180 secs @ 10 ms/tick */
#define MCHK_9K_MAX_ERRS	3

/* Number of CPUs on a 9000 */
#define	NCPU_9K			4


/* Address at which we start CPU's (this was implicit on earlier
 * VAXen, but Aquarius lets us specify any address we want)
 */
#define	CPU_START_ADDR	0x100

/* The following definitions are specific to the VAX 9000
 * Service Processor Unit (SPU)
 */

/* RXFCT field definitions */
#define RXFCT_VALID     0x80000000      /* Valid request avail */
#define RXFCT_IE        0x40000000      /* Interrupt enable */
#define RXFCT_STATUS    0x20000000      /* Status bit */
#define RXFCT_STATUS_SHIFT      29      /* Bit position of status */
#define RXFCT_SPARAM    0x00ffff00      /* Function parameter */
#define RXFCT_SPARAM_SHIFT      8       /* Position of sparam field */
#define RXFCT_FUNCT     0x000000ff      /* Function code */

/* Macros to set/get the status and sparam fields of RXFCT */
#define RXFCT_SET_STATUS(rxfct, val) \
        (rxfct) |= (((val) << RXFCT_STATUS_SHIFT) & RXFCT_STATUS)

#define RXFCT_GET_STATUS(rxfct) \
        (((rxfct) & RXFCT_STATUS) >> RXFCT_STATUS_SHIFT)

#define RXFCT_SET_SPARAM(rxfct, val) \
        (rxfct) |= (((val) << RXFCT_SPARAM_SHIFT) & RXFCT_SPARAM)

#define RXFCT_GET_SPARAM(rxfct) \
        (((rxfct) & RXFCT_SPARAM) >> RXFCT_SPARAM_SHIFT)

/* RXFCT function definitions */
#define RXFCT_RM_CPU            2       /* Remove processor */
#define RXFCT_ADD_CPU           3       /* Add processor */
#define RXFCT_MEM_BAD           4       /* Mark memory page as bad */
#define RXFCT_ALLOC_PAGE        5       /* Allocate pages of memory */
#define RXFCT_SEND_ERRLOG       6       /* Send error log entry */
#define RXFCT_SEND_OPCOM        7       /* Send OPCOM message */
#define RXFCT_GET_DG            8       /* Get datagram buffer */
#define RXFCT_SEND_DG           9       /* Send datagram buffer */
#define RXFCT_RETURN_DG_STATUS  10      /* Return datagram status */
#define RXFCT_SET_KEEPALIVE     11      /* Set keepalive state */
#define RXFCT_ABORT_DATALINK    12      /* Abort datalinks */
#define RXFCT_ERROR             13      /* Error interrupt */

/* TXFCT field definitions */
#define TXFCT_RDY       0x80000000      /* Valid request avail */
#define TXFCT_IE        0x40000000      /* Interrupt enable */
#define TXFCT_STATUS    0x20000000      /* Status bit */
#define TXFCT_STATUS_SHIFT      29      /* Bit position of status */
#define TXFCT_SPARAM    0x00ffff00      /* Function parameter */
#define TXFCT_SPARAM_SHIFT      8       /* Position of sparam field */
#define TXFCT_FUNCT     0x000000ff      /* Function code */

/* Macros to set/get the status and sparam fields of TXFCT */

#define TXFCT_GET_STATUS(txfct) \
        (((txfct) & TXFCT_STATUS) >> TXFCT_STATUS_SHIFT)

#define TXFCT_SET_SPARAM(txfct, val) \
        (txfct) |= (((val) << TXFCT_SPARAM_SHIFT) & TXFCT_SPARAM)

#define TXFCT_GET_SPARAM(txfct) \
        (((txfct) & TXFCT_SPARAM) >> TXFCT_SPARAM_SHIFT)


/* TXFCT function definitions */
#define TXFCT_GET_HW_CXT        1       /* Get CPU hardware context */
#define TXFCT_VB_OP             2       /* Virtual Block file operation */
#define TXFCT_KEEPALIVE         3       /* Send keepalive request */
#define TXFCT_SEND_DG           4       /* Send datagram */
#define TXFCT_RETURN_DG_STATUS  5       /* Return datagram status */
#define TXFCT_SWITCH_PRIMARY    6       /* Switch primary processor */
#define TXFCT_REBOOT_SYSTEM     7       /* Reboot system */
#define TXFCT_CLEAR_WS          8       /* Clear warm start flag */
#define TXFCT_CLEAR_CS          9       /* Clear cold start flag */
#define TXFCT_BOOT_CPU          10      /* Boot secondary processor */
#define TXFCT_HALT_AND_RM       11      /* Halt CPU and remove it */
#define TXFCT_HALT_AND_KEEP     12      /* Halt CPU and keep it. */
#define TXFCT_CONSOLE_QUIET     14      /* Shut up, console! */
#define TXFCT_SET_INTMODE       15      /* Set interrupt mode */
#define TXFCT_ABORT_DATALINK    16      /* Abort datalink */
#define TXFCT_IO_RESET          17      /* Reset XJAs */
#define TXFCT_DISABLE_VBOX      18      /* Disable vector processor(s) */
#define TXFCT_SET_KEEPALIVE     19      /* Set keepalive state */
#define TXFCT_ENABLE_ERRLOG	20	/* Enable error log transmission */

/* Interrupt modes for TXFCT_SET_INTMODE */
#define INTMODE_ABSOLUTE        0       /* Absolute mode */
#define INTMODE_ROUNDROBIN      1       /* Round-robin mode */

/* File access block for Virtual Block Access system */
struct spu_vba_accessblk {
        short   vba_opcode;     /* Requested operation */
        short   vba_buflen;     /* Length of data buffer */
        long    vba_bufaddr;    /* Physaddr of buffer */
        long    vba_modifiers;  /* Options */
        long    vba_fileid;     /* File handle */
        long    vba_param;      /* Optional parameter */
};

/* Virtual Block Access opcodes and modifiers */
#define FCT_OPEN        0x0001
#define FCT_M_DIRECTORY 0x0001
#define FCT_M_RECORD    0x0002
#define FCT_READ        0x0002
#define FCT_WRITE       0x0003
#define FCT_CLOSE       0x0004

/* Virtual Block Access status codes */
#define SPU_SUCCESS     0x0001
#define SPU_FNF         0x0008
#define SPU_FAIL        0x0010
#define SPU_NOACCESS    0x0018
#define SPU_EXCQUOTA    0x0020
#define SPU_TRUNCATED   0x0028
#define SPU_EOF         0x0030

/* Data Message Descriptor.  This is used for General File Access,
 * Error Log messages, and OPCOM messages.
 */
struct dmd {
        long    dmd_link;               /* Link field; not used by SPU */
        long    dmd_paddr;              /* phys. addr of dmd hdr */
        char    dmd_proto;              /* Destination protocol */
        char    dmd_prio;               /* Message priority */
        short   dmd_reserved;           /* (reserved) */
        short   dmd_link_id;            /* Which link (channel) to use */
        char    dmd_msg_type;           /* Message type */
        char    dmd_msg_subtype;        /* Message subtype */
        short   dmd_buflen;             /* Buffer size (in bytes) */
        char    dmd_pfn_count;          /* Number of pages in data area*/
        char    dmd_reserved2;          /* (reserved) */
        long    dmd_bufaddr;            /* buffer virtual addr */
        long    dmd_pfn[1];             /* PFN list starts here */
};

/* Definitions for various DAP parameters used in DMDs */
#define DMD_PROTO_DAP           1       /* DAP protocol in use */
#define DMD_PRIO_NORMAL         0       /* Normal priority */
#define DMD_PROTO_HIGH          1       /* High priority */
#define DMD_MSGTYP_DATA         0       /* Data message */
#define DMD_MSGTYP_CONTROL      1       /* Control message */
#define DMD_SUBTYP_ACCESS       0
#define DMD_SUBTYP_DEACCESS     1

#define SPU_MAX_KEEPALIVE_MISS	6	/* Number of keepalive misses before we
					 * crash the system.
					 */
