/*	@(#)psl.h	1.3	(ULTRIX)	2/12/86 */

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

/*-----------------------------------------------------------------------
 *
 * Modification History
 *
 * 12-Feb-86 -- jrs
 *	Added defn to quantify lower priority processes for use by BASEPRI
 *
 *	Derived from 4.2 BSD labelled:
 *		psl.h	6.1	83/07/29
 *
 *-----------------------------------------------------------------------
 */

/*
 * VAX program status longword
 */

#define	PSL_C		0x00000001	/* carry bit */
#define	PSL_V		0x00000002	/* overflow bit */
#define	PSL_Z		0x00000004	/* zero bit */
#define	PSL_N		0x00000008	/* negative bit */
#define	PSL_ALLCC	0x0000000f	/* all cc bits - unlikely */
#define	PSL_T		0x00000010	/* trace enable bit */
#define	PSL_IV		0x00000020	/* integer overflow enable bit */
#define	PSL_FU		0x00000040	/* floating point underflow enable */
#define	PSL_DV		0x00000080	/* decimal overflow enable bit */
#define	PSL_IPL		0x001f0000	/* interrupt priority level */
#define	PSL_PRVMOD	0x00c00000	/* previous mode (all on is user) */
#define	PSL_CURMOD	0x03000000	/* current mode (all on is user) */
#define	PSL_IS		0x04000000	/* interrupt stack */
#define	PSL_FPD		0x08000000	/* first part done */
#define	PSL_TP		0x40000000	/* trace pending */
#define	PSL_CM		0x80000000	/* compatibility mode */

#define	PSL_MBZ		0x3020ff00	/* must be zero bits */

#define	PSL_USERSET	(PSL_PRVMOD|PSL_CURMOD)
#define	PSL_USERCLR	(PSL_IS|PSL_IPL|PSL_MBZ)

#define	PSL_IPL_LOW	0x00010000	/* highest ipl not at intr level */
