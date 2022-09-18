#ifndef BI_INCLUDE
#define BI_INCLUDE 1


/*
 * 	@(#)bireg.h	4.1	(ULTRIX)	7/2/90";
 */
/************************************************************************
 *									*
 *			Copyright (c) 1985,86,87 by			*
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


/* ------------------------------------------------------------------------
 * Modification History: /sys/vaxbi/bireg.h
 *
 * 12-11-87	Robin L. and Larry C.
 *	Added portclass support to the system.
 *
 * 11-Mar-87 -- map
 *                      Added biinfo structure to bidata. Used for bvp
 *                      locking during adapter reset.
 *
 * 10-Jul-86   -- jaw	added adpt/nexus to ioctl
 *
 * 	5-Jun-86   -- jaw 	add bi noarb define.
 * 	5-Jun-86   -- jaw 	changes to config.
 *
 *	08-May-86 -- lp
 *		     AIE can have two different device types. One says
 *		     functioning TK50 firmware is present while the
 *		     other says its not.
 *
 *	04-Apr-86 -- afd
 *		     Added BIVECSIZE constant; and fixed parentheses in
 *		     SCB_BI_VEC_ADDR.
 *
 * 	05-Mar-86 -- jaw  VAXBI device and controller config code added.
 *		     todr code put in cpusw.
 *
 * 	18-Mar-86 -- jaw  add routines to cpu switch for nexus/unibus addreses
 *		     also got rid of some globals like nexnum.
 *		     ka8800 cleanup.
 *
 * 	04-feb-86 -- jaw  get rid of biic.h.
 *
 *	03-Feb-86 -- jaw  added SCB macros.
 *
 *	26-Oct-85 -- jaw  MMR is really NMR.
 *
 *	03-Sep-85 -- jaw  mod of error interrupts.
 *
 * 	19-Jun-85 -- jaw  VAX8200 name change.
 *
 *	05 Jun 85 -- jaw  cleanup...
 *
 *	20 Mar 85 -- jaw  add support for Vax 8200
 *
 * ------------------------------------------------------------------------
 */

/*
	BI options information for "biprobe".
	
 */
#ifndef LOCORE

struct bisw {

	short	bi_type;		/* bi device type */
	char	*bi_name;		/* name of the device*/
	int	(**probes)();
	int	(*bi_reset)();		/* reset routine for device */
	short	bi_flags;		/* Hard init enable (write to SST) */
};


/*  bi flags */
#define BIF_SST 0x1			/* do node reset before call init */
#define BIF_SET_HEIE 0x2		/* if set don't enable HES */
#define BIF_DEVICE 0x4			/* is a device in the config file */
#define BIF_CONTROLLER 0x8		/* is a controller in config file */
#define BIF_ADAPTER  0x10		/* adapters...uba's etc */
#define BIF_NOCONF 0x1000		/* Isn't config'd */

/*
	required registers
*/
struct bi_regs
{
	long	bi_typ;			/* device type register */
	long	bi_ctrl;		/* Control and status */
	long	bi_err;			/* Error summary */
	long	bi_err_int;		/* Error interrupt control */
	long	bi_int_dst;		/* interrupt destination mask */
	
};
/*
 	BI info....

*/
#define SCB_BI_OFFSET(binumber) \
	(((int)bidata[binumber].bivec_page << 1) \
	   - ((int)&scb.scb_stray << 1)) >> 1

#define SCB_BI_ADDR(binumber) \
	((bidata[binumber].bivec_page))

#define SCB_BI_LWOFFSET(bi_nodenum,level) \
	((bi_nodenum << 2) | level) 

#define SCB_BI_VEC_ADDR(binumber,bi_nodenum,level) \
	((bidata[binumber].bivec_page)+(((bi_nodenum << 2) | level)/4))

struct biic_regs
{
	long	biic_typ;		/* device type register */
	long	biic_ctrl;		/* Control and status */
	long	biic_err;		/* Error summary */
	long	biic_err_int;		/* Error interrupt control */
	long	biic_int_dst;		/* interrupt destination mask */
	long	biic_ip_msk;		/* IP interrupt mask */
	long	biic_ip_dst;		/* IP destination mask */
	long	biic_ip_src;		/* IP interrupt source */
	long	biic_strt;		/* start address */
	long	biic_end;		/* End address */
	long	biic_bci_ctrl;		/* BCI control register */
	long	biic_wrt_stat;		/* GPR write status */
	long 	biic_pad1[4];	
	long	biic_int_ctrl;		/* user interrupt control */
	long	biic_pad2[43];
	long	biic_gpr0;		/* General purpose registers */
	long	biic_gpr1;
	long 	biic_gpr2;
	long	biic_gpr3;
	
};

