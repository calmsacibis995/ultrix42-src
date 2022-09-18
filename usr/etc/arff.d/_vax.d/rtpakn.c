#ifndef lint
static  char    *sccsid = "@(#)rtpakn.c	4.1  (ULTRIX)        7/2/90";
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

    Pack a 9-character ASCII file name into a readable format.

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

#include <stdio.h>

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

extern int
    arnlen ();          /*  Find non-blank length of string.        */

rtpakn (unpacked, packed)

char
    * unpacked;

register char
    * packed;

/*********************************************************************

FUNCTIONAL DESCRIPTION:

    Packs an unpacked file name (six characters file name  immediately
    followed by three characters extension) into "normal" format (0 to
    5 characters file name, period, 0 to 3 characters extension).

FORMAL PARAMETERS:

    Unpacked_name.rt.r - The unpacked name.
    Packed_name.wt.r - The packed name.

IMPLICIT INPUTS:

    None.

IMPLICIT OUTPUTS:

    None.

ROUTINE VALUE:

    None.

SIDE EFFECTS:

    None.

*********************************************************************/

{
    register char
	* namptr;

    register int
	namlen;

#ifdef DEBUG
    fprintf (stderr, "paknam: entry\n");
#endif

    /*
     *  Get the filename portion.
     */

    for (namptr = unpacked, namlen = arnlen (namptr, 6);
	 namlen > 0;
	 namlen --)
    {
	* packed ++ = * namptr ++;
    }

    /*
     *  Put in the period.
     */

	    * packed ++ = '.';

    /*
     *  Get the extension portion.
     */

    for (namptr = unpacked + 6, namlen = arnlen (namptr, 3);
	 namlen > 0;
	 namlen --)
    {
	* packed ++ = * namptr ++;
    }

    /*
     *  Terminate the string.
     */

    * packed = '\0';

    /*
     *  That's it.
     */
}
