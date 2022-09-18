
/*
 * 	@(#)mem.h	4.1	(ULTRIX)	7/2/90";
 */

/************************************************************************
 *									*
 *			Copyright (c) 1985 by				*
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
 * Modification History:
 *
 * 19-Jan-88 -- jaw add support for calypso cvax.
 *
 * 02-Apr-86 -- jaw  add support for nautilus console and memory adapter
 *
 * 19-Jun-85 -- jaw
 *	VAX8200 name change.
 *
 * 06-Jun-85 -JAW
 *	Move memory marcos to ~/vaxbi/bimemreg.h.
 *
 * 12-Mar-85 -JAW
 *	Changes for support of the VAX8200 were merged in.
 *
 * 27-Feb-85 -tresvik
 *	Changes for support of the VAX8600 were merged in.
 *
 */

/*
 * Memory controller registers
 *
 * The way in which the data is stored in these registers varies
 * per cpu, so we define macros here to mask that.
 */

struct	mcr {
	int	mc_reg[6];		/* This number varies per ctl */
};

/*
 * Compute maximum possible number of memory controllers,
 * for sizing of the mcraddr array.
 */
#if VAX8200 || VAX6200
#define MAXNMCR		15
#else
#if VAX780
#define	MAXNMCR		4
#else
#define	MAXNMCR		1
#endif
#endif

/*
 * For each processor type we define 5 macros:
 *	M???_INH(mcr)		inhibits further crd interrupts from mcr
 *	M???_ENA(mcr)		enables another crd interrupt from mcr
 *	M???_ERR(mcr)		tells whether an error is waiting
 *	M???_SYN(mcr)		gives the syndrome bits of the error
 *	M???_ADDR(mcr)		gives the address of the error
 */

#if VAX8600
/*
 * 8600 register bit definitions
 */
#define	M8600_ICRD	0x400		/* inhibit crd interrupts */
#define M8600_TB_ERR	0xf00		/* translation buffer error mask */
/*
 * MDECC register
 */
#define	M8600_ADDR_PE	0x080000	/* address parity error */
#define M8600_DBL_ERR	0x100000	/* data double bit error */
#define	M8600_SNG_ERR	0x200000	/* data single bit error */
#define	M8600_BDT_ERR	0x400000	/* bad data error */

/*
 * ESPA register is used to address scratch pad registers in the Ebox.
 * To access a register in the scratch pad, write the ESPA with the address
 * and then read the ESPD register.  
 *
 * NOTE:  In assmebly code, the the mfpr instruction that reads the ESPD
 *	  register must immedately follow the mtpr instruction that setup
 *	  the ESPA register -- per the VENUS processor register spec.
 *
 * The scratchpad registers that are supplied for a single bit ECC 
 * error are:
 */
#define	SPAD_MSTAT1	0x25		/* scratch pad mstat1 register	*/
#define SPAD_MSTAT2	0x26		/* scratch pad mstat2 register	*/
#define SPAD_MDECC	0x27		/* scratch pad mdecc register	*/
#define SPAD_MEAR	0x27		/* scratch pad mear register	*/

#define M8600_MEMERR (tmp_mdecc & 0x780000)
#define M8600_HRDERR (tmp_mdecc & 0x580000)
#define M8600_ENA (mtpr(MERG, (mfpr(MERG) & ~M8600_ICRD)))
#define M8600_INH (mtpr(EHSR, 0), mtpr(MERG, (mfpr(MERG) | M8600_ICRD)))
#define M8600_SYN ((tmp_mdecc >> 9) & 0x3f)
#define M8600_ADDR (tmp_mear & 0x3ffffffc)
#define M8600_ARRAY (mtpr(PAMLOC, (tmp_mear & 0x3ff00000)), (mfpr(PAMACC) & 0x1f));
#define M8600_MDECC_BITS "\20\27MULT_ERR\26BAD_DT_ERR\25SNG_BIT_ERR\
\24DBL_BIT_ERR\23DT_ADDR_PE"
#define M8600_MSTAT1_BITS "\20\30CPR_PE_A\27CPR_PE_B\26ABUS_DT_PE\
\25ABUS_CTL_MSK_PE\24ABUS_ADR_PE\23ABUS_C/A_CYCLE\22ABUS_ADP_1\21ABUS_ADP_0\
\20TB_MISS\17BLK_HIT\16C0_TAG_MISS\15CHE_MISS\14TB_VAL_ERR\13TB_PTE_B_PE\
\12TB_PTE_A_PE\11TB_TAG_PE\10WR_DT_PE_B3\7WR_DT_PE_B2\6WR_DT_PE_B1\
\5WR_DT_PE_B0\4CHE_RD_DT_PE\3CHE_SEL\2ANY_REFL\1CP_BW_CHE_DT_PE"
#define M8600_MSTAT2_BITS "\20\20CP_BYT_WR\17ABUS_BD_DT_CODE\10MULT_ERR\
\7CHE_TAG_PE\6CHE_TAG_W_PE\5CHE_WRTN_BIT\4NXM\3CP-IO_BUF_ERR\2MBOX_LOCK"
#endif /* VAX8600 */

