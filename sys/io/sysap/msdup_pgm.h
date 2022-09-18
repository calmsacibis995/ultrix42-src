/*	@(#)msdup_pgm.h	4.1	(ULTRIX)	2/19/91	*/

/************************************************************************
 *									*
 *			Copyright (c) 1989 by				*
 *		Digital Equipment Corporation, Maynard, MA		*
 *			All rights reserved.				*
 *									*
 *   This software is furnished under a license and may be used and	*
 *   copied  only  in accordance with the terms of such license and	*
 *   with the  inclusion  of  the  above  copyright  notice.   This	*
 *   software  or  any  other copies thereof may not be provided or	*
 *   otherwise made available to any other person.  No title to and	*
 *   ownership of the software is hereby transferred.			*
 *									*
 *   The information in this software is subject to change  without	*
 *   notice  and should not be construed as a commitment by Digital	*
 *   Equipment Corporation.						*
 *									*
 *   Digital assumes no responsibility for the use  or  reliability	*
 *   of its software on equipment which is not supplied by Digital.	*
 *									*
 ************************************************************************/

/*
 *
 *   Facility:	Systems Communication Architecture
 *		Diagnostic/Utilities Protocol (DUP) Class Driver
 *
 *   Abstract:	This module contains DUP program file/header definitions.
 *
 *   Author:	Andrew J. Moskal	Creation Date:	dd-mmn-yyyy
 *
 *   Modification History:
 *
 */

/*
 *   Conditional check to include this header file only once.
 */
#ifndef MSDUP_pgm.h
#define MSDUP_pgm.h	0		/* Disable subsequent includes       */

/*
 *  Local constants
 */
#define	PGM_NAME_SIZE	6

/*
 *  DUP program flag definitions
 */
typedef struct _msdup_pgmflg {
    u_char	standalone	:  1;	/*   Standalone                      */
    u_char	overlay		:  1;	/*   Overlay segment                 */
    u_char	overlaywrt	:  1;	/*   Read/write overlay segment      */
    u_char	standard	:  1;	/*   Standard Dialogue               */
    u_char	nocrlf		:  1;	/*   Pass-thru                       */
				    /*    (Don't append CR/LF)           */
    u_char			:  3;	/*   Reserved                        */
} MSDUP_PGMFLG;				/* DUP program control flags         */

/*
 *  DUP program file header (U53 format) definitions
 */
typedef struct _msdup_pgmhdr {
    u_long		ini_size;	/*  Header/Initial segement size     */
    u_long		ovr_size;	/*  Overlay segment size             */
    u_char		name[ PGM_NAME_SIZE ];
					/*  Program name (blank filled)      */
    u_short		version;	/*  Program version number           */
    MSDUP_PGMFLG	flags;		/*  Program control flags            */
    u_char		xfer_tmo;	/*  Timeout value for data transfers */
} MSDUP_PGMHDR;				/* DUP program file header format    */

/*
 *  DUP program file (U53 format) definitions
 */
typedef struct _msdup_pgm {
    MSDUP_PGMHDR	hdr;		/*  DUP program file header          */
    u_char		body[ 1 ];	/*  DUP program body                 */
} MSDUP_PGM;				/* DUP program file format           */

#endif	MSDUP_pgm.h
