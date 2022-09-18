#ifndef lint
static  char    *sccsid = "@(#)arcpad.c	4.1  (ULTRIX)        7/2/90";
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

    Copy and blank-pad a string.

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

arcpad (dest, sour, len)

register char
    * dest,
    * sour;

register int
    len;

/*********************************************************************

FUNCTIONAL DESCRIPTION:

    Copies a source string to a destination string, and pads the dest-
    ination  string to a specified length with blanks.  Does not term-
    inate the destination string with a NUL.

FORMAL PARAMETERS:

    Destination.wt.r - The destination string.
    Source.rt.r - The source string.
    Length.rg.v - The desired length of the destination string.

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
    /*
     *  Copy source to destination.
     */

    while ((len > 0) && ('\0' != * sour))
    {
	* dest ++ = * sour ++;
	len --;
    }

    /*
     *  Blank fill destination.
     */

    while (len > 0)
    {
	* dest ++ = ' ';
	len --;
    }
}
