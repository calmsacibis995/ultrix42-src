/*
 * 	@(#)cvax.h	4.2	(ULTRIX)	9/6/90";
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
 * Modification History:	cvax.h
 *
 * DATE	dlh
 *	modify part of the cca structure to bring it up to date.  this was
 *	necessary when vector support was added for rigel (VAX 6000-400 
 *	or VAX6400)
 *
 * 01-Sep-88	darrell
 *	Changed the definition of ctsi to ctsia, and fixed the definition
 *	of a field in the module descriptor structure.
 *
 * 02-25-88	darrell
 *	Creation of this file.
 *
 **********************************************************************/


/*
 * VAX3600 (ka650) System Support Chip (SSC) registers
 * At 0x2014 0000 in Local Register space; mapped to "cvqssc"
 */
struct ssc_regs
{
	u_long ssc_sscbr;	/* SSC Base Addr Register		      */
	u_long ssc_pad1[3];	/* filler				      */
	u_long ssc_ssccr;	/* SSC Configuration Register		      */
	u_long ssc_pad2[3];	/* filler				      */
	u_long ssc_cbtcr;	/* CDAL Bus Timeout Control Register	      */
	u_long ssc_pad3[3];	/* filler				      */
	u_long ssc_output;	/* Output Port				      */
	u_long ssc_pad4[14];	/* filler				      */
	u_long ssc_toy;		/* time of year clock reg		      */
	u_long ssc_csrs;	/* Console Storage Receiver Status	      */
	u_long ssc_csrd;	/* Console Storage Receiver Data	      */
	u_long ssc_csts;	/* Console Storage Transmitter Status	      */
	u_long ssc_cstd;	/* Console Storage Transmitter Data	      */
	u_long ssc_crcs;	/* Console Receiver Control/Status	      */
	u_long ssc_crdb;	/* Console Receiver Data Buffer		      */
	u_long ssc_ctcs;	/* Console Transmitter Control/Status	      */
	u_long ssc_ctdb;	/* Console Transmitter Data Buffer	      */
	u_long ssc_pad5[19];	/* filler				      */
	u_long ssc_ioreset;	/* I/O System Reset Register		      */
	u_long ssc_pad6[4];	/* filler				      */
	u_long ssc_rdr;		/* Rom Data Register			      */
	u_long ssc_btc;		/* Bus Timeout Counter			      */
	u_long ssc_it;		/* Interval Timer			      */
	u_long ssc_pad7[1];	/* filler				      */
	u_long ssc_tcr0;	/* timer control reg 0			      */
	u_long ssc_tir0;	/* timer interval reg 0			      */
	u_long ssc_tnir0;	/* timer next interval reg 0		      */
	u_long ssc_tivr0;	/* timer interrupt vector reg 0		      */
	u_long ssc_tcr1;	/* timer control reg 1			      */
	u_long ssc_tir1;	/* timer interval reg 1			      */
	u_long ssc_tnir1;	/* timer next interval reg 1		      */
	u_long ssc_tivr1;	/* timer interrupt vector reg 1		      */
	u_long ssc_pad8[184];	/* pad to 0x20140400 for CPMBX		      */
	u_char ssc_cpmbx;	/* Console Program Mail Box: Lang & Hact      */
	u_char ssc_terminfo;	/* TTY info: Video Dev, MCS, CRT & ROM flags  */
	u_char ssc_keyboard;	/* Keyboard code			      */
	u_char :8;		/* filler				      */
	u_long ssc_pad9[67];	/* filler				      */
	u_long ssc_cca_addr;	/* Physical address of CCA		      */
	u_long ssc_ctsi_addr;	/* Physical address of CTSIA		      */
};

/*
 * VAX3300  and Firefox MSI (DSSI) I/O space.
 */
