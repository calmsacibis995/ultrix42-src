/*
 *            @(#)lmf.h	4.1     (ULTRIX)        7/2/90
 */

/************************************************************************
 *									*
 *                      Copyright (c) 1989 by                           *
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
 *									*
 ************************************************************************/

/* Definitions necessary to use lmf library. */

/************************************************************************
 *									*
 *			Modification history				*
 *									*
 *  19-Sep-89	Lisa Allgood
 *	Moved LMF_TOKEN and LMF_HARDWARE from lmfklic.h to here
 *  4-Jul-89	Giles Atkinson						*
 *	Add definition of allocation codes				*
 *  1-Jun-89	Giles Atkinson						*
 *	Add definition of flag argument to probe/test license		*
 *									*
 * Lisa Allgood - 4th May 1989						*
 *	Original version						*
 *									*
 ************************************************************************/

/*
 *	Structure to hold version number
 */


typedef struct version {
	short v_major;
	short v_minor;
} ver_t;

/* Flag values for probe/test license */

#define LMFF_MORE   (1<<16)		/* Always allocate another unit */

/* Values returned by lmf_license_info to indicate authorisation
 * without allocation of units.
 */

#define LMF_ACTIVITY -1			/* Product is activity licensed */
#define LMF_FAMILY   -2			/* Process is member of family */

/* Length of product token and hardware id fields for use with
 * the lmf_license_info() function.
 */

#define LMF_TOKEN	32
#define LMF_HARDWARE	32

int lmf_license_info();
int lmf_probe_license();
int lmf_release_license();
int lmf_test_license();
