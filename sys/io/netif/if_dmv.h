/* static	char	*sccsid = "@(#)if_dmv.h	4.1	ULTRIX	7/2/90"; */

/************************************************************************
 *									*
 *			Copyright (c) 1986 by				*
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

/* ------------------------------------------------------------------------
 * Modification History: /sys/vaxif/if_dmv.h
 *
 *  18-apr-86 -- ejf  Cloned from dmc driver.
 *
 * -----------------------------------------------------------------------
 */

/*
 * DMV-11 Interface
 */

struct dmvdevice {
        char    bsel0;
	char 	bsel1;
	char	bsel2;
	char 	bsel3;
	short	sel4;
	short	sel6;
	short	sel10;
};

/* packet types */
#define	DMV_IPTYPE	1
#define	DMV_TRAILER	2
#define	DMV_NTRAILER	16

/*
 * DMVMTU includes space for data (1024) + 
 * protocol header (256) + trailer descriptor (4).
 * The software link encapsulation header (dmv_header)
 * is handled separately.
 */
#define DMVMTU  1284

#define RDYSCAN 16      /* loop delay for RDYI after RQI */

/* defines for bsels, sels */
#define DMV_LBSEL_MASK	0xff		/* low byte of sel */
#define DMV_HBSEL_MASK	0xff00		/* high byte of sel */

/* defines for bsel0 */
#define DMV_IEI         0x1            /* enable input interrupts */
#define DMV_IEO         0x10            /* enable output interrupts */
#define DMV_RQI         0x80            /* port request bit */

/* defines for bsel1 */
#define DMV_ILOOP       0x1             /* internal loop - DMV11 mnt req */
#define DMV_DIAG        0x8             /* DMV11 diag mode  */
#define DMV_MCLR        0x40            /* DMV11 Master Clear */
#define DMV_RUN         0x80            /* DMV11 clock running */

/* defines for bsel2 */
#define DMV_CMD_MASK	0x7		/* command mask */
#define DMV_CMD_RXBUF	0x0		/* Rx buf command */
#define DMV_CMD_TXBUF	0x4		/* Tx buf command */
#define DMV_CMD_CNTL	0x1		/* Control command */
#define DMV_CMD_MODE	0x2		/* Mode command */
#define DMV_RSP_RXBUFOK	0x0		/* Rx buf disposition */
#define DMV_RSP_CNTL	0x1		/* Control response */
#define DMV_RSP_INFO	0x2		/* Information response */
#define DMV_RSP_RXBUF	0x3		/* Rx buf disposition */
#define DMV_RSP_TXBUFOK	0x4		/* Tx buf command/disposition */
#define DMV_RSP_TXBUF1	0x6		/* Tx buf disposition - no ACK */
#define DMV_RSP_TXBUF2	0x7		/* Tx buf disposition - not sent */
#define DMV_22BIT	0x8		/* 22 bit addressing enabled */
#define DMV_RDYI        0x10            /* port ready */
#define DMV_RDYO        0x80            /* port available */
#define DMV_MAINT_ILOOP	0x6		/* Maint Reg Emulation - int loopback */

/* defines for bsel3 */
#define DMV_TRIB_POINT  0x1            /* addr for point to point config */

/* defines for bsel4 */
#define	DMV_MODEM_DTR	0x40		/* data terminal ready - write modem */
#define DMV_TIMER_SEL	0764		/* select timer = 5 seconds */
#define DMV_TIMER_BAB	01750		/* babble timer = 10 seconds */

/* defines for bsel6 - mode commands */
#define DMV_MODE_HDPLX_DMC   0x0           /* half duplex DDCMP operation - DMC compatibility */
#define DMV_MODE_FDPLX_DMC   0x1           /* full duplex DDCMP operation - DMC compatibility */
#define DMV_MODE_HDPLX       0x2           /* half duplex DDCMP operation */
#define DMV_MODE_FDPLX       0x3           /* full duplex DDCMP operation */

/* defines for bsel6 - control commands */
#define DMV_CNTL_NOP        0x0           /* NOP */
#define DMV_CNTL_ESTRIB     0x1           /* Establish tributary */
#define DMV_CNTL_DETRIB     0x2           /* Delete tributary */
#define DMV_CNTL_RQSTRT     0x3           /* Request Start-up state */
#define DMV_CNTL_RQMAINT    0x4           /* Request Maintenance state */
#define DMV_CNTL_RQHALT     0x5           /* Request Halt state */
#define DMV_CNTL_WRMODEM    0x10           /* Write modem */
#define DMV_CNTL_RDMODEM    0x11           /* Read modem */
#define DMV_CNTL_READ	    0x20	  /* Read TSS/GSS */
#define DMV_CNTL_READZ	    0x40	  /* Read zero TSS/GSS */
#define DMV_CNTL_WRITE	    0x80	  /* Write TSS/GSS */

