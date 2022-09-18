#ifndef lint
static char	*sccsid = "@(#)EXCEPT.c	1.2	(ULTRIX)	1/27/86";
#endif lint

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

/************************************************************************
*
*			Modification History
*
*		David Metsky,	27-Jan-86
*
* 001	Replaced old version with BSD 4.3 version as part of upgrade
*
*	Based on:	EXCEPT.c	1.4 (Berkeley)	10/1/83
*
*************************************************************************/

#include	<signal.h>

/*
 * catch runtime arithmetic errors
 */
EXCEPT(signum, type)
	int signum, type;
{
	signal(SIGFPE, EXCEPT);
#ifndef vax
	ERROR("Overflow, underflow, or division by zero in arithmetic operation\n");
	return;
#endif notvax
#ifdef vax
	/*
	 * The values for this switch statement come from page 12-5 of
	 * Volume 1 of the 1978 VAX 11/780 Architecture Handbook
	 */
	switch (type) {
	case FPE_INTOVF_TRAP:
		ERROR("Integer overflow\n");
		return;
	case FPE_INTDIV_TRAP:
		ERROR("Integer division by zero\n");
		return;
	case FPE_FLTOVF_TRAP:
	case FPE_FLTOVF_FAULT:
		ERROR("Real overflow\n");
		return;
	case FPE_FLTDIV_TRAP:
	case FPE_FLTDIV_FAULT:
		ERROR("Real division by zero\n");
		return;
	case FPE_FLTUND_TRAP:
	case FPE_FLTUND_FAULT:
		ERROR("Real underflow\n");
		return;
	case FPE_DECOVF_TRAP:
	case FPE_SUBRNG_TRAP:
	default:
		ERROR("Undefined arithmetic exception type (%d)\n", type);
		return;
	}
#endif vax
}
