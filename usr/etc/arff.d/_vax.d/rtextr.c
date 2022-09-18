#ifndef lint
static  char    *sccsid = "@(#)rtextr.c	4.1  (ULTRIX)        7/2/90";
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

    Extracts files from RT-11 device.

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
 *  extract - Extract RT-11 file into native file.
 *  procent - Examine directory  entry  to  determine  whether  should
 *      extract.
 *  rtextr - Extract files.
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
    natnam [MAXNAMLN+1],/*  Native name of extracted file.          */
    * * xfilv;          /*  Wild card specifications.               */

static int
    xfilc;              /*  Number of wild cards specified.         */

/*
 * EXTERNAL REFERENCES:
 */

extern
    ardown (),          /*  Convert string to lower case.           */
    arr50a (),          /*  Convert RADIX 50 to ASCII.              */
    arwcmt (),          /*  Notify of wild card match.              */
    exit (),            /*  Terminate program.                      */
    fclose (),          /*  Close native file.                      */
    fprintf (),         /*  Print message to file.                  */
/*  perror (),              Print error message.                    */
    printf (),          /*  Print message.                          */
    rtgetb (),          /*  Get block from RT-11 device.            */
    rtopen (),          /*  Open RT-11 volume.                      */
    rtpakn (),          /*  Pack RT-11 file name.                   */
    rtscnd ();          /*  Scan RT-11 directory.                   */

extern FILE
    * fopen ();         /*  Open native file.                       */

extern int
    rtwmat ();          /*  Match file name against wild card.      */

extern char
    * pgmnam,           /*  Program's name.                         */
    rtbloc [BLKSIZ];    /*  RT-11 file data block buffer.           */

extern int
    modflg;             /*  Action modifier flags.                  */

static extract (wildspec, de_ptr, lcname, filpos)

char
    * wildspec,         /*  User's wild card specification.         */
    * lcname;           /*  Lower case version of RT-11 file name.  */

struct dirent
    * de_ptr;           /*  RT-11 directory entry.                  */

uword
    filpos;             /*  Start block of file on volume.          */

/*********************************************************************

FUNCTIONAL DESCRIPTION:

    Extracts an RT-11 file into the file whose  name  results  from  a
    specified user path/wildcard specification and an RT-11 file name.

FORMAL PARAMETERS:

    Wild_card_spec.rt.r - The user  specified  path/wildcard  specifi-
	cation.
    Directory_entry.rr.r - The RT-11 directory entry for the  file  to
	be extracted.
    Lower_case_file_name.rt.r - The lower case equivalent of the RT-11
	file name of the file.
    File_start.muw.v - The starting block number of the  file  in  the
	RT-11 device.

IMPLICIT INPUTS:

    modflg - The action modifier flags.

IMPLICIT OUTPUTS:

    rtbloc - The RT-11 file data block buffer.

ROUTINE VALUE:

    None.

SIDE EFFECTS:

    Creates a file.

    Changes the current position within the RT-11 device image.

    May exit with an error message.

*********************************************************************/

