
/*
 *	@(#)dkio.h	4.1	(ULTRIX)	7/2/90
 */

/************************************************************************
 *									*
 *			Copyright (c) 1984, 1986 by			*
 *		Digital Equipment Corporation, Maynard, MA		*
 *			All rights reserved.				*
 *									*
 *   This software is furnished under a license and may be used and	*
 *   copied  only  in accordance with the terms of such license and	*
 *   with the  inclusion  of  the  above  copyright  notice.   This	*
 *   software  or  any	other copies thereof may not be provided or	*
 *   otherwise made available to any other person.  No title to and	*
 *   ownership of the software is hereby transferred.			*
 *									*
 *   This software is  derived	from  software	received  from	the	*
 *   University    of	California,   Berkeley,   and	from   Bell	*
 *   Laboratories.  Use, duplication, or disclosure is	subject  to	*
 *   restrictions  under  license  agreements  with  University  of	*
 *   California and with AT&T.						*
 *									*
 *   The information in this software is subject to change  without	*
 *   notice  and should not be construed as a commitment by Digital	*
 *   Equipment Corporation.						*
 *									*
 *   Digital assumes no responsibility for the use  or	reliability	*
 *   of its software on equipment which is not supplied by Digital.	*
 *									*
 ************************************************************************/

/*
 * dkio.h	6.1	07/29/83
 *
 * Modification history
 *
 * Common structures and definitions for disk drivers and ioctl
 *
 *  6-Mar-86 - ricky palmer
 *
 *	Derived from 4.2BSD labeled: mtio.h	6.1	83/07/29.
 *	Moved DKIOCHDR to ioctl.h. V2.0
 *	Added dkop and dkget structures. V2.0
 *
 *  1-May-86 - ricky palmer
 *
 *	Added dkacc structure for bad block replacement effort. V2.0
 *
 */
#define NO_ACCESS -13	/* No access mscp command suport */

/* Structure for DKIOCDOP ioctl - disk operation command */
struct	dkop	{
	short	dk_op;			/* Operations defined below	*/
	daddr_t dk_count;		/* How many of them		*/
};

/* Structure for DKIOCGET ioctl - disk get status command */
struct	dkget	{
	short	dk_type;		/* Type of device defined below */
	short	dk_dsreg;		/* ``drive status'' register	*/
	short	dk_erreg;		/* ``error'' register		*/
	short	dk_resid;		/* Residual count		*/
};

/* Structure for DKIOCACC ioctl - disk access command */
struct	dkacc	{
	short	dk_opcode;		/* Operation code for access	*/
	long	dk_lbn;			/* Disk sector number		*/
	long	dk_length;		/* Length of transfer		*/
	unsigned dk_status;		/* Status of operation		*/
	unsigned dk_flags;		/* Status flags			*/
};