#if VAX780

/* on a 780, memory crd's occur only when bit 15 is set in the SBIER */
/* register; bit 14 there is an error bit which we also clear */
/* these bits are in the back of the ``red book'' (or in the VMS code) */

/* The following definitions are common to MS780-C and MS780-E */

#define	M780_ICRD	0x40000000	/* inhibit crd interrupts, in [2] */
#define	M780_HIER	0x20000000	/* high error rate, in reg[2] */
#define	M780_ERLOG	0x10000000	/* error log request, in reg[2] */

/* The following are the definitions necessary to support MS780-C memory */

/* for MS780-C, check for RDS in SBIER and the ERLOG bit in the controller */
#define M780C_HRDERR(mcr) \
	((mfpr (SBIER) & 0x2000) && ((mcr)->mc_reg[2] & (M780_ERLOG)))
#define	M780C_INH(mcr)	\
	(((mcr)->mc_reg[2] = (M780_ICRD|M780_HIER|M780_ERLOG)), \
		mtpr(SBIER, 7<<13))
#define	M780C_ENA(mcr)	\
	(((mcr)->mc_reg[2] = (M780_HIER|M780_ERLOG)), mtpr(SBIER, 7<<13))
#define	M780C_ERR(mcr)	\
	((mcr)->mc_reg[2] & (M780_ERLOG))
#define	M780C_SYN(mcr)		((mcr)->mc_reg[2] & 0xff)
#define	M780C_ADDR(mcr)		(((mcr)->mc_reg[2] >> 8) & 0xfffff)
#define M780C_ARRAY(mcr)	(((mcr)->mc_reg[2] >> 24) & 0xf)

#define M780C_CSRA_BITS	"\20\40SBI_PAR\37WR_SEQ\35INTLK_SEQ\
\34MLT_XMT\33XMT_FLT\30PWR_DWN\27PWR_UP\5CHIP_16K\4CHIP_4K\1INTRLV"
#define M780C_CSRC_BITS	"\20\37INH_CRD\36HIGH_ERR\35ERRLOG_REQ"

/*
 * The following definitions are necessary to support the MS780-E memory.
 * The second parameter of the following is 0 or 1 to select the lower
 * or upper controller repectively
 */

#define M780E_CRD 0x00000200	/* single bit detect & corrected */
#define M780E_RDS 0x00000400	/* uncorrectable error */

/* for MS780-E, check for RDS in SBIER and the RDS bit in the controller */
#define M780E_HRDERR(mcr, ctlr) \
	((mfpr (SBIER) & 0x2000) && ((mcr)->mc_reg[2+ctlr] & (M780E_RDS)))
#define	M780E_INH(mcr, ctlr)	\
	(((mcr)->mc_reg[2+ctlr] = \
	(M780_ICRD|M780_HIER|M780_ERLOG|M780E_RDS|M780E_CRD)), \
		mtpr(SBIER, 7<<13))
#define	M780E_ENA(mcr, ctlr)	\
	(((mcr)->mc_reg[2+ctlr] = \
	(M780_HIER|M780_ERLOG|M780E_RDS|M780E_CRD)), \
		mtpr(SBIER, 7<<13))
#define	M780E_ERR(mcr, ctlr)	\
	((mcr)->mc_reg[2+ctlr] & (M780_ERLOG))