struct sii_regs
{
	u_long sii_msidr0;	/* MSI Diag. Register 0			*/
	u_long sii_msidr1;	/* MSI Diag. Register 1			*/
	u_long sii_msidr2;	/* MSI Diag. Register 2			*/
	u_long sii_msicsr;	/* MSI Control and Status register.	*/
	u_long sii_msiid;	/* MSI ID register.			*/
	u_long sii_msislcs;	/* MSI Selector control/status 		*/
	u_long sii_msidestat;	/* MSI Selection detection status	*/
	u_long sii_msitr;	/* MSI Timeout Register.		*/
	u_long sii_msidata;	/* MSI Data register			*/
	u_long sii_msidmctlr;	/* MSI DMA control register		*/
	u_long sii_msidmlotc;	/* MSI DMA length to xfer		*/
	u_long sii_msidmaddrl;	/* MSI DMA address pointer		*/
	u_long sii_msidmaddrh;	/* MSI DMA address pointer		*/
	u_long sii_msidmabyte;	/* MSI DMA initial byte			*/
	u_long sii_msistlp;	/* MSI Short Target List Pointer	*/
	u_long sii_msiltlp;	/* MSI Long Target List Pointer		*/
	u_long sii_msiilp;	/* MSI Initiator List Pointer		*/
	u_long sii_msicr;	/* MSI (DSSI) Controll Register		*/
	u_long sii_msisr;	/* MSI (DSSI) Status Register		*/
	u_long sii_msidstat;	/* Data interupt control Register	*/
	u_long sii_msicomm;	/* MSI Command Register			*/
	u_long sii_msidcr;	/* MSI Diag. Control Register		*/
	u_long sii_mscccr;	/* MSI Clock Control Register		*/
	u_long sii_msiisr0;	/* MSI Internal State Register 0	*/
	u_long sii_msiisr1;	/* MSI Internal State Register 1	*/
	u_long sii_msiisr2;	/* MSI Internal State Register 2	*/
	u_long sii_msiisr3;	/* MSI Internal State Register 3	*/
};

/*
 * ni_regs describes three disjoint physical address spaces.  We
 * map them virtually contiguous on Firefox and Mayfair II so that
 * both Firefox and Mayfair II can use the same NI driver.
 */
struct ni_regs
{
	u_long ni_rdp;		/* NI Register Data Port		      */
	u_long ni_rap;		/* NI Register Address Port		      */
	u_long pad1[126];	/* filler				      */
	u_long ni_sar[32];	/* NI Station Address ROM		      */
	u_long pad2[96];	/* Reserved (4 copies of the NISA rROM)	      */
	u_long ni_nilrb[32768];	/* NI Local RAM Buffer			      */
};
						
/*
 * siibuf the MSI (DSSI) RAM buffer on Mayfair II and Firefox.
 */
struct siibuf
{
	u_long siibuf_msirb[32768];	/* MSI Buffer RAM		      */
};

/*
 * VAX3600 (ka650) Qbus map registers
 * At 0x2008 8000 in Local Register space; mapped to "cvqbm"
 */
struct cvqbm_regs
{
	union {
		struct {
			long qb_pad[512];	  /* need 2k bytes	      */
			struct pte qb_map[8192];  /* q-bus map registers      */
		} cqba; 
		struct uba_regs uba;
	} cvqbm_uba;
};

/*
 * Machine Check codes for CVAX CPUs.
 * Defines for number of machine check codes (in following array)
 *    and the index number of first disjoint code.
 */
#define NMCcVAX 15
#define MCcVAXDISJ 11
struct mcCVAXframe {
	int	mc1_bcnt;			/* byte count == 0xc */
	int	mc1_summary;			/* summary parameter */
	int	mc1_vap;			/* most recent virtual addr */
	int	mc1_internal_state1;		/* internal state 1 */
	int	mc1_internal_state2;		/* internal state 2 */
	int	mc1_pc; 			/* trapped pc */
	int	mc1_psl;			/* trapped psl */
};

/*
 * CQBIC registers
 * Note:  The registers refered to as: Master Error Address Register 
 *	  (cq_mear) and Slave Error Address Register (cq_sear)
 *	  are called cvq1_qbear and cvq1_dear in the cvq1_regs 
 *	  structure;
 */
