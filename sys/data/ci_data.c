#ifndef	lint
static char *sccsid = "@(#)ci_data.c	4.1	(ULTRIX)	7/2/90";
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
 *		Computer Interconnect Port Driver
 *
 *   Abstract:	This module contains Computer Interconnect Port Driver( CI )
 *		configurable variables and variables required by installation
 *		procedures.
 *
 *   Creator:	Todd M. Katz	Creation Date:	February 25, 1985
 *
 *   Modification History:
 *
 *   18-Sep-1989 	Pete Keilty
 *	Add errlogging variable ci_errlog.
 * 
 *   30-Aug-1988	Todd M. Katz
 *	Add two former CI PPD configuration variables ci_cippdburst and
 *	ci_cippdcontact.  They are now specified by the CI port driver instead
 *	of by the CI PPD because the optimum values for port polling burst size
 *	and contact frequency now differ on an individual port driver basis.
 *	The values for these parameters are passed to the CI PPD through CI PPD
 *	specific PCCB fields.
 *
 *   13-Apr-1988	Todd M. Katz
 *	Add the configuration variable ci_lpc_panic.  This configuration
 *	variable controls whether CI local port failures are recovered from or
 *	immediately panic the system.  Error recovery is the default but may be
 *	overruled by setting the configuration file option SCA_PANIC to an
 *	appropriate value.
 *
 *   08-Jan-1988	Todd M. Katz
 *	Formated module, revised comments, and made CI PPD completely
 *	independent from underlying port drivers.
 */

/* Libraries and Include Files.
 */
#include		"../h/types.h"
#include		"../io/scs/scaparam.h"

/* CI Configuration Variables.
 */
u_short		ci_cippdburst		/* CI PPD polling burst size	     */
		    = 4;		/*  MAX:   224; DEF:	  4; MIN:  1 */
u_short		ci_cippdcontact		/* CI PPD port contact intrvl( secs )*/
		    = 60;		/*  MAX: 32767; DEF:     60; MIN:  2 */
u_short		ci_max_reinits		/* CI max num consecutive reinits    */
		    = 10;		/*  MAX:   255; DEF:     10; MIN:  1 */
u_short		ci_maint_timer		/* CI port mnt timer enable bool flag*/
		    = 1;		/*  MAX:     1; DEF:      1; MIN:  0 */
u_short		ci_maint_intrvl		/* CI port maintenance timer interval*/
		    = 0;		/*  MAX:   100; DEF:      0; MIN:  0 */
u_short		ci_lpc_panic		/* CI local port crash panic flag    */
		    = SCA_PANIC;	/*  MAX:     3; DEF:      0; MIN:  0 */
u_short		ci_severity		/* CI console logging severity	     */
		    = SCA_SEVERITY;	/*  MAX:     5; DEF:	  0; MIN:  0 */
u_short		ci_errlog		/* CI errlog logging severity	     */
		    = SCA_ERRLOG0;	/*  MAX:     3; DEF:	  0; MIN:  0 */

/* CI Variables for Installation Procedures.
 */
u_short		ci_first_port = 0;	/* Port number of first local CI port*/