struct bi_nodespace {
	struct biic_regs biic;
	int	binode_pad[1984]; /* 8k ...size of BI nexus. */
};


struct bidata {
	struct bi_nodespace *bivirt;
	struct bi_nodespace *biphys;
	struct bi_nodespace *cpu_biic_addr; 
	int (**bivec_page)();
	int binodes_alive;
	int biintr_dst;
	int bi_err_cnt;
	unsigned bilast_err_time;
	struct {
		struct bisw *pbisw;
		int bierr;
		int bierr1;
	} bierr[16];
        struct {
                int lock;
                int incarn;
        } biinfo[16];

};


#endif

/* BI Device type register */
#define BITYP_TYPE	0x0000ffff	/* BI device type field */
#define BITYP_REV	0xffff0000	/* BI device revision field */

#define BI_MFA		0x00000101
#define BI_BUA		0x00000102
#define BI_BLA		0x00000103
#define BI_HSB		0x00004104
#define BI_KA820	0x00000105
#define BI_NBI		0x00000106
#define BI_XBI		0x00000107
#define BI_CIBCA	0x00000108
#define BI_COMB		0x00000109
#define BI_BAA		0x0000010a
#define BI_CIBCI	0x0000010b

#define BI_AIE_TK70	0x0000410b
#define BI_ACP		0x0000410c
#define BI_AIO		0x0000410d
#define BI_AIE_TK	0x0000410e
#define BI_AIE		0x0000410f
#define BI_XNA		0x00000118
#define BI_BDA		0x0000010e
#define BI_MEM1		0x00000001



/* BI control register */
#define BICTRL_BIICREV  0xff000000	/* BI interface chip revision */
#define BICTRL_BIICTYP	0x00ff0000	/* BI interface chip type */
#define BICTRL_HES	0x00008000 	/* BI hard error summary bit */
#define BICTRL_SES	0x00004000 	/* BI soft error summary bit */
#define	BICTRL_INIT	0x00002000	/* BI init node */
#define BICTRL_BROKE	0x00001000	/* BI broke bit */
#define BICTRL_STS	0x00000800	/* BIIC self test status bit */
#define BICTRL_SST	0x00000400	/* BI start self test bit */
#define BICTRL_UWP	0x00000100	/* BI unlock write pending bit */
#define BICTRL_HEIE	0x00000080	/* BI hard error interrupt enable */
#define BICTRL_SEIE	0x00000040	/* BI soft error interrupt enable */
#define BICTRL_ARB	0x00000030	/* BI arbitration control bits */
#define BICTRL_ID	0x0000000f	/* BI node id number */
#define BICTRL_HIARB	0x00000010	/* Fixed high arbitration */
#define BICTRL_NOARB	0x00000030	/* Disable arbitration */
/* BI Error register */
#define BIERR_NMR	0x40000000	/* No Ack of multi-respond command */
#define BIERR_MTCE	0x20000000	/* Master Transmit Check Error */
#define BIERR_CTE	0x10000000	/* Control Transmit Error */
#define BIERR_MPE	0x08000000	/* Master Parity Error	*/
#define BIERR_ISE	0x04000000	/* Interlock Sequence Error */
#define BIERR_TDF	0x02000000	/* Transmitter During Fault */
#define BIERR_IVE	0x01000000	/* Ident Vector Error */
#define BIERR_CPE	0x00800000	/* Command Parity Error */
#define BIERR_SPE	0x00400000	/* Slave Parity Error */
#define BIERR_RDS	0x00200000	/* Read data Substitute */
#define BIERR_RTO	0x00100000	/* Retry Time Out */
#define BIERR_STO	0x00080000	/* Stall Time Out */
#define BIERR_BTO	0x00040000	/* Bus Time Out */
#define BIERR_NEX	0x00020000	/* Non-Existent Address */
#define BIERR_ICE	0x00010000	/* Illegal Confirmation Error */
#define BIERR_UPEN	0x00000008	/* User Parity Enabled */
#define BIERR_IPE	0x00000004	/* ID Parity Error */
#define BIERR_CRD	0x00000002	/* Corrected Read Data */
#define BIERR_NPE	0x00000001	/* Null Bus Parity Error */