{
    char
	* natptr,       /*  Native file name pointer.               */
	* pstart,       /*  Native file name prefix start.          */
	rtchar,         /*  Byte from RT-11 block.                  */
	* rtptr,        /*  Pointer into RT-11 block.               */
	wchar;          /*  Wild card specification character.      */

    FILE
	* outfile;      /*  Native output file.                     */

    int
	i,              /*  File block counter.                     */
	j,              /*  Block byte counter.                     */
	pendcr,         /*  CR is pending.                          */
	pendnull,       /*  Number of NULs pending.                 */
	wildc;          /*  Wild card character encountered.        */

    long
	natsiz;         /*  Number of characters in native file.    */

    /*
     *  Generate the native file name.
     */

    if ((strlen (wildspec) + strlen (lcname)) > MAXNAMLN)
    {
	fprintf (stderr, "%s: File name too long\n", pgmnam);
	exit (1);
    }

    natptr = & natnam [0];
    pstart = wildspec;
    wildc  = 0;
    while ('\0' != (wchar = * wildspec ++))
    {
	switch (wchar)
	{
	case '%':
	case '*':
	    wildc = 1;
	    break;

	case '/':
	    while (pstart < wildspec)
	    {
		* natptr ++ = * pstart ++;
	    }
	    wildc = 0;
	    break;
	}
    }

    if (wildc || (pstart == wildspec) || (wchar == '\0'))
    {
	/*
	 *  Add on lowercase translation of RT-11 name.
	 */

	while (* natptr ++ = * lcname ++)
	;
    }
    else
    {
	/*
	 *  Use remainder of "wild card" specification.
	 */

	while (* natptr ++ = * pstart ++)
	;
    }

    /*
     *  Open the output file.
     */

    {
	char tmpnm[10];
	char *index(), *period;
	strcpy(&tmpnm[0], &natnam[0]);

	if((strlen(tmpnm) - 1) == ((period = index(tmpnm,'.')) - &tmpnm[0]))
	*period = '\0';

    if ((FILE *) NULL == (outfile = fopen (tmpnm, "w")))
    {
	fprintf (stderr, "%s: ", pgmnam);
	perror (natnam);
	exit (1);
    }
    }
    natsiz = 0;

    /*
     *  Copy the RT-11 file to the output file.
     */

    pendcr = 0;
    pendnull = 0;
    for (i = 0; i < de_ptr -> de_nblk; i ++)
    {
#ifdef DEBUG
	fprintf (stderr, "Getting block %d\n", i);
#endif
	rtgetb (filpos ++, rtbloc);
	for (j = BLKSIZ, rtptr = & rtbloc [0], rtchar = * rtptr ++;
	     j > 0;
	     j --, rtchar = * rtptr ++)
	{
	    if (modflg & FLG_PRIN)
	    {
		/*
		 *  Printable file transfer.  Delete NULL,  delete
		 *  CR before LF but not otherwise.
		 */

		if ('\0' == rtchar)
		{
		    continue;
		}

		if ('\n' != rtchar)
		{
		    if (pendcr)
		    {
			if (EOF == putc ('\r', outfile))
			{
			    fprintf (stderr, "%s: ", pgmnam);
			    perror (natnam);
			    exit (1);
			}
			natsiz ++;
		    }
		}
		pendcr = 0;

		if ('\r' == rtchar)
		{
		    pendcr = 1;
		}
		else
		{
		    /*
		     *  Use ferror() instead of putc() return, as putc
		     *  of 0377 yields EOF on all systems tested.
		     */

		    putc (rtchar, outfile);
		    if (ferror (outfile))
		    {
			fprintf (stderr, "%s: ", pgmnam);
			perror (natnam);
			exit (1);
		    }
		    natsiz ++;
		}
	    }
	    else
	    {
		/*
		 *  Binary file transfer.  Copy everything  except
		 *  trailing NUL characters.
		 */

		if ('\0' == rtchar)
		{
		    pendnull ++;
		    continue;
		}

		while (pendnull > 0)
		{
		    if (EOF == putc ('\0', outfile))
		    {
			fprintf (stderr, "%s: ", pgmnam);
			perror (natnam);
			exit (1);
		    }
		    pendnull --;
		    natsiz ++;
		}
		/*
		 *  Use ferror() instead of putc() return, as putc  of
		 *  0377 yields EOF on all systems tested.
		 */

		putc (rtchar, outfile);
		if (ferror (outfile))
		{
		    fprintf (stderr, "%s: ", pgmnam);
		    perror (natnam);
		    exit (1);
		}
		natsiz ++;
	    }
	}
    }
    if (pendcr)
    {
	if (EOF == putc ('\r', outfile))
	{
	    fprintf (stderr, "%s: ", pgmnam);
	    perror (natnam);
	    exit (1);
	}
	natsiz ++;
    }

    /*
     *  Close the output file.
     */

    if (EOF == fclose (outfile))
    {
	fprintf (stderr, "%s: ", pgmnam);
	perror (natnam);
	exit (1);
    }

    /*
     *  Notify user if appropriate.
     */

    if (modflg & FLG_VERB)
    {
	printf ("x %s, %ld characters\n", natnam, natsiz);
    }

    /*
     *  That's it.
     */
}

