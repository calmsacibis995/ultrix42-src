/* #ifndef lint
 * static	char	*sccsid = "@(#)if_dupreg.h	4.1	ULTRIX	7/2/90";
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

#define DUPSIZE 1024
#define DUPPRI PZERO+10

/*
 * Driver states.
 */
#define DUPINIT		20	/* set in driver init routine */
#define DUPWRITE 	21	/* to add a character to an mbuf or tnsmt from
				 * an mbuf */
#define DUPCHECKCRC	22	/* flag to cause a crc check */
#define DUPDONE		23	/* done with the mbuf = done with the packet */
#define DUPREADY	24	/* ready to receive or send data */
#define DUPFILLCH	25	/* causes a fill character to be sent */
#define DUPNEXT		26	/* get the next mbuf to receive data */
#define DUPDONTWRITE	27	/* dont write the control character to mbuf */
#define DUPPRETNS	28	/* pretransmit state	*/
#define DUPREAD		29	/* dup is reading characters    */
#define DUPPRETNT	30	/* pretransmit state - used with DUPPRETNS, 
			         * causes device to idle syncs	*/
#define ADDCRC  	31	/* watch for incoming crc characters */

/*
 * Device register bits.
 */
#define CTS	0020000 /* clear to send state on modem */
#define DSIE	0000040 /* data set interrupt enable */
#define CARRIER 0010000 /* carrier is up */
#define RING	0040000 /* modem ring line */
#define HALFDX	0000010 /* half duplex operation */
#define RXITEN	0000100	/* receive interrupt enable */
#define	RCVEN	0000020 /* receiver enable */
#define	RTS	0000004 /* request to send */
#define	DTR	0000002 /* data terminal ready */
#define DSR	0001000 /* data set ready */
#define	TXERR	0100000 /* transmit error */
#define	DEVRST	0000400 /* device reset */
#define	TXITEN	0000100 /* transmit done interrupt enable */
#define	TRAEN	0000020 /* transmit enable - send */
#define	DEC	0100000 /* enable bisync */
#define	NOCRC	0001000 
#define	RXERR	0100000 /* receive error */
#define	TSOM	0000400 /* transmit start of message */
#define	TEOM	0001000 /* transmit end of message */
#define STRIP	0000400 /* strip sync */
#define TXDONE  0000200 /* transmit done - ready to load next character */

#define	DUPADDR	dupaddr

struct	dupdevice {
	short	rcsr;
	union {
		short	Rbuf;
		short	Pcsr;
	} rs;
	short	tcsr;
	short	tbuf;
};

#define	rbuf	rs.Rbuf
#define pcsr	rs.Pcsr

