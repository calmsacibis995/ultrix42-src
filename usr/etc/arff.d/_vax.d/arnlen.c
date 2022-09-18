#ifndef lint
static  char    *sccsid = "@(#)arnlen.c	4.1  (ULTRIX)        7/2/90";
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

    Find the non-blank length of a string.

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

int arnlen (str, max)

register char
    * str;

int
    max;

/*********************************************************************

FUNCTIONAL DESCRIPTION:

    Determines the non-blank length of a string.

FORMAL PARAMETERS:

    String.rt.r - The string whose non-blank length is to be found.
    Maximum_length.rg.v - The number of characters in String.

IMPLICIT INPUTS:

    None.

IMPLICIT OUTPUTS:

    None.

ROUTINE VALUE:

    The number of characters in String until the last non-blank  char-
    acter.  Zero if String is entirely blank.

SIDE EFFECTS:

    None.

*********************************************************************/

{
    register int
	i;

    /*
     *  If maximum length is non-positive, non-blank length is 0.
     */

    if (max <= 0)
    {
	return (0);
    }

    /*
     *  Find index of last non-blank character.
     */

    for (i = max - 1; (i >= 0) && (str [i] == ' '); i --)
    {
    }

    /*
     *  Correct for 0-based indexing.
     */

    return (i + 1);
}
