#ifndef lint
static  char    *sccsid = "@(#)rtscnd.c	4.1  (ULTRIX)        7/2/90";
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

    Scans the device directory and calls user routine for each entry.

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
    rtgets (),          /*  Get directory segment.                  */
    rtidep ();          /*  Increment directory entry pointer.      */

extern int
    rt1seg ();          /*  Get first directory segment number.     */

extern struct homblk
    homblk;             /*  Device home block.                      */

extern struct dirseg
    dirseg;             /*  Directory segment buffer.               */

rtscnd (status, entrtn, segrtn)

word
    status;

int
    (* entrtn) (),
    (* segrtn) ();

/*********************************************************************

FUNCTIONAL DESCRIPTION:

    Scans a directory looking for entries that match  a  set  of  file
    status  codes.   For every entry that matches, calls a caller-sup-
    plied action routine.

FORMAL PARAMETERS:

    Status.rw.v - A mask which is anded with the status flags of  each
	directory  entry.  The Action_routine is called for each dire-
	ctory entry for which this result is non-zero.
    Entry_action.ra.v - The address of an action routine to be  called
	for every selected directory entry.  The arguments to the rou-
	time are:

	    Directory_entry.mr.r - The dirent structure describing the
		file.
	    File_start.ruw.v - The starting block number of the file.

	Any return value from the routine is ignored.  If Entry_action
	is  (int  (*)  ()) NULL, no routine is called for the selected
	directory entries.
    Segment_action.ra.v  - The address of an action routine to be cal-
	led at the end of the scan of every  directory  segment.   The
	arguments to the routine are:

	    Directory_segment.mr.r - The dirseg structure holding  the
		directory segment.
	    Segment_block.ruw.v - The starting  block  number  of  the
		directory segment.

	Any return value from the routine  is  ignored.   If  Segment_
	action  is  (int  (*)  ())  NULL, no routine is called for the
	directory segments.

IMPLICIT INPUTS:

    homblk - The home block of the RT-11 volume.   Used  to  determine
	the first directory segment.

IMPLICIT OUTPUTS:

    dirseg - A directory segment buffer.  Used  to  hold  the  current
	directory segment.

ROUTINE VALUE:

    None.

SIDE EFFECTS:

    The current position in the RT-11 volume is modified.

    May exit with an error message.

*********************************************************************/

{
    struct dirent
	* de_ptr;       /*  Current directory entry.                */

    int
	segnum;         /*  Block number of directory segment.      */

    uword
	filpos;         /*  File start block.                       */

    /*
     *  Check if anything is to be done.
     */

    if (((int (*) ()) NULL == entrtn) &&
	((int (*) ()) NULL == segrtn))
    {
	/*
	 *  No action.
	 */

	return;
    }

    /*
     *  Scan through all directory segments.
     */

    for (segnum = rt1seg ();
	 segnum != 0;
	 segnum = dirseg.ds_next)
    {
	/*
	 *  Get the directory segment.
	 */

	rtgets (segnum, & dirseg);

	if ((int (*) ()) NULL != entrtn)
	{
	    /*
	     *  Scan through the directory segment.
	     */

	    filpos = dirseg.ds_sblk;

	    de_ptr = (struct dirent *) (& dirseg.ds_dent [0]);
	    while (0 == (de_ptr -> de_stat & DE_ENDS))
	    {
		/*
		 *  See whether matches condition.
		 */

		if (de_ptr -> de_stat & status)
		{
		    /*
		     *  Yes.  Call user routine.
		     */

		    (* entrtn) (de_ptr, filpos);
		}

		/*
		 *  Update file position.
		 */

		filpos += de_ptr -> de_nblk;

		/*
		 *  Update the directory entry pointer.
		 */

		rtidep (& dirseg, & de_ptr);
	    }
	}

	if ((int (*) ()) NULL != segrtn)
	{
	    (* segrtn) (& dirseg, segnum);
	}
    }

    /*
     *  That's it.
     */
}
