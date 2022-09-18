#ifndef lint
static  char    *sccsid = "@(#)rtinit.c	4.1  (ULTRIX)        7/2/90";
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

    Initialize an RT-11 device.

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
#include <ctype.h>

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

static uword
    segtab [5] =
       {640, 1280, 2560, 5120, 10240};

/*
 * EXTERNAL REFERENCES:
 */

extern
    arcpad (),          /*  Copy and blank pad a string.            */
    arwcmt (),          /*  Notify of "wild card" match.            */
    exit (),            /*  Terminate program.                      */
    fprintf (),         /*  Print message to file.                  */
    rtidep (),          /*  Increment directory entry pointer.      */
    rtopen (),          /*  Open RT-11 device.                      */
    rtputb (),          /*  Write a block.                          */
    rtputs ();          /*  Write a directory segment.              */

extern char
    * strcpy ();        /*  Copy string.                            */

extern int
    arvfyn (),          /*  Verify action with user.                */
    strlen ();          /*  Find length of string.                  */

extern char
    * pgmnam;           /*  Program's name.                         */

extern int
    modflg;             /*  Action modifier flags.                  */

extern struct homblk
    homblk;             /*  Device home block.                      */

extern struct dirseg
    dirseg;             /*  First directory segment.                */

rtinit (filc, filv)

int
    filc;

char
    * * filv;

/*********************************************************************

FUNCTIONAL DESCRIPTION:

    Initializes an RT-11 volume.

FORMAL PARAMETERS:

    File_count.rg.v - A count of the number of arguments.
    File_vector.rt.ra - The arguments.

    The one and only element of File_vector is a comma separated  list
    of  options  to use when initializing the RT-11 volume.  The first
    (mandatory) item is a decimal digit string whose numeric value  is
    the  number  of blocks in the RT-11 volume.  The second (optional)
    item is a decimal digit string whose numeric value is  the  number
    of  directory  segments in the RT-11 volume.  The third (optional)
    item is a character string whose value  is  the  volume  identifi-
    cation.   The  fourth  (optional) item is a character string whose
    value is the volume owner.  The fifth (optional) item is a decimal
    digit  string  whose numeric value is the number of extra words in
    each directory entry.

    Items in the option list may be omitted by putting nothing between
    the delimiting commas or the delimiting comma and the end  of  the
    option string.  Trailing commas need not be specified.

IMPLICIT INPUTS:

    modflg - The action modifier flags.

IMPLICIT OUTPUTS:

    homblk - The RT-11 volume home block.
    dirseg - The directory segment buffer.

ROUTINE VALUE:

    None.

SIDE EFFECTS:

    Initializes the RT-11 volume.

    May exit with an error message.

*********************************************************************/

