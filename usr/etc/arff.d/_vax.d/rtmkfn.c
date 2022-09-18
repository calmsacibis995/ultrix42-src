#ifndef lint
static  char    *sccsid = "@(#)rtmkfn.c	4.1  (ULTRIX)        7/2/90";
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

    Derives an RT-11 file name from a native file name.

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

extern
    arar50 (),          /*  ASCII to RADIX 50 conversion.           */
    arr50a (),          /*  RADIX 50 to ASCII conversion.           */
    rtpakn ();          /*  Pack 9-character file name.             */

rtmkfn (s, r, p)

char
    * p,
    * s;

rad50
    * r;

/*********************************************************************

FUNCTIONAL DESCRIPTION:

    Makes a RADIX 50 RT-11 file name block from a native file name.

    Any path prefix is skipped.  The last file name of the native file
    name  is  used  to  construct the RT-11 file name.  The RT-11 file
    file name constructed depends on the native file name as follows:

     o  If the native file name has the form x.y, with x being at most
	six characters, then the RT-11 file name has x as the filename
	and the first three non-period characters of y as  the  exten-
	sion.
     o  If the native file name has the form x.y, with  x  being  more
	than six characters, or if the native file name as the form x,
	then the RT-11 file name as the first six characters of  x  as
	the  filename  and  the  next  three  characters  of  x as the
	extension.

FORMAL PARAMETERS:

    Native_name.rt.r - The native file name.
    RAD50_block.wr.r - The three-word RADIX 50 name block.
    ASCII_name.wt.r - The packed ASCII equivalent of RAD50_block.

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
    char
	* namptr1,
	* namptr2,
	upknam [9],
	* upkptr;

    int
	i,
	inchar;

    /*
     *  Skip over leading path name.
     */

    namptr1 = s;
    namptr2 = s;
    while ('\0' != (inchar = * namptr2 ++))
    {
	if ('/' == inchar)
	{
	    namptr1 = namptr2;
	}
    }

    /*
     *  Form unpacked ASCII name.
     */

    upkptr = & upknam [0];
    for (i = 0;
	 (i < 9) && ('\0' != (inchar = * namptr1 ++)) && ('.' != inchar);
	 i ++)
    {
	* upkptr ++ = inchar;
    }

    if (('.' == inchar) && (i <= 6))
    {
	while (i < 6)
	{
	    * upkptr ++ = ' ';
	    i ++;
	}
	while ((i < 9) && ('\0' != (inchar = * namptr1 ++)) && ('.' != inchar))
	{
	    * upkptr ++ = inchar;
	    i ++;
	}
    }

    while (i < 9)
    {
	* upkptr ++ = ' ';
	i ++;
    }

    /*
     *  Get the RADIX 50 file name block.
     */

    arar50 (upknam, r, 9);

    /*
     *  Form packed ASCII name.
     */

    arr50a (r, upknam, 9);
    rtpakn (upknam, p);

#ifdef DEBUG
    fprintf (stderr,
	"rtrepl\\maknam: from \"%s\" formed \"%s\"\n", s, p);
#endif
}
