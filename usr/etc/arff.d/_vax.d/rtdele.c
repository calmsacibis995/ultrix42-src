#ifndef lint
static  char    *sccsid = "@(#)rtdele.c	4.1  (ULTRIX)        7/2/90";
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

    Deletes files from an RT-11 device.

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
 *  entrtn - Delete a single file.
 *  rtdele - Delete all files matching conditions.
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

static char
    * * xfilv;          /*  Local copy of wild cards.               */

static int
    xfilc;              /*  Local count of wild cards.              */

/*
 * EXTERNAL REFERENCES:
 */

extern
    exit (),            /*  Terminate program.                      */
    fprintf (),         /*  Print message to file.                  */
    printf (),          /*  Print message.                          */
    arwcmt (),          /*  Notify of match on wild card.           */
    rtcons (),          /*  Consolidate free space in directory     */
			/*   segment.                               */
    rtopen (),          /*  Open RT-11 volume.                      */
    rtpakn (),          /*  Pack RT-11 file name.                   */
    rtscnd ();          /*  Scan RT-11 directory.                   */

extern int
    rtwmat ();          /*  RT-11 wild card match.                  */

extern char
    * pgmnam;           /*  Program's name.                         */

extern int
    modflg;             /*  Action modifier flags.                  */

extern struct homblk
    homblk;             /*  Home block buffer.                      */

static entrtn (de_ptr, filpos)

struct dirent
    * de_ptr;

uword
    filpos;

/*********************************************************************

FUNCTIONAL DESCRIPTION:

    Marks a file as deleted if it matches one or more of the user-sup-
    plied  file/wild card specifications.  (The actual updating of the
    directory segment takes place in the rtcons routine.)

FORMAL PARAMETERS:

    Directory_entry.mr.r - The directory entry for a file.
    File_position.rg.v - The position of the file in the RT-11  device
	image.   Not  needed by this routine, but part of the rtscnd()
	callback protocol.

IMPLICIT INPUTS:

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
	/*
	 *  We may someday want no file/wildcard specs  to
	 *  mean  everything,  so here it is.  Just delete
	 *  the check above.
	 */

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
#ifdef DEBUG
	fprintf (stderr, "Deleting entry for %s\n", ascnam);
#endif
	if (de_ptr -> de_stat & DE_PROT)
	{
	    /*
	     *  Cannot delete protected files.
	     */

	    fprintf (stderr, "%s: file protected: %s\n", pgmnam, ascnam);
	}
	else
	{
	    /*
	     *  Mark the file as deleted.
	     */

	    de_ptr -> de_stat = DE_EMPT;
	    if (modflg & FLG_VERB)
	    {
		printf ("d %s\n", ascnam);
	    }
	}
    }

    /*
     *  That's it.
     */
}

rtdele (filc, filv)

int
    filc;

char
    * * filv;

/*********************************************************************

FUNCTIONAL DESCRIPTION:

    Scans a directory looking for entries that match a set of wildcard
    specificatons.  Deletes all unprotected entries that match one  or
    more  of the wildcard specifications.  Consolidates adjacent empty
    directory entries into one empty directory entry, and deletes  any
    zero-length empty directory entries.  Directory entries describing
    adjacent blocks of free space are NOT consolidated across  direct-
    ory segment boundaries, just like real RT-11.

    Note that the absence of a wild card specification is NOT  equiva-
    lent  to  a *.* wild card specifications, unlike the convention in
    the rest of arff.

FORMAL PARAMETERS:

    Wild_card_count.rg.v - The number of wild card specifications.
    Wild_card.rt.ra - A set of ASCIZ strings that are  the  wild  card
	specifications.

IMPLICIT INPUTS:

    modflg - The action modifier flags.

IMPLICIT OUTPUTS:

    None.

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
     *  Check that something was specified.
     */

    if (filc <= 0)
    {
	fprintf (stderr,
	    "%s: File/wild card specification must be given for delete\n",
	    pgmnam);
	exit (1);
    }

    /*
     *  Will need to write the RT-11 device.
     */

    rtopen (ACC_WRIT);

    /*
     *  Delete the files.
     */

    xfilc = filc;
    xfilv = filv;

    rtscnd ((word) DE_PERM, entrtn, rtcons);

    /*
     *  That's it.
     */
}
