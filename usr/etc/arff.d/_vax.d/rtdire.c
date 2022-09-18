#ifndef lint
static  char    *sccsid = "@(#)rtdire.c	4.1  (ULTRIX)        7/2/90";
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

    Prints directory of an RT-11 device.

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
#include <time.h>
#include <stdio.h>

/*
 * TABLE OF CONTENTS:
 */

/*
 *  procent - Print directory for single directory entry.
 *  rtdire - Print directory for RT-11 volume.
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
    * monnam [] =       /*  Printable month names:                  */
       {"???", "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul",
	"Aug", "Sep", "Oct", "Nov", "Dec", "???", "???", "???"},
    * * xfilv;          /*  Wild card patterns.                     */

static int
    maxlin,             /*  Maximum number of files per line.       */
    numrpt,             /*  Number of files on this line.           */
    xfilc;              /*  Number of wild cards specified.         */

static uword
    numfil,             /*  Number files reported upon.             */
    numblk,             /*  Number blocks in files reported upon.   */
    numfre;             /*  Number free blocks in device.           */

/*
 * EXTERNAL REFERENCES:
 */

extern
    arr50a (),          /*  Translate RADIX50 to ASCII.             */
    arwcmt (),          /*  Notify of wild card match.              */
    printf (),          /*  Print message.                          */
    rtopen (),          /*  Open RT-11 volume.                      */
    rtpakn (),          /*  Pack file name.                         */
    rtscnd ();          /*  Scan directory.                         */

extern char
    * strncpy ();       /*  Copy string with maximum length.        */

extern int
    rtwmat ();          /*  Wild card matching.                     */

extern long
    time ();            /*  Get current time.                       */

extern struct homblk
    homblk;             /*  Home block for RT-11 device.            */

extern int
    modflg;             /*  Action modifier flags.                  */

extern struct tm
    * localtime ();     /*  Get local time.                         */

static procent (de_ptr, filpos)

struct dirent
    * de_ptr;

uword
    filpos;

/*********************************************************************

FUNCTIONAL DESCRIPTION:

    Prints the directory entry for a single file or free space.

FORMAL PARAMETERS:

    Directory_entry.rr.r - The directory entry structure for the  file
	or free space to be reported on.
    Entry_start.ruw.v - The starting block number of the file or  free
	space.

IMPLICIT INPUTS:

    maxlin - The maximum number of entries to  be  reported  on  on  a
	single line.
    monnam - The array of printable month names.
    numblk - The number of blocks in files that have been reported on.
    numfil - The number of files that have been reported on.
    numfre - The number of free blocks that have been reported on.
    numrpt - The number of entries that have already been reported  on
	on the current line.
    xfilc - The number of wild card patterns against which a file name
	is to be tested.
    xfilv - The wild card patterns against which a file name is to  be
	tested.

IMPLICIT OUTPUTS:

    numblk - The number of blocks in files that have been reported on.
    numfil - The number of files that have been reported on.
    numfre - The number of free blocks that have been reported on.
    numrpt - The number of entries that have been reported on  on  the
	current line.

ROUTINE VALUE:

    None.

SIDE EFFECTS:

    None.

*********************************************************************/

