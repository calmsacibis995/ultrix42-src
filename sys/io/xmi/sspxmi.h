/*
 *	@(#)sspxmi.h	4.1	(ULTRIX)	7/2/90	
 */

/************************************************************************
 *									*
 *			Copyright (c) 1989 by				*
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
 * Modification History: /sys/io/xmi/sspxmi.h
 *
 *	20-Jul-89 -- map (Mark Parenti)
 *		Original version
 *
 * ------------------------------------------------------------------------
 */


/*	SSP PD Interrupt Level - Used for XMI				*/
#define	UQ_XMI_LEVEL14	0x10000		/* XMI Interrupt level 14 */
#define	UQ_XMI_LEVEL15	0x20000		/* XMI Interrupt level 15 */
#define	UQ_XMI_LEVEL16	0x40000		/* XMI Interrupt level 16 */
#define	UQ_XMI_LEVEL17	0x80000		/* XMI Interrupt level 17 */

/*	SSP PSI(Page Size Indicator) - Used for XMI			*/
#define	SSP_PSI_512	0	/* 512 byte pages			*/
#define	SSP_PSI_1024	1	/* 1024 byte pages			*/
#define	SSP_PSI_2048	2	/* 2048 byte pages			*/
#define	SSP_PSI_4096	3	/* 4096 byte pages			*/
#define	SSP_PSI_8192	4	/* 8192 byte pages			*/

/*	SSP PFN Mask Value - Used for XMI				*/
#define	SSP_PFN_MASK	0x1FF	/* 25 bits of PFN are significant	*/
                                /* We get 16 by default			*/


