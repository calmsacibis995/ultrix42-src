#ifndef lint
static  char    *sccsid = "@(#)rtrepl.c	4.1  (ULTRIX)        7/2/90";
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
/************************************************************************
 *			Modification History				*
 *									*
 *	David L Ballenger, 22-Apr-1985					*
 * 0001	Change tmpnam variable to tmpname to avoid conflicts with 	*
 *	tmpnam routine declared in <stdio.h>.				*
 *									*
 ************************************************************************/

/*
FACILITY:

    RT-11 volume manipulation.

ABSTRACT:

    Replaces a file on an RT-11 device.

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
 *  delent - Delete RT-11 files with specified names.
 *  freent - Look for free space entry.
 *  freseg - Look for directory segment with free space entry.
 *  rtrepl - Replace files on RT-11 volume.
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
    paknam [11],        /*  ASCII RT-11 file name.                  */
    tmptmp [] = "/tmp/arffXXXXXX",
			/*  Template for temporary name.            */
    tmpname [sizeof tmptmp];
			/*  Temporary name.                         */

static int
    frenum,             /*  Number of directory segment.            */
    segful;             /*  Directory segment full flag.            */

static rad50
    r50blk [3];         /*  RADIX 50 RT-11 file name.               */

static uword
    wrksiz;             /*  Number of free blocks required.         */

/*
 * EXTERNAL REFERENCES:
 */

extern
    arwcmt (),          /*  Notify of "wild card" match.            */
    exit (),            /*  Terminate program.                      */
    fprintf (),         /*  Print message to file.                  */
    mktemp (),          /*  Make temporary file name.               */
    rtcons (),          /*  Consolidate free space in directory     */
			/*   segment.                               */
    rtctrd (),          /*  Copy file to RT-11 device.              */
    rtddep (),          /*  Decrement directory entry pointer.      */
    rtgets (),          /*  Read directory segment.                 */
    rtidep (),          /*  Increment directory entry pointer.      */
    rtmkfn (),          /*  Make RT-11 file name from native name.  */
    rtmvde (),          /*  Move directory entries.                 */
    rtputs (),          /*  Write directory segment.                */
    rtscnd (),          /*  Scan RT-11 device directory.            */
    rtspds (),          /*  Split RT-11 directory segment.          */
    strcpy (),          /*  Copy string.                            */
    unlink ();          /*  Delete native file.                     */

extern int
    arvfyn ();          /*  Verify directory initialization.        */

extern word
    rtdate ();          /*  Get current date in RT-11 format.       */

extern uword
    rtmkfi ();          /*  Make RT-11 file image.                  */

extern char
    * pgmnam;           /*  Program's name.                         */

extern int
    modflg;             /*  Action modifier flags.                  */

extern struct dirseg
    dirseg;             /*  RT-11 directory segment buffer.         */

static delent (de_ptr, filpos)

struct dirent
    * de_ptr;

uword
    filpos;

/*********************************************************************

FUNCTIONAL DESCRIPTION:

    Deletes RT-11 files matching a specified file name.

FORMAL PARAMETERS:

    Directory_entry.mr.r - An RT-11 directory entry.
    File_position.ruw.v - The starting block number of the file on the
	RT-11 device.  [Unused by this routine.]

IMPLICIT INPUTS:

    r50blk - The RADIX 50 RT-11 file name block for  the  file  to  be
	deleted.

IMPLICIT OUTPUTS:

    None.

ROUTINE VALUE:

    None.

SIDE EFFECTS:

    May exit with an error message.

*********************************************************************/

