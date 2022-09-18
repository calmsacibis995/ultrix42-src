#ifndef lint
static  char    *sccsid = "@(#)rtprot.c	4.1  (ULTRIX)        7/2/90";
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

    Protect and unprotect files on RT-11 devices.

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
 *  entrtn - Protect or unprotect a single selected file.
 *  segrtn - Protect  or  unprotect  selected  files  in  a  directory
 *      segment.
 *  rtprot - Protect selected files.
 *  rtunpr - Unprotect selected files.
 */

/*
 * MACROS:
 */

/*
 * EQUATED SYMBOLS:
 */

/*
 *  Codes for 'action' flag:
 */

#define PROTECT         1       /*  Protect files.                  */
#define UNPROTECT       2       /*  Unprotect files.                */

/*
 * OWN STORAGE:
 */

static char
    * * xfilv;

static int
    action,
    xfilc;

/*
 * EXTERNAL REFERENCES:
 */

extern
    arr50a (),          /*  Convert RADIX 50 to ASCII.              */
    arwcmt (),          /*  Notify of wild card match.              */
    printf (),          /*  Put message to standard output.         */
    rtpakn (),          /*  Pack 9-character ASCII name.            */
    rtputs (),          /*  Put directory segment.                  */
    rtscnd ();          /*  Scan RT-11 device directory.            */

extern int
    rtwmat ();          /*  Match wild card patterns.               */

extern int
    modflg;             /*  Action modifier flags.                  */

static entrtn (de_ptr, filpos)

struct dirent
    * de_ptr;

uword
    filpos;

/*********************************************************************

FUNCTIONAL DESCRIPTION:

    Protects or unprotects a file if it matches one  or  more  of  the
    user-supplied file/wild card specifications.  (The actual updating
    of the directory segment takes place in the segrtn routine.)

FORMAL PARAMETERS:

    Directory_entry.mr.r - The directory entry for a file.
    File_position.rg.v - The position of the file in the RT-11  device
	image.   Not  needed by this routine, but part of the rtscnd()
	callback protocol.

IMPLICIT INPUTS:

    action - A flag indicating whether protection or  unprotection  is
	desired.
    xargc - The number of wild card specifications.
    xargv - The wild card specifications.

IMPLICIT OUTPUTS:

    None.

ROUTINE VALUE:

    None.

SIDE EFFECTS:

    None.

*********************************************************************/

{
    char
	upknam [9],     /*  Unpacked RT-11 file name.               */
	ascnam [11];    /*  Packed RT-11 file name.                 */

    int
	i,              /*  General counter.                        */
	match;          /*  Wild card match result.                 */

    /*
     *  Get the file name.
     */

    arr50a (de_ptr -> de_name, upknam, 9);
    rtpakn (upknam, ascnam);

#ifdef DEBUG
    fprintf (stderr, "Looking at entry for %s\n", ascnam);
#endif
    /*
     *  Check the wild cards.
     */

    if (0 == xfilc)
    {
	match = 1;
    }
    else
    {
	match = 0;
	for (i = 0; i < xfilc; i ++)
	{
	    if (rtwmat (xfilv [i], ascnam))
	    {
		match = 1;
		arwcmt (i);
	    }
	}
    }

    if (match)
    {
	if (PROTECT == action)
	{
	    de_ptr -> de_stat |= DE_PROT;
	    if (modflg & FLG_VERB)
	    {
		printf ("p %s\n", ascnam);
	    }
	}
	else
	{
	    de_ptr -> de_stat &= (~ DE_PROT);
	    if (modflg & FLG_VERB)
	    {
		printf ("u %s\n", ascnam);
	    }
	}
    }

    /*
     *  That's it.
     */
}

static segrtn (ds_ptr, segnum)

struct dirseg
    * ds_ptr;

int
    segnum;

/*********************************************************************

FUNCTIONAL DESCRIPTION:

    Writes a directory segment back to the RT-11 device image.

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

rtprot (filc, filv)

int
    filc;

char
    * * filv;

/*********************************************************************

FUNCTIONAL DESCRIPTION:

    Protects the files on an RT-11 device image specified by  a  user-
    supplied set of file/wild card specifications.

FORMAL PARAMETERS:

    Wild_card_count.rg.v - The number of wild card specifications.
    Wild_card.rt.ra - A set of ASCIZ strings that are  the  wild  card
	specifications.

IMPLICIT INPUTS:

    None.

IMPLICIT OUTPUTS:

    action - The action to be performed on files (PROTECT).
    xfilc - The wild card specification count.
    xfilv - The wild card specifications.

ROUTINE VALUE:

    None.

SIDE EFFECTS:

    The current position in the RT-11 volume is modified.

    The RT-11 volume is modified.

    Error messages may be produced.

    May exit with an error message.

*********************************************************************/

{
    /*
     *  Will need to write RT-11 volume.
     */

    rtopen (ACC_WRIT);

    /*
     *  Protect the files.
     */

    action = PROTECT;
    xfilc = filc;
    xfilv = filv;

    rtscnd ((word) DE_PERM, entrtn, segrtn);

    /*
     *  That's it.
     */
}

rtunpr (filc, filv)

int
    filc;

char
    * * filv;

/*********************************************************************

FUNCTIONAL DESCRIPTION:

    Unprotects the files on an RT-11 device image specified by a user-
    supplied set of file/wild card specifications.

FORMAL PARAMETERS:

    Wild_card_count.rg.v - The number of wild card specifications.
    Wild_card.rt.ra - A set of ASCIZ strings that are  the  wild  card
	specifications.

IMPLICIT INPUTS:

    None.

IMPLICIT OUTPUTS:

    action - The action to be performed on files (UNPROTECT).
    xfilc - The wild card specification count.
    xfilv - The wild card specifications.

ROUTINE VALUE:

    None.

SIDE EFFECTS:

    The current position in the RT-11 volume is modified.

    The RT-11 volume is modified.

    Error messages may be produced.

    May exit with an error message.

*********************************************************************/

{
    /*
     *  Will need to write RT-11 volume.
     */

    rtopen (ACC_WRIT);

    /*
     *  Unprotect the files.
     */

    action = UNPROTECT;
    xfilc = filc;
    xfilv = filv;

    rtscnd ((word) DE_PERM, entrtn, segrtn);

    /*
     *  That's it.
     */
}
