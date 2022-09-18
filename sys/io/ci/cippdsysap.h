/*
 @(#)cippdsysap.h	4.1  (ULTRIX)        7/2/90
 */

/************************************************************************
 *                                                                      *
 *                      Copyright (c) 1988 - 1989 by                    *
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
 ************************************************************************
 *
 *
 *   Facility:	Systems Communication Architecture
 *		Computer Interconnect Port-to-Port Driver
 *
 *   Abstract:	This module contains Computer Interconnect Port-to-Port
 *		Driver( CI PPD ) data structure definitions visible to SYSAPs.
 *
 *   Creator:	Todd M. Katz	Creation Date:	July 31, 1987
 *
 *   Modification History:
 *
 *   08-Jan-1989	Todd M. Katz		TMK0002
 *	Remove the bit map ptdlogmap from the CIPPDLPIB.
 *
 *   08-Jan-1988	Todd M. Katz		TMK0001
 *	Formated module, revised comments, increased robustness, made CI
 *	PPD and GVP completely independent from underlying port drivers, and
 *	added SMP support.
 */

/* CI PPD Constants.
 */
#define	CIPPD_MAPSIZE	( 224 / 8 )	/* Size remote port bit map in bytes */
					/*  ( 224 is max number of CI ports )*/
					/*  (   8 is max number of MSI ports)*/

/* CI PPD Data Structure Definitions.
 */
typedef struct _cippdlpib {		/* CI PPD Local Port Information     */
    u_char	npaths;			/* Current number of paths	     */
    u_char	nform_paths;		/* Current number of formative paths */
    u_char	max_port;		/* Hardware maximum port number	     */
    u_char	protocol;		/* CI PPD protocol version level     */
    u_char	dbclogmap[ CIPPD_MAPSIZE ];/* Database conflict port log map */
} CIPPDLPIB;
