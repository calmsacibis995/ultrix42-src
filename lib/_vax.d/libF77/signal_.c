#ifndef lint
static char	*sccsid = " @(#)signal_.c	1.2	(ULTRIX)	1/16/86";
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
*	David Metsky		10-Jan-86
*
* 001	Replaced old version with BSD 4.3 version as part of upgrade.
*
*	Based on:	signal_.c	5.3		9/11/85
*
*************************************************************************/

/* change the action for a specified signal
 *
 * calling sequence:
 *	integer cursig, signal, savsig
 *	external proc
 *	cursig = signal(signum, proc, flag)
 * where:
 *	'cursig' will receive the current value of signal(2)
 *	'signum' must be in the range 0 <= signum <= 32
 *
 *	If 'flag' is negative, 'proc' must be an external proceedure name.
 *	
 *	If 'flag' is 0 or positive, it will be passed to signal(2) as the
 *	signal action flag. 0 resets the default action; 1 sets 'ignore'.
 *	'flag' may be the value returned from a previous call to signal.
 *
 * This routine arranges to trap user specified signals so that it can
 * pass the signum fortran style - by address. (boo)
 */

#include	"../libI77/f_errno.h"

static int (*dispatch[33])();
int (*signal())();
int sig_trap();

long signal_(sigp, procp, flag)
long *sigp, *flag;
int (*procp)();
{
	int (*oldsig)();
	int (*oldispatch)();

	oldispatch = dispatch[*sigp];

	if (*sigp < 0 || *sigp > 32)
		return(-((long)(errno=F_ERARG)));

	if (*flag < 0)	/* function address passed */
	{
		dispatch[*sigp] = procp;
		oldsig = signal((int)*sigp, sig_trap);
	}

	else		/* integer value passed */
		oldsig = signal((int)*sigp, (int)*flag);

	if (oldsig == sig_trap)
		return((long)oldispatch);
	return((long)oldsig);
}

sig_trap(sn)
int sn;
{
	long lsn = (long)sn;
	return((*dispatch[sn])(&lsn));
}
