#ifndef	lint
static        char    *sccsid = "@(#)gvp_data.c	4.1  (ULTRIX)        7/2/90";
#endif	lint

/************************************************************************
 *                                                                      *
 *                      Copyright (c) 1988 by                           *
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
 *		Generic Vaxport Port Driver
 *
 *   Abstract:	This module contains  Generic Vaxport Port Driver( GVP )
 *		configurable variables.
 *
 *   Creator:	Todd M. Katz	Creation Date:	September 22, 1987
 *
 *   Modification History:
 *
 *   08-Jan-1988	Todd M. Katz
 *	Formated module, revised comments, and made GVP completely
 *	independent from underlying port drivers.
 */

/* Libraries and Include Files.
 */
#include		"../h/types.h"
#include		"../io/scs/sca.h"
#include		"../io/scs/scaparam.h"
#include		"bvpssp.h"
#include		"dssc.h"
#include		"hsc.h"
#include		"scsnet.h"

/* Generic Vaxport Driver Configuration Variables.
 */
u_long		gvp_queue_retry		/* Queuing failure retry account     */
		    = 10000;		/*  MAX:    -1; DEF: 10000;  MIN:  1 */
u_long		gvp_max_bds		/* Maximum number Buffer Descriptors */
		    = GVP_MAX_BDS;	/*  MAX: 32767; DEF:     50; MIN:  0 */
