/* 	@(#)cons.h	4.1	(ULTRIX)	7/2/90 	*/
/************************************************************************
 *									*
 *			Copyright (c) 1984,86,87,88,89 by		*
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
 *   This software is  derived	from  software	received  from	the	*
 *   University    of	California,   Berkeley,   and	from   Bell	*
 *   Laboratories.  Use, duplication, or disclosure is	subject  to	*
 *   restrictions  under  license  agreements  with  University  of	*
 *   California and with AT&T.						*
 *									*
 *   The information in this software is subject to change  without	*
 *   notice  and should not be construed as a commitment by Digital	*
 *   Equipment Corporation.						*
 *									*
 *   Digital assumes no responsibility for the use  or	reliability	*
 *   of its software on equipment which is not supplied by Digital.	*
 *									*
 ************************************************************************/
/*	cons.h	6.1	83/07/29	*/
/* Modification History:
 *
 * 31-Jan-89
 *	Added ifdefs for mips based systems.
 *
 * 26-Apr-88    jaw
 *	Add VAX8820 support.
 *
 * 09-Apr-86 -- jaw  upgrade to new console.
 *
 * 02-Apr-86 -- jaw  add support for nautilus console and memory adapter
 *
 * 18-Mar-86 -- jrs
 *	Updated definitions for new release of 8800 console system
 *
 * 03-Mar-86 -- jrs
 *	Added definitions for 8800 console system
 *
 * 27-Feb-85 -tresvik
 *	Changes for support of the VAX8600 were merged in.
 *
 */

/*
 * VAX console interface registers
 */

#define RXCS_IE 	0x00000040	/* receiver interrupt enable */
#define RXCS_DONE	0x00000080	/* receiver done */

#define RXDB_DATA	0x000000ff	/* received character */
#define RXDB_ID 	0x00000f00	/* channel id */
#define RXDB_ERR	0x80000000	/* receiver error */
#define RXDB_ERROR	0x00008000	/* "" */

#define TXCS_IE 	0x00000040	/* transmitter interrupt enable */
#define TXCS_RDY	0x00000080	/* transmitter ready for next char */
#define TXDB_DATA	0x000000ff	/* transmitter byte */
#define TXDB_ID 	0x00000f00	/* channel id */

#define TXDB_DONE	0xf01		/* software done */
#define TXDB_BOOT	0xf02		/* reboot */
#define TXDB_CWSI	0xf03		/* clear warm start inhibit */
#define TXDB_CCSI	0xf04		/* clear cold-start inhibit */

#ifdef VAX8600
#define LOCAL_CONS	0x00010000	/* enable transmit on local console */
#define REMOTE_PORT	0x00020000	/* enable transmit on remote port */
#define EMM_PORT	0x00040000	/* enable transmit on EMM data line */
#define LOGICAL_CONS	0x00080000	/* enable transmit on logical console*/

#define WMASKNOW	0x00008000	/* write mask now */

/* id field */
#define LOCAL_CONS_ID	0x00000000	/* local console id */
#define REMOTE_PORT_ID	0x00000100	/* remote services port */
#define EMM_ID		0x00000200	/* emm id */
#define LOGIC_CONS_ID	0x00000300	/* logical console id */
#define NO_LINES	0x00000f00	/* no lines enabled */

/* DTR bits in RXCS */
#define LOCAL_DTR	LOCAL_CONS
#define REMOTE_DTR	REMOTE_PORT
#define EMM_DTR 	EMM_PORT
#define LOGICAL_DTR	LOGICAL_CONS
#endif /* VAX8600 */

#ifdef VAX8200
#define TXCS_BRE	0x00000100	/* Baud Rate enable (writting a new baud rate) */
#endif

#ifdef VAX8800
#define	N_MASK_ID	0x0f00		/* mask for entire id field */
#define N_MASK_DATA	0x00ff		/* mask for entire data field */

/* id field defns */
#define	N_LCL_CONS	0x0000		/* local console */
#define N_CSA1		0x0100		/* console storage device one */
#define N_CSA3		0x0200		/* console storage device three */
#define N_LCL_NOLOG	0x0300		/* local console w/o logging */
#define N_CSA2		0x0400		/* console storage device two */
#define N_CSM_DATA	0x0500		/* unsolicited csm data */
#define N_RMT_CONS	0x0600		/* remote console */
#define N_CONS_CNT	0x0700		/* incoming message continuation */
#define N_DIAG		0x0700		/* diagnostic data (txdb) */
#define N_CONS_MSG	0x0800		/* incoming message from console */
#define N_CONF_DATA	0x0900		/* return configuration data (rxdb) */
#define N_CSA_CMD	0x0900		/* storage command (txdb) */
#define N_MIC_CMD	0x0a00		/* microcode command */
#define N_MIC_DATA	0x0b00		/* microcode data */
#define N_CSM_CHECK	0x0c00		/* csm checksum */
#define N_TOY_DATA	0x0d00		/* toy clock data */
#define N_COMM		0x0f00		/* communications requests */