{
#ifdef DEBUG
    fprintf (stderr, "rtrepl\\delent: entry\n");
#endif

    /*
     *  Check if is permanent file with correct name.
     */

    if ((de_ptr -> de_stat & DE_PERM) &&
	(de_ptr -> de_name [0] == r50blk [0]) &&
	(de_ptr -> de_name [1] == r50blk [1]) &&
	(de_ptr -> de_name [2] == r50blk [2]))
    {
	/*
	 *  Abort if file is protected.
	 */

	if (de_ptr -> de_stat & DE_PROT)
	{
	    fprintf (stderr, "%s: Protected file: %s\n", pgmnam, paknam);
	    unlink (tmpname);
	    exit (1);
	}

	/*
	 *  Delete it.
	 */

	de_ptr -> de_stat = DE_EMPT;
#ifdef DEBUG
	fprintf (stderr, "rtrepl\\delent: deleting %s\n", paknam);
#endif
    }
}

static freent (de_ptr, filpos)

struct dirent
    * de_ptr;

uword
    filpos;

/*********************************************************************

FUNCTIONAL DESCRIPTION:

    Examine the RT-11 device directory for a free area  sufficient  to
    store a file image.

FORMAL PARAMETERS:

    Directory_entry.rr.r - A directory entry from the RT-11 directory.
    File_position.ruw.v - The starting position in the RT-11 device of
	the space  described  by  Directory_entry.   [Unused  in  this
	routine.]

IMPLICIT INPUTS:

    wrksiz - The number of blocks required.

IMPLICIT OUTPUTS:

    frenum - The directory segment number of the directory segment  in
	which  a  suitably  sized free area exists.  Set to -1 by this
	routine as a flag to the freseg() routine;  set to the segment
	number by the freseg() routine.

ROUTINE VALUE:

    None.

SIDE EFFECTS:

    None.

*********************************************************************/

{
#ifdef DEBUG
    fprintf (stderr, "rtrepl\\freent: looking at %06o\n", de_ptr);
#endif

    if ((de_ptr -> de_stat & DE_EMPT) &&
	(de_ptr -> de_nblk >= wrksiz) &&
	(0 == frenum))
    {
	frenum = -1;
#ifdef DEBUG
	fprintf (stderr, "rtrepl\\freent: free space found\n");
#endif
    }
}

static freseg (ds_ptr, segnum)

struct dirseg
    * ds_ptr;

int
    segnum;

/*********************************************************************

FUNCTIONAL DESCRIPTION:

    Notes the segment number of a directory  segment  describing  suf-
    ficient  free  space  to  store a file, and also notes whether the
    segment is full.

FORMAL PARAMETERS:

    Directory_segment.rr.r - The directory segment.
    Segment_number.ruw.v - The directory segment number.

IMPLICIT INPUTS:

    frenum - The directory segment number of the directory segment  in
	which  a  suitably  sized  free area exists.  Set to -1 by the
	freent() routine;  set to the segment number by this routine.

IMPLICIT OUTPUTS:

    frenum - The directory segment number of the directory segment  in
	which  a  suitably  sized  free area exists.  Set to -1 by the
	freent() routine;  set to the segment number by this routine.
    segful - A flag indicating whether the directory segment is full.

ROUTINE VALUE:

    None.

SIDE EFFECTS:

    None.

*********************************************************************/

{
    struct dirent
	* de_ptr;

#ifdef DEBUG
    fprintf (stderr, "rtrepl\\freseg: looking at %06o\n", ds_ptr);
#endif

    /*
     *  Check whether this  segment  has  a  sufficiently  sized  free
     *  space entry.
     */

    if (-1 == frenum)
    {
	/*
	 *  It did.  Set the segment number.
	 */

	frenum = segnum;
#ifdef DEBUG
	fprintf (stderr, "rtextr\\freseg: segment for free entry found\n");
#endif

	/*
	 *  Check if the segment is full.  If  the  directory  segment
	 *  has  room  for another directory entry, the segment is not
	 *  full.
	 */

	de_ptr = (struct dirent *) (& (ds_ptr -> ds_dent [0]));
	while (! (de_ptr -> de_stat & DE_ENDS))
	{
	    rtidep (ds_ptr, & de_ptr);
	}
	rtidep (ds_ptr, & de_ptr);

	if (de_ptr > (struct dirent *) (& (ds_ptr -> ds_dent [506])))
	{
	    segful = 1;
	}
	else
	{
	    segful = 0;
	}
#ifdef DEBUG
	fprintf (stderr, "Directory segment is %sfull\n",
	    (segful ? "" : "not "));
#endif
    }
}

