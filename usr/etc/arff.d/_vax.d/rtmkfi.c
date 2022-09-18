#ifndef lint
static  char    *sccsid = "@(#)rtmkfi.c	4.1  (ULTRIX)        7/2/90";
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

    Makes an RT-11 file image of a native file.

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
    fclose (),          /*  Close file.                             */
/*  perror (),              Print error message.                    */
    unlink ();          /*  Delete native file.                     */

extern FILE
    * fopen ();         /*  Open file.                              */

extern char
    * pgmnam;           /*  Program's name.                         */

extern int
    modflg;             /*  Action modifier flags.                  */

uword rtmkfi (natnam, tmpnam)

char
    * natnam,
    * tmpnam;

/*********************************************************************

FUNCTIONAL DESCRIPTION:

    Makes an RT-11 file image of a specified native file.

FORMAL PARAMETERS:

    Native_name.rt.r - The name of the native file.
    Temporary_name.rt.r - The name of the RT-11 file image.

IMPLICIT INPUTS:

    modflg - The action modifier flags.

IMPLICIT OUTPUTS:

    None.

ROUTINE VALUE:

    The number of blocks in the RT-11 file image.

SIDE EFFECTS:

    May exit with an error message.

*********************************************************************/

{
    FILE
	* infile,       /*  Input FILE structure.                   */
	* outfile;      /*  Output FILE structure.                  */

    int
	inchar;         /*  Input character.                        */

    long
	numbyt;         /*  Number of bytes in output file.         */

#ifdef DEBUG
    fprintf (stderr, "rtmkfi: making RT-11 file image\n");
#endif

    /*
     *  Construct the temporary file.
     */

    if ((FILE *) NULL == (infile = fopen (natnam, "r")))
    {
	fprintf (stderr, "%s: ", pgmnam);
	perror (natnam);
	exit (1);
    }
    if ((FILE *) NULL == (outfile = fopen (tmpnam, "w")))
    {
	fprintf (stderr, "%s: ", pgmnam);
	perror (tmpnam);
	exit (1);
    }

    numbyt = 0;
    if (modflg & FLG_PRIN)
    {
	while (EOF != (inchar = getc (infile)))
	{
	    if ('\0' != inchar)
	    {
		if ('\n' == inchar)
		{
		    if (EOF == putc ('\r', outfile))
		    {
			fprintf (stderr, "%s: ", pgmnam);
			perror (tmpnam);
			unlink (tmpnam);
			exit (1);
		    }
		    numbyt ++;
		}
		/*
		 *  Use explicit ferror() test, as putc will turn 0377
		 *  into EOF on some systems.   (Namely  all  of  them
		 *  tested.)
		 */
		putc (inchar, outfile);
		if (ferror (outfile))
		{
		    fprintf (stderr, "%s: ", pgmnam);
		    perror (tmpnam);
		    unlink (tmpnam);
		    exit (1);
		}
		numbyt ++;
	    }
	}
    }
    else
    {
	while (EOF != (inchar = getc (infile)))
	{
	    /*
	     *  Use explicit ferror() test, as putc will turn 0377 in-
	     *  to EOF on some systems.  (Namely all of them tested.)
	     */
	    putc (inchar, outfile);
	    if (ferror (outfile))
	    {
		fprintf (stderr, "%s: ", pgmnam);
		perror (tmpnam);
		unlink (tmpnam);
		exit (1);
	    }
	    numbyt ++;
	}
    }

    while (0 != (numbyt % BLKSIZ))
    {
	if (EOF == putc ('\0', outfile))
	{
	    fprintf (stderr, "%s: ", pgmnam);
	    perror (tmpnam);
	    unlink (tmpnam);
	    exit (1);
	}
	numbyt ++;
    }
    fclose (outfile);

    /*
     *  Check for input errors.
     */

    if (ferror (infile))
    {
	fprintf (stderr, "%s: ", pgmnam);
	perror (natnam);
	unlink (tmpnam);
	exit (1);
    }

    fclose (infile);

    /*
     *  Compute and return number of blocks.
     */

#ifdef DEBUG
    fprintf (stderr, "rtmkfi: RT-11 file is %u blocks\n",
	(uword) (numbyt / BLKSIZ));
#endif

    return ((uword) (numbyt / BLKSIZ));
}