/* defines for bsel6 - event/error code response */
#define DMV_ERR_RXTHRES     002           /* Receive Threshold error */
#define DMV_ERR_TXTHRES     004           /* Transmit Threshold error */
#define DMV_ERR_SETHRES     006           /* Select Threshold error */
#define DMV_EVT_RXSTRT_RU   010           /* Receive DDCMP STRT while running */
#define DMV_EVT_RXMNT_RU    012           /* Receive Maintenance msg while running */
#define DMV_EVT_RXMNT_HA    014           /* Receive Maintenance msg while halted */
#define DMV_EVT_RXSTRT_MA   016           /* Receive DDCMP STRT while in maintenance */
#define DMV_EVT_DEAD_TRIB   022           /* Trib polling state dead */
#define DMV_EVT_DDCMPRUN    024           /* Device running DDCMP over link */
#define DMV_ERR_BABBLE      026           /* Babbling tributary */
#define DMV_ERR_STREAM      030           /* Streaming tributary */
#define DMV_RES1_LOW        032           /* Low reserved code */
#define DMV_RES1_HIGH       076           /* High reserved code */
#define DMV_PRC_INVMODE     0104          /* Invalid mode proc err code */
#define DMV_PRC_LOW         0100          /* Low proc err code */
#define DMV_PRC_HIGH        0140           /* High proc err code */
#define DMV_RES2_LOW        0142          /* Low reserved code */
#define DMV_RES2_HIGH       0276          /* High reserved code */
#define DMV_PRC_BUF         0300          /* Buffer too small */
#define DMV_PRC_NXM         0302          /* Nonexistent memory */
#define DMV_EVT_DISCONN     0304          /* Modem disconnecting */
#define DMV_EVT_QOVF        0306          /* Queue overflow */
#define DMV_EVT_CARLOSS     0310          /* Modem carrier loss */

/* defines for bsel6 - information return key response */
#define DMV_INFO_MODEM      010           /* Modem status */
#define DMV_INFO_BUFRET     020           /* Buffer return complete */
#define DMV_INFO_READ	    0x20	  /* Read TSS/GSS */
#define DMV_INFO_READZ	    0x40	  /* Read zero TSS/GSS */


/* defines for bsel10 */
#define DMV_CCOUNT      0x3fff         /* character count mask */

/*
 *	Counter indexes into the rx/tx counters table.
 */
#define	DMVZ_RXBYTE	000		/* bytes received index */
#define	DMVZ_TXBYTE	001		/* bytes transmitted index */
#define	DMVZ_RXBLOK	002		/* blocks received */
#define	DMVZ_TXBLOK	003		/* blocks transmitted */
#define CTRZ_SIZE	004		/* size of counters table */

/*
 *	Counter and timer parameter address definitions
 */
#define DMV_KEY_SELINT	011		/* selection interval */
#define DMV_KEY_DEOUT	012		/* data errors outbound */
#define DMV_KEY_DEIN	013		/* data errors inbound */
#define DMV_KEY_LBERR	014		/* local buffer errors */
#define DMV_KEY_RBERR	015		/* remote buffer errors */
#define DMV_KEY_SELTO	016		/* selection timeouts */
#define DMV_KEY_REPTO	017		/* local and remote reply timeouts */
#define DMV_KEY_RSERR	015		/* remote station errors */
#define DMV_KEY_LSERR	016		/* local station errors */
#define DMV_KEY_SELECT	036		/* select timer address */
#define DMV_KEY_BABBLE	037		/* babble timer address */

/*
 *	Counter bit mask done definitions
 */
#define DMV_MSK_BUSY	0x1		/* mask lock bit */
#define DMV_MSK_SELINT	0x2		/* selection interval */
#define DMV_MSK_DEOUT	0x4		/* data errors outbound */
#define DMV_MSK_DEIN	0x8		/* data errors inbound */
#define DMV_MSK_LBERR	0x10		/* local buffer errors */
#define DMV_MSK_RBERR	0x20		/* remote buffer errors */
#define DMV_MSK_SELTO	0x40		/* selection timeouts */
#define DMV_MSK_REPTO	0x80		/* local and remote reply timeouts */
#define DMV_MSK_RSERR	0x100		/* remote station errors */
#define DMV_MSK_LSERR	0x200		/* local station errors */
#define DMV_MSK_CMPLT	0x3ff		/* mask of recorded counters */

/* defines for translation between ui flags and mode function codes */
#define DMV_MFLAGS_MASK		0x7	/* mode flags is 3 bits */
#define DMV_MFLAGS_FDPLX_DMC	0x0	/* full duplex, DMC compatible */
#define DMV_MFLAGS_FDPLX	0x4	/* full duplex, non DMC compatible */
#define DMV_MFLAGS_MAINT_FDPLX	0x1	/* full duplex, maintenance mode (MOP) */
#define DMV_MFLAGS_MAINT_HDPLX	0x3	/* half duplex, maintenance mode (MOP) */
#define DMV_MFLAGS_HDPLX_DMC	0x2	/* half duplex, DMC compatible */
#define DMV_MFLAGS_HDPLX	0x6	/* half duplex, non DMC compatible */

/*
 * queue literals
 */
#define NRCV 7
#define NXMT_MIN 7 
#define NXMT_MAX 24 
#define NXMT NXMT_MAX
#define NXMT_MASK	0x1f00		/* mask for # tx bufs in ui_flags */
#define NXMT_SHIFT	8		/* shift for # tx bufs in ui_flags */
#define NTOT (NRCV + NXMT)
#define	NCMDS	(NTOT+4)	/* size of command queue */
