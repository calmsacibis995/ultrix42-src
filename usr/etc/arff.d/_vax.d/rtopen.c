#ifndef lint
static  char    *sccsid = "@(#)rtopen.c	4.1  (ULTRIX)        7/2/90";
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

    Opens or closes a file as an RT-11 device image.

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
#include <errno.h>

/*
 * TABLE OF CONTENTS:
 */

/*
 *  rtopen - Open the RT-11 volume.
 *  rtclos - Close the RT-11 volume.
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
/*  perror (),              Print error message.                    */
    rtgetb ();          /*  Get block from RT-11 volume.            */

extern int
    close (),           /*  Close native file.                      */
    creat (),           /*  Create native file.                     */
    open (),            /*  Open native file.                       */
    unlink ();          /*  Delete native file.                     */

extern char
    * pgmnam,           /*  Program's name.                         */
    * rtdevn;           /*  RT-11 volume native name.               */

extern int
    errno,              /*  System call error number.               */
    modflg,             /*  Action modifier flags.                  */
    rtdev;              /*  RT-11 volume file number.               */

extern struct homblk
    homblk;             /*  RT-11 volume home block.                */

rtopen (access)

int
    access;

/*********************************************************************

FUNCTIONAL DESCRIPTION:

    Opens a file as an RT-11 device image.

FORMAL PARAMETERS:

    Access_type.rg.v - The access type desired.  One of ACC_READ, ACC_
	WRIT, ACC_CREA for read, write, or create access.

IMPLICIT INPUTS:

    rtdevn - The name of the RT-11 volume.

IMPLICIT OUTPUTS:

    rtdev - The file descriptor for the RT-11 device image.
    homblk - The home block for the RT-11 device image.

ROUTINE VALUE:

    None.

SIDE EFFECTS:

    May exit with an error message.

*********************************************************************/

{
    /*
     *  Open the RT-11 device.
     */

    switch (access)
    {
    case ACC_READ:

	/*
	 *  Open for read access.
	 */

	rtdev = open (rtdevn, 0);
	break;

    case ACC_WRIT:

	/*
	 *  Open for write access.
	 */

	rtdev = open (rtdevn, 2);
	break;

    case ACC_CREA:

	/*
	 *  Attempt to open for write access.  If the  file  does  not
	 *  exist, create it and open it for write access.
	 */

	rtdev = open (rtdevn, 2);
	if ((-1 == rtdev) && (ENOENT == errno))
	{
	    /*
	     *  It does not exist.  Attempt to create it.
	     */

	    rtdev = creat (rtdevn, 0644);
	    if (-1 != rtdev)
	    {
		/*
		 *  Create worked.  Close and open read/write.
		 */

		close (rtdev);
		rtdev = open (rtdevn, 2);
		if (-1 == rtdev)
		{
		    /*
		     *  Create worked, but open did not.  Delete it.
		     */

		    unlink (rtdevn);
		}
	    }
	}
	break;
    }

    if (-1 == rtdev)
    {
	fprintf (stderr, "%s: ", pgmnam);
	perror (rtdevn);
	exit (1);
    }

    rtgetb ((uword) HOMBLK, (char *) (& homblk));

    /*
     *  Check validity of home block.
     */

    if ((homblk.hb_clus != 1) ||
	(homblk.hb_dirs < DIRSEG1) ||
	(homblk.hb_dirs > (DIRSEG1 + 60)) ||
	(modflg & FLG_HOMB))
    {
	extern int bldboot;
	if ((! (modflg & FLG_HOMB)) && (access != ACC_CREA) && !bldboot)
	{
	    if (modflg & FLG_VERB) {
	    fprintf
	       (stderr,
		"%s: VOLUME HOME BLOCK IS CORRUPT\n",
		pgmnam);
	    fprintf
	       (stderr,
		"%s: Standard directory parameters assumed\n",
		pgmnam);
	   }
	}
	homblk.hb_dirs = DIRSEG1;
    }

    if (homblk.hb_dirs != DIRSEG1)
    {
	if (modflg & FLG_VERB)
	fprintf
	   (stderr,
	    "%s: Home block is curious but not necessarily corrupt\n",
	    pgmnam);
    }
}

rtclos ()

/*********************************************************************

FUNCTIONAL DESCRIPTION:

    Closes the file associated with the RT-11 device image.

FORMAL PARAMETERS:

    None.

IMPLICIT INPUTS:

    rtdev - The file descriptor for the RT-11 device image.
    rtdevn - The file name for the RT-11 device image.

IMPLICIT OUTPUTS:

    rtdev - The file descriptor for the RT-11 device image.

ROUTINE VALUE:

    None.

SIDE EFFECTS:

    May exit with an error message.

*********************************************************************/

{
    if (rtdev < 0)
    {
	fprintf (stderr, "%s: Invalid call to rtclose()\n", pgmnam);
	exit (1);
    }
    if (-1 == close (rtdev))
    {
	fprintf (stderr, "%s: ", pgmnam);
	perror (rtdevn);
	exit (1);
    }

    rtdev = -1;
}