struct cqbic_regs {
	u_long cq_scr;	/* System Configuration Register	      */
	u_long cq_dser;	/* DMA System Error Register		      */
	u_long cq_mear;	/* Master Error Address Register (DMA)	      */
	u_long cq_sear;	/* Slave Error Address Register	 (DMA)	      */
	u_long cq_map_base;	/* Q Bus Map Base address Register	      */
};

/*
 * These structures are used to keep track of the frequency of errors.
 * We keep the time of the last 2 errors for each category.
 * When a third error occurs, if the time elapsed since the "prev" one
 *   is less than 1 second, then we got 3 errors (prev, last, current)
 *   within 1 second.
 *
 * 				   used with following error bit... 
 */
struct cfpa_errcnt {		/* machine checks 1 thru 4 */
	u_int	cfpa_last;	/* time of most recent CFPA error */
	u_int	cfpa_prev;	/* time of previous CFPA error */
};

struct cdal_errcnt {		/* MSER_MCD */
	u_int	cdal_last;	/* time of most recent CDAL parity error */
	u_int	cdal_prev;	/* time of previous CDAL parity error */
};

struct cache_errcnt {		/* MSER_MCC */
	u_int	cache_last;	/* time of most recent 1st level cache parity error */
	u_int	cache_prev;	/* time of previous 1st level cache parity error */
};

struct qnxm_errcnt {		/* DSER_QNXM */
	u_int	qnxm_last;	/* time of most recent Q-22 Bus NXM */
	u_int	qnxm_prev;	/* time of previous Q-22 Bus NXM */
};

struct qngr_errcnt {		/* DSER_NOGRANT */
	u_int	qngr_last;	/* time of most recent Q-22 Bus No Grant timeouts */
	u_int	qngr_prev;	/* time of previous Q-22 Bus No Grant timeouts */
};

struct qpe_errcnt {		/* DSER_QPE */
	u_int	qpe_last;	/* time of most recent Q-22 Bus Device Parity err */
	u_int	qpe_prev;	/* time of previous Q-22 Bus Device Parity err */
};

struct dnxm_errcnt {		/* DSER_DNXM */
	u_int	dnxm_last;	/* time of most recent DMA NXM */
	u_int	dnxm_prev;	/* time of previous DMA NXM */
};

struct crd_errcnt {		/* MEM_CRD */
	u_int	crd_last;	/* time of most recent CRD err */
	u_int	crd_prev;	/* time of previous CRD err */
};

struct cdalW_errcnt {		/* MEM_CDAL */
	u_int	cdalW_last;	/* time of most recent CDAL write err */
	u_int	cdalW_prev;	/* time of previous CDAL write err */
};

struct rdsW_errcnt {		/* MEM_RDS */
	u_int	rdsW_last;	/* time of most recent RDS write err */
	u_int	rdsW_prev;	/* time of previous RDS write err */
};

struct tag_errcnt {		/* CACR_CPE */
	u_int	tag_last;	/* time of most recent 2nd lev cache tag parity err */
	u_int	tag_prev;	/* time of prev 2nd lev cache tag parity err */
};

/*
 * VAX3600 (ka650)
 * Physical start address of the Qbus memory.
 * The q-bus memory size is 4 meg.
 * Physical start address of the I/O space (where the 8Kbyte I/O page is).
 */
#define QMEMCVQ		((char *)(0x30000000))
#define QMEMSIZECVQ	(512*8192)
#define QDEVADDRCVQ	((u_short *)(0x20000000))

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
 * This is the Qbus Map Registers (8192 long words, 32K bytes, 64 pages),
 *    PLUS the Unibus config registers (2K bytes, 4 pages).
 *
 * Note: the address 0x2008 7800 is 2Kbytes (0x800 bytes) before the address
 *	of the Qbus map registers (0x2008 8000).
 */
#define CVQBMADDR	((short *)(0x20087800))
#define CVQBMSIZE	(512*68)

/*
 * VAX3600 (ka650): Mapping info for Fourth 'chunk' of Local Regs (3 pages)
 */
#define CVQSSCADDR	((short *)(0x20140000))
#define CVQSSCSIZE	(512*3)

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
 * Time limits for errors.  Recoverable errors are fatal if 3 errors occur
 * within the time period.  All times are given in 10 ms units (100ths of secs)
 * to be used with the 10ms units of the standard VAX TODR.
 */
