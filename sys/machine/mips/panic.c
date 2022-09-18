#ifndef lint
static char *sccsid = "@(#)panic.c	4.1	(ULTRIX)	7/2/90";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1983,86,87 by			*
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
 * Modification History
 *
 *	04-Oct-89 gmm
 *		Put back all the common code for panic() in subr_prf.c.
 *		With smp changes for MIPS code, there is no need to have
 *		separate panic() routines
 *	09-Feb-89 Randall Brown
 *		Created file.  Separated file from subr_prf.c
 *
 */

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/seg.h"
#include "../h/buf.h"
#include "../h/conf.h"
#include "../h/reboot.h"
#include "../h/vm.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/proc.h"
#include "../h/ioctl.h"
#include "../h/tty.h"
#include "../h/types.h"
#include "../h/errlog.h"
#include "../h/kmalloc.h"
#include "../h/cpudata.h"
#include "../machine/cpu.h"

panic_log(s)
	char *s;
{
	int i;
	char *strp;
	struct el_rec *elrp;
	struct el_pnc *elpp;
	/*
	 * Allocate an error log packet for the panic info
	 */
	elrp = ealloc(EL_PNCSIZE, EL_PRISEVERE);
	if (elrp != NULL) {
	    LSUBID(elrp,ELSW_PNC,EL_UNDEF,EL_UNDEF,EL_UNDEF,EL_UNDEF,EL_UNDEF);
	    elpp = &elrp->el_body.elpnc;

	    /*
	     * Put the panic string into the error log packet
	     */
	    strp = s;
	    for (i = 1; *strp++ != '\0' && i < EL_SIZE64; i++) ;
	    bcopy(s, elpp->pnc_asc, i);
	    elpp->pnc_asc[i-1] = '\0';

/* afdfix: may want to capture stack dump or all system registers for
mips chips */
	    EVALID(elrp);
	}

}