rtrepl (argc, argv)

int
    argc;

char
    * argv [];

/*********************************************************************

FUNCTIONAL DESCRIPTION:

    Replaces a set of file on the RT-11 device, or inserts  a  set  of
    files into an RT-11 device.

FORMAL PARAMETERS:

    File_name_count.rg.v - The number of file names.
    File_names.rt.ra - A vector of pointers to  ASCIZ  strings,  where
	each ASCIZ string is the name of a native file to be stored on
	the RT-11 volume.  The name of the corresponding RT-11 file is
	generated from this name.

IMPLICIT INPUTS:

    segful - An indication as to whether the directory segment  having
	a sufficiently sized free area is full.

IMPLICIT OUTPUTS:

    r50blk - The RADIX 50 RT-11 file name corresponding  to  a  native
	file name.
    freseg - The segment number of the directory segment having a suf-
	ficiently sized free area.
    dirseg - The directory segment work buffer.

ROUTINE VALUE:

    None.

SIDE EFFECTS:

    May exit with an error message.

*********************************************************************/

{
    int
	i;              /*  File name counter.                      */

    uword
	filpos;         /*  RT-11 file starting block number.       */

    struct dirent
	* deptr1,       /*  Directory                               */
	* deptr2,       /*   entry                                  */
	* deptr3;       /*    pointers.                             */

    /*
     *  Will need to write RT-11 volume.
     */

    rtopen (ACC_WRIT);

    /*
     *  If necessary, initialize directory.
     */

    if ((modflg & FLG_NEWD) && arvfyn("Really initialize directory? "))
    {
	/*
	 *  Get first directory segment.
	 */

	 rtgets (1, & dirseg);

	 /*
	  *  Initialize it.
	  */

	dirseg.ds_nseg = 1;             /*  One directory segment.  */
	dirseg.ds_next = 0;             /*  No next segment.        */
	dirseg.ds_hseg = 1;             /*  High segment is 1.      */
	dirseg.ds_xtra = 0;             /*  No extra words.         */
	dirseg.ds_sblk = DIRSEG1 + 2;   /*  One directory segment.  */

	/*
	 *  The magic size of the free space constructed by  the  fol-
	 *  lowing  is 494 free blocks, what the 4.2bsd/ULTRIX-32 arff
	 *  does.  This is probably the result of  some  undergraduate
	 *  student reading "the console RX01 can store 494 blocks."
	 *
	 *  It can;  but volume overhead accounts for 6 blocks, and  a
	 *  directory segment for 2 more, so the free space on an RX01
	 *  is 486 blocks.  The wrong  number  is  repeated  here  for
	 *  "compatibility."
	 */

	deptr1 = (struct dirent *) (& dirseg.ds_dent [0]);
	deptr1 -> de_stat = DE_EMPT;
	deptr1 -> de_nblk = 494;

	rtidep (& dirseg, & deptr1);
	deptr1 -> de_stat = DE_ENDS;

	/*
	 *  Rewrite it.
	 */

	rtputs (1, & dirseg);
    }

    /*
     *  Construct temporary file name.
     */

    strcpy (tmpname, tmptmp);
    mktemp (tmpname);

    /*
     *  Do each file in turn.
     */

    for (i = 0; i < argc; i ++)
    {
	/*
	 *  Construct RT-11 file name.
	 */

	rtmkfn (argv [i], r50blk, paknam);
	arwcmt (i);

	/*
	 *  Make RT-11 file image, retain size.
	 */

	wrksiz = rtmkfi (argv [i], tmpname);

	/*
	 *  Delete named file on RT-11 device.
	 */

#ifdef DEBUG
	fprintf (stderr, "rtrepl: scanning for deletion pass\n");
#endif
	rtscnd ((word) DE_PERM, delent, rtcons);
#ifdef DEBUG
	fprintf (stderr, "rtrepl: end of scan for deletion pass\n");
#endif

	/*
	 *  Find appropriately sized free space.
	 */

scan_free:

	frenum = 0;
#ifdef DEBUG
	fprintf (stderr, "rtrepl: scanning for free space\n");
#endif
	rtscnd ((word) -1, freent, freseg);
#ifdef DEBUG
	fprintf (stderr, "rerepl: free space scan complete\n");
#endif
	if (0 == frenum)
	{
	    fprintf
	       (stderr,
		"%s: Insufficient free space on device\n",
		pgmnam);
	    unlink (tmpname);
	    exit (1);
	}

	/*
	 *  If directory segment is full, split it and try again.
	 */

	if (segful)
	{
	    rtspds (frenum, tmpname);
	    goto scan_free;
	}

	/*
	 *  Find directory entry under which to store.
	 */

	rtgets (frenum, & dirseg);
	filpos = dirseg.ds_sblk;
	deptr1 = (struct dirent *) (& dirseg.ds_dent [0]);
	while (0 == (deptr1 -> de_stat & DE_ENDS))
	{
	    if ((deptr1 -> de_stat & DE_EMPT) &&
		(deptr1 -> de_nblk >= wrksiz))
	    {
		break;
	    }
	    filpos += deptr1 -> de_nblk;
	    rtidep (& dirseg, & deptr1);
	}
	if (! (deptr1 -> de_stat & DE_EMPT))
	{
	    fprintf (stderr, "%s: Logic error, D.E. allocation\n", pgmnam);
	    unlink (tmpname);
	    exit (1);
	}

	/*
	 *  Move directory entries down.
	 */

#ifdef DEBUG
	fprintf (stderr, "rtrepl: moving entries down\n");
#endif

	deptr2 = deptr1;
	while (0 == (deptr2 -> de_stat & DE_ENDS))
	{
	    rtidep (& dirseg, & deptr2);
	}

	deptr3 = deptr2;
	rtidep (& dirseg, & deptr3);
	while (deptr2 >= deptr1)
	{
#ifdef DEBUG
	    fprintf (stderr, "rtrepl: moving entry from %06o to %06o\n",
		deptr2, deptr3);
#endif
	    rtmvde (deptr3, deptr2, & dirseg);
	    deptr3 = deptr2;
	    rtddep (& dirseg, & deptr2);
	}
#ifdef DEBUG
	fprintf (stderr, "rtrepl: entries moved down\n");
#endif

	/*
	 *  Copy file into RT-11 device.
	 */

#ifdef DEBUG
	fprintf (stderr, "rtrepl: calling rtctrd\n");
#endif

	rtctrd (tmpname, filpos, wrksiz);
	unlink (tmpname);

	/*
	 *  Adjust directory entries.
	 */

#ifdef DEBUG
	fprintf (stderr, "rtrepl: adjusting directory entries\n");
#endif

	deptr1 -> de_stat = DE_PERM;
	deptr1 -> de_name [0] = r50blk [0];
	deptr1 -> de_name [1] = r50blk [1];
	deptr1 -> de_name [2] = r50blk [2];
	deptr1 -> de_nblk = wrksiz;
	deptr1 -> de_date = rtdate ();

	rtidep (& dirseg, & deptr1);

	deptr1 -> de_nblk -= wrksiz;

	/*
	 *  Replace directory block.
	 */

#ifdef DEBUG
	fprintf (stderr, "rtrepl: replacing directory block\n");
#endif

	rtputs (frenum, & dirseg);

	/*
	 *  Notify if verbose.
	 */

	if (modflg & FLG_VERB)
	{
	    printf ("r %s, %u blocks\n", paknam, wrksiz);
	}
    }
}