#define TIME_THRESH	100		/* 1 sec max for most error types */
#define TIME_THRESH_C1	6000		/* 60 sec max for 1st lev cache errs */
#define TIME_THRESH_C2	30000		/* 5 mins max for 2nd lev cache errs */

/*
 * External definitions for the module where each of these are used.
 */
extern struct pte CVQSSCmap[];		/* maps to virtual ssc_regs	      */
extern struct ssc_regs cvqssc[];	/* SSC regs			      */
extern struct pte CVQBMmap[];		/* maps to virtual cvqbm	      */
extern struct cvqbm_regs cvqbm[];	/* Qbus map registers		      */
extern struct cqbic_regs ffqregs[];	/* Firefox CQBIC registers	      */
extern struct ni_regs cvqni[];		/* Firefox NI regs, SA ROM & RAM buf  */
extern struct pte CVQNImap[];
extern struct sii_regs cvqmsi[];	/* Firefox DSSI registers	      */
extern struct pte CVQMSImap[];
extern struct siibuf cvqmsirb[];	/* Firefox DSSI RAM buffer	      */
extern struct pte CVQMSIRBmap[];
extern struct ctsi ctsi[];		/* CTSIA (needed for Firefox LEGSS)   */
extern struct pte CTSImap[];
extern char fgctsixs[];		/* CTSIA extended driver state	      */
extern struct pte FGCTSIXSmap[];

/* timers & counters for errors:    used with following error bit... */

extern struct cfpa_errcnt cfpa_errcnt;		/* machine checks 1 thru 4 */
extern struct cdal_errcnt cdal_errcnt;		/* MSER_MCD */
extern struct cache_errcnt cache_errcnt;	/* MSER_MCC */
extern struct qnxm_errcnt qnxm_errcnt;		/* DSER_QNXM */
extern struct qngr_errcnt qngr_errcnt;		/* DSER_NOGRANT */
extern struct qpe_errcnt qpe_errcnt;		/* DSER_QPE */
extern struct dnxm_errcnt dnxm_errcnt;		/* DSER_DNXM */
extern struct crd_errcnt crd_errcnt;		/* MEM_CRD */
extern struct cdalW_errcnt cdalW_errcnt;	/* MEM_CDAL */
extern struct rdsW_errcnt rdsW_errcnt;		/* MEM_RDS */
extern struct tag_errcnt tag_errcnt;		/* CACR_CPE */

extern char *mcCVAX[];

#define CCA_V_BOOTIP 0x1		/* set: bootstrap being attempted */
#define CCA_V_WARMIP 0x2		/* moved - where ??? */
#define CCA_V_REBOOT 0x10		/* set: OS is requesting a reboot */
#define CCA_V_NO_2ND_TEST  0x4		/* set: ??? */