/* incoming message defns */

#define	N_TYP_MASK	0x00f0		/* Message type */
#define	N_ENVIRON	0x0000		/* Environmental alert */
#define	N_CSA1_STAT	0x0010		/* console device one status */
#define	N_CSA2_STAT	0x0020		/* console device two status */
#define	N_CSA3_STAT	0x0050		/* console device three status */
#define N_TOY_PROT	0x0070		/* toy clock protocol error */

#define	N_MSG_MASK	0x000f		/* Message contents */

#define N_BLOWER	0x0000		/* blower out */
#define N_YELLOW	0x0001		/* yellow zone waring */
#define N_RED		0x0002		/* red zone waring */

#define	N_CS_OK		0x0000		/* storage operation success */
#define	N_POS_ERR	0x0001		/* POS error */
#define	N_OPEN_ERR	0x0002		/* open error */
#define	N_IO_ERR	0x0003		/* io.blk error */
#define	N_QIO_ERR	0x0004		/* qio error */
#define	N_WIN_ERR	0x0005		/* winchester write error */

/* configuration data defns */

#define	N_LEFT_AVAIL	0x0001		/* left cpu available (dual cpu) */
#define	N_CONF_BOUND	0x0001		/* bounded configuration (single cpu)*/
#define	N_RIGHT_AVAIL	0x0002		/* right cpu available */
#define	N_SCND_ENABLE	0x0004		/* secondary cpu enabled */
#define	N_SINGLE_CPU	0x0008		/* single cpu configuration */
#define	N_DIAG_MATCH	0x0010		/* diag key matched */
#define	N_OS_MATCH	0x0020		/* os key matched */
#define	N_LEFT_PRIM	0x0040		/* left cpu is primary */
#define	N_SLOW_CPU	0x0080		/* slow cpu */


/* polar star configuration data defines bits */

#define P_CPU0_AVAIL    0x0001          /* set if cpu 0 is availiable */
#define P_CPU1_AVAIL    0x0002          /* set if cpu 1 is availiable */
#define P_CPU2_AVAIL    0x0004          /* set if cpu 2 is availiable */
#define P_CPU3_AVAIL    0x0008          /* set if cpu 3 is availiable */
#define P_PRIMARY       0x00c0          /* 2 bit field say who is primary */


/* unsolicited csm data */

#define N_CSM_TOMM	0x0000		/* entered via tomm */
#define N_CSM_HALT	0x0080		/* entered via halt */
#define N_HALT_MASK	0x007f		/* halt code */

/* console storage commands */

#define N_CS_SEL1	0x0000		/* select console storage one */
#define N_CS_SEL2	0x0010		/* select console storage two */
#define N_CS_SEL3	0x0020		/* select console storage three */
#define N_CS_READ	0x0000		/* read from device */
#define N_CS_WRITE	0x0001		/* write to device */

/* command codes */

#define N_SOFT_DONE	0x0001		/* software done */
#define N_BOOT_ME	0x0002		/* reboot this processor */
#define N_CLR_WARM	0x0003		/* clear warm start flag */
#define N_CLR_COLD	0x0004		/* clear cold start flag */
#define N_BOOT_OTHER	0x0005		/* reboot other processor */
#define N_UNJAM		0x0006		/* unjam bus */
#define N_LOAD_BOOT	0x0007		/* execute @loadnboot */
#define N_TOY_READ	0x0008		/* request toy clock */
#define N_TOY_WRITE	0x0009		/* notify of intent to write toy */
#define N_DIS_SECND	0x000a		/* disable secondary */
#define N_SWT_PRIM	0x000c		/* switch primary cpu */
#define N_GET_CONF	0x000d		/* request configuration info */


/* additional polarstar commands */
#define P_BOOT_CPU0     0x0014          /* restart cpu 0 */
#define P_BOOT_CPU1     0x0015          /* restart cpu 1 */
#define P_BOOT_CPU2     0x0016          /* restart cpu 2 */
#define P_BOOT_CPU3     0x0017          /* restart cpu 3 */
#define P_DISABLE_CPU0  0x0018          /* disable cpu 0 */
#define P_DISABLE_CPU1  0x0019          /* disable cpu 1 */
#define P_DISABLE_CPU2  0x001a          /* disable cpu 2 */
#define P_DISABLE_CPU3  0x001b          /* disable cpu 3 */
#define P_FORCE_PRIMARY_CPU0 0x001c     /* force next primary to cpu 0 */
#define P_FORCE_PRIMARY_CPU1 0x001d     /* force next primary to cpu 1 */
#define P_FORCE_PRIMARY_CPU2 0x001e     /* force next primary to cpu 2 */
#define P_FORCE_PRIMARY_CPU3 0x001f     /* force next primary to cpu 3 */

#endif /* VAX8800 */
