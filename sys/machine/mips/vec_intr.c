#ifndef lint
static	char	*sccsid = "@(#)vec_intr.c	4.1	(ULTRIX)	7/2/90";
#endif lint

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
 * Revision History:
 *
 * 15-Jun-90    Mark Parenti
 *      Moved stray() and passive_release() to machdep.c
 *
 * 07-Jun-90	Paul Grist
 *	Added conditionals for vaxbi and vba scb pages and fixed 
 *	vaxbi to CVAXBI, the number of confured vaxbi instead of
 *	NVAXBI the largest# plus one.
 *
 * 06-Feb-90	Robin
 *	added DS5500 support
 *
 * 20-Dec-89    Paul Grist
 *      Added VMEbus support (vba adapters). added line to increase the
 *      size of the vector table by 256 entries for each configured
 *      vba adapter.
 *
 * 02-Feb-89	Kong
 *	Created the file.  This file declares a vector table
 *	for handling interrupts.  The vector table is called
 *	"scb".  The code path is as follow:
 *	On mips machine such as ISIS, CMAX, and MIPSFAIR, read a register
 *	to get the value of the vector.  Use this vector as a byte
 *	offset into the vector table "scb".  Call the address stored
 *	in the "scb".  Unlike in a VAX where the scb is used for all
 *	interrupts and exceptions, here the scb is used only for
 *	device interrupts.  
 *	TODO: perhaps error interrupts should go through the scb as well.
 */
#include "../machine/common/cpuconf.h"
#include "vaxbi.h"
#include "vba.h"

extern stray(),hardclock(),cnxint(),cnrint(),passive_release();
extern int cold;
int (*(scb[128]))() =	/* Declare an array of pointers to int functions */
{
	passive_release,stray,		stray,		stray,
	stray,		stray,		stray,		stray,
	stray,		stray,		stray,		stray,
	stray,		stray,		stray,		stray,
	stray,		stray,		stray,		stray,
	stray,		stray,		stray,		stray,
	stray,		stray,		stray,		stray,
	stray,		stray,		stray,		stray,
	stray,		stray,		stray,		stray,
	stray,		stray,		stray,		stray,
	stray,		stray,		stray,		stray,
	stray,		stray,		stray,		stray,
	hardclock,	stray,		stray,		stray,
	stray,		stray,		stray,		stray,
	stray,		stray,		stray,		stray,
	stray,		stray,		cnrint,		cnxint,
	stray,		stray,		stray,		stray,
	stray,		stray,		stray,		stray,
	stray,		stray,		stray,		stray,
	stray,		stray,		stray,		stray,
	stray,		stray,		stray,		stray,
	stray,		stray,		stray,		stray,
	stray,		stray,		stray,		stray,
	stray,		stray,		stray,		stray,
	stray,		stray,		stray,		stray,
	stray,		stray,		stray,		stray,
	stray,		stray,		stray,		stray,
	stray,		stray,		stray,		stray,
	stray,		stray,		stray,		stray,
	stray,		stray,		stray,		stray,
	stray,		stray,		stray,		stray,
	stray,		stray,		stray,		stray
};

/*
 * After the first page of SCB, reserve another n pages
 * of SCB for the Unibus devices, where n is the number
 * of Unibus adapters.  It is initialised by "unifind"
 * to point to stray, then point to the interrupt handlers
 * as devices are found.
 *
 * Here we initialise 1 element to zero just to make sure
 * UNIvec is allocated in data space and therefore follows
 * scb. Ditto for vax8800bivec.
 */
int (*(UNIvec[2 * 128]))() = {0};

#if CVAXBI > 0
int (*(vax8800bivec[CVAXBI * 512]))() = {0};
#endif /* CVAXBI */

#if CVBA > 0
int (*(VMEvec[CVBA * 256]))() = {0};
#endif /* CVBA */

volatile int cvec = {0};/* Device interrupt vector			*/
volatile int br = {0};	/* Bus request (IPL level on vax)		*/

