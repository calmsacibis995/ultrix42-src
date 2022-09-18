#ifndef lint
static char *sccsid = "@(#)vba_data.c	4.3	(ULTRIX)	4/4/91";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1990 by				*
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
 * Modification History:
 *
 *   22-Jan-1991	Mark Parenti
 *	Add defines and initialization for DMA PMR mapping.
 *
 *   15-Dec-1989	Mark Parenti
 *	Original version
 *
 * ------------------------------------------------------------------------
 */

#include "../machine/pte.h"
#include "../h/param.h"
#include "../h/systm.h"
#include "../h/map.h"
#include "../h/buf.h"
#include "../machine/cpu.h"
#include "../io/vme/vbareg.h"
#include "vba.h"

int vbanotconf();

/* VME Bus Request Level for adapter master cycles	*/
#define	VME_BR_0	0x0
#define	VME_BR_1	0x1
#define	VME_BR_2	0x2
#define	VME_BR_3	0x3

/* VME Arbitration timer definitions	*/

#define	VME_ARBTO_4US	0x0	/* 4 microseconds	*/
#define	VME_ARBTO_16US	0x1	/* 16 microseconds	*/
#define	VME_ARBTO_32US	0x2	/* 32 microseconds	*/
#define	VME_ARBTO_64US	0x3	/* 64 microseconds	*/
#define	VME_ARBTO_128US	0x4	/* 128 microseconds	*/
#define	VME_ARBTO_256US	0x5	/* 256 microseconds	*/
#define	VME_ARBTO_512US	0x6	/* 512 microseconds	*/
#define	VME_ARBTO_DIS	0x7	/* timeouts disabled	*/

/* VMEbus Interrupt Priority Levels handled by adapter	*/
#define	VME_IPL_1	0x0001
#define	VME_IPL_2	0x0002
#define	VME_IPL_3	0x0004
#define	VME_IPL_4	0x0008
#define	VME_IPL_5	0x0010
#define	VME_IPL_6	0x0020
#define	VME_IPL_7	0x0040
#define	VME_ALL_IPL	(VME_IPL_1 | \
			 VME_IPL_2 | \
			 VME_IPL_3 | \
			 VME_IPL_4 | \
			 VME_IPL_5 | \
			 VME_IPL_6 | \
			 VME_IPL_7 )

/* VMEbus Arbitration Select */
#define	VME_ARB_RR	0x0	/* Round-Robin Arbitration	*/
#define	VME_ARB_PRI	0x1	/* Priority Arbitration		*/
#define	VME_ARB_PRS	0x2	/* Prioritized Round Robin	*/
#define	VME_ARB_SGL	0x3	/* Single Level Arbitration	*/


/* VMEbus System Controller Select */
#define	VME_SYS_CONTROLLER	0x80000000 /* Adapter is VMEbus controller */
#define	VME_NOSYS_CONTROLLER	0x0	/* Adapter is not VMEbus controller */

/* VMEbus Bus Release Modes */
#define	VME_ROR			0x00	/* Release-on-request	*/
#define	VME_RWD			0x40	/* Release-when-done	*/

/* This field selects where in VME address space the DMA Page Map Registers */
/* are mapped. If 2 adapters are used they must use different values */
/* for this field.  Note that if VME_MAP_HIGH is selected only A32 DMA */
/* is supported.							*/
#define	VME_MAP_LOW		0x00	/* DMA mapped to first 1GB 	*/ 
#define	VME_MAP_HIGH		0x01	/* DMA mapped to second 1GB	*/


/*************************************************************************/
/*									 */
/* The following structure is used to customize various VMEbus 		 */
/* parameters. The default values specified should provide proper        */
/* VMEbus operation for most applications.  Care should be taken before  */
/* any of these values are modified. Not all adapters support all	 */
/* fields. 								 */
/*************************************************************************/
struct	vbadata vbadata = {
	VME_BR_3,		/* Bus Request Level for master cycles	*/
	VME_ARBTO_256US,	/* Arbitration Timeout		      	*/
	VME_ARB_RR,		/* Arbitration Method			*/
	VME_ALL_IPL,		/* Interrupt levels processed		*/
	VME_SYS_CONTROLLER,	/* System Controller			*/
	VME_ROR,		/* Release Type				*/
	VME_MAP_LOW		/* DMA mapped to first 1GB		*/
};