{
    static char
	msg1 [] = "%s: Illegal conversion, number of %s\n",
	msg2 [] = "%s: Truncated to 12 characters, %s\n",
	msg3 [] = "%s: Range error, number of %s\n";

    char
	inchar,         /*  Character from option string.           */
	* inptr,        /*  Pointer into option string.             */
	* outptr,       /*  Pointer into volume id/owner string.    */
	volid [13],     /*  Volume identification.                  */
	owner [13];     /*  Volume owner name.                      */

    long
	numblk;         /*  Number of blocks in RT-11 volume.       */

    uword
	numext,         /*  Number of extra words in dir segment.   */
	numseg;         /*  Number of directory segments.           */

    int
	i,              /*  General counter.                        */
	trunc;          /*  String truncation flag.                 */

    struct dirent
	* de_ptr;       /*  Directory entry pointer.                */

    /*
     *  Check arguments.
     */

    if (filc != 1)
    {
	fprintf
	   (stderr,
	    "%s: Wrong number of arguments for initialize\n",
	    pgmnam);
	exit (1);
    }

    /*
     *  Get values from argument.
     */

    inptr = * filv;
    arwcmt (0);

    /*
     *  Get number of blocks in RT-11 volume.
     */

    numblk = 0;
    while (('\0' != (inchar = * inptr ++)) && (',' != inchar))
    {
	if (! isdigit (inchar))
	{
	    fprintf (stderr, msg1, pgmnam, "blocks");
	    exit (1);
	}
	numblk *= 10;
	numblk += (inchar - '0');
    }

    /*
     *  Get number of directory segments.
     */

    if ('\0' == inchar)
    {
	numseg = 0;
    }
    else
    {
	numseg = 0;
	while (('\0' != (inchar = * inptr ++)) && (',' != inchar))
	{
	    if (! isdigit (inchar))
	    {
		fprintf (stderr, msg1, pgmnam, "directory segments");
		exit (1);
	    }
	    numseg *= 10;
	    numseg += (inchar - '0');
	}
    }

    /*
     *  Get volume identification.
     */

    if ('\0' == inchar)
    {
	volid [0] = '\0';
    }
    else
    {
	i = 12;
	trunc = 0;
	outptr = & volid [0];
	while (('\0' != (inchar = * inptr ++)) && (',' != inchar))
	{
	    if (i <= 0)
	    {
		trunc = 1;
	    }
	    else
	    {
		* outptr ++ = inchar;
	    }
	    i --;
	}
	* outptr = '\0';
	if (trunc)
	{
	    fprintf (stderr, msg2, pgmnam, "volume ID");
	}
    }

    /*
     *  Get volume owner.
     */

    if ('\0' == inchar)
    {
	owner [0] = '\0';
    }
    else
    {
	i = 12;
	trunc = 0;
	outptr = & owner [0];
	while (('\0' != (inchar = * inptr ++)) && (',' != inchar))
	{
	    if (i <= 0)
	    {
		trunc = 1;
	    }
	    else
	    {
		* outptr ++ = inchar;
	    }
	    i --;
	}
	* outptr = '\0';
	if (trunc)
	{
	    fprintf (stderr, msg2, pgmnam, "owner");
	}
    }

    /*
     *  Get number of extra words per directory segment.
     */

    if ('\0' == inchar)
    {
	numext = 0;
    }
    else
    {
	numext = 0;
	while ('\0' != (inchar = * inptr ++))
	{
	    if (! isdigit (inchar))
	    {
		fprintf (stderr, msg1, pgmnam, "extra words");
		exit (1);
	    }
	    numext *= 10;
	    numext += (inchar - '0');
	}
    }

    /*
     *  Validity check and apply defaults.
     */

    if ((numblk < 9l) || (65535l < numblk))
    {
	fprintf (stderr, msg3, pgmnam, "blocks");
	exit (1);
    }

    if (0 == numseg)
    {
	numseg = 31;
	for (i = 0; i < 5; i ++)
	{
	    if (numblk <= segtab [i])
	    {
		numseg = (uword) (1 << i);
		break;
	    }
	}
    }
    else
    {
	if (31 < numseg)
	{
	    fprintf (stderr, msg3, pgmnam, "directory segments");
	    exit (1);
	}
    }

    if (0 == strlen (volid))
    {
	strcpy (volid, "RT11A");
    }

    if (numext > 500)
    {
	fprintf (stderr, msg3, pgmnam, "extra words");
	exit (1);
    }

    /*
     *  Report parameters.
     */

    if (modflg & FLG_VERB)
    {
	printf ("Number of blocks in device:     %ld\n", numblk);
	printf ("Number of directory segments:   %u\n", numseg);
	printf ("Volume ID:                      %s\n", volid);
	printf ("Owner:                          %s\n", owner);
	printf ("Extra words in directory entry: %u\n\n", numext);
    }

    /*
     *  Inquire whether to really initialize.
     */

    if (! arvfyn ("ALL DATA ON VOLUME WILL BE DESTROYED\nReally initialize? "))
    {
	return;
    }

    /*
     *  Will need to write/create the RT-11 device.
     */

    rtopen (ACC_CREA);

    /*
     *  Rebuild home block.
     */

#ifdef DEBUG
    fprintf (stderr, "Rebuilding home block\n");
#endif
    for (outptr = (char *) (& homblk), i = 510; i > 0; i --)
    {
	* outptr ++ = '\0';
    }

    homblk.hb_clus = 1;         /*  Cluster size.                   */
    homblk.hb_dirs = DIRSEG1;   /*  First directory segment.        */
    homblk.hb_sysv = 0107351;   /*  System version: RAD 50 "V3A"    */
    arcpad (homblk.hb_voli, volid, 12); /*  Volume ID       */
    arcpad (homblk.hb_owne, owner, 12); /*  Owner name      */
    arcpad (homblk.hb_sysi, "DEC ULTRIX", 12);  /*  System ID       */

    trunc = 0;
    for (i = 0, inptr = (char *) (& homblk); i < 510; i ++)
    {
	trunc += (* inptr ++);
    }
    homblk.hb_chec = trunc;     /*  Checksum.                       */

#ifdef DEBUG
    fprintf (stderr, "Writing new home block\n");
#endif
    rtputb ((uword) HOMBLK, (char *) (& homblk));

    /*
     *  Rebuild directory segment 1.
     */

#ifdef DEBUG
    fprintf (stderr, "Rebuilding directory segment 1.\n");
#endif
    dirseg.ds_nseg = numseg;    /*  Number segments total.          */
    dirseg.ds_next = 0;         /*  Next segment number.            */
    dirseg.ds_hseg = 1;         /*  Highest segment used.           */
    dirseg.ds_xtra = numext;    /*  Extra words.                    */
    dirseg.ds_sblk =            /*  Start of data area.             */
	(uword) DIRSEG1 + (uword) 2 * numseg;

    de_ptr = (struct dirent *) (& dirseg.ds_dent [0]);

    de_ptr -> de_stat = DE_EMPT;        /*  Type: empty.            */
    de_ptr -> de_nblk =                 /*  Number of free blocks.  */
	numblk - dirseg.ds_sblk;

    rtidep (& dirseg, & de_ptr);

    de_ptr -> de_stat = DE_ENDS;        /*  Type: end segment.      */

#ifdef DEBUG
    fprintf (stderr, "Writing new directory segment 1\n");
#endif
    rtputs (1, (char *) (& dirseg));

    /*
     *  That is it.
     */
}

