#ifndef lint
static  char    *sccsid = "@(#)ardown.c	4.1  (ULTRIX)        7/2/90";
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

    Translates a string to lower case.

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

#include <ctype.h>
#include "arff.h"

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

ardown (outs, ins)

register char
    * outs,
    * ins;

/*********************************************************************

FUNCTIONAL DESCRIPTION:

    Translates a string to lower case.

FORMAL PARAMETERS:

    Input_string.rt.r - The string to be translated to lower case.
    Output_string.wt.r - The lower case equivalent of Input_string.

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
    register int
	inc;

    do
    {
	/*
	 *  Get source character.
	 */

	inc = (* ins ++) & CHARMASK;

	/*
	 *  Translate to lower case.
	 */

	if (isascii (inc) && isupper (inc))
	{
	    inc = tolower (inc);
	}

	/*
	 *  Store destination character.
	 */

	* outs ++ = inc;
    } while ('\0' != inc);
}
