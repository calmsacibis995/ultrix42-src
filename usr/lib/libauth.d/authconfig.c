#ifndef lint
static	char	*sccsid = "@(#)authconfig.c	4.1	(ULTRIX)	7/2/90";
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
 *   The information in this software is subject to change  without	*
 *   notice  and should not be construed as a commitment by Digital	*
 *   Equipment Corporation.						*
 *									*
 *   Digital assumes no responsibility for the use  or  reliability	*
 *   of its software on equipment which is not supplied by Digital.	*
 *									*
 ************************************************************************/
#include <stdio.h>
#include <strings.h>
#include <sys/svcinfo.h>
#include "auth.h"

#define	DEF_MIN_SIZE	6
#define	DEF_MAX_SIZE	16

/*
  Routine for reading in the configuration file.
*/
int max_pw_len = DEF_MAX_SIZE;
int min_pw_len = DEF_MIN_SIZE;
int soft_exp = 7*60*60*24;
int sec_level = SEC_ENHANCED;

config_auth()
{
	struct svcinfo *svcinfo;

	if((svcinfo=getsvc()) == NULL) {
		max_pw_len = DEF_MAX_SIZE;
		min_pw_len = DEF_MIN_SIZE;
		soft_exp = 7*60*60*24;
		sec_level = SEC_ENHANCED;
	} else {
		min_pw_len = svcinfo->svcauth.passlenmin;
		max_pw_len = svcinfo->svcauth.passlenmax;
		soft_exp = svcinfo->svcauth.softexp;
		sec_level = svcinfo->svcauth.seclevel;
	}
}