#define BIERR_BITS \
"\20\37NMR\36MTCE\35CTE\34MPE\33ISE\32IDF\31IVE\30CPE\27SPE\26RDS\
\25RTO\24STO\23BTO\22NEX\21ICE\4UPEN\3IPE\2CRD\1NPE"

/* error interrupt control register */
#define BIEINT_INTAB	0x01000000	/* Interrupt abort */
#define BIEINT_INTC	0x00800000	/* Interrupt complete */
#define BIEINT_SENT	0x00200000	/* interrupt command sent */
#define BIEINT_FORCE	0x00100000	/* Force error interrupt */
#define BIEINT_LEVEL	0x000f0000	/* levels of error int */
#define BIEINT_4LEVEL	0x00010000	/* level of error int */
#define BIEINT_5LEVEL	0x00020000	/* level of error int */
#define BIEINT_6LEVEL	0x00040000	/* level of error int */
#define BIEINT_7LEVEL	0x00080000	/* level of error int */
#define BIEINT_VECTOR	0x00003ffc	/* vector for error int */

#define BIEINT_BIVEC  0x50

/* interrupt destination */		
#define BIINT_DST	0x0000ffff	/* destination for intr */


#define BINODE_SIZE	8192		/* 8k node space size */
#define NBINODES	16

#define LEVEL14 0x100
#define LEVEL15 0x140
#define LEVEL16 0x180
#define LEVEL17 0x1c0
#define BIVECSIZE 0x40			/* size of each vector space */

/* ip mask */
#define BIICIP_MSK	0xFFFF0000	/* mask of node to accept IP */

/* ip dst */
#define BIICIP_DST	0x0000FFFF	/* Destination nodes for IP */

/* ip src */
#define BIICIP_SRC	0xFFFF0000	/* decode source of IP */

/* start address */
#define BIIC_START	0x3ffC0000	

/* end address */
#define BIIC_END	0x3ffC0000	

/* BCI control register */
#define BCI_BURSTEN	0x00020000	/* burst mode enable */
#define BCI_IPINTR	0x00010000	/* IP interrupt force */
#define BCI_MSEN	0x00008000	/* Multicast Space Enable */
#define BCI_BDCSTEN	0x00004000	/* Broadcast Enable */
#define BCI_STOPEN	0x00002000 	/* Stop Enable */
#define BCI_RESEN	0x00001000	/* reserve space enable */
#define BCI_IDENTEN	0x00000800	/* enable Ident's	*/
#define BCI_INVALEN	0x00000400	/* Enable invalidate's */
#define BCI_WINVALEN	0x00000200	/* Enable Write invalidate's */
#define BCI_UCSREN	0x00000100	/* enable access to user CSR's */
#define BCI_BICSREN	0x00000080	/* BIIC CSR space Enable */
#define BCI_INTREN	0x00000040	/* BI Interrupt enable */
#define BCI_IPINTREN	0x00000020	/* IP interrupt enable */
#define BCI_PNXTEN	0x00000010	/* Pipeline NXT enable */
#define BCI_RTOEVEN	0x00000008	/* Read Timeout EV enable */

/* Write gpr status */
#define BIIC_WRT0	0x10000000	/* gpr 0 */
#define BIIC_WRT1	0x20000000	/* gpr 1 */
#define BIIC_WRT2	0x40000000	/* gpr 2 */
#define BIIC_WRT3	0x80000000	/* gpr 3 */

/* User interrupt control register */
#define BIIC_INTAB	0xF0000000	/* Interrupt abort level */
#define BIIC_INTC	0x0F000000	/* Interrupt complete bits */
#define BIIC_SENT	0x00F00000	/* Interrupt sent bits */
#define BIIC_FORCE	0x000F0000	/* force interrupt level */
#define BIIC_EXVEC	0X00008000	/* external vector enable */
#define BIIC_VEC	0x00003FFC	/* interrupt vector */
#endif
