/*
@(#)arff.h	4.1  (ULTRIX)        7/2/90
*/

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

    Defines structures and constants.

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
 *  Bits for "modflg" global.
 */

#define FLG_HOMB        1       /*  Home block known corrupt.       */
#define FLG_NEWD        2       /*  Create 1 segment directory.     */
#define FLG_NOIN        4       /*  Do not use 4.2BSD mapping.      */
#define FLG_PRIN        8       /*  Printable file transfer.        */
#define FLG_VERB        16      /*  Be verbose.                     */
#define FLG_EXTR        32      /*  Extract boot file before boot update */

/*
 *  Volume access codes.
 */

#define ACC_READ        1       /*  Read only RT-11 volume.         */
#define ACC_WRIT        2       /*  Read and write RT-11 volume.    */
#define ACC_CREA        3       /*  Read and write RT-11 volume,    */
				/*   create if necessary.           */

/*
 *  RT-11 block information.
 */

#define BLKSIZ  512             /*  Size in bytes of a block.       */
#define DIRSEG1 6               /*  Location of first directory     */
				/*   segment.                       */
#define HOMBLK  1               /*  Location of device home block.  */

/*
 *  RT-11 system version.
 */

#define RTSYSV  0107123         /*  RT-11 version: V05 in RADIX 50  */

/*
 *  Directory entry status bits.
 */

#define DE_TENT 0000400         /*  Tentative file.                 */
#define DE_EMPT 0001000         /*  Empty area.                     */
#define DE_PERM 0002000         /*  Permanent file.                 */
#define DE_ENDS 0004000         /*  End of segment.                 */
#define DE_PROT 0100000         /*  Protected file.                 */

/*
 *  Utility symbols.
 */

#define CHARMASK        0xFF    /*  Character mask.                 */
#define MAXNAMLN        1023    /*  Maximum native file name length.*/

/*
 *  Utility data types.
 */

typedef short
    word;

typedef unsigned short
    uword,
    rad50;

/*
 *  Structure of the device home block.
 */

struct homblk
{
    uword       hb_bbrt [65];   /*  Bad block replacement table.    */
    char        hb_res1 [2];    /*  Reserved.                       */
    char        hb_init [38];   /*  Initialize/restore data area.   */
    char        hb_bupi [18];   /*  BUP information area.           */
    char        hb_res2 [278];  /*  Reserved.                       */
    word        hb_clus;        /*  Cluster size.                   */
    uword       hb_dirs;        /*  First directory segment.        */
    rad50       hb_sysv;        /*  System version.                 */
    char        hb_voli [12];   /*  Volume identification.          */
    char        hb_owne [12];   /*  Owner name.                     */
    char        hb_sysi [12];   /*  System identification.          */
    char        hb_res3 [2];    /*  Reserved.                       */
    word        hb_chec;        /*  Checksum.                       */
};

/*
 *  Structure of a directory segment.
 */

struct dirseg
{
    word        ds_nseg;        /*  Number of directory segments.   */
    uword       ds_next;        /*  Number of next directory seg-   */
				/*   ment.                          */
    word        ds_hseg;        /*  Number of highest directory     */
				/*   segment in use, 1 based - only */
				/*   valid in first directory       */
				/*   segment.                       */
    word        ds_xtra;        /*  Number of extra bytes in each   */
				/*   directory entry.               */
    uword       ds_sblk;        /*  Block number where storage      */
				/*   monitored by this directory    */
				/*   segment starts.                */
    word        ds_dent [507];  /*  Directory entry storage.        */
};

/*
 *  Structure of a directory entry.
 */

struct dirent
{
    word        de_stat;        /*  Status flags.                   */
    rad50       de_name [3];    /*  File name and extension.        */
    uword       de_nblk;        /*  Number of blocks.               */
    char        de_chan;        /*  Channel number on which open.   */
    char        de_jobn;        /*  Job number of owning job.       */
    word        de_date;        /*  Creation date.                  */
};
