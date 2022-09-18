
/*	@(#)bsc_messages.h	4.1	(ULTRIX)	7/2/90	*/

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

#define DLE	0x10
#define ENQ	0x2d 	
#define ETB	0x26
#define EOT	0x37
#define ETX	0x03
#define SOH	0x01	
#define NAK	0x3d
#define RVI     0x7c
#define IGS     0x1d
#define IRS	0x1e
#define IUS	0x1f
#define ITB	0x1f
#define ACK1	0x61
#define WACK    0x6b
/*	#define NULL	0x00	*/
#define PAD	0xff
#define X70	0x70
#define ACK0	0x70
#define STX	0x02
#define SYNC	0x32 
#define	SYN	0x32 
#define ESC	0x27
#define HT	0x05
#define NL	0x15
#define LF	0x25

#define	LPAD	(unsigned char)'\252'
/* #define LPAD	0xaa			*/

#define WAITBIT 0x40


