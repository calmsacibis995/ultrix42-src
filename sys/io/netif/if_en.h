/* static	char	*sccsid = "@(#)if_en.h	4.1	(ULTRIX)	7/2/90"; */

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

/************************************************************************
 *			Modification History				*
 *									*
 *	Larry Cohen  -	09/16/85					*
 * 		Add 43bsd alpha tape changes for subnet routing		*
 *									*
 ************************************************************************/


/*	if_en.h	6.1	83/07/29	*/

/*
 * Structure of a Ethernet header.
 */
struct	en_header {
	u_char	en_shost;
	u_char	en_dhost;
	u_short	en_type;
};

#define	ENTYPE_PUP	0x0200		/* PUP protocol */
#define	ENTYPE_IP	0x0201		/* IP protocol */

/*
 * The ENTYPE_NTRAILER packet types starting at
 * ENTYPE_TRAIL have (type-ENTYPE_TRAIL)*512 bytes
 * of data followed by an Ethernet type (as given above)
 * and then the (variable-length) header.
 */
#define	ENTYPE_TRAIL	0x1000		/* Trailer type */
#define	ENTYPE_NTRAILER	16

#define EN_BROADCAST	0		/* Hardware broadcast address */
