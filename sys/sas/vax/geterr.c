/*
 * geterr.c
 */
#ifndef lint
static char *sccsid = "@(#)geterr.c	4.1	ULTRIX	7/2/90";
#endif lint
/************************************************************************
 *									*
 *			Copyright (c) 1985 by				*
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
#include "vmb.h"

char	*errmsg;
char	errmsgbuf[40];

/*
 * Functional Discription:
 *	This routine is called when reporting errors returned from the
 *	VMB routines.  It returns a pointer to a string describing the 
 *	meaning of the input error code.
 *
 * Inputs:
 *	VMB error code
 *
 * Outputs:
 *	pointer to error message string
 *
 */
char *geterr(code)
	int	code;
{
	switch (code&0xffff) {
	case SS$_PARITY:
		errmsg="Parity Error";
		break;
	case SS$_BUFBYTALI:
		errmsg="Invalid Buffer Alignment";
		break;
	case SS$_CTLRERR: 
		errmsg="Fatal Controller Error";
		break;
	case SS$_NOSUCHDEV:
		errmsg="No Such Device";
		break;
	case SS$_ENDOFFILE:
		errmsg="End Of File";
		break;
	case SS$_DEVOFFLINE:
		errmsg="Device Offline";
		break;
	default:
		sprintf(errmsgbuf, "VMB error code %x", code);
		errmsg=errmsgbuf;
		break;
	}
	return(errmsg);
}
