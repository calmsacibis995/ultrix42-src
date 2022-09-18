/*	@(#)bsc_states.h	4.1	(ULTRIX)	7/2/90	*/
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

/*
 * BSC state definitions.			5/10/85
 */

short state;

#define	 CLOSED		0	/* closed */
#define	 LISTEN		1	/* listening for connection */
#define	 SYN_SENT	2	/* active, have sent syn */
#define	 SYN_RECEIVED	3	/* have send and received syn */

#define	 CLOSE_WAIT	5	/* rcvd fin, waiting for close */

/* states >  CLOSE_WAIT are those where user has closed */
#define	 CLOSING	7
#define	 LAST_ACK	8	

#define	 TIME_WAIT	10	/* in 2*msl quiet wait after close */
#define  SEND		12

#define INIT		13
#define SENDACK		14
#define SENDBLK		15
#define REXMT		16
#define MODEM		17
#define ERRORS		18
#define SENDBLKLAST	19
#define SENDACK0	20
#define SENDWACK	21
#define SENDSOC		22

#define INITFLT 1

#define R_ACK	1
#define R_WAIT	2
#define R_ENQ	3
#define R_ERBLK	5
#define R_NAK	6
#define R_OKBLK	7
#define R_RVI   8
#define R_SEQ	9
#define R_ERROR 11	
#define R_SEQER 010
#define TIMEOUT 012
#define X_ENQ   013
#define X_NAK	014
#define X_ACK   015
#define X_OKBLK	016
#define X_ERBLK 017

#define xmtcrc(X)	{crc16(X); xmt(X);}
#define incmod(X,Y)	{++X; if(X >= Y)X= 0;}

