#ifndef lint
static        char    *sccsid = "@(#)msi_data.c	4.1  (ULTRIX)        7/2/90";
#endif	lint

/************************************************************************
 *									*
 *			Copyright (c) 1989                              *
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
 ************************************************************************
 *
 *
 *   Facility:	Systems Communication Architecture
 *		Mayfair Storage Interconnect Port Driver
 *
 *   Abstract:	This module contains Mayfair Storage Interconnect Port
 *		Driver( MSI ) configurable variables.
 *
 *   Creator:	Todd M. Katz	Creation Date:	December 07, 1988
 *
 *   Modification History:
 */

/* Libraries and Include Files
 */
#include		"../h/types.h"
#include		"../io/scs/scaparam.h"

/* MSI Configuration Variables
 */
u_short		msi_cippdburst		/* CI PPD port polling burst size    */
		    = 8;		/*  MAX:    8; DEF:	  8; MIN:  1 */
u_short		msi_cippdcontct		/* CI PPD port contact intrvl( secs )*/
		    = 5;		/*  MAX: 32767; DEF:      5; MIN:  2 */
u_short		msi_lpc_panic		/* MSI local port crash panic flag   */
		    = SCA_PANIC;	/*  MAX:     3; DEF:      0; MIN:  0 */
u_short		msi_severity		/* MSI console logging severity	     */
		    = SCA_SEVERITY;	/*  MAX:     5; DEF:	  4; MIN:  0 */