/* rtboot performs rtboot magic for a real rt-11 console storage device */
/* lp@decvax */
#include <sys/file.h>
char sradname[20];

rtboot( cnt, files)
int cnt;
char **files;
{
	char *monsrc[2];
	char *index();
	int mondes, bootdes;

/* The defaults */
/*        monsrc[0] = "rt11fb.sys";
	monsrc[1] = "dl.sys";   */

	if(cnt == 2) {
		monsrc[0] = *files++;
		monsrc[1] = *files;
	} else {
		fprintf(stderr, "Must specify monitor & handler files\n");
		exit (0);
	}

/*        } else if (cnt == 1)
		monsrc[1] = *files;     */

	if (modflg & FLG_EXTR) {
		rtextr(2, monsrc); /* extract does an open on rl02 */
		rtclos();          /* so close it */
		if (modflg & FLG_VERB)
			printf("Extracted %s & %s\n", monsrc[0], monsrc[1]);
	}

	strncpy(&sradname[0], monsrc[0], index(monsrc[0],'.') - monsrc[0]);

	if ((mondes = open(monsrc[0], O_RDONLY, 0)) < 0) {
		printf("error opening monitor file %s\n", monsrc[0]);
		exit (0);
	}
	if ((bootdes = open(monsrc[1], O_RDONLY, 0)) < 0) {
		printf("error opening primary boot file %s\n", monsrc[1]);
		exit (0);
	}

	if(!rwbootb(mondes, bootdes))
		printf("error reading monitor file or primary boot file\n");
	if (modflg & FLG_VERB)
		printf("Boot block update done\n");

	exit (1);
}

#define BLKSZ 512
int bldboot = 0;
rwbootb(monitor, handler)
int monitor, handler;
{
	char driver[BLKSZ], blockm[4][BLKSZ];
	struct bb {
		short tmp1[25];
		short of1;
		short tmp[230];
	} blockb;
	int offset = 0, cnt;

	if(read(handler, &blockb, BLKSZ) != BLKSZ)
		return (0);

	(void) lseek (monitor, 512L, 0);        /* Skip 1st block */

	for (cnt = 0; cnt < 5; cnt++)
	if(read(monitor, &blockm[cnt][0], BLKSZ) != BLKSZ)   /* Read next 4 */
		return (0);


	arar50(&sradname[0], &blockm[3][0724], strlen(&sradname[0]));
#ifdef lp
	printf("offset to primary driver = %d\n",  blockb.of1);
#endif
	offset = lseek (handler,(long) blockb.of1, 0);

	/* May only get a partial read here */
	if((cnt = read(handler, driver, BLKSZ)) != BLKSZ)
		cnt +=  read(handler, driver+cnt, BLKSIZ - cnt);

	bldboot++;
	rtopen(ACC_WRIT);
	rtputb(0, driver);

	rtputb ((uword) HOMBLK, (char *) (& homblk));

	for(cnt = 2; cnt < 6; cnt++)
	rtputb(cnt, &blockm[cnt - 2][0]);

	rtclos();

	return (1);
}

