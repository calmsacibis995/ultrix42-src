/* static	char	*sccsid = "@(#)if_dmc.h	4.1	ULTRIX	7/2/90"; */

/************************************************************************
 *									*
 *			Copyright (c) 1984 by				*
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
 * Modification History: /sys/vaxif/if_dmc.h
 *
 * 18 Apr 86 -- ejf
 *	Added DECnet support
 *
 * 29 Oct 84 -- jrs
 *	Update with latest version (1.3) from
 *		Bill Nesheim (Cornell)
 *		Lou Salkind  (NYU)
 *
 *	Derived from 4.2BSD, labeled:
 *		if_dmc.h 6.1	83/07/29
 *
 * -----------------------------------------------------------------------
 */

/*
 * DMC-11 Interface
 */

struct dmcdevice {
        char    bsel0;
	char 	bsel1;
	char	bsel2;
	char 	bsel3;
	short	sel4;
	short	sel6;
};

/* packet types */
#define	DMC_IPTYPE	1
#define	DMC_TRAILER	2
#define	DMC_NTRAILER	16

/*
 * DMCMTU includes space for data (1024) + 
 * protocol header (256) + trailer descriptor (4).
 * The software link encapsulation header (dmc_header)
 * is handled separately.
 */
#define DMCMTU  1284

#define RDYSCAN 16      /* loop delay for RDYI after RQI */

/* defines for bsel0 */
#define DMC_BACCI       0
#define DMC_CNTLI       1
#define DMC_PERR        2
#define DMC_BASEI       3
#define DMC_WRITE       0               /* transmit block */
#define DMC_READ        4               /* read block */
#define DMC_RQI         0040            /* port request bit */
#define DMC_IEI         0100            /* enable input interrupts */
#define DMC_RDYI        0200            /* port ready */
#define DMC0BITS "\10\8RDI\7IEI\6RQI"

/* defines for bsel1 */
#define DMC_MCLR        0100            /* DMC11 Master Clear */
#define DMC_RUN         0200            /* clock running */
#define DMC_ILOOP       0010            /* internal loop */
#define DMC1BITS "\10\8RUN\7MCLR"

/* defines for bsel2 */
#define DMC_BACCO       0
#define DMC_CNTLO       1
#define DMC_OUX         0               /* transmit block */
#define DMC_OUR         4               /* read block */
#define DMC_IEO         0100            /* enable output interrupts */
#define DMC_RDYO        0200            /* port available */
#define DMC2BITS "\10\8RDO\7IEO"

/* defines for bsel3 */
#define	DMC_DMRCHK	0		/* loaded when verifying dev is dmr */
#define DMR_MASK	0377		/* mask to use */
#define	DMR_0001	001		/* checked when verifying dev is dmr */
#define	DMR_0002	002		/* checked when verifying dev is dmr */
#define	DMR_0100	0100		/* checked when verifying dev is dmr */
#define	DMR_0200	0200		/* checked when verifying dev is dmr */

/* defines for CNTLI mode */
#define DMC_HDPLX       02000           /* half duplex DDCMP operation */
#define DMC_SEC         04000           /* half duplex secondary station */
#define DMC_MAINT       00400           /* enter maintenance mode */

/* defines for BACCI/O and BASEI mode */
#define DMC_XMEM        0140000         /* xmem bit position */
#define DMC_CCOUNT      0037777         /* character count mask */
#define DMC_RESUME      0002000         /* resume (BASEI only) */

/* defines for CNTLO */
#define DMC_CNTMASK     01777

#define DMC_DATACK      01
#define DMC_TIMEOUT     02
#define DMC_NOBUFS      04
#define DMC_MAINTREC    010
#define DMC_LOSTDATA    020
#define DMC_DISCONN     0100
#define DMC_START       0200
#define DMC_NEXMEM      0400
#define DMC_ERROR       01000

#define DMC_FATAL       (DMC_ERROR|DMC_NEXMEM|DMC_START|DMC_LOSTDATA|DMC_MAINTREC)
#define CNTLO_BITS      \
        "\10\12ERROR\11NEXMEM\10START\7DISC\5LSTDATA\4MAINT\3NOBUF\2TIMEO\1DATACK"

/*
 *	Counter indexes into the base table.
 */
#define	DMCD_RXNOBUF	003		/* rx - no buffer available */
#define	DMCD_RXHCRC	004		/* rx - header crc */
#define	DMCD_RXDCRC	005		/* rx - data crc */
#define	DMCD_TXNOBUF    006		/* tx - no buffer available */
#define	DMCD_TXHCRC	007		/* tx - header crc */
#define	DMCD_TXDCRC	010		/* tx - data crc */
#define	DMCD_LOCALTMO	011		/* tx - reps sent, local timeout */
#define	DMCD_REMOTETMO	012		/* rx - reps rcvd, remote timeout */
#define	DMCD_RXBYTE	0163		/* bytes received index */

/*
 *	Counter indexes into the rx/tx counters table.
 */
#define	DMCZ_RXBYTE	000		/* bytes received index */
#define	DMCZ_TXBYTE	001		/* bytes transmitted index */
#define	DMCZ_RXBLOK	002		/* blocks received */
#define	DMCZ_TXBLOK	003		/* blocks transmitted */
#define CTRZ_SIZE	004		/* size of counters table */

/*
 *	Counter indexes into the zero baseline base table.
 */
#define	DMCZ_RXNOBUF	001		/* rx - no buffer available */
#define	DMCZ_RXHCRC	002		/* rx - header crc */
#define	DMCZ_RXDCRC	003		/* rx - data crc */
#define	DMCZ_TXNOBUF	004		/* tx - no buffer available */
#define	DMCZ_TXHCRC	005		/* tx - header crc */
#define	DMCZ_TXDCRC	006		/* tx - data crc */
#define	DMCZ_LOCALTMO	007		/* tx - reps sent, local timeout */
#define	DMCZ_REMOTETMO	010		/* rx - reps rcvd, remote timeout */
#define DMCZ_SIZE	011		/* sizeof zero baseline base table */

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
