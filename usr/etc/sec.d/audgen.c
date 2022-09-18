#ifndef lint
static char *sccsid = "@(#)audgen.c	4.1	ULTRIX	7/2/90";
#endif lint

/************************************************************************
 *									*
 *                      Copyright (c) 1989 by                           *
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
*/

/* command line interface to audgen() */
#include <sys/audit.h>
main ( argc, argv )
int argc;
char *argv[];
{
    char tmask[AUD_NPARAM];
    int i;

    if ( argc == 1 ) {
        printf ( "Usage: param[s]\n" );
        exit();
    }

    for ( i = 0; i < argc-1; i++ ) tmask[i] = T_CHARP;
    for ( ; i < AUD_NPARAM; i++ ) tmask[i] = '\0';
    if ( audgen ( AUDGEN8, tmask, &argv[1] ) == -1 ) perror ( "audgen" );
}
