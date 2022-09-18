#ifndef lint
static  char    *sccsid = "@(#)rtdsio.c	4.1  (ULTRIX)        7/2/90";
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

    Directory segment I/O on RT-11 device.

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
#include <stdio.h>

/*
 * TABLE OF CONTENTS:
 */

/*
 *  rt1seg - Get segment number of first directory segment.
 *  rtgets - Get directory segment.
 *  rtputs - Put directory segment.
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
    rtgetb (),
    rtputb ();

extern struct homblk
    homblk;

int rt1seg ()

/*********************************************************************

FUNCTIONAL DESCRIPTION:

    Computes the directory segment number of the first directory  seg-
    ment.

FORMAL PARAMETERS:

    None.

IMPLICIT INPUTS:

    homblk - The RT-11 device home block.

IMPLICIT OUTPUTS:

    None.

ROUTINE VALUE:

    The directory segment number of the first directory segment.

SIDE EFFECTS:

    None.

*********************************************************************/

{
    return ((int) (homblk.hb_dirs - DIRSEG1) / 2 + 1);
}

rtgets (segnum, ds_ptr)

int
    segnum;

struct dirseg
    * ds_ptr;

/*********************************************************************

FUNCTIONAL DESCRIPTION:

    Reads a directory segment an RT-11 device.

    The directory segments start in the block identified in  the  home
    block.  The directory segments are numbered starting at one.

FORMAL PARAMETERS:

    Segment_number.rg.v - The segment number of the directory  segment
	to read.
    Segment_buffer.wb.ra - The 1024-byte area into which the directory
	segment is to be read.

IMPLICIT INPUTS:

    None.

IMPLICIT OUTPUTS:

    None.

ROUTINE VALUE:

    None.

SIDE EFFECTS:

    Changes the current position in the RT-11 device image.

    May exit with an error message.

*********************************************************************/

{
    uword
	segblk;

#ifdef DEBUG
    fprintf (stderr, "getseg: getting segment %d\n", segnum);
#endif

    /*
     *  Get the segment.
     */

    segblk = (uword) (2 * (segnum - 1) + DIRSEG1);
    rtgetb (segblk ++, (char *) ds_ptr);
    rtgetb (segblk, ((char *) ds_ptr) + BLKSIZ);

    /*
     *  All done.
     */
}

rtputs (segnum, ds_ptr)

uword
    segnum;

struct dirseg
    * ds_ptr;

/*********************************************************************

FUNCTIONAL DESCRIPTION:

    Writes a directory segment to RT-11 device.

    The directory segments start in the block identified in  the  home
    block.  The directory segments are numbered starting at one.

FORMAL PARAMETERS:

    Segment_number.rg.v - The segment number of the directory  segment
	to be written.
    Segment_buffer.wb.ra - The 1024-byte area from which the directory
	segment is to be written.

IMPLICIT INPUTS:

    None.

IMPLICIT OUTPUTS:

    None.

ROUTINE VALUE:

    None.

SIDE EFFECTS:

    Changes the current position in the RT-11 device image.

    May exit with an error message.

*********************************************************************/

{
    uword
	segblk;

#ifdef DEBUG
    fprintf (stderr, "putseg: putting segment %d\n", segnum);
#endif

    /*
     *  Write the segment.
     */

    segblk = (uword) (2 * (segnum - 1) + DIRSEG1);
    rtputb (segblk ++, (char *) ds_ptr);
    rtputb (segblk, ((char *) ds_ptr) + BLKSIZ);

    /*
     *  All done.
     */
}