#define	M780E_SYN(mcr, ctlr)	((mcr)->mc_reg[2+ctlr] & 0x7f)
#define	M780E_ADDR(mcr, ctlr)	(((mcr)->mc_reg[2+ctlr] >> 11) & 0x1ffff)
#define M780E_ARRAY(mcr, ctlr)	(((mcr)->mc_reg[2+ctlr] >> 24) & 0x7)

#define M780E_CSRA_BITS	"\20\40SBI_PAR\37WR_SEQ\35INTLK_SEQ\
\34MLT_XMT\33XMT_FLT\30PWR_DWN\27PWR_UP\25ERR_SUM\24CNTR1_PAR\
\23CNTR0_PAR\22MISCONFIG\21CNTR1_MIS\20CNTR0_MIS\5CHIP_256K\4CHIP_64K\
\3INT_INTRLV\2UPPER\1EXTERNAL"
#define M780E_CSRC_BITS	"\20\40FORCE_PAR\37INH_CRD\36HIGH_ERR\35ERRLOG_REQ\
\13RDS\12CRD\10USEQ_PAR"

#endif

#if VAX750
#define	M750_RDS	0x80000000	/* uncorrectable error, in [0] */
#define M750_RDSLOST	0x40000000	/* uncorrectable error lost, in [0] */
#define	M750_CRD	0x20000000	/* correctable error, in [0] */
#define	M750_ENACRD	0x10000000	/* enable crd interrupts, in [1] */

#define	M750_INH(mcr)	((mcr)->mc_reg[1] = 0)
#define	M750_ENA(mcr)	((mcr)->mc_reg[0] = (M750_RDS|M750_RDSLOST|M750_CRD), \
			    (mcr)->mc_reg[1] = M750_ENACRD)
#define	M750_ERR(mcr)	((mcr)->mc_reg[0] & (M750_RDS|M750_CRD))
#define M750_HRDERR(mcr) ((mcr)->mc_reg[0] & M750_RDS)
#define	M750_SYN(mcr)	((mcr)->mc_reg[0] & 0x7f)
#define	M750_ADDR(mcr)	(((mcr)->mc_reg[0] >> 9) & 0x7fff)
#define M750_CSR0_BITS	"\20\40RDS\37LOST_ERR\36CRD"
#define M750_CSR1_BITS	"\20\35ENA_CRD\34PG_MODE\33DIAG_MODE\32ECC_DISABL"

#endif

#if VAX730
#define	M730_CRD	0x40000000	/* crd, in [1] */
#define	M730_ENACRD	0x10000000	/* enable crd interrupt, in [1] */
#define	M730_MME	0x08000000	/* mem-man enable (ala ipr), in [1] */

#define	M730_INH(mcr)	((mcr)->mc_reg[1] = M730_MME)
#define	M730_ENA(mcr)	((mcr)->mc_reg[1] = (M730_MME|M730_ENACRD))

/*
 * If the first reg has anything but zero then there is an error to be
 * logged.  This register is zeroed upon reading which is of course done when
 * errors are logged.  The 730 sure is different!!
 */

#define	M730_ERR(mcr)	((mcr)->mc_reg[0] & 0xffffffff) 
#define	M730_SYN(mcr)	((mcr)->mc_reg[0] & 0x7f)

/* If CRD is set then call it soft otherwise call it hard */

#define M730_HRDERR(mcr)   (((mcr)->mc_reg[1] & M730_CRD) ? 0 : 1)
#define	M730_ADDR(mcr)	(((mcr)->mc_reg[0] >> 9) & 0x7fff)
#define M730_CSR1_BITS	"\20\40UB_RDS\37CRD\36TB_DIAG\35ENA_CRD\34MME\
\33DIAG_CHK\32ECC_DIS\21UB_NXM\20UB_MAP_PAR\17UB_WR_ERR"
#define M730_CSR2_BITS	"\20\40IDC_PRESENT\31MEM_SIZ"

#endif

#define	MEMINTVL	(60*15)		/* 15 minutes */

#ifdef	KERNEL
int	nmcr;
struct {
    enum memtype {
	MEMTYPE_MS780C,
	MEMTYPE_MS780E,
	MEMTYPE_MS750,
	MEMTYPE_MS730,
	MEMTYPE_BI1,
	MEMTYPE_MS8800,
	MEMTYPE_MS6200,
    } memtype;
    struct mcr *mcraddr;
} mcrdata[MAXNMCR];
#endif
