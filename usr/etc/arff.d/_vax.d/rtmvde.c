#ifndef lint
static  char    *sccsid = "@(#)rtmvde.c	4.1  (ULTRIX)        7/2/90";
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

    Copies a directory entry.

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

rtmvde (ptr_de, ptr_so, ds_ptr)

struct dirseg
    * ds_ptr;

struct dirent
    * ptr_de,
    * ptr_so;

/*********************************************************************

FUNCTIONAL DESCRIPTION:

    Copies a directory entry from one place to another.

FORMAL PARAMETERS:

    Directory_segment.rr.r - The current directory segment.
    Destination.wr.r - The destination directory entry.
    Source.rr.r - The source directory entry.

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
	* chr_de,
	* chr_so;

    register int
	i;

#ifdef DEBUG
    fprintf (stderr, "rtmvde: copying from %06o to %06o\n",
	ptr_so, ptr_de);
#endif

    for (i = sizeof (struct dirent) + sizeof (word) * ds_ptr -> ds_xtra,
	    chr_de = (char *) ptr_de, chr_so = (char *) ptr_so;
	 i > 0;
	 i --)
    {
	* chr_de ++ = * chr_so ++;
    }
}