struct cca {
	char	*cca_base;		/* 4B: phs addr of cca */
	short	cca_size;		/* 2B: size (in bytes) of cca */
	char	cca_indent0;		/* 1B: ascii id (char 'C') */
	char	cca_indent1;		/* 1B: ascii id (char 'C') */
	char	cca_nproc;		/* 1B: max # procs which can be */
					/*     supported by this cca */
	char	cca_chksum;		/* 1B: chksum of 1st 9 bytes */
	char	cca_hflag;		/* 1B: flags:                        */
					/*	CCA_V_BOOTIP		0x01 */
					/*	CCA_V_USE_PCASHE	0x02 */
					/*	CCA_V_USE_BCASHE	0x04 */
					/*	CCA_V_BCASHE_CLEARABLE	0x08 */
					/*	CCA_V_REBOOT		0x10 */
					/*	CCA_V_REPROMPT		0x20 */
					/*	CCA_V_CON_REBOOT	0x40 */
	char	cca_b_revision;		/* 1B: rev # for cca format */
	unsigned long	cca_ready;	/* 8B: bitmask of processors which */
	unsigned long	cca_ready1;	/*     have data posted in trans buff */
	unsigned long	cca_console;	/* 8B: bitmask of processors known */
	unsigned long	cca_console1;	/*     to be in console mode */
	unsigned long 	cca_enabled;	/* 8B: bitmask of which processors */
	unsigned long 	cca_enabled1;	/*     are enabled to leave console */
					/*     mode */
	unsigned long	cca_bitmap_sz;	/* 4B: size in bytes of phys mem */
					/*     bitmap */
	unsigned long	cca_bitmap;	/* 4B: phys addre of phys mem bitmap */
	unsigned long 	cca_bitmap_cksum; /* 4B: n/a in XRP (Rigel) */
	unsigned long 	cca_tk50node;	/* 1B: (+3B spare) node numbers */
					/*	bits 4-7 : XMI node number */
					/*	bits 0-3 : BI node number */
	unsigned long 	cca_secstart;	/* 8B: bitmask of processors */
	unsigned long 	cca_secstart1;	/*     currently being started by */
					/*     primary processor */
	unsigned long 	cca_restartip;	/* 8B: bitmap of processors */
	unsigned long 	cca_restartip1;	/*     currently attempting restarts */
	unsigned long 	cca_resv1;	/* 24B: 3 longwords of reserved space */
	unsigned long 	cca_resv2;
	unsigned long 	cca_resv3;
	unsigned long 	cca_user_halted; /* 8B: bitmask of processors which */
	unsigned long 	cca_user_halted1; /*    entered consolke mode as a */
					  /*    result of a ^P */
	union cca_rev_dependent {
		struct {
			char            cca_serialnum[12];
	
			struct {
				char cvax_rev;
				char sccs_rev;
				char fpa_rev;
				char compat;
				int module_rev;
			} cca_revs[16];
	
			char    pad[284];
		} cca_rev3;
		struct {
			char            cca_serialnum[8];
					/* 8B: least significant 8 chars */
					/*     of serial number stored in */
					/*     EEPROM */
			struct {	/* 16*8B=128B: array of 16 quadwords */
					/*   containing chip and module */
					/*   revision info for processors in */
					/*   the system */
				char reserved1; /* 1B */
				char reserved2; /* 1B */
				char reserved3; /* 1B */
				char compat;    /* 1B: compatablility group?? */
				char module_rev[4]; /* 4B: module rev */
			} cca_revs[16];
			unsigned long cca_vec_enabled1;
			unsigned long cca_vec_enabled2;
					/* 8B: bitmask of processors which */
					/*     have enabled vector processors */
			unsigned long cca_vec_present1;
			unsigned long cca_vec_present2;
					/* 8B: bitmask of processors which */
					/*     have working vector processors */
			struct {	/* 16*4B=64B: vector processor rev */
					/*     info */
				char	cca_module_rev;
						/* 1B: binary copy of vector */
						/*     module # */
				char	cca_vectl_rev;
						/* 1B: binary copy of vectl */
						/*     chip rev */
				short	reserved;
						/* 2B: reserved */
				} cca_vec_revision[16];
			char    pad[208];
		} cca_rev4;
	} cca_rev_dependent;
	struct {		/* 64*168B=10752B: once buffer area for */
				/*     each possible XMI node */
		char	flags;	/* 1B: status flag */
				/*     RXRDY - a complete message is in rx */
		char	zdest;	/* 1B: if set then XMI node # sending to */
		char	zsrc;	/* 1B: if set then XMI node # receiving from */
		char	spare;	/* 1B: */
		char 	txlen;	/* 1B: length of message in tx */
		char	rxlen;	/* 1B: length of message in rx */
		short	zrxcd;	/* 1B: char-at-a-time communicaiton */
		char	tx[80];	/* 80B: buffer for transmit to primary */
		char	rx[80];	/* 80B: buffer for receipt from primary */
	} cca_buf[64];

};
struct cca ccabase;
#define RXRDY 0x1