static procent (de_ptr, filpos)

struct dirent
    * de_ptr;           /*  Entry of permanent file.                */

uword
    filpos;             /*  Starting block for file.                */

/*********************************************************************

FUNCTIONAL DESCRIPTION:

    Extracts a file from an RT-11 device.

FORMAL PARAMETERS:

    Directory_entry.rr.r - The directory entry for a permanent file on
	an RT-11 device.
    File_start.ruw.v - The starting block number of the  file  on  the
	RT-11 device.

IMPLICIT INPUTS:

    xfilc - The number of wild card specifications.
    xfilv - The wild card specifications.

IMPLICIT OUTPUTS:

    None.

ROUTINE VALUE:

    None.

SIDE EFFECTS:

    May exit with an error message.

*********************************************************************/

{
    char
	ascnam [12],    /*  A.B format file name.                   */
	lcname [12],    /*  a.b format file name.                   */
	upknam [9];     /*  AbbbbbBbbb format file name.            */

    int
	i;              /*  Wild card pattern counter.              */

    /*
     *  Get ASCII file names.
     */

    arr50a (de_ptr -> de_name, upknam, 9);
    rtpakn (upknam, ascnam);
    ardown (lcname, ascnam);

    /*
     *  See if should extract.
     */

    if (xfilc > 0)
    {
	/*
	 *  Check wild card status.
	 */

	for (i = 0; i < xfilc; i ++)
	{
#ifdef DEBUG
	    fprintf (stderr, "rtextr\\procent: checking \"%s\"\n",
		xfilv [i]);
#endif
	    /*
	     *  Matches, extract it.
	     */

	    if (rtwmat (xfilv [i], ascnam))
	    {
		extract (xfilv [i], de_ptr, lcname, filpos);
		arwcmt (i);
	    }
	}
    }
    else
    {
	/*
	 *  No wild cards, extract everything.
	 */

	extract ("", de_ptr, lcname, filpos);
    }

    /*
     *  That's it.
     */
}

rtextr (filc, filv)

int
    filc;

char
    * * filv;

/*********************************************************************

FUNCTIONAL DESCRIPTION:

    Extracts files from an RT-11 device.

    There are two data transfer modes.  In "printable" mode, NUL char-
    acters are deleted entirely, and CR is deleted when it occurs  be-
    fore LF.  In "binary" mode, trailing NULs in the file are deleted.
    The  first mode is useful when printable data (text) is to be read
    from the RT-11 device.  The second mode is useful when binary data
    is to be read from the RT-11 device.  The deletion of trailing NUL
    characters  in  the  second mode reflects the RT-11 method of NUL-
    -filling the buffer before writing it, as file sizes in RT-11  are
    known in multiples of blocks.

    Wild card specifications may be given.  These consist of  a  UNIX-
    style  path name whose last element is an RT-11 wild card specifi-
    cation.  Files matching the RT-11 wild card specification are  ex-
    tracted into the directory named by the UNIX-style path name.  The
    resulting file name is the original specification, if it contained
    no wild card characters, or the leading parts  of  the  path  name
    followed  by  the  lowercase equivalent of the RT-11 file name, if
    the specification contained any wild card characters.   Thus,  the
    specification  /usr/foo/*.C  might  generate  /usr/foo/bar.c,  but
    /usr/foo/BAR.C would generate /usr/foo/BAR.C.

FORMAL PARAMETERS:

    File_count.rg.v - A count of the wild card descriptors.
    File_specs.rt.ra - Wild card descriptors.  If supplied, only files
	matching  one or more wild card descriptors are to be extract-
	ed.

IMPLICIT INPUTS:

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
    /*
     *  Will need to read the RT-11 device.
     */

    rtopen (ACC_READ);

    /*
     *  Scan  through  all  directory  segments,  extract  appropriate
     *  entries.
     */

    if (filc == 0) {
	char *tmp[2];
	tmp[0] = "*.*";
	tmp[2] = "";
	xfilc = 1;
	xfilv = tmp;
    } else {
	xfilc = filc;
	xfilv = filv;
    }

    rtscnd ((word) DE_PERM, procent, (int (*) ()) NULL);

}
