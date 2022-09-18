
/*	7/2/90 (ULTRIX-32) @(#)bdareg.h	4.1	*/	

/************************************************************************
 *									*
 *			Copyright (c) 1985 by				*
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


/* ------------------------------------------------------------------------
 * Modification History: /sys/vaxbi/bdareg.h
 *
 * 04-feb-86 -- jaw  get rid of biic.h.
 *
 * 19-Jun-85 -- jaw
 *	VAX8200 name change.
 *
 *	20 Mar 85 -- jaw
 *		add support for VAX 8200.
 *
 * ------------------------------------------------------------------------
 */

struct bda_regs
{
	struct 	biic_regs bda_biic;	/* BIIC specific registers */
};

