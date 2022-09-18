#ifndef	lint
static char *sccsid = "@(#)vbavar.c	4.3	(ULTRIX)	12/20/90";
#endif	lint

/************************************************************************
 *                                                                      *
 *                      Copyright (c) 1990 by  		                *
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
 */

/* Libraries and Include Files.
 */
#include		"../h/types.h"
#include		"../h/param.h"
#include		"../h/time.h"
#include		"../h/ksched.h"
#include		"../h/errlog.h"
#include 		"../io/xmi/xmireg.h"
#include 		"../io/xmi/xbireg.h"
#include 		"../io/vme/xviareg.h"
#include 		"../io/vme/xvibreg.h"
#include 		"../io/vme/vbareg.h"
#include 		"../io/vme/vbavar.h"

struct	vba_hd	*head_vba = 0;

XVIAREGPTRS	xvia_regoff = {		/* 3MAX / Mipsfair II adapters	*/

		(u_int *)CSR_XVIA_OFF,
		(u_int *)VFADR_XVIA_OFF,
		(u_int *)CFADR_XVIA_OFF,
		(u_int *)IOR_XVIA_OFF,	
		(u_char *)BESR_XVIA_OFF,
		(u_char *)ICR_XVIA_OFF,
		(u_char *)ERRGI_XVIA_OFF,
		(u_char *)LVB_XVIA_OFF,
		(u_char *)ERR_XVIA_OFF,
		(u_int *)VIACSR_XVIA_OFF,
		(u_int *)VIACLR_XVIA_OFF,
		(u_short *)IVS_XVIA_OFF,	
		(u_char *)ARCR_XVIA_OFF,	
		(u_char *)TTR_XVIA_OFF,	
		(u_char *)RCR_XVIA_OFF,	
		(u_char *)LICR_XVIA_OFF,	
		(u_char *)LBTR_XVIA_OFF,	
		(u_char *)ICFR_XVIA_OFF,	
		(u_char *)AMSR_XVIA_OFF,
		(u_char *)S0C0_XVIA_OFF,
		(u_char *)S0C1_XVIA_OFF,
		(u_char *)S1C0_XVIA_OFF,
		(u_char *)S1C1_XVIA_OFF,	
		(u_int (*)[])PIOPMR_XVIA_OFF,
		(u_int (*)[])DMAPMR_XVIA_OFF
};

XVIBREGPTRS	xbia_xvib_regoff = {	/* XBIA+ adapter (XMI)	*/

		(u_int *)VDCR_XBIA_OFF,
		(u_int *)VESR_XBIA_OFF,
		(u_int *)VFADR_XBIA_OFF,
		(u_int *)VICR_XBIA_OFF,
		(u_int *)VVOR_XBIA_OFF,
		(u_int *)VEVR_XBIA_OFF,
		(u_int *)VBSR_XBIA_OFF,
		(u_int *)VCAR_XBIA_OFF,
		(u_int (*)[])DMAPMR_XBIA_OFF
};







