#ifndef lint
static  char    *sccsid = "@(#)arff.c	4.1  (ULTRIX)        7/2/90";
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

    Main   program.   Parses  command  and  calls  appropriate  action
    routines.

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

#ifndef RTDEVNAM
#define RTDEVNAM        "/dev/rf0.m11"
#endif

/*
 * OWN STORAGE:
 */

char
    * pgmnam,           /*  Program name.                           */
    rtbloc [BLKSIZ],    /*  RT-11 file data block.                  */
    * rtdevn = (char *) NULL;
			/*  Device image name.                      */

int
    modflg,             /*  Action modifier flags.                  */
    rtdev = -1;         /*  Device image file descriptor.           */

struct homblk
    homblk;             /*  Device image home block.                */

struct dirseg
    dirseg;             /*  Directory segment buffer.               */

/*
 * EXTERNAL REFERENCES:
 */
extern
    arpars (),          /*  Parse command.                          */
    arwcin (),          /*  Initialize for wild card match history. */
    arwcrp (),          /*  Report on wild card match history.      */
    rtclos ();          /*  Close RT-11 volume.                     */

int main (argc, argv)

int
    argc;

char
    * * argv;

/*********************************************************************

FUNCTIONAL DESCRIPTION:

    Main routine for arff.

    Causes command to be parsed.  Causes the RT-11 device image to  be
    opened.   Causes  the  appropriate action to be taken.  Causes the
    RT-11 device image to be closed.

FORMAL PARAMETERS:

    Argument_count.rg.v - The number of elements of Argument_text that
	are valid.
    Argument_text.rt.ra - The command line arguments.

IMPLICIT INPUTS:

    None.

IMPLICIT OUTPUTS:

    None.

ROUTINE VALUE:

    0, success.  (Unsuccessful exits are taken throughout the  program
    via the exit() routine.)

SIDE EFFECTS:

    May manipulate the RT-11 device image or  currently  mounted  file
    systems.

*********************************************************************/

{
    int
	(* actrtn) (),
	filc;

    char
	* * filv;

    /*
     *  Set the program name global.
     */

    pgmnam = argv [0];

    /*
     *  Parse command line argument.
     */

    arpars (argc, argv, & actrtn, & filc, & filv);

#ifdef DEBUG
    fprintf (stderr, "%s: file list", pgmnam);
    for (argc = 0; argc < filc; argc ++)
    {
	fprintf (stderr, " \"%s\"", filv [argc]);
    }
    fprintf (stderr, "\n");
#endif

    arwcin (filc);

    /*
     *  Set the device name.
     */

    if ((char *) NULL == rtdevn)
    {
	rtdevn = RTDEVNAM;
    }

    /*
     *  Perform the appropriate action.
     */

    (* actrtn) (filc, filv);

    /*
     *  Close the RT-11 device.
     */

    rtclos ();

    /*
     *  Report on unmatched wild cards.
     */

    arwcrp (filc, filv);

    /*
     *  Success.
     */

    return (0);
}
