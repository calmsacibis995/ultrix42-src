#ifndef lint
static  char    *sccsid = "@(#)rtcons.c	4.1  (ULTRIX)        7/2/90";
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

    Consolidates free space entries in a directory segment.

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
    rtidep (),          /*  Increment directory entry pointer.      */
    rtmvde (),          /*  Move directory entries.                 */
    rtputs ();          /*  Write RT-11 directory segment.          */

rtcons (ds_ptr, segnum)

struct dirseg
    * ds_ptr;

int
    segnum;

/*********************************************************************

FUNCTIONAL DESCRIPTION:

    Consolidate empty directory entries within a single directory seg-
    ment,  and  delete zero-length empty directory entries.  Write the
    consolidated directory segment to the RT-11 device image.

FORMAL PARAMETERS:

    Directory_segment.mr.r - The directory entry.
    Segment_number.rg.v - The directory segment number.

IMPLICIT INPUTS:

    None.

IMPLICIT OUTPUTS:

    None.

ROUTINE VALUE:

    None.

SIDE EFFECTS:

    Writes the directory segment to the RT-11 file.

*********************************************************************/

{
    struct dirent
	* dep_de,       /*  Destination directory entry pointer.    */
	* dep_so,       /*  Source directory entry pointer.         */
	* de_ptr;       /*  Top of list directory entry pointer.    */

#ifdef DEBUG
    fprintf (stderr, "rtcons:\n");
#endif

    /*
     *  Scan the directory entries.
     */

    de_ptr = (struct dirent *) (& (ds_ptr -> ds_dent [0]));
    while (0 == (de_ptr -> de_stat & DE_ENDS))
    {
#ifdef DEBUG
	fprintf (stderr, "Looking at entry at %06o\n", de_ptr);
#endif

	/*
	 *  If this entry is empty, compress.
	 */

	if (de_ptr -> de_stat & DE_EMPT)
	{
#ifdef DEBUG
	    fprintf (stderr, "Accumulating empty entries\n");
#endif
	    /*
	     *  Accumulate empty entries.
	     */

	    dep_so = de_ptr;
	    rtidep (ds_ptr, & dep_so);
	    dep_de = dep_so;
	    while (dep_so -> de_stat & DE_EMPT)
	    {
#ifdef DEBUG
		fprintf (stderr, "Entry at %06o is empty\n", dep_so);
#endif
		de_ptr -> de_nblk += dep_so -> de_nblk;
		rtidep (ds_ptr, & dep_so);
	    }

	    /*
	     *  Delete now unused empty  entries.   This  includes
	     *  the current entry if it has zero length.
	     */

	    if (0 == de_ptr -> de_nblk)
	    {
#ifdef DEBUG
		fprintf (stderr, "Zero length empty at %06o\n", de_ptr);
#endif
		dep_de = de_ptr;
	    }

	    if (dep_so != dep_de)
	    {
#ifdef DEBUG
		fprintf (stderr, "Copying directory entries\n");
#endif
		while (0 == (dep_so -> de_stat & DE_ENDS))
		{
		    rtmvde (dep_de, dep_so, ds_ptr);
		    rtidep (ds_ptr, & dep_so);
		    rtidep (ds_ptr, & dep_de);
		}
		dep_de -> de_stat = DE_ENDS;
	    }
	}

	/*
	 *  Point at the next used entry.  Note that deleting  the
	 *  zero-length  empty  entries may have made de_ptr point
	 *  to the end of the segment.
	 */

	if (0 == (de_ptr -> de_stat & DE_ENDS))
	{
	    rtidep (ds_ptr, & de_ptr);
	}
    }

    /*
     *  Rewrite the directory segment.
     */

#ifdef DEBUG
    fprintf (stderr, "Writing directory entry back\n");
#endif

    rtputs (segnum, ds_ptr);

    /*
     *  All done.
     */
}
