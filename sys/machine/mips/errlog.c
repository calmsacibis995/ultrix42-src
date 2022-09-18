#ifndef lint
static char *sccsid = "@(#)errlog.c	4.1	(ULTRIX)	7/2/90";
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
 *                                                                      *
 *   The information in this software is subject to change  without	*
 *   notice  and should not be construed as a commitment by Digital	*
 *   Equipment Corporation.						*
 *									*
 *   Digital assumes no responsibility for the use  or  reliability	*
 *   of its software on equipment which is not supplied by Digital.	*
 *									*
 ************************************************************************/
/*
 *   File name: errlog.c
 *
 *   Source file description: 
 *   	This file contains mips dependent error logging routines.
 *
 *   Functions:
 *	logstray		Log a stray interrupt
 *
 *   Modification history:
 *	
 */
#include "../h/types.h"
#include "../h/time.h"
#include "../h/kernel.h"
#include "../h/param.h"
#include "../../machine/common/cpuconf.h"
#include "../h/errlog.h"


/*
 * Function: logstray(type, el_ipl, el_vec)
 *
 * Function description:  Log a stray interrupt (SCB or UBA) 
 *
 * Arguments: type - type of stray (SCB or UBA)
 *	      el_ipl - ipl level
 *	      el_vec - interrupt vector
 *
 * Return value: None
 *
 * Side effects: None
 *
 */
unsigned int stray_time=0;

logstray(type, el_ipl, el_vec)
long type;
long el_ipl;
long el_vec;
{
	struct el_rec *elrp;
	
	/* if zero vector, only log one error every hour */
	if (el_vec == 0) {
		if (((unsigned)time.tv_sec) - stray_time < 3600)
				return;

		stray_time = time.tv_sec;
	}
	elrp = ealloc(sizeof(struct el_strayintr), EL_PRILOW);
	if (elrp != EL_FULL) {
	    LSUBID(elrp,ELCT_SINT,type,EL_UNDEF,EL_UNDEF,EL_UNDEF,EL_UNDEF);
	    elrp->el_body.elstrayintr.stray_ipl = (u_char)el_ipl;
	    elrp->el_body.elstrayintr.stray_vec = (short)el_vec;
	    EVALID(elrp);
	}
}

