/* static	char	*sccsid = "@(#)if_fddi.h	4.3	(ULTRIX)	10/10/90"; */

/************************************************************************
 *									*
 *			Copyright (c) 1988 by				*
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

 /* ---------------------------------------------------------------------
  *
  * Modification History:
  * 	
  * 27-Apr-90	chc (Chran-Ham Chang)
  * 	Created this module for fddi
  * ------------------------------------------------------------------- */

/*
 * Structure of a 100Mb/s FDDI header.
 */
struct	fddi_header {
	u_char  fddi_ph[3];	 
	u_char	fddi_fc;
	u_char	fddi_dhost[6];
	u_char	fddi_shost[6];
};

/*
 * FDDI Frame Control bits
 */
#define	FDDIFC_C		0x80		/* Class bit */
#define	FDDIFC_L		0x40		/* Address length bit */
#define	FDDIFC_F		0x30		/* Frame format bits */
#define	FDDIFC_Z		0x0f		/* Control bits */

/*
 * FDDI Preamble and Starting Delimiter
 */
#define FDDIPH0   0x020
#define FDDIPH1   0x038
#define FDDIPH2   0x000

/*
 * FDDI Frame Control values. (48-bit addressing only).
 */
#define	FDDIFC_VOID		0x40		/* Void frame */
#define	FDDIFC_NRT		0x80		/* Nonrestricted token */
#define	FDDIFC_RT		0xc0		/* Restricted token 
#define	FDDIFC_SMT_INFO		0x41		/* SMT Info */
#define	FDDIFC_SMT_NSA		0x4F		/* SMT Next station adrs */
#define	FDDIFC_MAC_BEACON	0xc2		/* MAC Beacon frame */
#define	FDDIFC_MAC_CLAIM	0xc3		/* MAC Claim frame */
#define	FDDIFC_LLC_ASYNC	0x50		/* Async. LLC frame */
#define	FDDIFC_LLC_SYNC		0xd0		/* Sync. LLC frame */
#define	FDDIFC_IMP_ASYNC	0x60		/* Implementor Async. */
#define	FDDIFC_IMP_SYNC		0xe0		/* Implementor Synch. */
#define FDDIFC_SMT    		0x40		/* SMT frame */
#define FDDIFC_MAC    		0xc0		/* MAC frame */

/*
 * FDDI Asynchronous LLC frame priorities
 */
#define	FDDIFC_LLC_PRI0		0x00
#define	FDDIFC_LLC_PRI1		0x01
#define	FDDIFC_LLC_PRI2		0x02
#define	FDDIFC_LLC_PRI3		0x03
#define	FDDIFC_LLC_PRI4		0x04
#define	FDDIFC_LLC_PRI5		0x05
#define	FDDIFC_LLC_PRI6		0x06
#define	FDDIFC_LLC_PRI7		0x07

#define FDDIMTU         4352

#define	FDDIMAX		4495		/* maximum fddi size */
#define	FDDILLCMIM	20		/* minimum LLC frame size */
#define FDDISMTMIM      37		/* minimum SMT frame size */
#define FDDIMACMIM	17		/* minimum MAC frame size */


