#ifndef lint
static char *sccsid = "@(#)audit_data.c	4.2    ULTRIX  8/7/90";
#endif lint
/************************************************************************
 *									*
 *                      Copyright (c) 1989,1990 by                      *
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
 *									*
 ************************************************************************/

/*
 *
 *   Modification history:
 *
 *   07 Jul 89 - scott
 *      created file
 *
 *   06 Aug 90 - scott
 *      add audsize flag
 *
*/

#include "audit.h"
#include "../h/audit.h"

/* global data */
int audswitch = 0;      /* audit on/off */
char audstyle = '\0';   /* style flags  */
#ifdef AUDIT
int audsize = AUDIT;    /* buffer size  */
#endif AUDIT

#ifndef AUDIT

/* need to be defined, but never used if AUDIT not defined */
char syscallauditmask[1];
char trustedauditmask[1];
char aud_param[1][AUD_NPARAM];
char aud_shm[1][AUD_NPARAM];

audcntl() {
}

audgen() {
}

audit_rec_build() {
}

#endif AUDIT
