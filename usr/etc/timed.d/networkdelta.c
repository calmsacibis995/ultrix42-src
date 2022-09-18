#ifndef lint
static	char	*sccsid = "@(#)networkdelta.c	4.1	(ULTRIX)	7/2/90";
#endif lint

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
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *static char sccsid[] = "@(#)networkdelta.c	2.1 (Berkeley) 12/10/85";
 */


#include "globals.h"
#include <protocols/timed.h>

extern int machup;

/*
 * `networkdelta' selects the largest set of deltas that fall within the
 * interval RANGE, and uses them to compute the network average delta 
 */

long networkdelta()
{
	int i, j, maxind, minind;
	int ext;
	int tempind;
	long tempdata;
	long x[NHOSTS];
	long average;

	for (i=0; i<slvcount; i++)
		x[i] = hp[i].delta;
	for (i=0; i<slvcount-1; i++) {
		tempdata = x[i];
		tempind = i;
		for (j=i+1; j<slvcount; j++) {
			if (x[j] < tempdata) {
				tempdata = x[j];
				tempind = j;
			}
		}
		x[tempind] = x[i];
		x[i] = tempdata;
	}

	/* this piece of code is critical: DO NOT TOUCH IT! */
/****/
	i=0; j=1; minind=0; maxind=1;
	if (machup == 2)
		goto compute;
	do {
		if (x[j]-x[i] <= RANGE)
			j++;
		else {
			if (j > i+1) 
 				j--; 
			if ((x[j]-x[i] <= RANGE) && (j-i >= maxind-minind)) {
				minind=i;
				maxind=j;
			}	
			i++;
			if(i = j)
				j++;
		}
	} while (j < machup);
	if ((x[machup-1] - x[i] <= RANGE) && (machup-i-1 >= maxind-minind)) {
		minind=i; maxind=machup-1;
	}
/****/
compute:
	ext = maxind - minind + 1;
	average = 0;
	for (i=minind; i<=maxind; i++)
		average += x[i];
	average /= ext;
	return(average);
}
