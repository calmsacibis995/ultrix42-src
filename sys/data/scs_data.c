#ifndef	lint
static char *sccsid = "@(#)scs_data.c	4.1	(ULTRIX)	7/2/90";
#endif	lint

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
 *		Systems Communication Services
 *
 *   Abstract:	This module contains Systems Communication Services( SCS )
 *		configurable variables and data structures.
 *
 *   Creator:	Todd M. Katz	Creation Date:	May 6, 1985
 *
 *   Modification History:
 *
 *   18-Sep-1989	Pete Keilty
 *	Added SCS errlogging varaible scs_errlog.
 *
 *   10-Feb-1989	Todd M. Katz		TMK0002
 *	Added SCS configuration variable scs_severity.
 *
 *   08-Jan-1988	Todd M. Katz		TMK0001
 *	Formated module, and revised comments.
 */

/* Libraries and Include Files.
 */
#include		"../h/types.h"
#include		"../io/scs/sca.h"
#include		"../io/scs/scaparam.h"
#include		"scs_data.h"
#include		"bvpssp.h"
#include		"dssc.h"
#include		"hsc.h"
#include		"scsnet.h"
#include		"uq.h"

/* SCS Configuration Variables.
 */
u_long		scs_cushion		/* SCS send credit cushion	     */
		    = 1;		/*  MAX:    16;	DEF:      1; MIN:  0 */
u_long		scs_dg_size		/* Maximum application datagram size */
		    = 256;		/*  MAX:   984; DEF:    567; MIN: 28 */
u_long		scs_max_conns		/* Maximum number of connections     */
		    = SCS_MAX_CONNS;	/*  MAX: 32767; DEF:     68; MIN:  2 */
u_long		scs_msg_size		/* Maximum appl sequenced msg size   */
		    = 256;		/*  MAX:   984; DEF:    112; MIN: 52 */
u_char		scs_node_name[ 8 ]	/* SCS node name( blank filled )     */
		    = SCS_NODE_NAME;
u_char		scs_sanity		/* SCS sanity timer interval( secs ) */
		    = 2;		/*  MAX:    60; DEF:      1; MIN:  1 */
u_short		scs_severity		/* SCS console logging severity	     */
		    = SCA_SEVERITY;	/*  MAX:     5; DEF:	  0; MIN:  0 */
scaaddr		scs_system_id		/* SCS system identification number  */
		    = SCS_SYSID;
u_short		scs_errlog		/* SCA system errlog severity level  */
		    = SCA_ERRLOG2;	/* MAX:	     3; DEF:      2; MIN:  0 */