struct ctsi_chan_blk {
	u_char	ctcb_dvatr;		/* Device attributes byte	      */
	u_char	ctcb_chatr;		/* Channel attributes byte	      */
	u_short	ctcb_statesize;		/* Driver state size		      */
	char	*ctcb_entry_p;		/* Driver entry point physical addr   */
	char	*ctcb_entry_v;		/* Driver entry point virtual addr    */
	char	*ctcb_iosegtbl_p;	/* I/O Segment table physical addr    */
	char	*ctcb_iosegtbl_v;	/* I/O Segment table virtual addr     */
	char	*ctcb_extdvrstate_p;	/* Extended Driver State physical addr*/
	char	*ctcb_extdvrstate_v;	/* Extended Driver State virtual addr */
};

struct ctsi_mode_desc {
	u_short	ctmd_pgcnt;		/* Module Descriptor page count	      */
	u_short :16;			/* unused			      */
	char	*ctmd_baseaddr;		/* Base Address of Firmware Module    */
};

struct ctsi_getchar_state {
	u_char	ctgs_flags;		/* Flags			      */
	u_char	ctgs_kbd;		/* Keyboard			      */
	u_short	:16;			/* unused			      */
	u_long	pad[3];			/* Reserved			      */
};

struct ctsi_putchar_state {
	u_char ctps_flags;		/* Flags			      */
	u_char	:8;			/* unused			      */
	u_short	:16;			/* unused			      */
	u_long pad[3];			/* unused			      */
};

struct ctsi {
	u_long	ct_base;		/* phys addr of base of CTSIA	      */
	u_short	ct_size;		/* size in bytes of the CTSIAZZ	      */
	u_short	ct_ident;		/* The ASCII characters "CT"	      */
	u_char	:8;			/* unused			      */
	u_char	ct_cksum;		/* Checksum			      */
	u_char	ct_flags;		/* Flags used by CTSIA routines	      */
	u_char	ct_revisn;		/* Rev. number  for the CTSIA format  */
	struct ctsi_mode_desc	ct_cons0;	/* Console Mod Descriptor 0   */
	struct ctsi_mode_desc	ct_cons1;	/* Console Mod Descriptor 1   */
	struct ctsi_mode_desc	ct_rcons0;	/* Remote Console Desc. 0     */
	struct ctsi_mode_desc	ct_rcons1;	/* Remote Console Desc. 1     */
	struct ctsi_mode_desc	ct_fcons;	/* Fallback Conssole Desc.    */
	struct ctsi_mode_desc	ct_ucons;	/* Utility Console Descriptor */
	struct ctsi_chan_blk	ct_std_in;	/* Standard Input Channel     */
	struct ctsi_chan_blk	ct_std_out;	/* Standard Output Channel    */
	struct ctsi_chan_blk	ct_fb_in;	/* Fallback Input Channel     */
	struct ctsi_chan_blk	ct_fb_out;	/* Fallback Output Channel    */
	struct ctsi_chan_blk	ct_rmot_in;	/* Remote Input Channel	      */
	struct ctsi_chan_blk	ct_rmot_out;	/* Remote Output Channel      */
	char	*ct_save_p;		/* SAVE physical address	      */
	char	*ct_save_v;		/* SAVE virutal address		      */
	char	*ct_restore_p;		/* RESTORE physical address	      */
	char	*ct_restore_v;		/* RESTORE virtual address	      */
	char	*ct_translate_p;	/* TRANSLATE physical address	      */
	char	*ct_translate_v;	/* TRANSLATE virtual address	      */
	char	*ct_getchar_p;		/* GET_CHARACTER physical address     */
	char	*ct_getchar_v;		/* GET_CHARACTER virtual address      */
	struct ctsi_getchar_state ct_getchar_state; /* GET_CHARACTER state   */
	char	*ct_putchar_p;		/* PUT_CHARACTER physical address     */
	char	*ct_putchar_v;		/* PUT_CHARACTER virtual address      */
	struct ctsi_putchar_state ct_putchar_state; /* PUT_CHARACTER state   */
	char	*ct_msgout_p;		/* MESSAGE_OUT physical address	      */
	char	*ct_msgout_v;		/* MESSAGE_OUT virtual address	      */
	char	*ct_rwp_p;		/* READ_WITH_PROMPT physical address  */
	char	*ct_rwp_v;		/* READ_WITH_PROMPT virtual address   */
};
