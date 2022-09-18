#ifndef lint
static  char    *sccsid = "@(#)rtspds.c	4.1  (ULTRIX)        7/2/90";
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

    Splits a directory segment into two.

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
    exit (),            /*  Terminate program.                      */
    fprintf (),         /*  Print message to file.                  */
    rtgets (),          /*  Get directory segment.                  */
    rtidep (),          /*  Increment directory entry pointer.      */
    rtmvde (),          /*  Move directory entries.                 */
    rtputs (),          /*  Put directory segment.                  */
    unlink ();          /*  Delete named file.                      */

extern char
    * pgmnam;           /*  Program's name.                         */

extern struct homblk
    homblk;             /*  Device home block.                      */

extern struct dirseg
    dirseg;             /*  Directory segment buffer.               */

rtspds (segnum, tmpnam)

int
    segnum;

char
    * tmpnam;

/*********************************************************************

FUNCTIONAL DESCRIPTION:

    Splits a full directory segment into two pieces.

FORMAL PARAMETERS:

    Segment_number.rg.v - The segment number  of  the  segment  to  be
	split.
    Failure_file.rt.r - The name of a file to delete if the  directory
	segment splitting fails.

IMPLICIT INPUTS:

    homblk - The RT-11 device home block.

IMPLICIT OUTPUTS:

    dirseg - The directory segment work area.

ROUTINE VALUE:

    None.

SIDE EFFECTS:

    May exit with an error message.

*********************************************************************/

{
    int
	new_num,                /*  Segment number of new segment.  */
	save_stat;              /*  Saved directory entry status.   */

    uword
	new_orig,               /*  New segment starting block num. */
	save_next;              /*  Saved next directory seg. ptr.  */

    struct dirent
	* deptr1,               /*  Directory entry                 */
	* deptr2;               /*   pointers.                      */

#ifdef DEBUG
    fprintf (stderr, "rtspds:  entry\n");
#endif

    /*
     *  Find next available segment number.
     */

    rtgets (1, & dirseg);
    new_num = dirseg.ds_hseg + 1;
    if (new_num > dirseg.ds_nseg)
    {
	fprintf (stderr, "%s: No free directory segments\n", pgmnam);
	unlink (tmpnam);
	exit (1);
    }

#ifdef DEBUG
    fprintf (stderr, "rtspds: next unused segment %d\n",
	new_num);
#endif

    /*
     *  Update next available segment number.
     */

    dirseg.ds_hseg = new_num;
    rtputs (1, & dirseg);

    /*
     *  Get the directory segment to be split.
     */

    rtgets (segnum, & dirseg);

    /*
     *  Save/set the "next segment" information.
     */

    save_next = dirseg.ds_next;
    dirseg.ds_next = new_num;

    /*
     *  Find an entry at which to split.
     */

    new_orig = dirseg.ds_sblk;
    deptr1 = (struct dirent *) (& dirseg.ds_dent [0]);
    while (deptr1 < (struct dirent *) (& dirseg.ds_dent [253]))
    {
	new_orig += deptr1 -> de_nblk;
	rtidep (& dirseg, & deptr1);
    }

    /*
     *  Form the first of the resulting  segments  by  truncating  the
     *  directory  segment  at  roughly  the halfway mark.  Do this by
     *  prematurely setting the end of segment marker.
     */

    save_stat = deptr1 -> de_stat;
    deptr1 -> de_stat = DE_ENDS;

    /*
     *  Write the first resulting segment to disk.
     */

#ifdef DEBUG
    fprintf (stderr, "rtspds: Writing back modified directory segment\n");
#endif

    rtputs (segnum, & dirseg);

    /*
     *  Recreate the original directory segment.
     */

    deptr1 -> de_stat = save_stat;
    dirseg.ds_next = save_next;

    /*
     *  Form the second of the resulting segments by throwing away the
     *  directory entries that are in the first directory segment.
     */

#ifdef DEBUG
    fprintf (stderr, "rtspds: moving entries up\n");
#endif

    deptr2 = (struct dirent *) (& dirseg.ds_dent [0]);
    while (! (deptr1 -> de_stat & DE_ENDS))
    {
	rtmvde (deptr2, deptr1, & dirseg);
	rtidep (& dirseg, & deptr1);
	rtidep (& dirseg, & deptr2);
    }
    deptr2 -> de_stat = DE_ENDS;
    dirseg.ds_sblk = new_orig;

    /*
     *  Write the resulting directory segment to disk.
     */

#ifdef DEBUG
    fprintf (stderr, "rtspds: writing new directory segment %d\n",
	new_num);
#endif

    rtputs (new_num, & dirseg);
}
