#ifndef lint
static  char    *sccsid = "@(#)rtpio.c	4.1  (ULTRIX)        7/2/90";
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

    Physical I/O on RT-11 device.

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
#include <sys/types.h>
#include <stdio.h>

/*
 * TABLE OF CONTENTS:
 */

/*
 *  secseek - Seek with sector interleaving.
 *  rtgetb - Get block from RT-11 volume.
 *  rtputb - Put block to RT-11 volume.
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

/*
extern
    exit (),              Terminate program.
    perror ();            Print error message.
*/

extern int
    read (),            /*  Read from a file.                       */
    write ();           /*  Write to a file.                        */

extern long
    lseek ();           /*  Seek to a spot in a file.               */

extern char
    * pgmnam,           /*  Program's name.                         */
    * rtdevn;           /*  RT-11 device image file name.           */

extern int
    modflg,             /*  Action modifier flags.                  */
    rtdev;              /*  RT-11 device image file descriptor.     */

static secseek (device, sector)

int
    device;

uword
    sector;

/*********************************************************************

FUNCTIONAL DESCRIPTION:

    Performs sector interleaving/mapping  for  ULTRIX-32  /dev/console
    RX01 devices.

FORMAL PARAMETERS:

    Device.rg.v - The file descriptor for the RX01 device.
    Sector.ruw.v - The sector number to be sought.

IMPLICIT INPUTS:

    None.

IMPLICIT OUTPUTS:

    None.

ROUTINE VALUE:

    None.

SIDE EFFECTS:

    Changes position in the RT-11 volume.

    May exit with an error code.

*********************************************************************/

{
    uword
	pcyl,           /*  Physical cylinder number.               */
	psec;           /*  Physical sector number.                 */

    off_t
	bytoff;         /*  Physical byte offset.                   */

    /*
     *  Compute the uncorrected physical sector and cylinder.
     */

    pcyl = sector / 26;
    psec = sector % 26;         /*  0, 1, 2, ..., 25                */

    /*
     *  Correct for 2:1 interleave.
     */

    psec <<= 1;                 /*  0, 2, 4, ..., 50                */
    if (psec >= 26)
    {
	psec -= 25;             /*  0, ..., 24, 1, ..., 25          */
    }

    /*
     *  Correct for 6 sector/cylinder skew.
     */

    psec += 6 * pcyl;
    psec %= 26;                 /*  0, ..., 25                      */

    /*
     *  Correct for start at cylinder 1, not 0.
     */

    pcyl ++;

    /*
     *  Correct for wrap around to cylinder 0.
     */

    if (77 == pcyl)
    {
	pcyl = 0;
    }

    /*
     *  Compute byte offset into device.
     */

    bytoff = (off_t) (26 * pcyl + psec) * (off_t) 128;

    /*
     *  Properly seek.
     */

    if (-1 == lseek (device, bytoff, 0))
    {
	fprintf (stderr, "%s: ", pgmnam);
	perror (rtdevn);
	exit (1);
    }
}

rtgetb (blknum, blkbuf)

uword
    blknum;

char
    * blkbuf;

/*********************************************************************

FUNCTIONAL DESCRIPTION:

    Reads a block from an RT-11 device.

    The blocks are numbered starting at zero.

FORMAL PARAMETERS:

    Block_number.ruw.v - The block number to be read.
    Block_buffer.wb.ra - The 512-byte area into which the block is  to
	be read.

IMPLICIT INPUTS:

    modflg - The action modification flags.
    rtdev - The file descriptor for  the  file  containing  the  RT-11
	device image.
    rtdevn - The file name for the RT-11 device image.

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
	* bufptr;       /*  Short read buffer pointer.              */

    int
	bytnum;         /*  Short read byte number.                 */

    uword
	lsec,           /*  Logical sector number.                  */
	lsec0,          /*  First logical sector number.            */
	lsec4;          /*  Last logical sector number + 1.         */

    off_t
	bytoff;         /*  Offset into file.                       */

#ifdef DEBUG
    fprintf (stderr, "getblk:  getting block %d\n", blknum);
