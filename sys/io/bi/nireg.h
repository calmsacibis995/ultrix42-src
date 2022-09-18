/*	@(#)nireg.h	4.1	(ULTRIX)	7/2/90	*/
/************************************************************************
 *									*
 *			Copyright (c) 1986, 87 by			*
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

#include "../io/bi/bvp_ni.h"
struct nidevice {
	unsigned long 	pc;
	unsigned long 	ps;
	unsigned long	pe;
	unsigned long	pd;
};
struct ni {
	GVPPQB *ni_pqb;
	struct nidevice *ni_regs;
	int unit;
	int alive;
	unsigned int phys_pqb;
	char *ui;
	char *mbuf_clusters[NI_NRECV];
};

#define MULTISIZE	6

/* Flags for ds_flags field of ni_softc */

#define NI_INIT		1
