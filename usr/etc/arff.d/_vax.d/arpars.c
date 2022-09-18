#ifndef lint
static  char    *sccsid = "@(#)arpars.c	4.1  (ULTRIX)        7/2/90";
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

    Parses command line for arff.

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
    rtdele (),          /*  Delete RT-11 file.                      */
    rtdire (),          /*  Directory of RT-11 device.              */
    rtextr (),          /*  Extract RT-11 files.                    */
    rtinit (),          /*  Initialize RT-11 device.                */
    rtprot (),          /*  Protect named files.                    */
    rtrepl (),          /*  Replace/add RT-11 files.                */
    rtunpr (),          /*  Unprotect named files.                  */
    rtboot ();          /*  Rewrite boot block                      */

extern char
    * pgmnam,           /*  Program name.                           */
    * rtdevn;           /*  RT-11 device image name.                */

extern int
    modflg;             /*  Action modifier flags.                  */

arpars (argc, argv, actrtn, filc, filv)

int
    argc;

char
    * * argv;

int
    (* (* actrtn)) ();

int
    * filc;

char
    * * * filv;

/*********************************************************************

FUNCTIONAL DESCRIPTION:

    Parses the parameters to arff.

    The command must look like:

	arff a[m] [arg...]

    where:

    arff    is the name of the command (usually "arff");
    a       is an action keyletter.  The available  action  keyletters
	    and their meanings are:

	    d   Delete files on the RT-11 device.
	    i   Initialize RT-11 device.
	    p   Protect files on the RT-11 device.
	    r   Replace files on the RT-11 device.
	    t   List files on the RT-11 device.
	    u   Unprotect files on the RT-11 device.
	    x   Extract files from the RT-11 device.
	    b   Update boot block (for 8600's rl02).

    m       is zero or more action modifier keyletters.  The available
	    action modifier keyletters are:

	    v   print descriptions of file actions as they  occur,  or
		give  complete rundown of RT-11 device image if action
		keyletter is t.
	    f   the next unused argument is the name of the file  con-
		taining the RT-11 device image.
	    m   do not use ULTRIX-32 /dev/console mapping.
	    c   create a new (1 segment) directory on the RT-11 device
		image.
	    p   printable (text) file transfer.   \r\n  in  the  RT-11
		device image is equivalent to \n in native files.
	    n   No extract - used only with 'b' before updating

	    The  action  modifier  keyletters can appear in any order,
	    but each action modifier keyletter can appear at most  one
	    time.

    arg     is a list of zero or more file specifications.  If  the  f
	    action  modifier keyletter is given, the first arg must be
	    the name of the file holding the RT-11 device image.   The
	    remaining args are arguments to the individual actions.

FORMAL PARAMETERS:

    Argument_count.rg.v - The number of  elements  of  Argument_vector
	that are valid.
    Argument_vector.rt.ra - The array of ASCIZ strings  that  are  the
	arguments.   Each  element  of Argument_vector is one token of
	the command.  The command format is given above.
    Action_routine.wa.r - The address of the  routine  which  performs
	the action specified by the action key letter.
    File_count.wg.r - The number of  RT-11  wild  card  specifications
	or  native file specifications given in the command, exclusive
	of any specification for the f action modifier keyletter.
    File_vector.wa.r - The address of a vector of  pointers  to  ASCIZ
	strings  containing  the  RT-11  wild  card  specifications or
	native file specifications.

IMPLICIT INPUTS:

    rtdevn - The name of the RT-11 device image.

IMPLICIT OUTPUTS:

    modflg - A bit mask showing which action modifier keyletters  were
	specified.
    rtdevn - The name of the RT-11 device image.

ROUTINE VALUE:

    None.

SIDE EFFECTS:

    May cause the program to exit with an error code, if a syntax  er-
    ror is discovered in the command line.

*********************************************************************/

