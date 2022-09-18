#ifndef lint
static  char    *sccsid = "@(#)rtdate.c	4.1  (ULTRIX)        7/2/90";
#endif lint

/************************************************************************
 *                                                                      *
 *                      Copyright (c) 1984 by                           *
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
 *                                                                      *
 ************************************************************************/

/*
FACILITY:

    RT-11 volume manipulation.

ABSTRACT:

    Constructs the current date in RT-11 format.

ENVIRONMENT:

    PRO/VENIX user mode.
    ULTRIX-11 user mode.
    ULTRIX-32 user mode.

AUTHOR: Brian Hetrick, CREATION DATE: 1 March 1985.

MODIFIED BY:

	Brian Hetrick, 01-Mar-85: Version 1.0
  000 - Original version of module.

*/

/*
 * INCLUDE FILES:
 */

#include "arff.h"
#include <time.h>

/*
 * TABLE OF CONTENTS:
 */

/*
 * MACROS:
 */

/*
 * EQUATED SYMBOLS:
 */

/*
 * OWN STORAGE:
 */

/*
 * EXTERNAL REFERENCES:
 */

extern long
    time ();            /*  Get current time.                       */

extern struct tm
    * localtime ();     /*  Get broken down local time.             */

word rtdate ()

/*********************************************************************

FUNCTIONAL DESCRIPTION:

    Constructs the current date in RT-11 format.

FORMAL PARAMETERS:

    None.

IMPLICIT INPUTS:

    None.

IMPLICIT OUTPUTS:

    None.

ROUTINE VALUE:

    The current date in RT-11 format.

SIDE EFFECTS:

    None.

*********************************************************************/

{
    word
	rttime;

    long
	curtim;

    struct tm
	* ltime;

#ifdef DEBUG
    fprintf (stderr, "rtdate: entry\n");
#endif

    time (& curtim);
    ltime = localtime (& curtim);

    rttime =
	((ltime -> tm_mon  +  1) << 10) |
	((ltime -> tm_mday     ) <<  5) |
	((ltime -> tm_year - 72)      );

    return (rttime);
}
