#ifndef	lint
static char *sccsid = "@(#)sysap_start.c	4.1	(ULTRIX)	7/2/90";
#endif	lint

/************************************************************************
 *                                                                      *
 *                      Copyright (c) 1987 by                           *
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
 *
 *   Abstract:	This module contains all of the routine(s) to initialize
 *		and or start System Applications (SYSAPS).
 *
 *   scs_start_sysaps		Attach and/or start System Applications
 *
 *   Creator:	Larry Cohen	Creation Date:  March 13, 1987
 *
 *   History:
 *
 */
/**/


#include	"../h/types.h"
#include	"../io/sysap/sysap_start.h"


/*
 *   Name:	scs_start_sysaps - Attach and/or start System Applications
 *
 *   Abstract:	This subroutine scans a function array "sysaps" for sysaps to
 *		attach and/or start.  
 *
 *   Inputs:
 *
 *   sysaps			- function array of sysaps to start
 *
 *   Outputs:
 *
 *   none
 *
 */

void
scs_start_sysaps()
{
	register int i;
	extern Sysap_start sysaps[];
	
	for (i=0; sysaps[i] != SYSAP_LAST && i<MAX_SYSAPS; i++)
		if (sysaps[i]) 
			(*sysaps[i])();

}
