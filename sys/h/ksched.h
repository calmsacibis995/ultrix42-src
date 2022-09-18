/*
 *	@(#)ksched.h	4.1	(ULTRIX)	7/2/90
 */

/************************************************************************
 *                                                                      *
 *                      Copyright (c) 1986 by                           *
 *              Digital Equipment Corporation, Maynard, MA              *
 *                      All rights reserved.                            *
 *                                                                      *
 *   This software is furnished under a license and may be used and     *
 *   copied  only  in accordance with the terms of such license and     *
 *   with the  inclusion  of  the  above  copyright  notice.   This     *
 *   software  or  any  other copies thereof may not be provided or     *
 *   otherwise made available to any other person.  No title to and     *
 *   ownership of the software is hereby transferred.                   *
 *                                                                      *
 *   The information in this software is subject to change  without     *
 *   notice  and should not be construed as a commitment by Digital     *
 *   Equipment Corporation.                                             *
 *                                                                      *
 *   Digital assumes no responsibility for the use  or  reliability     *
 *   of its software on equipment which is not supplied by Digital.     *
 *                                                                      *
 ************************************************************************
 *
 *
 *   Facility:	Kernel Subroutine Scheduler
 *
 *   Abstract:	This module contains all the data structure definitions
 *		constants and macros required for the kernel subroutine
 *		scheduler.
 *
 *   Creator:	Todd M. Katz	Creation Date:	July 16, 1986
 *
 *   History:
 *
 *   21-Sep-89	Pete Keilty
 *	Removed ipl from structure calling routine are the ones
 *	responsible for raising there ipl level.
 *
 */
/**/

/* Data Structures.
 */
struct	kschedblk {			/* Subroutine scheduler control block*/
	caddr_t 	 arg;		/* Kernel subroutine arguement	     */
	void		 (*func)();	/* Address of kernel subroutine      */
	struct kschedblk *next;		/* Link to next kschedblk on list    */
};
#ifdef KERNEL
struct	kschedblk *kschedq;		/* Listhead for scheduled subroutines*/
#endif
