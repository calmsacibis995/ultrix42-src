/* #ifndef lint
 * static	char	*sccsid = "@(#)if_dpvreg.h	4.1	ULTRIX	7/2/90";
 * #endif lint
 */
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

#define DPVSIZE 1024
#define DPVPRI PZERO+10

/*
 * Driver states.
 */
#define DPVINIT		20	/* set in driver init routine */
#define DPVWRITE 	21	/* to add a character to an mbuf or tnsmt from
				 * an mbuf */
#define DPVCHECKCRC	22	/* flag to cause a crc check */
#define DPVDONE		23	/* done with the mbuf = done with the packet */
#define DPVREADY	24	/* ready to receive or send data */
#define DPVFILLCH	25	/* causes a fill character to be sent */
#define DPVNEXT		26	/* get the next mbuf to receive data */
#define DPVDONTWRITE	27	/* dont write the control character to mbuf */
#define DPVPRETNS	28	/* pretransmit state	*/
#define DPVREAD		29	/* dup is reading characters    */
#define DPVPRETNT	30	/* pretransmit state - used with DPVPRETNS, 
			         * causes device to idle syncs	*/
#define ADDCRC  	31	/* watch for incoming crc characters */

/*
 * Device register bits.
 */
#define DSCNG	0100000 /* data set change */
#define ICALL	0040000 /* incoming call */
#define CTS	0020000 /* clear to send */
#define CARRIER	0010000 /* receiver ready - same as gettin carrier */
#define RXACT	0004000 /* receiver active */
#define RSRDY	0002000 /* receiver status ready */
#define DSR	0001000 /* data mode */
#define RDRDY	0000200 /* receive data ready */
#define RXITEN	0000100 /* receive interrupt enable */
#define DSITEN  0000040 /* data set interrupt enable */
#define RCVEN	0000020 /* rx enable */
#define LL	0000010 /* local loop */
#define RTS	0000004 /* request to send */
#define DTR	0000002 /* data terminal ready */
#define SFRL    0000001 /* select frequency or remote loopback */

#define NOCRC   0003400 /* inhibit crc */
#define RXERR	0100000 /* error check */
#define ROVR	0004000 /* receiver overrun */
#define RABORT  0002000 /* receive abort */
#define REOM	0001000 /* end of message */
#define RSOM	0000400 /* start of message */

#define APA	0100000 /* all parties address */
#define PROTO	0040000 /* protocol select
#define STRIP	0020000 /* strip sync or local loop */
#define SAM	0010000 /* second address mode select */
#define IDLE	0004000 /* idle mode select */

#define EXADD	0010000 /* extended address field */
#define EXCON	0004000 /* extended control field */
#define TXITEN	0000100 /* transmit interrupt enable */
#define SQTM	0000040 /* signal quality or test mode */
#define TRAEN	0000020 /* transmitter enable */
#define MAINT   0000010 /* maintenance mode select */
#define TBEMTY	0000004 /* transmit buffer empty */
#define TXACT	0000002 /* transmitter active */
#define RESET   0000001 /* device reset */

#define TERR	0100000 /* transmit data late */
#define XGO	0004000 /* transmit go ahead */
#define TXABORT	0002000 /* abort */
#define TEOM	0001000 /* end of message */
#define TSOM	0000400 /* start of message */


#define	DPVADDR	dpvaddr

struct	dpvdevice {
	short	rxcsr;
	union {
		short	RDS;
		short	PCS;
	} rs;
	short	pcscr;
	short	tdsr;
};

#define	rdsr	rs.RDS
#define pcsar	rs.PCS

