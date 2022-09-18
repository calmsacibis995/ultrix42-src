#ifndef lint
static  char    *sccsid = "@(#)rtctrd.c	4.1  (ULTRIX)        7/2/90";
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

    Copies an RT-11 file image to an RT-11 device.

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
    fclose (),          /*  Close native file.                      */
/*  perror (),              Print error message.                    */
    rtputb (),          /*  Write block onto RT-11 device.          */
    unlink ();          /*  Delete file.                            */

extern FILE
    * fopen ();         /*  Open native file.                       */

/*
extern int
    fread ();               Get "record" from file.                 
*/

extern char
    * pgmnam,           /*  Program name.                           */
    rtbloc [BLKSIZ];    /*  RT-11 data block buffer.                */

rtctrd (natnam, filpos, numblk)

char
    * natnam;

register uword
    filpos,
    numblk;

/*********************************************************************

FUNCTIONAL DESCRIPTION:

    Copies a native file into a specified region of the RT-11 device.

    The native file MUST be an exact multiple of BLKSIZ long.

FORMAL PARAMETERS:

    Native_file_name.rt.r - The name of the file to copy.
    File_position.ruw.v - The starting block number of the  region  of
	the RT-11 device.
    Number_blocks.ruw.v - The number of blocks in the native file  and
	in the region of the RT-11 device.

IMPLICIT INPUTS:

    None.

IMPLICIT OUTPUTS:

    rtbloc - The RT-11 file block buffer.

ROUTINE VALUE:

    None.

SIDE EFFECTS:

    May exit with an error message.

*********************************************************************/

{
    FILE
	* infile;

    /*
     *  Open the temporary file.
     */

    if ((FILE *) NULL == (infile = fopen (natnam, "r")))
    {
	fprintf (stderr, "%s: ", pgmnam);
	perror (natnam);
	unlink (natnam);
	exit (1);
    }

    /*
     *  Copy the file.
     */

    while (numblk > 0)
    {
	if (0 == fread (rtbloc, sizeof (char), BLKSIZ, infile))
	{
	    fprintf (stderr, "%s: ", pgmnam);
	    perror (natnam);
	    unlink (natnam);
	    exit (1);
	}
#ifdef DEBUG
	fprintf (stderr, "rtctrd: Writing block number %d\n", filpos);
#endif

	rtputb (filpos ++, rtbloc);
	numblk --;
    }

    fclose (infile);
}