{
    char
	ascnam [12],    /*  A.B format file name.                   */
	extnam [4],     /*  Bbb format file name extension.         */
	filnam [7],     /*  Abbbbb format file name.                */
	upknam [9];     /*  AbbbbbBbbb format file name.            */

    int
	i,              /*  Wild card pattern counter.              */
	match;          /*  Print file description flag.            */

    /*
     *  See whether is file or free.
     */

    if (de_ptr -> de_stat & DE_PERM)
    {
	/*
	 *  Is file.
	 *
	 *  Get ASCII file name.
	 */

	arr50a (de_ptr -> de_name, upknam, 9);

	/*
	 *  See if should print.
	 */

	if (xfilc > 0)
	{
	    /*
	     *  Check wild card status.
	     */

	    match = 0;
	    rtpakn (upknam, ascnam);
	    for (i = 0; i < xfilc; i ++)
	    {
		if (rtwmat (xfilv [i], ascnam))
		{
		    match = 1;
		    arwcmt (i);
		}
	    }
	}
	else
	{
	    /*
	     *  No wild cards, print everything.
	     */

	    match = 1;
	}

	/*
	 *  If should print, do so.
	 */

	if (match)
	{
	    numfil ++;
	    numblk += de_ptr -> de_nblk;
	    strncpy (filnam, upknam, 6);
	    filnam [6] = '\0';
	    strncpy (extnam, upknam + 6, 3);
	    extnam [3] = '\0';

	    printf ("%6s.%3s", filnam, extnam);

	    if (modflg & FLG_VERB)
	    {
		/*
		 *  Verbose.
		 */

		printf (" %5u%c ",
		    de_ptr -> de_nblk,
		    (de_ptr -> de_stat & DE_PROT ? 'P' : ' '));
		if (0 != de_ptr -> de_date)
		{
		    printf ("%02d-%3s-%02d",
			((de_ptr -> de_date) >> 5) & 0x1F,
			    monnam [((de_ptr -> de_date) >> 10) & 0x0F],
			    (de_ptr -> de_date & 0x1F) + 72);
		}
		else
		{
		    printf ("         ");
		}
		printf (" %5u",
		    filpos);
	    }
	}
    }
    else
    {
	/*
	 *  Is free.
	 */

	numfre += de_ptr -> de_nblk;

	/*
	 *  See if should print.
	 */

	match = (modflg & FLG_VERB);
	if (match)
	{
	    /*
	     *  Yes.
	     */

	    printf ("< UNUSED > %5u            %5u",
		de_ptr -> de_nblk,
		filpos);
	}
    }

    /*
     *  Do intercolumn spacing.
     */

    if (match)
    {
	numrpt ++;
	if (numrpt >= maxlin)
	{
	    printf ("\n");
	    numrpt = 0;
	}
	else
	{
	    if (modflg & FLG_VERB)
	    {
		printf ("      ");
	    }
	    else
	    {
		printf  ("    ");
	    }
	}
    }

    /*
     *  That's it.
     */
}

rtdire (filc, filv)

int
    filc;

char
    * * filv;

/*********************************************************************

FUNCTIONAL DESCRIPTION:

    Prints a directory from an RT-11 device.

    The output format is intended to be identical to  that  of  RT-11.
    If the "verbose" flag is set, the output is that of

	DIRECTORY/VOLUME/FULL/POSITION/COLUMNS:2

    if the "verbose" flag is not set, the output is that of

	DIRECTORY/BRIEF/COLUMNS:6

    RT-11  and  this  implementation  handle  wild cards slightly dif-
    ferently.  RT-11 does one DIRECTORY scan for  each  argument,  and
    prints  all  files  matching  the argument during each scan;  this
    implementation does one DIRECTORY scan, and prints all files  that
    match any argument during the scan.  Thus, the RT-11 command:

	DIRECTORY *.C,FOO.*

    would  list  FOO.C  twice,  once  with  the .C's and once with the
    FOO's;  this implementation lists it once.

FORMAL PARAMETERS:

    File_count.rg.v - A count of the wild card descriptors.
    File_specs.rt.ra - Wild card descriptors.  If supplied, only files
	matching one or more wild card descriptors are to be listed.

IMPLICIT INPUTS:

    homblk - The RT-11 device image home block.
    modflg - The modifier keyletters bit mask.

IMPLICIT OUTPUTS:

    None.

ROUTINE VALUE:

    None.

SIDE EFFECTS:

    Changes the current position in the RT-11 device image.

    May exit with an error message.

*********************************************************************/

{
    char
	volid [13];     /*  Volume ID and owner.                    */

    long
	tloc;           /*  Local time.                             */

    struct tm
	* ltime;        /*  Broken down local time.                 */

    /*
     *  Will need to read the RT-11 device.
     */

    rtopen (ACC_READ);

    /*
     *  Print header information.
     */

    time (& tloc);
    ltime = localtime (& tloc);
    printf (" %02d-%3s-%02d\n",
	ltime -> tm_mday, monnam [ltime -> tm_mon + 1],
	ltime -> tm_year);

    if (modflg & FLG_VERB)
    {
	strncpy (volid, homblk.hb_voli, 12);
	volid [12] = '\0';
	printf (" Volume ID: %12s\n", volid);
	strncpy (volid, homblk.hb_owne, 12);
	volid [12] = '\0';
	printf (" Owner    : %12s\n", volid);
	maxlin = 2;
    }
    else
    {
	maxlin = 6;
    }

    /*
     *  Scan  through  all  directory  segments,   print   appropriate
     *  entries.
     */

    numfil = 0;
    numblk = 0;
    numfre = 0;
    numrpt = 0;
    xfilc  = filc;
    xfilv  = filv;

    rtscnd ((word) -1, procent, (int (*) ()) NULL);

    /*
     *  Final report.
     */

    if (0 != numrpt)
    {
	printf ("\n");
    }
    printf (" %u Files, %u Blocks\n %u Free blocks\n",
	numfil, numblk, numfre);
}
