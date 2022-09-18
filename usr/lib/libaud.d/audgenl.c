#ifndef lint
static char *sccsid = "@(#)audgenl.c	4.1	ULTRIX	8/8/90";
#endif lint

/************************************************************************
 *									*
 *                      Copyright (c) 1990 by                           *
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
 *   08 Aug 90 - scott
 *      created file
 *
*/

#include <sys/audit.h>
#include <varargs.h>

/* variable format front-end for audgen(2) */
audgenl ( va_alist )
va_dcl
{
    va_list ap;
    unsigned event;
    char tokenp[AUD_NPARAM];
    char *audargv[AUD_NPARAM];
    int i;

    va_start ( ap );
    event = va_arg ( ap, unsigned );
    for ( i = 0; i < AUD_NPARAM; i++ ) {
        if ( (tokenp[i] = va_arg ( ap, int )) == 0 ) break;
        if ( A_TOKEN_PTR(tokenp[i]) ) audargv[i] = va_arg ( ap, char * );
        else audargv[i] = (char *)va_arg ( ap, int );
    }
    va_end ( ap );

    i = audgen ( event, tokenp, audargv );
    return(i); 
}