{
    register char
	keychr,         /*  Current keyletter.                      */
	* keyptr;       /*  Pointer into key.                       */

    register int
	argi;           /*  Index of start of file names.           */

    /*
     *  Ensure there is a key.
     */

    if (argc < 2)
    {
	fprintf (stderr, "Usage: %s key[mod...] [file...]\n", pgmnam);
	exit (1);
    }

    /*
     *  Parse the action keyletter.
     */

    keyptr = argv [1];

    switch (keychr = * keyptr ++)
    {
    case 'd':

	/*
	 *  Action is "delete".
	 */

	* actrtn = rtdele;
	break;

    case 'b':

	/*  Do the 8600 boot block magic */

	*actrtn = rtboot;
	break;

    case 'i':

	/*
	 *  Action is "initialize".
	 */

	* actrtn = rtinit;
	break;

    case 'p':

	/*
	 *  Action is "protect."
	 */

	* actrtn = rtprot;
	break;

    case 'r':

	/*
	 *  Action is "replace".
	 */

	* actrtn = rtrepl;
	break;

    case 't':

	/*
	 *  Action is "list".
	 */

	* actrtn = rtdire;
	break;

    case 'u':

	/*
	 *  Action is "unprotect".
	 */

	* actrtn = rtunpr;
	break;

    case 'x':

	/*
	 *  Action is "extract".
	 */

	* actrtn = rtextr;
	break;

    default:

	/*
	 *  Action is UNKNOWN.
	 */

	fprintf (stderr, "%s: Action keyletter %c unknown\n", pgmnam, keychr);
	exit (1);
    }

    /*
     *  Parse the modifier keyletters.
     */

    argi = 2;
    modflg = FLG_EXTR;

    while ('\0' != (keychr = * keyptr ++))
    {
	switch (keychr)
	{
	case 'c':

	    /*
	     *  Modifier is "create directory".
	     */

	    if (modflg & FLG_NEWD)
	    {
		fprintf (stderr, "%s: Multiple c modifiers\n", pgmnam);
		exit (1);
	    }
	    modflg |= FLG_NEWD;
	    break;

	case 'n':

	    /* modifier is no-extract */

	    if (*actrtn != rtboot) {
		fprintf (stderr, "%s: Can be used only with b option\n", pgmnam);
		exit (1);
	    }
	    modflg &= ~FLG_EXTR;  /* turn off extract flag */
	    break;


	case 'f':

	    /*
	     *  Modifier is "file".
	     */

	    if ((char *) NULL != rtdevn)
	    {
		fprintf (stderr, "%s: f modifier given twice\n", pgmnam);
		exit (1);
	    }
	    if (argi >= argc)
	    {
		fprintf
		   (stderr,
		    "%s: f modifier requires file name\n",
		    pgmnam);
		exit (1);
	    }
	    rtdevn = argv [argi ++];
	    break;

	case 'h':

	    /*
	     *  Modifier is "home block known to be corrupt."
	     */

	    if (modflg & FLG_HOMB)
	    {
		fprintf (stderr, "%s: h modifier given twice\n", pgmnam);
		exit (1);
	    }
	    modflg |= FLG_HOMB;
	    break;

	case 'm':

	    /*
	     *  Modifier is "no mapping".
	     */

	    if (modflg & FLG_NOIN)
	    {
		fprintf (stderr, "%s: m modifier given twice\n", pgmnam);
		exit (1);
	    }
	    modflg |= FLG_NOIN;
	    break;

	case 'p':

	    /*
	     *  Modifier is "printable data".
	     */

	    if (modflg & FLG_PRIN)
	    {
		fprintf (stderr, "%s: p modifier given twice\n", pgmnam);
		exit (1);
	    }
	    modflg |= FLG_PRIN;
	    break;

	case 'v':

	    /*
	     *  Modifier is "verbose".
	     */

	    if (modflg & FLG_VERB)
	    {
		fprintf (stderr, "%s: v modifier given twice\n", pgmnam);
		exit (1);
	    }
	    modflg |= FLG_VERB;
	    break;

	default:

	    /*
	     *  Modifier is UNKNOWN.
	     */

	    fprintf (stderr, "%s: Modifier %c unknown\n", keychr, pgmnam);
	    exit (1);
	}
    }

    /*
     *  m modifier is default on non-ULTRIX-32/4.2BSD.
     */

#ifndef ULTRIX32
    modflg |= FLG_NOIN;
#endif

    /*
     *  Figure out file count, vector address.
     */

    * filc = argc - argi;
    * filv = & (argv [argi]);

    /*
     *  That's it.
     */
}