#endif

    if (modflg & FLG_NOIN)
    {
	/*
	 *  Do not interleave.
	 */

	/*
	 *  Get byte offset.
	 */

	bytoff = (off_t) blknum * (off_t) BLKSIZ;

	/*
	 *  Read the block.
	 */

	if (-1 == lseek (rtdev, bytoff, 0))
	{
	    fprintf (stderr, "%s: ", pgmnam);
	    perror (rtdevn);
	    exit (1);
	}
	bytnum = read (rtdev, blkbuf, BLKSIZ);
	if (-1 == bytnum)
	{
	    fprintf (stderr, "%s: ", pgmnam);
	    perror (rtdevn);
	    exit (1);
	}
	if (bytnum < BLKSIZ)
	{
	    /*
	     *  Fill out short reads with zero.
	     */

	    bufptr = blkbuf + bytnum;
	    while (bytnum < BLKSIZ)
	    {
		* bufptr ++ = '\0';
		bytnum ++;
	    }
	}
    }
    else
    {
	/*
	 *  Interleave.  ULTRIX-32 and 4.2bsd do not use the  standard
	 *  sector interleaving method for the VAX-11/780 RX01 console
	 *  diskette.  So, it has to be done here.  The real RX driver
	 *  allows  interleaving,  so double density interleaving does
	 *  not need to be done here.
	 */

	lsec0 = (BLKSIZ / 128) * blknum;
	lsec4 = lsec0 + (BLKSIZ / 128);
	for (lsec = lsec0; lsec < lsec4; lsec ++, blkbuf += 128)
	{
	    secseek (rtdev, lsec);

	    bytnum = read (rtdev, blkbuf, 128);
	    if (-1 == bytnum)
	    {
		fprintf (stderr, "%s: ", pgmnam);
		perror (rtdevn);
		exit (1);
	    }
	    if (bytnum < 128)
	    {
		/*
		 *  Fill out short reads with zero.
		 */

		bufptr = blkbuf + bytnum;
		while (bytnum < 128)
		{
		    * bufptr ++ = '\0';
		    bytnum ++;
		}
	    }
	}
    }

    /*
     *  All done.
     */
}

rtputb (blknum, blkbuf)

uword
    blknum;

char
    * blkbuf;

/*********************************************************************

FUNCTIONAL DESCRIPTION:

    Writes a block to an RT-11 device.

    The blocks are numbered starting at zero.

FORMAL PARAMETERS:

    Block_number.ruw.v - The block number to be written.
    Block_buffer.wb.ra - The 512-byte area from which the block is  to
	be written.

IMPLICIT INPUTS:

    modflg - The action modification flags.
    rtdev - The file descriptor for  the  file  containing  the  RT-11
	device image.
    rtdevn - The file name for the RT-11 device image.

IMPLICIT OUTPUTS:

    None.

ROUTINE VALUE:

    None.

SIDE EFFECTS:

    Changes the current position in the RT-11 device image.

    May exit with an error message.

*********************************************************************/

{
    uword
	lsec,           /*  Logical sector number                   */
	lsec0,          /*  First logical sector number.            */
	lsec4;          /*  Last logical sector number + 1.         */

    off_t
	bytoff;         /*  Byte offset into file.                  */

#ifdef DEBUG
    fprintf (stderr, "rtputb:  putting block %d\n", blknum);
#endif

    if (modflg & FLG_NOIN)
    {
	/*
	 *  Do not interleave.
	 */

	/*
	 *  Get byte offset.
	 */

	bytoff = (off_t) blknum * (off_t) BLKSIZ;

	/*
	 *  Write the block.
	 */

	if (-1 == lseek (rtdev, bytoff, 0))
	{
	    fprintf (stderr, "%s: ", pgmnam);
	    perror (rtdevn);
	    exit (1);
	}
	if (-1 == write (rtdev, blkbuf, BLKSIZ))
	{
	    fprintf (stderr, "%s: ", pgmnam);
	    perror (rtdevn);
	    exit (1);
	}
    }
    else
    {
	/*
	 *  Interleave.  ULTRIX-32 and 4.2bsd do not use the  standard
	 *  sector interleaving method for the VAX-11/780 RX01 console
	 *  diskette.  So, it has to be done here.  The real RX driver
	 *  allows  interleaving,  so double density interleaving does
	 *  not need to be done here.
	 */

	lsec0 = (BLKSIZ / 128) * blknum;
	lsec4 = lsec0 + (BLKSIZ / 128);
	for (lsec = lsec0; lsec < lsec4; lsec ++, blkbuf += 128)
	{
	    secseek (rtdev, lsec);

	    if (-1 == write (rtdev, blkbuf, 128))
	    {
		fprintf (stderr, "%s: ", pgmnam);
		perror (rtdevn);
		exit (1);
	    }
	}
    }

    /*
     *  All done.
     */
}
