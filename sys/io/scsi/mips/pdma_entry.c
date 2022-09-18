#ifndef lint
static char *sccsid = "@(#)pdma_entry.c	4.2      (ULTRIX)  11/15/90";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1988 by				*
 *		Digital Equipment Corporation, Maynard, MA		*
 *			All rights reserved.				*
 *									*
 *   This software is furnished under a license and may be used and	*
 *   copied  only  in accordance with the terms of such license and	*
 *   with the  inclusion  of  the  above  copyright  notice.   This	*
 *   software  or  any	other copies thereof may not be provided or	*
 *   otherwise made available to any other person.  No title to and	*
 *   ownership of the software is hereby transferred.			*
 *									*
 *   The information in this software is subject to change  without	*
 *   notice  and should not be construed as a commitment by Digital	*
 *   Equipment Corporation.						*
 *									*
 *   Digital assumes no responsibility for the use  or	reliability	*
 *   of its software on equipment which is not supplied by Digital.	*
 *									*
 ************************************************************************/

/************************************************************************
 *
 * pdma_entry.c	11/10/89
 *
 * Pseudo DMA entry array for the PDMA control code.
 *
 * Modification history:
 *
 * 09/07/90     Maria Vella
 * Submitted this file to release.
 *
 * 07/25/90     Janet L. Schank
 * Removed DS3100 support.
 *
 * 10/12/89	John A. Gallant
 * Created this file to support the PDMA work in the scsi drivers.
 *
 * 11/10/89	John A. Gallant
 * Added the DS5000 entry.
 *
 ************************************************************************/

/* 
This file contains the Pseudo DMA entry points for all the PDMA systems.
Each entry has to be defined in their particular file. All new entries
have to be added to this array.
NOTE: 
    This array MUST be NULL terminated.  The routines the scan this array
     will stop scanning when a NULL pointer is encountered.
*/

#include "../data/scsi_data.c" 

/* External functions and variables. */

extern PDMA_ENTRY pdma_ds5000;		/* the entry for DS5000 */
extern PDMA_ENTRY pdma_ds5500;		/* the entry for DS5000 */
extern PDMA_ENTRY pdma_ds5000_100;		/* entry for 3min */

/************************************************************************/

PDMA_ENTRY *pdma_entry[] =
{

#ifdef DS5000
    &pdma_ds5000,
#endif
#ifdef DS5000
    &pdma_ds5500,
#endif
#ifdef DS5000_100
    &pdma_ds5000_100,
#endif

    ( 0 )			/* MUST: null termination */
};

/************************************************************************/
