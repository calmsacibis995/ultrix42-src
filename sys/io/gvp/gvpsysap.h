/*
 *	@(#)gvpsysap.h	4.1	(ULTRIX)	7/2/90
 */

/************************************************************************
 *									*
 *			Copyright (c) 1988 by				*
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
 *		Generic Vaxport Port Driver
 *
 *   Abstract:	This module contains Generic Vaxport Port Driver( GVP )
 *		data structure definitions visible to SYSAPs.
 *
 *   Creator:	Todd M. Katz	Creation Date:	November 20, 1985
 *
 *   Modification History:
 *
 *   19-Sep-1989 	Pete Keilty
 *	Added ovhd_pd to local port info. block (gvplpib.ovhd_pd).
 *
 *   02-Jun-1988	Ricky S. Palmer
 *	Removed struct entries for msi altogether.
 *
 *   29-Jan-1988        Ricky S. Palmer
 *      Added struct entries for msi in both the Local Port Information Block
 *      and the Path Information Block definitions.
 *
 *   08-Jan-1988	Todd M. Katz
 *	Formated module, revised comments, increased robustness, made GVP
 *	completely independent from underlying port drivers, restructured code
 *	paths, and added SMP support.
 */

/* Generic Vaxport Data Structure Definitions.
 */
typedef struct _gvpbname	{	/* Generic Vaxport Buffer Name	     */
    u_short	index;			/* Index into buffer descriptor array*/
#define ni_chain	0x00008000	/*  NI buffer chaining mask	     */
    u_short	key;			/* Key within buffer descriptor      */
} GVPBNAME;

typedef struct	_gvpbhandle {		/* Generic Vaxport Buffer Handle     */
    u_long	     boff;		/* Transfer offset of buffer	     */
    struct _gvpbname bname;		/* Generic Vaxport buffer name	     */
} GVPBHANDLE;

typedef struct _gvplpib {		/* Generic Vaxport Local Port	     */
					/*  Information			     */
    u_long	dg_size;		/* Size of application datagram      */
    u_long	msg_size;		/* Size of application message	     */
    u_short	ovhd_pd;		/* Size of PD overhead 		     */
    u_short	ovhd;			/* Size of PD + PPD overhead	     */
    union		    {		/* Implementation dependent fields   */
	struct _bvp_ssplpib bvp;	/*  BVP SSP specific information     */
	struct _cilpib	    ci;		/*  CI specific information	     */
    } type;
} GVPLPIB;

typedef struct _gvppib	{		/* Generic Vaxport Path Information  */
    union		{		/* Implementation dependent fields   */
	struct _cipib	ci;		/*  CI path information		     */
    } type;
} GVPPIB;
