#ifndef lint
static        char    *sccsid = "@(#)bvp_init.c	4.1  (ULTRIX)        7/2/90";
#endif lint
/************************************************************************
 *									*
 *			Copyright (c) 1988 - 1989 by			*
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
 *	Modification History
 *
 *	20-Jun-1989	Ali Rafieymehr
 *		Removed pcpu as function argument
 *
 *	05-Mar-1989	Todd M. Katz		TMK0002
 *		Use the ../machine link to refer to machine specific header
 *		files.
 *
 *	21-Jan-1988	Todd M. Katz		TMK0001
 *		Remove the superfluous invocation of scs_initialize().  It
 *		is invoked within the bvp probe routine.
 *
 *	21-Dec-87	map
 *		Changed return to return value from probe.
 *
 */

#include "../h/types.h"
#include "../h/buf.h"
#include "../h/param.h"
#include "../h/vmmac.h"
#include "../machine/scb.h"
#include "../machine/pte.h"
#include "../io/uba/ubavar.h"

#include "../io/bi/bireg.h"
#include "../io/bi/bdareg.h"

extern	int	numuba;
extern	int	nbicpus;
extern	int	nbitypes;
extern	int	scs_initialize();
extern	struct	bidata bidata[];

int	bvpsspinit();

/*
 *
 *
 *	Name:		bvpsspinit
 *	
 *	Abstract:	Init ssp device
 *			
 *	Inputs:
 *
 *	
 *	
 *	
 *
 *	Outputs:
 *
 *	
 *	
 *	
 *
 *	Return 
 *	Values:
 *
 *	
 *	
 * 
 *	Side		
 *	Effects:
 *
 */
bvpsspinit(nxv, nxp, binumber, binode, um)
struct biic_regs	*nxv;
caddr_t			*nxp;
int 			binumber;
int 			binode;
struct uba_ctlr		*um;

{
int			i;

	(void)bicon_vec( binumber, binode, LEVEL14, um);
	return(bvp_probe( nxv, binumber, binode, um ));
}



